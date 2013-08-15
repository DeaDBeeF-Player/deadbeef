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

#ifndef __GDK_GL_CONFIG_X11_H__
#define __GDK_GL_CONFIG_X11_H__

#include <gdk/gdkx.h>

#include <gdk/gdkglconfig.h>
#include <gdk/gdkglconfigimpl.h>

G_BEGIN_DECLS

typedef struct _GdkGLConfigImplX11      GdkGLConfigImplX11;
typedef struct _GdkGLConfigImplX11Class GdkGLConfigImplX11Class;

#define GDK_TYPE_GL_CONFIG_IMPL_X11              (gdk_gl_config_impl_x11_get_type ())
#define GDK_GL_CONFIG_IMPL_X11(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_CONFIG_IMPL_X11, GdkGLConfigImplX11))
#define GDK_GL_CONFIG_IMPL_X11_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_CONFIG_IMPL_X11, GdkGLConfigImplX11Class))
#define GDK_IS_GL_CONFIG_IMPL_X11(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_CONFIG_IMPL_X11))
#define GDK_IS_GL_CONFIG_IMPL_X11_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_CONFIG_IMPL_X11))
#define GDK_GL_CONFIG_IMPL_X11_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_CONFIG_IMPL_X11, GdkGLConfigImplX11Class))

struct _GdkGLConfigImplX11
{
  GdkGLConfigImpl parent_instance;

  Display *xdisplay;
  int screen_num;
  XVisualInfo *xvinfo;

  GdkScreen *screen;
};

struct _GdkGLConfigImplX11Class
{
  GdkGLConfigImplClass parent_class;

  Display*      (*get_xdisplay)      (GdkGLConfig  *glconfig);
  int           (*get_screen_number) (GdkGLConfig  *glconfig);
  XVisualInfo*  (*get_xvinfo)        (GdkGLConfig  *glconfig);
};

GType gdk_gl_config_impl_x11_get_type (void);

GdkGLConfig *_gdk_x11_gl_config_impl_new                          (GdkGLConfig *glconfig,
                                                                   const int   *attrib_list,
                                                                   gsize        n_attribs);
GdkGLConfig *_gdk_x11_gl_config_impl_new_for_screen               (GdkGLConfig *glconfig,
                                                                   GdkScreen   *screen,
                                                                   const int   *attrib_list,
                                                                   gsize        n_attribs);
GdkGLConfig *_gdk_x11_gl_config_impl_new_from_visualid_for_screen (GdkGLConfig *glconfig,
                                                                   GdkScreen   *screen,
                                                                   VisualID     xvisualid);

G_END_DECLS

#endif /* __GDK_GL_CONFIG_X11_H__ */
