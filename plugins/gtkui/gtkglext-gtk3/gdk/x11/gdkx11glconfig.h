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

#if !defined (__GDKGLX_H_INSIDE__) && !defined (GDK_GL_COMPILATION)
#error "Only <gdk/gdkglx.h> can be included directly."
#endif

#ifndef __GDK_X11_GL_CONFIG_H__
#define __GDK_X11_GL_CONFIG_H__

#include <gdk/gdkx.h>
#include <gdk/gdkgl.h>

G_BEGIN_DECLS

#define GDK_TYPE_X11_GL_CONFIG             (gdk_x11_gl_config_get_type ())
#define GDK_X11_GL_CONFIG(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_X11_GL_CONFIG, GdkX11GLConfig))
#define GDK_X11_GL_CONFIG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_X11_GL_CONFIG, GdkX11GLConfigClass))
#define GDK_IS_X11_GL_CONFIG(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_X11_GL_CONFIG))
#define GDK_IS_X11_GL_CONFIG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_X11_GL_CONFIG))
#define GDK_X11_GL_CONFIG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_X11_GL_CONFIG, GdkX11GLConfigClass))

#ifdef INSIDE_GDK_GL_X11
typedef struct _GdkX11GLConfig GdkX11GLConfig;
#else
typedef GdkGLConfig GdkX11GLConfig;
#endif
typedef struct _GdkX11GLConfigClass GdkX11GLConfigClass;

GType        gdk_x11_gl_config_get_type (void);

GdkGLConfig *gdk_x11_gl_config_new_for_display  (GdkDisplay *display,
                                                 const int  *attrib_list,
                                                 gsize       n_attribs);

GdkGLConfig *gdk_x11_gl_config_new_for_screen   (GdkScreen *screen,
                                                 const int *attrib_list,
                                                 gsize      n_attribs);

#ifndef GDK_MULTIHEAD_SAFE
GdkGLConfig *gdk_x11_gl_config_new_from_visualid            (VisualID   xvisualid);
#endif /* GDK_MULTIHEAD_SAFE */

GdkGLConfig *gdk_x11_gl_config_new_from_visualid_for_screen (GdkScreen *screen,
                                                             VisualID   xvisualid);

Display     *gdk_x11_gl_config_get_xdisplay      (GdkGLConfig  *glconfig);
int          gdk_x11_gl_config_get_screen_number (GdkGLConfig  *glconfig);
XVisualInfo *gdk_x11_gl_config_get_xvinfo        (GdkGLConfig  *glconfig);

#ifdef INSIDE_GDK_GL_X11

#define GDK_GL_CONFIG_XDISPLAY(glconfig)       (GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->xdisplay)
#define GDK_GL_CONFIG_SCREEN_XNUMBER(glconfig) (GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->screen_num)
#define GDK_GL_CONFIG_XVINFO(glconfig)         (GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->xvinfo)

#else

#define GDK_GL_CONFIG_XDISPLAY(glconfig)       (gdk_x11_gl_config_get_xdisplay (glconfig))
#define GDK_GL_CONFIG_SCREEN_XNUMBER(glconfig) (gdk_x11_gl_config_get_screen_number (glconfig))
#define GDK_GL_CONFIG_XVINFO(glconfig)         (gdk_x11_gl_config_get_xvinfo (glconfig))

#endif

G_END_DECLS

#endif /* __GDK_X11_GL_CONFIG_H__ */
