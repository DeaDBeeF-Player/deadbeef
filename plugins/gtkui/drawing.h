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

#ifndef __DRAWING_H
#define __DRAWING_H

#include <stdint.h>
#include <gtk/gtk.h>

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

enum {
    DDB_LIST_FONT = 0,
    DDB_GROUP_FONT = 1,
    DDB_TABSTRIP_FONT = 2,
    DDB_COLUMN_FONT = 3,
    DDB_SEEKBAR_FONT = 4,
};

void
drawctx_init (drawctx_t *const ctx);

void
draw_begin (drawctx_t *const ctx, cairo_t *cr);

void
draw_end (drawctx_t *const ctx);

void
draw_free (drawctx_t *const ctx);

void
draw_set_fg_color (drawctx_t *const ctx, float *rgb);

void
draw_line (drawctx_t *const ctx, float x1, float y1, float x2, float y2);

void
draw_rect (drawctx_t *const ctx, float x, float y, float w, float h, int fill);

float
draw_get_font_size (drawctx_t *const ctx);

void
draw_init_font (drawctx_t *const ctx, int type, int reset);

void
draw_init_font_style (drawctx_t *const ctx, int bold, int italic, int type);

void
draw_init_font_normal (drawctx_t *const ctx);

void
draw_text_custom (
    drawctx_t *const ctx,
    float x,
    float y,
    int width,
    int align,
    int type,
    int bold,
    int italic,
    const char *text);

void
draw_text_with_colors (drawctx_t *const ctx, float x, float y, int width, int align, const char *text);

void
draw_get_layout_extents (drawctx_t *const ctx, int *w, int *h);

void
draw_get_text_extents (drawctx_t *const ctx, const char *text, int len, int *w, int *h);

int
draw_is_ellipsized (drawctx_t *const ctx);

const char *
draw_get_text (drawctx_t *const ctx);

int
draw_get_listview_rowheight (drawctx_t *const ctx);

int
gtkui_listview_override_conf (const char *conf_str);

int
gtkui_listview_font_conf (const char *conf_str);

int
gtkui_listview_font_style_conf (const char *conf_str);

int
gtkui_listview_colors_conf (const char *conf_str);

int
gtkui_tabstrip_override_conf (const char *conf_str);

int
gtkui_tabstrip_colors_conf (const char *conf_str);

int
gtkui_tabstrip_font_conf (const char *conf_str);

int
gtkui_tabstrip_font_style_conf (const char *conf_str);

int
gtkui_bar_override_conf (const char *conf_str);

int
gtkui_bar_colors_conf (const char *conf_str);

void
gtkui_get_vis_custom_base_color (GdkColor *clr);

void
gtkui_get_vis_custom_background_color (GdkColor *clr);

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
gtkui_get_tabstrip_playing_text_color (GdkColor *clr);

void
gtkui_get_tabstrip_selected_text_color (GdkColor *clr);

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
gtkui_get_listview_playing_text_color (GdkColor *clr);

void
gtkui_get_listview_playing_row_color (GdkColor *clr);

void
gtkui_get_listview_group_text_color (GdkColor *clr);

void
gtkui_get_listview_column_text_color (GdkColor *clr);

void
gtkui_get_listview_cursor_color (GdkColor *clr);

const char *
gtkui_get_listview_text_font (void);

const char *
gtkui_get_listview_group_text_font (void);

const char *
gtkui_get_listview_column_text_font (void);

void
gtkui_init_theme_colors (void);

int
gtkui_override_listview_colors (void);

int
gtkui_override_bar_colors (void);

int
gtkui_override_tabstrip_colors (void);

const char *
gtkui_get_tabstrip_text_font (void);

#endif // __DRAWING_H
