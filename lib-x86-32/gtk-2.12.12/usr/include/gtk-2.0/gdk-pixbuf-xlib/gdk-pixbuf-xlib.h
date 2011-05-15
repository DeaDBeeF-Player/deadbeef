/* GdkPixbuf library - Xlib header file
 *
 * Authors: John Harper <john@dcs.warwick.ac.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GDK_PIXBUF_XLIB_H
#define GDK_PIXBUF_XLIB_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlibrgb.h>
#include <X11/Xlib.h>



/* init */

void gdk_pixbuf_xlib_init (Display *display, int screen_num);

void gdk_pixbuf_xlib_init_with_depth (Display *display, int screen_num,
				      int prefDepth);



/* render */

void gdk_pixbuf_xlib_render_threshold_alpha (GdkPixbuf *pixbuf, Pixmap bitmap,
					     int src_x, int src_y,
					     int dest_x, int dest_y,
					     int width, int height,
					     int alpha_threshold);

void gdk_pixbuf_xlib_render_to_drawable (GdkPixbuf *pixbuf,
					 Drawable drawable, GC gc,
					 int src_x, int src_y,
					 int dest_x, int dest_y,
					 int width, int height,
					 XlibRgbDither dither,
					 int x_dither, int y_dither);


void gdk_pixbuf_xlib_render_to_drawable_alpha (GdkPixbuf *pixbuf,
					       Drawable drawable,
					       int src_x, int src_y,
					       int dest_x, int dest_y,
					       int width, int height,
					       GdkPixbufAlphaMode alpha_mode,
					       int alpha_threshold,
					       XlibRgbDither dither,
					       int x_dither, int y_dither);

void gdk_pixbuf_xlib_render_pixmap_and_mask (GdkPixbuf *pixbuf,
					     Pixmap *pixmap_return,
					     Pixmap *mask_return,
					     int alpha_threshold);



/* drawable */

GdkPixbuf *gdk_pixbuf_xlib_get_from_drawable (GdkPixbuf *dest,
					      Drawable src,
					      Colormap cmap, Visual *visual,
					      int src_x, int src_y,
					      int dest_x, int dest_y,
					      int width, int height);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* GDK_PIXBUF_XLIB_H */
