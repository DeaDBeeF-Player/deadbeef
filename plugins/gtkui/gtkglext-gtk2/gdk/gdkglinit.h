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

#ifndef __GDK_GL_INIT_H__
#define __GDK_GL_INIT_H__

#include <gdk/gdkgldefs.h>
#include <gdk/gdkgltypes.h>

G_BEGIN_DECLS

/*
 * Initialization routines.
 */

gboolean gdk_gl_parse_args (int    *argc,
                            char ***argv);

gboolean gdk_gl_init_check (int    *argc,
                            char ***argv);

void     gdk_gl_init       (int    *argc,
                            char ***argv);

G_END_DECLS

#endif /* __GDK_GL_INIT_H__ */
