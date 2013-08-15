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

#if !defined (__GDKGL_H_INSIDE__) && !defined (GDK_GL_COMPILATION)
#error "Only <gdk/gdkgl.h> can be included directly."
#endif

#ifndef __GDK_GL_DRAWABLE_H__
#define __GDK_GL_DRAWABLE_H__

#include <gdk/gdkgldefs.h>
#include <gdk/gdkgltypes.h>

G_BEGIN_DECLS

typedef struct _GdkGLDrawableClass GdkGLDrawableClass;

#define GDK_TYPE_GL_DRAWABLE		  (gdk_gl_drawable_get_type ())
#define GDK_GL_DRAWABLE(inst)		  (G_TYPE_CHECK_INSTANCE_CAST ((inst), GDK_TYPE_GL_DRAWABLE, GdkGLDrawable))
#define GDK_GL_DRAWABLE_CLASS(vtable)	  (G_TYPE_CHECK_CLASS_CAST ((vtable), GDK_TYPE_GL_DRAWABLE, GdkGLDrawableClass))
#define GDK_IS_GL_DRAWABLE(inst)	  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), GDK_TYPE_GL_DRAWABLE))
#define GDK_IS_GL_DRAWABLE_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), GDK_TYPE_GL_DRAWABLE))
#define GDK_GL_DRAWABLE_GET_CLASS(inst)	  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GDK_TYPE_GL_DRAWABLE, GdkGLDrawableClass))

struct _GdkGLDrawableClass
{
  GTypeInterface base_iface;

  GdkGLContext* (*create_gl_context) (GdkGLDrawable *gldrawable,
                                      GdkGLContext  *share_list,
                                      gboolean       direct,
                                      int            render_type);

  gboolean      (*is_double_buffered)   (GdkGLDrawable *gldrawable);
  void          (*swap_buffers)         (GdkGLDrawable *gldrawable);
  void          (*wait_gl)              (GdkGLDrawable *gldrawable);
  void          (*wait_gdk)             (GdkGLDrawable *gldrawable);
  GdkGLConfig*  (*get_gl_config)        (GdkGLDrawable *gldrawable);
};

GType          gdk_gl_drawable_get_type           (void);

gboolean       gdk_gl_drawable_is_double_buffered (GdkGLDrawable *gldrawable);

void           gdk_gl_drawable_swap_buffers       (GdkGLDrawable *gldrawable);

void           gdk_gl_drawable_wait_gl            (GdkGLDrawable *gldrawable);

void           gdk_gl_drawable_wait_gdk           (GdkGLDrawable *gldrawable);

GdkGLConfig   *gdk_gl_drawable_get_gl_config      (GdkGLDrawable *gldrawable);

G_END_DECLS

#endif /* __GDK_GL_DRAWABLE_H__ */
