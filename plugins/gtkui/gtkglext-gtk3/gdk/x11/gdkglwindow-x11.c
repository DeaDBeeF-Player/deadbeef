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

#include <string.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglcontext-x11.h"
#include "gdkglwindow-x11.h"

#include <gdk/gdkglquery.h>

static GdkGLContext *_gdk_x11_gl_window_impl_create_gl_context  (GdkGLWindow  *glwindow,
                                                                 GdkGLContext *share_list,
                                                                 gboolean      direct,
                                                                 int           render_type);
static gboolean     _gdk_x11_gl_window_impl_is_double_buffered  (GdkGLWindow  *glwindow);
static void         _gdk_x11_gl_window_impl_swap_buffers        (GdkGLWindow  *glwindow);
static void         _gdk_x11_gl_window_impl_wait_gl             (GdkGLWindow  *glwindow);
static void         _gdk_x11_gl_window_impl_wait_gdk            (GdkGLWindow  *glwindow);
static GdkGLConfig *_gdk_x11_gl_window_impl_get_gl_config       (GdkGLWindow  *glwindow);
static Window       _gdk_x11_gl_window_impl_get_glxwindow       (GdkGLWindow  *glwindow);

G_DEFINE_TYPE (GdkGLWindowImplX11,
               gdk_gl_window_impl_x11,
               GDK_TYPE_GL_WINDOW_IMPL);

static void
gdk_gl_window_impl_x11_init (GdkGLWindowImplX11 *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  self->glxwindow = None;
  self->glconfig = NULL;
  self->is_destroyed = 0;
}

static void
_gdk_x11_gl_window_impl_destroy (GdkGLWindow *glwindow)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  GdkGLWindowImplX11 *x11_impl = GDK_GL_WINDOW_IMPL_X11 (glwindow->impl);
  Display *xdisplay;
  Bool (APIENTRY *ReleaseBuffersMESA) (Display*, GLXDrawable);

  if (x11_impl->is_destroyed)
    return;

  xdisplay = GDK_GL_CONFIG_XDISPLAY (x11_impl->glconfig);

  if (x11_impl->glxwindow == glXGetCurrentDrawable ())
    {
      glXWaitGL ();

      GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");
      glXMakeCurrent (xdisplay, None, NULL);
    }

  if (gdk_x11_gl_query_glx_extension (x11_impl->glconfig, "GLX_MESA_release_buffers"))
    {
      /* Release buffers if GLX_MESA_release_buffers is supported. */

      ReleaseBuffersMESA = (Bool (APIENTRY *)(Display*, GLXDrawable))
        gdk_gl_get_proc_address("glXReleaseBuffersMESA");

      GDK_GL_NOTE_FUNC_IMPL ("glXReleaseBuffersMESA");
      if (ReleaseBuffersMESA)
        ReleaseBuffersMESA (xdisplay, x11_impl->glxwindow);
    }

  x11_impl->glxwindow = None;

  x11_impl->is_destroyed = TRUE;
}

static void
gdk_gl_window_impl_x11_finalize (GObject *object)
{
  GdkGLWindowImplX11 *impl = GDK_GL_WINDOW_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_x11_gl_window_impl_destroy (GDK_GL_WINDOW (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  G_OBJECT_CLASS (gdk_gl_window_impl_x11_parent_class)->finalize (object);
}

static void
gdk_gl_window_impl_x11_class_init (GdkGLWindowImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->get_glxwindow = _gdk_x11_gl_window_impl_get_glxwindow;

  klass->parent_class.create_gl_context      = _gdk_x11_gl_window_impl_create_gl_context;
  klass->parent_class.is_double_buffered     = _gdk_x11_gl_window_impl_is_double_buffered;
  klass->parent_class.swap_buffers           = _gdk_x11_gl_window_impl_swap_buffers;
  klass->parent_class.wait_gl                = _gdk_x11_gl_window_impl_wait_gl;
  klass->parent_class.wait_gdk               = _gdk_x11_gl_window_impl_wait_gdk;
  klass->parent_class.get_gl_config          = _gdk_x11_gl_window_impl_get_gl_config;
  klass->parent_class.destroy_gl_window_impl = _gdk_x11_gl_window_impl_destroy;

  object_class->finalize = gdk_gl_window_impl_x11_finalize;
}

/*
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 */
GdkGLWindow *
_gdk_x11_gl_window_impl_new (GdkGLWindow *glwindow,
                             GdkGLConfig *glconfig,
                             GdkWindow   *window,
                             const int   *attrib_list)
{
  GdkGLWindowImplX11 *x11_impl;

  /* GLXWindow glxwindow; */
  Window glxwindow;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (glwindow), NULL);
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  /*
   * Get X Window.
   */

  glxwindow = GDK_WINDOW_XID (window);

  /*
   * Instantiate the GdkGLWindowImplX11 object.
   */

  x11_impl = g_object_new (GDK_TYPE_GL_WINDOW_IMPL_X11, NULL);
  GDK_GL_NOTE_FUNC ();

  x11_impl->glxwindow = glxwindow;
  x11_impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (x11_impl->glconfig));

  x11_impl->is_destroyed = FALSE;

  glwindow->impl = GDK_GL_WINDOW_IMPL(x11_impl);
  glwindow->window = window;
  g_object_add_weak_pointer (G_OBJECT (glwindow->window),
                             (gpointer *) &(glwindow->window));

  return glwindow;
}

static GdkGLContext *
_gdk_x11_gl_window_impl_create_gl_context (GdkGLWindow  *glwindow,
                                           GdkGLContext *share_list,
                                           gboolean      direct,
                                           int           render_type)
{
  GdkGLContext *glcontext;
  GdkGLContextImpl *impl;

  glcontext = g_object_new(GDK_TYPE_X11_GL_CONTEXT, NULL);

  g_return_val_if_fail(glcontext != NULL, NULL);

  impl = _gdk_x11_gl_context_impl_new(glcontext,
                                      GDK_GL_DRAWABLE(glwindow),
                                      share_list,
                                      direct,
                                      render_type);
  if (impl == NULL)
    g_object_unref(glcontext);

  g_return_val_if_fail(impl != NULL, NULL);

  return glcontext;
}

static gboolean
_gdk_x11_gl_window_impl_is_double_buffered (GdkGLWindow *glwindow)
{
  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (glwindow), FALSE);

  return gdk_gl_config_is_double_buffered (GDK_GL_WINDOW_IMPL_X11 (glwindow->impl)->glconfig);
}

static void
_gdk_x11_gl_window_impl_swap_buffers (GdkGLWindow *glwindow)
{
  Display *xdisplay;
  Window glxwindow;

  g_return_if_fail (GDK_IS_X11_GL_WINDOW (glwindow));

  xdisplay = GDK_GL_CONFIG_XDISPLAY (GDK_GL_WINDOW_IMPL_X11 (glwindow->impl)->glconfig);
  glxwindow = GDK_GL_WINDOW_IMPL_X11 (glwindow->impl)->glxwindow;

  if (glxwindow == None)
    return;

  GDK_GL_NOTE_FUNC_IMPL ("glXSwapBuffers");

  glXSwapBuffers (xdisplay, glxwindow);
}

static void
_gdk_x11_gl_window_impl_wait_gl (GdkGLWindow *glwindow)
{
  g_return_if_fail (GDK_IS_X11_GL_WINDOW (glwindow));

  glXWaitGL ();
}

static void
_gdk_x11_gl_window_impl_wait_gdk (GdkGLWindow *glwindow)
{
  g_return_if_fail (GDK_IS_X11_GL_WINDOW (glwindow));

  glXWaitX ();
}

static GdkGLConfig *
_gdk_x11_gl_window_impl_get_gl_config (GdkGLWindow *glwindow)
{
  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (glwindow), NULL);

  return GDK_GL_WINDOW_IMPL_X11 (glwindow->impl)->glconfig;
}

static Window
_gdk_x11_gl_window_impl_get_glxwindow (GdkGLWindow *glwindow)
{
  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (glwindow), None);

  return GDK_GL_WINDOW_IMPL_X11 (glwindow->impl)->glxwindow;
}
