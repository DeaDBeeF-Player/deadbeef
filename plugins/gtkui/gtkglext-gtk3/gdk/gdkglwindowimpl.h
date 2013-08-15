/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2012  Thomas Zimmermann
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

#ifndef __GDK_GL_WINDOW_IMPL_H__
#define __GDK_GL_WINDOW_IMPL_H__

#include <gdk/gdkgl.h>

G_BEGIN_DECLS

#define GDK_TYPE_GL_WINDOW_IMPL             (gdk_gl_window_impl_get_type ())
#define GDK_GL_WINDOW_IMPL(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_WINDOW_IMPL, GdkGLWindowImpl))
#define GDK_GL_WINDOW_IMPL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_WINDOW_IMPL, GdkGLWindowImplClass))
#define GDK_IS_GL_WINDOW_IMPL(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_WINDOW_IMPL))
#define GDK_IS_GL_WINDOW_IMPL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_WINDOW_IMPL))
#define GDK_GL_WINDOW_IMPL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_WINDOW_IMPL, GdkGLWindowImplClass))

typedef struct _GdkGLWindowImpl
{
  GObject parent;
} GdkGLWindowImpl;

typedef struct _GdkGLWindowImplClass
{
  GObjectClass parent_class;

  GdkGLContext* (*create_gl_context)      (GdkGLWindow  *gldrawable,
                                           GdkGLContext *share_list,
                                           gboolean      direct,
                                           int           render_type);
  gboolean      (*is_double_buffered)     (GdkGLWindow *glwindow);
  void          (*swap_buffers)           (GdkGLWindow *glwindow);
  void          (*wait_gl)                (GdkGLWindow *glwindow);
  void          (*wait_gdk)               (GdkGLWindow *glwindow);
  GdkGLConfig*  (*get_gl_config)          (GdkGLWindow *glwindow);
  void          (*destroy_gl_window_impl) (GdkGLWindow *glwindow);
} GdkGLWindowImplClass;

GType gdk_gl_window_impl_get_type (void);

G_END_DECLS

#endif /* __GDK_GL_WINDOW_IMPL_H__ */
