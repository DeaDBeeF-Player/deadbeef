/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2012 Thomas Zimmermann
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

#include <gdk/gdkgldebug.h>
#include <gdk/x11/gdkglx.h>

#include "gdkglconfig-x11.h"

struct _GdkX11GLConfig
{
  GdkGLConfig parent;
};

struct _GdkX11GLConfigClass
{
  GdkGLConfigClass parent_class;
};

G_DEFINE_TYPE (GdkX11GLConfig, gdk_x11_gl_config, GDK_TYPE_GL_CONFIG);

static void
gdk_x11_gl_config_init (GdkX11GLConfig *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_x11_gl_config_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (gdk_x11_gl_config_parent_class)->finalize (object);
}

static void
gdk_x11_gl_config_class_init (GdkX11GLConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  object_class->finalize = gdk_x11_gl_config_finalize;
}

/**
 * gdk_x11_gl_config_new_for_display:
 * @display: display.
 * @attrib_list: (array length=n_attribs): the attribute list.
 * @n_attribs: the number of attributes and values in attrib_list
 *
 * Creates a #GdkGLConfig on the given display.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_x11_gl_config_new_for_display(GdkDisplay *display, const int *attrib_list, gsize n_attribs)
{
  GdkGLConfig *glconfig;
  GdkGLConfig *impl;

  g_return_val_if_fail(GDK_IS_X11_DISPLAY(display), NULL);

  glconfig = g_object_new(GDK_TYPE_X11_GL_CONFIG, NULL);

  g_return_val_if_fail(glconfig != NULL, NULL);

  impl = _gdk_x11_gl_config_impl_new(glconfig, attrib_list, n_attribs);

  if (impl == NULL)
    g_object_unref(glconfig);

  g_return_val_if_fail(impl != NULL, NULL);

  return glconfig;
}

/**
 * gdk_x11_gl_config_new_for_screen:
 * @screen: target screen.
 * @attrib_list: (array length=n_attribs): the attribute list.
 * @n_attribs: the number of attributes and values in attrib_list
 *
 * Creates a #GdkGLConfig on the given display.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_x11_gl_config_new_for_screen(GdkScreen *screen, const int *attrib_list, gsize n_attribs)
{
  GdkGLConfig *glconfig;
  GdkGLConfig *impl;

  g_return_val_if_fail(GDK_IS_X11_SCREEN(screen), NULL);

  glconfig = g_object_new(GDK_TYPE_X11_GL_CONFIG, NULL);

  g_return_val_if_fail(glconfig != NULL, NULL);

  impl = _gdk_x11_gl_config_impl_new(glconfig, attrib_list, n_attribs);

  if (impl == NULL)
    g_object_unref(glconfig);

  g_return_val_if_fail(impl != NULL, NULL);

  return glconfig;
}

/**
 * gdk_x11_gl_config_new_from_visualid:
 * @xvisualid: visual ID.
 *
 * Creates #GdkGLConfig from given visual ID that specifies the OpenGL-capable
 * visual.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_x11_gl_config_new_from_visualid (VisualID xvisualid)
{
  GdkScreen *screen;
  GdkGLConfig *glconfig;
  GdkGLConfig *impl;

  GDK_GL_NOTE_FUNC ();

  screen = gdk_screen_get_default ();

  glconfig = g_object_new(GDK_TYPE_X11_GL_CONFIG, NULL);

  g_return_val_if_fail(glconfig != NULL, NULL);

  impl = _gdk_x11_gl_config_impl_new_from_visualid_for_screen (glconfig,
                                                               screen,
                                                               xvisualid);
  if (impl == NULL)
    g_object_unref(glconfig);

  g_return_val_if_fail(impl != NULL, NULL);

  return glconfig;
}

/**
 * gdk_x11_gl_config_new_from_visualid_for_screen:
 * @screen: target screen.
 * @xvisualid: visual ID.
 *
 * Creates #GdkGLConfig from given visual ID that specifies the OpenGL-capable
 * visual.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_x11_gl_config_new_from_visualid_for_screen (GdkScreen *screen,
                                                VisualID   xvisualid)
{
  GdkGLConfig *glconfig;
  GdkGLConfig *impl;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  glconfig = g_object_new(GDK_TYPE_X11_GL_CONFIG, NULL);

  g_return_val_if_fail(glconfig != NULL, NULL);

  impl = _gdk_x11_gl_config_impl_new_from_visualid_for_screen (glconfig,
                                                               screen,
                                                               xvisualid);
  if (impl == NULL)
    g_object_unref(glconfig);

  g_return_val_if_fail(impl != NULL, NULL);

  return glconfig;
}

/**
 * gdk_x11_gl_config_get_xdisplay:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets X Display.
 *
 * Return value: pointer to the Display.
 **/
Display *
gdk_x11_gl_config_get_xdisplay (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11_CLASS (glconfig)->get_xdisplay(glconfig);
}

/**
 * gdk_x11_gl_config_get_screen_number:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets X screen number.
 *
 * Return value: the screen number.
 **/
int
gdk_x11_gl_config_get_screen_number (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), 0);

  return GDK_GL_CONFIG_IMPL_X11_CLASS (glconfig)->get_screen_number(glconfig);
}

/**
 * gdk_x11_gl_config_get_xvinfo:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets XVisualInfo data.
 *
 * Return value: pointer to the XVisualInfo data.
 **/
XVisualInfo *
gdk_x11_gl_config_get_xvinfo (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11_CLASS (glconfig)->get_xvinfo(glconfig);
}
