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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "gdkglversion.h"

/**
 * gdk_gl_get_major_version:
 *
 * Returns the GtkGLExt library major version number.
 *
 * Returns: The major version number of the GtkGLExt library.
 **/
int
gdk_gl_get_major_version (void)
{
        return (GDKGLEXT_MAJOR_VERSION);
}

/**
 * gdk_gl_get_minor_version:
 *
 * Returns the GtkGLExt library minor version number.
 *
 * Returns: The minor version number of the GtkGLExt library.
 **/
int
gdk_gl_get_minor_version (void)
{
        return (GDKGLEXT_MINOR_VERSION);
}

/**
 * gdk_gl_get_micro_version:
 *
 * Returns the GtkGLExt library micro version number.
 *
 * Returns: The micro version number of the GtkGLExt library.
 **/
int
gdk_gl_get_micro_version (void)
{
        return (GDKGLEXT_MICRO_VERSION);
}

/**
 * gdk_gl_get_interface_age:
 *
 * Returns the GtkGLExt library interface age.
 *
 * Returns: The interface age of the GtkGLExt library.
 **/
int
gdk_gl_get_interface_age (void)
{
        return (GDKGLEXT_INTERFACE_AGE);
}

/**
 * gdk_gl_get_binary_age:
 *
 * Returns the GtkGLExt library binary age.
 *
 * Returns: The binary age of the GtkGLExt library.
 **/
int
gdk_gl_get_binary_age (void)
{
        return (GDKGLEXT_BINARY_AGE);
}
