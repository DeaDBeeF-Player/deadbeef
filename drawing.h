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

uintptr_t
draw_load_pixbuf (const char *fname);

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
draw_text (float x, float y, const char *text);

void
draw_get_text_extents (const char *text, int len, int *w, int *h);

#endif // __DRAWING_H
