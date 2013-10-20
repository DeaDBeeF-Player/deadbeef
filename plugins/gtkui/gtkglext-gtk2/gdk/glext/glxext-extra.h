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

/*
 * Additional GLX extensions.
 */

#ifndef __glxext_extra_h_
#define __glxext_extra_h_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

/*
 * 3DFX
 */

/*
 * ARB
 */

/*
 * EXT
 */

/* unknown */
#ifndef GLX_EXT_scene_marker
/*
#define GLX_SCENE_REQUIRED_EXT          0
*/
#endif

/* unknown */
#ifndef GLX_EXT_scene_marker
/* #define GLX_EXT_scene_marker 1 */
#endif

/*
 * MESA
 */

#ifndef GLX_MESA_agp_offset
#define GLX_MESA_agp_offset 1
#ifdef GLX_GLXEXT_PROTOTYPES
extern GLuint glXGetAGPOffsetMESA (const GLvoid *);
#endif /* GLX_GLXEXT_PROTOTYPES */
typedef GLuint ( * PFNGLXGETAGPOFFSETMESAPROC) (const GLvoid *pointer);
#endif

/*
 * NV
 */

#ifndef GLX_NV_float_buffer
#define GLX_FLOAT_COMPONENTS_NV         0x20B0
#endif

#ifndef GLX_NV_float_buffer
#define GLX_NV_float_buffer 1
#endif

#ifndef GLX_NV_vertex_array_range
#define GLX_NV_vertex_array_range 1
#ifdef GLX_GLXEXT_PROTOTYPES
extern void *glXAllocateMemoryNV (GLsizei, GLfloat, GLfloat, GLfloat);
extern void glXFreeMemoryNV (void *);
#endif /* GLX_GLXEXT_PROTOTYPES */
typedef void * ( * PFNGLXALLOCATEMEMORYNVPROC) (GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
typedef void ( * PFNGLXFREEMEMORYNVPROC) (void *pointer);
#endif

/*
 * OML
 */

/*
 * SGI
 */

/*
 * SGIS
 */

/* unknown */
#ifndef GLX_SGIS_color_range
/*
#define GLX_EXTENDED_RANGE_SGIS         0
#define GLX_MIN_RED_SGIS                0
#define GLX_MAX_RED_SGIS                0
#define GLX_MIN_GREEN_SGIS              0
#define GLX_MAX_GREEN_SGIS              0
#define GLX_MIN_BLUE_SGIS               0
#define GLX_MAX_BLUE_SGIS               0
#define GLX_MIN_ALPHA_SGIS              0
#define GLX_MAX_ALPHA_SGIS              0
*/
#endif

/* unknown */
#ifndef GLX_SGIS_color_range
/* #define GLX_SGIS_color_range 1 */
#endif

/*
 * SGIX
 */

/*
 * SUN
 */

#ifdef __cplusplus
}
#endif

#endif /* __glxext_extra_h_ */
