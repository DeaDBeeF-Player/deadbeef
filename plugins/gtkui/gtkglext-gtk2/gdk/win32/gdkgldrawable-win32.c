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
#include "gdkglpixmap-win32.h"
#include "gdkglwindow-win32.h"
#include "gdkgldrawable.h"

HDC
gdk_win32_gl_drawable_hdc_get (GdkGLDrawable *gldrawable)
{
  if (GDK_IS_GL_PIXMAP (gldrawable))
    {
      GdkGLPixmapImplWin32 *impl = GDK_GL_PIXMAP_IMPL_WIN32 (gldrawable);
      return GDK_GL_PIXMAP_IMPL_WIN32_HDC_GET (impl);
    }
  else if (GDK_IS_GL_WINDOW (gldrawable))
    {
      GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (gldrawable);
      return GDK_GL_WINDOW_IMPL_WIN32_HDC_GET (impl);
    }
  else
    g_warning ("GLDrawable should be GLPixmap or GLWindow");

  return NULL;
}

void
gdk_win32_gl_drawable_hdc_release (GdkGLDrawable *gldrawable)
{
  if (GDK_IS_GL_PIXMAP (gldrawable))
    {
      /* GLPixmap's memory DC doesn't need to be released. */
      /*
      GdkGLPixmapImplWin32 *impl = GDK_GL_PIXMAP_IMPL_WIN32 (gldrawable);
      GDK_GL_PIXMAP_IMPL_WIN32_HDC_RELEASE (impl);
      */
      return;
    }
  else if (GDK_IS_GL_WINDOW (gldrawable))
    {
      GdkGLWindowImplWin32 *impl = GDK_GL_WINDOW_IMPL_WIN32 (gldrawable);
      GDK_GL_WINDOW_IMPL_WIN32_HDC_RELEASE (impl);
    }
  else
    g_warning ("GLDrawable should be GLPixmap or GLWindow");
}
