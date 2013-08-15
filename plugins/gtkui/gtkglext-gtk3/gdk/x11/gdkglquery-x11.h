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

#ifndef __GDK_GL_QUERY_X11_H__
#define __GDK_GL_QUERY_X11_H__

#include "gdkglquery.h"

G_BEGIN_DECLS

gboolean
_gdk_x11_gl_query_extension_for_display (GdkDisplay *display);

gboolean
_gdk_x11_gl_query_version_for_display (GdkDisplay *display,
                                       int        *major,
                                       int        *minor);

GdkGLProc
_gdk_x11_gl_get_proc_address (const char *proc_name);

G_END_DECLS

#endif /* __GDK_GL_QUERY_X11_H__ */
