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

#ifndef __GDK_GL_CONFIG_IMPL_H__
#define __GDK_GL_CONFIG_IMPL_H__

#include <gdk/gdkgl.h>

G_BEGIN_DECLS

#define GDK_TYPE_GL_CONFIG_IMPL             (gdk_gl_config_impl_get_type ())
#define GDK_GL_CONFIG_IMPL(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_CONFIG_IMPL, GdkGLConfigImpl))
#define GDK_GL_CONFIG_IMPL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_CONFIG_IMPL, GdkGLConfigImplClass))
#define GDK_IS_GL_CONFIG_IMPL(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_CONFIG_IMPL))
#define GDK_IS_GL_CONFIG_IMPL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_CONFIG_IMPL))
#define GDK_GL_CONFIG_IMPL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_CONFIG_IMPL, GdkGLConfigImplClass))

typedef struct _GdkGLConfigImpl
{
  GObject parent;

  gint layer_plane;

  gint n_aux_buffers;

  gint n_sample_buffers;

  guint is_rgba            : 1;
  guint is_double_buffered : 1;
  guint as_single_mode     : 1;
  guint is_stereo          : 1;
  guint has_alpha          : 1;
  guint has_depth_buffer   : 1;
  guint has_stencil_buffer : 1;
  guint has_accum_buffer   : 1;
} GdkGLConfigImpl;

typedef struct _GdkGLConfigImplClass
{
  GObjectClass parent_class;

  GdkGLWindow* (*create_gl_window) (GdkGLConfig *glconfig,
                                    GdkWindow   *window,
                                    const int   *attrib_list);

  GdkScreen* (*get_screen) (GdkGLConfig *glconfig);
  gboolean   (*get_attrib) (GdkGLConfig *glconfig,
                            int          attribute,
                            int         *value);
  GdkVisual* (*get_visual) (GdkGLConfig *glconfig);
  gint       (*get_depth)  (GdkGLConfig *glconfig);

} GdkGLConfigImplClass;

GType gdk_gl_config_impl_get_type (void);

G_END_DECLS

#endif /* __GDK_GL_CONFIG_IMPL_H__ */
