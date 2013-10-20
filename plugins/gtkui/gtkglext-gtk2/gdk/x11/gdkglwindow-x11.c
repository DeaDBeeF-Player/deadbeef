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

#include <string.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglcontext-x11.h"
#include "gdkglwindow-x11.h"

#include <gdk/gdkglquery.h>

static gboolean     gdk_gl_window_impl_x11_make_context_current (GdkGLDrawable *draw,
                                                                 GdkGLDrawable *read,
                                                                 GdkGLContext  *glcontext);
static gboolean     gdk_gl_window_impl_x11_is_double_buffered   (GdkGLDrawable *gldrawable);
static void         gdk_gl_window_impl_x11_swap_buffers         (GdkGLDrawable *gldrawable);
/*
static gboolean     gdk_gl_window_impl_x11_gl_begin             (GdkGLDrawable *draw,
                                                                 GdkGLDrawable *read,
                                                                 GdkGLContext  *glcontext);
*/
static void         gdk_gl_window_impl_x11_gl_end               (GdkGLDrawable *gldrawable);
static GdkGLConfig *gdk_gl_window_impl_x11_get_gl_config        (GdkGLDrawable *gldrawable);

static void gdk_gl_window_impl_x11_class_init (GdkGLWindowImplX11Class *klass);
static void gdk_gl_window_impl_x11_finalize   (GObject                 *object);
static void gdk_gl_window_impl_x11_gl_drawable_interface_init (GdkGLDrawableClass *iface);

static gpointer parent_class = NULL;

GType
gdk_gl_window_impl_x11_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLWindowImplX11Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_window_impl_x11_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLWindowImplX11),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };
      static const GInterfaceInfo gl_drawable_interface_info = {
        (GInterfaceInitFunc) gdk_gl_window_impl_x11_gl_drawable_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL                    /* interface_data */
      };

      type = g_type_register_static (GDK_TYPE_GL_WINDOW,
                                     "GdkGLWindowImplX11",
                                     &type_info, 0);
      g_type_add_interface_static (type,
                                   GDK_TYPE_GL_DRAWABLE,
                                   &gl_drawable_interface_info);
    }

  return type;
}

static void
gdk_gl_window_impl_x11_class_init (GdkGLWindowImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_window_impl_x11_finalize;
}

void
_gdk_gl_window_destroy (GdkGLWindow *glwindow)
{
  GdkGLWindowImplX11 *impl = GDK_GL_WINDOW_IMPL_X11 (glwindow);
  Display *xdisplay;
  GdkGL_GLX_MESA_release_buffers *mesa_ext;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  xdisplay = GDK_GL_CONFIG_XDISPLAY (impl->glconfig);

  if (impl->glxwindow == glXGetCurrentDrawable ())
    {
      glXWaitGL ();

      GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");
      glXMakeCurrent (xdisplay, None, NULL);
    }

  /* If GLX_MESA_release_buffers is supported. */
  mesa_ext = gdk_gl_get_GLX_MESA_release_buffers (impl->glconfig);
  if (mesa_ext)
    {
      GDK_GL_NOTE_FUNC_IMPL ("glXReleaseBuffersMESA");
      mesa_ext->glXReleaseBuffersMESA (xdisplay, impl->glxwindow);
    }

  impl->glxwindow = None;

  impl->is_destroyed = TRUE;
}

static void
gdk_gl_window_impl_x11_finalize (GObject *object)
{
  GdkGLWindowImplX11 *impl = GDK_GL_WINDOW_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_gl_window_destroy (GDK_GL_WINDOW (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gdk_gl_window_impl_x11_gl_drawable_interface_init (GdkGLDrawableClass *iface)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  iface->create_new_context   = _gdk_x11_gl_context_new;
  iface->make_context_current =  gdk_gl_window_impl_x11_make_context_current;
  iface->is_double_buffered   =  gdk_gl_window_impl_x11_is_double_buffered;
  iface->swap_buffers         =  gdk_gl_window_impl_x11_swap_buffers;
  iface->wait_gl              = _gdk_gl_drawable_impl_x11_wait_gl;
  iface->wait_gdk             = _gdk_gl_drawable_impl_x11_wait_gdk;
  iface->gl_begin             =  gdk_gl_window_impl_x11_make_context_current;
  iface->gl_end               =  gdk_gl_window_impl_x11_gl_end;
  iface->get_gl_config        =  gdk_gl_window_impl_x11_get_gl_config;
  iface->get_size             = _gdk_gl_window_get_size;
}

/*
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 */
/**
 * gdk_gl_window_new:
 * @glconfig: a #GdkGLConfig.
 * @window: the #GdkWindow to be used as the rendering area.
 * @attrib_list: this must be set to NULL or empty (first attribute of None).
 *
 * Creates an on-screen rendering area.
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 *
 * Return value: the new #GdkGLWindow.
 **/
GdkGLWindow *
gdk_gl_window_new (GdkGLConfig *glconfig,
                   GdkWindow   *window,
                   const int   *attrib_list)
{
  GdkGLWindow *glwindow;
  GdkGLWindowImplX11 *impl;

  /* GLXWindow glxwindow; */
  Window glxwindow;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  /*
   * Get X Window.
   */

  glxwindow = GDK_DRAWABLE_XID (GDK_DRAWABLE (window));

  /*
   * Instantiate the GdkGLWindowImplX11 object.
   */

  glwindow = g_object_new (GDK_TYPE_GL_WINDOW_IMPL_X11, NULL);
  impl = GDK_GL_WINDOW_IMPL_X11 (glwindow);

  glwindow->drawable = GDK_DRAWABLE (window);
  g_object_add_weak_pointer (G_OBJECT (glwindow->drawable),
                             (gpointer *) &(glwindow->drawable));

  impl->glxwindow = glxwindow;

  impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (impl->glconfig));

  impl->is_destroyed = FALSE;

  return glwindow;
}

static gboolean
gdk_gl_window_impl_x11_make_context_current (GdkGLDrawable *draw,
                                             GdkGLDrawable *read,
                                             GdkGLContext  *glcontext)
{
  GdkGLConfig *glconfig;
  Window glxwindow;
  GLXContext glxcontext;

  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_X11 (draw), FALSE);
  g_return_val_if_fail (GDK_IS_GL_CONTEXT_IMPL_X11 (glcontext), FALSE);

  glconfig = GDK_GL_WINDOW_IMPL_X11 (draw)->glconfig;
  glxwindow = GDK_GL_WINDOW_IMPL_X11 (draw)->glxwindow;
  glxcontext = GDK_GL_CONTEXT_GLXCONTEXT (glcontext);

  if (glxwindow == None || glxcontext == NULL)
    return FALSE;

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  GDK_GL_NOTE (MISC,
    g_message (" -- Window: screen number = %d",
      GDK_SCREEN_XNUMBER (gdk_drawable_get_screen (GDK_DRAWABLE (draw)))));
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
  GDK_GL_NOTE (MISC,
    g_message (" -- Window: visual id = 0x%lx",
      GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (GDK_DRAWABLE (draw)))->visualid));

  GDK_GL_NOTE_FUNC_IMPL ("glXMakeCurrent");

  if (!glXMakeCurrent (GDK_GL_CONFIG_XDISPLAY (glconfig), glxwindow, glxcontext))
    {
      g_warning ("glXMakeCurrent() failed");
      _gdk_gl_context_set_gl_drawable (glcontext, NULL);
      /* currently unused. */
      /* _gdk_gl_context_set_gl_drawable_read (glcontext, NULL); */
      return FALSE;
    }

  _gdk_gl_context_set_gl_drawable (glcontext, draw);
  /* currently unused. */
  /* _gdk_gl_context_set_gl_drawable_read (glcontext, read); */

  if (_GDK_GL_CONFIG_AS_SINGLE_MODE (glconfig))
    {
      /* We do this because we are treating a double-buffered frame
         buffer as a single-buffered frame buffer because the system
         does not appear to export any suitable single-buffered
         visuals (in which the following are necessary). */
      glDrawBuffer (GL_FRONT);
      glReadBuffer (GL_FRONT);
    }

  GDK_GL_NOTE (MISC, _gdk_gl_print_gl_info ());

  return TRUE;
}

static gboolean
gdk_gl_window_impl_x11_is_double_buffered (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_X11 (gldrawable), FALSE);

  return gdk_gl_config_is_double_buffered (GDK_GL_WINDOW_IMPL_X11 (gldrawable)->glconfig);
}

static void
gdk_gl_window_impl_x11_swap_buffers (GdkGLDrawable *gldrawable)
{
  Display *xdisplay;
  Window glxwindow;

  g_return_if_fail (GDK_IS_GL_WINDOW_IMPL_X11 (gldrawable));

  xdisplay = GDK_GL_CONFIG_XDISPLAY (GDK_GL_WINDOW_IMPL_X11 (gldrawable)->glconfig);
  glxwindow = GDK_GL_WINDOW_IMPL_X11 (gldrawable)->glxwindow;

  if (glxwindow == None)
    return;

  GDK_GL_NOTE_FUNC_IMPL ("glXSwapBuffers");

  glXSwapBuffers (xdisplay, glxwindow);
}

/*
static gboolean
gdk_gl_window_impl_x11_gl_begin (GdkGLDrawable *draw,
                                 GdkGLDrawable *read,
                                 GdkGLContext  *glcontext)
{
  return gdk_gl_window_impl_x11_make_context_current (draw, read, glcontext);
}
*/

static void
gdk_gl_window_impl_x11_gl_end (GdkGLDrawable *gldrawable)
{
  /* do nothing */
}

static GdkGLConfig *
gdk_gl_window_impl_x11_get_gl_config (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_X11 (gldrawable), NULL);

  return GDK_GL_WINDOW_IMPL_X11 (gldrawable)->glconfig;
}

/**
 * gdk_x11_gl_window_get_glxwindow:
 * @glwindow: a #GdkGLWindow.
 *
 * Gets X Window.
 *
 * Return value: the Window.
 **/
Window
gdk_x11_gl_window_get_glxwindow (GdkGLWindow *glwindow)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_X11 (glwindow), None);

  return GDK_GL_WINDOW_IMPL_X11 (glwindow)->glxwindow;
}
