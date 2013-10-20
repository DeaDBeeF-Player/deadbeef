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

#include "gdkglwin32.h"
#include "gdkglprivate-win32.h"
#include "gdkglconfig-win32.h"
#include "gdkglcontext-win32.h"
#include "gdkglwindow-win32.h"

static gboolean     gdk_gl_window_impl_win32_make_context_current (GdkGLDrawable *draw,
                                                                   GdkGLDrawable *read,
                                                                   GdkGLContext  *glcontext);
static gboolean     gdk_gl_window_impl_win32_is_double_buffered   (GdkGLDrawable *gldrawable);
static void         gdk_gl_window_impl_win32_swap_buffers         (GdkGLDrawable *gldrawable);
static void         gdk_gl_window_impl_win32_wait_gl              (GdkGLDrawable *gldrawable);
static void         gdk_gl_window_impl_win32_wait_gdk             (GdkGLDrawable *gldrawable);
/*
static gboolean     gdk_gl_window_impl_win32_gl_begin             (GdkGLDrawable *draw,
                                                                   GdkGLDrawable *read,
                                                                   GdkGLContext  *glcontext);
*/
static void         gdk_gl_window_impl_win32_gl_end               (GdkGLDrawable *gldrawable);
static GdkGLConfig *gdk_gl_window_impl_win32_get_gl_config        (GdkGLDrawable *gldrawable);

static void gdk_gl_window_impl_win32_class_init (GdkGLWindowImplWin32Class *klass);
static void gdk_gl_window_impl_win32_finalize   (GObject                   *object);
static void gdk_gl_window_impl_win32_gl_drawable_interface_init (GdkGLDrawableClass *iface);

static gpointer parent_class = NULL;

GType
gdk_gl_window_impl_win32_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLWindowImplWin32Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_window_impl_win32_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLWindowImplWin32),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };
      static const GInterfaceInfo gl_drawable_interface_info = {
        (GInterfaceInitFunc) gdk_gl_window_impl_win32_gl_drawable_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL                    /* interface_data */
      };

      type = g_type_register_static (GDK_TYPE_GL_WINDOW,
                                     "GdkGLWindowImplWin32",
                                     &type_info, 0);
      g_type_add_interface_static (type,
                                   GDK_TYPE_GL_DRAWABLE,
                                   &gl_drawable_interface_info);
    }

  return type;
}

static void
gdk_gl_window_impl_win32_class_init (GdkGLWindowImplWin32Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_window_impl_win32_finalize;
}

void
_gdk_gl_window_destroy (GdkGLWindow *glwindow)
{
  GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (glwindow);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  /* Get DC. */
  if (impl->hdc == NULL)
    {
      impl->hdc = GetDC (impl->hwnd);
      if (impl->hdc == NULL)
        return;
    }

  if (impl->hdc == wglGetCurrentDC ())
    {
      glFinish ();

      GDK_GL_NOTE_FUNC_IMPL ("wglMakeCurrent");
      wglMakeCurrent (NULL, NULL);
    }

  /* Release DC. */
  if (impl->need_release_dc)
    ReleaseDC (impl->hwnd, impl->hdc);
  impl->hdc = NULL;

  impl->hwnd = NULL;

  impl->is_destroyed = TRUE;
}

static void
gdk_gl_window_impl_win32_finalize (GObject *object)
{
  GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_gl_window_destroy (GDK_GL_WINDOW (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gdk_gl_window_impl_win32_gl_drawable_interface_init (GdkGLDrawableClass *iface)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  iface->create_new_context   = _gdk_win32_gl_context_new;
  iface->make_context_current =  gdk_gl_window_impl_win32_make_context_current;
  iface->is_double_buffered   =  gdk_gl_window_impl_win32_is_double_buffered;
  iface->swap_buffers         =  gdk_gl_window_impl_win32_swap_buffers;
  iface->wait_gl              =  gdk_gl_window_impl_win32_wait_gl;
  iface->wait_gdk             =  gdk_gl_window_impl_win32_wait_gdk;
  iface->gl_begin             =  gdk_gl_window_impl_win32_make_context_current;
  iface->gl_end               =  gdk_gl_window_impl_win32_gl_end;
  iface->get_gl_config        =  gdk_gl_window_impl_win32_get_gl_config;
  iface->get_size             = _gdk_gl_window_get_size;
}

/*
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 */
GdkGLWindow *
gdk_gl_window_new (GdkGLConfig *glconfig,
                   GdkWindow   *window,
                   const int   *attrib_list)
{
  GdkGLWindow *glwindow;
  GdkGLWindowImplWin32 *impl;

  HWND hwnd;
  DWORD wndclass_style;
  gboolean need_release_dc;
  HDC hdc = NULL;
  PIXELFORMATDESCRIPTOR pfd;
  int pixel_format;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_WIN32 (glconfig), NULL);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  hwnd = (HWND) gdk_win32_drawable_get_handle (GDK_DRAWABLE (window));

  /* Private DC? */
  wndclass_style = GetClassLong (hwnd, GCL_STYLE);
  if (wndclass_style & CS_OWNDC)
    {
      GDK_GL_NOTE (MISC, g_message (" -- Private DC"));
      need_release_dc = FALSE;
    }
  else
    {
      GDK_GL_NOTE (MISC, g_message (" -- Common DC"));
      need_release_dc = TRUE;
    }

  /* Get DC. */
  hdc = GetDC (hwnd);
  if (hdc == NULL)
    {
      g_warning ("cannot get DC");
      goto FAIL;
    }

  /*
   * Choose pixel format.
   */

  pfd = *(GDK_GL_CONFIG_PFD (glconfig));
  /* Draw to window */
  pfd.dwFlags &= ~PFD_DRAW_TO_BITMAP;
  pfd.dwFlags |= PFD_DRAW_TO_WINDOW;

  /* Request pfd.cColorBits should exclude alpha bitplanes. */
  pfd.cColorBits = pfd.cRedBits + pfd.cGreenBits + pfd.cBlueBits;

  GDK_GL_NOTE_FUNC_IMPL ("ChoosePixelFormat");

  pixel_format = ChoosePixelFormat (hdc, &pfd);
  if (pixel_format == 0)
    {
      g_warning ("cannot choose pixel format");
      goto FAIL;
    }

  /*
   * Set pixel format.
   */

  GDK_GL_NOTE_FUNC_IMPL ("SetPixelFormat");

  if (!SetPixelFormat (hdc, pixel_format, &pfd))
    {
      g_warning ("cannot set pixel format");
      goto FAIL;
    }

  DescribePixelFormat (hdc, pixel_format, sizeof (pfd), &pfd);

  GDK_GL_NOTE (MISC, g_message (" -- impl->pixel_format = 0x%x", pixel_format));
  GDK_GL_NOTE (MISC, _gdk_win32_gl_print_pfd (&pfd));

  if (need_release_dc)
    {
      /* Release DC. */
      ReleaseDC (hwnd, hdc);
      hdc = NULL;
    }

  /*
   * Instantiate the GdkGLWindowImplWin32 object.
   */

  glwindow = g_object_new (GDK_TYPE_GL_WINDOW_IMPL_WIN32, NULL);
  impl = GDK_GL_WINDOW_IMPL_WIN32 (glwindow);

  glwindow->drawable = GDK_DRAWABLE (window);
  g_object_add_weak_pointer (G_OBJECT (glwindow->drawable),
                             (gpointer *) &(glwindow->drawable));

  impl->hwnd = hwnd;

  impl->pfd = pfd;
  impl->pixel_format = pixel_format;

  impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (impl->glconfig));

  impl->hdc = hdc;
  impl->need_release_dc = need_release_dc;

  impl->is_destroyed = FALSE;

  return glwindow;

 FAIL:

  /* Release DC. */
  if (need_release_dc && hdc != NULL)
    ReleaseDC (hwnd, hdc);

  return NULL;
}

static gboolean
gdk_gl_window_impl_win32_make_context_current (GdkGLDrawable *draw,
                                               GdkGLDrawable *read,
                                               GdkGLContext  *glcontext)
{
  GdkGLWindowImplWin32 *impl;
  HDC hdc;
  HGLRC hglrc;

  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_WIN32 (draw), FALSE);
  g_return_val_if_fail (GDK_IS_GL_CONTEXT_IMPL_WIN32 (glcontext), FALSE);

  if (GDK_GL_WINDOW_IS_DESTROYED (draw) ||
      GDK_GL_CONTEXT_IS_DESTROYED (glcontext))
    return FALSE;

  impl = GDK_GL_WINDOW_IMPL_WIN32 (draw);

  /* Get DC. */
  hdc = GDK_GL_WINDOW_IMPL_WIN32_HDC_GET (impl);

  /* Get GLRC. */
  hglrc = GDK_GL_CONTEXT_HGLRC (glcontext);

  GDK_GL_NOTE_FUNC_IMPL ("wglMakeCurrent");

  if (!wglMakeCurrent (hdc, hglrc))
    {
      g_warning ("wglMakeCurrent() failed");
      _gdk_gl_context_set_gl_drawable (glcontext, NULL);
      /* currently unused. */
      /* _gdk_gl_context_set_gl_drawable_read (glcontext, NULL); */
      return FALSE;
    }

  _gdk_gl_context_set_gl_drawable (glcontext, draw);
  /* currently unused. */
  /* _gdk_gl_context_set_gl_drawable_read (glcontext, read); */

  if (_GDK_GL_CONFIG_AS_SINGLE_MODE (impl->glconfig))
    {
      /* We do this because we are treating a double-buffered frame
         buffer as a single-buffered frame buffer because the system
         does not appear to export any suitable single-buffered
         visuals (in which the following are necessary). */
      glDrawBuffer (GL_FRONT);
      glReadBuffer (GL_FRONT);
    }

  GDK_GL_NOTE (MISC, _gdk_gl_print_gl_info ());

  /*
   * Do *NOT* release DC.
   *
   * With some graphics card, DC owned by rendering thread will be needed.
   */

  return TRUE;
}

static gboolean
gdk_gl_window_impl_win32_is_double_buffered (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_WIN32 (gldrawable), FALSE);

  return gdk_gl_config_is_double_buffered (GDK_GL_WINDOW_IMPL_WIN32 (gldrawable)->glconfig);
}

static void
gdk_gl_window_impl_win32_swap_buffers (GdkGLDrawable *gldrawable)
{
  GdkGLWindowImplWin32 *impl;
  HDC hdc;

  g_return_if_fail (GDK_IS_GL_WINDOW_IMPL_WIN32 (gldrawable));

  if (GDK_GL_WINDOW_IS_DESTROYED (gldrawable))
    return;

  impl = GDK_GL_WINDOW_IMPL_WIN32 (gldrawable);

  /* Get DC. */
  hdc = GDK_GL_WINDOW_IMPL_WIN32_HDC_GET (impl);

  GDK_GL_NOTE_FUNC_IMPL ("SwapBuffers");

  SwapBuffers (hdc);

  /* Release DC. */
  GDK_GL_WINDOW_IMPL_WIN32_HDC_RELEASE (impl);
}

static void
gdk_gl_window_impl_win32_wait_gl (GdkGLDrawable *gldrawable)
{
  GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (gldrawable);

  glFinish ();

  /* Release DC. */
  GDK_GL_WINDOW_IMPL_WIN32_HDC_RELEASE (impl);
}

static void
gdk_gl_window_impl_win32_wait_gdk (GdkGLDrawable *gldrawable)
{
  GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (gldrawable);

  GdiFlush ();

  /* Get DC. */
  GDK_GL_WINDOW_IMPL_WIN32_HDC_GET (impl);
}

/*
static gboolean
gdk_gl_window_impl_win32_gl_begin (GdkGLDrawable *draw,
                                   GdkGLDrawable *read,
                                   GdkGLContext  *glcontext)
{
  return gdk_gl_window_impl_win32_make_context_current (draw, read, glcontext);
}
*/

static void
gdk_gl_window_impl_win32_gl_end (GdkGLDrawable *gldrawable)
{
  GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (gldrawable);

  /* Release DC. */
  GDK_GL_WINDOW_IMPL_WIN32_HDC_RELEASE (impl);
}

static GdkGLConfig *
gdk_gl_window_impl_win32_get_gl_config (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_WIN32 (gldrawable), NULL);

  return GDK_GL_WINDOW_IMPL_WIN32 (gldrawable)->glconfig;
}

PIXELFORMATDESCRIPTOR *
gdk_win32_gl_window_get_pfd (GdkGLWindow *glwindow)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_WIN32 (glwindow), NULL);

  return &(GDK_GL_WINDOW_IMPL_WIN32 (glwindow)->pfd);
}

int
gdk_win32_gl_window_get_pixel_format (GdkGLWindow *glwindow)
{
  g_return_val_if_fail (GDK_IS_GL_WINDOW_IMPL_WIN32 (glwindow), 0);

  return GDK_GL_WINDOW_IMPL_WIN32 (glwindow)->pixel_format;
}
