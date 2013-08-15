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

#ifdef GDKGLEXT_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
#include <gdk/gdkwin32.h>
#endif

#include "gdkglprivate.h"
#include "gdkgldrawable.h"
#include "gdkglconfig.h"
#include "gdkglcontext.h"
#include "gdkglcontextimpl.h"

#ifdef GDKGLEXT_WINDOWING_X11
#include "x11/gdkglcontext-x11.h"
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
#include "win32/gdkglcontext-win32.h"
#endif

gboolean _gdk_gl_context_force_indirect = FALSE;

G_DEFINE_TYPE (GdkGLContext,    \
               gdk_gl_context,  \
               G_TYPE_OBJECT)

static void
gdk_gl_context_init (GdkGLContext *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_gl_context_class_init (GdkGLContextClass *klass)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

/**
 * gdk_gl_context_new:
 * @gldrawable: a #GdkGLDrawable.
 * @share_list: the #GdkGLContext with which to share display lists and texture
 *              objects. NULL indicates that no sharing is to take place.
 * @direct: whether rendering is to be done with a direct connection to
 *          the graphics system.
 * @render_type: GDK_GL_RGBA_TYPE.
 *
 * Creates a new OpenGL rendering context.
 *
 * Return value: the new #GdkGLContext.
 **/
GdkGLContext *
gdk_gl_context_new (GdkGLDrawable *gldrawable,
                    GdkGLContext  *share_list,
                    gboolean       direct,
                    int            render_type)
{
  g_return_val_if_fail (GDK_IS_GL_DRAWABLE (gldrawable), NULL);

  return GDK_GL_DRAWABLE_GET_CLASS (gldrawable)->create_gl_context (gldrawable,
                                                                    share_list,
                                                                    direct,
                                                                    render_type);
}

/**
 * gdk_gl_context_copy:
 * @glcontext: a #GdkGLContext.
 * @src: the source context.
 * @mask: which portions of @src state are to be copied to @glcontext.
 *
 * Copy state from @src rendering context to @glcontext.
 *
 * @mask contains the bitwise-OR of the same symbolic names that are passed to
 * the glPushAttrib() function. You can use GL_ALL_ATTRIB_BITS to copy all the
 * rendering state information.
 *
 * Return value: FALSE if it fails, TRUE otherwise.
 **/
gboolean
gdk_gl_context_copy (GdkGLContext  *glcontext,
                     GdkGLContext  *src,
                     unsigned long  mask)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->copy_gl_context_impl (glcontext,
                                                                                src,
                                                                                mask);
}

/**
 * gdk_gl_context_get_gl_drawable:
 * @glcontext: a #GdkGLContext.
 *
 * Gets #GdkGLDrawable to which the @glcontext is bound.
 *
 * Return value: the #GdkGLDrawable or NULL if no #GdkGLDrawable is bound.
 **/
GdkGLDrawable *
gdk_gl_context_get_gl_drawable (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->get_gl_drawable (glcontext);
}

/**
 * gdk_gl_context_get_gl_config:
 * @glcontext: a #GdkGLContext.
 *
 * Gets #GdkGLConfig with which the @glcontext is configured.
 *
 * Return value: the #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_context_get_gl_config (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->get_gl_config (glcontext);
}

/**
 * gdk_gl_context_get_share_list:
 * @glcontext: a #GdkGLContext.
 *
 * Gets #GdkGLContext with which the @glcontext shares the display lists and
 * texture objects.
 *
 * Return value: the #GdkGLContext.
 **/
GdkGLContext *
gdk_gl_context_get_share_list (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->get_share_list(glcontext);
}

/**
 * gdk_gl_context_is_direct:
 * @glcontext: a #GdkGLContext.
 *
 * Returns whether the @glcontext is a direct rendering context.
 *
 * Return value: TRUE if the @glcontext is a direct rendering contest.
 **/
gboolean
gdk_gl_context_is_direct (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->is_direct(glcontext);
}

/**
 * gdk_gl_context_get_render_type:
 * @glcontext: a #GdkGLContext.
 *
 * Gets render_type of the @glcontext.
 *
 * Return value: GDK_GL_RGBA_TYPE.
 **/
int
gdk_gl_context_get_render_type (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->get_render_type(glcontext);
}

gboolean
gdk_gl_context_make_current(GdkGLContext  *glcontext,
                            GdkGLDrawable *draw,
                            GdkGLDrawable *read)
{
  g_return_val_if_fail (GDK_IS_GL_CONTEXT (glcontext), FALSE);

  return GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->make_current(glcontext,
                                                                       draw,
                                                                       read);
}

/**
 * gdk_gl_context_release_current:
 *
 * Releases the current #GdkGLContext.
 **/
void
gdk_gl_context_release_current ()
{
  GdkGLContext *glcontext = gdk_gl_context_get_current();

  g_return_if_fail(glcontext != NULL);

  if (GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->make_uncurrent)
    GDK_GL_CONTEXT_IMPL_GET_CLASS (glcontext->impl)->make_uncurrent(glcontext);
}

/**
 * gdk_gl_context_get_current:
 *
 * Returns the current #GdkGLContext.
 *
 * Return value: the current #GdkGLContext or NULL if there is no current
 *               context.
 **/
GdkGLContext *
gdk_gl_context_get_current ()
{
  /*
   * There can only be one current context. So we try each target
   * and take the first non-NULL result. Hopefully the underlying
   * GL implementation behaves accordingly. But we probalby need
   * a better strategy here.
   */

  GdkGLContext *current = NULL;

#ifdef GDKGLEXT_WINDOWING_X11
    current = _gdk_x11_gl_context_impl_get_current();
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
  if (current == NULL)
    {
      current = _gdk_win32_gl_context_impl_get_current();
    }
#endif

  return current;
}
