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

#ifndef __GDK_GL_WIN32_H__
#define __GDK_GL_WIN32_H__

#include <gdk/gdkwin32.h>

#ifndef STRICT
#define STRICT                  /* We want strict type checks */
#endif
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include <GL/gl.h>

#include <gdk/win32/gdkglwglext.h>

/* MinGW's wingdi.h bug. */
#ifndef PFD_SWAP_LAYER_BUFFERS
#define PFD_SWAP_LAYER_BUFFERS      0x00000800
#endif

G_BEGIN_DECLS

gboolean               gdk_win32_gl_query_wgl_extension     (GdkGLConfig   *glconfig,
                                                             const char    *extension);

GdkGLConfig           *gdk_win32_gl_config_new_from_pixel_format (int pixel_format);

PIXELFORMATDESCRIPTOR *gdk_win32_gl_config_get_pfd          (GdkGLConfig   *glconfig);

GdkGLContext          *gdk_win32_gl_context_foreign_new     (GdkGLConfig   *glconfig,
                                                             GdkGLContext  *share_list,
                                                             HGLRC          hglrc);

HGLRC                  gdk_win32_gl_context_get_hglrc       (GdkGLContext  *glcontext);

HDC                    gdk_win32_gl_drawable_hdc_get        (GdkGLDrawable *gldrawable);
void                   gdk_win32_gl_drawable_hdc_release    (GdkGLDrawable *gldrawable);

PIXELFORMATDESCRIPTOR *gdk_win32_gl_pixmap_get_pfd          (GdkGLPixmap   *glpixmap);
int                    gdk_win32_gl_pixmap_get_pixel_format (GdkGLPixmap   *glpixmap);

PIXELFORMATDESCRIPTOR *gdk_win32_gl_window_get_pfd          (GdkGLWindow   *glwindow);
int                    gdk_win32_gl_window_get_pixel_format (GdkGLWindow   *glwindow);

#ifdef INSIDE_GDK_GL_WIN32

#define GDK_GL_CONFIG_PFD(glconfig)          (&(GDK_GL_CONFIG_IMPL_WIN32 (glconfig)->pfd))
#define GDK_GL_CONTEXT_HGLRC(glcontext)      (GDK_GL_CONTEXT_IMPL_WIN32 (glcontext)->hglrc)
#define GDK_GL_PIXMAP_PFD(glpixmap)          (&(GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap)->pfd))
#define GDK_GL_PIXMAP_PIXEL_FORMAT(glpixmap) (GDK_GL_PIXMAP_IMPL_WIN32 (glpixmap)->pixel_format)
#define GDK_GL_WINDOW_PFD(glwindow)          (&(GDK_GL_WINDOW_IMPL_WIN32 (glwindow)->pfd))
#define GDK_GL_WINDOW_PIXEL_FORMAT(glwindow) (GDK_GL_WINDOW_IMPL_WIN32 (glwindow)->pixel_format)

#else

#define GDK_GL_CONFIG_PFD(glconfig)          (gdk_win32_gl_config_get_pfd (glconfig))
#define GDK_GL_CONTEXT_HGLRC(glcontext)      (gdk_win32_gl_context_get_hglrc (glcontext))
#define GDK_GL_PIXMAP_PFD(glpixmap)          (gdk_win32_gl_pixmap_get_pfd (glpixmap))
#define GDK_GL_PIXMAP_PIXEL_FORMAT(glpixmap) (gdk_win32_gl_pixmap_get_pixel_format (glpixmap))
#define GDK_GL_WINDOW_PFD(glwindow)          (gdk_win32_gl_window_get_pfd (glwindow))
#define GDK_GL_WINDOW_PIXEL_FORMAT(glwindow) (gdk_win32_gl_window_get_pixel_format (glwindow))

#endif

G_END_DECLS

#endif /* __GDK_GL_WIN32_H__ */
