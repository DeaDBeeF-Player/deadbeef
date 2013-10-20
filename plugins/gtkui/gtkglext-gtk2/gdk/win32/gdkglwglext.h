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
 * This is a generated file.  Please modify "gen-gdkglwglext-h.pl".
 */

#ifndef __GDK_GL_WGLEXT_H__
#define __GDK_GL_WGLEXT_H__

#include <glib.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <GL/gl.h>

#include <gdk/gdkgldefs.h>
#include <gdk/gdkglquery.h>
#include <gdk/gdkglconfig.h>

#undef __wglext_h_
#undef WGL_WGLEXT_VERSION
#include <gdk/glext/wglext.h>
#include <gdk/glext/wglext-extra.h>

G_BEGIN_DECLS

/*
 * WGL_ARB_buffer_region
 */

/* wglCreateBufferRegionARB */
typedef HANDLE (WINAPI * GdkGLProc_wglCreateBufferRegionARB) (HDC hDC, int iLayerPlane, UINT uType);
GdkGLProc    gdk_gl_get_wglCreateBufferRegionARB (void);
#define      gdk_gl_wglCreateBufferRegionARB(proc, hDC, iLayerPlane, uType) \
  ( ((GdkGLProc_wglCreateBufferRegionARB) (proc)) (hDC, iLayerPlane, uType) )

/* wglDeleteBufferRegionARB */
typedef VOID (WINAPI * GdkGLProc_wglDeleteBufferRegionARB) (HANDLE hRegion);
GdkGLProc    gdk_gl_get_wglDeleteBufferRegionARB (void);
#define      gdk_gl_wglDeleteBufferRegionARB(proc, hRegion) \
  ( ((GdkGLProc_wglDeleteBufferRegionARB) (proc)) (hRegion) )

/* wglSaveBufferRegionARB */
typedef BOOL (WINAPI * GdkGLProc_wglSaveBufferRegionARB) (HANDLE hRegion, int x, int y, int width, int height);
GdkGLProc    gdk_gl_get_wglSaveBufferRegionARB (void);
#define      gdk_gl_wglSaveBufferRegionARB(proc, hRegion, x, y, width, height) \
  ( ((GdkGLProc_wglSaveBufferRegionARB) (proc)) (hRegion, x, y, width, height) )

/* wglRestoreBufferRegionARB */
typedef BOOL (WINAPI * GdkGLProc_wglRestoreBufferRegionARB) (HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
GdkGLProc    gdk_gl_get_wglRestoreBufferRegionARB (void);
#define      gdk_gl_wglRestoreBufferRegionARB(proc, hRegion, x, y, width, height, xSrc, ySrc) \
  ( ((GdkGLProc_wglRestoreBufferRegionARB) (proc)) (hRegion, x, y, width, height, xSrc, ySrc) )

/* proc struct */

typedef struct _GdkGL_WGL_ARB_buffer_region GdkGL_WGL_ARB_buffer_region;

struct _GdkGL_WGL_ARB_buffer_region
{
  GdkGLProc_wglCreateBufferRegionARB wglCreateBufferRegionARB;
  GdkGLProc_wglDeleteBufferRegionARB wglDeleteBufferRegionARB;
  GdkGLProc_wglSaveBufferRegionARB wglSaveBufferRegionARB;
  GdkGLProc_wglRestoreBufferRegionARB wglRestoreBufferRegionARB;
};

GdkGL_WGL_ARB_buffer_region *gdk_gl_get_WGL_ARB_buffer_region (GdkGLConfig *glconfig);

/*
 * WGL_ARB_extensions_string
 */

/* wglGetExtensionsStringARB */
typedef const char * (WINAPI * GdkGLProc_wglGetExtensionsStringARB) (HDC hdc);
GdkGLProc    gdk_gl_get_wglGetExtensionsStringARB (void);
#define      gdk_gl_wglGetExtensionsStringARB(proc, hdc) \
  ( ((GdkGLProc_wglGetExtensionsStringARB) (proc)) (hdc) )

/* proc struct */

typedef struct _GdkGL_WGL_ARB_extensions_string GdkGL_WGL_ARB_extensions_string;

struct _GdkGL_WGL_ARB_extensions_string
{
  GdkGLProc_wglGetExtensionsStringARB wglGetExtensionsStringARB;
};

GdkGL_WGL_ARB_extensions_string *gdk_gl_get_WGL_ARB_extensions_string (GdkGLConfig *glconfig);

/*
 * WGL_ARB_pixel_format
 */

/* wglGetPixelFormatAttribivARB */
typedef BOOL (WINAPI * GdkGLProc_wglGetPixelFormatAttribivARB) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
GdkGLProc    gdk_gl_get_wglGetPixelFormatAttribivARB (void);
#define      gdk_gl_wglGetPixelFormatAttribivARB(proc, hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues) \
  ( ((GdkGLProc_wglGetPixelFormatAttribivARB) (proc)) (hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues) )

/* wglGetPixelFormatAttribfvARB */
typedef BOOL (WINAPI * GdkGLProc_wglGetPixelFormatAttribfvARB) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
GdkGLProc    gdk_gl_get_wglGetPixelFormatAttribfvARB (void);
#define      gdk_gl_wglGetPixelFormatAttribfvARB(proc, hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues) \
  ( ((GdkGLProc_wglGetPixelFormatAttribfvARB) (proc)) (hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues) )

/* wglChoosePixelFormatARB */
typedef BOOL (WINAPI * GdkGLProc_wglChoosePixelFormatARB) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
GdkGLProc    gdk_gl_get_wglChoosePixelFormatARB (void);
#define      gdk_gl_wglChoosePixelFormatARB(proc, hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats) \
  ( ((GdkGLProc_wglChoosePixelFormatARB) (proc)) (hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats) )

/* proc struct */

typedef struct _GdkGL_WGL_ARB_pixel_format GdkGL_WGL_ARB_pixel_format;

struct _GdkGL_WGL_ARB_pixel_format
{
  GdkGLProc_wglGetPixelFormatAttribivARB wglGetPixelFormatAttribivARB;
  GdkGLProc_wglGetPixelFormatAttribfvARB wglGetPixelFormatAttribfvARB;
  GdkGLProc_wglChoosePixelFormatARB wglChoosePixelFormatARB;
};

GdkGL_WGL_ARB_pixel_format *gdk_gl_get_WGL_ARB_pixel_format (GdkGLConfig *glconfig);

/*
 * WGL_ARB_make_current_read
 */

/* wglMakeContextCurrentARB */
typedef BOOL (WINAPI * GdkGLProc_wglMakeContextCurrentARB) (HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
GdkGLProc    gdk_gl_get_wglMakeContextCurrentARB (void);
#define      gdk_gl_wglMakeContextCurrentARB(proc, hDrawDC, hReadDC, hglrc) \
  ( ((GdkGLProc_wglMakeContextCurrentARB) (proc)) (hDrawDC, hReadDC, hglrc) )

/* wglGetCurrentReadDCARB */
typedef HDC (WINAPI * GdkGLProc_wglGetCurrentReadDCARB) (void);
GdkGLProc    gdk_gl_get_wglGetCurrentReadDCARB (void);
#define      gdk_gl_wglGetCurrentReadDCARB(proc) \
  ( ((GdkGLProc_wglGetCurrentReadDCARB) (proc)) () )

/* proc struct */

typedef struct _GdkGL_WGL_ARB_make_current_read GdkGL_WGL_ARB_make_current_read;

struct _GdkGL_WGL_ARB_make_current_read
{
  GdkGLProc_wglMakeContextCurrentARB wglMakeContextCurrentARB;
  GdkGLProc_wglGetCurrentReadDCARB wglGetCurrentReadDCARB;
};

GdkGL_WGL_ARB_make_current_read *gdk_gl_get_WGL_ARB_make_current_read (GdkGLConfig *glconfig);

/*
 * WGL_ARB_pbuffer
 */

/* wglCreatePbufferARB */
typedef HPBUFFERARB (WINAPI * GdkGLProc_wglCreatePbufferARB) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
GdkGLProc    gdk_gl_get_wglCreatePbufferARB (void);
#define      gdk_gl_wglCreatePbufferARB(proc, hDC, iPixelFormat, iWidth, iHeight, piAttribList) \
  ( ((GdkGLProc_wglCreatePbufferARB) (proc)) (hDC, iPixelFormat, iWidth, iHeight, piAttribList) )

/* wglGetPbufferDCARB */
typedef HDC (WINAPI * GdkGLProc_wglGetPbufferDCARB) (HPBUFFERARB hPbuffer);
GdkGLProc    gdk_gl_get_wglGetPbufferDCARB (void);
#define      gdk_gl_wglGetPbufferDCARB(proc, hPbuffer) \
  ( ((GdkGLProc_wglGetPbufferDCARB) (proc)) (hPbuffer) )

/* wglReleasePbufferDCARB */
typedef int (WINAPI * GdkGLProc_wglReleasePbufferDCARB) (HPBUFFERARB hPbuffer, HDC hDC);
GdkGLProc    gdk_gl_get_wglReleasePbufferDCARB (void);
#define      gdk_gl_wglReleasePbufferDCARB(proc, hPbuffer, hDC) \
  ( ((GdkGLProc_wglReleasePbufferDCARB) (proc)) (hPbuffer, hDC) )

/* wglDestroyPbufferARB */
typedef BOOL (WINAPI * GdkGLProc_wglDestroyPbufferARB) (HPBUFFERARB hPbuffer);
GdkGLProc    gdk_gl_get_wglDestroyPbufferARB (void);
#define      gdk_gl_wglDestroyPbufferARB(proc, hPbuffer) \
  ( ((GdkGLProc_wglDestroyPbufferARB) (proc)) (hPbuffer) )

/* wglQueryPbufferARB */
typedef BOOL (WINAPI * GdkGLProc_wglQueryPbufferARB) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
GdkGLProc    gdk_gl_get_wglQueryPbufferARB (void);
#define      gdk_gl_wglQueryPbufferARB(proc, hPbuffer, iAttribute, piValue) \
  ( ((GdkGLProc_wglQueryPbufferARB) (proc)) (hPbuffer, iAttribute, piValue) )

/* proc struct */

typedef struct _GdkGL_WGL_ARB_pbuffer GdkGL_WGL_ARB_pbuffer;

struct _GdkGL_WGL_ARB_pbuffer
{
  GdkGLProc_wglCreatePbufferARB wglCreatePbufferARB;
  GdkGLProc_wglGetPbufferDCARB wglGetPbufferDCARB;
  GdkGLProc_wglReleasePbufferDCARB wglReleasePbufferDCARB;
  GdkGLProc_wglDestroyPbufferARB wglDestroyPbufferARB;
  GdkGLProc_wglQueryPbufferARB wglQueryPbufferARB;
};

GdkGL_WGL_ARB_pbuffer *gdk_gl_get_WGL_ARB_pbuffer (GdkGLConfig *glconfig);

/*
 * WGL_ARB_render_texture
 */

/* wglBindTexImageARB */
typedef BOOL (WINAPI * GdkGLProc_wglBindTexImageARB) (HPBUFFERARB hPbuffer, int iBuffer);
GdkGLProc    gdk_gl_get_wglBindTexImageARB (void);
#define      gdk_gl_wglBindTexImageARB(proc, hPbuffer, iBuffer) \
  ( ((GdkGLProc_wglBindTexImageARB) (proc)) (hPbuffer, iBuffer) )

/* wglReleaseTexImageARB */
typedef BOOL (WINAPI * GdkGLProc_wglReleaseTexImageARB) (HPBUFFERARB hPbuffer, int iBuffer);
GdkGLProc    gdk_gl_get_wglReleaseTexImageARB (void);
#define      gdk_gl_wglReleaseTexImageARB(proc, hPbuffer, iBuffer) \
  ( ((GdkGLProc_wglReleaseTexImageARB) (proc)) (hPbuffer, iBuffer) )

/* wglSetPbufferAttribARB */
typedef BOOL (WINAPI * GdkGLProc_wglSetPbufferAttribARB) (HPBUFFERARB hPbuffer, const int *piAttribList);
GdkGLProc    gdk_gl_get_wglSetPbufferAttribARB (void);
#define      gdk_gl_wglSetPbufferAttribARB(proc, hPbuffer, piAttribList) \
  ( ((GdkGLProc_wglSetPbufferAttribARB) (proc)) (hPbuffer, piAttribList) )

/* proc struct */

typedef struct _GdkGL_WGL_ARB_render_texture GdkGL_WGL_ARB_render_texture;

struct _GdkGL_WGL_ARB_render_texture
{
  GdkGLProc_wglBindTexImageARB wglBindTexImageARB;
  GdkGLProc_wglReleaseTexImageARB wglReleaseTexImageARB;
  GdkGLProc_wglSetPbufferAttribARB wglSetPbufferAttribARB;
};

GdkGL_WGL_ARB_render_texture *gdk_gl_get_WGL_ARB_render_texture (GdkGLConfig *glconfig);

/*
 * WGL_EXT_display_color_table
 */

/* wglCreateDisplayColorTableEXT */
typedef GLboolean (WINAPI * GdkGLProc_wglCreateDisplayColorTableEXT) (GLushort id);
GdkGLProc    gdk_gl_get_wglCreateDisplayColorTableEXT (void);
#define      gdk_gl_wglCreateDisplayColorTableEXT(proc, id) \
  ( ((GdkGLProc_wglCreateDisplayColorTableEXT) (proc)) (id) )

/* wglLoadDisplayColorTableEXT */
typedef GLboolean (WINAPI * GdkGLProc_wglLoadDisplayColorTableEXT) (const GLushort *table, GLuint length);
GdkGLProc    gdk_gl_get_wglLoadDisplayColorTableEXT (void);
#define      gdk_gl_wglLoadDisplayColorTableEXT(proc, table, length) \
  ( ((GdkGLProc_wglLoadDisplayColorTableEXT) (proc)) (table, length) )

/* wglBindDisplayColorTableEXT */
typedef GLboolean (WINAPI * GdkGLProc_wglBindDisplayColorTableEXT) (GLushort id);
GdkGLProc    gdk_gl_get_wglBindDisplayColorTableEXT (void);
#define      gdk_gl_wglBindDisplayColorTableEXT(proc, id) \
  ( ((GdkGLProc_wglBindDisplayColorTableEXT) (proc)) (id) )

/* wglDestroyDisplayColorTableEXT */
typedef VOID (WINAPI * GdkGLProc_wglDestroyDisplayColorTableEXT) (GLushort id);
GdkGLProc    gdk_gl_get_wglDestroyDisplayColorTableEXT (void);
#define      gdk_gl_wglDestroyDisplayColorTableEXT(proc, id) \
  ( ((GdkGLProc_wglDestroyDisplayColorTableEXT) (proc)) (id) )

/* proc struct */

typedef struct _GdkGL_WGL_EXT_display_color_table GdkGL_WGL_EXT_display_color_table;

struct _GdkGL_WGL_EXT_display_color_table
{
  GdkGLProc_wglCreateDisplayColorTableEXT wglCreateDisplayColorTableEXT;
  GdkGLProc_wglLoadDisplayColorTableEXT wglLoadDisplayColorTableEXT;
  GdkGLProc_wglBindDisplayColorTableEXT wglBindDisplayColorTableEXT;
  GdkGLProc_wglDestroyDisplayColorTableEXT wglDestroyDisplayColorTableEXT;
};

GdkGL_WGL_EXT_display_color_table *gdk_gl_get_WGL_EXT_display_color_table (GdkGLConfig *glconfig);

/*
 * WGL_EXT_extensions_string
 */

/* wglGetExtensionsStringEXT */
typedef const char * (WINAPI * GdkGLProc_wglGetExtensionsStringEXT) (void);
GdkGLProc    gdk_gl_get_wglGetExtensionsStringEXT (void);
#define      gdk_gl_wglGetExtensionsStringEXT(proc) \
  ( ((GdkGLProc_wglGetExtensionsStringEXT) (proc)) () )

/* proc struct */

typedef struct _GdkGL_WGL_EXT_extensions_string GdkGL_WGL_EXT_extensions_string;

struct _GdkGL_WGL_EXT_extensions_string
{
  GdkGLProc_wglGetExtensionsStringEXT wglGetExtensionsStringEXT;
};

GdkGL_WGL_EXT_extensions_string *gdk_gl_get_WGL_EXT_extensions_string (GdkGLConfig *glconfig);

/*
 * WGL_EXT_make_current_read
 */

/* wglMakeContextCurrentEXT */
typedef BOOL (WINAPI * GdkGLProc_wglMakeContextCurrentEXT) (HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
GdkGLProc    gdk_gl_get_wglMakeContextCurrentEXT (void);
#define      gdk_gl_wglMakeContextCurrentEXT(proc, hDrawDC, hReadDC, hglrc) \
  ( ((GdkGLProc_wglMakeContextCurrentEXT) (proc)) (hDrawDC, hReadDC, hglrc) )

/* wglGetCurrentReadDCEXT */
typedef HDC (WINAPI * GdkGLProc_wglGetCurrentReadDCEXT) (void);
GdkGLProc    gdk_gl_get_wglGetCurrentReadDCEXT (void);
#define      gdk_gl_wglGetCurrentReadDCEXT(proc) \
  ( ((GdkGLProc_wglGetCurrentReadDCEXT) (proc)) () )

/* proc struct */

typedef struct _GdkGL_WGL_EXT_make_current_read GdkGL_WGL_EXT_make_current_read;

struct _GdkGL_WGL_EXT_make_current_read
{
  GdkGLProc_wglMakeContextCurrentEXT wglMakeContextCurrentEXT;
  GdkGLProc_wglGetCurrentReadDCEXT wglGetCurrentReadDCEXT;
};

GdkGL_WGL_EXT_make_current_read *gdk_gl_get_WGL_EXT_make_current_read (GdkGLConfig *glconfig);

/*
 * WGL_EXT_pbuffer
 */

/* wglCreatePbufferEXT */
typedef HPBUFFEREXT (WINAPI * GdkGLProc_wglCreatePbufferEXT) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
GdkGLProc    gdk_gl_get_wglCreatePbufferEXT (void);
#define      gdk_gl_wglCreatePbufferEXT(proc, hDC, iPixelFormat, iWidth, iHeight, piAttribList) \
  ( ((GdkGLProc_wglCreatePbufferEXT) (proc)) (hDC, iPixelFormat, iWidth, iHeight, piAttribList) )

/* wglGetPbufferDCEXT */
typedef HDC (WINAPI * GdkGLProc_wglGetPbufferDCEXT) (HPBUFFEREXT hPbuffer);
GdkGLProc    gdk_gl_get_wglGetPbufferDCEXT (void);
#define      gdk_gl_wglGetPbufferDCEXT(proc, hPbuffer) \
  ( ((GdkGLProc_wglGetPbufferDCEXT) (proc)) (hPbuffer) )

/* wglReleasePbufferDCEXT */
typedef int (WINAPI * GdkGLProc_wglReleasePbufferDCEXT) (HPBUFFEREXT hPbuffer, HDC hDC);
GdkGLProc    gdk_gl_get_wglReleasePbufferDCEXT (void);
#define      gdk_gl_wglReleasePbufferDCEXT(proc, hPbuffer, hDC) \
  ( ((GdkGLProc_wglReleasePbufferDCEXT) (proc)) (hPbuffer, hDC) )

/* wglDestroyPbufferEXT */
typedef BOOL (WINAPI * GdkGLProc_wglDestroyPbufferEXT) (HPBUFFEREXT hPbuffer);
GdkGLProc    gdk_gl_get_wglDestroyPbufferEXT (void);
#define      gdk_gl_wglDestroyPbufferEXT(proc, hPbuffer) \
  ( ((GdkGLProc_wglDestroyPbufferEXT) (proc)) (hPbuffer) )

/* wglQueryPbufferEXT */
typedef BOOL (WINAPI * GdkGLProc_wglQueryPbufferEXT) (HPBUFFEREXT hPbuffer, int iAttribute, int *piValue);
GdkGLProc    gdk_gl_get_wglQueryPbufferEXT (void);
#define      gdk_gl_wglQueryPbufferEXT(proc, hPbuffer, iAttribute, piValue) \
  ( ((GdkGLProc_wglQueryPbufferEXT) (proc)) (hPbuffer, iAttribute, piValue) )

/* proc struct */

typedef struct _GdkGL_WGL_EXT_pbuffer GdkGL_WGL_EXT_pbuffer;

struct _GdkGL_WGL_EXT_pbuffer
{
  GdkGLProc_wglCreatePbufferEXT wglCreatePbufferEXT;
  GdkGLProc_wglGetPbufferDCEXT wglGetPbufferDCEXT;
  GdkGLProc_wglReleasePbufferDCEXT wglReleasePbufferDCEXT;
  GdkGLProc_wglDestroyPbufferEXT wglDestroyPbufferEXT;
  GdkGLProc_wglQueryPbufferEXT wglQueryPbufferEXT;
};

GdkGL_WGL_EXT_pbuffer *gdk_gl_get_WGL_EXT_pbuffer (GdkGLConfig *glconfig);

/*
 * WGL_EXT_pixel_format
 */

/* wglGetPixelFormatAttribivEXT */
typedef BOOL (WINAPI * GdkGLProc_wglGetPixelFormatAttribivEXT) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
GdkGLProc    gdk_gl_get_wglGetPixelFormatAttribivEXT (void);
#define      gdk_gl_wglGetPixelFormatAttribivEXT(proc, hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues) \
  ( ((GdkGLProc_wglGetPixelFormatAttribivEXT) (proc)) (hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues) )

/* wglGetPixelFormatAttribfvEXT */
typedef BOOL (WINAPI * GdkGLProc_wglGetPixelFormatAttribfvEXT) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
GdkGLProc    gdk_gl_get_wglGetPixelFormatAttribfvEXT (void);
#define      gdk_gl_wglGetPixelFormatAttribfvEXT(proc, hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues) \
  ( ((GdkGLProc_wglGetPixelFormatAttribfvEXT) (proc)) (hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues) )

/* wglChoosePixelFormatEXT */
typedef BOOL (WINAPI * GdkGLProc_wglChoosePixelFormatEXT) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
GdkGLProc    gdk_gl_get_wglChoosePixelFormatEXT (void);
#define      gdk_gl_wglChoosePixelFormatEXT(proc, hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats) \
  ( ((GdkGLProc_wglChoosePixelFormatEXT) (proc)) (hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats) )

/* proc struct */

typedef struct _GdkGL_WGL_EXT_pixel_format GdkGL_WGL_EXT_pixel_format;

struct _GdkGL_WGL_EXT_pixel_format
{
  GdkGLProc_wglGetPixelFormatAttribivEXT wglGetPixelFormatAttribivEXT;
  GdkGLProc_wglGetPixelFormatAttribfvEXT wglGetPixelFormatAttribfvEXT;
  GdkGLProc_wglChoosePixelFormatEXT wglChoosePixelFormatEXT;
};

GdkGL_WGL_EXT_pixel_format *gdk_gl_get_WGL_EXT_pixel_format (GdkGLConfig *glconfig);

/*
 * WGL_EXT_swap_control
 */

/* wglSwapIntervalEXT */
typedef BOOL (WINAPI * GdkGLProc_wglSwapIntervalEXT) (int interval);
GdkGLProc    gdk_gl_get_wglSwapIntervalEXT (void);
#define      gdk_gl_wglSwapIntervalEXT(proc, interval) \
  ( ((GdkGLProc_wglSwapIntervalEXT) (proc)) (interval) )

/* wglGetSwapIntervalEXT */
typedef int (WINAPI * GdkGLProc_wglGetSwapIntervalEXT) (void);
GdkGLProc    gdk_gl_get_wglGetSwapIntervalEXT (void);
#define      gdk_gl_wglGetSwapIntervalEXT(proc) \
  ( ((GdkGLProc_wglGetSwapIntervalEXT) (proc)) () )

/* proc struct */

typedef struct _GdkGL_WGL_EXT_swap_control GdkGL_WGL_EXT_swap_control;

struct _GdkGL_WGL_EXT_swap_control
{
  GdkGLProc_wglSwapIntervalEXT wglSwapIntervalEXT;
  GdkGLProc_wglGetSwapIntervalEXT wglGetSwapIntervalEXT;
};

GdkGL_WGL_EXT_swap_control *gdk_gl_get_WGL_EXT_swap_control (GdkGLConfig *glconfig);

/*
 * WGL_NV_vertex_array_range
 */

/* wglAllocateMemoryNV */
typedef void* (WINAPI * GdkGLProc_wglAllocateMemoryNV) (GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
GdkGLProc    gdk_gl_get_wglAllocateMemoryNV (void);
#define      gdk_gl_wglAllocateMemoryNV(proc, size, readfreq, writefreq, priority) \
  ( ((GdkGLProc_wglAllocateMemoryNV) (proc)) (size, readfreq, writefreq, priority) )

/* wglFreeMemoryNV */
typedef void (WINAPI * GdkGLProc_wglFreeMemoryNV) (void *pointer);
GdkGLProc    gdk_gl_get_wglFreeMemoryNV (void);
#define      gdk_gl_wglFreeMemoryNV(proc, pointer) \
  ( ((GdkGLProc_wglFreeMemoryNV) (proc)) (pointer) )

/* proc struct */

typedef struct _GdkGL_WGL_NV_vertex_array_range GdkGL_WGL_NV_vertex_array_range;

struct _GdkGL_WGL_NV_vertex_array_range
{
  GdkGLProc_wglAllocateMemoryNV wglAllocateMemoryNV;
  GdkGLProc_wglFreeMemoryNV wglFreeMemoryNV;
};

GdkGL_WGL_NV_vertex_array_range *gdk_gl_get_WGL_NV_vertex_array_range (GdkGLConfig *glconfig);

/*
 * WGL_OML_sync_control
 */

/* wglGetSyncValuesOML */
typedef BOOL (WINAPI * GdkGLProc_wglGetSyncValuesOML) (HDC hdc, INT64 *ust, INT64 *msc, INT64 *sbc);
GdkGLProc    gdk_gl_get_wglGetSyncValuesOML (void);
#define      gdk_gl_wglGetSyncValuesOML(proc, hdc, ust, msc, sbc) \
  ( ((GdkGLProc_wglGetSyncValuesOML) (proc)) (hdc, ust, msc, sbc) )

/* wglGetMscRateOML */
typedef BOOL (WINAPI * GdkGLProc_wglGetMscRateOML) (HDC hdc, INT32 *numerator, INT32 *denominator);
GdkGLProc    gdk_gl_get_wglGetMscRateOML (void);
#define      gdk_gl_wglGetMscRateOML(proc, hdc, numerator, denominator) \
  ( ((GdkGLProc_wglGetMscRateOML) (proc)) (hdc, numerator, denominator) )

/* wglSwapBuffersMscOML */
typedef INT64 (WINAPI * GdkGLProc_wglSwapBuffersMscOML) (HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
GdkGLProc    gdk_gl_get_wglSwapBuffersMscOML (void);
#define      gdk_gl_wglSwapBuffersMscOML(proc, hdc, target_msc, divisor, remainder) \
  ( ((GdkGLProc_wglSwapBuffersMscOML) (proc)) (hdc, target_msc, divisor, remainder) )

/* wglSwapLayerBuffersMscOML */
typedef INT64 (WINAPI * GdkGLProc_wglSwapLayerBuffersMscOML) (HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
GdkGLProc    gdk_gl_get_wglSwapLayerBuffersMscOML (void);
#define      gdk_gl_wglSwapLayerBuffersMscOML(proc, hdc, fuPlanes, target_msc, divisor, remainder) \
  ( ((GdkGLProc_wglSwapLayerBuffersMscOML) (proc)) (hdc, fuPlanes, target_msc, divisor, remainder) )

/* wglWaitForMscOML */
typedef BOOL (WINAPI * GdkGLProc_wglWaitForMscOML) (HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64 *ust, INT64 *msc, INT64 *sbc);
GdkGLProc    gdk_gl_get_wglWaitForMscOML (void);
#define      gdk_gl_wglWaitForMscOML(proc, hdc, target_msc, divisor, remainder, ust, msc, sbc) \
  ( ((GdkGLProc_wglWaitForMscOML) (proc)) (hdc, target_msc, divisor, remainder, ust, msc, sbc) )

/* wglWaitForSbcOML */
typedef BOOL (WINAPI * GdkGLProc_wglWaitForSbcOML) (HDC hdc, INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc);
GdkGLProc    gdk_gl_get_wglWaitForSbcOML (void);
#define      gdk_gl_wglWaitForSbcOML(proc, hdc, target_sbc, ust, msc, sbc) \
  ( ((GdkGLProc_wglWaitForSbcOML) (proc)) (hdc, target_sbc, ust, msc, sbc) )

/* proc struct */

typedef struct _GdkGL_WGL_OML_sync_control GdkGL_WGL_OML_sync_control;

struct _GdkGL_WGL_OML_sync_control
{
  GdkGLProc_wglGetSyncValuesOML wglGetSyncValuesOML;
  GdkGLProc_wglGetMscRateOML wglGetMscRateOML;
  GdkGLProc_wglSwapBuffersMscOML wglSwapBuffersMscOML;
  GdkGLProc_wglSwapLayerBuffersMscOML wglSwapLayerBuffersMscOML;
  GdkGLProc_wglWaitForMscOML wglWaitForMscOML;
  GdkGLProc_wglWaitForSbcOML wglWaitForSbcOML;
};

GdkGL_WGL_OML_sync_control *gdk_gl_get_WGL_OML_sync_control (GdkGLConfig *glconfig);

/*
 * WGL_I3D_digital_video_control
 */

/* wglGetDigitalVideoParametersI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetDigitalVideoParametersI3D) (HDC hDC, int iAttribute, int *piValue);
GdkGLProc    gdk_gl_get_wglGetDigitalVideoParametersI3D (void);
#define      gdk_gl_wglGetDigitalVideoParametersI3D(proc, hDC, iAttribute, piValue) \
  ( ((GdkGLProc_wglGetDigitalVideoParametersI3D) (proc)) (hDC, iAttribute, piValue) )

/* wglSetDigitalVideoParametersI3D */
typedef BOOL (WINAPI * GdkGLProc_wglSetDigitalVideoParametersI3D) (HDC hDC, int iAttribute, const int *piValue);
GdkGLProc    gdk_gl_get_wglSetDigitalVideoParametersI3D (void);
#define      gdk_gl_wglSetDigitalVideoParametersI3D(proc, hDC, iAttribute, piValue) \
  ( ((GdkGLProc_wglSetDigitalVideoParametersI3D) (proc)) (hDC, iAttribute, piValue) )

/* proc struct */

typedef struct _GdkGL_WGL_I3D_digital_video_control GdkGL_WGL_I3D_digital_video_control;

struct _GdkGL_WGL_I3D_digital_video_control
{
  GdkGLProc_wglGetDigitalVideoParametersI3D wglGetDigitalVideoParametersI3D;
  GdkGLProc_wglSetDigitalVideoParametersI3D wglSetDigitalVideoParametersI3D;
};

GdkGL_WGL_I3D_digital_video_control *gdk_gl_get_WGL_I3D_digital_video_control (GdkGLConfig *glconfig);

/*
 * WGL_I3D_gamma
 */

/* wglGetGammaTableParametersI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetGammaTableParametersI3D) (HDC hDC, int iAttribute, int *piValue);
GdkGLProc    gdk_gl_get_wglGetGammaTableParametersI3D (void);
#define      gdk_gl_wglGetGammaTableParametersI3D(proc, hDC, iAttribute, piValue) \
  ( ((GdkGLProc_wglGetGammaTableParametersI3D) (proc)) (hDC, iAttribute, piValue) )

/* wglSetGammaTableParametersI3D */
typedef BOOL (WINAPI * GdkGLProc_wglSetGammaTableParametersI3D) (HDC hDC, int iAttribute, const int *piValue);
GdkGLProc    gdk_gl_get_wglSetGammaTableParametersI3D (void);
#define      gdk_gl_wglSetGammaTableParametersI3D(proc, hDC, iAttribute, piValue) \
  ( ((GdkGLProc_wglSetGammaTableParametersI3D) (proc)) (hDC, iAttribute, piValue) )

/* wglGetGammaTableI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetGammaTableI3D) (HDC hDC, int iEntries, USHORT *puRed, USHORT *puGreen, USHORT *puBlue);
GdkGLProc    gdk_gl_get_wglGetGammaTableI3D (void);
#define      gdk_gl_wglGetGammaTableI3D(proc, hDC, iEntries, puRed, puGreen, puBlue) \
  ( ((GdkGLProc_wglGetGammaTableI3D) (proc)) (hDC, iEntries, puRed, puGreen, puBlue) )

/* wglSetGammaTableI3D */
typedef BOOL (WINAPI * GdkGLProc_wglSetGammaTableI3D) (HDC hDC, int iEntries, const USHORT *puRed, const USHORT *puGreen, const USHORT *puBlue);
GdkGLProc    gdk_gl_get_wglSetGammaTableI3D (void);
#define      gdk_gl_wglSetGammaTableI3D(proc, hDC, iEntries, puRed, puGreen, puBlue) \
  ( ((GdkGLProc_wglSetGammaTableI3D) (proc)) (hDC, iEntries, puRed, puGreen, puBlue) )

/* proc struct */

typedef struct _GdkGL_WGL_I3D_gamma GdkGL_WGL_I3D_gamma;

struct _GdkGL_WGL_I3D_gamma
{
  GdkGLProc_wglGetGammaTableParametersI3D wglGetGammaTableParametersI3D;
  GdkGLProc_wglSetGammaTableParametersI3D wglSetGammaTableParametersI3D;
  GdkGLProc_wglGetGammaTableI3D wglGetGammaTableI3D;
  GdkGLProc_wglSetGammaTableI3D wglSetGammaTableI3D;
};

GdkGL_WGL_I3D_gamma *gdk_gl_get_WGL_I3D_gamma (GdkGLConfig *glconfig);

/*
 * WGL_I3D_genlock
 */

/* wglEnableGenlockI3D */
typedef BOOL (WINAPI * GdkGLProc_wglEnableGenlockI3D) (HDC hDC);
GdkGLProc    gdk_gl_get_wglEnableGenlockI3D (void);
#define      gdk_gl_wglEnableGenlockI3D(proc, hDC) \
  ( ((GdkGLProc_wglEnableGenlockI3D) (proc)) (hDC) )

/* wglDisableGenlockI3D */
typedef BOOL (WINAPI * GdkGLProc_wglDisableGenlockI3D) (HDC hDC);
GdkGLProc    gdk_gl_get_wglDisableGenlockI3D (void);
#define      gdk_gl_wglDisableGenlockI3D(proc, hDC) \
  ( ((GdkGLProc_wglDisableGenlockI3D) (proc)) (hDC) )

/* wglIsEnabledGenlockI3D */
typedef BOOL (WINAPI * GdkGLProc_wglIsEnabledGenlockI3D) (HDC hDC, BOOL *pFlag);
GdkGLProc    gdk_gl_get_wglIsEnabledGenlockI3D (void);
#define      gdk_gl_wglIsEnabledGenlockI3D(proc, hDC, pFlag) \
  ( ((GdkGLProc_wglIsEnabledGenlockI3D) (proc)) (hDC, pFlag) )

/* wglGenlockSourceI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGenlockSourceI3D) (HDC hDC, UINT uSource);
GdkGLProc    gdk_gl_get_wglGenlockSourceI3D (void);
#define      gdk_gl_wglGenlockSourceI3D(proc, hDC, uSource) \
  ( ((GdkGLProc_wglGenlockSourceI3D) (proc)) (hDC, uSource) )

/* wglGetGenlockSourceI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetGenlockSourceI3D) (HDC hDC, UINT *uSource);
GdkGLProc    gdk_gl_get_wglGetGenlockSourceI3D (void);
#define      gdk_gl_wglGetGenlockSourceI3D(proc, hDC, uSource) \
  ( ((GdkGLProc_wglGetGenlockSourceI3D) (proc)) (hDC, uSource) )

/* wglGenlockSourceEdgeI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGenlockSourceEdgeI3D) (HDC hDC, UINT uEdge);
GdkGLProc    gdk_gl_get_wglGenlockSourceEdgeI3D (void);
#define      gdk_gl_wglGenlockSourceEdgeI3D(proc, hDC, uEdge) \
  ( ((GdkGLProc_wglGenlockSourceEdgeI3D) (proc)) (hDC, uEdge) )

/* wglGetGenlockSourceEdgeI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetGenlockSourceEdgeI3D) (HDC hDC, UINT *uEdge);
GdkGLProc    gdk_gl_get_wglGetGenlockSourceEdgeI3D (void);
#define      gdk_gl_wglGetGenlockSourceEdgeI3D(proc, hDC, uEdge) \
  ( ((GdkGLProc_wglGetGenlockSourceEdgeI3D) (proc)) (hDC, uEdge) )

/* wglGenlockSampleRateI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGenlockSampleRateI3D) (HDC hDC, UINT uRate);
GdkGLProc    gdk_gl_get_wglGenlockSampleRateI3D (void);
#define      gdk_gl_wglGenlockSampleRateI3D(proc, hDC, uRate) \
  ( ((GdkGLProc_wglGenlockSampleRateI3D) (proc)) (hDC, uRate) )

/* wglGetGenlockSampleRateI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetGenlockSampleRateI3D) (HDC hDC, UINT *uRate);
GdkGLProc    gdk_gl_get_wglGetGenlockSampleRateI3D (void);
#define      gdk_gl_wglGetGenlockSampleRateI3D(proc, hDC, uRate) \
  ( ((GdkGLProc_wglGetGenlockSampleRateI3D) (proc)) (hDC, uRate) )

/* wglGenlockSourceDelayI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGenlockSourceDelayI3D) (HDC hDC, UINT uDelay);
GdkGLProc    gdk_gl_get_wglGenlockSourceDelayI3D (void);
#define      gdk_gl_wglGenlockSourceDelayI3D(proc, hDC, uDelay) \
  ( ((GdkGLProc_wglGenlockSourceDelayI3D) (proc)) (hDC, uDelay) )

/* wglGetGenlockSourceDelayI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetGenlockSourceDelayI3D) (HDC hDC, UINT *uDelay);
GdkGLProc    gdk_gl_get_wglGetGenlockSourceDelayI3D (void);
#define      gdk_gl_wglGetGenlockSourceDelayI3D(proc, hDC, uDelay) \
  ( ((GdkGLProc_wglGetGenlockSourceDelayI3D) (proc)) (hDC, uDelay) )

/* wglQueryGenlockMaxSourceDelayI3D */
typedef BOOL (WINAPI * GdkGLProc_wglQueryGenlockMaxSourceDelayI3D) (HDC hDC, UINT *uMaxLineDelay, UINT *uMaxPixelDelay);
GdkGLProc    gdk_gl_get_wglQueryGenlockMaxSourceDelayI3D (void);
#define      gdk_gl_wglQueryGenlockMaxSourceDelayI3D(proc, hDC, uMaxLineDelay, uMaxPixelDelay) \
  ( ((GdkGLProc_wglQueryGenlockMaxSourceDelayI3D) (proc)) (hDC, uMaxLineDelay, uMaxPixelDelay) )

/* proc struct */

typedef struct _GdkGL_WGL_I3D_genlock GdkGL_WGL_I3D_genlock;

struct _GdkGL_WGL_I3D_genlock
{
  GdkGLProc_wglEnableGenlockI3D wglEnableGenlockI3D;
  GdkGLProc_wglDisableGenlockI3D wglDisableGenlockI3D;
  GdkGLProc_wglIsEnabledGenlockI3D wglIsEnabledGenlockI3D;
  GdkGLProc_wglGenlockSourceI3D wglGenlockSourceI3D;
  GdkGLProc_wglGetGenlockSourceI3D wglGetGenlockSourceI3D;
  GdkGLProc_wglGenlockSourceEdgeI3D wglGenlockSourceEdgeI3D;
  GdkGLProc_wglGetGenlockSourceEdgeI3D wglGetGenlockSourceEdgeI3D;
  GdkGLProc_wglGenlockSampleRateI3D wglGenlockSampleRateI3D;
  GdkGLProc_wglGetGenlockSampleRateI3D wglGetGenlockSampleRateI3D;
  GdkGLProc_wglGenlockSourceDelayI3D wglGenlockSourceDelayI3D;
  GdkGLProc_wglGetGenlockSourceDelayI3D wglGetGenlockSourceDelayI3D;
  GdkGLProc_wglQueryGenlockMaxSourceDelayI3D wglQueryGenlockMaxSourceDelayI3D;
};

GdkGL_WGL_I3D_genlock *gdk_gl_get_WGL_I3D_genlock (GdkGLConfig *glconfig);

/*
 * WGL_I3D_image_buffer
 */

/* wglCreateImageBufferI3D */
typedef LPVOID (WINAPI * GdkGLProc_wglCreateImageBufferI3D) (HDC hDC, DWORD dwSize, UINT uFlags);
GdkGLProc    gdk_gl_get_wglCreateImageBufferI3D (void);
#define      gdk_gl_wglCreateImageBufferI3D(proc, hDC, dwSize, uFlags) \
  ( ((GdkGLProc_wglCreateImageBufferI3D) (proc)) (hDC, dwSize, uFlags) )

/* wglDestroyImageBufferI3D */
typedef BOOL (WINAPI * GdkGLProc_wglDestroyImageBufferI3D) (HDC hDC, LPVOID pAddress);
GdkGLProc    gdk_gl_get_wglDestroyImageBufferI3D (void);
#define      gdk_gl_wglDestroyImageBufferI3D(proc, hDC, pAddress) \
  ( ((GdkGLProc_wglDestroyImageBufferI3D) (proc)) (hDC, pAddress) )

/* wglAssociateImageBufferEventsI3D */
typedef BOOL (WINAPI * GdkGLProc_wglAssociateImageBufferEventsI3D) (HDC hDC, const HANDLE *pEvent, const LPVOID *pAddress, const DWORD *pSize, UINT count);
GdkGLProc    gdk_gl_get_wglAssociateImageBufferEventsI3D (void);
#define      gdk_gl_wglAssociateImageBufferEventsI3D(proc, hDC, pEvent, pAddress, pSize, count) \
  ( ((GdkGLProc_wglAssociateImageBufferEventsI3D) (proc)) (hDC, pEvent, pAddress, pSize, count) )

/* wglReleaseImageBufferEventsI3D */
typedef BOOL (WINAPI * GdkGLProc_wglReleaseImageBufferEventsI3D) (HDC hDC, const LPVOID *pAddress, UINT count);
GdkGLProc    gdk_gl_get_wglReleaseImageBufferEventsI3D (void);
#define      gdk_gl_wglReleaseImageBufferEventsI3D(proc, hDC, pAddress, count) \
  ( ((GdkGLProc_wglReleaseImageBufferEventsI3D) (proc)) (hDC, pAddress, count) )

/* proc struct */

typedef struct _GdkGL_WGL_I3D_image_buffer GdkGL_WGL_I3D_image_buffer;

struct _GdkGL_WGL_I3D_image_buffer
{
  GdkGLProc_wglCreateImageBufferI3D wglCreateImageBufferI3D;
  GdkGLProc_wglDestroyImageBufferI3D wglDestroyImageBufferI3D;
  GdkGLProc_wglAssociateImageBufferEventsI3D wglAssociateImageBufferEventsI3D;
  GdkGLProc_wglReleaseImageBufferEventsI3D wglReleaseImageBufferEventsI3D;
};

GdkGL_WGL_I3D_image_buffer *gdk_gl_get_WGL_I3D_image_buffer (GdkGLConfig *glconfig);

/*
 * WGL_I3D_swap_frame_lock
 */

/* wglEnableFrameLockI3D */
typedef BOOL (WINAPI * GdkGLProc_wglEnableFrameLockI3D) (void);
GdkGLProc    gdk_gl_get_wglEnableFrameLockI3D (void);
#define      gdk_gl_wglEnableFrameLockI3D(proc) \
  ( ((GdkGLProc_wglEnableFrameLockI3D) (proc)) () )

/* wglDisableFrameLockI3D */
typedef BOOL (WINAPI * GdkGLProc_wglDisableFrameLockI3D) (void);
GdkGLProc    gdk_gl_get_wglDisableFrameLockI3D (void);
#define      gdk_gl_wglDisableFrameLockI3D(proc) \
  ( ((GdkGLProc_wglDisableFrameLockI3D) (proc)) () )

/* wglIsEnabledFrameLockI3D */
typedef BOOL (WINAPI * GdkGLProc_wglIsEnabledFrameLockI3D) (BOOL *pFlag);
GdkGLProc    gdk_gl_get_wglIsEnabledFrameLockI3D (void);
#define      gdk_gl_wglIsEnabledFrameLockI3D(proc, pFlag) \
  ( ((GdkGLProc_wglIsEnabledFrameLockI3D) (proc)) (pFlag) )

/* wglQueryFrameLockMasterI3D */
typedef BOOL (WINAPI * GdkGLProc_wglQueryFrameLockMasterI3D) (BOOL *pFlag);
GdkGLProc    gdk_gl_get_wglQueryFrameLockMasterI3D (void);
#define      gdk_gl_wglQueryFrameLockMasterI3D(proc, pFlag) \
  ( ((GdkGLProc_wglQueryFrameLockMasterI3D) (proc)) (pFlag) )

/* proc struct */

typedef struct _GdkGL_WGL_I3D_swap_frame_lock GdkGL_WGL_I3D_swap_frame_lock;

struct _GdkGL_WGL_I3D_swap_frame_lock
{
  GdkGLProc_wglEnableFrameLockI3D wglEnableFrameLockI3D;
  GdkGLProc_wglDisableFrameLockI3D wglDisableFrameLockI3D;
  GdkGLProc_wglIsEnabledFrameLockI3D wglIsEnabledFrameLockI3D;
  GdkGLProc_wglQueryFrameLockMasterI3D wglQueryFrameLockMasterI3D;
};

GdkGL_WGL_I3D_swap_frame_lock *gdk_gl_get_WGL_I3D_swap_frame_lock (GdkGLConfig *glconfig);

/*
 * WGL_I3D_swap_frame_usage
 */

/* wglGetFrameUsageI3D */
typedef BOOL (WINAPI * GdkGLProc_wglGetFrameUsageI3D) (float *pUsage);
GdkGLProc    gdk_gl_get_wglGetFrameUsageI3D (void);
#define      gdk_gl_wglGetFrameUsageI3D(proc, pUsage) \
  ( ((GdkGLProc_wglGetFrameUsageI3D) (proc)) (pUsage) )

/* wglBeginFrameTrackingI3D */
typedef BOOL (WINAPI * GdkGLProc_wglBeginFrameTrackingI3D) (void);
GdkGLProc    gdk_gl_get_wglBeginFrameTrackingI3D (void);
#define      gdk_gl_wglBeginFrameTrackingI3D(proc) \
  ( ((GdkGLProc_wglBeginFrameTrackingI3D) (proc)) () )

/* wglEndFrameTrackingI3D */
typedef BOOL (WINAPI * GdkGLProc_wglEndFrameTrackingI3D) (void);
GdkGLProc    gdk_gl_get_wglEndFrameTrackingI3D (void);
#define      gdk_gl_wglEndFrameTrackingI3D(proc) \
  ( ((GdkGLProc_wglEndFrameTrackingI3D) (proc)) () )

/* wglQueryFrameTrackingI3D */
typedef BOOL (WINAPI * GdkGLProc_wglQueryFrameTrackingI3D) (DWORD *pFrameCount, DWORD *pMissedFrames, float *pLastMissedUsage);
GdkGLProc    gdk_gl_get_wglQueryFrameTrackingI3D (void);
#define      gdk_gl_wglQueryFrameTrackingI3D(proc, pFrameCount, pMissedFrames, pLastMissedUsage) \
  ( ((GdkGLProc_wglQueryFrameTrackingI3D) (proc)) (pFrameCount, pMissedFrames, pLastMissedUsage) )

/* proc struct */

typedef struct _GdkGL_WGL_I3D_swap_frame_usage GdkGL_WGL_I3D_swap_frame_usage;

struct _GdkGL_WGL_I3D_swap_frame_usage
{
  GdkGLProc_wglGetFrameUsageI3D wglGetFrameUsageI3D;
  GdkGLProc_wglBeginFrameTrackingI3D wglBeginFrameTrackingI3D;
  GdkGLProc_wglEndFrameTrackingI3D wglEndFrameTrackingI3D;
  GdkGLProc_wglQueryFrameTrackingI3D wglQueryFrameTrackingI3D;
};

GdkGL_WGL_I3D_swap_frame_usage *gdk_gl_get_WGL_I3D_swap_frame_usage (GdkGLConfig *glconfig);

G_END_DECLS

#endif /* __GDK_GL_WGLEXT_H__ */
