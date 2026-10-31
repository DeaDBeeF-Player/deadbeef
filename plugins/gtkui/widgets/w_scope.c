/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/



#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <deadbeef/fastftoi.h>
#include "../../../shared/scope/scope.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_scope.h"

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

typedef enum {
    SCOPE_SCALE_AUTO,
    SCOPE_SCALE_1X,
    SCOPE_SCALE_2X,
    SCOPE_SCALE_3X,
    SCOPE_SCALE_4X,
} scope_scale_t;

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
    GtkWidget *drawarea;

    guint drawtimer;

    intptr_t mutex;

    scope_scale_t scale;

    gboolean is_listening;
    ddb_scope_t scope;
    ddb_scope_draw_data_t draw_data;

    uint32_t draw_color;
    uint32_t background_color;

    gboolean use_custom_color;
    gboolean use_custom_background_color;
    // custom colors are remembered while the "use custom" option is off, if they were ever chosen
    gboolean custom_color_set;
    gboolean custom_background_color_set;
    GdkColor custom_color;
    GdkColor custom_background_color;

    cairo_surface_t *surf;

    gboolean updating_menu; // suppress menu event handlers
    GtkWidget *menu;

    GtkWidget *mode_multichannel_item;
    GtkWidget *mode_mono_item;

    GtkWidget *scale_auto_item;
    GtkWidget *scale_1x_item;
    GtkWidget *scale_2x_item;
    GtkWidget *scale_3x_item;
    GtkWidget *scale_4x_item;

    GtkWidget *fragment_duration_50ms_item;
    GtkWidget *fragment_duration_100ms_item;
    GtkWidget *fragment_duration_200ms_item;
    GtkWidget *fragment_duration_300ms_item;
    GtkWidget *fragment_duration_500ms_item;

    GtkWidget *preferences_item;
} w_scope_t;

///// scope vis
void
w_scope_destroy (ddb_gtkui_widget_t *w) {
    w_scope_t *s = (w_scope_t *)w;
    deadbeef->vis_waveform_unlisten (w);
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (s->surf) {
        cairo_surface_destroy (s->surf);
        s->surf = NULL;
    }

    ddb_scope_dealloc (&s->scope);
    ddb_scope_draw_data_dealloc (&s->draw_data);

    if (s->mutex) {
        deadbeef->mutex_free (s->mutex);
        s->mutex = 0;
    }
}

gboolean
w_scope_draw_cb (void *data) {
    w_scope_t *s = data;
    gtk_widget_queue_draw (s->drawarea);
    return TRUE;
}

static void
scope_wavedata_listener (void *ctx, const ddb_audio_data_t *data) {
    w_scope_t *w = ctx;

    deadbeef->mutex_lock (w->mutex);
    ddb_scope_process (&w->scope, data->fmt->samplerate, data->fmt->channels, data->data, data->nframes);
    deadbeef->mutex_unlock (w->mutex);
}

static inline uint32_t
_alpha_blend (uint32_t color, uint32_t background_color, float alpha) {
    uint32_t sr = color & 0xff;
    uint32_t sg = (color & 0xff00) >> 8;
    uint32_t sb = (color & 0xff0000) >> 16;

    uint32_t dr = background_color & 0xff;
    uint32_t dg = (background_color & 0xff00) >> 8;
    uint32_t db = (background_color & 0xff0000) >> 16;

    uint32_t r = min (0xff, ftoi (sr * alpha + dr * (1 - alpha)));
    uint32_t g = min (0xff, ftoi (sg * alpha + dg * (1 - alpha)));
    uint32_t b = min (0xff, ftoi (sb * alpha + db * (1 - alpha)));

    uint32_t res = r | (g << 8) | (b << 16) | 0xff000000;
    return res;
}

static inline void
_draw_vline_aa (uint8_t *restrict data, int stride, int x0, float y0, float y1, uint32_t color, uint32_t background_color) {
    int floor_y0 = ftoi (floorf (y0));
    int ceil_y1 = ftoi (ceilf (y1));
    uint32_t *ptr = (uint32_t *)&data[floor_y0 * stride + x0 * 4];
    int y = floor_y0;
    while (y <= ceil_y1) {
        float d = y0 - y;

        uint32_t final_color = color;

        if (d > 0 && d < 1) {
            final_color = _alpha_blend (color, background_color, 1 - d);
        }
        else {
            d = y - y1;
            if (d > 0 && d < 1) {
                final_color = _alpha_blend (color, background_color, 1 - d);
            }
        }

        *ptr = final_color;
        y++;
        ptr += stride / sizeof (uint32_t);
    }
}

static uint32_t
_gdk_color_convert_to_uint32 (const GdkColor *base_color) {
    uint8_t red = (base_color->red & 0xff00) >> 8;
    uint8_t green = (base_color->green & 0xff00) >> 8;
    uint8_t blue = (base_color->blue & 0xff00) >> 8;

    uint32_t color = (0xff << 24) | (red << 16) | (green << 8) | blue;
    return color;
}

static void
_scope_update_preferences (w_scope_t *scope) {
    GdkColor base_color;
    if (scope->use_custom_color && scope->custom_color_set) {
        base_color = scope->custom_color;
    }
    else {
        gtkui_get_vis_custom_base_color (&base_color);
    }
    scope->draw_color = _gdk_color_convert_to_uint32 (&base_color);

    GdkColor background_color;
    if (scope->use_custom_background_color && scope->custom_background_color_set) {
        background_color = scope->custom_background_color;
    }
    else {
        gtkui_get_vis_custom_background_color (&background_color);
    }
    scope->background_color = _gdk_color_convert_to_uint32 (&background_color);
}

static void
_scope_update_listening (w_scope_t *w) {
    gboolean is_visible = gtk_widget_get_mapped (w->drawarea);
    if (w->is_listening && !is_visible) {
        deadbeef->vis_waveform_unlisten (w);
        w->is_listening = FALSE;
    }
    else if (!w->is_listening && is_visible) {
        deadbeef->vis_waveform_listen (w, scope_wavedata_listener);
        w->is_listening = TRUE;
    }
}

static void
_scope_unmap (GtkWidget *self, gpointer user_data) {
    _scope_update_listening (user_data);
}

gboolean
scope_draw_cairo (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    GtkAllocation draw_rect;
    gtk_widget_get_allocation (widget, &draw_rect);

    GtkAllocation orig_size = draw_rect;

    w_scope_t *w = user_data;
    _scope_update_listening (w);
    _scope_update_preferences (w);

    float scale_factor;
    switch (w->scale) {
    case SCOPE_SCALE_AUTO:
        scale_factor = 1;
        break;
    case SCOPE_SCALE_1X:
        scale_factor = 1;
        break;
    case SCOPE_SCALE_2X:
        scale_factor = 1 / 2.f;
        break;
    case SCOPE_SCALE_3X:
        scale_factor = 1 / 3.f;
        break;
    case SCOPE_SCALE_4X:
        scale_factor = 1 / 4.f;
        break;
    }

    draw_rect.width *= scale_factor;
    draw_rect.height *= scale_factor;

    deadbeef->mutex_lock (w->mutex);
    if (w->scope.sample_count != 0) {

        ddb_scope_tick (&w->scope);
        ddb_scope_get_draw_data (&w->scope, (int)(draw_rect.width), (int)(draw_rect.height), 1, &w->draw_data);
    }
    deadbeef->mutex_unlock (w->mutex);

    if (!w->surf || cairo_image_surface_get_width (w->surf) != draw_rect.width || cairo_image_surface_get_height (w->surf) != draw_rect.height) {
        if (w->surf) {
            cairo_surface_destroy (w->surf);
            w->surf = NULL;
        }
        w->surf = cairo_image_surface_create (CAIRO_FORMAT_RGB24, draw_rect.width, draw_rect.height);
    }

    cairo_surface_flush (w->surf);
    unsigned char *data = cairo_image_surface_get_data (w->surf);
    if (!data) {
        return FALSE;
    }
    int stride = cairo_image_surface_get_stride (w->surf);

    //    memset (data, 0, draw_rect.height * stride);

    // fill with background color
    uint8_t *color_data = data;
    for (int y = 0; y < draw_rect.height; y++) {
        uint32_t *row_data = (uint32_t *)color_data;
        for (int x = 0; x < draw_rect.width; x++) {
            *row_data = w->background_color;
            row_data++;
        }
        color_data += stride;
    }

    if (w->draw_data.point_count != 0 && draw_rect.height > 2) {

        int width = w->draw_data.point_count;
        ddb_scope_point_t *minmax = w->draw_data.points;
        int channels = w->draw_data.mode == DDB_SCOPE_MONO ? 1 : w->draw_data.channels;

        fpu_control ctl = 0;
        fpu_setround (&ctl);

        for (int c = 0; c < channels; c++) {
            for (int x = 0; x < width; x++) {
                float ymin = min (draw_rect.height - 1, max (0, minmax->ymin));
                float ymax = min (draw_rect.height - 1, max (0, minmax->ymax));
                _draw_vline_aa (data, stride, x, ymin, ymax, w->draw_color, w->background_color);
                minmax++;
            }
        }

        fpu_restore (ctl);
    }
    cairo_surface_mark_dirty (w->surf);
    cairo_save (cr);

    cairo_scale (cr, (double)orig_size.width / (double)draw_rect.width, (double)orig_size.height / (double)draw_rect.height);
    cairo_set_source_surface (cr, w->surf, 0, 0);
    cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_FAST);
    cairo_paint (cr);

    cairo_restore (cr);

    return FALSE;
}

gboolean
scope_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    return scope_draw_cairo (widget, cr, user_data);
}

gboolean
scope_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = scope_draw_cairo (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

void
w_scope_init (ddb_gtkui_widget_t *w) {
    w_scope_t *s = (w_scope_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    _scope_update_listening (s);
    s->drawtimer = g_timeout_add (33, w_scope_draw_cb, w);
}

static void
_scope_menu_update (w_scope_t *s) {
    s->updating_menu = TRUE;
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->mode_mono_item), s->scope.mode == DDB_SCOPE_MONO);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->mode_multichannel_item), s->scope.mode == DDB_SCOPE_MULTICHANNEL);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->fragment_duration_50ms_item), s->scope.fragment_duration == 50);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->fragment_duration_100ms_item), s->scope.fragment_duration == 100);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->fragment_duration_200ms_item), s->scope.fragment_duration == 200);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->fragment_duration_300ms_item), s->scope.fragment_duration == 300);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->fragment_duration_500ms_item), s->scope.fragment_duration == 500);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->scale_auto_item), s->scale == SCOPE_SCALE_AUTO);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->scale_1x_item), s->scale == SCOPE_SCALE_1X);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->scale_2x_item), s->scale == SCOPE_SCALE_2X);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->scale_3x_item), s->scale == SCOPE_SCALE_3X);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->scale_4x_item), s->scale == SCOPE_SCALE_4X);
    s->updating_menu = FALSE;
}

static gboolean
_scope_button_press (GtkWidget *self, GdkEventButton *event, gpointer user_data) {
    if (w_get_design_mode ()) {
        return FALSE;
    }
    w_scope_t *s = user_data;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        _scope_menu_update (s);
        gtk_menu_popup_at_pointer (GTK_MENU (s->menu), NULL);
    }
    return TRUE;
}

static gboolean
_scope_show_preferences (w_scope_t *s) {
    GdkColor base_color;
    gtkui_get_vis_custom_base_color (&base_color);
    GdkColor background_color;
    gtkui_get_vis_custom_background_color (&background_color);

    w_vis_color_row_t rows[] = {
        { _("Use custom color"), &s->use_custom_color, &s->custom_color_set, &s->custom_color, base_color },
        { _("Use custom background color"), &s->use_custom_background_color, &s->custom_background_color_set, &s->custom_background_color, background_color },
    };
    return w_vis_color_preferences_dialog (_("Scope Preferences"), rows, sizeof (rows) / sizeof (rows[0]));
}

static void
_scope_menu_activate (GtkWidget *self, gpointer user_data) {
    w_scope_t *s = user_data;

    if (s->updating_menu) {
        return;
    }

    if (self == s->preferences_item) {
        if (!_scope_show_preferences (s)) {
            return;
        }
    }
    else if (self == s->mode_multichannel_item) {
        s->scope.mode = DDB_SCOPE_MULTICHANNEL;
    }
    else if (self == s->mode_mono_item) {
        s->scope.mode = DDB_SCOPE_MONO;
    }
    else if (self == s->scale_auto_item) {
        s->scale = SCOPE_SCALE_AUTO;
    }
    else if (self == s->scale_1x_item) {
        s->scale = SCOPE_SCALE_1X;
    }
    else if (self == s->scale_2x_item) {
        s->scale = SCOPE_SCALE_2X;
    }
    else if (self == s->scale_3x_item) {
        s->scale = SCOPE_SCALE_3X;
    }
    else if (self == s->scale_4x_item) {
        s->scale = SCOPE_SCALE_4X;
    }
    else if (self == s->fragment_duration_50ms_item) {
        s->scope.fragment_duration = 50;
    }
    else if (self == s->fragment_duration_100ms_item) {
        s->scope.fragment_duration = 100;
    }
    else if (self == s->fragment_duration_200ms_item) {
        s->scope.fragment_duration = 200;
    }
    else if (self == s->fragment_duration_300ms_item) {
        s->scope.fragment_duration = 300;
    }
    else if (self == s->fragment_duration_500ms_item) {
        s->scope.fragment_duration = 500;
    }
    w_save_with_action (_("Change Oscilloscope Settings"));
}

static void
_scope_deserialize_from_keyvalues (ddb_gtkui_widget_t *widget, const char **keyvalues) {
    w_scope_t *w = (w_scope_t *)widget;
    w->scope.mode = DDB_SCOPE_MULTICHANNEL;
    w->scale = SCOPE_SCALE_AUTO;
    w->scope.fragment_duration = 300;
    w->use_custom_color = FALSE;
    w->use_custom_background_color = FALSE;
    w->custom_color_set = FALSE;
    w->custom_background_color_set = FALSE;
    for (int i = 0; keyvalues[i]; i += 2) {
        // values of non-string json types are passed as NULL
        if (keyvalues[i + 1] == NULL) {
            continue;
        }
        if (!strcmp (keyvalues[i], "useCustomColor")) {
            w->use_custom_color = !strcmp (keyvalues[i + 1], "1");
        }
        else if (!strcmp (keyvalues[i], "customColor")) {
            w->custom_color_set = w_vis_color_from_string (keyvalues[i + 1], &w->custom_color);
        }
        else if (!strcmp (keyvalues[i], "useCustomBackgroundColor")) {
            w->use_custom_background_color = !strcmp (keyvalues[i + 1], "1");
        }
        else if (!strcmp (keyvalues[i], "customBackgroundColor")) {
            w->custom_background_color_set = w_vis_color_from_string (keyvalues[i + 1], &w->custom_background_color);
        }
        else if (!strcmp (keyvalues[i], "renderMode")) {
            if (!strcmp (keyvalues[i + 1], "mono")) {
                w->scope.mode = DDB_SCOPE_MONO;
            }
        }
        else if (!strcmp (keyvalues[i], "scaleMode")) {
            if (!strcmp (keyvalues[i + 1], "1x")) {
                w->scale = SCOPE_SCALE_1X;
            }
            else if (!strcmp (keyvalues[i + 1], "2x")) {
                w->scale = SCOPE_SCALE_2X;
            }
            else if (!strcmp (keyvalues[i + 1], "3x")) {
                w->scale = SCOPE_SCALE_3X;
            }
            else if (!strcmp (keyvalues[i + 1], "4x")) {
                w->scale = SCOPE_SCALE_4X;
            }
        }
        else if (!strcmp (keyvalues[i], "fragmentDuration")) {
            if (!strcmp (keyvalues[i + 1], "50")) {
                w->scope.fragment_duration = 50;
            }
            else if (!strcmp (keyvalues[i + 1], "100")) {
                w->scope.fragment_duration = 100;
            }
            else if (!strcmp (keyvalues[i + 1], "200")) {
                w->scope.fragment_duration = 200;
            }
            else if (!strcmp (keyvalues[i + 1], "300")) {
                w->scope.fragment_duration = 300;
            }
            else if (!strcmp (keyvalues[i + 1], "500")) {
                w->scope.fragment_duration = 500;
            }
        }
    }
}

static char const **
_scope_serialize_to_keyvalues (ddb_gtkui_widget_t *widget) {
    w_scope_t *w = (w_scope_t *)widget;
    char const **keyvalues = calloc (7 * 2 + 1, sizeof (char *));
    keyvalues[0] = "renderMode";
    switch (w->scope.mode) {
    case DDB_SCOPE_MONO:
        keyvalues[1] = "mono";
        break;
    case DDB_SCOPE_MULTICHANNEL:
        keyvalues[1] = "multichannel";
        break;
    }
    keyvalues[2] = "scaleMode";
    switch (w->scale) {
    case SCOPE_SCALE_AUTO:
        keyvalues[3] = "auto";
        break;
    case SCOPE_SCALE_1X:
        keyvalues[3] = "1x";
        break;
    case SCOPE_SCALE_2X:
        keyvalues[3] = "2x";
        break;
    case SCOPE_SCALE_3X:
        keyvalues[3] = "3x";
        break;
    case SCOPE_SCALE_4X:
        keyvalues[3] = "4x";
        break;
    }
    keyvalues[4] = "fragmentDuration";
    switch (w->scope.fragment_duration) {
    case 50:
        keyvalues[5] = "50";
        break;
    case 100:
        keyvalues[5] = "100";
        break;
    case 200:
        keyvalues[5] = "200";
        break;
    case 500:
        keyvalues[5] = "500";
        break;
    case 300:
    default:
        keyvalues[5] = "300";
        break;
    }
    int n = 6;
    keyvalues[n++] = "useCustomColor";
    keyvalues[n++] = w->use_custom_color ? "1" : "0";
    if (w->custom_color_set) {
        keyvalues[n++] = "customColor";
        keyvalues[n++] = w_vis_color_to_string (&w->custom_color);
    }
    keyvalues[n++] = "useCustomBackgroundColor";
    keyvalues[n++] = w->use_custom_background_color ? "1" : "0";
    if (w->custom_background_color_set) {
        keyvalues[n++] = "customBackgroundColor";
        keyvalues[n++] = w_vis_color_to_string (&w->custom_background_color);
    }
    return keyvalues;
}

static void
_scope_free_serialized_keyvalues (ddb_gtkui_widget_t *widget, char const **keyvalues) {
    // only the color values are allocated
    for (int i = 0; keyvalues[i]; i += 2) {
        if (!strcmp (keyvalues[i], "customColor") || !strcmp (keyvalues[i], "customBackgroundColor")) {
            free ((char *)keyvalues[i + 1]);
        }
    }
    free (keyvalues);
}

ddb_gtkui_widget_t *
w_scope_create (void) {
    w_scope_t *w = malloc (sizeof (w_scope_t));
    memset (w, 0, sizeof (w_scope_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_scope_init;
    w->base.destroy = w_scope_destroy;
    w->exapi._size = sizeof (ddb_gtkui_widget_extended_api_t);
    w->exapi.deserialize_from_keyvalues = _scope_deserialize_from_keyvalues;
    w->exapi.serialize_to_keyvalues = _scope_serialize_to_keyvalues;
    w->exapi.free_serialized_keyvalues = _scope_free_serialized_keyvalues;
    w->drawarea = gtk_drawing_area_new ();

    ddb_scope_init (&w->scope);
    w->scope.mode = DDB_SCOPE_MULTICHANNEL;
    w->scope.fragment_duration = 300;

    w->mutex = deadbeef->mutex_create ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect_after ((gpointer)w->drawarea, "expose_event", G_CALLBACK (scope_expose_event), w);
#else
    g_signal_connect_after ((gpointer)w->drawarea, "draw", G_CALLBACK (scope_draw), w);
#endif

    g_signal_connect ((gpointer)w->drawarea, "unmap", G_CALLBACK (_scope_unmap), w);

    g_signal_connect ((gpointer)w->base.widget, "button-press-event", G_CALLBACK (_scope_button_press), w);

    w_override_signals (w->base.widget, w);

    w->menu = gtk_menu_new ();

    GtkWidget *rendering_mode_item = gtk_menu_item_new_with_mnemonic (_ ("Rendering Mode"));
    gtk_widget_show (rendering_mode_item);
    GtkWidget *rendering_mode_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (rendering_mode_item), rendering_mode_menu);

    w->mode_multichannel_item = gtk_check_menu_item_new_with_mnemonic (_ ("Multichannel"));
    gtk_widget_show (w->mode_multichannel_item);

    w->mode_mono_item = gtk_check_menu_item_new_with_mnemonic (_ ("Mono"));
    gtk_widget_show (w->mode_mono_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (rendering_mode_menu), w->mode_multichannel_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (rendering_mode_menu), w->mode_mono_item, 1);

    GtkWidget *scale_item = gtk_menu_item_new_with_mnemonic (_ ("Scale"));
    gtk_widget_show (scale_item);
    GtkWidget *scale_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (scale_item), scale_menu);

    w->scale_auto_item = gtk_check_menu_item_new_with_mnemonic (_ ("Auto"));
    gtk_widget_show (w->scale_auto_item);
    w->scale_1x_item = gtk_check_menu_item_new_with_mnemonic (_ ("1x"));
    gtk_widget_show (w->scale_1x_item);
    w->scale_2x_item = gtk_check_menu_item_new_with_mnemonic (_ ("2x"));
    gtk_widget_show (w->scale_2x_item);
    w->scale_3x_item = gtk_check_menu_item_new_with_mnemonic (_ ("3x"));
    gtk_widget_show (w->scale_3x_item);
    w->scale_4x_item = gtk_check_menu_item_new_with_mnemonic (_ ("4x"));
    gtk_widget_show (w->scale_4x_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (scale_menu), w->scale_auto_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (scale_menu), w->scale_1x_item, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL (scale_menu), w->scale_2x_item, 2);
    gtk_menu_shell_insert (GTK_MENU_SHELL (scale_menu), w->scale_3x_item, 3);
    gtk_menu_shell_insert (GTK_MENU_SHELL (scale_menu), w->scale_4x_item, 4);

    GtkWidget *fragment_duration_item = gtk_menu_item_new_with_mnemonic (_ ("Fragment Duration"));
    gtk_widget_show (fragment_duration_item);
    GtkWidget *fragment_duration_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (fragment_duration_item), fragment_duration_menu);

    w->fragment_duration_50ms_item = gtk_check_menu_item_new_with_mnemonic (_ ("50 ms"));
    gtk_widget_show (w->fragment_duration_50ms_item);
    w->fragment_duration_100ms_item = gtk_check_menu_item_new_with_mnemonic (_ ("100 ms"));
    gtk_widget_show (w->fragment_duration_100ms_item);
    w->fragment_duration_200ms_item = gtk_check_menu_item_new_with_mnemonic (_ ("200 ms"));
    gtk_widget_show (w->fragment_duration_200ms_item);
    w->fragment_duration_300ms_item = gtk_check_menu_item_new_with_mnemonic (_ ("300 ms"));
    gtk_widget_show (w->fragment_duration_300ms_item);
    w->fragment_duration_500ms_item = gtk_check_menu_item_new_with_mnemonic (_ ("500 ms"));
    gtk_widget_show (w->fragment_duration_500ms_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (fragment_duration_menu), w->fragment_duration_50ms_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (fragment_duration_menu), w->fragment_duration_100ms_item, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL (fragment_duration_menu), w->fragment_duration_200ms_item, 2);
    gtk_menu_shell_insert (GTK_MENU_SHELL (fragment_duration_menu), w->fragment_duration_300ms_item, 3);
    gtk_menu_shell_insert (GTK_MENU_SHELL (fragment_duration_menu), w->fragment_duration_500ms_item, 4);

    GtkWidget *separator_item = gtk_separator_menu_item_new ();
    gtk_widget_show (separator_item);

    w->preferences_item = gtk_menu_item_new_with_mnemonic (_("Preferences"));
    gtk_widget_show (w->preferences_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), rendering_mode_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), scale_item, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), fragment_duration_item, 2);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), separator_item, 3);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), w->preferences_item, 4);

    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->mode_multichannel_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->mode_mono_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->scale_auto_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->scale_1x_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->scale_2x_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->scale_3x_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->scale_4x_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->fragment_duration_50ms_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->fragment_duration_100ms_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->fragment_duration_200ms_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->fragment_duration_300ms_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->fragment_duration_500ms_item), TRUE);

    g_signal_connect ((gpointer)w->mode_multichannel_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->mode_mono_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->scale_auto_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->scale_1x_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->scale_2x_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->scale_3x_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->scale_4x_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->fragment_duration_50ms_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->fragment_duration_100ms_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->fragment_duration_200ms_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->fragment_duration_300ms_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->fragment_duration_500ms_item, "activate", G_CALLBACK (_scope_menu_activate), w);
    g_signal_connect ((gpointer)w->preferences_item, "activate", G_CALLBACK (_scope_menu_activate), w);

    return (ddb_gtkui_widget_t *)w;
}
