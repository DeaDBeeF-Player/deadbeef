/* GtkGLExt - OpenGL Extension to GTK+
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

#ifndef __GTK_GL_VERSION_H__
#define __GTK_GL_VERSION_H__

#include <glib.h>

#include <gtk/gtkgldefs.h>

G_BEGIN_DECLS

/*
 * Compile time version.
 */
#define GTKGLEXT_MAJOR_VERSION (1)
#define GTKGLEXT_MINOR_VERSION (2)
#define GTKGLEXT_MICRO_VERSION (0)
#define GTKGLEXT_INTERFACE_AGE (0)
#define GTKGLEXT_BINARY_AGE    (0)

/*
 * Check whether a GtkGLExt version equal to or greater than
 * major.minor.micro is present.
 */
#define	GTKGLEXT_CHECK_VERSION(major, minor, micro)                          \
  (GTKGLEXT_MAJOR_VERSION > (major) ||                                       \
  (GTKGLEXT_MAJOR_VERSION == (major) && GTKGLEXT_MINOR_VERSION > (minor)) || \
  (GTKGLEXT_MAJOR_VERSION == (major) && GTKGLEXT_MINOR_VERSION == (minor) && \
   GTKGLEXT_MICRO_VERSION >= (micro)))

/*
 * Library version.
 */
GTK_GL_VAR const guint gtkglext_major_version;
GTK_GL_VAR const guint gtkglext_minor_version;
GTK_GL_VAR const guint gtkglext_micro_version;
GTK_GL_VAR const guint gtkglext_interface_age;
GTK_GL_VAR const guint gtkglext_binary_age;

G_END_DECLS

#endif /* __GTK_GL_VERSION_H__ */
