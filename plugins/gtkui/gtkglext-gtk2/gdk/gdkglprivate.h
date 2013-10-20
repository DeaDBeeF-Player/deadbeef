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

#ifndef __GDK_GL_PRIVATE_H__
#define __GDK_GL_PRIVATE_H__

#include <gdk/gdkgldefs.h>
#include <gdk/gdkgldebug.h>
#include <gdk/gdkgltokens.h>
#include <gdk/gdkgltypes.h>
#include <gdk/gdkgldrawable.h>

G_BEGIN_DECLS

#define _GDK_GL_CONCAT(x, y) x##y

#define _GDK_GL_CONFIG_AS_SINGLE_MODE(glconfig) ((glconfig)->as_single_mode)

void _gdk_gl_context_destroy (GdkGLContext  *glcontext);

void _gdk_gl_pixmap_destroy  (GdkGLPixmap   *glpixmap);
void _gdk_gl_pixmap_get_size (GdkGLDrawable *gldrawable,
                              gint          *width,
                              gint          *height);

void _gdk_gl_window_destroy  (GdkGLWindow   *glwindow);
void _gdk_gl_window_get_size (GdkGLDrawable *gldrawable,
                              gint          *width,
                              gint          *height);

void _gdk_gl_print_gl_info (void);

/* Internal globals */

extern gboolean _gdk_gl_config_no_standard_colormap;

extern gboolean _gdk_gl_context_force_indirect;

G_END_DECLS

#endif /* __GDK_GL_PRIVATE_H__ */
