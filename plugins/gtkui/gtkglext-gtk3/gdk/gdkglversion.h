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

#if !defined (__GDKGL_H_INSIDE__) && !defined (GDK_GL_COMPILATION)
#error "Only <gdk/gdkgl.h> can be included directly."
#endif

#ifndef __GDK_GL_VERSION_H__
#define __GDK_GL_VERSION_H__

#include <glib.h>

#include <gdk/gdkgldefs.h>

G_BEGIN_DECLS

/*
 * Compile time version.
 */
#define GDKGLEXT_MAJOR_VERSION (2)
#define GDKGLEXT_MINOR_VERSION (99)
#define GDKGLEXT_MICRO_VERSION (0)
#define GDKGLEXT_INTERFACE_AGE (0)
#define GDKGLEXT_BINARY_AGE    (9900)

/*
 * Check whether a GdkGLExt version equal to or greater than
 * major.minor.micro is present.
 */
#define	GDKGLEXT_CHECK_VERSION(major, minor, micro)                          \
  (GDKGLEXT_MAJOR_VERSION > (major) ||                                       \
  (GDKGLEXT_MAJOR_VERSION == (major) && GDKGLEXT_MINOR_VERSION > (minor)) || \
  (GDKGLEXT_MAJOR_VERSION == (major) && GDKGLEXT_MINOR_VERSION == (minor) && \
   GDKGLEXT_MICRO_VERSION >= (micro)))

int
gdk_gl_get_major_version (void);

int
gdk_gl_get_minor_version (void);

int
gdk_gl_get_micro_version (void);

int
gdk_gl_get_interface_age (void);

int
gdk_gl_get_binary_age (void);

G_END_DECLS

#endif /* __GDK_GL_VERSION_H__ */
