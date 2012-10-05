/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __DRAWING_H
#define __DRAWING_H

#include <stdint.h>

typedef struct {
    cairo_t *drawable;
    GdkColor clrfg;
    GdkColor clrbg;
    int pango_ready;
    PangoContext *pangoctx;
    PangoLayout *pangolayout;
    GtkStyle *font_style;
    PangoWeight font_weight;
} drawctx_t;

// abstract api for drawing primitives

void
drawctx_init (drawctx_t *ctx);

void
draw_begin (drawctx_t *ctx, cairo_t *cr);

void
draw_end (drawctx_t *ctx);

void
draw_free (drawctx_t *ctx);

void
draw_set_fg_color (drawctx_t *ctx, float *rgb);

void
draw_line (drawctx_t *ctx, float x1, float y1, float x2, float y2);

void
draw_rect (drawctx_t *ctx, float x, float y, float w, float h, int fill);

float
draw_get_font_size (drawctx_t *ctx);

void
draw_init_font (drawctx_t *ctx, GtkStyle *style);

void
draw_init_font_bold (drawctx_t *ctx);

void
draw_init_font_normal (drawctx_t *ctx);

void
draw_text (drawctx_t *ctx, float x, float y, int width, int align, const char *text);

void
draw_text_with_colors (drawctx_t *ctx, float x, float y, int width, int align, const char *text);

void
draw_get_text_extents (drawctx_t *ctx, const char *text, int len, int *w, int *h);

int
draw_get_listview_rowheight (drawctx_t *ctx);

void
gtkui_get_bar_foreground_color (GdkColor *clr);

void
gtkui_get_bar_background_color (GdkColor *clr);

void
gtkui_get_tabstrip_dark_color (GdkColor *clr);

void
gtkui_get_tabstrip_mid_color (GdkColor *clr);

void
gtkui_get_tabstrip_light_color (GdkColor *clr);

void
gtkui_get_tabstrip_base_color (GdkColor *clr);

void
gtkui_get_tabstrip_text_color (GdkColor *clr);

void
gtkui_get_listview_even_row_color (GdkColor *clr);

void
gtkui_get_listview_odd_row_color (GdkColor *clr);

void
gtkui_get_listview_selection_color (GdkColor *clr);

void
gtkui_get_listview_text_color (GdkColor *clr);

void
gtkui_get_listview_selected_text_color (GdkColor *clr);

void
gtkui_get_listview_cursor_color (GdkColor *clr);

void
gtkui_init_theme_colors (void);

int
gtkui_override_listview_colors (void);

int
gtkui_override_bar_colors (void);

int
gtkui_override_tabstrip_colors (void);

#endif // __DRAWING_H
