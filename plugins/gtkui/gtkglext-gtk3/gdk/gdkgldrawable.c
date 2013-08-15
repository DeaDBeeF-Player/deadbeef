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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <gdk/gdk.h>

#include "gdkglprivate.h"
#include "gdkglcontext.h"
#include "gdkgldrawable.h"

GType
gdk_gl_drawable_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLDrawableClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL
      };

      type = g_type_register_static (G_TYPE_INTERFACE,
                                     "GdkGLDrawable",
                                     &type_info, 0);
    }

  return type;
}

/**
 * gdk_gl_drawable_is_double_buffered:
 * @gldrawable: a #GdkGLDrawable.
 *
 * Returns whether the @gldrawable supports the double-buffered visual.
 *
 * Return value: TRUE if the double-buffered visual is supported,
 *               FALSE otherwise.
 **/
gboolean
gdk_gl_drawable_is_double_buffered (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_DRAWABLE (gldrawable), FALSE);

  return GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->is_double_buffered (gldrawable);
}

/**
 * gdk_gl_drawable_swap_buffers:
 * @gldrawable: a #GdkGLDrawable.
 *
 * Exchange front and back buffers.
 *
 **/
void
gdk_gl_drawable_swap_buffers (GdkGLDrawable *gldrawable)
{
  g_return_if_fail (GDK_IS_GL_DRAWABLE (gldrawable));

  GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->swap_buffers (gldrawable);
}

/**
 * gdk_gl_drawable_wait_gl:
 * @gldrawable: a #GdkGLDrawable.
 *
 * Complete OpenGL execution prior to subsequent GDK drawing calls.
 *
 **/
void
gdk_gl_drawable_wait_gl (GdkGLDrawable *gldrawable)
{
  g_return_if_fail (GDK_IS_GL_DRAWABLE (gldrawable));

  GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->wait_gl (gldrawable);
}

/**
 * gdk_gl_drawable_wait_gdk:
 * @gldrawable: a #GdkGLDrawable.
 *
 * Complete GDK drawing execution prior to subsequent OpenGL calls.
 *
 **/
void
gdk_gl_drawable_wait_gdk (GdkGLDrawable *gldrawable)
{
  g_return_if_fail (GDK_IS_GL_DRAWABLE (gldrawable));

  GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->wait_gdk (gldrawable);
}

/**
 * gdk_gl_drawable_get_gl_config:
 * @gldrawable: a #GdkGLDrawable.
 *
 * Gets #GdkGLConfig with which the @gldrawable is configured.
 *
 * Return value: the #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_drawable_get_gl_config (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_DRAWABLE (gldrawable), NULL);

  return GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->get_gl_config (gldrawable);
}
