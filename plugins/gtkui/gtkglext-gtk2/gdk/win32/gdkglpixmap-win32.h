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

#ifndef __GDK_GL_PIXMAP_WIN32_H__
#define __GDK_GL_PIXMAP_WIN32_H__

#include <gdk/gdkglpixmap.h>
#include <gdk/win32/gdkglwin32.h>

G_BEGIN_DECLS

typedef struct _GdkGLPixmapImplWin32      GdkGLPixmapImplWin32;
typedef struct _GdkGLPixmapImplWin32Class GdkGLPixmapImplWin32Class;

#define GDK_TYPE_GL_PIXMAP_IMPL_WIN32              (gdk_gl_pixmap_impl_win32_get_type ())
#define GDK_GL_PIXMAP_IMPL_WIN32(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_PIXMAP_IMPL_WIN32, GdkGLPixmapImplWin32))
#define GDK_GL_PIXMAP_IMPL_WIN32_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_PIXMAP_IMPL_WIN32, GdkGLPixmapImplWin32Class))
#define GDK_IS_GL_PIXMAP_IMPL_WIN32(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_PIXMAP_IMPL_WIN32))
#define GDK_IS_GL_PIXMAP_IMPL_WIN32_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_PIXMAP_IMPL_WIN32))
#define GDK_GL_PIXMAP_IMPL_WIN32_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_PIXMAP_IMPL_WIN32, GdkGLPixmapImplWin32Class))

struct _GdkGLPixmapImplWin32
{
  GdkGLPixmap parent_instance;

  GdkPixmap *pixmap_gl;

  int width;
  int height;

  PIXELFORMATDESCRIPTOR pfd;
  int pixel_format;

  GdkGLConfig *glconfig;

  /*< private >*/
  HDC hdc_gl;

  HDC hdc_gdk;
  HBITMAP hbitmap_gdk;

  guint is_destroyed : 1;
};

struct _GdkGLPixmapImplWin32Class
{
  GdkGLPixmapClass parent_class;
};

GType gdk_gl_pixmap_impl_win32_get_type (void);

#define GDK_GL_PIXMAP_IMPL_WIN32_HDC_GET(impl) \
  ( (impl)->hdc_gl )

#define GDK_GL_PIXMAP_IMPL_WIN32_HDC_RELEASE(impl) \
  ( (void) 0 )

G_END_DECLS

#endif /* __GDK_GL_PIXMAP_WIN32_H__ */
