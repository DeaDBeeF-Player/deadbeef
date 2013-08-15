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

#include "gdkglwindow-x11.h"

struct _GdkX11GLWindow
{
  GdkGLWindow parent;
};

struct _GdkX11GLWindowClass
{
  GdkGLWindowClass parent_class;
};

G_DEFINE_TYPE (GdkX11GLWindow,
               gdk_x11_gl_window,
               GDK_TYPE_GL_WINDOW)

static void
gdk_x11_gl_window_init (GdkX11GLWindow *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_x11_gl_window_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (gdk_x11_gl_window_parent_class)->finalize (object);
}

static void
gdk_x11_gl_window_class_init (GdkX11GLWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  object_class->finalize = gdk_x11_gl_window_finalize;
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
  g_return_val_if_fail (GDK_IS_X11_GL_WINDOW (glwindow), None);

  return GDK_GL_WINDOW_IMPL_X11_CLASS (glwindow->impl)->get_glxwindow(glwindow);
}
