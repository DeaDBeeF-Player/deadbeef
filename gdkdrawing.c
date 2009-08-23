#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "drawing.h"
#include "support.h"

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
    if (pango_ready) {
        g_object_unref (pangolayout);
        g_object_unref (pangoctx);
        pango_ready = 0;
    }
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

uintptr_t
draw_load_pixbuf (const char *fname) {
    return (uintptr_t)create_pixbuf (fname);
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
    return (float)pango_font_description_get_size (desc) / PANGO_SCALE * dpi / 72;
}

void
draw_text (float x, float y, const char *text) {
    draw_init_font ();
    pango_layout_set_text (pangolayout, text, -1);
    gdk_draw_layout (drawable, gc, x, y, pangolayout);
}

void
draw_text_with_colors (float x, float y, const char *text) {
    draw_init_font ();
    pango_layout_set_text (pangolayout, text, -1);
    gdk_draw_layout_with_colors (drawable, gc, x, y, pangolayout, &clrfg, &clrbg);
}

void
draw_get_text_extents (const char *text, int len, int *w, int *h) {
    draw_init_font ();
    pango_layout_set_text (pangolayout, text, len);
    PangoRectangle ext;
    pango_layout_get_pixel_extents (pangolayout, &ext, NULL);
    *w = ext.width;
    *h = ext.height;
}
