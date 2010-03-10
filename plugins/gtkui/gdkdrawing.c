/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
//#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "drawing.h"
#include "support.h"
#include "gtkui.h"

static GdkDrawable *drawable;
static GdkGC *gc;
static GdkColor clrfg;
static GdkColor clrbg;
static int pango_ready;
static PangoContext *pangoctx;
static PangoLayout *pangolayout;

void
draw_begin (uintptr_t canvas) {
    drawable = GDK_DRAWABLE (canvas);
    gc = gdk_gc_new (drawable);
}

void
draw_end (void) {
//    if (pango_ready) {
//        g_object_unref (pangolayout);
//        g_object_unref (pangoctx);
//        pango_ready = 0;
//    }
    drawable = NULL;
    if (gc) {
        g_object_unref (gc);
        gc = NULL;
    }
}

void
draw_copy (uintptr_t dest_canvas, uintptr_t src_canvas, int dx, int dy, int sx, int sy, int w, int h) {
    gdk_draw_drawable (GDK_DRAWABLE (dest_canvas), gc, GDK_DRAWABLE (src_canvas), dx, dy, sx, sy, w, h);
}

void
draw_pixbuf (uintptr_t dest_canvas, uintptr_t pixbuf, int dx, int dy, int sx, int sy, int w, int h) {
    gdk_pixbuf_render_to_drawable (GDK_PIXBUF (pixbuf), GDK_DRAWABLE (dest_canvas), gc, sx, sy, dx, dy, w, h, GDK_RGB_DITHER_NONE, 0, 0);
}

void
draw_get_canvas_size (uintptr_t canvas, int *w, int *h) {
	gdk_drawable_get_size (GDK_DRAWABLE (canvas), w, h);
}

void
draw_set_fg_color (float *rgb) {
    clrfg.red = rgb[0] * 0xffff;
    clrfg.green = rgb[1] * 0xffff;
    clrfg.blue = rgb[2] * 0xffff;
    gdk_gc_set_rgb_fg_color (gc, &clrfg);
}

void
draw_set_bg_color (float *rgb) {
    clrbg.red = rgb[0] * 0xffff;
    clrbg.green = rgb[1] * 0xffff;
    clrbg.blue = rgb[2] * 0xffff;
    gdk_gc_set_rgb_bg_color (gc, &clrbg);
}

void
draw_line (float x1, float y1, float x2, float y2) {
    gdk_draw_line (drawable, gc, x1, y1, x2, y2);
}

void
draw_rect (float x, float y, float w, float h, int fill) {
    gdk_draw_rectangle (drawable, gc, fill, x, y, w, h);
}

static inline void
draw_init_font (void) {
    if (!pango_ready) {
        pangoctx = gdk_pango_context_get ();
        pangolayout = pango_layout_new (pangoctx);
        pango_layout_set_ellipsize (pangolayout, PANGO_ELLIPSIZE_END);
        GtkStyle *style = gtk_widget_get_default_style ();
        PangoFontDescription *desc = style->font_desc;
        pango_layout_set_font_description (pangolayout, desc);
        pango_ready = 1;
    }
}

float
draw_get_font_size (void) {
    GdkScreen *screen = gdk_screen_get_default ();
    float dpi = gdk_screen_get_resolution (screen);
    GtkStyle *style = gtk_widget_get_default_style ();
    PangoFontDescription *desc = style->font_desc;
    return (float)(pango_font_description_get_size (desc) / PANGO_SCALE * dpi / 72);
}

void
draw_text (float x, float y, int width, int align, const char *text) {
    draw_init_font ();
    pango_layout_set_width (pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (pangolayout, align ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);
    pango_layout_set_text (pangolayout, text, -1);
    gdk_draw_layout (drawable, gc, x, y, pangolayout);
}

void
draw_text_with_colors (float x, float y, int width, int align, const char *text) {
    draw_init_font ();
    pango_layout_set_width (pangolayout, width*PANGO_SCALE);
    pango_layout_set_alignment (pangolayout, align ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);
    pango_layout_set_text (pangolayout, text, -1);
    gdk_draw_layout_with_colors (drawable, gc, x, y, pangolayout, &clrfg, &clrbg);
}

void
draw_get_text_extents (const char *text, int len, int *w, int *h) {
    draw_init_font ();
    pango_layout_set_width (pangolayout, 1000 * PANGO_SCALE);
    pango_layout_set_alignment (pangolayout, PANGO_ALIGN_LEFT);
    pango_layout_set_text (pangolayout, text, len);
    PangoRectangle ink;
    PangoRectangle log;
    pango_layout_get_pixel_extents (pangolayout, &ink, &log);
    *w = ink.width;
    *h = ink.height;
}

static GdkColor gtkui_back_color;
static GdkColor gtkui_selection_color;
static GdkColor gtkui_dark_color;
static GdkColor gtkui_mid_color;
static GdkColor gtkui_light_color;
static GdkColor gtkui_even_row_color;
static GdkColor gtkui_odd_row_color;
static GdkColor gtkui_text_color;
static GdkColor gtkui_selected_text_color;

void
gtkui_init_theme_colors (void) {
    int override = deadbeef->conf_get_int ("gtkui.override_theme_colors", 0);

    extern GtkWidget *mainwin;
    GtkStyle *style = mainwin->style;
    char color_text[100];
    const char *clr;

    if (!override) {
        memcpy (&gtkui_selection_color, &style->base[GTK_STATE_SELECTED], sizeof (GdkColor));
        memcpy (&gtkui_back_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_dark_color, &style->dark[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_mid_color, &style->mid[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_light_color, &style->light[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_even_row_color, &style->light[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_odd_row_color, &style->mid[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_text_color, &style->fg[GTK_STATE_NORMAL], sizeof (GdkColor));
        memcpy (&gtkui_selected_text_color, &style->fg[GTK_STATE_SELECTED], sizeof (GdkColor));
    }
    else {
        snprintf (color_text, sizeof (color_text), "%d %d %d", style->base[GTK_STATE_SELECTED].red, style->base[GTK_STATE_SELECTED].green, style->base[GTK_STATE_SELECTED].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.selection", color_text);
        sscanf (clr, "%d %d %d", &gtkui_selection_color.red, &gtkui_selection_color.green, &gtkui_selection_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->fg[GTK_STATE_NORMAL].red, style->fg[GTK_STATE_NORMAL].green, style->fg[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.back", color_text);
        sscanf (clr, "%d %d %d", &gtkui_back_color.red, &gtkui_back_color.green, &gtkui_back_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->dark[GTK_STATE_NORMAL].red, style->dark[GTK_STATE_NORMAL].green, style->dark[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.dark", color_text);
        sscanf (clr, "%d %d %d", &gtkui_dark_color.red, &gtkui_dark_color.green, &gtkui_dark_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->mid[GTK_STATE_NORMAL].red, style->mid[GTK_STATE_NORMAL].green, style->mid[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.mid", color_text);
        sscanf (clr, "%d %d %d", &gtkui_mid_color.red, &gtkui_mid_color.green, &gtkui_mid_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->light[GTK_STATE_NORMAL].red, style->light[GTK_STATE_NORMAL].green, style->light[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.light", color_text);
        sscanf (clr, "%d %d %d", &gtkui_light_color.red, &gtkui_light_color.green, &gtkui_light_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->light[GTK_STATE_NORMAL].red, style->light[GTK_STATE_NORMAL].green, style->light[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.even_row", color_text);
        sscanf (clr, "%d %d %d", &gtkui_even_row_color.red, &gtkui_even_row_color.green, &gtkui_even_row_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->mid[GTK_STATE_NORMAL].red, style->mid[GTK_STATE_NORMAL].green, style->mid[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.odd_row", color_text);
        sscanf (clr, "%d %d %d", &gtkui_odd_row_color.red, &gtkui_odd_row_color.green, &gtkui_odd_row_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->fg[GTK_STATE_NORMAL].red, style->fg[GTK_STATE_NORMAL].green, style->fg[GTK_STATE_NORMAL].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.text", color_text);
        sscanf (clr, "%d %d %d", &gtkui_text_color.red, &gtkui_text_color.green, &gtkui_text_color.blue);

        snprintf (color_text, sizeof (color_text), "%d %d %d", style->fg[GTK_STATE_SELECTED].red, style->fg[GTK_STATE_SELECTED].green, style->fg[GTK_STATE_SELECTED].blue);
        clr = deadbeef->conf_get_str ("gtkui.color.selected_text", color_text);
        sscanf (clr, "%d %d %d", &gtkui_selected_text_color.red, &gtkui_selected_text_color.green, &gtkui_selected_text_color.blue);
    }
}

GdkColor *
gtkui_get_back_color (void) {
    return &gtkui_back_color;
}

GdkColor *
gtkui_get_selection_color (void) {
    return &gtkui_selection_color;
}

GdkColor *
gtkui_get_dark_color (void) {
    return &gtkui_dark_color;
}

GdkColor *
gtkui_get_mid_color (void) {
    return &gtkui_mid_color;
}

GdkColor *
gtkui_get_light_color (void) {
    return &gtkui_light_color;
}

GdkColor *
gtkui_get_even_row_color (void) {
    return &gtkui_even_row_color;
}

GdkColor *
gtkui_get_odd_row_color (void) {
    return &gtkui_odd_row_color;
}

GdkColor *
gtkui_get_text_color (void) {
    return &gtkui_text_color;
}

GdkColor *
gtkui_get_selected_text_color (void) {
    return &gtkui_selected_text_color;
}
