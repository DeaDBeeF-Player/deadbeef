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

#ifndef __GDK_GL_DEFS_H__
#define __GDK_GL_DEFS_H__

#include <glib.h>

#include <gdkglext-config.h>

G_BEGIN_DECLS

#ifdef G_OS_WIN32
#  ifdef GDK_GL_COMPILATION
#    define GDK_GL_VAR __declspec(dllexport)
#  else
#    define GDK_GL_VAR extern __declspec(dllimport)
#  endif
#else
#  define GDK_GL_VAR extern
#endif

G_END_DECLS

#endif /* __GDK_GL_DEFS_H__ */
