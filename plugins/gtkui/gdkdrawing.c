/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "drawing.h"
#include "support.h"
#include "gtkui.h"

static char gtkui_listview_text_font[1000];
static char gtkui_listview_group_text_font[1000];
static char gtkui_listview_column_text_font[1000];
static char gtkui_tabstrip_text_font[1000];

static GtkWidget *theme_entry;

static PangoFontDescription *
get_new_font_description_from_type (int type)
{
    PangoFontDescription *desc;
    switch (type) {
        case DDB_LIST_FONT:
            desc = pango_font_description_from_string (gtkui_listview_text_font);
            break;
        case DDB_GROUP_FONT:
            desc = pango_font_description_from_string (gtkui_listview_group_text_font);
            break;
        case DDB_TABSTRIP_FONT:
            desc = pango_font_description_from_string (gtkui_tabstrip_text_font);
            break;
        case DDB_COLUMN_FONT:
            desc = pango_font_description_from_string (gtkui_listview_column_text_font);
            break;
        default:
            desc = NULL;
    }
    return desc;
}

static int
get_pango_alignment (int align)
{
    int alignment = 0;
    switch (align) {
        case 0:
            alignment = PANGO_ALIGN_LEFT;
            break;
        case 1:
            alignment = PANGO_ALIGN_RIGHT;
            break;
        case 2:
            alignment = PANGO_ALIGN_CENTER;
            break;
        default:
            alignment = PANGO_ALIGN_LEFT;
            break;
    }
    return alignment;
}

void
draw_begin (drawctx_t * const ctx, cairo_t *cr) {
    ctx->drawable = cr;
}

void
draw_end (drawctx_t * const ctx) {
    ctx->drawable = NULL;
}

void
draw_set_fg_color (drawctx_t * const ctx, float *rgb) {
    cairo_set_source_rgb (ctx->drawable, rgb[0], rgb[1], rgb[2]);
}

void
draw_line (drawctx_t * const ctx, float x1, float y1, float x2, float y2) {
    cairo_move_to (ctx->drawable, x1, y1);
    cairo_line_to (ctx->drawable, x2, y2);
    cairo_stroke (ctx->drawable);
}

void
draw_rect (drawctx_t * const ctx, float x, float y, float w, float h, int fill) {
    cairo_rectangle (ctx->drawable, x, y, w, h);
    fill ? cairo_fill (ctx->drawable) : cairo_stroke (ctx->drawable);
}

void
draw_free (drawctx_t * const ctx) {
    draw_end (ctx);
    if (ctx->pangoctx) {
        g_object_unref (ctx->pangoctx);
        ctx->pangoctx = NULL;
    }
    if (ctx->pangolayout) {
        g_object_unref (ctx->pangolayout);
        ctx->pangolayout = NULL;
    }
    if (ctx->font_style) {
        g_object_unref (ctx->font_style);
        ctx->font_style = NULL;
    }
}

void
draw_init_font (drawctx_t * const ctx, int type, int reset) {
    if (reset || !ctx->pango_ready) {
        if (ctx->pangoctx) {
            g_object_unref (ctx->pangoctx);
            ctx->pangoctx = NULL;
        }
        if (ctx->pangolayout) {
            g_object_unref (ctx->pangolayout);
            ctx->pangolayout = NULL;
        }
        if (ctx->font_style) {
            g_object_unref (ctx->font_style);
            ctx->font_style = NULL;
        }

        ctx->font_style = gtk_style_new ();
        if (ctx->font_style->font_desc) {
            pango_font_description_free (ctx->font_style->font_desc);
            ctx->font_style->font_desc = get_new_font_description_from_type (type);
        }

        ctx->pangoctx = gdk_pango_context_get ();
        ctx->pangolayout = pango_layout_new (ctx->pangoctx);
        pango_layout_set_ellipsize (ctx->pangolayout, PANGO_ELLIPSIZE_END);
        PangoFontDescription *desc = ctx->font_style->font_desc;
        ctx->font_weight = pango_font_description_get_weight (desc);
        pango_layout_set_font_description (ctx->pangolayout, desc);
        ctx->pango_ready = 1;
    }
    else if (ctx->pango_ready) {
        PangoFontDescription *desc = ctx->font_style->font_desc;
        pango_layout_set_font_description (ctx->pangolayout, desc);
    }
}

void
draw_init_font_style (drawctx_t * const ctx, int bold, int italic, int type) {
    PangoFontDescription *desc_default = ctx->font_style->font_desc;
    if (desc_default != NULL) {
        pango_layout_set_font_description (ctx->pangolayout, desc_default);
    }
    PangoFontDescription *desc = pango_font_description_copy (pango_layout_get_font_description (ctx->pangolayout));
    if (bold) {
        pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
    }
    if (italic) {
        pango_font_description_set_style (desc, PANGO_STYLE_ITALIC);
    }
    pango_layout_set_font_description (ctx->pangolayout, desc);
    pango_font_description_free (desc);
}

void
draw_init_font_normal (drawctx_t * const ctx) {
    pango_font_description_set_weight (ctx->font_style->font_desc, ctx->font_weight);
    pango_layout_set_font_description (ctx->pangolayout, ctx->font_style->font_desc);
}

float
draw_get_font_size (drawctx_t * const ctx) {
    draw_init_font (ctx, 0, 0);
    GdkScreen *screen = gdk_screen_get_default ();
    float dpi = gdk_screen_get_resolution (screen);
    PangoFontDescription *desc = ctx->font_style->font_desc;
    return (float)(pango_font_description_get_size (desc) / PANGO_SCALE * dpi / 72);
}

void
draw_text (drawctx_t * const ctx, float x, float y, int width, int align, const char *text) {
    draw_init_font (ctx, 0, 0);
    pango_layout_set_width (ctx->pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (ctx->pangolayout, get_pango_alignment (align));
    pango_layout_set_text (ctx->pangolayout, text, -1);
    cairo_move_to (ctx->drawable, x, y);
    pango_cairo_show_layout (ctx->drawable, ctx->pangolayout);
}

void
draw_text_custom (drawctx_t * const ctx, float x, float y, int width, int align, int type, int bold, int italic, const char *text) {
    draw_init_font (ctx, type, 0);
    if (bold || italic) {
        draw_init_font_style (ctx, bold, italic, type);
    }
    pango_layout_set_width (ctx->pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (ctx->pangolayout, get_pango_alignment (align));
    pango_layout_set_text (ctx->pangolayout, text, -1);
    cairo_move_to (ctx->drawable, x, y);
    pango_cairo_show_layout (ctx->drawable, ctx->pangolayout);
}

void
draw_text_with_colors (drawctx_t * const ctx, float x, float y, int width, int align, const char *text) {
    draw_init_font (ctx, 0, 0);
    pango_layout_set_width (ctx->pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (ctx->pangolayout, get_pango_alignment (align));
    pango_layout_set_text (ctx->pangolayout, text, -1);
//    gdk_draw_layout_with_colors (ctx->drawable, gc, x, y, ctx->pangolayout, &clrfg, &clrbg);
    cairo_move_to (ctx->drawable, x, y);
    pango_cairo_show_layout (ctx->drawable, ctx->pangolayout);
}

void
draw_get_layout_extents (drawctx_t * const ctx, int *w, int *h) {
    PangoRectangle log;
    pango_layout_get_pixel_extents (ctx->pangolayout, NULL, &log);
    if (w) {
        *w = log.width;
    }
    if (h) {
        *h = log.height;
    }
}

void
draw_get_text_extents (drawctx_t * const ctx, const char *text, int len, int *w, int *h) {
    draw_init_font (ctx, 0, 0);
    pango_layout_set_width (ctx->pangolayout, -1);
    pango_layout_set_alignment (ctx->pangolayout, PANGO_ALIGN_LEFT);
    pango_layout_set_text (ctx->pangolayout, text, len);
    draw_get_layout_extents (ctx, w, h);
}

int
draw_is_ellipsized (drawctx_t * const ctx) {
    return pango_layout_is_ellipsized (ctx->pangolayout);
}

const char *
draw_get_text (drawctx_t * const ctx) {
    return pango_layout_get_text (ctx->pangolayout);
}

int
draw_get_listview_rowheight (drawctx_t * const ctx) {
    PangoFontDescription *font_desc = pango_font_description_copy (pango_layout_get_font_description (ctx->pangolayout));
    PangoFontMetrics *metrics = pango_context_get_metrics (ctx->pangoctx,
            font_desc,
            pango_context_get_language (ctx->pangoctx));
    int row_height = (pango_font_metrics_get_ascent (metrics) +
            pango_font_metrics_get_descent (metrics));
    pango_font_metrics_unref (metrics);
    pango_font_description_free (font_desc);
    return PANGO_PIXELS(row_height)+6;
}

void
drawctx_init (drawctx_t * const ctx) {
    memset (ctx, 0, sizeof (drawctx_t));
    ctx->font_weight = PANGO_WEIGHT_NORMAL;
}

static GdkColor gtkui_bar_foreground_color;
static GdkColor gtkui_bar_background_color;

static GdkColor gtkui_tabstrip_dark_color;
static GdkColor gtkui_tabstrip_mid_color;
static GdkColor gtkui_tabstrip_light_color;
static GdkColor gtkui_tabstrip_base_color;
static GdkColor gtkui_tabstrip_text_color;
static GdkColor gtkui_tabstrip_playing_text_color;
static GdkColor gtkui_tabstrip_selected_text_color;

static GdkColor gtkui_listview_even_row_color;
static GdkColor gtkui_listview_odd_row_color;
static GdkColor gtkui_listview_selection_color;
static GdkColor gtkui_listview_text_color;
static GdkColor gtkui_listview_selected_text_color;
static GdkColor gtkui_listview_playing_text_color;
static GdkColor gtkui_listview_group_text_color;
static GdkColor gtkui_listview_column_text_color;
static GdkColor gtkui_listview_cursor_color;

static GdkColor gtkui_visualization_base_color;
static GdkColor gtkui_visualization_background_color;

static int override_listview_colors = 0;
static int override_bar_colors = 0;
static int override_tabstrip_colors = 0;
static int use_custom_visualization_color = 0;
static int use_custom_visualization_background_color = 0;

int
gtkui_listview_override_conf (const char *conf_str) {
    return !strcmp(conf_str, "gtkui.override_listview_colors");
}

int
gtkui_listview_font_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.font.listview", strlen("gtkui.font.listview"));
}

int
gtkui_listview_font_style_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.italic", strlen("gtkui.italic")) || !strncmp(conf_str, "gtkui.embolden", strlen("gtkui.embolden"));
}

int
gtkui_listview_colors_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.color.listview", strlen("gtkui.color.listview"));
}

int
gtkui_tabstrip_override_conf (const char *conf_str) {
    return !strcmp(conf_str, "gtkui.override_tabstrip_colors");
}

int
gtkui_tabstrip_font_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.font.tabstrip", strlen("gtkui.font.tabstrip"));
}

int
gtkui_tabstrip_font_style_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.tabstrip_italic", strlen("gtkui.tabstrip_italic")) || !strncmp(conf_str, "gtkui.tabstrip_embolden", strlen("gtkui.tabstrip_embolden"));
}

int
gtkui_tabstrip_colors_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.color.tabstrip", strlen("gtkui.color.tabstrip"));
}

int
gtkui_bar_override_conf (const char *conf_str) {
    return !strcmp(conf_str, "gtkui.override_bar_colors");
}

int
gtkui_bar_colors_conf (const char *conf_str) {
    return !strncmp(conf_str, "gtkui.color.bar", strlen("gtkui.color.bar"));
}

int
gtkui_override_listview_colors (void) {
    return override_listview_colors;
}

int
gtkui_override_bar_colors (void) {
    return override_bar_colors;
}

int
gtkui_override_tabstrip_colors (void) {
    return override_tabstrip_colors;
}

#if 0
static void
color_dump (const char *name, GdkColor *c) {
    printf ("%s: %x %x %x\n", name, c->red>>8, c->green>>8, c->blue>>8);
}

color_lerp_component (guint16 from, guint16 to, float factor) {
    int32_t result = (int32_t)from + (int32_t)(((int32_t)to - (int32_t)from) * factor);
    if (result < 0) {
        return 0;
    }
    else if (result > 0xffff) {
        return 0xffff;
    }
    return (guint16)result;
}

static GdkColor
color_lerp (GdkColor from, GdkColor to, float factor) {
    GdkColor result;
    result.red = color_lerp_component(from.red, to.red, factor);
    result.green = color_lerp_component(from.green, to.green, factor);
    result.blue = color_lerp_component(from.blue, to.blue, factor);
    return result;
}
#endif

static void
_init_color_for_name(GdkColor *color, const char *name, const GdkColor *def) {
    char color_text[100];
    snprintf (color_text, sizeof (color_text), "%hd %hd %hd", def->red, def->green, def->blue);
    const char *clr = deadbeef->conf_get_str_fast (name, color_text);
    sscanf (clr, "%hd %hd %hd", &color->red, &color->green, &color->blue);
}

void
gtkui_init_theme_colors (void) {
    if (!theme_entry) {
        theme_entry = gtk_entry_new ();
    }

    deadbeef->conf_lock ();
    override_listview_colors= deadbeef->conf_get_int ("gtkui.override_listview_colors", 0);
    override_bar_colors = deadbeef->conf_get_int ("gtkui.override_bar_colors", 0);
    override_tabstrip_colors = deadbeef->conf_get_int ("gtkui.override_tabstrip_colors", 0);
    use_custom_visualization_color = deadbeef->conf_get_int ("gtkui.vis.use_custom_base_color", 0);
    use_custom_visualization_background_color = deadbeef->conf_get_int ("gtkui.vis.use_custom_background_color", 0);

    extern GtkWidget *mainwin;
    GtkStyle *style = gtk_widget_get_style (mainwin);
    GtkStyle *entry_style = gtk_widget_get_style (theme_entry);
    char *font_name = pango_font_description_to_string (style->font_desc);

    // HACK: if gtk says selected color is the same as background -- set it
    // to a shade of blue
    int use_hardcoded_accent_color = memcmp (&style->bg[GTK_STATE_NORMAL], &gtkui_bar_foreground_color, sizeof (gtkui_bar_foreground_color));

    GdkColor hardcoded_accent_color = {
        .red = 0x2b84,
        .green = 0x7fff,
        .blue = 0xbae0,
    };

    if (use_hardcoded_accent_color) {
        memcpy (&gtkui_visualization_base_color, &hardcoded_accent_color, sizeof (GdkColor));
    }
    else {
        memcpy (&gtkui_visualization_base_color, &style->base[GTK_STATE_SELECTED], sizeof (GdkColor));
    }

    if (use_custom_visualization_color) {
        _init_color_for_name(&gtkui_visualization_base_color, "gtkui.vis.custom_base_color", &gtkui_visualization_base_color);
    }

    if (use_custom_visualization_background_color) {
        _init_color_for_name(&gtkui_visualization_background_color, "gtkui.vis.custom_background_color", &gtkui_visualization_background_color);
    }
    else {
        memcpy (&gtkui_visualization_background_color, &style->black, sizeof (GdkColor));
    }

    if (!override_bar_colors) {
        memcpy (&gtkui_bar_background_color, &style->text[GTK_STATE_NORMAL], sizeof (GdkColor));

        if (use_hardcoded_accent_color) {
            memcpy (&gtkui_bar_foreground_color, &hardcoded_accent_color, sizeof (GdkColor));
        }
        else {
            memcpy (&gtkui_bar_foreground_color, &style->base[GTK_STATE_SELECTED], sizeof (GdkColor));
        }
    }
    else {
        _init_color_for_name(&gtkui_bar_foreground_color, "gtkui.color.bar_foreground", &entry_style->base[GTK_STATE_SELECTED]);
        _init_color_for_name(&gtkui_bar_background_color, "gtkui.color.bar_background", &entry_style->fg[GTK_STATE_NORMAL]);
    }

    if (!override_tabstrip_colors) {
        memcpy (&gtkui_tabstrip_dark_color, &style->dark[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_mid_color, &style->mid[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_light_color, &style->light[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_base_color, &style->bg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_text_color, &style->text[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_playing_text_color, &style->text[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_tabstrip_selected_text_color, &style->text[GTK_STATE_NORMAL], sizeof (GdkColor));
        strncpy (gtkui_tabstrip_text_font, font_name, sizeof (gtkui_tabstrip_text_font));
    }
    else {
        _init_color_for_name(&gtkui_tabstrip_dark_color, "gtkui.color.tabstrip_dark", &style->dark[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_tabstrip_mid_color, "gtkui.color.tabstrip_mid", &style->mid[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_tabstrip_light_color, "gtkui.color.tabstrip_light", &style->light[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_tabstrip_base_color, "gtkui.color.tabstrip_base", &style->bg[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_tabstrip_text_color, "gtkui.color.tabstrip_text", &style->text[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_tabstrip_playing_text_color, "gtkui.color.tabstrip_playing_text", &style->text[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_tabstrip_selected_text_color, "gtkui.color.tabstrip_selected_text", &style->text[GTK_STATE_NORMAL]);

        strncpy (gtkui_tabstrip_text_font, deadbeef->conf_get_str_fast ("gtkui.font.tabstrip_text", font_name), sizeof (gtkui_tabstrip_text_font));
    }

    if (!override_listview_colors) {
        memcpy (&gtkui_listview_even_row_color, &style->light[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_odd_row_color, &style->mid[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_selection_color, &style->bg[GTK_STATE_SELECTED], sizeof (GdkColor));
        memcpy (&gtkui_listview_text_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_selected_text_color, &style->fg[GTK_STATE_SELECTED], sizeof (GdkColor));
        memcpy (&gtkui_listview_playing_text_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_group_text_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_column_text_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_listview_cursor_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        strncpy (gtkui_listview_text_font, font_name, sizeof (gtkui_listview_text_font));
        strncpy (gtkui_listview_group_text_font, font_name, sizeof (gtkui_listview_group_text_font));
        strncpy (gtkui_listview_column_text_font, font_name, sizeof (gtkui_listview_column_text_font));
    }
    else {
        _init_color_for_name(&gtkui_listview_even_row_color, "gtkui.color.listview_even_row", &style->light[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_listview_odd_row_color, "gtkui.color.listview_odd_row", &style->mid[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_listview_selection_color, "gtkui.color.listview_selection", &style->bg[GTK_STATE_SELECTED]);
        _init_color_for_name(&gtkui_listview_text_color, "gtkui.color.listview_text", &style->fg[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_listview_selected_text_color, "gtkui.color.listview_selected_text", &style->fg[GTK_STATE_SELECTED]);
        _init_color_for_name(&gtkui_listview_playing_text_color, "gtkui.color.listview_playing_text", &style->fg[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_listview_group_text_color, "gtkui.color.listview_group_text", &style->fg[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_listview_column_text_color, "gtkui.color.listview_column_text", &style->fg[GTK_STATE_NORMAL]);
        _init_color_for_name(&gtkui_listview_cursor_color, "gtkui.color.listview_cursor", &style->fg[GTK_STATE_SELECTED]);

        strncpy (gtkui_listview_text_font, deadbeef->conf_get_str_fast ("gtkui.font.listview_text", font_name), sizeof (gtkui_listview_text_font));
        strncpy (gtkui_listview_group_text_font, deadbeef->conf_get_str_fast ("gtkui.font.listview_group_text", font_name), sizeof (gtkui_listview_group_text_font));
        strncpy (gtkui_listview_column_text_font, deadbeef->conf_get_str_fast ("gtkui.font.listview_column_text", font_name), sizeof (gtkui_listview_column_text_font));
    }

    free (font_name);
    font_name = NULL;

    deadbeef->conf_unlock ();
}

void
gtkui_get_vis_custom_base_color (GdkColor *clr) {
    memcpy (clr, &gtkui_visualization_base_color, sizeof (GdkColor));
}

void
gtkui_get_vis_custom_background_color (GdkColor *clr) {
    memcpy (clr, &gtkui_visualization_background_color, sizeof (GdkColor));
}

void
gtkui_get_bar_foreground_color (GdkColor *clr) {
    memcpy (clr, &gtkui_bar_foreground_color, sizeof (GdkColor));
}

void
gtkui_get_bar_background_color (GdkColor *clr) {
    memcpy (clr, &gtkui_bar_background_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_dark_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_dark_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_mid_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_mid_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_light_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_light_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_base_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_base_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_text_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_playing_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_playing_text_color, sizeof (GdkColor));
}

void
gtkui_get_tabstrip_selected_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_tabstrip_selected_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_even_row_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_even_row_color, sizeof (GdkColor));
}

void
gtkui_get_listview_odd_row_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_odd_row_color, sizeof (GdkColor));
}

void
gtkui_get_listview_selection_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_selection_color, sizeof (GdkColor));
}

void
gtkui_get_listview_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_selected_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_selected_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_playing_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_playing_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_group_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_group_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_column_text_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_column_text_color, sizeof (GdkColor));
}

void
gtkui_get_listview_cursor_color (GdkColor *clr) {
    memcpy (clr, &gtkui_listview_cursor_color, sizeof (GdkColor));
}

const char *
gtkui_get_listview_text_font (void) {
    return gtkui_listview_text_font;
}

const char *
gtkui_get_listview_group_text_font (void) {
    return gtkui_listview_group_text_font;
}

const char *
gtkui_get_listview_column_text_font (void) {
    return gtkui_listview_column_text_font;
}

const char *
gtkui_get_tabstrip_text_font (void) {
    return gtkui_tabstrip_text_font;
}
