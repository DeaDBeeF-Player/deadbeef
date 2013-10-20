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
 * Additional WGL extensions.
 */

#ifndef __wglext_extra_h_
#define __wglext_extra_h_

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
 * ATI
 */

#ifndef WGL_ATI_pixel_format_float
#define WGL_TYPE_RGBA_FLOAT_ATI            0x21A0
#define GL_TYPE_RGBA_FLOAT_ATI             0x8820
#define GL_COLOR_CLEAR_UNCLAMPED_VALUE_ATI 0x8835
#endif

#ifndef WGL_ATI_pixel_format_float
#define WGL_ATI_pixel_format_float  1
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

/*
 * I3D
 */

#ifndef WGL_I3D_unknown_genlock_extension_name
#define WGL_GENLOCK_SOURCE_MULTIVIEW_I3D     0x2044
#define WGL_GENLOCK_SOURCE_EXTENAL_SYNC_I3D  0x2045
#define WGL_GENLOCK_SOURCE_EXTENAL_FIELD_I3D 0x2046
#define WGL_GENLOCK_SOURCE_EXTENAL_TTL_I3D   0x2047
#define WGL_GENLOCK_SOURCE_DIGITAL_SYNC_I3D  0x2048
#define WGL_GENLOCK_SOURCE_DIGITAL_FIELD_I3D 0x2049
#define WGL_GENLOCK_SOURCE_EDGE_FALLING_I3D  0x204A
#define WGL_GENLOCK_SOURCE_EDGE_RISING_I3D   0x204B
#define WGL_GENLOCK_SOURCE_EDGE_BOTH_I3D     0x204C
#endif

#ifndef WGL_I3D_unknown_gamma_extension_name
#define WGL_GAMMA_TABLE_SIZE_I3D       0x204E
#define WGL_GAMMA_EXCLUDE_DESKTOP_I3D  0x204F
#endif

#ifndef WGL_I3D_unknown_digital_video_cursor_extension_name
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_FRAMEBUFFER_I3D 0x2050
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_VALUE_I3D       0x2051
#define WGL_DIGITAL_VIDEO_CURSOR_INCLUDED_I3D          0x2052
#define WGL_DIGITAL_VIDEO_GAMMA_CORRECTED_I3D          0x2053
#endif

/*
 * NV
 */

/*
 * OML
 */

#ifdef __cplusplus
}
#endif

#endif /* __wglext_extra_h_ */
