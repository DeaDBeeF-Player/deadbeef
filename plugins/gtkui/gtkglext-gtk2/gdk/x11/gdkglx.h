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

#ifndef __GDK_GL_X_H__
#define __GDK_GL_X_H__

#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <gdk/x11/gdkglglxext.h>

G_BEGIN_DECLS

gboolean      gdk_x11_gl_query_glx_extension      (GdkGLConfig  *glconfig,
                                                   const char   *extension);

#ifndef GDK_MULTIHEAD_SAFE
GdkGLConfig  *gdk_x11_gl_config_new_from_visualid            (VisualID   xvisualid);
#endif /* GDK_MULTIHEAD_SAFE */
#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
GdkGLConfig  *gdk_x11_gl_config_new_from_visualid_for_screen (GdkScreen *screen,
                                                              VisualID   xvisualid);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

Display      *gdk_x11_gl_config_get_xdisplay      (GdkGLConfig  *glconfig);
int           gdk_x11_gl_config_get_screen_number (GdkGLConfig  *glconfig);
XVisualInfo  *gdk_x11_gl_config_get_xvinfo        (GdkGLConfig  *glconfig);
gboolean      gdk_x11_gl_config_is_mesa_glx       (GdkGLConfig  *glconfig);

GdkGLContext *gdk_x11_gl_context_foreign_new      (GdkGLConfig  *glconfig,
                                                   GdkGLContext *share_list,
                                                   GLXContext    glxcontext);

GLXContext    gdk_x11_gl_context_get_glxcontext   (GdkGLContext *glcontext);

GLXPixmap     gdk_x11_gl_pixmap_get_glxpixmap     (GdkGLPixmap  *glpixmap);

Window        gdk_x11_gl_window_get_glxwindow     (GdkGLWindow  *glwindow);

#ifdef INSIDE_GDK_GL_X11

#define GDK_GL_CONFIG_XDISPLAY(glconfig)       (GDK_GL_CONFIG_IMPL_X11 (glconfig)->xdisplay)
#define GDK_GL_CONFIG_SCREEN_XNUMBER(glconfig) (GDK_GL_CONFIG_IMPL_X11 (glconfig)->screen_num)
#define GDK_GL_CONFIG_XVINFO(glconfig)         (GDK_GL_CONFIG_IMPL_X11 (glconfig)->xvinfo)
#define GDK_GL_CONFIG_XCOLORMAP(glconfig)      (GDK_COLORMAP_XCOLORMAP (GDK_GL_CONFIG_IMPL_X11 (glconfig)->colormap))
#define GDK_GL_CONTEXT_GLXCONTEXT(glcontext)   (GDK_GL_CONTEXT_IMPL_X11 (glcontext)->glxcontext)
#define GDK_GL_PIXMAP_GLXPIXMAP(glpixmap)      (GDK_GL_PIXMAP_IMPL_X11 (glpixmap)->glxpixmap)
#define GDK_GL_WINDOW_GLXWINDOW(glwindow)      (GDK_GL_WINDOW_IMPL_X11 (glwindow)->glxwindow)

#else

#define GDK_GL_CONFIG_XDISPLAY(glconfig)       (gdk_x11_gl_config_get_xdisplay (glconfig))
#define GDK_GL_CONFIG_SCREEN_XNUMBER(glconfig) (gdk_x11_gl_config_get_screen_number (glconfig))
#define GDK_GL_CONFIG_XVINFO(glconfig)         (gdk_x11_gl_config_get_xvinfo (glconfig))
#define GDK_GL_CONFIG_XCOLORMAP(glconfig)      (GDK_COLORMAP_XCOLORMAP (gdk_gl_config_get_colormap (glconfig)))
#define GDK_GL_CONTEXT_GLXCONTEXT(glcontext)   (gdk_x11_gl_context_get_glxcontext (glcontext))
#define GDK_GL_PIXMAP_GLXPIXMAP(glpixmap)      (gdk_x11_gl_pixmap_get_glxpixmap (glpixmap))
#define GDK_GL_WINDOW_GLXWINDOW(glwindow)      (gdk_x11_gl_window_get_glxwindow (glwindow))

#endif

G_END_DECLS

#endif /* __GDK_GL_X_H__ */
