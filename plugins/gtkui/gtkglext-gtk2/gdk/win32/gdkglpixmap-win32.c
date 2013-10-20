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
#include "gdkglpixmap-win32.h"

static void gdk_gl_pixmap_sync_gl  (GdkGLPixmap *glpixmap);
static void gdk_gl_pixmap_sync_gdk (GdkGLPixmap *glpixmap);

static gboolean     gdk_gl_pixmap_impl_win32_make_context_current (GdkGLDrawable *draw,
                                                                   GdkGLDrawable *read,
                                                                   GdkGLContext  *glcontext);
static gboolean     gdk_gl_pixmap_impl_win32_is_double_buffered   (GdkGLDrawable *gldrawable);
static void         gdk_gl_pixmap_impl_win32_swap_buffers         (GdkGLDrawable *gldrawable);
static void         gdk_gl_pixmap_impl_win32_wait_gl              (GdkGLDrawable *gldrawable);
static void         gdk_gl_pixmap_impl_win32_wait_gdk             (GdkGLDrawable *gldrawable);
static gboolean     gdk_gl_pixmap_impl_win32_gl_begin             (GdkGLDrawable *draw,
                                                                   GdkGLDrawable *read,
                                                                   GdkGLContext  *glcontext);
static void         gdk_gl_pixmap_impl_win32_gl_end               (GdkGLDrawable *gldrawable);
static GdkGLConfig *gdk_gl_pixmap_impl_win32_get_gl_config        (GdkGLDrawable *gldrawable);

static void gdk_gl_pixmap_impl_win32_class_init (GdkGLPixmapImplWin32Class *klass);
static void gdk_gl_pixmap_impl_win32_finalize   (GObject                   *object);
static void gdk_gl_pixmap_impl_win32_gl_drawable_interface_init (GdkGLDrawableClass *iface);

static gpointer parent_class = NULL;

GType
gdk_gl_pixmap_impl_win32_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLPixmapImplWin32Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_pixmap_impl_win32_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLPixmapImplWin32),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };
      static const GInterfaceInfo gl_drawable_interface_info = {
        (GInterfaceInitFunc) gdk_gl_pixmap_impl_win32_gl_drawable_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL                    /* interface_data */
      };

      type = g_type_register_static (GDK_TYPE_GL_PIXMAP,
                                     "GdkGLPixmapImplWin32",
                                     &type_info, 0);
      g_type_add_interface_static (type,
                                   GDK_TYPE_GL_DRAWABLE,
                                   &gl_drawable_interface_info);
    }

  return type;
}

static void
gdk_gl_pixmap_impl_win32_class_init (GdkGLPixmapImplWin32Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_pixmap_impl_win32_finalize;
}

void
_gdk_gl_pixmap_destroy (GdkGLPixmap *glpixmap)
{
  GdkGLPixmapImplWin32 *impl = GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (impl->is_destroyed)
    return;

  if (impl->hdc_gl == wglGetCurrentDC ())
    {
      glFinish ();

      GDK_GL_NOTE_FUNC_IMPL ("wglMakeCurrent");
      wglMakeCurrent (NULL, NULL);
    }

  DeleteDC (impl->hdc_gl);
  impl->hdc_gl = NULL;

  DeleteDC (impl->hdc_gdk);
  impl->hdc_gdk = NULL;

  g_object_unref (G_OBJECT (impl->pixmap_gl));
  impl->pixmap_gl = NULL;

  impl->is_destroyed = TRUE;
}

static void
gdk_gl_pixmap_impl_win32_finalize (GObject *object)
{
  GdkGLPixmapImplWin32 *impl = GDK_GL_PIXMAP_IMPL_WIN32 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  _gdk_gl_pixmap_destroy (GDK_GL_PIXMAP (object));

  g_object_unref (G_OBJECT (impl->glconfig));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gdk_gl_pixmap_impl_win32_gl_drawable_interface_init (GdkGLDrawableClass *iface)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  iface->create_new_context   = _gdk_win32_gl_context_new;
  iface->make_context_current =  gdk_gl_pixmap_impl_win32_make_context_current;
  iface->is_double_buffered   =  gdk_gl_pixmap_impl_win32_is_double_buffered;
  iface->swap_buffers         =  gdk_gl_pixmap_impl_win32_swap_buffers;
  iface->wait_gl              =  gdk_gl_pixmap_impl_win32_wait_gl;
  iface->wait_gdk             =  gdk_gl_pixmap_impl_win32_wait_gdk;
  iface->gl_begin             =  gdk_gl_pixmap_impl_win32_gl_begin;
  iface->gl_end               =  gdk_gl_pixmap_impl_win32_gl_end;
  iface->get_gl_config        =  gdk_gl_pixmap_impl_win32_get_gl_config;
  iface->get_size             = _gdk_gl_pixmap_get_size;
}

/*
 * attrib_list is currently unused. This must be set to NULL or empty
 * (first attribute of None). See GLX 1.3 spec.
 */
GdkGLPixmap *
gdk_gl_pixmap_new (GdkGLConfig *glconfig,
                   GdkPixmap   *pixmap,
                   const int   *attrib_list)
{
  GdkGLPixmap *glpixmap;
  GdkGLPixmapImplWin32 *impl;

  gint width, height;
  gint depth;
  GdkPixmap *pixmap_gl = NULL;

  HBITMAP hbitmap_gl;
  HDC hdc_gl = NULL;
  PIXELFORMATDESCRIPTOR pfd;
  int pixel_format;

  HBITMAP hbitmap_gdk;
  HDC hdc_gdk = NULL;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_WIN32 (glconfig), NULL);
  g_return_val_if_fail (GDK_IS_PIXMAP (pixmap), NULL);

  /*
   * Create offscreen rendering area.
   */

  gdk_drawable_get_size (GDK_DRAWABLE (pixmap), &width, &height);
  depth = gdk_drawable_get_depth (GDK_DRAWABLE (pixmap));

  pixmap_gl = gdk_pixmap_new (NULL, width, height, depth);
  if (pixmap_gl == NULL)
    goto FAIL;

  /*
   * Source (OpenGL) DIB
   */

  hbitmap_gl = (HBITMAP) gdk_win32_drawable_get_handle (GDK_DRAWABLE (pixmap_gl));

  /* Create a memory DC. */
  hdc_gl = CreateCompatibleDC (NULL);
  if (hdc_gl == NULL)
    {
      g_warning ("cannot create a memory DC");
      goto FAIL;
    }

  /* Select the bitmap. */
  if (SelectObject (hdc_gl, hbitmap_gl) == NULL)
    {
      g_warning ("cannot select DIB");
      goto FAIL;
    }

  /*
   * Choose pixel format.
   */

  pfd = *(GDK_GL_CONFIG_PFD (glconfig));
  /* Draw to bitmap */
  pfd.dwFlags &= ~PFD_DRAW_TO_WINDOW;
  pfd.dwFlags |= PFD_DRAW_TO_BITMAP;

  /* Request pfd.cColorBits should exclude alpha bitplanes. */
  pfd.cColorBits = pfd.cRedBits + pfd.cGreenBits + pfd.cBlueBits;

  GDK_GL_NOTE_FUNC_IMPL ("ChoosePixelFormat");

  pixel_format = ChoosePixelFormat (hdc_gl, &pfd);
  if (pixel_format == 0)
    {
      g_warning ("cannot choose pixel format");
      goto FAIL;
    }

  /*
   * Set pixel format.
   */

  GDK_GL_NOTE_FUNC_IMPL ("SetPixelFormat");

  if (!SetPixelFormat (hdc_gl, pixel_format, &pfd))
    {
      g_warning ("cannot set pixel format");
      goto FAIL;
    }

  DescribePixelFormat (hdc_gl, pixel_format, sizeof (pfd), &pfd);

  GDK_GL_NOTE (MISC, g_message (" -- impl->pixel_format = 0x%x", pixel_format));
  GDK_GL_NOTE (MISC, _gdk_win32_gl_print_pfd (&pfd));

  /*
   * Destination (GDK) DIB
   */

  hbitmap_gdk = (HBITMAP) gdk_win32_drawable_get_handle (GDK_DRAWABLE (pixmap));

  /* Create a memory DC. */
  hdc_gdk = CreateCompatibleDC (hdc_gl);
  if (hdc_gdk == NULL)
    {
      g_warning ("cannot create a memory DC");
      goto FAIL;
    }

  /*
   * Instantiate the GdkGLPixmapImplWin32 object.
   */

  glpixmap = g_object_new (GDK_TYPE_GL_PIXMAP_IMPL_WIN32, NULL);
  impl = GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap);

  glpixmap->drawable = GDK_DRAWABLE (pixmap);
  g_object_add_weak_pointer (G_OBJECT (glpixmap->drawable),
                             (gpointer *) &(glpixmap->drawable));

  impl->pixmap_gl = pixmap_gl;

  impl->width = width;
  impl->height = height;

  impl->pfd = pfd;
  impl->pixel_format = pixel_format;

  impl->glconfig = glconfig;
  g_object_ref (G_OBJECT (impl->glconfig));

  impl->hdc_gl = hdc_gl;

  impl->hdc_gdk = hdc_gdk;
  impl->hbitmap_gdk = hbitmap_gdk;

  impl->is_destroyed = FALSE;

  return glpixmap;

 FAIL:

  if (hdc_gdk != NULL)
    DeleteDC (hdc_gdk);

  if (hdc_gl != NULL)
    DeleteDC (hdc_gl);

  if (pixmap_gl != NULL)
    g_object_unref (G_OBJECT (pixmap_gl));

  return NULL;  
}

static void
gdk_gl_pixmap_sync_gl (GdkGLPixmap *glpixmap)
{
  GdkGLPixmapImplWin32 *impl;
  int width, height;
  HBITMAP hbitmap_old;

  g_return_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (glpixmap));

  GDK_GL_NOTE_FUNC_PRIVATE ();

  impl = GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap);

  width = impl->width;
  height = impl->height;

  /*
   * Copy OpenGL bitmap to GDK bitmap.
   */

  hbitmap_old = SelectObject (impl->hdc_gdk, impl->hbitmap_gdk);
  if (hbitmap_old == NULL)
    {
      g_warning ("cannot select DIB");
      return;
    }

  if (!StretchBlt (impl->hdc_gdk, 0, 0,      width,  height,
		   impl->hdc_gl,  0, height, width, -height,
		   SRCCOPY))
    g_warning ("StretchBlt() failed");

  SelectObject (impl->hdc_gdk, hbitmap_old);
}

static void
gdk_gl_pixmap_sync_gdk (GdkGLPixmap *glpixmap)
{
  GdkGLPixmapImplWin32 *impl;
  int width, height;
  HBITMAP hbitmap_old;

  g_return_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (glpixmap));

  GDK_GL_NOTE_FUNC_PRIVATE ();

  impl = GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap);

  width = impl->width;
  height = impl->height;

  /*
   * Copy GDK bitmap to OpenGL bitmap.
   */

  hbitmap_old = SelectObject (impl->hdc_gdk, impl->hbitmap_gdk);
  if (hbitmap_old == NULL)
    {
      g_warning ("cannot select DIB");
      return;
    }

  if (!StretchBlt (impl->hdc_gl,  0, 0,      width,  height,
		   impl->hdc_gdk, 0, height, width, -height,
		   SRCCOPY))
    g_warning ("StretchBlt() failed");

  SelectObject (impl->hdc_gdk, hbitmap_old);
}

static gboolean
gdk_gl_pixmap_impl_win32_make_context_current (GdkGLDrawable *draw,
                                               GdkGLDrawable *read,
                                               GdkGLContext  *glcontext)
{
  GdkGLPixmapImplWin32 *impl;
  HDC hdc;
  HGLRC hglrc;

  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (draw), FALSE);
  g_return_val_if_fail (GDK_IS_GL_CONTEXT_IMPL_WIN32 (glcontext), FALSE);

  if (GDK_GL_PIXMAP_IS_DESTROYED (draw) ||
      GDK_GL_CONTEXT_IS_DESTROYED (glcontext))
    return FALSE;

  impl = GDK_GL_PIXMAP_IMPL_WIN32 (draw);

  /* Get DC. */
  hdc = GDK_GL_PIXMAP_IMPL_WIN32_HDC_GET (impl);

  /* Get GLRC. */
  hglrc = GDK_GL_CONTEXT_HGLRC (glcontext);

  GDK_GL_NOTE_FUNC_IMPL ("wglMakeCurrent");

  if (!wglMakeCurrent (hdc, hglrc))
    {
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

  /* Do *NOT* release DC. */

  return TRUE;
}

static gboolean
gdk_gl_pixmap_impl_win32_is_double_buffered (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (gldrawable), FALSE);

  return gdk_gl_config_is_double_buffered (GDK_GL_PIXMAP_IMPL_WIN32 (gldrawable)->glconfig);
}

static void
gdk_gl_pixmap_impl_win32_swap_buffers (GdkGLDrawable *gldrawable)
{
  GdkGLPixmapImplWin32 *impl;
  HDC hdc;

  g_return_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (gldrawable));

  if (GDK_GL_PIXMAP_IS_DESTROYED (gldrawable))
    return;

  impl = GDK_GL_PIXMAP_IMPL_WIN32 (gldrawable);

  /* Get DC. */
  hdc = GDK_GL_PIXMAP_IMPL_WIN32_HDC_GET (impl);

  GDK_GL_NOTE_FUNC_IMPL ("SwapBuffers");

  SwapBuffers (hdc);

  /* Release DC. */
  /* GDK_GL_PIXMAP_IMPL_WIN32_HDC_RELEASE (impl); */
}

static void
gdk_gl_pixmap_impl_win32_wait_gl (GdkGLDrawable *gldrawable)
{
  glFinish ();

  /* Sync. */
  gdk_gl_pixmap_sync_gl (GDK_GL_PIXMAP (gldrawable));
}

static void
gdk_gl_pixmap_impl_win32_wait_gdk (GdkGLDrawable *gldrawable)
{
  GdiFlush ();

  /* Sync. */
  gdk_gl_pixmap_sync_gdk (GDK_GL_PIXMAP (gldrawable));
}

static gboolean
gdk_gl_pixmap_impl_win32_gl_begin (GdkGLDrawable *draw,
                                   GdkGLDrawable *read,
                                   GdkGLContext  *glcontext)
{
  gboolean ret;

  ret = gdk_gl_pixmap_impl_win32_make_context_current (draw, read, glcontext);
  if (!ret)
    return FALSE;

  gdk_gl_pixmap_impl_win32_wait_gdk (draw);

  return TRUE;
}

static void
gdk_gl_pixmap_impl_win32_gl_end (GdkGLDrawable *gldrawable)
{
  gdk_gl_pixmap_impl_win32_wait_gl (gldrawable);
}

static GdkGLConfig *
gdk_gl_pixmap_impl_win32_get_gl_config (GdkGLDrawable *gldrawable)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (gldrawable), NULL);

  return GDK_GL_PIXMAP_IMPL_WIN32 (gldrawable)->glconfig;
}

PIXELFORMATDESCRIPTOR *
gdk_win32_gl_pixmap_get_pfd (GdkGLPixmap *glpixmap)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (glpixmap), NULL);

  return &(GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap)->pfd);
}

int
gdk_win32_gl_pixmap_get_pixel_format (GdkGLPixmap *glpixmap)
{
  g_return_val_if_fail (GDK_IS_GL_PIXMAP_IMPL_WIN32 (glpixmap), 0);

  return GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap)->pixel_format;
}
