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

#ifndef __GDK_GL_WINDOW_X11_H__
#define __GDK_GL_WINDOW_X11_H__

#include <gdk/gdkglwindow.h>
#include <gdk/x11/gdkglx.h>

G_BEGIN_DECLS

typedef struct _GdkGLWindowImplX11      GdkGLWindowImplX11;
typedef struct _GdkGLWindowImplX11Class GdkGLWindowImplX11Class;

#define GDK_TYPE_GL_WINDOW_IMPL_X11              (gdk_gl_window_impl_x11_get_type ())
#define GDK_GL_WINDOW_IMPL_X11(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_WINDOW_IMPL_X11, GdkGLWindowImplX11))
#define GDK_GL_WINDOW_IMPL_X11_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_WINDOW_IMPL_X11, GdkGLWindowImplX11Class))
#define GDK_IS_GL_WINDOW_IMPL_X11(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_WINDOW_IMPL_X11))
#define GDK_IS_GL_WINDOW_IMPL_X11_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_WINDOW_IMPL_X11))
#define GDK_GL_WINDOW_IMPL_X11_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_WINDOW_IMPL_X11, GdkGLWindowImplX11Class))

struct _GdkGLWindowImplX11
{
  GdkGLWindow parent_instance;

  /* GLXWindow glxwindow; */
  Window glxwindow;

  GdkGLConfig *glconfig;

  guint is_destroyed : 1;
};

struct _GdkGLWindowImplX11Class
{
  GdkGLWindowClass parent_class;
};

GType gdk_gl_window_impl_x11_get_type (void);

G_END_DECLS

#endif /* __GDK_GL_WINDOW_X11_H__ */
