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
#ifndef __DRAWING_H
#define __DRAWING_H

#include <stdint.h>

// abstract api for drawing primitives

void
draw_init (void);

void
draw_free (void);

void
draw_begin (uintptr_t canvas);

void
draw_end (void);

void
draw_get_canvas_size (uintptr_t canvas, int *w, int *h);

void
draw_copy (uintptr_t dest_canvas, uintptr_t src_canvas, int dx, int dy, int sx, int sy, int w, int h);

void
draw_pixbuf (uintptr_t dest_canvas, uintptr_t pixbuf, int dx, int dy, int sx, int sy, int w, int h);

void
draw_set_fg_color (float *rgb);

void
draw_set_bg_color (float *rgb);

void
draw_line (float x1, float y1, float x2, float y2);

void
draw_rect (float x, float y, float w, float h, int fill);

float
draw_get_font_size (void);

void
draw_init_font (GtkStyle *style);

void
draw_init_font_bold (void);

void
draw_text (float x, float y, int width, int align, const char *text);

void
draw_text_with_colors (float x, float y, int width, int align, const char *text);

void
draw_get_text_extents (const char *text, int len, int *w, int *h);


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
