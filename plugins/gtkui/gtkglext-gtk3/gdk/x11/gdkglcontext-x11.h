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

#ifndef __GDK_GL_CONTEXT_X11_H__
#define __GDK_GL_CONTEXT_X11_H__

#include <GL/glx.h>

#include <gdk/gdkglcontext.h>
#include <gdk/gdkglcontextimpl.h>

G_BEGIN_DECLS

typedef struct _GdkGLContextImplX11      GdkGLContextImplX11;
typedef struct _GdkGLContextImplX11Class GdkGLContextImplX11Class;

#define GDK_TYPE_GL_CONTEXT_IMPL_X11            (gdk_gl_context_impl_x11_get_type ())
#define GDK_GL_CONTEXT_IMPL_X11(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GL_CONTEXT_IMPL_X11, GdkGLContextImplX11))
#define GDK_GL_CONTEXT_IMPL_X11_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GL_CONTEXT_IMPL_X11, GdkGLContextImplX11Class))
#define GDK_IS_GL_CONTEXT_IMPL_X11(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GL_CONTEXT_IMPL_X11))
#define GDK_IS_GL_CONTEXT_IMPL_X11_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GL_CONTEXT_IMPL_X11))
#define GDK_GL_CONTEXT_IMPL_X11_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GL_CONTEXT_IMPL_X11, GdkGLContextImplX11Class))

struct _GdkGLContextImplX11
{
  GdkGLContextImpl parent_instance;

  GLXContext glxcontext;
  GdkGLContext *share_list;
  gboolean is_direct;
  int render_type;

  GdkGLConfig *glconfig;

  GdkGLDrawable *gldrawable;
  GdkGLDrawable *gldrawable_read; /* currently unused. */

  guint is_destroyed : 1;
  guint is_foreign   : 1;
};

struct _GdkGLContextImplX11Class
{
  GdkGLContextImplClass parent_class;

  GLXContext (*get_glxcontext) (GdkGLContext *glcontext);
};

GType gdk_gl_context_impl_x11_get_type (void);

GdkGLContextImpl *_gdk_x11_gl_context_impl_new (GdkGLContext  *glcontext,
                                                GdkGLDrawable *gldrawable,
                                                GdkGLContext  *share_list,
                                                gboolean       direct,
                                                int            render_type);

GdkGLContextImpl *_gdk_x11_gl_context_impl_new_from_glxcontext (GdkGLContext *glcontext,
                                                                GdkGLConfig  *glconfig,
                                                                GdkGLContext *share_list,
                                                                GLXContext    glxcontext);

void _gdk_x11_gl_context_impl_set_gl_drawable (GdkGLContext  *glcontext,
                                               GdkGLDrawable *gldrawable);

GdkGLContext *
_gdk_x11_gl_context_impl_get_current (void);

G_END_DECLS

#endif /* __GDK_GL_CONTEXT_X11_H__ */
