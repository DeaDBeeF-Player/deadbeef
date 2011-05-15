/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GDK_IMAGE_H__
#define __GDK_IMAGE_H__

#include <gdk/gdktypes.h>

G_BEGIN_DECLS

/* Types of images.
 *   Normal: Normal X image type. These are slow as they involve passing
 *	     the entire image through the X connection each time a draw
 *	     request is required. On Win32, a bitmap.
 *   Shared: Shared memory X image type. These are fast as the X server
 *	     and the program actually use the same piece of memory. They
 *	     should be used with care though as there is the possibility
 *	     for both the X server and the program to be reading/writing
 *	     the image simultaneously and producing undesired results.
 *	     On Win32, also a bitmap.
 */
typedef enum
{
  GDK_IMAGE_NORMAL,
  GDK_IMAGE_SHARED,
  GDK_IMAGE_FASTEST
} GdkImageType;

typedef struct _GdkImageClass GdkImageClass;

#define GDK_TYPE_IMAGE              (gdk_image_get_type ())
#define GDK_IMAGE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_IMAGE, GdkImage))
#define GDK_IMAGE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_IMAGE, GdkImageClass))
#define GDK_IS_IMAGE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_IMAGE))
#define GDK_IS_IMAGE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_IMAGE))
#define GDK_IMAGE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_IMAGE, GdkImageClass))

struct _GdkImage
{
  GObject parent_instance;

  /*< public >*/
  
  GdkImageType	type; /* read only. */
  GdkVisual    *visual;	    /* read only. visual used to create the image */
  GdkByteOrder	byte_order; /* read only. */
  gint		width; /* read only. */
  gint		height; /* read only. */
  guint16	depth; /* read only. */
  guint16	bpp;	        /* read only. bytes per pixel */
  guint16	bpl;	        /* read only. bytes per line */
  guint16       bits_per_pixel; /* read only. bits per pixel */
  gpointer	mem;

  GdkColormap  *colormap; /* read only. */

  /*< private >*/
  gpointer windowing_data; /* read only. */
};

struct _GdkImageClass
{
  GObjectClass parent_class;
};

GType     gdk_image_get_type   (void) G_GNUC_CONST;

GdkImage*  gdk_image_new       (GdkImageType  type,
				GdkVisual    *visual,
				gint	      width,
				gint	      height);

#ifndef GDK_DISABLE_DEPRECATED
GdkImage*  gdk_image_get       (GdkDrawable  *drawable,
				gint	      x,
				gint	      y,
				gint	      width,
				gint	      height);

GdkImage * gdk_image_ref       (GdkImage     *image);
void       gdk_image_unref     (GdkImage     *image);
#endif

void	   gdk_image_put_pixel (GdkImage     *image,
				gint	      x,
				gint	      y,
				guint32	      pixel);
guint32	   gdk_image_get_pixel (GdkImage     *image,
				gint	      x,
				gint	      y);

void       gdk_image_set_colormap (GdkImage    *image,
                                   GdkColormap *colormap);
GdkColormap* gdk_image_get_colormap (GdkImage    *image);


#ifdef GDK_ENABLE_BROKEN
GdkImage* gdk_image_new_bitmap (GdkVisual     *visual,
				gpointer      data,
				gint          width,
				gint          height);
#endif /* GDK_ENABLE_BROKEN */

#ifndef GDK_DISABLE_DEPRECATED
#define gdk_image_destroy              gdk_image_unref
#endif /* GDK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __GDK_IMAGE_H__ */
