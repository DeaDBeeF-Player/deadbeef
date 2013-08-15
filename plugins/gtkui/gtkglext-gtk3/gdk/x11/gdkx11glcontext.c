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

#include "gdkglcontext-x11.h"

struct _GdkX11GLContext
{
  GdkGLContext parent;
};

struct _GdkX11GLContextClass
{
  GdkGLContextClass parent_class;
};

G_DEFINE_TYPE (GdkX11GLContext, gdk_x11_gl_context, GDK_TYPE_GL_CONTEXT);

static void
gdk_x11_gl_context_init (GdkX11GLContext *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_x11_gl_context_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (gdk_x11_gl_context_parent_class)->finalize (object);
}

static void
gdk_x11_gl_context_class_init (GdkX11GLContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  object_class->finalize = gdk_x11_gl_context_finalize;
}

/**
 * gdk_x11_gl_context_foreign_new:
 * @glconfig: #GdkGLConfig that represents the visual the GLXContext uses.
 * @share_list: the #GdkGLContext which shares display lists with the
 *              GLXContext, or NULL.
 * @glxcontext: exsisting GLXContext.
 *
 * Creates #GdkGLContext from existing GLXContext.
 *
 * Return value: the newly-created #GdkGLContext wrapper.
 **/
GdkGLContext *
gdk_x11_gl_context_foreign_new (GdkGLConfig  *glconfig,
                                GdkGLContext *share_list,
                                GLXContext    glxcontext)
{
  GdkGLContext *glcontext;
  GdkGLContextImpl *impl;

  GDK_GL_NOTE_FUNC ();

  glcontext = g_object_new(GDK_TYPE_GL_CONTEXT_IMPL_X11, NULL);

  g_return_val_if_fail(glcontext != NULL, NULL);

  impl = _gdk_x11_gl_context_impl_new_from_glxcontext(glcontext,
                                                      glconfig,
                                                      share_list,
                                                      glxcontext);
  if (impl == NULL)
    g_object_unref(glcontext);

  g_return_val_if_fail(impl != NULL, NULL);

  return glcontext;
}

/**
 * gdk_x11_gl_context_get_glxcontext:
 * @glcontext: a #GdkGLContext.
 *
 * Gets GLXContext.
 *
 * Return value: the GLXContext.
 **/
GLXContext
gdk_x11_gl_context_get_glxcontext (GdkGLContext *glcontext)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONTEXT (glcontext), NULL);

  return GDK_GL_CONTEXT_IMPL_X11_CLASS (glcontext)->get_glxcontext(glcontext);
}
