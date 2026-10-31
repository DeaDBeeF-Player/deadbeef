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
#include "../../../shared/analyzer/analyzer.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../support.h"
#ifdef DDB_WARN_DEPRECATED
#    undef DDB_WARN_DEPRECATED
#endif
#include "../widgets.h"
#include "w_spectrum.h"

#define SpectrumVisXOffset 40
#define SpectrumVisYOffset 12

typedef struct {
    ddb_gtkui_widget_t base;
    ddb_gtkui_widget_extended_api_t exapi;
    GtkWidget *drawarea;
    guint drawtimer;

    intptr_t mutex;

    gboolean is_listening;
    ddb_analyzer_t analyzer;
    ddb_analyzer_draw_data_t draw_data;
    ddb_waveformat_t fmt;
    ddb_audio_data_t input_data;

    float grid_color[3];
    float peak_color[3];
    float bar_color[3];
    float background_color[3];

    gboolean use_custom_peak_color;
    gboolean use_custom_bar_color;
    gboolean use_custom_background_color;
    // custom colors are remembered while the "use custom" option is off, if they were ever chosen
    gboolean custom_peak_color_set;
    gboolean custom_bar_color_set;
    gboolean custom_background_color_set;
    GdkColor custom_peak_color;
    GdkColor custom_bar_color;
    GdkColor custom_background_color;

    cairo_surface_t *surf;

    gboolean updating_menu; // suppress menu event handlers
    GtkWidget *menu;
    GtkWidget *mode_discrete_item;
    GtkWidget *mode_12_item;
    GtkWidget *mode_24_item;

    GtkWidget *gap_none_item;
    GtkWidget *gap_2_item;
    GtkWidget *gap_3_item;
    GtkWidget *gap_4_item;
    GtkWidget *gap_5_item;
    GtkWidget *gap_6_item;
    GtkWidget *gap_7_item;
    GtkWidget *gap_8_item;
    GtkWidget *gap_9_item;
    GtkWidget *gap_10_item;

    GtkWidget *preferences_item;
} w_spectrum_t;

///// spectrum vis
void
w_spectrum_destroy (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    deadbeef->vis_spectrum_unlisten (w);
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    if (s->surf) {
        cairo_surface_destroy (s->surf);
        s->surf = NULL;
    }

    ddb_analyzer_dealloc (&s->analyzer);
    ddb_analyzer_draw_data_dealloc (&s->draw_data);
    free (s->input_data.data);
    s->input_data.data = NULL;

    if (s->mutex) {
        deadbeef->mutex_free (s->mutex);
        s->mutex = 0;
    }
}

gboolean
w_spectrum_draw_cb (void *data) {
    w_spectrum_t *s = data;
    gtk_widget_queue_draw (s->drawarea);
    return TRUE;
}

static void
spectrum_audio_listener (void *ctx, const ddb_audio_data_t *data) {
    w_spectrum_t *w = ctx;

    deadbeef->mutex_lock (w->mutex);
    // copy the input data for later consumption
    if (w->input_data.nframes != data->nframes || w->input_data.fmt->channels != data->fmt->channels) {
        free (w->input_data.data);
        w->input_data.data = NULL;

        if (data->nframes > 0) {
            w->input_data.data = malloc (data->nframes * data->fmt->channels * sizeof (float));
        }
        w->input_data.nframes = data->nframes;
    }
    memcpy (w->input_data.fmt, data->fmt, sizeof (ddb_waveformat_t));
    if (w->input_data.data != NULL && data->nframes > 0) {
        memcpy (w->input_data.data, data->data, data->nframes * data->fmt->channels * sizeof (float));
    }
    deadbeef->mutex_unlock (w->mutex);
}

static void
_spectrum_update_listening (w_spectrum_t *w) {
    gboolean is_visible = gtk_widget_get_mapped (w->drawarea);
    if (w->is_listening && !is_visible) {
        deadbeef->vis_spectrum_unlisten (w);
        w->is_listening = FALSE;
    }
    else if (!w->is_listening && is_visible) {
        deadbeef->vis_spectrum_listen2 (w, spectrum_audio_listener);
        w->is_listening = TRUE;
    }
}

static void
_spectrum_unmap (GtkWidget *self, gpointer user_data) {
    _spectrum_update_listening (user_data);
}

static void
_spectrum_update_preferences (w_spectrum_t *spectrum) {
    GdkColor color;
    gtkui_get_vis_custom_base_color (&color);

    spectrum->grid_color[0] = 0.5f;
    spectrum->grid_color[1] = 0.5f;
    spectrum->grid_color[2] = 0.5f;

    // saturate the peaks slightly
    spectrum->peak_color[0] = color.red / 65535.f;
    spectrum->peak_color[1] = color.green / 65535.f;
    spectrum->peak_color[2] = color.blue / 65535.f;

    spectrum->peak_color[0] += (1.f - spectrum->peak_color[0]) * 0.5f;
    spectrum->peak_color[1] += (1.f - spectrum->peak_color[1]) * 0.5f;
    spectrum->peak_color[2] += (1.f - spectrum->peak_color[2]) * 0.5f;

    spectrum->bar_color[0] = color.red / 65535.f;
    spectrum->bar_color[1] = color.green / 65535.f;
    spectrum->bar_color[2] = color.blue / 65535.f;

    gtkui_get_vis_custom_background_color (&color);
    spectrum->background_color[0] = color.red / 65535.f;
    spectrum->background_color[1] = color.green / 65535.f;
    spectrum->background_color[2] = color.blue / 65535.f;

    if (spectrum->use_custom_peak_color && spectrum->custom_peak_color_set) {
        spectrum->peak_color[0] = spectrum->custom_peak_color.red / 65535.f;
        spectrum->peak_color[1] = spectrum->custom_peak_color.green / 65535.f;
        spectrum->peak_color[2] = spectrum->custom_peak_color.blue / 65535.f;
    }
    if (spectrum->use_custom_bar_color && spectrum->custom_bar_color_set) {
        spectrum->bar_color[0] = spectrum->custom_bar_color.red / 65535.f;
        spectrum->bar_color[1] = spectrum->custom_bar_color.green / 65535.f;
        spectrum->bar_color[2] = spectrum->custom_bar_color.blue / 65535.f;
    }
    if (spectrum->use_custom_background_color && spectrum->custom_background_color_set) {
        spectrum->background_color[0] = spectrum->custom_background_color.red / 65535.f;
        spectrum->background_color[1] = spectrum->custom_background_color.green / 65535.f;
        spectrum->background_color[2] = spectrum->custom_background_color.blue / 65535.f;
    }
}

static void
_spectrum_draw_grid (w_spectrum_t *w, cairo_t *cr, GtkAllocation size) {
    // horz lines, db scale
    float lower = -floor (w->analyzer.db_lower_bound);
    for (int db = 0; db < lower; db += 10) {
        float y = (float)(db / lower) * (size.height - SpectrumVisYOffset);

        cairo_move_to (cr, SpectrumVisXOffset, y + SpectrumVisYOffset);
        cairo_line_to (cr, size.width - 1, y + SpectrumVisYOffset);
    }

    static const double dash[2] = { 1, 2 };
    cairo_set_dash (cr, dash, 2, 0);
    cairo_stroke (cr);
    cairo_set_dash (cr, NULL, 0, 0);

    // db text
    cairo_set_font_size (cr, 10);

    for (int db = 0; db < lower; db += 10) {
        float y = (float)(db / lower) * (size.height - SpectrumVisYOffset);

        char str[20];
        snprintf (str, sizeof (str), "%d dB", -db);

        cairo_move_to (cr, 0, y + 9 + SpectrumVisYOffset);
        cairo_show_text (cr, str);
    }
}

static void
_spectrum_draw_frequency_labels (w_spectrum_t *w, cairo_t *cr, GtkAllocation size) {
    // octaves text
    for (int i = 0; i < w->draw_data.label_freq_count; i++) {
        if (w->draw_data.label_freq_positions[i] < 0 || w->draw_data.label_freq_positions[i] >= size.width) {
            continue;
        }

        cairo_move_to (cr, w->draw_data.label_freq_positions[i] + SpectrumVisXOffset, 9);
        cairo_show_text (cr, w->draw_data.label_freq_texts[i]);
    }
}

static gboolean
spectrum_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    w_spectrum_t *w = user_data;

    _spectrum_update_preferences (w);

    _spectrum_update_listening (w);

    cairo_set_source_rgb (cr, w->background_color[0], w->background_color[1], w->background_color[2]);
    cairo_paint (cr);

    if (w->input_data.nframes == 0) {
        return FALSE;
    }

    GtkAllocation a;
    gtk_widget_get_allocation (widget, &a);

    deadbeef->mutex_lock (w->mutex);
    ddb_analyzer_process (&w->analyzer, w->input_data.fmt->samplerate, w->input_data.fmt->channels, w->input_data.data, w->input_data.nframes);
    ddb_analyzer_tick (&w->analyzer);

    ddb_analyzer_get_draw_data (&w->analyzer, a.width - SpectrumVisXOffset, a.height - SpectrumVisYOffset, &w->draw_data);
    deadbeef->mutex_unlock (w->mutex);

    cairo_set_source_rgb (cr, w->grid_color[0], w->grid_color[1], w->grid_color[2]);
    _spectrum_draw_grid (w, cr, a);
    _spectrum_draw_frequency_labels (w, cr, a);

    // bars
    ddb_analyzer_draw_bar_t *bar = w->draw_data.bars;
    cairo_set_source_rgb (cr, w->bar_color[0], w->bar_color[1], w->bar_color[2]);
    for (int i = 0; i < w->draw_data.bar_count; i++, bar++) {
        if (w->analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
            cairo_move_to (cr, bar->xpos + SpectrumVisXOffset, a.height - bar->bar_height + SpectrumVisYOffset);
            cairo_line_to (cr, bar->xpos + SpectrumVisXOffset, a.height - 1);
        }
        else {
            cairo_rectangle (cr, bar->xpos + SpectrumVisXOffset, a.height - bar->bar_height + SpectrumVisYOffset, w->draw_data.bar_width, bar->bar_height);
        }
    }

    if (w->analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        cairo_set_line_width (cr, 1);
        cairo_stroke (cr);
    }
    else {
        cairo_fill (cr);
    }

    // peaks
    bar = w->draw_data.bars;
    cairo_set_source_rgb (cr, w->peak_color[0], w->peak_color[1], w->peak_color[2]);
    for (int i = 0; i < w->draw_data.bar_count; i++, bar++) {
        cairo_rectangle (cr, bar->xpos + SpectrumVisXOffset, a.height - bar->peak_ypos - 1 + SpectrumVisYOffset, w->draw_data.bar_width, 1);
    }
    cairo_fill (cr);

    return FALSE;
}

gboolean
spectrum_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
    gboolean res = spectrum_draw (widget, cr, user_data);
    cairo_destroy (cr);
    return res;
}

void
w_spectrum_init (ddb_gtkui_widget_t *w) {
    w_spectrum_t *s = (w_spectrum_t *)w;
    if (s->drawtimer) {
        g_source_remove (s->drawtimer);
        s->drawtimer = 0;
    }
    s->drawtimer = g_timeout_add (33, w_spectrum_draw_cb, w);
}

static void
_spectrum_menu_update (w_spectrum_t *s) {
    s->updating_menu = TRUE;
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->mode_discrete_item), s->analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->mode_12_item), s->analyzer.mode == DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS && s->analyzer.octave_bars_step == 2);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->mode_24_item), s->analyzer.mode == DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS && s->analyzer.octave_bars_step == 1);

    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_none_item), s->analyzer.bar_gap_denominator == 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_2_item), s->analyzer.bar_gap_denominator == 2);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_3_item), s->analyzer.bar_gap_denominator == 3);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_4_item), s->analyzer.bar_gap_denominator == 4);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_5_item), s->analyzer.bar_gap_denominator == 5);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_6_item), s->analyzer.bar_gap_denominator == 6);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_7_item), s->analyzer.bar_gap_denominator == 7);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_8_item), s->analyzer.bar_gap_denominator == 8);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_9_item), s->analyzer.bar_gap_denominator == 9);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (s->gap_10_item), s->analyzer.bar_gap_denominator == 10);
    s->updating_menu = FALSE;
}

static gboolean
_spectrum_button_press (GtkWidget *self, GdkEventButton *event, gpointer user_data) {
    if (w_get_design_mode ()) {
        return FALSE;
    }
    w_spectrum_t *s = user_data;

    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        _spectrum_menu_update (s);
        gtk_menu_popup_at_pointer (GTK_MENU (s->menu), NULL);
    }
    return TRUE;
}

static gboolean
_spectrum_show_preferences (w_spectrum_t *s) {
    // the colors that are used when no custom color is chosen
    GdkColor base_color;
    gtkui_get_vis_custom_base_color (&base_color);
    GdkColor peak_color = base_color;
    peak_color.red += (65535 - peak_color.red) / 2;
    peak_color.green += (65535 - peak_color.green) / 2;
    peak_color.blue += (65535 - peak_color.blue) / 2;
    GdkColor background_color;
    gtkui_get_vis_custom_background_color (&background_color);

    w_vis_color_row_t rows[] = {
        { _("Use custom peak color"), &s->use_custom_peak_color, &s->custom_peak_color_set, &s->custom_peak_color, peak_color },
        { _("Use custom bar color"), &s->use_custom_bar_color, &s->custom_bar_color_set, &s->custom_bar_color, base_color },
        { _("Use custom background color"), &s->use_custom_background_color, &s->custom_background_color_set, &s->custom_background_color, background_color },
    };
    return w_vis_color_preferences_dialog (_("Spectrum Analyzer Preferences"), rows, sizeof (rows) / sizeof (rows[0]));
}

static void
_spectrum_menu_activate (GtkWidget *self, gpointer user_data) {
    w_spectrum_t *s = user_data;

    if (s->updating_menu) {
        return;
    }

    if (self == s->preferences_item) {
        if (!_spectrum_show_preferences (s)) {
            return;
        }
    }
    else if (self == s->mode_discrete_item) {
        s->analyzer.mode = DDB_ANALYZER_MODE_FREQUENCIES;
        s->analyzer.mode_did_change = 1;
    }
    else if (self == s->mode_12_item) {
        s->analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
        s->analyzer.octave_bars_step = 2;
        s->analyzer.mode_did_change = 1;
    }
    else if (self == s->mode_24_item) {
        s->analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
        s->analyzer.octave_bars_step = 1;
        s->analyzer.mode_did_change = 1;
    }
    else if (self == s->gap_none_item) {
        s->analyzer.bar_gap_denominator = 0;
    }
    else if (self == s->gap_2_item) {
        s->analyzer.bar_gap_denominator = 2;
    }
    else if (self == s->gap_3_item) {
        s->analyzer.bar_gap_denominator = 3;
    }
    else if (self == s->gap_4_item) {
        s->analyzer.bar_gap_denominator = 4;
    }
    else if (self == s->gap_5_item) {
        s->analyzer.bar_gap_denominator = 5;
    }
    else if (self == s->gap_6_item) {
        s->analyzer.bar_gap_denominator = 6;
    }
    else if (self == s->gap_7_item) {
        s->analyzer.bar_gap_denominator = 7;
    }
    else if (self == s->gap_8_item) {
        s->analyzer.bar_gap_denominator = 8;
    }
    else if (self == s->gap_9_item) {
        s->analyzer.bar_gap_denominator = 9;
    }
    else if (self == s->gap_10_item) {
        s->analyzer.bar_gap_denominator = 10;
    }
    w_save_with_action (_("Change Spectrum Settings"));
}

static void
_spectrum_deserialize_from_keyvalues (ddb_gtkui_widget_t *widget, const char **keyvalues) {
    w_spectrum_t *s = (w_spectrum_t *)widget;

    s->analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
    s->analyzer.bar_gap_denominator = 3;
    s->use_custom_peak_color = FALSE;
    s->use_custom_bar_color = FALSE;
    s->use_custom_background_color = FALSE;
    s->custom_peak_color_set = FALSE;
    s->custom_bar_color_set = FALSE;
    s->custom_background_color_set = FALSE;

    for (int i = 0; keyvalues[i]; i += 2) {
        // values of non-string json types are passed as NULL
        if (keyvalues[i + 1] == NULL) {
            continue;
        }
        if (!strcmp (keyvalues[i], "useCustomPeakColor")) {
            s->use_custom_peak_color = !strcmp (keyvalues[i + 1], "1");
        }
        else if (!strcmp (keyvalues[i], "useCustomBarColor")) {
            s->use_custom_bar_color = !strcmp (keyvalues[i + 1], "1");
        }
        else if (!strcmp (keyvalues[i], "useCustomBackgroundColor")) {
            s->use_custom_background_color = !strcmp (keyvalues[i + 1], "1");
        }
        else if (!strcmp (keyvalues[i], "customPeakColor")) {
            s->custom_peak_color_set = w_vis_color_from_string (keyvalues[i + 1], &s->custom_peak_color);
        }
        else if (!strcmp (keyvalues[i], "customBarColor")) {
            s->custom_bar_color_set = w_vis_color_from_string (keyvalues[i + 1], &s->custom_bar_color);
        }
        else if (!strcmp (keyvalues[i], "customBackgroundColor")) {
            s->custom_background_color_set = w_vis_color_from_string (keyvalues[i + 1], &s->custom_background_color);
        }
        else if (!strcmp (keyvalues[i], "renderMode")) {
            if (!strcmp (keyvalues[i + 1], "frequencies")) {
                s->analyzer.mode = DDB_ANALYZER_MODE_FREQUENCIES;
            }
        }
        else if (!strcmp (keyvalues[i], "distanceBetweenBars")) {
            s->analyzer.bar_gap_denominator = atoi (keyvalues[i + 1]);
        }
        else if (!strcmp (keyvalues[i], "barGranularity")) {
            s->analyzer.octave_bars_step = atoi (keyvalues[i + 1]);
        }
    }
}

static char const **
_spectrum_serialize_to_keyvalues (ddb_gtkui_widget_t *widget) {
    w_spectrum_t *s = (w_spectrum_t *)widget;

    char const **keyvalues = calloc (9 * 2 + 1, sizeof (char *));

    keyvalues[0] = "renderMode";
    switch (s->analyzer.mode) {
    case DDB_ANALYZER_MODE_FREQUENCIES:
        keyvalues[1] = "frequencies";
        break;
    case DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS:
        keyvalues[1] = "bands";
        break;
    }

    char temp[10];

    keyvalues[2] = "distanceBetweenBars";
    snprintf (temp, sizeof (temp), "%d", s->analyzer.bar_gap_denominator);
    keyvalues[3] = strdup (temp);

    keyvalues[4] = "barGranularity";
    snprintf (temp, sizeof (temp), "%d", s->analyzer.octave_bars_step);
    keyvalues[5] = strdup (temp);

    int n = 6;
    keyvalues[n++] = "useCustomPeakColor";
    keyvalues[n++] = s->use_custom_peak_color ? "1" : "0";
    if (s->custom_peak_color_set) {
        keyvalues[n++] = "customPeakColor";
        keyvalues[n++] = w_vis_color_to_string (&s->custom_peak_color);
    }
    keyvalues[n++] = "useCustomBarColor";
    keyvalues[n++] = s->use_custom_bar_color ? "1" : "0";
    if (s->custom_bar_color_set) {
        keyvalues[n++] = "customBarColor";
        keyvalues[n++] = w_vis_color_to_string (&s->custom_bar_color);
    }
    keyvalues[n++] = "useCustomBackgroundColor";
    keyvalues[n++] = s->use_custom_background_color ? "1" : "0";
    if (s->custom_background_color_set) {
        keyvalues[n++] = "customBackgroundColor";
        keyvalues[n++] = w_vis_color_to_string (&s->custom_background_color);
    }

    return keyvalues;
}

static void
_spectrum_free_serialized_keyvalues (ddb_gtkui_widget_t *widget, char const **keyvalues) {
    // only the numbers and the colors are allocated
    for (int i = 0; keyvalues[i]; i += 2) {
        if (!strcmp (keyvalues[i], "distanceBetweenBars") || !strcmp (keyvalues[i], "barGranularity")
            || !strcmp (keyvalues[i], "customPeakColor") || !strcmp (keyvalues[i], "customBarColor")
            || !strcmp (keyvalues[i], "customBackgroundColor")) {
            free ((char *)keyvalues[i + 1]);
        }
    }
    free (keyvalues);
}

ddb_gtkui_widget_t *
w_spectrum_create (void) {
    w_spectrum_t *w = calloc (1, sizeof (w_spectrum_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_spectrum_init;
    w->base.destroy = w_spectrum_destroy;
    w->exapi._size = sizeof (ddb_gtkui_widget_extended_api_t);
    w->exapi.deserialize_from_keyvalues = _spectrum_deserialize_from_keyvalues;
    w->exapi.serialize_to_keyvalues = _spectrum_serialize_to_keyvalues;
    w->exapi.free_serialized_keyvalues = _spectrum_free_serialized_keyvalues;
    w->drawarea = gtk_drawing_area_new ();
    gtk_widget_show (w->drawarea);
    gtk_container_add (GTK_CONTAINER (w->base.widget), w->drawarea);
#if !GTK_CHECK_VERSION(3, 0, 0)
    g_signal_connect_after ((gpointer)w->drawarea, "expose_event", G_CALLBACK (spectrum_expose_event), w);
#else
    g_signal_connect_after ((gpointer)w->drawarea, "draw", G_CALLBACK (spectrum_draw), w);
#endif

    g_signal_connect ((gpointer)w->drawarea, "unmap", G_CALLBACK (_spectrum_unmap), w);
    g_signal_connect ((gpointer)w->base.widget, "button-press-event", G_CALLBACK (_spectrum_button_press), w);

    w_override_signals (w->base.widget, w);

    w->menu = gtk_menu_new ();

    GtkWidget *rendering_mode_item = gtk_menu_item_new_with_mnemonic (_ ("Rendering Mode"));
    gtk_widget_show (rendering_mode_item);
    GtkWidget *rendering_mode_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (rendering_mode_item), rendering_mode_menu);

    w->mode_discrete_item = gtk_check_menu_item_new_with_mnemonic (_ ("Discrete Frequencies"));
    gtk_widget_show (w->mode_discrete_item);

    w->mode_12_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/12 Octave Bands"));
    gtk_widget_show (w->mode_12_item);

    w->mode_24_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/24 Octave Bands"));
    gtk_widget_show (w->mode_24_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (rendering_mode_menu), w->mode_discrete_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (rendering_mode_menu), w->mode_12_item, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL (rendering_mode_menu), w->mode_24_item, 1);

    GtkWidget *gap_size_item = gtk_menu_item_new_with_mnemonic (_ ("Gap Size"));
    gtk_widget_show (gap_size_item);
    GtkWidget *gap_size_menu = gtk_menu_new ();
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (gap_size_item), gap_size_menu);

    w->gap_none_item = gtk_check_menu_item_new_with_mnemonic (_ ("None"));
    gtk_widget_show (w->gap_none_item);

    w->gap_2_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/2 Bar"));
    gtk_widget_show (w->gap_2_item);

    w->gap_3_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/3 Bar"));
    gtk_widget_show (w->gap_3_item);

    w->gap_4_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/4 Bar"));
    gtk_widget_show (w->gap_4_item);

    w->gap_5_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/5 Bar"));
    gtk_widget_show (w->gap_5_item);

    w->gap_6_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/6 Bar"));
    gtk_widget_show (w->gap_6_item);

    w->gap_7_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/7 Bar"));
    gtk_widget_show (w->gap_7_item);

    w->gap_8_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/8 Bar"));
    gtk_widget_show (w->gap_8_item);

    w->gap_9_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/9 Bar"));
    gtk_widget_show (w->gap_9_item);

    w->gap_10_item = gtk_check_menu_item_new_with_mnemonic (_ ("1/10 Bar"));
    gtk_widget_show (w->gap_10_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_none_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_2_item, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_3_item, 2);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_4_item, 3);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_5_item, 4);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_6_item, 5);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_7_item, 6);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_8_item, 7);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_9_item, 8);
    gtk_menu_shell_insert (GTK_MENU_SHELL (gap_size_menu), w->gap_10_item, 9);

    GtkWidget *separator_item = gtk_separator_menu_item_new ();
    gtk_widget_show (separator_item);

    w->preferences_item = gtk_menu_item_new_with_mnemonic (_("Preferences"));
    gtk_widget_show (w->preferences_item);

    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), rendering_mode_item, 0);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), gap_size_item, 1);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), separator_item, 2);
    gtk_menu_shell_insert (GTK_MENU_SHELL (w->menu), w->preferences_item, 3);

    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->mode_discrete_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->mode_12_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->mode_24_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_none_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_2_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_3_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_4_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_5_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_6_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_7_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_8_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_9_item), TRUE);
    gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (w->gap_10_item), TRUE);

    g_signal_connect ((gpointer)w->mode_discrete_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->mode_12_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->mode_24_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_none_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_2_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_3_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_4_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_5_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_6_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_7_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_8_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_9_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->gap_10_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);
    g_signal_connect ((gpointer)w->preferences_item, "activate", G_CALLBACK (_spectrum_menu_activate), w);

    w->input_data.fmt = &w->fmt;

    // using the analyzer framework
    ddb_analyzer_init (&w->analyzer);
    w->analyzer.db_lower_bound = -80;
    w->analyzer.peak_hold = 10;
    w->analyzer.view_width = 1000;
    w->analyzer.fractional_bars = 1;
    w->analyzer.octave_bars_step = 2;
    w->analyzer.max_of_stereo_data = 1;
    w->analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;

    w->mutex = deadbeef->mutex_create ();

    _spectrum_update_listening (w);
    return (ddb_gtkui_widget_t *)w;
}
