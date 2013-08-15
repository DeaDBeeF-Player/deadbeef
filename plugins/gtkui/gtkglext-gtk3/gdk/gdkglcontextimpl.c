/* GdkGLExt - OpenGL Extension to GDK
 * Copyright (C) 2012  Thomas Zimmermann
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
#endif

#include <gdk/gdk.h>            /* for gdk_error_trap_(push|pop) () */

#include "gdkgldebug.h"
#include "gdkglcontextimpl.h"

G_DEFINE_TYPE (GdkGLContextImpl, gdk_gl_context_impl, G_TYPE_OBJECT);

static void
gdk_gl_context_impl_init (GdkGLContextImpl *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_gl_context_impl_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (gdk_gl_context_impl_parent_class)->finalize (object);
}

static void
gdk_gl_context_impl_class_init (GdkGLContextImplClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->copy_gl_context_impl = NULL;
  klass->get_gl_drawable = NULL;
  klass->get_gl_config = NULL;
  klass->get_share_list = NULL;
  klass->is_direct = NULL;
  klass->get_render_type = NULL;

  object_class->finalize = gdk_gl_context_impl_finalize;
}
