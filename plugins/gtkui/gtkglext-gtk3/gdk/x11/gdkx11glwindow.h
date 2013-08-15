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

#if !defined (__GDKGLX_H_INSIDE__) && !defined (GDK_GL_COMPILATION)
#error "Only <gdk/gdkglx.h> can be included directly."
#endif

#ifndef __GDK_X11_GL_WINDOW_H__
#define __GDK_X11_GL_WINDOW_H__

#include <gdk/gdkx.h>

#include <gdk/gdkgl.h>

G_BEGIN_DECLS

#define GDK_TYPE_X11_GL_WINDOW             (gdk_x11_gl_window_get_type ())
#define GDK_X11_GL_WINDOW(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_X11_GL_WINDOW, GdkX11GLWindow))
#define GDK_X11_GL_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_X11_GL_WINDOW, GdkX11GLWindowClass))
#define GDK_IS_X11_GL_WINDOW(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_X11_GL_WINDOW))
#define GDK_IS_X11_GL_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_X11_GL_WINDOW))
#define GDK_X11_GL_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_X11_GL_WINDOW, GdkX11GLWindowClass))

#ifdef INSIDE_GDK_GL_X11
typedef struct _GdkX11GLWindow GdkX11GLWindow;
#else
typedef GdkGLWindow GdkX11GLWindow;
#endif
typedef struct _GdkX11GLWindowClass GdkX11GLWindowClass;

GType         gdk_x11_gl_window_get_type (void);

Window        gdk_x11_gl_window_get_glxwindow     (GdkGLWindow  *glwindow);

#ifdef INSIDE_GDK_GL_X11

#define GDK_GL_WINDOW_GLXWINDOW(glwindow)      (GDK_GL_WINDOW_IMPL_X11 (glwindow->impl)->glxwindow)

#else

#define GDK_GL_WINDOW_GLXWINDOW(glwindow)      (gdk_x11_gl_window_get_glxwindow (glwindow))

#endif

G_END_DECLS

#endif /* __GDK_X11_GL_WINDOW_H__ */
