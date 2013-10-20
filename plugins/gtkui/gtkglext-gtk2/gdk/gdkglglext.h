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
 * This is a generated file.  Please modify "gen-gdkglglext-h.pl".
 */

#ifndef __GDK_GL_GLEXT_H__
#define __GDK_GL_GLEXT_H__

#include <glib.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <GL/gl.h>

#include <gdk/gdkgldefs.h>
#include <gdk/gdkglquery.h>

G_BEGIN_DECLS

#ifndef HAVE_GLHALFNV
#if defined(GL_NV_half_float) && defined(GDKGLEXT_NEED_GLHALFNV_TYPEDEF)
typedef unsigned short GLhalfNV;
#endif
#endif

/* Avoid old glext.h bug. */
#if !defined(GL_SGIS_point_parameters) && defined(GL_POINT_SIZE_MIN_SGIS)
#define GL_SGIS_point_parameters 1
#endif

#undef __glext_h_
#undef GL_GLEXT_VERSION
#include <gdk/glext/glext.h>
#include <gdk/glext/glext-extra.h>

/*
 * GL_VERSION_1_2
 */

/* glBlendColor */
typedef void (APIENTRY * GdkGLProc_glBlendColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GdkGLProc    gdk_gl_get_glBlendColor (void);
#define      gdk_gl_glBlendColor(proc, red, green, blue, alpha) \
  ( ((GdkGLProc_glBlendColor) (proc)) (red, green, blue, alpha) )

/* glBlendEquation */
typedef void (APIENTRY * GdkGLProc_glBlendEquation) (GLenum mode);
GdkGLProc    gdk_gl_get_glBlendEquation (void);
#define      gdk_gl_glBlendEquation(proc, mode) \
  ( ((GdkGLProc_glBlendEquation) (proc)) (mode) )

/* glDrawRangeElements */
typedef void (APIENTRY * GdkGLProc_glDrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
GdkGLProc    gdk_gl_get_glDrawRangeElements (void);
#define      gdk_gl_glDrawRangeElements(proc, mode, start, end, count, type, indices) \
  ( ((GdkGLProc_glDrawRangeElements) (proc)) (mode, start, end, count, type, indices) )

/* glColorTable */
typedef void (APIENTRY * GdkGLProc_glColorTable) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
GdkGLProc    gdk_gl_get_glColorTable (void);
#define      gdk_gl_glColorTable(proc, target, internalformat, width, format, type, table) \
  ( ((GdkGLProc_glColorTable) (proc)) (target, internalformat, width, format, type, table) )

/* glColorTableParameterfv */
typedef void (APIENTRY * GdkGLProc_glColorTableParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glColorTableParameterfv (void);
#define      gdk_gl_glColorTableParameterfv(proc, target, pname, params) \
  ( ((GdkGLProc_glColorTableParameterfv) (proc)) (target, pname, params) )

/* glColorTableParameteriv */
typedef void (APIENTRY * GdkGLProc_glColorTableParameteriv) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glColorTableParameteriv (void);
#define      gdk_gl_glColorTableParameteriv(proc, target, pname, params) \
  ( ((GdkGLProc_glColorTableParameteriv) (proc)) (target, pname, params) )

/* glCopyColorTable */
typedef void (APIENTRY * GdkGLProc_glCopyColorTable) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyColorTable (void);
#define      gdk_gl_glCopyColorTable(proc, target, internalformat, x, y, width) \
  ( ((GdkGLProc_glCopyColorTable) (proc)) (target, internalformat, x, y, width) )

/* glGetColorTable */
typedef void (APIENTRY * GdkGLProc_glGetColorTable) (GLenum target, GLenum format, GLenum type, GLvoid *table);
GdkGLProc    gdk_gl_get_glGetColorTable (void);
#define      gdk_gl_glGetColorTable(proc, target, format, type, table) \
  ( ((GdkGLProc_glGetColorTable) (proc)) (target, format, type, table) )

/* glGetColorTableParameterfv */
typedef void (APIENTRY * GdkGLProc_glGetColorTableParameterfv) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetColorTableParameterfv (void);
#define      gdk_gl_glGetColorTableParameterfv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetColorTableParameterfv) (proc)) (target, pname, params) )

/* glGetColorTableParameteriv */
typedef void (APIENTRY * GdkGLProc_glGetColorTableParameteriv) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetColorTableParameteriv (void);
#define      gdk_gl_glGetColorTableParameteriv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetColorTableParameteriv) (proc)) (target, pname, params) )

/* glColorSubTable */
typedef void (APIENTRY * GdkGLProc_glColorSubTable) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
GdkGLProc    gdk_gl_get_glColorSubTable (void);
#define      gdk_gl_glColorSubTable(proc, target, start, count, format, type, data) \
  ( ((GdkGLProc_glColorSubTable) (proc)) (target, start, count, format, type, data) )

/* glCopyColorSubTable */
typedef void (APIENTRY * GdkGLProc_glCopyColorSubTable) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyColorSubTable (void);
#define      gdk_gl_glCopyColorSubTable(proc, target, start, x, y, width) \
  ( ((GdkGLProc_glCopyColorSubTable) (proc)) (target, start, x, y, width) )

/* glConvolutionFilter1D */
typedef void (APIENTRY * GdkGLProc_glConvolutionFilter1D) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
GdkGLProc    gdk_gl_get_glConvolutionFilter1D (void);
#define      gdk_gl_glConvolutionFilter1D(proc, target, internalformat, width, format, type, image) \
  ( ((GdkGLProc_glConvolutionFilter1D) (proc)) (target, internalformat, width, format, type, image) )

/* glConvolutionFilter2D */
typedef void (APIENTRY * GdkGLProc_glConvolutionFilter2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
GdkGLProc    gdk_gl_get_glConvolutionFilter2D (void);
#define      gdk_gl_glConvolutionFilter2D(proc, target, internalformat, width, height, format, type, image) \
  ( ((GdkGLProc_glConvolutionFilter2D) (proc)) (target, internalformat, width, height, format, type, image) )

/* glConvolutionParameterf */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameterf) (GLenum target, GLenum pname, GLfloat params);
GdkGLProc    gdk_gl_get_glConvolutionParameterf (void);
#define      gdk_gl_glConvolutionParameterf(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameterf) (proc)) (target, pname, params) )

/* glConvolutionParameterfv */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glConvolutionParameterfv (void);
#define      gdk_gl_glConvolutionParameterfv(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameterfv) (proc)) (target, pname, params) )

/* glConvolutionParameteri */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameteri) (GLenum target, GLenum pname, GLint params);
GdkGLProc    gdk_gl_get_glConvolutionParameteri (void);
#define      gdk_gl_glConvolutionParameteri(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameteri) (proc)) (target, pname, params) )

/* glConvolutionParameteriv */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameteriv) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glConvolutionParameteriv (void);
#define      gdk_gl_glConvolutionParameteriv(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameteriv) (proc)) (target, pname, params) )

/* glCopyConvolutionFilter1D */
typedef void (APIENTRY * GdkGLProc_glCopyConvolutionFilter1D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyConvolutionFilter1D (void);
#define      gdk_gl_glCopyConvolutionFilter1D(proc, target, internalformat, x, y, width) \
  ( ((GdkGLProc_glCopyConvolutionFilter1D) (proc)) (target, internalformat, x, y, width) )

/* glCopyConvolutionFilter2D */
typedef void (APIENTRY * GdkGLProc_glCopyConvolutionFilter2D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glCopyConvolutionFilter2D (void);
#define      gdk_gl_glCopyConvolutionFilter2D(proc, target, internalformat, x, y, width, height) \
  ( ((GdkGLProc_glCopyConvolutionFilter2D) (proc)) (target, internalformat, x, y, width, height) )

/* glGetConvolutionFilter */
typedef void (APIENTRY * GdkGLProc_glGetConvolutionFilter) (GLenum target, GLenum format, GLenum type, GLvoid *image);
GdkGLProc    gdk_gl_get_glGetConvolutionFilter (void);
#define      gdk_gl_glGetConvolutionFilter(proc, target, format, type, image) \
  ( ((GdkGLProc_glGetConvolutionFilter) (proc)) (target, format, type, image) )

/* glGetConvolutionParameterfv */
typedef void (APIENTRY * GdkGLProc_glGetConvolutionParameterfv) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetConvolutionParameterfv (void);
#define      gdk_gl_glGetConvolutionParameterfv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetConvolutionParameterfv) (proc)) (target, pname, params) )

/* glGetConvolutionParameteriv */
typedef void (APIENTRY * GdkGLProc_glGetConvolutionParameteriv) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetConvolutionParameteriv (void);
#define      gdk_gl_glGetConvolutionParameteriv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetConvolutionParameteriv) (proc)) (target, pname, params) )

/* glGetSeparableFilter */
typedef void (APIENTRY * GdkGLProc_glGetSeparableFilter) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
GdkGLProc    gdk_gl_get_glGetSeparableFilter (void);
#define      gdk_gl_glGetSeparableFilter(proc, target, format, type, row, column, span) \
  ( ((GdkGLProc_glGetSeparableFilter) (proc)) (target, format, type, row, column, span) )

/* glSeparableFilter2D */
typedef void (APIENTRY * GdkGLProc_glSeparableFilter2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
GdkGLProc    gdk_gl_get_glSeparableFilter2D (void);
#define      gdk_gl_glSeparableFilter2D(proc, target, internalformat, width, height, format, type, row, column) \
  ( ((GdkGLProc_glSeparableFilter2D) (proc)) (target, internalformat, width, height, format, type, row, column) )

/* glGetHistogram */
typedef void (APIENTRY * GdkGLProc_glGetHistogram) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
GdkGLProc    gdk_gl_get_glGetHistogram (void);
#define      gdk_gl_glGetHistogram(proc, target, reset, format, type, values) \
  ( ((GdkGLProc_glGetHistogram) (proc)) (target, reset, format, type, values) )

/* glGetHistogramParameterfv */
typedef void (APIENTRY * GdkGLProc_glGetHistogramParameterfv) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetHistogramParameterfv (void);
#define      gdk_gl_glGetHistogramParameterfv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetHistogramParameterfv) (proc)) (target, pname, params) )

/* glGetHistogramParameteriv */
typedef void (APIENTRY * GdkGLProc_glGetHistogramParameteriv) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetHistogramParameteriv (void);
#define      gdk_gl_glGetHistogramParameteriv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetHistogramParameteriv) (proc)) (target, pname, params) )

/* glGetMinmax */
typedef void (APIENTRY * GdkGLProc_glGetMinmax) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
GdkGLProc    gdk_gl_get_glGetMinmax (void);
#define      gdk_gl_glGetMinmax(proc, target, reset, format, type, values) \
  ( ((GdkGLProc_glGetMinmax) (proc)) (target, reset, format, type, values) )

/* glGetMinmaxParameterfv */
typedef void (APIENTRY * GdkGLProc_glGetMinmaxParameterfv) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetMinmaxParameterfv (void);
#define      gdk_gl_glGetMinmaxParameterfv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetMinmaxParameterfv) (proc)) (target, pname, params) )

/* glGetMinmaxParameteriv */
typedef void (APIENTRY * GdkGLProc_glGetMinmaxParameteriv) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetMinmaxParameteriv (void);
#define      gdk_gl_glGetMinmaxParameteriv(proc, target, pname, params) \
  ( ((GdkGLProc_glGetMinmaxParameteriv) (proc)) (target, pname, params) )

/* glHistogram */
typedef void (APIENTRY * GdkGLProc_glHistogram) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
GdkGLProc    gdk_gl_get_glHistogram (void);
#define      gdk_gl_glHistogram(proc, target, width, internalformat, sink) \
  ( ((GdkGLProc_glHistogram) (proc)) (target, width, internalformat, sink) )

/* glMinmax */
typedef void (APIENTRY * GdkGLProc_glMinmax) (GLenum target, GLenum internalformat, GLboolean sink);
GdkGLProc    gdk_gl_get_glMinmax (void);
#define      gdk_gl_glMinmax(proc, target, internalformat, sink) \
  ( ((GdkGLProc_glMinmax) (proc)) (target, internalformat, sink) )

/* glResetHistogram */
typedef void (APIENTRY * GdkGLProc_glResetHistogram) (GLenum target);
GdkGLProc    gdk_gl_get_glResetHistogram (void);
#define      gdk_gl_glResetHistogram(proc, target) \
  ( ((GdkGLProc_glResetHistogram) (proc)) (target) )

/* glResetMinmax */
typedef void (APIENTRY * GdkGLProc_glResetMinmax) (GLenum target);
GdkGLProc    gdk_gl_get_glResetMinmax (void);
#define      gdk_gl_glResetMinmax(proc, target) \
  ( ((GdkGLProc_glResetMinmax) (proc)) (target) )

/* glTexImage3D */
typedef void (APIENTRY * GdkGLProc_glTexImage3D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexImage3D (void);
#define      gdk_gl_glTexImage3D(proc, target, level, internalformat, width, height, depth, border, format, type, pixels) \
  ( ((GdkGLProc_glTexImage3D) (proc)) (target, level, internalformat, width, height, depth, border, format, type, pixels) )

/* glTexSubImage3D */
typedef void (APIENTRY * GdkGLProc_glTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexSubImage3D (void);
#define      gdk_gl_glTexSubImage3D(proc, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
  ( ((GdkGLProc_glTexSubImage3D) (proc)) (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) )

/* glCopyTexSubImage3D */
typedef void (APIENTRY * GdkGLProc_glCopyTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glCopyTexSubImage3D (void);
#define      gdk_gl_glCopyTexSubImage3D(proc, target, level, xoffset, yoffset, zoffset, x, y, width, height) \
  ( ((GdkGLProc_glCopyTexSubImage3D) (proc)) (target, level, xoffset, yoffset, zoffset, x, y, width, height) )

/* proc struct */

typedef struct _GdkGL_GL_VERSION_1_2 GdkGL_GL_VERSION_1_2;

struct _GdkGL_GL_VERSION_1_2
{
  GdkGLProc_glBlendColor glBlendColor;
  GdkGLProc_glBlendEquation glBlendEquation;
  GdkGLProc_glDrawRangeElements glDrawRangeElements;
  GdkGLProc_glColorTable glColorTable;
  GdkGLProc_glColorTableParameterfv glColorTableParameterfv;
  GdkGLProc_glColorTableParameteriv glColorTableParameteriv;
  GdkGLProc_glCopyColorTable glCopyColorTable;
  GdkGLProc_glGetColorTable glGetColorTable;
  GdkGLProc_glGetColorTableParameterfv glGetColorTableParameterfv;
  GdkGLProc_glGetColorTableParameteriv glGetColorTableParameteriv;
  GdkGLProc_glColorSubTable glColorSubTable;
  GdkGLProc_glCopyColorSubTable glCopyColorSubTable;
  GdkGLProc_glConvolutionFilter1D glConvolutionFilter1D;
  GdkGLProc_glConvolutionFilter2D glConvolutionFilter2D;
  GdkGLProc_glConvolutionParameterf glConvolutionParameterf;
  GdkGLProc_glConvolutionParameterfv glConvolutionParameterfv;
  GdkGLProc_glConvolutionParameteri glConvolutionParameteri;
  GdkGLProc_glConvolutionParameteriv glConvolutionParameteriv;
  GdkGLProc_glCopyConvolutionFilter1D glCopyConvolutionFilter1D;
  GdkGLProc_glCopyConvolutionFilter2D glCopyConvolutionFilter2D;
  GdkGLProc_glGetConvolutionFilter glGetConvolutionFilter;
  GdkGLProc_glGetConvolutionParameterfv glGetConvolutionParameterfv;
  GdkGLProc_glGetConvolutionParameteriv glGetConvolutionParameteriv;
  GdkGLProc_glGetSeparableFilter glGetSeparableFilter;
  GdkGLProc_glSeparableFilter2D glSeparableFilter2D;
  GdkGLProc_glGetHistogram glGetHistogram;
  GdkGLProc_glGetHistogramParameterfv glGetHistogramParameterfv;
  GdkGLProc_glGetHistogramParameteriv glGetHistogramParameteriv;
  GdkGLProc_glGetMinmax glGetMinmax;
  GdkGLProc_glGetMinmaxParameterfv glGetMinmaxParameterfv;
  GdkGLProc_glGetMinmaxParameteriv glGetMinmaxParameteriv;
  GdkGLProc_glHistogram glHistogram;
  GdkGLProc_glMinmax glMinmax;
  GdkGLProc_glResetHistogram glResetHistogram;
  GdkGLProc_glResetMinmax glResetMinmax;
  GdkGLProc_glTexImage3D glTexImage3D;
  GdkGLProc_glTexSubImage3D glTexSubImage3D;
  GdkGLProc_glCopyTexSubImage3D glCopyTexSubImage3D;
};

GdkGL_GL_VERSION_1_2 *gdk_gl_get_GL_VERSION_1_2 (void);

/*
 * GL_VERSION_1_3
 */

/* glActiveTexture */
typedef void (APIENTRY * GdkGLProc_glActiveTexture) (GLenum texture);
GdkGLProc    gdk_gl_get_glActiveTexture (void);
#define      gdk_gl_glActiveTexture(proc, texture) \
  ( ((GdkGLProc_glActiveTexture) (proc)) (texture) )

/* glClientActiveTexture */
typedef void (APIENTRY * GdkGLProc_glClientActiveTexture) (GLenum texture);
GdkGLProc    gdk_gl_get_glClientActiveTexture (void);
#define      gdk_gl_glClientActiveTexture(proc, texture) \
  ( ((GdkGLProc_glClientActiveTexture) (proc)) (texture) )

/* glMultiTexCoord1d */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1d) (GLenum target, GLdouble s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1d (void);
#define      gdk_gl_glMultiTexCoord1d(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1d) (proc)) (target, s) )

/* glMultiTexCoord1dv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dv) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dv (void);
#define      gdk_gl_glMultiTexCoord1dv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1dv) (proc)) (target, v) )

/* glMultiTexCoord1f */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1f) (GLenum target, GLfloat s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1f (void);
#define      gdk_gl_glMultiTexCoord1f(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1f) (proc)) (target, s) )

/* glMultiTexCoord1fv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fv) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fv (void);
#define      gdk_gl_glMultiTexCoord1fv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1fv) (proc)) (target, v) )

/* glMultiTexCoord1i */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1i) (GLenum target, GLint s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1i (void);
#define      gdk_gl_glMultiTexCoord1i(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1i) (proc)) (target, s) )

/* glMultiTexCoord1iv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1iv) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1iv (void);
#define      gdk_gl_glMultiTexCoord1iv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1iv) (proc)) (target, v) )

/* glMultiTexCoord1s */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1s) (GLenum target, GLshort s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1s (void);
#define      gdk_gl_glMultiTexCoord1s(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1s) (proc)) (target, s) )

/* glMultiTexCoord1sv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1sv) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1sv (void);
#define      gdk_gl_glMultiTexCoord1sv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1sv) (proc)) (target, v) )

/* glMultiTexCoord2d */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2d) (GLenum target, GLdouble s, GLdouble t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2d (void);
#define      gdk_gl_glMultiTexCoord2d(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2d) (proc)) (target, s, t) )

/* glMultiTexCoord2dv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dv) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dv (void);
#define      gdk_gl_glMultiTexCoord2dv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2dv) (proc)) (target, v) )

/* glMultiTexCoord2f */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2f) (GLenum target, GLfloat s, GLfloat t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2f (void);
#define      gdk_gl_glMultiTexCoord2f(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2f) (proc)) (target, s, t) )

/* glMultiTexCoord2fv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fv) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fv (void);
#define      gdk_gl_glMultiTexCoord2fv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2fv) (proc)) (target, v) )

/* glMultiTexCoord2i */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2i) (GLenum target, GLint s, GLint t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2i (void);
#define      gdk_gl_glMultiTexCoord2i(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2i) (proc)) (target, s, t) )

/* glMultiTexCoord2iv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2iv) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2iv (void);
#define      gdk_gl_glMultiTexCoord2iv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2iv) (proc)) (target, v) )

/* glMultiTexCoord2s */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2s) (GLenum target, GLshort s, GLshort t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2s (void);
#define      gdk_gl_glMultiTexCoord2s(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2s) (proc)) (target, s, t) )

/* glMultiTexCoord2sv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2sv) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2sv (void);
#define      gdk_gl_glMultiTexCoord2sv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2sv) (proc)) (target, v) )

/* glMultiTexCoord3d */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3d) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3d (void);
#define      gdk_gl_glMultiTexCoord3d(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3d) (proc)) (target, s, t, r) )

/* glMultiTexCoord3dv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dv) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dv (void);
#define      gdk_gl_glMultiTexCoord3dv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3dv) (proc)) (target, v) )

/* glMultiTexCoord3f */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3f) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3f (void);
#define      gdk_gl_glMultiTexCoord3f(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3f) (proc)) (target, s, t, r) )

/* glMultiTexCoord3fv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fv) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fv (void);
#define      gdk_gl_glMultiTexCoord3fv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3fv) (proc)) (target, v) )

/* glMultiTexCoord3i */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3i) (GLenum target, GLint s, GLint t, GLint r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3i (void);
#define      gdk_gl_glMultiTexCoord3i(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3i) (proc)) (target, s, t, r) )

/* glMultiTexCoord3iv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3iv) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3iv (void);
#define      gdk_gl_glMultiTexCoord3iv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3iv) (proc)) (target, v) )

/* glMultiTexCoord3s */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3s) (GLenum target, GLshort s, GLshort t, GLshort r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3s (void);
#define      gdk_gl_glMultiTexCoord3s(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3s) (proc)) (target, s, t, r) )

/* glMultiTexCoord3sv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3sv) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3sv (void);
#define      gdk_gl_glMultiTexCoord3sv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3sv) (proc)) (target, v) )

/* glMultiTexCoord4d */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4d) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4d (void);
#define      gdk_gl_glMultiTexCoord4d(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4d) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4dv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dv) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dv (void);
#define      gdk_gl_glMultiTexCoord4dv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4dv) (proc)) (target, v) )

/* glMultiTexCoord4f */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4f) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4f (void);
#define      gdk_gl_glMultiTexCoord4f(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4f) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4fv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fv) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fv (void);
#define      gdk_gl_glMultiTexCoord4fv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4fv) (proc)) (target, v) )

/* glMultiTexCoord4i */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4i) (GLenum target, GLint s, GLint t, GLint r, GLint q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4i (void);
#define      gdk_gl_glMultiTexCoord4i(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4i) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4iv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4iv) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4iv (void);
#define      gdk_gl_glMultiTexCoord4iv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4iv) (proc)) (target, v) )

/* glMultiTexCoord4s */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4s) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4s (void);
#define      gdk_gl_glMultiTexCoord4s(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4s) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4sv */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4sv) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4sv (void);
#define      gdk_gl_glMultiTexCoord4sv(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4sv) (proc)) (target, v) )

/* glLoadTransposeMatrixf */
typedef void (APIENTRY * GdkGLProc_glLoadTransposeMatrixf) (const GLfloat *m);
GdkGLProc    gdk_gl_get_glLoadTransposeMatrixf (void);
#define      gdk_gl_glLoadTransposeMatrixf(proc, m) \
  ( ((GdkGLProc_glLoadTransposeMatrixf) (proc)) (m) )

/* glLoadTransposeMatrixd */
typedef void (APIENTRY * GdkGLProc_glLoadTransposeMatrixd) (const GLdouble *m);
GdkGLProc    gdk_gl_get_glLoadTransposeMatrixd (void);
#define      gdk_gl_glLoadTransposeMatrixd(proc, m) \
  ( ((GdkGLProc_glLoadTransposeMatrixd) (proc)) (m) )

/* glMultTransposeMatrixf */
typedef void (APIENTRY * GdkGLProc_glMultTransposeMatrixf) (const GLfloat *m);
GdkGLProc    gdk_gl_get_glMultTransposeMatrixf (void);
#define      gdk_gl_glMultTransposeMatrixf(proc, m) \
  ( ((GdkGLProc_glMultTransposeMatrixf) (proc)) (m) )

/* glMultTransposeMatrixd */
typedef void (APIENTRY * GdkGLProc_glMultTransposeMatrixd) (const GLdouble *m);
GdkGLProc    gdk_gl_get_glMultTransposeMatrixd (void);
#define      gdk_gl_glMultTransposeMatrixd(proc, m) \
  ( ((GdkGLProc_glMultTransposeMatrixd) (proc)) (m) )

/* glSampleCoverage */
typedef void (APIENTRY * GdkGLProc_glSampleCoverage) (GLclampf value, GLboolean invert);
GdkGLProc    gdk_gl_get_glSampleCoverage (void);
#define      gdk_gl_glSampleCoverage(proc, value, invert) \
  ( ((GdkGLProc_glSampleCoverage) (proc)) (value, invert) )

/* glCompressedTexImage3D */
typedef void (APIENTRY * GdkGLProc_glCompressedTexImage3D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexImage3D (void);
#define      gdk_gl_glCompressedTexImage3D(proc, target, level, internalformat, width, height, depth, border, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexImage3D) (proc)) (target, level, internalformat, width, height, depth, border, imageSize, data) )

/* glCompressedTexImage2D */
typedef void (APIENTRY * GdkGLProc_glCompressedTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexImage2D (void);
#define      gdk_gl_glCompressedTexImage2D(proc, target, level, internalformat, width, height, border, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexImage2D) (proc)) (target, level, internalformat, width, height, border, imageSize, data) )

/* glCompressedTexImage1D */
typedef void (APIENTRY * GdkGLProc_glCompressedTexImage1D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexImage1D (void);
#define      gdk_gl_glCompressedTexImage1D(proc, target, level, internalformat, width, border, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexImage1D) (proc)) (target, level, internalformat, width, border, imageSize, data) )

/* glCompressedTexSubImage3D */
typedef void (APIENTRY * GdkGLProc_glCompressedTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexSubImage3D (void);
#define      gdk_gl_glCompressedTexSubImage3D(proc, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexSubImage3D) (proc)) (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) )

/* glCompressedTexSubImage2D */
typedef void (APIENTRY * GdkGLProc_glCompressedTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexSubImage2D (void);
#define      gdk_gl_glCompressedTexSubImage2D(proc, target, level, xoffset, yoffset, width, height, format, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexSubImage2D) (proc)) (target, level, xoffset, yoffset, width, height, format, imageSize, data) )

/* glCompressedTexSubImage1D */
typedef void (APIENTRY * GdkGLProc_glCompressedTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexSubImage1D (void);
#define      gdk_gl_glCompressedTexSubImage1D(proc, target, level, xoffset, width, format, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexSubImage1D) (proc)) (target, level, xoffset, width, format, imageSize, data) )

/* glGetCompressedTexImage */
typedef void (APIENTRY * GdkGLProc_glGetCompressedTexImage) (GLenum target, GLint level, GLvoid *img);
GdkGLProc    gdk_gl_get_glGetCompressedTexImage (void);
#define      gdk_gl_glGetCompressedTexImage(proc, target, level, img) \
  ( ((GdkGLProc_glGetCompressedTexImage) (proc)) (target, level, img) )

/* proc struct */

typedef struct _GdkGL_GL_VERSION_1_3 GdkGL_GL_VERSION_1_3;

struct _GdkGL_GL_VERSION_1_3
{
  GdkGLProc_glActiveTexture glActiveTexture;
  GdkGLProc_glClientActiveTexture glClientActiveTexture;
  GdkGLProc_glMultiTexCoord1d glMultiTexCoord1d;
  GdkGLProc_glMultiTexCoord1dv glMultiTexCoord1dv;
  GdkGLProc_glMultiTexCoord1f glMultiTexCoord1f;
  GdkGLProc_glMultiTexCoord1fv glMultiTexCoord1fv;
  GdkGLProc_glMultiTexCoord1i glMultiTexCoord1i;
  GdkGLProc_glMultiTexCoord1iv glMultiTexCoord1iv;
  GdkGLProc_glMultiTexCoord1s glMultiTexCoord1s;
  GdkGLProc_glMultiTexCoord1sv glMultiTexCoord1sv;
  GdkGLProc_glMultiTexCoord2d glMultiTexCoord2d;
  GdkGLProc_glMultiTexCoord2dv glMultiTexCoord2dv;
  GdkGLProc_glMultiTexCoord2f glMultiTexCoord2f;
  GdkGLProc_glMultiTexCoord2fv glMultiTexCoord2fv;
  GdkGLProc_glMultiTexCoord2i glMultiTexCoord2i;
  GdkGLProc_glMultiTexCoord2iv glMultiTexCoord2iv;
  GdkGLProc_glMultiTexCoord2s glMultiTexCoord2s;
  GdkGLProc_glMultiTexCoord2sv glMultiTexCoord2sv;
  GdkGLProc_glMultiTexCoord3d glMultiTexCoord3d;
  GdkGLProc_glMultiTexCoord3dv glMultiTexCoord3dv;
  GdkGLProc_glMultiTexCoord3f glMultiTexCoord3f;
  GdkGLProc_glMultiTexCoord3fv glMultiTexCoord3fv;
  GdkGLProc_glMultiTexCoord3i glMultiTexCoord3i;
  GdkGLProc_glMultiTexCoord3iv glMultiTexCoord3iv;
  GdkGLProc_glMultiTexCoord3s glMultiTexCoord3s;
  GdkGLProc_glMultiTexCoord3sv glMultiTexCoord3sv;
  GdkGLProc_glMultiTexCoord4d glMultiTexCoord4d;
  GdkGLProc_glMultiTexCoord4dv glMultiTexCoord4dv;
  GdkGLProc_glMultiTexCoord4f glMultiTexCoord4f;
  GdkGLProc_glMultiTexCoord4fv glMultiTexCoord4fv;
  GdkGLProc_glMultiTexCoord4i glMultiTexCoord4i;
  GdkGLProc_glMultiTexCoord4iv glMultiTexCoord4iv;
  GdkGLProc_glMultiTexCoord4s glMultiTexCoord4s;
  GdkGLProc_glMultiTexCoord4sv glMultiTexCoord4sv;
  GdkGLProc_glLoadTransposeMatrixf glLoadTransposeMatrixf;
  GdkGLProc_glLoadTransposeMatrixd glLoadTransposeMatrixd;
  GdkGLProc_glMultTransposeMatrixf glMultTransposeMatrixf;
  GdkGLProc_glMultTransposeMatrixd glMultTransposeMatrixd;
  GdkGLProc_glSampleCoverage glSampleCoverage;
  GdkGLProc_glCompressedTexImage3D glCompressedTexImage3D;
  GdkGLProc_glCompressedTexImage2D glCompressedTexImage2D;
  GdkGLProc_glCompressedTexImage1D glCompressedTexImage1D;
  GdkGLProc_glCompressedTexSubImage3D glCompressedTexSubImage3D;
  GdkGLProc_glCompressedTexSubImage2D glCompressedTexSubImage2D;
  GdkGLProc_glCompressedTexSubImage1D glCompressedTexSubImage1D;
  GdkGLProc_glGetCompressedTexImage glGetCompressedTexImage;
};

GdkGL_GL_VERSION_1_3 *gdk_gl_get_GL_VERSION_1_3 (void);

/*
 * GL_VERSION_1_4
 */

/* glBlendFuncSeparate */
typedef void (APIENTRY * GdkGLProc_glBlendFuncSeparate) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GdkGLProc    gdk_gl_get_glBlendFuncSeparate (void);
#define      gdk_gl_glBlendFuncSeparate(proc, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha) \
  ( ((GdkGLProc_glBlendFuncSeparate) (proc)) (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha) )

/* glFogCoordf */
typedef void (APIENTRY * GdkGLProc_glFogCoordf) (GLfloat coord);
GdkGLProc    gdk_gl_get_glFogCoordf (void);
#define      gdk_gl_glFogCoordf(proc, coord) \
  ( ((GdkGLProc_glFogCoordf) (proc)) (coord) )

/* glFogCoordfv */
typedef void (APIENTRY * GdkGLProc_glFogCoordfv) (const GLfloat *coord);
GdkGLProc    gdk_gl_get_glFogCoordfv (void);
#define      gdk_gl_glFogCoordfv(proc, coord) \
  ( ((GdkGLProc_glFogCoordfv) (proc)) (coord) )

/* glFogCoordd */
typedef void (APIENTRY * GdkGLProc_glFogCoordd) (GLdouble coord);
GdkGLProc    gdk_gl_get_glFogCoordd (void);
#define      gdk_gl_glFogCoordd(proc, coord) \
  ( ((GdkGLProc_glFogCoordd) (proc)) (coord) )

/* glFogCoorddv */
typedef void (APIENTRY * GdkGLProc_glFogCoorddv) (const GLdouble *coord);
GdkGLProc    gdk_gl_get_glFogCoorddv (void);
#define      gdk_gl_glFogCoorddv(proc, coord) \
  ( ((GdkGLProc_glFogCoorddv) (proc)) (coord) )

/* glFogCoordPointer */
typedef void (APIENTRY * GdkGLProc_glFogCoordPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glFogCoordPointer (void);
#define      gdk_gl_glFogCoordPointer(proc, type, stride, pointer) \
  ( ((GdkGLProc_glFogCoordPointer) (proc)) (type, stride, pointer) )

/* glMultiDrawArrays */
typedef void (APIENTRY * GdkGLProc_glMultiDrawArrays) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawArrays (void);
#define      gdk_gl_glMultiDrawArrays(proc, mode, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawArrays) (proc)) (mode, first, count, primcount) )

/* glMultiDrawElements */
typedef void (APIENTRY * GdkGLProc_glMultiDrawElements) (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawElements (void);
#define      gdk_gl_glMultiDrawElements(proc, mode, count, type, indices, primcount) \
  ( ((GdkGLProc_glMultiDrawElements) (proc)) (mode, count, type, indices, primcount) )

/* glPointParameterf */
typedef void (APIENTRY * GdkGLProc_glPointParameterf) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPointParameterf (void);
#define      gdk_gl_glPointParameterf(proc, pname, param) \
  ( ((GdkGLProc_glPointParameterf) (proc)) (pname, param) )

/* glPointParameterfv */
typedef void (APIENTRY * GdkGLProc_glPointParameterfv) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glPointParameterfv (void);
#define      gdk_gl_glPointParameterfv(proc, pname, params) \
  ( ((GdkGLProc_glPointParameterfv) (proc)) (pname, params) )

/* glPointParameteri */
typedef void (APIENTRY * GdkGLProc_glPointParameteri) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glPointParameteri (void);
#define      gdk_gl_glPointParameteri(proc, pname, param) \
  ( ((GdkGLProc_glPointParameteri) (proc)) (pname, param) )

/* glPointParameteriv */
typedef void (APIENTRY * GdkGLProc_glPointParameteriv) (GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glPointParameteriv (void);
#define      gdk_gl_glPointParameteriv(proc, pname, params) \
  ( ((GdkGLProc_glPointParameteriv) (proc)) (pname, params) )

/* glSecondaryColor3b */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3b) (GLbyte red, GLbyte green, GLbyte blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3b (void);
#define      gdk_gl_glSecondaryColor3b(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3b) (proc)) (red, green, blue) )

/* glSecondaryColor3bv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3bv) (const GLbyte *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3bv (void);
#define      gdk_gl_glSecondaryColor3bv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3bv) (proc)) (v) )

/* glSecondaryColor3d */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3d) (GLdouble red, GLdouble green, GLdouble blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3d (void);
#define      gdk_gl_glSecondaryColor3d(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3d) (proc)) (red, green, blue) )

/* glSecondaryColor3dv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3dv) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3dv (void);
#define      gdk_gl_glSecondaryColor3dv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3dv) (proc)) (v) )

/* glSecondaryColor3f */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3f) (GLfloat red, GLfloat green, GLfloat blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3f (void);
#define      gdk_gl_glSecondaryColor3f(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3f) (proc)) (red, green, blue) )

/* glSecondaryColor3fv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3fv) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3fv (void);
#define      gdk_gl_glSecondaryColor3fv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3fv) (proc)) (v) )

/* glSecondaryColor3i */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3i) (GLint red, GLint green, GLint blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3i (void);
#define      gdk_gl_glSecondaryColor3i(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3i) (proc)) (red, green, blue) )

/* glSecondaryColor3iv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3iv) (const GLint *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3iv (void);
#define      gdk_gl_glSecondaryColor3iv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3iv) (proc)) (v) )

/* glSecondaryColor3s */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3s) (GLshort red, GLshort green, GLshort blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3s (void);
#define      gdk_gl_glSecondaryColor3s(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3s) (proc)) (red, green, blue) )

/* glSecondaryColor3sv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3sv) (const GLshort *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3sv (void);
#define      gdk_gl_glSecondaryColor3sv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3sv) (proc)) (v) )

/* glSecondaryColor3ub */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3ub (void);
#define      gdk_gl_glSecondaryColor3ub(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3ub) (proc)) (red, green, blue) )

/* glSecondaryColor3ubv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3ubv) (const GLubyte *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3ubv (void);
#define      gdk_gl_glSecondaryColor3ubv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3ubv) (proc)) (v) )

/* glSecondaryColor3ui */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3ui) (GLuint red, GLuint green, GLuint blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3ui (void);
#define      gdk_gl_glSecondaryColor3ui(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3ui) (proc)) (red, green, blue) )

/* glSecondaryColor3uiv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3uiv) (const GLuint *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3uiv (void);
#define      gdk_gl_glSecondaryColor3uiv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3uiv) (proc)) (v) )

/* glSecondaryColor3us */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3us) (GLushort red, GLushort green, GLushort blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3us (void);
#define      gdk_gl_glSecondaryColor3us(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3us) (proc)) (red, green, blue) )

/* glSecondaryColor3usv */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3usv) (const GLushort *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3usv (void);
#define      gdk_gl_glSecondaryColor3usv(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3usv) (proc)) (v) )

/* glSecondaryColorPointer */
typedef void (APIENTRY * GdkGLProc_glSecondaryColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glSecondaryColorPointer (void);
#define      gdk_gl_glSecondaryColorPointer(proc, size, type, stride, pointer) \
  ( ((GdkGLProc_glSecondaryColorPointer) (proc)) (size, type, stride, pointer) )

/* glWindowPos2d */
typedef void (APIENTRY * GdkGLProc_glWindowPos2d) (GLdouble x, GLdouble y);
GdkGLProc    gdk_gl_get_glWindowPos2d (void);
#define      gdk_gl_glWindowPos2d(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2d) (proc)) (x, y) )

/* glWindowPos2dv */
typedef void (APIENTRY * GdkGLProc_glWindowPos2dv) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos2dv (void);
#define      gdk_gl_glWindowPos2dv(proc, v) \
  ( ((GdkGLProc_glWindowPos2dv) (proc)) (v) )

/* glWindowPos2f */
typedef void (APIENTRY * GdkGLProc_glWindowPos2f) (GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glWindowPos2f (void);
#define      gdk_gl_glWindowPos2f(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2f) (proc)) (x, y) )

/* glWindowPos2fv */
typedef void (APIENTRY * GdkGLProc_glWindowPos2fv) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos2fv (void);
#define      gdk_gl_glWindowPos2fv(proc, v) \
  ( ((GdkGLProc_glWindowPos2fv) (proc)) (v) )

/* glWindowPos2i */
typedef void (APIENTRY * GdkGLProc_glWindowPos2i) (GLint x, GLint y);
GdkGLProc    gdk_gl_get_glWindowPos2i (void);
#define      gdk_gl_glWindowPos2i(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2i) (proc)) (x, y) )

/* glWindowPos2iv */
typedef void (APIENTRY * GdkGLProc_glWindowPos2iv) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos2iv (void);
#define      gdk_gl_glWindowPos2iv(proc, v) \
  ( ((GdkGLProc_glWindowPos2iv) (proc)) (v) )

/* glWindowPos2s */
typedef void (APIENTRY * GdkGLProc_glWindowPos2s) (GLshort x, GLshort y);
GdkGLProc    gdk_gl_get_glWindowPos2s (void);
#define      gdk_gl_glWindowPos2s(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2s) (proc)) (x, y) )

/* glWindowPos2sv */
typedef void (APIENTRY * GdkGLProc_glWindowPos2sv) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos2sv (void);
#define      gdk_gl_glWindowPos2sv(proc, v) \
  ( ((GdkGLProc_glWindowPos2sv) (proc)) (v) )

/* glWindowPos3d */
typedef void (APIENTRY * GdkGLProc_glWindowPos3d) (GLdouble x, GLdouble y, GLdouble z);
GdkGLProc    gdk_gl_get_glWindowPos3d (void);
#define      gdk_gl_glWindowPos3d(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3d) (proc)) (x, y, z) )

/* glWindowPos3dv */
typedef void (APIENTRY * GdkGLProc_glWindowPos3dv) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos3dv (void);
#define      gdk_gl_glWindowPos3dv(proc, v) \
  ( ((GdkGLProc_glWindowPos3dv) (proc)) (v) )

/* glWindowPos3f */
typedef void (APIENTRY * GdkGLProc_glWindowPos3f) (GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glWindowPos3f (void);
#define      gdk_gl_glWindowPos3f(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3f) (proc)) (x, y, z) )

/* glWindowPos3fv */
typedef void (APIENTRY * GdkGLProc_glWindowPos3fv) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos3fv (void);
#define      gdk_gl_glWindowPos3fv(proc, v) \
  ( ((GdkGLProc_glWindowPos3fv) (proc)) (v) )

/* glWindowPos3i */
typedef void (APIENTRY * GdkGLProc_glWindowPos3i) (GLint x, GLint y, GLint z);
GdkGLProc    gdk_gl_get_glWindowPos3i (void);
#define      gdk_gl_glWindowPos3i(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3i) (proc)) (x, y, z) )

/* glWindowPos3iv */
typedef void (APIENTRY * GdkGLProc_glWindowPos3iv) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos3iv (void);
#define      gdk_gl_glWindowPos3iv(proc, v) \
  ( ((GdkGLProc_glWindowPos3iv) (proc)) (v) )

/* glWindowPos3s */
typedef void (APIENTRY * GdkGLProc_glWindowPos3s) (GLshort x, GLshort y, GLshort z);
GdkGLProc    gdk_gl_get_glWindowPos3s (void);
#define      gdk_gl_glWindowPos3s(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3s) (proc)) (x, y, z) )

/* glWindowPos3sv */
typedef void (APIENTRY * GdkGLProc_glWindowPos3sv) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos3sv (void);
#define      gdk_gl_glWindowPos3sv(proc, v) \
  ( ((GdkGLProc_glWindowPos3sv) (proc)) (v) )

/* proc struct */

typedef struct _GdkGL_GL_VERSION_1_4 GdkGL_GL_VERSION_1_4;

struct _GdkGL_GL_VERSION_1_4
{
  GdkGLProc_glBlendFuncSeparate glBlendFuncSeparate;
  GdkGLProc_glFogCoordf glFogCoordf;
  GdkGLProc_glFogCoordfv glFogCoordfv;
  GdkGLProc_glFogCoordd glFogCoordd;
  GdkGLProc_glFogCoorddv glFogCoorddv;
  GdkGLProc_glFogCoordPointer glFogCoordPointer;
  GdkGLProc_glMultiDrawArrays glMultiDrawArrays;
  GdkGLProc_glMultiDrawElements glMultiDrawElements;
  GdkGLProc_glPointParameterf glPointParameterf;
  GdkGLProc_glPointParameterfv glPointParameterfv;
  GdkGLProc_glPointParameteri glPointParameteri;
  GdkGLProc_glPointParameteriv glPointParameteriv;
  GdkGLProc_glSecondaryColor3b glSecondaryColor3b;
  GdkGLProc_glSecondaryColor3bv glSecondaryColor3bv;
  GdkGLProc_glSecondaryColor3d glSecondaryColor3d;
  GdkGLProc_glSecondaryColor3dv glSecondaryColor3dv;
  GdkGLProc_glSecondaryColor3f glSecondaryColor3f;
  GdkGLProc_glSecondaryColor3fv glSecondaryColor3fv;
  GdkGLProc_glSecondaryColor3i glSecondaryColor3i;
  GdkGLProc_glSecondaryColor3iv glSecondaryColor3iv;
  GdkGLProc_glSecondaryColor3s glSecondaryColor3s;
  GdkGLProc_glSecondaryColor3sv glSecondaryColor3sv;
  GdkGLProc_glSecondaryColor3ub glSecondaryColor3ub;
  GdkGLProc_glSecondaryColor3ubv glSecondaryColor3ubv;
  GdkGLProc_glSecondaryColor3ui glSecondaryColor3ui;
  GdkGLProc_glSecondaryColor3uiv glSecondaryColor3uiv;
  GdkGLProc_glSecondaryColor3us glSecondaryColor3us;
  GdkGLProc_glSecondaryColor3usv glSecondaryColor3usv;
  GdkGLProc_glSecondaryColorPointer glSecondaryColorPointer;
  GdkGLProc_glWindowPos2d glWindowPos2d;
  GdkGLProc_glWindowPos2dv glWindowPos2dv;
  GdkGLProc_glWindowPos2f glWindowPos2f;
  GdkGLProc_glWindowPos2fv glWindowPos2fv;
  GdkGLProc_glWindowPos2i glWindowPos2i;
  GdkGLProc_glWindowPos2iv glWindowPos2iv;
  GdkGLProc_glWindowPos2s glWindowPos2s;
  GdkGLProc_glWindowPos2sv glWindowPos2sv;
  GdkGLProc_glWindowPos3d glWindowPos3d;
  GdkGLProc_glWindowPos3dv glWindowPos3dv;
  GdkGLProc_glWindowPos3f glWindowPos3f;
  GdkGLProc_glWindowPos3fv glWindowPos3fv;
  GdkGLProc_glWindowPos3i glWindowPos3i;
  GdkGLProc_glWindowPos3iv glWindowPos3iv;
  GdkGLProc_glWindowPos3s glWindowPos3s;
  GdkGLProc_glWindowPos3sv glWindowPos3sv;
};

GdkGL_GL_VERSION_1_4 *gdk_gl_get_GL_VERSION_1_4 (void);

/*
 * GL_ARB_multitexture
 */

/* glActiveTextureARB */
typedef void (APIENTRY * GdkGLProc_glActiveTextureARB) (GLenum texture);
GdkGLProc    gdk_gl_get_glActiveTextureARB (void);
#define      gdk_gl_glActiveTextureARB(proc, texture) \
  ( ((GdkGLProc_glActiveTextureARB) (proc)) (texture) )

/* glClientActiveTextureARB */
typedef void (APIENTRY * GdkGLProc_glClientActiveTextureARB) (GLenum texture);
GdkGLProc    gdk_gl_get_glClientActiveTextureARB (void);
#define      gdk_gl_glClientActiveTextureARB(proc, texture) \
  ( ((GdkGLProc_glClientActiveTextureARB) (proc)) (texture) )

/* glMultiTexCoord1dARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dARB) (GLenum target, GLdouble s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dARB (void);
#define      gdk_gl_glMultiTexCoord1dARB(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1dARB) (proc)) (target, s) )

/* glMultiTexCoord1dvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dvARB) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dvARB (void);
#define      gdk_gl_glMultiTexCoord1dvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1dvARB) (proc)) (target, v) )

/* glMultiTexCoord1fARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fARB) (GLenum target, GLfloat s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fARB (void);
#define      gdk_gl_glMultiTexCoord1fARB(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1fARB) (proc)) (target, s) )

/* glMultiTexCoord1fvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fvARB) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fvARB (void);
#define      gdk_gl_glMultiTexCoord1fvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1fvARB) (proc)) (target, v) )

/* glMultiTexCoord1iARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1iARB) (GLenum target, GLint s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1iARB (void);
#define      gdk_gl_glMultiTexCoord1iARB(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1iARB) (proc)) (target, s) )

/* glMultiTexCoord1ivARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1ivARB) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1ivARB (void);
#define      gdk_gl_glMultiTexCoord1ivARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1ivARB) (proc)) (target, v) )

/* glMultiTexCoord1sARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1sARB) (GLenum target, GLshort s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1sARB (void);
#define      gdk_gl_glMultiTexCoord1sARB(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1sARB) (proc)) (target, s) )

/* glMultiTexCoord1svARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1svARB) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1svARB (void);
#define      gdk_gl_glMultiTexCoord1svARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1svARB) (proc)) (target, v) )

/* glMultiTexCoord2dARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dARB) (GLenum target, GLdouble s, GLdouble t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dARB (void);
#define      gdk_gl_glMultiTexCoord2dARB(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2dARB) (proc)) (target, s, t) )

/* glMultiTexCoord2dvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dvARB) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dvARB (void);
#define      gdk_gl_glMultiTexCoord2dvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2dvARB) (proc)) (target, v) )

/* glMultiTexCoord2fARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fARB (void);
#define      gdk_gl_glMultiTexCoord2fARB(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2fARB) (proc)) (target, s, t) )

/* glMultiTexCoord2fvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fvARB) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fvARB (void);
#define      gdk_gl_glMultiTexCoord2fvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2fvARB) (proc)) (target, v) )

/* glMultiTexCoord2iARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2iARB) (GLenum target, GLint s, GLint t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2iARB (void);
#define      gdk_gl_glMultiTexCoord2iARB(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2iARB) (proc)) (target, s, t) )

/* glMultiTexCoord2ivARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2ivARB) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2ivARB (void);
#define      gdk_gl_glMultiTexCoord2ivARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2ivARB) (proc)) (target, v) )

/* glMultiTexCoord2sARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2sARB) (GLenum target, GLshort s, GLshort t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2sARB (void);
#define      gdk_gl_glMultiTexCoord2sARB(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2sARB) (proc)) (target, s, t) )

/* glMultiTexCoord2svARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2svARB) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2svARB (void);
#define      gdk_gl_glMultiTexCoord2svARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2svARB) (proc)) (target, v) )

/* glMultiTexCoord3dARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dARB) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dARB (void);
#define      gdk_gl_glMultiTexCoord3dARB(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3dARB) (proc)) (target, s, t, r) )

/* glMultiTexCoord3dvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dvARB) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dvARB (void);
#define      gdk_gl_glMultiTexCoord3dvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3dvARB) (proc)) (target, v) )

/* glMultiTexCoord3fARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fARB) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fARB (void);
#define      gdk_gl_glMultiTexCoord3fARB(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3fARB) (proc)) (target, s, t, r) )

/* glMultiTexCoord3fvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fvARB) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fvARB (void);
#define      gdk_gl_glMultiTexCoord3fvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3fvARB) (proc)) (target, v) )

/* glMultiTexCoord3iARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3iARB) (GLenum target, GLint s, GLint t, GLint r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3iARB (void);
#define      gdk_gl_glMultiTexCoord3iARB(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3iARB) (proc)) (target, s, t, r) )

/* glMultiTexCoord3ivARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3ivARB) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3ivARB (void);
#define      gdk_gl_glMultiTexCoord3ivARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3ivARB) (proc)) (target, v) )

/* glMultiTexCoord3sARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3sARB) (GLenum target, GLshort s, GLshort t, GLshort r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3sARB (void);
#define      gdk_gl_glMultiTexCoord3sARB(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3sARB) (proc)) (target, s, t, r) )

/* glMultiTexCoord3svARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3svARB) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3svARB (void);
#define      gdk_gl_glMultiTexCoord3svARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3svARB) (proc)) (target, v) )

/* glMultiTexCoord4dARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dARB) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dARB (void);
#define      gdk_gl_glMultiTexCoord4dARB(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4dARB) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4dvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dvARB) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dvARB (void);
#define      gdk_gl_glMultiTexCoord4dvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4dvARB) (proc)) (target, v) )

/* glMultiTexCoord4fARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fARB) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fARB (void);
#define      gdk_gl_glMultiTexCoord4fARB(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4fARB) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4fvARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fvARB) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fvARB (void);
#define      gdk_gl_glMultiTexCoord4fvARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4fvARB) (proc)) (target, v) )

/* glMultiTexCoord4iARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4iARB) (GLenum target, GLint s, GLint t, GLint r, GLint q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4iARB (void);
#define      gdk_gl_glMultiTexCoord4iARB(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4iARB) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4ivARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4ivARB) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4ivARB (void);
#define      gdk_gl_glMultiTexCoord4ivARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4ivARB) (proc)) (target, v) )

/* glMultiTexCoord4sARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4sARB) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4sARB (void);
#define      gdk_gl_glMultiTexCoord4sARB(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4sARB) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4svARB */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4svARB) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4svARB (void);
#define      gdk_gl_glMultiTexCoord4svARB(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4svARB) (proc)) (target, v) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_multitexture GdkGL_GL_ARB_multitexture;

struct _GdkGL_GL_ARB_multitexture
{
  GdkGLProc_glActiveTextureARB glActiveTextureARB;
  GdkGLProc_glClientActiveTextureARB glClientActiveTextureARB;
  GdkGLProc_glMultiTexCoord1dARB glMultiTexCoord1dARB;
  GdkGLProc_glMultiTexCoord1dvARB glMultiTexCoord1dvARB;
  GdkGLProc_glMultiTexCoord1fARB glMultiTexCoord1fARB;
  GdkGLProc_glMultiTexCoord1fvARB glMultiTexCoord1fvARB;
  GdkGLProc_glMultiTexCoord1iARB glMultiTexCoord1iARB;
  GdkGLProc_glMultiTexCoord1ivARB glMultiTexCoord1ivARB;
  GdkGLProc_glMultiTexCoord1sARB glMultiTexCoord1sARB;
  GdkGLProc_glMultiTexCoord1svARB glMultiTexCoord1svARB;
  GdkGLProc_glMultiTexCoord2dARB glMultiTexCoord2dARB;
  GdkGLProc_glMultiTexCoord2dvARB glMultiTexCoord2dvARB;
  GdkGLProc_glMultiTexCoord2fARB glMultiTexCoord2fARB;
  GdkGLProc_glMultiTexCoord2fvARB glMultiTexCoord2fvARB;
  GdkGLProc_glMultiTexCoord2iARB glMultiTexCoord2iARB;
  GdkGLProc_glMultiTexCoord2ivARB glMultiTexCoord2ivARB;
  GdkGLProc_glMultiTexCoord2sARB glMultiTexCoord2sARB;
  GdkGLProc_glMultiTexCoord2svARB glMultiTexCoord2svARB;
  GdkGLProc_glMultiTexCoord3dARB glMultiTexCoord3dARB;
  GdkGLProc_glMultiTexCoord3dvARB glMultiTexCoord3dvARB;
  GdkGLProc_glMultiTexCoord3fARB glMultiTexCoord3fARB;
  GdkGLProc_glMultiTexCoord3fvARB glMultiTexCoord3fvARB;
  GdkGLProc_glMultiTexCoord3iARB glMultiTexCoord3iARB;
  GdkGLProc_glMultiTexCoord3ivARB glMultiTexCoord3ivARB;
  GdkGLProc_glMultiTexCoord3sARB glMultiTexCoord3sARB;
  GdkGLProc_glMultiTexCoord3svARB glMultiTexCoord3svARB;
  GdkGLProc_glMultiTexCoord4dARB glMultiTexCoord4dARB;
  GdkGLProc_glMultiTexCoord4dvARB glMultiTexCoord4dvARB;
  GdkGLProc_glMultiTexCoord4fARB glMultiTexCoord4fARB;
  GdkGLProc_glMultiTexCoord4fvARB glMultiTexCoord4fvARB;
  GdkGLProc_glMultiTexCoord4iARB glMultiTexCoord4iARB;
  GdkGLProc_glMultiTexCoord4ivARB glMultiTexCoord4ivARB;
  GdkGLProc_glMultiTexCoord4sARB glMultiTexCoord4sARB;
  GdkGLProc_glMultiTexCoord4svARB glMultiTexCoord4svARB;
};

GdkGL_GL_ARB_multitexture *gdk_gl_get_GL_ARB_multitexture (void);

/*
 * GL_ARB_transpose_matrix
 */

/* glLoadTransposeMatrixfARB */
typedef void (APIENTRY * GdkGLProc_glLoadTransposeMatrixfARB) (const GLfloat *m);
GdkGLProc    gdk_gl_get_glLoadTransposeMatrixfARB (void);
#define      gdk_gl_glLoadTransposeMatrixfARB(proc, m) \
  ( ((GdkGLProc_glLoadTransposeMatrixfARB) (proc)) (m) )

/* glLoadTransposeMatrixdARB */
typedef void (APIENTRY * GdkGLProc_glLoadTransposeMatrixdARB) (const GLdouble *m);
GdkGLProc    gdk_gl_get_glLoadTransposeMatrixdARB (void);
#define      gdk_gl_glLoadTransposeMatrixdARB(proc, m) \
  ( ((GdkGLProc_glLoadTransposeMatrixdARB) (proc)) (m) )

/* glMultTransposeMatrixfARB */
typedef void (APIENTRY * GdkGLProc_glMultTransposeMatrixfARB) (const GLfloat *m);
GdkGLProc    gdk_gl_get_glMultTransposeMatrixfARB (void);
#define      gdk_gl_glMultTransposeMatrixfARB(proc, m) \
  ( ((GdkGLProc_glMultTransposeMatrixfARB) (proc)) (m) )

/* glMultTransposeMatrixdARB */
typedef void (APIENTRY * GdkGLProc_glMultTransposeMatrixdARB) (const GLdouble *m);
GdkGLProc    gdk_gl_get_glMultTransposeMatrixdARB (void);
#define      gdk_gl_glMultTransposeMatrixdARB(proc, m) \
  ( ((GdkGLProc_glMultTransposeMatrixdARB) (proc)) (m) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_transpose_matrix GdkGL_GL_ARB_transpose_matrix;

struct _GdkGL_GL_ARB_transpose_matrix
{
  GdkGLProc_glLoadTransposeMatrixfARB glLoadTransposeMatrixfARB;
  GdkGLProc_glLoadTransposeMatrixdARB glLoadTransposeMatrixdARB;
  GdkGLProc_glMultTransposeMatrixfARB glMultTransposeMatrixfARB;
  GdkGLProc_glMultTransposeMatrixdARB glMultTransposeMatrixdARB;
};

GdkGL_GL_ARB_transpose_matrix *gdk_gl_get_GL_ARB_transpose_matrix (void);

/*
 * GL_ARB_multisample
 */

/* glSampleCoverageARB */
typedef void (APIENTRY * GdkGLProc_glSampleCoverageARB) (GLclampf value, GLboolean invert);
GdkGLProc    gdk_gl_get_glSampleCoverageARB (void);
#define      gdk_gl_glSampleCoverageARB(proc, value, invert) \
  ( ((GdkGLProc_glSampleCoverageARB) (proc)) (value, invert) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_multisample GdkGL_GL_ARB_multisample;

struct _GdkGL_GL_ARB_multisample
{
  GdkGLProc_glSampleCoverageARB glSampleCoverageARB;
};

GdkGL_GL_ARB_multisample *gdk_gl_get_GL_ARB_multisample (void);

/*
 * GL_ARB_texture_compression
 */

/* glCompressedTexImage3DARB */
typedef void (APIENTRY * GdkGLProc_glCompressedTexImage3DARB) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexImage3DARB (void);
#define      gdk_gl_glCompressedTexImage3DARB(proc, target, level, internalformat, width, height, depth, border, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexImage3DARB) (proc)) (target, level, internalformat, width, height, depth, border, imageSize, data) )

/* glCompressedTexImage2DARB */
typedef void (APIENTRY * GdkGLProc_glCompressedTexImage2DARB) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexImage2DARB (void);
#define      gdk_gl_glCompressedTexImage2DARB(proc, target, level, internalformat, width, height, border, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexImage2DARB) (proc)) (target, level, internalformat, width, height, border, imageSize, data) )

/* glCompressedTexImage1DARB */
typedef void (APIENTRY * GdkGLProc_glCompressedTexImage1DARB) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexImage1DARB (void);
#define      gdk_gl_glCompressedTexImage1DARB(proc, target, level, internalformat, width, border, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexImage1DARB) (proc)) (target, level, internalformat, width, border, imageSize, data) )

/* glCompressedTexSubImage3DARB */
typedef void (APIENTRY * GdkGLProc_glCompressedTexSubImage3DARB) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexSubImage3DARB (void);
#define      gdk_gl_glCompressedTexSubImage3DARB(proc, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexSubImage3DARB) (proc)) (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data) )

/* glCompressedTexSubImage2DARB */
typedef void (APIENTRY * GdkGLProc_glCompressedTexSubImage2DARB) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexSubImage2DARB (void);
#define      gdk_gl_glCompressedTexSubImage2DARB(proc, target, level, xoffset, yoffset, width, height, format, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexSubImage2DARB) (proc)) (target, level, xoffset, yoffset, width, height, format, imageSize, data) )

/* glCompressedTexSubImage1DARB */
typedef void (APIENTRY * GdkGLProc_glCompressedTexSubImage1DARB) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
GdkGLProc    gdk_gl_get_glCompressedTexSubImage1DARB (void);
#define      gdk_gl_glCompressedTexSubImage1DARB(proc, target, level, xoffset, width, format, imageSize, data) \
  ( ((GdkGLProc_glCompressedTexSubImage1DARB) (proc)) (target, level, xoffset, width, format, imageSize, data) )

/* glGetCompressedTexImageARB */
typedef void (APIENTRY * GdkGLProc_glGetCompressedTexImageARB) (GLenum target, GLint level, GLvoid *img);
GdkGLProc    gdk_gl_get_glGetCompressedTexImageARB (void);
#define      gdk_gl_glGetCompressedTexImageARB(proc, target, level, img) \
  ( ((GdkGLProc_glGetCompressedTexImageARB) (proc)) (target, level, img) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_texture_compression GdkGL_GL_ARB_texture_compression;

struct _GdkGL_GL_ARB_texture_compression
{
  GdkGLProc_glCompressedTexImage3DARB glCompressedTexImage3DARB;
  GdkGLProc_glCompressedTexImage2DARB glCompressedTexImage2DARB;
  GdkGLProc_glCompressedTexImage1DARB glCompressedTexImage1DARB;
  GdkGLProc_glCompressedTexSubImage3DARB glCompressedTexSubImage3DARB;
  GdkGLProc_glCompressedTexSubImage2DARB glCompressedTexSubImage2DARB;
  GdkGLProc_glCompressedTexSubImage1DARB glCompressedTexSubImage1DARB;
  GdkGLProc_glGetCompressedTexImageARB glGetCompressedTexImageARB;
};

GdkGL_GL_ARB_texture_compression *gdk_gl_get_GL_ARB_texture_compression (void);

/*
 * GL_ARB_point_parameters
 */

/* glPointParameterfARB */
typedef void (APIENTRY * GdkGLProc_glPointParameterfARB) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPointParameterfARB (void);
#define      gdk_gl_glPointParameterfARB(proc, pname, param) \
  ( ((GdkGLProc_glPointParameterfARB) (proc)) (pname, param) )

/* glPointParameterfvARB */
typedef void (APIENTRY * GdkGLProc_glPointParameterfvARB) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glPointParameterfvARB (void);
#define      gdk_gl_glPointParameterfvARB(proc, pname, params) \
  ( ((GdkGLProc_glPointParameterfvARB) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_point_parameters GdkGL_GL_ARB_point_parameters;

struct _GdkGL_GL_ARB_point_parameters
{
  GdkGLProc_glPointParameterfARB glPointParameterfARB;
  GdkGLProc_glPointParameterfvARB glPointParameterfvARB;
};

GdkGL_GL_ARB_point_parameters *gdk_gl_get_GL_ARB_point_parameters (void);

/*
 * GL_ARB_vertex_blend
 */

/* glWeightbvARB */
typedef void (APIENTRY * GdkGLProc_glWeightbvARB) (GLint size, const GLbyte *weights);
GdkGLProc    gdk_gl_get_glWeightbvARB (void);
#define      gdk_gl_glWeightbvARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightbvARB) (proc)) (size, weights) )

/* glWeightsvARB */
typedef void (APIENTRY * GdkGLProc_glWeightsvARB) (GLint size, const GLshort *weights);
GdkGLProc    gdk_gl_get_glWeightsvARB (void);
#define      gdk_gl_glWeightsvARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightsvARB) (proc)) (size, weights) )

/* glWeightivARB */
typedef void (APIENTRY * GdkGLProc_glWeightivARB) (GLint size, const GLint *weights);
GdkGLProc    gdk_gl_get_glWeightivARB (void);
#define      gdk_gl_glWeightivARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightivARB) (proc)) (size, weights) )

/* glWeightfvARB */
typedef void (APIENTRY * GdkGLProc_glWeightfvARB) (GLint size, const GLfloat *weights);
GdkGLProc    gdk_gl_get_glWeightfvARB (void);
#define      gdk_gl_glWeightfvARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightfvARB) (proc)) (size, weights) )

/* glWeightdvARB */
typedef void (APIENTRY * GdkGLProc_glWeightdvARB) (GLint size, const GLdouble *weights);
GdkGLProc    gdk_gl_get_glWeightdvARB (void);
#define      gdk_gl_glWeightdvARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightdvARB) (proc)) (size, weights) )

/* glWeightubvARB */
typedef void (APIENTRY * GdkGLProc_glWeightubvARB) (GLint size, const GLubyte *weights);
GdkGLProc    gdk_gl_get_glWeightubvARB (void);
#define      gdk_gl_glWeightubvARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightubvARB) (proc)) (size, weights) )

/* glWeightusvARB */
typedef void (APIENTRY * GdkGLProc_glWeightusvARB) (GLint size, const GLushort *weights);
GdkGLProc    gdk_gl_get_glWeightusvARB (void);
#define      gdk_gl_glWeightusvARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightusvARB) (proc)) (size, weights) )

/* glWeightuivARB */
typedef void (APIENTRY * GdkGLProc_glWeightuivARB) (GLint size, const GLuint *weights);
GdkGLProc    gdk_gl_get_glWeightuivARB (void);
#define      gdk_gl_glWeightuivARB(proc, size, weights) \
  ( ((GdkGLProc_glWeightuivARB) (proc)) (size, weights) )

/* glWeightPointerARB */
typedef void (APIENTRY * GdkGLProc_glWeightPointerARB) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glWeightPointerARB (void);
#define      gdk_gl_glWeightPointerARB(proc, size, type, stride, pointer) \
  ( ((GdkGLProc_glWeightPointerARB) (proc)) (size, type, stride, pointer) )

/* glVertexBlendARB */
typedef void (APIENTRY * GdkGLProc_glVertexBlendARB) (GLint count);
GdkGLProc    gdk_gl_get_glVertexBlendARB (void);
#define      gdk_gl_glVertexBlendARB(proc, count) \
  ( ((GdkGLProc_glVertexBlendARB) (proc)) (count) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_vertex_blend GdkGL_GL_ARB_vertex_blend;

struct _GdkGL_GL_ARB_vertex_blend
{
  GdkGLProc_glWeightbvARB glWeightbvARB;
  GdkGLProc_glWeightsvARB glWeightsvARB;
  GdkGLProc_glWeightivARB glWeightivARB;
  GdkGLProc_glWeightfvARB glWeightfvARB;
  GdkGLProc_glWeightdvARB glWeightdvARB;
  GdkGLProc_glWeightubvARB glWeightubvARB;
  GdkGLProc_glWeightusvARB glWeightusvARB;
  GdkGLProc_glWeightuivARB glWeightuivARB;
  GdkGLProc_glWeightPointerARB glWeightPointerARB;
  GdkGLProc_glVertexBlendARB glVertexBlendARB;
};

GdkGL_GL_ARB_vertex_blend *gdk_gl_get_GL_ARB_vertex_blend (void);

/*
 * GL_ARB_matrix_palette
 */

/* glCurrentPaletteMatrixARB */
typedef void (APIENTRY * GdkGLProc_glCurrentPaletteMatrixARB) (GLint index);
GdkGLProc    gdk_gl_get_glCurrentPaletteMatrixARB (void);
#define      gdk_gl_glCurrentPaletteMatrixARB(proc, index) \
  ( ((GdkGLProc_glCurrentPaletteMatrixARB) (proc)) (index) )

/* glMatrixIndexubvARB */
typedef void (APIENTRY * GdkGLProc_glMatrixIndexubvARB) (GLint size, const GLubyte *indices);
GdkGLProc    gdk_gl_get_glMatrixIndexubvARB (void);
#define      gdk_gl_glMatrixIndexubvARB(proc, size, indices) \
  ( ((GdkGLProc_glMatrixIndexubvARB) (proc)) (size, indices) )

/* glMatrixIndexusvARB */
typedef void (APIENTRY * GdkGLProc_glMatrixIndexusvARB) (GLint size, const GLushort *indices);
GdkGLProc    gdk_gl_get_glMatrixIndexusvARB (void);
#define      gdk_gl_glMatrixIndexusvARB(proc, size, indices) \
  ( ((GdkGLProc_glMatrixIndexusvARB) (proc)) (size, indices) )

/* glMatrixIndexuivARB */
typedef void (APIENTRY * GdkGLProc_glMatrixIndexuivARB) (GLint size, const GLuint *indices);
GdkGLProc    gdk_gl_get_glMatrixIndexuivARB (void);
#define      gdk_gl_glMatrixIndexuivARB(proc, size, indices) \
  ( ((GdkGLProc_glMatrixIndexuivARB) (proc)) (size, indices) )

/* glMatrixIndexPointerARB */
typedef void (APIENTRY * GdkGLProc_glMatrixIndexPointerARB) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glMatrixIndexPointerARB (void);
#define      gdk_gl_glMatrixIndexPointerARB(proc, size, type, stride, pointer) \
  ( ((GdkGLProc_glMatrixIndexPointerARB) (proc)) (size, type, stride, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_matrix_palette GdkGL_GL_ARB_matrix_palette;

struct _GdkGL_GL_ARB_matrix_palette
{
  GdkGLProc_glCurrentPaletteMatrixARB glCurrentPaletteMatrixARB;
  GdkGLProc_glMatrixIndexubvARB glMatrixIndexubvARB;
  GdkGLProc_glMatrixIndexusvARB glMatrixIndexusvARB;
  GdkGLProc_glMatrixIndexuivARB glMatrixIndexuivARB;
  GdkGLProc_glMatrixIndexPointerARB glMatrixIndexPointerARB;
};

GdkGL_GL_ARB_matrix_palette *gdk_gl_get_GL_ARB_matrix_palette (void);

/*
 * GL_ARB_window_pos
 */

/* glWindowPos2dARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2dARB) (GLdouble x, GLdouble y);
GdkGLProc    gdk_gl_get_glWindowPos2dARB (void);
#define      gdk_gl_glWindowPos2dARB(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2dARB) (proc)) (x, y) )

/* glWindowPos2dvARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2dvARB) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos2dvARB (void);
#define      gdk_gl_glWindowPos2dvARB(proc, v) \
  ( ((GdkGLProc_glWindowPos2dvARB) (proc)) (v) )

/* glWindowPos2fARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2fARB) (GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glWindowPos2fARB (void);
#define      gdk_gl_glWindowPos2fARB(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2fARB) (proc)) (x, y) )

/* glWindowPos2fvARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2fvARB) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos2fvARB (void);
#define      gdk_gl_glWindowPos2fvARB(proc, v) \
  ( ((GdkGLProc_glWindowPos2fvARB) (proc)) (v) )

/* glWindowPos2iARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2iARB) (GLint x, GLint y);
GdkGLProc    gdk_gl_get_glWindowPos2iARB (void);
#define      gdk_gl_glWindowPos2iARB(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2iARB) (proc)) (x, y) )

/* glWindowPos2ivARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2ivARB) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos2ivARB (void);
#define      gdk_gl_glWindowPos2ivARB(proc, v) \
  ( ((GdkGLProc_glWindowPos2ivARB) (proc)) (v) )

/* glWindowPos2sARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2sARB) (GLshort x, GLshort y);
GdkGLProc    gdk_gl_get_glWindowPos2sARB (void);
#define      gdk_gl_glWindowPos2sARB(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2sARB) (proc)) (x, y) )

/* glWindowPos2svARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos2svARB) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos2svARB (void);
#define      gdk_gl_glWindowPos2svARB(proc, v) \
  ( ((GdkGLProc_glWindowPos2svARB) (proc)) (v) )

/* glWindowPos3dARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3dARB) (GLdouble x, GLdouble y, GLdouble z);
GdkGLProc    gdk_gl_get_glWindowPos3dARB (void);
#define      gdk_gl_glWindowPos3dARB(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3dARB) (proc)) (x, y, z) )

/* glWindowPos3dvARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3dvARB) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos3dvARB (void);
#define      gdk_gl_glWindowPos3dvARB(proc, v) \
  ( ((GdkGLProc_glWindowPos3dvARB) (proc)) (v) )

/* glWindowPos3fARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3fARB) (GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glWindowPos3fARB (void);
#define      gdk_gl_glWindowPos3fARB(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3fARB) (proc)) (x, y, z) )

/* glWindowPos3fvARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3fvARB) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos3fvARB (void);
#define      gdk_gl_glWindowPos3fvARB(proc, v) \
  ( ((GdkGLProc_glWindowPos3fvARB) (proc)) (v) )

/* glWindowPos3iARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3iARB) (GLint x, GLint y, GLint z);
GdkGLProc    gdk_gl_get_glWindowPos3iARB (void);
#define      gdk_gl_glWindowPos3iARB(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3iARB) (proc)) (x, y, z) )

/* glWindowPos3ivARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3ivARB) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos3ivARB (void);
#define      gdk_gl_glWindowPos3ivARB(proc, v) \
  ( ((GdkGLProc_glWindowPos3ivARB) (proc)) (v) )

/* glWindowPos3sARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3sARB) (GLshort x, GLshort y, GLshort z);
GdkGLProc    gdk_gl_get_glWindowPos3sARB (void);
#define      gdk_gl_glWindowPos3sARB(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3sARB) (proc)) (x, y, z) )

/* glWindowPos3svARB */
typedef void (APIENTRY * GdkGLProc_glWindowPos3svARB) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos3svARB (void);
#define      gdk_gl_glWindowPos3svARB(proc, v) \
  ( ((GdkGLProc_glWindowPos3svARB) (proc)) (v) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_window_pos GdkGL_GL_ARB_window_pos;

struct _GdkGL_GL_ARB_window_pos
{
  GdkGLProc_glWindowPos2dARB glWindowPos2dARB;
  GdkGLProc_glWindowPos2dvARB glWindowPos2dvARB;
  GdkGLProc_glWindowPos2fARB glWindowPos2fARB;
  GdkGLProc_glWindowPos2fvARB glWindowPos2fvARB;
  GdkGLProc_glWindowPos2iARB glWindowPos2iARB;
  GdkGLProc_glWindowPos2ivARB glWindowPos2ivARB;
  GdkGLProc_glWindowPos2sARB glWindowPos2sARB;
  GdkGLProc_glWindowPos2svARB glWindowPos2svARB;
  GdkGLProc_glWindowPos3dARB glWindowPos3dARB;
  GdkGLProc_glWindowPos3dvARB glWindowPos3dvARB;
  GdkGLProc_glWindowPos3fARB glWindowPos3fARB;
  GdkGLProc_glWindowPos3fvARB glWindowPos3fvARB;
  GdkGLProc_glWindowPos3iARB glWindowPos3iARB;
  GdkGLProc_glWindowPos3ivARB glWindowPos3ivARB;
  GdkGLProc_glWindowPos3sARB glWindowPos3sARB;
  GdkGLProc_glWindowPos3svARB glWindowPos3svARB;
};

GdkGL_GL_ARB_window_pos *gdk_gl_get_GL_ARB_window_pos (void);

/*
 * GL_ARB_vertex_program
 */

/* glVertexAttrib1dARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1dARB) (GLuint index, GLdouble x);
GdkGLProc    gdk_gl_get_glVertexAttrib1dARB (void);
#define      gdk_gl_glVertexAttrib1dARB(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1dARB) (proc)) (index, x) )

/* glVertexAttrib1dvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1dvARB) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1dvARB (void);
#define      gdk_gl_glVertexAttrib1dvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1dvARB) (proc)) (index, v) )

/* glVertexAttrib1fARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1fARB) (GLuint index, GLfloat x);
GdkGLProc    gdk_gl_get_glVertexAttrib1fARB (void);
#define      gdk_gl_glVertexAttrib1fARB(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1fARB) (proc)) (index, x) )

/* glVertexAttrib1fvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1fvARB) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1fvARB (void);
#define      gdk_gl_glVertexAttrib1fvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1fvARB) (proc)) (index, v) )

/* glVertexAttrib1sARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1sARB) (GLuint index, GLshort x);
GdkGLProc    gdk_gl_get_glVertexAttrib1sARB (void);
#define      gdk_gl_glVertexAttrib1sARB(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1sARB) (proc)) (index, x) )

/* glVertexAttrib1svARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1svARB) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1svARB (void);
#define      gdk_gl_glVertexAttrib1svARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1svARB) (proc)) (index, v) )

/* glVertexAttrib2dARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2dARB) (GLuint index, GLdouble x, GLdouble y);
GdkGLProc    gdk_gl_get_glVertexAttrib2dARB (void);
#define      gdk_gl_glVertexAttrib2dARB(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2dARB) (proc)) (index, x, y) )

/* glVertexAttrib2dvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2dvARB) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2dvARB (void);
#define      gdk_gl_glVertexAttrib2dvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2dvARB) (proc)) (index, v) )

/* glVertexAttrib2fARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2fARB) (GLuint index, GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glVertexAttrib2fARB (void);
#define      gdk_gl_glVertexAttrib2fARB(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2fARB) (proc)) (index, x, y) )

/* glVertexAttrib2fvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2fvARB) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2fvARB (void);
#define      gdk_gl_glVertexAttrib2fvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2fvARB) (proc)) (index, v) )

/* glVertexAttrib2sARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2sARB) (GLuint index, GLshort x, GLshort y);
GdkGLProc    gdk_gl_get_glVertexAttrib2sARB (void);
#define      gdk_gl_glVertexAttrib2sARB(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2sARB) (proc)) (index, x, y) )

/* glVertexAttrib2svARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2svARB) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2svARB (void);
#define      gdk_gl_glVertexAttrib2svARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2svARB) (proc)) (index, v) )

/* glVertexAttrib3dARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3dARB) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
GdkGLProc    gdk_gl_get_glVertexAttrib3dARB (void);
#define      gdk_gl_glVertexAttrib3dARB(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3dARB) (proc)) (index, x, y, z) )

/* glVertexAttrib3dvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3dvARB) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3dvARB (void);
#define      gdk_gl_glVertexAttrib3dvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3dvARB) (proc)) (index, v) )

/* glVertexAttrib3fARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3fARB) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glVertexAttrib3fARB (void);
#define      gdk_gl_glVertexAttrib3fARB(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3fARB) (proc)) (index, x, y, z) )

/* glVertexAttrib3fvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3fvARB) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3fvARB (void);
#define      gdk_gl_glVertexAttrib3fvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3fvARB) (proc)) (index, v) )

/* glVertexAttrib3sARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3sARB) (GLuint index, GLshort x, GLshort y, GLshort z);
GdkGLProc    gdk_gl_get_glVertexAttrib3sARB (void);
#define      gdk_gl_glVertexAttrib3sARB(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3sARB) (proc)) (index, x, y, z) )

/* glVertexAttrib3svARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3svARB) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3svARB (void);
#define      gdk_gl_glVertexAttrib3svARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3svARB) (proc)) (index, v) )

/* glVertexAttrib4NbvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NbvARB) (GLuint index, const GLbyte *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4NbvARB (void);
#define      gdk_gl_glVertexAttrib4NbvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4NbvARB) (proc)) (index, v) )

/* glVertexAttrib4NivARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NivARB) (GLuint index, const GLint *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4NivARB (void);
#define      gdk_gl_glVertexAttrib4NivARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4NivARB) (proc)) (index, v) )

/* glVertexAttrib4NsvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NsvARB) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4NsvARB (void);
#define      gdk_gl_glVertexAttrib4NsvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4NsvARB) (proc)) (index, v) )

/* glVertexAttrib4NubARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NubARB) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GdkGLProc    gdk_gl_get_glVertexAttrib4NubARB (void);
#define      gdk_gl_glVertexAttrib4NubARB(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4NubARB) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4NubvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NubvARB) (GLuint index, const GLubyte *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4NubvARB (void);
#define      gdk_gl_glVertexAttrib4NubvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4NubvARB) (proc)) (index, v) )

/* glVertexAttrib4NuivARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NuivARB) (GLuint index, const GLuint *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4NuivARB (void);
#define      gdk_gl_glVertexAttrib4NuivARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4NuivARB) (proc)) (index, v) )

/* glVertexAttrib4NusvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4NusvARB) (GLuint index, const GLushort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4NusvARB (void);
#define      gdk_gl_glVertexAttrib4NusvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4NusvARB) (proc)) (index, v) )

/* glVertexAttrib4bvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4bvARB) (GLuint index, const GLbyte *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4bvARB (void);
#define      gdk_gl_glVertexAttrib4bvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4bvARB) (proc)) (index, v) )

/* glVertexAttrib4dARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4dARB) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glVertexAttrib4dARB (void);
#define      gdk_gl_glVertexAttrib4dARB(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4dARB) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4dvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4dvARB) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4dvARB (void);
#define      gdk_gl_glVertexAttrib4dvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4dvARB) (proc)) (index, v) )

/* glVertexAttrib4fARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4fARB) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glVertexAttrib4fARB (void);
#define      gdk_gl_glVertexAttrib4fARB(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4fARB) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4fvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4fvARB) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4fvARB (void);
#define      gdk_gl_glVertexAttrib4fvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4fvARB) (proc)) (index, v) )

/* glVertexAttrib4ivARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4ivARB) (GLuint index, const GLint *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4ivARB (void);
#define      gdk_gl_glVertexAttrib4ivARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4ivARB) (proc)) (index, v) )

/* glVertexAttrib4sARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4sARB) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
GdkGLProc    gdk_gl_get_glVertexAttrib4sARB (void);
#define      gdk_gl_glVertexAttrib4sARB(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4sARB) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4svARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4svARB) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4svARB (void);
#define      gdk_gl_glVertexAttrib4svARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4svARB) (proc)) (index, v) )

/* glVertexAttrib4ubvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4ubvARB) (GLuint index, const GLubyte *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4ubvARB (void);
#define      gdk_gl_glVertexAttrib4ubvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4ubvARB) (proc)) (index, v) )

/* glVertexAttrib4uivARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4uivARB) (GLuint index, const GLuint *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4uivARB (void);
#define      gdk_gl_glVertexAttrib4uivARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4uivARB) (proc)) (index, v) )

/* glVertexAttrib4usvARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4usvARB) (GLuint index, const GLushort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4usvARB (void);
#define      gdk_gl_glVertexAttrib4usvARB(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4usvARB) (proc)) (index, v) )

/* glVertexAttribPointerARB */
typedef void (APIENTRY * GdkGLProc_glVertexAttribPointerARB) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glVertexAttribPointerARB (void);
#define      gdk_gl_glVertexAttribPointerARB(proc, index, size, type, normalized, stride, pointer) \
  ( ((GdkGLProc_glVertexAttribPointerARB) (proc)) (index, size, type, normalized, stride, pointer) )

/* glEnableVertexAttribArrayARB */
typedef void (APIENTRY * GdkGLProc_glEnableVertexAttribArrayARB) (GLuint index);
GdkGLProc    gdk_gl_get_glEnableVertexAttribArrayARB (void);
#define      gdk_gl_glEnableVertexAttribArrayARB(proc, index) \
  ( ((GdkGLProc_glEnableVertexAttribArrayARB) (proc)) (index) )

/* glDisableVertexAttribArrayARB */
typedef void (APIENTRY * GdkGLProc_glDisableVertexAttribArrayARB) (GLuint index);
GdkGLProc    gdk_gl_get_glDisableVertexAttribArrayARB (void);
#define      gdk_gl_glDisableVertexAttribArrayARB(proc, index) \
  ( ((GdkGLProc_glDisableVertexAttribArrayARB) (proc)) (index) )

/* glProgramStringARB */
typedef void (APIENTRY * GdkGLProc_glProgramStringARB) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
GdkGLProc    gdk_gl_get_glProgramStringARB (void);
#define      gdk_gl_glProgramStringARB(proc, target, format, len, string) \
  ( ((GdkGLProc_glProgramStringARB) (proc)) (target, format, len, string) )

/* glBindProgramARB */
typedef void (APIENTRY * GdkGLProc_glBindProgramARB) (GLenum target, GLuint program);
GdkGLProc    gdk_gl_get_glBindProgramARB (void);
#define      gdk_gl_glBindProgramARB(proc, target, program) \
  ( ((GdkGLProc_glBindProgramARB) (proc)) (target, program) )

/* glDeleteProgramsARB */
typedef void (APIENTRY * GdkGLProc_glDeleteProgramsARB) (GLsizei n, const GLuint *programs);
GdkGLProc    gdk_gl_get_glDeleteProgramsARB (void);
#define      gdk_gl_glDeleteProgramsARB(proc, n, programs) \
  ( ((GdkGLProc_glDeleteProgramsARB) (proc)) (n, programs) )

/* glGenProgramsARB */
typedef void (APIENTRY * GdkGLProc_glGenProgramsARB) (GLsizei n, GLuint *programs);
GdkGLProc    gdk_gl_get_glGenProgramsARB (void);
#define      gdk_gl_glGenProgramsARB(proc, n, programs) \
  ( ((GdkGLProc_glGenProgramsARB) (proc)) (n, programs) )

/* glProgramEnvParameter4dARB */
typedef void (APIENTRY * GdkGLProc_glProgramEnvParameter4dARB) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glProgramEnvParameter4dARB (void);
#define      gdk_gl_glProgramEnvParameter4dARB(proc, target, index, x, y, z, w) \
  ( ((GdkGLProc_glProgramEnvParameter4dARB) (proc)) (target, index, x, y, z, w) )

/* glProgramEnvParameter4dvARB */
typedef void (APIENTRY * GdkGLProc_glProgramEnvParameter4dvARB) (GLenum target, GLuint index, const GLdouble *params);
GdkGLProc    gdk_gl_get_glProgramEnvParameter4dvARB (void);
#define      gdk_gl_glProgramEnvParameter4dvARB(proc, target, index, params) \
  ( ((GdkGLProc_glProgramEnvParameter4dvARB) (proc)) (target, index, params) )

/* glProgramEnvParameter4fARB */
typedef void (APIENTRY * GdkGLProc_glProgramEnvParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glProgramEnvParameter4fARB (void);
#define      gdk_gl_glProgramEnvParameter4fARB(proc, target, index, x, y, z, w) \
  ( ((GdkGLProc_glProgramEnvParameter4fARB) (proc)) (target, index, x, y, z, w) )

/* glProgramEnvParameter4fvARB */
typedef void (APIENTRY * GdkGLProc_glProgramEnvParameter4fvARB) (GLenum target, GLuint index, const GLfloat *params);
GdkGLProc    gdk_gl_get_glProgramEnvParameter4fvARB (void);
#define      gdk_gl_glProgramEnvParameter4fvARB(proc, target, index, params) \
  ( ((GdkGLProc_glProgramEnvParameter4fvARB) (proc)) (target, index, params) )

/* glProgramLocalParameter4dARB */
typedef void (APIENTRY * GdkGLProc_glProgramLocalParameter4dARB) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glProgramLocalParameter4dARB (void);
#define      gdk_gl_glProgramLocalParameter4dARB(proc, target, index, x, y, z, w) \
  ( ((GdkGLProc_glProgramLocalParameter4dARB) (proc)) (target, index, x, y, z, w) )

/* glProgramLocalParameter4dvARB */
typedef void (APIENTRY * GdkGLProc_glProgramLocalParameter4dvARB) (GLenum target, GLuint index, const GLdouble *params);
GdkGLProc    gdk_gl_get_glProgramLocalParameter4dvARB (void);
#define      gdk_gl_glProgramLocalParameter4dvARB(proc, target, index, params) \
  ( ((GdkGLProc_glProgramLocalParameter4dvARB) (proc)) (target, index, params) )

/* glProgramLocalParameter4fARB */
typedef void (APIENTRY * GdkGLProc_glProgramLocalParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glProgramLocalParameter4fARB (void);
#define      gdk_gl_glProgramLocalParameter4fARB(proc, target, index, x, y, z, w) \
  ( ((GdkGLProc_glProgramLocalParameter4fARB) (proc)) (target, index, x, y, z, w) )

/* glProgramLocalParameter4fvARB */
typedef void (APIENTRY * GdkGLProc_glProgramLocalParameter4fvARB) (GLenum target, GLuint index, const GLfloat *params);
GdkGLProc    gdk_gl_get_glProgramLocalParameter4fvARB (void);
#define      gdk_gl_glProgramLocalParameter4fvARB(proc, target, index, params) \
  ( ((GdkGLProc_glProgramLocalParameter4fvARB) (proc)) (target, index, params) )

/* glGetProgramEnvParameterdvARB */
typedef void (APIENTRY * GdkGLProc_glGetProgramEnvParameterdvARB) (GLenum target, GLuint index, GLdouble *params);
GdkGLProc    gdk_gl_get_glGetProgramEnvParameterdvARB (void);
#define      gdk_gl_glGetProgramEnvParameterdvARB(proc, target, index, params) \
  ( ((GdkGLProc_glGetProgramEnvParameterdvARB) (proc)) (target, index, params) )

/* glGetProgramEnvParameterfvARB */
typedef void (APIENTRY * GdkGLProc_glGetProgramEnvParameterfvARB) (GLenum target, GLuint index, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetProgramEnvParameterfvARB (void);
#define      gdk_gl_glGetProgramEnvParameterfvARB(proc, target, index, params) \
  ( ((GdkGLProc_glGetProgramEnvParameterfvARB) (proc)) (target, index, params) )

/* glGetProgramLocalParameterdvARB */
typedef void (APIENTRY * GdkGLProc_glGetProgramLocalParameterdvARB) (GLenum target, GLuint index, GLdouble *params);
GdkGLProc    gdk_gl_get_glGetProgramLocalParameterdvARB (void);
#define      gdk_gl_glGetProgramLocalParameterdvARB(proc, target, index, params) \
  ( ((GdkGLProc_glGetProgramLocalParameterdvARB) (proc)) (target, index, params) )

/* glGetProgramLocalParameterfvARB */
typedef void (APIENTRY * GdkGLProc_glGetProgramLocalParameterfvARB) (GLenum target, GLuint index, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetProgramLocalParameterfvARB (void);
#define      gdk_gl_glGetProgramLocalParameterfvARB(proc, target, index, params) \
  ( ((GdkGLProc_glGetProgramLocalParameterfvARB) (proc)) (target, index, params) )

/* glGetProgramivARB */
typedef void (APIENTRY * GdkGLProc_glGetProgramivARB) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetProgramivARB (void);
#define      gdk_gl_glGetProgramivARB(proc, target, pname, params) \
  ( ((GdkGLProc_glGetProgramivARB) (proc)) (target, pname, params) )

/* glGetProgramStringARB */
typedef void (APIENTRY * GdkGLProc_glGetProgramStringARB) (GLenum target, GLenum pname, GLvoid *string);
GdkGLProc    gdk_gl_get_glGetProgramStringARB (void);
#define      gdk_gl_glGetProgramStringARB(proc, target, pname, string) \
  ( ((GdkGLProc_glGetProgramStringARB) (proc)) (target, pname, string) )

/* glGetVertexAttribdvARB */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribdvARB) (GLuint index, GLenum pname, GLdouble *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribdvARB (void);
#define      gdk_gl_glGetVertexAttribdvARB(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribdvARB) (proc)) (index, pname, params) )

/* glGetVertexAttribfvARB */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribfvARB) (GLuint index, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribfvARB (void);
#define      gdk_gl_glGetVertexAttribfvARB(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribfvARB) (proc)) (index, pname, params) )

/* glGetVertexAttribivARB */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribivARB) (GLuint index, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribivARB (void);
#define      gdk_gl_glGetVertexAttribivARB(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribivARB) (proc)) (index, pname, params) )

/* glGetVertexAttribPointervARB */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribPointervARB) (GLuint index, GLenum pname, GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glGetVertexAttribPointervARB (void);
#define      gdk_gl_glGetVertexAttribPointervARB(proc, index, pname, pointer) \
  ( ((GdkGLProc_glGetVertexAttribPointervARB) (proc)) (index, pname, pointer) )

/* glIsProgramARB */
typedef GLboolean (APIENTRY * GdkGLProc_glIsProgramARB) (GLuint program);
GdkGLProc    gdk_gl_get_glIsProgramARB (void);
#define      gdk_gl_glIsProgramARB(proc, program) \
  ( ((GdkGLProc_glIsProgramARB) (proc)) (program) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_vertex_program GdkGL_GL_ARB_vertex_program;

struct _GdkGL_GL_ARB_vertex_program
{
  GdkGLProc_glVertexAttrib1dARB glVertexAttrib1dARB;
  GdkGLProc_glVertexAttrib1dvARB glVertexAttrib1dvARB;
  GdkGLProc_glVertexAttrib1fARB glVertexAttrib1fARB;
  GdkGLProc_glVertexAttrib1fvARB glVertexAttrib1fvARB;
  GdkGLProc_glVertexAttrib1sARB glVertexAttrib1sARB;
  GdkGLProc_glVertexAttrib1svARB glVertexAttrib1svARB;
  GdkGLProc_glVertexAttrib2dARB glVertexAttrib2dARB;
  GdkGLProc_glVertexAttrib2dvARB glVertexAttrib2dvARB;
  GdkGLProc_glVertexAttrib2fARB glVertexAttrib2fARB;
  GdkGLProc_glVertexAttrib2fvARB glVertexAttrib2fvARB;
  GdkGLProc_glVertexAttrib2sARB glVertexAttrib2sARB;
  GdkGLProc_glVertexAttrib2svARB glVertexAttrib2svARB;
  GdkGLProc_glVertexAttrib3dARB glVertexAttrib3dARB;
  GdkGLProc_glVertexAttrib3dvARB glVertexAttrib3dvARB;
  GdkGLProc_glVertexAttrib3fARB glVertexAttrib3fARB;
  GdkGLProc_glVertexAttrib3fvARB glVertexAttrib3fvARB;
  GdkGLProc_glVertexAttrib3sARB glVertexAttrib3sARB;
  GdkGLProc_glVertexAttrib3svARB glVertexAttrib3svARB;
  GdkGLProc_glVertexAttrib4NbvARB glVertexAttrib4NbvARB;
  GdkGLProc_glVertexAttrib4NivARB glVertexAttrib4NivARB;
  GdkGLProc_glVertexAttrib4NsvARB glVertexAttrib4NsvARB;
  GdkGLProc_glVertexAttrib4NubARB glVertexAttrib4NubARB;
  GdkGLProc_glVertexAttrib4NubvARB glVertexAttrib4NubvARB;
  GdkGLProc_glVertexAttrib4NuivARB glVertexAttrib4NuivARB;
  GdkGLProc_glVertexAttrib4NusvARB glVertexAttrib4NusvARB;
  GdkGLProc_glVertexAttrib4bvARB glVertexAttrib4bvARB;
  GdkGLProc_glVertexAttrib4dARB glVertexAttrib4dARB;
  GdkGLProc_glVertexAttrib4dvARB glVertexAttrib4dvARB;
  GdkGLProc_glVertexAttrib4fARB glVertexAttrib4fARB;
  GdkGLProc_glVertexAttrib4fvARB glVertexAttrib4fvARB;
  GdkGLProc_glVertexAttrib4ivARB glVertexAttrib4ivARB;
  GdkGLProc_glVertexAttrib4sARB glVertexAttrib4sARB;
  GdkGLProc_glVertexAttrib4svARB glVertexAttrib4svARB;
  GdkGLProc_glVertexAttrib4ubvARB glVertexAttrib4ubvARB;
  GdkGLProc_glVertexAttrib4uivARB glVertexAttrib4uivARB;
  GdkGLProc_glVertexAttrib4usvARB glVertexAttrib4usvARB;
  GdkGLProc_glVertexAttribPointerARB glVertexAttribPointerARB;
  GdkGLProc_glEnableVertexAttribArrayARB glEnableVertexAttribArrayARB;
  GdkGLProc_glDisableVertexAttribArrayARB glDisableVertexAttribArrayARB;
  GdkGLProc_glProgramStringARB glProgramStringARB;
  GdkGLProc_glBindProgramARB glBindProgramARB;
  GdkGLProc_glDeleteProgramsARB glDeleteProgramsARB;
  GdkGLProc_glGenProgramsARB glGenProgramsARB;
  GdkGLProc_glProgramEnvParameter4dARB glProgramEnvParameter4dARB;
  GdkGLProc_glProgramEnvParameter4dvARB glProgramEnvParameter4dvARB;
  GdkGLProc_glProgramEnvParameter4fARB glProgramEnvParameter4fARB;
  GdkGLProc_glProgramEnvParameter4fvARB glProgramEnvParameter4fvARB;
  GdkGLProc_glProgramLocalParameter4dARB glProgramLocalParameter4dARB;
  GdkGLProc_glProgramLocalParameter4dvARB glProgramLocalParameter4dvARB;
  GdkGLProc_glProgramLocalParameter4fARB glProgramLocalParameter4fARB;
  GdkGLProc_glProgramLocalParameter4fvARB glProgramLocalParameter4fvARB;
  GdkGLProc_glGetProgramEnvParameterdvARB glGetProgramEnvParameterdvARB;
  GdkGLProc_glGetProgramEnvParameterfvARB glGetProgramEnvParameterfvARB;
  GdkGLProc_glGetProgramLocalParameterdvARB glGetProgramLocalParameterdvARB;
  GdkGLProc_glGetProgramLocalParameterfvARB glGetProgramLocalParameterfvARB;
  GdkGLProc_glGetProgramivARB glGetProgramivARB;
  GdkGLProc_glGetProgramStringARB glGetProgramStringARB;
  GdkGLProc_glGetVertexAttribdvARB glGetVertexAttribdvARB;
  GdkGLProc_glGetVertexAttribfvARB glGetVertexAttribfvARB;
  GdkGLProc_glGetVertexAttribivARB glGetVertexAttribivARB;
  GdkGLProc_glGetVertexAttribPointervARB glGetVertexAttribPointervARB;
  GdkGLProc_glIsProgramARB glIsProgramARB;
};

GdkGL_GL_ARB_vertex_program *gdk_gl_get_GL_ARB_vertex_program (void);

/*
 * GL_ARB_vertex_buffer_object
 */

/* glBindBufferARB */
typedef void (APIENTRY * GdkGLProc_glBindBufferARB) (GLenum target, GLuint buffer);
GdkGLProc    gdk_gl_get_glBindBufferARB (void);
#define      gdk_gl_glBindBufferARB(proc, target, buffer) \
  ( ((GdkGLProc_glBindBufferARB) (proc)) (target, buffer) )

/* glDeleteBuffersARB */
typedef void (APIENTRY * GdkGLProc_glDeleteBuffersARB) (GLsizei n, const GLuint *buffers);
GdkGLProc    gdk_gl_get_glDeleteBuffersARB (void);
#define      gdk_gl_glDeleteBuffersARB(proc, n, buffers) \
  ( ((GdkGLProc_glDeleteBuffersARB) (proc)) (n, buffers) )

/* glGenBuffersARB */
typedef void (APIENTRY * GdkGLProc_glGenBuffersARB) (GLsizei n, GLuint *buffers);
GdkGLProc    gdk_gl_get_glGenBuffersARB (void);
#define      gdk_gl_glGenBuffersARB(proc, n, buffers) \
  ( ((GdkGLProc_glGenBuffersARB) (proc)) (n, buffers) )

/* glIsBufferARB */
typedef GLboolean (APIENTRY * GdkGLProc_glIsBufferARB) (GLuint buffer);
GdkGLProc    gdk_gl_get_glIsBufferARB (void);
#define      gdk_gl_glIsBufferARB(proc, buffer) \
  ( ((GdkGLProc_glIsBufferARB) (proc)) (buffer) )

/* glBufferDataARB */
typedef void (APIENTRY * GdkGLProc_glBufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GdkGLProc    gdk_gl_get_glBufferDataARB (void);
#define      gdk_gl_glBufferDataARB(proc, target, size, data, usage) \
  ( ((GdkGLProc_glBufferDataARB) (proc)) (target, size, data, usage) )

/* glBufferSubDataARB */
typedef void (APIENTRY * GdkGLProc_glBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
GdkGLProc    gdk_gl_get_glBufferSubDataARB (void);
#define      gdk_gl_glBufferSubDataARB(proc, target, offset, size, data) \
  ( ((GdkGLProc_glBufferSubDataARB) (proc)) (target, offset, size, data) )

/* glGetBufferSubDataARB */
typedef void (APIENTRY * GdkGLProc_glGetBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
GdkGLProc    gdk_gl_get_glGetBufferSubDataARB (void);
#define      gdk_gl_glGetBufferSubDataARB(proc, target, offset, size, data) \
  ( ((GdkGLProc_glGetBufferSubDataARB) (proc)) (target, offset, size, data) )

/* glMapBufferARB */
typedef GLvoid* (APIENTRY * GdkGLProc_glMapBufferARB) (GLenum target, GLenum access);
GdkGLProc    gdk_gl_get_glMapBufferARB (void);
#define      gdk_gl_glMapBufferARB(proc, target, access) \
  ( ((GdkGLProc_glMapBufferARB) (proc)) (target, access) )

/* glUnmapBufferARB */
typedef GLboolean (APIENTRY * GdkGLProc_glUnmapBufferARB) (GLenum target);
GdkGLProc    gdk_gl_get_glUnmapBufferARB (void);
#define      gdk_gl_glUnmapBufferARB(proc, target) \
  ( ((GdkGLProc_glUnmapBufferARB) (proc)) (target) )

/* glGetBufferParameterivARB */
typedef void (APIENTRY * GdkGLProc_glGetBufferParameterivARB) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetBufferParameterivARB (void);
#define      gdk_gl_glGetBufferParameterivARB(proc, target, pname, params) \
  ( ((GdkGLProc_glGetBufferParameterivARB) (proc)) (target, pname, params) )

/* glGetBufferPointervARB */
typedef void (APIENTRY * GdkGLProc_glGetBufferPointervARB) (GLenum target, GLenum pname, GLvoid* *params);
GdkGLProc    gdk_gl_get_glGetBufferPointervARB (void);
#define      gdk_gl_glGetBufferPointervARB(proc, target, pname, params) \
  ( ((GdkGLProc_glGetBufferPointervARB) (proc)) (target, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_ARB_vertex_buffer_object GdkGL_GL_ARB_vertex_buffer_object;

struct _GdkGL_GL_ARB_vertex_buffer_object
{
  GdkGLProc_glBindBufferARB glBindBufferARB;
  GdkGLProc_glDeleteBuffersARB glDeleteBuffersARB;
  GdkGLProc_glGenBuffersARB glGenBuffersARB;
  GdkGLProc_glIsBufferARB glIsBufferARB;
  GdkGLProc_glBufferDataARB glBufferDataARB;
  GdkGLProc_glBufferSubDataARB glBufferSubDataARB;
  GdkGLProc_glGetBufferSubDataARB glGetBufferSubDataARB;
  GdkGLProc_glMapBufferARB glMapBufferARB;
  GdkGLProc_glUnmapBufferARB glUnmapBufferARB;
  GdkGLProc_glGetBufferParameterivARB glGetBufferParameterivARB;
  GdkGLProc_glGetBufferPointervARB glGetBufferPointervARB;
};

GdkGL_GL_ARB_vertex_buffer_object *gdk_gl_get_GL_ARB_vertex_buffer_object (void);

/*
 * GL_EXT_blend_color
 */

/* glBlendColorEXT */
typedef void (APIENTRY * GdkGLProc_glBlendColorEXT) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
GdkGLProc    gdk_gl_get_glBlendColorEXT (void);
#define      gdk_gl_glBlendColorEXT(proc, red, green, blue, alpha) \
  ( ((GdkGLProc_glBlendColorEXT) (proc)) (red, green, blue, alpha) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_blend_color GdkGL_GL_EXT_blend_color;

struct _GdkGL_GL_EXT_blend_color
{
  GdkGLProc_glBlendColorEXT glBlendColorEXT;
};

GdkGL_GL_EXT_blend_color *gdk_gl_get_GL_EXT_blend_color (void);

/*
 * GL_EXT_polygon_offset
 */

/* glPolygonOffsetEXT */
typedef void (APIENTRY * GdkGLProc_glPolygonOffsetEXT) (GLfloat factor, GLfloat bias);
GdkGLProc    gdk_gl_get_glPolygonOffsetEXT (void);
#define      gdk_gl_glPolygonOffsetEXT(proc, factor, bias) \
  ( ((GdkGLProc_glPolygonOffsetEXT) (proc)) (factor, bias) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_polygon_offset GdkGL_GL_EXT_polygon_offset;

struct _GdkGL_GL_EXT_polygon_offset
{
  GdkGLProc_glPolygonOffsetEXT glPolygonOffsetEXT;
};

GdkGL_GL_EXT_polygon_offset *gdk_gl_get_GL_EXT_polygon_offset (void);

/*
 * GL_EXT_texture3D
 */

/* glTexImage3DEXT */
typedef void (APIENTRY * GdkGLProc_glTexImage3DEXT) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexImage3DEXT (void);
#define      gdk_gl_glTexImage3DEXT(proc, target, level, internalformat, width, height, depth, border, format, type, pixels) \
  ( ((GdkGLProc_glTexImage3DEXT) (proc)) (target, level, internalformat, width, height, depth, border, format, type, pixels) )

/* glTexSubImage3DEXT */
typedef void (APIENTRY * GdkGLProc_glTexSubImage3DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexSubImage3DEXT (void);
#define      gdk_gl_glTexSubImage3DEXT(proc, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) \
  ( ((GdkGLProc_glTexSubImage3DEXT) (proc)) (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_texture3D GdkGL_GL_EXT_texture3D;

struct _GdkGL_GL_EXT_texture3D
{
  GdkGLProc_glTexImage3DEXT glTexImage3DEXT;
  GdkGLProc_glTexSubImage3DEXT glTexSubImage3DEXT;
};

GdkGL_GL_EXT_texture3D *gdk_gl_get_GL_EXT_texture3D (void);

/*
 * GL_SGIS_texture_filter4
 */

/* glGetTexFilterFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glGetTexFilterFuncSGIS) (GLenum target, GLenum filter, GLfloat *weights);
GdkGLProc    gdk_gl_get_glGetTexFilterFuncSGIS (void);
#define      gdk_gl_glGetTexFilterFuncSGIS(proc, target, filter, weights) \
  ( ((GdkGLProc_glGetTexFilterFuncSGIS) (proc)) (target, filter, weights) )

/* glTexFilterFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glTexFilterFuncSGIS) (GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);
GdkGLProc    gdk_gl_get_glTexFilterFuncSGIS (void);
#define      gdk_gl_glTexFilterFuncSGIS(proc, target, filter, n, weights) \
  ( ((GdkGLProc_glTexFilterFuncSGIS) (proc)) (target, filter, n, weights) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_texture_filter4 GdkGL_GL_SGIS_texture_filter4;

struct _GdkGL_GL_SGIS_texture_filter4
{
  GdkGLProc_glGetTexFilterFuncSGIS glGetTexFilterFuncSGIS;
  GdkGLProc_glTexFilterFuncSGIS glTexFilterFuncSGIS;
};

GdkGL_GL_SGIS_texture_filter4 *gdk_gl_get_GL_SGIS_texture_filter4 (void);

/*
 * GL_EXT_subtexture
 */

/* glTexSubImage1DEXT */
typedef void (APIENTRY * GdkGLProc_glTexSubImage1DEXT) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexSubImage1DEXT (void);
#define      gdk_gl_glTexSubImage1DEXT(proc, target, level, xoffset, width, format, type, pixels) \
  ( ((GdkGLProc_glTexSubImage1DEXT) (proc)) (target, level, xoffset, width, format, type, pixels) )

/* glTexSubImage2DEXT */
typedef void (APIENTRY * GdkGLProc_glTexSubImage2DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexSubImage2DEXT (void);
#define      gdk_gl_glTexSubImage2DEXT(proc, target, level, xoffset, yoffset, width, height, format, type, pixels) \
  ( ((GdkGLProc_glTexSubImage2DEXT) (proc)) (target, level, xoffset, yoffset, width, height, format, type, pixels) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_subtexture GdkGL_GL_EXT_subtexture;

struct _GdkGL_GL_EXT_subtexture
{
  GdkGLProc_glTexSubImage1DEXT glTexSubImage1DEXT;
  GdkGLProc_glTexSubImage2DEXT glTexSubImage2DEXT;
};

GdkGL_GL_EXT_subtexture *gdk_gl_get_GL_EXT_subtexture (void);

/*
 * GL_EXT_copy_texture
 */

/* glCopyTexImage1DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyTexImage1DEXT) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
GdkGLProc    gdk_gl_get_glCopyTexImage1DEXT (void);
#define      gdk_gl_glCopyTexImage1DEXT(proc, target, level, internalformat, x, y, width, border) \
  ( ((GdkGLProc_glCopyTexImage1DEXT) (proc)) (target, level, internalformat, x, y, width, border) )

/* glCopyTexImage2DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyTexImage2DEXT) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GdkGLProc    gdk_gl_get_glCopyTexImage2DEXT (void);
#define      gdk_gl_glCopyTexImage2DEXT(proc, target, level, internalformat, x, y, width, height, border) \
  ( ((GdkGLProc_glCopyTexImage2DEXT) (proc)) (target, level, internalformat, x, y, width, height, border) )

/* glCopyTexSubImage1DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyTexSubImage1DEXT) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyTexSubImage1DEXT (void);
#define      gdk_gl_glCopyTexSubImage1DEXT(proc, target, level, xoffset, x, y, width) \
  ( ((GdkGLProc_glCopyTexSubImage1DEXT) (proc)) (target, level, xoffset, x, y, width) )

/* glCopyTexSubImage2DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyTexSubImage2DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glCopyTexSubImage2DEXT (void);
#define      gdk_gl_glCopyTexSubImage2DEXT(proc, target, level, xoffset, yoffset, x, y, width, height) \
  ( ((GdkGLProc_glCopyTexSubImage2DEXT) (proc)) (target, level, xoffset, yoffset, x, y, width, height) )

/* glCopyTexSubImage3DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyTexSubImage3DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glCopyTexSubImage3DEXT (void);
#define      gdk_gl_glCopyTexSubImage3DEXT(proc, target, level, xoffset, yoffset, zoffset, x, y, width, height) \
  ( ((GdkGLProc_glCopyTexSubImage3DEXT) (proc)) (target, level, xoffset, yoffset, zoffset, x, y, width, height) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_copy_texture GdkGL_GL_EXT_copy_texture;

struct _GdkGL_GL_EXT_copy_texture
{
  GdkGLProc_glCopyTexImage1DEXT glCopyTexImage1DEXT;
  GdkGLProc_glCopyTexImage2DEXT glCopyTexImage2DEXT;
  GdkGLProc_glCopyTexSubImage1DEXT glCopyTexSubImage1DEXT;
  GdkGLProc_glCopyTexSubImage2DEXT glCopyTexSubImage2DEXT;
  GdkGLProc_glCopyTexSubImage3DEXT glCopyTexSubImage3DEXT;
};

GdkGL_GL_EXT_copy_texture *gdk_gl_get_GL_EXT_copy_texture (void);

/*
 * GL_EXT_histogram
 */

/* glGetHistogramEXT */
typedef void (APIENTRY * GdkGLProc_glGetHistogramEXT) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
GdkGLProc    gdk_gl_get_glGetHistogramEXT (void);
#define      gdk_gl_glGetHistogramEXT(proc, target, reset, format, type, values) \
  ( ((GdkGLProc_glGetHistogramEXT) (proc)) (target, reset, format, type, values) )

/* glGetHistogramParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glGetHistogramParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetHistogramParameterfvEXT (void);
#define      gdk_gl_glGetHistogramParameterfvEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetHistogramParameterfvEXT) (proc)) (target, pname, params) )

/* glGetHistogramParameterivEXT */
typedef void (APIENTRY * GdkGLProc_glGetHistogramParameterivEXT) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetHistogramParameterivEXT (void);
#define      gdk_gl_glGetHistogramParameterivEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetHistogramParameterivEXT) (proc)) (target, pname, params) )

/* glGetMinmaxEXT */
typedef void (APIENTRY * GdkGLProc_glGetMinmaxEXT) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
GdkGLProc    gdk_gl_get_glGetMinmaxEXT (void);
#define      gdk_gl_glGetMinmaxEXT(proc, target, reset, format, type, values) \
  ( ((GdkGLProc_glGetMinmaxEXT) (proc)) (target, reset, format, type, values) )

/* glGetMinmaxParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glGetMinmaxParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetMinmaxParameterfvEXT (void);
#define      gdk_gl_glGetMinmaxParameterfvEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetMinmaxParameterfvEXT) (proc)) (target, pname, params) )

/* glGetMinmaxParameterivEXT */
typedef void (APIENTRY * GdkGLProc_glGetMinmaxParameterivEXT) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetMinmaxParameterivEXT (void);
#define      gdk_gl_glGetMinmaxParameterivEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetMinmaxParameterivEXT) (proc)) (target, pname, params) )

/* glHistogramEXT */
typedef void (APIENTRY * GdkGLProc_glHistogramEXT) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
GdkGLProc    gdk_gl_get_glHistogramEXT (void);
#define      gdk_gl_glHistogramEXT(proc, target, width, internalformat, sink) \
  ( ((GdkGLProc_glHistogramEXT) (proc)) (target, width, internalformat, sink) )

/* glMinmaxEXT */
typedef void (APIENTRY * GdkGLProc_glMinmaxEXT) (GLenum target, GLenum internalformat, GLboolean sink);
GdkGLProc    gdk_gl_get_glMinmaxEXT (void);
#define      gdk_gl_glMinmaxEXT(proc, target, internalformat, sink) \
  ( ((GdkGLProc_glMinmaxEXT) (proc)) (target, internalformat, sink) )

/* glResetHistogramEXT */
typedef void (APIENTRY * GdkGLProc_glResetHistogramEXT) (GLenum target);
GdkGLProc    gdk_gl_get_glResetHistogramEXT (void);
#define      gdk_gl_glResetHistogramEXT(proc, target) \
  ( ((GdkGLProc_glResetHistogramEXT) (proc)) (target) )

/* glResetMinmaxEXT */
typedef void (APIENTRY * GdkGLProc_glResetMinmaxEXT) (GLenum target);
GdkGLProc    gdk_gl_get_glResetMinmaxEXT (void);
#define      gdk_gl_glResetMinmaxEXT(proc, target) \
  ( ((GdkGLProc_glResetMinmaxEXT) (proc)) (target) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_histogram GdkGL_GL_EXT_histogram;

struct _GdkGL_GL_EXT_histogram
{
  GdkGLProc_glGetHistogramEXT glGetHistogramEXT;
  GdkGLProc_glGetHistogramParameterfvEXT glGetHistogramParameterfvEXT;
  GdkGLProc_glGetHistogramParameterivEXT glGetHistogramParameterivEXT;
  GdkGLProc_glGetMinmaxEXT glGetMinmaxEXT;
  GdkGLProc_glGetMinmaxParameterfvEXT glGetMinmaxParameterfvEXT;
  GdkGLProc_glGetMinmaxParameterivEXT glGetMinmaxParameterivEXT;
  GdkGLProc_glHistogramEXT glHistogramEXT;
  GdkGLProc_glMinmaxEXT glMinmaxEXT;
  GdkGLProc_glResetHistogramEXT glResetHistogramEXT;
  GdkGLProc_glResetMinmaxEXT glResetMinmaxEXT;
};

GdkGL_GL_EXT_histogram *gdk_gl_get_GL_EXT_histogram (void);

/*
 * GL_EXT_convolution
 */

/* glConvolutionFilter1DEXT */
typedef void (APIENTRY * GdkGLProc_glConvolutionFilter1DEXT) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
GdkGLProc    gdk_gl_get_glConvolutionFilter1DEXT (void);
#define      gdk_gl_glConvolutionFilter1DEXT(proc, target, internalformat, width, format, type, image) \
  ( ((GdkGLProc_glConvolutionFilter1DEXT) (proc)) (target, internalformat, width, format, type, image) )

/* glConvolutionFilter2DEXT */
typedef void (APIENTRY * GdkGLProc_glConvolutionFilter2DEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
GdkGLProc    gdk_gl_get_glConvolutionFilter2DEXT (void);
#define      gdk_gl_glConvolutionFilter2DEXT(proc, target, internalformat, width, height, format, type, image) \
  ( ((GdkGLProc_glConvolutionFilter2DEXT) (proc)) (target, internalformat, width, height, format, type, image) )

/* glConvolutionParameterfEXT */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameterfEXT) (GLenum target, GLenum pname, GLfloat params);
GdkGLProc    gdk_gl_get_glConvolutionParameterfEXT (void);
#define      gdk_gl_glConvolutionParameterfEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameterfEXT) (proc)) (target, pname, params) )

/* glConvolutionParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameterfvEXT) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glConvolutionParameterfvEXT (void);
#define      gdk_gl_glConvolutionParameterfvEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameterfvEXT) (proc)) (target, pname, params) )

/* glConvolutionParameteriEXT */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameteriEXT) (GLenum target, GLenum pname, GLint params);
GdkGLProc    gdk_gl_get_glConvolutionParameteriEXT (void);
#define      gdk_gl_glConvolutionParameteriEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameteriEXT) (proc)) (target, pname, params) )

/* glConvolutionParameterivEXT */
typedef void (APIENTRY * GdkGLProc_glConvolutionParameterivEXT) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glConvolutionParameterivEXT (void);
#define      gdk_gl_glConvolutionParameterivEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glConvolutionParameterivEXT) (proc)) (target, pname, params) )

/* glCopyConvolutionFilter1DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyConvolutionFilter1DEXT) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyConvolutionFilter1DEXT (void);
#define      gdk_gl_glCopyConvolutionFilter1DEXT(proc, target, internalformat, x, y, width) \
  ( ((GdkGLProc_glCopyConvolutionFilter1DEXT) (proc)) (target, internalformat, x, y, width) )

/* glCopyConvolutionFilter2DEXT */
typedef void (APIENTRY * GdkGLProc_glCopyConvolutionFilter2DEXT) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glCopyConvolutionFilter2DEXT (void);
#define      gdk_gl_glCopyConvolutionFilter2DEXT(proc, target, internalformat, x, y, width, height) \
  ( ((GdkGLProc_glCopyConvolutionFilter2DEXT) (proc)) (target, internalformat, x, y, width, height) )

/* glGetConvolutionFilterEXT */
typedef void (APIENTRY * GdkGLProc_glGetConvolutionFilterEXT) (GLenum target, GLenum format, GLenum type, GLvoid *image);
GdkGLProc    gdk_gl_get_glGetConvolutionFilterEXT (void);
#define      gdk_gl_glGetConvolutionFilterEXT(proc, target, format, type, image) \
  ( ((GdkGLProc_glGetConvolutionFilterEXT) (proc)) (target, format, type, image) )

/* glGetConvolutionParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glGetConvolutionParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetConvolutionParameterfvEXT (void);
#define      gdk_gl_glGetConvolutionParameterfvEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetConvolutionParameterfvEXT) (proc)) (target, pname, params) )

/* glGetConvolutionParameterivEXT */
typedef void (APIENTRY * GdkGLProc_glGetConvolutionParameterivEXT) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetConvolutionParameterivEXT (void);
#define      gdk_gl_glGetConvolutionParameterivEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetConvolutionParameterivEXT) (proc)) (target, pname, params) )

/* glGetSeparableFilterEXT */
typedef void (APIENTRY * GdkGLProc_glGetSeparableFilterEXT) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
GdkGLProc    gdk_gl_get_glGetSeparableFilterEXT (void);
#define      gdk_gl_glGetSeparableFilterEXT(proc, target, format, type, row, column, span) \
  ( ((GdkGLProc_glGetSeparableFilterEXT) (proc)) (target, format, type, row, column, span) )

/* glSeparableFilter2DEXT */
typedef void (APIENTRY * GdkGLProc_glSeparableFilter2DEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
GdkGLProc    gdk_gl_get_glSeparableFilter2DEXT (void);
#define      gdk_gl_glSeparableFilter2DEXT(proc, target, internalformat, width, height, format, type, row, column) \
  ( ((GdkGLProc_glSeparableFilter2DEXT) (proc)) (target, internalformat, width, height, format, type, row, column) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_convolution GdkGL_GL_EXT_convolution;

struct _GdkGL_GL_EXT_convolution
{
  GdkGLProc_glConvolutionFilter1DEXT glConvolutionFilter1DEXT;
  GdkGLProc_glConvolutionFilter2DEXT glConvolutionFilter2DEXT;
  GdkGLProc_glConvolutionParameterfEXT glConvolutionParameterfEXT;
  GdkGLProc_glConvolutionParameterfvEXT glConvolutionParameterfvEXT;
  GdkGLProc_glConvolutionParameteriEXT glConvolutionParameteriEXT;
  GdkGLProc_glConvolutionParameterivEXT glConvolutionParameterivEXT;
  GdkGLProc_glCopyConvolutionFilter1DEXT glCopyConvolutionFilter1DEXT;
  GdkGLProc_glCopyConvolutionFilter2DEXT glCopyConvolutionFilter2DEXT;
  GdkGLProc_glGetConvolutionFilterEXT glGetConvolutionFilterEXT;
  GdkGLProc_glGetConvolutionParameterfvEXT glGetConvolutionParameterfvEXT;
  GdkGLProc_glGetConvolutionParameterivEXT glGetConvolutionParameterivEXT;
  GdkGLProc_glGetSeparableFilterEXT glGetSeparableFilterEXT;
  GdkGLProc_glSeparableFilter2DEXT glSeparableFilter2DEXT;
};

GdkGL_GL_EXT_convolution *gdk_gl_get_GL_EXT_convolution (void);

/*
 * GL_SGI_color_table
 */

/* glColorTableSGI */
typedef void (APIENTRY * GdkGLProc_glColorTableSGI) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
GdkGLProc    gdk_gl_get_glColorTableSGI (void);
#define      gdk_gl_glColorTableSGI(proc, target, internalformat, width, format, type, table) \
  ( ((GdkGLProc_glColorTableSGI) (proc)) (target, internalformat, width, format, type, table) )

/* glColorTableParameterfvSGI */
typedef void (APIENTRY * GdkGLProc_glColorTableParameterfvSGI) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glColorTableParameterfvSGI (void);
#define      gdk_gl_glColorTableParameterfvSGI(proc, target, pname, params) \
  ( ((GdkGLProc_glColorTableParameterfvSGI) (proc)) (target, pname, params) )

/* glColorTableParameterivSGI */
typedef void (APIENTRY * GdkGLProc_glColorTableParameterivSGI) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glColorTableParameterivSGI (void);
#define      gdk_gl_glColorTableParameterivSGI(proc, target, pname, params) \
  ( ((GdkGLProc_glColorTableParameterivSGI) (proc)) (target, pname, params) )

/* glCopyColorTableSGI */
typedef void (APIENTRY * GdkGLProc_glCopyColorTableSGI) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyColorTableSGI (void);
#define      gdk_gl_glCopyColorTableSGI(proc, target, internalformat, x, y, width) \
  ( ((GdkGLProc_glCopyColorTableSGI) (proc)) (target, internalformat, x, y, width) )

/* glGetColorTableSGI */
typedef void (APIENTRY * GdkGLProc_glGetColorTableSGI) (GLenum target, GLenum format, GLenum type, GLvoid *table);
GdkGLProc    gdk_gl_get_glGetColorTableSGI (void);
#define      gdk_gl_glGetColorTableSGI(proc, target, format, type, table) \
  ( ((GdkGLProc_glGetColorTableSGI) (proc)) (target, format, type, table) )

/* glGetColorTableParameterfvSGI */
typedef void (APIENTRY * GdkGLProc_glGetColorTableParameterfvSGI) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetColorTableParameterfvSGI (void);
#define      gdk_gl_glGetColorTableParameterfvSGI(proc, target, pname, params) \
  ( ((GdkGLProc_glGetColorTableParameterfvSGI) (proc)) (target, pname, params) )

/* glGetColorTableParameterivSGI */
typedef void (APIENTRY * GdkGLProc_glGetColorTableParameterivSGI) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetColorTableParameterivSGI (void);
#define      gdk_gl_glGetColorTableParameterivSGI(proc, target, pname, params) \
  ( ((GdkGLProc_glGetColorTableParameterivSGI) (proc)) (target, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_SGI_color_table GdkGL_GL_SGI_color_table;

struct _GdkGL_GL_SGI_color_table
{
  GdkGLProc_glColorTableSGI glColorTableSGI;
  GdkGLProc_glColorTableParameterfvSGI glColorTableParameterfvSGI;
  GdkGLProc_glColorTableParameterivSGI glColorTableParameterivSGI;
  GdkGLProc_glCopyColorTableSGI glCopyColorTableSGI;
  GdkGLProc_glGetColorTableSGI glGetColorTableSGI;
  GdkGLProc_glGetColorTableParameterfvSGI glGetColorTableParameterfvSGI;
  GdkGLProc_glGetColorTableParameterivSGI glGetColorTableParameterivSGI;
};

GdkGL_GL_SGI_color_table *gdk_gl_get_GL_SGI_color_table (void);

/*
 * GL_SGIX_pixel_texture
 */

/* glPixelTexGenSGIX */
typedef void (APIENTRY * GdkGLProc_glPixelTexGenSGIX) (GLenum mode);
GdkGLProc    gdk_gl_get_glPixelTexGenSGIX (void);
#define      gdk_gl_glPixelTexGenSGIX(proc, mode) \
  ( ((GdkGLProc_glPixelTexGenSGIX) (proc)) (mode) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_pixel_texture GdkGL_GL_SGIX_pixel_texture;

struct _GdkGL_GL_SGIX_pixel_texture
{
  GdkGLProc_glPixelTexGenSGIX glPixelTexGenSGIX;
};

GdkGL_GL_SGIX_pixel_texture *gdk_gl_get_GL_SGIX_pixel_texture (void);

/*
 * GL_SGIS_pixel_texture
 */

/* glPixelTexGenParameteriSGIS */
typedef void (APIENTRY * GdkGLProc_glPixelTexGenParameteriSGIS) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glPixelTexGenParameteriSGIS (void);
#define      gdk_gl_glPixelTexGenParameteriSGIS(proc, pname, param) \
  ( ((GdkGLProc_glPixelTexGenParameteriSGIS) (proc)) (pname, param) )

/* glPixelTexGenParameterivSGIS */
typedef void (APIENTRY * GdkGLProc_glPixelTexGenParameterivSGIS) (GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glPixelTexGenParameterivSGIS (void);
#define      gdk_gl_glPixelTexGenParameterivSGIS(proc, pname, params) \
  ( ((GdkGLProc_glPixelTexGenParameterivSGIS) (proc)) (pname, params) )

/* glPixelTexGenParameterfSGIS */
typedef void (APIENTRY * GdkGLProc_glPixelTexGenParameterfSGIS) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPixelTexGenParameterfSGIS (void);
#define      gdk_gl_glPixelTexGenParameterfSGIS(proc, pname, param) \
  ( ((GdkGLProc_glPixelTexGenParameterfSGIS) (proc)) (pname, param) )

/* glPixelTexGenParameterfvSGIS */
typedef void (APIENTRY * GdkGLProc_glPixelTexGenParameterfvSGIS) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glPixelTexGenParameterfvSGIS (void);
#define      gdk_gl_glPixelTexGenParameterfvSGIS(proc, pname, params) \
  ( ((GdkGLProc_glPixelTexGenParameterfvSGIS) (proc)) (pname, params) )

/* glGetPixelTexGenParameterivSGIS */
typedef void (APIENTRY * GdkGLProc_glGetPixelTexGenParameterivSGIS) (GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetPixelTexGenParameterivSGIS (void);
#define      gdk_gl_glGetPixelTexGenParameterivSGIS(proc, pname, params) \
  ( ((GdkGLProc_glGetPixelTexGenParameterivSGIS) (proc)) (pname, params) )

/* glGetPixelTexGenParameterfvSGIS */
typedef void (APIENTRY * GdkGLProc_glGetPixelTexGenParameterfvSGIS) (GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetPixelTexGenParameterfvSGIS (void);
#define      gdk_gl_glGetPixelTexGenParameterfvSGIS(proc, pname, params) \
  ( ((GdkGLProc_glGetPixelTexGenParameterfvSGIS) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_pixel_texture GdkGL_GL_SGIS_pixel_texture;

struct _GdkGL_GL_SGIS_pixel_texture
{
  GdkGLProc_glPixelTexGenParameteriSGIS glPixelTexGenParameteriSGIS;
  GdkGLProc_glPixelTexGenParameterivSGIS glPixelTexGenParameterivSGIS;
  GdkGLProc_glPixelTexGenParameterfSGIS glPixelTexGenParameterfSGIS;
  GdkGLProc_glPixelTexGenParameterfvSGIS glPixelTexGenParameterfvSGIS;
  GdkGLProc_glGetPixelTexGenParameterivSGIS glGetPixelTexGenParameterivSGIS;
  GdkGLProc_glGetPixelTexGenParameterfvSGIS glGetPixelTexGenParameterfvSGIS;
};

GdkGL_GL_SGIS_pixel_texture *gdk_gl_get_GL_SGIS_pixel_texture (void);

/*
 * GL_SGIS_texture4D
 */

/* glTexImage4DSGIS */
typedef void (APIENTRY * GdkGLProc_glTexImage4DSGIS) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexImage4DSGIS (void);
#define      gdk_gl_glTexImage4DSGIS(proc, target, level, internalformat, width, height, depth, size4d, border, format, type, pixels) \
  ( ((GdkGLProc_glTexImage4DSGIS) (proc)) (target, level, internalformat, width, height, depth, size4d, border, format, type, pixels) )

/* glTexSubImage4DSGIS */
typedef void (APIENTRY * GdkGLProc_glTexSubImage4DSGIS) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels);
GdkGLProc    gdk_gl_get_glTexSubImage4DSGIS (void);
#define      gdk_gl_glTexSubImage4DSGIS(proc, target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels) \
  ( ((GdkGLProc_glTexSubImage4DSGIS) (proc)) (target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_texture4D GdkGL_GL_SGIS_texture4D;

struct _GdkGL_GL_SGIS_texture4D
{
  GdkGLProc_glTexImage4DSGIS glTexImage4DSGIS;
  GdkGLProc_glTexSubImage4DSGIS glTexSubImage4DSGIS;
};

GdkGL_GL_SGIS_texture4D *gdk_gl_get_GL_SGIS_texture4D (void);

/*
 * GL_EXT_texture_object
 */

/* glAreTexturesResidentEXT */
typedef GLboolean (APIENTRY * GdkGLProc_glAreTexturesResidentEXT) (GLsizei n, const GLuint *textures, GLboolean *residences);
GdkGLProc    gdk_gl_get_glAreTexturesResidentEXT (void);
#define      gdk_gl_glAreTexturesResidentEXT(proc, n, textures, residences) \
  ( ((GdkGLProc_glAreTexturesResidentEXT) (proc)) (n, textures, residences) )

/* glBindTextureEXT */
typedef void (APIENTRY * GdkGLProc_glBindTextureEXT) (GLenum target, GLuint texture);
GdkGLProc    gdk_gl_get_glBindTextureEXT (void);
#define      gdk_gl_glBindTextureEXT(proc, target, texture) \
  ( ((GdkGLProc_glBindTextureEXT) (proc)) (target, texture) )

/* glDeleteTexturesEXT */
typedef void (APIENTRY * GdkGLProc_glDeleteTexturesEXT) (GLsizei n, const GLuint *textures);
GdkGLProc    gdk_gl_get_glDeleteTexturesEXT (void);
#define      gdk_gl_glDeleteTexturesEXT(proc, n, textures) \
  ( ((GdkGLProc_glDeleteTexturesEXT) (proc)) (n, textures) )

/* glGenTexturesEXT */
typedef void (APIENTRY * GdkGLProc_glGenTexturesEXT) (GLsizei n, GLuint *textures);
GdkGLProc    gdk_gl_get_glGenTexturesEXT (void);
#define      gdk_gl_glGenTexturesEXT(proc, n, textures) \
  ( ((GdkGLProc_glGenTexturesEXT) (proc)) (n, textures) )

/* glIsTextureEXT */
typedef GLboolean (APIENTRY * GdkGLProc_glIsTextureEXT) (GLuint texture);
GdkGLProc    gdk_gl_get_glIsTextureEXT (void);
#define      gdk_gl_glIsTextureEXT(proc, texture) \
  ( ((GdkGLProc_glIsTextureEXT) (proc)) (texture) )

/* glPrioritizeTexturesEXT */
typedef void (APIENTRY * GdkGLProc_glPrioritizeTexturesEXT) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
GdkGLProc    gdk_gl_get_glPrioritizeTexturesEXT (void);
#define      gdk_gl_glPrioritizeTexturesEXT(proc, n, textures, priorities) \
  ( ((GdkGLProc_glPrioritizeTexturesEXT) (proc)) (n, textures, priorities) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_texture_object GdkGL_GL_EXT_texture_object;

struct _GdkGL_GL_EXT_texture_object
{
  GdkGLProc_glAreTexturesResidentEXT glAreTexturesResidentEXT;
  GdkGLProc_glBindTextureEXT glBindTextureEXT;
  GdkGLProc_glDeleteTexturesEXT glDeleteTexturesEXT;
  GdkGLProc_glGenTexturesEXT glGenTexturesEXT;
  GdkGLProc_glIsTextureEXT glIsTextureEXT;
  GdkGLProc_glPrioritizeTexturesEXT glPrioritizeTexturesEXT;
};

GdkGL_GL_EXT_texture_object *gdk_gl_get_GL_EXT_texture_object (void);

/*
 * GL_SGIS_detail_texture
 */

/* glDetailTexFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glDetailTexFuncSGIS) (GLenum target, GLsizei n, const GLfloat *points);
GdkGLProc    gdk_gl_get_glDetailTexFuncSGIS (void);
#define      gdk_gl_glDetailTexFuncSGIS(proc, target, n, points) \
  ( ((GdkGLProc_glDetailTexFuncSGIS) (proc)) (target, n, points) )

/* glGetDetailTexFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glGetDetailTexFuncSGIS) (GLenum target, GLfloat *points);
GdkGLProc    gdk_gl_get_glGetDetailTexFuncSGIS (void);
#define      gdk_gl_glGetDetailTexFuncSGIS(proc, target, points) \
  ( ((GdkGLProc_glGetDetailTexFuncSGIS) (proc)) (target, points) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_detail_texture GdkGL_GL_SGIS_detail_texture;

struct _GdkGL_GL_SGIS_detail_texture
{
  GdkGLProc_glDetailTexFuncSGIS glDetailTexFuncSGIS;
  GdkGLProc_glGetDetailTexFuncSGIS glGetDetailTexFuncSGIS;
};

GdkGL_GL_SGIS_detail_texture *gdk_gl_get_GL_SGIS_detail_texture (void);

/*
 * GL_SGIS_sharpen_texture
 */

/* glSharpenTexFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glSharpenTexFuncSGIS) (GLenum target, GLsizei n, const GLfloat *points);
GdkGLProc    gdk_gl_get_glSharpenTexFuncSGIS (void);
#define      gdk_gl_glSharpenTexFuncSGIS(proc, target, n, points) \
  ( ((GdkGLProc_glSharpenTexFuncSGIS) (proc)) (target, n, points) )

/* glGetSharpenTexFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glGetSharpenTexFuncSGIS) (GLenum target, GLfloat *points);
GdkGLProc    gdk_gl_get_glGetSharpenTexFuncSGIS (void);
#define      gdk_gl_glGetSharpenTexFuncSGIS(proc, target, points) \
  ( ((GdkGLProc_glGetSharpenTexFuncSGIS) (proc)) (target, points) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_sharpen_texture GdkGL_GL_SGIS_sharpen_texture;

struct _GdkGL_GL_SGIS_sharpen_texture
{
  GdkGLProc_glSharpenTexFuncSGIS glSharpenTexFuncSGIS;
  GdkGLProc_glGetSharpenTexFuncSGIS glGetSharpenTexFuncSGIS;
};

GdkGL_GL_SGIS_sharpen_texture *gdk_gl_get_GL_SGIS_sharpen_texture (void);

/*
 * GL_SGIS_multisample
 */

/* glSampleMaskSGIS */
typedef void (APIENTRY * GdkGLProc_glSampleMaskSGIS) (GLclampf value, GLboolean invert);
GdkGLProc    gdk_gl_get_glSampleMaskSGIS (void);
#define      gdk_gl_glSampleMaskSGIS(proc, value, invert) \
  ( ((GdkGLProc_glSampleMaskSGIS) (proc)) (value, invert) )

/* glSamplePatternSGIS */
typedef void (APIENTRY * GdkGLProc_glSamplePatternSGIS) (GLenum pattern);
GdkGLProc    gdk_gl_get_glSamplePatternSGIS (void);
#define      gdk_gl_glSamplePatternSGIS(proc, pattern) \
  ( ((GdkGLProc_glSamplePatternSGIS) (proc)) (pattern) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_multisample GdkGL_GL_SGIS_multisample;

struct _GdkGL_GL_SGIS_multisample
{
  GdkGLProc_glSampleMaskSGIS glSampleMaskSGIS;
  GdkGLProc_glSamplePatternSGIS glSamplePatternSGIS;
};

GdkGL_GL_SGIS_multisample *gdk_gl_get_GL_SGIS_multisample (void);

/*
 * GL_EXT_vertex_array
 */

/* glArrayElementEXT */
typedef void (APIENTRY * GdkGLProc_glArrayElementEXT) (GLint i);
GdkGLProc    gdk_gl_get_glArrayElementEXT (void);
#define      gdk_gl_glArrayElementEXT(proc, i) \
  ( ((GdkGLProc_glArrayElementEXT) (proc)) (i) )

/* glColorPointerEXT */
typedef void (APIENTRY * GdkGLProc_glColorPointerEXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glColorPointerEXT (void);
#define      gdk_gl_glColorPointerEXT(proc, size, type, stride, count, pointer) \
  ( ((GdkGLProc_glColorPointerEXT) (proc)) (size, type, stride, count, pointer) )

/* glDrawArraysEXT */
typedef void (APIENTRY * GdkGLProc_glDrawArraysEXT) (GLenum mode, GLint first, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawArraysEXT (void);
#define      gdk_gl_glDrawArraysEXT(proc, mode, first, count) \
  ( ((GdkGLProc_glDrawArraysEXT) (proc)) (mode, first, count) )

/* glEdgeFlagPointerEXT */
typedef void (APIENTRY * GdkGLProc_glEdgeFlagPointerEXT) (GLsizei stride, GLsizei count, const GLboolean *pointer);
GdkGLProc    gdk_gl_get_glEdgeFlagPointerEXT (void);
#define      gdk_gl_glEdgeFlagPointerEXT(proc, stride, count, pointer) \
  ( ((GdkGLProc_glEdgeFlagPointerEXT) (proc)) (stride, count, pointer) )

/* glGetPointervEXT */
typedef void (APIENTRY * GdkGLProc_glGetPointervEXT) (GLenum pname, GLvoid* *params);
GdkGLProc    gdk_gl_get_glGetPointervEXT (void);
#define      gdk_gl_glGetPointervEXT(proc, pname, params) \
  ( ((GdkGLProc_glGetPointervEXT) (proc)) (pname, params) )

/* glIndexPointerEXT */
typedef void (APIENTRY * GdkGLProc_glIndexPointerEXT) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glIndexPointerEXT (void);
#define      gdk_gl_glIndexPointerEXT(proc, type, stride, count, pointer) \
  ( ((GdkGLProc_glIndexPointerEXT) (proc)) (type, stride, count, pointer) )

/* glNormalPointerEXT */
typedef void (APIENTRY * GdkGLProc_glNormalPointerEXT) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glNormalPointerEXT (void);
#define      gdk_gl_glNormalPointerEXT(proc, type, stride, count, pointer) \
  ( ((GdkGLProc_glNormalPointerEXT) (proc)) (type, stride, count, pointer) )

/* glTexCoordPointerEXT */
typedef void (APIENTRY * GdkGLProc_glTexCoordPointerEXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glTexCoordPointerEXT (void);
#define      gdk_gl_glTexCoordPointerEXT(proc, size, type, stride, count, pointer) \
  ( ((GdkGLProc_glTexCoordPointerEXT) (proc)) (size, type, stride, count, pointer) )

/* glVertexPointerEXT */
typedef void (APIENTRY * GdkGLProc_glVertexPointerEXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glVertexPointerEXT (void);
#define      gdk_gl_glVertexPointerEXT(proc, size, type, stride, count, pointer) \
  ( ((GdkGLProc_glVertexPointerEXT) (proc)) (size, type, stride, count, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_vertex_array GdkGL_GL_EXT_vertex_array;

struct _GdkGL_GL_EXT_vertex_array
{
  GdkGLProc_glArrayElementEXT glArrayElementEXT;
  GdkGLProc_glColorPointerEXT glColorPointerEXT;
  GdkGLProc_glDrawArraysEXT glDrawArraysEXT;
  GdkGLProc_glEdgeFlagPointerEXT glEdgeFlagPointerEXT;
  GdkGLProc_glGetPointervEXT glGetPointervEXT;
  GdkGLProc_glIndexPointerEXT glIndexPointerEXT;
  GdkGLProc_glNormalPointerEXT glNormalPointerEXT;
  GdkGLProc_glTexCoordPointerEXT glTexCoordPointerEXT;
  GdkGLProc_glVertexPointerEXT glVertexPointerEXT;
};

GdkGL_GL_EXT_vertex_array *gdk_gl_get_GL_EXT_vertex_array (void);

/*
 * GL_EXT_blend_minmax
 */

/* glBlendEquationEXT */
typedef void (APIENTRY * GdkGLProc_glBlendEquationEXT) (GLenum mode);
GdkGLProc    gdk_gl_get_glBlendEquationEXT (void);
#define      gdk_gl_glBlendEquationEXT(proc, mode) \
  ( ((GdkGLProc_glBlendEquationEXT) (proc)) (mode) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_blend_minmax GdkGL_GL_EXT_blend_minmax;

struct _GdkGL_GL_EXT_blend_minmax
{
  GdkGLProc_glBlendEquationEXT glBlendEquationEXT;
};

GdkGL_GL_EXT_blend_minmax *gdk_gl_get_GL_EXT_blend_minmax (void);

/*
 * GL_SGIX_sprite
 */

/* glSpriteParameterfSGIX */
typedef void (APIENTRY * GdkGLProc_glSpriteParameterfSGIX) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glSpriteParameterfSGIX (void);
#define      gdk_gl_glSpriteParameterfSGIX(proc, pname, param) \
  ( ((GdkGLProc_glSpriteParameterfSGIX) (proc)) (pname, param) )

/* glSpriteParameterfvSGIX */
typedef void (APIENTRY * GdkGLProc_glSpriteParameterfvSGIX) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glSpriteParameterfvSGIX (void);
#define      gdk_gl_glSpriteParameterfvSGIX(proc, pname, params) \
  ( ((GdkGLProc_glSpriteParameterfvSGIX) (proc)) (pname, params) )

/* glSpriteParameteriSGIX */
typedef void (APIENTRY * GdkGLProc_glSpriteParameteriSGIX) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glSpriteParameteriSGIX (void);
#define      gdk_gl_glSpriteParameteriSGIX(proc, pname, param) \
  ( ((GdkGLProc_glSpriteParameteriSGIX) (proc)) (pname, param) )

/* glSpriteParameterivSGIX */
typedef void (APIENTRY * GdkGLProc_glSpriteParameterivSGIX) (GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glSpriteParameterivSGIX (void);
#define      gdk_gl_glSpriteParameterivSGIX(proc, pname, params) \
  ( ((GdkGLProc_glSpriteParameterivSGIX) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_sprite GdkGL_GL_SGIX_sprite;

struct _GdkGL_GL_SGIX_sprite
{
  GdkGLProc_glSpriteParameterfSGIX glSpriteParameterfSGIX;
  GdkGLProc_glSpriteParameterfvSGIX glSpriteParameterfvSGIX;
  GdkGLProc_glSpriteParameteriSGIX glSpriteParameteriSGIX;
  GdkGLProc_glSpriteParameterivSGIX glSpriteParameterivSGIX;
};

GdkGL_GL_SGIX_sprite *gdk_gl_get_GL_SGIX_sprite (void);

/*
 * GL_EXT_point_parameters
 */

/* glPointParameterfEXT */
typedef void (APIENTRY * GdkGLProc_glPointParameterfEXT) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPointParameterfEXT (void);
#define      gdk_gl_glPointParameterfEXT(proc, pname, param) \
  ( ((GdkGLProc_glPointParameterfEXT) (proc)) (pname, param) )

/* glPointParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glPointParameterfvEXT) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glPointParameterfvEXT (void);
#define      gdk_gl_glPointParameterfvEXT(proc, pname, params) \
  ( ((GdkGLProc_glPointParameterfvEXT) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_point_parameters GdkGL_GL_EXT_point_parameters;

struct _GdkGL_GL_EXT_point_parameters
{
  GdkGLProc_glPointParameterfEXT glPointParameterfEXT;
  GdkGLProc_glPointParameterfvEXT glPointParameterfvEXT;
};

GdkGL_GL_EXT_point_parameters *gdk_gl_get_GL_EXT_point_parameters (void);

/*
 * GL_SGIS_point_parameters
 */

/* glPointParameterfSGIS */
typedef void (APIENTRY * GdkGLProc_glPointParameterfSGIS) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPointParameterfSGIS (void);
#define      gdk_gl_glPointParameterfSGIS(proc, pname, param) \
  ( ((GdkGLProc_glPointParameterfSGIS) (proc)) (pname, param) )

/* glPointParameterfvSGIS */
typedef void (APIENTRY * GdkGLProc_glPointParameterfvSGIS) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glPointParameterfvSGIS (void);
#define      gdk_gl_glPointParameterfvSGIS(proc, pname, params) \
  ( ((GdkGLProc_glPointParameterfvSGIS) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_point_parameters GdkGL_GL_SGIS_point_parameters;

struct _GdkGL_GL_SGIS_point_parameters
{
  GdkGLProc_glPointParameterfSGIS glPointParameterfSGIS;
  GdkGLProc_glPointParameterfvSGIS glPointParameterfvSGIS;
};

GdkGL_GL_SGIS_point_parameters *gdk_gl_get_GL_SGIS_point_parameters (void);

/*
 * GL_SGIX_instruments
 */

/* glGetInstrumentsSGIX */
typedef GLint (APIENTRY * GdkGLProc_glGetInstrumentsSGIX) (void);
GdkGLProc    gdk_gl_get_glGetInstrumentsSGIX (void);
#define      gdk_gl_glGetInstrumentsSGIX(proc) \
  ( ((GdkGLProc_glGetInstrumentsSGIX) (proc)) () )

/* glInstrumentsBufferSGIX */
typedef void (APIENTRY * GdkGLProc_glInstrumentsBufferSGIX) (GLsizei size, GLint *buffer);
GdkGLProc    gdk_gl_get_glInstrumentsBufferSGIX (void);
#define      gdk_gl_glInstrumentsBufferSGIX(proc, size, buffer) \
  ( ((GdkGLProc_glInstrumentsBufferSGIX) (proc)) (size, buffer) )

/* glPollInstrumentsSGIX */
typedef GLint (APIENTRY * GdkGLProc_glPollInstrumentsSGIX) (GLint *marker_p);
GdkGLProc    gdk_gl_get_glPollInstrumentsSGIX (void);
#define      gdk_gl_glPollInstrumentsSGIX(proc, marker_p) \
  ( ((GdkGLProc_glPollInstrumentsSGIX) (proc)) (marker_p) )

/* glReadInstrumentsSGIX */
typedef void (APIENTRY * GdkGLProc_glReadInstrumentsSGIX) (GLint marker);
GdkGLProc    gdk_gl_get_glReadInstrumentsSGIX (void);
#define      gdk_gl_glReadInstrumentsSGIX(proc, marker) \
  ( ((GdkGLProc_glReadInstrumentsSGIX) (proc)) (marker) )

/* glStartInstrumentsSGIX */
typedef void (APIENTRY * GdkGLProc_glStartInstrumentsSGIX) (void);
GdkGLProc    gdk_gl_get_glStartInstrumentsSGIX (void);
#define      gdk_gl_glStartInstrumentsSGIX(proc) \
  ( ((GdkGLProc_glStartInstrumentsSGIX) (proc)) () )

/* glStopInstrumentsSGIX */
typedef void (APIENTRY * GdkGLProc_glStopInstrumentsSGIX) (GLint marker);
GdkGLProc    gdk_gl_get_glStopInstrumentsSGIX (void);
#define      gdk_gl_glStopInstrumentsSGIX(proc, marker) \
  ( ((GdkGLProc_glStopInstrumentsSGIX) (proc)) (marker) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_instruments GdkGL_GL_SGIX_instruments;

struct _GdkGL_GL_SGIX_instruments
{
  GdkGLProc_glGetInstrumentsSGIX glGetInstrumentsSGIX;
  GdkGLProc_glInstrumentsBufferSGIX glInstrumentsBufferSGIX;
  GdkGLProc_glPollInstrumentsSGIX glPollInstrumentsSGIX;
  GdkGLProc_glReadInstrumentsSGIX glReadInstrumentsSGIX;
  GdkGLProc_glStartInstrumentsSGIX glStartInstrumentsSGIX;
  GdkGLProc_glStopInstrumentsSGIX glStopInstrumentsSGIX;
};

GdkGL_GL_SGIX_instruments *gdk_gl_get_GL_SGIX_instruments (void);

/*
 * GL_SGIX_framezoom
 */

/* glFrameZoomSGIX */
typedef void (APIENTRY * GdkGLProc_glFrameZoomSGIX) (GLint factor);
GdkGLProc    gdk_gl_get_glFrameZoomSGIX (void);
#define      gdk_gl_glFrameZoomSGIX(proc, factor) \
  ( ((GdkGLProc_glFrameZoomSGIX) (proc)) (factor) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_framezoom GdkGL_GL_SGIX_framezoom;

struct _GdkGL_GL_SGIX_framezoom
{
  GdkGLProc_glFrameZoomSGIX glFrameZoomSGIX;
};

GdkGL_GL_SGIX_framezoom *gdk_gl_get_GL_SGIX_framezoom (void);

/*
 * GL_SGIX_tag_sample_buffer
 */

/* glTagSampleBufferSGIX */
typedef void (APIENTRY * GdkGLProc_glTagSampleBufferSGIX) (void);
GdkGLProc    gdk_gl_get_glTagSampleBufferSGIX (void);
#define      gdk_gl_glTagSampleBufferSGIX(proc) \
  ( ((GdkGLProc_glTagSampleBufferSGIX) (proc)) () )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_tag_sample_buffer GdkGL_GL_SGIX_tag_sample_buffer;

struct _GdkGL_GL_SGIX_tag_sample_buffer
{
  GdkGLProc_glTagSampleBufferSGIX glTagSampleBufferSGIX;
};

GdkGL_GL_SGIX_tag_sample_buffer *gdk_gl_get_GL_SGIX_tag_sample_buffer (void);

/*
 * GL_SGIX_polynomial_ffd
 */

/* glDeformationMap3dSGIX */
typedef void (APIENTRY * GdkGLProc_glDeformationMap3dSGIX) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points);
GdkGLProc    gdk_gl_get_glDeformationMap3dSGIX (void);
#define      gdk_gl_glDeformationMap3dSGIX(proc, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
  ( ((GdkGLProc_glDeformationMap3dSGIX) (proc)) (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) )

/* glDeformationMap3fSGIX */
typedef void (APIENTRY * GdkGLProc_glDeformationMap3fSGIX) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points);
GdkGLProc    gdk_gl_get_glDeformationMap3fSGIX (void);
#define      gdk_gl_glDeformationMap3fSGIX(proc, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) \
  ( ((GdkGLProc_glDeformationMap3fSGIX) (proc)) (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, w1, w2, wstride, worder, points) )

/* glDeformSGIX */
typedef void (APIENTRY * GdkGLProc_glDeformSGIX) (GLbitfield mask);
GdkGLProc    gdk_gl_get_glDeformSGIX (void);
#define      gdk_gl_glDeformSGIX(proc, mask) \
  ( ((GdkGLProc_glDeformSGIX) (proc)) (mask) )

/* glLoadIdentityDeformationMapSGIX */
typedef void (APIENTRY * GdkGLProc_glLoadIdentityDeformationMapSGIX) (GLbitfield mask);
GdkGLProc    gdk_gl_get_glLoadIdentityDeformationMapSGIX (void);
#define      gdk_gl_glLoadIdentityDeformationMapSGIX(proc, mask) \
  ( ((GdkGLProc_glLoadIdentityDeformationMapSGIX) (proc)) (mask) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_polynomial_ffd GdkGL_GL_SGIX_polynomial_ffd;

struct _GdkGL_GL_SGIX_polynomial_ffd
{
  GdkGLProc_glDeformationMap3dSGIX glDeformationMap3dSGIX;
  GdkGLProc_glDeformationMap3fSGIX glDeformationMap3fSGIX;
  GdkGLProc_glDeformSGIX glDeformSGIX;
  GdkGLProc_glLoadIdentityDeformationMapSGIX glLoadIdentityDeformationMapSGIX;
};

GdkGL_GL_SGIX_polynomial_ffd *gdk_gl_get_GL_SGIX_polynomial_ffd (void);

/*
 * GL_SGIX_reference_plane
 */

/* glReferencePlaneSGIX */
typedef void (APIENTRY * GdkGLProc_glReferencePlaneSGIX) (const GLdouble *equation);
GdkGLProc    gdk_gl_get_glReferencePlaneSGIX (void);
#define      gdk_gl_glReferencePlaneSGIX(proc, equation) \
  ( ((GdkGLProc_glReferencePlaneSGIX) (proc)) (equation) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_reference_plane GdkGL_GL_SGIX_reference_plane;

struct _GdkGL_GL_SGIX_reference_plane
{
  GdkGLProc_glReferencePlaneSGIX glReferencePlaneSGIX;
};

GdkGL_GL_SGIX_reference_plane *gdk_gl_get_GL_SGIX_reference_plane (void);

/*
 * GL_SGIX_flush_raster
 */

/* glFlushRasterSGIX */
typedef void (APIENTRY * GdkGLProc_glFlushRasterSGIX) (void);
GdkGLProc    gdk_gl_get_glFlushRasterSGIX (void);
#define      gdk_gl_glFlushRasterSGIX(proc) \
  ( ((GdkGLProc_glFlushRasterSGIX) (proc)) () )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_flush_raster GdkGL_GL_SGIX_flush_raster;

struct _GdkGL_GL_SGIX_flush_raster
{
  GdkGLProc_glFlushRasterSGIX glFlushRasterSGIX;
};

GdkGL_GL_SGIX_flush_raster *gdk_gl_get_GL_SGIX_flush_raster (void);

/*
 * GL_SGIS_fog_function
 */

/* glFogFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glFogFuncSGIS) (GLsizei n, const GLfloat *points);
GdkGLProc    gdk_gl_get_glFogFuncSGIS (void);
#define      gdk_gl_glFogFuncSGIS(proc, n, points) \
  ( ((GdkGLProc_glFogFuncSGIS) (proc)) (n, points) )

/* glGetFogFuncSGIS */
typedef void (APIENTRY * GdkGLProc_glGetFogFuncSGIS) (GLfloat *points);
GdkGLProc    gdk_gl_get_glGetFogFuncSGIS (void);
#define      gdk_gl_glGetFogFuncSGIS(proc, points) \
  ( ((GdkGLProc_glGetFogFuncSGIS) (proc)) (points) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_fog_function GdkGL_GL_SGIS_fog_function;

struct _GdkGL_GL_SGIS_fog_function
{
  GdkGLProc_glFogFuncSGIS glFogFuncSGIS;
  GdkGLProc_glGetFogFuncSGIS glGetFogFuncSGIS;
};

GdkGL_GL_SGIS_fog_function *gdk_gl_get_GL_SGIS_fog_function (void);

/*
 * GL_HP_image_transform
 */

/* glImageTransformParameteriHP */
typedef void (APIENTRY * GdkGLProc_glImageTransformParameteriHP) (GLenum target, GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glImageTransformParameteriHP (void);
#define      gdk_gl_glImageTransformParameteriHP(proc, target, pname, param) \
  ( ((GdkGLProc_glImageTransformParameteriHP) (proc)) (target, pname, param) )

/* glImageTransformParameterfHP */
typedef void (APIENTRY * GdkGLProc_glImageTransformParameterfHP) (GLenum target, GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glImageTransformParameterfHP (void);
#define      gdk_gl_glImageTransformParameterfHP(proc, target, pname, param) \
  ( ((GdkGLProc_glImageTransformParameterfHP) (proc)) (target, pname, param) )

/* glImageTransformParameterivHP */
typedef void (APIENTRY * GdkGLProc_glImageTransformParameterivHP) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glImageTransformParameterivHP (void);
#define      gdk_gl_glImageTransformParameterivHP(proc, target, pname, params) \
  ( ((GdkGLProc_glImageTransformParameterivHP) (proc)) (target, pname, params) )

/* glImageTransformParameterfvHP */
typedef void (APIENTRY * GdkGLProc_glImageTransformParameterfvHP) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glImageTransformParameterfvHP (void);
#define      gdk_gl_glImageTransformParameterfvHP(proc, target, pname, params) \
  ( ((GdkGLProc_glImageTransformParameterfvHP) (proc)) (target, pname, params) )

/* glGetImageTransformParameterivHP */
typedef void (APIENTRY * GdkGLProc_glGetImageTransformParameterivHP) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetImageTransformParameterivHP (void);
#define      gdk_gl_glGetImageTransformParameterivHP(proc, target, pname, params) \
  ( ((GdkGLProc_glGetImageTransformParameterivHP) (proc)) (target, pname, params) )

/* glGetImageTransformParameterfvHP */
typedef void (APIENTRY * GdkGLProc_glGetImageTransformParameterfvHP) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetImageTransformParameterfvHP (void);
#define      gdk_gl_glGetImageTransformParameterfvHP(proc, target, pname, params) \
  ( ((GdkGLProc_glGetImageTransformParameterfvHP) (proc)) (target, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_HP_image_transform GdkGL_GL_HP_image_transform;

struct _GdkGL_GL_HP_image_transform
{
  GdkGLProc_glImageTransformParameteriHP glImageTransformParameteriHP;
  GdkGLProc_glImageTransformParameterfHP glImageTransformParameterfHP;
  GdkGLProc_glImageTransformParameterivHP glImageTransformParameterivHP;
  GdkGLProc_glImageTransformParameterfvHP glImageTransformParameterfvHP;
  GdkGLProc_glGetImageTransformParameterivHP glGetImageTransformParameterivHP;
  GdkGLProc_glGetImageTransformParameterfvHP glGetImageTransformParameterfvHP;
};

GdkGL_GL_HP_image_transform *gdk_gl_get_GL_HP_image_transform (void);

/*
 * GL_EXT_color_subtable
 */

/* glColorSubTableEXT */
typedef void (APIENTRY * GdkGLProc_glColorSubTableEXT) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
GdkGLProc    gdk_gl_get_glColorSubTableEXT (void);
#define      gdk_gl_glColorSubTableEXT(proc, target, start, count, format, type, data) \
  ( ((GdkGLProc_glColorSubTableEXT) (proc)) (target, start, count, format, type, data) )

/* glCopyColorSubTableEXT */
typedef void (APIENTRY * GdkGLProc_glCopyColorSubTableEXT) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
GdkGLProc    gdk_gl_get_glCopyColorSubTableEXT (void);
#define      gdk_gl_glCopyColorSubTableEXT(proc, target, start, x, y, width) \
  ( ((GdkGLProc_glCopyColorSubTableEXT) (proc)) (target, start, x, y, width) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_color_subtable GdkGL_GL_EXT_color_subtable;

struct _GdkGL_GL_EXT_color_subtable
{
  GdkGLProc_glColorSubTableEXT glColorSubTableEXT;
  GdkGLProc_glCopyColorSubTableEXT glCopyColorSubTableEXT;
};

GdkGL_GL_EXT_color_subtable *gdk_gl_get_GL_EXT_color_subtable (void);

/*
 * GL_PGI_misc_hints
 */

/* glHintPGI */
typedef void (APIENTRY * GdkGLProc_glHintPGI) (GLenum target, GLint mode);
GdkGLProc    gdk_gl_get_glHintPGI (void);
#define      gdk_gl_glHintPGI(proc, target, mode) \
  ( ((GdkGLProc_glHintPGI) (proc)) (target, mode) )

/* proc struct */

typedef struct _GdkGL_GL_PGI_misc_hints GdkGL_GL_PGI_misc_hints;

struct _GdkGL_GL_PGI_misc_hints
{
  GdkGLProc_glHintPGI glHintPGI;
};

GdkGL_GL_PGI_misc_hints *gdk_gl_get_GL_PGI_misc_hints (void);

/*
 * GL_EXT_paletted_texture
 */

/* glColorTableEXT */
typedef void (APIENTRY * GdkGLProc_glColorTableEXT) (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
GdkGLProc    gdk_gl_get_glColorTableEXT (void);
#define      gdk_gl_glColorTableEXT(proc, target, internalFormat, width, format, type, table) \
  ( ((GdkGLProc_glColorTableEXT) (proc)) (target, internalFormat, width, format, type, table) )

/* glGetColorTableEXT */
typedef void (APIENTRY * GdkGLProc_glGetColorTableEXT) (GLenum target, GLenum format, GLenum type, GLvoid *data);
GdkGLProc    gdk_gl_get_glGetColorTableEXT (void);
#define      gdk_gl_glGetColorTableEXT(proc, target, format, type, data) \
  ( ((GdkGLProc_glGetColorTableEXT) (proc)) (target, format, type, data) )

/* glGetColorTableParameterivEXT */
typedef void (APIENTRY * GdkGLProc_glGetColorTableParameterivEXT) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetColorTableParameterivEXT (void);
#define      gdk_gl_glGetColorTableParameterivEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetColorTableParameterivEXT) (proc)) (target, pname, params) )

/* glGetColorTableParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glGetColorTableParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetColorTableParameterfvEXT (void);
#define      gdk_gl_glGetColorTableParameterfvEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glGetColorTableParameterfvEXT) (proc)) (target, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_paletted_texture GdkGL_GL_EXT_paletted_texture;

struct _GdkGL_GL_EXT_paletted_texture
{
  GdkGLProc_glColorTableEXT glColorTableEXT;
  GdkGLProc_glGetColorTableEXT glGetColorTableEXT;
  GdkGLProc_glGetColorTableParameterivEXT glGetColorTableParameterivEXT;
  GdkGLProc_glGetColorTableParameterfvEXT glGetColorTableParameterfvEXT;
};

GdkGL_GL_EXT_paletted_texture *gdk_gl_get_GL_EXT_paletted_texture (void);

/*
 * GL_SGIX_list_priority
 */

/* glGetListParameterfvSGIX */
typedef void (APIENTRY * GdkGLProc_glGetListParameterfvSGIX) (GLuint list, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetListParameterfvSGIX (void);
#define      gdk_gl_glGetListParameterfvSGIX(proc, list, pname, params) \
  ( ((GdkGLProc_glGetListParameterfvSGIX) (proc)) (list, pname, params) )

/* glGetListParameterivSGIX */
typedef void (APIENTRY * GdkGLProc_glGetListParameterivSGIX) (GLuint list, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetListParameterivSGIX (void);
#define      gdk_gl_glGetListParameterivSGIX(proc, list, pname, params) \
  ( ((GdkGLProc_glGetListParameterivSGIX) (proc)) (list, pname, params) )

/* glListParameterfSGIX */
typedef void (APIENTRY * GdkGLProc_glListParameterfSGIX) (GLuint list, GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glListParameterfSGIX (void);
#define      gdk_gl_glListParameterfSGIX(proc, list, pname, param) \
  ( ((GdkGLProc_glListParameterfSGIX) (proc)) (list, pname, param) )

/* glListParameterfvSGIX */
typedef void (APIENTRY * GdkGLProc_glListParameterfvSGIX) (GLuint list, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glListParameterfvSGIX (void);
#define      gdk_gl_glListParameterfvSGIX(proc, list, pname, params) \
  ( ((GdkGLProc_glListParameterfvSGIX) (proc)) (list, pname, params) )

/* glListParameteriSGIX */
typedef void (APIENTRY * GdkGLProc_glListParameteriSGIX) (GLuint list, GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glListParameteriSGIX (void);
#define      gdk_gl_glListParameteriSGIX(proc, list, pname, param) \
  ( ((GdkGLProc_glListParameteriSGIX) (proc)) (list, pname, param) )

/* glListParameterivSGIX */
typedef void (APIENTRY * GdkGLProc_glListParameterivSGIX) (GLuint list, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glListParameterivSGIX (void);
#define      gdk_gl_glListParameterivSGIX(proc, list, pname, params) \
  ( ((GdkGLProc_glListParameterivSGIX) (proc)) (list, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_list_priority GdkGL_GL_SGIX_list_priority;

struct _GdkGL_GL_SGIX_list_priority
{
  GdkGLProc_glGetListParameterfvSGIX glGetListParameterfvSGIX;
  GdkGLProc_glGetListParameterivSGIX glGetListParameterivSGIX;
  GdkGLProc_glListParameterfSGIX glListParameterfSGIX;
  GdkGLProc_glListParameterfvSGIX glListParameterfvSGIX;
  GdkGLProc_glListParameteriSGIX glListParameteriSGIX;
  GdkGLProc_glListParameterivSGIX glListParameterivSGIX;
};

GdkGL_GL_SGIX_list_priority *gdk_gl_get_GL_SGIX_list_priority (void);

/*
 * GL_EXT_index_material
 */

/* glIndexMaterialEXT */
typedef void (APIENTRY * GdkGLProc_glIndexMaterialEXT) (GLenum face, GLenum mode);
GdkGLProc    gdk_gl_get_glIndexMaterialEXT (void);
#define      gdk_gl_glIndexMaterialEXT(proc, face, mode) \
  ( ((GdkGLProc_glIndexMaterialEXT) (proc)) (face, mode) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_index_material GdkGL_GL_EXT_index_material;

struct _GdkGL_GL_EXT_index_material
{
  GdkGLProc_glIndexMaterialEXT glIndexMaterialEXT;
};

GdkGL_GL_EXT_index_material *gdk_gl_get_GL_EXT_index_material (void);

/*
 * GL_EXT_index_func
 */

/* glIndexFuncEXT */
typedef void (APIENTRY * GdkGLProc_glIndexFuncEXT) (GLenum func, GLclampf ref);
GdkGLProc    gdk_gl_get_glIndexFuncEXT (void);
#define      gdk_gl_glIndexFuncEXT(proc, func, ref) \
  ( ((GdkGLProc_glIndexFuncEXT) (proc)) (func, ref) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_index_func GdkGL_GL_EXT_index_func;

struct _GdkGL_GL_EXT_index_func
{
  GdkGLProc_glIndexFuncEXT glIndexFuncEXT;
};

GdkGL_GL_EXT_index_func *gdk_gl_get_GL_EXT_index_func (void);

/*
 * GL_EXT_compiled_vertex_array
 */

/* glLockArraysEXT */
typedef void (APIENTRY * GdkGLProc_glLockArraysEXT) (GLint first, GLsizei count);
GdkGLProc    gdk_gl_get_glLockArraysEXT (void);
#define      gdk_gl_glLockArraysEXT(proc, first, count) \
  ( ((GdkGLProc_glLockArraysEXT) (proc)) (first, count) )

/* glUnlockArraysEXT */
typedef void (APIENTRY * GdkGLProc_glUnlockArraysEXT) (void);
GdkGLProc    gdk_gl_get_glUnlockArraysEXT (void);
#define      gdk_gl_glUnlockArraysEXT(proc) \
  ( ((GdkGLProc_glUnlockArraysEXT) (proc)) () )

/* proc struct */

typedef struct _GdkGL_GL_EXT_compiled_vertex_array GdkGL_GL_EXT_compiled_vertex_array;

struct _GdkGL_GL_EXT_compiled_vertex_array
{
  GdkGLProc_glLockArraysEXT glLockArraysEXT;
  GdkGLProc_glUnlockArraysEXT glUnlockArraysEXT;
};

GdkGL_GL_EXT_compiled_vertex_array *gdk_gl_get_GL_EXT_compiled_vertex_array (void);

/*
 * GL_EXT_cull_vertex
 */

/* glCullParameterdvEXT */
typedef void (APIENTRY * GdkGLProc_glCullParameterdvEXT) (GLenum pname, GLdouble *params);
GdkGLProc    gdk_gl_get_glCullParameterdvEXT (void);
#define      gdk_gl_glCullParameterdvEXT(proc, pname, params) \
  ( ((GdkGLProc_glCullParameterdvEXT) (proc)) (pname, params) )

/* glCullParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glCullParameterfvEXT) (GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glCullParameterfvEXT (void);
#define      gdk_gl_glCullParameterfvEXT(proc, pname, params) \
  ( ((GdkGLProc_glCullParameterfvEXT) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_cull_vertex GdkGL_GL_EXT_cull_vertex;

struct _GdkGL_GL_EXT_cull_vertex
{
  GdkGLProc_glCullParameterdvEXT glCullParameterdvEXT;
  GdkGLProc_glCullParameterfvEXT glCullParameterfvEXT;
};

GdkGL_GL_EXT_cull_vertex *gdk_gl_get_GL_EXT_cull_vertex (void);

/*
 * GL_SGIX_fragment_lighting
 */

/* glFragmentColorMaterialSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentColorMaterialSGIX) (GLenum face, GLenum mode);
GdkGLProc    gdk_gl_get_glFragmentColorMaterialSGIX (void);
#define      gdk_gl_glFragmentColorMaterialSGIX(proc, face, mode) \
  ( ((GdkGLProc_glFragmentColorMaterialSGIX) (proc)) (face, mode) )

/* glFragmentLightfSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightfSGIX) (GLenum light, GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glFragmentLightfSGIX (void);
#define      gdk_gl_glFragmentLightfSGIX(proc, light, pname, param) \
  ( ((GdkGLProc_glFragmentLightfSGIX) (proc)) (light, pname, param) )

/* glFragmentLightfvSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightfvSGIX) (GLenum light, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glFragmentLightfvSGIX (void);
#define      gdk_gl_glFragmentLightfvSGIX(proc, light, pname, params) \
  ( ((GdkGLProc_glFragmentLightfvSGIX) (proc)) (light, pname, params) )

/* glFragmentLightiSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightiSGIX) (GLenum light, GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glFragmentLightiSGIX (void);
#define      gdk_gl_glFragmentLightiSGIX(proc, light, pname, param) \
  ( ((GdkGLProc_glFragmentLightiSGIX) (proc)) (light, pname, param) )

/* glFragmentLightivSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightivSGIX) (GLenum light, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glFragmentLightivSGIX (void);
#define      gdk_gl_glFragmentLightivSGIX(proc, light, pname, params) \
  ( ((GdkGLProc_glFragmentLightivSGIX) (proc)) (light, pname, params) )

/* glFragmentLightModelfSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModelfSGIX) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glFragmentLightModelfSGIX (void);
#define      gdk_gl_glFragmentLightModelfSGIX(proc, pname, param) \
  ( ((GdkGLProc_glFragmentLightModelfSGIX) (proc)) (pname, param) )

/* glFragmentLightModelfvSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModelfvSGIX) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glFragmentLightModelfvSGIX (void);
#define      gdk_gl_glFragmentLightModelfvSGIX(proc, pname, params) \
  ( ((GdkGLProc_glFragmentLightModelfvSGIX) (proc)) (pname, params) )

/* glFragmentLightModeliSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModeliSGIX) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glFragmentLightModeliSGIX (void);
#define      gdk_gl_glFragmentLightModeliSGIX(proc, pname, param) \
  ( ((GdkGLProc_glFragmentLightModeliSGIX) (proc)) (pname, param) )

/* glFragmentLightModelivSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModelivSGIX) (GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glFragmentLightModelivSGIX (void);
#define      gdk_gl_glFragmentLightModelivSGIX(proc, pname, params) \
  ( ((GdkGLProc_glFragmentLightModelivSGIX) (proc)) (pname, params) )

/* glFragmentMaterialfSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialfSGIX) (GLenum face, GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glFragmentMaterialfSGIX (void);
#define      gdk_gl_glFragmentMaterialfSGIX(proc, face, pname, param) \
  ( ((GdkGLProc_glFragmentMaterialfSGIX) (proc)) (face, pname, param) )

/* glFragmentMaterialfvSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialfvSGIX) (GLenum face, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glFragmentMaterialfvSGIX (void);
#define      gdk_gl_glFragmentMaterialfvSGIX(proc, face, pname, params) \
  ( ((GdkGLProc_glFragmentMaterialfvSGIX) (proc)) (face, pname, params) )

/* glFragmentMaterialiSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialiSGIX) (GLenum face, GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glFragmentMaterialiSGIX (void);
#define      gdk_gl_glFragmentMaterialiSGIX(proc, face, pname, param) \
  ( ((GdkGLProc_glFragmentMaterialiSGIX) (proc)) (face, pname, param) )

/* glFragmentMaterialivSGIX */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialivSGIX) (GLenum face, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glFragmentMaterialivSGIX (void);
#define      gdk_gl_glFragmentMaterialivSGIX(proc, face, pname, params) \
  ( ((GdkGLProc_glFragmentMaterialivSGIX) (proc)) (face, pname, params) )

/* glGetFragmentLightfvSGIX */
typedef void (APIENTRY * GdkGLProc_glGetFragmentLightfvSGIX) (GLenum light, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetFragmentLightfvSGIX (void);
#define      gdk_gl_glGetFragmentLightfvSGIX(proc, light, pname, params) \
  ( ((GdkGLProc_glGetFragmentLightfvSGIX) (proc)) (light, pname, params) )

/* glGetFragmentLightivSGIX */
typedef void (APIENTRY * GdkGLProc_glGetFragmentLightivSGIX) (GLenum light, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetFragmentLightivSGIX (void);
#define      gdk_gl_glGetFragmentLightivSGIX(proc, light, pname, params) \
  ( ((GdkGLProc_glGetFragmentLightivSGIX) (proc)) (light, pname, params) )

/* glGetFragmentMaterialfvSGIX */
typedef void (APIENTRY * GdkGLProc_glGetFragmentMaterialfvSGIX) (GLenum face, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetFragmentMaterialfvSGIX (void);
#define      gdk_gl_glGetFragmentMaterialfvSGIX(proc, face, pname, params) \
  ( ((GdkGLProc_glGetFragmentMaterialfvSGIX) (proc)) (face, pname, params) )

/* glGetFragmentMaterialivSGIX */
typedef void (APIENTRY * GdkGLProc_glGetFragmentMaterialivSGIX) (GLenum face, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetFragmentMaterialivSGIX (void);
#define      gdk_gl_glGetFragmentMaterialivSGIX(proc, face, pname, params) \
  ( ((GdkGLProc_glGetFragmentMaterialivSGIX) (proc)) (face, pname, params) )

/* glLightEnviSGIX */
typedef void (APIENTRY * GdkGLProc_glLightEnviSGIX) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glLightEnviSGIX (void);
#define      gdk_gl_glLightEnviSGIX(proc, pname, param) \
  ( ((GdkGLProc_glLightEnviSGIX) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_fragment_lighting GdkGL_GL_SGIX_fragment_lighting;

struct _GdkGL_GL_SGIX_fragment_lighting
{
  GdkGLProc_glFragmentColorMaterialSGIX glFragmentColorMaterialSGIX;
  GdkGLProc_glFragmentLightfSGIX glFragmentLightfSGIX;
  GdkGLProc_glFragmentLightfvSGIX glFragmentLightfvSGIX;
  GdkGLProc_glFragmentLightiSGIX glFragmentLightiSGIX;
  GdkGLProc_glFragmentLightivSGIX glFragmentLightivSGIX;
  GdkGLProc_glFragmentLightModelfSGIX glFragmentLightModelfSGIX;
  GdkGLProc_glFragmentLightModelfvSGIX glFragmentLightModelfvSGIX;
  GdkGLProc_glFragmentLightModeliSGIX glFragmentLightModeliSGIX;
  GdkGLProc_glFragmentLightModelivSGIX glFragmentLightModelivSGIX;
  GdkGLProc_glFragmentMaterialfSGIX glFragmentMaterialfSGIX;
  GdkGLProc_glFragmentMaterialfvSGIX glFragmentMaterialfvSGIX;
  GdkGLProc_glFragmentMaterialiSGIX glFragmentMaterialiSGIX;
  GdkGLProc_glFragmentMaterialivSGIX glFragmentMaterialivSGIX;
  GdkGLProc_glGetFragmentLightfvSGIX glGetFragmentLightfvSGIX;
  GdkGLProc_glGetFragmentLightivSGIX glGetFragmentLightivSGIX;
  GdkGLProc_glGetFragmentMaterialfvSGIX glGetFragmentMaterialfvSGIX;
  GdkGLProc_glGetFragmentMaterialivSGIX glGetFragmentMaterialivSGIX;
  GdkGLProc_glLightEnviSGIX glLightEnviSGIX;
};

GdkGL_GL_SGIX_fragment_lighting *gdk_gl_get_GL_SGIX_fragment_lighting (void);

/*
 * GL_EXT_draw_range_elements
 */

/* glDrawRangeElementsEXT */
typedef void (APIENTRY * GdkGLProc_glDrawRangeElementsEXT) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
GdkGLProc    gdk_gl_get_glDrawRangeElementsEXT (void);
#define      gdk_gl_glDrawRangeElementsEXT(proc, mode, start, end, count, type, indices) \
  ( ((GdkGLProc_glDrawRangeElementsEXT) (proc)) (mode, start, end, count, type, indices) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_draw_range_elements GdkGL_GL_EXT_draw_range_elements;

struct _GdkGL_GL_EXT_draw_range_elements
{
  GdkGLProc_glDrawRangeElementsEXT glDrawRangeElementsEXT;
};

GdkGL_GL_EXT_draw_range_elements *gdk_gl_get_GL_EXT_draw_range_elements (void);

/*
 * GL_EXT_light_texture
 */

/* glApplyTextureEXT */
typedef void (APIENTRY * GdkGLProc_glApplyTextureEXT) (GLenum mode);
GdkGLProc    gdk_gl_get_glApplyTextureEXT (void);
#define      gdk_gl_glApplyTextureEXT(proc, mode) \
  ( ((GdkGLProc_glApplyTextureEXT) (proc)) (mode) )

/* glTextureLightEXT */
typedef void (APIENTRY * GdkGLProc_glTextureLightEXT) (GLenum pname);
GdkGLProc    gdk_gl_get_glTextureLightEXT (void);
#define      gdk_gl_glTextureLightEXT(proc, pname) \
  ( ((GdkGLProc_glTextureLightEXT) (proc)) (pname) )

/* glTextureMaterialEXT */
typedef void (APIENTRY * GdkGLProc_glTextureMaterialEXT) (GLenum face, GLenum mode);
GdkGLProc    gdk_gl_get_glTextureMaterialEXT (void);
#define      gdk_gl_glTextureMaterialEXT(proc, face, mode) \
  ( ((GdkGLProc_glTextureMaterialEXT) (proc)) (face, mode) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_light_texture GdkGL_GL_EXT_light_texture;

struct _GdkGL_GL_EXT_light_texture
{
  GdkGLProc_glApplyTextureEXT glApplyTextureEXT;
  GdkGLProc_glTextureLightEXT glTextureLightEXT;
  GdkGLProc_glTextureMaterialEXT glTextureMaterialEXT;
};

GdkGL_GL_EXT_light_texture *gdk_gl_get_GL_EXT_light_texture (void);

/*
 * GL_SGIX_async
 */

/* glAsyncMarkerSGIX */
typedef void (APIENTRY * GdkGLProc_glAsyncMarkerSGIX) (GLuint marker);
GdkGLProc    gdk_gl_get_glAsyncMarkerSGIX (void);
#define      gdk_gl_glAsyncMarkerSGIX(proc, marker) \
  ( ((GdkGLProc_glAsyncMarkerSGIX) (proc)) (marker) )

/* glFinishAsyncSGIX */
typedef GLint (APIENTRY * GdkGLProc_glFinishAsyncSGIX) (GLuint *markerp);
GdkGLProc    gdk_gl_get_glFinishAsyncSGIX (void);
#define      gdk_gl_glFinishAsyncSGIX(proc, markerp) \
  ( ((GdkGLProc_glFinishAsyncSGIX) (proc)) (markerp) )

/* glPollAsyncSGIX */
typedef GLint (APIENTRY * GdkGLProc_glPollAsyncSGIX) (GLuint *markerp);
GdkGLProc    gdk_gl_get_glPollAsyncSGIX (void);
#define      gdk_gl_glPollAsyncSGIX(proc, markerp) \
  ( ((GdkGLProc_glPollAsyncSGIX) (proc)) (markerp) )

/* glGenAsyncMarkersSGIX */
typedef GLuint (APIENTRY * GdkGLProc_glGenAsyncMarkersSGIX) (GLsizei range);
GdkGLProc    gdk_gl_get_glGenAsyncMarkersSGIX (void);
#define      gdk_gl_glGenAsyncMarkersSGIX(proc, range) \
  ( ((GdkGLProc_glGenAsyncMarkersSGIX) (proc)) (range) )

/* glDeleteAsyncMarkersSGIX */
typedef void (APIENTRY * GdkGLProc_glDeleteAsyncMarkersSGIX) (GLuint marker, GLsizei range);
GdkGLProc    gdk_gl_get_glDeleteAsyncMarkersSGIX (void);
#define      gdk_gl_glDeleteAsyncMarkersSGIX(proc, marker, range) \
  ( ((GdkGLProc_glDeleteAsyncMarkersSGIX) (proc)) (marker, range) )

/* glIsAsyncMarkerSGIX */
typedef GLboolean (APIENTRY * GdkGLProc_glIsAsyncMarkerSGIX) (GLuint marker);
GdkGLProc    gdk_gl_get_glIsAsyncMarkerSGIX (void);
#define      gdk_gl_glIsAsyncMarkerSGIX(proc, marker) \
  ( ((GdkGLProc_glIsAsyncMarkerSGIX) (proc)) (marker) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_async GdkGL_GL_SGIX_async;

struct _GdkGL_GL_SGIX_async
{
  GdkGLProc_glAsyncMarkerSGIX glAsyncMarkerSGIX;
  GdkGLProc_glFinishAsyncSGIX glFinishAsyncSGIX;
  GdkGLProc_glPollAsyncSGIX glPollAsyncSGIX;
  GdkGLProc_glGenAsyncMarkersSGIX glGenAsyncMarkersSGIX;
  GdkGLProc_glDeleteAsyncMarkersSGIX glDeleteAsyncMarkersSGIX;
  GdkGLProc_glIsAsyncMarkerSGIX glIsAsyncMarkerSGIX;
};

GdkGL_GL_SGIX_async *gdk_gl_get_GL_SGIX_async (void);

/*
 * GL_INTEL_parallel_arrays
 */

/* glVertexPointervINTEL */
typedef void (APIENTRY * GdkGLProc_glVertexPointervINTEL) (GLint size, GLenum type, const GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glVertexPointervINTEL (void);
#define      gdk_gl_glVertexPointervINTEL(proc, size, type, pointer) \
  ( ((GdkGLProc_glVertexPointervINTEL) (proc)) (size, type, pointer) )

/* glNormalPointervINTEL */
typedef void (APIENTRY * GdkGLProc_glNormalPointervINTEL) (GLenum type, const GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glNormalPointervINTEL (void);
#define      gdk_gl_glNormalPointervINTEL(proc, type, pointer) \
  ( ((GdkGLProc_glNormalPointervINTEL) (proc)) (type, pointer) )

/* glColorPointervINTEL */
typedef void (APIENTRY * GdkGLProc_glColorPointervINTEL) (GLint size, GLenum type, const GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glColorPointervINTEL (void);
#define      gdk_gl_glColorPointervINTEL(proc, size, type, pointer) \
  ( ((GdkGLProc_glColorPointervINTEL) (proc)) (size, type, pointer) )

/* glTexCoordPointervINTEL */
typedef void (APIENTRY * GdkGLProc_glTexCoordPointervINTEL) (GLint size, GLenum type, const GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glTexCoordPointervINTEL (void);
#define      gdk_gl_glTexCoordPointervINTEL(proc, size, type, pointer) \
  ( ((GdkGLProc_glTexCoordPointervINTEL) (proc)) (size, type, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_INTEL_parallel_arrays GdkGL_GL_INTEL_parallel_arrays;

struct _GdkGL_GL_INTEL_parallel_arrays
{
  GdkGLProc_glVertexPointervINTEL glVertexPointervINTEL;
  GdkGLProc_glNormalPointervINTEL glNormalPointervINTEL;
  GdkGLProc_glColorPointervINTEL glColorPointervINTEL;
  GdkGLProc_glTexCoordPointervINTEL glTexCoordPointervINTEL;
};

GdkGL_GL_INTEL_parallel_arrays *gdk_gl_get_GL_INTEL_parallel_arrays (void);

/*
 * GL_EXT_pixel_transform
 */

/* glPixelTransformParameteriEXT */
typedef void (APIENTRY * GdkGLProc_glPixelTransformParameteriEXT) (GLenum target, GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glPixelTransformParameteriEXT (void);
#define      gdk_gl_glPixelTransformParameteriEXT(proc, target, pname, param) \
  ( ((GdkGLProc_glPixelTransformParameteriEXT) (proc)) (target, pname, param) )

/* glPixelTransformParameterfEXT */
typedef void (APIENTRY * GdkGLProc_glPixelTransformParameterfEXT) (GLenum target, GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPixelTransformParameterfEXT (void);
#define      gdk_gl_glPixelTransformParameterfEXT(proc, target, pname, param) \
  ( ((GdkGLProc_glPixelTransformParameterfEXT) (proc)) (target, pname, param) )

/* glPixelTransformParameterivEXT */
typedef void (APIENTRY * GdkGLProc_glPixelTransformParameterivEXT) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glPixelTransformParameterivEXT (void);
#define      gdk_gl_glPixelTransformParameterivEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glPixelTransformParameterivEXT) (proc)) (target, pname, params) )

/* glPixelTransformParameterfvEXT */
typedef void (APIENTRY * GdkGLProc_glPixelTransformParameterfvEXT) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glPixelTransformParameterfvEXT (void);
#define      gdk_gl_glPixelTransformParameterfvEXT(proc, target, pname, params) \
  ( ((GdkGLProc_glPixelTransformParameterfvEXT) (proc)) (target, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_pixel_transform GdkGL_GL_EXT_pixel_transform;

struct _GdkGL_GL_EXT_pixel_transform
{
  GdkGLProc_glPixelTransformParameteriEXT glPixelTransformParameteriEXT;
  GdkGLProc_glPixelTransformParameterfEXT glPixelTransformParameterfEXT;
  GdkGLProc_glPixelTransformParameterivEXT glPixelTransformParameterivEXT;
  GdkGLProc_glPixelTransformParameterfvEXT glPixelTransformParameterfvEXT;
};

GdkGL_GL_EXT_pixel_transform *gdk_gl_get_GL_EXT_pixel_transform (void);

/*
 * GL_EXT_secondary_color
 */

/* glSecondaryColor3bEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3bEXT) (GLbyte red, GLbyte green, GLbyte blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3bEXT (void);
#define      gdk_gl_glSecondaryColor3bEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3bEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3bvEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3bvEXT) (const GLbyte *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3bvEXT (void);
#define      gdk_gl_glSecondaryColor3bvEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3bvEXT) (proc)) (v) )

/* glSecondaryColor3dEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3dEXT) (GLdouble red, GLdouble green, GLdouble blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3dEXT (void);
#define      gdk_gl_glSecondaryColor3dEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3dEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3dvEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3dvEXT) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3dvEXT (void);
#define      gdk_gl_glSecondaryColor3dvEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3dvEXT) (proc)) (v) )

/* glSecondaryColor3fEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3fEXT) (GLfloat red, GLfloat green, GLfloat blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3fEXT (void);
#define      gdk_gl_glSecondaryColor3fEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3fEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3fvEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3fvEXT) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3fvEXT (void);
#define      gdk_gl_glSecondaryColor3fvEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3fvEXT) (proc)) (v) )

/* glSecondaryColor3iEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3iEXT) (GLint red, GLint green, GLint blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3iEXT (void);
#define      gdk_gl_glSecondaryColor3iEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3iEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3ivEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3ivEXT) (const GLint *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3ivEXT (void);
#define      gdk_gl_glSecondaryColor3ivEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3ivEXT) (proc)) (v) )

/* glSecondaryColor3sEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3sEXT) (GLshort red, GLshort green, GLshort blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3sEXT (void);
#define      gdk_gl_glSecondaryColor3sEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3sEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3svEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3svEXT) (const GLshort *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3svEXT (void);
#define      gdk_gl_glSecondaryColor3svEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3svEXT) (proc)) (v) )

/* glSecondaryColor3ubEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3ubEXT) (GLubyte red, GLubyte green, GLubyte blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3ubEXT (void);
#define      gdk_gl_glSecondaryColor3ubEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3ubEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3ubvEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3ubvEXT) (const GLubyte *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3ubvEXT (void);
#define      gdk_gl_glSecondaryColor3ubvEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3ubvEXT) (proc)) (v) )

/* glSecondaryColor3uiEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3uiEXT) (GLuint red, GLuint green, GLuint blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3uiEXT (void);
#define      gdk_gl_glSecondaryColor3uiEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3uiEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3uivEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3uivEXT) (const GLuint *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3uivEXT (void);
#define      gdk_gl_glSecondaryColor3uivEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3uivEXT) (proc)) (v) )

/* glSecondaryColor3usEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3usEXT) (GLushort red, GLushort green, GLushort blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3usEXT (void);
#define      gdk_gl_glSecondaryColor3usEXT(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3usEXT) (proc)) (red, green, blue) )

/* glSecondaryColor3usvEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3usvEXT) (const GLushort *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3usvEXT (void);
#define      gdk_gl_glSecondaryColor3usvEXT(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3usvEXT) (proc)) (v) )

/* glSecondaryColorPointerEXT */
typedef void (APIENTRY * GdkGLProc_glSecondaryColorPointerEXT) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glSecondaryColorPointerEXT (void);
#define      gdk_gl_glSecondaryColorPointerEXT(proc, size, type, stride, pointer) \
  ( ((GdkGLProc_glSecondaryColorPointerEXT) (proc)) (size, type, stride, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_secondary_color GdkGL_GL_EXT_secondary_color;

struct _GdkGL_GL_EXT_secondary_color
{
  GdkGLProc_glSecondaryColor3bEXT glSecondaryColor3bEXT;
  GdkGLProc_glSecondaryColor3bvEXT glSecondaryColor3bvEXT;
  GdkGLProc_glSecondaryColor3dEXT glSecondaryColor3dEXT;
  GdkGLProc_glSecondaryColor3dvEXT glSecondaryColor3dvEXT;
  GdkGLProc_glSecondaryColor3fEXT glSecondaryColor3fEXT;
  GdkGLProc_glSecondaryColor3fvEXT glSecondaryColor3fvEXT;
  GdkGLProc_glSecondaryColor3iEXT glSecondaryColor3iEXT;
  GdkGLProc_glSecondaryColor3ivEXT glSecondaryColor3ivEXT;
  GdkGLProc_glSecondaryColor3sEXT glSecondaryColor3sEXT;
  GdkGLProc_glSecondaryColor3svEXT glSecondaryColor3svEXT;
  GdkGLProc_glSecondaryColor3ubEXT glSecondaryColor3ubEXT;
  GdkGLProc_glSecondaryColor3ubvEXT glSecondaryColor3ubvEXT;
  GdkGLProc_glSecondaryColor3uiEXT glSecondaryColor3uiEXT;
  GdkGLProc_glSecondaryColor3uivEXT glSecondaryColor3uivEXT;
  GdkGLProc_glSecondaryColor3usEXT glSecondaryColor3usEXT;
  GdkGLProc_glSecondaryColor3usvEXT glSecondaryColor3usvEXT;
  GdkGLProc_glSecondaryColorPointerEXT glSecondaryColorPointerEXT;
};

GdkGL_GL_EXT_secondary_color *gdk_gl_get_GL_EXT_secondary_color (void);

/*
 * GL_EXT_texture_perturb_normal
 */

/* glTextureNormalEXT */
typedef void (APIENTRY * GdkGLProc_glTextureNormalEXT) (GLenum mode);
GdkGLProc    gdk_gl_get_glTextureNormalEXT (void);
#define      gdk_gl_glTextureNormalEXT(proc, mode) \
  ( ((GdkGLProc_glTextureNormalEXT) (proc)) (mode) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_texture_perturb_normal GdkGL_GL_EXT_texture_perturb_normal;

struct _GdkGL_GL_EXT_texture_perturb_normal
{
  GdkGLProc_glTextureNormalEXT glTextureNormalEXT;
};

GdkGL_GL_EXT_texture_perturb_normal *gdk_gl_get_GL_EXT_texture_perturb_normal (void);

/*
 * GL_EXT_multi_draw_arrays
 */

/* glMultiDrawArraysEXT */
typedef void (APIENTRY * GdkGLProc_glMultiDrawArraysEXT) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawArraysEXT (void);
#define      gdk_gl_glMultiDrawArraysEXT(proc, mode, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawArraysEXT) (proc)) (mode, first, count, primcount) )

/* glMultiDrawElementsEXT */
typedef void (APIENTRY * GdkGLProc_glMultiDrawElementsEXT) (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawElementsEXT (void);
#define      gdk_gl_glMultiDrawElementsEXT(proc, mode, count, type, indices, primcount) \
  ( ((GdkGLProc_glMultiDrawElementsEXT) (proc)) (mode, count, type, indices, primcount) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_multi_draw_arrays GdkGL_GL_EXT_multi_draw_arrays;

struct _GdkGL_GL_EXT_multi_draw_arrays
{
  GdkGLProc_glMultiDrawArraysEXT glMultiDrawArraysEXT;
  GdkGLProc_glMultiDrawElementsEXT glMultiDrawElementsEXT;
};

GdkGL_GL_EXT_multi_draw_arrays *gdk_gl_get_GL_EXT_multi_draw_arrays (void);

/*
 * GL_EXT_fog_coord
 */

/* glFogCoordfEXT */
typedef void (APIENTRY * GdkGLProc_glFogCoordfEXT) (GLfloat coord);
GdkGLProc    gdk_gl_get_glFogCoordfEXT (void);
#define      gdk_gl_glFogCoordfEXT(proc, coord) \
  ( ((GdkGLProc_glFogCoordfEXT) (proc)) (coord) )

/* glFogCoordfvEXT */
typedef void (APIENTRY * GdkGLProc_glFogCoordfvEXT) (const GLfloat *coord);
GdkGLProc    gdk_gl_get_glFogCoordfvEXT (void);
#define      gdk_gl_glFogCoordfvEXT(proc, coord) \
  ( ((GdkGLProc_glFogCoordfvEXT) (proc)) (coord) )

/* glFogCoorddEXT */
typedef void (APIENTRY * GdkGLProc_glFogCoorddEXT) (GLdouble coord);
GdkGLProc    gdk_gl_get_glFogCoorddEXT (void);
#define      gdk_gl_glFogCoorddEXT(proc, coord) \
  ( ((GdkGLProc_glFogCoorddEXT) (proc)) (coord) )

/* glFogCoorddvEXT */
typedef void (APIENTRY * GdkGLProc_glFogCoorddvEXT) (const GLdouble *coord);
GdkGLProc    gdk_gl_get_glFogCoorddvEXT (void);
#define      gdk_gl_glFogCoorddvEXT(proc, coord) \
  ( ((GdkGLProc_glFogCoorddvEXT) (proc)) (coord) )

/* glFogCoordPointerEXT */
typedef void (APIENTRY * GdkGLProc_glFogCoordPointerEXT) (GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glFogCoordPointerEXT (void);
#define      gdk_gl_glFogCoordPointerEXT(proc, type, stride, pointer) \
  ( ((GdkGLProc_glFogCoordPointerEXT) (proc)) (type, stride, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_fog_coord GdkGL_GL_EXT_fog_coord;

struct _GdkGL_GL_EXT_fog_coord
{
  GdkGLProc_glFogCoordfEXT glFogCoordfEXT;
  GdkGLProc_glFogCoordfvEXT glFogCoordfvEXT;
  GdkGLProc_glFogCoorddEXT glFogCoorddEXT;
  GdkGLProc_glFogCoorddvEXT glFogCoorddvEXT;
  GdkGLProc_glFogCoordPointerEXT glFogCoordPointerEXT;
};

GdkGL_GL_EXT_fog_coord *gdk_gl_get_GL_EXT_fog_coord (void);

/*
 * GL_EXT_coordinate_frame
 */

/* glTangent3bEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3bEXT) (GLbyte tx, GLbyte ty, GLbyte tz);
GdkGLProc    gdk_gl_get_glTangent3bEXT (void);
#define      gdk_gl_glTangent3bEXT(proc, tx, ty, tz) \
  ( ((GdkGLProc_glTangent3bEXT) (proc)) (tx, ty, tz) )

/* glTangent3bvEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3bvEXT) (const GLbyte *v);
GdkGLProc    gdk_gl_get_glTangent3bvEXT (void);
#define      gdk_gl_glTangent3bvEXT(proc, v) \
  ( ((GdkGLProc_glTangent3bvEXT) (proc)) (v) )

/* glTangent3dEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3dEXT) (GLdouble tx, GLdouble ty, GLdouble tz);
GdkGLProc    gdk_gl_get_glTangent3dEXT (void);
#define      gdk_gl_glTangent3dEXT(proc, tx, ty, tz) \
  ( ((GdkGLProc_glTangent3dEXT) (proc)) (tx, ty, tz) )

/* glTangent3dvEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3dvEXT) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glTangent3dvEXT (void);
#define      gdk_gl_glTangent3dvEXT(proc, v) \
  ( ((GdkGLProc_glTangent3dvEXT) (proc)) (v) )

/* glTangent3fEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3fEXT) (GLfloat tx, GLfloat ty, GLfloat tz);
GdkGLProc    gdk_gl_get_glTangent3fEXT (void);
#define      gdk_gl_glTangent3fEXT(proc, tx, ty, tz) \
  ( ((GdkGLProc_glTangent3fEXT) (proc)) (tx, ty, tz) )

/* glTangent3fvEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3fvEXT) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glTangent3fvEXT (void);
#define      gdk_gl_glTangent3fvEXT(proc, v) \
  ( ((GdkGLProc_glTangent3fvEXT) (proc)) (v) )

/* glTangent3iEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3iEXT) (GLint tx, GLint ty, GLint tz);
GdkGLProc    gdk_gl_get_glTangent3iEXT (void);
#define      gdk_gl_glTangent3iEXT(proc, tx, ty, tz) \
  ( ((GdkGLProc_glTangent3iEXT) (proc)) (tx, ty, tz) )

/* glTangent3ivEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3ivEXT) (const GLint *v);
GdkGLProc    gdk_gl_get_glTangent3ivEXT (void);
#define      gdk_gl_glTangent3ivEXT(proc, v) \
  ( ((GdkGLProc_glTangent3ivEXT) (proc)) (v) )

/* glTangent3sEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3sEXT) (GLshort tx, GLshort ty, GLshort tz);
GdkGLProc    gdk_gl_get_glTangent3sEXT (void);
#define      gdk_gl_glTangent3sEXT(proc, tx, ty, tz) \
  ( ((GdkGLProc_glTangent3sEXT) (proc)) (tx, ty, tz) )

/* glTangent3svEXT */
typedef void (APIENTRY * GdkGLProc_glTangent3svEXT) (const GLshort *v);
GdkGLProc    gdk_gl_get_glTangent3svEXT (void);
#define      gdk_gl_glTangent3svEXT(proc, v) \
  ( ((GdkGLProc_glTangent3svEXT) (proc)) (v) )

/* glBinormal3bEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3bEXT) (GLbyte bx, GLbyte by, GLbyte bz);
GdkGLProc    gdk_gl_get_glBinormal3bEXT (void);
#define      gdk_gl_glBinormal3bEXT(proc, bx, by, bz) \
  ( ((GdkGLProc_glBinormal3bEXT) (proc)) (bx, by, bz) )

/* glBinormal3bvEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3bvEXT) (const GLbyte *v);
GdkGLProc    gdk_gl_get_glBinormal3bvEXT (void);
#define      gdk_gl_glBinormal3bvEXT(proc, v) \
  ( ((GdkGLProc_glBinormal3bvEXT) (proc)) (v) )

/* glBinormal3dEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3dEXT) (GLdouble bx, GLdouble by, GLdouble bz);
GdkGLProc    gdk_gl_get_glBinormal3dEXT (void);
#define      gdk_gl_glBinormal3dEXT(proc, bx, by, bz) \
  ( ((GdkGLProc_glBinormal3dEXT) (proc)) (bx, by, bz) )

/* glBinormal3dvEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3dvEXT) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glBinormal3dvEXT (void);
#define      gdk_gl_glBinormal3dvEXT(proc, v) \
  ( ((GdkGLProc_glBinormal3dvEXT) (proc)) (v) )

/* glBinormal3fEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3fEXT) (GLfloat bx, GLfloat by, GLfloat bz);
GdkGLProc    gdk_gl_get_glBinormal3fEXT (void);
#define      gdk_gl_glBinormal3fEXT(proc, bx, by, bz) \
  ( ((GdkGLProc_glBinormal3fEXT) (proc)) (bx, by, bz) )

/* glBinormal3fvEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3fvEXT) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glBinormal3fvEXT (void);
#define      gdk_gl_glBinormal3fvEXT(proc, v) \
  ( ((GdkGLProc_glBinormal3fvEXT) (proc)) (v) )

/* glBinormal3iEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3iEXT) (GLint bx, GLint by, GLint bz);
GdkGLProc    gdk_gl_get_glBinormal3iEXT (void);
#define      gdk_gl_glBinormal3iEXT(proc, bx, by, bz) \
  ( ((GdkGLProc_glBinormal3iEXT) (proc)) (bx, by, bz) )

/* glBinormal3ivEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3ivEXT) (const GLint *v);
GdkGLProc    gdk_gl_get_glBinormal3ivEXT (void);
#define      gdk_gl_glBinormal3ivEXT(proc, v) \
  ( ((GdkGLProc_glBinormal3ivEXT) (proc)) (v) )

/* glBinormal3sEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3sEXT) (GLshort bx, GLshort by, GLshort bz);
GdkGLProc    gdk_gl_get_glBinormal3sEXT (void);
#define      gdk_gl_glBinormal3sEXT(proc, bx, by, bz) \
  ( ((GdkGLProc_glBinormal3sEXT) (proc)) (bx, by, bz) )

/* glBinormal3svEXT */
typedef void (APIENTRY * GdkGLProc_glBinormal3svEXT) (const GLshort *v);
GdkGLProc    gdk_gl_get_glBinormal3svEXT (void);
#define      gdk_gl_glBinormal3svEXT(proc, v) \
  ( ((GdkGLProc_glBinormal3svEXT) (proc)) (v) )

/* glTangentPointerEXT */
typedef void (APIENTRY * GdkGLProc_glTangentPointerEXT) (GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glTangentPointerEXT (void);
#define      gdk_gl_glTangentPointerEXT(proc, type, stride, pointer) \
  ( ((GdkGLProc_glTangentPointerEXT) (proc)) (type, stride, pointer) )

/* glBinormalPointerEXT */
typedef void (APIENTRY * GdkGLProc_glBinormalPointerEXT) (GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glBinormalPointerEXT (void);
#define      gdk_gl_glBinormalPointerEXT(proc, type, stride, pointer) \
  ( ((GdkGLProc_glBinormalPointerEXT) (proc)) (type, stride, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_coordinate_frame GdkGL_GL_EXT_coordinate_frame;

struct _GdkGL_GL_EXT_coordinate_frame
{
  GdkGLProc_glTangent3bEXT glTangent3bEXT;
  GdkGLProc_glTangent3bvEXT glTangent3bvEXT;
  GdkGLProc_glTangent3dEXT glTangent3dEXT;
  GdkGLProc_glTangent3dvEXT glTangent3dvEXT;
  GdkGLProc_glTangent3fEXT glTangent3fEXT;
  GdkGLProc_glTangent3fvEXT glTangent3fvEXT;
  GdkGLProc_glTangent3iEXT glTangent3iEXT;
  GdkGLProc_glTangent3ivEXT glTangent3ivEXT;
  GdkGLProc_glTangent3sEXT glTangent3sEXT;
  GdkGLProc_glTangent3svEXT glTangent3svEXT;
  GdkGLProc_glBinormal3bEXT glBinormal3bEXT;
  GdkGLProc_glBinormal3bvEXT glBinormal3bvEXT;
  GdkGLProc_glBinormal3dEXT glBinormal3dEXT;
  GdkGLProc_glBinormal3dvEXT glBinormal3dvEXT;
  GdkGLProc_glBinormal3fEXT glBinormal3fEXT;
  GdkGLProc_glBinormal3fvEXT glBinormal3fvEXT;
  GdkGLProc_glBinormal3iEXT glBinormal3iEXT;
  GdkGLProc_glBinormal3ivEXT glBinormal3ivEXT;
  GdkGLProc_glBinormal3sEXT glBinormal3sEXT;
  GdkGLProc_glBinormal3svEXT glBinormal3svEXT;
  GdkGLProc_glTangentPointerEXT glTangentPointerEXT;
  GdkGLProc_glBinormalPointerEXT glBinormalPointerEXT;
};

GdkGL_GL_EXT_coordinate_frame *gdk_gl_get_GL_EXT_coordinate_frame (void);

/*
 * GL_SUNX_constant_data
 */

/* glFinishTextureSUNX */
typedef void (APIENTRY * GdkGLProc_glFinishTextureSUNX) (void);
GdkGLProc    gdk_gl_get_glFinishTextureSUNX (void);
#define      gdk_gl_glFinishTextureSUNX(proc) \
  ( ((GdkGLProc_glFinishTextureSUNX) (proc)) () )

/* proc struct */

typedef struct _GdkGL_GL_SUNX_constant_data GdkGL_GL_SUNX_constant_data;

struct _GdkGL_GL_SUNX_constant_data
{
  GdkGLProc_glFinishTextureSUNX glFinishTextureSUNX;
};

GdkGL_GL_SUNX_constant_data *gdk_gl_get_GL_SUNX_constant_data (void);

/*
 * GL_SUN_global_alpha
 */

/* glGlobalAlphaFactorbSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactorbSUN) (GLbyte factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactorbSUN (void);
#define      gdk_gl_glGlobalAlphaFactorbSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactorbSUN) (proc)) (factor) )

/* glGlobalAlphaFactorsSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactorsSUN) (GLshort factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactorsSUN (void);
#define      gdk_gl_glGlobalAlphaFactorsSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactorsSUN) (proc)) (factor) )

/* glGlobalAlphaFactoriSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactoriSUN) (GLint factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactoriSUN (void);
#define      gdk_gl_glGlobalAlphaFactoriSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactoriSUN) (proc)) (factor) )

/* glGlobalAlphaFactorfSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactorfSUN) (GLfloat factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactorfSUN (void);
#define      gdk_gl_glGlobalAlphaFactorfSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactorfSUN) (proc)) (factor) )

/* glGlobalAlphaFactordSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactordSUN) (GLdouble factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactordSUN (void);
#define      gdk_gl_glGlobalAlphaFactordSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactordSUN) (proc)) (factor) )

/* glGlobalAlphaFactorubSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactorubSUN) (GLubyte factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactorubSUN (void);
#define      gdk_gl_glGlobalAlphaFactorubSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactorubSUN) (proc)) (factor) )

/* glGlobalAlphaFactorusSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactorusSUN) (GLushort factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactorusSUN (void);
#define      gdk_gl_glGlobalAlphaFactorusSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactorusSUN) (proc)) (factor) )

/* glGlobalAlphaFactoruiSUN */
typedef void (APIENTRY * GdkGLProc_glGlobalAlphaFactoruiSUN) (GLuint factor);
GdkGLProc    gdk_gl_get_glGlobalAlphaFactoruiSUN (void);
#define      gdk_gl_glGlobalAlphaFactoruiSUN(proc, factor) \
  ( ((GdkGLProc_glGlobalAlphaFactoruiSUN) (proc)) (factor) )

/* proc struct */

typedef struct _GdkGL_GL_SUN_global_alpha GdkGL_GL_SUN_global_alpha;

struct _GdkGL_GL_SUN_global_alpha
{
  GdkGLProc_glGlobalAlphaFactorbSUN glGlobalAlphaFactorbSUN;
  GdkGLProc_glGlobalAlphaFactorsSUN glGlobalAlphaFactorsSUN;
  GdkGLProc_glGlobalAlphaFactoriSUN glGlobalAlphaFactoriSUN;
  GdkGLProc_glGlobalAlphaFactorfSUN glGlobalAlphaFactorfSUN;
  GdkGLProc_glGlobalAlphaFactordSUN glGlobalAlphaFactordSUN;
  GdkGLProc_glGlobalAlphaFactorubSUN glGlobalAlphaFactorubSUN;
  GdkGLProc_glGlobalAlphaFactorusSUN glGlobalAlphaFactorusSUN;
  GdkGLProc_glGlobalAlphaFactoruiSUN glGlobalAlphaFactoruiSUN;
};

GdkGL_GL_SUN_global_alpha *gdk_gl_get_GL_SUN_global_alpha (void);

/*
 * GL_SUN_triangle_list
 */

/* glReplacementCodeuiSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiSUN) (GLuint code);
GdkGLProc    gdk_gl_get_glReplacementCodeuiSUN (void);
#define      gdk_gl_glReplacementCodeuiSUN(proc, code) \
  ( ((GdkGLProc_glReplacementCodeuiSUN) (proc)) (code) )

/* glReplacementCodeusSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeusSUN) (GLushort code);
GdkGLProc    gdk_gl_get_glReplacementCodeusSUN (void);
#define      gdk_gl_glReplacementCodeusSUN(proc, code) \
  ( ((GdkGLProc_glReplacementCodeusSUN) (proc)) (code) )

/* glReplacementCodeubSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeubSUN) (GLubyte code);
GdkGLProc    gdk_gl_get_glReplacementCodeubSUN (void);
#define      gdk_gl_glReplacementCodeubSUN(proc, code) \
  ( ((GdkGLProc_glReplacementCodeubSUN) (proc)) (code) )

/* glReplacementCodeuivSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuivSUN) (const GLuint *code);
GdkGLProc    gdk_gl_get_glReplacementCodeuivSUN (void);
#define      gdk_gl_glReplacementCodeuivSUN(proc, code) \
  ( ((GdkGLProc_glReplacementCodeuivSUN) (proc)) (code) )

/* glReplacementCodeusvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeusvSUN) (const GLushort *code);
GdkGLProc    gdk_gl_get_glReplacementCodeusvSUN (void);
#define      gdk_gl_glReplacementCodeusvSUN(proc, code) \
  ( ((GdkGLProc_glReplacementCodeusvSUN) (proc)) (code) )

/* glReplacementCodeubvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeubvSUN) (const GLubyte *code);
GdkGLProc    gdk_gl_get_glReplacementCodeubvSUN (void);
#define      gdk_gl_glReplacementCodeubvSUN(proc, code) \
  ( ((GdkGLProc_glReplacementCodeubvSUN) (proc)) (code) )

/* glReplacementCodePointerSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodePointerSUN) (GLenum type, GLsizei stride, const GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glReplacementCodePointerSUN (void);
#define      gdk_gl_glReplacementCodePointerSUN(proc, type, stride, pointer) \
  ( ((GdkGLProc_glReplacementCodePointerSUN) (proc)) (type, stride, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_SUN_triangle_list GdkGL_GL_SUN_triangle_list;

struct _GdkGL_GL_SUN_triangle_list
{
  GdkGLProc_glReplacementCodeuiSUN glReplacementCodeuiSUN;
  GdkGLProc_glReplacementCodeusSUN glReplacementCodeusSUN;
  GdkGLProc_glReplacementCodeubSUN glReplacementCodeubSUN;
  GdkGLProc_glReplacementCodeuivSUN glReplacementCodeuivSUN;
  GdkGLProc_glReplacementCodeusvSUN glReplacementCodeusvSUN;
  GdkGLProc_glReplacementCodeubvSUN glReplacementCodeubvSUN;
  GdkGLProc_glReplacementCodePointerSUN glReplacementCodePointerSUN;
};

GdkGL_GL_SUN_triangle_list *gdk_gl_get_GL_SUN_triangle_list (void);

/*
 * GL_SUN_vertex
 */

/* glColor4ubVertex2fSUN */
typedef void (APIENTRY * GdkGLProc_glColor4ubVertex2fSUN) (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glColor4ubVertex2fSUN (void);
#define      gdk_gl_glColor4ubVertex2fSUN(proc, r, g, b, a, x, y) \
  ( ((GdkGLProc_glColor4ubVertex2fSUN) (proc)) (r, g, b, a, x, y) )

/* glColor4ubVertex2fvSUN */
typedef void (APIENTRY * GdkGLProc_glColor4ubVertex2fvSUN) (const GLubyte *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glColor4ubVertex2fvSUN (void);
#define      gdk_gl_glColor4ubVertex2fvSUN(proc, c, v) \
  ( ((GdkGLProc_glColor4ubVertex2fvSUN) (proc)) (c, v) )

/* glColor4ubVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glColor4ubVertex3fSUN) (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glColor4ubVertex3fSUN (void);
#define      gdk_gl_glColor4ubVertex3fSUN(proc, r, g, b, a, x, y, z) \
  ( ((GdkGLProc_glColor4ubVertex3fSUN) (proc)) (r, g, b, a, x, y, z) )

/* glColor4ubVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glColor4ubVertex3fvSUN) (const GLubyte *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glColor4ubVertex3fvSUN (void);
#define      gdk_gl_glColor4ubVertex3fvSUN(proc, c, v) \
  ( ((GdkGLProc_glColor4ubVertex3fvSUN) (proc)) (c, v) )

/* glColor3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glColor3fVertex3fSUN) (GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glColor3fVertex3fSUN (void);
#define      gdk_gl_glColor3fVertex3fSUN(proc, r, g, b, x, y, z) \
  ( ((GdkGLProc_glColor3fVertex3fSUN) (proc)) (r, g, b, x, y, z) )

/* glColor3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glColor3fVertex3fvSUN) (const GLfloat *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glColor3fVertex3fvSUN (void);
#define      gdk_gl_glColor3fVertex3fvSUN(proc, c, v) \
  ( ((GdkGLProc_glColor3fVertex3fvSUN) (proc)) (c, v) )

/* glNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glNormal3fVertex3fSUN) (GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glNormal3fVertex3fSUN (void);
#define      gdk_gl_glNormal3fVertex3fSUN(proc, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glNormal3fVertex3fSUN) (proc)) (nx, ny, nz, x, y, z) )

/* glNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glNormal3fVertex3fvSUN) (const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glNormal3fVertex3fvSUN (void);
#define      gdk_gl_glNormal3fVertex3fvSUN(proc, n, v) \
  ( ((GdkGLProc_glNormal3fVertex3fvSUN) (proc)) (n, v) )

/* glColor4fNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glColor4fNormal3fVertex3fSUN) (GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glColor4fNormal3fVertex3fSUN (void);
#define      gdk_gl_glColor4fNormal3fVertex3fSUN(proc, r, g, b, a, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glColor4fNormal3fVertex3fSUN) (proc)) (r, g, b, a, nx, ny, nz, x, y, z) )

/* glColor4fNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glColor4fNormal3fVertex3fvSUN) (const GLfloat *c, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glColor4fNormal3fVertex3fvSUN (void);
#define      gdk_gl_glColor4fNormal3fVertex3fvSUN(proc, c, n, v) \
  ( ((GdkGLProc_glColor4fNormal3fVertex3fvSUN) (proc)) (c, n, v) )

/* glTexCoord2fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glTexCoord2fVertex3fSUN (void);
#define      gdk_gl_glTexCoord2fVertex3fSUN(proc, s, t, x, y, z) \
  ( ((GdkGLProc_glTexCoord2fVertex3fSUN) (proc)) (s, t, x, y, z) )

/* glTexCoord2fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fVertex3fvSUN) (const GLfloat *tc, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord2fVertex3fvSUN (void);
#define      gdk_gl_glTexCoord2fVertex3fvSUN(proc, tc, v) \
  ( ((GdkGLProc_glTexCoord2fVertex3fvSUN) (proc)) (tc, v) )

/* glTexCoord4fVertex4fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord4fVertex4fSUN) (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glTexCoord4fVertex4fSUN (void);
#define      gdk_gl_glTexCoord4fVertex4fSUN(proc, s, t, p, q, x, y, z, w) \
  ( ((GdkGLProc_glTexCoord4fVertex4fSUN) (proc)) (s, t, p, q, x, y, z, w) )

/* glTexCoord4fVertex4fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord4fVertex4fvSUN) (const GLfloat *tc, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord4fVertex4fvSUN (void);
#define      gdk_gl_glTexCoord4fVertex4fvSUN(proc, tc, v) \
  ( ((GdkGLProc_glTexCoord4fVertex4fvSUN) (proc)) (tc, v) )

/* glTexCoord2fColor4ubVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fColor4ubVertex3fSUN) (GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glTexCoord2fColor4ubVertex3fSUN (void);
#define      gdk_gl_glTexCoord2fColor4ubVertex3fSUN(proc, s, t, r, g, b, a, x, y, z) \
  ( ((GdkGLProc_glTexCoord2fColor4ubVertex3fSUN) (proc)) (s, t, r, g, b, a, x, y, z) )

/* glTexCoord2fColor4ubVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fColor4ubVertex3fvSUN) (const GLfloat *tc, const GLubyte *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord2fColor4ubVertex3fvSUN (void);
#define      gdk_gl_glTexCoord2fColor4ubVertex3fvSUN(proc, tc, c, v) \
  ( ((GdkGLProc_glTexCoord2fColor4ubVertex3fvSUN) (proc)) (tc, c, v) )

/* glTexCoord2fColor3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fColor3fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glTexCoord2fColor3fVertex3fSUN (void);
#define      gdk_gl_glTexCoord2fColor3fVertex3fSUN(proc, s, t, r, g, b, x, y, z) \
  ( ((GdkGLProc_glTexCoord2fColor3fVertex3fSUN) (proc)) (s, t, r, g, b, x, y, z) )

/* glTexCoord2fColor3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fColor3fVertex3fvSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord2fColor3fVertex3fvSUN (void);
#define      gdk_gl_glTexCoord2fColor3fVertex3fvSUN(proc, tc, c, v) \
  ( ((GdkGLProc_glTexCoord2fColor3fVertex3fvSUN) (proc)) (tc, c, v) )

/* glTexCoord2fNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fNormal3fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glTexCoord2fNormal3fVertex3fSUN (void);
#define      gdk_gl_glTexCoord2fNormal3fVertex3fSUN(proc, s, t, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glTexCoord2fNormal3fVertex3fSUN) (proc)) (s, t, nx, ny, nz, x, y, z) )

/* glTexCoord2fNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fNormal3fVertex3fvSUN) (const GLfloat *tc, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord2fNormal3fVertex3fvSUN (void);
#define      gdk_gl_glTexCoord2fNormal3fVertex3fvSUN(proc, tc, n, v) \
  ( ((GdkGLProc_glTexCoord2fNormal3fVertex3fvSUN) (proc)) (tc, n, v) )

/* glTexCoord2fColor4fNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fSUN (void);
#define      gdk_gl_glTexCoord2fColor4fNormal3fVertex3fSUN(proc, s, t, r, g, b, a, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fSUN) (proc)) (s, t, r, g, b, a, nx, ny, nz, x, y, z) )

/* glTexCoord2fColor4fNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fvSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fvSUN (void);
#define      gdk_gl_glTexCoord2fColor4fNormal3fVertex3fvSUN(proc, tc, c, n, v) \
  ( ((GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fvSUN) (proc)) (tc, c, n, v) )

/* glTexCoord4fColor4fNormal3fVertex4fSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fSUN) (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fSUN (void);
#define      gdk_gl_glTexCoord4fColor4fNormal3fVertex4fSUN(proc, s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w) \
  ( ((GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fSUN) (proc)) (s, t, p, q, r, g, b, a, nx, ny, nz, x, y, z, w) )

/* glTexCoord4fColor4fNormal3fVertex4fvSUN */
typedef void (APIENTRY * GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fvSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fvSUN (void);
#define      gdk_gl_glTexCoord4fColor4fNormal3fVertex4fvSUN(proc, tc, c, n, v) \
  ( ((GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fvSUN) (proc)) (tc, c, n, v) )

/* glReplacementCodeuiVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiVertex3fSUN) (GLuint rc, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiVertex3fSUN(proc, rc, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiVertex3fSUN) (proc)) (rc, x, y, z) )

/* glReplacementCodeuiVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiVertex3fvSUN) (const GLuint *rc, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiVertex3fvSUN(proc, rc, v) \
  ( ((GdkGLProc_glReplacementCodeuiVertex3fvSUN) (proc)) (rc, v) )

/* glReplacementCodeuiColor4ubVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiColor4ubVertex3fSUN) (GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiColor4ubVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiColor4ubVertex3fSUN(proc, rc, r, g, b, a, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiColor4ubVertex3fSUN) (proc)) (rc, r, g, b, a, x, y, z) )

/* glReplacementCodeuiColor4ubVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiColor4ubVertex3fvSUN) (const GLuint *rc, const GLubyte *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiColor4ubVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiColor4ubVertex3fvSUN(proc, rc, c, v) \
  ( ((GdkGLProc_glReplacementCodeuiColor4ubVertex3fvSUN) (proc)) (rc, c, v) )

/* glReplacementCodeuiColor3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiColor3fVertex3fSUN) (GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiColor3fVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiColor3fVertex3fSUN(proc, rc, r, g, b, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiColor3fVertex3fSUN) (proc)) (rc, r, g, b, x, y, z) )

/* glReplacementCodeuiColor3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiColor3fVertex3fvSUN) (const GLuint *rc, const GLfloat *c, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiColor3fVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiColor3fVertex3fvSUN(proc, rc, c, v) \
  ( ((GdkGLProc_glReplacementCodeuiColor3fVertex3fvSUN) (proc)) (rc, c, v) )

/* glReplacementCodeuiNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiNormal3fVertex3fSUN) (GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiNormal3fVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiNormal3fVertex3fSUN(proc, rc, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiNormal3fVertex3fSUN) (proc)) (rc, nx, ny, nz, x, y, z) )

/* glReplacementCodeuiNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiNormal3fVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiNormal3fVertex3fvSUN(proc, rc, n, v) \
  ( ((GdkGLProc_glReplacementCodeuiNormal3fVertex3fvSUN) (proc)) (rc, n, v) )

/* glReplacementCodeuiColor4fNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fSUN) (GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiColor4fNormal3fVertex3fSUN(proc, rc, r, g, b, a, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fSUN) (proc)) (rc, r, g, b, a, nx, ny, nz, x, y, z) )

/* glReplacementCodeuiColor4fNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiColor4fNormal3fVertex3fvSUN(proc, rc, c, n, v) \
  ( ((GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fvSUN) (proc)) (rc, c, n, v) )

/* glReplacementCodeuiTexCoord2fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiTexCoord2fVertex3fSUN(proc, rc, s, t, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fSUN) (proc)) (rc, s, t, x, y, z) )

/* glReplacementCodeuiTexCoord2fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fvSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiTexCoord2fVertex3fvSUN(proc, rc, tc, v) \
  ( ((GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fvSUN) (proc)) (rc, tc, v) )

/* glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN(proc, rc, s, t, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) (proc)) (rc, s, t, nx, ny, nz, x, y, z) )

/* glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN(proc, rc, tc, n, v) \
  ( ((GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) (proc)) (rc, tc, n, v) )

/* glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (void);
#define      gdk_gl_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN(proc, rc, s, t, r, g, b, a, nx, ny, nz, x, y, z) \
  ( ((GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) (proc)) (rc, s, t, r, g, b, a, nx, ny, nz, x, y, z) )

/* glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
typedef void (APIENTRY * GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
GdkGLProc    gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (void);
#define      gdk_gl_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN(proc, rc, tc, c, n, v) \
  ( ((GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) (proc)) (rc, tc, c, n, v) )

/* proc struct */

typedef struct _GdkGL_GL_SUN_vertex GdkGL_GL_SUN_vertex;

struct _GdkGL_GL_SUN_vertex
{
  GdkGLProc_glColor4ubVertex2fSUN glColor4ubVertex2fSUN;
  GdkGLProc_glColor4ubVertex2fvSUN glColor4ubVertex2fvSUN;
  GdkGLProc_glColor4ubVertex3fSUN glColor4ubVertex3fSUN;
  GdkGLProc_glColor4ubVertex3fvSUN glColor4ubVertex3fvSUN;
  GdkGLProc_glColor3fVertex3fSUN glColor3fVertex3fSUN;
  GdkGLProc_glColor3fVertex3fvSUN glColor3fVertex3fvSUN;
  GdkGLProc_glNormal3fVertex3fSUN glNormal3fVertex3fSUN;
  GdkGLProc_glNormal3fVertex3fvSUN glNormal3fVertex3fvSUN;
  GdkGLProc_glColor4fNormal3fVertex3fSUN glColor4fNormal3fVertex3fSUN;
  GdkGLProc_glColor4fNormal3fVertex3fvSUN glColor4fNormal3fVertex3fvSUN;
  GdkGLProc_glTexCoord2fVertex3fSUN glTexCoord2fVertex3fSUN;
  GdkGLProc_glTexCoord2fVertex3fvSUN glTexCoord2fVertex3fvSUN;
  GdkGLProc_glTexCoord4fVertex4fSUN glTexCoord4fVertex4fSUN;
  GdkGLProc_glTexCoord4fVertex4fvSUN glTexCoord4fVertex4fvSUN;
  GdkGLProc_glTexCoord2fColor4ubVertex3fSUN glTexCoord2fColor4ubVertex3fSUN;
  GdkGLProc_glTexCoord2fColor4ubVertex3fvSUN glTexCoord2fColor4ubVertex3fvSUN;
  GdkGLProc_glTexCoord2fColor3fVertex3fSUN glTexCoord2fColor3fVertex3fSUN;
  GdkGLProc_glTexCoord2fColor3fVertex3fvSUN glTexCoord2fColor3fVertex3fvSUN;
  GdkGLProc_glTexCoord2fNormal3fVertex3fSUN glTexCoord2fNormal3fVertex3fSUN;
  GdkGLProc_glTexCoord2fNormal3fVertex3fvSUN glTexCoord2fNormal3fVertex3fvSUN;
  GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fSUN glTexCoord2fColor4fNormal3fVertex3fSUN;
  GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fvSUN glTexCoord2fColor4fNormal3fVertex3fvSUN;
  GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fSUN glTexCoord4fColor4fNormal3fVertex4fSUN;
  GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fvSUN glTexCoord4fColor4fNormal3fVertex4fvSUN;
  GdkGLProc_glReplacementCodeuiVertex3fSUN glReplacementCodeuiVertex3fSUN;
  GdkGLProc_glReplacementCodeuiVertex3fvSUN glReplacementCodeuiVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiColor4ubVertex3fSUN glReplacementCodeuiColor4ubVertex3fSUN;
  GdkGLProc_glReplacementCodeuiColor4ubVertex3fvSUN glReplacementCodeuiColor4ubVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiColor3fVertex3fSUN glReplacementCodeuiColor3fVertex3fSUN;
  GdkGLProc_glReplacementCodeuiColor3fVertex3fvSUN glReplacementCodeuiColor3fVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiNormal3fVertex3fSUN glReplacementCodeuiNormal3fVertex3fSUN;
  GdkGLProc_glReplacementCodeuiNormal3fVertex3fvSUN glReplacementCodeuiNormal3fVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fSUN glReplacementCodeuiColor4fNormal3fVertex3fSUN;
  GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fvSUN glReplacementCodeuiColor4fNormal3fVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fSUN glReplacementCodeuiTexCoord2fVertex3fSUN;
  GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fvSUN glReplacementCodeuiTexCoord2fVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN;
  GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN;
  GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN;
  GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN;
};

GdkGL_GL_SUN_vertex *gdk_gl_get_GL_SUN_vertex (void);

/*
 * GL_EXT_blend_func_separate
 */

/* glBlendFuncSeparateEXT */
typedef void (APIENTRY * GdkGLProc_glBlendFuncSeparateEXT) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GdkGLProc    gdk_gl_get_glBlendFuncSeparateEXT (void);
#define      gdk_gl_glBlendFuncSeparateEXT(proc, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha) \
  ( ((GdkGLProc_glBlendFuncSeparateEXT) (proc)) (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_blend_func_separate GdkGL_GL_EXT_blend_func_separate;

struct _GdkGL_GL_EXT_blend_func_separate
{
  GdkGLProc_glBlendFuncSeparateEXT glBlendFuncSeparateEXT;
};

GdkGL_GL_EXT_blend_func_separate *gdk_gl_get_GL_EXT_blend_func_separate (void);

/*
 * GL_INGR_blend_func_separate
 */

/* glBlendFuncSeparateINGR */
typedef void (APIENTRY * GdkGLProc_glBlendFuncSeparateINGR) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
GdkGLProc    gdk_gl_get_glBlendFuncSeparateINGR (void);
#define      gdk_gl_glBlendFuncSeparateINGR(proc, sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha) \
  ( ((GdkGLProc_glBlendFuncSeparateINGR) (proc)) (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha) )

/* proc struct */

typedef struct _GdkGL_GL_INGR_blend_func_separate GdkGL_GL_INGR_blend_func_separate;

struct _GdkGL_GL_INGR_blend_func_separate
{
  GdkGLProc_glBlendFuncSeparateINGR glBlendFuncSeparateINGR;
};

GdkGL_GL_INGR_blend_func_separate *gdk_gl_get_GL_INGR_blend_func_separate (void);

/*
 * GL_EXT_vertex_weighting
 */

/* glVertexWeightfEXT */
typedef void (APIENTRY * GdkGLProc_glVertexWeightfEXT) (GLfloat weight);
GdkGLProc    gdk_gl_get_glVertexWeightfEXT (void);
#define      gdk_gl_glVertexWeightfEXT(proc, weight) \
  ( ((GdkGLProc_glVertexWeightfEXT) (proc)) (weight) )

/* glVertexWeightfvEXT */
typedef void (APIENTRY * GdkGLProc_glVertexWeightfvEXT) (const GLfloat *weight);
GdkGLProc    gdk_gl_get_glVertexWeightfvEXT (void);
#define      gdk_gl_glVertexWeightfvEXT(proc, weight) \
  ( ((GdkGLProc_glVertexWeightfvEXT) (proc)) (weight) )

/* glVertexWeightPointerEXT */
typedef void (APIENTRY * GdkGLProc_glVertexWeightPointerEXT) (GLsizei size, GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glVertexWeightPointerEXT (void);
#define      gdk_gl_glVertexWeightPointerEXT(proc, size, type, stride, pointer) \
  ( ((GdkGLProc_glVertexWeightPointerEXT) (proc)) (size, type, stride, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_vertex_weighting GdkGL_GL_EXT_vertex_weighting;

struct _GdkGL_GL_EXT_vertex_weighting
{
  GdkGLProc_glVertexWeightfEXT glVertexWeightfEXT;
  GdkGLProc_glVertexWeightfvEXT glVertexWeightfvEXT;
  GdkGLProc_glVertexWeightPointerEXT glVertexWeightPointerEXT;
};

GdkGL_GL_EXT_vertex_weighting *gdk_gl_get_GL_EXT_vertex_weighting (void);

/*
 * GL_NV_vertex_array_range
 */

/* glFlushVertexArrayRangeNV */
typedef void (APIENTRY * GdkGLProc_glFlushVertexArrayRangeNV) (void);
GdkGLProc    gdk_gl_get_glFlushVertexArrayRangeNV (void);
#define      gdk_gl_glFlushVertexArrayRangeNV(proc) \
  ( ((GdkGLProc_glFlushVertexArrayRangeNV) (proc)) () )

/* glVertexArrayRangeNV */
typedef void (APIENTRY * GdkGLProc_glVertexArrayRangeNV) (GLsizei length, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glVertexArrayRangeNV (void);
#define      gdk_gl_glVertexArrayRangeNV(proc, length, pointer) \
  ( ((GdkGLProc_glVertexArrayRangeNV) (proc)) (length, pointer) )

/* proc struct */

typedef struct _GdkGL_GL_NV_vertex_array_range GdkGL_GL_NV_vertex_array_range;

struct _GdkGL_GL_NV_vertex_array_range
{
  GdkGLProc_glFlushVertexArrayRangeNV glFlushVertexArrayRangeNV;
  GdkGLProc_glVertexArrayRangeNV glVertexArrayRangeNV;
};

GdkGL_GL_NV_vertex_array_range *gdk_gl_get_GL_NV_vertex_array_range (void);

/*
 * GL_NV_register_combiners
 */

/* glCombinerParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glCombinerParameterfvNV) (GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glCombinerParameterfvNV (void);
#define      gdk_gl_glCombinerParameterfvNV(proc, pname, params) \
  ( ((GdkGLProc_glCombinerParameterfvNV) (proc)) (pname, params) )

/* glCombinerParameterfNV */
typedef void (APIENTRY * GdkGLProc_glCombinerParameterfNV) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glCombinerParameterfNV (void);
#define      gdk_gl_glCombinerParameterfNV(proc, pname, param) \
  ( ((GdkGLProc_glCombinerParameterfNV) (proc)) (pname, param) )

/* glCombinerParameterivNV */
typedef void (APIENTRY * GdkGLProc_glCombinerParameterivNV) (GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glCombinerParameterivNV (void);
#define      gdk_gl_glCombinerParameterivNV(proc, pname, params) \
  ( ((GdkGLProc_glCombinerParameterivNV) (proc)) (pname, params) )

/* glCombinerParameteriNV */
typedef void (APIENTRY * GdkGLProc_glCombinerParameteriNV) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glCombinerParameteriNV (void);
#define      gdk_gl_glCombinerParameteriNV(proc, pname, param) \
  ( ((GdkGLProc_glCombinerParameteriNV) (proc)) (pname, param) )

/* glCombinerInputNV */
typedef void (APIENTRY * GdkGLProc_glCombinerInputNV) (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
GdkGLProc    gdk_gl_get_glCombinerInputNV (void);
#define      gdk_gl_glCombinerInputNV(proc, stage, portion, variable, input, mapping, componentUsage) \
  ( ((GdkGLProc_glCombinerInputNV) (proc)) (stage, portion, variable, input, mapping, componentUsage) )

/* glCombinerOutputNV */
typedef void (APIENTRY * GdkGLProc_glCombinerOutputNV) (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
GdkGLProc    gdk_gl_get_glCombinerOutputNV (void);
#define      gdk_gl_glCombinerOutputNV(proc, stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum) \
  ( ((GdkGLProc_glCombinerOutputNV) (proc)) (stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum) )

/* glFinalCombinerInputNV */
typedef void (APIENTRY * GdkGLProc_glFinalCombinerInputNV) (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
GdkGLProc    gdk_gl_get_glFinalCombinerInputNV (void);
#define      gdk_gl_glFinalCombinerInputNV(proc, variable, input, mapping, componentUsage) \
  ( ((GdkGLProc_glFinalCombinerInputNV) (proc)) (variable, input, mapping, componentUsage) )

/* glGetCombinerInputParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetCombinerInputParameterfvNV) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetCombinerInputParameterfvNV (void);
#define      gdk_gl_glGetCombinerInputParameterfvNV(proc, stage, portion, variable, pname, params) \
  ( ((GdkGLProc_glGetCombinerInputParameterfvNV) (proc)) (stage, portion, variable, pname, params) )

/* glGetCombinerInputParameterivNV */
typedef void (APIENTRY * GdkGLProc_glGetCombinerInputParameterivNV) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetCombinerInputParameterivNV (void);
#define      gdk_gl_glGetCombinerInputParameterivNV(proc, stage, portion, variable, pname, params) \
  ( ((GdkGLProc_glGetCombinerInputParameterivNV) (proc)) (stage, portion, variable, pname, params) )

/* glGetCombinerOutputParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetCombinerOutputParameterfvNV) (GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetCombinerOutputParameterfvNV (void);
#define      gdk_gl_glGetCombinerOutputParameterfvNV(proc, stage, portion, pname, params) \
  ( ((GdkGLProc_glGetCombinerOutputParameterfvNV) (proc)) (stage, portion, pname, params) )

/* glGetCombinerOutputParameterivNV */
typedef void (APIENTRY * GdkGLProc_glGetCombinerOutputParameterivNV) (GLenum stage, GLenum portion, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetCombinerOutputParameterivNV (void);
#define      gdk_gl_glGetCombinerOutputParameterivNV(proc, stage, portion, pname, params) \
  ( ((GdkGLProc_glGetCombinerOutputParameterivNV) (proc)) (stage, portion, pname, params) )

/* glGetFinalCombinerInputParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetFinalCombinerInputParameterfvNV) (GLenum variable, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetFinalCombinerInputParameterfvNV (void);
#define      gdk_gl_glGetFinalCombinerInputParameterfvNV(proc, variable, pname, params) \
  ( ((GdkGLProc_glGetFinalCombinerInputParameterfvNV) (proc)) (variable, pname, params) )

/* glGetFinalCombinerInputParameterivNV */
typedef void (APIENTRY * GdkGLProc_glGetFinalCombinerInputParameterivNV) (GLenum variable, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetFinalCombinerInputParameterivNV (void);
#define      gdk_gl_glGetFinalCombinerInputParameterivNV(proc, variable, pname, params) \
  ( ((GdkGLProc_glGetFinalCombinerInputParameterivNV) (proc)) (variable, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_NV_register_combiners GdkGL_GL_NV_register_combiners;

struct _GdkGL_GL_NV_register_combiners
{
  GdkGLProc_glCombinerParameterfvNV glCombinerParameterfvNV;
  GdkGLProc_glCombinerParameterfNV glCombinerParameterfNV;
  GdkGLProc_glCombinerParameterivNV glCombinerParameterivNV;
  GdkGLProc_glCombinerParameteriNV glCombinerParameteriNV;
  GdkGLProc_glCombinerInputNV glCombinerInputNV;
  GdkGLProc_glCombinerOutputNV glCombinerOutputNV;
  GdkGLProc_glFinalCombinerInputNV glFinalCombinerInputNV;
  GdkGLProc_glGetCombinerInputParameterfvNV glGetCombinerInputParameterfvNV;
  GdkGLProc_glGetCombinerInputParameterivNV glGetCombinerInputParameterivNV;
  GdkGLProc_glGetCombinerOutputParameterfvNV glGetCombinerOutputParameterfvNV;
  GdkGLProc_glGetCombinerOutputParameterivNV glGetCombinerOutputParameterivNV;
  GdkGLProc_glGetFinalCombinerInputParameterfvNV glGetFinalCombinerInputParameterfvNV;
  GdkGLProc_glGetFinalCombinerInputParameterivNV glGetFinalCombinerInputParameterivNV;
};

GdkGL_GL_NV_register_combiners *gdk_gl_get_GL_NV_register_combiners (void);

/*
 * GL_MESA_resize_buffers
 */

/* glResizeBuffersMESA */
typedef void (APIENTRY * GdkGLProc_glResizeBuffersMESA) (void);
GdkGLProc    gdk_gl_get_glResizeBuffersMESA (void);
#define      gdk_gl_glResizeBuffersMESA(proc) \
  ( ((GdkGLProc_glResizeBuffersMESA) (proc)) () )

/* proc struct */

typedef struct _GdkGL_GL_MESA_resize_buffers GdkGL_GL_MESA_resize_buffers;

struct _GdkGL_GL_MESA_resize_buffers
{
  GdkGLProc_glResizeBuffersMESA glResizeBuffersMESA;
};

GdkGL_GL_MESA_resize_buffers *gdk_gl_get_GL_MESA_resize_buffers (void);

/*
 * GL_MESA_window_pos
 */

/* glWindowPos2dMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2dMESA) (GLdouble x, GLdouble y);
GdkGLProc    gdk_gl_get_glWindowPos2dMESA (void);
#define      gdk_gl_glWindowPos2dMESA(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2dMESA) (proc)) (x, y) )

/* glWindowPos2dvMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2dvMESA) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos2dvMESA (void);
#define      gdk_gl_glWindowPos2dvMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos2dvMESA) (proc)) (v) )

/* glWindowPos2fMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2fMESA) (GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glWindowPos2fMESA (void);
#define      gdk_gl_glWindowPos2fMESA(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2fMESA) (proc)) (x, y) )

/* glWindowPos2fvMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2fvMESA) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos2fvMESA (void);
#define      gdk_gl_glWindowPos2fvMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos2fvMESA) (proc)) (v) )

/* glWindowPos2iMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2iMESA) (GLint x, GLint y);
GdkGLProc    gdk_gl_get_glWindowPos2iMESA (void);
#define      gdk_gl_glWindowPos2iMESA(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2iMESA) (proc)) (x, y) )

/* glWindowPos2ivMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2ivMESA) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos2ivMESA (void);
#define      gdk_gl_glWindowPos2ivMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos2ivMESA) (proc)) (v) )

/* glWindowPos2sMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2sMESA) (GLshort x, GLshort y);
GdkGLProc    gdk_gl_get_glWindowPos2sMESA (void);
#define      gdk_gl_glWindowPos2sMESA(proc, x, y) \
  ( ((GdkGLProc_glWindowPos2sMESA) (proc)) (x, y) )

/* glWindowPos2svMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos2svMESA) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos2svMESA (void);
#define      gdk_gl_glWindowPos2svMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos2svMESA) (proc)) (v) )

/* glWindowPos3dMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3dMESA) (GLdouble x, GLdouble y, GLdouble z);
GdkGLProc    gdk_gl_get_glWindowPos3dMESA (void);
#define      gdk_gl_glWindowPos3dMESA(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3dMESA) (proc)) (x, y, z) )

/* glWindowPos3dvMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3dvMESA) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos3dvMESA (void);
#define      gdk_gl_glWindowPos3dvMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos3dvMESA) (proc)) (v) )

/* glWindowPos3fMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3fMESA) (GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glWindowPos3fMESA (void);
#define      gdk_gl_glWindowPos3fMESA(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3fMESA) (proc)) (x, y, z) )

/* glWindowPos3fvMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3fvMESA) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos3fvMESA (void);
#define      gdk_gl_glWindowPos3fvMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos3fvMESA) (proc)) (v) )

/* glWindowPos3iMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3iMESA) (GLint x, GLint y, GLint z);
GdkGLProc    gdk_gl_get_glWindowPos3iMESA (void);
#define      gdk_gl_glWindowPos3iMESA(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3iMESA) (proc)) (x, y, z) )

/* glWindowPos3ivMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3ivMESA) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos3ivMESA (void);
#define      gdk_gl_glWindowPos3ivMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos3ivMESA) (proc)) (v) )

/* glWindowPos3sMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3sMESA) (GLshort x, GLshort y, GLshort z);
GdkGLProc    gdk_gl_get_glWindowPos3sMESA (void);
#define      gdk_gl_glWindowPos3sMESA(proc, x, y, z) \
  ( ((GdkGLProc_glWindowPos3sMESA) (proc)) (x, y, z) )

/* glWindowPos3svMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos3svMESA) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos3svMESA (void);
#define      gdk_gl_glWindowPos3svMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos3svMESA) (proc)) (v) )

/* glWindowPos4dMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4dMESA) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glWindowPos4dMESA (void);
#define      gdk_gl_glWindowPos4dMESA(proc, x, y, z, w) \
  ( ((GdkGLProc_glWindowPos4dMESA) (proc)) (x, y, z, w) )

/* glWindowPos4dvMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4dvMESA) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glWindowPos4dvMESA (void);
#define      gdk_gl_glWindowPos4dvMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos4dvMESA) (proc)) (v) )

/* glWindowPos4fMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4fMESA) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glWindowPos4fMESA (void);
#define      gdk_gl_glWindowPos4fMESA(proc, x, y, z, w) \
  ( ((GdkGLProc_glWindowPos4fMESA) (proc)) (x, y, z, w) )

/* glWindowPos4fvMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4fvMESA) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glWindowPos4fvMESA (void);
#define      gdk_gl_glWindowPos4fvMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos4fvMESA) (proc)) (v) )

/* glWindowPos4iMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4iMESA) (GLint x, GLint y, GLint z, GLint w);
GdkGLProc    gdk_gl_get_glWindowPos4iMESA (void);
#define      gdk_gl_glWindowPos4iMESA(proc, x, y, z, w) \
  ( ((GdkGLProc_glWindowPos4iMESA) (proc)) (x, y, z, w) )

/* glWindowPos4ivMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4ivMESA) (const GLint *v);
GdkGLProc    gdk_gl_get_glWindowPos4ivMESA (void);
#define      gdk_gl_glWindowPos4ivMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos4ivMESA) (proc)) (v) )

/* glWindowPos4sMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4sMESA) (GLshort x, GLshort y, GLshort z, GLshort w);
GdkGLProc    gdk_gl_get_glWindowPos4sMESA (void);
#define      gdk_gl_glWindowPos4sMESA(proc, x, y, z, w) \
  ( ((GdkGLProc_glWindowPos4sMESA) (proc)) (x, y, z, w) )

/* glWindowPos4svMESA */
typedef void (APIENTRY * GdkGLProc_glWindowPos4svMESA) (const GLshort *v);
GdkGLProc    gdk_gl_get_glWindowPos4svMESA (void);
#define      gdk_gl_glWindowPos4svMESA(proc, v) \
  ( ((GdkGLProc_glWindowPos4svMESA) (proc)) (v) )

/* proc struct */

typedef struct _GdkGL_GL_MESA_window_pos GdkGL_GL_MESA_window_pos;

struct _GdkGL_GL_MESA_window_pos
{
  GdkGLProc_glWindowPos2dMESA glWindowPos2dMESA;
  GdkGLProc_glWindowPos2dvMESA glWindowPos2dvMESA;
  GdkGLProc_glWindowPos2fMESA glWindowPos2fMESA;
  GdkGLProc_glWindowPos2fvMESA glWindowPos2fvMESA;
  GdkGLProc_glWindowPos2iMESA glWindowPos2iMESA;
  GdkGLProc_glWindowPos2ivMESA glWindowPos2ivMESA;
  GdkGLProc_glWindowPos2sMESA glWindowPos2sMESA;
  GdkGLProc_glWindowPos2svMESA glWindowPos2svMESA;
  GdkGLProc_glWindowPos3dMESA glWindowPos3dMESA;
  GdkGLProc_glWindowPos3dvMESA glWindowPos3dvMESA;
  GdkGLProc_glWindowPos3fMESA glWindowPos3fMESA;
  GdkGLProc_glWindowPos3fvMESA glWindowPos3fvMESA;
  GdkGLProc_glWindowPos3iMESA glWindowPos3iMESA;
  GdkGLProc_glWindowPos3ivMESA glWindowPos3ivMESA;
  GdkGLProc_glWindowPos3sMESA glWindowPos3sMESA;
  GdkGLProc_glWindowPos3svMESA glWindowPos3svMESA;
  GdkGLProc_glWindowPos4dMESA glWindowPos4dMESA;
  GdkGLProc_glWindowPos4dvMESA glWindowPos4dvMESA;
  GdkGLProc_glWindowPos4fMESA glWindowPos4fMESA;
  GdkGLProc_glWindowPos4fvMESA glWindowPos4fvMESA;
  GdkGLProc_glWindowPos4iMESA glWindowPos4iMESA;
  GdkGLProc_glWindowPos4ivMESA glWindowPos4ivMESA;
  GdkGLProc_glWindowPos4sMESA glWindowPos4sMESA;
  GdkGLProc_glWindowPos4svMESA glWindowPos4svMESA;
};

GdkGL_GL_MESA_window_pos *gdk_gl_get_GL_MESA_window_pos (void);

/*
 * GL_IBM_multimode_draw_arrays
 */

/* glMultiModeDrawArraysIBM */
typedef void (APIENTRY * GdkGLProc_glMultiModeDrawArraysIBM) (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride);
GdkGLProc    gdk_gl_get_glMultiModeDrawArraysIBM (void);
#define      gdk_gl_glMultiModeDrawArraysIBM(proc, mode, first, count, primcount, modestride) \
  ( ((GdkGLProc_glMultiModeDrawArraysIBM) (proc)) (mode, first, count, primcount, modestride) )

/* glMultiModeDrawElementsIBM */
typedef void (APIENTRY * GdkGLProc_glMultiModeDrawElementsIBM) (const GLenum *mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount, GLint modestride);
GdkGLProc    gdk_gl_get_glMultiModeDrawElementsIBM (void);
#define      gdk_gl_glMultiModeDrawElementsIBM(proc, mode, count, type, indices, primcount, modestride) \
  ( ((GdkGLProc_glMultiModeDrawElementsIBM) (proc)) (mode, count, type, indices, primcount, modestride) )

/* proc struct */

typedef struct _GdkGL_GL_IBM_multimode_draw_arrays GdkGL_GL_IBM_multimode_draw_arrays;

struct _GdkGL_GL_IBM_multimode_draw_arrays
{
  GdkGLProc_glMultiModeDrawArraysIBM glMultiModeDrawArraysIBM;
  GdkGLProc_glMultiModeDrawElementsIBM glMultiModeDrawElementsIBM;
};

GdkGL_GL_IBM_multimode_draw_arrays *gdk_gl_get_GL_IBM_multimode_draw_arrays (void);

/*
 * GL_IBM_vertex_array_lists
 */

/* glColorPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glColorPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glColorPointerListIBM (void);
#define      gdk_gl_glColorPointerListIBM(proc, size, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glColorPointerListIBM) (proc)) (size, type, stride, pointer, ptrstride) )

/* glSecondaryColorPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glSecondaryColorPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glSecondaryColorPointerListIBM (void);
#define      gdk_gl_glSecondaryColorPointerListIBM(proc, size, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glSecondaryColorPointerListIBM) (proc)) (size, type, stride, pointer, ptrstride) )

/* glEdgeFlagPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glEdgeFlagPointerListIBM) (GLint stride, const GLboolean* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glEdgeFlagPointerListIBM (void);
#define      gdk_gl_glEdgeFlagPointerListIBM(proc, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glEdgeFlagPointerListIBM) (proc)) (stride, pointer, ptrstride) )

/* glFogCoordPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glFogCoordPointerListIBM) (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glFogCoordPointerListIBM (void);
#define      gdk_gl_glFogCoordPointerListIBM(proc, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glFogCoordPointerListIBM) (proc)) (type, stride, pointer, ptrstride) )

/* glIndexPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glIndexPointerListIBM) (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glIndexPointerListIBM (void);
#define      gdk_gl_glIndexPointerListIBM(proc, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glIndexPointerListIBM) (proc)) (type, stride, pointer, ptrstride) )

/* glNormalPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glNormalPointerListIBM) (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glNormalPointerListIBM (void);
#define      gdk_gl_glNormalPointerListIBM(proc, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glNormalPointerListIBM) (proc)) (type, stride, pointer, ptrstride) )

/* glTexCoordPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glTexCoordPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glTexCoordPointerListIBM (void);
#define      gdk_gl_glTexCoordPointerListIBM(proc, size, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glTexCoordPointerListIBM) (proc)) (size, type, stride, pointer, ptrstride) )

/* glVertexPointerListIBM */
typedef void (APIENTRY * GdkGLProc_glVertexPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
GdkGLProc    gdk_gl_get_glVertexPointerListIBM (void);
#define      gdk_gl_glVertexPointerListIBM(proc, size, type, stride, pointer, ptrstride) \
  ( ((GdkGLProc_glVertexPointerListIBM) (proc)) (size, type, stride, pointer, ptrstride) )

/* proc struct */

typedef struct _GdkGL_GL_IBM_vertex_array_lists GdkGL_GL_IBM_vertex_array_lists;

struct _GdkGL_GL_IBM_vertex_array_lists
{
  GdkGLProc_glColorPointerListIBM glColorPointerListIBM;
  GdkGLProc_glSecondaryColorPointerListIBM glSecondaryColorPointerListIBM;
  GdkGLProc_glEdgeFlagPointerListIBM glEdgeFlagPointerListIBM;
  GdkGLProc_glFogCoordPointerListIBM glFogCoordPointerListIBM;
  GdkGLProc_glIndexPointerListIBM glIndexPointerListIBM;
  GdkGLProc_glNormalPointerListIBM glNormalPointerListIBM;
  GdkGLProc_glTexCoordPointerListIBM glTexCoordPointerListIBM;
  GdkGLProc_glVertexPointerListIBM glVertexPointerListIBM;
};

GdkGL_GL_IBM_vertex_array_lists *gdk_gl_get_GL_IBM_vertex_array_lists (void);

/*
 * GL_3DFX_tbuffer
 */

/* glTbufferMask3DFX */
typedef void (APIENTRY * GdkGLProc_glTbufferMask3DFX) (GLuint mask);
GdkGLProc    gdk_gl_get_glTbufferMask3DFX (void);
#define      gdk_gl_glTbufferMask3DFX(proc, mask) \
  ( ((GdkGLProc_glTbufferMask3DFX) (proc)) (mask) )

/* proc struct */

typedef struct _GdkGL_GL_3DFX_tbuffer GdkGL_GL_3DFX_tbuffer;

struct _GdkGL_GL_3DFX_tbuffer
{
  GdkGLProc_glTbufferMask3DFX glTbufferMask3DFX;
};

GdkGL_GL_3DFX_tbuffer *gdk_gl_get_GL_3DFX_tbuffer (void);

/*
 * GL_EXT_multisample
 */

/* glSampleMaskEXT */
typedef void (APIENTRY * GdkGLProc_glSampleMaskEXT) (GLclampf value, GLboolean invert);
GdkGLProc    gdk_gl_get_glSampleMaskEXT (void);
#define      gdk_gl_glSampleMaskEXT(proc, value, invert) \
  ( ((GdkGLProc_glSampleMaskEXT) (proc)) (value, invert) )

/* glSamplePatternEXT */
typedef void (APIENTRY * GdkGLProc_glSamplePatternEXT) (GLenum pattern);
GdkGLProc    gdk_gl_get_glSamplePatternEXT (void);
#define      gdk_gl_glSamplePatternEXT(proc, pattern) \
  ( ((GdkGLProc_glSamplePatternEXT) (proc)) (pattern) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_multisample GdkGL_GL_EXT_multisample;

struct _GdkGL_GL_EXT_multisample
{
  GdkGLProc_glSampleMaskEXT glSampleMaskEXT;
  GdkGLProc_glSamplePatternEXT glSamplePatternEXT;
};

GdkGL_GL_EXT_multisample *gdk_gl_get_GL_EXT_multisample (void);

/*
 * GL_SGIS_texture_color_mask
 */

/* glTextureColorMaskSGIS */
typedef void (APIENTRY * GdkGLProc_glTextureColorMaskSGIS) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
GdkGLProc    gdk_gl_get_glTextureColorMaskSGIS (void);
#define      gdk_gl_glTextureColorMaskSGIS(proc, red, green, blue, alpha) \
  ( ((GdkGLProc_glTextureColorMaskSGIS) (proc)) (red, green, blue, alpha) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_texture_color_mask GdkGL_GL_SGIS_texture_color_mask;

struct _GdkGL_GL_SGIS_texture_color_mask
{
  GdkGLProc_glTextureColorMaskSGIS glTextureColorMaskSGIS;
};

GdkGL_GL_SGIS_texture_color_mask *gdk_gl_get_GL_SGIS_texture_color_mask (void);

/*
 * GL_SGIX_igloo_interface
 */

/* glIglooInterfaceSGIX */
typedef void (APIENTRY * GdkGLProc_glIglooInterfaceSGIX) (GLenum pname, const GLvoid *params);
GdkGLProc    gdk_gl_get_glIglooInterfaceSGIX (void);
#define      gdk_gl_glIglooInterfaceSGIX(proc, pname, params) \
  ( ((GdkGLProc_glIglooInterfaceSGIX) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_igloo_interface GdkGL_GL_SGIX_igloo_interface;

struct _GdkGL_GL_SGIX_igloo_interface
{
  GdkGLProc_glIglooInterfaceSGIX glIglooInterfaceSGIX;
};

GdkGL_GL_SGIX_igloo_interface *gdk_gl_get_GL_SGIX_igloo_interface (void);

/*
 * GL_NV_fence
 */

/* glDeleteFencesNV */
typedef void (APIENTRY * GdkGLProc_glDeleteFencesNV) (GLsizei n, const GLuint *fences);
GdkGLProc    gdk_gl_get_glDeleteFencesNV (void);
#define      gdk_gl_glDeleteFencesNV(proc, n, fences) \
  ( ((GdkGLProc_glDeleteFencesNV) (proc)) (n, fences) )

/* glGenFencesNV */
typedef void (APIENTRY * GdkGLProc_glGenFencesNV) (GLsizei n, GLuint *fences);
GdkGLProc    gdk_gl_get_glGenFencesNV (void);
#define      gdk_gl_glGenFencesNV(proc, n, fences) \
  ( ((GdkGLProc_glGenFencesNV) (proc)) (n, fences) )

/* glIsFenceNV */
typedef GLboolean (APIENTRY * GdkGLProc_glIsFenceNV) (GLuint fence);
GdkGLProc    gdk_gl_get_glIsFenceNV (void);
#define      gdk_gl_glIsFenceNV(proc, fence) \
  ( ((GdkGLProc_glIsFenceNV) (proc)) (fence) )

/* glTestFenceNV */
typedef GLboolean (APIENTRY * GdkGLProc_glTestFenceNV) (GLuint fence);
GdkGLProc    gdk_gl_get_glTestFenceNV (void);
#define      gdk_gl_glTestFenceNV(proc, fence) \
  ( ((GdkGLProc_glTestFenceNV) (proc)) (fence) )

/* glGetFenceivNV */
typedef void (APIENTRY * GdkGLProc_glGetFenceivNV) (GLuint fence, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetFenceivNV (void);
#define      gdk_gl_glGetFenceivNV(proc, fence, pname, params) \
  ( ((GdkGLProc_glGetFenceivNV) (proc)) (fence, pname, params) )

/* glFinishFenceNV */
typedef void (APIENTRY * GdkGLProc_glFinishFenceNV) (GLuint fence);
GdkGLProc    gdk_gl_get_glFinishFenceNV (void);
#define      gdk_gl_glFinishFenceNV(proc, fence) \
  ( ((GdkGLProc_glFinishFenceNV) (proc)) (fence) )

/* glSetFenceNV */
typedef void (APIENTRY * GdkGLProc_glSetFenceNV) (GLuint fence, GLenum condition);
GdkGLProc    gdk_gl_get_glSetFenceNV (void);
#define      gdk_gl_glSetFenceNV(proc, fence, condition) \
  ( ((GdkGLProc_glSetFenceNV) (proc)) (fence, condition) )

/* proc struct */

typedef struct _GdkGL_GL_NV_fence GdkGL_GL_NV_fence;

struct _GdkGL_GL_NV_fence
{
  GdkGLProc_glDeleteFencesNV glDeleteFencesNV;
  GdkGLProc_glGenFencesNV glGenFencesNV;
  GdkGLProc_glIsFenceNV glIsFenceNV;
  GdkGLProc_glTestFenceNV glTestFenceNV;
  GdkGLProc_glGetFenceivNV glGetFenceivNV;
  GdkGLProc_glFinishFenceNV glFinishFenceNV;
  GdkGLProc_glSetFenceNV glSetFenceNV;
};

GdkGL_GL_NV_fence *gdk_gl_get_GL_NV_fence (void);

/*
 * GL_NV_evaluators
 */

/* glMapControlPointsNV */
typedef void (APIENTRY * GdkGLProc_glMapControlPointsNV) (GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const GLvoid *points);
GdkGLProc    gdk_gl_get_glMapControlPointsNV (void);
#define      gdk_gl_glMapControlPointsNV(proc, target, index, type, ustride, vstride, uorder, vorder, packed, points) \
  ( ((GdkGLProc_glMapControlPointsNV) (proc)) (target, index, type, ustride, vstride, uorder, vorder, packed, points) )

/* glMapParameterivNV */
typedef void (APIENTRY * GdkGLProc_glMapParameterivNV) (GLenum target, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glMapParameterivNV (void);
#define      gdk_gl_glMapParameterivNV(proc, target, pname, params) \
  ( ((GdkGLProc_glMapParameterivNV) (proc)) (target, pname, params) )

/* glMapParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glMapParameterfvNV) (GLenum target, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glMapParameterfvNV (void);
#define      gdk_gl_glMapParameterfvNV(proc, target, pname, params) \
  ( ((GdkGLProc_glMapParameterfvNV) (proc)) (target, pname, params) )

/* glGetMapControlPointsNV */
typedef void (APIENTRY * GdkGLProc_glGetMapControlPointsNV) (GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid *points);
GdkGLProc    gdk_gl_get_glGetMapControlPointsNV (void);
#define      gdk_gl_glGetMapControlPointsNV(proc, target, index, type, ustride, vstride, packed, points) \
  ( ((GdkGLProc_glGetMapControlPointsNV) (proc)) (target, index, type, ustride, vstride, packed, points) )

/* glGetMapParameterivNV */
typedef void (APIENTRY * GdkGLProc_glGetMapParameterivNV) (GLenum target, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetMapParameterivNV (void);
#define      gdk_gl_glGetMapParameterivNV(proc, target, pname, params) \
  ( ((GdkGLProc_glGetMapParameterivNV) (proc)) (target, pname, params) )

/* glGetMapParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetMapParameterfvNV) (GLenum target, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetMapParameterfvNV (void);
#define      gdk_gl_glGetMapParameterfvNV(proc, target, pname, params) \
  ( ((GdkGLProc_glGetMapParameterfvNV) (proc)) (target, pname, params) )

/* glGetMapAttribParameterivNV */
typedef void (APIENTRY * GdkGLProc_glGetMapAttribParameterivNV) (GLenum target, GLuint index, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetMapAttribParameterivNV (void);
#define      gdk_gl_glGetMapAttribParameterivNV(proc, target, index, pname, params) \
  ( ((GdkGLProc_glGetMapAttribParameterivNV) (proc)) (target, index, pname, params) )

/* glGetMapAttribParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetMapAttribParameterfvNV) (GLenum target, GLuint index, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetMapAttribParameterfvNV (void);
#define      gdk_gl_glGetMapAttribParameterfvNV(proc, target, index, pname, params) \
  ( ((GdkGLProc_glGetMapAttribParameterfvNV) (proc)) (target, index, pname, params) )

/* glEvalMapsNV */
typedef void (APIENTRY * GdkGLProc_glEvalMapsNV) (GLenum target, GLenum mode);
GdkGLProc    gdk_gl_get_glEvalMapsNV (void);
#define      gdk_gl_glEvalMapsNV(proc, target, mode) \
  ( ((GdkGLProc_glEvalMapsNV) (proc)) (target, mode) )

/* proc struct */

typedef struct _GdkGL_GL_NV_evaluators GdkGL_GL_NV_evaluators;

struct _GdkGL_GL_NV_evaluators
{
  GdkGLProc_glMapControlPointsNV glMapControlPointsNV;
  GdkGLProc_glMapParameterivNV glMapParameterivNV;
  GdkGLProc_glMapParameterfvNV glMapParameterfvNV;
  GdkGLProc_glGetMapControlPointsNV glGetMapControlPointsNV;
  GdkGLProc_glGetMapParameterivNV glGetMapParameterivNV;
  GdkGLProc_glGetMapParameterfvNV glGetMapParameterfvNV;
  GdkGLProc_glGetMapAttribParameterivNV glGetMapAttribParameterivNV;
  GdkGLProc_glGetMapAttribParameterfvNV glGetMapAttribParameterfvNV;
  GdkGLProc_glEvalMapsNV glEvalMapsNV;
};

GdkGL_GL_NV_evaluators *gdk_gl_get_GL_NV_evaluators (void);

/*
 * GL_NV_register_combiners2
 */

/* glCombinerStageParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glCombinerStageParameterfvNV) (GLenum stage, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glCombinerStageParameterfvNV (void);
#define      gdk_gl_glCombinerStageParameterfvNV(proc, stage, pname, params) \
  ( ((GdkGLProc_glCombinerStageParameterfvNV) (proc)) (stage, pname, params) )

/* glGetCombinerStageParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetCombinerStageParameterfvNV) (GLenum stage, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetCombinerStageParameterfvNV (void);
#define      gdk_gl_glGetCombinerStageParameterfvNV(proc, stage, pname, params) \
  ( ((GdkGLProc_glGetCombinerStageParameterfvNV) (proc)) (stage, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_NV_register_combiners2 GdkGL_GL_NV_register_combiners2;

struct _GdkGL_GL_NV_register_combiners2
{
  GdkGLProc_glCombinerStageParameterfvNV glCombinerStageParameterfvNV;
  GdkGLProc_glGetCombinerStageParameterfvNV glGetCombinerStageParameterfvNV;
};

GdkGL_GL_NV_register_combiners2 *gdk_gl_get_GL_NV_register_combiners2 (void);

/*
 * GL_NV_vertex_program
 */

/* glAreProgramsResidentNV */
typedef GLboolean (APIENTRY * GdkGLProc_glAreProgramsResidentNV) (GLsizei n, const GLuint *programs, GLboolean *residences);
GdkGLProc    gdk_gl_get_glAreProgramsResidentNV (void);
#define      gdk_gl_glAreProgramsResidentNV(proc, n, programs, residences) \
  ( ((GdkGLProc_glAreProgramsResidentNV) (proc)) (n, programs, residences) )

/* glBindProgramNV */
typedef void (APIENTRY * GdkGLProc_glBindProgramNV) (GLenum target, GLuint id);
GdkGLProc    gdk_gl_get_glBindProgramNV (void);
#define      gdk_gl_glBindProgramNV(proc, target, id) \
  ( ((GdkGLProc_glBindProgramNV) (proc)) (target, id) )

/* glDeleteProgramsNV */
typedef void (APIENTRY * GdkGLProc_glDeleteProgramsNV) (GLsizei n, const GLuint *programs);
GdkGLProc    gdk_gl_get_glDeleteProgramsNV (void);
#define      gdk_gl_glDeleteProgramsNV(proc, n, programs) \
  ( ((GdkGLProc_glDeleteProgramsNV) (proc)) (n, programs) )

/* glExecuteProgramNV */
typedef void (APIENTRY * GdkGLProc_glExecuteProgramNV) (GLenum target, GLuint id, const GLfloat *params);
GdkGLProc    gdk_gl_get_glExecuteProgramNV (void);
#define      gdk_gl_glExecuteProgramNV(proc, target, id, params) \
  ( ((GdkGLProc_glExecuteProgramNV) (proc)) (target, id, params) )

/* glGenProgramsNV */
typedef void (APIENTRY * GdkGLProc_glGenProgramsNV) (GLsizei n, GLuint *programs);
GdkGLProc    gdk_gl_get_glGenProgramsNV (void);
#define      gdk_gl_glGenProgramsNV(proc, n, programs) \
  ( ((GdkGLProc_glGenProgramsNV) (proc)) (n, programs) )

/* glGetProgramParameterdvNV */
typedef void (APIENTRY * GdkGLProc_glGetProgramParameterdvNV) (GLenum target, GLuint index, GLenum pname, GLdouble *params);
GdkGLProc    gdk_gl_get_glGetProgramParameterdvNV (void);
#define      gdk_gl_glGetProgramParameterdvNV(proc, target, index, pname, params) \
  ( ((GdkGLProc_glGetProgramParameterdvNV) (proc)) (target, index, pname, params) )

/* glGetProgramParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetProgramParameterfvNV) (GLenum target, GLuint index, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetProgramParameterfvNV (void);
#define      gdk_gl_glGetProgramParameterfvNV(proc, target, index, pname, params) \
  ( ((GdkGLProc_glGetProgramParameterfvNV) (proc)) (target, index, pname, params) )

/* glGetProgramivNV */
typedef void (APIENTRY * GdkGLProc_glGetProgramivNV) (GLuint id, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetProgramivNV (void);
#define      gdk_gl_glGetProgramivNV(proc, id, pname, params) \
  ( ((GdkGLProc_glGetProgramivNV) (proc)) (id, pname, params) )

/* glGetProgramStringNV */
typedef void (APIENTRY * GdkGLProc_glGetProgramStringNV) (GLuint id, GLenum pname, GLubyte *program);
GdkGLProc    gdk_gl_get_glGetProgramStringNV (void);
#define      gdk_gl_glGetProgramStringNV(proc, id, pname, program) \
  ( ((GdkGLProc_glGetProgramStringNV) (proc)) (id, pname, program) )

/* glGetTrackMatrixivNV */
typedef void (APIENTRY * GdkGLProc_glGetTrackMatrixivNV) (GLenum target, GLuint address, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetTrackMatrixivNV (void);
#define      gdk_gl_glGetTrackMatrixivNV(proc, target, address, pname, params) \
  ( ((GdkGLProc_glGetTrackMatrixivNV) (proc)) (target, address, pname, params) )

/* glGetVertexAttribdvNV */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribdvNV) (GLuint index, GLenum pname, GLdouble *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribdvNV (void);
#define      gdk_gl_glGetVertexAttribdvNV(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribdvNV) (proc)) (index, pname, params) )

/* glGetVertexAttribfvNV */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribfvNV) (GLuint index, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribfvNV (void);
#define      gdk_gl_glGetVertexAttribfvNV(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribfvNV) (proc)) (index, pname, params) )

/* glGetVertexAttribivNV */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribivNV) (GLuint index, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribivNV (void);
#define      gdk_gl_glGetVertexAttribivNV(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribivNV) (proc)) (index, pname, params) )

/* glGetVertexAttribPointervNV */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribPointervNV) (GLuint index, GLenum pname, GLvoid* *pointer);
GdkGLProc    gdk_gl_get_glGetVertexAttribPointervNV (void);
#define      gdk_gl_glGetVertexAttribPointervNV(proc, index, pname, pointer) \
  ( ((GdkGLProc_glGetVertexAttribPointervNV) (proc)) (index, pname, pointer) )

/* glIsProgramNV */
typedef GLboolean (APIENTRY * GdkGLProc_glIsProgramNV) (GLuint id);
GdkGLProc    gdk_gl_get_glIsProgramNV (void);
#define      gdk_gl_glIsProgramNV(proc, id) \
  ( ((GdkGLProc_glIsProgramNV) (proc)) (id) )

/* glLoadProgramNV */
typedef void (APIENTRY * GdkGLProc_glLoadProgramNV) (GLenum target, GLuint id, GLsizei len, const GLubyte *program);
GdkGLProc    gdk_gl_get_glLoadProgramNV (void);
#define      gdk_gl_glLoadProgramNV(proc, target, id, len, program) \
  ( ((GdkGLProc_glLoadProgramNV) (proc)) (target, id, len, program) )

/* glProgramParameter4dNV */
typedef void (APIENTRY * GdkGLProc_glProgramParameter4dNV) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glProgramParameter4dNV (void);
#define      gdk_gl_glProgramParameter4dNV(proc, target, index, x, y, z, w) \
  ( ((GdkGLProc_glProgramParameter4dNV) (proc)) (target, index, x, y, z, w) )

/* glProgramParameter4dvNV */
typedef void (APIENTRY * GdkGLProc_glProgramParameter4dvNV) (GLenum target, GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glProgramParameter4dvNV (void);
#define      gdk_gl_glProgramParameter4dvNV(proc, target, index, v) \
  ( ((GdkGLProc_glProgramParameter4dvNV) (proc)) (target, index, v) )

/* glProgramParameter4fNV */
typedef void (APIENTRY * GdkGLProc_glProgramParameter4fNV) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glProgramParameter4fNV (void);
#define      gdk_gl_glProgramParameter4fNV(proc, target, index, x, y, z, w) \
  ( ((GdkGLProc_glProgramParameter4fNV) (proc)) (target, index, x, y, z, w) )

/* glProgramParameter4fvNV */
typedef void (APIENTRY * GdkGLProc_glProgramParameter4fvNV) (GLenum target, GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glProgramParameter4fvNV (void);
#define      gdk_gl_glProgramParameter4fvNV(proc, target, index, v) \
  ( ((GdkGLProc_glProgramParameter4fvNV) (proc)) (target, index, v) )

/* glProgramParameters4dvNV */
typedef void (APIENTRY * GdkGLProc_glProgramParameters4dvNV) (GLenum target, GLuint index, GLuint count, const GLdouble *v);
GdkGLProc    gdk_gl_get_glProgramParameters4dvNV (void);
#define      gdk_gl_glProgramParameters4dvNV(proc, target, index, count, v) \
  ( ((GdkGLProc_glProgramParameters4dvNV) (proc)) (target, index, count, v) )

/* glProgramParameters4fvNV */
typedef void (APIENTRY * GdkGLProc_glProgramParameters4fvNV) (GLenum target, GLuint index, GLuint count, const GLfloat *v);
GdkGLProc    gdk_gl_get_glProgramParameters4fvNV (void);
#define      gdk_gl_glProgramParameters4fvNV(proc, target, index, count, v) \
  ( ((GdkGLProc_glProgramParameters4fvNV) (proc)) (target, index, count, v) )

/* glRequestResidentProgramsNV */
typedef void (APIENTRY * GdkGLProc_glRequestResidentProgramsNV) (GLsizei n, const GLuint *programs);
GdkGLProc    gdk_gl_get_glRequestResidentProgramsNV (void);
#define      gdk_gl_glRequestResidentProgramsNV(proc, n, programs) \
  ( ((GdkGLProc_glRequestResidentProgramsNV) (proc)) (n, programs) )

/* glTrackMatrixNV */
typedef void (APIENTRY * GdkGLProc_glTrackMatrixNV) (GLenum target, GLuint address, GLenum matrix, GLenum transform);
GdkGLProc    gdk_gl_get_glTrackMatrixNV (void);
#define      gdk_gl_glTrackMatrixNV(proc, target, address, matrix, transform) \
  ( ((GdkGLProc_glTrackMatrixNV) (proc)) (target, address, matrix, transform) )

/* glVertexAttribPointerNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribPointerNV) (GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glVertexAttribPointerNV (void);
#define      gdk_gl_glVertexAttribPointerNV(proc, index, fsize, type, stride, pointer) \
  ( ((GdkGLProc_glVertexAttribPointerNV) (proc)) (index, fsize, type, stride, pointer) )

/* glVertexAttrib1dNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1dNV) (GLuint index, GLdouble x);
GdkGLProc    gdk_gl_get_glVertexAttrib1dNV (void);
#define      gdk_gl_glVertexAttrib1dNV(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1dNV) (proc)) (index, x) )

/* glVertexAttrib1dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1dvNV) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1dvNV (void);
#define      gdk_gl_glVertexAttrib1dvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1dvNV) (proc)) (index, v) )

/* glVertexAttrib1fNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1fNV) (GLuint index, GLfloat x);
GdkGLProc    gdk_gl_get_glVertexAttrib1fNV (void);
#define      gdk_gl_glVertexAttrib1fNV(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1fNV) (proc)) (index, x) )

/* glVertexAttrib1fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1fvNV) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1fvNV (void);
#define      gdk_gl_glVertexAttrib1fvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1fvNV) (proc)) (index, v) )

/* glVertexAttrib1sNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1sNV) (GLuint index, GLshort x);
GdkGLProc    gdk_gl_get_glVertexAttrib1sNV (void);
#define      gdk_gl_glVertexAttrib1sNV(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1sNV) (proc)) (index, x) )

/* glVertexAttrib1svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1svNV) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1svNV (void);
#define      gdk_gl_glVertexAttrib1svNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1svNV) (proc)) (index, v) )

/* glVertexAttrib2dNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2dNV) (GLuint index, GLdouble x, GLdouble y);
GdkGLProc    gdk_gl_get_glVertexAttrib2dNV (void);
#define      gdk_gl_glVertexAttrib2dNV(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2dNV) (proc)) (index, x, y) )

/* glVertexAttrib2dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2dvNV) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2dvNV (void);
#define      gdk_gl_glVertexAttrib2dvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2dvNV) (proc)) (index, v) )

/* glVertexAttrib2fNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2fNV) (GLuint index, GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glVertexAttrib2fNV (void);
#define      gdk_gl_glVertexAttrib2fNV(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2fNV) (proc)) (index, x, y) )

/* glVertexAttrib2fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2fvNV) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2fvNV (void);
#define      gdk_gl_glVertexAttrib2fvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2fvNV) (proc)) (index, v) )

/* glVertexAttrib2sNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2sNV) (GLuint index, GLshort x, GLshort y);
GdkGLProc    gdk_gl_get_glVertexAttrib2sNV (void);
#define      gdk_gl_glVertexAttrib2sNV(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2sNV) (proc)) (index, x, y) )

/* glVertexAttrib2svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2svNV) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2svNV (void);
#define      gdk_gl_glVertexAttrib2svNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2svNV) (proc)) (index, v) )

/* glVertexAttrib3dNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3dNV) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
GdkGLProc    gdk_gl_get_glVertexAttrib3dNV (void);
#define      gdk_gl_glVertexAttrib3dNV(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3dNV) (proc)) (index, x, y, z) )

/* glVertexAttrib3dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3dvNV) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3dvNV (void);
#define      gdk_gl_glVertexAttrib3dvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3dvNV) (proc)) (index, v) )

/* glVertexAttrib3fNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3fNV) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glVertexAttrib3fNV (void);
#define      gdk_gl_glVertexAttrib3fNV(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3fNV) (proc)) (index, x, y, z) )

/* glVertexAttrib3fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3fvNV) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3fvNV (void);
#define      gdk_gl_glVertexAttrib3fvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3fvNV) (proc)) (index, v) )

/* glVertexAttrib3sNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3sNV) (GLuint index, GLshort x, GLshort y, GLshort z);
GdkGLProc    gdk_gl_get_glVertexAttrib3sNV (void);
#define      gdk_gl_glVertexAttrib3sNV(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3sNV) (proc)) (index, x, y, z) )

/* glVertexAttrib3svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3svNV) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3svNV (void);
#define      gdk_gl_glVertexAttrib3svNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3svNV) (proc)) (index, v) )

/* glVertexAttrib4dNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4dNV) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glVertexAttrib4dNV (void);
#define      gdk_gl_glVertexAttrib4dNV(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4dNV) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4dvNV) (GLuint index, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4dvNV (void);
#define      gdk_gl_glVertexAttrib4dvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4dvNV) (proc)) (index, v) )

/* glVertexAttrib4fNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4fNV) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glVertexAttrib4fNV (void);
#define      gdk_gl_glVertexAttrib4fNV(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4fNV) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4fvNV) (GLuint index, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4fvNV (void);
#define      gdk_gl_glVertexAttrib4fvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4fvNV) (proc)) (index, v) )

/* glVertexAttrib4sNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4sNV) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
GdkGLProc    gdk_gl_get_glVertexAttrib4sNV (void);
#define      gdk_gl_glVertexAttrib4sNV(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4sNV) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4svNV) (GLuint index, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4svNV (void);
#define      gdk_gl_glVertexAttrib4svNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4svNV) (proc)) (index, v) )

/* glVertexAttrib4ubNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4ubNV) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GdkGLProc    gdk_gl_get_glVertexAttrib4ubNV (void);
#define      gdk_gl_glVertexAttrib4ubNV(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4ubNV) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4ubvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4ubvNV) (GLuint index, const GLubyte *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4ubvNV (void);
#define      gdk_gl_glVertexAttrib4ubvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4ubvNV) (proc)) (index, v) )

/* glVertexAttribs1dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs1dvNV) (GLuint index, GLsizei count, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttribs1dvNV (void);
#define      gdk_gl_glVertexAttribs1dvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs1dvNV) (proc)) (index, count, v) )

/* glVertexAttribs1fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs1fvNV) (GLuint index, GLsizei count, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttribs1fvNV (void);
#define      gdk_gl_glVertexAttribs1fvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs1fvNV) (proc)) (index, count, v) )

/* glVertexAttribs1svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs1svNV) (GLuint index, GLsizei count, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttribs1svNV (void);
#define      gdk_gl_glVertexAttribs1svNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs1svNV) (proc)) (index, count, v) )

/* glVertexAttribs2dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs2dvNV) (GLuint index, GLsizei count, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttribs2dvNV (void);
#define      gdk_gl_glVertexAttribs2dvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs2dvNV) (proc)) (index, count, v) )

/* glVertexAttribs2fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs2fvNV) (GLuint index, GLsizei count, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttribs2fvNV (void);
#define      gdk_gl_glVertexAttribs2fvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs2fvNV) (proc)) (index, count, v) )

/* glVertexAttribs2svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs2svNV) (GLuint index, GLsizei count, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttribs2svNV (void);
#define      gdk_gl_glVertexAttribs2svNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs2svNV) (proc)) (index, count, v) )

/* glVertexAttribs3dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs3dvNV) (GLuint index, GLsizei count, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttribs3dvNV (void);
#define      gdk_gl_glVertexAttribs3dvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs3dvNV) (proc)) (index, count, v) )

/* glVertexAttribs3fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs3fvNV) (GLuint index, GLsizei count, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttribs3fvNV (void);
#define      gdk_gl_glVertexAttribs3fvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs3fvNV) (proc)) (index, count, v) )

/* glVertexAttribs3svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs3svNV) (GLuint index, GLsizei count, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttribs3svNV (void);
#define      gdk_gl_glVertexAttribs3svNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs3svNV) (proc)) (index, count, v) )

/* glVertexAttribs4dvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs4dvNV) (GLuint index, GLsizei count, const GLdouble *v);
GdkGLProc    gdk_gl_get_glVertexAttribs4dvNV (void);
#define      gdk_gl_glVertexAttribs4dvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs4dvNV) (proc)) (index, count, v) )

/* glVertexAttribs4fvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs4fvNV) (GLuint index, GLsizei count, const GLfloat *v);
GdkGLProc    gdk_gl_get_glVertexAttribs4fvNV (void);
#define      gdk_gl_glVertexAttribs4fvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs4fvNV) (proc)) (index, count, v) )

/* glVertexAttribs4svNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs4svNV) (GLuint index, GLsizei count, const GLshort *v);
GdkGLProc    gdk_gl_get_glVertexAttribs4svNV (void);
#define      gdk_gl_glVertexAttribs4svNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs4svNV) (proc)) (index, count, v) )

/* glVertexAttribs4ubvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs4ubvNV) (GLuint index, GLsizei count, const GLubyte *v);
GdkGLProc    gdk_gl_get_glVertexAttribs4ubvNV (void);
#define      gdk_gl_glVertexAttribs4ubvNV(proc, index, count, v) \
  ( ((GdkGLProc_glVertexAttribs4ubvNV) (proc)) (index, count, v) )

/* proc struct */

typedef struct _GdkGL_GL_NV_vertex_program GdkGL_GL_NV_vertex_program;

struct _GdkGL_GL_NV_vertex_program
{
  GdkGLProc_glAreProgramsResidentNV glAreProgramsResidentNV;
  GdkGLProc_glBindProgramNV glBindProgramNV;
  GdkGLProc_glDeleteProgramsNV glDeleteProgramsNV;
  GdkGLProc_glExecuteProgramNV glExecuteProgramNV;
  GdkGLProc_glGenProgramsNV glGenProgramsNV;
  GdkGLProc_glGetProgramParameterdvNV glGetProgramParameterdvNV;
  GdkGLProc_glGetProgramParameterfvNV glGetProgramParameterfvNV;
  GdkGLProc_glGetProgramivNV glGetProgramivNV;
  GdkGLProc_glGetProgramStringNV glGetProgramStringNV;
  GdkGLProc_glGetTrackMatrixivNV glGetTrackMatrixivNV;
  GdkGLProc_glGetVertexAttribdvNV glGetVertexAttribdvNV;
  GdkGLProc_glGetVertexAttribfvNV glGetVertexAttribfvNV;
  GdkGLProc_glGetVertexAttribivNV glGetVertexAttribivNV;
  GdkGLProc_glGetVertexAttribPointervNV glGetVertexAttribPointervNV;
  GdkGLProc_glIsProgramNV glIsProgramNV;
  GdkGLProc_glLoadProgramNV glLoadProgramNV;
  GdkGLProc_glProgramParameter4dNV glProgramParameter4dNV;
  GdkGLProc_glProgramParameter4dvNV glProgramParameter4dvNV;
  GdkGLProc_glProgramParameter4fNV glProgramParameter4fNV;
  GdkGLProc_glProgramParameter4fvNV glProgramParameter4fvNV;
  GdkGLProc_glProgramParameters4dvNV glProgramParameters4dvNV;
  GdkGLProc_glProgramParameters4fvNV glProgramParameters4fvNV;
  GdkGLProc_glRequestResidentProgramsNV glRequestResidentProgramsNV;
  GdkGLProc_glTrackMatrixNV glTrackMatrixNV;
  GdkGLProc_glVertexAttribPointerNV glVertexAttribPointerNV;
  GdkGLProc_glVertexAttrib1dNV glVertexAttrib1dNV;
  GdkGLProc_glVertexAttrib1dvNV glVertexAttrib1dvNV;
  GdkGLProc_glVertexAttrib1fNV glVertexAttrib1fNV;
  GdkGLProc_glVertexAttrib1fvNV glVertexAttrib1fvNV;
  GdkGLProc_glVertexAttrib1sNV glVertexAttrib1sNV;
  GdkGLProc_glVertexAttrib1svNV glVertexAttrib1svNV;
  GdkGLProc_glVertexAttrib2dNV glVertexAttrib2dNV;
  GdkGLProc_glVertexAttrib2dvNV glVertexAttrib2dvNV;
  GdkGLProc_glVertexAttrib2fNV glVertexAttrib2fNV;
  GdkGLProc_glVertexAttrib2fvNV glVertexAttrib2fvNV;
  GdkGLProc_glVertexAttrib2sNV glVertexAttrib2sNV;
  GdkGLProc_glVertexAttrib2svNV glVertexAttrib2svNV;
  GdkGLProc_glVertexAttrib3dNV glVertexAttrib3dNV;
  GdkGLProc_glVertexAttrib3dvNV glVertexAttrib3dvNV;
  GdkGLProc_glVertexAttrib3fNV glVertexAttrib3fNV;
  GdkGLProc_glVertexAttrib3fvNV glVertexAttrib3fvNV;
  GdkGLProc_glVertexAttrib3sNV glVertexAttrib3sNV;
  GdkGLProc_glVertexAttrib3svNV glVertexAttrib3svNV;
  GdkGLProc_glVertexAttrib4dNV glVertexAttrib4dNV;
  GdkGLProc_glVertexAttrib4dvNV glVertexAttrib4dvNV;
  GdkGLProc_glVertexAttrib4fNV glVertexAttrib4fNV;
  GdkGLProc_glVertexAttrib4fvNV glVertexAttrib4fvNV;
  GdkGLProc_glVertexAttrib4sNV glVertexAttrib4sNV;
  GdkGLProc_glVertexAttrib4svNV glVertexAttrib4svNV;
  GdkGLProc_glVertexAttrib4ubNV glVertexAttrib4ubNV;
  GdkGLProc_glVertexAttrib4ubvNV glVertexAttrib4ubvNV;
  GdkGLProc_glVertexAttribs1dvNV glVertexAttribs1dvNV;
  GdkGLProc_glVertexAttribs1fvNV glVertexAttribs1fvNV;
  GdkGLProc_glVertexAttribs1svNV glVertexAttribs1svNV;
  GdkGLProc_glVertexAttribs2dvNV glVertexAttribs2dvNV;
  GdkGLProc_glVertexAttribs2fvNV glVertexAttribs2fvNV;
  GdkGLProc_glVertexAttribs2svNV glVertexAttribs2svNV;
  GdkGLProc_glVertexAttribs3dvNV glVertexAttribs3dvNV;
  GdkGLProc_glVertexAttribs3fvNV glVertexAttribs3fvNV;
  GdkGLProc_glVertexAttribs3svNV glVertexAttribs3svNV;
  GdkGLProc_glVertexAttribs4dvNV glVertexAttribs4dvNV;
  GdkGLProc_glVertexAttribs4fvNV glVertexAttribs4fvNV;
  GdkGLProc_glVertexAttribs4svNV glVertexAttribs4svNV;
  GdkGLProc_glVertexAttribs4ubvNV glVertexAttribs4ubvNV;
};

GdkGL_GL_NV_vertex_program *gdk_gl_get_GL_NV_vertex_program (void);

/*
 * GL_ATI_envmap_bumpmap
 */

/* glTexBumpParameterivATI */
typedef void (APIENTRY * GdkGLProc_glTexBumpParameterivATI) (GLenum pname, const GLint *param);
GdkGLProc    gdk_gl_get_glTexBumpParameterivATI (void);
#define      gdk_gl_glTexBumpParameterivATI(proc, pname, param) \
  ( ((GdkGLProc_glTexBumpParameterivATI) (proc)) (pname, param) )

/* glTexBumpParameterfvATI */
typedef void (APIENTRY * GdkGLProc_glTexBumpParameterfvATI) (GLenum pname, const GLfloat *param);
GdkGLProc    gdk_gl_get_glTexBumpParameterfvATI (void);
#define      gdk_gl_glTexBumpParameterfvATI(proc, pname, param) \
  ( ((GdkGLProc_glTexBumpParameterfvATI) (proc)) (pname, param) )

/* glGetTexBumpParameterivATI */
typedef void (APIENTRY * GdkGLProc_glGetTexBumpParameterivATI) (GLenum pname, GLint *param);
GdkGLProc    gdk_gl_get_glGetTexBumpParameterivATI (void);
#define      gdk_gl_glGetTexBumpParameterivATI(proc, pname, param) \
  ( ((GdkGLProc_glGetTexBumpParameterivATI) (proc)) (pname, param) )

/* glGetTexBumpParameterfvATI */
typedef void (APIENTRY * GdkGLProc_glGetTexBumpParameterfvATI) (GLenum pname, GLfloat *param);
GdkGLProc    gdk_gl_get_glGetTexBumpParameterfvATI (void);
#define      gdk_gl_glGetTexBumpParameterfvATI(proc, pname, param) \
  ( ((GdkGLProc_glGetTexBumpParameterfvATI) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_envmap_bumpmap GdkGL_GL_ATI_envmap_bumpmap;

struct _GdkGL_GL_ATI_envmap_bumpmap
{
  GdkGLProc_glTexBumpParameterivATI glTexBumpParameterivATI;
  GdkGLProc_glTexBumpParameterfvATI glTexBumpParameterfvATI;
  GdkGLProc_glGetTexBumpParameterivATI glGetTexBumpParameterivATI;
  GdkGLProc_glGetTexBumpParameterfvATI glGetTexBumpParameterfvATI;
};

GdkGL_GL_ATI_envmap_bumpmap *gdk_gl_get_GL_ATI_envmap_bumpmap (void);

/*
 * GL_ATI_fragment_shader
 */

/* glGenFragmentShadersATI */
typedef GLuint (APIENTRY * GdkGLProc_glGenFragmentShadersATI) (GLuint range);
GdkGLProc    gdk_gl_get_glGenFragmentShadersATI (void);
#define      gdk_gl_glGenFragmentShadersATI(proc, range) \
  ( ((GdkGLProc_glGenFragmentShadersATI) (proc)) (range) )

/* glBindFragmentShaderATI */
typedef void (APIENTRY * GdkGLProc_glBindFragmentShaderATI) (GLuint id);
GdkGLProc    gdk_gl_get_glBindFragmentShaderATI (void);
#define      gdk_gl_glBindFragmentShaderATI(proc, id) \
  ( ((GdkGLProc_glBindFragmentShaderATI) (proc)) (id) )

/* glDeleteFragmentShaderATI */
typedef void (APIENTRY * GdkGLProc_glDeleteFragmentShaderATI) (GLuint id);
GdkGLProc    gdk_gl_get_glDeleteFragmentShaderATI (void);
#define      gdk_gl_glDeleteFragmentShaderATI(proc, id) \
  ( ((GdkGLProc_glDeleteFragmentShaderATI) (proc)) (id) )

/* glBeginFragmentShaderATI */
typedef void (APIENTRY * GdkGLProc_glBeginFragmentShaderATI) (void);
GdkGLProc    gdk_gl_get_glBeginFragmentShaderATI (void);
#define      gdk_gl_glBeginFragmentShaderATI(proc) \
  ( ((GdkGLProc_glBeginFragmentShaderATI) (proc)) () )

/* glEndFragmentShaderATI */
typedef void (APIENTRY * GdkGLProc_glEndFragmentShaderATI) (void);
GdkGLProc    gdk_gl_get_glEndFragmentShaderATI (void);
#define      gdk_gl_glEndFragmentShaderATI(proc) \
  ( ((GdkGLProc_glEndFragmentShaderATI) (proc)) () )

/* glPassTexCoordATI */
typedef void (APIENTRY * GdkGLProc_glPassTexCoordATI) (GLuint dst, GLuint coord, GLenum swizzle);
GdkGLProc    gdk_gl_get_glPassTexCoordATI (void);
#define      gdk_gl_glPassTexCoordATI(proc, dst, coord, swizzle) \
  ( ((GdkGLProc_glPassTexCoordATI) (proc)) (dst, coord, swizzle) )

/* glSampleMapATI */
typedef void (APIENTRY * GdkGLProc_glSampleMapATI) (GLuint dst, GLuint interp, GLenum swizzle);
GdkGLProc    gdk_gl_get_glSampleMapATI (void);
#define      gdk_gl_glSampleMapATI(proc, dst, interp, swizzle) \
  ( ((GdkGLProc_glSampleMapATI) (proc)) (dst, interp, swizzle) )

/* glColorFragmentOp1ATI */
typedef void (APIENTRY * GdkGLProc_glColorFragmentOp1ATI) (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
GdkGLProc    gdk_gl_get_glColorFragmentOp1ATI (void);
#define      gdk_gl_glColorFragmentOp1ATI(proc, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod) \
  ( ((GdkGLProc_glColorFragmentOp1ATI) (proc)) (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod) )

/* glColorFragmentOp2ATI */
typedef void (APIENTRY * GdkGLProc_glColorFragmentOp2ATI) (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
GdkGLProc    gdk_gl_get_glColorFragmentOp2ATI (void);
#define      gdk_gl_glColorFragmentOp2ATI(proc, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod) \
  ( ((GdkGLProc_glColorFragmentOp2ATI) (proc)) (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod) )

/* glColorFragmentOp3ATI */
typedef void (APIENTRY * GdkGLProc_glColorFragmentOp3ATI) (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
GdkGLProc    gdk_gl_get_glColorFragmentOp3ATI (void);
#define      gdk_gl_glColorFragmentOp3ATI(proc, op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod) \
  ( ((GdkGLProc_glColorFragmentOp3ATI) (proc)) (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod) )

/* glAlphaFragmentOp1ATI */
typedef void (APIENTRY * GdkGLProc_glAlphaFragmentOp1ATI) (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
GdkGLProc    gdk_gl_get_glAlphaFragmentOp1ATI (void);
#define      gdk_gl_glAlphaFragmentOp1ATI(proc, op, dst, dstMod, arg1, arg1Rep, arg1Mod) \
  ( ((GdkGLProc_glAlphaFragmentOp1ATI) (proc)) (op, dst, dstMod, arg1, arg1Rep, arg1Mod) )

/* glAlphaFragmentOp2ATI */
typedef void (APIENTRY * GdkGLProc_glAlphaFragmentOp2ATI) (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
GdkGLProc    gdk_gl_get_glAlphaFragmentOp2ATI (void);
#define      gdk_gl_glAlphaFragmentOp2ATI(proc, op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod) \
  ( ((GdkGLProc_glAlphaFragmentOp2ATI) (proc)) (op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod) )

/* glAlphaFragmentOp3ATI */
typedef void (APIENTRY * GdkGLProc_glAlphaFragmentOp3ATI) (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
GdkGLProc    gdk_gl_get_glAlphaFragmentOp3ATI (void);
#define      gdk_gl_glAlphaFragmentOp3ATI(proc, op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod) \
  ( ((GdkGLProc_glAlphaFragmentOp3ATI) (proc)) (op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod) )

/* glSetFragmentShaderConstantATI */
typedef void (APIENTRY * GdkGLProc_glSetFragmentShaderConstantATI) (GLuint dst, const GLfloat *value);
GdkGLProc    gdk_gl_get_glSetFragmentShaderConstantATI (void);
#define      gdk_gl_glSetFragmentShaderConstantATI(proc, dst, value) \
  ( ((GdkGLProc_glSetFragmentShaderConstantATI) (proc)) (dst, value) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_fragment_shader GdkGL_GL_ATI_fragment_shader;

struct _GdkGL_GL_ATI_fragment_shader
{
  GdkGLProc_glGenFragmentShadersATI glGenFragmentShadersATI;
  GdkGLProc_glBindFragmentShaderATI glBindFragmentShaderATI;
  GdkGLProc_glDeleteFragmentShaderATI glDeleteFragmentShaderATI;
  GdkGLProc_glBeginFragmentShaderATI glBeginFragmentShaderATI;
  GdkGLProc_glEndFragmentShaderATI glEndFragmentShaderATI;
  GdkGLProc_glPassTexCoordATI glPassTexCoordATI;
  GdkGLProc_glSampleMapATI glSampleMapATI;
  GdkGLProc_glColorFragmentOp1ATI glColorFragmentOp1ATI;
  GdkGLProc_glColorFragmentOp2ATI glColorFragmentOp2ATI;
  GdkGLProc_glColorFragmentOp3ATI glColorFragmentOp3ATI;
  GdkGLProc_glAlphaFragmentOp1ATI glAlphaFragmentOp1ATI;
  GdkGLProc_glAlphaFragmentOp2ATI glAlphaFragmentOp2ATI;
  GdkGLProc_glAlphaFragmentOp3ATI glAlphaFragmentOp3ATI;
  GdkGLProc_glSetFragmentShaderConstantATI glSetFragmentShaderConstantATI;
};

GdkGL_GL_ATI_fragment_shader *gdk_gl_get_GL_ATI_fragment_shader (void);

/*
 * GL_ATI_pn_triangles
 */

/* glPNTrianglesiATI */
typedef void (APIENTRY * GdkGLProc_glPNTrianglesiATI) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glPNTrianglesiATI (void);
#define      gdk_gl_glPNTrianglesiATI(proc, pname, param) \
  ( ((GdkGLProc_glPNTrianglesiATI) (proc)) (pname, param) )

/* glPNTrianglesfATI */
typedef void (APIENTRY * GdkGLProc_glPNTrianglesfATI) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPNTrianglesfATI (void);
#define      gdk_gl_glPNTrianglesfATI(proc, pname, param) \
  ( ((GdkGLProc_glPNTrianglesfATI) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_pn_triangles GdkGL_GL_ATI_pn_triangles;

struct _GdkGL_GL_ATI_pn_triangles
{
  GdkGLProc_glPNTrianglesiATI glPNTrianglesiATI;
  GdkGLProc_glPNTrianglesfATI glPNTrianglesfATI;
};

GdkGL_GL_ATI_pn_triangles *gdk_gl_get_GL_ATI_pn_triangles (void);

/*
 * GL_ATI_vertex_array_object
 */

/* glNewObjectBufferATI */
typedef GLuint (APIENTRY * GdkGLProc_glNewObjectBufferATI) (GLsizei size, const GLvoid *pointer, GLenum usage);
GdkGLProc    gdk_gl_get_glNewObjectBufferATI (void);
#define      gdk_gl_glNewObjectBufferATI(proc, size, pointer, usage) \
  ( ((GdkGLProc_glNewObjectBufferATI) (proc)) (size, pointer, usage) )

/* glIsObjectBufferATI */
typedef GLboolean (APIENTRY * GdkGLProc_glIsObjectBufferATI) (GLuint buffer);
GdkGLProc    gdk_gl_get_glIsObjectBufferATI (void);
#define      gdk_gl_glIsObjectBufferATI(proc, buffer) \
  ( ((GdkGLProc_glIsObjectBufferATI) (proc)) (buffer) )

/* glUpdateObjectBufferATI */
typedef void (APIENTRY * GdkGLProc_glUpdateObjectBufferATI) (GLuint buffer, GLuint offset, GLsizei size, const GLvoid *pointer, GLenum preserve);
GdkGLProc    gdk_gl_get_glUpdateObjectBufferATI (void);
#define      gdk_gl_glUpdateObjectBufferATI(proc, buffer, offset, size, pointer, preserve) \
  ( ((GdkGLProc_glUpdateObjectBufferATI) (proc)) (buffer, offset, size, pointer, preserve) )

/* glGetObjectBufferfvATI */
typedef void (APIENTRY * GdkGLProc_glGetObjectBufferfvATI) (GLuint buffer, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetObjectBufferfvATI (void);
#define      gdk_gl_glGetObjectBufferfvATI(proc, buffer, pname, params) \
  ( ((GdkGLProc_glGetObjectBufferfvATI) (proc)) (buffer, pname, params) )

/* glGetObjectBufferivATI */
typedef void (APIENTRY * GdkGLProc_glGetObjectBufferivATI) (GLuint buffer, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetObjectBufferivATI (void);
#define      gdk_gl_glGetObjectBufferivATI(proc, buffer, pname, params) \
  ( ((GdkGLProc_glGetObjectBufferivATI) (proc)) (buffer, pname, params) )

/* glFreeObjectBufferATI */
typedef void (APIENTRY * GdkGLProc_glFreeObjectBufferATI) (GLuint buffer);
GdkGLProc    gdk_gl_get_glFreeObjectBufferATI (void);
#define      gdk_gl_glFreeObjectBufferATI(proc, buffer) \
  ( ((GdkGLProc_glFreeObjectBufferATI) (proc)) (buffer) )

/* glArrayObjectATI */
typedef void (APIENTRY * GdkGLProc_glArrayObjectATI) (GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
GdkGLProc    gdk_gl_get_glArrayObjectATI (void);
#define      gdk_gl_glArrayObjectATI(proc, array, size, type, stride, buffer, offset) \
  ( ((GdkGLProc_glArrayObjectATI) (proc)) (array, size, type, stride, buffer, offset) )

/* glGetArrayObjectfvATI */
typedef void (APIENTRY * GdkGLProc_glGetArrayObjectfvATI) (GLenum array, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetArrayObjectfvATI (void);
#define      gdk_gl_glGetArrayObjectfvATI(proc, array, pname, params) \
  ( ((GdkGLProc_glGetArrayObjectfvATI) (proc)) (array, pname, params) )

/* glGetArrayObjectivATI */
typedef void (APIENTRY * GdkGLProc_glGetArrayObjectivATI) (GLenum array, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetArrayObjectivATI (void);
#define      gdk_gl_glGetArrayObjectivATI(proc, array, pname, params) \
  ( ((GdkGLProc_glGetArrayObjectivATI) (proc)) (array, pname, params) )

/* glVariantArrayObjectATI */
typedef void (APIENTRY * GdkGLProc_glVariantArrayObjectATI) (GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
GdkGLProc    gdk_gl_get_glVariantArrayObjectATI (void);
#define      gdk_gl_glVariantArrayObjectATI(proc, id, type, stride, buffer, offset) \
  ( ((GdkGLProc_glVariantArrayObjectATI) (proc)) (id, type, stride, buffer, offset) )

/* glGetVariantArrayObjectfvATI */
typedef void (APIENTRY * GdkGLProc_glGetVariantArrayObjectfvATI) (GLuint id, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetVariantArrayObjectfvATI (void);
#define      gdk_gl_glGetVariantArrayObjectfvATI(proc, id, pname, params) \
  ( ((GdkGLProc_glGetVariantArrayObjectfvATI) (proc)) (id, pname, params) )

/* glGetVariantArrayObjectivATI */
typedef void (APIENTRY * GdkGLProc_glGetVariantArrayObjectivATI) (GLuint id, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetVariantArrayObjectivATI (void);
#define      gdk_gl_glGetVariantArrayObjectivATI(proc, id, pname, params) \
  ( ((GdkGLProc_glGetVariantArrayObjectivATI) (proc)) (id, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_vertex_array_object GdkGL_GL_ATI_vertex_array_object;

struct _GdkGL_GL_ATI_vertex_array_object
{
  GdkGLProc_glNewObjectBufferATI glNewObjectBufferATI;
  GdkGLProc_glIsObjectBufferATI glIsObjectBufferATI;
  GdkGLProc_glUpdateObjectBufferATI glUpdateObjectBufferATI;
  GdkGLProc_glGetObjectBufferfvATI glGetObjectBufferfvATI;
  GdkGLProc_glGetObjectBufferivATI glGetObjectBufferivATI;
  GdkGLProc_glFreeObjectBufferATI glFreeObjectBufferATI;
  GdkGLProc_glArrayObjectATI glArrayObjectATI;
  GdkGLProc_glGetArrayObjectfvATI glGetArrayObjectfvATI;
  GdkGLProc_glGetArrayObjectivATI glGetArrayObjectivATI;
  GdkGLProc_glVariantArrayObjectATI glVariantArrayObjectATI;
  GdkGLProc_glGetVariantArrayObjectfvATI glGetVariantArrayObjectfvATI;
  GdkGLProc_glGetVariantArrayObjectivATI glGetVariantArrayObjectivATI;
};

GdkGL_GL_ATI_vertex_array_object *gdk_gl_get_GL_ATI_vertex_array_object (void);

/*
 * GL_EXT_vertex_shader
 */

/* glBeginVertexShaderEXT */
typedef void (APIENTRY * GdkGLProc_glBeginVertexShaderEXT) (void);
GdkGLProc    gdk_gl_get_glBeginVertexShaderEXT (void);
#define      gdk_gl_glBeginVertexShaderEXT(proc) \
  ( ((GdkGLProc_glBeginVertexShaderEXT) (proc)) () )

/* glEndVertexShaderEXT */
typedef void (APIENTRY * GdkGLProc_glEndVertexShaderEXT) (void);
GdkGLProc    gdk_gl_get_glEndVertexShaderEXT (void);
#define      gdk_gl_glEndVertexShaderEXT(proc) \
  ( ((GdkGLProc_glEndVertexShaderEXT) (proc)) () )

/* glBindVertexShaderEXT */
typedef void (APIENTRY * GdkGLProc_glBindVertexShaderEXT) (GLuint id);
GdkGLProc    gdk_gl_get_glBindVertexShaderEXT (void);
#define      gdk_gl_glBindVertexShaderEXT(proc, id) \
  ( ((GdkGLProc_glBindVertexShaderEXT) (proc)) (id) )

/* glGenVertexShadersEXT */
typedef GLuint (APIENTRY * GdkGLProc_glGenVertexShadersEXT) (GLuint range);
GdkGLProc    gdk_gl_get_glGenVertexShadersEXT (void);
#define      gdk_gl_glGenVertexShadersEXT(proc, range) \
  ( ((GdkGLProc_glGenVertexShadersEXT) (proc)) (range) )

/* glDeleteVertexShaderEXT */
typedef void (APIENTRY * GdkGLProc_glDeleteVertexShaderEXT) (GLuint id);
GdkGLProc    gdk_gl_get_glDeleteVertexShaderEXT (void);
#define      gdk_gl_glDeleteVertexShaderEXT(proc, id) \
  ( ((GdkGLProc_glDeleteVertexShaderEXT) (proc)) (id) )

/* glShaderOp1EXT */
typedef void (APIENTRY * GdkGLProc_glShaderOp1EXT) (GLenum op, GLuint res, GLuint arg1);
GdkGLProc    gdk_gl_get_glShaderOp1EXT (void);
#define      gdk_gl_glShaderOp1EXT(proc, op, res, arg1) \
  ( ((GdkGLProc_glShaderOp1EXT) (proc)) (op, res, arg1) )

/* glShaderOp2EXT */
typedef void (APIENTRY * GdkGLProc_glShaderOp2EXT) (GLenum op, GLuint res, GLuint arg1, GLuint arg2);
GdkGLProc    gdk_gl_get_glShaderOp2EXT (void);
#define      gdk_gl_glShaderOp2EXT(proc, op, res, arg1, arg2) \
  ( ((GdkGLProc_glShaderOp2EXT) (proc)) (op, res, arg1, arg2) )

/* glShaderOp3EXT */
typedef void (APIENTRY * GdkGLProc_glShaderOp3EXT) (GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3);
GdkGLProc    gdk_gl_get_glShaderOp3EXT (void);
#define      gdk_gl_glShaderOp3EXT(proc, op, res, arg1, arg2, arg3) \
  ( ((GdkGLProc_glShaderOp3EXT) (proc)) (op, res, arg1, arg2, arg3) )

/* glSwizzleEXT */
typedef void (APIENTRY * GdkGLProc_glSwizzleEXT) (GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
GdkGLProc    gdk_gl_get_glSwizzleEXT (void);
#define      gdk_gl_glSwizzleEXT(proc, res, in, outX, outY, outZ, outW) \
  ( ((GdkGLProc_glSwizzleEXT) (proc)) (res, in, outX, outY, outZ, outW) )

/* glWriteMaskEXT */
typedef void (APIENTRY * GdkGLProc_glWriteMaskEXT) (GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
GdkGLProc    gdk_gl_get_glWriteMaskEXT (void);
#define      gdk_gl_glWriteMaskEXT(proc, res, in, outX, outY, outZ, outW) \
  ( ((GdkGLProc_glWriteMaskEXT) (proc)) (res, in, outX, outY, outZ, outW) )

/* glInsertComponentEXT */
typedef void (APIENTRY * GdkGLProc_glInsertComponentEXT) (GLuint res, GLuint src, GLuint num);
GdkGLProc    gdk_gl_get_glInsertComponentEXT (void);
#define      gdk_gl_glInsertComponentEXT(proc, res, src, num) \
  ( ((GdkGLProc_glInsertComponentEXT) (proc)) (res, src, num) )

/* glExtractComponentEXT */
typedef void (APIENTRY * GdkGLProc_glExtractComponentEXT) (GLuint res, GLuint src, GLuint num);
GdkGLProc    gdk_gl_get_glExtractComponentEXT (void);
#define      gdk_gl_glExtractComponentEXT(proc, res, src, num) \
  ( ((GdkGLProc_glExtractComponentEXT) (proc)) (res, src, num) )

/* glGenSymbolsEXT */
typedef GLuint (APIENTRY * GdkGLProc_glGenSymbolsEXT) (GLenum datatype, GLenum storagetype, GLenum range, GLuint components);
GdkGLProc    gdk_gl_get_glGenSymbolsEXT (void);
#define      gdk_gl_glGenSymbolsEXT(proc, datatype, storagetype, range, components) \
  ( ((GdkGLProc_glGenSymbolsEXT) (proc)) (datatype, storagetype, range, components) )

/* glSetInvariantEXT */
typedef void (APIENTRY * GdkGLProc_glSetInvariantEXT) (GLuint id, GLenum type, const GLvoid *addr);
GdkGLProc    gdk_gl_get_glSetInvariantEXT (void);
#define      gdk_gl_glSetInvariantEXT(proc, id, type, addr) \
  ( ((GdkGLProc_glSetInvariantEXT) (proc)) (id, type, addr) )

/* glSetLocalConstantEXT */
typedef void (APIENTRY * GdkGLProc_glSetLocalConstantEXT) (GLuint id, GLenum type, const GLvoid *addr);
GdkGLProc    gdk_gl_get_glSetLocalConstantEXT (void);
#define      gdk_gl_glSetLocalConstantEXT(proc, id, type, addr) \
  ( ((GdkGLProc_glSetLocalConstantEXT) (proc)) (id, type, addr) )

/* glVariantbvEXT */
typedef void (APIENTRY * GdkGLProc_glVariantbvEXT) (GLuint id, const GLbyte *addr);
GdkGLProc    gdk_gl_get_glVariantbvEXT (void);
#define      gdk_gl_glVariantbvEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantbvEXT) (proc)) (id, addr) )

/* glVariantsvEXT */
typedef void (APIENTRY * GdkGLProc_glVariantsvEXT) (GLuint id, const GLshort *addr);
GdkGLProc    gdk_gl_get_glVariantsvEXT (void);
#define      gdk_gl_glVariantsvEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantsvEXT) (proc)) (id, addr) )

/* glVariantivEXT */
typedef void (APIENTRY * GdkGLProc_glVariantivEXT) (GLuint id, const GLint *addr);
GdkGLProc    gdk_gl_get_glVariantivEXT (void);
#define      gdk_gl_glVariantivEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantivEXT) (proc)) (id, addr) )

/* glVariantfvEXT */
typedef void (APIENTRY * GdkGLProc_glVariantfvEXT) (GLuint id, const GLfloat *addr);
GdkGLProc    gdk_gl_get_glVariantfvEXT (void);
#define      gdk_gl_glVariantfvEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantfvEXT) (proc)) (id, addr) )

/* glVariantdvEXT */
typedef void (APIENTRY * GdkGLProc_glVariantdvEXT) (GLuint id, const GLdouble *addr);
GdkGLProc    gdk_gl_get_glVariantdvEXT (void);
#define      gdk_gl_glVariantdvEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantdvEXT) (proc)) (id, addr) )

/* glVariantubvEXT */
typedef void (APIENTRY * GdkGLProc_glVariantubvEXT) (GLuint id, const GLubyte *addr);
GdkGLProc    gdk_gl_get_glVariantubvEXT (void);
#define      gdk_gl_glVariantubvEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantubvEXT) (proc)) (id, addr) )

/* glVariantusvEXT */
typedef void (APIENTRY * GdkGLProc_glVariantusvEXT) (GLuint id, const GLushort *addr);
GdkGLProc    gdk_gl_get_glVariantusvEXT (void);
#define      gdk_gl_glVariantusvEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantusvEXT) (proc)) (id, addr) )

/* glVariantuivEXT */
typedef void (APIENTRY * GdkGLProc_glVariantuivEXT) (GLuint id, const GLuint *addr);
GdkGLProc    gdk_gl_get_glVariantuivEXT (void);
#define      gdk_gl_glVariantuivEXT(proc, id, addr) \
  ( ((GdkGLProc_glVariantuivEXT) (proc)) (id, addr) )

/* glVariantPointerEXT */
typedef void (APIENTRY * GdkGLProc_glVariantPointerEXT) (GLuint id, GLenum type, GLuint stride, const GLvoid *addr);
GdkGLProc    gdk_gl_get_glVariantPointerEXT (void);
#define      gdk_gl_glVariantPointerEXT(proc, id, type, stride, addr) \
  ( ((GdkGLProc_glVariantPointerEXT) (proc)) (id, type, stride, addr) )

/* glEnableVariantClientStateEXT */
typedef void (APIENTRY * GdkGLProc_glEnableVariantClientStateEXT) (GLuint id);
GdkGLProc    gdk_gl_get_glEnableVariantClientStateEXT (void);
#define      gdk_gl_glEnableVariantClientStateEXT(proc, id) \
  ( ((GdkGLProc_glEnableVariantClientStateEXT) (proc)) (id) )

/* glDisableVariantClientStateEXT */
typedef void (APIENTRY * GdkGLProc_glDisableVariantClientStateEXT) (GLuint id);
GdkGLProc    gdk_gl_get_glDisableVariantClientStateEXT (void);
#define      gdk_gl_glDisableVariantClientStateEXT(proc, id) \
  ( ((GdkGLProc_glDisableVariantClientStateEXT) (proc)) (id) )

/* glBindLightParameterEXT */
typedef GLuint (APIENTRY * GdkGLProc_glBindLightParameterEXT) (GLenum light, GLenum value);
GdkGLProc    gdk_gl_get_glBindLightParameterEXT (void);
#define      gdk_gl_glBindLightParameterEXT(proc, light, value) \
  ( ((GdkGLProc_glBindLightParameterEXT) (proc)) (light, value) )

/* glBindMaterialParameterEXT */
typedef GLuint (APIENTRY * GdkGLProc_glBindMaterialParameterEXT) (GLenum face, GLenum value);
GdkGLProc    gdk_gl_get_glBindMaterialParameterEXT (void);
#define      gdk_gl_glBindMaterialParameterEXT(proc, face, value) \
  ( ((GdkGLProc_glBindMaterialParameterEXT) (proc)) (face, value) )

/* glBindTexGenParameterEXT */
typedef GLuint (APIENTRY * GdkGLProc_glBindTexGenParameterEXT) (GLenum unit, GLenum coord, GLenum value);
GdkGLProc    gdk_gl_get_glBindTexGenParameterEXT (void);
#define      gdk_gl_glBindTexGenParameterEXT(proc, unit, coord, value) \
  ( ((GdkGLProc_glBindTexGenParameterEXT) (proc)) (unit, coord, value) )

/* glBindTextureUnitParameterEXT */
typedef GLuint (APIENTRY * GdkGLProc_glBindTextureUnitParameterEXT) (GLenum unit, GLenum value);
GdkGLProc    gdk_gl_get_glBindTextureUnitParameterEXT (void);
#define      gdk_gl_glBindTextureUnitParameterEXT(proc, unit, value) \
  ( ((GdkGLProc_glBindTextureUnitParameterEXT) (proc)) (unit, value) )

/* glBindParameterEXT */
typedef GLuint (APIENTRY * GdkGLProc_glBindParameterEXT) (GLenum value);
GdkGLProc    gdk_gl_get_glBindParameterEXT (void);
#define      gdk_gl_glBindParameterEXT(proc, value) \
  ( ((GdkGLProc_glBindParameterEXT) (proc)) (value) )

/* glIsVariantEnabledEXT */
typedef GLboolean (APIENTRY * GdkGLProc_glIsVariantEnabledEXT) (GLuint id, GLenum cap);
GdkGLProc    gdk_gl_get_glIsVariantEnabledEXT (void);
#define      gdk_gl_glIsVariantEnabledEXT(proc, id, cap) \
  ( ((GdkGLProc_glIsVariantEnabledEXT) (proc)) (id, cap) )

/* glGetVariantBooleanvEXT */
typedef void (APIENTRY * GdkGLProc_glGetVariantBooleanvEXT) (GLuint id, GLenum value, GLboolean *data);
GdkGLProc    gdk_gl_get_glGetVariantBooleanvEXT (void);
#define      gdk_gl_glGetVariantBooleanvEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetVariantBooleanvEXT) (proc)) (id, value, data) )

/* glGetVariantIntegervEXT */
typedef void (APIENTRY * GdkGLProc_glGetVariantIntegervEXT) (GLuint id, GLenum value, GLint *data);
GdkGLProc    gdk_gl_get_glGetVariantIntegervEXT (void);
#define      gdk_gl_glGetVariantIntegervEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetVariantIntegervEXT) (proc)) (id, value, data) )

/* glGetVariantFloatvEXT */
typedef void (APIENTRY * GdkGLProc_glGetVariantFloatvEXT) (GLuint id, GLenum value, GLfloat *data);
GdkGLProc    gdk_gl_get_glGetVariantFloatvEXT (void);
#define      gdk_gl_glGetVariantFloatvEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetVariantFloatvEXT) (proc)) (id, value, data) )

/* glGetVariantPointervEXT */
typedef void (APIENTRY * GdkGLProc_glGetVariantPointervEXT) (GLuint id, GLenum value, GLvoid* *data);
GdkGLProc    gdk_gl_get_glGetVariantPointervEXT (void);
#define      gdk_gl_glGetVariantPointervEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetVariantPointervEXT) (proc)) (id, value, data) )

/* glGetInvariantBooleanvEXT */
typedef void (APIENTRY * GdkGLProc_glGetInvariantBooleanvEXT) (GLuint id, GLenum value, GLboolean *data);
GdkGLProc    gdk_gl_get_glGetInvariantBooleanvEXT (void);
#define      gdk_gl_glGetInvariantBooleanvEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetInvariantBooleanvEXT) (proc)) (id, value, data) )

/* glGetInvariantIntegervEXT */
typedef void (APIENTRY * GdkGLProc_glGetInvariantIntegervEXT) (GLuint id, GLenum value, GLint *data);
GdkGLProc    gdk_gl_get_glGetInvariantIntegervEXT (void);
#define      gdk_gl_glGetInvariantIntegervEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetInvariantIntegervEXT) (proc)) (id, value, data) )

/* glGetInvariantFloatvEXT */
typedef void (APIENTRY * GdkGLProc_glGetInvariantFloatvEXT) (GLuint id, GLenum value, GLfloat *data);
GdkGLProc    gdk_gl_get_glGetInvariantFloatvEXT (void);
#define      gdk_gl_glGetInvariantFloatvEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetInvariantFloatvEXT) (proc)) (id, value, data) )

/* glGetLocalConstantBooleanvEXT */
typedef void (APIENTRY * GdkGLProc_glGetLocalConstantBooleanvEXT) (GLuint id, GLenum value, GLboolean *data);
GdkGLProc    gdk_gl_get_glGetLocalConstantBooleanvEXT (void);
#define      gdk_gl_glGetLocalConstantBooleanvEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetLocalConstantBooleanvEXT) (proc)) (id, value, data) )

/* glGetLocalConstantIntegervEXT */
typedef void (APIENTRY * GdkGLProc_glGetLocalConstantIntegervEXT) (GLuint id, GLenum value, GLint *data);
GdkGLProc    gdk_gl_get_glGetLocalConstantIntegervEXT (void);
#define      gdk_gl_glGetLocalConstantIntegervEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetLocalConstantIntegervEXT) (proc)) (id, value, data) )

/* glGetLocalConstantFloatvEXT */
typedef void (APIENTRY * GdkGLProc_glGetLocalConstantFloatvEXT) (GLuint id, GLenum value, GLfloat *data);
GdkGLProc    gdk_gl_get_glGetLocalConstantFloatvEXT (void);
#define      gdk_gl_glGetLocalConstantFloatvEXT(proc, id, value, data) \
  ( ((GdkGLProc_glGetLocalConstantFloatvEXT) (proc)) (id, value, data) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_vertex_shader GdkGL_GL_EXT_vertex_shader;

struct _GdkGL_GL_EXT_vertex_shader
{
  GdkGLProc_glBeginVertexShaderEXT glBeginVertexShaderEXT;
  GdkGLProc_glEndVertexShaderEXT glEndVertexShaderEXT;
  GdkGLProc_glBindVertexShaderEXT glBindVertexShaderEXT;
  GdkGLProc_glGenVertexShadersEXT glGenVertexShadersEXT;
  GdkGLProc_glDeleteVertexShaderEXT glDeleteVertexShaderEXT;
  GdkGLProc_glShaderOp1EXT glShaderOp1EXT;
  GdkGLProc_glShaderOp2EXT glShaderOp2EXT;
  GdkGLProc_glShaderOp3EXT glShaderOp3EXT;
  GdkGLProc_glSwizzleEXT glSwizzleEXT;
  GdkGLProc_glWriteMaskEXT glWriteMaskEXT;
  GdkGLProc_glInsertComponentEXT glInsertComponentEXT;
  GdkGLProc_glExtractComponentEXT glExtractComponentEXT;
  GdkGLProc_glGenSymbolsEXT glGenSymbolsEXT;
  GdkGLProc_glSetInvariantEXT glSetInvariantEXT;
  GdkGLProc_glSetLocalConstantEXT glSetLocalConstantEXT;
  GdkGLProc_glVariantbvEXT glVariantbvEXT;
  GdkGLProc_glVariantsvEXT glVariantsvEXT;
  GdkGLProc_glVariantivEXT glVariantivEXT;
  GdkGLProc_glVariantfvEXT glVariantfvEXT;
  GdkGLProc_glVariantdvEXT glVariantdvEXT;
  GdkGLProc_glVariantubvEXT glVariantubvEXT;
  GdkGLProc_glVariantusvEXT glVariantusvEXT;
  GdkGLProc_glVariantuivEXT glVariantuivEXT;
  GdkGLProc_glVariantPointerEXT glVariantPointerEXT;
  GdkGLProc_glEnableVariantClientStateEXT glEnableVariantClientStateEXT;
  GdkGLProc_glDisableVariantClientStateEXT glDisableVariantClientStateEXT;
  GdkGLProc_glBindLightParameterEXT glBindLightParameterEXT;
  GdkGLProc_glBindMaterialParameterEXT glBindMaterialParameterEXT;
  GdkGLProc_glBindTexGenParameterEXT glBindTexGenParameterEXT;
  GdkGLProc_glBindTextureUnitParameterEXT glBindTextureUnitParameterEXT;
  GdkGLProc_glBindParameterEXT glBindParameterEXT;
  GdkGLProc_glIsVariantEnabledEXT glIsVariantEnabledEXT;
  GdkGLProc_glGetVariantBooleanvEXT glGetVariantBooleanvEXT;
  GdkGLProc_glGetVariantIntegervEXT glGetVariantIntegervEXT;
  GdkGLProc_glGetVariantFloatvEXT glGetVariantFloatvEXT;
  GdkGLProc_glGetVariantPointervEXT glGetVariantPointervEXT;
  GdkGLProc_glGetInvariantBooleanvEXT glGetInvariantBooleanvEXT;
  GdkGLProc_glGetInvariantIntegervEXT glGetInvariantIntegervEXT;
  GdkGLProc_glGetInvariantFloatvEXT glGetInvariantFloatvEXT;
  GdkGLProc_glGetLocalConstantBooleanvEXT glGetLocalConstantBooleanvEXT;
  GdkGLProc_glGetLocalConstantIntegervEXT glGetLocalConstantIntegervEXT;
  GdkGLProc_glGetLocalConstantFloatvEXT glGetLocalConstantFloatvEXT;
};

GdkGL_GL_EXT_vertex_shader *gdk_gl_get_GL_EXT_vertex_shader (void);

/*
 * GL_ATI_vertex_streams
 */

/* glVertexStream1sATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1sATI) (GLenum stream, GLshort x);
GdkGLProc    gdk_gl_get_glVertexStream1sATI (void);
#define      gdk_gl_glVertexStream1sATI(proc, stream, x) \
  ( ((GdkGLProc_glVertexStream1sATI) (proc)) (stream, x) )

/* glVertexStream1svATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1svATI) (GLenum stream, const GLshort *coords);
GdkGLProc    gdk_gl_get_glVertexStream1svATI (void);
#define      gdk_gl_glVertexStream1svATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream1svATI) (proc)) (stream, coords) )

/* glVertexStream1iATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1iATI) (GLenum stream, GLint x);
GdkGLProc    gdk_gl_get_glVertexStream1iATI (void);
#define      gdk_gl_glVertexStream1iATI(proc, stream, x) \
  ( ((GdkGLProc_glVertexStream1iATI) (proc)) (stream, x) )

/* glVertexStream1ivATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1ivATI) (GLenum stream, const GLint *coords);
GdkGLProc    gdk_gl_get_glVertexStream1ivATI (void);
#define      gdk_gl_glVertexStream1ivATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream1ivATI) (proc)) (stream, coords) )

/* glVertexStream1fATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1fATI) (GLenum stream, GLfloat x);
GdkGLProc    gdk_gl_get_glVertexStream1fATI (void);
#define      gdk_gl_glVertexStream1fATI(proc, stream, x) \
  ( ((GdkGLProc_glVertexStream1fATI) (proc)) (stream, x) )

/* glVertexStream1fvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1fvATI) (GLenum stream, const GLfloat *coords);
GdkGLProc    gdk_gl_get_glVertexStream1fvATI (void);
#define      gdk_gl_glVertexStream1fvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream1fvATI) (proc)) (stream, coords) )

/* glVertexStream1dATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1dATI) (GLenum stream, GLdouble x);
GdkGLProc    gdk_gl_get_glVertexStream1dATI (void);
#define      gdk_gl_glVertexStream1dATI(proc, stream, x) \
  ( ((GdkGLProc_glVertexStream1dATI) (proc)) (stream, x) )

/* glVertexStream1dvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream1dvATI) (GLenum stream, const GLdouble *coords);
GdkGLProc    gdk_gl_get_glVertexStream1dvATI (void);
#define      gdk_gl_glVertexStream1dvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream1dvATI) (proc)) (stream, coords) )

/* glVertexStream2sATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2sATI) (GLenum stream, GLshort x, GLshort y);
GdkGLProc    gdk_gl_get_glVertexStream2sATI (void);
#define      gdk_gl_glVertexStream2sATI(proc, stream, x, y) \
  ( ((GdkGLProc_glVertexStream2sATI) (proc)) (stream, x, y) )

/* glVertexStream2svATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2svATI) (GLenum stream, const GLshort *coords);
GdkGLProc    gdk_gl_get_glVertexStream2svATI (void);
#define      gdk_gl_glVertexStream2svATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream2svATI) (proc)) (stream, coords) )

/* glVertexStream2iATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2iATI) (GLenum stream, GLint x, GLint y);
GdkGLProc    gdk_gl_get_glVertexStream2iATI (void);
#define      gdk_gl_glVertexStream2iATI(proc, stream, x, y) \
  ( ((GdkGLProc_glVertexStream2iATI) (proc)) (stream, x, y) )

/* glVertexStream2ivATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2ivATI) (GLenum stream, const GLint *coords);
GdkGLProc    gdk_gl_get_glVertexStream2ivATI (void);
#define      gdk_gl_glVertexStream2ivATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream2ivATI) (proc)) (stream, coords) )

/* glVertexStream2fATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2fATI) (GLenum stream, GLfloat x, GLfloat y);
GdkGLProc    gdk_gl_get_glVertexStream2fATI (void);
#define      gdk_gl_glVertexStream2fATI(proc, stream, x, y) \
  ( ((GdkGLProc_glVertexStream2fATI) (proc)) (stream, x, y) )

/* glVertexStream2fvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2fvATI) (GLenum stream, const GLfloat *coords);
GdkGLProc    gdk_gl_get_glVertexStream2fvATI (void);
#define      gdk_gl_glVertexStream2fvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream2fvATI) (proc)) (stream, coords) )

/* glVertexStream2dATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2dATI) (GLenum stream, GLdouble x, GLdouble y);
GdkGLProc    gdk_gl_get_glVertexStream2dATI (void);
#define      gdk_gl_glVertexStream2dATI(proc, stream, x, y) \
  ( ((GdkGLProc_glVertexStream2dATI) (proc)) (stream, x, y) )

/* glVertexStream2dvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream2dvATI) (GLenum stream, const GLdouble *coords);
GdkGLProc    gdk_gl_get_glVertexStream2dvATI (void);
#define      gdk_gl_glVertexStream2dvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream2dvATI) (proc)) (stream, coords) )

/* glVertexStream3sATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3sATI) (GLenum stream, GLshort x, GLshort y, GLshort z);
GdkGLProc    gdk_gl_get_glVertexStream3sATI (void);
#define      gdk_gl_glVertexStream3sATI(proc, stream, x, y, z) \
  ( ((GdkGLProc_glVertexStream3sATI) (proc)) (stream, x, y, z) )

/* glVertexStream3svATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3svATI) (GLenum stream, const GLshort *coords);
GdkGLProc    gdk_gl_get_glVertexStream3svATI (void);
#define      gdk_gl_glVertexStream3svATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream3svATI) (proc)) (stream, coords) )

/* glVertexStream3iATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3iATI) (GLenum stream, GLint x, GLint y, GLint z);
GdkGLProc    gdk_gl_get_glVertexStream3iATI (void);
#define      gdk_gl_glVertexStream3iATI(proc, stream, x, y, z) \
  ( ((GdkGLProc_glVertexStream3iATI) (proc)) (stream, x, y, z) )

/* glVertexStream3ivATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3ivATI) (GLenum stream, const GLint *coords);
GdkGLProc    gdk_gl_get_glVertexStream3ivATI (void);
#define      gdk_gl_glVertexStream3ivATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream3ivATI) (proc)) (stream, coords) )

/* glVertexStream3fATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3fATI) (GLenum stream, GLfloat x, GLfloat y, GLfloat z);
GdkGLProc    gdk_gl_get_glVertexStream3fATI (void);
#define      gdk_gl_glVertexStream3fATI(proc, stream, x, y, z) \
  ( ((GdkGLProc_glVertexStream3fATI) (proc)) (stream, x, y, z) )

/* glVertexStream3fvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3fvATI) (GLenum stream, const GLfloat *coords);
GdkGLProc    gdk_gl_get_glVertexStream3fvATI (void);
#define      gdk_gl_glVertexStream3fvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream3fvATI) (proc)) (stream, coords) )

/* glVertexStream3dATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3dATI) (GLenum stream, GLdouble x, GLdouble y, GLdouble z);
GdkGLProc    gdk_gl_get_glVertexStream3dATI (void);
#define      gdk_gl_glVertexStream3dATI(proc, stream, x, y, z) \
  ( ((GdkGLProc_glVertexStream3dATI) (proc)) (stream, x, y, z) )

/* glVertexStream3dvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream3dvATI) (GLenum stream, const GLdouble *coords);
GdkGLProc    gdk_gl_get_glVertexStream3dvATI (void);
#define      gdk_gl_glVertexStream3dvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream3dvATI) (proc)) (stream, coords) )

/* glVertexStream4sATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4sATI) (GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w);
GdkGLProc    gdk_gl_get_glVertexStream4sATI (void);
#define      gdk_gl_glVertexStream4sATI(proc, stream, x, y, z, w) \
  ( ((GdkGLProc_glVertexStream4sATI) (proc)) (stream, x, y, z, w) )

/* glVertexStream4svATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4svATI) (GLenum stream, const GLshort *coords);
GdkGLProc    gdk_gl_get_glVertexStream4svATI (void);
#define      gdk_gl_glVertexStream4svATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream4svATI) (proc)) (stream, coords) )

/* glVertexStream4iATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4iATI) (GLenum stream, GLint x, GLint y, GLint z, GLint w);
GdkGLProc    gdk_gl_get_glVertexStream4iATI (void);
#define      gdk_gl_glVertexStream4iATI(proc, stream, x, y, z, w) \
  ( ((GdkGLProc_glVertexStream4iATI) (proc)) (stream, x, y, z, w) )

/* glVertexStream4ivATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4ivATI) (GLenum stream, const GLint *coords);
GdkGLProc    gdk_gl_get_glVertexStream4ivATI (void);
#define      gdk_gl_glVertexStream4ivATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream4ivATI) (proc)) (stream, coords) )

/* glVertexStream4fATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4fATI) (GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glVertexStream4fATI (void);
#define      gdk_gl_glVertexStream4fATI(proc, stream, x, y, z, w) \
  ( ((GdkGLProc_glVertexStream4fATI) (proc)) (stream, x, y, z, w) )

/* glVertexStream4fvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4fvATI) (GLenum stream, const GLfloat *coords);
GdkGLProc    gdk_gl_get_glVertexStream4fvATI (void);
#define      gdk_gl_glVertexStream4fvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream4fvATI) (proc)) (stream, coords) )

/* glVertexStream4dATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4dATI) (GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glVertexStream4dATI (void);
#define      gdk_gl_glVertexStream4dATI(proc, stream, x, y, z, w) \
  ( ((GdkGLProc_glVertexStream4dATI) (proc)) (stream, x, y, z, w) )

/* glVertexStream4dvATI */
typedef void (APIENTRY * GdkGLProc_glVertexStream4dvATI) (GLenum stream, const GLdouble *coords);
GdkGLProc    gdk_gl_get_glVertexStream4dvATI (void);
#define      gdk_gl_glVertexStream4dvATI(proc, stream, coords) \
  ( ((GdkGLProc_glVertexStream4dvATI) (proc)) (stream, coords) )

/* glNormalStream3bATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3bATI) (GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz);
GdkGLProc    gdk_gl_get_glNormalStream3bATI (void);
#define      gdk_gl_glNormalStream3bATI(proc, stream, nx, ny, nz) \
  ( ((GdkGLProc_glNormalStream3bATI) (proc)) (stream, nx, ny, nz) )

/* glNormalStream3bvATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3bvATI) (GLenum stream, const GLbyte *coords);
GdkGLProc    gdk_gl_get_glNormalStream3bvATI (void);
#define      gdk_gl_glNormalStream3bvATI(proc, stream, coords) \
  ( ((GdkGLProc_glNormalStream3bvATI) (proc)) (stream, coords) )

/* glNormalStream3sATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3sATI) (GLenum stream, GLshort nx, GLshort ny, GLshort nz);
GdkGLProc    gdk_gl_get_glNormalStream3sATI (void);
#define      gdk_gl_glNormalStream3sATI(proc, stream, nx, ny, nz) \
  ( ((GdkGLProc_glNormalStream3sATI) (proc)) (stream, nx, ny, nz) )

/* glNormalStream3svATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3svATI) (GLenum stream, const GLshort *coords);
GdkGLProc    gdk_gl_get_glNormalStream3svATI (void);
#define      gdk_gl_glNormalStream3svATI(proc, stream, coords) \
  ( ((GdkGLProc_glNormalStream3svATI) (proc)) (stream, coords) )

/* glNormalStream3iATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3iATI) (GLenum stream, GLint nx, GLint ny, GLint nz);
GdkGLProc    gdk_gl_get_glNormalStream3iATI (void);
#define      gdk_gl_glNormalStream3iATI(proc, stream, nx, ny, nz) \
  ( ((GdkGLProc_glNormalStream3iATI) (proc)) (stream, nx, ny, nz) )

/* glNormalStream3ivATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3ivATI) (GLenum stream, const GLint *coords);
GdkGLProc    gdk_gl_get_glNormalStream3ivATI (void);
#define      gdk_gl_glNormalStream3ivATI(proc, stream, coords) \
  ( ((GdkGLProc_glNormalStream3ivATI) (proc)) (stream, coords) )

/* glNormalStream3fATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3fATI) (GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz);
GdkGLProc    gdk_gl_get_glNormalStream3fATI (void);
#define      gdk_gl_glNormalStream3fATI(proc, stream, nx, ny, nz) \
  ( ((GdkGLProc_glNormalStream3fATI) (proc)) (stream, nx, ny, nz) )

/* glNormalStream3fvATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3fvATI) (GLenum stream, const GLfloat *coords);
GdkGLProc    gdk_gl_get_glNormalStream3fvATI (void);
#define      gdk_gl_glNormalStream3fvATI(proc, stream, coords) \
  ( ((GdkGLProc_glNormalStream3fvATI) (proc)) (stream, coords) )

/* glNormalStream3dATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3dATI) (GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz);
GdkGLProc    gdk_gl_get_glNormalStream3dATI (void);
#define      gdk_gl_glNormalStream3dATI(proc, stream, nx, ny, nz) \
  ( ((GdkGLProc_glNormalStream3dATI) (proc)) (stream, nx, ny, nz) )

/* glNormalStream3dvATI */
typedef void (APIENTRY * GdkGLProc_glNormalStream3dvATI) (GLenum stream, const GLdouble *coords);
GdkGLProc    gdk_gl_get_glNormalStream3dvATI (void);
#define      gdk_gl_glNormalStream3dvATI(proc, stream, coords) \
  ( ((GdkGLProc_glNormalStream3dvATI) (proc)) (stream, coords) )

/* glClientActiveVertexStreamATI */
typedef void (APIENTRY * GdkGLProc_glClientActiveVertexStreamATI) (GLenum stream);
GdkGLProc    gdk_gl_get_glClientActiveVertexStreamATI (void);
#define      gdk_gl_glClientActiveVertexStreamATI(proc, stream) \
  ( ((GdkGLProc_glClientActiveVertexStreamATI) (proc)) (stream) )

/* glVertexBlendEnviATI */
typedef void (APIENTRY * GdkGLProc_glVertexBlendEnviATI) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glVertexBlendEnviATI (void);
#define      gdk_gl_glVertexBlendEnviATI(proc, pname, param) \
  ( ((GdkGLProc_glVertexBlendEnviATI) (proc)) (pname, param) )

/* glVertexBlendEnvfATI */
typedef void (APIENTRY * GdkGLProc_glVertexBlendEnvfATI) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glVertexBlendEnvfATI (void);
#define      gdk_gl_glVertexBlendEnvfATI(proc, pname, param) \
  ( ((GdkGLProc_glVertexBlendEnvfATI) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_vertex_streams GdkGL_GL_ATI_vertex_streams;

struct _GdkGL_GL_ATI_vertex_streams
{
  GdkGLProc_glVertexStream1sATI glVertexStream1sATI;
  GdkGLProc_glVertexStream1svATI glVertexStream1svATI;
  GdkGLProc_glVertexStream1iATI glVertexStream1iATI;
  GdkGLProc_glVertexStream1ivATI glVertexStream1ivATI;
  GdkGLProc_glVertexStream1fATI glVertexStream1fATI;
  GdkGLProc_glVertexStream1fvATI glVertexStream1fvATI;
  GdkGLProc_glVertexStream1dATI glVertexStream1dATI;
  GdkGLProc_glVertexStream1dvATI glVertexStream1dvATI;
  GdkGLProc_glVertexStream2sATI glVertexStream2sATI;
  GdkGLProc_glVertexStream2svATI glVertexStream2svATI;
  GdkGLProc_glVertexStream2iATI glVertexStream2iATI;
  GdkGLProc_glVertexStream2ivATI glVertexStream2ivATI;
  GdkGLProc_glVertexStream2fATI glVertexStream2fATI;
  GdkGLProc_glVertexStream2fvATI glVertexStream2fvATI;
  GdkGLProc_glVertexStream2dATI glVertexStream2dATI;
  GdkGLProc_glVertexStream2dvATI glVertexStream2dvATI;
  GdkGLProc_glVertexStream3sATI glVertexStream3sATI;
  GdkGLProc_glVertexStream3svATI glVertexStream3svATI;
  GdkGLProc_glVertexStream3iATI glVertexStream3iATI;
  GdkGLProc_glVertexStream3ivATI glVertexStream3ivATI;
  GdkGLProc_glVertexStream3fATI glVertexStream3fATI;
  GdkGLProc_glVertexStream3fvATI glVertexStream3fvATI;
  GdkGLProc_glVertexStream3dATI glVertexStream3dATI;
  GdkGLProc_glVertexStream3dvATI glVertexStream3dvATI;
  GdkGLProc_glVertexStream4sATI glVertexStream4sATI;
  GdkGLProc_glVertexStream4svATI glVertexStream4svATI;
  GdkGLProc_glVertexStream4iATI glVertexStream4iATI;
  GdkGLProc_glVertexStream4ivATI glVertexStream4ivATI;
  GdkGLProc_glVertexStream4fATI glVertexStream4fATI;
  GdkGLProc_glVertexStream4fvATI glVertexStream4fvATI;
  GdkGLProc_glVertexStream4dATI glVertexStream4dATI;
  GdkGLProc_glVertexStream4dvATI glVertexStream4dvATI;
  GdkGLProc_glNormalStream3bATI glNormalStream3bATI;
  GdkGLProc_glNormalStream3bvATI glNormalStream3bvATI;
  GdkGLProc_glNormalStream3sATI glNormalStream3sATI;
  GdkGLProc_glNormalStream3svATI glNormalStream3svATI;
  GdkGLProc_glNormalStream3iATI glNormalStream3iATI;
  GdkGLProc_glNormalStream3ivATI glNormalStream3ivATI;
  GdkGLProc_glNormalStream3fATI glNormalStream3fATI;
  GdkGLProc_glNormalStream3fvATI glNormalStream3fvATI;
  GdkGLProc_glNormalStream3dATI glNormalStream3dATI;
  GdkGLProc_glNormalStream3dvATI glNormalStream3dvATI;
  GdkGLProc_glClientActiveVertexStreamATI glClientActiveVertexStreamATI;
  GdkGLProc_glVertexBlendEnviATI glVertexBlendEnviATI;
  GdkGLProc_glVertexBlendEnvfATI glVertexBlendEnvfATI;
};

GdkGL_GL_ATI_vertex_streams *gdk_gl_get_GL_ATI_vertex_streams (void);

/*
 * GL_ATI_element_array
 */

/* glElementPointerATI */
typedef void (APIENTRY * GdkGLProc_glElementPointerATI) (GLenum type, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glElementPointerATI (void);
#define      gdk_gl_glElementPointerATI(proc, type, pointer) \
  ( ((GdkGLProc_glElementPointerATI) (proc)) (type, pointer) )

/* glDrawElementArrayATI */
typedef void (APIENTRY * GdkGLProc_glDrawElementArrayATI) (GLenum mode, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawElementArrayATI (void);
#define      gdk_gl_glDrawElementArrayATI(proc, mode, count) \
  ( ((GdkGLProc_glDrawElementArrayATI) (proc)) (mode, count) )

/* glDrawRangeElementArrayATI */
typedef void (APIENTRY * GdkGLProc_glDrawRangeElementArrayATI) (GLenum mode, GLuint start, GLuint end, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawRangeElementArrayATI (void);
#define      gdk_gl_glDrawRangeElementArrayATI(proc, mode, start, end, count) \
  ( ((GdkGLProc_glDrawRangeElementArrayATI) (proc)) (mode, start, end, count) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_element_array GdkGL_GL_ATI_element_array;

struct _GdkGL_GL_ATI_element_array
{
  GdkGLProc_glElementPointerATI glElementPointerATI;
  GdkGLProc_glDrawElementArrayATI glDrawElementArrayATI;
  GdkGLProc_glDrawRangeElementArrayATI glDrawRangeElementArrayATI;
};

GdkGL_GL_ATI_element_array *gdk_gl_get_GL_ATI_element_array (void);

/*
 * GL_SUN_mesh_array
 */

/* glDrawMeshArraysSUN */
typedef void (APIENTRY * GdkGLProc_glDrawMeshArraysSUN) (GLenum mode, GLint first, GLsizei count, GLsizei width);
GdkGLProc    gdk_gl_get_glDrawMeshArraysSUN (void);
#define      gdk_gl_glDrawMeshArraysSUN(proc, mode, first, count, width) \
  ( ((GdkGLProc_glDrawMeshArraysSUN) (proc)) (mode, first, count, width) )

/* proc struct */

typedef struct _GdkGL_GL_SUN_mesh_array GdkGL_GL_SUN_mesh_array;

struct _GdkGL_GL_SUN_mesh_array
{
  GdkGLProc_glDrawMeshArraysSUN glDrawMeshArraysSUN;
};

GdkGL_GL_SUN_mesh_array *gdk_gl_get_GL_SUN_mesh_array (void);

/*
 * GL_NV_occlusion_query
 */

/* glGenOcclusionQueriesNV */
typedef void (APIENTRY * GdkGLProc_glGenOcclusionQueriesNV) (GLsizei n, GLuint *ids);
GdkGLProc    gdk_gl_get_glGenOcclusionQueriesNV (void);
#define      gdk_gl_glGenOcclusionQueriesNV(proc, n, ids) \
  ( ((GdkGLProc_glGenOcclusionQueriesNV) (proc)) (n, ids) )

/* glDeleteOcclusionQueriesNV */
typedef void (APIENTRY * GdkGLProc_glDeleteOcclusionQueriesNV) (GLsizei n, const GLuint *ids);
GdkGLProc    gdk_gl_get_glDeleteOcclusionQueriesNV (void);
#define      gdk_gl_glDeleteOcclusionQueriesNV(proc, n, ids) \
  ( ((GdkGLProc_glDeleteOcclusionQueriesNV) (proc)) (n, ids) )

/* glIsOcclusionQueryNV */
typedef GLboolean (APIENTRY * GdkGLProc_glIsOcclusionQueryNV) (GLuint id);
GdkGLProc    gdk_gl_get_glIsOcclusionQueryNV (void);
#define      gdk_gl_glIsOcclusionQueryNV(proc, id) \
  ( ((GdkGLProc_glIsOcclusionQueryNV) (proc)) (id) )

/* glBeginOcclusionQueryNV */
typedef void (APIENTRY * GdkGLProc_glBeginOcclusionQueryNV) (GLuint id);
GdkGLProc    gdk_gl_get_glBeginOcclusionQueryNV (void);
#define      gdk_gl_glBeginOcclusionQueryNV(proc, id) \
  ( ((GdkGLProc_glBeginOcclusionQueryNV) (proc)) (id) )

/* glEndOcclusionQueryNV */
typedef void (APIENTRY * GdkGLProc_glEndOcclusionQueryNV) (void);
GdkGLProc    gdk_gl_get_glEndOcclusionQueryNV (void);
#define      gdk_gl_glEndOcclusionQueryNV(proc) \
  ( ((GdkGLProc_glEndOcclusionQueryNV) (proc)) () )

/* glGetOcclusionQueryivNV */
typedef void (APIENTRY * GdkGLProc_glGetOcclusionQueryivNV) (GLuint id, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetOcclusionQueryivNV (void);
#define      gdk_gl_glGetOcclusionQueryivNV(proc, id, pname, params) \
  ( ((GdkGLProc_glGetOcclusionQueryivNV) (proc)) (id, pname, params) )

/* glGetOcclusionQueryuivNV */
typedef void (APIENTRY * GdkGLProc_glGetOcclusionQueryuivNV) (GLuint id, GLenum pname, GLuint *params);
GdkGLProc    gdk_gl_get_glGetOcclusionQueryuivNV (void);
#define      gdk_gl_glGetOcclusionQueryuivNV(proc, id, pname, params) \
  ( ((GdkGLProc_glGetOcclusionQueryuivNV) (proc)) (id, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_NV_occlusion_query GdkGL_GL_NV_occlusion_query;

struct _GdkGL_GL_NV_occlusion_query
{
  GdkGLProc_glGenOcclusionQueriesNV glGenOcclusionQueriesNV;
  GdkGLProc_glDeleteOcclusionQueriesNV glDeleteOcclusionQueriesNV;
  GdkGLProc_glIsOcclusionQueryNV glIsOcclusionQueryNV;
  GdkGLProc_glBeginOcclusionQueryNV glBeginOcclusionQueryNV;
  GdkGLProc_glEndOcclusionQueryNV glEndOcclusionQueryNV;
  GdkGLProc_glGetOcclusionQueryivNV glGetOcclusionQueryivNV;
  GdkGLProc_glGetOcclusionQueryuivNV glGetOcclusionQueryuivNV;
};

GdkGL_GL_NV_occlusion_query *gdk_gl_get_GL_NV_occlusion_query (void);

/*
 * GL_NV_point_sprite
 */

/* glPointParameteriNV */
typedef void (APIENTRY * GdkGLProc_glPointParameteriNV) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glPointParameteriNV (void);
#define      gdk_gl_glPointParameteriNV(proc, pname, param) \
  ( ((GdkGLProc_glPointParameteriNV) (proc)) (pname, param) )

/* glPointParameterivNV */
typedef void (APIENTRY * GdkGLProc_glPointParameterivNV) (GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glPointParameterivNV (void);
#define      gdk_gl_glPointParameterivNV(proc, pname, params) \
  ( ((GdkGLProc_glPointParameterivNV) (proc)) (pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_NV_point_sprite GdkGL_GL_NV_point_sprite;

struct _GdkGL_GL_NV_point_sprite
{
  GdkGLProc_glPointParameteriNV glPointParameteriNV;
  GdkGLProc_glPointParameterivNV glPointParameterivNV;
};

GdkGL_GL_NV_point_sprite *gdk_gl_get_GL_NV_point_sprite (void);

/*
 * GL_EXT_stencil_two_side
 */

/* glActiveStencilFaceEXT */
typedef void (APIENTRY * GdkGLProc_glActiveStencilFaceEXT) (GLenum face);
GdkGLProc    gdk_gl_get_glActiveStencilFaceEXT (void);
#define      gdk_gl_glActiveStencilFaceEXT(proc, face) \
  ( ((GdkGLProc_glActiveStencilFaceEXT) (proc)) (face) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_stencil_two_side GdkGL_GL_EXT_stencil_two_side;

struct _GdkGL_GL_EXT_stencil_two_side
{
  GdkGLProc_glActiveStencilFaceEXT glActiveStencilFaceEXT;
};

GdkGL_GL_EXT_stencil_two_side *gdk_gl_get_GL_EXT_stencil_two_side (void);

/*
 * GL_APPLE_element_array
 */

/* glElementPointerAPPLE */
typedef void (APIENTRY * GdkGLProc_glElementPointerAPPLE) (GLenum type, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glElementPointerAPPLE (void);
#define      gdk_gl_glElementPointerAPPLE(proc, type, pointer) \
  ( ((GdkGLProc_glElementPointerAPPLE) (proc)) (type, pointer) )

/* glDrawElementArrayAPPLE */
typedef void (APIENTRY * GdkGLProc_glDrawElementArrayAPPLE) (GLenum mode, GLint first, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawElementArrayAPPLE (void);
#define      gdk_gl_glDrawElementArrayAPPLE(proc, mode, first, count) \
  ( ((GdkGLProc_glDrawElementArrayAPPLE) (proc)) (mode, first, count) )

/* glDrawRangeElementArrayAPPLE */
typedef void (APIENTRY * GdkGLProc_glDrawRangeElementArrayAPPLE) (GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawRangeElementArrayAPPLE (void);
#define      gdk_gl_glDrawRangeElementArrayAPPLE(proc, mode, start, end, first, count) \
  ( ((GdkGLProc_glDrawRangeElementArrayAPPLE) (proc)) (mode, start, end, first, count) )

/* glMultiDrawElementArrayAPPLE */
typedef void (APIENTRY * GdkGLProc_glMultiDrawElementArrayAPPLE) (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawElementArrayAPPLE (void);
#define      gdk_gl_glMultiDrawElementArrayAPPLE(proc, mode, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawElementArrayAPPLE) (proc)) (mode, first, count, primcount) )

/* glMultiDrawRangeElementArrayAPPLE */
typedef void (APIENTRY * GdkGLProc_glMultiDrawRangeElementArrayAPPLE) (GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawRangeElementArrayAPPLE (void);
#define      gdk_gl_glMultiDrawRangeElementArrayAPPLE(proc, mode, start, end, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawRangeElementArrayAPPLE) (proc)) (mode, start, end, first, count, primcount) )

/* proc struct */

typedef struct _GdkGL_GL_APPLE_element_array GdkGL_GL_APPLE_element_array;

struct _GdkGL_GL_APPLE_element_array
{
  GdkGLProc_glElementPointerAPPLE glElementPointerAPPLE;
  GdkGLProc_glDrawElementArrayAPPLE glDrawElementArrayAPPLE;
  GdkGLProc_glDrawRangeElementArrayAPPLE glDrawRangeElementArrayAPPLE;
  GdkGLProc_glMultiDrawElementArrayAPPLE glMultiDrawElementArrayAPPLE;
  GdkGLProc_glMultiDrawRangeElementArrayAPPLE glMultiDrawRangeElementArrayAPPLE;
};

GdkGL_GL_APPLE_element_array *gdk_gl_get_GL_APPLE_element_array (void);

/*
 * GL_APPLE_fence
 */

/* glGenFencesAPPLE */
typedef void (APIENTRY * GdkGLProc_glGenFencesAPPLE) (GLsizei n, GLuint *fences);
GdkGLProc    gdk_gl_get_glGenFencesAPPLE (void);
#define      gdk_gl_glGenFencesAPPLE(proc, n, fences) \
  ( ((GdkGLProc_glGenFencesAPPLE) (proc)) (n, fences) )

/* glDeleteFencesAPPLE */
typedef void (APIENTRY * GdkGLProc_glDeleteFencesAPPLE) (GLsizei n, const GLuint *fences);
GdkGLProc    gdk_gl_get_glDeleteFencesAPPLE (void);
#define      gdk_gl_glDeleteFencesAPPLE(proc, n, fences) \
  ( ((GdkGLProc_glDeleteFencesAPPLE) (proc)) (n, fences) )

/* glSetFenceAPPLE */
typedef void (APIENTRY * GdkGLProc_glSetFenceAPPLE) (GLuint fence);
GdkGLProc    gdk_gl_get_glSetFenceAPPLE (void);
#define      gdk_gl_glSetFenceAPPLE(proc, fence) \
  ( ((GdkGLProc_glSetFenceAPPLE) (proc)) (fence) )

/* glIsFenceAPPLE */
typedef GLboolean (APIENTRY * GdkGLProc_glIsFenceAPPLE) (GLuint fence);
GdkGLProc    gdk_gl_get_glIsFenceAPPLE (void);
#define      gdk_gl_glIsFenceAPPLE(proc, fence) \
  ( ((GdkGLProc_glIsFenceAPPLE) (proc)) (fence) )

/* glTestFenceAPPLE */
typedef GLboolean (APIENTRY * GdkGLProc_glTestFenceAPPLE) (GLuint fence);
GdkGLProc    gdk_gl_get_glTestFenceAPPLE (void);
#define      gdk_gl_glTestFenceAPPLE(proc, fence) \
  ( ((GdkGLProc_glTestFenceAPPLE) (proc)) (fence) )

/* glFinishFenceAPPLE */
typedef void (APIENTRY * GdkGLProc_glFinishFenceAPPLE) (GLuint fence);
GdkGLProc    gdk_gl_get_glFinishFenceAPPLE (void);
#define      gdk_gl_glFinishFenceAPPLE(proc, fence) \
  ( ((GdkGLProc_glFinishFenceAPPLE) (proc)) (fence) )

/* glTestObjectAPPLE */
typedef GLboolean (APIENTRY * GdkGLProc_glTestObjectAPPLE) (GLenum object, GLuint name);
GdkGLProc    gdk_gl_get_glTestObjectAPPLE (void);
#define      gdk_gl_glTestObjectAPPLE(proc, object, name) \
  ( ((GdkGLProc_glTestObjectAPPLE) (proc)) (object, name) )

/* glFinishObjectAPPLE */
typedef void (APIENTRY * GdkGLProc_glFinishObjectAPPLE) (GLenum object, GLint name);
GdkGLProc    gdk_gl_get_glFinishObjectAPPLE (void);
#define      gdk_gl_glFinishObjectAPPLE(proc, object, name) \
  ( ((GdkGLProc_glFinishObjectAPPLE) (proc)) (object, name) )

/* proc struct */

typedef struct _GdkGL_GL_APPLE_fence GdkGL_GL_APPLE_fence;

struct _GdkGL_GL_APPLE_fence
{
  GdkGLProc_glGenFencesAPPLE glGenFencesAPPLE;
  GdkGLProc_glDeleteFencesAPPLE glDeleteFencesAPPLE;
  GdkGLProc_glSetFenceAPPLE glSetFenceAPPLE;
  GdkGLProc_glIsFenceAPPLE glIsFenceAPPLE;
  GdkGLProc_glTestFenceAPPLE glTestFenceAPPLE;
  GdkGLProc_glFinishFenceAPPLE glFinishFenceAPPLE;
  GdkGLProc_glTestObjectAPPLE glTestObjectAPPLE;
  GdkGLProc_glFinishObjectAPPLE glFinishObjectAPPLE;
};

GdkGL_GL_APPLE_fence *gdk_gl_get_GL_APPLE_fence (void);

/*
 * GL_APPLE_vertex_array_object
 */

/* glBindVertexArrayAPPLE */
typedef void (APIENTRY * GdkGLProc_glBindVertexArrayAPPLE) (GLuint array);
GdkGLProc    gdk_gl_get_glBindVertexArrayAPPLE (void);
#define      gdk_gl_glBindVertexArrayAPPLE(proc, array) \
  ( ((GdkGLProc_glBindVertexArrayAPPLE) (proc)) (array) )

/* glDeleteVertexArraysAPPLE */
typedef void (APIENTRY * GdkGLProc_glDeleteVertexArraysAPPLE) (GLsizei n, const GLuint *arrays);
GdkGLProc    gdk_gl_get_glDeleteVertexArraysAPPLE (void);
#define      gdk_gl_glDeleteVertexArraysAPPLE(proc, n, arrays) \
  ( ((GdkGLProc_glDeleteVertexArraysAPPLE) (proc)) (n, arrays) )

/* glGenVertexArraysAPPLE */
typedef void (APIENTRY * GdkGLProc_glGenVertexArraysAPPLE) (GLsizei n, const GLuint *arrays);
GdkGLProc    gdk_gl_get_glGenVertexArraysAPPLE (void);
#define      gdk_gl_glGenVertexArraysAPPLE(proc, n, arrays) \
  ( ((GdkGLProc_glGenVertexArraysAPPLE) (proc)) (n, arrays) )

/* glIsVertexArrayAPPLE */
typedef GLboolean (APIENTRY * GdkGLProc_glIsVertexArrayAPPLE) (GLuint array);
GdkGLProc    gdk_gl_get_glIsVertexArrayAPPLE (void);
#define      gdk_gl_glIsVertexArrayAPPLE(proc, array) \
  ( ((GdkGLProc_glIsVertexArrayAPPLE) (proc)) (array) )

/* proc struct */

typedef struct _GdkGL_GL_APPLE_vertex_array_object GdkGL_GL_APPLE_vertex_array_object;

struct _GdkGL_GL_APPLE_vertex_array_object
{
  GdkGLProc_glBindVertexArrayAPPLE glBindVertexArrayAPPLE;
  GdkGLProc_glDeleteVertexArraysAPPLE glDeleteVertexArraysAPPLE;
  GdkGLProc_glGenVertexArraysAPPLE glGenVertexArraysAPPLE;
  GdkGLProc_glIsVertexArrayAPPLE glIsVertexArrayAPPLE;
};

GdkGL_GL_APPLE_vertex_array_object *gdk_gl_get_GL_APPLE_vertex_array_object (void);

/*
 * GL_APPLE_vertex_array_range
 */

/* glVertexArrayRangeAPPLE */
typedef void (APIENTRY * GdkGLProc_glVertexArrayRangeAPPLE) (GLsizei length, GLvoid *pointer);
GdkGLProc    gdk_gl_get_glVertexArrayRangeAPPLE (void);
#define      gdk_gl_glVertexArrayRangeAPPLE(proc, length, pointer) \
  ( ((GdkGLProc_glVertexArrayRangeAPPLE) (proc)) (length, pointer) )

/* glFlushVertexArrayRangeAPPLE */
typedef void (APIENTRY * GdkGLProc_glFlushVertexArrayRangeAPPLE) (GLsizei length, GLvoid *pointer);
GdkGLProc    gdk_gl_get_glFlushVertexArrayRangeAPPLE (void);
#define      gdk_gl_glFlushVertexArrayRangeAPPLE(proc, length, pointer) \
  ( ((GdkGLProc_glFlushVertexArrayRangeAPPLE) (proc)) (length, pointer) )

/* glVertexArrayParameteriAPPLE */
typedef void (APIENTRY * GdkGLProc_glVertexArrayParameteriAPPLE) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glVertexArrayParameteriAPPLE (void);
#define      gdk_gl_glVertexArrayParameteriAPPLE(proc, pname, param) \
  ( ((GdkGLProc_glVertexArrayParameteriAPPLE) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_APPLE_vertex_array_range GdkGL_GL_APPLE_vertex_array_range;

struct _GdkGL_GL_APPLE_vertex_array_range
{
  GdkGLProc_glVertexArrayRangeAPPLE glVertexArrayRangeAPPLE;
  GdkGLProc_glFlushVertexArrayRangeAPPLE glFlushVertexArrayRangeAPPLE;
  GdkGLProc_glVertexArrayParameteriAPPLE glVertexArrayParameteriAPPLE;
};

GdkGL_GL_APPLE_vertex_array_range *gdk_gl_get_GL_APPLE_vertex_array_range (void);

/*
 * GL_ATI_draw_buffers
 */

/* glDrawBuffersATI */
typedef void (APIENTRY * GdkGLProc_glDrawBuffersATI) (GLsizei n, const GLenum *bufs);
GdkGLProc    gdk_gl_get_glDrawBuffersATI (void);
#define      gdk_gl_glDrawBuffersATI(proc, n, bufs) \
  ( ((GdkGLProc_glDrawBuffersATI) (proc)) (n, bufs) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_draw_buffers GdkGL_GL_ATI_draw_buffers;

struct _GdkGL_GL_ATI_draw_buffers
{
  GdkGLProc_glDrawBuffersATI glDrawBuffersATI;
};

GdkGL_GL_ATI_draw_buffers *gdk_gl_get_GL_ATI_draw_buffers (void);

/*
 * GL_NV_fragment_program
 */

/* glProgramNamedParameter4fNV */
typedef void (APIENTRY * GdkGLProc_glProgramNamedParameter4fNV) (GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
GdkGLProc    gdk_gl_get_glProgramNamedParameter4fNV (void);
#define      gdk_gl_glProgramNamedParameter4fNV(proc, id, len, name, x, y, z, w) \
  ( ((GdkGLProc_glProgramNamedParameter4fNV) (proc)) (id, len, name, x, y, z, w) )

/* glProgramNamedParameter4dNV */
typedef void (APIENTRY * GdkGLProc_glProgramNamedParameter4dNV) (GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
GdkGLProc    gdk_gl_get_glProgramNamedParameter4dNV (void);
#define      gdk_gl_glProgramNamedParameter4dNV(proc, id, len, name, x, y, z, w) \
  ( ((GdkGLProc_glProgramNamedParameter4dNV) (proc)) (id, len, name, x, y, z, w) )

/* glProgramNamedParameter4fvNV */
typedef void (APIENTRY * GdkGLProc_glProgramNamedParameter4fvNV) (GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v);
GdkGLProc    gdk_gl_get_glProgramNamedParameter4fvNV (void);
#define      gdk_gl_glProgramNamedParameter4fvNV(proc, id, len, name, v) \
  ( ((GdkGLProc_glProgramNamedParameter4fvNV) (proc)) (id, len, name, v) )

/* glProgramNamedParameter4dvNV */
typedef void (APIENTRY * GdkGLProc_glProgramNamedParameter4dvNV) (GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v);
GdkGLProc    gdk_gl_get_glProgramNamedParameter4dvNV (void);
#define      gdk_gl_glProgramNamedParameter4dvNV(proc, id, len, name, v) \
  ( ((GdkGLProc_glProgramNamedParameter4dvNV) (proc)) (id, len, name, v) )

/* glGetProgramNamedParameterfvNV */
typedef void (APIENTRY * GdkGLProc_glGetProgramNamedParameterfvNV) (GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetProgramNamedParameterfvNV (void);
#define      gdk_gl_glGetProgramNamedParameterfvNV(proc, id, len, name, params) \
  ( ((GdkGLProc_glGetProgramNamedParameterfvNV) (proc)) (id, len, name, params) )

/* glGetProgramNamedParameterdvNV */
typedef void (APIENTRY * GdkGLProc_glGetProgramNamedParameterdvNV) (GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);
GdkGLProc    gdk_gl_get_glGetProgramNamedParameterdvNV (void);
#define      gdk_gl_glGetProgramNamedParameterdvNV(proc, id, len, name, params) \
  ( ((GdkGLProc_glGetProgramNamedParameterdvNV) (proc)) (id, len, name, params) )

/* proc struct */

typedef struct _GdkGL_GL_NV_fragment_program GdkGL_GL_NV_fragment_program;

struct _GdkGL_GL_NV_fragment_program
{
  GdkGLProc_glProgramNamedParameter4fNV glProgramNamedParameter4fNV;
  GdkGLProc_glProgramNamedParameter4dNV glProgramNamedParameter4dNV;
  GdkGLProc_glProgramNamedParameter4fvNV glProgramNamedParameter4fvNV;
  GdkGLProc_glProgramNamedParameter4dvNV glProgramNamedParameter4dvNV;
  GdkGLProc_glGetProgramNamedParameterfvNV glGetProgramNamedParameterfvNV;
  GdkGLProc_glGetProgramNamedParameterdvNV glGetProgramNamedParameterdvNV;
};

GdkGL_GL_NV_fragment_program *gdk_gl_get_GL_NV_fragment_program (void);

/*
 * GL_NV_half_float
 */

/* glVertex2hNV */
typedef void (APIENTRY * GdkGLProc_glVertex2hNV) (GLhalfNV x, GLhalfNV y);
GdkGLProc    gdk_gl_get_glVertex2hNV (void);
#define      gdk_gl_glVertex2hNV(proc, x, y) \
  ( ((GdkGLProc_glVertex2hNV) (proc)) (x, y) )

/* glVertex2hvNV */
typedef void (APIENTRY * GdkGLProc_glVertex2hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertex2hvNV (void);
#define      gdk_gl_glVertex2hvNV(proc, v) \
  ( ((GdkGLProc_glVertex2hvNV) (proc)) (v) )

/* glVertex3hNV */
typedef void (APIENTRY * GdkGLProc_glVertex3hNV) (GLhalfNV x, GLhalfNV y, GLhalfNV z);
GdkGLProc    gdk_gl_get_glVertex3hNV (void);
#define      gdk_gl_glVertex3hNV(proc, x, y, z) \
  ( ((GdkGLProc_glVertex3hNV) (proc)) (x, y, z) )

/* glVertex3hvNV */
typedef void (APIENTRY * GdkGLProc_glVertex3hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertex3hvNV (void);
#define      gdk_gl_glVertex3hvNV(proc, v) \
  ( ((GdkGLProc_glVertex3hvNV) (proc)) (v) )

/* glVertex4hNV */
typedef void (APIENTRY * GdkGLProc_glVertex4hNV) (GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w);
GdkGLProc    gdk_gl_get_glVertex4hNV (void);
#define      gdk_gl_glVertex4hNV(proc, x, y, z, w) \
  ( ((GdkGLProc_glVertex4hNV) (proc)) (x, y, z, w) )

/* glVertex4hvNV */
typedef void (APIENTRY * GdkGLProc_glVertex4hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertex4hvNV (void);
#define      gdk_gl_glVertex4hvNV(proc, v) \
  ( ((GdkGLProc_glVertex4hvNV) (proc)) (v) )

/* glNormal3hNV */
typedef void (APIENTRY * GdkGLProc_glNormal3hNV) (GLhalfNV nx, GLhalfNV ny, GLhalfNV nz);
GdkGLProc    gdk_gl_get_glNormal3hNV (void);
#define      gdk_gl_glNormal3hNV(proc, nx, ny, nz) \
  ( ((GdkGLProc_glNormal3hNV) (proc)) (nx, ny, nz) )

/* glNormal3hvNV */
typedef void (APIENTRY * GdkGLProc_glNormal3hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glNormal3hvNV (void);
#define      gdk_gl_glNormal3hvNV(proc, v) \
  ( ((GdkGLProc_glNormal3hvNV) (proc)) (v) )

/* glColor3hNV */
typedef void (APIENTRY * GdkGLProc_glColor3hNV) (GLhalfNV red, GLhalfNV green, GLhalfNV blue);
GdkGLProc    gdk_gl_get_glColor3hNV (void);
#define      gdk_gl_glColor3hNV(proc, red, green, blue) \
  ( ((GdkGLProc_glColor3hNV) (proc)) (red, green, blue) )

/* glColor3hvNV */
typedef void (APIENTRY * GdkGLProc_glColor3hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glColor3hvNV (void);
#define      gdk_gl_glColor3hvNV(proc, v) \
  ( ((GdkGLProc_glColor3hvNV) (proc)) (v) )

/* glColor4hNV */
typedef void (APIENTRY * GdkGLProc_glColor4hNV) (GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha);
GdkGLProc    gdk_gl_get_glColor4hNV (void);
#define      gdk_gl_glColor4hNV(proc, red, green, blue, alpha) \
  ( ((GdkGLProc_glColor4hNV) (proc)) (red, green, blue, alpha) )

/* glColor4hvNV */
typedef void (APIENTRY * GdkGLProc_glColor4hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glColor4hvNV (void);
#define      gdk_gl_glColor4hvNV(proc, v) \
  ( ((GdkGLProc_glColor4hvNV) (proc)) (v) )

/* glTexCoord1hNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord1hNV) (GLhalfNV s);
GdkGLProc    gdk_gl_get_glTexCoord1hNV (void);
#define      gdk_gl_glTexCoord1hNV(proc, s) \
  ( ((GdkGLProc_glTexCoord1hNV) (proc)) (s) )

/* glTexCoord1hvNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord1hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glTexCoord1hvNV (void);
#define      gdk_gl_glTexCoord1hvNV(proc, v) \
  ( ((GdkGLProc_glTexCoord1hvNV) (proc)) (v) )

/* glTexCoord2hNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord2hNV) (GLhalfNV s, GLhalfNV t);
GdkGLProc    gdk_gl_get_glTexCoord2hNV (void);
#define      gdk_gl_glTexCoord2hNV(proc, s, t) \
  ( ((GdkGLProc_glTexCoord2hNV) (proc)) (s, t) )

/* glTexCoord2hvNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord2hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glTexCoord2hvNV (void);
#define      gdk_gl_glTexCoord2hvNV(proc, v) \
  ( ((GdkGLProc_glTexCoord2hvNV) (proc)) (v) )

/* glTexCoord3hNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord3hNV) (GLhalfNV s, GLhalfNV t, GLhalfNV r);
GdkGLProc    gdk_gl_get_glTexCoord3hNV (void);
#define      gdk_gl_glTexCoord3hNV(proc, s, t, r) \
  ( ((GdkGLProc_glTexCoord3hNV) (proc)) (s, t, r) )

/* glTexCoord3hvNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord3hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glTexCoord3hvNV (void);
#define      gdk_gl_glTexCoord3hvNV(proc, v) \
  ( ((GdkGLProc_glTexCoord3hvNV) (proc)) (v) )

/* glTexCoord4hNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord4hNV) (GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q);
GdkGLProc    gdk_gl_get_glTexCoord4hNV (void);
#define      gdk_gl_glTexCoord4hNV(proc, s, t, r, q) \
  ( ((GdkGLProc_glTexCoord4hNV) (proc)) (s, t, r, q) )

/* glTexCoord4hvNV */
typedef void (APIENTRY * GdkGLProc_glTexCoord4hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glTexCoord4hvNV (void);
#define      gdk_gl_glTexCoord4hvNV(proc, v) \
  ( ((GdkGLProc_glTexCoord4hvNV) (proc)) (v) )

/* glMultiTexCoord1hNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1hNV) (GLenum target, GLhalfNV s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1hNV (void);
#define      gdk_gl_glMultiTexCoord1hNV(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1hNV) (proc)) (target, s) )

/* glMultiTexCoord1hvNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1hvNV) (GLenum target, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1hvNV (void);
#define      gdk_gl_glMultiTexCoord1hvNV(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1hvNV) (proc)) (target, v) )

/* glMultiTexCoord2hNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2hNV) (GLenum target, GLhalfNV s, GLhalfNV t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2hNV (void);
#define      gdk_gl_glMultiTexCoord2hNV(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2hNV) (proc)) (target, s, t) )

/* glMultiTexCoord2hvNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2hvNV) (GLenum target, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2hvNV (void);
#define      gdk_gl_glMultiTexCoord2hvNV(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2hvNV) (proc)) (target, v) )

/* glMultiTexCoord3hNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3hNV) (GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3hNV (void);
#define      gdk_gl_glMultiTexCoord3hNV(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3hNV) (proc)) (target, s, t, r) )

/* glMultiTexCoord3hvNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3hvNV) (GLenum target, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3hvNV (void);
#define      gdk_gl_glMultiTexCoord3hvNV(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3hvNV) (proc)) (target, v) )

/* glMultiTexCoord4hNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4hNV) (GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4hNV (void);
#define      gdk_gl_glMultiTexCoord4hNV(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4hNV) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4hvNV */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4hvNV) (GLenum target, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4hvNV (void);
#define      gdk_gl_glMultiTexCoord4hvNV(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4hvNV) (proc)) (target, v) )

/* glFogCoordhNV */
typedef void (APIENTRY * GdkGLProc_glFogCoordhNV) (GLhalfNV fog);
GdkGLProc    gdk_gl_get_glFogCoordhNV (void);
#define      gdk_gl_glFogCoordhNV(proc, fog) \
  ( ((GdkGLProc_glFogCoordhNV) (proc)) (fog) )

/* glFogCoordhvNV */
typedef void (APIENTRY * GdkGLProc_glFogCoordhvNV) (const GLhalfNV *fog);
GdkGLProc    gdk_gl_get_glFogCoordhvNV (void);
#define      gdk_gl_glFogCoordhvNV(proc, fog) \
  ( ((GdkGLProc_glFogCoordhvNV) (proc)) (fog) )

/* glSecondaryColor3hNV */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3hNV) (GLhalfNV red, GLhalfNV green, GLhalfNV blue);
GdkGLProc    gdk_gl_get_glSecondaryColor3hNV (void);
#define      gdk_gl_glSecondaryColor3hNV(proc, red, green, blue) \
  ( ((GdkGLProc_glSecondaryColor3hNV) (proc)) (red, green, blue) )

/* glSecondaryColor3hvNV */
typedef void (APIENTRY * GdkGLProc_glSecondaryColor3hvNV) (const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glSecondaryColor3hvNV (void);
#define      gdk_gl_glSecondaryColor3hvNV(proc, v) \
  ( ((GdkGLProc_glSecondaryColor3hvNV) (proc)) (v) )

/* glVertexWeighthNV */
typedef void (APIENTRY * GdkGLProc_glVertexWeighthNV) (GLhalfNV weight);
GdkGLProc    gdk_gl_get_glVertexWeighthNV (void);
#define      gdk_gl_glVertexWeighthNV(proc, weight) \
  ( ((GdkGLProc_glVertexWeighthNV) (proc)) (weight) )

/* glVertexWeighthvNV */
typedef void (APIENTRY * GdkGLProc_glVertexWeighthvNV) (const GLhalfNV *weight);
GdkGLProc    gdk_gl_get_glVertexWeighthvNV (void);
#define      gdk_gl_glVertexWeighthvNV(proc, weight) \
  ( ((GdkGLProc_glVertexWeighthvNV) (proc)) (weight) )

/* glVertexAttrib1hNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1hNV) (GLuint index, GLhalfNV x);
GdkGLProc    gdk_gl_get_glVertexAttrib1hNV (void);
#define      gdk_gl_glVertexAttrib1hNV(proc, index, x) \
  ( ((GdkGLProc_glVertexAttrib1hNV) (proc)) (index, x) )

/* glVertexAttrib1hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib1hvNV) (GLuint index, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttrib1hvNV (void);
#define      gdk_gl_glVertexAttrib1hvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib1hvNV) (proc)) (index, v) )

/* glVertexAttrib2hNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2hNV) (GLuint index, GLhalfNV x, GLhalfNV y);
GdkGLProc    gdk_gl_get_glVertexAttrib2hNV (void);
#define      gdk_gl_glVertexAttrib2hNV(proc, index, x, y) \
  ( ((GdkGLProc_glVertexAttrib2hNV) (proc)) (index, x, y) )

/* glVertexAttrib2hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib2hvNV) (GLuint index, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttrib2hvNV (void);
#define      gdk_gl_glVertexAttrib2hvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib2hvNV) (proc)) (index, v) )

/* glVertexAttrib3hNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3hNV) (GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z);
GdkGLProc    gdk_gl_get_glVertexAttrib3hNV (void);
#define      gdk_gl_glVertexAttrib3hNV(proc, index, x, y, z) \
  ( ((GdkGLProc_glVertexAttrib3hNV) (proc)) (index, x, y, z) )

/* glVertexAttrib3hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib3hvNV) (GLuint index, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttrib3hvNV (void);
#define      gdk_gl_glVertexAttrib3hvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib3hvNV) (proc)) (index, v) )

/* glVertexAttrib4hNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4hNV) (GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w);
GdkGLProc    gdk_gl_get_glVertexAttrib4hNV (void);
#define      gdk_gl_glVertexAttrib4hNV(proc, index, x, y, z, w) \
  ( ((GdkGLProc_glVertexAttrib4hNV) (proc)) (index, x, y, z, w) )

/* glVertexAttrib4hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttrib4hvNV) (GLuint index, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttrib4hvNV (void);
#define      gdk_gl_glVertexAttrib4hvNV(proc, index, v) \
  ( ((GdkGLProc_glVertexAttrib4hvNV) (proc)) (index, v) )

/* glVertexAttribs1hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs1hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttribs1hvNV (void);
#define      gdk_gl_glVertexAttribs1hvNV(proc, index, n, v) \
  ( ((GdkGLProc_glVertexAttribs1hvNV) (proc)) (index, n, v) )

/* glVertexAttribs2hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs2hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttribs2hvNV (void);
#define      gdk_gl_glVertexAttribs2hvNV(proc, index, n, v) \
  ( ((GdkGLProc_glVertexAttribs2hvNV) (proc)) (index, n, v) )

/* glVertexAttribs3hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs3hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttribs3hvNV (void);
#define      gdk_gl_glVertexAttribs3hvNV(proc, index, n, v) \
  ( ((GdkGLProc_glVertexAttribs3hvNV) (proc)) (index, n, v) )

/* glVertexAttribs4hvNV */
typedef void (APIENTRY * GdkGLProc_glVertexAttribs4hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
GdkGLProc    gdk_gl_get_glVertexAttribs4hvNV (void);
#define      gdk_gl_glVertexAttribs4hvNV(proc, index, n, v) \
  ( ((GdkGLProc_glVertexAttribs4hvNV) (proc)) (index, n, v) )

/* proc struct */

typedef struct _GdkGL_GL_NV_half_float GdkGL_GL_NV_half_float;

struct _GdkGL_GL_NV_half_float
{
  GdkGLProc_glVertex2hNV glVertex2hNV;
  GdkGLProc_glVertex2hvNV glVertex2hvNV;
  GdkGLProc_glVertex3hNV glVertex3hNV;
  GdkGLProc_glVertex3hvNV glVertex3hvNV;
  GdkGLProc_glVertex4hNV glVertex4hNV;
  GdkGLProc_glVertex4hvNV glVertex4hvNV;
  GdkGLProc_glNormal3hNV glNormal3hNV;
  GdkGLProc_glNormal3hvNV glNormal3hvNV;
  GdkGLProc_glColor3hNV glColor3hNV;
  GdkGLProc_glColor3hvNV glColor3hvNV;
  GdkGLProc_glColor4hNV glColor4hNV;
  GdkGLProc_glColor4hvNV glColor4hvNV;
  GdkGLProc_glTexCoord1hNV glTexCoord1hNV;
  GdkGLProc_glTexCoord1hvNV glTexCoord1hvNV;
  GdkGLProc_glTexCoord2hNV glTexCoord2hNV;
  GdkGLProc_glTexCoord2hvNV glTexCoord2hvNV;
  GdkGLProc_glTexCoord3hNV glTexCoord3hNV;
  GdkGLProc_glTexCoord3hvNV glTexCoord3hvNV;
  GdkGLProc_glTexCoord4hNV glTexCoord4hNV;
  GdkGLProc_glTexCoord4hvNV glTexCoord4hvNV;
  GdkGLProc_glMultiTexCoord1hNV glMultiTexCoord1hNV;
  GdkGLProc_glMultiTexCoord1hvNV glMultiTexCoord1hvNV;
  GdkGLProc_glMultiTexCoord2hNV glMultiTexCoord2hNV;
  GdkGLProc_glMultiTexCoord2hvNV glMultiTexCoord2hvNV;
  GdkGLProc_glMultiTexCoord3hNV glMultiTexCoord3hNV;
  GdkGLProc_glMultiTexCoord3hvNV glMultiTexCoord3hvNV;
  GdkGLProc_glMultiTexCoord4hNV glMultiTexCoord4hNV;
  GdkGLProc_glMultiTexCoord4hvNV glMultiTexCoord4hvNV;
  GdkGLProc_glFogCoordhNV glFogCoordhNV;
  GdkGLProc_glFogCoordhvNV glFogCoordhvNV;
  GdkGLProc_glSecondaryColor3hNV glSecondaryColor3hNV;
  GdkGLProc_glSecondaryColor3hvNV glSecondaryColor3hvNV;
  GdkGLProc_glVertexWeighthNV glVertexWeighthNV;
  GdkGLProc_glVertexWeighthvNV glVertexWeighthvNV;
  GdkGLProc_glVertexAttrib1hNV glVertexAttrib1hNV;
  GdkGLProc_glVertexAttrib1hvNV glVertexAttrib1hvNV;
  GdkGLProc_glVertexAttrib2hNV glVertexAttrib2hNV;
  GdkGLProc_glVertexAttrib2hvNV glVertexAttrib2hvNV;
  GdkGLProc_glVertexAttrib3hNV glVertexAttrib3hNV;
  GdkGLProc_glVertexAttrib3hvNV glVertexAttrib3hvNV;
  GdkGLProc_glVertexAttrib4hNV glVertexAttrib4hNV;
  GdkGLProc_glVertexAttrib4hvNV glVertexAttrib4hvNV;
  GdkGLProc_glVertexAttribs1hvNV glVertexAttribs1hvNV;
  GdkGLProc_glVertexAttribs2hvNV glVertexAttribs2hvNV;
  GdkGLProc_glVertexAttribs3hvNV glVertexAttribs3hvNV;
  GdkGLProc_glVertexAttribs4hvNV glVertexAttribs4hvNV;
};

GdkGL_GL_NV_half_float *gdk_gl_get_GL_NV_half_float (void);

/*
 * GL_NV_pixel_data_range
 */

/* glPixelDataRangeNV */
typedef void (APIENTRY * GdkGLProc_glPixelDataRangeNV) (GLenum target, GLsizei length, GLvoid *pointer);
GdkGLProc    gdk_gl_get_glPixelDataRangeNV (void);
#define      gdk_gl_glPixelDataRangeNV(proc, target, length, pointer) \
  ( ((GdkGLProc_glPixelDataRangeNV) (proc)) (target, length, pointer) )

/* glFlushPixelDataRangeNV */
typedef void (APIENTRY * GdkGLProc_glFlushPixelDataRangeNV) (GLenum target);
GdkGLProc    gdk_gl_get_glFlushPixelDataRangeNV (void);
#define      gdk_gl_glFlushPixelDataRangeNV(proc, target) \
  ( ((GdkGLProc_glFlushPixelDataRangeNV) (proc)) (target) )

/* proc struct */

typedef struct _GdkGL_GL_NV_pixel_data_range GdkGL_GL_NV_pixel_data_range;

struct _GdkGL_GL_NV_pixel_data_range
{
  GdkGLProc_glPixelDataRangeNV glPixelDataRangeNV;
  GdkGLProc_glFlushPixelDataRangeNV glFlushPixelDataRangeNV;
};

GdkGL_GL_NV_pixel_data_range *gdk_gl_get_GL_NV_pixel_data_range (void);

/*
 * GL_NV_primitive_restart
 */

/* glPrimitiveRestartNV */
typedef void (APIENTRY * GdkGLProc_glPrimitiveRestartNV) (void);
GdkGLProc    gdk_gl_get_glPrimitiveRestartNV (void);
#define      gdk_gl_glPrimitiveRestartNV(proc) \
  ( ((GdkGLProc_glPrimitiveRestartNV) (proc)) () )

/* glPrimitiveRestartIndexNV */
typedef void (APIENTRY * GdkGLProc_glPrimitiveRestartIndexNV) (GLuint index);
GdkGLProc    gdk_gl_get_glPrimitiveRestartIndexNV (void);
#define      gdk_gl_glPrimitiveRestartIndexNV(proc, index) \
  ( ((GdkGLProc_glPrimitiveRestartIndexNV) (proc)) (index) )

/* proc struct */

typedef struct _GdkGL_GL_NV_primitive_restart GdkGL_GL_NV_primitive_restart;

struct _GdkGL_GL_NV_primitive_restart
{
  GdkGLProc_glPrimitiveRestartNV glPrimitiveRestartNV;
  GdkGLProc_glPrimitiveRestartIndexNV glPrimitiveRestartIndexNV;
};

GdkGL_GL_NV_primitive_restart *gdk_gl_get_GL_NV_primitive_restart (void);

/*
 * GL_ATI_map_object_buffer
 */

/* glMapObjectBufferATI */
typedef GLvoid* (APIENTRY * GdkGLProc_glMapObjectBufferATI) (GLuint buffer);
GdkGLProc    gdk_gl_get_glMapObjectBufferATI (void);
#define      gdk_gl_glMapObjectBufferATI(proc, buffer) \
  ( ((GdkGLProc_glMapObjectBufferATI) (proc)) (buffer) )

/* glUnmapObjectBufferATI */
typedef void (APIENTRY * GdkGLProc_glUnmapObjectBufferATI) (GLuint buffer);
GdkGLProc    gdk_gl_get_glUnmapObjectBufferATI (void);
#define      gdk_gl_glUnmapObjectBufferATI(proc, buffer) \
  ( ((GdkGLProc_glUnmapObjectBufferATI) (proc)) (buffer) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_map_object_buffer GdkGL_GL_ATI_map_object_buffer;

struct _GdkGL_GL_ATI_map_object_buffer
{
  GdkGLProc_glMapObjectBufferATI glMapObjectBufferATI;
  GdkGLProc_glUnmapObjectBufferATI glUnmapObjectBufferATI;
};

GdkGL_GL_ATI_map_object_buffer *gdk_gl_get_GL_ATI_map_object_buffer (void);

/*
 * GL_ATI_separate_stencil
 */

/* glStencilOpSeparateATI */
typedef void (APIENTRY * GdkGLProc_glStencilOpSeparateATI) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
GdkGLProc    gdk_gl_get_glStencilOpSeparateATI (void);
#define      gdk_gl_glStencilOpSeparateATI(proc, face, sfail, dpfail, dppass) \
  ( ((GdkGLProc_glStencilOpSeparateATI) (proc)) (face, sfail, dpfail, dppass) )

/* glStencilFuncSeparateATI */
typedef void (APIENTRY * GdkGLProc_glStencilFuncSeparateATI) (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
GdkGLProc    gdk_gl_get_glStencilFuncSeparateATI (void);
#define      gdk_gl_glStencilFuncSeparateATI(proc, frontfunc, backfunc, ref, mask) \
  ( ((GdkGLProc_glStencilFuncSeparateATI) (proc)) (frontfunc, backfunc, ref, mask) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_separate_stencil GdkGL_GL_ATI_separate_stencil;

struct _GdkGL_GL_ATI_separate_stencil
{
  GdkGLProc_glStencilOpSeparateATI glStencilOpSeparateATI;
  GdkGLProc_glStencilFuncSeparateATI glStencilFuncSeparateATI;
};

GdkGL_GL_ATI_separate_stencil *gdk_gl_get_GL_ATI_separate_stencil (void);

/*
 * GL_ATI_vertex_attrib_array_object
 */

/* glVertexAttribArrayObjectATI */
typedef void (APIENTRY * GdkGLProc_glVertexAttribArrayObjectATI) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset);
GdkGLProc    gdk_gl_get_glVertexAttribArrayObjectATI (void);
#define      gdk_gl_glVertexAttribArrayObjectATI(proc, index, size, type, normalized, stride, buffer, offset) \
  ( ((GdkGLProc_glVertexAttribArrayObjectATI) (proc)) (index, size, type, normalized, stride, buffer, offset) )

/* glGetVertexAttribArrayObjectfvATI */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribArrayObjectfvATI) (GLuint index, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribArrayObjectfvATI (void);
#define      gdk_gl_glGetVertexAttribArrayObjectfvATI(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribArrayObjectfvATI) (proc)) (index, pname, params) )

/* glGetVertexAttribArrayObjectivATI */
typedef void (APIENTRY * GdkGLProc_glGetVertexAttribArrayObjectivATI) (GLuint index, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetVertexAttribArrayObjectivATI (void);
#define      gdk_gl_glGetVertexAttribArrayObjectivATI(proc, index, pname, params) \
  ( ((GdkGLProc_glGetVertexAttribArrayObjectivATI) (proc)) (index, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_vertex_attrib_array_object GdkGL_GL_ATI_vertex_attrib_array_object;

struct _GdkGL_GL_ATI_vertex_attrib_array_object
{
  GdkGLProc_glVertexAttribArrayObjectATI glVertexAttribArrayObjectATI;
  GdkGLProc_glGetVertexAttribArrayObjectfvATI glGetVertexAttribArrayObjectfvATI;
  GdkGLProc_glGetVertexAttribArrayObjectivATI glGetVertexAttribArrayObjectivATI;
};

GdkGL_GL_ATI_vertex_attrib_array_object *gdk_gl_get_GL_ATI_vertex_attrib_array_object (void);

/*
 * GL_APPLE_texture_range
 */

/* glTextureRangeAPPLE */
typedef void (APIENTRY * GdkGLProc_glTextureRangeAPPLE) (GLenum target, GLsizei length, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glTextureRangeAPPLE (void);
#define      gdk_gl_glTextureRangeAPPLE(proc, target, length, pointer) \
  ( ((GdkGLProc_glTextureRangeAPPLE) (proc)) (target, length, pointer) )

/* glGetTexParameterPointervAPPLE */
typedef void (APIENTRY * GdkGLProc_glGetTexParameterPointervAPPLE) (GLenum target, GLenum pname, GLvoid **params);
GdkGLProc    gdk_gl_get_glGetTexParameterPointervAPPLE (void);
#define      gdk_gl_glGetTexParameterPointervAPPLE(proc, target, pname, params) \
  ( ((GdkGLProc_glGetTexParameterPointervAPPLE) (proc)) (target, pname, params) )

/* proc struct */

typedef struct _GdkGL_GL_APPLE_texture_range GdkGL_GL_APPLE_texture_range;

struct _GdkGL_GL_APPLE_texture_range
{
  GdkGLProc_glTextureRangeAPPLE glTextureRangeAPPLE;
  GdkGLProc_glGetTexParameterPointervAPPLE glGetTexParameterPointervAPPLE;
};

GdkGL_GL_APPLE_texture_range *gdk_gl_get_GL_APPLE_texture_range (void);

/*
 * GL_APPLE_vertex_program_evaluators
 */

/* glEnableVertexAttribAPPLE */
typedef void (APIENTRY * GdkGLProc_glEnableVertexAttribAPPLE) (GLuint index, GLenum pname);
GdkGLProc    gdk_gl_get_glEnableVertexAttribAPPLE (void);
#define      gdk_gl_glEnableVertexAttribAPPLE(proc, index, pname) \
  ( ((GdkGLProc_glEnableVertexAttribAPPLE) (proc)) (index, pname) )

/* glDisableVertexAttribAPPLE */
typedef void (APIENTRY * GdkGLProc_glDisableVertexAttribAPPLE) (GLuint index, GLenum pname);
GdkGLProc    gdk_gl_get_glDisableVertexAttribAPPLE (void);
#define      gdk_gl_glDisableVertexAttribAPPLE(proc, index, pname) \
  ( ((GdkGLProc_glDisableVertexAttribAPPLE) (proc)) (index, pname) )

/* glIsVertexAttribEnabledAPPLE */
typedef GLboolean (APIENTRY * GdkGLProc_glIsVertexAttribEnabledAPPLE) (GLuint index, GLenum pname);
GdkGLProc    gdk_gl_get_glIsVertexAttribEnabledAPPLE (void);
#define      gdk_gl_glIsVertexAttribEnabledAPPLE(proc, index, pname) \
  ( ((GdkGLProc_glIsVertexAttribEnabledAPPLE) (proc)) (index, pname) )

/* glMapVertexAttrib1dAPPLE */
typedef void (APIENTRY * GdkGLProc_glMapVertexAttrib1dAPPLE) (GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
GdkGLProc    gdk_gl_get_glMapVertexAttrib1dAPPLE (void);
#define      gdk_gl_glMapVertexAttrib1dAPPLE(proc, index, size, u1, u2, stride, order, points) \
  ( ((GdkGLProc_glMapVertexAttrib1dAPPLE) (proc)) (index, size, u1, u2, stride, order, points) )

/* glMapVertexAttrib1fAPPLE */
typedef void (APIENTRY * GdkGLProc_glMapVertexAttrib1fAPPLE) (GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
GdkGLProc    gdk_gl_get_glMapVertexAttrib1fAPPLE (void);
#define      gdk_gl_glMapVertexAttrib1fAPPLE(proc, index, size, u1, u2, stride, order, points) \
  ( ((GdkGLProc_glMapVertexAttrib1fAPPLE) (proc)) (index, size, u1, u2, stride, order, points) )

/* glMapVertexAttrib2dAPPLE */
typedef void (APIENTRY * GdkGLProc_glMapVertexAttrib2dAPPLE) (GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
GdkGLProc    gdk_gl_get_glMapVertexAttrib2dAPPLE (void);
#define      gdk_gl_glMapVertexAttrib2dAPPLE(proc, index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
  ( ((GdkGLProc_glMapVertexAttrib2dAPPLE) (proc)) (index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) )

/* glMapVertexAttrib2fAPPLE */
typedef void (APIENTRY * GdkGLProc_glMapVertexAttrib2fAPPLE) (GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
GdkGLProc    gdk_gl_get_glMapVertexAttrib2fAPPLE (void);
#define      gdk_gl_glMapVertexAttrib2fAPPLE(proc, index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) \
  ( ((GdkGLProc_glMapVertexAttrib2fAPPLE) (proc)) (index, size, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points) )

/* proc struct */

typedef struct _GdkGL_GL_APPLE_vertex_program_evaluators GdkGL_GL_APPLE_vertex_program_evaluators;

struct _GdkGL_GL_APPLE_vertex_program_evaluators
{
  GdkGLProc_glEnableVertexAttribAPPLE glEnableVertexAttribAPPLE;
  GdkGLProc_glDisableVertexAttribAPPLE glDisableVertexAttribAPPLE;
  GdkGLProc_glIsVertexAttribEnabledAPPLE glIsVertexAttribEnabledAPPLE;
  GdkGLProc_glMapVertexAttrib1dAPPLE glMapVertexAttrib1dAPPLE;
  GdkGLProc_glMapVertexAttrib1fAPPLE glMapVertexAttrib1fAPPLE;
  GdkGLProc_glMapVertexAttrib2dAPPLE glMapVertexAttrib2dAPPLE;
  GdkGLProc_glMapVertexAttrib2fAPPLE glMapVertexAttrib2fAPPLE;
};

GdkGL_GL_APPLE_vertex_program_evaluators *gdk_gl_get_GL_APPLE_vertex_program_evaluators (void);

/*
 * GL_ATI_blend_equation_separate
 */

/* glBlendEquationSeparateATI */
typedef void (APIENTRY * GdkGLProc_glBlendEquationSeparateATI) (GLenum equationRGB, GLenum equationAlpha);
GdkGLProc    gdk_gl_get_glBlendEquationSeparateATI (void);
#define      gdk_gl_glBlendEquationSeparateATI(proc, equationRGB, equationAlpha) \
  ( ((GdkGLProc_glBlendEquationSeparateATI) (proc)) (equationRGB, equationAlpha) )

/* proc struct */

typedef struct _GdkGL_GL_ATI_blend_equation_separate GdkGL_GL_ATI_blend_equation_separate;

struct _GdkGL_GL_ATI_blend_equation_separate
{
  GdkGLProc_glBlendEquationSeparateATI glBlendEquationSeparateATI;
};

GdkGL_GL_ATI_blend_equation_separate *gdk_gl_get_GL_ATI_blend_equation_separate (void);

/*
 * GL_ATIX_pn_triangles
 */

/* glPNTrianglesiATIX */
typedef void (APIENTRY * GdkGLProc_glPNTrianglesiATIX) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glPNTrianglesiATIX (void);
#define      gdk_gl_glPNTrianglesiATIX(proc, pname, param) \
  ( ((GdkGLProc_glPNTrianglesiATIX) (proc)) (pname, param) )

/* glPNTrianglesfATIX */
typedef void (APIENTRY * GdkGLProc_glPNTrianglesfATIX) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glPNTrianglesfATIX (void);
#define      gdk_gl_glPNTrianglesfATIX(proc, pname, param) \
  ( ((GdkGLProc_glPNTrianglesfATIX) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_ATIX_pn_triangles GdkGL_GL_ATIX_pn_triangles;

struct _GdkGL_GL_ATIX_pn_triangles
{
  GdkGLProc_glPNTrianglesiATIX glPNTrianglesiATIX;
  GdkGLProc_glPNTrianglesfATIX glPNTrianglesfATIX;
};

GdkGL_GL_ATIX_pn_triangles *gdk_gl_get_GL_ATIX_pn_triangles (void);

/*
 * GL_Autodesk_facet_normal
 */

/* glFacetNormal3b */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3b) (GLbyte nx, GLbyte ny, GLbyte nz);
GdkGLProc    gdk_gl_get_glFacetNormal3b (void);
#define      gdk_gl_glFacetNormal3b(proc, nx, ny, nz) \
  ( ((GdkGLProc_glFacetNormal3b) (proc)) (nx, ny, nz) )

/* glFacetNormal3d */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3d) (GLdouble nx, GLdouble ny, GLdouble nz);
GdkGLProc    gdk_gl_get_glFacetNormal3d (void);
#define      gdk_gl_glFacetNormal3d(proc, nx, ny, nz) \
  ( ((GdkGLProc_glFacetNormal3d) (proc)) (nx, ny, nz) )

/* glFacetNormal3f */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3f) (GLfloat nx, GLfloat ny, GLfloat nz);
GdkGLProc    gdk_gl_get_glFacetNormal3f (void);
#define      gdk_gl_glFacetNormal3f(proc, nx, ny, nz) \
  ( ((GdkGLProc_glFacetNormal3f) (proc)) (nx, ny, nz) )

/* glFacetNormal3i */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3i) (GLint nx, GLint ny, GLint nz);
GdkGLProc    gdk_gl_get_glFacetNormal3i (void);
#define      gdk_gl_glFacetNormal3i(proc, nx, ny, nz) \
  ( ((GdkGLProc_glFacetNormal3i) (proc)) (nx, ny, nz) )

/* glFacetNormal3s */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3s) (GLshort nx, GLshort ny, GLshort nz);
GdkGLProc    gdk_gl_get_glFacetNormal3s (void);
#define      gdk_gl_glFacetNormal3s(proc, nx, ny, nz) \
  ( ((GdkGLProc_glFacetNormal3s) (proc)) (nx, ny, nz) )

/* glFacetNormal3bv */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3bv) (const GLbyte *v);
GdkGLProc    gdk_gl_get_glFacetNormal3bv (void);
#define      gdk_gl_glFacetNormal3bv(proc, v) \
  ( ((GdkGLProc_glFacetNormal3bv) (proc)) (v) )

/* glFacetNormal3dv */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3dv) (const GLdouble *v);
GdkGLProc    gdk_gl_get_glFacetNormal3dv (void);
#define      gdk_gl_glFacetNormal3dv(proc, v) \
  ( ((GdkGLProc_glFacetNormal3dv) (proc)) (v) )

/* glFacetNormal3fv */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3fv) (const GLfloat *v);
GdkGLProc    gdk_gl_get_glFacetNormal3fv (void);
#define      gdk_gl_glFacetNormal3fv(proc, v) \
  ( ((GdkGLProc_glFacetNormal3fv) (proc)) (v) )

/* glFacetNormal3iv */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3iv) (const GLint *v);
GdkGLProc    gdk_gl_get_glFacetNormal3iv (void);
#define      gdk_gl_glFacetNormal3iv(proc, v) \
  ( ((GdkGLProc_glFacetNormal3iv) (proc)) (v) )

/* glFacetNormal3sv */
typedef void (APIENTRY * GdkGLProc_glFacetNormal3sv) (const GLshort *v);
GdkGLProc    gdk_gl_get_glFacetNormal3sv (void);
#define      gdk_gl_glFacetNormal3sv(proc, v) \
  ( ((GdkGLProc_glFacetNormal3sv) (proc)) (v) )

/* proc struct */

typedef struct _GdkGL_GL_Autodesk_facet_normal GdkGL_GL_Autodesk_facet_normal;

struct _GdkGL_GL_Autodesk_facet_normal
{
  GdkGLProc_glFacetNormal3b glFacetNormal3b;
  GdkGLProc_glFacetNormal3d glFacetNormal3d;
  GdkGLProc_glFacetNormal3f glFacetNormal3f;
  GdkGLProc_glFacetNormal3i glFacetNormal3i;
  GdkGLProc_glFacetNormal3s glFacetNormal3s;
  GdkGLProc_glFacetNormal3bv glFacetNormal3bv;
  GdkGLProc_glFacetNormal3dv glFacetNormal3dv;
  GdkGLProc_glFacetNormal3fv glFacetNormal3fv;
  GdkGLProc_glFacetNormal3iv glFacetNormal3iv;
  GdkGLProc_glFacetNormal3sv glFacetNormal3sv;
};

GdkGL_GL_Autodesk_facet_normal *gdk_gl_get_GL_Autodesk_facet_normal (void);

/*
 * GL_Autodesk_valid_back_buffer_hint
 */

/* glWindowBackBufferHint */
typedef void (APIENTRY * GdkGLProc_glWindowBackBufferHint) (void);
GdkGLProc    gdk_gl_get_glWindowBackBufferHint (void);
#define      gdk_gl_glWindowBackBufferHint(proc) \
  ( ((GdkGLProc_glWindowBackBufferHint) (proc)) () )

/* glValidBackBufferHint */
typedef GLboolean (APIENTRY * GdkGLProc_glValidBackBufferHint) (GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glValidBackBufferHint (void);
#define      gdk_gl_glValidBackBufferHint(proc, x, y, width, height) \
  ( ((GdkGLProc_glValidBackBufferHint) (proc)) (x, y, width, height) )

/* proc struct */

typedef struct _GdkGL_GL_Autodesk_valid_back_buffer_hint GdkGL_GL_Autodesk_valid_back_buffer_hint;

struct _GdkGL_GL_Autodesk_valid_back_buffer_hint
{
  GdkGLProc_glWindowBackBufferHint glWindowBackBufferHint;
  GdkGLProc_glValidBackBufferHint glValidBackBufferHint;
};

GdkGL_GL_Autodesk_valid_back_buffer_hint *gdk_gl_get_GL_Autodesk_valid_back_buffer_hint (void);

/*
 * GL_EXT_depth_bounds_test
 */

/* glDepthBoundsEXT */
typedef void (APIENTRY * GdkGLProc_glDepthBoundsEXT) (GLclampd zmin, GLclampd zmax);
GdkGLProc    gdk_gl_get_glDepthBoundsEXT (void);
#define      gdk_gl_glDepthBoundsEXT(proc, zmin, zmax) \
  ( ((GdkGLProc_glDepthBoundsEXT) (proc)) (zmin, zmax) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_depth_bounds_test GdkGL_GL_EXT_depth_bounds_test;

struct _GdkGL_GL_EXT_depth_bounds_test
{
  GdkGLProc_glDepthBoundsEXT glDepthBoundsEXT;
};

GdkGL_GL_EXT_depth_bounds_test *gdk_gl_get_GL_EXT_depth_bounds_test (void);

/*
 * GL_EXT_fragment_lighting
 */

/* glFragmentLightModelfEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModelfEXT) (GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glFragmentLightModelfEXT (void);
#define      gdk_gl_glFragmentLightModelfEXT(proc, pname, param) \
  ( ((GdkGLProc_glFragmentLightModelfEXT) (proc)) (pname, param) )

/* glFragmentLightModelfvEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModelfvEXT) (GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glFragmentLightModelfvEXT (void);
#define      gdk_gl_glFragmentLightModelfvEXT(proc, pname, params) \
  ( ((GdkGLProc_glFragmentLightModelfvEXT) (proc)) (pname, params) )

/* glFragmentLightModeliEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModeliEXT) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glFragmentLightModeliEXT (void);
#define      gdk_gl_glFragmentLightModeliEXT(proc, pname, param) \
  ( ((GdkGLProc_glFragmentLightModeliEXT) (proc)) (pname, param) )

/* glFragmentLightModelivEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightModelivEXT) (GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glFragmentLightModelivEXT (void);
#define      gdk_gl_glFragmentLightModelivEXT(proc, pname, params) \
  ( ((GdkGLProc_glFragmentLightModelivEXT) (proc)) (pname, params) )

/* glFragmentLightfEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightfEXT) (GLenum light, GLenum pname, GLfloat param);
GdkGLProc    gdk_gl_get_glFragmentLightfEXT (void);
#define      gdk_gl_glFragmentLightfEXT(proc, light, pname, param) \
  ( ((GdkGLProc_glFragmentLightfEXT) (proc)) (light, pname, param) )

/* glFragmentLightfvEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightfvEXT) (GLenum light, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glFragmentLightfvEXT (void);
#define      gdk_gl_glFragmentLightfvEXT(proc, light, pname, params) \
  ( ((GdkGLProc_glFragmentLightfvEXT) (proc)) (light, pname, params) )

/* glFragmentLightiEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightiEXT) (GLenum light, GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glFragmentLightiEXT (void);
#define      gdk_gl_glFragmentLightiEXT(proc, light, pname, param) \
  ( ((GdkGLProc_glFragmentLightiEXT) (proc)) (light, pname, param) )

/* glFragmentLightivEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentLightivEXT) (GLenum light, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glFragmentLightivEXT (void);
#define      gdk_gl_glFragmentLightivEXT(proc, light, pname, params) \
  ( ((GdkGLProc_glFragmentLightivEXT) (proc)) (light, pname, params) )

/* glGetFragmentLightfvEXT */
typedef void (APIENTRY * GdkGLProc_glGetFragmentLightfvEXT) (GLenum light, GLenum pname, GLfloat *params);
GdkGLProc    gdk_gl_get_glGetFragmentLightfvEXT (void);
#define      gdk_gl_glGetFragmentLightfvEXT(proc, light, pname, params) \
  ( ((GdkGLProc_glGetFragmentLightfvEXT) (proc)) (light, pname, params) )

/* glGetFragmentLightivEXT */
typedef void (APIENTRY * GdkGLProc_glGetFragmentLightivEXT) (GLenum light, GLenum pname, GLint *params);
GdkGLProc    gdk_gl_get_glGetFragmentLightivEXT (void);
#define      gdk_gl_glGetFragmentLightivEXT(proc, light, pname, params) \
  ( ((GdkGLProc_glGetFragmentLightivEXT) (proc)) (light, pname, params) )

/* glFragmentMaterialfEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialfEXT) (GLenum face, GLenum pname, const GLfloat param);
GdkGLProc    gdk_gl_get_glFragmentMaterialfEXT (void);
#define      gdk_gl_glFragmentMaterialfEXT(proc, face, pname, param) \
  ( ((GdkGLProc_glFragmentMaterialfEXT) (proc)) (face, pname, param) )

/* glFragmentMaterialfvEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialfvEXT) (GLenum face, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glFragmentMaterialfvEXT (void);
#define      gdk_gl_glFragmentMaterialfvEXT(proc, face, pname, params) \
  ( ((GdkGLProc_glFragmentMaterialfvEXT) (proc)) (face, pname, params) )

/* glFragmentMaterialiEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialiEXT) (GLenum face, GLenum pname, const GLint param);
GdkGLProc    gdk_gl_get_glFragmentMaterialiEXT (void);
#define      gdk_gl_glFragmentMaterialiEXT(proc, face, pname, param) \
  ( ((GdkGLProc_glFragmentMaterialiEXT) (proc)) (face, pname, param) )

/* glFragmentMaterialivEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentMaterialivEXT) (GLenum face, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glFragmentMaterialivEXT (void);
#define      gdk_gl_glFragmentMaterialivEXT(proc, face, pname, params) \
  ( ((GdkGLProc_glFragmentMaterialivEXT) (proc)) (face, pname, params) )

/* glFragmentColorMaterialEXT */
typedef void (APIENTRY * GdkGLProc_glFragmentColorMaterialEXT) (GLenum face, GLenum mode);
GdkGLProc    gdk_gl_get_glFragmentColorMaterialEXT (void);
#define      gdk_gl_glFragmentColorMaterialEXT(proc, face, mode) \
  ( ((GdkGLProc_glFragmentColorMaterialEXT) (proc)) (face, mode) )

/* glGetFragmentMaterialfvEXT */
typedef void (APIENTRY * GdkGLProc_glGetFragmentMaterialfvEXT) (GLenum face, GLenum pname, const GLfloat *params);
GdkGLProc    gdk_gl_get_glGetFragmentMaterialfvEXT (void);
#define      gdk_gl_glGetFragmentMaterialfvEXT(proc, face, pname, params) \
  ( ((GdkGLProc_glGetFragmentMaterialfvEXT) (proc)) (face, pname, params) )

/* glGetFragmentMaterialivEXT */
typedef void (APIENTRY * GdkGLProc_glGetFragmentMaterialivEXT) (GLenum face, GLenum pname, const GLint *params);
GdkGLProc    gdk_gl_get_glGetFragmentMaterialivEXT (void);
#define      gdk_gl_glGetFragmentMaterialivEXT(proc, face, pname, params) \
  ( ((GdkGLProc_glGetFragmentMaterialivEXT) (proc)) (face, pname, params) )

/* glLightEnviEXT */
typedef void (APIENTRY * GdkGLProc_glLightEnviEXT) (GLenum pname, GLint param);
GdkGLProc    gdk_gl_get_glLightEnviEXT (void);
#define      gdk_gl_glLightEnviEXT(proc, pname, param) \
  ( ((GdkGLProc_glLightEnviEXT) (proc)) (pname, param) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_fragment_lighting GdkGL_GL_EXT_fragment_lighting;

struct _GdkGL_GL_EXT_fragment_lighting
{
  GdkGLProc_glFragmentLightModelfEXT glFragmentLightModelfEXT;
  GdkGLProc_glFragmentLightModelfvEXT glFragmentLightModelfvEXT;
  GdkGLProc_glFragmentLightModeliEXT glFragmentLightModeliEXT;
  GdkGLProc_glFragmentLightModelivEXT glFragmentLightModelivEXT;
  GdkGLProc_glFragmentLightfEXT glFragmentLightfEXT;
  GdkGLProc_glFragmentLightfvEXT glFragmentLightfvEXT;
  GdkGLProc_glFragmentLightiEXT glFragmentLightiEXT;
  GdkGLProc_glFragmentLightivEXT glFragmentLightivEXT;
  GdkGLProc_glGetFragmentLightfvEXT glGetFragmentLightfvEXT;
  GdkGLProc_glGetFragmentLightivEXT glGetFragmentLightivEXT;
  GdkGLProc_glFragmentMaterialfEXT glFragmentMaterialfEXT;
  GdkGLProc_glFragmentMaterialfvEXT glFragmentMaterialfvEXT;
  GdkGLProc_glFragmentMaterialiEXT glFragmentMaterialiEXT;
  GdkGLProc_glFragmentMaterialivEXT glFragmentMaterialivEXT;
  GdkGLProc_glFragmentColorMaterialEXT glFragmentColorMaterialEXT;
  GdkGLProc_glGetFragmentMaterialfvEXT glGetFragmentMaterialfvEXT;
  GdkGLProc_glGetFragmentMaterialivEXT glGetFragmentMaterialivEXT;
  GdkGLProc_glLightEnviEXT glLightEnviEXT;
};

GdkGL_GL_EXT_fragment_lighting *gdk_gl_get_GL_EXT_fragment_lighting (void);

/*
 * GL_EXT_multitexture
 */

/* glMultiTexCoord1dEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dEXT) (GLenum target, GLdouble s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dEXT (void);
#define      gdk_gl_glMultiTexCoord1dEXT(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1dEXT) (proc)) (target, s) )

/* glMultiTexCoord1dvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dvEXT) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dvEXT (void);
#define      gdk_gl_glMultiTexCoord1dvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1dvEXT) (proc)) (target, v) )

/* glMultiTexCoord1fEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fEXT) (GLenum target, GLfloat s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fEXT (void);
#define      gdk_gl_glMultiTexCoord1fEXT(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1fEXT) (proc)) (target, s) )

/* glMultiTexCoord1fvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fvEXT) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fvEXT (void);
#define      gdk_gl_glMultiTexCoord1fvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1fvEXT) (proc)) (target, v) )

/* glMultiTexCoord1iEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1iEXT) (GLenum target, GLint s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1iEXT (void);
#define      gdk_gl_glMultiTexCoord1iEXT(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1iEXT) (proc)) (target, s) )

/* glMultiTexCoord1ivEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1ivEXT) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1ivEXT (void);
#define      gdk_gl_glMultiTexCoord1ivEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1ivEXT) (proc)) (target, v) )

/* glMultiTexCoord1sEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1sEXT) (GLenum target, GLshort s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1sEXT (void);
#define      gdk_gl_glMultiTexCoord1sEXT(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1sEXT) (proc)) (target, s) )

/* glMultiTexCoord1svEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1svEXT) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1svEXT (void);
#define      gdk_gl_glMultiTexCoord1svEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1svEXT) (proc)) (target, v) )

/* glMultiTexCoord2dEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dEXT) (GLenum target, GLdouble s, GLdouble t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dEXT (void);
#define      gdk_gl_glMultiTexCoord2dEXT(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2dEXT) (proc)) (target, s, t) )

/* glMultiTexCoord2dvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dvEXT) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dvEXT (void);
#define      gdk_gl_glMultiTexCoord2dvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2dvEXT) (proc)) (target, v) )

/* glMultiTexCoord2fEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fEXT) (GLenum target, GLfloat s, GLfloat t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fEXT (void);
#define      gdk_gl_glMultiTexCoord2fEXT(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2fEXT) (proc)) (target, s, t) )

/* glMultiTexCoord2fvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fvEXT) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fvEXT (void);
#define      gdk_gl_glMultiTexCoord2fvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2fvEXT) (proc)) (target, v) )

/* glMultiTexCoord2iEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2iEXT) (GLenum target, GLint s, GLint t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2iEXT (void);
#define      gdk_gl_glMultiTexCoord2iEXT(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2iEXT) (proc)) (target, s, t) )

/* glMultiTexCoord2ivEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2ivEXT) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2ivEXT (void);
#define      gdk_gl_glMultiTexCoord2ivEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2ivEXT) (proc)) (target, v) )

/* glMultiTexCoord2sEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2sEXT) (GLenum target, GLshort s, GLshort t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2sEXT (void);
#define      gdk_gl_glMultiTexCoord2sEXT(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2sEXT) (proc)) (target, s, t) )

/* glMultiTexCoord2svEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2svEXT) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2svEXT (void);
#define      gdk_gl_glMultiTexCoord2svEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2svEXT) (proc)) (target, v) )

/* glMultiTexCoord3dEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dEXT) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dEXT (void);
#define      gdk_gl_glMultiTexCoord3dEXT(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3dEXT) (proc)) (target, s, t, r) )

/* glMultiTexCoord3dvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dvEXT) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dvEXT (void);
#define      gdk_gl_glMultiTexCoord3dvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3dvEXT) (proc)) (target, v) )

/* glMultiTexCoord3fEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fEXT) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fEXT (void);
#define      gdk_gl_glMultiTexCoord3fEXT(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3fEXT) (proc)) (target, s, t, r) )

/* glMultiTexCoord3fvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fvEXT) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fvEXT (void);
#define      gdk_gl_glMultiTexCoord3fvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3fvEXT) (proc)) (target, v) )

/* glMultiTexCoord3iEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3iEXT) (GLenum target, GLint s, GLint t, GLint r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3iEXT (void);
#define      gdk_gl_glMultiTexCoord3iEXT(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3iEXT) (proc)) (target, s, t, r) )

/* glMultiTexCoord3ivEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3ivEXT) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3ivEXT (void);
#define      gdk_gl_glMultiTexCoord3ivEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3ivEXT) (proc)) (target, v) )

/* glMultiTexCoord3sEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3sEXT) (GLenum target, GLshort s, GLshort t, GLshort r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3sEXT (void);
#define      gdk_gl_glMultiTexCoord3sEXT(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3sEXT) (proc)) (target, s, t, r) )

/* glMultiTexCoord3svEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3svEXT) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3svEXT (void);
#define      gdk_gl_glMultiTexCoord3svEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3svEXT) (proc)) (target, v) )

/* glMultiTexCoord4dEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dEXT) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dEXT (void);
#define      gdk_gl_glMultiTexCoord4dEXT(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4dEXT) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4dvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dvEXT) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dvEXT (void);
#define      gdk_gl_glMultiTexCoord4dvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4dvEXT) (proc)) (target, v) )

/* glMultiTexCoord4fEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fEXT) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fEXT (void);
#define      gdk_gl_glMultiTexCoord4fEXT(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4fEXT) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4fvEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fvEXT) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fvEXT (void);
#define      gdk_gl_glMultiTexCoord4fvEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4fvEXT) (proc)) (target, v) )

/* glMultiTexCoord4iEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4iEXT) (GLenum target, GLint s, GLint t, GLint r, GLint q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4iEXT (void);
#define      gdk_gl_glMultiTexCoord4iEXT(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4iEXT) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4ivEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4ivEXT) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4ivEXT (void);
#define      gdk_gl_glMultiTexCoord4ivEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4ivEXT) (proc)) (target, v) )

/* glMultiTexCoord4sEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4sEXT) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4sEXT (void);
#define      gdk_gl_glMultiTexCoord4sEXT(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4sEXT) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4svEXT */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4svEXT) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4svEXT (void);
#define      gdk_gl_glMultiTexCoord4svEXT(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4svEXT) (proc)) (target, v) )

/* glInterleavedTextureCoordSetsEXT */
typedef void (APIENTRY * GdkGLProc_glInterleavedTextureCoordSetsEXT) (GLint factor);
GdkGLProc    gdk_gl_get_glInterleavedTextureCoordSetsEXT (void);
#define      gdk_gl_glInterleavedTextureCoordSetsEXT(proc, factor) \
  ( ((GdkGLProc_glInterleavedTextureCoordSetsEXT) (proc)) (factor) )

/* glSelectTextureEXT */
typedef void (APIENTRY * GdkGLProc_glSelectTextureEXT) (GLenum target);
GdkGLProc    gdk_gl_get_glSelectTextureEXT (void);
#define      gdk_gl_glSelectTextureEXT(proc, target) \
  ( ((GdkGLProc_glSelectTextureEXT) (proc)) (target) )

/* glSelectTextureCoordSetEXT */
typedef void (APIENTRY * GdkGLProc_glSelectTextureCoordSetEXT) (GLenum target);
GdkGLProc    gdk_gl_get_glSelectTextureCoordSetEXT (void);
#define      gdk_gl_glSelectTextureCoordSetEXT(proc, target) \
  ( ((GdkGLProc_glSelectTextureCoordSetEXT) (proc)) (target) )

/* glSelectTextureTransformEXT */
typedef void (APIENTRY * GdkGLProc_glSelectTextureTransformEXT) (GLenum target);
GdkGLProc    gdk_gl_get_glSelectTextureTransformEXT (void);
#define      gdk_gl_glSelectTextureTransformEXT(proc, target) \
  ( ((GdkGLProc_glSelectTextureTransformEXT) (proc)) (target) )

/* proc struct */

typedef struct _GdkGL_GL_EXT_multitexture GdkGL_GL_EXT_multitexture;

struct _GdkGL_GL_EXT_multitexture
{
  GdkGLProc_glMultiTexCoord1dEXT glMultiTexCoord1dEXT;
  GdkGLProc_glMultiTexCoord1dvEXT glMultiTexCoord1dvEXT;
  GdkGLProc_glMultiTexCoord1fEXT glMultiTexCoord1fEXT;
  GdkGLProc_glMultiTexCoord1fvEXT glMultiTexCoord1fvEXT;
  GdkGLProc_glMultiTexCoord1iEXT glMultiTexCoord1iEXT;
  GdkGLProc_glMultiTexCoord1ivEXT glMultiTexCoord1ivEXT;
  GdkGLProc_glMultiTexCoord1sEXT glMultiTexCoord1sEXT;
  GdkGLProc_glMultiTexCoord1svEXT glMultiTexCoord1svEXT;
  GdkGLProc_glMultiTexCoord2dEXT glMultiTexCoord2dEXT;
  GdkGLProc_glMultiTexCoord2dvEXT glMultiTexCoord2dvEXT;
  GdkGLProc_glMultiTexCoord2fEXT glMultiTexCoord2fEXT;
  GdkGLProc_glMultiTexCoord2fvEXT glMultiTexCoord2fvEXT;
  GdkGLProc_glMultiTexCoord2iEXT glMultiTexCoord2iEXT;
  GdkGLProc_glMultiTexCoord2ivEXT glMultiTexCoord2ivEXT;
  GdkGLProc_glMultiTexCoord2sEXT glMultiTexCoord2sEXT;
  GdkGLProc_glMultiTexCoord2svEXT glMultiTexCoord2svEXT;
  GdkGLProc_glMultiTexCoord3dEXT glMultiTexCoord3dEXT;
  GdkGLProc_glMultiTexCoord3dvEXT glMultiTexCoord3dvEXT;
  GdkGLProc_glMultiTexCoord3fEXT glMultiTexCoord3fEXT;
  GdkGLProc_glMultiTexCoord3fvEXT glMultiTexCoord3fvEXT;
  GdkGLProc_glMultiTexCoord3iEXT glMultiTexCoord3iEXT;
  GdkGLProc_glMultiTexCoord3ivEXT glMultiTexCoord3ivEXT;
  GdkGLProc_glMultiTexCoord3sEXT glMultiTexCoord3sEXT;
  GdkGLProc_glMultiTexCoord3svEXT glMultiTexCoord3svEXT;
  GdkGLProc_glMultiTexCoord4dEXT glMultiTexCoord4dEXT;
  GdkGLProc_glMultiTexCoord4dvEXT glMultiTexCoord4dvEXT;
  GdkGLProc_glMultiTexCoord4fEXT glMultiTexCoord4fEXT;
  GdkGLProc_glMultiTexCoord4fvEXT glMultiTexCoord4fvEXT;
  GdkGLProc_glMultiTexCoord4iEXT glMultiTexCoord4iEXT;
  GdkGLProc_glMultiTexCoord4ivEXT glMultiTexCoord4ivEXT;
  GdkGLProc_glMultiTexCoord4sEXT glMultiTexCoord4sEXT;
  GdkGLProc_glMultiTexCoord4svEXT glMultiTexCoord4svEXT;
  GdkGLProc_glInterleavedTextureCoordSetsEXT glInterleavedTextureCoordSetsEXT;
  GdkGLProc_glSelectTextureEXT glSelectTextureEXT;
  GdkGLProc_glSelectTextureCoordSetEXT glSelectTextureCoordSetEXT;
  GdkGLProc_glSelectTextureTransformEXT glSelectTextureTransformEXT;
};

GdkGL_GL_EXT_multitexture *gdk_gl_get_GL_EXT_multitexture (void);

/*
 * GL_EXT_scene_marker
 */

/* glBeginSceneEXT */
typedef void (APIENTRY * GdkGLProc_glBeginSceneEXT) (void);
GdkGLProc    gdk_gl_get_glBeginSceneEXT (void);
#define      gdk_gl_glBeginSceneEXT(proc) \
  ( ((GdkGLProc_glBeginSceneEXT) (proc)) () )

/* glEndSceneEXT */
typedef void (APIENTRY * GdkGLProc_glEndSceneEXT) (void);
GdkGLProc    gdk_gl_get_glEndSceneEXT (void);
#define      gdk_gl_glEndSceneEXT(proc) \
  ( ((GdkGLProc_glEndSceneEXT) (proc)) () )

/* proc struct */

typedef struct _GdkGL_GL_EXT_scene_marker GdkGL_GL_EXT_scene_marker;

struct _GdkGL_GL_EXT_scene_marker
{
  GdkGLProc_glBeginSceneEXT glBeginSceneEXT;
  GdkGLProc_glEndSceneEXT glEndSceneEXT;
};

GdkGL_GL_EXT_scene_marker *gdk_gl_get_GL_EXT_scene_marker (void);

/*
 * GL_IBM_static_data
 */

/* glFlushStaticDataIBM */
typedef void (APIENTRY * GdkGLProc_glFlushStaticDataIBM) (GLenum target);
GdkGLProc    gdk_gl_get_glFlushStaticDataIBM (void);
#define      gdk_gl_glFlushStaticDataIBM(proc, target) \
  ( ((GdkGLProc_glFlushStaticDataIBM) (proc)) (target) )

/* proc struct */

typedef struct _GdkGL_GL_IBM_static_data GdkGL_GL_IBM_static_data;

struct _GdkGL_GL_IBM_static_data
{
  GdkGLProc_glFlushStaticDataIBM glFlushStaticDataIBM;
};

GdkGL_GL_IBM_static_data *gdk_gl_get_GL_IBM_static_data (void);

/*
 * GL_KTX_buffer_region
 */

/* glBufferRegionEnabled */
typedef GLuint (APIENTRY * GdkGLProc_glBufferRegionEnabled) (void);
GdkGLProc    gdk_gl_get_glBufferRegionEnabled (void);
#define      gdk_gl_glBufferRegionEnabled(proc) \
  ( ((GdkGLProc_glBufferRegionEnabled) (proc)) () )

/* glNewBufferRegion */
typedef GLuint (APIENTRY * GdkGLProc_glNewBufferRegion) (GLenum region);
GdkGLProc    gdk_gl_get_glNewBufferRegion (void);
#define      gdk_gl_glNewBufferRegion(proc, region) \
  ( ((GdkGLProc_glNewBufferRegion) (proc)) (region) )

/* glDeleteBufferRegion */
typedef void (APIENTRY * GdkGLProc_glDeleteBufferRegion) (GLenum region);
GdkGLProc    gdk_gl_get_glDeleteBufferRegion (void);
#define      gdk_gl_glDeleteBufferRegion(proc, region) \
  ( ((GdkGLProc_glDeleteBufferRegion) (proc)) (region) )

/* glReadBufferRegion */
typedef void (APIENTRY * GdkGLProc_glReadBufferRegion) (GLuint region, GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glReadBufferRegion (void);
#define      gdk_gl_glReadBufferRegion(proc, region, x, y, width, height) \
  ( ((GdkGLProc_glReadBufferRegion) (proc)) (region, x, y, width, height) )

/* glDrawBufferRegion */
typedef void (APIENTRY * GdkGLProc_glDrawBufferRegion) (GLuint region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest);
GdkGLProc    gdk_gl_get_glDrawBufferRegion (void);
#define      gdk_gl_glDrawBufferRegion(proc, region, x, y, width, height, xDest, yDest) \
  ( ((GdkGLProc_glDrawBufferRegion) (proc)) (region, x, y, width, height, xDest, yDest) )

/* proc struct */

typedef struct _GdkGL_GL_KTX_buffer_region GdkGL_GL_KTX_buffer_region;

struct _GdkGL_GL_KTX_buffer_region
{
  GdkGLProc_glBufferRegionEnabled glBufferRegionEnabled;
  GdkGLProc_glNewBufferRegion glNewBufferRegion;
  GdkGLProc_glDeleteBufferRegion glDeleteBufferRegion;
  GdkGLProc_glReadBufferRegion glReadBufferRegion;
  GdkGLProc_glDrawBufferRegion glDrawBufferRegion;
};

GdkGL_GL_KTX_buffer_region *gdk_gl_get_GL_KTX_buffer_region (void);

/*
 * GL_NV_element_array
 */

/* glElementPointerNV */
typedef void (APIENTRY * GdkGLProc_glElementPointerNV) (GLenum type, const GLvoid *pointer);
GdkGLProc    gdk_gl_get_glElementPointerNV (void);
#define      gdk_gl_glElementPointerNV(proc, type, pointer) \
  ( ((GdkGLProc_glElementPointerNV) (proc)) (type, pointer) )

/* glDrawElementArrayNV */
typedef void (APIENTRY * GdkGLProc_glDrawElementArrayNV) (GLenum mode, GLint first, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawElementArrayNV (void);
#define      gdk_gl_glDrawElementArrayNV(proc, mode, first, count) \
  ( ((GdkGLProc_glDrawElementArrayNV) (proc)) (mode, first, count) )

/* glDrawRangeElementArrayNV */
typedef void (APIENTRY * GdkGLProc_glDrawRangeElementArrayNV) (GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
GdkGLProc    gdk_gl_get_glDrawRangeElementArrayNV (void);
#define      gdk_gl_glDrawRangeElementArrayNV(proc, mode, start, end, first, count) \
  ( ((GdkGLProc_glDrawRangeElementArrayNV) (proc)) (mode, start, end, first, count) )

/* glMultiDrawElementArrayNV */
typedef void (APIENTRY * GdkGLProc_glMultiDrawElementArrayNV) (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawElementArrayNV (void);
#define      gdk_gl_glMultiDrawElementArrayNV(proc, mode, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawElementArrayNV) (proc)) (mode, first, count, primcount) )

/* glMultiDrawRangeElementArrayNV */
typedef void (APIENTRY * GdkGLProc_glMultiDrawRangeElementArrayNV) (GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawRangeElementArrayNV (void);
#define      gdk_gl_glMultiDrawRangeElementArrayNV(proc, mode, start, end, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawRangeElementArrayNV) (proc)) (mode, start, end, first, count, primcount) )

/* proc struct */

typedef struct _GdkGL_GL_NV_element_array GdkGL_GL_NV_element_array;

struct _GdkGL_GL_NV_element_array
{
  GdkGLProc_glElementPointerNV glElementPointerNV;
  GdkGLProc_glDrawElementArrayNV glDrawElementArrayNV;
  GdkGLProc_glDrawRangeElementArrayNV glDrawRangeElementArrayNV;
  GdkGLProc_glMultiDrawElementArrayNV glMultiDrawElementArrayNV;
  GdkGLProc_glMultiDrawRangeElementArrayNV glMultiDrawRangeElementArrayNV;
};

GdkGL_GL_NV_element_array *gdk_gl_get_GL_NV_element_array (void);

/*
 * GL_NV_stencil_two_side
 */

/* glActiveStencilFaceNV */
typedef void (APIENTRY * GdkGLProc_glActiveStencilFaceNV) (GLenum face);
GdkGLProc    gdk_gl_get_glActiveStencilFaceNV (void);
#define      gdk_gl_glActiveStencilFaceNV(proc, face) \
  ( ((GdkGLProc_glActiveStencilFaceNV) (proc)) (face) )

/* proc struct */

typedef struct _GdkGL_GL_NV_stencil_two_side GdkGL_GL_NV_stencil_two_side;

struct _GdkGL_GL_NV_stencil_two_side
{
  GdkGLProc_glActiveStencilFaceNV glActiveStencilFaceNV;
};

GdkGL_GL_NV_stencil_two_side *gdk_gl_get_GL_NV_stencil_two_side (void);

/*
 * GL_SGIS_multitexture
 */

/* glMultiTexCoord1dSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dSGIS) (GLenum target, GLdouble s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dSGIS (void);
#define      gdk_gl_glMultiTexCoord1dSGIS(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1dSGIS) (proc)) (target, s) )

/* glMultiTexCoord1dvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1dvSGIS) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1dvSGIS (void);
#define      gdk_gl_glMultiTexCoord1dvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1dvSGIS) (proc)) (target, v) )

/* glMultiTexCoord1fSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fSGIS) (GLenum target, GLfloat s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fSGIS (void);
#define      gdk_gl_glMultiTexCoord1fSGIS(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1fSGIS) (proc)) (target, s) )

/* glMultiTexCoord1fvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1fvSGIS) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1fvSGIS (void);
#define      gdk_gl_glMultiTexCoord1fvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1fvSGIS) (proc)) (target, v) )

/* glMultiTexCoord1iSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1iSGIS) (GLenum target, GLint s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1iSGIS (void);
#define      gdk_gl_glMultiTexCoord1iSGIS(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1iSGIS) (proc)) (target, s) )

/* glMultiTexCoord1ivSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1ivSGIS) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1ivSGIS (void);
#define      gdk_gl_glMultiTexCoord1ivSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1ivSGIS) (proc)) (target, v) )

/* glMultiTexCoord1sSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1sSGIS) (GLenum target, GLshort s);
GdkGLProc    gdk_gl_get_glMultiTexCoord1sSGIS (void);
#define      gdk_gl_glMultiTexCoord1sSGIS(proc, target, s) \
  ( ((GdkGLProc_glMultiTexCoord1sSGIS) (proc)) (target, s) )

/* glMultiTexCoord1svSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord1svSGIS) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord1svSGIS (void);
#define      gdk_gl_glMultiTexCoord1svSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord1svSGIS) (proc)) (target, v) )

/* glMultiTexCoord2dSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dSGIS) (GLenum target, GLdouble s, GLdouble t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dSGIS (void);
#define      gdk_gl_glMultiTexCoord2dSGIS(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2dSGIS) (proc)) (target, s, t) )

/* glMultiTexCoord2dvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2dvSGIS) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2dvSGIS (void);
#define      gdk_gl_glMultiTexCoord2dvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2dvSGIS) (proc)) (target, v) )

/* glMultiTexCoord2fSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fSGIS) (GLenum target, GLfloat s, GLfloat t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fSGIS (void);
#define      gdk_gl_glMultiTexCoord2fSGIS(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2fSGIS) (proc)) (target, s, t) )

/* glMultiTexCoord2fvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2fvSGIS) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2fvSGIS (void);
#define      gdk_gl_glMultiTexCoord2fvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2fvSGIS) (proc)) (target, v) )

/* glMultiTexCoord2iSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2iSGIS) (GLenum target, GLint s, GLint t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2iSGIS (void);
#define      gdk_gl_glMultiTexCoord2iSGIS(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2iSGIS) (proc)) (target, s, t) )

/* glMultiTexCoord2ivSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2ivSGIS) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2ivSGIS (void);
#define      gdk_gl_glMultiTexCoord2ivSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2ivSGIS) (proc)) (target, v) )

/* glMultiTexCoord2sSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2sSGIS) (GLenum target, GLshort s, GLshort t);
GdkGLProc    gdk_gl_get_glMultiTexCoord2sSGIS (void);
#define      gdk_gl_glMultiTexCoord2sSGIS(proc, target, s, t) \
  ( ((GdkGLProc_glMultiTexCoord2sSGIS) (proc)) (target, s, t) )

/* glMultiTexCoord2svSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord2svSGIS) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord2svSGIS (void);
#define      gdk_gl_glMultiTexCoord2svSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord2svSGIS) (proc)) (target, v) )

/* glMultiTexCoord3dSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dSGIS) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dSGIS (void);
#define      gdk_gl_glMultiTexCoord3dSGIS(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3dSGIS) (proc)) (target, s, t, r) )

/* glMultiTexCoord3dvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3dvSGIS) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3dvSGIS (void);
#define      gdk_gl_glMultiTexCoord3dvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3dvSGIS) (proc)) (target, v) )

/* glMultiTexCoord3fSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fSGIS) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fSGIS (void);
#define      gdk_gl_glMultiTexCoord3fSGIS(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3fSGIS) (proc)) (target, s, t, r) )

/* glMultiTexCoord3fvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3fvSGIS) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3fvSGIS (void);
#define      gdk_gl_glMultiTexCoord3fvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3fvSGIS) (proc)) (target, v) )

/* glMultiTexCoord3iSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3iSGIS) (GLenum target, GLint s, GLint t, GLint r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3iSGIS (void);
#define      gdk_gl_glMultiTexCoord3iSGIS(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3iSGIS) (proc)) (target, s, t, r) )

/* glMultiTexCoord3ivSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3ivSGIS) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3ivSGIS (void);
#define      gdk_gl_glMultiTexCoord3ivSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3ivSGIS) (proc)) (target, v) )

/* glMultiTexCoord3sSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3sSGIS) (GLenum target, GLshort s, GLshort t, GLshort r);
GdkGLProc    gdk_gl_get_glMultiTexCoord3sSGIS (void);
#define      gdk_gl_glMultiTexCoord3sSGIS(proc, target, s, t, r) \
  ( ((GdkGLProc_glMultiTexCoord3sSGIS) (proc)) (target, s, t, r) )

/* glMultiTexCoord3svSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord3svSGIS) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord3svSGIS (void);
#define      gdk_gl_glMultiTexCoord3svSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord3svSGIS) (proc)) (target, v) )

/* glMultiTexCoord4dSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dSGIS) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dSGIS (void);
#define      gdk_gl_glMultiTexCoord4dSGIS(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4dSGIS) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4dvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4dvSGIS) (GLenum target, const GLdouble *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4dvSGIS (void);
#define      gdk_gl_glMultiTexCoord4dvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4dvSGIS) (proc)) (target, v) )

/* glMultiTexCoord4fSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fSGIS) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fSGIS (void);
#define      gdk_gl_glMultiTexCoord4fSGIS(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4fSGIS) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4fvSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4fvSGIS) (GLenum target, const GLfloat *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4fvSGIS (void);
#define      gdk_gl_glMultiTexCoord4fvSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4fvSGIS) (proc)) (target, v) )

/* glMultiTexCoord4iSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4iSGIS) (GLenum target, GLint s, GLint t, GLint r, GLint q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4iSGIS (void);
#define      gdk_gl_glMultiTexCoord4iSGIS(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4iSGIS) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4ivSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4ivSGIS) (GLenum target, const GLint *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4ivSGIS (void);
#define      gdk_gl_glMultiTexCoord4ivSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4ivSGIS) (proc)) (target, v) )

/* glMultiTexCoord4sSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4sSGIS) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
GdkGLProc    gdk_gl_get_glMultiTexCoord4sSGIS (void);
#define      gdk_gl_glMultiTexCoord4sSGIS(proc, target, s, t, r, q) \
  ( ((GdkGLProc_glMultiTexCoord4sSGIS) (proc)) (target, s, t, r, q) )

/* glMultiTexCoord4svSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoord4svSGIS) (GLenum target, const GLshort *v);
GdkGLProc    gdk_gl_get_glMultiTexCoord4svSGIS (void);
#define      gdk_gl_glMultiTexCoord4svSGIS(proc, target, v) \
  ( ((GdkGLProc_glMultiTexCoord4svSGIS) (proc)) (target, v) )

/* glMultiTexCoordPointerSGIS */
typedef void (APIENTRY * GdkGLProc_glMultiTexCoordPointerSGIS) (GLenum target, GLint size, GLenum type, GLsizei stride, const void *pointer);
GdkGLProc    gdk_gl_get_glMultiTexCoordPointerSGIS (void);
#define      gdk_gl_glMultiTexCoordPointerSGIS(proc, target, size, type, stride, pointer) \
  ( ((GdkGLProc_glMultiTexCoordPointerSGIS) (proc)) (target, size, type, stride, pointer) )

/* glSelectTextureSGIS */
typedef void (APIENTRY * GdkGLProc_glSelectTextureSGIS) (GLenum target);
GdkGLProc    gdk_gl_get_glSelectTextureSGIS (void);
#define      gdk_gl_glSelectTextureSGIS(proc, target) \
  ( ((GdkGLProc_glSelectTextureSGIS) (proc)) (target) )

/* glSelectTextureCoordSetSGIS */
typedef void (APIENTRY * GdkGLProc_glSelectTextureCoordSetSGIS) (GLenum target);
GdkGLProc    gdk_gl_get_glSelectTextureCoordSetSGIS (void);
#define      gdk_gl_glSelectTextureCoordSetSGIS(proc, target) \
  ( ((GdkGLProc_glSelectTextureCoordSetSGIS) (proc)) (target) )

/* proc struct */

typedef struct _GdkGL_GL_SGIS_multitexture GdkGL_GL_SGIS_multitexture;

struct _GdkGL_GL_SGIS_multitexture
{
  GdkGLProc_glMultiTexCoord1dSGIS glMultiTexCoord1dSGIS;
  GdkGLProc_glMultiTexCoord1dvSGIS glMultiTexCoord1dvSGIS;
  GdkGLProc_glMultiTexCoord1fSGIS glMultiTexCoord1fSGIS;
  GdkGLProc_glMultiTexCoord1fvSGIS glMultiTexCoord1fvSGIS;
  GdkGLProc_glMultiTexCoord1iSGIS glMultiTexCoord1iSGIS;
  GdkGLProc_glMultiTexCoord1ivSGIS glMultiTexCoord1ivSGIS;
  GdkGLProc_glMultiTexCoord1sSGIS glMultiTexCoord1sSGIS;
  GdkGLProc_glMultiTexCoord1svSGIS glMultiTexCoord1svSGIS;
  GdkGLProc_glMultiTexCoord2dSGIS glMultiTexCoord2dSGIS;
  GdkGLProc_glMultiTexCoord2dvSGIS glMultiTexCoord2dvSGIS;
  GdkGLProc_glMultiTexCoord2fSGIS glMultiTexCoord2fSGIS;
  GdkGLProc_glMultiTexCoord2fvSGIS glMultiTexCoord2fvSGIS;
  GdkGLProc_glMultiTexCoord2iSGIS glMultiTexCoord2iSGIS;
  GdkGLProc_glMultiTexCoord2ivSGIS glMultiTexCoord2ivSGIS;
  GdkGLProc_glMultiTexCoord2sSGIS glMultiTexCoord2sSGIS;
  GdkGLProc_glMultiTexCoord2svSGIS glMultiTexCoord2svSGIS;
  GdkGLProc_glMultiTexCoord3dSGIS glMultiTexCoord3dSGIS;
  GdkGLProc_glMultiTexCoord3dvSGIS glMultiTexCoord3dvSGIS;
  GdkGLProc_glMultiTexCoord3fSGIS glMultiTexCoord3fSGIS;
  GdkGLProc_glMultiTexCoord3fvSGIS glMultiTexCoord3fvSGIS;
  GdkGLProc_glMultiTexCoord3iSGIS glMultiTexCoord3iSGIS;
  GdkGLProc_glMultiTexCoord3ivSGIS glMultiTexCoord3ivSGIS;
  GdkGLProc_glMultiTexCoord3sSGIS glMultiTexCoord3sSGIS;
  GdkGLProc_glMultiTexCoord3svSGIS glMultiTexCoord3svSGIS;
  GdkGLProc_glMultiTexCoord4dSGIS glMultiTexCoord4dSGIS;
  GdkGLProc_glMultiTexCoord4dvSGIS glMultiTexCoord4dvSGIS;
  GdkGLProc_glMultiTexCoord4fSGIS glMultiTexCoord4fSGIS;
  GdkGLProc_glMultiTexCoord4fvSGIS glMultiTexCoord4fvSGIS;
  GdkGLProc_glMultiTexCoord4iSGIS glMultiTexCoord4iSGIS;
  GdkGLProc_glMultiTexCoord4ivSGIS glMultiTexCoord4ivSGIS;
  GdkGLProc_glMultiTexCoord4sSGIS glMultiTexCoord4sSGIS;
  GdkGLProc_glMultiTexCoord4svSGIS glMultiTexCoord4svSGIS;
  GdkGLProc_glMultiTexCoordPointerSGIS glMultiTexCoordPointerSGIS;
  GdkGLProc_glSelectTextureSGIS glSelectTextureSGIS;
  GdkGLProc_glSelectTextureCoordSetSGIS glSelectTextureCoordSetSGIS;
};

GdkGL_GL_SGIS_multitexture *gdk_gl_get_GL_SGIS_multitexture (void);

/*
 * GL_SGIX_fog_texture
 */

/* glTextureFogSGIX */
typedef void (APIENTRY * GdkGLProc_glTextureFogSGIX) (GLenum pname);
GdkGLProc    gdk_gl_get_glTextureFogSGIX (void);
#define      gdk_gl_glTextureFogSGIX(proc, pname) \
  ( ((GdkGLProc_glTextureFogSGIX) (proc)) (pname) )

/* proc struct */

typedef struct _GdkGL_GL_SGIX_fog_texture GdkGL_GL_SGIX_fog_texture;

struct _GdkGL_GL_SGIX_fog_texture
{
  GdkGLProc_glTextureFogSGIX glTextureFogSGIX;
};

GdkGL_GL_SGIX_fog_texture *gdk_gl_get_GL_SGIX_fog_texture (void);

/*
 * GL_SUN_multi_draw_arrays
 */

/* glMultiDrawArraysSUN */
typedef void (APIENTRY * GdkGLProc_glMultiDrawArraysSUN) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawArraysSUN (void);
#define      gdk_gl_glMultiDrawArraysSUN(proc, mode, first, count, primcount) \
  ( ((GdkGLProc_glMultiDrawArraysSUN) (proc)) (mode, first, count, primcount) )

/* glMultiDrawElementsSUN */
typedef void (APIENTRY * GdkGLProc_glMultiDrawElementsSUN) (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
GdkGLProc    gdk_gl_get_glMultiDrawElementsSUN (void);
#define      gdk_gl_glMultiDrawElementsSUN(proc, mode, count, type, indices, primcount) \
  ( ((GdkGLProc_glMultiDrawElementsSUN) (proc)) (mode, count, type, indices, primcount) )

/* proc struct */

typedef struct _GdkGL_GL_SUN_multi_draw_arrays GdkGL_GL_SUN_multi_draw_arrays;

struct _GdkGL_GL_SUN_multi_draw_arrays
{
  GdkGLProc_glMultiDrawArraysSUN glMultiDrawArraysSUN;
  GdkGLProc_glMultiDrawElementsSUN glMultiDrawElementsSUN;
};

GdkGL_GL_SUN_multi_draw_arrays *gdk_gl_get_GL_SUN_multi_draw_arrays (void);

/*
 * GL_WIN_swap_hint
 */

/* glAddSwapHintRectWIN */
typedef void (APIENTRY * GdkGLProc_glAddSwapHintRectWIN) (GLint x, GLint y, GLsizei width, GLsizei height);
GdkGLProc    gdk_gl_get_glAddSwapHintRectWIN (void);
#define      gdk_gl_glAddSwapHintRectWIN(proc, x, y, width, height) \
  ( ((GdkGLProc_glAddSwapHintRectWIN) (proc)) (x, y, width, height) )

/* proc struct */

typedef struct _GdkGL_GL_WIN_swap_hint GdkGL_GL_WIN_swap_hint;

struct _GdkGL_GL_WIN_swap_hint
{
  GdkGLProc_glAddSwapHintRectWIN glAddSwapHintRectWIN;
};

GdkGL_GL_WIN_swap_hint *gdk_gl_get_GL_WIN_swap_hint (void);

G_END_DECLS

#endif /* __GDK_GL_GLEXT_H__ */
