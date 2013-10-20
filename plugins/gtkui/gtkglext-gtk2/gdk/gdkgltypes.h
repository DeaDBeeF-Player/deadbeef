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

#ifndef __GDK_GL_TYPES_H__
#define __GDK_GL_TYPES_H__

#include <gdk/gdktypes.h>

G_BEGIN_DECLS
 
/*
 * Forward declarations of commonly used types
 */

typedef void (*GdkGLProc)(void);

typedef struct _GdkGLConfig   GdkGLConfig;
typedef struct _GdkGLContext  GdkGLContext;

typedef struct _GdkGLDrawable GdkGLDrawable;

typedef struct _GdkGLPixmap   GdkGLPixmap;
typedef struct _GdkGLWindow   GdkGLWindow;

G_END_DECLS

#endif /* __GDK_GL_TYPES_H__ */
