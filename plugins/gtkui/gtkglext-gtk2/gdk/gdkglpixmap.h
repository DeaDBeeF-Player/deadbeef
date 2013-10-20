/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2002-2004  Naofumi Yasufuku
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.
 */

#ifndef __GDK_GL_PIXMAP_H__
#define __GDK_GL_PIXMAP_H__

#include <gdk/gdkgldefs.h>
#include <gdk/gdkgltypes.h>

#include <gdk/gdkpixmap.h>

G_BEGIN_DECLS

typedef struct _GdkGLPixmapClass GdkGLPixmapClass;

#define GDK_TYPE_GL_PIXMAP              (gdk_gl_pixmap_get_type ())
#define GDK_GL_PIXMAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_PIXMAP, GdkGLPixmap))
#define GDK_GL_PIXMAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_PIXMAP, GdkGLPixmapClass))
#define GDK_IS_GL_PIXMAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_PIXMAP))
#define GDK_IS_GL_PIXMAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_PIXMAP))
#define GDK_GL_PIXMAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_PIXMAP, GdkGLPixmapClass))

struct _GdkGLPixmap
{
  GdkDrawable parent_instance;

  GdkDrawable *drawable;        /* Associated GdkPixmap */
};

struct _GdkGLPixmapClass
{
  GdkDrawableClass parent_class;
};

GType        gdk_gl_pixmap_get_type         (void);

/*
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 */
GdkGLPixmap *gdk_gl_pixmap_new              (GdkGLConfig *glconfig,
                                             GdkPixmap   *pixmap,
                                             const int   *attrib_list);

void         gdk_gl_pixmap_destroy          (GdkGLPixmap *glpixmap);

GdkPixmap   *gdk_gl_pixmap_get_pixmap       (GdkGLPixmap *glpixmap);

/*
 * OpenGL extension to GdkPixmap
 */

GdkGLPixmap *gdk_pixmap_set_gl_capability   (GdkPixmap   *pixmap,
                                             GdkGLConfig *glconfig,
                                             const int   *attrib_list);

void         gdk_pixmap_unset_gl_capability (GdkPixmap   *pixmap);

gboolean     gdk_pixmap_is_gl_capable       (GdkPixmap   *pixmap);

GdkGLPixmap *gdk_pixmap_get_gl_pixmap       (GdkPixmap   *pixmap);

#define      gdk_pixmap_get_gl_drawable(pixmap)         \
  GDK_GL_DRAWABLE (gdk_pixmap_get_gl_pixmap (pixmap))

G_END_DECLS

#endif /* __GDK_GL_PIXMAP_H__ */
