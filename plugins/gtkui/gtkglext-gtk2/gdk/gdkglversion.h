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

#ifndef __GDK_GL_VERSION_H__
#define __GDK_GL_VERSION_H__

#include <glib.h>

#include <gdk/gdkgldefs.h>

G_BEGIN_DECLS

/*
 * Compile time version.
 */
#define GDKGLEXT_MAJOR_VERSION (1)
#define GDKGLEXT_MINOR_VERSION (2)
#define GDKGLEXT_MICRO_VERSION (0)
#define GDKGLEXT_INTERFACE_AGE (0)
#define GDKGLEXT_BINARY_AGE    (0)

/*
 * Check whether a GdkGLExt version equal to or greater than
 * major.minor.micro is present.
 */
#define	GDKGLEXT_CHECK_VERSION(major, minor, micro)                          \
  (GDKGLEXT_MAJOR_VERSION > (major) ||                                       \
  (GDKGLEXT_MAJOR_VERSION == (major) && GDKGLEXT_MINOR_VERSION > (minor)) || \
  (GDKGLEXT_MAJOR_VERSION == (major) && GDKGLEXT_MINOR_VERSION == (minor) && \
   GDKGLEXT_MICRO_VERSION >= (micro)))

/*
 * Library version.
 */
GDK_GL_VAR const guint gdkglext_major_version;
GDK_GL_VAR const guint gdkglext_minor_version;
GDK_GL_VAR const guint gdkglext_micro_version;
GDK_GL_VAR const guint gdkglext_interface_age;
GDK_GL_VAR const guint gdkglext_binary_age;

G_END_DECLS

#endif /* __GDK_GL_VERSION_H__ */
