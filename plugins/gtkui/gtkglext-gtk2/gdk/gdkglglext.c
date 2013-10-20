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
 * This is a generated file.  Please modify "gen-gdkglglext-c.pl".
 */

#include "gdkglprivate.h"
#include "gdkglquery.h"
#include "gdkglcontext.h"
#include "gdkglglext.h"

/*
 * GL_VERSION_1_2
 */

static GdkGL_GL_VERSION_1_2 _procs_GL_VERSION_1_2 = {
  (GdkGLProc_glBlendColor) -1,
  (GdkGLProc_glBlendEquation) -1,
  (GdkGLProc_glDrawRangeElements) -1,
  (GdkGLProc_glColorTable) -1,
  (GdkGLProc_glColorTableParameterfv) -1,
  (GdkGLProc_glColorTableParameteriv) -1,
  (GdkGLProc_glCopyColorTable) -1,
  (GdkGLProc_glGetColorTable) -1,
  (GdkGLProc_glGetColorTableParameterfv) -1,
  (GdkGLProc_glGetColorTableParameteriv) -1,
  (GdkGLProc_glColorSubTable) -1,
  (GdkGLProc_glCopyColorSubTable) -1,
  (GdkGLProc_glConvolutionFilter1D) -1,
  (GdkGLProc_glConvolutionFilter2D) -1,
  (GdkGLProc_glConvolutionParameterf) -1,
  (GdkGLProc_glConvolutionParameterfv) -1,
  (GdkGLProc_glConvolutionParameteri) -1,
  (GdkGLProc_glConvolutionParameteriv) -1,
  (GdkGLProc_glCopyConvolutionFilter1D) -1,
  (GdkGLProc_glCopyConvolutionFilter2D) -1,
  (GdkGLProc_glGetConvolutionFilter) -1,
  (GdkGLProc_glGetConvolutionParameterfv) -1,
  (GdkGLProc_glGetConvolutionParameteriv) -1,
  (GdkGLProc_glGetSeparableFilter) -1,
  (GdkGLProc_glSeparableFilter2D) -1,
  (GdkGLProc_glGetHistogram) -1,
  (GdkGLProc_glGetHistogramParameterfv) -1,
  (GdkGLProc_glGetHistogramParameteriv) -1,
  (GdkGLProc_glGetMinmax) -1,
  (GdkGLProc_glGetMinmaxParameterfv) -1,
  (GdkGLProc_glGetMinmaxParameteriv) -1,
  (GdkGLProc_glHistogram) -1,
  (GdkGLProc_glMinmax) -1,
  (GdkGLProc_glResetHistogram) -1,
  (GdkGLProc_glResetMinmax) -1,
  (GdkGLProc_glTexImage3D) -1,
  (GdkGLProc_glTexSubImage3D) -1,
  (GdkGLProc_glCopyTexSubImage3D) -1
};

/* glBlendColor */
GdkGLProc
gdk_gl_get_glBlendColor (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glBlendColor == (GdkGLProc_glBlendColor) -1)
    _procs_GL_VERSION_1_2.glBlendColor =
      (GdkGLProc_glBlendColor) gdk_gl_get_proc_address ("glBlendColor");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendColor () - %s",
               (_procs_GL_VERSION_1_2.glBlendColor) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glBlendColor);
}

/* glBlendEquation */
GdkGLProc
gdk_gl_get_glBlendEquation (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glBlendEquation == (GdkGLProc_glBlendEquation) -1)
    _procs_GL_VERSION_1_2.glBlendEquation =
      (GdkGLProc_glBlendEquation) gdk_gl_get_proc_address ("glBlendEquation");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendEquation () - %s",
               (_procs_GL_VERSION_1_2.glBlendEquation) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glBlendEquation);
}

/* glDrawRangeElements */
GdkGLProc
gdk_gl_get_glDrawRangeElements (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glDrawRangeElements == (GdkGLProc_glDrawRangeElements) -1)
    _procs_GL_VERSION_1_2.glDrawRangeElements =
      (GdkGLProc_glDrawRangeElements) gdk_gl_get_proc_address ("glDrawRangeElements");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawRangeElements () - %s",
               (_procs_GL_VERSION_1_2.glDrawRangeElements) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glDrawRangeElements);
}

/* glColorTable */
GdkGLProc
gdk_gl_get_glColorTable (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glColorTable == (GdkGLProc_glColorTable) -1)
    _procs_GL_VERSION_1_2.glColorTable =
      (GdkGLProc_glColorTable) gdk_gl_get_proc_address ("glColorTable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTable () - %s",
               (_procs_GL_VERSION_1_2.glColorTable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glColorTable);
}

/* glColorTableParameterfv */
GdkGLProc
gdk_gl_get_glColorTableParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glColorTableParameterfv == (GdkGLProc_glColorTableParameterfv) -1)
    _procs_GL_VERSION_1_2.glColorTableParameterfv =
      (GdkGLProc_glColorTableParameterfv) gdk_gl_get_proc_address ("glColorTableParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTableParameterfv () - %s",
               (_procs_GL_VERSION_1_2.glColorTableParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glColorTableParameterfv);
}

/* glColorTableParameteriv */
GdkGLProc
gdk_gl_get_glColorTableParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glColorTableParameteriv == (GdkGLProc_glColorTableParameteriv) -1)
    _procs_GL_VERSION_1_2.glColorTableParameteriv =
      (GdkGLProc_glColorTableParameteriv) gdk_gl_get_proc_address ("glColorTableParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTableParameteriv () - %s",
               (_procs_GL_VERSION_1_2.glColorTableParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glColorTableParameteriv);
}

/* glCopyColorTable */
GdkGLProc
gdk_gl_get_glCopyColorTable (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glCopyColorTable == (GdkGLProc_glCopyColorTable) -1)
    _procs_GL_VERSION_1_2.glCopyColorTable =
      (GdkGLProc_glCopyColorTable) gdk_gl_get_proc_address ("glCopyColorTable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyColorTable () - %s",
               (_procs_GL_VERSION_1_2.glCopyColorTable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glCopyColorTable);
}

/* glGetColorTable */
GdkGLProc
gdk_gl_get_glGetColorTable (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetColorTable == (GdkGLProc_glGetColorTable) -1)
    _procs_GL_VERSION_1_2.glGetColorTable =
      (GdkGLProc_glGetColorTable) gdk_gl_get_proc_address ("glGetColorTable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTable () - %s",
               (_procs_GL_VERSION_1_2.glGetColorTable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetColorTable);
}

/* glGetColorTableParameterfv */
GdkGLProc
gdk_gl_get_glGetColorTableParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetColorTableParameterfv == (GdkGLProc_glGetColorTableParameterfv) -1)
    _procs_GL_VERSION_1_2.glGetColorTableParameterfv =
      (GdkGLProc_glGetColorTableParameterfv) gdk_gl_get_proc_address ("glGetColorTableParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableParameterfv () - %s",
               (_procs_GL_VERSION_1_2.glGetColorTableParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetColorTableParameterfv);
}

/* glGetColorTableParameteriv */
GdkGLProc
gdk_gl_get_glGetColorTableParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetColorTableParameteriv == (GdkGLProc_glGetColorTableParameteriv) -1)
    _procs_GL_VERSION_1_2.glGetColorTableParameteriv =
      (GdkGLProc_glGetColorTableParameteriv) gdk_gl_get_proc_address ("glGetColorTableParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableParameteriv () - %s",
               (_procs_GL_VERSION_1_2.glGetColorTableParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetColorTableParameteriv);
}

/* glColorSubTable */
GdkGLProc
gdk_gl_get_glColorSubTable (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glColorSubTable == (GdkGLProc_glColorSubTable) -1)
    _procs_GL_VERSION_1_2.glColorSubTable =
      (GdkGLProc_glColorSubTable) gdk_gl_get_proc_address ("glColorSubTable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorSubTable () - %s",
               (_procs_GL_VERSION_1_2.glColorSubTable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glColorSubTable);
}

/* glCopyColorSubTable */
GdkGLProc
gdk_gl_get_glCopyColorSubTable (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glCopyColorSubTable == (GdkGLProc_glCopyColorSubTable) -1)
    _procs_GL_VERSION_1_2.glCopyColorSubTable =
      (GdkGLProc_glCopyColorSubTable) gdk_gl_get_proc_address ("glCopyColorSubTable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyColorSubTable () - %s",
               (_procs_GL_VERSION_1_2.glCopyColorSubTable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glCopyColorSubTable);
}

/* glConvolutionFilter1D */
GdkGLProc
gdk_gl_get_glConvolutionFilter1D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glConvolutionFilter1D == (GdkGLProc_glConvolutionFilter1D) -1)
    _procs_GL_VERSION_1_2.glConvolutionFilter1D =
      (GdkGLProc_glConvolutionFilter1D) gdk_gl_get_proc_address ("glConvolutionFilter1D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionFilter1D () - %s",
               (_procs_GL_VERSION_1_2.glConvolutionFilter1D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glConvolutionFilter1D);
}

/* glConvolutionFilter2D */
GdkGLProc
gdk_gl_get_glConvolutionFilter2D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glConvolutionFilter2D == (GdkGLProc_glConvolutionFilter2D) -1)
    _procs_GL_VERSION_1_2.glConvolutionFilter2D =
      (GdkGLProc_glConvolutionFilter2D) gdk_gl_get_proc_address ("glConvolutionFilter2D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionFilter2D () - %s",
               (_procs_GL_VERSION_1_2.glConvolutionFilter2D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glConvolutionFilter2D);
}

/* glConvolutionParameterf */
GdkGLProc
gdk_gl_get_glConvolutionParameterf (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glConvolutionParameterf == (GdkGLProc_glConvolutionParameterf) -1)
    _procs_GL_VERSION_1_2.glConvolutionParameterf =
      (GdkGLProc_glConvolutionParameterf) gdk_gl_get_proc_address ("glConvolutionParameterf");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameterf () - %s",
               (_procs_GL_VERSION_1_2.glConvolutionParameterf) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glConvolutionParameterf);
}

/* glConvolutionParameterfv */
GdkGLProc
gdk_gl_get_glConvolutionParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glConvolutionParameterfv == (GdkGLProc_glConvolutionParameterfv) -1)
    _procs_GL_VERSION_1_2.glConvolutionParameterfv =
      (GdkGLProc_glConvolutionParameterfv) gdk_gl_get_proc_address ("glConvolutionParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameterfv () - %s",
               (_procs_GL_VERSION_1_2.glConvolutionParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glConvolutionParameterfv);
}

/* glConvolutionParameteri */
GdkGLProc
gdk_gl_get_glConvolutionParameteri (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glConvolutionParameteri == (GdkGLProc_glConvolutionParameteri) -1)
    _procs_GL_VERSION_1_2.glConvolutionParameteri =
      (GdkGLProc_glConvolutionParameteri) gdk_gl_get_proc_address ("glConvolutionParameteri");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameteri () - %s",
               (_procs_GL_VERSION_1_2.glConvolutionParameteri) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glConvolutionParameteri);
}

/* glConvolutionParameteriv */
GdkGLProc
gdk_gl_get_glConvolutionParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glConvolutionParameteriv == (GdkGLProc_glConvolutionParameteriv) -1)
    _procs_GL_VERSION_1_2.glConvolutionParameteriv =
      (GdkGLProc_glConvolutionParameteriv) gdk_gl_get_proc_address ("glConvolutionParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameteriv () - %s",
               (_procs_GL_VERSION_1_2.glConvolutionParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glConvolutionParameteriv);
}

/* glCopyConvolutionFilter1D */
GdkGLProc
gdk_gl_get_glCopyConvolutionFilter1D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glCopyConvolutionFilter1D == (GdkGLProc_glCopyConvolutionFilter1D) -1)
    _procs_GL_VERSION_1_2.glCopyConvolutionFilter1D =
      (GdkGLProc_glCopyConvolutionFilter1D) gdk_gl_get_proc_address ("glCopyConvolutionFilter1D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyConvolutionFilter1D () - %s",
               (_procs_GL_VERSION_1_2.glCopyConvolutionFilter1D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glCopyConvolutionFilter1D);
}

/* glCopyConvolutionFilter2D */
GdkGLProc
gdk_gl_get_glCopyConvolutionFilter2D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glCopyConvolutionFilter2D == (GdkGLProc_glCopyConvolutionFilter2D) -1)
    _procs_GL_VERSION_1_2.glCopyConvolutionFilter2D =
      (GdkGLProc_glCopyConvolutionFilter2D) gdk_gl_get_proc_address ("glCopyConvolutionFilter2D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyConvolutionFilter2D () - %s",
               (_procs_GL_VERSION_1_2.glCopyConvolutionFilter2D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glCopyConvolutionFilter2D);
}

/* glGetConvolutionFilter */
GdkGLProc
gdk_gl_get_glGetConvolutionFilter (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetConvolutionFilter == (GdkGLProc_glGetConvolutionFilter) -1)
    _procs_GL_VERSION_1_2.glGetConvolutionFilter =
      (GdkGLProc_glGetConvolutionFilter) gdk_gl_get_proc_address ("glGetConvolutionFilter");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetConvolutionFilter () - %s",
               (_procs_GL_VERSION_1_2.glGetConvolutionFilter) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetConvolutionFilter);
}

/* glGetConvolutionParameterfv */
GdkGLProc
gdk_gl_get_glGetConvolutionParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetConvolutionParameterfv == (GdkGLProc_glGetConvolutionParameterfv) -1)
    _procs_GL_VERSION_1_2.glGetConvolutionParameterfv =
      (GdkGLProc_glGetConvolutionParameterfv) gdk_gl_get_proc_address ("glGetConvolutionParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetConvolutionParameterfv () - %s",
               (_procs_GL_VERSION_1_2.glGetConvolutionParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetConvolutionParameterfv);
}

/* glGetConvolutionParameteriv */
GdkGLProc
gdk_gl_get_glGetConvolutionParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetConvolutionParameteriv == (GdkGLProc_glGetConvolutionParameteriv) -1)
    _procs_GL_VERSION_1_2.glGetConvolutionParameteriv =
      (GdkGLProc_glGetConvolutionParameteriv) gdk_gl_get_proc_address ("glGetConvolutionParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetConvolutionParameteriv () - %s",
               (_procs_GL_VERSION_1_2.glGetConvolutionParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetConvolutionParameteriv);
}

/* glGetSeparableFilter */
GdkGLProc
gdk_gl_get_glGetSeparableFilter (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetSeparableFilter == (GdkGLProc_glGetSeparableFilter) -1)
    _procs_GL_VERSION_1_2.glGetSeparableFilter =
      (GdkGLProc_glGetSeparableFilter) gdk_gl_get_proc_address ("glGetSeparableFilter");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetSeparableFilter () - %s",
               (_procs_GL_VERSION_1_2.glGetSeparableFilter) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetSeparableFilter);
}

/* glSeparableFilter2D */
GdkGLProc
gdk_gl_get_glSeparableFilter2D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glSeparableFilter2D == (GdkGLProc_glSeparableFilter2D) -1)
    _procs_GL_VERSION_1_2.glSeparableFilter2D =
      (GdkGLProc_glSeparableFilter2D) gdk_gl_get_proc_address ("glSeparableFilter2D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSeparableFilter2D () - %s",
               (_procs_GL_VERSION_1_2.glSeparableFilter2D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glSeparableFilter2D);
}

/* glGetHistogram */
GdkGLProc
gdk_gl_get_glGetHistogram (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetHistogram == (GdkGLProc_glGetHistogram) -1)
    _procs_GL_VERSION_1_2.glGetHistogram =
      (GdkGLProc_glGetHistogram) gdk_gl_get_proc_address ("glGetHistogram");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetHistogram () - %s",
               (_procs_GL_VERSION_1_2.glGetHistogram) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetHistogram);
}

/* glGetHistogramParameterfv */
GdkGLProc
gdk_gl_get_glGetHistogramParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetHistogramParameterfv == (GdkGLProc_glGetHistogramParameterfv) -1)
    _procs_GL_VERSION_1_2.glGetHistogramParameterfv =
      (GdkGLProc_glGetHistogramParameterfv) gdk_gl_get_proc_address ("glGetHistogramParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetHistogramParameterfv () - %s",
               (_procs_GL_VERSION_1_2.glGetHistogramParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetHistogramParameterfv);
}

/* glGetHistogramParameteriv */
GdkGLProc
gdk_gl_get_glGetHistogramParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetHistogramParameteriv == (GdkGLProc_glGetHistogramParameteriv) -1)
    _procs_GL_VERSION_1_2.glGetHistogramParameteriv =
      (GdkGLProc_glGetHistogramParameteriv) gdk_gl_get_proc_address ("glGetHistogramParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetHistogramParameteriv () - %s",
               (_procs_GL_VERSION_1_2.glGetHistogramParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetHistogramParameteriv);
}

/* glGetMinmax */
GdkGLProc
gdk_gl_get_glGetMinmax (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetMinmax == (GdkGLProc_glGetMinmax) -1)
    _procs_GL_VERSION_1_2.glGetMinmax =
      (GdkGLProc_glGetMinmax) gdk_gl_get_proc_address ("glGetMinmax");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMinmax () - %s",
               (_procs_GL_VERSION_1_2.glGetMinmax) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetMinmax);
}

/* glGetMinmaxParameterfv */
GdkGLProc
gdk_gl_get_glGetMinmaxParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetMinmaxParameterfv == (GdkGLProc_glGetMinmaxParameterfv) -1)
    _procs_GL_VERSION_1_2.glGetMinmaxParameterfv =
      (GdkGLProc_glGetMinmaxParameterfv) gdk_gl_get_proc_address ("glGetMinmaxParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMinmaxParameterfv () - %s",
               (_procs_GL_VERSION_1_2.glGetMinmaxParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetMinmaxParameterfv);
}

/* glGetMinmaxParameteriv */
GdkGLProc
gdk_gl_get_glGetMinmaxParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glGetMinmaxParameteriv == (GdkGLProc_glGetMinmaxParameteriv) -1)
    _procs_GL_VERSION_1_2.glGetMinmaxParameteriv =
      (GdkGLProc_glGetMinmaxParameteriv) gdk_gl_get_proc_address ("glGetMinmaxParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMinmaxParameteriv () - %s",
               (_procs_GL_VERSION_1_2.glGetMinmaxParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glGetMinmaxParameteriv);
}

/* glHistogram */
GdkGLProc
gdk_gl_get_glHistogram (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glHistogram == (GdkGLProc_glHistogram) -1)
    _procs_GL_VERSION_1_2.glHistogram =
      (GdkGLProc_glHistogram) gdk_gl_get_proc_address ("glHistogram");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glHistogram () - %s",
               (_procs_GL_VERSION_1_2.glHistogram) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glHistogram);
}

/* glMinmax */
GdkGLProc
gdk_gl_get_glMinmax (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glMinmax == (GdkGLProc_glMinmax) -1)
    _procs_GL_VERSION_1_2.glMinmax =
      (GdkGLProc_glMinmax) gdk_gl_get_proc_address ("glMinmax");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMinmax () - %s",
               (_procs_GL_VERSION_1_2.glMinmax) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glMinmax);
}

/* glResetHistogram */
GdkGLProc
gdk_gl_get_glResetHistogram (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glResetHistogram == (GdkGLProc_glResetHistogram) -1)
    _procs_GL_VERSION_1_2.glResetHistogram =
      (GdkGLProc_glResetHistogram) gdk_gl_get_proc_address ("glResetHistogram");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glResetHistogram () - %s",
               (_procs_GL_VERSION_1_2.glResetHistogram) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glResetHistogram);
}

/* glResetMinmax */
GdkGLProc
gdk_gl_get_glResetMinmax (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glResetMinmax == (GdkGLProc_glResetMinmax) -1)
    _procs_GL_VERSION_1_2.glResetMinmax =
      (GdkGLProc_glResetMinmax) gdk_gl_get_proc_address ("glResetMinmax");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glResetMinmax () - %s",
               (_procs_GL_VERSION_1_2.glResetMinmax) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glResetMinmax);
}

/* glTexImage3D */
GdkGLProc
gdk_gl_get_glTexImage3D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glTexImage3D == (GdkGLProc_glTexImage3D) -1)
    _procs_GL_VERSION_1_2.glTexImage3D =
      (GdkGLProc_glTexImage3D) gdk_gl_get_proc_address ("glTexImage3D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexImage3D () - %s",
               (_procs_GL_VERSION_1_2.glTexImage3D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glTexImage3D);
}

/* glTexSubImage3D */
GdkGLProc
gdk_gl_get_glTexSubImage3D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glTexSubImage3D == (GdkGLProc_glTexSubImage3D) -1)
    _procs_GL_VERSION_1_2.glTexSubImage3D =
      (GdkGLProc_glTexSubImage3D) gdk_gl_get_proc_address ("glTexSubImage3D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexSubImage3D () - %s",
               (_procs_GL_VERSION_1_2.glTexSubImage3D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glTexSubImage3D);
}

/* glCopyTexSubImage3D */
GdkGLProc
gdk_gl_get_glCopyTexSubImage3D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_2.glCopyTexSubImage3D == (GdkGLProc_glCopyTexSubImage3D) -1)
    _procs_GL_VERSION_1_2.glCopyTexSubImage3D =
      (GdkGLProc_glCopyTexSubImage3D) gdk_gl_get_proc_address ("glCopyTexSubImage3D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyTexSubImage3D () - %s",
               (_procs_GL_VERSION_1_2.glCopyTexSubImage3D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_2.glCopyTexSubImage3D);
}

/* Get GL_VERSION_1_2 functions */
GdkGL_GL_VERSION_1_2 *
gdk_gl_get_GL_VERSION_1_2 (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported =  (gdk_gl_get_glBlendColor () != NULL);
      supported &= (gdk_gl_get_glBlendEquation () != NULL);
      supported &= (gdk_gl_get_glDrawRangeElements () != NULL);
      supported &= (gdk_gl_get_glColorTable () != NULL);
      supported &= (gdk_gl_get_glColorTableParameterfv () != NULL);
      supported &= (gdk_gl_get_glColorTableParameteriv () != NULL);
      supported &= (gdk_gl_get_glCopyColorTable () != NULL);
      supported &= (gdk_gl_get_glGetColorTable () != NULL);
      supported &= (gdk_gl_get_glGetColorTableParameterfv () != NULL);
      supported &= (gdk_gl_get_glGetColorTableParameteriv () != NULL);
      supported &= (gdk_gl_get_glColorSubTable () != NULL);
      supported &= (gdk_gl_get_glCopyColorSubTable () != NULL);
      supported &= (gdk_gl_get_glConvolutionFilter1D () != NULL);
      supported &= (gdk_gl_get_glConvolutionFilter2D () != NULL);
      supported &= (gdk_gl_get_glConvolutionParameterf () != NULL);
      supported &= (gdk_gl_get_glConvolutionParameterfv () != NULL);
      supported &= (gdk_gl_get_glConvolutionParameteri () != NULL);
      supported &= (gdk_gl_get_glConvolutionParameteriv () != NULL);
      supported &= (gdk_gl_get_glCopyConvolutionFilter1D () != NULL);
      supported &= (gdk_gl_get_glCopyConvolutionFilter2D () != NULL);
      supported &= (gdk_gl_get_glGetConvolutionFilter () != NULL);
      supported &= (gdk_gl_get_glGetConvolutionParameterfv () != NULL);
      supported &= (gdk_gl_get_glGetConvolutionParameteriv () != NULL);
      supported &= (gdk_gl_get_glGetSeparableFilter () != NULL);
      supported &= (gdk_gl_get_glSeparableFilter2D () != NULL);
      supported &= (gdk_gl_get_glGetHistogram () != NULL);
      supported &= (gdk_gl_get_glGetHistogramParameterfv () != NULL);
      supported &= (gdk_gl_get_glGetHistogramParameteriv () != NULL);
      supported &= (gdk_gl_get_glGetMinmax () != NULL);
      supported &= (gdk_gl_get_glGetMinmaxParameterfv () != NULL);
      supported &= (gdk_gl_get_glGetMinmaxParameteriv () != NULL);
      supported &= (gdk_gl_get_glHistogram () != NULL);
      supported &= (gdk_gl_get_glMinmax () != NULL);
      supported &= (gdk_gl_get_glResetHistogram () != NULL);
      supported &= (gdk_gl_get_glResetMinmax () != NULL);
      supported &= (gdk_gl_get_glTexImage3D () != NULL);
      supported &= (gdk_gl_get_glTexSubImage3D () != NULL);
      supported &= (gdk_gl_get_glCopyTexSubImage3D () != NULL);
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_VERSION_1_2 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_VERSION_1_2;
}

/*
 * GL_VERSION_1_3
 */

static GdkGL_GL_VERSION_1_3 _procs_GL_VERSION_1_3 = {
  (GdkGLProc_glActiveTexture) -1,
  (GdkGLProc_glClientActiveTexture) -1,
  (GdkGLProc_glMultiTexCoord1d) -1,
  (GdkGLProc_glMultiTexCoord1dv) -1,
  (GdkGLProc_glMultiTexCoord1f) -1,
  (GdkGLProc_glMultiTexCoord1fv) -1,
  (GdkGLProc_glMultiTexCoord1i) -1,
  (GdkGLProc_glMultiTexCoord1iv) -1,
  (GdkGLProc_glMultiTexCoord1s) -1,
  (GdkGLProc_glMultiTexCoord1sv) -1,
  (GdkGLProc_glMultiTexCoord2d) -1,
  (GdkGLProc_glMultiTexCoord2dv) -1,
  (GdkGLProc_glMultiTexCoord2f) -1,
  (GdkGLProc_glMultiTexCoord2fv) -1,
  (GdkGLProc_glMultiTexCoord2i) -1,
  (GdkGLProc_glMultiTexCoord2iv) -1,
  (GdkGLProc_glMultiTexCoord2s) -1,
  (GdkGLProc_glMultiTexCoord2sv) -1,
  (GdkGLProc_glMultiTexCoord3d) -1,
  (GdkGLProc_glMultiTexCoord3dv) -1,
  (GdkGLProc_glMultiTexCoord3f) -1,
  (GdkGLProc_glMultiTexCoord3fv) -1,
  (GdkGLProc_glMultiTexCoord3i) -1,
  (GdkGLProc_glMultiTexCoord3iv) -1,
  (GdkGLProc_glMultiTexCoord3s) -1,
  (GdkGLProc_glMultiTexCoord3sv) -1,
  (GdkGLProc_glMultiTexCoord4d) -1,
  (GdkGLProc_glMultiTexCoord4dv) -1,
  (GdkGLProc_glMultiTexCoord4f) -1,
  (GdkGLProc_glMultiTexCoord4fv) -1,
  (GdkGLProc_glMultiTexCoord4i) -1,
  (GdkGLProc_glMultiTexCoord4iv) -1,
  (GdkGLProc_glMultiTexCoord4s) -1,
  (GdkGLProc_glMultiTexCoord4sv) -1,
  (GdkGLProc_glLoadTransposeMatrixf) -1,
  (GdkGLProc_glLoadTransposeMatrixd) -1,
  (GdkGLProc_glMultTransposeMatrixf) -1,
  (GdkGLProc_glMultTransposeMatrixd) -1,
  (GdkGLProc_glSampleCoverage) -1,
  (GdkGLProc_glCompressedTexImage3D) -1,
  (GdkGLProc_glCompressedTexImage2D) -1,
  (GdkGLProc_glCompressedTexImage1D) -1,
  (GdkGLProc_glCompressedTexSubImage3D) -1,
  (GdkGLProc_glCompressedTexSubImage2D) -1,
  (GdkGLProc_glCompressedTexSubImage1D) -1,
  (GdkGLProc_glGetCompressedTexImage) -1
};

/* glActiveTexture */
GdkGLProc
gdk_gl_get_glActiveTexture (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glActiveTexture == (GdkGLProc_glActiveTexture) -1)
    _procs_GL_VERSION_1_3.glActiveTexture =
      (GdkGLProc_glActiveTexture) gdk_gl_get_proc_address ("glActiveTexture");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glActiveTexture () - %s",
               (_procs_GL_VERSION_1_3.glActiveTexture) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glActiveTexture);
}

/* glClientActiveTexture */
GdkGLProc
gdk_gl_get_glClientActiveTexture (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glClientActiveTexture == (GdkGLProc_glClientActiveTexture) -1)
    _procs_GL_VERSION_1_3.glClientActiveTexture =
      (GdkGLProc_glClientActiveTexture) gdk_gl_get_proc_address ("glClientActiveTexture");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glClientActiveTexture () - %s",
               (_procs_GL_VERSION_1_3.glClientActiveTexture) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glClientActiveTexture);
}

/* glMultiTexCoord1d */
GdkGLProc
gdk_gl_get_glMultiTexCoord1d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1d == (GdkGLProc_glMultiTexCoord1d) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1d =
      (GdkGLProc_glMultiTexCoord1d) gdk_gl_get_proc_address ("glMultiTexCoord1d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1d () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1d);
}

/* glMultiTexCoord1dv */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1dv == (GdkGLProc_glMultiTexCoord1dv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1dv =
      (GdkGLProc_glMultiTexCoord1dv) gdk_gl_get_proc_address ("glMultiTexCoord1dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1dv);
}

/* glMultiTexCoord1f */
GdkGLProc
gdk_gl_get_glMultiTexCoord1f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1f == (GdkGLProc_glMultiTexCoord1f) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1f =
      (GdkGLProc_glMultiTexCoord1f) gdk_gl_get_proc_address ("glMultiTexCoord1f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1f () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1f);
}

/* glMultiTexCoord1fv */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1fv == (GdkGLProc_glMultiTexCoord1fv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1fv =
      (GdkGLProc_glMultiTexCoord1fv) gdk_gl_get_proc_address ("glMultiTexCoord1fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1fv);
}

/* glMultiTexCoord1i */
GdkGLProc
gdk_gl_get_glMultiTexCoord1i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1i == (GdkGLProc_glMultiTexCoord1i) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1i =
      (GdkGLProc_glMultiTexCoord1i) gdk_gl_get_proc_address ("glMultiTexCoord1i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1i () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1i);
}

/* glMultiTexCoord1iv */
GdkGLProc
gdk_gl_get_glMultiTexCoord1iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1iv == (GdkGLProc_glMultiTexCoord1iv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1iv =
      (GdkGLProc_glMultiTexCoord1iv) gdk_gl_get_proc_address ("glMultiTexCoord1iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1iv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1iv);
}

/* glMultiTexCoord1s */
GdkGLProc
gdk_gl_get_glMultiTexCoord1s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1s == (GdkGLProc_glMultiTexCoord1s) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1s =
      (GdkGLProc_glMultiTexCoord1s) gdk_gl_get_proc_address ("glMultiTexCoord1s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1s () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1s);
}

/* glMultiTexCoord1sv */
GdkGLProc
gdk_gl_get_glMultiTexCoord1sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord1sv == (GdkGLProc_glMultiTexCoord1sv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord1sv =
      (GdkGLProc_glMultiTexCoord1sv) gdk_gl_get_proc_address ("glMultiTexCoord1sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1sv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord1sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord1sv);
}

/* glMultiTexCoord2d */
GdkGLProc
gdk_gl_get_glMultiTexCoord2d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2d == (GdkGLProc_glMultiTexCoord2d) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2d =
      (GdkGLProc_glMultiTexCoord2d) gdk_gl_get_proc_address ("glMultiTexCoord2d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2d () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2d);
}

/* glMultiTexCoord2dv */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2dv == (GdkGLProc_glMultiTexCoord2dv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2dv =
      (GdkGLProc_glMultiTexCoord2dv) gdk_gl_get_proc_address ("glMultiTexCoord2dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2dv);
}

/* glMultiTexCoord2f */
GdkGLProc
gdk_gl_get_glMultiTexCoord2f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2f == (GdkGLProc_glMultiTexCoord2f) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2f =
      (GdkGLProc_glMultiTexCoord2f) gdk_gl_get_proc_address ("glMultiTexCoord2f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2f () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2f);
}

/* glMultiTexCoord2fv */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2fv == (GdkGLProc_glMultiTexCoord2fv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2fv =
      (GdkGLProc_glMultiTexCoord2fv) gdk_gl_get_proc_address ("glMultiTexCoord2fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2fv);
}

/* glMultiTexCoord2i */
GdkGLProc
gdk_gl_get_glMultiTexCoord2i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2i == (GdkGLProc_glMultiTexCoord2i) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2i =
      (GdkGLProc_glMultiTexCoord2i) gdk_gl_get_proc_address ("glMultiTexCoord2i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2i () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2i);
}

/* glMultiTexCoord2iv */
GdkGLProc
gdk_gl_get_glMultiTexCoord2iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2iv == (GdkGLProc_glMultiTexCoord2iv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2iv =
      (GdkGLProc_glMultiTexCoord2iv) gdk_gl_get_proc_address ("glMultiTexCoord2iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2iv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2iv);
}

/* glMultiTexCoord2s */
GdkGLProc
gdk_gl_get_glMultiTexCoord2s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2s == (GdkGLProc_glMultiTexCoord2s) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2s =
      (GdkGLProc_glMultiTexCoord2s) gdk_gl_get_proc_address ("glMultiTexCoord2s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2s () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2s);
}

/* glMultiTexCoord2sv */
GdkGLProc
gdk_gl_get_glMultiTexCoord2sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord2sv == (GdkGLProc_glMultiTexCoord2sv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord2sv =
      (GdkGLProc_glMultiTexCoord2sv) gdk_gl_get_proc_address ("glMultiTexCoord2sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2sv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord2sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord2sv);
}

/* glMultiTexCoord3d */
GdkGLProc
gdk_gl_get_glMultiTexCoord3d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3d == (GdkGLProc_glMultiTexCoord3d) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3d =
      (GdkGLProc_glMultiTexCoord3d) gdk_gl_get_proc_address ("glMultiTexCoord3d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3d () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3d);
}

/* glMultiTexCoord3dv */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3dv == (GdkGLProc_glMultiTexCoord3dv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3dv =
      (GdkGLProc_glMultiTexCoord3dv) gdk_gl_get_proc_address ("glMultiTexCoord3dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3dv);
}

/* glMultiTexCoord3f */
GdkGLProc
gdk_gl_get_glMultiTexCoord3f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3f == (GdkGLProc_glMultiTexCoord3f) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3f =
      (GdkGLProc_glMultiTexCoord3f) gdk_gl_get_proc_address ("glMultiTexCoord3f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3f () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3f);
}

/* glMultiTexCoord3fv */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3fv == (GdkGLProc_glMultiTexCoord3fv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3fv =
      (GdkGLProc_glMultiTexCoord3fv) gdk_gl_get_proc_address ("glMultiTexCoord3fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3fv);
}

/* glMultiTexCoord3i */
GdkGLProc
gdk_gl_get_glMultiTexCoord3i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3i == (GdkGLProc_glMultiTexCoord3i) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3i =
      (GdkGLProc_glMultiTexCoord3i) gdk_gl_get_proc_address ("glMultiTexCoord3i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3i () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3i);
}

/* glMultiTexCoord3iv */
GdkGLProc
gdk_gl_get_glMultiTexCoord3iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3iv == (GdkGLProc_glMultiTexCoord3iv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3iv =
      (GdkGLProc_glMultiTexCoord3iv) gdk_gl_get_proc_address ("glMultiTexCoord3iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3iv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3iv);
}

/* glMultiTexCoord3s */
GdkGLProc
gdk_gl_get_glMultiTexCoord3s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3s == (GdkGLProc_glMultiTexCoord3s) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3s =
      (GdkGLProc_glMultiTexCoord3s) gdk_gl_get_proc_address ("glMultiTexCoord3s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3s () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3s);
}

/* glMultiTexCoord3sv */
GdkGLProc
gdk_gl_get_glMultiTexCoord3sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord3sv == (GdkGLProc_glMultiTexCoord3sv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord3sv =
      (GdkGLProc_glMultiTexCoord3sv) gdk_gl_get_proc_address ("glMultiTexCoord3sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3sv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord3sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord3sv);
}

/* glMultiTexCoord4d */
GdkGLProc
gdk_gl_get_glMultiTexCoord4d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4d == (GdkGLProc_glMultiTexCoord4d) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4d =
      (GdkGLProc_glMultiTexCoord4d) gdk_gl_get_proc_address ("glMultiTexCoord4d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4d () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4d);
}

/* glMultiTexCoord4dv */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4dv == (GdkGLProc_glMultiTexCoord4dv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4dv =
      (GdkGLProc_glMultiTexCoord4dv) gdk_gl_get_proc_address ("glMultiTexCoord4dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4dv);
}

/* glMultiTexCoord4f */
GdkGLProc
gdk_gl_get_glMultiTexCoord4f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4f == (GdkGLProc_glMultiTexCoord4f) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4f =
      (GdkGLProc_glMultiTexCoord4f) gdk_gl_get_proc_address ("glMultiTexCoord4f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4f () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4f);
}

/* glMultiTexCoord4fv */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4fv == (GdkGLProc_glMultiTexCoord4fv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4fv =
      (GdkGLProc_glMultiTexCoord4fv) gdk_gl_get_proc_address ("glMultiTexCoord4fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4fv);
}

/* glMultiTexCoord4i */
GdkGLProc
gdk_gl_get_glMultiTexCoord4i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4i == (GdkGLProc_glMultiTexCoord4i) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4i =
      (GdkGLProc_glMultiTexCoord4i) gdk_gl_get_proc_address ("glMultiTexCoord4i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4i () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4i);
}

/* glMultiTexCoord4iv */
GdkGLProc
gdk_gl_get_glMultiTexCoord4iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4iv == (GdkGLProc_glMultiTexCoord4iv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4iv =
      (GdkGLProc_glMultiTexCoord4iv) gdk_gl_get_proc_address ("glMultiTexCoord4iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4iv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4iv);
}

/* glMultiTexCoord4s */
GdkGLProc
gdk_gl_get_glMultiTexCoord4s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4s == (GdkGLProc_glMultiTexCoord4s) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4s =
      (GdkGLProc_glMultiTexCoord4s) gdk_gl_get_proc_address ("glMultiTexCoord4s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4s () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4s);
}

/* glMultiTexCoord4sv */
GdkGLProc
gdk_gl_get_glMultiTexCoord4sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultiTexCoord4sv == (GdkGLProc_glMultiTexCoord4sv) -1)
    _procs_GL_VERSION_1_3.glMultiTexCoord4sv =
      (GdkGLProc_glMultiTexCoord4sv) gdk_gl_get_proc_address ("glMultiTexCoord4sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4sv () - %s",
               (_procs_GL_VERSION_1_3.glMultiTexCoord4sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultiTexCoord4sv);
}

/* glLoadTransposeMatrixf */
GdkGLProc
gdk_gl_get_glLoadTransposeMatrixf (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glLoadTransposeMatrixf == (GdkGLProc_glLoadTransposeMatrixf) -1)
    _procs_GL_VERSION_1_3.glLoadTransposeMatrixf =
      (GdkGLProc_glLoadTransposeMatrixf) gdk_gl_get_proc_address ("glLoadTransposeMatrixf");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLoadTransposeMatrixf () - %s",
               (_procs_GL_VERSION_1_3.glLoadTransposeMatrixf) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glLoadTransposeMatrixf);
}

/* glLoadTransposeMatrixd */
GdkGLProc
gdk_gl_get_glLoadTransposeMatrixd (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glLoadTransposeMatrixd == (GdkGLProc_glLoadTransposeMatrixd) -1)
    _procs_GL_VERSION_1_3.glLoadTransposeMatrixd =
      (GdkGLProc_glLoadTransposeMatrixd) gdk_gl_get_proc_address ("glLoadTransposeMatrixd");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLoadTransposeMatrixd () - %s",
               (_procs_GL_VERSION_1_3.glLoadTransposeMatrixd) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glLoadTransposeMatrixd);
}

/* glMultTransposeMatrixf */
GdkGLProc
gdk_gl_get_glMultTransposeMatrixf (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultTransposeMatrixf == (GdkGLProc_glMultTransposeMatrixf) -1)
    _procs_GL_VERSION_1_3.glMultTransposeMatrixf =
      (GdkGLProc_glMultTransposeMatrixf) gdk_gl_get_proc_address ("glMultTransposeMatrixf");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultTransposeMatrixf () - %s",
               (_procs_GL_VERSION_1_3.glMultTransposeMatrixf) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultTransposeMatrixf);
}

/* glMultTransposeMatrixd */
GdkGLProc
gdk_gl_get_glMultTransposeMatrixd (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glMultTransposeMatrixd == (GdkGLProc_glMultTransposeMatrixd) -1)
    _procs_GL_VERSION_1_3.glMultTransposeMatrixd =
      (GdkGLProc_glMultTransposeMatrixd) gdk_gl_get_proc_address ("glMultTransposeMatrixd");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultTransposeMatrixd () - %s",
               (_procs_GL_VERSION_1_3.glMultTransposeMatrixd) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glMultTransposeMatrixd);
}

/* glSampleCoverage */
GdkGLProc
gdk_gl_get_glSampleCoverage (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glSampleCoverage == (GdkGLProc_glSampleCoverage) -1)
    _procs_GL_VERSION_1_3.glSampleCoverage =
      (GdkGLProc_glSampleCoverage) gdk_gl_get_proc_address ("glSampleCoverage");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSampleCoverage () - %s",
               (_procs_GL_VERSION_1_3.glSampleCoverage) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glSampleCoverage);
}

/* glCompressedTexImage3D */
GdkGLProc
gdk_gl_get_glCompressedTexImage3D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glCompressedTexImage3D == (GdkGLProc_glCompressedTexImage3D) -1)
    _procs_GL_VERSION_1_3.glCompressedTexImage3D =
      (GdkGLProc_glCompressedTexImage3D) gdk_gl_get_proc_address ("glCompressedTexImage3D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexImage3D () - %s",
               (_procs_GL_VERSION_1_3.glCompressedTexImage3D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glCompressedTexImage3D);
}

/* glCompressedTexImage2D */
GdkGLProc
gdk_gl_get_glCompressedTexImage2D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glCompressedTexImage2D == (GdkGLProc_glCompressedTexImage2D) -1)
    _procs_GL_VERSION_1_3.glCompressedTexImage2D =
      (GdkGLProc_glCompressedTexImage2D) gdk_gl_get_proc_address ("glCompressedTexImage2D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexImage2D () - %s",
               (_procs_GL_VERSION_1_3.glCompressedTexImage2D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glCompressedTexImage2D);
}

/* glCompressedTexImage1D */
GdkGLProc
gdk_gl_get_glCompressedTexImage1D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glCompressedTexImage1D == (GdkGLProc_glCompressedTexImage1D) -1)
    _procs_GL_VERSION_1_3.glCompressedTexImage1D =
      (GdkGLProc_glCompressedTexImage1D) gdk_gl_get_proc_address ("glCompressedTexImage1D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexImage1D () - %s",
               (_procs_GL_VERSION_1_3.glCompressedTexImage1D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glCompressedTexImage1D);
}

/* glCompressedTexSubImage3D */
GdkGLProc
gdk_gl_get_glCompressedTexSubImage3D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glCompressedTexSubImage3D == (GdkGLProc_glCompressedTexSubImage3D) -1)
    _procs_GL_VERSION_1_3.glCompressedTexSubImage3D =
      (GdkGLProc_glCompressedTexSubImage3D) gdk_gl_get_proc_address ("glCompressedTexSubImage3D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexSubImage3D () - %s",
               (_procs_GL_VERSION_1_3.glCompressedTexSubImage3D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glCompressedTexSubImage3D);
}

/* glCompressedTexSubImage2D */
GdkGLProc
gdk_gl_get_glCompressedTexSubImage2D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glCompressedTexSubImage2D == (GdkGLProc_glCompressedTexSubImage2D) -1)
    _procs_GL_VERSION_1_3.glCompressedTexSubImage2D =
      (GdkGLProc_glCompressedTexSubImage2D) gdk_gl_get_proc_address ("glCompressedTexSubImage2D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexSubImage2D () - %s",
               (_procs_GL_VERSION_1_3.glCompressedTexSubImage2D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glCompressedTexSubImage2D);
}

/* glCompressedTexSubImage1D */
GdkGLProc
gdk_gl_get_glCompressedTexSubImage1D (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glCompressedTexSubImage1D == (GdkGLProc_glCompressedTexSubImage1D) -1)
    _procs_GL_VERSION_1_3.glCompressedTexSubImage1D =
      (GdkGLProc_glCompressedTexSubImage1D) gdk_gl_get_proc_address ("glCompressedTexSubImage1D");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexSubImage1D () - %s",
               (_procs_GL_VERSION_1_3.glCompressedTexSubImage1D) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glCompressedTexSubImage1D);
}

/* glGetCompressedTexImage */
GdkGLProc
gdk_gl_get_glGetCompressedTexImage (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_3.glGetCompressedTexImage == (GdkGLProc_glGetCompressedTexImage) -1)
    _procs_GL_VERSION_1_3.glGetCompressedTexImage =
      (GdkGLProc_glGetCompressedTexImage) gdk_gl_get_proc_address ("glGetCompressedTexImage");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCompressedTexImage () - %s",
               (_procs_GL_VERSION_1_3.glGetCompressedTexImage) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_3.glGetCompressedTexImage);
}

/* Get GL_VERSION_1_3 functions */
GdkGL_GL_VERSION_1_3 *
gdk_gl_get_GL_VERSION_1_3 (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported =  (gdk_gl_get_glActiveTexture () != NULL);
      supported &= (gdk_gl_get_glClientActiveTexture () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1d () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1dv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1f () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1fv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1i () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1iv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1s () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord1sv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2d () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2dv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2f () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2fv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2i () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2iv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2s () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord2sv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3d () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3dv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3f () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3fv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3i () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3iv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3s () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord3sv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4d () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4dv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4f () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4fv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4i () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4iv () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4s () != NULL);
      supported &= (gdk_gl_get_glMultiTexCoord4sv () != NULL);
      supported &= (gdk_gl_get_glLoadTransposeMatrixf () != NULL);
      supported &= (gdk_gl_get_glLoadTransposeMatrixd () != NULL);
      supported &= (gdk_gl_get_glMultTransposeMatrixf () != NULL);
      supported &= (gdk_gl_get_glMultTransposeMatrixd () != NULL);
      supported &= (gdk_gl_get_glSampleCoverage () != NULL);
      supported &= (gdk_gl_get_glCompressedTexImage3D () != NULL);
      supported &= (gdk_gl_get_glCompressedTexImage2D () != NULL);
      supported &= (gdk_gl_get_glCompressedTexImage1D () != NULL);
      supported &= (gdk_gl_get_glCompressedTexSubImage3D () != NULL);
      supported &= (gdk_gl_get_glCompressedTexSubImage2D () != NULL);
      supported &= (gdk_gl_get_glCompressedTexSubImage1D () != NULL);
      supported &= (gdk_gl_get_glGetCompressedTexImage () != NULL);
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_VERSION_1_3 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_VERSION_1_3;
}

/*
 * GL_VERSION_1_4
 */

static GdkGL_GL_VERSION_1_4 _procs_GL_VERSION_1_4 = {
  (GdkGLProc_glBlendFuncSeparate) -1,
  (GdkGLProc_glFogCoordf) -1,
  (GdkGLProc_glFogCoordfv) -1,
  (GdkGLProc_glFogCoordd) -1,
  (GdkGLProc_glFogCoorddv) -1,
  (GdkGLProc_glFogCoordPointer) -1,
  (GdkGLProc_glMultiDrawArrays) -1,
  (GdkGLProc_glMultiDrawElements) -1,
  (GdkGLProc_glPointParameterf) -1,
  (GdkGLProc_glPointParameterfv) -1,
  (GdkGLProc_glPointParameteri) -1,
  (GdkGLProc_glPointParameteriv) -1,
  (GdkGLProc_glSecondaryColor3b) -1,
  (GdkGLProc_glSecondaryColor3bv) -1,
  (GdkGLProc_glSecondaryColor3d) -1,
  (GdkGLProc_glSecondaryColor3dv) -1,
  (GdkGLProc_glSecondaryColor3f) -1,
  (GdkGLProc_glSecondaryColor3fv) -1,
  (GdkGLProc_glSecondaryColor3i) -1,
  (GdkGLProc_glSecondaryColor3iv) -1,
  (GdkGLProc_glSecondaryColor3s) -1,
  (GdkGLProc_glSecondaryColor3sv) -1,
  (GdkGLProc_glSecondaryColor3ub) -1,
  (GdkGLProc_glSecondaryColor3ubv) -1,
  (GdkGLProc_glSecondaryColor3ui) -1,
  (GdkGLProc_glSecondaryColor3uiv) -1,
  (GdkGLProc_glSecondaryColor3us) -1,
  (GdkGLProc_glSecondaryColor3usv) -1,
  (GdkGLProc_glSecondaryColorPointer) -1,
  (GdkGLProc_glWindowPos2d) -1,
  (GdkGLProc_glWindowPos2dv) -1,
  (GdkGLProc_glWindowPos2f) -1,
  (GdkGLProc_glWindowPos2fv) -1,
  (GdkGLProc_glWindowPos2i) -1,
  (GdkGLProc_glWindowPos2iv) -1,
  (GdkGLProc_glWindowPos2s) -1,
  (GdkGLProc_glWindowPos2sv) -1,
  (GdkGLProc_glWindowPos3d) -1,
  (GdkGLProc_glWindowPos3dv) -1,
  (GdkGLProc_glWindowPos3f) -1,
  (GdkGLProc_glWindowPos3fv) -1,
  (GdkGLProc_glWindowPos3i) -1,
  (GdkGLProc_glWindowPos3iv) -1,
  (GdkGLProc_glWindowPos3s) -1,
  (GdkGLProc_glWindowPos3sv) -1
};

/* glBlendFuncSeparate */
GdkGLProc
gdk_gl_get_glBlendFuncSeparate (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glBlendFuncSeparate == (GdkGLProc_glBlendFuncSeparate) -1)
    _procs_GL_VERSION_1_4.glBlendFuncSeparate =
      (GdkGLProc_glBlendFuncSeparate) gdk_gl_get_proc_address ("glBlendFuncSeparate");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendFuncSeparate () - %s",
               (_procs_GL_VERSION_1_4.glBlendFuncSeparate) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glBlendFuncSeparate);
}

/* glFogCoordf */
GdkGLProc
gdk_gl_get_glFogCoordf (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glFogCoordf == (GdkGLProc_glFogCoordf) -1)
    _procs_GL_VERSION_1_4.glFogCoordf =
      (GdkGLProc_glFogCoordf) gdk_gl_get_proc_address ("glFogCoordf");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordf () - %s",
               (_procs_GL_VERSION_1_4.glFogCoordf) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glFogCoordf);
}

/* glFogCoordfv */
GdkGLProc
gdk_gl_get_glFogCoordfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glFogCoordfv == (GdkGLProc_glFogCoordfv) -1)
    _procs_GL_VERSION_1_4.glFogCoordfv =
      (GdkGLProc_glFogCoordfv) gdk_gl_get_proc_address ("glFogCoordfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordfv () - %s",
               (_procs_GL_VERSION_1_4.glFogCoordfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glFogCoordfv);
}

/* glFogCoordd */
GdkGLProc
gdk_gl_get_glFogCoordd (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glFogCoordd == (GdkGLProc_glFogCoordd) -1)
    _procs_GL_VERSION_1_4.glFogCoordd =
      (GdkGLProc_glFogCoordd) gdk_gl_get_proc_address ("glFogCoordd");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordd () - %s",
               (_procs_GL_VERSION_1_4.glFogCoordd) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glFogCoordd);
}

/* glFogCoorddv */
GdkGLProc
gdk_gl_get_glFogCoorddv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glFogCoorddv == (GdkGLProc_glFogCoorddv) -1)
    _procs_GL_VERSION_1_4.glFogCoorddv =
      (GdkGLProc_glFogCoorddv) gdk_gl_get_proc_address ("glFogCoorddv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoorddv () - %s",
               (_procs_GL_VERSION_1_4.glFogCoorddv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glFogCoorddv);
}

/* glFogCoordPointer */
GdkGLProc
gdk_gl_get_glFogCoordPointer (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glFogCoordPointer == (GdkGLProc_glFogCoordPointer) -1)
    _procs_GL_VERSION_1_4.glFogCoordPointer =
      (GdkGLProc_glFogCoordPointer) gdk_gl_get_proc_address ("glFogCoordPointer");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordPointer () - %s",
               (_procs_GL_VERSION_1_4.glFogCoordPointer) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glFogCoordPointer);
}

/* glMultiDrawArrays */
GdkGLProc
gdk_gl_get_glMultiDrawArrays (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glMultiDrawArrays == (GdkGLProc_glMultiDrawArrays) -1)
    _procs_GL_VERSION_1_4.glMultiDrawArrays =
      (GdkGLProc_glMultiDrawArrays) gdk_gl_get_proc_address ("glMultiDrawArrays");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawArrays () - %s",
               (_procs_GL_VERSION_1_4.glMultiDrawArrays) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glMultiDrawArrays);
}

/* glMultiDrawElements */
GdkGLProc
gdk_gl_get_glMultiDrawElements (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glMultiDrawElements == (GdkGLProc_glMultiDrawElements) -1)
    _procs_GL_VERSION_1_4.glMultiDrawElements =
      (GdkGLProc_glMultiDrawElements) gdk_gl_get_proc_address ("glMultiDrawElements");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawElements () - %s",
               (_procs_GL_VERSION_1_4.glMultiDrawElements) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glMultiDrawElements);
}

/* glPointParameterf */
GdkGLProc
gdk_gl_get_glPointParameterf (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glPointParameterf == (GdkGLProc_glPointParameterf) -1)
    _procs_GL_VERSION_1_4.glPointParameterf =
      (GdkGLProc_glPointParameterf) gdk_gl_get_proc_address ("glPointParameterf");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterf () - %s",
               (_procs_GL_VERSION_1_4.glPointParameterf) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glPointParameterf);
}

/* glPointParameterfv */
GdkGLProc
gdk_gl_get_glPointParameterfv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glPointParameterfv == (GdkGLProc_glPointParameterfv) -1)
    _procs_GL_VERSION_1_4.glPointParameterfv =
      (GdkGLProc_glPointParameterfv) gdk_gl_get_proc_address ("glPointParameterfv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfv () - %s",
               (_procs_GL_VERSION_1_4.glPointParameterfv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glPointParameterfv);
}

/* glPointParameteri */
GdkGLProc
gdk_gl_get_glPointParameteri (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glPointParameteri == (GdkGLProc_glPointParameteri) -1)
    _procs_GL_VERSION_1_4.glPointParameteri =
      (GdkGLProc_glPointParameteri) gdk_gl_get_proc_address ("glPointParameteri");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameteri () - %s",
               (_procs_GL_VERSION_1_4.glPointParameteri) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glPointParameteri);
}

/* glPointParameteriv */
GdkGLProc
gdk_gl_get_glPointParameteriv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glPointParameteriv == (GdkGLProc_glPointParameteriv) -1)
    _procs_GL_VERSION_1_4.glPointParameteriv =
      (GdkGLProc_glPointParameteriv) gdk_gl_get_proc_address ("glPointParameteriv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameteriv () - %s",
               (_procs_GL_VERSION_1_4.glPointParameteriv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glPointParameteriv);
}

/* glSecondaryColor3b */
GdkGLProc
gdk_gl_get_glSecondaryColor3b (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3b == (GdkGLProc_glSecondaryColor3b) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3b =
      (GdkGLProc_glSecondaryColor3b) gdk_gl_get_proc_address ("glSecondaryColor3b");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3b () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3b) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3b);
}

/* glSecondaryColor3bv */
GdkGLProc
gdk_gl_get_glSecondaryColor3bv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3bv == (GdkGLProc_glSecondaryColor3bv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3bv =
      (GdkGLProc_glSecondaryColor3bv) gdk_gl_get_proc_address ("glSecondaryColor3bv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3bv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3bv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3bv);
}

/* glSecondaryColor3d */
GdkGLProc
gdk_gl_get_glSecondaryColor3d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3d == (GdkGLProc_glSecondaryColor3d) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3d =
      (GdkGLProc_glSecondaryColor3d) gdk_gl_get_proc_address ("glSecondaryColor3d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3d () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3d);
}

/* glSecondaryColor3dv */
GdkGLProc
gdk_gl_get_glSecondaryColor3dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3dv == (GdkGLProc_glSecondaryColor3dv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3dv =
      (GdkGLProc_glSecondaryColor3dv) gdk_gl_get_proc_address ("glSecondaryColor3dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3dv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3dv);
}

/* glSecondaryColor3f */
GdkGLProc
gdk_gl_get_glSecondaryColor3f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3f == (GdkGLProc_glSecondaryColor3f) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3f =
      (GdkGLProc_glSecondaryColor3f) gdk_gl_get_proc_address ("glSecondaryColor3f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3f () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3f);
}

/* glSecondaryColor3fv */
GdkGLProc
gdk_gl_get_glSecondaryColor3fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3fv == (GdkGLProc_glSecondaryColor3fv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3fv =
      (GdkGLProc_glSecondaryColor3fv) gdk_gl_get_proc_address ("glSecondaryColor3fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3fv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3fv);
}

/* glSecondaryColor3i */
GdkGLProc
gdk_gl_get_glSecondaryColor3i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3i == (GdkGLProc_glSecondaryColor3i) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3i =
      (GdkGLProc_glSecondaryColor3i) gdk_gl_get_proc_address ("glSecondaryColor3i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3i () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3i);
}

/* glSecondaryColor3iv */
GdkGLProc
gdk_gl_get_glSecondaryColor3iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3iv == (GdkGLProc_glSecondaryColor3iv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3iv =
      (GdkGLProc_glSecondaryColor3iv) gdk_gl_get_proc_address ("glSecondaryColor3iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3iv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3iv);
}

/* glSecondaryColor3s */
GdkGLProc
gdk_gl_get_glSecondaryColor3s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3s == (GdkGLProc_glSecondaryColor3s) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3s =
      (GdkGLProc_glSecondaryColor3s) gdk_gl_get_proc_address ("glSecondaryColor3s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3s () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3s);
}

/* glSecondaryColor3sv */
GdkGLProc
gdk_gl_get_glSecondaryColor3sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3sv == (GdkGLProc_glSecondaryColor3sv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3sv =
      (GdkGLProc_glSecondaryColor3sv) gdk_gl_get_proc_address ("glSecondaryColor3sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3sv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3sv);
}

/* glSecondaryColor3ub */
GdkGLProc
gdk_gl_get_glSecondaryColor3ub (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3ub == (GdkGLProc_glSecondaryColor3ub) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3ub =
      (GdkGLProc_glSecondaryColor3ub) gdk_gl_get_proc_address ("glSecondaryColor3ub");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3ub () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3ub) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3ub);
}

/* glSecondaryColor3ubv */
GdkGLProc
gdk_gl_get_glSecondaryColor3ubv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3ubv == (GdkGLProc_glSecondaryColor3ubv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3ubv =
      (GdkGLProc_glSecondaryColor3ubv) gdk_gl_get_proc_address ("glSecondaryColor3ubv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3ubv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3ubv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3ubv);
}

/* glSecondaryColor3ui */
GdkGLProc
gdk_gl_get_glSecondaryColor3ui (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3ui == (GdkGLProc_glSecondaryColor3ui) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3ui =
      (GdkGLProc_glSecondaryColor3ui) gdk_gl_get_proc_address ("glSecondaryColor3ui");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3ui () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3ui) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3ui);
}

/* glSecondaryColor3uiv */
GdkGLProc
gdk_gl_get_glSecondaryColor3uiv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3uiv == (GdkGLProc_glSecondaryColor3uiv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3uiv =
      (GdkGLProc_glSecondaryColor3uiv) gdk_gl_get_proc_address ("glSecondaryColor3uiv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3uiv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3uiv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3uiv);
}

/* glSecondaryColor3us */
GdkGLProc
gdk_gl_get_glSecondaryColor3us (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3us == (GdkGLProc_glSecondaryColor3us) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3us =
      (GdkGLProc_glSecondaryColor3us) gdk_gl_get_proc_address ("glSecondaryColor3us");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3us () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3us) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3us);
}

/* glSecondaryColor3usv */
GdkGLProc
gdk_gl_get_glSecondaryColor3usv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColor3usv == (GdkGLProc_glSecondaryColor3usv) -1)
    _procs_GL_VERSION_1_4.glSecondaryColor3usv =
      (GdkGLProc_glSecondaryColor3usv) gdk_gl_get_proc_address ("glSecondaryColor3usv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3usv () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColor3usv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColor3usv);
}

/* glSecondaryColorPointer */
GdkGLProc
gdk_gl_get_glSecondaryColorPointer (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glSecondaryColorPointer == (GdkGLProc_glSecondaryColorPointer) -1)
    _procs_GL_VERSION_1_4.glSecondaryColorPointer =
      (GdkGLProc_glSecondaryColorPointer) gdk_gl_get_proc_address ("glSecondaryColorPointer");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColorPointer () - %s",
               (_procs_GL_VERSION_1_4.glSecondaryColorPointer) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glSecondaryColorPointer);
}

/* glWindowPos2d */
GdkGLProc
gdk_gl_get_glWindowPos2d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2d == (GdkGLProc_glWindowPos2d) -1)
    _procs_GL_VERSION_1_4.glWindowPos2d =
      (GdkGLProc_glWindowPos2d) gdk_gl_get_proc_address ("glWindowPos2d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2d () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2d);
}

/* glWindowPos2dv */
GdkGLProc
gdk_gl_get_glWindowPos2dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2dv == (GdkGLProc_glWindowPos2dv) -1)
    _procs_GL_VERSION_1_4.glWindowPos2dv =
      (GdkGLProc_glWindowPos2dv) gdk_gl_get_proc_address ("glWindowPos2dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2dv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2dv);
}

/* glWindowPos2f */
GdkGLProc
gdk_gl_get_glWindowPos2f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2f == (GdkGLProc_glWindowPos2f) -1)
    _procs_GL_VERSION_1_4.glWindowPos2f =
      (GdkGLProc_glWindowPos2f) gdk_gl_get_proc_address ("glWindowPos2f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2f () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2f);
}

/* glWindowPos2fv */
GdkGLProc
gdk_gl_get_glWindowPos2fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2fv == (GdkGLProc_glWindowPos2fv) -1)
    _procs_GL_VERSION_1_4.glWindowPos2fv =
      (GdkGLProc_glWindowPos2fv) gdk_gl_get_proc_address ("glWindowPos2fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2fv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2fv);
}

/* glWindowPos2i */
GdkGLProc
gdk_gl_get_glWindowPos2i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2i == (GdkGLProc_glWindowPos2i) -1)
    _procs_GL_VERSION_1_4.glWindowPos2i =
      (GdkGLProc_glWindowPos2i) gdk_gl_get_proc_address ("glWindowPos2i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2i () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2i);
}

/* glWindowPos2iv */
GdkGLProc
gdk_gl_get_glWindowPos2iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2iv == (GdkGLProc_glWindowPos2iv) -1)
    _procs_GL_VERSION_1_4.glWindowPos2iv =
      (GdkGLProc_glWindowPos2iv) gdk_gl_get_proc_address ("glWindowPos2iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2iv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2iv);
}

/* glWindowPos2s */
GdkGLProc
gdk_gl_get_glWindowPos2s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2s == (GdkGLProc_glWindowPos2s) -1)
    _procs_GL_VERSION_1_4.glWindowPos2s =
      (GdkGLProc_glWindowPos2s) gdk_gl_get_proc_address ("glWindowPos2s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2s () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2s);
}

/* glWindowPos2sv */
GdkGLProc
gdk_gl_get_glWindowPos2sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos2sv == (GdkGLProc_glWindowPos2sv) -1)
    _procs_GL_VERSION_1_4.glWindowPos2sv =
      (GdkGLProc_glWindowPos2sv) gdk_gl_get_proc_address ("glWindowPos2sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2sv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos2sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos2sv);
}

/* glWindowPos3d */
GdkGLProc
gdk_gl_get_glWindowPos3d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3d == (GdkGLProc_glWindowPos3d) -1)
    _procs_GL_VERSION_1_4.glWindowPos3d =
      (GdkGLProc_glWindowPos3d) gdk_gl_get_proc_address ("glWindowPos3d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3d () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3d);
}

/* glWindowPos3dv */
GdkGLProc
gdk_gl_get_glWindowPos3dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3dv == (GdkGLProc_glWindowPos3dv) -1)
    _procs_GL_VERSION_1_4.glWindowPos3dv =
      (GdkGLProc_glWindowPos3dv) gdk_gl_get_proc_address ("glWindowPos3dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3dv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3dv);
}

/* glWindowPos3f */
GdkGLProc
gdk_gl_get_glWindowPos3f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3f == (GdkGLProc_glWindowPos3f) -1)
    _procs_GL_VERSION_1_4.glWindowPos3f =
      (GdkGLProc_glWindowPos3f) gdk_gl_get_proc_address ("glWindowPos3f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3f () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3f);
}

/* glWindowPos3fv */
GdkGLProc
gdk_gl_get_glWindowPos3fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3fv == (GdkGLProc_glWindowPos3fv) -1)
    _procs_GL_VERSION_1_4.glWindowPos3fv =
      (GdkGLProc_glWindowPos3fv) gdk_gl_get_proc_address ("glWindowPos3fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3fv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3fv);
}

/* glWindowPos3i */
GdkGLProc
gdk_gl_get_glWindowPos3i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3i == (GdkGLProc_glWindowPos3i) -1)
    _procs_GL_VERSION_1_4.glWindowPos3i =
      (GdkGLProc_glWindowPos3i) gdk_gl_get_proc_address ("glWindowPos3i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3i () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3i);
}

/* glWindowPos3iv */
GdkGLProc
gdk_gl_get_glWindowPos3iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3iv == (GdkGLProc_glWindowPos3iv) -1)
    _procs_GL_VERSION_1_4.glWindowPos3iv =
      (GdkGLProc_glWindowPos3iv) gdk_gl_get_proc_address ("glWindowPos3iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3iv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3iv);
}

/* glWindowPos3s */
GdkGLProc
gdk_gl_get_glWindowPos3s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3s == (GdkGLProc_glWindowPos3s) -1)
    _procs_GL_VERSION_1_4.glWindowPos3s =
      (GdkGLProc_glWindowPos3s) gdk_gl_get_proc_address ("glWindowPos3s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3s () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3s);
}

/* glWindowPos3sv */
GdkGLProc
gdk_gl_get_glWindowPos3sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_VERSION_1_4.glWindowPos3sv == (GdkGLProc_glWindowPos3sv) -1)
    _procs_GL_VERSION_1_4.glWindowPos3sv =
      (GdkGLProc_glWindowPos3sv) gdk_gl_get_proc_address ("glWindowPos3sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3sv () - %s",
               (_procs_GL_VERSION_1_4.glWindowPos3sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_VERSION_1_4.glWindowPos3sv);
}

/* Get GL_VERSION_1_4 functions */
GdkGL_GL_VERSION_1_4 *
gdk_gl_get_GL_VERSION_1_4 (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported =  (gdk_gl_get_glBlendFuncSeparate () != NULL);
      supported &= (gdk_gl_get_glFogCoordf () != NULL);
      supported &= (gdk_gl_get_glFogCoordfv () != NULL);
      supported &= (gdk_gl_get_glFogCoordd () != NULL);
      supported &= (gdk_gl_get_glFogCoorddv () != NULL);
      supported &= (gdk_gl_get_glFogCoordPointer () != NULL);
      supported &= (gdk_gl_get_glMultiDrawArrays () != NULL);
      supported &= (gdk_gl_get_glMultiDrawElements () != NULL);
      supported &= (gdk_gl_get_glPointParameterf () != NULL);
      supported &= (gdk_gl_get_glPointParameterfv () != NULL);
      supported &= (gdk_gl_get_glPointParameteri () != NULL);
      supported &= (gdk_gl_get_glPointParameteriv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3b () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3bv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3d () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3dv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3f () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3fv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3i () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3iv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3s () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3sv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3ub () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3ubv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3ui () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3uiv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3us () != NULL);
      supported &= (gdk_gl_get_glSecondaryColor3usv () != NULL);
      supported &= (gdk_gl_get_glSecondaryColorPointer () != NULL);
      supported &= (gdk_gl_get_glWindowPos2d () != NULL);
      supported &= (gdk_gl_get_glWindowPos2dv () != NULL);
      supported &= (gdk_gl_get_glWindowPos2f () != NULL);
      supported &= (gdk_gl_get_glWindowPos2fv () != NULL);
      supported &= (gdk_gl_get_glWindowPos2i () != NULL);
      supported &= (gdk_gl_get_glWindowPos2iv () != NULL);
      supported &= (gdk_gl_get_glWindowPos2s () != NULL);
      supported &= (gdk_gl_get_glWindowPos2sv () != NULL);
      supported &= (gdk_gl_get_glWindowPos3d () != NULL);
      supported &= (gdk_gl_get_glWindowPos3dv () != NULL);
      supported &= (gdk_gl_get_glWindowPos3f () != NULL);
      supported &= (gdk_gl_get_glWindowPos3fv () != NULL);
      supported &= (gdk_gl_get_glWindowPos3i () != NULL);
      supported &= (gdk_gl_get_glWindowPos3iv () != NULL);
      supported &= (gdk_gl_get_glWindowPos3s () != NULL);
      supported &= (gdk_gl_get_glWindowPos3sv () != NULL);
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_VERSION_1_4 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_VERSION_1_4;
}

/*
 * GL_ARB_multitexture
 */

static GdkGL_GL_ARB_multitexture _procs_GL_ARB_multitexture = {
  (GdkGLProc_glActiveTextureARB) -1,
  (GdkGLProc_glClientActiveTextureARB) -1,
  (GdkGLProc_glMultiTexCoord1dARB) -1,
  (GdkGLProc_glMultiTexCoord1dvARB) -1,
  (GdkGLProc_glMultiTexCoord1fARB) -1,
  (GdkGLProc_glMultiTexCoord1fvARB) -1,
  (GdkGLProc_glMultiTexCoord1iARB) -1,
  (GdkGLProc_glMultiTexCoord1ivARB) -1,
  (GdkGLProc_glMultiTexCoord1sARB) -1,
  (GdkGLProc_glMultiTexCoord1svARB) -1,
  (GdkGLProc_glMultiTexCoord2dARB) -1,
  (GdkGLProc_glMultiTexCoord2dvARB) -1,
  (GdkGLProc_glMultiTexCoord2fARB) -1,
  (GdkGLProc_glMultiTexCoord2fvARB) -1,
  (GdkGLProc_glMultiTexCoord2iARB) -1,
  (GdkGLProc_glMultiTexCoord2ivARB) -1,
  (GdkGLProc_glMultiTexCoord2sARB) -1,
  (GdkGLProc_glMultiTexCoord2svARB) -1,
  (GdkGLProc_glMultiTexCoord3dARB) -1,
  (GdkGLProc_glMultiTexCoord3dvARB) -1,
  (GdkGLProc_glMultiTexCoord3fARB) -1,
  (GdkGLProc_glMultiTexCoord3fvARB) -1,
  (GdkGLProc_glMultiTexCoord3iARB) -1,
  (GdkGLProc_glMultiTexCoord3ivARB) -1,
  (GdkGLProc_glMultiTexCoord3sARB) -1,
  (GdkGLProc_glMultiTexCoord3svARB) -1,
  (GdkGLProc_glMultiTexCoord4dARB) -1,
  (GdkGLProc_glMultiTexCoord4dvARB) -1,
  (GdkGLProc_glMultiTexCoord4fARB) -1,
  (GdkGLProc_glMultiTexCoord4fvARB) -1,
  (GdkGLProc_glMultiTexCoord4iARB) -1,
  (GdkGLProc_glMultiTexCoord4ivARB) -1,
  (GdkGLProc_glMultiTexCoord4sARB) -1,
  (GdkGLProc_glMultiTexCoord4svARB) -1
};

/* glActiveTextureARB */
GdkGLProc
gdk_gl_get_glActiveTextureARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glActiveTextureARB == (GdkGLProc_glActiveTextureARB) -1)
    _procs_GL_ARB_multitexture.glActiveTextureARB =
      (GdkGLProc_glActiveTextureARB) gdk_gl_get_proc_address ("glActiveTextureARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glActiveTextureARB () - %s",
               (_procs_GL_ARB_multitexture.glActiveTextureARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glActiveTextureARB);
}

/* glClientActiveTextureARB */
GdkGLProc
gdk_gl_get_glClientActiveTextureARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glClientActiveTextureARB == (GdkGLProc_glClientActiveTextureARB) -1)
    _procs_GL_ARB_multitexture.glClientActiveTextureARB =
      (GdkGLProc_glClientActiveTextureARB) gdk_gl_get_proc_address ("glClientActiveTextureARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glClientActiveTextureARB () - %s",
               (_procs_GL_ARB_multitexture.glClientActiveTextureARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glClientActiveTextureARB);
}

/* glMultiTexCoord1dARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1dARB == (GdkGLProc_glMultiTexCoord1dARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1dARB =
      (GdkGLProc_glMultiTexCoord1dARB) gdk_gl_get_proc_address ("glMultiTexCoord1dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1dARB);
}

/* glMultiTexCoord1dvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1dvARB == (GdkGLProc_glMultiTexCoord1dvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1dvARB =
      (GdkGLProc_glMultiTexCoord1dvARB) gdk_gl_get_proc_address ("glMultiTexCoord1dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1dvARB);
}

/* glMultiTexCoord1fARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1fARB == (GdkGLProc_glMultiTexCoord1fARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1fARB =
      (GdkGLProc_glMultiTexCoord1fARB) gdk_gl_get_proc_address ("glMultiTexCoord1fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1fARB);
}

/* glMultiTexCoord1fvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1fvARB == (GdkGLProc_glMultiTexCoord1fvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1fvARB =
      (GdkGLProc_glMultiTexCoord1fvARB) gdk_gl_get_proc_address ("glMultiTexCoord1fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1fvARB);
}

/* glMultiTexCoord1iARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1iARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1iARB == (GdkGLProc_glMultiTexCoord1iARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1iARB =
      (GdkGLProc_glMultiTexCoord1iARB) gdk_gl_get_proc_address ("glMultiTexCoord1iARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1iARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1iARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1iARB);
}

/* glMultiTexCoord1ivARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1ivARB == (GdkGLProc_glMultiTexCoord1ivARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1ivARB =
      (GdkGLProc_glMultiTexCoord1ivARB) gdk_gl_get_proc_address ("glMultiTexCoord1ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1ivARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1ivARB);
}

/* glMultiTexCoord1sARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1sARB == (GdkGLProc_glMultiTexCoord1sARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1sARB =
      (GdkGLProc_glMultiTexCoord1sARB) gdk_gl_get_proc_address ("glMultiTexCoord1sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1sARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1sARB);
}

/* glMultiTexCoord1svARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord1svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord1svARB == (GdkGLProc_glMultiTexCoord1svARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord1svARB =
      (GdkGLProc_glMultiTexCoord1svARB) gdk_gl_get_proc_address ("glMultiTexCoord1svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1svARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord1svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord1svARB);
}

/* glMultiTexCoord2dARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2dARB == (GdkGLProc_glMultiTexCoord2dARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2dARB =
      (GdkGLProc_glMultiTexCoord2dARB) gdk_gl_get_proc_address ("glMultiTexCoord2dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2dARB);
}

/* glMultiTexCoord2dvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2dvARB == (GdkGLProc_glMultiTexCoord2dvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2dvARB =
      (GdkGLProc_glMultiTexCoord2dvARB) gdk_gl_get_proc_address ("glMultiTexCoord2dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2dvARB);
}

/* glMultiTexCoord2fARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2fARB == (GdkGLProc_glMultiTexCoord2fARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2fARB =
      (GdkGLProc_glMultiTexCoord2fARB) gdk_gl_get_proc_address ("glMultiTexCoord2fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2fARB);
}

/* glMultiTexCoord2fvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2fvARB == (GdkGLProc_glMultiTexCoord2fvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2fvARB =
      (GdkGLProc_glMultiTexCoord2fvARB) gdk_gl_get_proc_address ("glMultiTexCoord2fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2fvARB);
}

/* glMultiTexCoord2iARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2iARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2iARB == (GdkGLProc_glMultiTexCoord2iARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2iARB =
      (GdkGLProc_glMultiTexCoord2iARB) gdk_gl_get_proc_address ("glMultiTexCoord2iARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2iARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2iARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2iARB);
}

/* glMultiTexCoord2ivARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2ivARB == (GdkGLProc_glMultiTexCoord2ivARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2ivARB =
      (GdkGLProc_glMultiTexCoord2ivARB) gdk_gl_get_proc_address ("glMultiTexCoord2ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2ivARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2ivARB);
}

/* glMultiTexCoord2sARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2sARB == (GdkGLProc_glMultiTexCoord2sARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2sARB =
      (GdkGLProc_glMultiTexCoord2sARB) gdk_gl_get_proc_address ("glMultiTexCoord2sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2sARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2sARB);
}

/* glMultiTexCoord2svARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord2svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord2svARB == (GdkGLProc_glMultiTexCoord2svARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord2svARB =
      (GdkGLProc_glMultiTexCoord2svARB) gdk_gl_get_proc_address ("glMultiTexCoord2svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2svARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord2svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord2svARB);
}

/* glMultiTexCoord3dARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3dARB == (GdkGLProc_glMultiTexCoord3dARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3dARB =
      (GdkGLProc_glMultiTexCoord3dARB) gdk_gl_get_proc_address ("glMultiTexCoord3dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3dARB);
}

/* glMultiTexCoord3dvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3dvARB == (GdkGLProc_glMultiTexCoord3dvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3dvARB =
      (GdkGLProc_glMultiTexCoord3dvARB) gdk_gl_get_proc_address ("glMultiTexCoord3dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3dvARB);
}

/* glMultiTexCoord3fARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3fARB == (GdkGLProc_glMultiTexCoord3fARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3fARB =
      (GdkGLProc_glMultiTexCoord3fARB) gdk_gl_get_proc_address ("glMultiTexCoord3fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3fARB);
}

/* glMultiTexCoord3fvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3fvARB == (GdkGLProc_glMultiTexCoord3fvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3fvARB =
      (GdkGLProc_glMultiTexCoord3fvARB) gdk_gl_get_proc_address ("glMultiTexCoord3fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3fvARB);
}

/* glMultiTexCoord3iARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3iARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3iARB == (GdkGLProc_glMultiTexCoord3iARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3iARB =
      (GdkGLProc_glMultiTexCoord3iARB) gdk_gl_get_proc_address ("glMultiTexCoord3iARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3iARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3iARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3iARB);
}

/* glMultiTexCoord3ivARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3ivARB == (GdkGLProc_glMultiTexCoord3ivARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3ivARB =
      (GdkGLProc_glMultiTexCoord3ivARB) gdk_gl_get_proc_address ("glMultiTexCoord3ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3ivARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3ivARB);
}

/* glMultiTexCoord3sARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3sARB == (GdkGLProc_glMultiTexCoord3sARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3sARB =
      (GdkGLProc_glMultiTexCoord3sARB) gdk_gl_get_proc_address ("glMultiTexCoord3sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3sARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3sARB);
}

/* glMultiTexCoord3svARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord3svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord3svARB == (GdkGLProc_glMultiTexCoord3svARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord3svARB =
      (GdkGLProc_glMultiTexCoord3svARB) gdk_gl_get_proc_address ("glMultiTexCoord3svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3svARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord3svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord3svARB);
}

/* glMultiTexCoord4dARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4dARB == (GdkGLProc_glMultiTexCoord4dARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4dARB =
      (GdkGLProc_glMultiTexCoord4dARB) gdk_gl_get_proc_address ("glMultiTexCoord4dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4dARB);
}

/* glMultiTexCoord4dvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4dvARB == (GdkGLProc_glMultiTexCoord4dvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4dvARB =
      (GdkGLProc_glMultiTexCoord4dvARB) gdk_gl_get_proc_address ("glMultiTexCoord4dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4dvARB);
}

/* glMultiTexCoord4fARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4fARB == (GdkGLProc_glMultiTexCoord4fARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4fARB =
      (GdkGLProc_glMultiTexCoord4fARB) gdk_gl_get_proc_address ("glMultiTexCoord4fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4fARB);
}

/* glMultiTexCoord4fvARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4fvARB == (GdkGLProc_glMultiTexCoord4fvARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4fvARB =
      (GdkGLProc_glMultiTexCoord4fvARB) gdk_gl_get_proc_address ("glMultiTexCoord4fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fvARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4fvARB);
}

/* glMultiTexCoord4iARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4iARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4iARB == (GdkGLProc_glMultiTexCoord4iARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4iARB =
      (GdkGLProc_glMultiTexCoord4iARB) gdk_gl_get_proc_address ("glMultiTexCoord4iARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4iARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4iARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4iARB);
}

/* glMultiTexCoord4ivARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4ivARB == (GdkGLProc_glMultiTexCoord4ivARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4ivARB =
      (GdkGLProc_glMultiTexCoord4ivARB) gdk_gl_get_proc_address ("glMultiTexCoord4ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4ivARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4ivARB);
}

/* glMultiTexCoord4sARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4sARB == (GdkGLProc_glMultiTexCoord4sARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4sARB =
      (GdkGLProc_glMultiTexCoord4sARB) gdk_gl_get_proc_address ("glMultiTexCoord4sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4sARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4sARB);
}

/* glMultiTexCoord4svARB */
GdkGLProc
gdk_gl_get_glMultiTexCoord4svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multitexture.glMultiTexCoord4svARB == (GdkGLProc_glMultiTexCoord4svARB) -1)
    _procs_GL_ARB_multitexture.glMultiTexCoord4svARB =
      (GdkGLProc_glMultiTexCoord4svARB) gdk_gl_get_proc_address ("glMultiTexCoord4svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4svARB () - %s",
               (_procs_GL_ARB_multitexture.glMultiTexCoord4svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multitexture.glMultiTexCoord4svARB);
}

/* Get GL_ARB_multitexture functions */
GdkGL_GL_ARB_multitexture *
gdk_gl_get_GL_ARB_multitexture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_multitexture");

      if (supported)
        {
          supported &= (gdk_gl_get_glActiveTextureARB () != NULL);
          supported &= (gdk_gl_get_glClientActiveTextureARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1dARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1dvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1fARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1fvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1iARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1ivARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1sARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1svARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2dARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2dvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2fARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2fvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2iARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2ivARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2sARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2svARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3dARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3dvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3fARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3fvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3iARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3ivARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3sARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3svARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4dARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4dvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4fARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4fvARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4iARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4ivARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4sARB () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4svARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_multitexture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_multitexture;
}

/*
 * GL_ARB_transpose_matrix
 */

static GdkGL_GL_ARB_transpose_matrix _procs_GL_ARB_transpose_matrix = {
  (GdkGLProc_glLoadTransposeMatrixfARB) -1,
  (GdkGLProc_glLoadTransposeMatrixdARB) -1,
  (GdkGLProc_glMultTransposeMatrixfARB) -1,
  (GdkGLProc_glMultTransposeMatrixdARB) -1
};

/* glLoadTransposeMatrixfARB */
GdkGLProc
gdk_gl_get_glLoadTransposeMatrixfARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixfARB == (GdkGLProc_glLoadTransposeMatrixfARB) -1)
    _procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixfARB =
      (GdkGLProc_glLoadTransposeMatrixfARB) gdk_gl_get_proc_address ("glLoadTransposeMatrixfARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLoadTransposeMatrixfARB () - %s",
               (_procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixfARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixfARB);
}

/* glLoadTransposeMatrixdARB */
GdkGLProc
gdk_gl_get_glLoadTransposeMatrixdARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixdARB == (GdkGLProc_glLoadTransposeMatrixdARB) -1)
    _procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixdARB =
      (GdkGLProc_glLoadTransposeMatrixdARB) gdk_gl_get_proc_address ("glLoadTransposeMatrixdARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLoadTransposeMatrixdARB () - %s",
               (_procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixdARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_transpose_matrix.glLoadTransposeMatrixdARB);
}

/* glMultTransposeMatrixfARB */
GdkGLProc
gdk_gl_get_glMultTransposeMatrixfARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_transpose_matrix.glMultTransposeMatrixfARB == (GdkGLProc_glMultTransposeMatrixfARB) -1)
    _procs_GL_ARB_transpose_matrix.glMultTransposeMatrixfARB =
      (GdkGLProc_glMultTransposeMatrixfARB) gdk_gl_get_proc_address ("glMultTransposeMatrixfARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultTransposeMatrixfARB () - %s",
               (_procs_GL_ARB_transpose_matrix.glMultTransposeMatrixfARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_transpose_matrix.glMultTransposeMatrixfARB);
}

/* glMultTransposeMatrixdARB */
GdkGLProc
gdk_gl_get_glMultTransposeMatrixdARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_transpose_matrix.glMultTransposeMatrixdARB == (GdkGLProc_glMultTransposeMatrixdARB) -1)
    _procs_GL_ARB_transpose_matrix.glMultTransposeMatrixdARB =
      (GdkGLProc_glMultTransposeMatrixdARB) gdk_gl_get_proc_address ("glMultTransposeMatrixdARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultTransposeMatrixdARB () - %s",
               (_procs_GL_ARB_transpose_matrix.glMultTransposeMatrixdARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_transpose_matrix.glMultTransposeMatrixdARB);
}

/* Get GL_ARB_transpose_matrix functions */
GdkGL_GL_ARB_transpose_matrix *
gdk_gl_get_GL_ARB_transpose_matrix (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_transpose_matrix");

      if (supported)
        {
          supported &= (gdk_gl_get_glLoadTransposeMatrixfARB () != NULL);
          supported &= (gdk_gl_get_glLoadTransposeMatrixdARB () != NULL);
          supported &= (gdk_gl_get_glMultTransposeMatrixfARB () != NULL);
          supported &= (gdk_gl_get_glMultTransposeMatrixdARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_transpose_matrix () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_transpose_matrix;
}

/*
 * GL_ARB_multisample
 */

static GdkGL_GL_ARB_multisample _procs_GL_ARB_multisample = {
  (GdkGLProc_glSampleCoverageARB) -1
};

/* glSampleCoverageARB */
GdkGLProc
gdk_gl_get_glSampleCoverageARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_multisample.glSampleCoverageARB == (GdkGLProc_glSampleCoverageARB) -1)
    _procs_GL_ARB_multisample.glSampleCoverageARB =
      (GdkGLProc_glSampleCoverageARB) gdk_gl_get_proc_address ("glSampleCoverageARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSampleCoverageARB () - %s",
               (_procs_GL_ARB_multisample.glSampleCoverageARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_multisample.glSampleCoverageARB);
}

/* Get GL_ARB_multisample functions */
GdkGL_GL_ARB_multisample *
gdk_gl_get_GL_ARB_multisample (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_multisample");

      if (supported)
        {
          supported &= (gdk_gl_get_glSampleCoverageARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_multisample () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_multisample;
}

/*
 * GL_ARB_texture_compression
 */

static GdkGL_GL_ARB_texture_compression _procs_GL_ARB_texture_compression = {
  (GdkGLProc_glCompressedTexImage3DARB) -1,
  (GdkGLProc_glCompressedTexImage2DARB) -1,
  (GdkGLProc_glCompressedTexImage1DARB) -1,
  (GdkGLProc_glCompressedTexSubImage3DARB) -1,
  (GdkGLProc_glCompressedTexSubImage2DARB) -1,
  (GdkGLProc_glCompressedTexSubImage1DARB) -1,
  (GdkGLProc_glGetCompressedTexImageARB) -1
};

/* glCompressedTexImage3DARB */
GdkGLProc
gdk_gl_get_glCompressedTexImage3DARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glCompressedTexImage3DARB == (GdkGLProc_glCompressedTexImage3DARB) -1)
    _procs_GL_ARB_texture_compression.glCompressedTexImage3DARB =
      (GdkGLProc_glCompressedTexImage3DARB) gdk_gl_get_proc_address ("glCompressedTexImage3DARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexImage3DARB () - %s",
               (_procs_GL_ARB_texture_compression.glCompressedTexImage3DARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glCompressedTexImage3DARB);
}

/* glCompressedTexImage2DARB */
GdkGLProc
gdk_gl_get_glCompressedTexImage2DARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glCompressedTexImage2DARB == (GdkGLProc_glCompressedTexImage2DARB) -1)
    _procs_GL_ARB_texture_compression.glCompressedTexImage2DARB =
      (GdkGLProc_glCompressedTexImage2DARB) gdk_gl_get_proc_address ("glCompressedTexImage2DARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexImage2DARB () - %s",
               (_procs_GL_ARB_texture_compression.glCompressedTexImage2DARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glCompressedTexImage2DARB);
}

/* glCompressedTexImage1DARB */
GdkGLProc
gdk_gl_get_glCompressedTexImage1DARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glCompressedTexImage1DARB == (GdkGLProc_glCompressedTexImage1DARB) -1)
    _procs_GL_ARB_texture_compression.glCompressedTexImage1DARB =
      (GdkGLProc_glCompressedTexImage1DARB) gdk_gl_get_proc_address ("glCompressedTexImage1DARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexImage1DARB () - %s",
               (_procs_GL_ARB_texture_compression.glCompressedTexImage1DARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glCompressedTexImage1DARB);
}

/* glCompressedTexSubImage3DARB */
GdkGLProc
gdk_gl_get_glCompressedTexSubImage3DARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glCompressedTexSubImage3DARB == (GdkGLProc_glCompressedTexSubImage3DARB) -1)
    _procs_GL_ARB_texture_compression.glCompressedTexSubImage3DARB =
      (GdkGLProc_glCompressedTexSubImage3DARB) gdk_gl_get_proc_address ("glCompressedTexSubImage3DARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexSubImage3DARB () - %s",
               (_procs_GL_ARB_texture_compression.glCompressedTexSubImage3DARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glCompressedTexSubImage3DARB);
}

/* glCompressedTexSubImage2DARB */
GdkGLProc
gdk_gl_get_glCompressedTexSubImage2DARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glCompressedTexSubImage2DARB == (GdkGLProc_glCompressedTexSubImage2DARB) -1)
    _procs_GL_ARB_texture_compression.glCompressedTexSubImage2DARB =
      (GdkGLProc_glCompressedTexSubImage2DARB) gdk_gl_get_proc_address ("glCompressedTexSubImage2DARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexSubImage2DARB () - %s",
               (_procs_GL_ARB_texture_compression.glCompressedTexSubImage2DARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glCompressedTexSubImage2DARB);
}

/* glCompressedTexSubImage1DARB */
GdkGLProc
gdk_gl_get_glCompressedTexSubImage1DARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glCompressedTexSubImage1DARB == (GdkGLProc_glCompressedTexSubImage1DARB) -1)
    _procs_GL_ARB_texture_compression.glCompressedTexSubImage1DARB =
      (GdkGLProc_glCompressedTexSubImage1DARB) gdk_gl_get_proc_address ("glCompressedTexSubImage1DARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCompressedTexSubImage1DARB () - %s",
               (_procs_GL_ARB_texture_compression.glCompressedTexSubImage1DARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glCompressedTexSubImage1DARB);
}

/* glGetCompressedTexImageARB */
GdkGLProc
gdk_gl_get_glGetCompressedTexImageARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_texture_compression.glGetCompressedTexImageARB == (GdkGLProc_glGetCompressedTexImageARB) -1)
    _procs_GL_ARB_texture_compression.glGetCompressedTexImageARB =
      (GdkGLProc_glGetCompressedTexImageARB) gdk_gl_get_proc_address ("glGetCompressedTexImageARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCompressedTexImageARB () - %s",
               (_procs_GL_ARB_texture_compression.glGetCompressedTexImageARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_texture_compression.glGetCompressedTexImageARB);
}

/* Get GL_ARB_texture_compression functions */
GdkGL_GL_ARB_texture_compression *
gdk_gl_get_GL_ARB_texture_compression (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_texture_compression");

      if (supported)
        {
          supported &= (gdk_gl_get_glCompressedTexImage3DARB () != NULL);
          supported &= (gdk_gl_get_glCompressedTexImage2DARB () != NULL);
          supported &= (gdk_gl_get_glCompressedTexImage1DARB () != NULL);
          supported &= (gdk_gl_get_glCompressedTexSubImage3DARB () != NULL);
          supported &= (gdk_gl_get_glCompressedTexSubImage2DARB () != NULL);
          supported &= (gdk_gl_get_glCompressedTexSubImage1DARB () != NULL);
          supported &= (gdk_gl_get_glGetCompressedTexImageARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_texture_compression () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_texture_compression;
}

/*
 * GL_ARB_point_parameters
 */

static GdkGL_GL_ARB_point_parameters _procs_GL_ARB_point_parameters = {
  (GdkGLProc_glPointParameterfARB) -1,
  (GdkGLProc_glPointParameterfvARB) -1
};

/* glPointParameterfARB */
GdkGLProc
gdk_gl_get_glPointParameterfARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_point_parameters.glPointParameterfARB == (GdkGLProc_glPointParameterfARB) -1)
    _procs_GL_ARB_point_parameters.glPointParameterfARB =
      (GdkGLProc_glPointParameterfARB) gdk_gl_get_proc_address ("glPointParameterfARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfARB () - %s",
               (_procs_GL_ARB_point_parameters.glPointParameterfARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_point_parameters.glPointParameterfARB);
}

/* glPointParameterfvARB */
GdkGLProc
gdk_gl_get_glPointParameterfvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_point_parameters.glPointParameterfvARB == (GdkGLProc_glPointParameterfvARB) -1)
    _procs_GL_ARB_point_parameters.glPointParameterfvARB =
      (GdkGLProc_glPointParameterfvARB) gdk_gl_get_proc_address ("glPointParameterfvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfvARB () - %s",
               (_procs_GL_ARB_point_parameters.glPointParameterfvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_point_parameters.glPointParameterfvARB);
}

/* Get GL_ARB_point_parameters functions */
GdkGL_GL_ARB_point_parameters *
gdk_gl_get_GL_ARB_point_parameters (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_point_parameters");

      if (supported)
        {
          supported &= (gdk_gl_get_glPointParameterfARB () != NULL);
          supported &= (gdk_gl_get_glPointParameterfvARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_point_parameters () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_point_parameters;
}

/*
 * GL_ARB_vertex_blend
 */

static GdkGL_GL_ARB_vertex_blend _procs_GL_ARB_vertex_blend = {
  (GdkGLProc_glWeightbvARB) -1,
  (GdkGLProc_glWeightsvARB) -1,
  (GdkGLProc_glWeightivARB) -1,
  (GdkGLProc_glWeightfvARB) -1,
  (GdkGLProc_glWeightdvARB) -1,
  (GdkGLProc_glWeightubvARB) -1,
  (GdkGLProc_glWeightusvARB) -1,
  (GdkGLProc_glWeightuivARB) -1,
  (GdkGLProc_glWeightPointerARB) -1,
  (GdkGLProc_glVertexBlendARB) -1
};

/* glWeightbvARB */
GdkGLProc
gdk_gl_get_glWeightbvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightbvARB == (GdkGLProc_glWeightbvARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightbvARB =
      (GdkGLProc_glWeightbvARB) gdk_gl_get_proc_address ("glWeightbvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightbvARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightbvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightbvARB);
}

/* glWeightsvARB */
GdkGLProc
gdk_gl_get_glWeightsvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightsvARB == (GdkGLProc_glWeightsvARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightsvARB =
      (GdkGLProc_glWeightsvARB) gdk_gl_get_proc_address ("glWeightsvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightsvARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightsvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightsvARB);
}

/* glWeightivARB */
GdkGLProc
gdk_gl_get_glWeightivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightivARB == (GdkGLProc_glWeightivARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightivARB =
      (GdkGLProc_glWeightivARB) gdk_gl_get_proc_address ("glWeightivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightivARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightivARB);
}

/* glWeightfvARB */
GdkGLProc
gdk_gl_get_glWeightfvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightfvARB == (GdkGLProc_glWeightfvARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightfvARB =
      (GdkGLProc_glWeightfvARB) gdk_gl_get_proc_address ("glWeightfvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightfvARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightfvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightfvARB);
}

/* glWeightdvARB */
GdkGLProc
gdk_gl_get_glWeightdvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightdvARB == (GdkGLProc_glWeightdvARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightdvARB =
      (GdkGLProc_glWeightdvARB) gdk_gl_get_proc_address ("glWeightdvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightdvARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightdvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightdvARB);
}

/* glWeightubvARB */
GdkGLProc
gdk_gl_get_glWeightubvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightubvARB == (GdkGLProc_glWeightubvARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightubvARB =
      (GdkGLProc_glWeightubvARB) gdk_gl_get_proc_address ("glWeightubvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightubvARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightubvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightubvARB);
}

/* glWeightusvARB */
GdkGLProc
gdk_gl_get_glWeightusvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightusvARB == (GdkGLProc_glWeightusvARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightusvARB =
      (GdkGLProc_glWeightusvARB) gdk_gl_get_proc_address ("glWeightusvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightusvARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightusvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightusvARB);
}

/* glWeightuivARB */
GdkGLProc
gdk_gl_get_glWeightuivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightuivARB == (GdkGLProc_glWeightuivARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightuivARB =
      (GdkGLProc_glWeightuivARB) gdk_gl_get_proc_address ("glWeightuivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightuivARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightuivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightuivARB);
}

/* glWeightPointerARB */
GdkGLProc
gdk_gl_get_glWeightPointerARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glWeightPointerARB == (GdkGLProc_glWeightPointerARB) -1)
    _procs_GL_ARB_vertex_blend.glWeightPointerARB =
      (GdkGLProc_glWeightPointerARB) gdk_gl_get_proc_address ("glWeightPointerARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWeightPointerARB () - %s",
               (_procs_GL_ARB_vertex_blend.glWeightPointerARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glWeightPointerARB);
}

/* glVertexBlendARB */
GdkGLProc
gdk_gl_get_glVertexBlendARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_blend.glVertexBlendARB == (GdkGLProc_glVertexBlendARB) -1)
    _procs_GL_ARB_vertex_blend.glVertexBlendARB =
      (GdkGLProc_glVertexBlendARB) gdk_gl_get_proc_address ("glVertexBlendARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexBlendARB () - %s",
               (_procs_GL_ARB_vertex_blend.glVertexBlendARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_blend.glVertexBlendARB);
}

/* Get GL_ARB_vertex_blend functions */
GdkGL_GL_ARB_vertex_blend *
gdk_gl_get_GL_ARB_vertex_blend (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_vertex_blend");

      if (supported)
        {
          supported &= (gdk_gl_get_glWeightbvARB () != NULL);
          supported &= (gdk_gl_get_glWeightsvARB () != NULL);
          supported &= (gdk_gl_get_glWeightivARB () != NULL);
          supported &= (gdk_gl_get_glWeightfvARB () != NULL);
          supported &= (gdk_gl_get_glWeightdvARB () != NULL);
          supported &= (gdk_gl_get_glWeightubvARB () != NULL);
          supported &= (gdk_gl_get_glWeightusvARB () != NULL);
          supported &= (gdk_gl_get_glWeightuivARB () != NULL);
          supported &= (gdk_gl_get_glWeightPointerARB () != NULL);
          supported &= (gdk_gl_get_glVertexBlendARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_vertex_blend () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_vertex_blend;
}

/*
 * GL_ARB_matrix_palette
 */

static GdkGL_GL_ARB_matrix_palette _procs_GL_ARB_matrix_palette = {
  (GdkGLProc_glCurrentPaletteMatrixARB) -1,
  (GdkGLProc_glMatrixIndexubvARB) -1,
  (GdkGLProc_glMatrixIndexusvARB) -1,
  (GdkGLProc_glMatrixIndexuivARB) -1,
  (GdkGLProc_glMatrixIndexPointerARB) -1
};

/* glCurrentPaletteMatrixARB */
GdkGLProc
gdk_gl_get_glCurrentPaletteMatrixARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_matrix_palette.glCurrentPaletteMatrixARB == (GdkGLProc_glCurrentPaletteMatrixARB) -1)
    _procs_GL_ARB_matrix_palette.glCurrentPaletteMatrixARB =
      (GdkGLProc_glCurrentPaletteMatrixARB) gdk_gl_get_proc_address ("glCurrentPaletteMatrixARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCurrentPaletteMatrixARB () - %s",
               (_procs_GL_ARB_matrix_palette.glCurrentPaletteMatrixARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_matrix_palette.glCurrentPaletteMatrixARB);
}

/* glMatrixIndexubvARB */
GdkGLProc
gdk_gl_get_glMatrixIndexubvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_matrix_palette.glMatrixIndexubvARB == (GdkGLProc_glMatrixIndexubvARB) -1)
    _procs_GL_ARB_matrix_palette.glMatrixIndexubvARB =
      (GdkGLProc_glMatrixIndexubvARB) gdk_gl_get_proc_address ("glMatrixIndexubvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMatrixIndexubvARB () - %s",
               (_procs_GL_ARB_matrix_palette.glMatrixIndexubvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_matrix_palette.glMatrixIndexubvARB);
}

/* glMatrixIndexusvARB */
GdkGLProc
gdk_gl_get_glMatrixIndexusvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_matrix_palette.glMatrixIndexusvARB == (GdkGLProc_glMatrixIndexusvARB) -1)
    _procs_GL_ARB_matrix_palette.glMatrixIndexusvARB =
      (GdkGLProc_glMatrixIndexusvARB) gdk_gl_get_proc_address ("glMatrixIndexusvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMatrixIndexusvARB () - %s",
               (_procs_GL_ARB_matrix_palette.glMatrixIndexusvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_matrix_palette.glMatrixIndexusvARB);
}

/* glMatrixIndexuivARB */
GdkGLProc
gdk_gl_get_glMatrixIndexuivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_matrix_palette.glMatrixIndexuivARB == (GdkGLProc_glMatrixIndexuivARB) -1)
    _procs_GL_ARB_matrix_palette.glMatrixIndexuivARB =
      (GdkGLProc_glMatrixIndexuivARB) gdk_gl_get_proc_address ("glMatrixIndexuivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMatrixIndexuivARB () - %s",
               (_procs_GL_ARB_matrix_palette.glMatrixIndexuivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_matrix_palette.glMatrixIndexuivARB);
}

/* glMatrixIndexPointerARB */
GdkGLProc
gdk_gl_get_glMatrixIndexPointerARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_matrix_palette.glMatrixIndexPointerARB == (GdkGLProc_glMatrixIndexPointerARB) -1)
    _procs_GL_ARB_matrix_palette.glMatrixIndexPointerARB =
      (GdkGLProc_glMatrixIndexPointerARB) gdk_gl_get_proc_address ("glMatrixIndexPointerARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMatrixIndexPointerARB () - %s",
               (_procs_GL_ARB_matrix_palette.glMatrixIndexPointerARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_matrix_palette.glMatrixIndexPointerARB);
}

/* Get GL_ARB_matrix_palette functions */
GdkGL_GL_ARB_matrix_palette *
gdk_gl_get_GL_ARB_matrix_palette (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_matrix_palette");

      if (supported)
        {
          supported &= (gdk_gl_get_glCurrentPaletteMatrixARB () != NULL);
          supported &= (gdk_gl_get_glMatrixIndexubvARB () != NULL);
          supported &= (gdk_gl_get_glMatrixIndexusvARB () != NULL);
          supported &= (gdk_gl_get_glMatrixIndexuivARB () != NULL);
          supported &= (gdk_gl_get_glMatrixIndexPointerARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_matrix_palette () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_matrix_palette;
}

/*
 * GL_ARB_window_pos
 */

static GdkGL_GL_ARB_window_pos _procs_GL_ARB_window_pos = {
  (GdkGLProc_glWindowPos2dARB) -1,
  (GdkGLProc_glWindowPos2dvARB) -1,
  (GdkGLProc_glWindowPos2fARB) -1,
  (GdkGLProc_glWindowPos2fvARB) -1,
  (GdkGLProc_glWindowPos2iARB) -1,
  (GdkGLProc_glWindowPos2ivARB) -1,
  (GdkGLProc_glWindowPos2sARB) -1,
  (GdkGLProc_glWindowPos2svARB) -1,
  (GdkGLProc_glWindowPos3dARB) -1,
  (GdkGLProc_glWindowPos3dvARB) -1,
  (GdkGLProc_glWindowPos3fARB) -1,
  (GdkGLProc_glWindowPos3fvARB) -1,
  (GdkGLProc_glWindowPos3iARB) -1,
  (GdkGLProc_glWindowPos3ivARB) -1,
  (GdkGLProc_glWindowPos3sARB) -1,
  (GdkGLProc_glWindowPos3svARB) -1
};

/* glWindowPos2dARB */
GdkGLProc
gdk_gl_get_glWindowPos2dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2dARB == (GdkGLProc_glWindowPos2dARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2dARB =
      (GdkGLProc_glWindowPos2dARB) gdk_gl_get_proc_address ("glWindowPos2dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2dARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2dARB);
}

/* glWindowPos2dvARB */
GdkGLProc
gdk_gl_get_glWindowPos2dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2dvARB == (GdkGLProc_glWindowPos2dvARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2dvARB =
      (GdkGLProc_glWindowPos2dvARB) gdk_gl_get_proc_address ("glWindowPos2dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2dvARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2dvARB);
}

/* glWindowPos2fARB */
GdkGLProc
gdk_gl_get_glWindowPos2fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2fARB == (GdkGLProc_glWindowPos2fARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2fARB =
      (GdkGLProc_glWindowPos2fARB) gdk_gl_get_proc_address ("glWindowPos2fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2fARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2fARB);
}

/* glWindowPos2fvARB */
GdkGLProc
gdk_gl_get_glWindowPos2fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2fvARB == (GdkGLProc_glWindowPos2fvARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2fvARB =
      (GdkGLProc_glWindowPos2fvARB) gdk_gl_get_proc_address ("glWindowPos2fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2fvARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2fvARB);
}

/* glWindowPos2iARB */
GdkGLProc
gdk_gl_get_glWindowPos2iARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2iARB == (GdkGLProc_glWindowPos2iARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2iARB =
      (GdkGLProc_glWindowPos2iARB) gdk_gl_get_proc_address ("glWindowPos2iARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2iARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2iARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2iARB);
}

/* glWindowPos2ivARB */
GdkGLProc
gdk_gl_get_glWindowPos2ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2ivARB == (GdkGLProc_glWindowPos2ivARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2ivARB =
      (GdkGLProc_glWindowPos2ivARB) gdk_gl_get_proc_address ("glWindowPos2ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2ivARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2ivARB);
}

/* glWindowPos2sARB */
GdkGLProc
gdk_gl_get_glWindowPos2sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2sARB == (GdkGLProc_glWindowPos2sARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2sARB =
      (GdkGLProc_glWindowPos2sARB) gdk_gl_get_proc_address ("glWindowPos2sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2sARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2sARB);
}

/* glWindowPos2svARB */
GdkGLProc
gdk_gl_get_glWindowPos2svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos2svARB == (GdkGLProc_glWindowPos2svARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos2svARB =
      (GdkGLProc_glWindowPos2svARB) gdk_gl_get_proc_address ("glWindowPos2svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2svARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos2svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos2svARB);
}

/* glWindowPos3dARB */
GdkGLProc
gdk_gl_get_glWindowPos3dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3dARB == (GdkGLProc_glWindowPos3dARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3dARB =
      (GdkGLProc_glWindowPos3dARB) gdk_gl_get_proc_address ("glWindowPos3dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3dARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3dARB);
}

/* glWindowPos3dvARB */
GdkGLProc
gdk_gl_get_glWindowPos3dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3dvARB == (GdkGLProc_glWindowPos3dvARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3dvARB =
      (GdkGLProc_glWindowPos3dvARB) gdk_gl_get_proc_address ("glWindowPos3dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3dvARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3dvARB);
}

/* glWindowPos3fARB */
GdkGLProc
gdk_gl_get_glWindowPos3fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3fARB == (GdkGLProc_glWindowPos3fARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3fARB =
      (GdkGLProc_glWindowPos3fARB) gdk_gl_get_proc_address ("glWindowPos3fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3fARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3fARB);
}

/* glWindowPos3fvARB */
GdkGLProc
gdk_gl_get_glWindowPos3fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3fvARB == (GdkGLProc_glWindowPos3fvARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3fvARB =
      (GdkGLProc_glWindowPos3fvARB) gdk_gl_get_proc_address ("glWindowPos3fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3fvARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3fvARB);
}

/* glWindowPos3iARB */
GdkGLProc
gdk_gl_get_glWindowPos3iARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3iARB == (GdkGLProc_glWindowPos3iARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3iARB =
      (GdkGLProc_glWindowPos3iARB) gdk_gl_get_proc_address ("glWindowPos3iARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3iARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3iARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3iARB);
}

/* glWindowPos3ivARB */
GdkGLProc
gdk_gl_get_glWindowPos3ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3ivARB == (GdkGLProc_glWindowPos3ivARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3ivARB =
      (GdkGLProc_glWindowPos3ivARB) gdk_gl_get_proc_address ("glWindowPos3ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3ivARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3ivARB);
}

/* glWindowPos3sARB */
GdkGLProc
gdk_gl_get_glWindowPos3sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3sARB == (GdkGLProc_glWindowPos3sARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3sARB =
      (GdkGLProc_glWindowPos3sARB) gdk_gl_get_proc_address ("glWindowPos3sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3sARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3sARB);
}

/* glWindowPos3svARB */
GdkGLProc
gdk_gl_get_glWindowPos3svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_window_pos.glWindowPos3svARB == (GdkGLProc_glWindowPos3svARB) -1)
    _procs_GL_ARB_window_pos.glWindowPos3svARB =
      (GdkGLProc_glWindowPos3svARB) gdk_gl_get_proc_address ("glWindowPos3svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3svARB () - %s",
               (_procs_GL_ARB_window_pos.glWindowPos3svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_window_pos.glWindowPos3svARB);
}

/* Get GL_ARB_window_pos functions */
GdkGL_GL_ARB_window_pos *
gdk_gl_get_GL_ARB_window_pos (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_window_pos");

      if (supported)
        {
          supported &= (gdk_gl_get_glWindowPos2dARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2dvARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2fARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2fvARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2iARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2ivARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2sARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos2svARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3dARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3dvARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3fARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3fvARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3iARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3ivARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3sARB () != NULL);
          supported &= (gdk_gl_get_glWindowPos3svARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_window_pos () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_window_pos;
}

/*
 * GL_ARB_vertex_program
 */

static GdkGL_GL_ARB_vertex_program _procs_GL_ARB_vertex_program = {
  (GdkGLProc_glVertexAttrib1dARB) -1,
  (GdkGLProc_glVertexAttrib1dvARB) -1,
  (GdkGLProc_glVertexAttrib1fARB) -1,
  (GdkGLProc_glVertexAttrib1fvARB) -1,
  (GdkGLProc_glVertexAttrib1sARB) -1,
  (GdkGLProc_glVertexAttrib1svARB) -1,
  (GdkGLProc_glVertexAttrib2dARB) -1,
  (GdkGLProc_glVertexAttrib2dvARB) -1,
  (GdkGLProc_glVertexAttrib2fARB) -1,
  (GdkGLProc_glVertexAttrib2fvARB) -1,
  (GdkGLProc_glVertexAttrib2sARB) -1,
  (GdkGLProc_glVertexAttrib2svARB) -1,
  (GdkGLProc_glVertexAttrib3dARB) -1,
  (GdkGLProc_glVertexAttrib3dvARB) -1,
  (GdkGLProc_glVertexAttrib3fARB) -1,
  (GdkGLProc_glVertexAttrib3fvARB) -1,
  (GdkGLProc_glVertexAttrib3sARB) -1,
  (GdkGLProc_glVertexAttrib3svARB) -1,
  (GdkGLProc_glVertexAttrib4NbvARB) -1,
  (GdkGLProc_glVertexAttrib4NivARB) -1,
  (GdkGLProc_glVertexAttrib4NsvARB) -1,
  (GdkGLProc_glVertexAttrib4NubARB) -1,
  (GdkGLProc_glVertexAttrib4NubvARB) -1,
  (GdkGLProc_glVertexAttrib4NuivARB) -1,
  (GdkGLProc_glVertexAttrib4NusvARB) -1,
  (GdkGLProc_glVertexAttrib4bvARB) -1,
  (GdkGLProc_glVertexAttrib4dARB) -1,
  (GdkGLProc_glVertexAttrib4dvARB) -1,
  (GdkGLProc_glVertexAttrib4fARB) -1,
  (GdkGLProc_glVertexAttrib4fvARB) -1,
  (GdkGLProc_glVertexAttrib4ivARB) -1,
  (GdkGLProc_glVertexAttrib4sARB) -1,
  (GdkGLProc_glVertexAttrib4svARB) -1,
  (GdkGLProc_glVertexAttrib4ubvARB) -1,
  (GdkGLProc_glVertexAttrib4uivARB) -1,
  (GdkGLProc_glVertexAttrib4usvARB) -1,
  (GdkGLProc_glVertexAttribPointerARB) -1,
  (GdkGLProc_glEnableVertexAttribArrayARB) -1,
  (GdkGLProc_glDisableVertexAttribArrayARB) -1,
  (GdkGLProc_glProgramStringARB) -1,
  (GdkGLProc_glBindProgramARB) -1,
  (GdkGLProc_glDeleteProgramsARB) -1,
  (GdkGLProc_glGenProgramsARB) -1,
  (GdkGLProc_glProgramEnvParameter4dARB) -1,
  (GdkGLProc_glProgramEnvParameter4dvARB) -1,
  (GdkGLProc_glProgramEnvParameter4fARB) -1,
  (GdkGLProc_glProgramEnvParameter4fvARB) -1,
  (GdkGLProc_glProgramLocalParameter4dARB) -1,
  (GdkGLProc_glProgramLocalParameter4dvARB) -1,
  (GdkGLProc_glProgramLocalParameter4fARB) -1,
  (GdkGLProc_glProgramLocalParameter4fvARB) -1,
  (GdkGLProc_glGetProgramEnvParameterdvARB) -1,
  (GdkGLProc_glGetProgramEnvParameterfvARB) -1,
  (GdkGLProc_glGetProgramLocalParameterdvARB) -1,
  (GdkGLProc_glGetProgramLocalParameterfvARB) -1,
  (GdkGLProc_glGetProgramivARB) -1,
  (GdkGLProc_glGetProgramStringARB) -1,
  (GdkGLProc_glGetVertexAttribdvARB) -1,
  (GdkGLProc_glGetVertexAttribfvARB) -1,
  (GdkGLProc_glGetVertexAttribivARB) -1,
  (GdkGLProc_glGetVertexAttribPointervARB) -1,
  (GdkGLProc_glIsProgramARB) -1
};

/* glVertexAttrib1dARB */
GdkGLProc
gdk_gl_get_glVertexAttrib1dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib1dARB == (GdkGLProc_glVertexAttrib1dARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib1dARB =
      (GdkGLProc_glVertexAttrib1dARB) gdk_gl_get_proc_address ("glVertexAttrib1dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1dARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib1dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib1dARB);
}

/* glVertexAttrib1dvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib1dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib1dvARB == (GdkGLProc_glVertexAttrib1dvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib1dvARB =
      (GdkGLProc_glVertexAttrib1dvARB) gdk_gl_get_proc_address ("glVertexAttrib1dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1dvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib1dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib1dvARB);
}

/* glVertexAttrib1fARB */
GdkGLProc
gdk_gl_get_glVertexAttrib1fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib1fARB == (GdkGLProc_glVertexAttrib1fARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib1fARB =
      (GdkGLProc_glVertexAttrib1fARB) gdk_gl_get_proc_address ("glVertexAttrib1fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1fARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib1fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib1fARB);
}

/* glVertexAttrib1fvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib1fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib1fvARB == (GdkGLProc_glVertexAttrib1fvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib1fvARB =
      (GdkGLProc_glVertexAttrib1fvARB) gdk_gl_get_proc_address ("glVertexAttrib1fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1fvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib1fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib1fvARB);
}

/* glVertexAttrib1sARB */
GdkGLProc
gdk_gl_get_glVertexAttrib1sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib1sARB == (GdkGLProc_glVertexAttrib1sARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib1sARB =
      (GdkGLProc_glVertexAttrib1sARB) gdk_gl_get_proc_address ("glVertexAttrib1sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1sARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib1sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib1sARB);
}

/* glVertexAttrib1svARB */
GdkGLProc
gdk_gl_get_glVertexAttrib1svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib1svARB == (GdkGLProc_glVertexAttrib1svARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib1svARB =
      (GdkGLProc_glVertexAttrib1svARB) gdk_gl_get_proc_address ("glVertexAttrib1svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1svARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib1svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib1svARB);
}

/* glVertexAttrib2dARB */
GdkGLProc
gdk_gl_get_glVertexAttrib2dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib2dARB == (GdkGLProc_glVertexAttrib2dARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib2dARB =
      (GdkGLProc_glVertexAttrib2dARB) gdk_gl_get_proc_address ("glVertexAttrib2dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2dARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib2dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib2dARB);
}

/* glVertexAttrib2dvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib2dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib2dvARB == (GdkGLProc_glVertexAttrib2dvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib2dvARB =
      (GdkGLProc_glVertexAttrib2dvARB) gdk_gl_get_proc_address ("glVertexAttrib2dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2dvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib2dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib2dvARB);
}

/* glVertexAttrib2fARB */
GdkGLProc
gdk_gl_get_glVertexAttrib2fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib2fARB == (GdkGLProc_glVertexAttrib2fARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib2fARB =
      (GdkGLProc_glVertexAttrib2fARB) gdk_gl_get_proc_address ("glVertexAttrib2fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2fARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib2fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib2fARB);
}

/* glVertexAttrib2fvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib2fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib2fvARB == (GdkGLProc_glVertexAttrib2fvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib2fvARB =
      (GdkGLProc_glVertexAttrib2fvARB) gdk_gl_get_proc_address ("glVertexAttrib2fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2fvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib2fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib2fvARB);
}

/* glVertexAttrib2sARB */
GdkGLProc
gdk_gl_get_glVertexAttrib2sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib2sARB == (GdkGLProc_glVertexAttrib2sARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib2sARB =
      (GdkGLProc_glVertexAttrib2sARB) gdk_gl_get_proc_address ("glVertexAttrib2sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2sARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib2sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib2sARB);
}

/* glVertexAttrib2svARB */
GdkGLProc
gdk_gl_get_glVertexAttrib2svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib2svARB == (GdkGLProc_glVertexAttrib2svARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib2svARB =
      (GdkGLProc_glVertexAttrib2svARB) gdk_gl_get_proc_address ("glVertexAttrib2svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2svARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib2svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib2svARB);
}

/* glVertexAttrib3dARB */
GdkGLProc
gdk_gl_get_glVertexAttrib3dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib3dARB == (GdkGLProc_glVertexAttrib3dARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib3dARB =
      (GdkGLProc_glVertexAttrib3dARB) gdk_gl_get_proc_address ("glVertexAttrib3dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3dARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib3dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib3dARB);
}

/* glVertexAttrib3dvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib3dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib3dvARB == (GdkGLProc_glVertexAttrib3dvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib3dvARB =
      (GdkGLProc_glVertexAttrib3dvARB) gdk_gl_get_proc_address ("glVertexAttrib3dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3dvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib3dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib3dvARB);
}

/* glVertexAttrib3fARB */
GdkGLProc
gdk_gl_get_glVertexAttrib3fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib3fARB == (GdkGLProc_glVertexAttrib3fARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib3fARB =
      (GdkGLProc_glVertexAttrib3fARB) gdk_gl_get_proc_address ("glVertexAttrib3fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3fARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib3fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib3fARB);
}

/* glVertexAttrib3fvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib3fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib3fvARB == (GdkGLProc_glVertexAttrib3fvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib3fvARB =
      (GdkGLProc_glVertexAttrib3fvARB) gdk_gl_get_proc_address ("glVertexAttrib3fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3fvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib3fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib3fvARB);
}

/* glVertexAttrib3sARB */
GdkGLProc
gdk_gl_get_glVertexAttrib3sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib3sARB == (GdkGLProc_glVertexAttrib3sARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib3sARB =
      (GdkGLProc_glVertexAttrib3sARB) gdk_gl_get_proc_address ("glVertexAttrib3sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3sARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib3sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib3sARB);
}

/* glVertexAttrib3svARB */
GdkGLProc
gdk_gl_get_glVertexAttrib3svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib3svARB == (GdkGLProc_glVertexAttrib3svARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib3svARB =
      (GdkGLProc_glVertexAttrib3svARB) gdk_gl_get_proc_address ("glVertexAttrib3svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3svARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib3svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib3svARB);
}

/* glVertexAttrib4NbvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NbvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NbvARB == (GdkGLProc_glVertexAttrib4NbvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NbvARB =
      (GdkGLProc_glVertexAttrib4NbvARB) gdk_gl_get_proc_address ("glVertexAttrib4NbvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NbvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NbvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NbvARB);
}

/* glVertexAttrib4NivARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NivARB == (GdkGLProc_glVertexAttrib4NivARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NivARB =
      (GdkGLProc_glVertexAttrib4NivARB) gdk_gl_get_proc_address ("glVertexAttrib4NivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NivARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NivARB);
}

/* glVertexAttrib4NsvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NsvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NsvARB == (GdkGLProc_glVertexAttrib4NsvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NsvARB =
      (GdkGLProc_glVertexAttrib4NsvARB) gdk_gl_get_proc_address ("glVertexAttrib4NsvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NsvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NsvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NsvARB);
}

/* glVertexAttrib4NubARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NubARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NubARB == (GdkGLProc_glVertexAttrib4NubARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NubARB =
      (GdkGLProc_glVertexAttrib4NubARB) gdk_gl_get_proc_address ("glVertexAttrib4NubARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NubARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NubARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NubARB);
}

/* glVertexAttrib4NubvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NubvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NubvARB == (GdkGLProc_glVertexAttrib4NubvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NubvARB =
      (GdkGLProc_glVertexAttrib4NubvARB) gdk_gl_get_proc_address ("glVertexAttrib4NubvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NubvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NubvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NubvARB);
}

/* glVertexAttrib4NuivARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NuivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NuivARB == (GdkGLProc_glVertexAttrib4NuivARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NuivARB =
      (GdkGLProc_glVertexAttrib4NuivARB) gdk_gl_get_proc_address ("glVertexAttrib4NuivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NuivARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NuivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NuivARB);
}

/* glVertexAttrib4NusvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4NusvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4NusvARB == (GdkGLProc_glVertexAttrib4NusvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4NusvARB =
      (GdkGLProc_glVertexAttrib4NusvARB) gdk_gl_get_proc_address ("glVertexAttrib4NusvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4NusvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4NusvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4NusvARB);
}

/* glVertexAttrib4bvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4bvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4bvARB == (GdkGLProc_glVertexAttrib4bvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4bvARB =
      (GdkGLProc_glVertexAttrib4bvARB) gdk_gl_get_proc_address ("glVertexAttrib4bvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4bvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4bvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4bvARB);
}

/* glVertexAttrib4dARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4dARB == (GdkGLProc_glVertexAttrib4dARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4dARB =
      (GdkGLProc_glVertexAttrib4dARB) gdk_gl_get_proc_address ("glVertexAttrib4dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4dARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4dARB);
}

/* glVertexAttrib4dvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4dvARB == (GdkGLProc_glVertexAttrib4dvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4dvARB =
      (GdkGLProc_glVertexAttrib4dvARB) gdk_gl_get_proc_address ("glVertexAttrib4dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4dvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4dvARB);
}

/* glVertexAttrib4fARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4fARB == (GdkGLProc_glVertexAttrib4fARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4fARB =
      (GdkGLProc_glVertexAttrib4fARB) gdk_gl_get_proc_address ("glVertexAttrib4fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4fARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4fARB);
}

/* glVertexAttrib4fvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4fvARB == (GdkGLProc_glVertexAttrib4fvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4fvARB =
      (GdkGLProc_glVertexAttrib4fvARB) gdk_gl_get_proc_address ("glVertexAttrib4fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4fvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4fvARB);
}

/* glVertexAttrib4ivARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4ivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4ivARB == (GdkGLProc_glVertexAttrib4ivARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4ivARB =
      (GdkGLProc_glVertexAttrib4ivARB) gdk_gl_get_proc_address ("glVertexAttrib4ivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4ivARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4ivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4ivARB);
}

/* glVertexAttrib4sARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4sARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4sARB == (GdkGLProc_glVertexAttrib4sARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4sARB =
      (GdkGLProc_glVertexAttrib4sARB) gdk_gl_get_proc_address ("glVertexAttrib4sARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4sARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4sARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4sARB);
}

/* glVertexAttrib4svARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4svARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4svARB == (GdkGLProc_glVertexAttrib4svARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4svARB =
      (GdkGLProc_glVertexAttrib4svARB) gdk_gl_get_proc_address ("glVertexAttrib4svARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4svARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4svARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4svARB);
}

/* glVertexAttrib4ubvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4ubvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4ubvARB == (GdkGLProc_glVertexAttrib4ubvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4ubvARB =
      (GdkGLProc_glVertexAttrib4ubvARB) gdk_gl_get_proc_address ("glVertexAttrib4ubvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4ubvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4ubvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4ubvARB);
}

/* glVertexAttrib4uivARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4uivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4uivARB == (GdkGLProc_glVertexAttrib4uivARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4uivARB =
      (GdkGLProc_glVertexAttrib4uivARB) gdk_gl_get_proc_address ("glVertexAttrib4uivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4uivARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4uivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4uivARB);
}

/* glVertexAttrib4usvARB */
GdkGLProc
gdk_gl_get_glVertexAttrib4usvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttrib4usvARB == (GdkGLProc_glVertexAttrib4usvARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttrib4usvARB =
      (GdkGLProc_glVertexAttrib4usvARB) gdk_gl_get_proc_address ("glVertexAttrib4usvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4usvARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttrib4usvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttrib4usvARB);
}

/* glVertexAttribPointerARB */
GdkGLProc
gdk_gl_get_glVertexAttribPointerARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glVertexAttribPointerARB == (GdkGLProc_glVertexAttribPointerARB) -1)
    _procs_GL_ARB_vertex_program.glVertexAttribPointerARB =
      (GdkGLProc_glVertexAttribPointerARB) gdk_gl_get_proc_address ("glVertexAttribPointerARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribPointerARB () - %s",
               (_procs_GL_ARB_vertex_program.glVertexAttribPointerARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glVertexAttribPointerARB);
}

/* glEnableVertexAttribArrayARB */
GdkGLProc
gdk_gl_get_glEnableVertexAttribArrayARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glEnableVertexAttribArrayARB == (GdkGLProc_glEnableVertexAttribArrayARB) -1)
    _procs_GL_ARB_vertex_program.glEnableVertexAttribArrayARB =
      (GdkGLProc_glEnableVertexAttribArrayARB) gdk_gl_get_proc_address ("glEnableVertexAttribArrayARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEnableVertexAttribArrayARB () - %s",
               (_procs_GL_ARB_vertex_program.glEnableVertexAttribArrayARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glEnableVertexAttribArrayARB);
}

/* glDisableVertexAttribArrayARB */
GdkGLProc
gdk_gl_get_glDisableVertexAttribArrayARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glDisableVertexAttribArrayARB == (GdkGLProc_glDisableVertexAttribArrayARB) -1)
    _procs_GL_ARB_vertex_program.glDisableVertexAttribArrayARB =
      (GdkGLProc_glDisableVertexAttribArrayARB) gdk_gl_get_proc_address ("glDisableVertexAttribArrayARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDisableVertexAttribArrayARB () - %s",
               (_procs_GL_ARB_vertex_program.glDisableVertexAttribArrayARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glDisableVertexAttribArrayARB);
}

/* glProgramStringARB */
GdkGLProc
gdk_gl_get_glProgramStringARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramStringARB == (GdkGLProc_glProgramStringARB) -1)
    _procs_GL_ARB_vertex_program.glProgramStringARB =
      (GdkGLProc_glProgramStringARB) gdk_gl_get_proc_address ("glProgramStringARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramStringARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramStringARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramStringARB);
}

/* glBindProgramARB */
GdkGLProc
gdk_gl_get_glBindProgramARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glBindProgramARB == (GdkGLProc_glBindProgramARB) -1)
    _procs_GL_ARB_vertex_program.glBindProgramARB =
      (GdkGLProc_glBindProgramARB) gdk_gl_get_proc_address ("glBindProgramARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindProgramARB () - %s",
               (_procs_GL_ARB_vertex_program.glBindProgramARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glBindProgramARB);
}

/* glDeleteProgramsARB */
GdkGLProc
gdk_gl_get_glDeleteProgramsARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glDeleteProgramsARB == (GdkGLProc_glDeleteProgramsARB) -1)
    _procs_GL_ARB_vertex_program.glDeleteProgramsARB =
      (GdkGLProc_glDeleteProgramsARB) gdk_gl_get_proc_address ("glDeleteProgramsARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteProgramsARB () - %s",
               (_procs_GL_ARB_vertex_program.glDeleteProgramsARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glDeleteProgramsARB);
}

/* glGenProgramsARB */
GdkGLProc
gdk_gl_get_glGenProgramsARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGenProgramsARB == (GdkGLProc_glGenProgramsARB) -1)
    _procs_GL_ARB_vertex_program.glGenProgramsARB =
      (GdkGLProc_glGenProgramsARB) gdk_gl_get_proc_address ("glGenProgramsARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenProgramsARB () - %s",
               (_procs_GL_ARB_vertex_program.glGenProgramsARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGenProgramsARB);
}

/* glProgramEnvParameter4dARB */
GdkGLProc
gdk_gl_get_glProgramEnvParameter4dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramEnvParameter4dARB == (GdkGLProc_glProgramEnvParameter4dARB) -1)
    _procs_GL_ARB_vertex_program.glProgramEnvParameter4dARB =
      (GdkGLProc_glProgramEnvParameter4dARB) gdk_gl_get_proc_address ("glProgramEnvParameter4dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramEnvParameter4dARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramEnvParameter4dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramEnvParameter4dARB);
}

/* glProgramEnvParameter4dvARB */
GdkGLProc
gdk_gl_get_glProgramEnvParameter4dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramEnvParameter4dvARB == (GdkGLProc_glProgramEnvParameter4dvARB) -1)
    _procs_GL_ARB_vertex_program.glProgramEnvParameter4dvARB =
      (GdkGLProc_glProgramEnvParameter4dvARB) gdk_gl_get_proc_address ("glProgramEnvParameter4dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramEnvParameter4dvARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramEnvParameter4dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramEnvParameter4dvARB);
}

/* glProgramEnvParameter4fARB */
GdkGLProc
gdk_gl_get_glProgramEnvParameter4fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramEnvParameter4fARB == (GdkGLProc_glProgramEnvParameter4fARB) -1)
    _procs_GL_ARB_vertex_program.glProgramEnvParameter4fARB =
      (GdkGLProc_glProgramEnvParameter4fARB) gdk_gl_get_proc_address ("glProgramEnvParameter4fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramEnvParameter4fARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramEnvParameter4fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramEnvParameter4fARB);
}

/* glProgramEnvParameter4fvARB */
GdkGLProc
gdk_gl_get_glProgramEnvParameter4fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramEnvParameter4fvARB == (GdkGLProc_glProgramEnvParameter4fvARB) -1)
    _procs_GL_ARB_vertex_program.glProgramEnvParameter4fvARB =
      (GdkGLProc_glProgramEnvParameter4fvARB) gdk_gl_get_proc_address ("glProgramEnvParameter4fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramEnvParameter4fvARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramEnvParameter4fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramEnvParameter4fvARB);
}

/* glProgramLocalParameter4dARB */
GdkGLProc
gdk_gl_get_glProgramLocalParameter4dARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramLocalParameter4dARB == (GdkGLProc_glProgramLocalParameter4dARB) -1)
    _procs_GL_ARB_vertex_program.glProgramLocalParameter4dARB =
      (GdkGLProc_glProgramLocalParameter4dARB) gdk_gl_get_proc_address ("glProgramLocalParameter4dARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramLocalParameter4dARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramLocalParameter4dARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramLocalParameter4dARB);
}

/* glProgramLocalParameter4dvARB */
GdkGLProc
gdk_gl_get_glProgramLocalParameter4dvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramLocalParameter4dvARB == (GdkGLProc_glProgramLocalParameter4dvARB) -1)
    _procs_GL_ARB_vertex_program.glProgramLocalParameter4dvARB =
      (GdkGLProc_glProgramLocalParameter4dvARB) gdk_gl_get_proc_address ("glProgramLocalParameter4dvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramLocalParameter4dvARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramLocalParameter4dvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramLocalParameter4dvARB);
}

/* glProgramLocalParameter4fARB */
GdkGLProc
gdk_gl_get_glProgramLocalParameter4fARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramLocalParameter4fARB == (GdkGLProc_glProgramLocalParameter4fARB) -1)
    _procs_GL_ARB_vertex_program.glProgramLocalParameter4fARB =
      (GdkGLProc_glProgramLocalParameter4fARB) gdk_gl_get_proc_address ("glProgramLocalParameter4fARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramLocalParameter4fARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramLocalParameter4fARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramLocalParameter4fARB);
}

/* glProgramLocalParameter4fvARB */
GdkGLProc
gdk_gl_get_glProgramLocalParameter4fvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glProgramLocalParameter4fvARB == (GdkGLProc_glProgramLocalParameter4fvARB) -1)
    _procs_GL_ARB_vertex_program.glProgramLocalParameter4fvARB =
      (GdkGLProc_glProgramLocalParameter4fvARB) gdk_gl_get_proc_address ("glProgramLocalParameter4fvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramLocalParameter4fvARB () - %s",
               (_procs_GL_ARB_vertex_program.glProgramLocalParameter4fvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glProgramLocalParameter4fvARB);
}

/* glGetProgramEnvParameterdvARB */
GdkGLProc
gdk_gl_get_glGetProgramEnvParameterdvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetProgramEnvParameterdvARB == (GdkGLProc_glGetProgramEnvParameterdvARB) -1)
    _procs_GL_ARB_vertex_program.glGetProgramEnvParameterdvARB =
      (GdkGLProc_glGetProgramEnvParameterdvARB) gdk_gl_get_proc_address ("glGetProgramEnvParameterdvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramEnvParameterdvARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetProgramEnvParameterdvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetProgramEnvParameterdvARB);
}

/* glGetProgramEnvParameterfvARB */
GdkGLProc
gdk_gl_get_glGetProgramEnvParameterfvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetProgramEnvParameterfvARB == (GdkGLProc_glGetProgramEnvParameterfvARB) -1)
    _procs_GL_ARB_vertex_program.glGetProgramEnvParameterfvARB =
      (GdkGLProc_glGetProgramEnvParameterfvARB) gdk_gl_get_proc_address ("glGetProgramEnvParameterfvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramEnvParameterfvARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetProgramEnvParameterfvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetProgramEnvParameterfvARB);
}

/* glGetProgramLocalParameterdvARB */
GdkGLProc
gdk_gl_get_glGetProgramLocalParameterdvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetProgramLocalParameterdvARB == (GdkGLProc_glGetProgramLocalParameterdvARB) -1)
    _procs_GL_ARB_vertex_program.glGetProgramLocalParameterdvARB =
      (GdkGLProc_glGetProgramLocalParameterdvARB) gdk_gl_get_proc_address ("glGetProgramLocalParameterdvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramLocalParameterdvARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetProgramLocalParameterdvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetProgramLocalParameterdvARB);
}

/* glGetProgramLocalParameterfvARB */
GdkGLProc
gdk_gl_get_glGetProgramLocalParameterfvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetProgramLocalParameterfvARB == (GdkGLProc_glGetProgramLocalParameterfvARB) -1)
    _procs_GL_ARB_vertex_program.glGetProgramLocalParameterfvARB =
      (GdkGLProc_glGetProgramLocalParameterfvARB) gdk_gl_get_proc_address ("glGetProgramLocalParameterfvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramLocalParameterfvARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetProgramLocalParameterfvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetProgramLocalParameterfvARB);
}

/* glGetProgramivARB */
GdkGLProc
gdk_gl_get_glGetProgramivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetProgramivARB == (GdkGLProc_glGetProgramivARB) -1)
    _procs_GL_ARB_vertex_program.glGetProgramivARB =
      (GdkGLProc_glGetProgramivARB) gdk_gl_get_proc_address ("glGetProgramivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramivARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetProgramivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetProgramivARB);
}

/* glGetProgramStringARB */
GdkGLProc
gdk_gl_get_glGetProgramStringARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetProgramStringARB == (GdkGLProc_glGetProgramStringARB) -1)
    _procs_GL_ARB_vertex_program.glGetProgramStringARB =
      (GdkGLProc_glGetProgramStringARB) gdk_gl_get_proc_address ("glGetProgramStringARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramStringARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetProgramStringARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetProgramStringARB);
}

/* glGetVertexAttribdvARB */
GdkGLProc
gdk_gl_get_glGetVertexAttribdvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetVertexAttribdvARB == (GdkGLProc_glGetVertexAttribdvARB) -1)
    _procs_GL_ARB_vertex_program.glGetVertexAttribdvARB =
      (GdkGLProc_glGetVertexAttribdvARB) gdk_gl_get_proc_address ("glGetVertexAttribdvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribdvARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetVertexAttribdvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetVertexAttribdvARB);
}

/* glGetVertexAttribfvARB */
GdkGLProc
gdk_gl_get_glGetVertexAttribfvARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetVertexAttribfvARB == (GdkGLProc_glGetVertexAttribfvARB) -1)
    _procs_GL_ARB_vertex_program.glGetVertexAttribfvARB =
      (GdkGLProc_glGetVertexAttribfvARB) gdk_gl_get_proc_address ("glGetVertexAttribfvARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribfvARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetVertexAttribfvARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetVertexAttribfvARB);
}

/* glGetVertexAttribivARB */
GdkGLProc
gdk_gl_get_glGetVertexAttribivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetVertexAttribivARB == (GdkGLProc_glGetVertexAttribivARB) -1)
    _procs_GL_ARB_vertex_program.glGetVertexAttribivARB =
      (GdkGLProc_glGetVertexAttribivARB) gdk_gl_get_proc_address ("glGetVertexAttribivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribivARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetVertexAttribivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetVertexAttribivARB);
}

/* glGetVertexAttribPointervARB */
GdkGLProc
gdk_gl_get_glGetVertexAttribPointervARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glGetVertexAttribPointervARB == (GdkGLProc_glGetVertexAttribPointervARB) -1)
    _procs_GL_ARB_vertex_program.glGetVertexAttribPointervARB =
      (GdkGLProc_glGetVertexAttribPointervARB) gdk_gl_get_proc_address ("glGetVertexAttribPointervARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribPointervARB () - %s",
               (_procs_GL_ARB_vertex_program.glGetVertexAttribPointervARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glGetVertexAttribPointervARB);
}

/* glIsProgramARB */
GdkGLProc
gdk_gl_get_glIsProgramARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_program.glIsProgramARB == (GdkGLProc_glIsProgramARB) -1)
    _procs_GL_ARB_vertex_program.glIsProgramARB =
      (GdkGLProc_glIsProgramARB) gdk_gl_get_proc_address ("glIsProgramARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsProgramARB () - %s",
               (_procs_GL_ARB_vertex_program.glIsProgramARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_program.glIsProgramARB);
}

/* Get GL_ARB_vertex_program functions */
GdkGL_GL_ARB_vertex_program *
gdk_gl_get_GL_ARB_vertex_program (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_vertex_program");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertexAttrib1dARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1dvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1fARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1fvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1sARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1svARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2dARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2dvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2fARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2fvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2sARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2svARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3dARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3dvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3fARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3fvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3sARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3svARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NbvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NivARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NsvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NubARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NubvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NuivARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4NusvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4bvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4dARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4dvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4fARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4fvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4ivARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4sARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4svARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4ubvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4uivARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4usvARB () != NULL);
          supported &= (gdk_gl_get_glVertexAttribPointerARB () != NULL);
          supported &= (gdk_gl_get_glEnableVertexAttribArrayARB () != NULL);
          supported &= (gdk_gl_get_glDisableVertexAttribArrayARB () != NULL);
          supported &= (gdk_gl_get_glProgramStringARB () != NULL);
          supported &= (gdk_gl_get_glBindProgramARB () != NULL);
          supported &= (gdk_gl_get_glDeleteProgramsARB () != NULL);
          supported &= (gdk_gl_get_glGenProgramsARB () != NULL);
          supported &= (gdk_gl_get_glProgramEnvParameter4dARB () != NULL);
          supported &= (gdk_gl_get_glProgramEnvParameter4dvARB () != NULL);
          supported &= (gdk_gl_get_glProgramEnvParameter4fARB () != NULL);
          supported &= (gdk_gl_get_glProgramEnvParameter4fvARB () != NULL);
          supported &= (gdk_gl_get_glProgramLocalParameter4dARB () != NULL);
          supported &= (gdk_gl_get_glProgramLocalParameter4dvARB () != NULL);
          supported &= (gdk_gl_get_glProgramLocalParameter4fARB () != NULL);
          supported &= (gdk_gl_get_glProgramLocalParameter4fvARB () != NULL);
          supported &= (gdk_gl_get_glGetProgramEnvParameterdvARB () != NULL);
          supported &= (gdk_gl_get_glGetProgramEnvParameterfvARB () != NULL);
          supported &= (gdk_gl_get_glGetProgramLocalParameterdvARB () != NULL);
          supported &= (gdk_gl_get_glGetProgramLocalParameterfvARB () != NULL);
          supported &= (gdk_gl_get_glGetProgramivARB () != NULL);
          supported &= (gdk_gl_get_glGetProgramStringARB () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribdvARB () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribfvARB () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribivARB () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribPointervARB () != NULL);
          supported &= (gdk_gl_get_glIsProgramARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_vertex_program () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_vertex_program;
}

/*
 * GL_ARB_vertex_buffer_object
 */

static GdkGL_GL_ARB_vertex_buffer_object _procs_GL_ARB_vertex_buffer_object = {
  (GdkGLProc_glBindBufferARB) -1,
  (GdkGLProc_glDeleteBuffersARB) -1,
  (GdkGLProc_glGenBuffersARB) -1,
  (GdkGLProc_glIsBufferARB) -1,
  (GdkGLProc_glBufferDataARB) -1,
  (GdkGLProc_glBufferSubDataARB) -1,
  (GdkGLProc_glGetBufferSubDataARB) -1,
  (GdkGLProc_glMapBufferARB) -1,
  (GdkGLProc_glUnmapBufferARB) -1,
  (GdkGLProc_glGetBufferParameterivARB) -1,
  (GdkGLProc_glGetBufferPointervARB) -1
};

/* glBindBufferARB */
GdkGLProc
gdk_gl_get_glBindBufferARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glBindBufferARB == (GdkGLProc_glBindBufferARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glBindBufferARB =
      (GdkGLProc_glBindBufferARB) gdk_gl_get_proc_address ("glBindBufferARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindBufferARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glBindBufferARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glBindBufferARB);
}

/* glDeleteBuffersARB */
GdkGLProc
gdk_gl_get_glDeleteBuffersARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glDeleteBuffersARB == (GdkGLProc_glDeleteBuffersARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glDeleteBuffersARB =
      (GdkGLProc_glDeleteBuffersARB) gdk_gl_get_proc_address ("glDeleteBuffersARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteBuffersARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glDeleteBuffersARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glDeleteBuffersARB);
}

/* glGenBuffersARB */
GdkGLProc
gdk_gl_get_glGenBuffersARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glGenBuffersARB == (GdkGLProc_glGenBuffersARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glGenBuffersARB =
      (GdkGLProc_glGenBuffersARB) gdk_gl_get_proc_address ("glGenBuffersARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenBuffersARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glGenBuffersARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glGenBuffersARB);
}

/* glIsBufferARB */
GdkGLProc
gdk_gl_get_glIsBufferARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glIsBufferARB == (GdkGLProc_glIsBufferARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glIsBufferARB =
      (GdkGLProc_glIsBufferARB) gdk_gl_get_proc_address ("glIsBufferARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsBufferARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glIsBufferARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glIsBufferARB);
}

/* glBufferDataARB */
GdkGLProc
gdk_gl_get_glBufferDataARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glBufferDataARB == (GdkGLProc_glBufferDataARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glBufferDataARB =
      (GdkGLProc_glBufferDataARB) gdk_gl_get_proc_address ("glBufferDataARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBufferDataARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glBufferDataARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glBufferDataARB);
}

/* glBufferSubDataARB */
GdkGLProc
gdk_gl_get_glBufferSubDataARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glBufferSubDataARB == (GdkGLProc_glBufferSubDataARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glBufferSubDataARB =
      (GdkGLProc_glBufferSubDataARB) gdk_gl_get_proc_address ("glBufferSubDataARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBufferSubDataARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glBufferSubDataARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glBufferSubDataARB);
}

/* glGetBufferSubDataARB */
GdkGLProc
gdk_gl_get_glGetBufferSubDataARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glGetBufferSubDataARB == (GdkGLProc_glGetBufferSubDataARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glGetBufferSubDataARB =
      (GdkGLProc_glGetBufferSubDataARB) gdk_gl_get_proc_address ("glGetBufferSubDataARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetBufferSubDataARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glGetBufferSubDataARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glGetBufferSubDataARB);
}

/* glMapBufferARB */
GdkGLProc
gdk_gl_get_glMapBufferARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glMapBufferARB == (GdkGLProc_glMapBufferARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glMapBufferARB =
      (GdkGLProc_glMapBufferARB) gdk_gl_get_proc_address ("glMapBufferARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapBufferARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glMapBufferARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glMapBufferARB);
}

/* glUnmapBufferARB */
GdkGLProc
gdk_gl_get_glUnmapBufferARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glUnmapBufferARB == (GdkGLProc_glUnmapBufferARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glUnmapBufferARB =
      (GdkGLProc_glUnmapBufferARB) gdk_gl_get_proc_address ("glUnmapBufferARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glUnmapBufferARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glUnmapBufferARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glUnmapBufferARB);
}

/* glGetBufferParameterivARB */
GdkGLProc
gdk_gl_get_glGetBufferParameterivARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glGetBufferParameterivARB == (GdkGLProc_glGetBufferParameterivARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glGetBufferParameterivARB =
      (GdkGLProc_glGetBufferParameterivARB) gdk_gl_get_proc_address ("glGetBufferParameterivARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetBufferParameterivARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glGetBufferParameterivARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glGetBufferParameterivARB);
}

/* glGetBufferPointervARB */
GdkGLProc
gdk_gl_get_glGetBufferPointervARB (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ARB_vertex_buffer_object.glGetBufferPointervARB == (GdkGLProc_glGetBufferPointervARB) -1)
    _procs_GL_ARB_vertex_buffer_object.glGetBufferPointervARB =
      (GdkGLProc_glGetBufferPointervARB) gdk_gl_get_proc_address ("glGetBufferPointervARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetBufferPointervARB () - %s",
               (_procs_GL_ARB_vertex_buffer_object.glGetBufferPointervARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ARB_vertex_buffer_object.glGetBufferPointervARB);
}

/* Get GL_ARB_vertex_buffer_object functions */
GdkGL_GL_ARB_vertex_buffer_object *
gdk_gl_get_GL_ARB_vertex_buffer_object (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ARB_vertex_buffer_object");

      if (supported)
        {
          supported &= (gdk_gl_get_glBindBufferARB () != NULL);
          supported &= (gdk_gl_get_glDeleteBuffersARB () != NULL);
          supported &= (gdk_gl_get_glGenBuffersARB () != NULL);
          supported &= (gdk_gl_get_glIsBufferARB () != NULL);
          supported &= (gdk_gl_get_glBufferDataARB () != NULL);
          supported &= (gdk_gl_get_glBufferSubDataARB () != NULL);
          supported &= (gdk_gl_get_glGetBufferSubDataARB () != NULL);
          supported &= (gdk_gl_get_glMapBufferARB () != NULL);
          supported &= (gdk_gl_get_glUnmapBufferARB () != NULL);
          supported &= (gdk_gl_get_glGetBufferParameterivARB () != NULL);
          supported &= (gdk_gl_get_glGetBufferPointervARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ARB_vertex_buffer_object () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ARB_vertex_buffer_object;
}

/*
 * GL_EXT_blend_color
 */

static GdkGL_GL_EXT_blend_color _procs_GL_EXT_blend_color = {
  (GdkGLProc_glBlendColorEXT) -1
};

/* glBlendColorEXT */
GdkGLProc
gdk_gl_get_glBlendColorEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_blend_color.glBlendColorEXT == (GdkGLProc_glBlendColorEXT) -1)
    _procs_GL_EXT_blend_color.glBlendColorEXT =
      (GdkGLProc_glBlendColorEXT) gdk_gl_get_proc_address ("glBlendColorEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendColorEXT () - %s",
               (_procs_GL_EXT_blend_color.glBlendColorEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_blend_color.glBlendColorEXT);
}

/* Get GL_EXT_blend_color functions */
GdkGL_GL_EXT_blend_color *
gdk_gl_get_GL_EXT_blend_color (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_blend_color");

      if (supported)
        {
          supported &= (gdk_gl_get_glBlendColorEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_blend_color () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_blend_color;
}

/*
 * GL_EXT_polygon_offset
 */

static GdkGL_GL_EXT_polygon_offset _procs_GL_EXT_polygon_offset = {
  (GdkGLProc_glPolygonOffsetEXT) -1
};

/* glPolygonOffsetEXT */
GdkGLProc
gdk_gl_get_glPolygonOffsetEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_polygon_offset.glPolygonOffsetEXT == (GdkGLProc_glPolygonOffsetEXT) -1)
    _procs_GL_EXT_polygon_offset.glPolygonOffsetEXT =
      (GdkGLProc_glPolygonOffsetEXT) gdk_gl_get_proc_address ("glPolygonOffsetEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPolygonOffsetEXT () - %s",
               (_procs_GL_EXT_polygon_offset.glPolygonOffsetEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_polygon_offset.glPolygonOffsetEXT);
}

/* Get GL_EXT_polygon_offset functions */
GdkGL_GL_EXT_polygon_offset *
gdk_gl_get_GL_EXT_polygon_offset (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_polygon_offset");

      if (supported)
        {
          supported &= (gdk_gl_get_glPolygonOffsetEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_polygon_offset () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_polygon_offset;
}

/*
 * GL_EXT_texture3D
 */

static GdkGL_GL_EXT_texture3D _procs_GL_EXT_texture3D = {
  (GdkGLProc_glTexImage3DEXT) -1,
  (GdkGLProc_glTexSubImage3DEXT) -1
};

/* glTexImage3DEXT */
GdkGLProc
gdk_gl_get_glTexImage3DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture3D.glTexImage3DEXT == (GdkGLProc_glTexImage3DEXT) -1)
    _procs_GL_EXT_texture3D.glTexImage3DEXT =
      (GdkGLProc_glTexImage3DEXT) gdk_gl_get_proc_address ("glTexImage3DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexImage3DEXT () - %s",
               (_procs_GL_EXT_texture3D.glTexImage3DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture3D.glTexImage3DEXT);
}

/* glTexSubImage3DEXT */
GdkGLProc
gdk_gl_get_glTexSubImage3DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture3D.glTexSubImage3DEXT == (GdkGLProc_glTexSubImage3DEXT) -1)
    _procs_GL_EXT_texture3D.glTexSubImage3DEXT =
      (GdkGLProc_glTexSubImage3DEXT) gdk_gl_get_proc_address ("glTexSubImage3DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexSubImage3DEXT () - %s",
               (_procs_GL_EXT_texture3D.glTexSubImage3DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture3D.glTexSubImage3DEXT);
}

/* Get GL_EXT_texture3D functions */
GdkGL_GL_EXT_texture3D *
gdk_gl_get_GL_EXT_texture3D (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_texture3D");

      if (supported)
        {
          supported &= (gdk_gl_get_glTexImage3DEXT () != NULL);
          supported &= (gdk_gl_get_glTexSubImage3DEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_texture3D () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_texture3D;
}

/*
 * GL_SGIS_texture_filter4
 */

static GdkGL_GL_SGIS_texture_filter4 _procs_GL_SGIS_texture_filter4 = {
  (GdkGLProc_glGetTexFilterFuncSGIS) -1,
  (GdkGLProc_glTexFilterFuncSGIS) -1
};

/* glGetTexFilterFuncSGIS */
GdkGLProc
gdk_gl_get_glGetTexFilterFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_texture_filter4.glGetTexFilterFuncSGIS == (GdkGLProc_glGetTexFilterFuncSGIS) -1)
    _procs_GL_SGIS_texture_filter4.glGetTexFilterFuncSGIS =
      (GdkGLProc_glGetTexFilterFuncSGIS) gdk_gl_get_proc_address ("glGetTexFilterFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetTexFilterFuncSGIS () - %s",
               (_procs_GL_SGIS_texture_filter4.glGetTexFilterFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_texture_filter4.glGetTexFilterFuncSGIS);
}

/* glTexFilterFuncSGIS */
GdkGLProc
gdk_gl_get_glTexFilterFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_texture_filter4.glTexFilterFuncSGIS == (GdkGLProc_glTexFilterFuncSGIS) -1)
    _procs_GL_SGIS_texture_filter4.glTexFilterFuncSGIS =
      (GdkGLProc_glTexFilterFuncSGIS) gdk_gl_get_proc_address ("glTexFilterFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexFilterFuncSGIS () - %s",
               (_procs_GL_SGIS_texture_filter4.glTexFilterFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_texture_filter4.glTexFilterFuncSGIS);
}

/* Get GL_SGIS_texture_filter4 functions */
GdkGL_GL_SGIS_texture_filter4 *
gdk_gl_get_GL_SGIS_texture_filter4 (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_texture_filter4");

      if (supported)
        {
          supported &= (gdk_gl_get_glGetTexFilterFuncSGIS () != NULL);
          supported &= (gdk_gl_get_glTexFilterFuncSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_texture_filter4 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_texture_filter4;
}

/*
 * GL_EXT_subtexture
 */

static GdkGL_GL_EXT_subtexture _procs_GL_EXT_subtexture = {
  (GdkGLProc_glTexSubImage1DEXT) -1,
  (GdkGLProc_glTexSubImage2DEXT) -1
};

/* glTexSubImage1DEXT */
GdkGLProc
gdk_gl_get_glTexSubImage1DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_subtexture.glTexSubImage1DEXT == (GdkGLProc_glTexSubImage1DEXT) -1)
    _procs_GL_EXT_subtexture.glTexSubImage1DEXT =
      (GdkGLProc_glTexSubImage1DEXT) gdk_gl_get_proc_address ("glTexSubImage1DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexSubImage1DEXT () - %s",
               (_procs_GL_EXT_subtexture.glTexSubImage1DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_subtexture.glTexSubImage1DEXT);
}

/* glTexSubImage2DEXT */
GdkGLProc
gdk_gl_get_glTexSubImage2DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_subtexture.glTexSubImage2DEXT == (GdkGLProc_glTexSubImage2DEXT) -1)
    _procs_GL_EXT_subtexture.glTexSubImage2DEXT =
      (GdkGLProc_glTexSubImage2DEXT) gdk_gl_get_proc_address ("glTexSubImage2DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexSubImage2DEXT () - %s",
               (_procs_GL_EXT_subtexture.glTexSubImage2DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_subtexture.glTexSubImage2DEXT);
}

/* Get GL_EXT_subtexture functions */
GdkGL_GL_EXT_subtexture *
gdk_gl_get_GL_EXT_subtexture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_subtexture");

      if (supported)
        {
          supported &= (gdk_gl_get_glTexSubImage1DEXT () != NULL);
          supported &= (gdk_gl_get_glTexSubImage2DEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_subtexture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_subtexture;
}

/*
 * GL_EXT_copy_texture
 */

static GdkGL_GL_EXT_copy_texture _procs_GL_EXT_copy_texture = {
  (GdkGLProc_glCopyTexImage1DEXT) -1,
  (GdkGLProc_glCopyTexImage2DEXT) -1,
  (GdkGLProc_glCopyTexSubImage1DEXT) -1,
  (GdkGLProc_glCopyTexSubImage2DEXT) -1,
  (GdkGLProc_glCopyTexSubImage3DEXT) -1
};

/* glCopyTexImage1DEXT */
GdkGLProc
gdk_gl_get_glCopyTexImage1DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_copy_texture.glCopyTexImage1DEXT == (GdkGLProc_glCopyTexImage1DEXT) -1)
    _procs_GL_EXT_copy_texture.glCopyTexImage1DEXT =
      (GdkGLProc_glCopyTexImage1DEXT) gdk_gl_get_proc_address ("glCopyTexImage1DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyTexImage1DEXT () - %s",
               (_procs_GL_EXT_copy_texture.glCopyTexImage1DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_copy_texture.glCopyTexImage1DEXT);
}

/* glCopyTexImage2DEXT */
GdkGLProc
gdk_gl_get_glCopyTexImage2DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_copy_texture.glCopyTexImage2DEXT == (GdkGLProc_glCopyTexImage2DEXT) -1)
    _procs_GL_EXT_copy_texture.glCopyTexImage2DEXT =
      (GdkGLProc_glCopyTexImage2DEXT) gdk_gl_get_proc_address ("glCopyTexImage2DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyTexImage2DEXT () - %s",
               (_procs_GL_EXT_copy_texture.glCopyTexImage2DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_copy_texture.glCopyTexImage2DEXT);
}

/* glCopyTexSubImage1DEXT */
GdkGLProc
gdk_gl_get_glCopyTexSubImage1DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_copy_texture.glCopyTexSubImage1DEXT == (GdkGLProc_glCopyTexSubImage1DEXT) -1)
    _procs_GL_EXT_copy_texture.glCopyTexSubImage1DEXT =
      (GdkGLProc_glCopyTexSubImage1DEXT) gdk_gl_get_proc_address ("glCopyTexSubImage1DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyTexSubImage1DEXT () - %s",
               (_procs_GL_EXT_copy_texture.glCopyTexSubImage1DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_copy_texture.glCopyTexSubImage1DEXT);
}

/* glCopyTexSubImage2DEXT */
GdkGLProc
gdk_gl_get_glCopyTexSubImage2DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_copy_texture.glCopyTexSubImage2DEXT == (GdkGLProc_glCopyTexSubImage2DEXT) -1)
    _procs_GL_EXT_copy_texture.glCopyTexSubImage2DEXT =
      (GdkGLProc_glCopyTexSubImage2DEXT) gdk_gl_get_proc_address ("glCopyTexSubImage2DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyTexSubImage2DEXT () - %s",
               (_procs_GL_EXT_copy_texture.glCopyTexSubImage2DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_copy_texture.glCopyTexSubImage2DEXT);
}

/* glCopyTexSubImage3DEXT */
GdkGLProc
gdk_gl_get_glCopyTexSubImage3DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_copy_texture.glCopyTexSubImage3DEXT == (GdkGLProc_glCopyTexSubImage3DEXT) -1)
    _procs_GL_EXT_copy_texture.glCopyTexSubImage3DEXT =
      (GdkGLProc_glCopyTexSubImage3DEXT) gdk_gl_get_proc_address ("glCopyTexSubImage3DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyTexSubImage3DEXT () - %s",
               (_procs_GL_EXT_copy_texture.glCopyTexSubImage3DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_copy_texture.glCopyTexSubImage3DEXT);
}

/* Get GL_EXT_copy_texture functions */
GdkGL_GL_EXT_copy_texture *
gdk_gl_get_GL_EXT_copy_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_copy_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glCopyTexImage1DEXT () != NULL);
          supported &= (gdk_gl_get_glCopyTexImage2DEXT () != NULL);
          supported &= (gdk_gl_get_glCopyTexSubImage1DEXT () != NULL);
          supported &= (gdk_gl_get_glCopyTexSubImage2DEXT () != NULL);
          supported &= (gdk_gl_get_glCopyTexSubImage3DEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_copy_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_copy_texture;
}

/*
 * GL_EXT_histogram
 */

static GdkGL_GL_EXT_histogram _procs_GL_EXT_histogram = {
  (GdkGLProc_glGetHistogramEXT) -1,
  (GdkGLProc_glGetHistogramParameterfvEXT) -1,
  (GdkGLProc_glGetHistogramParameterivEXT) -1,
  (GdkGLProc_glGetMinmaxEXT) -1,
  (GdkGLProc_glGetMinmaxParameterfvEXT) -1,
  (GdkGLProc_glGetMinmaxParameterivEXT) -1,
  (GdkGLProc_glHistogramEXT) -1,
  (GdkGLProc_glMinmaxEXT) -1,
  (GdkGLProc_glResetHistogramEXT) -1,
  (GdkGLProc_glResetMinmaxEXT) -1
};

/* glGetHistogramEXT */
GdkGLProc
gdk_gl_get_glGetHistogramEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glGetHistogramEXT == (GdkGLProc_glGetHistogramEXT) -1)
    _procs_GL_EXT_histogram.glGetHistogramEXT =
      (GdkGLProc_glGetHistogramEXT) gdk_gl_get_proc_address ("glGetHistogramEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetHistogramEXT () - %s",
               (_procs_GL_EXT_histogram.glGetHistogramEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glGetHistogramEXT);
}

/* glGetHistogramParameterfvEXT */
GdkGLProc
gdk_gl_get_glGetHistogramParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glGetHistogramParameterfvEXT == (GdkGLProc_glGetHistogramParameterfvEXT) -1)
    _procs_GL_EXT_histogram.glGetHistogramParameterfvEXT =
      (GdkGLProc_glGetHistogramParameterfvEXT) gdk_gl_get_proc_address ("glGetHistogramParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetHistogramParameterfvEXT () - %s",
               (_procs_GL_EXT_histogram.glGetHistogramParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glGetHistogramParameterfvEXT);
}

/* glGetHistogramParameterivEXT */
GdkGLProc
gdk_gl_get_glGetHistogramParameterivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glGetHistogramParameterivEXT == (GdkGLProc_glGetHistogramParameterivEXT) -1)
    _procs_GL_EXT_histogram.glGetHistogramParameterivEXT =
      (GdkGLProc_glGetHistogramParameterivEXT) gdk_gl_get_proc_address ("glGetHistogramParameterivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetHistogramParameterivEXT () - %s",
               (_procs_GL_EXT_histogram.glGetHistogramParameterivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glGetHistogramParameterivEXT);
}

/* glGetMinmaxEXT */
GdkGLProc
gdk_gl_get_glGetMinmaxEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glGetMinmaxEXT == (GdkGLProc_glGetMinmaxEXT) -1)
    _procs_GL_EXT_histogram.glGetMinmaxEXT =
      (GdkGLProc_glGetMinmaxEXT) gdk_gl_get_proc_address ("glGetMinmaxEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMinmaxEXT () - %s",
               (_procs_GL_EXT_histogram.glGetMinmaxEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glGetMinmaxEXT);
}

/* glGetMinmaxParameterfvEXT */
GdkGLProc
gdk_gl_get_glGetMinmaxParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glGetMinmaxParameterfvEXT == (GdkGLProc_glGetMinmaxParameterfvEXT) -1)
    _procs_GL_EXT_histogram.glGetMinmaxParameterfvEXT =
      (GdkGLProc_glGetMinmaxParameterfvEXT) gdk_gl_get_proc_address ("glGetMinmaxParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMinmaxParameterfvEXT () - %s",
               (_procs_GL_EXT_histogram.glGetMinmaxParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glGetMinmaxParameterfvEXT);
}

/* glGetMinmaxParameterivEXT */
GdkGLProc
gdk_gl_get_glGetMinmaxParameterivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glGetMinmaxParameterivEXT == (GdkGLProc_glGetMinmaxParameterivEXT) -1)
    _procs_GL_EXT_histogram.glGetMinmaxParameterivEXT =
      (GdkGLProc_glGetMinmaxParameterivEXT) gdk_gl_get_proc_address ("glGetMinmaxParameterivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMinmaxParameterivEXT () - %s",
               (_procs_GL_EXT_histogram.glGetMinmaxParameterivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glGetMinmaxParameterivEXT);
}

/* glHistogramEXT */
GdkGLProc
gdk_gl_get_glHistogramEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glHistogramEXT == (GdkGLProc_glHistogramEXT) -1)
    _procs_GL_EXT_histogram.glHistogramEXT =
      (GdkGLProc_glHistogramEXT) gdk_gl_get_proc_address ("glHistogramEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glHistogramEXT () - %s",
               (_procs_GL_EXT_histogram.glHistogramEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glHistogramEXT);
}

/* glMinmaxEXT */
GdkGLProc
gdk_gl_get_glMinmaxEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glMinmaxEXT == (GdkGLProc_glMinmaxEXT) -1)
    _procs_GL_EXT_histogram.glMinmaxEXT =
      (GdkGLProc_glMinmaxEXT) gdk_gl_get_proc_address ("glMinmaxEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMinmaxEXT () - %s",
               (_procs_GL_EXT_histogram.glMinmaxEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glMinmaxEXT);
}

/* glResetHistogramEXT */
GdkGLProc
gdk_gl_get_glResetHistogramEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glResetHistogramEXT == (GdkGLProc_glResetHistogramEXT) -1)
    _procs_GL_EXT_histogram.glResetHistogramEXT =
      (GdkGLProc_glResetHistogramEXT) gdk_gl_get_proc_address ("glResetHistogramEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glResetHistogramEXT () - %s",
               (_procs_GL_EXT_histogram.glResetHistogramEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glResetHistogramEXT);
}

/* glResetMinmaxEXT */
GdkGLProc
gdk_gl_get_glResetMinmaxEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_histogram.glResetMinmaxEXT == (GdkGLProc_glResetMinmaxEXT) -1)
    _procs_GL_EXT_histogram.glResetMinmaxEXT =
      (GdkGLProc_glResetMinmaxEXT) gdk_gl_get_proc_address ("glResetMinmaxEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glResetMinmaxEXT () - %s",
               (_procs_GL_EXT_histogram.glResetMinmaxEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_histogram.glResetMinmaxEXT);
}

/* Get GL_EXT_histogram functions */
GdkGL_GL_EXT_histogram *
gdk_gl_get_GL_EXT_histogram (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_histogram");

      if (supported)
        {
          supported &= (gdk_gl_get_glGetHistogramEXT () != NULL);
          supported &= (gdk_gl_get_glGetHistogramParameterfvEXT () != NULL);
          supported &= (gdk_gl_get_glGetHistogramParameterivEXT () != NULL);
          supported &= (gdk_gl_get_glGetMinmaxEXT () != NULL);
          supported &= (gdk_gl_get_glGetMinmaxParameterfvEXT () != NULL);
          supported &= (gdk_gl_get_glGetMinmaxParameterivEXT () != NULL);
          supported &= (gdk_gl_get_glHistogramEXT () != NULL);
          supported &= (gdk_gl_get_glMinmaxEXT () != NULL);
          supported &= (gdk_gl_get_glResetHistogramEXT () != NULL);
          supported &= (gdk_gl_get_glResetMinmaxEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_histogram () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_histogram;
}

/*
 * GL_EXT_convolution
 */

static GdkGL_GL_EXT_convolution _procs_GL_EXT_convolution = {
  (GdkGLProc_glConvolutionFilter1DEXT) -1,
  (GdkGLProc_glConvolutionFilter2DEXT) -1,
  (GdkGLProc_glConvolutionParameterfEXT) -1,
  (GdkGLProc_glConvolutionParameterfvEXT) -1,
  (GdkGLProc_glConvolutionParameteriEXT) -1,
  (GdkGLProc_glConvolutionParameterivEXT) -1,
  (GdkGLProc_glCopyConvolutionFilter1DEXT) -1,
  (GdkGLProc_glCopyConvolutionFilter2DEXT) -1,
  (GdkGLProc_glGetConvolutionFilterEXT) -1,
  (GdkGLProc_glGetConvolutionParameterfvEXT) -1,
  (GdkGLProc_glGetConvolutionParameterivEXT) -1,
  (GdkGLProc_glGetSeparableFilterEXT) -1,
  (GdkGLProc_glSeparableFilter2DEXT) -1
};

/* glConvolutionFilter1DEXT */
GdkGLProc
gdk_gl_get_glConvolutionFilter1DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glConvolutionFilter1DEXT == (GdkGLProc_glConvolutionFilter1DEXT) -1)
    _procs_GL_EXT_convolution.glConvolutionFilter1DEXT =
      (GdkGLProc_glConvolutionFilter1DEXT) gdk_gl_get_proc_address ("glConvolutionFilter1DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionFilter1DEXT () - %s",
               (_procs_GL_EXT_convolution.glConvolutionFilter1DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glConvolutionFilter1DEXT);
}

/* glConvolutionFilter2DEXT */
GdkGLProc
gdk_gl_get_glConvolutionFilter2DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glConvolutionFilter2DEXT == (GdkGLProc_glConvolutionFilter2DEXT) -1)
    _procs_GL_EXT_convolution.glConvolutionFilter2DEXT =
      (GdkGLProc_glConvolutionFilter2DEXT) gdk_gl_get_proc_address ("glConvolutionFilter2DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionFilter2DEXT () - %s",
               (_procs_GL_EXT_convolution.glConvolutionFilter2DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glConvolutionFilter2DEXT);
}

/* glConvolutionParameterfEXT */
GdkGLProc
gdk_gl_get_glConvolutionParameterfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glConvolutionParameterfEXT == (GdkGLProc_glConvolutionParameterfEXT) -1)
    _procs_GL_EXT_convolution.glConvolutionParameterfEXT =
      (GdkGLProc_glConvolutionParameterfEXT) gdk_gl_get_proc_address ("glConvolutionParameterfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameterfEXT () - %s",
               (_procs_GL_EXT_convolution.glConvolutionParameterfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glConvolutionParameterfEXT);
}

/* glConvolutionParameterfvEXT */
GdkGLProc
gdk_gl_get_glConvolutionParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glConvolutionParameterfvEXT == (GdkGLProc_glConvolutionParameterfvEXT) -1)
    _procs_GL_EXT_convolution.glConvolutionParameterfvEXT =
      (GdkGLProc_glConvolutionParameterfvEXT) gdk_gl_get_proc_address ("glConvolutionParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameterfvEXT () - %s",
               (_procs_GL_EXT_convolution.glConvolutionParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glConvolutionParameterfvEXT);
}

/* glConvolutionParameteriEXT */
GdkGLProc
gdk_gl_get_glConvolutionParameteriEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glConvolutionParameteriEXT == (GdkGLProc_glConvolutionParameteriEXT) -1)
    _procs_GL_EXT_convolution.glConvolutionParameteriEXT =
      (GdkGLProc_glConvolutionParameteriEXT) gdk_gl_get_proc_address ("glConvolutionParameteriEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameteriEXT () - %s",
               (_procs_GL_EXT_convolution.glConvolutionParameteriEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glConvolutionParameteriEXT);
}

/* glConvolutionParameterivEXT */
GdkGLProc
gdk_gl_get_glConvolutionParameterivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glConvolutionParameterivEXT == (GdkGLProc_glConvolutionParameterivEXT) -1)
    _procs_GL_EXT_convolution.glConvolutionParameterivEXT =
      (GdkGLProc_glConvolutionParameterivEXT) gdk_gl_get_proc_address ("glConvolutionParameterivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glConvolutionParameterivEXT () - %s",
               (_procs_GL_EXT_convolution.glConvolutionParameterivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glConvolutionParameterivEXT);
}

/* glCopyConvolutionFilter1DEXT */
GdkGLProc
gdk_gl_get_glCopyConvolutionFilter1DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glCopyConvolutionFilter1DEXT == (GdkGLProc_glCopyConvolutionFilter1DEXT) -1)
    _procs_GL_EXT_convolution.glCopyConvolutionFilter1DEXT =
      (GdkGLProc_glCopyConvolutionFilter1DEXT) gdk_gl_get_proc_address ("glCopyConvolutionFilter1DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyConvolutionFilter1DEXT () - %s",
               (_procs_GL_EXT_convolution.glCopyConvolutionFilter1DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glCopyConvolutionFilter1DEXT);
}

/* glCopyConvolutionFilter2DEXT */
GdkGLProc
gdk_gl_get_glCopyConvolutionFilter2DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glCopyConvolutionFilter2DEXT == (GdkGLProc_glCopyConvolutionFilter2DEXT) -1)
    _procs_GL_EXT_convolution.glCopyConvolutionFilter2DEXT =
      (GdkGLProc_glCopyConvolutionFilter2DEXT) gdk_gl_get_proc_address ("glCopyConvolutionFilter2DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyConvolutionFilter2DEXT () - %s",
               (_procs_GL_EXT_convolution.glCopyConvolutionFilter2DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glCopyConvolutionFilter2DEXT);
}

/* glGetConvolutionFilterEXT */
GdkGLProc
gdk_gl_get_glGetConvolutionFilterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glGetConvolutionFilterEXT == (GdkGLProc_glGetConvolutionFilterEXT) -1)
    _procs_GL_EXT_convolution.glGetConvolutionFilterEXT =
      (GdkGLProc_glGetConvolutionFilterEXT) gdk_gl_get_proc_address ("glGetConvolutionFilterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetConvolutionFilterEXT () - %s",
               (_procs_GL_EXT_convolution.glGetConvolutionFilterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glGetConvolutionFilterEXT);
}

/* glGetConvolutionParameterfvEXT */
GdkGLProc
gdk_gl_get_glGetConvolutionParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glGetConvolutionParameterfvEXT == (GdkGLProc_glGetConvolutionParameterfvEXT) -1)
    _procs_GL_EXT_convolution.glGetConvolutionParameterfvEXT =
      (GdkGLProc_glGetConvolutionParameterfvEXT) gdk_gl_get_proc_address ("glGetConvolutionParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetConvolutionParameterfvEXT () - %s",
               (_procs_GL_EXT_convolution.glGetConvolutionParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glGetConvolutionParameterfvEXT);
}

/* glGetConvolutionParameterivEXT */
GdkGLProc
gdk_gl_get_glGetConvolutionParameterivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glGetConvolutionParameterivEXT == (GdkGLProc_glGetConvolutionParameterivEXT) -1)
    _procs_GL_EXT_convolution.glGetConvolutionParameterivEXT =
      (GdkGLProc_glGetConvolutionParameterivEXT) gdk_gl_get_proc_address ("glGetConvolutionParameterivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetConvolutionParameterivEXT () - %s",
               (_procs_GL_EXT_convolution.glGetConvolutionParameterivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glGetConvolutionParameterivEXT);
}

/* glGetSeparableFilterEXT */
GdkGLProc
gdk_gl_get_glGetSeparableFilterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glGetSeparableFilterEXT == (GdkGLProc_glGetSeparableFilterEXT) -1)
    _procs_GL_EXT_convolution.glGetSeparableFilterEXT =
      (GdkGLProc_glGetSeparableFilterEXT) gdk_gl_get_proc_address ("glGetSeparableFilterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetSeparableFilterEXT () - %s",
               (_procs_GL_EXT_convolution.glGetSeparableFilterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glGetSeparableFilterEXT);
}

/* glSeparableFilter2DEXT */
GdkGLProc
gdk_gl_get_glSeparableFilter2DEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_convolution.glSeparableFilter2DEXT == (GdkGLProc_glSeparableFilter2DEXT) -1)
    _procs_GL_EXT_convolution.glSeparableFilter2DEXT =
      (GdkGLProc_glSeparableFilter2DEXT) gdk_gl_get_proc_address ("glSeparableFilter2DEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSeparableFilter2DEXT () - %s",
               (_procs_GL_EXT_convolution.glSeparableFilter2DEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_convolution.glSeparableFilter2DEXT);
}

/* Get GL_EXT_convolution functions */
GdkGL_GL_EXT_convolution *
gdk_gl_get_GL_EXT_convolution (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_convolution");

      if (supported)
        {
          supported &= (gdk_gl_get_glConvolutionFilter1DEXT () != NULL);
          supported &= (gdk_gl_get_glConvolutionFilter2DEXT () != NULL);
          supported &= (gdk_gl_get_glConvolutionParameterfEXT () != NULL);
          supported &= (gdk_gl_get_glConvolutionParameterfvEXT () != NULL);
          supported &= (gdk_gl_get_glConvolutionParameteriEXT () != NULL);
          supported &= (gdk_gl_get_glConvolutionParameterivEXT () != NULL);
          supported &= (gdk_gl_get_glCopyConvolutionFilter1DEXT () != NULL);
          supported &= (gdk_gl_get_glCopyConvolutionFilter2DEXT () != NULL);
          supported &= (gdk_gl_get_glGetConvolutionFilterEXT () != NULL);
          supported &= (gdk_gl_get_glGetConvolutionParameterfvEXT () != NULL);
          supported &= (gdk_gl_get_glGetConvolutionParameterivEXT () != NULL);
          supported &= (gdk_gl_get_glGetSeparableFilterEXT () != NULL);
          supported &= (gdk_gl_get_glSeparableFilter2DEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_convolution () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_convolution;
}

/*
 * GL_SGI_color_table
 */

static GdkGL_GL_SGI_color_table _procs_GL_SGI_color_table = {
  (GdkGLProc_glColorTableSGI) -1,
  (GdkGLProc_glColorTableParameterfvSGI) -1,
  (GdkGLProc_glColorTableParameterivSGI) -1,
  (GdkGLProc_glCopyColorTableSGI) -1,
  (GdkGLProc_glGetColorTableSGI) -1,
  (GdkGLProc_glGetColorTableParameterfvSGI) -1,
  (GdkGLProc_glGetColorTableParameterivSGI) -1
};

/* glColorTableSGI */
GdkGLProc
gdk_gl_get_glColorTableSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glColorTableSGI == (GdkGLProc_glColorTableSGI) -1)
    _procs_GL_SGI_color_table.glColorTableSGI =
      (GdkGLProc_glColorTableSGI) gdk_gl_get_proc_address ("glColorTableSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTableSGI () - %s",
               (_procs_GL_SGI_color_table.glColorTableSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glColorTableSGI);
}

/* glColorTableParameterfvSGI */
GdkGLProc
gdk_gl_get_glColorTableParameterfvSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glColorTableParameterfvSGI == (GdkGLProc_glColorTableParameterfvSGI) -1)
    _procs_GL_SGI_color_table.glColorTableParameterfvSGI =
      (GdkGLProc_glColorTableParameterfvSGI) gdk_gl_get_proc_address ("glColorTableParameterfvSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTableParameterfvSGI () - %s",
               (_procs_GL_SGI_color_table.glColorTableParameterfvSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glColorTableParameterfvSGI);
}

/* glColorTableParameterivSGI */
GdkGLProc
gdk_gl_get_glColorTableParameterivSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glColorTableParameterivSGI == (GdkGLProc_glColorTableParameterivSGI) -1)
    _procs_GL_SGI_color_table.glColorTableParameterivSGI =
      (GdkGLProc_glColorTableParameterivSGI) gdk_gl_get_proc_address ("glColorTableParameterivSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTableParameterivSGI () - %s",
               (_procs_GL_SGI_color_table.glColorTableParameterivSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glColorTableParameterivSGI);
}

/* glCopyColorTableSGI */
GdkGLProc
gdk_gl_get_glCopyColorTableSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glCopyColorTableSGI == (GdkGLProc_glCopyColorTableSGI) -1)
    _procs_GL_SGI_color_table.glCopyColorTableSGI =
      (GdkGLProc_glCopyColorTableSGI) gdk_gl_get_proc_address ("glCopyColorTableSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyColorTableSGI () - %s",
               (_procs_GL_SGI_color_table.glCopyColorTableSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glCopyColorTableSGI);
}

/* glGetColorTableSGI */
GdkGLProc
gdk_gl_get_glGetColorTableSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glGetColorTableSGI == (GdkGLProc_glGetColorTableSGI) -1)
    _procs_GL_SGI_color_table.glGetColorTableSGI =
      (GdkGLProc_glGetColorTableSGI) gdk_gl_get_proc_address ("glGetColorTableSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableSGI () - %s",
               (_procs_GL_SGI_color_table.glGetColorTableSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glGetColorTableSGI);
}

/* glGetColorTableParameterfvSGI */
GdkGLProc
gdk_gl_get_glGetColorTableParameterfvSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glGetColorTableParameterfvSGI == (GdkGLProc_glGetColorTableParameterfvSGI) -1)
    _procs_GL_SGI_color_table.glGetColorTableParameterfvSGI =
      (GdkGLProc_glGetColorTableParameterfvSGI) gdk_gl_get_proc_address ("glGetColorTableParameterfvSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableParameterfvSGI () - %s",
               (_procs_GL_SGI_color_table.glGetColorTableParameterfvSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glGetColorTableParameterfvSGI);
}

/* glGetColorTableParameterivSGI */
GdkGLProc
gdk_gl_get_glGetColorTableParameterivSGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGI_color_table.glGetColorTableParameterivSGI == (GdkGLProc_glGetColorTableParameterivSGI) -1)
    _procs_GL_SGI_color_table.glGetColorTableParameterivSGI =
      (GdkGLProc_glGetColorTableParameterivSGI) gdk_gl_get_proc_address ("glGetColorTableParameterivSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableParameterivSGI () - %s",
               (_procs_GL_SGI_color_table.glGetColorTableParameterivSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGI_color_table.glGetColorTableParameterivSGI);
}

/* Get GL_SGI_color_table functions */
GdkGL_GL_SGI_color_table *
gdk_gl_get_GL_SGI_color_table (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGI_color_table");

      if (supported)
        {
          supported &= (gdk_gl_get_glColorTableSGI () != NULL);
          supported &= (gdk_gl_get_glColorTableParameterfvSGI () != NULL);
          supported &= (gdk_gl_get_glColorTableParameterivSGI () != NULL);
          supported &= (gdk_gl_get_glCopyColorTableSGI () != NULL);
          supported &= (gdk_gl_get_glGetColorTableSGI () != NULL);
          supported &= (gdk_gl_get_glGetColorTableParameterfvSGI () != NULL);
          supported &= (gdk_gl_get_glGetColorTableParameterivSGI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGI_color_table () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGI_color_table;
}

/*
 * GL_SGIX_pixel_texture
 */

static GdkGL_GL_SGIX_pixel_texture _procs_GL_SGIX_pixel_texture = {
  (GdkGLProc_glPixelTexGenSGIX) -1
};

/* glPixelTexGenSGIX */
GdkGLProc
gdk_gl_get_glPixelTexGenSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_pixel_texture.glPixelTexGenSGIX == (GdkGLProc_glPixelTexGenSGIX) -1)
    _procs_GL_SGIX_pixel_texture.glPixelTexGenSGIX =
      (GdkGLProc_glPixelTexGenSGIX) gdk_gl_get_proc_address ("glPixelTexGenSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTexGenSGIX () - %s",
               (_procs_GL_SGIX_pixel_texture.glPixelTexGenSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_pixel_texture.glPixelTexGenSGIX);
}

/* Get GL_SGIX_pixel_texture functions */
GdkGL_GL_SGIX_pixel_texture *
gdk_gl_get_GL_SGIX_pixel_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_pixel_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glPixelTexGenSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_pixel_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_pixel_texture;
}

/*
 * GL_SGIS_pixel_texture
 */

static GdkGL_GL_SGIS_pixel_texture _procs_GL_SGIS_pixel_texture = {
  (GdkGLProc_glPixelTexGenParameteriSGIS) -1,
  (GdkGLProc_glPixelTexGenParameterivSGIS) -1,
  (GdkGLProc_glPixelTexGenParameterfSGIS) -1,
  (GdkGLProc_glPixelTexGenParameterfvSGIS) -1,
  (GdkGLProc_glGetPixelTexGenParameterivSGIS) -1,
  (GdkGLProc_glGetPixelTexGenParameterfvSGIS) -1
};

/* glPixelTexGenParameteriSGIS */
GdkGLProc
gdk_gl_get_glPixelTexGenParameteriSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameteriSGIS == (GdkGLProc_glPixelTexGenParameteriSGIS) -1)
    _procs_GL_SGIS_pixel_texture.glPixelTexGenParameteriSGIS =
      (GdkGLProc_glPixelTexGenParameteriSGIS) gdk_gl_get_proc_address ("glPixelTexGenParameteriSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTexGenParameteriSGIS () - %s",
               (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameteriSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameteriSGIS);
}

/* glPixelTexGenParameterivSGIS */
GdkGLProc
gdk_gl_get_glPixelTexGenParameterivSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterivSGIS == (GdkGLProc_glPixelTexGenParameterivSGIS) -1)
    _procs_GL_SGIS_pixel_texture.glPixelTexGenParameterivSGIS =
      (GdkGLProc_glPixelTexGenParameterivSGIS) gdk_gl_get_proc_address ("glPixelTexGenParameterivSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTexGenParameterivSGIS () - %s",
               (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterivSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterivSGIS);
}

/* glPixelTexGenParameterfSGIS */
GdkGLProc
gdk_gl_get_glPixelTexGenParameterfSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfSGIS == (GdkGLProc_glPixelTexGenParameterfSGIS) -1)
    _procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfSGIS =
      (GdkGLProc_glPixelTexGenParameterfSGIS) gdk_gl_get_proc_address ("glPixelTexGenParameterfSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTexGenParameterfSGIS () - %s",
               (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfSGIS);
}

/* glPixelTexGenParameterfvSGIS */
GdkGLProc
gdk_gl_get_glPixelTexGenParameterfvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfvSGIS == (GdkGLProc_glPixelTexGenParameterfvSGIS) -1)
    _procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfvSGIS =
      (GdkGLProc_glPixelTexGenParameterfvSGIS) gdk_gl_get_proc_address ("glPixelTexGenParameterfvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTexGenParameterfvSGIS () - %s",
               (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_pixel_texture.glPixelTexGenParameterfvSGIS);
}

/* glGetPixelTexGenParameterivSGIS */
GdkGLProc
gdk_gl_get_glGetPixelTexGenParameterivSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterivSGIS == (GdkGLProc_glGetPixelTexGenParameterivSGIS) -1)
    _procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterivSGIS =
      (GdkGLProc_glGetPixelTexGenParameterivSGIS) gdk_gl_get_proc_address ("glGetPixelTexGenParameterivSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetPixelTexGenParameterivSGIS () - %s",
               (_procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterivSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterivSGIS);
}

/* glGetPixelTexGenParameterfvSGIS */
GdkGLProc
gdk_gl_get_glGetPixelTexGenParameterfvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterfvSGIS == (GdkGLProc_glGetPixelTexGenParameterfvSGIS) -1)
    _procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterfvSGIS =
      (GdkGLProc_glGetPixelTexGenParameterfvSGIS) gdk_gl_get_proc_address ("glGetPixelTexGenParameterfvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetPixelTexGenParameterfvSGIS () - %s",
               (_procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterfvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_pixel_texture.glGetPixelTexGenParameterfvSGIS);
}

/* Get GL_SGIS_pixel_texture functions */
GdkGL_GL_SGIS_pixel_texture *
gdk_gl_get_GL_SGIS_pixel_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_pixel_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glPixelTexGenParameteriSGIS () != NULL);
          supported &= (gdk_gl_get_glPixelTexGenParameterivSGIS () != NULL);
          supported &= (gdk_gl_get_glPixelTexGenParameterfSGIS () != NULL);
          supported &= (gdk_gl_get_glPixelTexGenParameterfvSGIS () != NULL);
          supported &= (gdk_gl_get_glGetPixelTexGenParameterivSGIS () != NULL);
          supported &= (gdk_gl_get_glGetPixelTexGenParameterfvSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_pixel_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_pixel_texture;
}

/*
 * GL_SGIS_texture4D
 */

static GdkGL_GL_SGIS_texture4D _procs_GL_SGIS_texture4D = {
  (GdkGLProc_glTexImage4DSGIS) -1,
  (GdkGLProc_glTexSubImage4DSGIS) -1
};

/* glTexImage4DSGIS */
GdkGLProc
gdk_gl_get_glTexImage4DSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_texture4D.glTexImage4DSGIS == (GdkGLProc_glTexImage4DSGIS) -1)
    _procs_GL_SGIS_texture4D.glTexImage4DSGIS =
      (GdkGLProc_glTexImage4DSGIS) gdk_gl_get_proc_address ("glTexImage4DSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexImage4DSGIS () - %s",
               (_procs_GL_SGIS_texture4D.glTexImage4DSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_texture4D.glTexImage4DSGIS);
}

/* glTexSubImage4DSGIS */
GdkGLProc
gdk_gl_get_glTexSubImage4DSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_texture4D.glTexSubImage4DSGIS == (GdkGLProc_glTexSubImage4DSGIS) -1)
    _procs_GL_SGIS_texture4D.glTexSubImage4DSGIS =
      (GdkGLProc_glTexSubImage4DSGIS) gdk_gl_get_proc_address ("glTexSubImage4DSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexSubImage4DSGIS () - %s",
               (_procs_GL_SGIS_texture4D.glTexSubImage4DSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_texture4D.glTexSubImage4DSGIS);
}

/* Get GL_SGIS_texture4D functions */
GdkGL_GL_SGIS_texture4D *
gdk_gl_get_GL_SGIS_texture4D (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_texture4D");

      if (supported)
        {
          supported &= (gdk_gl_get_glTexImage4DSGIS () != NULL);
          supported &= (gdk_gl_get_glTexSubImage4DSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_texture4D () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_texture4D;
}

/*
 * GL_EXT_texture_object
 */

static GdkGL_GL_EXT_texture_object _procs_GL_EXT_texture_object = {
  (GdkGLProc_glAreTexturesResidentEXT) -1,
  (GdkGLProc_glBindTextureEXT) -1,
  (GdkGLProc_glDeleteTexturesEXT) -1,
  (GdkGLProc_glGenTexturesEXT) -1,
  (GdkGLProc_glIsTextureEXT) -1,
  (GdkGLProc_glPrioritizeTexturesEXT) -1
};

/* glAreTexturesResidentEXT */
GdkGLProc
gdk_gl_get_glAreTexturesResidentEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_object.glAreTexturesResidentEXT == (GdkGLProc_glAreTexturesResidentEXT) -1)
    _procs_GL_EXT_texture_object.glAreTexturesResidentEXT =
      (GdkGLProc_glAreTexturesResidentEXT) gdk_gl_get_proc_address ("glAreTexturesResidentEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAreTexturesResidentEXT () - %s",
               (_procs_GL_EXT_texture_object.glAreTexturesResidentEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_object.glAreTexturesResidentEXT);
}

/* glBindTextureEXT */
GdkGLProc
gdk_gl_get_glBindTextureEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_object.glBindTextureEXT == (GdkGLProc_glBindTextureEXT) -1)
    _procs_GL_EXT_texture_object.glBindTextureEXT =
      (GdkGLProc_glBindTextureEXT) gdk_gl_get_proc_address ("glBindTextureEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindTextureEXT () - %s",
               (_procs_GL_EXT_texture_object.glBindTextureEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_object.glBindTextureEXT);
}

/* glDeleteTexturesEXT */
GdkGLProc
gdk_gl_get_glDeleteTexturesEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_object.glDeleteTexturesEXT == (GdkGLProc_glDeleteTexturesEXT) -1)
    _procs_GL_EXT_texture_object.glDeleteTexturesEXT =
      (GdkGLProc_glDeleteTexturesEXT) gdk_gl_get_proc_address ("glDeleteTexturesEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteTexturesEXT () - %s",
               (_procs_GL_EXT_texture_object.glDeleteTexturesEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_object.glDeleteTexturesEXT);
}

/* glGenTexturesEXT */
GdkGLProc
gdk_gl_get_glGenTexturesEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_object.glGenTexturesEXT == (GdkGLProc_glGenTexturesEXT) -1)
    _procs_GL_EXT_texture_object.glGenTexturesEXT =
      (GdkGLProc_glGenTexturesEXT) gdk_gl_get_proc_address ("glGenTexturesEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenTexturesEXT () - %s",
               (_procs_GL_EXT_texture_object.glGenTexturesEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_object.glGenTexturesEXT);
}

/* glIsTextureEXT */
GdkGLProc
gdk_gl_get_glIsTextureEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_object.glIsTextureEXT == (GdkGLProc_glIsTextureEXT) -1)
    _procs_GL_EXT_texture_object.glIsTextureEXT =
      (GdkGLProc_glIsTextureEXT) gdk_gl_get_proc_address ("glIsTextureEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsTextureEXT () - %s",
               (_procs_GL_EXT_texture_object.glIsTextureEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_object.glIsTextureEXT);
}

/* glPrioritizeTexturesEXT */
GdkGLProc
gdk_gl_get_glPrioritizeTexturesEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_object.glPrioritizeTexturesEXT == (GdkGLProc_glPrioritizeTexturesEXT) -1)
    _procs_GL_EXT_texture_object.glPrioritizeTexturesEXT =
      (GdkGLProc_glPrioritizeTexturesEXT) gdk_gl_get_proc_address ("glPrioritizeTexturesEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPrioritizeTexturesEXT () - %s",
               (_procs_GL_EXT_texture_object.glPrioritizeTexturesEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_object.glPrioritizeTexturesEXT);
}

/* Get GL_EXT_texture_object functions */
GdkGL_GL_EXT_texture_object *
gdk_gl_get_GL_EXT_texture_object (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_texture_object");

      if (supported)
        {
          supported &= (gdk_gl_get_glAreTexturesResidentEXT () != NULL);
          supported &= (gdk_gl_get_glBindTextureEXT () != NULL);
          supported &= (gdk_gl_get_glDeleteTexturesEXT () != NULL);
          supported &= (gdk_gl_get_glGenTexturesEXT () != NULL);
          supported &= (gdk_gl_get_glIsTextureEXT () != NULL);
          supported &= (gdk_gl_get_glPrioritizeTexturesEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_texture_object () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_texture_object;
}

/*
 * GL_SGIS_detail_texture
 */

static GdkGL_GL_SGIS_detail_texture _procs_GL_SGIS_detail_texture = {
  (GdkGLProc_glDetailTexFuncSGIS) -1,
  (GdkGLProc_glGetDetailTexFuncSGIS) -1
};

/* glDetailTexFuncSGIS */
GdkGLProc
gdk_gl_get_glDetailTexFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_detail_texture.glDetailTexFuncSGIS == (GdkGLProc_glDetailTexFuncSGIS) -1)
    _procs_GL_SGIS_detail_texture.glDetailTexFuncSGIS =
      (GdkGLProc_glDetailTexFuncSGIS) gdk_gl_get_proc_address ("glDetailTexFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDetailTexFuncSGIS () - %s",
               (_procs_GL_SGIS_detail_texture.glDetailTexFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_detail_texture.glDetailTexFuncSGIS);
}

/* glGetDetailTexFuncSGIS */
GdkGLProc
gdk_gl_get_glGetDetailTexFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_detail_texture.glGetDetailTexFuncSGIS == (GdkGLProc_glGetDetailTexFuncSGIS) -1)
    _procs_GL_SGIS_detail_texture.glGetDetailTexFuncSGIS =
      (GdkGLProc_glGetDetailTexFuncSGIS) gdk_gl_get_proc_address ("glGetDetailTexFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetDetailTexFuncSGIS () - %s",
               (_procs_GL_SGIS_detail_texture.glGetDetailTexFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_detail_texture.glGetDetailTexFuncSGIS);
}

/* Get GL_SGIS_detail_texture functions */
GdkGL_GL_SGIS_detail_texture *
gdk_gl_get_GL_SGIS_detail_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_detail_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glDetailTexFuncSGIS () != NULL);
          supported &= (gdk_gl_get_glGetDetailTexFuncSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_detail_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_detail_texture;
}

/*
 * GL_SGIS_sharpen_texture
 */

static GdkGL_GL_SGIS_sharpen_texture _procs_GL_SGIS_sharpen_texture = {
  (GdkGLProc_glSharpenTexFuncSGIS) -1,
  (GdkGLProc_glGetSharpenTexFuncSGIS) -1
};

/* glSharpenTexFuncSGIS */
GdkGLProc
gdk_gl_get_glSharpenTexFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_sharpen_texture.glSharpenTexFuncSGIS == (GdkGLProc_glSharpenTexFuncSGIS) -1)
    _procs_GL_SGIS_sharpen_texture.glSharpenTexFuncSGIS =
      (GdkGLProc_glSharpenTexFuncSGIS) gdk_gl_get_proc_address ("glSharpenTexFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSharpenTexFuncSGIS () - %s",
               (_procs_GL_SGIS_sharpen_texture.glSharpenTexFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_sharpen_texture.glSharpenTexFuncSGIS);
}

/* glGetSharpenTexFuncSGIS */
GdkGLProc
gdk_gl_get_glGetSharpenTexFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_sharpen_texture.glGetSharpenTexFuncSGIS == (GdkGLProc_glGetSharpenTexFuncSGIS) -1)
    _procs_GL_SGIS_sharpen_texture.glGetSharpenTexFuncSGIS =
      (GdkGLProc_glGetSharpenTexFuncSGIS) gdk_gl_get_proc_address ("glGetSharpenTexFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetSharpenTexFuncSGIS () - %s",
               (_procs_GL_SGIS_sharpen_texture.glGetSharpenTexFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_sharpen_texture.glGetSharpenTexFuncSGIS);
}

/* Get GL_SGIS_sharpen_texture functions */
GdkGL_GL_SGIS_sharpen_texture *
gdk_gl_get_GL_SGIS_sharpen_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_sharpen_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glSharpenTexFuncSGIS () != NULL);
          supported &= (gdk_gl_get_glGetSharpenTexFuncSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_sharpen_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_sharpen_texture;
}

/*
 * GL_SGIS_multisample
 */

static GdkGL_GL_SGIS_multisample _procs_GL_SGIS_multisample = {
  (GdkGLProc_glSampleMaskSGIS) -1,
  (GdkGLProc_glSamplePatternSGIS) -1
};

/* glSampleMaskSGIS */
GdkGLProc
gdk_gl_get_glSampleMaskSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multisample.glSampleMaskSGIS == (GdkGLProc_glSampleMaskSGIS) -1)
    _procs_GL_SGIS_multisample.glSampleMaskSGIS =
      (GdkGLProc_glSampleMaskSGIS) gdk_gl_get_proc_address ("glSampleMaskSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSampleMaskSGIS () - %s",
               (_procs_GL_SGIS_multisample.glSampleMaskSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multisample.glSampleMaskSGIS);
}

/* glSamplePatternSGIS */
GdkGLProc
gdk_gl_get_glSamplePatternSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multisample.glSamplePatternSGIS == (GdkGLProc_glSamplePatternSGIS) -1)
    _procs_GL_SGIS_multisample.glSamplePatternSGIS =
      (GdkGLProc_glSamplePatternSGIS) gdk_gl_get_proc_address ("glSamplePatternSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSamplePatternSGIS () - %s",
               (_procs_GL_SGIS_multisample.glSamplePatternSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multisample.glSamplePatternSGIS);
}

/* Get GL_SGIS_multisample functions */
GdkGL_GL_SGIS_multisample *
gdk_gl_get_GL_SGIS_multisample (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_multisample");

      if (supported)
        {
          supported &= (gdk_gl_get_glSampleMaskSGIS () != NULL);
          supported &= (gdk_gl_get_glSamplePatternSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_multisample () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_multisample;
}

/*
 * GL_EXT_vertex_array
 */

static GdkGL_GL_EXT_vertex_array _procs_GL_EXT_vertex_array = {
  (GdkGLProc_glArrayElementEXT) -1,
  (GdkGLProc_glColorPointerEXT) -1,
  (GdkGLProc_glDrawArraysEXT) -1,
  (GdkGLProc_glEdgeFlagPointerEXT) -1,
  (GdkGLProc_glGetPointervEXT) -1,
  (GdkGLProc_glIndexPointerEXT) -1,
  (GdkGLProc_glNormalPointerEXT) -1,
  (GdkGLProc_glTexCoordPointerEXT) -1,
  (GdkGLProc_glVertexPointerEXT) -1
};

/* glArrayElementEXT */
GdkGLProc
gdk_gl_get_glArrayElementEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glArrayElementEXT == (GdkGLProc_glArrayElementEXT) -1)
    _procs_GL_EXT_vertex_array.glArrayElementEXT =
      (GdkGLProc_glArrayElementEXT) gdk_gl_get_proc_address ("glArrayElementEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glArrayElementEXT () - %s",
               (_procs_GL_EXT_vertex_array.glArrayElementEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glArrayElementEXT);
}

/* glColorPointerEXT */
GdkGLProc
gdk_gl_get_glColorPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glColorPointerEXT == (GdkGLProc_glColorPointerEXT) -1)
    _procs_GL_EXT_vertex_array.glColorPointerEXT =
      (GdkGLProc_glColorPointerEXT) gdk_gl_get_proc_address ("glColorPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorPointerEXT () - %s",
               (_procs_GL_EXT_vertex_array.glColorPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glColorPointerEXT);
}

/* glDrawArraysEXT */
GdkGLProc
gdk_gl_get_glDrawArraysEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glDrawArraysEXT == (GdkGLProc_glDrawArraysEXT) -1)
    _procs_GL_EXT_vertex_array.glDrawArraysEXT =
      (GdkGLProc_glDrawArraysEXT) gdk_gl_get_proc_address ("glDrawArraysEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawArraysEXT () - %s",
               (_procs_GL_EXT_vertex_array.glDrawArraysEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glDrawArraysEXT);
}

/* glEdgeFlagPointerEXT */
GdkGLProc
gdk_gl_get_glEdgeFlagPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glEdgeFlagPointerEXT == (GdkGLProc_glEdgeFlagPointerEXT) -1)
    _procs_GL_EXT_vertex_array.glEdgeFlagPointerEXT =
      (GdkGLProc_glEdgeFlagPointerEXT) gdk_gl_get_proc_address ("glEdgeFlagPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEdgeFlagPointerEXT () - %s",
               (_procs_GL_EXT_vertex_array.glEdgeFlagPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glEdgeFlagPointerEXT);
}

/* glGetPointervEXT */
GdkGLProc
gdk_gl_get_glGetPointervEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glGetPointervEXT == (GdkGLProc_glGetPointervEXT) -1)
    _procs_GL_EXT_vertex_array.glGetPointervEXT =
      (GdkGLProc_glGetPointervEXT) gdk_gl_get_proc_address ("glGetPointervEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetPointervEXT () - %s",
               (_procs_GL_EXT_vertex_array.glGetPointervEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glGetPointervEXT);
}

/* glIndexPointerEXT */
GdkGLProc
gdk_gl_get_glIndexPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glIndexPointerEXT == (GdkGLProc_glIndexPointerEXT) -1)
    _procs_GL_EXT_vertex_array.glIndexPointerEXT =
      (GdkGLProc_glIndexPointerEXT) gdk_gl_get_proc_address ("glIndexPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIndexPointerEXT () - %s",
               (_procs_GL_EXT_vertex_array.glIndexPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glIndexPointerEXT);
}

/* glNormalPointerEXT */
GdkGLProc
gdk_gl_get_glNormalPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glNormalPointerEXT == (GdkGLProc_glNormalPointerEXT) -1)
    _procs_GL_EXT_vertex_array.glNormalPointerEXT =
      (GdkGLProc_glNormalPointerEXT) gdk_gl_get_proc_address ("glNormalPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalPointerEXT () - %s",
               (_procs_GL_EXT_vertex_array.glNormalPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glNormalPointerEXT);
}

/* glTexCoordPointerEXT */
GdkGLProc
gdk_gl_get_glTexCoordPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glTexCoordPointerEXT == (GdkGLProc_glTexCoordPointerEXT) -1)
    _procs_GL_EXT_vertex_array.glTexCoordPointerEXT =
      (GdkGLProc_glTexCoordPointerEXT) gdk_gl_get_proc_address ("glTexCoordPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoordPointerEXT () - %s",
               (_procs_GL_EXT_vertex_array.glTexCoordPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glTexCoordPointerEXT);
}

/* glVertexPointerEXT */
GdkGLProc
gdk_gl_get_glVertexPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_array.glVertexPointerEXT == (GdkGLProc_glVertexPointerEXT) -1)
    _procs_GL_EXT_vertex_array.glVertexPointerEXT =
      (GdkGLProc_glVertexPointerEXT) gdk_gl_get_proc_address ("glVertexPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexPointerEXT () - %s",
               (_procs_GL_EXT_vertex_array.glVertexPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_array.glVertexPointerEXT);
}

/* Get GL_EXT_vertex_array functions */
GdkGL_GL_EXT_vertex_array *
gdk_gl_get_GL_EXT_vertex_array (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_vertex_array");

      if (supported)
        {
          supported &= (gdk_gl_get_glArrayElementEXT () != NULL);
          supported &= (gdk_gl_get_glColorPointerEXT () != NULL);
          supported &= (gdk_gl_get_glDrawArraysEXT () != NULL);
          supported &= (gdk_gl_get_glEdgeFlagPointerEXT () != NULL);
          supported &= (gdk_gl_get_glGetPointervEXT () != NULL);
          supported &= (gdk_gl_get_glIndexPointerEXT () != NULL);
          supported &= (gdk_gl_get_glNormalPointerEXT () != NULL);
          supported &= (gdk_gl_get_glTexCoordPointerEXT () != NULL);
          supported &= (gdk_gl_get_glVertexPointerEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_vertex_array () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_vertex_array;
}

/*
 * GL_EXT_blend_minmax
 */

static GdkGL_GL_EXT_blend_minmax _procs_GL_EXT_blend_minmax = {
  (GdkGLProc_glBlendEquationEXT) -1
};

/* glBlendEquationEXT */
GdkGLProc
gdk_gl_get_glBlendEquationEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_blend_minmax.glBlendEquationEXT == (GdkGLProc_glBlendEquationEXT) -1)
    _procs_GL_EXT_blend_minmax.glBlendEquationEXT =
      (GdkGLProc_glBlendEquationEXT) gdk_gl_get_proc_address ("glBlendEquationEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendEquationEXT () - %s",
               (_procs_GL_EXT_blend_minmax.glBlendEquationEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_blend_minmax.glBlendEquationEXT);
}

/* Get GL_EXT_blend_minmax functions */
GdkGL_GL_EXT_blend_minmax *
gdk_gl_get_GL_EXT_blend_minmax (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_blend_minmax");

      if (supported)
        {
          supported &= (gdk_gl_get_glBlendEquationEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_blend_minmax () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_blend_minmax;
}

/*
 * GL_SGIX_sprite
 */

static GdkGL_GL_SGIX_sprite _procs_GL_SGIX_sprite = {
  (GdkGLProc_glSpriteParameterfSGIX) -1,
  (GdkGLProc_glSpriteParameterfvSGIX) -1,
  (GdkGLProc_glSpriteParameteriSGIX) -1,
  (GdkGLProc_glSpriteParameterivSGIX) -1
};

/* glSpriteParameterfSGIX */
GdkGLProc
gdk_gl_get_glSpriteParameterfSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_sprite.glSpriteParameterfSGIX == (GdkGLProc_glSpriteParameterfSGIX) -1)
    _procs_GL_SGIX_sprite.glSpriteParameterfSGIX =
      (GdkGLProc_glSpriteParameterfSGIX) gdk_gl_get_proc_address ("glSpriteParameterfSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSpriteParameterfSGIX () - %s",
               (_procs_GL_SGIX_sprite.glSpriteParameterfSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_sprite.glSpriteParameterfSGIX);
}

/* glSpriteParameterfvSGIX */
GdkGLProc
gdk_gl_get_glSpriteParameterfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_sprite.glSpriteParameterfvSGIX == (GdkGLProc_glSpriteParameterfvSGIX) -1)
    _procs_GL_SGIX_sprite.glSpriteParameterfvSGIX =
      (GdkGLProc_glSpriteParameterfvSGIX) gdk_gl_get_proc_address ("glSpriteParameterfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSpriteParameterfvSGIX () - %s",
               (_procs_GL_SGIX_sprite.glSpriteParameterfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_sprite.glSpriteParameterfvSGIX);
}

/* glSpriteParameteriSGIX */
GdkGLProc
gdk_gl_get_glSpriteParameteriSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_sprite.glSpriteParameteriSGIX == (GdkGLProc_glSpriteParameteriSGIX) -1)
    _procs_GL_SGIX_sprite.glSpriteParameteriSGIX =
      (GdkGLProc_glSpriteParameteriSGIX) gdk_gl_get_proc_address ("glSpriteParameteriSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSpriteParameteriSGIX () - %s",
               (_procs_GL_SGIX_sprite.glSpriteParameteriSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_sprite.glSpriteParameteriSGIX);
}

/* glSpriteParameterivSGIX */
GdkGLProc
gdk_gl_get_glSpriteParameterivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_sprite.glSpriteParameterivSGIX == (GdkGLProc_glSpriteParameterivSGIX) -1)
    _procs_GL_SGIX_sprite.glSpriteParameterivSGIX =
      (GdkGLProc_glSpriteParameterivSGIX) gdk_gl_get_proc_address ("glSpriteParameterivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSpriteParameterivSGIX () - %s",
               (_procs_GL_SGIX_sprite.glSpriteParameterivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_sprite.glSpriteParameterivSGIX);
}

/* Get GL_SGIX_sprite functions */
GdkGL_GL_SGIX_sprite *
gdk_gl_get_GL_SGIX_sprite (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_sprite");

      if (supported)
        {
          supported &= (gdk_gl_get_glSpriteParameterfSGIX () != NULL);
          supported &= (gdk_gl_get_glSpriteParameterfvSGIX () != NULL);
          supported &= (gdk_gl_get_glSpriteParameteriSGIX () != NULL);
          supported &= (gdk_gl_get_glSpriteParameterivSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_sprite () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_sprite;
}

/*
 * GL_EXT_point_parameters
 */

static GdkGL_GL_EXT_point_parameters _procs_GL_EXT_point_parameters = {
  (GdkGLProc_glPointParameterfEXT) -1,
  (GdkGLProc_glPointParameterfvEXT) -1
};

/* glPointParameterfEXT */
GdkGLProc
gdk_gl_get_glPointParameterfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_point_parameters.glPointParameterfEXT == (GdkGLProc_glPointParameterfEXT) -1)
    _procs_GL_EXT_point_parameters.glPointParameterfEXT =
      (GdkGLProc_glPointParameterfEXT) gdk_gl_get_proc_address ("glPointParameterfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfEXT () - %s",
               (_procs_GL_EXT_point_parameters.glPointParameterfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_point_parameters.glPointParameterfEXT);
}

/* glPointParameterfvEXT */
GdkGLProc
gdk_gl_get_glPointParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_point_parameters.glPointParameterfvEXT == (GdkGLProc_glPointParameterfvEXT) -1)
    _procs_GL_EXT_point_parameters.glPointParameterfvEXT =
      (GdkGLProc_glPointParameterfvEXT) gdk_gl_get_proc_address ("glPointParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfvEXT () - %s",
               (_procs_GL_EXT_point_parameters.glPointParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_point_parameters.glPointParameterfvEXT);
}

/* Get GL_EXT_point_parameters functions */
GdkGL_GL_EXT_point_parameters *
gdk_gl_get_GL_EXT_point_parameters (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_point_parameters");

      if (supported)
        {
          supported &= (gdk_gl_get_glPointParameterfEXT () != NULL);
          supported &= (gdk_gl_get_glPointParameterfvEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_point_parameters () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_point_parameters;
}

/*
 * GL_SGIS_point_parameters
 */

static GdkGL_GL_SGIS_point_parameters _procs_GL_SGIS_point_parameters = {
  (GdkGLProc_glPointParameterfSGIS) -1,
  (GdkGLProc_glPointParameterfvSGIS) -1
};

/* glPointParameterfSGIS */
GdkGLProc
gdk_gl_get_glPointParameterfSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_point_parameters.glPointParameterfSGIS == (GdkGLProc_glPointParameterfSGIS) -1)
    _procs_GL_SGIS_point_parameters.glPointParameterfSGIS =
      (GdkGLProc_glPointParameterfSGIS) gdk_gl_get_proc_address ("glPointParameterfSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfSGIS () - %s",
               (_procs_GL_SGIS_point_parameters.glPointParameterfSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_point_parameters.glPointParameterfSGIS);
}

/* glPointParameterfvSGIS */
GdkGLProc
gdk_gl_get_glPointParameterfvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_point_parameters.glPointParameterfvSGIS == (GdkGLProc_glPointParameterfvSGIS) -1)
    _procs_GL_SGIS_point_parameters.glPointParameterfvSGIS =
      (GdkGLProc_glPointParameterfvSGIS) gdk_gl_get_proc_address ("glPointParameterfvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterfvSGIS () - %s",
               (_procs_GL_SGIS_point_parameters.glPointParameterfvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_point_parameters.glPointParameterfvSGIS);
}

/* Get GL_SGIS_point_parameters functions */
GdkGL_GL_SGIS_point_parameters *
gdk_gl_get_GL_SGIS_point_parameters (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_point_parameters");

      if (supported)
        {
          supported &= (gdk_gl_get_glPointParameterfSGIS () != NULL);
          supported &= (gdk_gl_get_glPointParameterfvSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_point_parameters () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_point_parameters;
}

/*
 * GL_SGIX_instruments
 */

static GdkGL_GL_SGIX_instruments _procs_GL_SGIX_instruments = {
  (GdkGLProc_glGetInstrumentsSGIX) -1,
  (GdkGLProc_glInstrumentsBufferSGIX) -1,
  (GdkGLProc_glPollInstrumentsSGIX) -1,
  (GdkGLProc_glReadInstrumentsSGIX) -1,
  (GdkGLProc_glStartInstrumentsSGIX) -1,
  (GdkGLProc_glStopInstrumentsSGIX) -1
};

/* glGetInstrumentsSGIX */
GdkGLProc
gdk_gl_get_glGetInstrumentsSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_instruments.glGetInstrumentsSGIX == (GdkGLProc_glGetInstrumentsSGIX) -1)
    _procs_GL_SGIX_instruments.glGetInstrumentsSGIX =
      (GdkGLProc_glGetInstrumentsSGIX) gdk_gl_get_proc_address ("glGetInstrumentsSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetInstrumentsSGIX () - %s",
               (_procs_GL_SGIX_instruments.glGetInstrumentsSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_instruments.glGetInstrumentsSGIX);
}

/* glInstrumentsBufferSGIX */
GdkGLProc
gdk_gl_get_glInstrumentsBufferSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_instruments.glInstrumentsBufferSGIX == (GdkGLProc_glInstrumentsBufferSGIX) -1)
    _procs_GL_SGIX_instruments.glInstrumentsBufferSGIX =
      (GdkGLProc_glInstrumentsBufferSGIX) gdk_gl_get_proc_address ("glInstrumentsBufferSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glInstrumentsBufferSGIX () - %s",
               (_procs_GL_SGIX_instruments.glInstrumentsBufferSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_instruments.glInstrumentsBufferSGIX);
}

/* glPollInstrumentsSGIX */
GdkGLProc
gdk_gl_get_glPollInstrumentsSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_instruments.glPollInstrumentsSGIX == (GdkGLProc_glPollInstrumentsSGIX) -1)
    _procs_GL_SGIX_instruments.glPollInstrumentsSGIX =
      (GdkGLProc_glPollInstrumentsSGIX) gdk_gl_get_proc_address ("glPollInstrumentsSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPollInstrumentsSGIX () - %s",
               (_procs_GL_SGIX_instruments.glPollInstrumentsSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_instruments.glPollInstrumentsSGIX);
}

/* glReadInstrumentsSGIX */
GdkGLProc
gdk_gl_get_glReadInstrumentsSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_instruments.glReadInstrumentsSGIX == (GdkGLProc_glReadInstrumentsSGIX) -1)
    _procs_GL_SGIX_instruments.glReadInstrumentsSGIX =
      (GdkGLProc_glReadInstrumentsSGIX) gdk_gl_get_proc_address ("glReadInstrumentsSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReadInstrumentsSGIX () - %s",
               (_procs_GL_SGIX_instruments.glReadInstrumentsSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_instruments.glReadInstrumentsSGIX);
}

/* glStartInstrumentsSGIX */
GdkGLProc
gdk_gl_get_glStartInstrumentsSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_instruments.glStartInstrumentsSGIX == (GdkGLProc_glStartInstrumentsSGIX) -1)
    _procs_GL_SGIX_instruments.glStartInstrumentsSGIX =
      (GdkGLProc_glStartInstrumentsSGIX) gdk_gl_get_proc_address ("glStartInstrumentsSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glStartInstrumentsSGIX () - %s",
               (_procs_GL_SGIX_instruments.glStartInstrumentsSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_instruments.glStartInstrumentsSGIX);
}

/* glStopInstrumentsSGIX */
GdkGLProc
gdk_gl_get_glStopInstrumentsSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_instruments.glStopInstrumentsSGIX == (GdkGLProc_glStopInstrumentsSGIX) -1)
    _procs_GL_SGIX_instruments.glStopInstrumentsSGIX =
      (GdkGLProc_glStopInstrumentsSGIX) gdk_gl_get_proc_address ("glStopInstrumentsSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glStopInstrumentsSGIX () - %s",
               (_procs_GL_SGIX_instruments.glStopInstrumentsSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_instruments.glStopInstrumentsSGIX);
}

/* Get GL_SGIX_instruments functions */
GdkGL_GL_SGIX_instruments *
gdk_gl_get_GL_SGIX_instruments (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_instruments");

      if (supported)
        {
          supported &= (gdk_gl_get_glGetInstrumentsSGIX () != NULL);
          supported &= (gdk_gl_get_glInstrumentsBufferSGIX () != NULL);
          supported &= (gdk_gl_get_glPollInstrumentsSGIX () != NULL);
          supported &= (gdk_gl_get_glReadInstrumentsSGIX () != NULL);
          supported &= (gdk_gl_get_glStartInstrumentsSGIX () != NULL);
          supported &= (gdk_gl_get_glStopInstrumentsSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_instruments () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_instruments;
}

/*
 * GL_SGIX_framezoom
 */

static GdkGL_GL_SGIX_framezoom _procs_GL_SGIX_framezoom = {
  (GdkGLProc_glFrameZoomSGIX) -1
};

/* glFrameZoomSGIX */
GdkGLProc
gdk_gl_get_glFrameZoomSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_framezoom.glFrameZoomSGIX == (GdkGLProc_glFrameZoomSGIX) -1)
    _procs_GL_SGIX_framezoom.glFrameZoomSGIX =
      (GdkGLProc_glFrameZoomSGIX) gdk_gl_get_proc_address ("glFrameZoomSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFrameZoomSGIX () - %s",
               (_procs_GL_SGIX_framezoom.glFrameZoomSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_framezoom.glFrameZoomSGIX);
}

/* Get GL_SGIX_framezoom functions */
GdkGL_GL_SGIX_framezoom *
gdk_gl_get_GL_SGIX_framezoom (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_framezoom");

      if (supported)
        {
          supported &= (gdk_gl_get_glFrameZoomSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_framezoom () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_framezoom;
}

/*
 * GL_SGIX_tag_sample_buffer
 */

static GdkGL_GL_SGIX_tag_sample_buffer _procs_GL_SGIX_tag_sample_buffer = {
  (GdkGLProc_glTagSampleBufferSGIX) -1
};

/* glTagSampleBufferSGIX */
GdkGLProc
gdk_gl_get_glTagSampleBufferSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_tag_sample_buffer.glTagSampleBufferSGIX == (GdkGLProc_glTagSampleBufferSGIX) -1)
    _procs_GL_SGIX_tag_sample_buffer.glTagSampleBufferSGIX =
      (GdkGLProc_glTagSampleBufferSGIX) gdk_gl_get_proc_address ("glTagSampleBufferSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTagSampleBufferSGIX () - %s",
               (_procs_GL_SGIX_tag_sample_buffer.glTagSampleBufferSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_tag_sample_buffer.glTagSampleBufferSGIX);
}

/* Get GL_SGIX_tag_sample_buffer functions */
GdkGL_GL_SGIX_tag_sample_buffer *
gdk_gl_get_GL_SGIX_tag_sample_buffer (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_tag_sample_buffer");

      if (supported)
        {
          supported &= (gdk_gl_get_glTagSampleBufferSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_tag_sample_buffer () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_tag_sample_buffer;
}

/*
 * GL_SGIX_polynomial_ffd
 */

static GdkGL_GL_SGIX_polynomial_ffd _procs_GL_SGIX_polynomial_ffd = {
  (GdkGLProc_glDeformationMap3dSGIX) -1,
  (GdkGLProc_glDeformationMap3fSGIX) -1,
  (GdkGLProc_glDeformSGIX) -1,
  (GdkGLProc_glLoadIdentityDeformationMapSGIX) -1
};

/* glDeformationMap3dSGIX */
GdkGLProc
gdk_gl_get_glDeformationMap3dSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_polynomial_ffd.glDeformationMap3dSGIX == (GdkGLProc_glDeformationMap3dSGIX) -1)
    _procs_GL_SGIX_polynomial_ffd.glDeformationMap3dSGIX =
      (GdkGLProc_glDeformationMap3dSGIX) gdk_gl_get_proc_address ("glDeformationMap3dSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeformationMap3dSGIX () - %s",
               (_procs_GL_SGIX_polynomial_ffd.glDeformationMap3dSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_polynomial_ffd.glDeformationMap3dSGIX);
}

/* glDeformationMap3fSGIX */
GdkGLProc
gdk_gl_get_glDeformationMap3fSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_polynomial_ffd.glDeformationMap3fSGIX == (GdkGLProc_glDeformationMap3fSGIX) -1)
    _procs_GL_SGIX_polynomial_ffd.glDeformationMap3fSGIX =
      (GdkGLProc_glDeformationMap3fSGIX) gdk_gl_get_proc_address ("glDeformationMap3fSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeformationMap3fSGIX () - %s",
               (_procs_GL_SGIX_polynomial_ffd.glDeformationMap3fSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_polynomial_ffd.glDeformationMap3fSGIX);
}

/* glDeformSGIX */
GdkGLProc
gdk_gl_get_glDeformSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_polynomial_ffd.glDeformSGIX == (GdkGLProc_glDeformSGIX) -1)
    _procs_GL_SGIX_polynomial_ffd.glDeformSGIX =
      (GdkGLProc_glDeformSGIX) gdk_gl_get_proc_address ("glDeformSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeformSGIX () - %s",
               (_procs_GL_SGIX_polynomial_ffd.glDeformSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_polynomial_ffd.glDeformSGIX);
}

/* glLoadIdentityDeformationMapSGIX */
GdkGLProc
gdk_gl_get_glLoadIdentityDeformationMapSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_polynomial_ffd.glLoadIdentityDeformationMapSGIX == (GdkGLProc_glLoadIdentityDeformationMapSGIX) -1)
    _procs_GL_SGIX_polynomial_ffd.glLoadIdentityDeformationMapSGIX =
      (GdkGLProc_glLoadIdentityDeformationMapSGIX) gdk_gl_get_proc_address ("glLoadIdentityDeformationMapSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLoadIdentityDeformationMapSGIX () - %s",
               (_procs_GL_SGIX_polynomial_ffd.glLoadIdentityDeformationMapSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_polynomial_ffd.glLoadIdentityDeformationMapSGIX);
}

/* Get GL_SGIX_polynomial_ffd functions */
GdkGL_GL_SGIX_polynomial_ffd *
gdk_gl_get_GL_SGIX_polynomial_ffd (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_polynomial_ffd");

      if (supported)
        {
          supported &= (gdk_gl_get_glDeformationMap3dSGIX () != NULL);
          supported &= (gdk_gl_get_glDeformationMap3fSGIX () != NULL);
          supported &= (gdk_gl_get_glDeformSGIX () != NULL);
          supported &= (gdk_gl_get_glLoadIdentityDeformationMapSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_polynomial_ffd () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_polynomial_ffd;
}

/*
 * GL_SGIX_reference_plane
 */

static GdkGL_GL_SGIX_reference_plane _procs_GL_SGIX_reference_plane = {
  (GdkGLProc_glReferencePlaneSGIX) -1
};

/* glReferencePlaneSGIX */
GdkGLProc
gdk_gl_get_glReferencePlaneSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_reference_plane.glReferencePlaneSGIX == (GdkGLProc_glReferencePlaneSGIX) -1)
    _procs_GL_SGIX_reference_plane.glReferencePlaneSGIX =
      (GdkGLProc_glReferencePlaneSGIX) gdk_gl_get_proc_address ("glReferencePlaneSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReferencePlaneSGIX () - %s",
               (_procs_GL_SGIX_reference_plane.glReferencePlaneSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_reference_plane.glReferencePlaneSGIX);
}

/* Get GL_SGIX_reference_plane functions */
GdkGL_GL_SGIX_reference_plane *
gdk_gl_get_GL_SGIX_reference_plane (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_reference_plane");

      if (supported)
        {
          supported &= (gdk_gl_get_glReferencePlaneSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_reference_plane () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_reference_plane;
}

/*
 * GL_SGIX_flush_raster
 */

static GdkGL_GL_SGIX_flush_raster _procs_GL_SGIX_flush_raster = {
  (GdkGLProc_glFlushRasterSGIX) -1
};

/* glFlushRasterSGIX */
GdkGLProc
gdk_gl_get_glFlushRasterSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_flush_raster.glFlushRasterSGIX == (GdkGLProc_glFlushRasterSGIX) -1)
    _procs_GL_SGIX_flush_raster.glFlushRasterSGIX =
      (GdkGLProc_glFlushRasterSGIX) gdk_gl_get_proc_address ("glFlushRasterSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFlushRasterSGIX () - %s",
               (_procs_GL_SGIX_flush_raster.glFlushRasterSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_flush_raster.glFlushRasterSGIX);
}

/* Get GL_SGIX_flush_raster functions */
GdkGL_GL_SGIX_flush_raster *
gdk_gl_get_GL_SGIX_flush_raster (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_flush_raster");

      if (supported)
        {
          supported &= (gdk_gl_get_glFlushRasterSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_flush_raster () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_flush_raster;
}

/*
 * GL_SGIS_fog_function
 */

static GdkGL_GL_SGIS_fog_function _procs_GL_SGIS_fog_function = {
  (GdkGLProc_glFogFuncSGIS) -1,
  (GdkGLProc_glGetFogFuncSGIS) -1
};

/* glFogFuncSGIS */
GdkGLProc
gdk_gl_get_glFogFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_fog_function.glFogFuncSGIS == (GdkGLProc_glFogFuncSGIS) -1)
    _procs_GL_SGIS_fog_function.glFogFuncSGIS =
      (GdkGLProc_glFogFuncSGIS) gdk_gl_get_proc_address ("glFogFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogFuncSGIS () - %s",
               (_procs_GL_SGIS_fog_function.glFogFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_fog_function.glFogFuncSGIS);
}

/* glGetFogFuncSGIS */
GdkGLProc
gdk_gl_get_glGetFogFuncSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_fog_function.glGetFogFuncSGIS == (GdkGLProc_glGetFogFuncSGIS) -1)
    _procs_GL_SGIS_fog_function.glGetFogFuncSGIS =
      (GdkGLProc_glGetFogFuncSGIS) gdk_gl_get_proc_address ("glGetFogFuncSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFogFuncSGIS () - %s",
               (_procs_GL_SGIS_fog_function.glGetFogFuncSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_fog_function.glGetFogFuncSGIS);
}

/* Get GL_SGIS_fog_function functions */
GdkGL_GL_SGIS_fog_function *
gdk_gl_get_GL_SGIS_fog_function (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_fog_function");

      if (supported)
        {
          supported &= (gdk_gl_get_glFogFuncSGIS () != NULL);
          supported &= (gdk_gl_get_glGetFogFuncSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_fog_function () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_fog_function;
}

/*
 * GL_HP_image_transform
 */

static GdkGL_GL_HP_image_transform _procs_GL_HP_image_transform = {
  (GdkGLProc_glImageTransformParameteriHP) -1,
  (GdkGLProc_glImageTransformParameterfHP) -1,
  (GdkGLProc_glImageTransformParameterivHP) -1,
  (GdkGLProc_glImageTransformParameterfvHP) -1,
  (GdkGLProc_glGetImageTransformParameterivHP) -1,
  (GdkGLProc_glGetImageTransformParameterfvHP) -1
};

/* glImageTransformParameteriHP */
GdkGLProc
gdk_gl_get_glImageTransformParameteriHP (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_HP_image_transform.glImageTransformParameteriHP == (GdkGLProc_glImageTransformParameteriHP) -1)
    _procs_GL_HP_image_transform.glImageTransformParameteriHP =
      (GdkGLProc_glImageTransformParameteriHP) gdk_gl_get_proc_address ("glImageTransformParameteriHP");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glImageTransformParameteriHP () - %s",
               (_procs_GL_HP_image_transform.glImageTransformParameteriHP) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_HP_image_transform.glImageTransformParameteriHP);
}

/* glImageTransformParameterfHP */
GdkGLProc
gdk_gl_get_glImageTransformParameterfHP (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_HP_image_transform.glImageTransformParameterfHP == (GdkGLProc_glImageTransformParameterfHP) -1)
    _procs_GL_HP_image_transform.glImageTransformParameterfHP =
      (GdkGLProc_glImageTransformParameterfHP) gdk_gl_get_proc_address ("glImageTransformParameterfHP");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glImageTransformParameterfHP () - %s",
               (_procs_GL_HP_image_transform.glImageTransformParameterfHP) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_HP_image_transform.glImageTransformParameterfHP);
}

/* glImageTransformParameterivHP */
GdkGLProc
gdk_gl_get_glImageTransformParameterivHP (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_HP_image_transform.glImageTransformParameterivHP == (GdkGLProc_glImageTransformParameterivHP) -1)
    _procs_GL_HP_image_transform.glImageTransformParameterivHP =
      (GdkGLProc_glImageTransformParameterivHP) gdk_gl_get_proc_address ("glImageTransformParameterivHP");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glImageTransformParameterivHP () - %s",
               (_procs_GL_HP_image_transform.glImageTransformParameterivHP) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_HP_image_transform.glImageTransformParameterivHP);
}

/* glImageTransformParameterfvHP */
GdkGLProc
gdk_gl_get_glImageTransformParameterfvHP (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_HP_image_transform.glImageTransformParameterfvHP == (GdkGLProc_glImageTransformParameterfvHP) -1)
    _procs_GL_HP_image_transform.glImageTransformParameterfvHP =
      (GdkGLProc_glImageTransformParameterfvHP) gdk_gl_get_proc_address ("glImageTransformParameterfvHP");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glImageTransformParameterfvHP () - %s",
               (_procs_GL_HP_image_transform.glImageTransformParameterfvHP) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_HP_image_transform.glImageTransformParameterfvHP);
}

/* glGetImageTransformParameterivHP */
GdkGLProc
gdk_gl_get_glGetImageTransformParameterivHP (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_HP_image_transform.glGetImageTransformParameterivHP == (GdkGLProc_glGetImageTransformParameterivHP) -1)
    _procs_GL_HP_image_transform.glGetImageTransformParameterivHP =
      (GdkGLProc_glGetImageTransformParameterivHP) gdk_gl_get_proc_address ("glGetImageTransformParameterivHP");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetImageTransformParameterivHP () - %s",
               (_procs_GL_HP_image_transform.glGetImageTransformParameterivHP) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_HP_image_transform.glGetImageTransformParameterivHP);
}

/* glGetImageTransformParameterfvHP */
GdkGLProc
gdk_gl_get_glGetImageTransformParameterfvHP (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_HP_image_transform.glGetImageTransformParameterfvHP == (GdkGLProc_glGetImageTransformParameterfvHP) -1)
    _procs_GL_HP_image_transform.glGetImageTransformParameterfvHP =
      (GdkGLProc_glGetImageTransformParameterfvHP) gdk_gl_get_proc_address ("glGetImageTransformParameterfvHP");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetImageTransformParameterfvHP () - %s",
               (_procs_GL_HP_image_transform.glGetImageTransformParameterfvHP) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_HP_image_transform.glGetImageTransformParameterfvHP);
}

/* Get GL_HP_image_transform functions */
GdkGL_GL_HP_image_transform *
gdk_gl_get_GL_HP_image_transform (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_HP_image_transform");

      if (supported)
        {
          supported &= (gdk_gl_get_glImageTransformParameteriHP () != NULL);
          supported &= (gdk_gl_get_glImageTransformParameterfHP () != NULL);
          supported &= (gdk_gl_get_glImageTransformParameterivHP () != NULL);
          supported &= (gdk_gl_get_glImageTransformParameterfvHP () != NULL);
          supported &= (gdk_gl_get_glGetImageTransformParameterivHP () != NULL);
          supported &= (gdk_gl_get_glGetImageTransformParameterfvHP () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_HP_image_transform () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_HP_image_transform;
}

/*
 * GL_EXT_color_subtable
 */

static GdkGL_GL_EXT_color_subtable _procs_GL_EXT_color_subtable = {
  (GdkGLProc_glColorSubTableEXT) -1,
  (GdkGLProc_glCopyColorSubTableEXT) -1
};

/* glColorSubTableEXT */
GdkGLProc
gdk_gl_get_glColorSubTableEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_color_subtable.glColorSubTableEXT == (GdkGLProc_glColorSubTableEXT) -1)
    _procs_GL_EXT_color_subtable.glColorSubTableEXT =
      (GdkGLProc_glColorSubTableEXT) gdk_gl_get_proc_address ("glColorSubTableEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorSubTableEXT () - %s",
               (_procs_GL_EXT_color_subtable.glColorSubTableEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_color_subtable.glColorSubTableEXT);
}

/* glCopyColorSubTableEXT */
GdkGLProc
gdk_gl_get_glCopyColorSubTableEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_color_subtable.glCopyColorSubTableEXT == (GdkGLProc_glCopyColorSubTableEXT) -1)
    _procs_GL_EXT_color_subtable.glCopyColorSubTableEXT =
      (GdkGLProc_glCopyColorSubTableEXT) gdk_gl_get_proc_address ("glCopyColorSubTableEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCopyColorSubTableEXT () - %s",
               (_procs_GL_EXT_color_subtable.glCopyColorSubTableEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_color_subtable.glCopyColorSubTableEXT);
}

/* Get GL_EXT_color_subtable functions */
GdkGL_GL_EXT_color_subtable *
gdk_gl_get_GL_EXT_color_subtable (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_color_subtable");

      if (supported)
        {
          supported &= (gdk_gl_get_glColorSubTableEXT () != NULL);
          supported &= (gdk_gl_get_glCopyColorSubTableEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_color_subtable () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_color_subtable;
}

/*
 * GL_PGI_misc_hints
 */

static GdkGL_GL_PGI_misc_hints _procs_GL_PGI_misc_hints = {
  (GdkGLProc_glHintPGI) -1
};

/* glHintPGI */
GdkGLProc
gdk_gl_get_glHintPGI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_PGI_misc_hints.glHintPGI == (GdkGLProc_glHintPGI) -1)
    _procs_GL_PGI_misc_hints.glHintPGI =
      (GdkGLProc_glHintPGI) gdk_gl_get_proc_address ("glHintPGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glHintPGI () - %s",
               (_procs_GL_PGI_misc_hints.glHintPGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_PGI_misc_hints.glHintPGI);
}

/* Get GL_PGI_misc_hints functions */
GdkGL_GL_PGI_misc_hints *
gdk_gl_get_GL_PGI_misc_hints (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_PGI_misc_hints");

      if (supported)
        {
          supported &= (gdk_gl_get_glHintPGI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_PGI_misc_hints () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_PGI_misc_hints;
}

/*
 * GL_EXT_paletted_texture
 */

static GdkGL_GL_EXT_paletted_texture _procs_GL_EXT_paletted_texture = {
  (GdkGLProc_glColorTableEXT) -1,
  (GdkGLProc_glGetColorTableEXT) -1,
  (GdkGLProc_glGetColorTableParameterivEXT) -1,
  (GdkGLProc_glGetColorTableParameterfvEXT) -1
};

/* glColorTableEXT */
GdkGLProc
gdk_gl_get_glColorTableEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_paletted_texture.glColorTableEXT == (GdkGLProc_glColorTableEXT) -1)
    _procs_GL_EXT_paletted_texture.glColorTableEXT =
      (GdkGLProc_glColorTableEXT) gdk_gl_get_proc_address ("glColorTableEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorTableEXT () - %s",
               (_procs_GL_EXT_paletted_texture.glColorTableEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_paletted_texture.glColorTableEXT);
}

/* glGetColorTableEXT */
GdkGLProc
gdk_gl_get_glGetColorTableEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_paletted_texture.glGetColorTableEXT == (GdkGLProc_glGetColorTableEXT) -1)
    _procs_GL_EXT_paletted_texture.glGetColorTableEXT =
      (GdkGLProc_glGetColorTableEXT) gdk_gl_get_proc_address ("glGetColorTableEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableEXT () - %s",
               (_procs_GL_EXT_paletted_texture.glGetColorTableEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_paletted_texture.glGetColorTableEXT);
}

/* glGetColorTableParameterivEXT */
GdkGLProc
gdk_gl_get_glGetColorTableParameterivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_paletted_texture.glGetColorTableParameterivEXT == (GdkGLProc_glGetColorTableParameterivEXT) -1)
    _procs_GL_EXT_paletted_texture.glGetColorTableParameterivEXT =
      (GdkGLProc_glGetColorTableParameterivEXT) gdk_gl_get_proc_address ("glGetColorTableParameterivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableParameterivEXT () - %s",
               (_procs_GL_EXT_paletted_texture.glGetColorTableParameterivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_paletted_texture.glGetColorTableParameterivEXT);
}

/* glGetColorTableParameterfvEXT */
GdkGLProc
gdk_gl_get_glGetColorTableParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_paletted_texture.glGetColorTableParameterfvEXT == (GdkGLProc_glGetColorTableParameterfvEXT) -1)
    _procs_GL_EXT_paletted_texture.glGetColorTableParameterfvEXT =
      (GdkGLProc_glGetColorTableParameterfvEXT) gdk_gl_get_proc_address ("glGetColorTableParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetColorTableParameterfvEXT () - %s",
               (_procs_GL_EXT_paletted_texture.glGetColorTableParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_paletted_texture.glGetColorTableParameterfvEXT);
}

/* Get GL_EXT_paletted_texture functions */
GdkGL_GL_EXT_paletted_texture *
gdk_gl_get_GL_EXT_paletted_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_paletted_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glColorTableEXT () != NULL);
          supported &= (gdk_gl_get_glGetColorTableEXT () != NULL);
          supported &= (gdk_gl_get_glGetColorTableParameterivEXT () != NULL);
          supported &= (gdk_gl_get_glGetColorTableParameterfvEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_paletted_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_paletted_texture;
}

/*
 * GL_SGIX_list_priority
 */

static GdkGL_GL_SGIX_list_priority _procs_GL_SGIX_list_priority = {
  (GdkGLProc_glGetListParameterfvSGIX) -1,
  (GdkGLProc_glGetListParameterivSGIX) -1,
  (GdkGLProc_glListParameterfSGIX) -1,
  (GdkGLProc_glListParameterfvSGIX) -1,
  (GdkGLProc_glListParameteriSGIX) -1,
  (GdkGLProc_glListParameterivSGIX) -1
};

/* glGetListParameterfvSGIX */
GdkGLProc
gdk_gl_get_glGetListParameterfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_list_priority.glGetListParameterfvSGIX == (GdkGLProc_glGetListParameterfvSGIX) -1)
    _procs_GL_SGIX_list_priority.glGetListParameterfvSGIX =
      (GdkGLProc_glGetListParameterfvSGIX) gdk_gl_get_proc_address ("glGetListParameterfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetListParameterfvSGIX () - %s",
               (_procs_GL_SGIX_list_priority.glGetListParameterfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_list_priority.glGetListParameterfvSGIX);
}

/* glGetListParameterivSGIX */
GdkGLProc
gdk_gl_get_glGetListParameterivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_list_priority.glGetListParameterivSGIX == (GdkGLProc_glGetListParameterivSGIX) -1)
    _procs_GL_SGIX_list_priority.glGetListParameterivSGIX =
      (GdkGLProc_glGetListParameterivSGIX) gdk_gl_get_proc_address ("glGetListParameterivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetListParameterivSGIX () - %s",
               (_procs_GL_SGIX_list_priority.glGetListParameterivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_list_priority.glGetListParameterivSGIX);
}

/* glListParameterfSGIX */
GdkGLProc
gdk_gl_get_glListParameterfSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_list_priority.glListParameterfSGIX == (GdkGLProc_glListParameterfSGIX) -1)
    _procs_GL_SGIX_list_priority.glListParameterfSGIX =
      (GdkGLProc_glListParameterfSGIX) gdk_gl_get_proc_address ("glListParameterfSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glListParameterfSGIX () - %s",
               (_procs_GL_SGIX_list_priority.glListParameterfSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_list_priority.glListParameterfSGIX);
}

/* glListParameterfvSGIX */
GdkGLProc
gdk_gl_get_glListParameterfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_list_priority.glListParameterfvSGIX == (GdkGLProc_glListParameterfvSGIX) -1)
    _procs_GL_SGIX_list_priority.glListParameterfvSGIX =
      (GdkGLProc_glListParameterfvSGIX) gdk_gl_get_proc_address ("glListParameterfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glListParameterfvSGIX () - %s",
               (_procs_GL_SGIX_list_priority.glListParameterfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_list_priority.glListParameterfvSGIX);
}

/* glListParameteriSGIX */
GdkGLProc
gdk_gl_get_glListParameteriSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_list_priority.glListParameteriSGIX == (GdkGLProc_glListParameteriSGIX) -1)
    _procs_GL_SGIX_list_priority.glListParameteriSGIX =
      (GdkGLProc_glListParameteriSGIX) gdk_gl_get_proc_address ("glListParameteriSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glListParameteriSGIX () - %s",
               (_procs_GL_SGIX_list_priority.glListParameteriSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_list_priority.glListParameteriSGIX);
}

/* glListParameterivSGIX */
GdkGLProc
gdk_gl_get_glListParameterivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_list_priority.glListParameterivSGIX == (GdkGLProc_glListParameterivSGIX) -1)
    _procs_GL_SGIX_list_priority.glListParameterivSGIX =
      (GdkGLProc_glListParameterivSGIX) gdk_gl_get_proc_address ("glListParameterivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glListParameterivSGIX () - %s",
               (_procs_GL_SGIX_list_priority.glListParameterivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_list_priority.glListParameterivSGIX);
}

/* Get GL_SGIX_list_priority functions */
GdkGL_GL_SGIX_list_priority *
gdk_gl_get_GL_SGIX_list_priority (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_list_priority");

      if (supported)
        {
          supported &= (gdk_gl_get_glGetListParameterfvSGIX () != NULL);
          supported &= (gdk_gl_get_glGetListParameterivSGIX () != NULL);
          supported &= (gdk_gl_get_glListParameterfSGIX () != NULL);
          supported &= (gdk_gl_get_glListParameterfvSGIX () != NULL);
          supported &= (gdk_gl_get_glListParameteriSGIX () != NULL);
          supported &= (gdk_gl_get_glListParameterivSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_list_priority () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_list_priority;
}

/*
 * GL_EXT_index_material
 */

static GdkGL_GL_EXT_index_material _procs_GL_EXT_index_material = {
  (GdkGLProc_glIndexMaterialEXT) -1
};

/* glIndexMaterialEXT */
GdkGLProc
gdk_gl_get_glIndexMaterialEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_index_material.glIndexMaterialEXT == (GdkGLProc_glIndexMaterialEXT) -1)
    _procs_GL_EXT_index_material.glIndexMaterialEXT =
      (GdkGLProc_glIndexMaterialEXT) gdk_gl_get_proc_address ("glIndexMaterialEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIndexMaterialEXT () - %s",
               (_procs_GL_EXT_index_material.glIndexMaterialEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_index_material.glIndexMaterialEXT);
}

/* Get GL_EXT_index_material functions */
GdkGL_GL_EXT_index_material *
gdk_gl_get_GL_EXT_index_material (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_index_material");

      if (supported)
        {
          supported &= (gdk_gl_get_glIndexMaterialEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_index_material () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_index_material;
}

/*
 * GL_EXT_index_func
 */

static GdkGL_GL_EXT_index_func _procs_GL_EXT_index_func = {
  (GdkGLProc_glIndexFuncEXT) -1
};

/* glIndexFuncEXT */
GdkGLProc
gdk_gl_get_glIndexFuncEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_index_func.glIndexFuncEXT == (GdkGLProc_glIndexFuncEXT) -1)
    _procs_GL_EXT_index_func.glIndexFuncEXT =
      (GdkGLProc_glIndexFuncEXT) gdk_gl_get_proc_address ("glIndexFuncEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIndexFuncEXT () - %s",
               (_procs_GL_EXT_index_func.glIndexFuncEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_index_func.glIndexFuncEXT);
}

/* Get GL_EXT_index_func functions */
GdkGL_GL_EXT_index_func *
gdk_gl_get_GL_EXT_index_func (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_index_func");

      if (supported)
        {
          supported &= (gdk_gl_get_glIndexFuncEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_index_func () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_index_func;
}

/*
 * GL_EXT_compiled_vertex_array
 */

static GdkGL_GL_EXT_compiled_vertex_array _procs_GL_EXT_compiled_vertex_array = {
  (GdkGLProc_glLockArraysEXT) -1,
  (GdkGLProc_glUnlockArraysEXT) -1
};

/* glLockArraysEXT */
GdkGLProc
gdk_gl_get_glLockArraysEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_compiled_vertex_array.glLockArraysEXT == (GdkGLProc_glLockArraysEXT) -1)
    _procs_GL_EXT_compiled_vertex_array.glLockArraysEXT =
      (GdkGLProc_glLockArraysEXT) gdk_gl_get_proc_address ("glLockArraysEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLockArraysEXT () - %s",
               (_procs_GL_EXT_compiled_vertex_array.glLockArraysEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_compiled_vertex_array.glLockArraysEXT);
}

/* glUnlockArraysEXT */
GdkGLProc
gdk_gl_get_glUnlockArraysEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_compiled_vertex_array.glUnlockArraysEXT == (GdkGLProc_glUnlockArraysEXT) -1)
    _procs_GL_EXT_compiled_vertex_array.glUnlockArraysEXT =
      (GdkGLProc_glUnlockArraysEXT) gdk_gl_get_proc_address ("glUnlockArraysEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glUnlockArraysEXT () - %s",
               (_procs_GL_EXT_compiled_vertex_array.glUnlockArraysEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_compiled_vertex_array.glUnlockArraysEXT);
}

/* Get GL_EXT_compiled_vertex_array functions */
GdkGL_GL_EXT_compiled_vertex_array *
gdk_gl_get_GL_EXT_compiled_vertex_array (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_compiled_vertex_array");

      if (supported)
        {
          supported &= (gdk_gl_get_glLockArraysEXT () != NULL);
          supported &= (gdk_gl_get_glUnlockArraysEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_compiled_vertex_array () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_compiled_vertex_array;
}

/*
 * GL_EXT_cull_vertex
 */

static GdkGL_GL_EXT_cull_vertex _procs_GL_EXT_cull_vertex = {
  (GdkGLProc_glCullParameterdvEXT) -1,
  (GdkGLProc_glCullParameterfvEXT) -1
};

/* glCullParameterdvEXT */
GdkGLProc
gdk_gl_get_glCullParameterdvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_cull_vertex.glCullParameterdvEXT == (GdkGLProc_glCullParameterdvEXT) -1)
    _procs_GL_EXT_cull_vertex.glCullParameterdvEXT =
      (GdkGLProc_glCullParameterdvEXT) gdk_gl_get_proc_address ("glCullParameterdvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCullParameterdvEXT () - %s",
               (_procs_GL_EXT_cull_vertex.glCullParameterdvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_cull_vertex.glCullParameterdvEXT);
}

/* glCullParameterfvEXT */
GdkGLProc
gdk_gl_get_glCullParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_cull_vertex.glCullParameterfvEXT == (GdkGLProc_glCullParameterfvEXT) -1)
    _procs_GL_EXT_cull_vertex.glCullParameterfvEXT =
      (GdkGLProc_glCullParameterfvEXT) gdk_gl_get_proc_address ("glCullParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCullParameterfvEXT () - %s",
               (_procs_GL_EXT_cull_vertex.glCullParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_cull_vertex.glCullParameterfvEXT);
}

/* Get GL_EXT_cull_vertex functions */
GdkGL_GL_EXT_cull_vertex *
gdk_gl_get_GL_EXT_cull_vertex (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_cull_vertex");

      if (supported)
        {
          supported &= (gdk_gl_get_glCullParameterdvEXT () != NULL);
          supported &= (gdk_gl_get_glCullParameterfvEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_cull_vertex () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_cull_vertex;
}

/*
 * GL_SGIX_fragment_lighting
 */

static GdkGL_GL_SGIX_fragment_lighting _procs_GL_SGIX_fragment_lighting = {
  (GdkGLProc_glFragmentColorMaterialSGIX) -1,
  (GdkGLProc_glFragmentLightfSGIX) -1,
  (GdkGLProc_glFragmentLightfvSGIX) -1,
  (GdkGLProc_glFragmentLightiSGIX) -1,
  (GdkGLProc_glFragmentLightivSGIX) -1,
  (GdkGLProc_glFragmentLightModelfSGIX) -1,
  (GdkGLProc_glFragmentLightModelfvSGIX) -1,
  (GdkGLProc_glFragmentLightModeliSGIX) -1,
  (GdkGLProc_glFragmentLightModelivSGIX) -1,
  (GdkGLProc_glFragmentMaterialfSGIX) -1,
  (GdkGLProc_glFragmentMaterialfvSGIX) -1,
  (GdkGLProc_glFragmentMaterialiSGIX) -1,
  (GdkGLProc_glFragmentMaterialivSGIX) -1,
  (GdkGLProc_glGetFragmentLightfvSGIX) -1,
  (GdkGLProc_glGetFragmentLightivSGIX) -1,
  (GdkGLProc_glGetFragmentMaterialfvSGIX) -1,
  (GdkGLProc_glGetFragmentMaterialivSGIX) -1,
  (GdkGLProc_glLightEnviSGIX) -1
};

/* glFragmentColorMaterialSGIX */
GdkGLProc
gdk_gl_get_glFragmentColorMaterialSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentColorMaterialSGIX == (GdkGLProc_glFragmentColorMaterialSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentColorMaterialSGIX =
      (GdkGLProc_glFragmentColorMaterialSGIX) gdk_gl_get_proc_address ("glFragmentColorMaterialSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentColorMaterialSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentColorMaterialSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentColorMaterialSGIX);
}

/* glFragmentLightfSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightfSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightfSGIX == (GdkGLProc_glFragmentLightfSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightfSGIX =
      (GdkGLProc_glFragmentLightfSGIX) gdk_gl_get_proc_address ("glFragmentLightfSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightfSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightfSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightfSGIX);
}

/* glFragmentLightfvSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightfvSGIX == (GdkGLProc_glFragmentLightfvSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightfvSGIX =
      (GdkGLProc_glFragmentLightfvSGIX) gdk_gl_get_proc_address ("glFragmentLightfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightfvSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightfvSGIX);
}

/* glFragmentLightiSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightiSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightiSGIX == (GdkGLProc_glFragmentLightiSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightiSGIX =
      (GdkGLProc_glFragmentLightiSGIX) gdk_gl_get_proc_address ("glFragmentLightiSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightiSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightiSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightiSGIX);
}

/* glFragmentLightivSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightivSGIX == (GdkGLProc_glFragmentLightivSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightivSGIX =
      (GdkGLProc_glFragmentLightivSGIX) gdk_gl_get_proc_address ("glFragmentLightivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightivSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightivSGIX);
}

/* glFragmentLightModelfSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightModelfSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelfSGIX == (GdkGLProc_glFragmentLightModelfSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightModelfSGIX =
      (GdkGLProc_glFragmentLightModelfSGIX) gdk_gl_get_proc_address ("glFragmentLightModelfSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModelfSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelfSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelfSGIX);
}

/* glFragmentLightModelfvSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightModelfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelfvSGIX == (GdkGLProc_glFragmentLightModelfvSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightModelfvSGIX =
      (GdkGLProc_glFragmentLightModelfvSGIX) gdk_gl_get_proc_address ("glFragmentLightModelfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModelfvSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelfvSGIX);
}

/* glFragmentLightModeliSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightModeliSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightModeliSGIX == (GdkGLProc_glFragmentLightModeliSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightModeliSGIX =
      (GdkGLProc_glFragmentLightModeliSGIX) gdk_gl_get_proc_address ("glFragmentLightModeliSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModeliSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightModeliSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightModeliSGIX);
}

/* glFragmentLightModelivSGIX */
GdkGLProc
gdk_gl_get_glFragmentLightModelivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelivSGIX == (GdkGLProc_glFragmentLightModelivSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentLightModelivSGIX =
      (GdkGLProc_glFragmentLightModelivSGIX) gdk_gl_get_proc_address ("glFragmentLightModelivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModelivSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentLightModelivSGIX);
}

/* glFragmentMaterialfSGIX */
GdkGLProc
gdk_gl_get_glFragmentMaterialfSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialfSGIX == (GdkGLProc_glFragmentMaterialfSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentMaterialfSGIX =
      (GdkGLProc_glFragmentMaterialfSGIX) gdk_gl_get_proc_address ("glFragmentMaterialfSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialfSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialfSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialfSGIX);
}

/* glFragmentMaterialfvSGIX */
GdkGLProc
gdk_gl_get_glFragmentMaterialfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialfvSGIX == (GdkGLProc_glFragmentMaterialfvSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentMaterialfvSGIX =
      (GdkGLProc_glFragmentMaterialfvSGIX) gdk_gl_get_proc_address ("glFragmentMaterialfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialfvSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialfvSGIX);
}

/* glFragmentMaterialiSGIX */
GdkGLProc
gdk_gl_get_glFragmentMaterialiSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialiSGIX == (GdkGLProc_glFragmentMaterialiSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentMaterialiSGIX =
      (GdkGLProc_glFragmentMaterialiSGIX) gdk_gl_get_proc_address ("glFragmentMaterialiSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialiSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialiSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialiSGIX);
}

/* glFragmentMaterialivSGIX */
GdkGLProc
gdk_gl_get_glFragmentMaterialivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialivSGIX == (GdkGLProc_glFragmentMaterialivSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glFragmentMaterialivSGIX =
      (GdkGLProc_glFragmentMaterialivSGIX) gdk_gl_get_proc_address ("glFragmentMaterialivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialivSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glFragmentMaterialivSGIX);
}

/* glGetFragmentLightfvSGIX */
GdkGLProc
gdk_gl_get_glGetFragmentLightfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glGetFragmentLightfvSGIX == (GdkGLProc_glGetFragmentLightfvSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glGetFragmentLightfvSGIX =
      (GdkGLProc_glGetFragmentLightfvSGIX) gdk_gl_get_proc_address ("glGetFragmentLightfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentLightfvSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glGetFragmentLightfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glGetFragmentLightfvSGIX);
}

/* glGetFragmentLightivSGIX */
GdkGLProc
gdk_gl_get_glGetFragmentLightivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glGetFragmentLightivSGIX == (GdkGLProc_glGetFragmentLightivSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glGetFragmentLightivSGIX =
      (GdkGLProc_glGetFragmentLightivSGIX) gdk_gl_get_proc_address ("glGetFragmentLightivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentLightivSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glGetFragmentLightivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glGetFragmentLightivSGIX);
}

/* glGetFragmentMaterialfvSGIX */
GdkGLProc
gdk_gl_get_glGetFragmentMaterialfvSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialfvSGIX == (GdkGLProc_glGetFragmentMaterialfvSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialfvSGIX =
      (GdkGLProc_glGetFragmentMaterialfvSGIX) gdk_gl_get_proc_address ("glGetFragmentMaterialfvSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentMaterialfvSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialfvSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialfvSGIX);
}

/* glGetFragmentMaterialivSGIX */
GdkGLProc
gdk_gl_get_glGetFragmentMaterialivSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialivSGIX == (GdkGLProc_glGetFragmentMaterialivSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialivSGIX =
      (GdkGLProc_glGetFragmentMaterialivSGIX) gdk_gl_get_proc_address ("glGetFragmentMaterialivSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentMaterialivSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialivSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glGetFragmentMaterialivSGIX);
}

/* glLightEnviSGIX */
GdkGLProc
gdk_gl_get_glLightEnviSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fragment_lighting.glLightEnviSGIX == (GdkGLProc_glLightEnviSGIX) -1)
    _procs_GL_SGIX_fragment_lighting.glLightEnviSGIX =
      (GdkGLProc_glLightEnviSGIX) gdk_gl_get_proc_address ("glLightEnviSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLightEnviSGIX () - %s",
               (_procs_GL_SGIX_fragment_lighting.glLightEnviSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fragment_lighting.glLightEnviSGIX);
}

/* Get GL_SGIX_fragment_lighting functions */
GdkGL_GL_SGIX_fragment_lighting *
gdk_gl_get_GL_SGIX_fragment_lighting (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_fragment_lighting");

      if (supported)
        {
          supported &= (gdk_gl_get_glFragmentColorMaterialSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightfSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightfvSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightiSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightivSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModelfSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModelfvSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModeliSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModelivSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialfSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialfvSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialiSGIX () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialivSGIX () != NULL);
          supported &= (gdk_gl_get_glGetFragmentLightfvSGIX () != NULL);
          supported &= (gdk_gl_get_glGetFragmentLightivSGIX () != NULL);
          supported &= (gdk_gl_get_glGetFragmentMaterialfvSGIX () != NULL);
          supported &= (gdk_gl_get_glGetFragmentMaterialivSGIX () != NULL);
          supported &= (gdk_gl_get_glLightEnviSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_fragment_lighting () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_fragment_lighting;
}

/*
 * GL_EXT_draw_range_elements
 */

static GdkGL_GL_EXT_draw_range_elements _procs_GL_EXT_draw_range_elements = {
  (GdkGLProc_glDrawRangeElementsEXT) -1
};

/* glDrawRangeElementsEXT */
GdkGLProc
gdk_gl_get_glDrawRangeElementsEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_draw_range_elements.glDrawRangeElementsEXT == (GdkGLProc_glDrawRangeElementsEXT) -1)
    _procs_GL_EXT_draw_range_elements.glDrawRangeElementsEXT =
      (GdkGLProc_glDrawRangeElementsEXT) gdk_gl_get_proc_address ("glDrawRangeElementsEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawRangeElementsEXT () - %s",
               (_procs_GL_EXT_draw_range_elements.glDrawRangeElementsEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_draw_range_elements.glDrawRangeElementsEXT);
}

/* Get GL_EXT_draw_range_elements functions */
GdkGL_GL_EXT_draw_range_elements *
gdk_gl_get_GL_EXT_draw_range_elements (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_draw_range_elements");

      if (supported)
        {
          supported &= (gdk_gl_get_glDrawRangeElementsEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_draw_range_elements () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_draw_range_elements;
}

/*
 * GL_EXT_light_texture
 */

static GdkGL_GL_EXT_light_texture _procs_GL_EXT_light_texture = {
  (GdkGLProc_glApplyTextureEXT) -1,
  (GdkGLProc_glTextureLightEXT) -1,
  (GdkGLProc_glTextureMaterialEXT) -1
};

/* glApplyTextureEXT */
GdkGLProc
gdk_gl_get_glApplyTextureEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_light_texture.glApplyTextureEXT == (GdkGLProc_glApplyTextureEXT) -1)
    _procs_GL_EXT_light_texture.glApplyTextureEXT =
      (GdkGLProc_glApplyTextureEXT) gdk_gl_get_proc_address ("glApplyTextureEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glApplyTextureEXT () - %s",
               (_procs_GL_EXT_light_texture.glApplyTextureEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_light_texture.glApplyTextureEXT);
}

/* glTextureLightEXT */
GdkGLProc
gdk_gl_get_glTextureLightEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_light_texture.glTextureLightEXT == (GdkGLProc_glTextureLightEXT) -1)
    _procs_GL_EXT_light_texture.glTextureLightEXT =
      (GdkGLProc_glTextureLightEXT) gdk_gl_get_proc_address ("glTextureLightEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTextureLightEXT () - %s",
               (_procs_GL_EXT_light_texture.glTextureLightEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_light_texture.glTextureLightEXT);
}

/* glTextureMaterialEXT */
GdkGLProc
gdk_gl_get_glTextureMaterialEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_light_texture.glTextureMaterialEXT == (GdkGLProc_glTextureMaterialEXT) -1)
    _procs_GL_EXT_light_texture.glTextureMaterialEXT =
      (GdkGLProc_glTextureMaterialEXT) gdk_gl_get_proc_address ("glTextureMaterialEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTextureMaterialEXT () - %s",
               (_procs_GL_EXT_light_texture.glTextureMaterialEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_light_texture.glTextureMaterialEXT);
}

/* Get GL_EXT_light_texture functions */
GdkGL_GL_EXT_light_texture *
gdk_gl_get_GL_EXT_light_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_light_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glApplyTextureEXT () != NULL);
          supported &= (gdk_gl_get_glTextureLightEXT () != NULL);
          supported &= (gdk_gl_get_glTextureMaterialEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_light_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_light_texture;
}

/*
 * GL_SGIX_async
 */

static GdkGL_GL_SGIX_async _procs_GL_SGIX_async = {
  (GdkGLProc_glAsyncMarkerSGIX) -1,
  (GdkGLProc_glFinishAsyncSGIX) -1,
  (GdkGLProc_glPollAsyncSGIX) -1,
  (GdkGLProc_glGenAsyncMarkersSGIX) -1,
  (GdkGLProc_glDeleteAsyncMarkersSGIX) -1,
  (GdkGLProc_glIsAsyncMarkerSGIX) -1
};

/* glAsyncMarkerSGIX */
GdkGLProc
gdk_gl_get_glAsyncMarkerSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_async.glAsyncMarkerSGIX == (GdkGLProc_glAsyncMarkerSGIX) -1)
    _procs_GL_SGIX_async.glAsyncMarkerSGIX =
      (GdkGLProc_glAsyncMarkerSGIX) gdk_gl_get_proc_address ("glAsyncMarkerSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAsyncMarkerSGIX () - %s",
               (_procs_GL_SGIX_async.glAsyncMarkerSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_async.glAsyncMarkerSGIX);
}

/* glFinishAsyncSGIX */
GdkGLProc
gdk_gl_get_glFinishAsyncSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_async.glFinishAsyncSGIX == (GdkGLProc_glFinishAsyncSGIX) -1)
    _procs_GL_SGIX_async.glFinishAsyncSGIX =
      (GdkGLProc_glFinishAsyncSGIX) gdk_gl_get_proc_address ("glFinishAsyncSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFinishAsyncSGIX () - %s",
               (_procs_GL_SGIX_async.glFinishAsyncSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_async.glFinishAsyncSGIX);
}

/* glPollAsyncSGIX */
GdkGLProc
gdk_gl_get_glPollAsyncSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_async.glPollAsyncSGIX == (GdkGLProc_glPollAsyncSGIX) -1)
    _procs_GL_SGIX_async.glPollAsyncSGIX =
      (GdkGLProc_glPollAsyncSGIX) gdk_gl_get_proc_address ("glPollAsyncSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPollAsyncSGIX () - %s",
               (_procs_GL_SGIX_async.glPollAsyncSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_async.glPollAsyncSGIX);
}

/* glGenAsyncMarkersSGIX */
GdkGLProc
gdk_gl_get_glGenAsyncMarkersSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_async.glGenAsyncMarkersSGIX == (GdkGLProc_glGenAsyncMarkersSGIX) -1)
    _procs_GL_SGIX_async.glGenAsyncMarkersSGIX =
      (GdkGLProc_glGenAsyncMarkersSGIX) gdk_gl_get_proc_address ("glGenAsyncMarkersSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenAsyncMarkersSGIX () - %s",
               (_procs_GL_SGIX_async.glGenAsyncMarkersSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_async.glGenAsyncMarkersSGIX);
}

/* glDeleteAsyncMarkersSGIX */
GdkGLProc
gdk_gl_get_glDeleteAsyncMarkersSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_async.glDeleteAsyncMarkersSGIX == (GdkGLProc_glDeleteAsyncMarkersSGIX) -1)
    _procs_GL_SGIX_async.glDeleteAsyncMarkersSGIX =
      (GdkGLProc_glDeleteAsyncMarkersSGIX) gdk_gl_get_proc_address ("glDeleteAsyncMarkersSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteAsyncMarkersSGIX () - %s",
               (_procs_GL_SGIX_async.glDeleteAsyncMarkersSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_async.glDeleteAsyncMarkersSGIX);
}

/* glIsAsyncMarkerSGIX */
GdkGLProc
gdk_gl_get_glIsAsyncMarkerSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_async.glIsAsyncMarkerSGIX == (GdkGLProc_glIsAsyncMarkerSGIX) -1)
    _procs_GL_SGIX_async.glIsAsyncMarkerSGIX =
      (GdkGLProc_glIsAsyncMarkerSGIX) gdk_gl_get_proc_address ("glIsAsyncMarkerSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsAsyncMarkerSGIX () - %s",
               (_procs_GL_SGIX_async.glIsAsyncMarkerSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_async.glIsAsyncMarkerSGIX);
}

/* Get GL_SGIX_async functions */
GdkGL_GL_SGIX_async *
gdk_gl_get_GL_SGIX_async (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_async");

      if (supported)
        {
          supported &= (gdk_gl_get_glAsyncMarkerSGIX () != NULL);
          supported &= (gdk_gl_get_glFinishAsyncSGIX () != NULL);
          supported &= (gdk_gl_get_glPollAsyncSGIX () != NULL);
          supported &= (gdk_gl_get_glGenAsyncMarkersSGIX () != NULL);
          supported &= (gdk_gl_get_glDeleteAsyncMarkersSGIX () != NULL);
          supported &= (gdk_gl_get_glIsAsyncMarkerSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_async () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_async;
}

/*
 * GL_INTEL_parallel_arrays
 */

static GdkGL_GL_INTEL_parallel_arrays _procs_GL_INTEL_parallel_arrays = {
  (GdkGLProc_glVertexPointervINTEL) -1,
  (GdkGLProc_glNormalPointervINTEL) -1,
  (GdkGLProc_glColorPointervINTEL) -1,
  (GdkGLProc_glTexCoordPointervINTEL) -1
};

/* glVertexPointervINTEL */
GdkGLProc
gdk_gl_get_glVertexPointervINTEL (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_INTEL_parallel_arrays.glVertexPointervINTEL == (GdkGLProc_glVertexPointervINTEL) -1)
    _procs_GL_INTEL_parallel_arrays.glVertexPointervINTEL =
      (GdkGLProc_glVertexPointervINTEL) gdk_gl_get_proc_address ("glVertexPointervINTEL");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexPointervINTEL () - %s",
               (_procs_GL_INTEL_parallel_arrays.glVertexPointervINTEL) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_INTEL_parallel_arrays.glVertexPointervINTEL);
}

/* glNormalPointervINTEL */
GdkGLProc
gdk_gl_get_glNormalPointervINTEL (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_INTEL_parallel_arrays.glNormalPointervINTEL == (GdkGLProc_glNormalPointervINTEL) -1)
    _procs_GL_INTEL_parallel_arrays.glNormalPointervINTEL =
      (GdkGLProc_glNormalPointervINTEL) gdk_gl_get_proc_address ("glNormalPointervINTEL");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalPointervINTEL () - %s",
               (_procs_GL_INTEL_parallel_arrays.glNormalPointervINTEL) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_INTEL_parallel_arrays.glNormalPointervINTEL);
}

/* glColorPointervINTEL */
GdkGLProc
gdk_gl_get_glColorPointervINTEL (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_INTEL_parallel_arrays.glColorPointervINTEL == (GdkGLProc_glColorPointervINTEL) -1)
    _procs_GL_INTEL_parallel_arrays.glColorPointervINTEL =
      (GdkGLProc_glColorPointervINTEL) gdk_gl_get_proc_address ("glColorPointervINTEL");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorPointervINTEL () - %s",
               (_procs_GL_INTEL_parallel_arrays.glColorPointervINTEL) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_INTEL_parallel_arrays.glColorPointervINTEL);
}

/* glTexCoordPointervINTEL */
GdkGLProc
gdk_gl_get_glTexCoordPointervINTEL (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_INTEL_parallel_arrays.glTexCoordPointervINTEL == (GdkGLProc_glTexCoordPointervINTEL) -1)
    _procs_GL_INTEL_parallel_arrays.glTexCoordPointervINTEL =
      (GdkGLProc_glTexCoordPointervINTEL) gdk_gl_get_proc_address ("glTexCoordPointervINTEL");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoordPointervINTEL () - %s",
               (_procs_GL_INTEL_parallel_arrays.glTexCoordPointervINTEL) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_INTEL_parallel_arrays.glTexCoordPointervINTEL);
}

/* Get GL_INTEL_parallel_arrays functions */
GdkGL_GL_INTEL_parallel_arrays *
gdk_gl_get_GL_INTEL_parallel_arrays (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_INTEL_parallel_arrays");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertexPointervINTEL () != NULL);
          supported &= (gdk_gl_get_glNormalPointervINTEL () != NULL);
          supported &= (gdk_gl_get_glColorPointervINTEL () != NULL);
          supported &= (gdk_gl_get_glTexCoordPointervINTEL () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_INTEL_parallel_arrays () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_INTEL_parallel_arrays;
}

/*
 * GL_EXT_pixel_transform
 */

static GdkGL_GL_EXT_pixel_transform _procs_GL_EXT_pixel_transform = {
  (GdkGLProc_glPixelTransformParameteriEXT) -1,
  (GdkGLProc_glPixelTransformParameterfEXT) -1,
  (GdkGLProc_glPixelTransformParameterivEXT) -1,
  (GdkGLProc_glPixelTransformParameterfvEXT) -1
};

/* glPixelTransformParameteriEXT */
GdkGLProc
gdk_gl_get_glPixelTransformParameteriEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_pixel_transform.glPixelTransformParameteriEXT == (GdkGLProc_glPixelTransformParameteriEXT) -1)
    _procs_GL_EXT_pixel_transform.glPixelTransformParameteriEXT =
      (GdkGLProc_glPixelTransformParameteriEXT) gdk_gl_get_proc_address ("glPixelTransformParameteriEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTransformParameteriEXT () - %s",
               (_procs_GL_EXT_pixel_transform.glPixelTransformParameteriEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_pixel_transform.glPixelTransformParameteriEXT);
}

/* glPixelTransformParameterfEXT */
GdkGLProc
gdk_gl_get_glPixelTransformParameterfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_pixel_transform.glPixelTransformParameterfEXT == (GdkGLProc_glPixelTransformParameterfEXT) -1)
    _procs_GL_EXT_pixel_transform.glPixelTransformParameterfEXT =
      (GdkGLProc_glPixelTransformParameterfEXT) gdk_gl_get_proc_address ("glPixelTransformParameterfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTransformParameterfEXT () - %s",
               (_procs_GL_EXT_pixel_transform.glPixelTransformParameterfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_pixel_transform.glPixelTransformParameterfEXT);
}

/* glPixelTransformParameterivEXT */
GdkGLProc
gdk_gl_get_glPixelTransformParameterivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_pixel_transform.glPixelTransformParameterivEXT == (GdkGLProc_glPixelTransformParameterivEXT) -1)
    _procs_GL_EXT_pixel_transform.glPixelTransformParameterivEXT =
      (GdkGLProc_glPixelTransformParameterivEXT) gdk_gl_get_proc_address ("glPixelTransformParameterivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTransformParameterivEXT () - %s",
               (_procs_GL_EXT_pixel_transform.glPixelTransformParameterivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_pixel_transform.glPixelTransformParameterivEXT);
}

/* glPixelTransformParameterfvEXT */
GdkGLProc
gdk_gl_get_glPixelTransformParameterfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_pixel_transform.glPixelTransformParameterfvEXT == (GdkGLProc_glPixelTransformParameterfvEXT) -1)
    _procs_GL_EXT_pixel_transform.glPixelTransformParameterfvEXT =
      (GdkGLProc_glPixelTransformParameterfvEXT) gdk_gl_get_proc_address ("glPixelTransformParameterfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelTransformParameterfvEXT () - %s",
               (_procs_GL_EXT_pixel_transform.glPixelTransformParameterfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_pixel_transform.glPixelTransformParameterfvEXT);
}

/* Get GL_EXT_pixel_transform functions */
GdkGL_GL_EXT_pixel_transform *
gdk_gl_get_GL_EXT_pixel_transform (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_pixel_transform");

      if (supported)
        {
          supported &= (gdk_gl_get_glPixelTransformParameteriEXT () != NULL);
          supported &= (gdk_gl_get_glPixelTransformParameterfEXT () != NULL);
          supported &= (gdk_gl_get_glPixelTransformParameterivEXT () != NULL);
          supported &= (gdk_gl_get_glPixelTransformParameterfvEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_pixel_transform () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_pixel_transform;
}

/*
 * GL_EXT_secondary_color
 */

static GdkGL_GL_EXT_secondary_color _procs_GL_EXT_secondary_color = {
  (GdkGLProc_glSecondaryColor3bEXT) -1,
  (GdkGLProc_glSecondaryColor3bvEXT) -1,
  (GdkGLProc_glSecondaryColor3dEXT) -1,
  (GdkGLProc_glSecondaryColor3dvEXT) -1,
  (GdkGLProc_glSecondaryColor3fEXT) -1,
  (GdkGLProc_glSecondaryColor3fvEXT) -1,
  (GdkGLProc_glSecondaryColor3iEXT) -1,
  (GdkGLProc_glSecondaryColor3ivEXT) -1,
  (GdkGLProc_glSecondaryColor3sEXT) -1,
  (GdkGLProc_glSecondaryColor3svEXT) -1,
  (GdkGLProc_glSecondaryColor3ubEXT) -1,
  (GdkGLProc_glSecondaryColor3ubvEXT) -1,
  (GdkGLProc_glSecondaryColor3uiEXT) -1,
  (GdkGLProc_glSecondaryColor3uivEXT) -1,
  (GdkGLProc_glSecondaryColor3usEXT) -1,
  (GdkGLProc_glSecondaryColor3usvEXT) -1,
  (GdkGLProc_glSecondaryColorPointerEXT) -1
};

/* glSecondaryColor3bEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3bEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3bEXT == (GdkGLProc_glSecondaryColor3bEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3bEXT =
      (GdkGLProc_glSecondaryColor3bEXT) gdk_gl_get_proc_address ("glSecondaryColor3bEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3bEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3bEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3bEXT);
}

/* glSecondaryColor3bvEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3bvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3bvEXT == (GdkGLProc_glSecondaryColor3bvEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3bvEXT =
      (GdkGLProc_glSecondaryColor3bvEXT) gdk_gl_get_proc_address ("glSecondaryColor3bvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3bvEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3bvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3bvEXT);
}

/* glSecondaryColor3dEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3dEXT == (GdkGLProc_glSecondaryColor3dEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3dEXT =
      (GdkGLProc_glSecondaryColor3dEXT) gdk_gl_get_proc_address ("glSecondaryColor3dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3dEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3dEXT);
}

/* glSecondaryColor3dvEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3dvEXT == (GdkGLProc_glSecondaryColor3dvEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3dvEXT =
      (GdkGLProc_glSecondaryColor3dvEXT) gdk_gl_get_proc_address ("glSecondaryColor3dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3dvEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3dvEXT);
}

/* glSecondaryColor3fEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3fEXT == (GdkGLProc_glSecondaryColor3fEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3fEXT =
      (GdkGLProc_glSecondaryColor3fEXT) gdk_gl_get_proc_address ("glSecondaryColor3fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3fEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3fEXT);
}

/* glSecondaryColor3fvEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3fvEXT == (GdkGLProc_glSecondaryColor3fvEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3fvEXT =
      (GdkGLProc_glSecondaryColor3fvEXT) gdk_gl_get_proc_address ("glSecondaryColor3fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3fvEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3fvEXT);
}

/* glSecondaryColor3iEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3iEXT == (GdkGLProc_glSecondaryColor3iEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3iEXT =
      (GdkGLProc_glSecondaryColor3iEXT) gdk_gl_get_proc_address ("glSecondaryColor3iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3iEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3iEXT);
}

/* glSecondaryColor3ivEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3ivEXT == (GdkGLProc_glSecondaryColor3ivEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3ivEXT =
      (GdkGLProc_glSecondaryColor3ivEXT) gdk_gl_get_proc_address ("glSecondaryColor3ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3ivEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3ivEXT);
}

/* glSecondaryColor3sEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3sEXT == (GdkGLProc_glSecondaryColor3sEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3sEXT =
      (GdkGLProc_glSecondaryColor3sEXT) gdk_gl_get_proc_address ("glSecondaryColor3sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3sEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3sEXT);
}

/* glSecondaryColor3svEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3svEXT == (GdkGLProc_glSecondaryColor3svEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3svEXT =
      (GdkGLProc_glSecondaryColor3svEXT) gdk_gl_get_proc_address ("glSecondaryColor3svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3svEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3svEXT);
}

/* glSecondaryColor3ubEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3ubEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3ubEXT == (GdkGLProc_glSecondaryColor3ubEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3ubEXT =
      (GdkGLProc_glSecondaryColor3ubEXT) gdk_gl_get_proc_address ("glSecondaryColor3ubEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3ubEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3ubEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3ubEXT);
}

/* glSecondaryColor3ubvEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3ubvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3ubvEXT == (GdkGLProc_glSecondaryColor3ubvEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3ubvEXT =
      (GdkGLProc_glSecondaryColor3ubvEXT) gdk_gl_get_proc_address ("glSecondaryColor3ubvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3ubvEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3ubvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3ubvEXT);
}

/* glSecondaryColor3uiEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3uiEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3uiEXT == (GdkGLProc_glSecondaryColor3uiEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3uiEXT =
      (GdkGLProc_glSecondaryColor3uiEXT) gdk_gl_get_proc_address ("glSecondaryColor3uiEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3uiEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3uiEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3uiEXT);
}

/* glSecondaryColor3uivEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3uivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3uivEXT == (GdkGLProc_glSecondaryColor3uivEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3uivEXT =
      (GdkGLProc_glSecondaryColor3uivEXT) gdk_gl_get_proc_address ("glSecondaryColor3uivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3uivEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3uivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3uivEXT);
}

/* glSecondaryColor3usEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3usEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3usEXT == (GdkGLProc_glSecondaryColor3usEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3usEXT =
      (GdkGLProc_glSecondaryColor3usEXT) gdk_gl_get_proc_address ("glSecondaryColor3usEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3usEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3usEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3usEXT);
}

/* glSecondaryColor3usvEXT */
GdkGLProc
gdk_gl_get_glSecondaryColor3usvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColor3usvEXT == (GdkGLProc_glSecondaryColor3usvEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColor3usvEXT =
      (GdkGLProc_glSecondaryColor3usvEXT) gdk_gl_get_proc_address ("glSecondaryColor3usvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3usvEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColor3usvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColor3usvEXT);
}

/* glSecondaryColorPointerEXT */
GdkGLProc
gdk_gl_get_glSecondaryColorPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_secondary_color.glSecondaryColorPointerEXT == (GdkGLProc_glSecondaryColorPointerEXT) -1)
    _procs_GL_EXT_secondary_color.glSecondaryColorPointerEXT =
      (GdkGLProc_glSecondaryColorPointerEXT) gdk_gl_get_proc_address ("glSecondaryColorPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColorPointerEXT () - %s",
               (_procs_GL_EXT_secondary_color.glSecondaryColorPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_secondary_color.glSecondaryColorPointerEXT);
}

/* Get GL_EXT_secondary_color functions */
GdkGL_GL_EXT_secondary_color *
gdk_gl_get_GL_EXT_secondary_color (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_secondary_color");

      if (supported)
        {
          supported &= (gdk_gl_get_glSecondaryColor3bEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3bvEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3dEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3dvEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3fEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3fvEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3iEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3ivEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3sEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3svEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3ubEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3ubvEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3uiEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3uivEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3usEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3usvEXT () != NULL);
          supported &= (gdk_gl_get_glSecondaryColorPointerEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_secondary_color () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_secondary_color;
}

/*
 * GL_EXT_texture_perturb_normal
 */

static GdkGL_GL_EXT_texture_perturb_normal _procs_GL_EXT_texture_perturb_normal = {
  (GdkGLProc_glTextureNormalEXT) -1
};

/* glTextureNormalEXT */
GdkGLProc
gdk_gl_get_glTextureNormalEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_texture_perturb_normal.glTextureNormalEXT == (GdkGLProc_glTextureNormalEXT) -1)
    _procs_GL_EXT_texture_perturb_normal.glTextureNormalEXT =
      (GdkGLProc_glTextureNormalEXT) gdk_gl_get_proc_address ("glTextureNormalEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTextureNormalEXT () - %s",
               (_procs_GL_EXT_texture_perturb_normal.glTextureNormalEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_texture_perturb_normal.glTextureNormalEXT);
}

/* Get GL_EXT_texture_perturb_normal functions */
GdkGL_GL_EXT_texture_perturb_normal *
gdk_gl_get_GL_EXT_texture_perturb_normal (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_texture_perturb_normal");

      if (supported)
        {
          supported &= (gdk_gl_get_glTextureNormalEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_texture_perturb_normal () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_texture_perturb_normal;
}

/*
 * GL_EXT_multi_draw_arrays
 */

static GdkGL_GL_EXT_multi_draw_arrays _procs_GL_EXT_multi_draw_arrays = {
  (GdkGLProc_glMultiDrawArraysEXT) -1,
  (GdkGLProc_glMultiDrawElementsEXT) -1
};

/* glMultiDrawArraysEXT */
GdkGLProc
gdk_gl_get_glMultiDrawArraysEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multi_draw_arrays.glMultiDrawArraysEXT == (GdkGLProc_glMultiDrawArraysEXT) -1)
    _procs_GL_EXT_multi_draw_arrays.glMultiDrawArraysEXT =
      (GdkGLProc_glMultiDrawArraysEXT) gdk_gl_get_proc_address ("glMultiDrawArraysEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawArraysEXT () - %s",
               (_procs_GL_EXT_multi_draw_arrays.glMultiDrawArraysEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multi_draw_arrays.glMultiDrawArraysEXT);
}

/* glMultiDrawElementsEXT */
GdkGLProc
gdk_gl_get_glMultiDrawElementsEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multi_draw_arrays.glMultiDrawElementsEXT == (GdkGLProc_glMultiDrawElementsEXT) -1)
    _procs_GL_EXT_multi_draw_arrays.glMultiDrawElementsEXT =
      (GdkGLProc_glMultiDrawElementsEXT) gdk_gl_get_proc_address ("glMultiDrawElementsEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawElementsEXT () - %s",
               (_procs_GL_EXT_multi_draw_arrays.glMultiDrawElementsEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multi_draw_arrays.glMultiDrawElementsEXT);
}

/* Get GL_EXT_multi_draw_arrays functions */
GdkGL_GL_EXT_multi_draw_arrays *
gdk_gl_get_GL_EXT_multi_draw_arrays (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_multi_draw_arrays");

      if (supported)
        {
          supported &= (gdk_gl_get_glMultiDrawArraysEXT () != NULL);
          supported &= (gdk_gl_get_glMultiDrawElementsEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_multi_draw_arrays () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_multi_draw_arrays;
}

/*
 * GL_EXT_fog_coord
 */

static GdkGL_GL_EXT_fog_coord _procs_GL_EXT_fog_coord = {
  (GdkGLProc_glFogCoordfEXT) -1,
  (GdkGLProc_glFogCoordfvEXT) -1,
  (GdkGLProc_glFogCoorddEXT) -1,
  (GdkGLProc_glFogCoorddvEXT) -1,
  (GdkGLProc_glFogCoordPointerEXT) -1
};

/* glFogCoordfEXT */
GdkGLProc
gdk_gl_get_glFogCoordfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fog_coord.glFogCoordfEXT == (GdkGLProc_glFogCoordfEXT) -1)
    _procs_GL_EXT_fog_coord.glFogCoordfEXT =
      (GdkGLProc_glFogCoordfEXT) gdk_gl_get_proc_address ("glFogCoordfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordfEXT () - %s",
               (_procs_GL_EXT_fog_coord.glFogCoordfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fog_coord.glFogCoordfEXT);
}

/* glFogCoordfvEXT */
GdkGLProc
gdk_gl_get_glFogCoordfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fog_coord.glFogCoordfvEXT == (GdkGLProc_glFogCoordfvEXT) -1)
    _procs_GL_EXT_fog_coord.glFogCoordfvEXT =
      (GdkGLProc_glFogCoordfvEXT) gdk_gl_get_proc_address ("glFogCoordfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordfvEXT () - %s",
               (_procs_GL_EXT_fog_coord.glFogCoordfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fog_coord.glFogCoordfvEXT);
}

/* glFogCoorddEXT */
GdkGLProc
gdk_gl_get_glFogCoorddEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fog_coord.glFogCoorddEXT == (GdkGLProc_glFogCoorddEXT) -1)
    _procs_GL_EXT_fog_coord.glFogCoorddEXT =
      (GdkGLProc_glFogCoorddEXT) gdk_gl_get_proc_address ("glFogCoorddEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoorddEXT () - %s",
               (_procs_GL_EXT_fog_coord.glFogCoorddEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fog_coord.glFogCoorddEXT);
}

/* glFogCoorddvEXT */
GdkGLProc
gdk_gl_get_glFogCoorddvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fog_coord.glFogCoorddvEXT == (GdkGLProc_glFogCoorddvEXT) -1)
    _procs_GL_EXT_fog_coord.glFogCoorddvEXT =
      (GdkGLProc_glFogCoorddvEXT) gdk_gl_get_proc_address ("glFogCoorddvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoorddvEXT () - %s",
               (_procs_GL_EXT_fog_coord.glFogCoorddvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fog_coord.glFogCoorddvEXT);
}

/* glFogCoordPointerEXT */
GdkGLProc
gdk_gl_get_glFogCoordPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fog_coord.glFogCoordPointerEXT == (GdkGLProc_glFogCoordPointerEXT) -1)
    _procs_GL_EXT_fog_coord.glFogCoordPointerEXT =
      (GdkGLProc_glFogCoordPointerEXT) gdk_gl_get_proc_address ("glFogCoordPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordPointerEXT () - %s",
               (_procs_GL_EXT_fog_coord.glFogCoordPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fog_coord.glFogCoordPointerEXT);
}

/* Get GL_EXT_fog_coord functions */
GdkGL_GL_EXT_fog_coord *
gdk_gl_get_GL_EXT_fog_coord (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_fog_coord");

      if (supported)
        {
          supported &= (gdk_gl_get_glFogCoordfEXT () != NULL);
          supported &= (gdk_gl_get_glFogCoordfvEXT () != NULL);
          supported &= (gdk_gl_get_glFogCoorddEXT () != NULL);
          supported &= (gdk_gl_get_glFogCoorddvEXT () != NULL);
          supported &= (gdk_gl_get_glFogCoordPointerEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_fog_coord () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_fog_coord;
}

/*
 * GL_EXT_coordinate_frame
 */

static GdkGL_GL_EXT_coordinate_frame _procs_GL_EXT_coordinate_frame = {
  (GdkGLProc_glTangent3bEXT) -1,
  (GdkGLProc_glTangent3bvEXT) -1,
  (GdkGLProc_glTangent3dEXT) -1,
  (GdkGLProc_glTangent3dvEXT) -1,
  (GdkGLProc_glTangent3fEXT) -1,
  (GdkGLProc_glTangent3fvEXT) -1,
  (GdkGLProc_glTangent3iEXT) -1,
  (GdkGLProc_glTangent3ivEXT) -1,
  (GdkGLProc_glTangent3sEXT) -1,
  (GdkGLProc_glTangent3svEXT) -1,
  (GdkGLProc_glBinormal3bEXT) -1,
  (GdkGLProc_glBinormal3bvEXT) -1,
  (GdkGLProc_glBinormal3dEXT) -1,
  (GdkGLProc_glBinormal3dvEXT) -1,
  (GdkGLProc_glBinormal3fEXT) -1,
  (GdkGLProc_glBinormal3fvEXT) -1,
  (GdkGLProc_glBinormal3iEXT) -1,
  (GdkGLProc_glBinormal3ivEXT) -1,
  (GdkGLProc_glBinormal3sEXT) -1,
  (GdkGLProc_glBinormal3svEXT) -1,
  (GdkGLProc_glTangentPointerEXT) -1,
  (GdkGLProc_glBinormalPointerEXT) -1
};

/* glTangent3bEXT */
GdkGLProc
gdk_gl_get_glTangent3bEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3bEXT == (GdkGLProc_glTangent3bEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3bEXT =
      (GdkGLProc_glTangent3bEXT) gdk_gl_get_proc_address ("glTangent3bEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3bEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3bEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3bEXT);
}

/* glTangent3bvEXT */
GdkGLProc
gdk_gl_get_glTangent3bvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3bvEXT == (GdkGLProc_glTangent3bvEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3bvEXT =
      (GdkGLProc_glTangent3bvEXT) gdk_gl_get_proc_address ("glTangent3bvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3bvEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3bvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3bvEXT);
}

/* glTangent3dEXT */
GdkGLProc
gdk_gl_get_glTangent3dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3dEXT == (GdkGLProc_glTangent3dEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3dEXT =
      (GdkGLProc_glTangent3dEXT) gdk_gl_get_proc_address ("glTangent3dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3dEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3dEXT);
}

/* glTangent3dvEXT */
GdkGLProc
gdk_gl_get_glTangent3dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3dvEXT == (GdkGLProc_glTangent3dvEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3dvEXT =
      (GdkGLProc_glTangent3dvEXT) gdk_gl_get_proc_address ("glTangent3dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3dvEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3dvEXT);
}

/* glTangent3fEXT */
GdkGLProc
gdk_gl_get_glTangent3fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3fEXT == (GdkGLProc_glTangent3fEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3fEXT =
      (GdkGLProc_glTangent3fEXT) gdk_gl_get_proc_address ("glTangent3fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3fEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3fEXT);
}

/* glTangent3fvEXT */
GdkGLProc
gdk_gl_get_glTangent3fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3fvEXT == (GdkGLProc_glTangent3fvEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3fvEXT =
      (GdkGLProc_glTangent3fvEXT) gdk_gl_get_proc_address ("glTangent3fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3fvEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3fvEXT);
}

/* glTangent3iEXT */
GdkGLProc
gdk_gl_get_glTangent3iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3iEXT == (GdkGLProc_glTangent3iEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3iEXT =
      (GdkGLProc_glTangent3iEXT) gdk_gl_get_proc_address ("glTangent3iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3iEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3iEXT);
}

/* glTangent3ivEXT */
GdkGLProc
gdk_gl_get_glTangent3ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3ivEXT == (GdkGLProc_glTangent3ivEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3ivEXT =
      (GdkGLProc_glTangent3ivEXT) gdk_gl_get_proc_address ("glTangent3ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3ivEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3ivEXT);
}

/* glTangent3sEXT */
GdkGLProc
gdk_gl_get_glTangent3sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3sEXT == (GdkGLProc_glTangent3sEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3sEXT =
      (GdkGLProc_glTangent3sEXT) gdk_gl_get_proc_address ("glTangent3sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3sEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3sEXT);
}

/* glTangent3svEXT */
GdkGLProc
gdk_gl_get_glTangent3svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangent3svEXT == (GdkGLProc_glTangent3svEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangent3svEXT =
      (GdkGLProc_glTangent3svEXT) gdk_gl_get_proc_address ("glTangent3svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangent3svEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangent3svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangent3svEXT);
}

/* glBinormal3bEXT */
GdkGLProc
gdk_gl_get_glBinormal3bEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3bEXT == (GdkGLProc_glBinormal3bEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3bEXT =
      (GdkGLProc_glBinormal3bEXT) gdk_gl_get_proc_address ("glBinormal3bEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3bEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3bEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3bEXT);
}

/* glBinormal3bvEXT */
GdkGLProc
gdk_gl_get_glBinormal3bvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3bvEXT == (GdkGLProc_glBinormal3bvEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3bvEXT =
      (GdkGLProc_glBinormal3bvEXT) gdk_gl_get_proc_address ("glBinormal3bvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3bvEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3bvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3bvEXT);
}

/* glBinormal3dEXT */
GdkGLProc
gdk_gl_get_glBinormal3dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3dEXT == (GdkGLProc_glBinormal3dEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3dEXT =
      (GdkGLProc_glBinormal3dEXT) gdk_gl_get_proc_address ("glBinormal3dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3dEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3dEXT);
}

/* glBinormal3dvEXT */
GdkGLProc
gdk_gl_get_glBinormal3dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3dvEXT == (GdkGLProc_glBinormal3dvEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3dvEXT =
      (GdkGLProc_glBinormal3dvEXT) gdk_gl_get_proc_address ("glBinormal3dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3dvEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3dvEXT);
}

/* glBinormal3fEXT */
GdkGLProc
gdk_gl_get_glBinormal3fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3fEXT == (GdkGLProc_glBinormal3fEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3fEXT =
      (GdkGLProc_glBinormal3fEXT) gdk_gl_get_proc_address ("glBinormal3fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3fEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3fEXT);
}

/* glBinormal3fvEXT */
GdkGLProc
gdk_gl_get_glBinormal3fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3fvEXT == (GdkGLProc_glBinormal3fvEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3fvEXT =
      (GdkGLProc_glBinormal3fvEXT) gdk_gl_get_proc_address ("glBinormal3fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3fvEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3fvEXT);
}

/* glBinormal3iEXT */
GdkGLProc
gdk_gl_get_glBinormal3iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3iEXT == (GdkGLProc_glBinormal3iEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3iEXT =
      (GdkGLProc_glBinormal3iEXT) gdk_gl_get_proc_address ("glBinormal3iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3iEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3iEXT);
}

/* glBinormal3ivEXT */
GdkGLProc
gdk_gl_get_glBinormal3ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3ivEXT == (GdkGLProc_glBinormal3ivEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3ivEXT =
      (GdkGLProc_glBinormal3ivEXT) gdk_gl_get_proc_address ("glBinormal3ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3ivEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3ivEXT);
}

/* glBinormal3sEXT */
GdkGLProc
gdk_gl_get_glBinormal3sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3sEXT == (GdkGLProc_glBinormal3sEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3sEXT =
      (GdkGLProc_glBinormal3sEXT) gdk_gl_get_proc_address ("glBinormal3sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3sEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3sEXT);
}

/* glBinormal3svEXT */
GdkGLProc
gdk_gl_get_glBinormal3svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormal3svEXT == (GdkGLProc_glBinormal3svEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormal3svEXT =
      (GdkGLProc_glBinormal3svEXT) gdk_gl_get_proc_address ("glBinormal3svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormal3svEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormal3svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormal3svEXT);
}

/* glTangentPointerEXT */
GdkGLProc
gdk_gl_get_glTangentPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glTangentPointerEXT == (GdkGLProc_glTangentPointerEXT) -1)
    _procs_GL_EXT_coordinate_frame.glTangentPointerEXT =
      (GdkGLProc_glTangentPointerEXT) gdk_gl_get_proc_address ("glTangentPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTangentPointerEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glTangentPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glTangentPointerEXT);
}

/* glBinormalPointerEXT */
GdkGLProc
gdk_gl_get_glBinormalPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_coordinate_frame.glBinormalPointerEXT == (GdkGLProc_glBinormalPointerEXT) -1)
    _procs_GL_EXT_coordinate_frame.glBinormalPointerEXT =
      (GdkGLProc_glBinormalPointerEXT) gdk_gl_get_proc_address ("glBinormalPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBinormalPointerEXT () - %s",
               (_procs_GL_EXT_coordinate_frame.glBinormalPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_coordinate_frame.glBinormalPointerEXT);
}

/* Get GL_EXT_coordinate_frame functions */
GdkGL_GL_EXT_coordinate_frame *
gdk_gl_get_GL_EXT_coordinate_frame (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_coordinate_frame");

      if (supported)
        {
          supported &= (gdk_gl_get_glTangent3bEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3bvEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3dEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3dvEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3fEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3fvEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3iEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3ivEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3sEXT () != NULL);
          supported &= (gdk_gl_get_glTangent3svEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3bEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3bvEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3dEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3dvEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3fEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3fvEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3iEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3ivEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3sEXT () != NULL);
          supported &= (gdk_gl_get_glBinormal3svEXT () != NULL);
          supported &= (gdk_gl_get_glTangentPointerEXT () != NULL);
          supported &= (gdk_gl_get_glBinormalPointerEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_coordinate_frame () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_coordinate_frame;
}

/*
 * GL_SUNX_constant_data
 */

static GdkGL_GL_SUNX_constant_data _procs_GL_SUNX_constant_data = {
  (GdkGLProc_glFinishTextureSUNX) -1
};

/* glFinishTextureSUNX */
GdkGLProc
gdk_gl_get_glFinishTextureSUNX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUNX_constant_data.glFinishTextureSUNX == (GdkGLProc_glFinishTextureSUNX) -1)
    _procs_GL_SUNX_constant_data.glFinishTextureSUNX =
      (GdkGLProc_glFinishTextureSUNX) gdk_gl_get_proc_address ("glFinishTextureSUNX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFinishTextureSUNX () - %s",
               (_procs_GL_SUNX_constant_data.glFinishTextureSUNX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUNX_constant_data.glFinishTextureSUNX);
}

/* Get GL_SUNX_constant_data functions */
GdkGL_GL_SUNX_constant_data *
gdk_gl_get_GL_SUNX_constant_data (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SUNX_constant_data");

      if (supported)
        {
          supported &= (gdk_gl_get_glFinishTextureSUNX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SUNX_constant_data () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SUNX_constant_data;
}

/*
 * GL_SUN_global_alpha
 */

static GdkGL_GL_SUN_global_alpha _procs_GL_SUN_global_alpha = {
  (GdkGLProc_glGlobalAlphaFactorbSUN) -1,
  (GdkGLProc_glGlobalAlphaFactorsSUN) -1,
  (GdkGLProc_glGlobalAlphaFactoriSUN) -1,
  (GdkGLProc_glGlobalAlphaFactorfSUN) -1,
  (GdkGLProc_glGlobalAlphaFactordSUN) -1,
  (GdkGLProc_glGlobalAlphaFactorubSUN) -1,
  (GdkGLProc_glGlobalAlphaFactorusSUN) -1,
  (GdkGLProc_glGlobalAlphaFactoruiSUN) -1
};

/* glGlobalAlphaFactorbSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactorbSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorbSUN == (GdkGLProc_glGlobalAlphaFactorbSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactorbSUN =
      (GdkGLProc_glGlobalAlphaFactorbSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactorbSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactorbSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorbSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorbSUN);
}

/* glGlobalAlphaFactorsSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactorsSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorsSUN == (GdkGLProc_glGlobalAlphaFactorsSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactorsSUN =
      (GdkGLProc_glGlobalAlphaFactorsSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactorsSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactorsSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorsSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorsSUN);
}

/* glGlobalAlphaFactoriSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactoriSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactoriSUN == (GdkGLProc_glGlobalAlphaFactoriSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactoriSUN =
      (GdkGLProc_glGlobalAlphaFactoriSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactoriSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactoriSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactoriSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactoriSUN);
}

/* glGlobalAlphaFactorfSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactorfSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorfSUN == (GdkGLProc_glGlobalAlphaFactorfSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactorfSUN =
      (GdkGLProc_glGlobalAlphaFactorfSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactorfSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactorfSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorfSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorfSUN);
}

/* glGlobalAlphaFactordSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactordSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactordSUN == (GdkGLProc_glGlobalAlphaFactordSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactordSUN =
      (GdkGLProc_glGlobalAlphaFactordSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactordSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactordSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactordSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactordSUN);
}

/* glGlobalAlphaFactorubSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactorubSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorubSUN == (GdkGLProc_glGlobalAlphaFactorubSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactorubSUN =
      (GdkGLProc_glGlobalAlphaFactorubSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactorubSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactorubSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorubSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorubSUN);
}

/* glGlobalAlphaFactorusSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactorusSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorusSUN == (GdkGLProc_glGlobalAlphaFactorusSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactorusSUN =
      (GdkGLProc_glGlobalAlphaFactorusSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactorusSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactorusSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorusSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactorusSUN);
}

/* glGlobalAlphaFactoruiSUN */
GdkGLProc
gdk_gl_get_glGlobalAlphaFactoruiSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_global_alpha.glGlobalAlphaFactoruiSUN == (GdkGLProc_glGlobalAlphaFactoruiSUN) -1)
    _procs_GL_SUN_global_alpha.glGlobalAlphaFactoruiSUN =
      (GdkGLProc_glGlobalAlphaFactoruiSUN) gdk_gl_get_proc_address ("glGlobalAlphaFactoruiSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGlobalAlphaFactoruiSUN () - %s",
               (_procs_GL_SUN_global_alpha.glGlobalAlphaFactoruiSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_global_alpha.glGlobalAlphaFactoruiSUN);
}

/* Get GL_SUN_global_alpha functions */
GdkGL_GL_SUN_global_alpha *
gdk_gl_get_GL_SUN_global_alpha (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SUN_global_alpha");

      if (supported)
        {
          supported &= (gdk_gl_get_glGlobalAlphaFactorbSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactorsSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactoriSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactorfSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactordSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactorubSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactorusSUN () != NULL);
          supported &= (gdk_gl_get_glGlobalAlphaFactoruiSUN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SUN_global_alpha () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SUN_global_alpha;
}

/*
 * GL_SUN_triangle_list
 */

static GdkGL_GL_SUN_triangle_list _procs_GL_SUN_triangle_list = {
  (GdkGLProc_glReplacementCodeuiSUN) -1,
  (GdkGLProc_glReplacementCodeusSUN) -1,
  (GdkGLProc_glReplacementCodeubSUN) -1,
  (GdkGLProc_glReplacementCodeuivSUN) -1,
  (GdkGLProc_glReplacementCodeusvSUN) -1,
  (GdkGLProc_glReplacementCodeubvSUN) -1,
  (GdkGLProc_glReplacementCodePointerSUN) -1
};

/* glReplacementCodeuiSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodeuiSUN == (GdkGLProc_glReplacementCodeuiSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodeuiSUN =
      (GdkGLProc_glReplacementCodeuiSUN) gdk_gl_get_proc_address ("glReplacementCodeuiSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodeuiSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodeuiSUN);
}

/* glReplacementCodeusSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeusSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodeusSUN == (GdkGLProc_glReplacementCodeusSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodeusSUN =
      (GdkGLProc_glReplacementCodeusSUN) gdk_gl_get_proc_address ("glReplacementCodeusSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeusSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodeusSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodeusSUN);
}

/* glReplacementCodeubSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeubSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodeubSUN == (GdkGLProc_glReplacementCodeubSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodeubSUN =
      (GdkGLProc_glReplacementCodeubSUN) gdk_gl_get_proc_address ("glReplacementCodeubSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeubSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodeubSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodeubSUN);
}

/* glReplacementCodeuivSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuivSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodeuivSUN == (GdkGLProc_glReplacementCodeuivSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodeuivSUN =
      (GdkGLProc_glReplacementCodeuivSUN) gdk_gl_get_proc_address ("glReplacementCodeuivSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuivSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodeuivSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodeuivSUN);
}

/* glReplacementCodeusvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeusvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodeusvSUN == (GdkGLProc_glReplacementCodeusvSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodeusvSUN =
      (GdkGLProc_glReplacementCodeusvSUN) gdk_gl_get_proc_address ("glReplacementCodeusvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeusvSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodeusvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodeusvSUN);
}

/* glReplacementCodeubvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeubvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodeubvSUN == (GdkGLProc_glReplacementCodeubvSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodeubvSUN =
      (GdkGLProc_glReplacementCodeubvSUN) gdk_gl_get_proc_address ("glReplacementCodeubvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeubvSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodeubvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodeubvSUN);
}

/* glReplacementCodePointerSUN */
GdkGLProc
gdk_gl_get_glReplacementCodePointerSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_triangle_list.glReplacementCodePointerSUN == (GdkGLProc_glReplacementCodePointerSUN) -1)
    _procs_GL_SUN_triangle_list.glReplacementCodePointerSUN =
      (GdkGLProc_glReplacementCodePointerSUN) gdk_gl_get_proc_address ("glReplacementCodePointerSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodePointerSUN () - %s",
               (_procs_GL_SUN_triangle_list.glReplacementCodePointerSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_triangle_list.glReplacementCodePointerSUN);
}

/* Get GL_SUN_triangle_list functions */
GdkGL_GL_SUN_triangle_list *
gdk_gl_get_GL_SUN_triangle_list (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SUN_triangle_list");

      if (supported)
        {
          supported &= (gdk_gl_get_glReplacementCodeuiSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeusSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeubSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuivSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeusvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeubvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodePointerSUN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SUN_triangle_list () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SUN_triangle_list;
}

/*
 * GL_SUN_vertex
 */

static GdkGL_GL_SUN_vertex _procs_GL_SUN_vertex = {
  (GdkGLProc_glColor4ubVertex2fSUN) -1,
  (GdkGLProc_glColor4ubVertex2fvSUN) -1,
  (GdkGLProc_glColor4ubVertex3fSUN) -1,
  (GdkGLProc_glColor4ubVertex3fvSUN) -1,
  (GdkGLProc_glColor3fVertex3fSUN) -1,
  (GdkGLProc_glColor3fVertex3fvSUN) -1,
  (GdkGLProc_glNormal3fVertex3fSUN) -1,
  (GdkGLProc_glNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glColor4fNormal3fVertex3fSUN) -1,
  (GdkGLProc_glColor4fNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glTexCoord2fVertex3fSUN) -1,
  (GdkGLProc_glTexCoord2fVertex3fvSUN) -1,
  (GdkGLProc_glTexCoord4fVertex4fSUN) -1,
  (GdkGLProc_glTexCoord4fVertex4fvSUN) -1,
  (GdkGLProc_glTexCoord2fColor4ubVertex3fSUN) -1,
  (GdkGLProc_glTexCoord2fColor4ubVertex3fvSUN) -1,
  (GdkGLProc_glTexCoord2fColor3fVertex3fSUN) -1,
  (GdkGLProc_glTexCoord2fColor3fVertex3fvSUN) -1,
  (GdkGLProc_glTexCoord2fNormal3fVertex3fSUN) -1,
  (GdkGLProc_glTexCoord2fNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fSUN) -1,
  (GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fSUN) -1,
  (GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiColor4ubVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiColor4ubVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiColor3fVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiColor3fVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiNormal3fVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) -1,
  (GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) -1,
  (GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) -1
};

/* glColor4ubVertex2fSUN */
GdkGLProc
gdk_gl_get_glColor4ubVertex2fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor4ubVertex2fSUN == (GdkGLProc_glColor4ubVertex2fSUN) -1)
    _procs_GL_SUN_vertex.glColor4ubVertex2fSUN =
      (GdkGLProc_glColor4ubVertex2fSUN) gdk_gl_get_proc_address ("glColor4ubVertex2fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4ubVertex2fSUN () - %s",
               (_procs_GL_SUN_vertex.glColor4ubVertex2fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor4ubVertex2fSUN);
}

/* glColor4ubVertex2fvSUN */
GdkGLProc
gdk_gl_get_glColor4ubVertex2fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor4ubVertex2fvSUN == (GdkGLProc_glColor4ubVertex2fvSUN) -1)
    _procs_GL_SUN_vertex.glColor4ubVertex2fvSUN =
      (GdkGLProc_glColor4ubVertex2fvSUN) gdk_gl_get_proc_address ("glColor4ubVertex2fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4ubVertex2fvSUN () - %s",
               (_procs_GL_SUN_vertex.glColor4ubVertex2fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor4ubVertex2fvSUN);
}

/* glColor4ubVertex3fSUN */
GdkGLProc
gdk_gl_get_glColor4ubVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor4ubVertex3fSUN == (GdkGLProc_glColor4ubVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glColor4ubVertex3fSUN =
      (GdkGLProc_glColor4ubVertex3fSUN) gdk_gl_get_proc_address ("glColor4ubVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4ubVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glColor4ubVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor4ubVertex3fSUN);
}

/* glColor4ubVertex3fvSUN */
GdkGLProc
gdk_gl_get_glColor4ubVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor4ubVertex3fvSUN == (GdkGLProc_glColor4ubVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glColor4ubVertex3fvSUN =
      (GdkGLProc_glColor4ubVertex3fvSUN) gdk_gl_get_proc_address ("glColor4ubVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4ubVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glColor4ubVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor4ubVertex3fvSUN);
}

/* glColor3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glColor3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor3fVertex3fSUN == (GdkGLProc_glColor3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glColor3fVertex3fSUN =
      (GdkGLProc_glColor3fVertex3fSUN) gdk_gl_get_proc_address ("glColor3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glColor3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor3fVertex3fSUN);
}

/* glColor3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glColor3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor3fVertex3fvSUN == (GdkGLProc_glColor3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glColor3fVertex3fvSUN =
      (GdkGLProc_glColor3fVertex3fvSUN) gdk_gl_get_proc_address ("glColor3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glColor3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor3fVertex3fvSUN);
}

/* glNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glNormal3fVertex3fSUN == (GdkGLProc_glNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glNormal3fVertex3fSUN =
      (GdkGLProc_glNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glNormal3fVertex3fSUN);
}

/* glNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glNormal3fVertex3fvSUN == (GdkGLProc_glNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glNormal3fVertex3fvSUN =
      (GdkGLProc_glNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glNormal3fVertex3fvSUN);
}

/* glColor4fNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glColor4fNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor4fNormal3fVertex3fSUN == (GdkGLProc_glColor4fNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glColor4fNormal3fVertex3fSUN =
      (GdkGLProc_glColor4fNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glColor4fNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4fNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glColor4fNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor4fNormal3fVertex3fSUN);
}

/* glColor4fNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glColor4fNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glColor4fNormal3fVertex3fvSUN == (GdkGLProc_glColor4fNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glColor4fNormal3fVertex3fvSUN =
      (GdkGLProc_glColor4fNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glColor4fNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4fNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glColor4fNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glColor4fNormal3fVertex3fvSUN);
}

/* glTexCoord2fVertex3fSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fVertex3fSUN == (GdkGLProc_glTexCoord2fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fVertex3fSUN =
      (GdkGLProc_glTexCoord2fVertex3fSUN) gdk_gl_get_proc_address ("glTexCoord2fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fVertex3fSUN);
}

/* glTexCoord2fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fVertex3fvSUN == (GdkGLProc_glTexCoord2fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fVertex3fvSUN =
      (GdkGLProc_glTexCoord2fVertex3fvSUN) gdk_gl_get_proc_address ("glTexCoord2fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fVertex3fvSUN);
}

/* glTexCoord4fVertex4fSUN */
GdkGLProc
gdk_gl_get_glTexCoord4fVertex4fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord4fVertex4fSUN == (GdkGLProc_glTexCoord4fVertex4fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord4fVertex4fSUN =
      (GdkGLProc_glTexCoord4fVertex4fSUN) gdk_gl_get_proc_address ("glTexCoord4fVertex4fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord4fVertex4fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord4fVertex4fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord4fVertex4fSUN);
}

/* glTexCoord4fVertex4fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord4fVertex4fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord4fVertex4fvSUN == (GdkGLProc_glTexCoord4fVertex4fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord4fVertex4fvSUN =
      (GdkGLProc_glTexCoord4fVertex4fvSUN) gdk_gl_get_proc_address ("glTexCoord4fVertex4fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord4fVertex4fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord4fVertex4fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord4fVertex4fvSUN);
}

/* glTexCoord2fColor4ubVertex3fSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fColor4ubVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fSUN == (GdkGLProc_glTexCoord2fColor4ubVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fSUN =
      (GdkGLProc_glTexCoord2fColor4ubVertex3fSUN) gdk_gl_get_proc_address ("glTexCoord2fColor4ubVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fColor4ubVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fSUN);
}

/* glTexCoord2fColor4ubVertex3fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fColor4ubVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fvSUN == (GdkGLProc_glTexCoord2fColor4ubVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fvSUN =
      (GdkGLProc_glTexCoord2fColor4ubVertex3fvSUN) gdk_gl_get_proc_address ("glTexCoord2fColor4ubVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fColor4ubVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fColor4ubVertex3fvSUN);
}

/* glTexCoord2fColor3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fColor3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fSUN == (GdkGLProc_glTexCoord2fColor3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fSUN =
      (GdkGLProc_glTexCoord2fColor3fVertex3fSUN) gdk_gl_get_proc_address ("glTexCoord2fColor3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fColor3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fSUN);
}

/* glTexCoord2fColor3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fColor3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fvSUN == (GdkGLProc_glTexCoord2fColor3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fvSUN =
      (GdkGLProc_glTexCoord2fColor3fVertex3fvSUN) gdk_gl_get_proc_address ("glTexCoord2fColor3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fColor3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fColor3fVertex3fvSUN);
}

/* glTexCoord2fNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fSUN == (GdkGLProc_glTexCoord2fNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fSUN =
      (GdkGLProc_glTexCoord2fNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glTexCoord2fNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fSUN);
}

/* glTexCoord2fNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fvSUN == (GdkGLProc_glTexCoord2fNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fvSUN =
      (GdkGLProc_glTexCoord2fNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glTexCoord2fNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fNormal3fVertex3fvSUN);
}

/* glTexCoord2fColor4fNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fSUN == (GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fSUN =
      (GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glTexCoord2fColor4fNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fSUN);
}

/* glTexCoord2fColor4fNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fvSUN == (GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fvSUN =
      (GdkGLProc_glTexCoord2fColor4fNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glTexCoord2fColor4fNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord2fColor4fNormal3fVertex3fvSUN);
}

/* glTexCoord4fColor4fNormal3fVertex4fSUN */
GdkGLProc
gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fSUN == (GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fSUN =
      (GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fSUN) gdk_gl_get_proc_address ("glTexCoord4fColor4fNormal3fVertex4fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fSUN);
}

/* glTexCoord4fColor4fNormal3fVertex4fvSUN */
GdkGLProc
gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fvSUN == (GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fvSUN) -1)
    _procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fvSUN =
      (GdkGLProc_glTexCoord4fColor4fNormal3fVertex4fvSUN) gdk_gl_get_proc_address ("glTexCoord4fColor4fNormal3fVertex4fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fvSUN () - %s",
               (_procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glTexCoord4fColor4fNormal3fVertex4fvSUN);
}

/* glReplacementCodeuiVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiVertex3fSUN == (GdkGLProc_glReplacementCodeuiVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiVertex3fSUN);
}

/* glReplacementCodeuiVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiVertex3fvSUN == (GdkGLProc_glReplacementCodeuiVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiVertex3fvSUN);
}

/* glReplacementCodeuiColor4ubVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiColor4ubVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fSUN == (GdkGLProc_glReplacementCodeuiColor4ubVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiColor4ubVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiColor4ubVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiColor4ubVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fSUN);
}

/* glReplacementCodeuiColor4ubVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiColor4ubVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fvSUN == (GdkGLProc_glReplacementCodeuiColor4ubVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiColor4ubVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiColor4ubVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiColor4ubVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiColor4ubVertex3fvSUN);
}

/* glReplacementCodeuiColor3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiColor3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fSUN == (GdkGLProc_glReplacementCodeuiColor3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiColor3fVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiColor3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiColor3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fSUN);
}

/* glReplacementCodeuiColor3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiColor3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fvSUN == (GdkGLProc_glReplacementCodeuiColor3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiColor3fVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiColor3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiColor3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiColor3fVertex3fvSUN);
}

/* glReplacementCodeuiNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fSUN == (GdkGLProc_glReplacementCodeuiNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fSUN);
}

/* glReplacementCodeuiNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fvSUN == (GdkGLProc_glReplacementCodeuiNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiNormal3fVertex3fvSUN);
}

/* glReplacementCodeuiColor4fNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fSUN == (GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiColor4fNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fSUN);
}

/* glReplacementCodeuiColor4fNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fvSUN == (GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiColor4fNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiColor4fNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiColor4fNormal3fVertex3fvSUN);
}

/* glReplacementCodeuiTexCoord2fVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fSUN == (GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiTexCoord2fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fSUN);
}

/* glReplacementCodeuiTexCoord2fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fvSUN == (GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiTexCoord2fVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiTexCoord2fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fVertex3fvSUN);
}

/* glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN == (GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN);
}

/* glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN == (GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN);
}

/* glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN == (GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN =
      (GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) gdk_gl_get_proc_address ("glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN);
}

/* glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
GdkGLProc
gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN == (GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) -1)
    _procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN =
      (GdkGLProc_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) gdk_gl_get_proc_address ("glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN () - %s",
               (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_vertex.glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN);
}

/* Get GL_SUN_vertex functions */
GdkGL_GL_SUN_vertex *
gdk_gl_get_GL_SUN_vertex (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SUN_vertex");

      if (supported)
        {
          supported &= (gdk_gl_get_glColor4ubVertex2fSUN () != NULL);
          supported &= (gdk_gl_get_glColor4ubVertex2fvSUN () != NULL);
          supported &= (gdk_gl_get_glColor4ubVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glColor4ubVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glColor3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glColor3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glColor4fNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glColor4fNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord4fVertex4fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord4fVertex4fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fColor4ubVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fColor4ubVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fColor3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fColor3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord2fColor4fNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fSUN () != NULL);
          supported &= (gdk_gl_get_glTexCoord4fColor4fNormal3fVertex4fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiColor4ubVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiColor4ubVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiColor3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiColor3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiColor4fNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiTexCoord2fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN () != NULL);
          supported &= (gdk_gl_get_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SUN_vertex () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SUN_vertex;
}

/*
 * GL_EXT_blend_func_separate
 */

static GdkGL_GL_EXT_blend_func_separate _procs_GL_EXT_blend_func_separate = {
  (GdkGLProc_glBlendFuncSeparateEXT) -1
};

/* glBlendFuncSeparateEXT */
GdkGLProc
gdk_gl_get_glBlendFuncSeparateEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_blend_func_separate.glBlendFuncSeparateEXT == (GdkGLProc_glBlendFuncSeparateEXT) -1)
    _procs_GL_EXT_blend_func_separate.glBlendFuncSeparateEXT =
      (GdkGLProc_glBlendFuncSeparateEXT) gdk_gl_get_proc_address ("glBlendFuncSeparateEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendFuncSeparateEXT () - %s",
               (_procs_GL_EXT_blend_func_separate.glBlendFuncSeparateEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_blend_func_separate.glBlendFuncSeparateEXT);
}

/* Get GL_EXT_blend_func_separate functions */
GdkGL_GL_EXT_blend_func_separate *
gdk_gl_get_GL_EXT_blend_func_separate (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_blend_func_separate");

      if (supported)
        {
          supported &= (gdk_gl_get_glBlendFuncSeparateEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_blend_func_separate () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_blend_func_separate;
}

/*
 * GL_INGR_blend_func_separate
 */

static GdkGL_GL_INGR_blend_func_separate _procs_GL_INGR_blend_func_separate = {
  (GdkGLProc_glBlendFuncSeparateINGR) -1
};

/* glBlendFuncSeparateINGR */
GdkGLProc
gdk_gl_get_glBlendFuncSeparateINGR (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_INGR_blend_func_separate.glBlendFuncSeparateINGR == (GdkGLProc_glBlendFuncSeparateINGR) -1)
    _procs_GL_INGR_blend_func_separate.glBlendFuncSeparateINGR =
      (GdkGLProc_glBlendFuncSeparateINGR) gdk_gl_get_proc_address ("glBlendFuncSeparateINGR");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendFuncSeparateINGR () - %s",
               (_procs_GL_INGR_blend_func_separate.glBlendFuncSeparateINGR) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_INGR_blend_func_separate.glBlendFuncSeparateINGR);
}

/* Get GL_INGR_blend_func_separate functions */
GdkGL_GL_INGR_blend_func_separate *
gdk_gl_get_GL_INGR_blend_func_separate (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_INGR_blend_func_separate");

      if (supported)
        {
          supported &= (gdk_gl_get_glBlendFuncSeparateINGR () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_INGR_blend_func_separate () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_INGR_blend_func_separate;
}

/*
 * GL_EXT_vertex_weighting
 */

static GdkGL_GL_EXT_vertex_weighting _procs_GL_EXT_vertex_weighting = {
  (GdkGLProc_glVertexWeightfEXT) -1,
  (GdkGLProc_glVertexWeightfvEXT) -1,
  (GdkGLProc_glVertexWeightPointerEXT) -1
};

/* glVertexWeightfEXT */
GdkGLProc
gdk_gl_get_glVertexWeightfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_weighting.glVertexWeightfEXT == (GdkGLProc_glVertexWeightfEXT) -1)
    _procs_GL_EXT_vertex_weighting.glVertexWeightfEXT =
      (GdkGLProc_glVertexWeightfEXT) gdk_gl_get_proc_address ("glVertexWeightfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexWeightfEXT () - %s",
               (_procs_GL_EXT_vertex_weighting.glVertexWeightfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_weighting.glVertexWeightfEXT);
}

/* glVertexWeightfvEXT */
GdkGLProc
gdk_gl_get_glVertexWeightfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_weighting.glVertexWeightfvEXT == (GdkGLProc_glVertexWeightfvEXT) -1)
    _procs_GL_EXT_vertex_weighting.glVertexWeightfvEXT =
      (GdkGLProc_glVertexWeightfvEXT) gdk_gl_get_proc_address ("glVertexWeightfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexWeightfvEXT () - %s",
               (_procs_GL_EXT_vertex_weighting.glVertexWeightfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_weighting.glVertexWeightfvEXT);
}

/* glVertexWeightPointerEXT */
GdkGLProc
gdk_gl_get_glVertexWeightPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_weighting.glVertexWeightPointerEXT == (GdkGLProc_glVertexWeightPointerEXT) -1)
    _procs_GL_EXT_vertex_weighting.glVertexWeightPointerEXT =
      (GdkGLProc_glVertexWeightPointerEXT) gdk_gl_get_proc_address ("glVertexWeightPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexWeightPointerEXT () - %s",
               (_procs_GL_EXT_vertex_weighting.glVertexWeightPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_weighting.glVertexWeightPointerEXT);
}

/* Get GL_EXT_vertex_weighting functions */
GdkGL_GL_EXT_vertex_weighting *
gdk_gl_get_GL_EXT_vertex_weighting (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_vertex_weighting");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertexWeightfEXT () != NULL);
          supported &= (gdk_gl_get_glVertexWeightfvEXT () != NULL);
          supported &= (gdk_gl_get_glVertexWeightPointerEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_vertex_weighting () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_vertex_weighting;
}

/*
 * GL_NV_vertex_array_range
 */

static GdkGL_GL_NV_vertex_array_range _procs_GL_NV_vertex_array_range = {
  (GdkGLProc_glFlushVertexArrayRangeNV) -1,
  (GdkGLProc_glVertexArrayRangeNV) -1
};

/* glFlushVertexArrayRangeNV */
GdkGLProc
gdk_gl_get_glFlushVertexArrayRangeNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_array_range.glFlushVertexArrayRangeNV == (GdkGLProc_glFlushVertexArrayRangeNV) -1)
    _procs_GL_NV_vertex_array_range.glFlushVertexArrayRangeNV =
      (GdkGLProc_glFlushVertexArrayRangeNV) gdk_gl_get_proc_address ("glFlushVertexArrayRangeNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFlushVertexArrayRangeNV () - %s",
               (_procs_GL_NV_vertex_array_range.glFlushVertexArrayRangeNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_array_range.glFlushVertexArrayRangeNV);
}

/* glVertexArrayRangeNV */
GdkGLProc
gdk_gl_get_glVertexArrayRangeNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_array_range.glVertexArrayRangeNV == (GdkGLProc_glVertexArrayRangeNV) -1)
    _procs_GL_NV_vertex_array_range.glVertexArrayRangeNV =
      (GdkGLProc_glVertexArrayRangeNV) gdk_gl_get_proc_address ("glVertexArrayRangeNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexArrayRangeNV () - %s",
               (_procs_GL_NV_vertex_array_range.glVertexArrayRangeNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_array_range.glVertexArrayRangeNV);
}

/* Get GL_NV_vertex_array_range functions */
GdkGL_GL_NV_vertex_array_range *
gdk_gl_get_GL_NV_vertex_array_range (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_vertex_array_range");

      if (supported)
        {
          supported &= (gdk_gl_get_glFlushVertexArrayRangeNV () != NULL);
          supported &= (gdk_gl_get_glVertexArrayRangeNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_vertex_array_range () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_vertex_array_range;
}

/*
 * GL_NV_register_combiners
 */

static GdkGL_GL_NV_register_combiners _procs_GL_NV_register_combiners = {
  (GdkGLProc_glCombinerParameterfvNV) -1,
  (GdkGLProc_glCombinerParameterfNV) -1,
  (GdkGLProc_glCombinerParameterivNV) -1,
  (GdkGLProc_glCombinerParameteriNV) -1,
  (GdkGLProc_glCombinerInputNV) -1,
  (GdkGLProc_glCombinerOutputNV) -1,
  (GdkGLProc_glFinalCombinerInputNV) -1,
  (GdkGLProc_glGetCombinerInputParameterfvNV) -1,
  (GdkGLProc_glGetCombinerInputParameterivNV) -1,
  (GdkGLProc_glGetCombinerOutputParameterfvNV) -1,
  (GdkGLProc_glGetCombinerOutputParameterivNV) -1,
  (GdkGLProc_glGetFinalCombinerInputParameterfvNV) -1,
  (GdkGLProc_glGetFinalCombinerInputParameterivNV) -1
};

/* glCombinerParameterfvNV */
GdkGLProc
gdk_gl_get_glCombinerParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glCombinerParameterfvNV == (GdkGLProc_glCombinerParameterfvNV) -1)
    _procs_GL_NV_register_combiners.glCombinerParameterfvNV =
      (GdkGLProc_glCombinerParameterfvNV) gdk_gl_get_proc_address ("glCombinerParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerParameterfvNV () - %s",
               (_procs_GL_NV_register_combiners.glCombinerParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glCombinerParameterfvNV);
}

/* glCombinerParameterfNV */
GdkGLProc
gdk_gl_get_glCombinerParameterfNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glCombinerParameterfNV == (GdkGLProc_glCombinerParameterfNV) -1)
    _procs_GL_NV_register_combiners.glCombinerParameterfNV =
      (GdkGLProc_glCombinerParameterfNV) gdk_gl_get_proc_address ("glCombinerParameterfNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerParameterfNV () - %s",
               (_procs_GL_NV_register_combiners.glCombinerParameterfNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glCombinerParameterfNV);
}

/* glCombinerParameterivNV */
GdkGLProc
gdk_gl_get_glCombinerParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glCombinerParameterivNV == (GdkGLProc_glCombinerParameterivNV) -1)
    _procs_GL_NV_register_combiners.glCombinerParameterivNV =
      (GdkGLProc_glCombinerParameterivNV) gdk_gl_get_proc_address ("glCombinerParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerParameterivNV () - %s",
               (_procs_GL_NV_register_combiners.glCombinerParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glCombinerParameterivNV);
}

/* glCombinerParameteriNV */
GdkGLProc
gdk_gl_get_glCombinerParameteriNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glCombinerParameteriNV == (GdkGLProc_glCombinerParameteriNV) -1)
    _procs_GL_NV_register_combiners.glCombinerParameteriNV =
      (GdkGLProc_glCombinerParameteriNV) gdk_gl_get_proc_address ("glCombinerParameteriNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerParameteriNV () - %s",
               (_procs_GL_NV_register_combiners.glCombinerParameteriNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glCombinerParameteriNV);
}

/* glCombinerInputNV */
GdkGLProc
gdk_gl_get_glCombinerInputNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glCombinerInputNV == (GdkGLProc_glCombinerInputNV) -1)
    _procs_GL_NV_register_combiners.glCombinerInputNV =
      (GdkGLProc_glCombinerInputNV) gdk_gl_get_proc_address ("glCombinerInputNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerInputNV () - %s",
               (_procs_GL_NV_register_combiners.glCombinerInputNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glCombinerInputNV);
}

/* glCombinerOutputNV */
GdkGLProc
gdk_gl_get_glCombinerOutputNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glCombinerOutputNV == (GdkGLProc_glCombinerOutputNV) -1)
    _procs_GL_NV_register_combiners.glCombinerOutputNV =
      (GdkGLProc_glCombinerOutputNV) gdk_gl_get_proc_address ("glCombinerOutputNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerOutputNV () - %s",
               (_procs_GL_NV_register_combiners.glCombinerOutputNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glCombinerOutputNV);
}

/* glFinalCombinerInputNV */
GdkGLProc
gdk_gl_get_glFinalCombinerInputNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glFinalCombinerInputNV == (GdkGLProc_glFinalCombinerInputNV) -1)
    _procs_GL_NV_register_combiners.glFinalCombinerInputNV =
      (GdkGLProc_glFinalCombinerInputNV) gdk_gl_get_proc_address ("glFinalCombinerInputNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFinalCombinerInputNV () - %s",
               (_procs_GL_NV_register_combiners.glFinalCombinerInputNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glFinalCombinerInputNV);
}

/* glGetCombinerInputParameterfvNV */
GdkGLProc
gdk_gl_get_glGetCombinerInputParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glGetCombinerInputParameterfvNV == (GdkGLProc_glGetCombinerInputParameterfvNV) -1)
    _procs_GL_NV_register_combiners.glGetCombinerInputParameterfvNV =
      (GdkGLProc_glGetCombinerInputParameterfvNV) gdk_gl_get_proc_address ("glGetCombinerInputParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCombinerInputParameterfvNV () - %s",
               (_procs_GL_NV_register_combiners.glGetCombinerInputParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glGetCombinerInputParameterfvNV);
}

/* glGetCombinerInputParameterivNV */
GdkGLProc
gdk_gl_get_glGetCombinerInputParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glGetCombinerInputParameterivNV == (GdkGLProc_glGetCombinerInputParameterivNV) -1)
    _procs_GL_NV_register_combiners.glGetCombinerInputParameterivNV =
      (GdkGLProc_glGetCombinerInputParameterivNV) gdk_gl_get_proc_address ("glGetCombinerInputParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCombinerInputParameterivNV () - %s",
               (_procs_GL_NV_register_combiners.glGetCombinerInputParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glGetCombinerInputParameterivNV);
}

/* glGetCombinerOutputParameterfvNV */
GdkGLProc
gdk_gl_get_glGetCombinerOutputParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glGetCombinerOutputParameterfvNV == (GdkGLProc_glGetCombinerOutputParameterfvNV) -1)
    _procs_GL_NV_register_combiners.glGetCombinerOutputParameterfvNV =
      (GdkGLProc_glGetCombinerOutputParameterfvNV) gdk_gl_get_proc_address ("glGetCombinerOutputParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCombinerOutputParameterfvNV () - %s",
               (_procs_GL_NV_register_combiners.glGetCombinerOutputParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glGetCombinerOutputParameterfvNV);
}

/* glGetCombinerOutputParameterivNV */
GdkGLProc
gdk_gl_get_glGetCombinerOutputParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glGetCombinerOutputParameterivNV == (GdkGLProc_glGetCombinerOutputParameterivNV) -1)
    _procs_GL_NV_register_combiners.glGetCombinerOutputParameterivNV =
      (GdkGLProc_glGetCombinerOutputParameterivNV) gdk_gl_get_proc_address ("glGetCombinerOutputParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCombinerOutputParameterivNV () - %s",
               (_procs_GL_NV_register_combiners.glGetCombinerOutputParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glGetCombinerOutputParameterivNV);
}

/* glGetFinalCombinerInputParameterfvNV */
GdkGLProc
gdk_gl_get_glGetFinalCombinerInputParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterfvNV == (GdkGLProc_glGetFinalCombinerInputParameterfvNV) -1)
    _procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterfvNV =
      (GdkGLProc_glGetFinalCombinerInputParameterfvNV) gdk_gl_get_proc_address ("glGetFinalCombinerInputParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFinalCombinerInputParameterfvNV () - %s",
               (_procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterfvNV);
}

/* glGetFinalCombinerInputParameterivNV */
GdkGLProc
gdk_gl_get_glGetFinalCombinerInputParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterivNV == (GdkGLProc_glGetFinalCombinerInputParameterivNV) -1)
    _procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterivNV =
      (GdkGLProc_glGetFinalCombinerInputParameterivNV) gdk_gl_get_proc_address ("glGetFinalCombinerInputParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFinalCombinerInputParameterivNV () - %s",
               (_procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners.glGetFinalCombinerInputParameterivNV);
}

/* Get GL_NV_register_combiners functions */
GdkGL_GL_NV_register_combiners *
gdk_gl_get_GL_NV_register_combiners (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_register_combiners");

      if (supported)
        {
          supported &= (gdk_gl_get_glCombinerParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glCombinerParameterfNV () != NULL);
          supported &= (gdk_gl_get_glCombinerParameterivNV () != NULL);
          supported &= (gdk_gl_get_glCombinerParameteriNV () != NULL);
          supported &= (gdk_gl_get_glCombinerInputNV () != NULL);
          supported &= (gdk_gl_get_glCombinerOutputNV () != NULL);
          supported &= (gdk_gl_get_glFinalCombinerInputNV () != NULL);
          supported &= (gdk_gl_get_glGetCombinerInputParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetCombinerInputParameterivNV () != NULL);
          supported &= (gdk_gl_get_glGetCombinerOutputParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetCombinerOutputParameterivNV () != NULL);
          supported &= (gdk_gl_get_glGetFinalCombinerInputParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetFinalCombinerInputParameterivNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_register_combiners () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_register_combiners;
}

/*
 * GL_MESA_resize_buffers
 */

static GdkGL_GL_MESA_resize_buffers _procs_GL_MESA_resize_buffers = {
  (GdkGLProc_glResizeBuffersMESA) -1
};

/* glResizeBuffersMESA */
GdkGLProc
gdk_gl_get_glResizeBuffersMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_resize_buffers.glResizeBuffersMESA == (GdkGLProc_glResizeBuffersMESA) -1)
    _procs_GL_MESA_resize_buffers.glResizeBuffersMESA =
      (GdkGLProc_glResizeBuffersMESA) gdk_gl_get_proc_address ("glResizeBuffersMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glResizeBuffersMESA () - %s",
               (_procs_GL_MESA_resize_buffers.glResizeBuffersMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_resize_buffers.glResizeBuffersMESA);
}

/* Get GL_MESA_resize_buffers functions */
GdkGL_GL_MESA_resize_buffers *
gdk_gl_get_GL_MESA_resize_buffers (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_MESA_resize_buffers");

      if (supported)
        {
          supported &= (gdk_gl_get_glResizeBuffersMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_MESA_resize_buffers () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_MESA_resize_buffers;
}

/*
 * GL_MESA_window_pos
 */

static GdkGL_GL_MESA_window_pos _procs_GL_MESA_window_pos = {
  (GdkGLProc_glWindowPos2dMESA) -1,
  (GdkGLProc_glWindowPos2dvMESA) -1,
  (GdkGLProc_glWindowPos2fMESA) -1,
  (GdkGLProc_glWindowPos2fvMESA) -1,
  (GdkGLProc_glWindowPos2iMESA) -1,
  (GdkGLProc_glWindowPos2ivMESA) -1,
  (GdkGLProc_glWindowPos2sMESA) -1,
  (GdkGLProc_glWindowPos2svMESA) -1,
  (GdkGLProc_glWindowPos3dMESA) -1,
  (GdkGLProc_glWindowPos3dvMESA) -1,
  (GdkGLProc_glWindowPos3fMESA) -1,
  (GdkGLProc_glWindowPos3fvMESA) -1,
  (GdkGLProc_glWindowPos3iMESA) -1,
  (GdkGLProc_glWindowPos3ivMESA) -1,
  (GdkGLProc_glWindowPos3sMESA) -1,
  (GdkGLProc_glWindowPos3svMESA) -1,
  (GdkGLProc_glWindowPos4dMESA) -1,
  (GdkGLProc_glWindowPos4dvMESA) -1,
  (GdkGLProc_glWindowPos4fMESA) -1,
  (GdkGLProc_glWindowPos4fvMESA) -1,
  (GdkGLProc_glWindowPos4iMESA) -1,
  (GdkGLProc_glWindowPos4ivMESA) -1,
  (GdkGLProc_glWindowPos4sMESA) -1,
  (GdkGLProc_glWindowPos4svMESA) -1
};

/* glWindowPos2dMESA */
GdkGLProc
gdk_gl_get_glWindowPos2dMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2dMESA == (GdkGLProc_glWindowPos2dMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2dMESA =
      (GdkGLProc_glWindowPos2dMESA) gdk_gl_get_proc_address ("glWindowPos2dMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2dMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2dMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2dMESA);
}

/* glWindowPos2dvMESA */
GdkGLProc
gdk_gl_get_glWindowPos2dvMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2dvMESA == (GdkGLProc_glWindowPos2dvMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2dvMESA =
      (GdkGLProc_glWindowPos2dvMESA) gdk_gl_get_proc_address ("glWindowPos2dvMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2dvMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2dvMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2dvMESA);
}

/* glWindowPos2fMESA */
GdkGLProc
gdk_gl_get_glWindowPos2fMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2fMESA == (GdkGLProc_glWindowPos2fMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2fMESA =
      (GdkGLProc_glWindowPos2fMESA) gdk_gl_get_proc_address ("glWindowPos2fMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2fMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2fMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2fMESA);
}

/* glWindowPos2fvMESA */
GdkGLProc
gdk_gl_get_glWindowPos2fvMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2fvMESA == (GdkGLProc_glWindowPos2fvMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2fvMESA =
      (GdkGLProc_glWindowPos2fvMESA) gdk_gl_get_proc_address ("glWindowPos2fvMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2fvMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2fvMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2fvMESA);
}

/* glWindowPos2iMESA */
GdkGLProc
gdk_gl_get_glWindowPos2iMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2iMESA == (GdkGLProc_glWindowPos2iMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2iMESA =
      (GdkGLProc_glWindowPos2iMESA) gdk_gl_get_proc_address ("glWindowPos2iMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2iMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2iMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2iMESA);
}

/* glWindowPos2ivMESA */
GdkGLProc
gdk_gl_get_glWindowPos2ivMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2ivMESA == (GdkGLProc_glWindowPos2ivMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2ivMESA =
      (GdkGLProc_glWindowPos2ivMESA) gdk_gl_get_proc_address ("glWindowPos2ivMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2ivMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2ivMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2ivMESA);
}

/* glWindowPos2sMESA */
GdkGLProc
gdk_gl_get_glWindowPos2sMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2sMESA == (GdkGLProc_glWindowPos2sMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2sMESA =
      (GdkGLProc_glWindowPos2sMESA) gdk_gl_get_proc_address ("glWindowPos2sMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2sMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2sMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2sMESA);
}

/* glWindowPos2svMESA */
GdkGLProc
gdk_gl_get_glWindowPos2svMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos2svMESA == (GdkGLProc_glWindowPos2svMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos2svMESA =
      (GdkGLProc_glWindowPos2svMESA) gdk_gl_get_proc_address ("glWindowPos2svMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos2svMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos2svMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos2svMESA);
}

/* glWindowPos3dMESA */
GdkGLProc
gdk_gl_get_glWindowPos3dMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3dMESA == (GdkGLProc_glWindowPos3dMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3dMESA =
      (GdkGLProc_glWindowPos3dMESA) gdk_gl_get_proc_address ("glWindowPos3dMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3dMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3dMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3dMESA);
}

/* glWindowPos3dvMESA */
GdkGLProc
gdk_gl_get_glWindowPos3dvMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3dvMESA == (GdkGLProc_glWindowPos3dvMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3dvMESA =
      (GdkGLProc_glWindowPos3dvMESA) gdk_gl_get_proc_address ("glWindowPos3dvMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3dvMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3dvMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3dvMESA);
}

/* glWindowPos3fMESA */
GdkGLProc
gdk_gl_get_glWindowPos3fMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3fMESA == (GdkGLProc_glWindowPos3fMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3fMESA =
      (GdkGLProc_glWindowPos3fMESA) gdk_gl_get_proc_address ("glWindowPos3fMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3fMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3fMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3fMESA);
}

/* glWindowPos3fvMESA */
GdkGLProc
gdk_gl_get_glWindowPos3fvMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3fvMESA == (GdkGLProc_glWindowPos3fvMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3fvMESA =
      (GdkGLProc_glWindowPos3fvMESA) gdk_gl_get_proc_address ("glWindowPos3fvMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3fvMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3fvMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3fvMESA);
}

/* glWindowPos3iMESA */
GdkGLProc
gdk_gl_get_glWindowPos3iMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3iMESA == (GdkGLProc_glWindowPos3iMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3iMESA =
      (GdkGLProc_glWindowPos3iMESA) gdk_gl_get_proc_address ("glWindowPos3iMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3iMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3iMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3iMESA);
}

/* glWindowPos3ivMESA */
GdkGLProc
gdk_gl_get_glWindowPos3ivMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3ivMESA == (GdkGLProc_glWindowPos3ivMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3ivMESA =
      (GdkGLProc_glWindowPos3ivMESA) gdk_gl_get_proc_address ("glWindowPos3ivMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3ivMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3ivMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3ivMESA);
}

/* glWindowPos3sMESA */
GdkGLProc
gdk_gl_get_glWindowPos3sMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3sMESA == (GdkGLProc_glWindowPos3sMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3sMESA =
      (GdkGLProc_glWindowPos3sMESA) gdk_gl_get_proc_address ("glWindowPos3sMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3sMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3sMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3sMESA);
}

/* glWindowPos3svMESA */
GdkGLProc
gdk_gl_get_glWindowPos3svMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos3svMESA == (GdkGLProc_glWindowPos3svMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos3svMESA =
      (GdkGLProc_glWindowPos3svMESA) gdk_gl_get_proc_address ("glWindowPos3svMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos3svMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos3svMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos3svMESA);
}

/* glWindowPos4dMESA */
GdkGLProc
gdk_gl_get_glWindowPos4dMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4dMESA == (GdkGLProc_glWindowPos4dMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4dMESA =
      (GdkGLProc_glWindowPos4dMESA) gdk_gl_get_proc_address ("glWindowPos4dMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4dMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4dMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4dMESA);
}

/* glWindowPos4dvMESA */
GdkGLProc
gdk_gl_get_glWindowPos4dvMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4dvMESA == (GdkGLProc_glWindowPos4dvMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4dvMESA =
      (GdkGLProc_glWindowPos4dvMESA) gdk_gl_get_proc_address ("glWindowPos4dvMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4dvMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4dvMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4dvMESA);
}

/* glWindowPos4fMESA */
GdkGLProc
gdk_gl_get_glWindowPos4fMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4fMESA == (GdkGLProc_glWindowPos4fMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4fMESA =
      (GdkGLProc_glWindowPos4fMESA) gdk_gl_get_proc_address ("glWindowPos4fMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4fMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4fMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4fMESA);
}

/* glWindowPos4fvMESA */
GdkGLProc
gdk_gl_get_glWindowPos4fvMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4fvMESA == (GdkGLProc_glWindowPos4fvMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4fvMESA =
      (GdkGLProc_glWindowPos4fvMESA) gdk_gl_get_proc_address ("glWindowPos4fvMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4fvMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4fvMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4fvMESA);
}

/* glWindowPos4iMESA */
GdkGLProc
gdk_gl_get_glWindowPos4iMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4iMESA == (GdkGLProc_glWindowPos4iMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4iMESA =
      (GdkGLProc_glWindowPos4iMESA) gdk_gl_get_proc_address ("glWindowPos4iMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4iMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4iMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4iMESA);
}

/* glWindowPos4ivMESA */
GdkGLProc
gdk_gl_get_glWindowPos4ivMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4ivMESA == (GdkGLProc_glWindowPos4ivMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4ivMESA =
      (GdkGLProc_glWindowPos4ivMESA) gdk_gl_get_proc_address ("glWindowPos4ivMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4ivMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4ivMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4ivMESA);
}

/* glWindowPos4sMESA */
GdkGLProc
gdk_gl_get_glWindowPos4sMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4sMESA == (GdkGLProc_glWindowPos4sMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4sMESA =
      (GdkGLProc_glWindowPos4sMESA) gdk_gl_get_proc_address ("glWindowPos4sMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4sMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4sMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4sMESA);
}

/* glWindowPos4svMESA */
GdkGLProc
gdk_gl_get_glWindowPos4svMESA (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_MESA_window_pos.glWindowPos4svMESA == (GdkGLProc_glWindowPos4svMESA) -1)
    _procs_GL_MESA_window_pos.glWindowPos4svMESA =
      (GdkGLProc_glWindowPos4svMESA) gdk_gl_get_proc_address ("glWindowPos4svMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowPos4svMESA () - %s",
               (_procs_GL_MESA_window_pos.glWindowPos4svMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_MESA_window_pos.glWindowPos4svMESA);
}

/* Get GL_MESA_window_pos functions */
GdkGL_GL_MESA_window_pos *
gdk_gl_get_GL_MESA_window_pos (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_MESA_window_pos");

      if (supported)
        {
          supported &= (gdk_gl_get_glWindowPos2dMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2dvMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2fMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2fvMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2iMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2ivMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2sMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos2svMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3dMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3dvMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3fMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3fvMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3iMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3ivMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3sMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos3svMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4dMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4dvMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4fMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4fvMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4iMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4ivMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4sMESA () != NULL);
          supported &= (gdk_gl_get_glWindowPos4svMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_MESA_window_pos () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_MESA_window_pos;
}

/*
 * GL_IBM_multimode_draw_arrays
 */

static GdkGL_GL_IBM_multimode_draw_arrays _procs_GL_IBM_multimode_draw_arrays = {
  (GdkGLProc_glMultiModeDrawArraysIBM) -1,
  (GdkGLProc_glMultiModeDrawElementsIBM) -1
};

/* glMultiModeDrawArraysIBM */
GdkGLProc
gdk_gl_get_glMultiModeDrawArraysIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawArraysIBM == (GdkGLProc_glMultiModeDrawArraysIBM) -1)
    _procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawArraysIBM =
      (GdkGLProc_glMultiModeDrawArraysIBM) gdk_gl_get_proc_address ("glMultiModeDrawArraysIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiModeDrawArraysIBM () - %s",
               (_procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawArraysIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawArraysIBM);
}

/* glMultiModeDrawElementsIBM */
GdkGLProc
gdk_gl_get_glMultiModeDrawElementsIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawElementsIBM == (GdkGLProc_glMultiModeDrawElementsIBM) -1)
    _procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawElementsIBM =
      (GdkGLProc_glMultiModeDrawElementsIBM) gdk_gl_get_proc_address ("glMultiModeDrawElementsIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiModeDrawElementsIBM () - %s",
               (_procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawElementsIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_multimode_draw_arrays.glMultiModeDrawElementsIBM);
}

/* Get GL_IBM_multimode_draw_arrays functions */
GdkGL_GL_IBM_multimode_draw_arrays *
gdk_gl_get_GL_IBM_multimode_draw_arrays (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_IBM_multimode_draw_arrays");

      if (supported)
        {
          supported &= (gdk_gl_get_glMultiModeDrawArraysIBM () != NULL);
          supported &= (gdk_gl_get_glMultiModeDrawElementsIBM () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_IBM_multimode_draw_arrays () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_IBM_multimode_draw_arrays;
}

/*
 * GL_IBM_vertex_array_lists
 */

static GdkGL_GL_IBM_vertex_array_lists _procs_GL_IBM_vertex_array_lists = {
  (GdkGLProc_glColorPointerListIBM) -1,
  (GdkGLProc_glSecondaryColorPointerListIBM) -1,
  (GdkGLProc_glEdgeFlagPointerListIBM) -1,
  (GdkGLProc_glFogCoordPointerListIBM) -1,
  (GdkGLProc_glIndexPointerListIBM) -1,
  (GdkGLProc_glNormalPointerListIBM) -1,
  (GdkGLProc_glTexCoordPointerListIBM) -1,
  (GdkGLProc_glVertexPointerListIBM) -1
};

/* glColorPointerListIBM */
GdkGLProc
gdk_gl_get_glColorPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glColorPointerListIBM == (GdkGLProc_glColorPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glColorPointerListIBM =
      (GdkGLProc_glColorPointerListIBM) gdk_gl_get_proc_address ("glColorPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glColorPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glColorPointerListIBM);
}

/* glSecondaryColorPointerListIBM */
GdkGLProc
gdk_gl_get_glSecondaryColorPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glSecondaryColorPointerListIBM == (GdkGLProc_glSecondaryColorPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glSecondaryColorPointerListIBM =
      (GdkGLProc_glSecondaryColorPointerListIBM) gdk_gl_get_proc_address ("glSecondaryColorPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColorPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glSecondaryColorPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glSecondaryColorPointerListIBM);
}

/* glEdgeFlagPointerListIBM */
GdkGLProc
gdk_gl_get_glEdgeFlagPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glEdgeFlagPointerListIBM == (GdkGLProc_glEdgeFlagPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glEdgeFlagPointerListIBM =
      (GdkGLProc_glEdgeFlagPointerListIBM) gdk_gl_get_proc_address ("glEdgeFlagPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEdgeFlagPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glEdgeFlagPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glEdgeFlagPointerListIBM);
}

/* glFogCoordPointerListIBM */
GdkGLProc
gdk_gl_get_glFogCoordPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glFogCoordPointerListIBM == (GdkGLProc_glFogCoordPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glFogCoordPointerListIBM =
      (GdkGLProc_glFogCoordPointerListIBM) gdk_gl_get_proc_address ("glFogCoordPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glFogCoordPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glFogCoordPointerListIBM);
}

/* glIndexPointerListIBM */
GdkGLProc
gdk_gl_get_glIndexPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glIndexPointerListIBM == (GdkGLProc_glIndexPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glIndexPointerListIBM =
      (GdkGLProc_glIndexPointerListIBM) gdk_gl_get_proc_address ("glIndexPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIndexPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glIndexPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glIndexPointerListIBM);
}

/* glNormalPointerListIBM */
GdkGLProc
gdk_gl_get_glNormalPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glNormalPointerListIBM == (GdkGLProc_glNormalPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glNormalPointerListIBM =
      (GdkGLProc_glNormalPointerListIBM) gdk_gl_get_proc_address ("glNormalPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glNormalPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glNormalPointerListIBM);
}

/* glTexCoordPointerListIBM */
GdkGLProc
gdk_gl_get_glTexCoordPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glTexCoordPointerListIBM == (GdkGLProc_glTexCoordPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glTexCoordPointerListIBM =
      (GdkGLProc_glTexCoordPointerListIBM) gdk_gl_get_proc_address ("glTexCoordPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoordPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glTexCoordPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glTexCoordPointerListIBM);
}

/* glVertexPointerListIBM */
GdkGLProc
gdk_gl_get_glVertexPointerListIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_vertex_array_lists.glVertexPointerListIBM == (GdkGLProc_glVertexPointerListIBM) -1)
    _procs_GL_IBM_vertex_array_lists.glVertexPointerListIBM =
      (GdkGLProc_glVertexPointerListIBM) gdk_gl_get_proc_address ("glVertexPointerListIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexPointerListIBM () - %s",
               (_procs_GL_IBM_vertex_array_lists.glVertexPointerListIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_vertex_array_lists.glVertexPointerListIBM);
}

/* Get GL_IBM_vertex_array_lists functions */
GdkGL_GL_IBM_vertex_array_lists *
gdk_gl_get_GL_IBM_vertex_array_lists (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_IBM_vertex_array_lists");

      if (supported)
        {
          supported &= (gdk_gl_get_glColorPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glSecondaryColorPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glEdgeFlagPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glFogCoordPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glIndexPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glNormalPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glTexCoordPointerListIBM () != NULL);
          supported &= (gdk_gl_get_glVertexPointerListIBM () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_IBM_vertex_array_lists () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_IBM_vertex_array_lists;
}

/*
 * GL_3DFX_tbuffer
 */

static GdkGL_GL_3DFX_tbuffer _procs_GL_3DFX_tbuffer = {
  (GdkGLProc_glTbufferMask3DFX) -1
};

/* glTbufferMask3DFX */
GdkGLProc
gdk_gl_get_glTbufferMask3DFX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_3DFX_tbuffer.glTbufferMask3DFX == (GdkGLProc_glTbufferMask3DFX) -1)
    _procs_GL_3DFX_tbuffer.glTbufferMask3DFX =
      (GdkGLProc_glTbufferMask3DFX) gdk_gl_get_proc_address ("glTbufferMask3DFX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTbufferMask3DFX () - %s",
               (_procs_GL_3DFX_tbuffer.glTbufferMask3DFX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_3DFX_tbuffer.glTbufferMask3DFX);
}

/* Get GL_3DFX_tbuffer functions */
GdkGL_GL_3DFX_tbuffer *
gdk_gl_get_GL_3DFX_tbuffer (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_3DFX_tbuffer");

      if (supported)
        {
          supported &= (gdk_gl_get_glTbufferMask3DFX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_3DFX_tbuffer () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_3DFX_tbuffer;
}

/*
 * GL_EXT_multisample
 */

static GdkGL_GL_EXT_multisample _procs_GL_EXT_multisample = {
  (GdkGLProc_glSampleMaskEXT) -1,
  (GdkGLProc_glSamplePatternEXT) -1
};

/* glSampleMaskEXT */
GdkGLProc
gdk_gl_get_glSampleMaskEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multisample.glSampleMaskEXT == (GdkGLProc_glSampleMaskEXT) -1)
    _procs_GL_EXT_multisample.glSampleMaskEXT =
      (GdkGLProc_glSampleMaskEXT) gdk_gl_get_proc_address ("glSampleMaskEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSampleMaskEXT () - %s",
               (_procs_GL_EXT_multisample.glSampleMaskEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multisample.glSampleMaskEXT);
}

/* glSamplePatternEXT */
GdkGLProc
gdk_gl_get_glSamplePatternEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multisample.glSamplePatternEXT == (GdkGLProc_glSamplePatternEXT) -1)
    _procs_GL_EXT_multisample.glSamplePatternEXT =
      (GdkGLProc_glSamplePatternEXT) gdk_gl_get_proc_address ("glSamplePatternEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSamplePatternEXT () - %s",
               (_procs_GL_EXT_multisample.glSamplePatternEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multisample.glSamplePatternEXT);
}

/* Get GL_EXT_multisample functions */
GdkGL_GL_EXT_multisample *
gdk_gl_get_GL_EXT_multisample (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_multisample");

      if (supported)
        {
          supported &= (gdk_gl_get_glSampleMaskEXT () != NULL);
          supported &= (gdk_gl_get_glSamplePatternEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_multisample () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_multisample;
}

/*
 * GL_SGIS_texture_color_mask
 */

static GdkGL_GL_SGIS_texture_color_mask _procs_GL_SGIS_texture_color_mask = {
  (GdkGLProc_glTextureColorMaskSGIS) -1
};

/* glTextureColorMaskSGIS */
GdkGLProc
gdk_gl_get_glTextureColorMaskSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_texture_color_mask.glTextureColorMaskSGIS == (GdkGLProc_glTextureColorMaskSGIS) -1)
    _procs_GL_SGIS_texture_color_mask.glTextureColorMaskSGIS =
      (GdkGLProc_glTextureColorMaskSGIS) gdk_gl_get_proc_address ("glTextureColorMaskSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTextureColorMaskSGIS () - %s",
               (_procs_GL_SGIS_texture_color_mask.glTextureColorMaskSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_texture_color_mask.glTextureColorMaskSGIS);
}

/* Get GL_SGIS_texture_color_mask functions */
GdkGL_GL_SGIS_texture_color_mask *
gdk_gl_get_GL_SGIS_texture_color_mask (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_texture_color_mask");

      if (supported)
        {
          supported &= (gdk_gl_get_glTextureColorMaskSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_texture_color_mask () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_texture_color_mask;
}

/*
 * GL_SGIX_igloo_interface
 */

static GdkGL_GL_SGIX_igloo_interface _procs_GL_SGIX_igloo_interface = {
  (GdkGLProc_glIglooInterfaceSGIX) -1
};

/* glIglooInterfaceSGIX */
GdkGLProc
gdk_gl_get_glIglooInterfaceSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_igloo_interface.glIglooInterfaceSGIX == (GdkGLProc_glIglooInterfaceSGIX) -1)
    _procs_GL_SGIX_igloo_interface.glIglooInterfaceSGIX =
      (GdkGLProc_glIglooInterfaceSGIX) gdk_gl_get_proc_address ("glIglooInterfaceSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIglooInterfaceSGIX () - %s",
               (_procs_GL_SGIX_igloo_interface.glIglooInterfaceSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_igloo_interface.glIglooInterfaceSGIX);
}

/* Get GL_SGIX_igloo_interface functions */
GdkGL_GL_SGIX_igloo_interface *
gdk_gl_get_GL_SGIX_igloo_interface (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_igloo_interface");

      if (supported)
        {
          supported &= (gdk_gl_get_glIglooInterfaceSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_igloo_interface () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_igloo_interface;
}

/*
 * GL_NV_fence
 */

static GdkGL_GL_NV_fence _procs_GL_NV_fence = {
  (GdkGLProc_glDeleteFencesNV) -1,
  (GdkGLProc_glGenFencesNV) -1,
  (GdkGLProc_glIsFenceNV) -1,
  (GdkGLProc_glTestFenceNV) -1,
  (GdkGLProc_glGetFenceivNV) -1,
  (GdkGLProc_glFinishFenceNV) -1,
  (GdkGLProc_glSetFenceNV) -1
};

/* glDeleteFencesNV */
GdkGLProc
gdk_gl_get_glDeleteFencesNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glDeleteFencesNV == (GdkGLProc_glDeleteFencesNV) -1)
    _procs_GL_NV_fence.glDeleteFencesNV =
      (GdkGLProc_glDeleteFencesNV) gdk_gl_get_proc_address ("glDeleteFencesNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteFencesNV () - %s",
               (_procs_GL_NV_fence.glDeleteFencesNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glDeleteFencesNV);
}

/* glGenFencesNV */
GdkGLProc
gdk_gl_get_glGenFencesNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glGenFencesNV == (GdkGLProc_glGenFencesNV) -1)
    _procs_GL_NV_fence.glGenFencesNV =
      (GdkGLProc_glGenFencesNV) gdk_gl_get_proc_address ("glGenFencesNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenFencesNV () - %s",
               (_procs_GL_NV_fence.glGenFencesNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glGenFencesNV);
}

/* glIsFenceNV */
GdkGLProc
gdk_gl_get_glIsFenceNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glIsFenceNV == (GdkGLProc_glIsFenceNV) -1)
    _procs_GL_NV_fence.glIsFenceNV =
      (GdkGLProc_glIsFenceNV) gdk_gl_get_proc_address ("glIsFenceNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsFenceNV () - %s",
               (_procs_GL_NV_fence.glIsFenceNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glIsFenceNV);
}

/* glTestFenceNV */
GdkGLProc
gdk_gl_get_glTestFenceNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glTestFenceNV == (GdkGLProc_glTestFenceNV) -1)
    _procs_GL_NV_fence.glTestFenceNV =
      (GdkGLProc_glTestFenceNV) gdk_gl_get_proc_address ("glTestFenceNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTestFenceNV () - %s",
               (_procs_GL_NV_fence.glTestFenceNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glTestFenceNV);
}

/* glGetFenceivNV */
GdkGLProc
gdk_gl_get_glGetFenceivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glGetFenceivNV == (GdkGLProc_glGetFenceivNV) -1)
    _procs_GL_NV_fence.glGetFenceivNV =
      (GdkGLProc_glGetFenceivNV) gdk_gl_get_proc_address ("glGetFenceivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFenceivNV () - %s",
               (_procs_GL_NV_fence.glGetFenceivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glGetFenceivNV);
}

/* glFinishFenceNV */
GdkGLProc
gdk_gl_get_glFinishFenceNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glFinishFenceNV == (GdkGLProc_glFinishFenceNV) -1)
    _procs_GL_NV_fence.glFinishFenceNV =
      (GdkGLProc_glFinishFenceNV) gdk_gl_get_proc_address ("glFinishFenceNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFinishFenceNV () - %s",
               (_procs_GL_NV_fence.glFinishFenceNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glFinishFenceNV);
}

/* glSetFenceNV */
GdkGLProc
gdk_gl_get_glSetFenceNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fence.glSetFenceNV == (GdkGLProc_glSetFenceNV) -1)
    _procs_GL_NV_fence.glSetFenceNV =
      (GdkGLProc_glSetFenceNV) gdk_gl_get_proc_address ("glSetFenceNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSetFenceNV () - %s",
               (_procs_GL_NV_fence.glSetFenceNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fence.glSetFenceNV);
}

/* Get GL_NV_fence functions */
GdkGL_GL_NV_fence *
gdk_gl_get_GL_NV_fence (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_fence");

      if (supported)
        {
          supported &= (gdk_gl_get_glDeleteFencesNV () != NULL);
          supported &= (gdk_gl_get_glGenFencesNV () != NULL);
          supported &= (gdk_gl_get_glIsFenceNV () != NULL);
          supported &= (gdk_gl_get_glTestFenceNV () != NULL);
          supported &= (gdk_gl_get_glGetFenceivNV () != NULL);
          supported &= (gdk_gl_get_glFinishFenceNV () != NULL);
          supported &= (gdk_gl_get_glSetFenceNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_fence () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_fence;
}

/*
 * GL_NV_evaluators
 */

static GdkGL_GL_NV_evaluators _procs_GL_NV_evaluators = {
  (GdkGLProc_glMapControlPointsNV) -1,
  (GdkGLProc_glMapParameterivNV) -1,
  (GdkGLProc_glMapParameterfvNV) -1,
  (GdkGLProc_glGetMapControlPointsNV) -1,
  (GdkGLProc_glGetMapParameterivNV) -1,
  (GdkGLProc_glGetMapParameterfvNV) -1,
  (GdkGLProc_glGetMapAttribParameterivNV) -1,
  (GdkGLProc_glGetMapAttribParameterfvNV) -1,
  (GdkGLProc_glEvalMapsNV) -1
};

/* glMapControlPointsNV */
GdkGLProc
gdk_gl_get_glMapControlPointsNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glMapControlPointsNV == (GdkGLProc_glMapControlPointsNV) -1)
    _procs_GL_NV_evaluators.glMapControlPointsNV =
      (GdkGLProc_glMapControlPointsNV) gdk_gl_get_proc_address ("glMapControlPointsNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapControlPointsNV () - %s",
               (_procs_GL_NV_evaluators.glMapControlPointsNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glMapControlPointsNV);
}

/* glMapParameterivNV */
GdkGLProc
gdk_gl_get_glMapParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glMapParameterivNV == (GdkGLProc_glMapParameterivNV) -1)
    _procs_GL_NV_evaluators.glMapParameterivNV =
      (GdkGLProc_glMapParameterivNV) gdk_gl_get_proc_address ("glMapParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapParameterivNV () - %s",
               (_procs_GL_NV_evaluators.glMapParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glMapParameterivNV);
}

/* glMapParameterfvNV */
GdkGLProc
gdk_gl_get_glMapParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glMapParameterfvNV == (GdkGLProc_glMapParameterfvNV) -1)
    _procs_GL_NV_evaluators.glMapParameterfvNV =
      (GdkGLProc_glMapParameterfvNV) gdk_gl_get_proc_address ("glMapParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapParameterfvNV () - %s",
               (_procs_GL_NV_evaluators.glMapParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glMapParameterfvNV);
}

/* glGetMapControlPointsNV */
GdkGLProc
gdk_gl_get_glGetMapControlPointsNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glGetMapControlPointsNV == (GdkGLProc_glGetMapControlPointsNV) -1)
    _procs_GL_NV_evaluators.glGetMapControlPointsNV =
      (GdkGLProc_glGetMapControlPointsNV) gdk_gl_get_proc_address ("glGetMapControlPointsNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMapControlPointsNV () - %s",
               (_procs_GL_NV_evaluators.glGetMapControlPointsNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glGetMapControlPointsNV);
}

/* glGetMapParameterivNV */
GdkGLProc
gdk_gl_get_glGetMapParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glGetMapParameterivNV == (GdkGLProc_glGetMapParameterivNV) -1)
    _procs_GL_NV_evaluators.glGetMapParameterivNV =
      (GdkGLProc_glGetMapParameterivNV) gdk_gl_get_proc_address ("glGetMapParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMapParameterivNV () - %s",
               (_procs_GL_NV_evaluators.glGetMapParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glGetMapParameterivNV);
}

/* glGetMapParameterfvNV */
GdkGLProc
gdk_gl_get_glGetMapParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glGetMapParameterfvNV == (GdkGLProc_glGetMapParameterfvNV) -1)
    _procs_GL_NV_evaluators.glGetMapParameterfvNV =
      (GdkGLProc_glGetMapParameterfvNV) gdk_gl_get_proc_address ("glGetMapParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMapParameterfvNV () - %s",
               (_procs_GL_NV_evaluators.glGetMapParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glGetMapParameterfvNV);
}

/* glGetMapAttribParameterivNV */
GdkGLProc
gdk_gl_get_glGetMapAttribParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glGetMapAttribParameterivNV == (GdkGLProc_glGetMapAttribParameterivNV) -1)
    _procs_GL_NV_evaluators.glGetMapAttribParameterivNV =
      (GdkGLProc_glGetMapAttribParameterivNV) gdk_gl_get_proc_address ("glGetMapAttribParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMapAttribParameterivNV () - %s",
               (_procs_GL_NV_evaluators.glGetMapAttribParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glGetMapAttribParameterivNV);
}

/* glGetMapAttribParameterfvNV */
GdkGLProc
gdk_gl_get_glGetMapAttribParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glGetMapAttribParameterfvNV == (GdkGLProc_glGetMapAttribParameterfvNV) -1)
    _procs_GL_NV_evaluators.glGetMapAttribParameterfvNV =
      (GdkGLProc_glGetMapAttribParameterfvNV) gdk_gl_get_proc_address ("glGetMapAttribParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetMapAttribParameterfvNV () - %s",
               (_procs_GL_NV_evaluators.glGetMapAttribParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glGetMapAttribParameterfvNV);
}

/* glEvalMapsNV */
GdkGLProc
gdk_gl_get_glEvalMapsNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_evaluators.glEvalMapsNV == (GdkGLProc_glEvalMapsNV) -1)
    _procs_GL_NV_evaluators.glEvalMapsNV =
      (GdkGLProc_glEvalMapsNV) gdk_gl_get_proc_address ("glEvalMapsNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEvalMapsNV () - %s",
               (_procs_GL_NV_evaluators.glEvalMapsNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_evaluators.glEvalMapsNV);
}

/* Get GL_NV_evaluators functions */
GdkGL_GL_NV_evaluators *
gdk_gl_get_GL_NV_evaluators (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_evaluators");

      if (supported)
        {
          supported &= (gdk_gl_get_glMapControlPointsNV () != NULL);
          supported &= (gdk_gl_get_glMapParameterivNV () != NULL);
          supported &= (gdk_gl_get_glMapParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetMapControlPointsNV () != NULL);
          supported &= (gdk_gl_get_glGetMapParameterivNV () != NULL);
          supported &= (gdk_gl_get_glGetMapParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetMapAttribParameterivNV () != NULL);
          supported &= (gdk_gl_get_glGetMapAttribParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glEvalMapsNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_evaluators () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_evaluators;
}

/*
 * GL_NV_register_combiners2
 */

static GdkGL_GL_NV_register_combiners2 _procs_GL_NV_register_combiners2 = {
  (GdkGLProc_glCombinerStageParameterfvNV) -1,
  (GdkGLProc_glGetCombinerStageParameterfvNV) -1
};

/* glCombinerStageParameterfvNV */
GdkGLProc
gdk_gl_get_glCombinerStageParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners2.glCombinerStageParameterfvNV == (GdkGLProc_glCombinerStageParameterfvNV) -1)
    _procs_GL_NV_register_combiners2.glCombinerStageParameterfvNV =
      (GdkGLProc_glCombinerStageParameterfvNV) gdk_gl_get_proc_address ("glCombinerStageParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glCombinerStageParameterfvNV () - %s",
               (_procs_GL_NV_register_combiners2.glCombinerStageParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners2.glCombinerStageParameterfvNV);
}

/* glGetCombinerStageParameterfvNV */
GdkGLProc
gdk_gl_get_glGetCombinerStageParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_register_combiners2.glGetCombinerStageParameterfvNV == (GdkGLProc_glGetCombinerStageParameterfvNV) -1)
    _procs_GL_NV_register_combiners2.glGetCombinerStageParameterfvNV =
      (GdkGLProc_glGetCombinerStageParameterfvNV) gdk_gl_get_proc_address ("glGetCombinerStageParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetCombinerStageParameterfvNV () - %s",
               (_procs_GL_NV_register_combiners2.glGetCombinerStageParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_register_combiners2.glGetCombinerStageParameterfvNV);
}

/* Get GL_NV_register_combiners2 functions */
GdkGL_GL_NV_register_combiners2 *
gdk_gl_get_GL_NV_register_combiners2 (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_register_combiners2");

      if (supported)
        {
          supported &= (gdk_gl_get_glCombinerStageParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetCombinerStageParameterfvNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_register_combiners2 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_register_combiners2;
}

/*
 * GL_NV_vertex_program
 */

static GdkGL_GL_NV_vertex_program _procs_GL_NV_vertex_program = {
  (GdkGLProc_glAreProgramsResidentNV) -1,
  (GdkGLProc_glBindProgramNV) -1,
  (GdkGLProc_glDeleteProgramsNV) -1,
  (GdkGLProc_glExecuteProgramNV) -1,
  (GdkGLProc_glGenProgramsNV) -1,
  (GdkGLProc_glGetProgramParameterdvNV) -1,
  (GdkGLProc_glGetProgramParameterfvNV) -1,
  (GdkGLProc_glGetProgramivNV) -1,
  (GdkGLProc_glGetProgramStringNV) -1,
  (GdkGLProc_glGetTrackMatrixivNV) -1,
  (GdkGLProc_glGetVertexAttribdvNV) -1,
  (GdkGLProc_glGetVertexAttribfvNV) -1,
  (GdkGLProc_glGetVertexAttribivNV) -1,
  (GdkGLProc_glGetVertexAttribPointervNV) -1,
  (GdkGLProc_glIsProgramNV) -1,
  (GdkGLProc_glLoadProgramNV) -1,
  (GdkGLProc_glProgramParameter4dNV) -1,
  (GdkGLProc_glProgramParameter4dvNV) -1,
  (GdkGLProc_glProgramParameter4fNV) -1,
  (GdkGLProc_glProgramParameter4fvNV) -1,
  (GdkGLProc_glProgramParameters4dvNV) -1,
  (GdkGLProc_glProgramParameters4fvNV) -1,
  (GdkGLProc_glRequestResidentProgramsNV) -1,
  (GdkGLProc_glTrackMatrixNV) -1,
  (GdkGLProc_glVertexAttribPointerNV) -1,
  (GdkGLProc_glVertexAttrib1dNV) -1,
  (GdkGLProc_glVertexAttrib1dvNV) -1,
  (GdkGLProc_glVertexAttrib1fNV) -1,
  (GdkGLProc_glVertexAttrib1fvNV) -1,
  (GdkGLProc_glVertexAttrib1sNV) -1,
  (GdkGLProc_glVertexAttrib1svNV) -1,
  (GdkGLProc_glVertexAttrib2dNV) -1,
  (GdkGLProc_glVertexAttrib2dvNV) -1,
  (GdkGLProc_glVertexAttrib2fNV) -1,
  (GdkGLProc_glVertexAttrib2fvNV) -1,
  (GdkGLProc_glVertexAttrib2sNV) -1,
  (GdkGLProc_glVertexAttrib2svNV) -1,
  (GdkGLProc_glVertexAttrib3dNV) -1,
  (GdkGLProc_glVertexAttrib3dvNV) -1,
  (GdkGLProc_glVertexAttrib3fNV) -1,
  (GdkGLProc_glVertexAttrib3fvNV) -1,
  (GdkGLProc_glVertexAttrib3sNV) -1,
  (GdkGLProc_glVertexAttrib3svNV) -1,
  (GdkGLProc_glVertexAttrib4dNV) -1,
  (GdkGLProc_glVertexAttrib4dvNV) -1,
  (GdkGLProc_glVertexAttrib4fNV) -1,
  (GdkGLProc_glVertexAttrib4fvNV) -1,
  (GdkGLProc_glVertexAttrib4sNV) -1,
  (GdkGLProc_glVertexAttrib4svNV) -1,
  (GdkGLProc_glVertexAttrib4ubNV) -1,
  (GdkGLProc_glVertexAttrib4ubvNV) -1,
  (GdkGLProc_glVertexAttribs1dvNV) -1,
  (GdkGLProc_glVertexAttribs1fvNV) -1,
  (GdkGLProc_glVertexAttribs1svNV) -1,
  (GdkGLProc_glVertexAttribs2dvNV) -1,
  (GdkGLProc_glVertexAttribs2fvNV) -1,
  (GdkGLProc_glVertexAttribs2svNV) -1,
  (GdkGLProc_glVertexAttribs3dvNV) -1,
  (GdkGLProc_glVertexAttribs3fvNV) -1,
  (GdkGLProc_glVertexAttribs3svNV) -1,
  (GdkGLProc_glVertexAttribs4dvNV) -1,
  (GdkGLProc_glVertexAttribs4fvNV) -1,
  (GdkGLProc_glVertexAttribs4svNV) -1,
  (GdkGLProc_glVertexAttribs4ubvNV) -1
};

/* glAreProgramsResidentNV */
GdkGLProc
gdk_gl_get_glAreProgramsResidentNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glAreProgramsResidentNV == (GdkGLProc_glAreProgramsResidentNV) -1)
    _procs_GL_NV_vertex_program.glAreProgramsResidentNV =
      (GdkGLProc_glAreProgramsResidentNV) gdk_gl_get_proc_address ("glAreProgramsResidentNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAreProgramsResidentNV () - %s",
               (_procs_GL_NV_vertex_program.glAreProgramsResidentNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glAreProgramsResidentNV);
}

/* glBindProgramNV */
GdkGLProc
gdk_gl_get_glBindProgramNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glBindProgramNV == (GdkGLProc_glBindProgramNV) -1)
    _procs_GL_NV_vertex_program.glBindProgramNV =
      (GdkGLProc_glBindProgramNV) gdk_gl_get_proc_address ("glBindProgramNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindProgramNV () - %s",
               (_procs_GL_NV_vertex_program.glBindProgramNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glBindProgramNV);
}

/* glDeleteProgramsNV */
GdkGLProc
gdk_gl_get_glDeleteProgramsNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glDeleteProgramsNV == (GdkGLProc_glDeleteProgramsNV) -1)
    _procs_GL_NV_vertex_program.glDeleteProgramsNV =
      (GdkGLProc_glDeleteProgramsNV) gdk_gl_get_proc_address ("glDeleteProgramsNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteProgramsNV () - %s",
               (_procs_GL_NV_vertex_program.glDeleteProgramsNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glDeleteProgramsNV);
}

/* glExecuteProgramNV */
GdkGLProc
gdk_gl_get_glExecuteProgramNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glExecuteProgramNV == (GdkGLProc_glExecuteProgramNV) -1)
    _procs_GL_NV_vertex_program.glExecuteProgramNV =
      (GdkGLProc_glExecuteProgramNV) gdk_gl_get_proc_address ("glExecuteProgramNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glExecuteProgramNV () - %s",
               (_procs_GL_NV_vertex_program.glExecuteProgramNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glExecuteProgramNV);
}

/* glGenProgramsNV */
GdkGLProc
gdk_gl_get_glGenProgramsNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGenProgramsNV == (GdkGLProc_glGenProgramsNV) -1)
    _procs_GL_NV_vertex_program.glGenProgramsNV =
      (GdkGLProc_glGenProgramsNV) gdk_gl_get_proc_address ("glGenProgramsNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenProgramsNV () - %s",
               (_procs_GL_NV_vertex_program.glGenProgramsNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGenProgramsNV);
}

/* glGetProgramParameterdvNV */
GdkGLProc
gdk_gl_get_glGetProgramParameterdvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetProgramParameterdvNV == (GdkGLProc_glGetProgramParameterdvNV) -1)
    _procs_GL_NV_vertex_program.glGetProgramParameterdvNV =
      (GdkGLProc_glGetProgramParameterdvNV) gdk_gl_get_proc_address ("glGetProgramParameterdvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramParameterdvNV () - %s",
               (_procs_GL_NV_vertex_program.glGetProgramParameterdvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetProgramParameterdvNV);
}

/* glGetProgramParameterfvNV */
GdkGLProc
gdk_gl_get_glGetProgramParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetProgramParameterfvNV == (GdkGLProc_glGetProgramParameterfvNV) -1)
    _procs_GL_NV_vertex_program.glGetProgramParameterfvNV =
      (GdkGLProc_glGetProgramParameterfvNV) gdk_gl_get_proc_address ("glGetProgramParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramParameterfvNV () - %s",
               (_procs_GL_NV_vertex_program.glGetProgramParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetProgramParameterfvNV);
}

/* glGetProgramivNV */
GdkGLProc
gdk_gl_get_glGetProgramivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetProgramivNV == (GdkGLProc_glGetProgramivNV) -1)
    _procs_GL_NV_vertex_program.glGetProgramivNV =
      (GdkGLProc_glGetProgramivNV) gdk_gl_get_proc_address ("glGetProgramivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramivNV () - %s",
               (_procs_GL_NV_vertex_program.glGetProgramivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetProgramivNV);
}

/* glGetProgramStringNV */
GdkGLProc
gdk_gl_get_glGetProgramStringNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetProgramStringNV == (GdkGLProc_glGetProgramStringNV) -1)
    _procs_GL_NV_vertex_program.glGetProgramStringNV =
      (GdkGLProc_glGetProgramStringNV) gdk_gl_get_proc_address ("glGetProgramStringNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramStringNV () - %s",
               (_procs_GL_NV_vertex_program.glGetProgramStringNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetProgramStringNV);
}

/* glGetTrackMatrixivNV */
GdkGLProc
gdk_gl_get_glGetTrackMatrixivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetTrackMatrixivNV == (GdkGLProc_glGetTrackMatrixivNV) -1)
    _procs_GL_NV_vertex_program.glGetTrackMatrixivNV =
      (GdkGLProc_glGetTrackMatrixivNV) gdk_gl_get_proc_address ("glGetTrackMatrixivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetTrackMatrixivNV () - %s",
               (_procs_GL_NV_vertex_program.glGetTrackMatrixivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetTrackMatrixivNV);
}

/* glGetVertexAttribdvNV */
GdkGLProc
gdk_gl_get_glGetVertexAttribdvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetVertexAttribdvNV == (GdkGLProc_glGetVertexAttribdvNV) -1)
    _procs_GL_NV_vertex_program.glGetVertexAttribdvNV =
      (GdkGLProc_glGetVertexAttribdvNV) gdk_gl_get_proc_address ("glGetVertexAttribdvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribdvNV () - %s",
               (_procs_GL_NV_vertex_program.glGetVertexAttribdvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetVertexAttribdvNV);
}

/* glGetVertexAttribfvNV */
GdkGLProc
gdk_gl_get_glGetVertexAttribfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetVertexAttribfvNV == (GdkGLProc_glGetVertexAttribfvNV) -1)
    _procs_GL_NV_vertex_program.glGetVertexAttribfvNV =
      (GdkGLProc_glGetVertexAttribfvNV) gdk_gl_get_proc_address ("glGetVertexAttribfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribfvNV () - %s",
               (_procs_GL_NV_vertex_program.glGetVertexAttribfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetVertexAttribfvNV);
}

/* glGetVertexAttribivNV */
GdkGLProc
gdk_gl_get_glGetVertexAttribivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetVertexAttribivNV == (GdkGLProc_glGetVertexAttribivNV) -1)
    _procs_GL_NV_vertex_program.glGetVertexAttribivNV =
      (GdkGLProc_glGetVertexAttribivNV) gdk_gl_get_proc_address ("glGetVertexAttribivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribivNV () - %s",
               (_procs_GL_NV_vertex_program.glGetVertexAttribivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetVertexAttribivNV);
}

/* glGetVertexAttribPointervNV */
GdkGLProc
gdk_gl_get_glGetVertexAttribPointervNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glGetVertexAttribPointervNV == (GdkGLProc_glGetVertexAttribPointervNV) -1)
    _procs_GL_NV_vertex_program.glGetVertexAttribPointervNV =
      (GdkGLProc_glGetVertexAttribPointervNV) gdk_gl_get_proc_address ("glGetVertexAttribPointervNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribPointervNV () - %s",
               (_procs_GL_NV_vertex_program.glGetVertexAttribPointervNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glGetVertexAttribPointervNV);
}

/* glIsProgramNV */
GdkGLProc
gdk_gl_get_glIsProgramNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glIsProgramNV == (GdkGLProc_glIsProgramNV) -1)
    _procs_GL_NV_vertex_program.glIsProgramNV =
      (GdkGLProc_glIsProgramNV) gdk_gl_get_proc_address ("glIsProgramNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsProgramNV () - %s",
               (_procs_GL_NV_vertex_program.glIsProgramNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glIsProgramNV);
}

/* glLoadProgramNV */
GdkGLProc
gdk_gl_get_glLoadProgramNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glLoadProgramNV == (GdkGLProc_glLoadProgramNV) -1)
    _procs_GL_NV_vertex_program.glLoadProgramNV =
      (GdkGLProc_glLoadProgramNV) gdk_gl_get_proc_address ("glLoadProgramNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLoadProgramNV () - %s",
               (_procs_GL_NV_vertex_program.glLoadProgramNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glLoadProgramNV);
}

/* glProgramParameter4dNV */
GdkGLProc
gdk_gl_get_glProgramParameter4dNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glProgramParameter4dNV == (GdkGLProc_glProgramParameter4dNV) -1)
    _procs_GL_NV_vertex_program.glProgramParameter4dNV =
      (GdkGLProc_glProgramParameter4dNV) gdk_gl_get_proc_address ("glProgramParameter4dNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramParameter4dNV () - %s",
               (_procs_GL_NV_vertex_program.glProgramParameter4dNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glProgramParameter4dNV);
}

/* glProgramParameter4dvNV */
GdkGLProc
gdk_gl_get_glProgramParameter4dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glProgramParameter4dvNV == (GdkGLProc_glProgramParameter4dvNV) -1)
    _procs_GL_NV_vertex_program.glProgramParameter4dvNV =
      (GdkGLProc_glProgramParameter4dvNV) gdk_gl_get_proc_address ("glProgramParameter4dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramParameter4dvNV () - %s",
               (_procs_GL_NV_vertex_program.glProgramParameter4dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glProgramParameter4dvNV);
}

/* glProgramParameter4fNV */
GdkGLProc
gdk_gl_get_glProgramParameter4fNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glProgramParameter4fNV == (GdkGLProc_glProgramParameter4fNV) -1)
    _procs_GL_NV_vertex_program.glProgramParameter4fNV =
      (GdkGLProc_glProgramParameter4fNV) gdk_gl_get_proc_address ("glProgramParameter4fNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramParameter4fNV () - %s",
               (_procs_GL_NV_vertex_program.glProgramParameter4fNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glProgramParameter4fNV);
}

/* glProgramParameter4fvNV */
GdkGLProc
gdk_gl_get_glProgramParameter4fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glProgramParameter4fvNV == (GdkGLProc_glProgramParameter4fvNV) -1)
    _procs_GL_NV_vertex_program.glProgramParameter4fvNV =
      (GdkGLProc_glProgramParameter4fvNV) gdk_gl_get_proc_address ("glProgramParameter4fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramParameter4fvNV () - %s",
               (_procs_GL_NV_vertex_program.glProgramParameter4fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glProgramParameter4fvNV);
}

/* glProgramParameters4dvNV */
GdkGLProc
gdk_gl_get_glProgramParameters4dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glProgramParameters4dvNV == (GdkGLProc_glProgramParameters4dvNV) -1)
    _procs_GL_NV_vertex_program.glProgramParameters4dvNV =
      (GdkGLProc_glProgramParameters4dvNV) gdk_gl_get_proc_address ("glProgramParameters4dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramParameters4dvNV () - %s",
               (_procs_GL_NV_vertex_program.glProgramParameters4dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glProgramParameters4dvNV);
}

/* glProgramParameters4fvNV */
GdkGLProc
gdk_gl_get_glProgramParameters4fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glProgramParameters4fvNV == (GdkGLProc_glProgramParameters4fvNV) -1)
    _procs_GL_NV_vertex_program.glProgramParameters4fvNV =
      (GdkGLProc_glProgramParameters4fvNV) gdk_gl_get_proc_address ("glProgramParameters4fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramParameters4fvNV () - %s",
               (_procs_GL_NV_vertex_program.glProgramParameters4fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glProgramParameters4fvNV);
}

/* glRequestResidentProgramsNV */
GdkGLProc
gdk_gl_get_glRequestResidentProgramsNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glRequestResidentProgramsNV == (GdkGLProc_glRequestResidentProgramsNV) -1)
    _procs_GL_NV_vertex_program.glRequestResidentProgramsNV =
      (GdkGLProc_glRequestResidentProgramsNV) gdk_gl_get_proc_address ("glRequestResidentProgramsNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glRequestResidentProgramsNV () - %s",
               (_procs_GL_NV_vertex_program.glRequestResidentProgramsNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glRequestResidentProgramsNV);
}

/* glTrackMatrixNV */
GdkGLProc
gdk_gl_get_glTrackMatrixNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glTrackMatrixNV == (GdkGLProc_glTrackMatrixNV) -1)
    _procs_GL_NV_vertex_program.glTrackMatrixNV =
      (GdkGLProc_glTrackMatrixNV) gdk_gl_get_proc_address ("glTrackMatrixNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTrackMatrixNV () - %s",
               (_procs_GL_NV_vertex_program.glTrackMatrixNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glTrackMatrixNV);
}

/* glVertexAttribPointerNV */
GdkGLProc
gdk_gl_get_glVertexAttribPointerNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribPointerNV == (GdkGLProc_glVertexAttribPointerNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribPointerNV =
      (GdkGLProc_glVertexAttribPointerNV) gdk_gl_get_proc_address ("glVertexAttribPointerNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribPointerNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribPointerNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribPointerNV);
}

/* glVertexAttrib1dNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1dNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib1dNV == (GdkGLProc_glVertexAttrib1dNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib1dNV =
      (GdkGLProc_glVertexAttrib1dNV) gdk_gl_get_proc_address ("glVertexAttrib1dNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1dNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib1dNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib1dNV);
}

/* glVertexAttrib1dvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib1dvNV == (GdkGLProc_glVertexAttrib1dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib1dvNV =
      (GdkGLProc_glVertexAttrib1dvNV) gdk_gl_get_proc_address ("glVertexAttrib1dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib1dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib1dvNV);
}

/* glVertexAttrib1fNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1fNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib1fNV == (GdkGLProc_glVertexAttrib1fNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib1fNV =
      (GdkGLProc_glVertexAttrib1fNV) gdk_gl_get_proc_address ("glVertexAttrib1fNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1fNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib1fNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib1fNV);
}

/* glVertexAttrib1fvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib1fvNV == (GdkGLProc_glVertexAttrib1fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib1fvNV =
      (GdkGLProc_glVertexAttrib1fvNV) gdk_gl_get_proc_address ("glVertexAttrib1fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib1fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib1fvNV);
}

/* glVertexAttrib1sNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1sNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib1sNV == (GdkGLProc_glVertexAttrib1sNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib1sNV =
      (GdkGLProc_glVertexAttrib1sNV) gdk_gl_get_proc_address ("glVertexAttrib1sNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1sNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib1sNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib1sNV);
}

/* glVertexAttrib1svNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib1svNV == (GdkGLProc_glVertexAttrib1svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib1svNV =
      (GdkGLProc_glVertexAttrib1svNV) gdk_gl_get_proc_address ("glVertexAttrib1svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib1svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib1svNV);
}

/* glVertexAttrib2dNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2dNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib2dNV == (GdkGLProc_glVertexAttrib2dNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib2dNV =
      (GdkGLProc_glVertexAttrib2dNV) gdk_gl_get_proc_address ("glVertexAttrib2dNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2dNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib2dNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib2dNV);
}

/* glVertexAttrib2dvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib2dvNV == (GdkGLProc_glVertexAttrib2dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib2dvNV =
      (GdkGLProc_glVertexAttrib2dvNV) gdk_gl_get_proc_address ("glVertexAttrib2dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib2dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib2dvNV);
}

/* glVertexAttrib2fNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2fNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib2fNV == (GdkGLProc_glVertexAttrib2fNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib2fNV =
      (GdkGLProc_glVertexAttrib2fNV) gdk_gl_get_proc_address ("glVertexAttrib2fNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2fNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib2fNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib2fNV);
}

/* glVertexAttrib2fvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib2fvNV == (GdkGLProc_glVertexAttrib2fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib2fvNV =
      (GdkGLProc_glVertexAttrib2fvNV) gdk_gl_get_proc_address ("glVertexAttrib2fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib2fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib2fvNV);
}

/* glVertexAttrib2sNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2sNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib2sNV == (GdkGLProc_glVertexAttrib2sNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib2sNV =
      (GdkGLProc_glVertexAttrib2sNV) gdk_gl_get_proc_address ("glVertexAttrib2sNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2sNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib2sNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib2sNV);
}

/* glVertexAttrib2svNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib2svNV == (GdkGLProc_glVertexAttrib2svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib2svNV =
      (GdkGLProc_glVertexAttrib2svNV) gdk_gl_get_proc_address ("glVertexAttrib2svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib2svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib2svNV);
}

/* glVertexAttrib3dNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3dNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib3dNV == (GdkGLProc_glVertexAttrib3dNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib3dNV =
      (GdkGLProc_glVertexAttrib3dNV) gdk_gl_get_proc_address ("glVertexAttrib3dNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3dNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib3dNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib3dNV);
}

/* glVertexAttrib3dvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib3dvNV == (GdkGLProc_glVertexAttrib3dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib3dvNV =
      (GdkGLProc_glVertexAttrib3dvNV) gdk_gl_get_proc_address ("glVertexAttrib3dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib3dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib3dvNV);
}

/* glVertexAttrib3fNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3fNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib3fNV == (GdkGLProc_glVertexAttrib3fNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib3fNV =
      (GdkGLProc_glVertexAttrib3fNV) gdk_gl_get_proc_address ("glVertexAttrib3fNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3fNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib3fNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib3fNV);
}

/* glVertexAttrib3fvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib3fvNV == (GdkGLProc_glVertexAttrib3fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib3fvNV =
      (GdkGLProc_glVertexAttrib3fvNV) gdk_gl_get_proc_address ("glVertexAttrib3fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib3fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib3fvNV);
}

/* glVertexAttrib3sNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3sNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib3sNV == (GdkGLProc_glVertexAttrib3sNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib3sNV =
      (GdkGLProc_glVertexAttrib3sNV) gdk_gl_get_proc_address ("glVertexAttrib3sNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3sNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib3sNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib3sNV);
}

/* glVertexAttrib3svNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib3svNV == (GdkGLProc_glVertexAttrib3svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib3svNV =
      (GdkGLProc_glVertexAttrib3svNV) gdk_gl_get_proc_address ("glVertexAttrib3svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib3svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib3svNV);
}

/* glVertexAttrib4dNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4dNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4dNV == (GdkGLProc_glVertexAttrib4dNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4dNV =
      (GdkGLProc_glVertexAttrib4dNV) gdk_gl_get_proc_address ("glVertexAttrib4dNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4dNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4dNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4dNV);
}

/* glVertexAttrib4dvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4dvNV == (GdkGLProc_glVertexAttrib4dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4dvNV =
      (GdkGLProc_glVertexAttrib4dvNV) gdk_gl_get_proc_address ("glVertexAttrib4dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4dvNV);
}

/* glVertexAttrib4fNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4fNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4fNV == (GdkGLProc_glVertexAttrib4fNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4fNV =
      (GdkGLProc_glVertexAttrib4fNV) gdk_gl_get_proc_address ("glVertexAttrib4fNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4fNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4fNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4fNV);
}

/* glVertexAttrib4fvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4fvNV == (GdkGLProc_glVertexAttrib4fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4fvNV =
      (GdkGLProc_glVertexAttrib4fvNV) gdk_gl_get_proc_address ("glVertexAttrib4fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4fvNV);
}

/* glVertexAttrib4sNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4sNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4sNV == (GdkGLProc_glVertexAttrib4sNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4sNV =
      (GdkGLProc_glVertexAttrib4sNV) gdk_gl_get_proc_address ("glVertexAttrib4sNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4sNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4sNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4sNV);
}

/* glVertexAttrib4svNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4svNV == (GdkGLProc_glVertexAttrib4svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4svNV =
      (GdkGLProc_glVertexAttrib4svNV) gdk_gl_get_proc_address ("glVertexAttrib4svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4svNV);
}

/* glVertexAttrib4ubNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4ubNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4ubNV == (GdkGLProc_glVertexAttrib4ubNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4ubNV =
      (GdkGLProc_glVertexAttrib4ubNV) gdk_gl_get_proc_address ("glVertexAttrib4ubNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4ubNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4ubNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4ubNV);
}

/* glVertexAttrib4ubvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4ubvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttrib4ubvNV == (GdkGLProc_glVertexAttrib4ubvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttrib4ubvNV =
      (GdkGLProc_glVertexAttrib4ubvNV) gdk_gl_get_proc_address ("glVertexAttrib4ubvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4ubvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttrib4ubvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttrib4ubvNV);
}

/* glVertexAttribs1dvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs1dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs1dvNV == (GdkGLProc_glVertexAttribs1dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs1dvNV =
      (GdkGLProc_glVertexAttribs1dvNV) gdk_gl_get_proc_address ("glVertexAttribs1dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs1dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs1dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs1dvNV);
}

/* glVertexAttribs1fvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs1fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs1fvNV == (GdkGLProc_glVertexAttribs1fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs1fvNV =
      (GdkGLProc_glVertexAttribs1fvNV) gdk_gl_get_proc_address ("glVertexAttribs1fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs1fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs1fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs1fvNV);
}

/* glVertexAttribs1svNV */
GdkGLProc
gdk_gl_get_glVertexAttribs1svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs1svNV == (GdkGLProc_glVertexAttribs1svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs1svNV =
      (GdkGLProc_glVertexAttribs1svNV) gdk_gl_get_proc_address ("glVertexAttribs1svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs1svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs1svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs1svNV);
}

/* glVertexAttribs2dvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs2dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs2dvNV == (GdkGLProc_glVertexAttribs2dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs2dvNV =
      (GdkGLProc_glVertexAttribs2dvNV) gdk_gl_get_proc_address ("glVertexAttribs2dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs2dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs2dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs2dvNV);
}

/* glVertexAttribs2fvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs2fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs2fvNV == (GdkGLProc_glVertexAttribs2fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs2fvNV =
      (GdkGLProc_glVertexAttribs2fvNV) gdk_gl_get_proc_address ("glVertexAttribs2fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs2fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs2fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs2fvNV);
}

/* glVertexAttribs2svNV */
GdkGLProc
gdk_gl_get_glVertexAttribs2svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs2svNV == (GdkGLProc_glVertexAttribs2svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs2svNV =
      (GdkGLProc_glVertexAttribs2svNV) gdk_gl_get_proc_address ("glVertexAttribs2svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs2svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs2svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs2svNV);
}

/* glVertexAttribs3dvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs3dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs3dvNV == (GdkGLProc_glVertexAttribs3dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs3dvNV =
      (GdkGLProc_glVertexAttribs3dvNV) gdk_gl_get_proc_address ("glVertexAttribs3dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs3dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs3dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs3dvNV);
}

/* glVertexAttribs3fvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs3fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs3fvNV == (GdkGLProc_glVertexAttribs3fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs3fvNV =
      (GdkGLProc_glVertexAttribs3fvNV) gdk_gl_get_proc_address ("glVertexAttribs3fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs3fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs3fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs3fvNV);
}

/* glVertexAttribs3svNV */
GdkGLProc
gdk_gl_get_glVertexAttribs3svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs3svNV == (GdkGLProc_glVertexAttribs3svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs3svNV =
      (GdkGLProc_glVertexAttribs3svNV) gdk_gl_get_proc_address ("glVertexAttribs3svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs3svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs3svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs3svNV);
}

/* glVertexAttribs4dvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs4dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs4dvNV == (GdkGLProc_glVertexAttribs4dvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs4dvNV =
      (GdkGLProc_glVertexAttribs4dvNV) gdk_gl_get_proc_address ("glVertexAttribs4dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs4dvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs4dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs4dvNV);
}

/* glVertexAttribs4fvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs4fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs4fvNV == (GdkGLProc_glVertexAttribs4fvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs4fvNV =
      (GdkGLProc_glVertexAttribs4fvNV) gdk_gl_get_proc_address ("glVertexAttribs4fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs4fvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs4fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs4fvNV);
}

/* glVertexAttribs4svNV */
GdkGLProc
gdk_gl_get_glVertexAttribs4svNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs4svNV == (GdkGLProc_glVertexAttribs4svNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs4svNV =
      (GdkGLProc_glVertexAttribs4svNV) gdk_gl_get_proc_address ("glVertexAttribs4svNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs4svNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs4svNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs4svNV);
}

/* glVertexAttribs4ubvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs4ubvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_vertex_program.glVertexAttribs4ubvNV == (GdkGLProc_glVertexAttribs4ubvNV) -1)
    _procs_GL_NV_vertex_program.glVertexAttribs4ubvNV =
      (GdkGLProc_glVertexAttribs4ubvNV) gdk_gl_get_proc_address ("glVertexAttribs4ubvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs4ubvNV () - %s",
               (_procs_GL_NV_vertex_program.glVertexAttribs4ubvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_vertex_program.glVertexAttribs4ubvNV);
}

/* Get GL_NV_vertex_program functions */
GdkGL_GL_NV_vertex_program *
gdk_gl_get_GL_NV_vertex_program (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_vertex_program");

      if (supported)
        {
          supported &= (gdk_gl_get_glAreProgramsResidentNV () != NULL);
          supported &= (gdk_gl_get_glBindProgramNV () != NULL);
          supported &= (gdk_gl_get_glDeleteProgramsNV () != NULL);
          supported &= (gdk_gl_get_glExecuteProgramNV () != NULL);
          supported &= (gdk_gl_get_glGenProgramsNV () != NULL);
          supported &= (gdk_gl_get_glGetProgramParameterdvNV () != NULL);
          supported &= (gdk_gl_get_glGetProgramParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetProgramivNV () != NULL);
          supported &= (gdk_gl_get_glGetProgramStringNV () != NULL);
          supported &= (gdk_gl_get_glGetTrackMatrixivNV () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribdvNV () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribfvNV () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribivNV () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribPointervNV () != NULL);
          supported &= (gdk_gl_get_glIsProgramNV () != NULL);
          supported &= (gdk_gl_get_glLoadProgramNV () != NULL);
          supported &= (gdk_gl_get_glProgramParameter4dNV () != NULL);
          supported &= (gdk_gl_get_glProgramParameter4dvNV () != NULL);
          supported &= (gdk_gl_get_glProgramParameter4fNV () != NULL);
          supported &= (gdk_gl_get_glProgramParameter4fvNV () != NULL);
          supported &= (gdk_gl_get_glProgramParameters4dvNV () != NULL);
          supported &= (gdk_gl_get_glProgramParameters4fvNV () != NULL);
          supported &= (gdk_gl_get_glRequestResidentProgramsNV () != NULL);
          supported &= (gdk_gl_get_glTrackMatrixNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribPointerNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1dNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1fNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1sNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2dNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2fNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2sNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3dNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3fNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3sNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4dNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4fNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4sNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4ubNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4ubvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs1dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs1fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs1svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs2dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs2fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs2svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs3dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs3fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs3svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs4dvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs4fvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs4svNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs4ubvNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_vertex_program () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_vertex_program;
}

/*
 * GL_ATI_envmap_bumpmap
 */

static GdkGL_GL_ATI_envmap_bumpmap _procs_GL_ATI_envmap_bumpmap = {
  (GdkGLProc_glTexBumpParameterivATI) -1,
  (GdkGLProc_glTexBumpParameterfvATI) -1,
  (GdkGLProc_glGetTexBumpParameterivATI) -1,
  (GdkGLProc_glGetTexBumpParameterfvATI) -1
};

/* glTexBumpParameterivATI */
GdkGLProc
gdk_gl_get_glTexBumpParameterivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_envmap_bumpmap.glTexBumpParameterivATI == (GdkGLProc_glTexBumpParameterivATI) -1)
    _procs_GL_ATI_envmap_bumpmap.glTexBumpParameterivATI =
      (GdkGLProc_glTexBumpParameterivATI) gdk_gl_get_proc_address ("glTexBumpParameterivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexBumpParameterivATI () - %s",
               (_procs_GL_ATI_envmap_bumpmap.glTexBumpParameterivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_envmap_bumpmap.glTexBumpParameterivATI);
}

/* glTexBumpParameterfvATI */
GdkGLProc
gdk_gl_get_glTexBumpParameterfvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_envmap_bumpmap.glTexBumpParameterfvATI == (GdkGLProc_glTexBumpParameterfvATI) -1)
    _procs_GL_ATI_envmap_bumpmap.glTexBumpParameterfvATI =
      (GdkGLProc_glTexBumpParameterfvATI) gdk_gl_get_proc_address ("glTexBumpParameterfvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexBumpParameterfvATI () - %s",
               (_procs_GL_ATI_envmap_bumpmap.glTexBumpParameterfvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_envmap_bumpmap.glTexBumpParameterfvATI);
}

/* glGetTexBumpParameterivATI */
GdkGLProc
gdk_gl_get_glGetTexBumpParameterivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterivATI == (GdkGLProc_glGetTexBumpParameterivATI) -1)
    _procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterivATI =
      (GdkGLProc_glGetTexBumpParameterivATI) gdk_gl_get_proc_address ("glGetTexBumpParameterivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetTexBumpParameterivATI () - %s",
               (_procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterivATI);
}

/* glGetTexBumpParameterfvATI */
GdkGLProc
gdk_gl_get_glGetTexBumpParameterfvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterfvATI == (GdkGLProc_glGetTexBumpParameterfvATI) -1)
    _procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterfvATI =
      (GdkGLProc_glGetTexBumpParameterfvATI) gdk_gl_get_proc_address ("glGetTexBumpParameterfvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetTexBumpParameterfvATI () - %s",
               (_procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterfvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_envmap_bumpmap.glGetTexBumpParameterfvATI);
}

/* Get GL_ATI_envmap_bumpmap functions */
GdkGL_GL_ATI_envmap_bumpmap *
gdk_gl_get_GL_ATI_envmap_bumpmap (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_envmap_bumpmap");

      if (supported)
        {
          supported &= (gdk_gl_get_glTexBumpParameterivATI () != NULL);
          supported &= (gdk_gl_get_glTexBumpParameterfvATI () != NULL);
          supported &= (gdk_gl_get_glGetTexBumpParameterivATI () != NULL);
          supported &= (gdk_gl_get_glGetTexBumpParameterfvATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_envmap_bumpmap () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_envmap_bumpmap;
}

/*
 * GL_ATI_fragment_shader
 */

static GdkGL_GL_ATI_fragment_shader _procs_GL_ATI_fragment_shader = {
  (GdkGLProc_glGenFragmentShadersATI) -1,
  (GdkGLProc_glBindFragmentShaderATI) -1,
  (GdkGLProc_glDeleteFragmentShaderATI) -1,
  (GdkGLProc_glBeginFragmentShaderATI) -1,
  (GdkGLProc_glEndFragmentShaderATI) -1,
  (GdkGLProc_glPassTexCoordATI) -1,
  (GdkGLProc_glSampleMapATI) -1,
  (GdkGLProc_glColorFragmentOp1ATI) -1,
  (GdkGLProc_glColorFragmentOp2ATI) -1,
  (GdkGLProc_glColorFragmentOp3ATI) -1,
  (GdkGLProc_glAlphaFragmentOp1ATI) -1,
  (GdkGLProc_glAlphaFragmentOp2ATI) -1,
  (GdkGLProc_glAlphaFragmentOp3ATI) -1,
  (GdkGLProc_glSetFragmentShaderConstantATI) -1
};

/* glGenFragmentShadersATI */
GdkGLProc
gdk_gl_get_glGenFragmentShadersATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glGenFragmentShadersATI == (GdkGLProc_glGenFragmentShadersATI) -1)
    _procs_GL_ATI_fragment_shader.glGenFragmentShadersATI =
      (GdkGLProc_glGenFragmentShadersATI) gdk_gl_get_proc_address ("glGenFragmentShadersATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenFragmentShadersATI () - %s",
               (_procs_GL_ATI_fragment_shader.glGenFragmentShadersATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glGenFragmentShadersATI);
}

/* glBindFragmentShaderATI */
GdkGLProc
gdk_gl_get_glBindFragmentShaderATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glBindFragmentShaderATI == (GdkGLProc_glBindFragmentShaderATI) -1)
    _procs_GL_ATI_fragment_shader.glBindFragmentShaderATI =
      (GdkGLProc_glBindFragmentShaderATI) gdk_gl_get_proc_address ("glBindFragmentShaderATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindFragmentShaderATI () - %s",
               (_procs_GL_ATI_fragment_shader.glBindFragmentShaderATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glBindFragmentShaderATI);
}

/* glDeleteFragmentShaderATI */
GdkGLProc
gdk_gl_get_glDeleteFragmentShaderATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glDeleteFragmentShaderATI == (GdkGLProc_glDeleteFragmentShaderATI) -1)
    _procs_GL_ATI_fragment_shader.glDeleteFragmentShaderATI =
      (GdkGLProc_glDeleteFragmentShaderATI) gdk_gl_get_proc_address ("glDeleteFragmentShaderATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteFragmentShaderATI () - %s",
               (_procs_GL_ATI_fragment_shader.glDeleteFragmentShaderATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glDeleteFragmentShaderATI);
}

/* glBeginFragmentShaderATI */
GdkGLProc
gdk_gl_get_glBeginFragmentShaderATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glBeginFragmentShaderATI == (GdkGLProc_glBeginFragmentShaderATI) -1)
    _procs_GL_ATI_fragment_shader.glBeginFragmentShaderATI =
      (GdkGLProc_glBeginFragmentShaderATI) gdk_gl_get_proc_address ("glBeginFragmentShaderATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBeginFragmentShaderATI () - %s",
               (_procs_GL_ATI_fragment_shader.glBeginFragmentShaderATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glBeginFragmentShaderATI);
}

/* glEndFragmentShaderATI */
GdkGLProc
gdk_gl_get_glEndFragmentShaderATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glEndFragmentShaderATI == (GdkGLProc_glEndFragmentShaderATI) -1)
    _procs_GL_ATI_fragment_shader.glEndFragmentShaderATI =
      (GdkGLProc_glEndFragmentShaderATI) gdk_gl_get_proc_address ("glEndFragmentShaderATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEndFragmentShaderATI () - %s",
               (_procs_GL_ATI_fragment_shader.glEndFragmentShaderATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glEndFragmentShaderATI);
}

/* glPassTexCoordATI */
GdkGLProc
gdk_gl_get_glPassTexCoordATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glPassTexCoordATI == (GdkGLProc_glPassTexCoordATI) -1)
    _procs_GL_ATI_fragment_shader.glPassTexCoordATI =
      (GdkGLProc_glPassTexCoordATI) gdk_gl_get_proc_address ("glPassTexCoordATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPassTexCoordATI () - %s",
               (_procs_GL_ATI_fragment_shader.glPassTexCoordATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glPassTexCoordATI);
}

/* glSampleMapATI */
GdkGLProc
gdk_gl_get_glSampleMapATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glSampleMapATI == (GdkGLProc_glSampleMapATI) -1)
    _procs_GL_ATI_fragment_shader.glSampleMapATI =
      (GdkGLProc_glSampleMapATI) gdk_gl_get_proc_address ("glSampleMapATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSampleMapATI () - %s",
               (_procs_GL_ATI_fragment_shader.glSampleMapATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glSampleMapATI);
}

/* glColorFragmentOp1ATI */
GdkGLProc
gdk_gl_get_glColorFragmentOp1ATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glColorFragmentOp1ATI == (GdkGLProc_glColorFragmentOp1ATI) -1)
    _procs_GL_ATI_fragment_shader.glColorFragmentOp1ATI =
      (GdkGLProc_glColorFragmentOp1ATI) gdk_gl_get_proc_address ("glColorFragmentOp1ATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorFragmentOp1ATI () - %s",
               (_procs_GL_ATI_fragment_shader.glColorFragmentOp1ATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glColorFragmentOp1ATI);
}

/* glColorFragmentOp2ATI */
GdkGLProc
gdk_gl_get_glColorFragmentOp2ATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glColorFragmentOp2ATI == (GdkGLProc_glColorFragmentOp2ATI) -1)
    _procs_GL_ATI_fragment_shader.glColorFragmentOp2ATI =
      (GdkGLProc_glColorFragmentOp2ATI) gdk_gl_get_proc_address ("glColorFragmentOp2ATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorFragmentOp2ATI () - %s",
               (_procs_GL_ATI_fragment_shader.glColorFragmentOp2ATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glColorFragmentOp2ATI);
}

/* glColorFragmentOp3ATI */
GdkGLProc
gdk_gl_get_glColorFragmentOp3ATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glColorFragmentOp3ATI == (GdkGLProc_glColorFragmentOp3ATI) -1)
    _procs_GL_ATI_fragment_shader.glColorFragmentOp3ATI =
      (GdkGLProc_glColorFragmentOp3ATI) gdk_gl_get_proc_address ("glColorFragmentOp3ATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColorFragmentOp3ATI () - %s",
               (_procs_GL_ATI_fragment_shader.glColorFragmentOp3ATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glColorFragmentOp3ATI);
}

/* glAlphaFragmentOp1ATI */
GdkGLProc
gdk_gl_get_glAlphaFragmentOp1ATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp1ATI == (GdkGLProc_glAlphaFragmentOp1ATI) -1)
    _procs_GL_ATI_fragment_shader.glAlphaFragmentOp1ATI =
      (GdkGLProc_glAlphaFragmentOp1ATI) gdk_gl_get_proc_address ("glAlphaFragmentOp1ATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAlphaFragmentOp1ATI () - %s",
               (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp1ATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp1ATI);
}

/* glAlphaFragmentOp2ATI */
GdkGLProc
gdk_gl_get_glAlphaFragmentOp2ATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp2ATI == (GdkGLProc_glAlphaFragmentOp2ATI) -1)
    _procs_GL_ATI_fragment_shader.glAlphaFragmentOp2ATI =
      (GdkGLProc_glAlphaFragmentOp2ATI) gdk_gl_get_proc_address ("glAlphaFragmentOp2ATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAlphaFragmentOp2ATI () - %s",
               (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp2ATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp2ATI);
}

/* glAlphaFragmentOp3ATI */
GdkGLProc
gdk_gl_get_glAlphaFragmentOp3ATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp3ATI == (GdkGLProc_glAlphaFragmentOp3ATI) -1)
    _procs_GL_ATI_fragment_shader.glAlphaFragmentOp3ATI =
      (GdkGLProc_glAlphaFragmentOp3ATI) gdk_gl_get_proc_address ("glAlphaFragmentOp3ATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAlphaFragmentOp3ATI () - %s",
               (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp3ATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glAlphaFragmentOp3ATI);
}

/* glSetFragmentShaderConstantATI */
GdkGLProc
gdk_gl_get_glSetFragmentShaderConstantATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_fragment_shader.glSetFragmentShaderConstantATI == (GdkGLProc_glSetFragmentShaderConstantATI) -1)
    _procs_GL_ATI_fragment_shader.glSetFragmentShaderConstantATI =
      (GdkGLProc_glSetFragmentShaderConstantATI) gdk_gl_get_proc_address ("glSetFragmentShaderConstantATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSetFragmentShaderConstantATI () - %s",
               (_procs_GL_ATI_fragment_shader.glSetFragmentShaderConstantATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_fragment_shader.glSetFragmentShaderConstantATI);
}

/* Get GL_ATI_fragment_shader functions */
GdkGL_GL_ATI_fragment_shader *
gdk_gl_get_GL_ATI_fragment_shader (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_fragment_shader");

      if (supported)
        {
          supported &= (gdk_gl_get_glGenFragmentShadersATI () != NULL);
          supported &= (gdk_gl_get_glBindFragmentShaderATI () != NULL);
          supported &= (gdk_gl_get_glDeleteFragmentShaderATI () != NULL);
          supported &= (gdk_gl_get_glBeginFragmentShaderATI () != NULL);
          supported &= (gdk_gl_get_glEndFragmentShaderATI () != NULL);
          supported &= (gdk_gl_get_glPassTexCoordATI () != NULL);
          supported &= (gdk_gl_get_glSampleMapATI () != NULL);
          supported &= (gdk_gl_get_glColorFragmentOp1ATI () != NULL);
          supported &= (gdk_gl_get_glColorFragmentOp2ATI () != NULL);
          supported &= (gdk_gl_get_glColorFragmentOp3ATI () != NULL);
          supported &= (gdk_gl_get_glAlphaFragmentOp1ATI () != NULL);
          supported &= (gdk_gl_get_glAlphaFragmentOp2ATI () != NULL);
          supported &= (gdk_gl_get_glAlphaFragmentOp3ATI () != NULL);
          supported &= (gdk_gl_get_glSetFragmentShaderConstantATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_fragment_shader () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_fragment_shader;
}

/*
 * GL_ATI_pn_triangles
 */

static GdkGL_GL_ATI_pn_triangles _procs_GL_ATI_pn_triangles = {
  (GdkGLProc_glPNTrianglesiATI) -1,
  (GdkGLProc_glPNTrianglesfATI) -1
};

/* glPNTrianglesiATI */
GdkGLProc
gdk_gl_get_glPNTrianglesiATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_pn_triangles.glPNTrianglesiATI == (GdkGLProc_glPNTrianglesiATI) -1)
    _procs_GL_ATI_pn_triangles.glPNTrianglesiATI =
      (GdkGLProc_glPNTrianglesiATI) gdk_gl_get_proc_address ("glPNTrianglesiATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPNTrianglesiATI () - %s",
               (_procs_GL_ATI_pn_triangles.glPNTrianglesiATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_pn_triangles.glPNTrianglesiATI);
}

/* glPNTrianglesfATI */
GdkGLProc
gdk_gl_get_glPNTrianglesfATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_pn_triangles.glPNTrianglesfATI == (GdkGLProc_glPNTrianglesfATI) -1)
    _procs_GL_ATI_pn_triangles.glPNTrianglesfATI =
      (GdkGLProc_glPNTrianglesfATI) gdk_gl_get_proc_address ("glPNTrianglesfATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPNTrianglesfATI () - %s",
               (_procs_GL_ATI_pn_triangles.glPNTrianglesfATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_pn_triangles.glPNTrianglesfATI);
}

/* Get GL_ATI_pn_triangles functions */
GdkGL_GL_ATI_pn_triangles *
gdk_gl_get_GL_ATI_pn_triangles (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_pn_triangles");

      if (supported)
        {
          supported &= (gdk_gl_get_glPNTrianglesiATI () != NULL);
          supported &= (gdk_gl_get_glPNTrianglesfATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_pn_triangles () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_pn_triangles;
}

/*
 * GL_ATI_vertex_array_object
 */

static GdkGL_GL_ATI_vertex_array_object _procs_GL_ATI_vertex_array_object = {
  (GdkGLProc_glNewObjectBufferATI) -1,
  (GdkGLProc_glIsObjectBufferATI) -1,
  (GdkGLProc_glUpdateObjectBufferATI) -1,
  (GdkGLProc_glGetObjectBufferfvATI) -1,
  (GdkGLProc_glGetObjectBufferivATI) -1,
  (GdkGLProc_glFreeObjectBufferATI) -1,
  (GdkGLProc_glArrayObjectATI) -1,
  (GdkGLProc_glGetArrayObjectfvATI) -1,
  (GdkGLProc_glGetArrayObjectivATI) -1,
  (GdkGLProc_glVariantArrayObjectATI) -1,
  (GdkGLProc_glGetVariantArrayObjectfvATI) -1,
  (GdkGLProc_glGetVariantArrayObjectivATI) -1
};

/* glNewObjectBufferATI */
GdkGLProc
gdk_gl_get_glNewObjectBufferATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glNewObjectBufferATI == (GdkGLProc_glNewObjectBufferATI) -1)
    _procs_GL_ATI_vertex_array_object.glNewObjectBufferATI =
      (GdkGLProc_glNewObjectBufferATI) gdk_gl_get_proc_address ("glNewObjectBufferATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNewObjectBufferATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glNewObjectBufferATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glNewObjectBufferATI);
}

/* glIsObjectBufferATI */
GdkGLProc
gdk_gl_get_glIsObjectBufferATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glIsObjectBufferATI == (GdkGLProc_glIsObjectBufferATI) -1)
    _procs_GL_ATI_vertex_array_object.glIsObjectBufferATI =
      (GdkGLProc_glIsObjectBufferATI) gdk_gl_get_proc_address ("glIsObjectBufferATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsObjectBufferATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glIsObjectBufferATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glIsObjectBufferATI);
}

/* glUpdateObjectBufferATI */
GdkGLProc
gdk_gl_get_glUpdateObjectBufferATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glUpdateObjectBufferATI == (GdkGLProc_glUpdateObjectBufferATI) -1)
    _procs_GL_ATI_vertex_array_object.glUpdateObjectBufferATI =
      (GdkGLProc_glUpdateObjectBufferATI) gdk_gl_get_proc_address ("glUpdateObjectBufferATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glUpdateObjectBufferATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glUpdateObjectBufferATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glUpdateObjectBufferATI);
}

/* glGetObjectBufferfvATI */
GdkGLProc
gdk_gl_get_glGetObjectBufferfvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glGetObjectBufferfvATI == (GdkGLProc_glGetObjectBufferfvATI) -1)
    _procs_GL_ATI_vertex_array_object.glGetObjectBufferfvATI =
      (GdkGLProc_glGetObjectBufferfvATI) gdk_gl_get_proc_address ("glGetObjectBufferfvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetObjectBufferfvATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glGetObjectBufferfvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glGetObjectBufferfvATI);
}

/* glGetObjectBufferivATI */
GdkGLProc
gdk_gl_get_glGetObjectBufferivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glGetObjectBufferivATI == (GdkGLProc_glGetObjectBufferivATI) -1)
    _procs_GL_ATI_vertex_array_object.glGetObjectBufferivATI =
      (GdkGLProc_glGetObjectBufferivATI) gdk_gl_get_proc_address ("glGetObjectBufferivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetObjectBufferivATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glGetObjectBufferivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glGetObjectBufferivATI);
}

/* glFreeObjectBufferATI */
GdkGLProc
gdk_gl_get_glFreeObjectBufferATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glFreeObjectBufferATI == (GdkGLProc_glFreeObjectBufferATI) -1)
    _procs_GL_ATI_vertex_array_object.glFreeObjectBufferATI =
      (GdkGLProc_glFreeObjectBufferATI) gdk_gl_get_proc_address ("glFreeObjectBufferATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFreeObjectBufferATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glFreeObjectBufferATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glFreeObjectBufferATI);
}

/* glArrayObjectATI */
GdkGLProc
gdk_gl_get_glArrayObjectATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glArrayObjectATI == (GdkGLProc_glArrayObjectATI) -1)
    _procs_GL_ATI_vertex_array_object.glArrayObjectATI =
      (GdkGLProc_glArrayObjectATI) gdk_gl_get_proc_address ("glArrayObjectATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glArrayObjectATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glArrayObjectATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glArrayObjectATI);
}

/* glGetArrayObjectfvATI */
GdkGLProc
gdk_gl_get_glGetArrayObjectfvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glGetArrayObjectfvATI == (GdkGLProc_glGetArrayObjectfvATI) -1)
    _procs_GL_ATI_vertex_array_object.glGetArrayObjectfvATI =
      (GdkGLProc_glGetArrayObjectfvATI) gdk_gl_get_proc_address ("glGetArrayObjectfvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetArrayObjectfvATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glGetArrayObjectfvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glGetArrayObjectfvATI);
}

/* glGetArrayObjectivATI */
GdkGLProc
gdk_gl_get_glGetArrayObjectivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glGetArrayObjectivATI == (GdkGLProc_glGetArrayObjectivATI) -1)
    _procs_GL_ATI_vertex_array_object.glGetArrayObjectivATI =
      (GdkGLProc_glGetArrayObjectivATI) gdk_gl_get_proc_address ("glGetArrayObjectivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetArrayObjectivATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glGetArrayObjectivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glGetArrayObjectivATI);
}

/* glVariantArrayObjectATI */
GdkGLProc
gdk_gl_get_glVariantArrayObjectATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glVariantArrayObjectATI == (GdkGLProc_glVariantArrayObjectATI) -1)
    _procs_GL_ATI_vertex_array_object.glVariantArrayObjectATI =
      (GdkGLProc_glVariantArrayObjectATI) gdk_gl_get_proc_address ("glVariantArrayObjectATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantArrayObjectATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glVariantArrayObjectATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glVariantArrayObjectATI);
}

/* glGetVariantArrayObjectfvATI */
GdkGLProc
gdk_gl_get_glGetVariantArrayObjectfvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectfvATI == (GdkGLProc_glGetVariantArrayObjectfvATI) -1)
    _procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectfvATI =
      (GdkGLProc_glGetVariantArrayObjectfvATI) gdk_gl_get_proc_address ("glGetVariantArrayObjectfvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVariantArrayObjectfvATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectfvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectfvATI);
}

/* glGetVariantArrayObjectivATI */
GdkGLProc
gdk_gl_get_glGetVariantArrayObjectivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectivATI == (GdkGLProc_glGetVariantArrayObjectivATI) -1)
    _procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectivATI =
      (GdkGLProc_glGetVariantArrayObjectivATI) gdk_gl_get_proc_address ("glGetVariantArrayObjectivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVariantArrayObjectivATI () - %s",
               (_procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_array_object.glGetVariantArrayObjectivATI);
}

/* Get GL_ATI_vertex_array_object functions */
GdkGL_GL_ATI_vertex_array_object *
gdk_gl_get_GL_ATI_vertex_array_object (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_vertex_array_object");

      if (supported)
        {
          supported &= (gdk_gl_get_glNewObjectBufferATI () != NULL);
          supported &= (gdk_gl_get_glIsObjectBufferATI () != NULL);
          supported &= (gdk_gl_get_glUpdateObjectBufferATI () != NULL);
          supported &= (gdk_gl_get_glGetObjectBufferfvATI () != NULL);
          supported &= (gdk_gl_get_glGetObjectBufferivATI () != NULL);
          supported &= (gdk_gl_get_glFreeObjectBufferATI () != NULL);
          supported &= (gdk_gl_get_glArrayObjectATI () != NULL);
          supported &= (gdk_gl_get_glGetArrayObjectfvATI () != NULL);
          supported &= (gdk_gl_get_glGetArrayObjectivATI () != NULL);
          supported &= (gdk_gl_get_glVariantArrayObjectATI () != NULL);
          supported &= (gdk_gl_get_glGetVariantArrayObjectfvATI () != NULL);
          supported &= (gdk_gl_get_glGetVariantArrayObjectivATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_vertex_array_object () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_vertex_array_object;
}

/*
 * GL_EXT_vertex_shader
 */

static GdkGL_GL_EXT_vertex_shader _procs_GL_EXT_vertex_shader = {
  (GdkGLProc_glBeginVertexShaderEXT) -1,
  (GdkGLProc_glEndVertexShaderEXT) -1,
  (GdkGLProc_glBindVertexShaderEXT) -1,
  (GdkGLProc_glGenVertexShadersEXT) -1,
  (GdkGLProc_glDeleteVertexShaderEXT) -1,
  (GdkGLProc_glShaderOp1EXT) -1,
  (GdkGLProc_glShaderOp2EXT) -1,
  (GdkGLProc_glShaderOp3EXT) -1,
  (GdkGLProc_glSwizzleEXT) -1,
  (GdkGLProc_glWriteMaskEXT) -1,
  (GdkGLProc_glInsertComponentEXT) -1,
  (GdkGLProc_glExtractComponentEXT) -1,
  (GdkGLProc_glGenSymbolsEXT) -1,
  (GdkGLProc_glSetInvariantEXT) -1,
  (GdkGLProc_glSetLocalConstantEXT) -1,
  (GdkGLProc_glVariantbvEXT) -1,
  (GdkGLProc_glVariantsvEXT) -1,
  (GdkGLProc_glVariantivEXT) -1,
  (GdkGLProc_glVariantfvEXT) -1,
  (GdkGLProc_glVariantdvEXT) -1,
  (GdkGLProc_glVariantubvEXT) -1,
  (GdkGLProc_glVariantusvEXT) -1,
  (GdkGLProc_glVariantuivEXT) -1,
  (GdkGLProc_glVariantPointerEXT) -1,
  (GdkGLProc_glEnableVariantClientStateEXT) -1,
  (GdkGLProc_glDisableVariantClientStateEXT) -1,
  (GdkGLProc_glBindLightParameterEXT) -1,
  (GdkGLProc_glBindMaterialParameterEXT) -1,
  (GdkGLProc_glBindTexGenParameterEXT) -1,
  (GdkGLProc_glBindTextureUnitParameterEXT) -1,
  (GdkGLProc_glBindParameterEXT) -1,
  (GdkGLProc_glIsVariantEnabledEXT) -1,
  (GdkGLProc_glGetVariantBooleanvEXT) -1,
  (GdkGLProc_glGetVariantIntegervEXT) -1,
  (GdkGLProc_glGetVariantFloatvEXT) -1,
  (GdkGLProc_glGetVariantPointervEXT) -1,
  (GdkGLProc_glGetInvariantBooleanvEXT) -1,
  (GdkGLProc_glGetInvariantIntegervEXT) -1,
  (GdkGLProc_glGetInvariantFloatvEXT) -1,
  (GdkGLProc_glGetLocalConstantBooleanvEXT) -1,
  (GdkGLProc_glGetLocalConstantIntegervEXT) -1,
  (GdkGLProc_glGetLocalConstantFloatvEXT) -1
};

/* glBeginVertexShaderEXT */
GdkGLProc
gdk_gl_get_glBeginVertexShaderEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBeginVertexShaderEXT == (GdkGLProc_glBeginVertexShaderEXT) -1)
    _procs_GL_EXT_vertex_shader.glBeginVertexShaderEXT =
      (GdkGLProc_glBeginVertexShaderEXT) gdk_gl_get_proc_address ("glBeginVertexShaderEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBeginVertexShaderEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBeginVertexShaderEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBeginVertexShaderEXT);
}

/* glEndVertexShaderEXT */
GdkGLProc
gdk_gl_get_glEndVertexShaderEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glEndVertexShaderEXT == (GdkGLProc_glEndVertexShaderEXT) -1)
    _procs_GL_EXT_vertex_shader.glEndVertexShaderEXT =
      (GdkGLProc_glEndVertexShaderEXT) gdk_gl_get_proc_address ("glEndVertexShaderEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEndVertexShaderEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glEndVertexShaderEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glEndVertexShaderEXT);
}

/* glBindVertexShaderEXT */
GdkGLProc
gdk_gl_get_glBindVertexShaderEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBindVertexShaderEXT == (GdkGLProc_glBindVertexShaderEXT) -1)
    _procs_GL_EXT_vertex_shader.glBindVertexShaderEXT =
      (GdkGLProc_glBindVertexShaderEXT) gdk_gl_get_proc_address ("glBindVertexShaderEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindVertexShaderEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBindVertexShaderEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBindVertexShaderEXT);
}

/* glGenVertexShadersEXT */
GdkGLProc
gdk_gl_get_glGenVertexShadersEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGenVertexShadersEXT == (GdkGLProc_glGenVertexShadersEXT) -1)
    _procs_GL_EXT_vertex_shader.glGenVertexShadersEXT =
      (GdkGLProc_glGenVertexShadersEXT) gdk_gl_get_proc_address ("glGenVertexShadersEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenVertexShadersEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGenVertexShadersEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGenVertexShadersEXT);
}

/* glDeleteVertexShaderEXT */
GdkGLProc
gdk_gl_get_glDeleteVertexShaderEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glDeleteVertexShaderEXT == (GdkGLProc_glDeleteVertexShaderEXT) -1)
    _procs_GL_EXT_vertex_shader.glDeleteVertexShaderEXT =
      (GdkGLProc_glDeleteVertexShaderEXT) gdk_gl_get_proc_address ("glDeleteVertexShaderEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteVertexShaderEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glDeleteVertexShaderEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glDeleteVertexShaderEXT);
}

/* glShaderOp1EXT */
GdkGLProc
gdk_gl_get_glShaderOp1EXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glShaderOp1EXT == (GdkGLProc_glShaderOp1EXT) -1)
    _procs_GL_EXT_vertex_shader.glShaderOp1EXT =
      (GdkGLProc_glShaderOp1EXT) gdk_gl_get_proc_address ("glShaderOp1EXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glShaderOp1EXT () - %s",
               (_procs_GL_EXT_vertex_shader.glShaderOp1EXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glShaderOp1EXT);
}

/* glShaderOp2EXT */
GdkGLProc
gdk_gl_get_glShaderOp2EXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glShaderOp2EXT == (GdkGLProc_glShaderOp2EXT) -1)
    _procs_GL_EXT_vertex_shader.glShaderOp2EXT =
      (GdkGLProc_glShaderOp2EXT) gdk_gl_get_proc_address ("glShaderOp2EXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glShaderOp2EXT () - %s",
               (_procs_GL_EXT_vertex_shader.glShaderOp2EXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glShaderOp2EXT);
}

/* glShaderOp3EXT */
GdkGLProc
gdk_gl_get_glShaderOp3EXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glShaderOp3EXT == (GdkGLProc_glShaderOp3EXT) -1)
    _procs_GL_EXT_vertex_shader.glShaderOp3EXT =
      (GdkGLProc_glShaderOp3EXT) gdk_gl_get_proc_address ("glShaderOp3EXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glShaderOp3EXT () - %s",
               (_procs_GL_EXT_vertex_shader.glShaderOp3EXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glShaderOp3EXT);
}

/* glSwizzleEXT */
GdkGLProc
gdk_gl_get_glSwizzleEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glSwizzleEXT == (GdkGLProc_glSwizzleEXT) -1)
    _procs_GL_EXT_vertex_shader.glSwizzleEXT =
      (GdkGLProc_glSwizzleEXT) gdk_gl_get_proc_address ("glSwizzleEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSwizzleEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glSwizzleEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glSwizzleEXT);
}

/* glWriteMaskEXT */
GdkGLProc
gdk_gl_get_glWriteMaskEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glWriteMaskEXT == (GdkGLProc_glWriteMaskEXT) -1)
    _procs_GL_EXT_vertex_shader.glWriteMaskEXT =
      (GdkGLProc_glWriteMaskEXT) gdk_gl_get_proc_address ("glWriteMaskEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWriteMaskEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glWriteMaskEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glWriteMaskEXT);
}

/* glInsertComponentEXT */
GdkGLProc
gdk_gl_get_glInsertComponentEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glInsertComponentEXT == (GdkGLProc_glInsertComponentEXT) -1)
    _procs_GL_EXT_vertex_shader.glInsertComponentEXT =
      (GdkGLProc_glInsertComponentEXT) gdk_gl_get_proc_address ("glInsertComponentEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glInsertComponentEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glInsertComponentEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glInsertComponentEXT);
}

/* glExtractComponentEXT */
GdkGLProc
gdk_gl_get_glExtractComponentEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glExtractComponentEXT == (GdkGLProc_glExtractComponentEXT) -1)
    _procs_GL_EXT_vertex_shader.glExtractComponentEXT =
      (GdkGLProc_glExtractComponentEXT) gdk_gl_get_proc_address ("glExtractComponentEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glExtractComponentEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glExtractComponentEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glExtractComponentEXT);
}

/* glGenSymbolsEXT */
GdkGLProc
gdk_gl_get_glGenSymbolsEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGenSymbolsEXT == (GdkGLProc_glGenSymbolsEXT) -1)
    _procs_GL_EXT_vertex_shader.glGenSymbolsEXT =
      (GdkGLProc_glGenSymbolsEXT) gdk_gl_get_proc_address ("glGenSymbolsEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenSymbolsEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGenSymbolsEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGenSymbolsEXT);
}

/* glSetInvariantEXT */
GdkGLProc
gdk_gl_get_glSetInvariantEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glSetInvariantEXT == (GdkGLProc_glSetInvariantEXT) -1)
    _procs_GL_EXT_vertex_shader.glSetInvariantEXT =
      (GdkGLProc_glSetInvariantEXT) gdk_gl_get_proc_address ("glSetInvariantEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSetInvariantEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glSetInvariantEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glSetInvariantEXT);
}

/* glSetLocalConstantEXT */
GdkGLProc
gdk_gl_get_glSetLocalConstantEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glSetLocalConstantEXT == (GdkGLProc_glSetLocalConstantEXT) -1)
    _procs_GL_EXT_vertex_shader.glSetLocalConstantEXT =
      (GdkGLProc_glSetLocalConstantEXT) gdk_gl_get_proc_address ("glSetLocalConstantEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSetLocalConstantEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glSetLocalConstantEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glSetLocalConstantEXT);
}

/* glVariantbvEXT */
GdkGLProc
gdk_gl_get_glVariantbvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantbvEXT == (GdkGLProc_glVariantbvEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantbvEXT =
      (GdkGLProc_glVariantbvEXT) gdk_gl_get_proc_address ("glVariantbvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantbvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantbvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantbvEXT);
}

/* glVariantsvEXT */
GdkGLProc
gdk_gl_get_glVariantsvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantsvEXT == (GdkGLProc_glVariantsvEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantsvEXT =
      (GdkGLProc_glVariantsvEXT) gdk_gl_get_proc_address ("glVariantsvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantsvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantsvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantsvEXT);
}

/* glVariantivEXT */
GdkGLProc
gdk_gl_get_glVariantivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantivEXT == (GdkGLProc_glVariantivEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantivEXT =
      (GdkGLProc_glVariantivEXT) gdk_gl_get_proc_address ("glVariantivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantivEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantivEXT);
}

/* glVariantfvEXT */
GdkGLProc
gdk_gl_get_glVariantfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantfvEXT == (GdkGLProc_glVariantfvEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantfvEXT =
      (GdkGLProc_glVariantfvEXT) gdk_gl_get_proc_address ("glVariantfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantfvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantfvEXT);
}

/* glVariantdvEXT */
GdkGLProc
gdk_gl_get_glVariantdvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantdvEXT == (GdkGLProc_glVariantdvEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantdvEXT =
      (GdkGLProc_glVariantdvEXT) gdk_gl_get_proc_address ("glVariantdvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantdvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantdvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantdvEXT);
}

/* glVariantubvEXT */
GdkGLProc
gdk_gl_get_glVariantubvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantubvEXT == (GdkGLProc_glVariantubvEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantubvEXT =
      (GdkGLProc_glVariantubvEXT) gdk_gl_get_proc_address ("glVariantubvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantubvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantubvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantubvEXT);
}

/* glVariantusvEXT */
GdkGLProc
gdk_gl_get_glVariantusvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantusvEXT == (GdkGLProc_glVariantusvEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantusvEXT =
      (GdkGLProc_glVariantusvEXT) gdk_gl_get_proc_address ("glVariantusvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantusvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantusvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantusvEXT);
}

/* glVariantuivEXT */
GdkGLProc
gdk_gl_get_glVariantuivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantuivEXT == (GdkGLProc_glVariantuivEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantuivEXT =
      (GdkGLProc_glVariantuivEXT) gdk_gl_get_proc_address ("glVariantuivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantuivEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantuivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantuivEXT);
}

/* glVariantPointerEXT */
GdkGLProc
gdk_gl_get_glVariantPointerEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glVariantPointerEXT == (GdkGLProc_glVariantPointerEXT) -1)
    _procs_GL_EXT_vertex_shader.glVariantPointerEXT =
      (GdkGLProc_glVariantPointerEXT) gdk_gl_get_proc_address ("glVariantPointerEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVariantPointerEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glVariantPointerEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glVariantPointerEXT);
}

/* glEnableVariantClientStateEXT */
GdkGLProc
gdk_gl_get_glEnableVariantClientStateEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glEnableVariantClientStateEXT == (GdkGLProc_glEnableVariantClientStateEXT) -1)
    _procs_GL_EXT_vertex_shader.glEnableVariantClientStateEXT =
      (GdkGLProc_glEnableVariantClientStateEXT) gdk_gl_get_proc_address ("glEnableVariantClientStateEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEnableVariantClientStateEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glEnableVariantClientStateEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glEnableVariantClientStateEXT);
}

/* glDisableVariantClientStateEXT */
GdkGLProc
gdk_gl_get_glDisableVariantClientStateEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glDisableVariantClientStateEXT == (GdkGLProc_glDisableVariantClientStateEXT) -1)
    _procs_GL_EXT_vertex_shader.glDisableVariantClientStateEXT =
      (GdkGLProc_glDisableVariantClientStateEXT) gdk_gl_get_proc_address ("glDisableVariantClientStateEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDisableVariantClientStateEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glDisableVariantClientStateEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glDisableVariantClientStateEXT);
}

/* glBindLightParameterEXT */
GdkGLProc
gdk_gl_get_glBindLightParameterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBindLightParameterEXT == (GdkGLProc_glBindLightParameterEXT) -1)
    _procs_GL_EXT_vertex_shader.glBindLightParameterEXT =
      (GdkGLProc_glBindLightParameterEXT) gdk_gl_get_proc_address ("glBindLightParameterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindLightParameterEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBindLightParameterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBindLightParameterEXT);
}

/* glBindMaterialParameterEXT */
GdkGLProc
gdk_gl_get_glBindMaterialParameterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBindMaterialParameterEXT == (GdkGLProc_glBindMaterialParameterEXT) -1)
    _procs_GL_EXT_vertex_shader.glBindMaterialParameterEXT =
      (GdkGLProc_glBindMaterialParameterEXT) gdk_gl_get_proc_address ("glBindMaterialParameterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindMaterialParameterEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBindMaterialParameterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBindMaterialParameterEXT);
}

/* glBindTexGenParameterEXT */
GdkGLProc
gdk_gl_get_glBindTexGenParameterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBindTexGenParameterEXT == (GdkGLProc_glBindTexGenParameterEXT) -1)
    _procs_GL_EXT_vertex_shader.glBindTexGenParameterEXT =
      (GdkGLProc_glBindTexGenParameterEXT) gdk_gl_get_proc_address ("glBindTexGenParameterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindTexGenParameterEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBindTexGenParameterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBindTexGenParameterEXT);
}

/* glBindTextureUnitParameterEXT */
GdkGLProc
gdk_gl_get_glBindTextureUnitParameterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBindTextureUnitParameterEXT == (GdkGLProc_glBindTextureUnitParameterEXT) -1)
    _procs_GL_EXT_vertex_shader.glBindTextureUnitParameterEXT =
      (GdkGLProc_glBindTextureUnitParameterEXT) gdk_gl_get_proc_address ("glBindTextureUnitParameterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindTextureUnitParameterEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBindTextureUnitParameterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBindTextureUnitParameterEXT);
}

/* glBindParameterEXT */
GdkGLProc
gdk_gl_get_glBindParameterEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glBindParameterEXT == (GdkGLProc_glBindParameterEXT) -1)
    _procs_GL_EXT_vertex_shader.glBindParameterEXT =
      (GdkGLProc_glBindParameterEXT) gdk_gl_get_proc_address ("glBindParameterEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindParameterEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glBindParameterEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glBindParameterEXT);
}

/* glIsVariantEnabledEXT */
GdkGLProc
gdk_gl_get_glIsVariantEnabledEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glIsVariantEnabledEXT == (GdkGLProc_glIsVariantEnabledEXT) -1)
    _procs_GL_EXT_vertex_shader.glIsVariantEnabledEXT =
      (GdkGLProc_glIsVariantEnabledEXT) gdk_gl_get_proc_address ("glIsVariantEnabledEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsVariantEnabledEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glIsVariantEnabledEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glIsVariantEnabledEXT);
}

/* glGetVariantBooleanvEXT */
GdkGLProc
gdk_gl_get_glGetVariantBooleanvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetVariantBooleanvEXT == (GdkGLProc_glGetVariantBooleanvEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetVariantBooleanvEXT =
      (GdkGLProc_glGetVariantBooleanvEXT) gdk_gl_get_proc_address ("glGetVariantBooleanvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVariantBooleanvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetVariantBooleanvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetVariantBooleanvEXT);
}

/* glGetVariantIntegervEXT */
GdkGLProc
gdk_gl_get_glGetVariantIntegervEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetVariantIntegervEXT == (GdkGLProc_glGetVariantIntegervEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetVariantIntegervEXT =
      (GdkGLProc_glGetVariantIntegervEXT) gdk_gl_get_proc_address ("glGetVariantIntegervEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVariantIntegervEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetVariantIntegervEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetVariantIntegervEXT);
}

/* glGetVariantFloatvEXT */
GdkGLProc
gdk_gl_get_glGetVariantFloatvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetVariantFloatvEXT == (GdkGLProc_glGetVariantFloatvEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetVariantFloatvEXT =
      (GdkGLProc_glGetVariantFloatvEXT) gdk_gl_get_proc_address ("glGetVariantFloatvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVariantFloatvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetVariantFloatvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetVariantFloatvEXT);
}

/* glGetVariantPointervEXT */
GdkGLProc
gdk_gl_get_glGetVariantPointervEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetVariantPointervEXT == (GdkGLProc_glGetVariantPointervEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetVariantPointervEXT =
      (GdkGLProc_glGetVariantPointervEXT) gdk_gl_get_proc_address ("glGetVariantPointervEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVariantPointervEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetVariantPointervEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetVariantPointervEXT);
}

/* glGetInvariantBooleanvEXT */
GdkGLProc
gdk_gl_get_glGetInvariantBooleanvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetInvariantBooleanvEXT == (GdkGLProc_glGetInvariantBooleanvEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetInvariantBooleanvEXT =
      (GdkGLProc_glGetInvariantBooleanvEXT) gdk_gl_get_proc_address ("glGetInvariantBooleanvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetInvariantBooleanvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetInvariantBooleanvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetInvariantBooleanvEXT);
}

/* glGetInvariantIntegervEXT */
GdkGLProc
gdk_gl_get_glGetInvariantIntegervEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetInvariantIntegervEXT == (GdkGLProc_glGetInvariantIntegervEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetInvariantIntegervEXT =
      (GdkGLProc_glGetInvariantIntegervEXT) gdk_gl_get_proc_address ("glGetInvariantIntegervEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetInvariantIntegervEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetInvariantIntegervEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetInvariantIntegervEXT);
}

/* glGetInvariantFloatvEXT */
GdkGLProc
gdk_gl_get_glGetInvariantFloatvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetInvariantFloatvEXT == (GdkGLProc_glGetInvariantFloatvEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetInvariantFloatvEXT =
      (GdkGLProc_glGetInvariantFloatvEXT) gdk_gl_get_proc_address ("glGetInvariantFloatvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetInvariantFloatvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetInvariantFloatvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetInvariantFloatvEXT);
}

/* glGetLocalConstantBooleanvEXT */
GdkGLProc
gdk_gl_get_glGetLocalConstantBooleanvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetLocalConstantBooleanvEXT == (GdkGLProc_glGetLocalConstantBooleanvEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetLocalConstantBooleanvEXT =
      (GdkGLProc_glGetLocalConstantBooleanvEXT) gdk_gl_get_proc_address ("glGetLocalConstantBooleanvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetLocalConstantBooleanvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetLocalConstantBooleanvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetLocalConstantBooleanvEXT);
}

/* glGetLocalConstantIntegervEXT */
GdkGLProc
gdk_gl_get_glGetLocalConstantIntegervEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetLocalConstantIntegervEXT == (GdkGLProc_glGetLocalConstantIntegervEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetLocalConstantIntegervEXT =
      (GdkGLProc_glGetLocalConstantIntegervEXT) gdk_gl_get_proc_address ("glGetLocalConstantIntegervEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetLocalConstantIntegervEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetLocalConstantIntegervEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetLocalConstantIntegervEXT);
}

/* glGetLocalConstantFloatvEXT */
GdkGLProc
gdk_gl_get_glGetLocalConstantFloatvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_vertex_shader.glGetLocalConstantFloatvEXT == (GdkGLProc_glGetLocalConstantFloatvEXT) -1)
    _procs_GL_EXT_vertex_shader.glGetLocalConstantFloatvEXT =
      (GdkGLProc_glGetLocalConstantFloatvEXT) gdk_gl_get_proc_address ("glGetLocalConstantFloatvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetLocalConstantFloatvEXT () - %s",
               (_procs_GL_EXT_vertex_shader.glGetLocalConstantFloatvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_vertex_shader.glGetLocalConstantFloatvEXT);
}

/* Get GL_EXT_vertex_shader functions */
GdkGL_GL_EXT_vertex_shader *
gdk_gl_get_GL_EXT_vertex_shader (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_vertex_shader");

      if (supported)
        {
          supported &= (gdk_gl_get_glBeginVertexShaderEXT () != NULL);
          supported &= (gdk_gl_get_glEndVertexShaderEXT () != NULL);
          supported &= (gdk_gl_get_glBindVertexShaderEXT () != NULL);
          supported &= (gdk_gl_get_glGenVertexShadersEXT () != NULL);
          supported &= (gdk_gl_get_glDeleteVertexShaderEXT () != NULL);
          supported &= (gdk_gl_get_glShaderOp1EXT () != NULL);
          supported &= (gdk_gl_get_glShaderOp2EXT () != NULL);
          supported &= (gdk_gl_get_glShaderOp3EXT () != NULL);
          supported &= (gdk_gl_get_glSwizzleEXT () != NULL);
          supported &= (gdk_gl_get_glWriteMaskEXT () != NULL);
          supported &= (gdk_gl_get_glInsertComponentEXT () != NULL);
          supported &= (gdk_gl_get_glExtractComponentEXT () != NULL);
          supported &= (gdk_gl_get_glGenSymbolsEXT () != NULL);
          supported &= (gdk_gl_get_glSetInvariantEXT () != NULL);
          supported &= (gdk_gl_get_glSetLocalConstantEXT () != NULL);
          supported &= (gdk_gl_get_glVariantbvEXT () != NULL);
          supported &= (gdk_gl_get_glVariantsvEXT () != NULL);
          supported &= (gdk_gl_get_glVariantivEXT () != NULL);
          supported &= (gdk_gl_get_glVariantfvEXT () != NULL);
          supported &= (gdk_gl_get_glVariantdvEXT () != NULL);
          supported &= (gdk_gl_get_glVariantubvEXT () != NULL);
          supported &= (gdk_gl_get_glVariantusvEXT () != NULL);
          supported &= (gdk_gl_get_glVariantuivEXT () != NULL);
          supported &= (gdk_gl_get_glVariantPointerEXT () != NULL);
          supported &= (gdk_gl_get_glEnableVariantClientStateEXT () != NULL);
          supported &= (gdk_gl_get_glDisableVariantClientStateEXT () != NULL);
          supported &= (gdk_gl_get_glBindLightParameterEXT () != NULL);
          supported &= (gdk_gl_get_glBindMaterialParameterEXT () != NULL);
          supported &= (gdk_gl_get_glBindTexGenParameterEXT () != NULL);
          supported &= (gdk_gl_get_glBindTextureUnitParameterEXT () != NULL);
          supported &= (gdk_gl_get_glBindParameterEXT () != NULL);
          supported &= (gdk_gl_get_glIsVariantEnabledEXT () != NULL);
          supported &= (gdk_gl_get_glGetVariantBooleanvEXT () != NULL);
          supported &= (gdk_gl_get_glGetVariantIntegervEXT () != NULL);
          supported &= (gdk_gl_get_glGetVariantFloatvEXT () != NULL);
          supported &= (gdk_gl_get_glGetVariantPointervEXT () != NULL);
          supported &= (gdk_gl_get_glGetInvariantBooleanvEXT () != NULL);
          supported &= (gdk_gl_get_glGetInvariantIntegervEXT () != NULL);
          supported &= (gdk_gl_get_glGetInvariantFloatvEXT () != NULL);
          supported &= (gdk_gl_get_glGetLocalConstantBooleanvEXT () != NULL);
          supported &= (gdk_gl_get_glGetLocalConstantIntegervEXT () != NULL);
          supported &= (gdk_gl_get_glGetLocalConstantFloatvEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_vertex_shader () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_vertex_shader;
}

/*
 * GL_ATI_vertex_streams
 */

static GdkGL_GL_ATI_vertex_streams _procs_GL_ATI_vertex_streams = {
  (GdkGLProc_glVertexStream1sATI) -1,
  (GdkGLProc_glVertexStream1svATI) -1,
  (GdkGLProc_glVertexStream1iATI) -1,
  (GdkGLProc_glVertexStream1ivATI) -1,
  (GdkGLProc_glVertexStream1fATI) -1,
  (GdkGLProc_glVertexStream1fvATI) -1,
  (GdkGLProc_glVertexStream1dATI) -1,
  (GdkGLProc_glVertexStream1dvATI) -1,
  (GdkGLProc_glVertexStream2sATI) -1,
  (GdkGLProc_glVertexStream2svATI) -1,
  (GdkGLProc_glVertexStream2iATI) -1,
  (GdkGLProc_glVertexStream2ivATI) -1,
  (GdkGLProc_glVertexStream2fATI) -1,
  (GdkGLProc_glVertexStream2fvATI) -1,
  (GdkGLProc_glVertexStream2dATI) -1,
  (GdkGLProc_glVertexStream2dvATI) -1,
  (GdkGLProc_glVertexStream3sATI) -1,
  (GdkGLProc_glVertexStream3svATI) -1,
  (GdkGLProc_glVertexStream3iATI) -1,
  (GdkGLProc_glVertexStream3ivATI) -1,
  (GdkGLProc_glVertexStream3fATI) -1,
  (GdkGLProc_glVertexStream3fvATI) -1,
  (GdkGLProc_glVertexStream3dATI) -1,
  (GdkGLProc_glVertexStream3dvATI) -1,
  (GdkGLProc_glVertexStream4sATI) -1,
  (GdkGLProc_glVertexStream4svATI) -1,
  (GdkGLProc_glVertexStream4iATI) -1,
  (GdkGLProc_glVertexStream4ivATI) -1,
  (GdkGLProc_glVertexStream4fATI) -1,
  (GdkGLProc_glVertexStream4fvATI) -1,
  (GdkGLProc_glVertexStream4dATI) -1,
  (GdkGLProc_glVertexStream4dvATI) -1,
  (GdkGLProc_glNormalStream3bATI) -1,
  (GdkGLProc_glNormalStream3bvATI) -1,
  (GdkGLProc_glNormalStream3sATI) -1,
  (GdkGLProc_glNormalStream3svATI) -1,
  (GdkGLProc_glNormalStream3iATI) -1,
  (GdkGLProc_glNormalStream3ivATI) -1,
  (GdkGLProc_glNormalStream3fATI) -1,
  (GdkGLProc_glNormalStream3fvATI) -1,
  (GdkGLProc_glNormalStream3dATI) -1,
  (GdkGLProc_glNormalStream3dvATI) -1,
  (GdkGLProc_glClientActiveVertexStreamATI) -1,
  (GdkGLProc_glVertexBlendEnviATI) -1,
  (GdkGLProc_glVertexBlendEnvfATI) -1
};

/* glVertexStream1sATI */
GdkGLProc
gdk_gl_get_glVertexStream1sATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1sATI == (GdkGLProc_glVertexStream1sATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1sATI =
      (GdkGLProc_glVertexStream1sATI) gdk_gl_get_proc_address ("glVertexStream1sATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1sATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1sATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1sATI);
}

/* glVertexStream1svATI */
GdkGLProc
gdk_gl_get_glVertexStream1svATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1svATI == (GdkGLProc_glVertexStream1svATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1svATI =
      (GdkGLProc_glVertexStream1svATI) gdk_gl_get_proc_address ("glVertexStream1svATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1svATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1svATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1svATI);
}

/* glVertexStream1iATI */
GdkGLProc
gdk_gl_get_glVertexStream1iATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1iATI == (GdkGLProc_glVertexStream1iATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1iATI =
      (GdkGLProc_glVertexStream1iATI) gdk_gl_get_proc_address ("glVertexStream1iATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1iATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1iATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1iATI);
}

/* glVertexStream1ivATI */
GdkGLProc
gdk_gl_get_glVertexStream1ivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1ivATI == (GdkGLProc_glVertexStream1ivATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1ivATI =
      (GdkGLProc_glVertexStream1ivATI) gdk_gl_get_proc_address ("glVertexStream1ivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1ivATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1ivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1ivATI);
}

/* glVertexStream1fATI */
GdkGLProc
gdk_gl_get_glVertexStream1fATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1fATI == (GdkGLProc_glVertexStream1fATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1fATI =
      (GdkGLProc_glVertexStream1fATI) gdk_gl_get_proc_address ("glVertexStream1fATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1fATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1fATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1fATI);
}

/* glVertexStream1fvATI */
GdkGLProc
gdk_gl_get_glVertexStream1fvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1fvATI == (GdkGLProc_glVertexStream1fvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1fvATI =
      (GdkGLProc_glVertexStream1fvATI) gdk_gl_get_proc_address ("glVertexStream1fvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1fvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1fvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1fvATI);
}

/* glVertexStream1dATI */
GdkGLProc
gdk_gl_get_glVertexStream1dATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1dATI == (GdkGLProc_glVertexStream1dATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1dATI =
      (GdkGLProc_glVertexStream1dATI) gdk_gl_get_proc_address ("glVertexStream1dATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1dATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1dATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1dATI);
}

/* glVertexStream1dvATI */
GdkGLProc
gdk_gl_get_glVertexStream1dvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream1dvATI == (GdkGLProc_glVertexStream1dvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream1dvATI =
      (GdkGLProc_glVertexStream1dvATI) gdk_gl_get_proc_address ("glVertexStream1dvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream1dvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream1dvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream1dvATI);
}

/* glVertexStream2sATI */
GdkGLProc
gdk_gl_get_glVertexStream2sATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2sATI == (GdkGLProc_glVertexStream2sATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2sATI =
      (GdkGLProc_glVertexStream2sATI) gdk_gl_get_proc_address ("glVertexStream2sATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2sATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2sATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2sATI);
}

/* glVertexStream2svATI */
GdkGLProc
gdk_gl_get_glVertexStream2svATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2svATI == (GdkGLProc_glVertexStream2svATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2svATI =
      (GdkGLProc_glVertexStream2svATI) gdk_gl_get_proc_address ("glVertexStream2svATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2svATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2svATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2svATI);
}

/* glVertexStream2iATI */
GdkGLProc
gdk_gl_get_glVertexStream2iATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2iATI == (GdkGLProc_glVertexStream2iATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2iATI =
      (GdkGLProc_glVertexStream2iATI) gdk_gl_get_proc_address ("glVertexStream2iATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2iATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2iATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2iATI);
}

/* glVertexStream2ivATI */
GdkGLProc
gdk_gl_get_glVertexStream2ivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2ivATI == (GdkGLProc_glVertexStream2ivATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2ivATI =
      (GdkGLProc_glVertexStream2ivATI) gdk_gl_get_proc_address ("glVertexStream2ivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2ivATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2ivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2ivATI);
}

/* glVertexStream2fATI */
GdkGLProc
gdk_gl_get_glVertexStream2fATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2fATI == (GdkGLProc_glVertexStream2fATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2fATI =
      (GdkGLProc_glVertexStream2fATI) gdk_gl_get_proc_address ("glVertexStream2fATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2fATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2fATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2fATI);
}

/* glVertexStream2fvATI */
GdkGLProc
gdk_gl_get_glVertexStream2fvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2fvATI == (GdkGLProc_glVertexStream2fvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2fvATI =
      (GdkGLProc_glVertexStream2fvATI) gdk_gl_get_proc_address ("glVertexStream2fvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2fvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2fvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2fvATI);
}

/* glVertexStream2dATI */
GdkGLProc
gdk_gl_get_glVertexStream2dATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2dATI == (GdkGLProc_glVertexStream2dATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2dATI =
      (GdkGLProc_glVertexStream2dATI) gdk_gl_get_proc_address ("glVertexStream2dATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2dATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2dATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2dATI);
}

/* glVertexStream2dvATI */
GdkGLProc
gdk_gl_get_glVertexStream2dvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream2dvATI == (GdkGLProc_glVertexStream2dvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream2dvATI =
      (GdkGLProc_glVertexStream2dvATI) gdk_gl_get_proc_address ("glVertexStream2dvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream2dvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream2dvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream2dvATI);
}

/* glVertexStream3sATI */
GdkGLProc
gdk_gl_get_glVertexStream3sATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3sATI == (GdkGLProc_glVertexStream3sATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3sATI =
      (GdkGLProc_glVertexStream3sATI) gdk_gl_get_proc_address ("glVertexStream3sATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3sATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3sATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3sATI);
}

/* glVertexStream3svATI */
GdkGLProc
gdk_gl_get_glVertexStream3svATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3svATI == (GdkGLProc_glVertexStream3svATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3svATI =
      (GdkGLProc_glVertexStream3svATI) gdk_gl_get_proc_address ("glVertexStream3svATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3svATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3svATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3svATI);
}

/* glVertexStream3iATI */
GdkGLProc
gdk_gl_get_glVertexStream3iATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3iATI == (GdkGLProc_glVertexStream3iATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3iATI =
      (GdkGLProc_glVertexStream3iATI) gdk_gl_get_proc_address ("glVertexStream3iATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3iATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3iATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3iATI);
}

/* glVertexStream3ivATI */
GdkGLProc
gdk_gl_get_glVertexStream3ivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3ivATI == (GdkGLProc_glVertexStream3ivATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3ivATI =
      (GdkGLProc_glVertexStream3ivATI) gdk_gl_get_proc_address ("glVertexStream3ivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3ivATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3ivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3ivATI);
}

/* glVertexStream3fATI */
GdkGLProc
gdk_gl_get_glVertexStream3fATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3fATI == (GdkGLProc_glVertexStream3fATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3fATI =
      (GdkGLProc_glVertexStream3fATI) gdk_gl_get_proc_address ("glVertexStream3fATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3fATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3fATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3fATI);
}

/* glVertexStream3fvATI */
GdkGLProc
gdk_gl_get_glVertexStream3fvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3fvATI == (GdkGLProc_glVertexStream3fvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3fvATI =
      (GdkGLProc_glVertexStream3fvATI) gdk_gl_get_proc_address ("glVertexStream3fvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3fvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3fvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3fvATI);
}

/* glVertexStream3dATI */
GdkGLProc
gdk_gl_get_glVertexStream3dATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3dATI == (GdkGLProc_glVertexStream3dATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3dATI =
      (GdkGLProc_glVertexStream3dATI) gdk_gl_get_proc_address ("glVertexStream3dATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3dATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3dATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3dATI);
}

/* glVertexStream3dvATI */
GdkGLProc
gdk_gl_get_glVertexStream3dvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream3dvATI == (GdkGLProc_glVertexStream3dvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream3dvATI =
      (GdkGLProc_glVertexStream3dvATI) gdk_gl_get_proc_address ("glVertexStream3dvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream3dvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream3dvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream3dvATI);
}

/* glVertexStream4sATI */
GdkGLProc
gdk_gl_get_glVertexStream4sATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4sATI == (GdkGLProc_glVertexStream4sATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4sATI =
      (GdkGLProc_glVertexStream4sATI) gdk_gl_get_proc_address ("glVertexStream4sATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4sATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4sATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4sATI);
}

/* glVertexStream4svATI */
GdkGLProc
gdk_gl_get_glVertexStream4svATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4svATI == (GdkGLProc_glVertexStream4svATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4svATI =
      (GdkGLProc_glVertexStream4svATI) gdk_gl_get_proc_address ("glVertexStream4svATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4svATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4svATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4svATI);
}

/* glVertexStream4iATI */
GdkGLProc
gdk_gl_get_glVertexStream4iATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4iATI == (GdkGLProc_glVertexStream4iATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4iATI =
      (GdkGLProc_glVertexStream4iATI) gdk_gl_get_proc_address ("glVertexStream4iATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4iATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4iATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4iATI);
}

/* glVertexStream4ivATI */
GdkGLProc
gdk_gl_get_glVertexStream4ivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4ivATI == (GdkGLProc_glVertexStream4ivATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4ivATI =
      (GdkGLProc_glVertexStream4ivATI) gdk_gl_get_proc_address ("glVertexStream4ivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4ivATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4ivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4ivATI);
}

/* glVertexStream4fATI */
GdkGLProc
gdk_gl_get_glVertexStream4fATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4fATI == (GdkGLProc_glVertexStream4fATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4fATI =
      (GdkGLProc_glVertexStream4fATI) gdk_gl_get_proc_address ("glVertexStream4fATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4fATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4fATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4fATI);
}

/* glVertexStream4fvATI */
GdkGLProc
gdk_gl_get_glVertexStream4fvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4fvATI == (GdkGLProc_glVertexStream4fvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4fvATI =
      (GdkGLProc_glVertexStream4fvATI) gdk_gl_get_proc_address ("glVertexStream4fvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4fvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4fvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4fvATI);
}

/* glVertexStream4dATI */
GdkGLProc
gdk_gl_get_glVertexStream4dATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4dATI == (GdkGLProc_glVertexStream4dATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4dATI =
      (GdkGLProc_glVertexStream4dATI) gdk_gl_get_proc_address ("glVertexStream4dATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4dATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4dATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4dATI);
}

/* glVertexStream4dvATI */
GdkGLProc
gdk_gl_get_glVertexStream4dvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexStream4dvATI == (GdkGLProc_glVertexStream4dvATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexStream4dvATI =
      (GdkGLProc_glVertexStream4dvATI) gdk_gl_get_proc_address ("glVertexStream4dvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexStream4dvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexStream4dvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexStream4dvATI);
}

/* glNormalStream3bATI */
GdkGLProc
gdk_gl_get_glNormalStream3bATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3bATI == (GdkGLProc_glNormalStream3bATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3bATI =
      (GdkGLProc_glNormalStream3bATI) gdk_gl_get_proc_address ("glNormalStream3bATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3bATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3bATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3bATI);
}

/* glNormalStream3bvATI */
GdkGLProc
gdk_gl_get_glNormalStream3bvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3bvATI == (GdkGLProc_glNormalStream3bvATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3bvATI =
      (GdkGLProc_glNormalStream3bvATI) gdk_gl_get_proc_address ("glNormalStream3bvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3bvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3bvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3bvATI);
}

/* glNormalStream3sATI */
GdkGLProc
gdk_gl_get_glNormalStream3sATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3sATI == (GdkGLProc_glNormalStream3sATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3sATI =
      (GdkGLProc_glNormalStream3sATI) gdk_gl_get_proc_address ("glNormalStream3sATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3sATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3sATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3sATI);
}

/* glNormalStream3svATI */
GdkGLProc
gdk_gl_get_glNormalStream3svATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3svATI == (GdkGLProc_glNormalStream3svATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3svATI =
      (GdkGLProc_glNormalStream3svATI) gdk_gl_get_proc_address ("glNormalStream3svATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3svATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3svATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3svATI);
}

/* glNormalStream3iATI */
GdkGLProc
gdk_gl_get_glNormalStream3iATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3iATI == (GdkGLProc_glNormalStream3iATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3iATI =
      (GdkGLProc_glNormalStream3iATI) gdk_gl_get_proc_address ("glNormalStream3iATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3iATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3iATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3iATI);
}

/* glNormalStream3ivATI */
GdkGLProc
gdk_gl_get_glNormalStream3ivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3ivATI == (GdkGLProc_glNormalStream3ivATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3ivATI =
      (GdkGLProc_glNormalStream3ivATI) gdk_gl_get_proc_address ("glNormalStream3ivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3ivATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3ivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3ivATI);
}

/* glNormalStream3fATI */
GdkGLProc
gdk_gl_get_glNormalStream3fATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3fATI == (GdkGLProc_glNormalStream3fATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3fATI =
      (GdkGLProc_glNormalStream3fATI) gdk_gl_get_proc_address ("glNormalStream3fATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3fATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3fATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3fATI);
}

/* glNormalStream3fvATI */
GdkGLProc
gdk_gl_get_glNormalStream3fvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3fvATI == (GdkGLProc_glNormalStream3fvATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3fvATI =
      (GdkGLProc_glNormalStream3fvATI) gdk_gl_get_proc_address ("glNormalStream3fvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3fvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3fvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3fvATI);
}

/* glNormalStream3dATI */
GdkGLProc
gdk_gl_get_glNormalStream3dATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3dATI == (GdkGLProc_glNormalStream3dATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3dATI =
      (GdkGLProc_glNormalStream3dATI) gdk_gl_get_proc_address ("glNormalStream3dATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3dATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3dATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3dATI);
}

/* glNormalStream3dvATI */
GdkGLProc
gdk_gl_get_glNormalStream3dvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glNormalStream3dvATI == (GdkGLProc_glNormalStream3dvATI) -1)
    _procs_GL_ATI_vertex_streams.glNormalStream3dvATI =
      (GdkGLProc_glNormalStream3dvATI) gdk_gl_get_proc_address ("glNormalStream3dvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormalStream3dvATI () - %s",
               (_procs_GL_ATI_vertex_streams.glNormalStream3dvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glNormalStream3dvATI);
}

/* glClientActiveVertexStreamATI */
GdkGLProc
gdk_gl_get_glClientActiveVertexStreamATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glClientActiveVertexStreamATI == (GdkGLProc_glClientActiveVertexStreamATI) -1)
    _procs_GL_ATI_vertex_streams.glClientActiveVertexStreamATI =
      (GdkGLProc_glClientActiveVertexStreamATI) gdk_gl_get_proc_address ("glClientActiveVertexStreamATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glClientActiveVertexStreamATI () - %s",
               (_procs_GL_ATI_vertex_streams.glClientActiveVertexStreamATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glClientActiveVertexStreamATI);
}

/* glVertexBlendEnviATI */
GdkGLProc
gdk_gl_get_glVertexBlendEnviATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexBlendEnviATI == (GdkGLProc_glVertexBlendEnviATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexBlendEnviATI =
      (GdkGLProc_glVertexBlendEnviATI) gdk_gl_get_proc_address ("glVertexBlendEnviATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexBlendEnviATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexBlendEnviATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexBlendEnviATI);
}

/* glVertexBlendEnvfATI */
GdkGLProc
gdk_gl_get_glVertexBlendEnvfATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_streams.glVertexBlendEnvfATI == (GdkGLProc_glVertexBlendEnvfATI) -1)
    _procs_GL_ATI_vertex_streams.glVertexBlendEnvfATI =
      (GdkGLProc_glVertexBlendEnvfATI) gdk_gl_get_proc_address ("glVertexBlendEnvfATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexBlendEnvfATI () - %s",
               (_procs_GL_ATI_vertex_streams.glVertexBlendEnvfATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_streams.glVertexBlendEnvfATI);
}

/* Get GL_ATI_vertex_streams functions */
GdkGL_GL_ATI_vertex_streams *
gdk_gl_get_GL_ATI_vertex_streams (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_vertex_streams");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertexStream1sATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1svATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1iATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1ivATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1fATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1fvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1dATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream1dvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2sATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2svATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2iATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2ivATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2fATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2fvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2dATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream2dvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3sATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3svATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3iATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3ivATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3fATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3fvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3dATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream3dvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4sATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4svATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4iATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4ivATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4fATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4fvATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4dATI () != NULL);
          supported &= (gdk_gl_get_glVertexStream4dvATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3bATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3bvATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3sATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3svATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3iATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3ivATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3fATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3fvATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3dATI () != NULL);
          supported &= (gdk_gl_get_glNormalStream3dvATI () != NULL);
          supported &= (gdk_gl_get_glClientActiveVertexStreamATI () != NULL);
          supported &= (gdk_gl_get_glVertexBlendEnviATI () != NULL);
          supported &= (gdk_gl_get_glVertexBlendEnvfATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_vertex_streams () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_vertex_streams;
}

/*
 * GL_ATI_element_array
 */

static GdkGL_GL_ATI_element_array _procs_GL_ATI_element_array = {
  (GdkGLProc_glElementPointerATI) -1,
  (GdkGLProc_glDrawElementArrayATI) -1,
  (GdkGLProc_glDrawRangeElementArrayATI) -1
};

/* glElementPointerATI */
GdkGLProc
gdk_gl_get_glElementPointerATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_element_array.glElementPointerATI == (GdkGLProc_glElementPointerATI) -1)
    _procs_GL_ATI_element_array.glElementPointerATI =
      (GdkGLProc_glElementPointerATI) gdk_gl_get_proc_address ("glElementPointerATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glElementPointerATI () - %s",
               (_procs_GL_ATI_element_array.glElementPointerATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_element_array.glElementPointerATI);
}

/* glDrawElementArrayATI */
GdkGLProc
gdk_gl_get_glDrawElementArrayATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_element_array.glDrawElementArrayATI == (GdkGLProc_glDrawElementArrayATI) -1)
    _procs_GL_ATI_element_array.glDrawElementArrayATI =
      (GdkGLProc_glDrawElementArrayATI) gdk_gl_get_proc_address ("glDrawElementArrayATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawElementArrayATI () - %s",
               (_procs_GL_ATI_element_array.glDrawElementArrayATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_element_array.glDrawElementArrayATI);
}

/* glDrawRangeElementArrayATI */
GdkGLProc
gdk_gl_get_glDrawRangeElementArrayATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_element_array.glDrawRangeElementArrayATI == (GdkGLProc_glDrawRangeElementArrayATI) -1)
    _procs_GL_ATI_element_array.glDrawRangeElementArrayATI =
      (GdkGLProc_glDrawRangeElementArrayATI) gdk_gl_get_proc_address ("glDrawRangeElementArrayATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawRangeElementArrayATI () - %s",
               (_procs_GL_ATI_element_array.glDrawRangeElementArrayATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_element_array.glDrawRangeElementArrayATI);
}

/* Get GL_ATI_element_array functions */
GdkGL_GL_ATI_element_array *
gdk_gl_get_GL_ATI_element_array (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_element_array");

      if (supported)
        {
          supported &= (gdk_gl_get_glElementPointerATI () != NULL);
          supported &= (gdk_gl_get_glDrawElementArrayATI () != NULL);
          supported &= (gdk_gl_get_glDrawRangeElementArrayATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_element_array () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_element_array;
}

/*
 * GL_SUN_mesh_array
 */

static GdkGL_GL_SUN_mesh_array _procs_GL_SUN_mesh_array = {
  (GdkGLProc_glDrawMeshArraysSUN) -1
};

/* glDrawMeshArraysSUN */
GdkGLProc
gdk_gl_get_glDrawMeshArraysSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_mesh_array.glDrawMeshArraysSUN == (GdkGLProc_glDrawMeshArraysSUN) -1)
    _procs_GL_SUN_mesh_array.glDrawMeshArraysSUN =
      (GdkGLProc_glDrawMeshArraysSUN) gdk_gl_get_proc_address ("glDrawMeshArraysSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawMeshArraysSUN () - %s",
               (_procs_GL_SUN_mesh_array.glDrawMeshArraysSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_mesh_array.glDrawMeshArraysSUN);
}

/* Get GL_SUN_mesh_array functions */
GdkGL_GL_SUN_mesh_array *
gdk_gl_get_GL_SUN_mesh_array (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SUN_mesh_array");

      if (supported)
        {
          supported &= (gdk_gl_get_glDrawMeshArraysSUN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SUN_mesh_array () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SUN_mesh_array;
}

/*
 * GL_NV_occlusion_query
 */

static GdkGL_GL_NV_occlusion_query _procs_GL_NV_occlusion_query = {
  (GdkGLProc_glGenOcclusionQueriesNV) -1,
  (GdkGLProc_glDeleteOcclusionQueriesNV) -1,
  (GdkGLProc_glIsOcclusionQueryNV) -1,
  (GdkGLProc_glBeginOcclusionQueryNV) -1,
  (GdkGLProc_glEndOcclusionQueryNV) -1,
  (GdkGLProc_glGetOcclusionQueryivNV) -1,
  (GdkGLProc_glGetOcclusionQueryuivNV) -1
};

/* glGenOcclusionQueriesNV */
GdkGLProc
gdk_gl_get_glGenOcclusionQueriesNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glGenOcclusionQueriesNV == (GdkGLProc_glGenOcclusionQueriesNV) -1)
    _procs_GL_NV_occlusion_query.glGenOcclusionQueriesNV =
      (GdkGLProc_glGenOcclusionQueriesNV) gdk_gl_get_proc_address ("glGenOcclusionQueriesNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenOcclusionQueriesNV () - %s",
               (_procs_GL_NV_occlusion_query.glGenOcclusionQueriesNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glGenOcclusionQueriesNV);
}

/* glDeleteOcclusionQueriesNV */
GdkGLProc
gdk_gl_get_glDeleteOcclusionQueriesNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glDeleteOcclusionQueriesNV == (GdkGLProc_glDeleteOcclusionQueriesNV) -1)
    _procs_GL_NV_occlusion_query.glDeleteOcclusionQueriesNV =
      (GdkGLProc_glDeleteOcclusionQueriesNV) gdk_gl_get_proc_address ("glDeleteOcclusionQueriesNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteOcclusionQueriesNV () - %s",
               (_procs_GL_NV_occlusion_query.glDeleteOcclusionQueriesNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glDeleteOcclusionQueriesNV);
}

/* glIsOcclusionQueryNV */
GdkGLProc
gdk_gl_get_glIsOcclusionQueryNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glIsOcclusionQueryNV == (GdkGLProc_glIsOcclusionQueryNV) -1)
    _procs_GL_NV_occlusion_query.glIsOcclusionQueryNV =
      (GdkGLProc_glIsOcclusionQueryNV) gdk_gl_get_proc_address ("glIsOcclusionQueryNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsOcclusionQueryNV () - %s",
               (_procs_GL_NV_occlusion_query.glIsOcclusionQueryNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glIsOcclusionQueryNV);
}

/* glBeginOcclusionQueryNV */
GdkGLProc
gdk_gl_get_glBeginOcclusionQueryNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glBeginOcclusionQueryNV == (GdkGLProc_glBeginOcclusionQueryNV) -1)
    _procs_GL_NV_occlusion_query.glBeginOcclusionQueryNV =
      (GdkGLProc_glBeginOcclusionQueryNV) gdk_gl_get_proc_address ("glBeginOcclusionQueryNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBeginOcclusionQueryNV () - %s",
               (_procs_GL_NV_occlusion_query.glBeginOcclusionQueryNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glBeginOcclusionQueryNV);
}

/* glEndOcclusionQueryNV */
GdkGLProc
gdk_gl_get_glEndOcclusionQueryNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glEndOcclusionQueryNV == (GdkGLProc_glEndOcclusionQueryNV) -1)
    _procs_GL_NV_occlusion_query.glEndOcclusionQueryNV =
      (GdkGLProc_glEndOcclusionQueryNV) gdk_gl_get_proc_address ("glEndOcclusionQueryNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEndOcclusionQueryNV () - %s",
               (_procs_GL_NV_occlusion_query.glEndOcclusionQueryNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glEndOcclusionQueryNV);
}

/* glGetOcclusionQueryivNV */
GdkGLProc
gdk_gl_get_glGetOcclusionQueryivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glGetOcclusionQueryivNV == (GdkGLProc_glGetOcclusionQueryivNV) -1)
    _procs_GL_NV_occlusion_query.glGetOcclusionQueryivNV =
      (GdkGLProc_glGetOcclusionQueryivNV) gdk_gl_get_proc_address ("glGetOcclusionQueryivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetOcclusionQueryivNV () - %s",
               (_procs_GL_NV_occlusion_query.glGetOcclusionQueryivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glGetOcclusionQueryivNV);
}

/* glGetOcclusionQueryuivNV */
GdkGLProc
gdk_gl_get_glGetOcclusionQueryuivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_occlusion_query.glGetOcclusionQueryuivNV == (GdkGLProc_glGetOcclusionQueryuivNV) -1)
    _procs_GL_NV_occlusion_query.glGetOcclusionQueryuivNV =
      (GdkGLProc_glGetOcclusionQueryuivNV) gdk_gl_get_proc_address ("glGetOcclusionQueryuivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetOcclusionQueryuivNV () - %s",
               (_procs_GL_NV_occlusion_query.glGetOcclusionQueryuivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_occlusion_query.glGetOcclusionQueryuivNV);
}

/* Get GL_NV_occlusion_query functions */
GdkGL_GL_NV_occlusion_query *
gdk_gl_get_GL_NV_occlusion_query (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_occlusion_query");

      if (supported)
        {
          supported &= (gdk_gl_get_glGenOcclusionQueriesNV () != NULL);
          supported &= (gdk_gl_get_glDeleteOcclusionQueriesNV () != NULL);
          supported &= (gdk_gl_get_glIsOcclusionQueryNV () != NULL);
          supported &= (gdk_gl_get_glBeginOcclusionQueryNV () != NULL);
          supported &= (gdk_gl_get_glEndOcclusionQueryNV () != NULL);
          supported &= (gdk_gl_get_glGetOcclusionQueryivNV () != NULL);
          supported &= (gdk_gl_get_glGetOcclusionQueryuivNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_occlusion_query () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_occlusion_query;
}

/*
 * GL_NV_point_sprite
 */

static GdkGL_GL_NV_point_sprite _procs_GL_NV_point_sprite = {
  (GdkGLProc_glPointParameteriNV) -1,
  (GdkGLProc_glPointParameterivNV) -1
};

/* glPointParameteriNV */
GdkGLProc
gdk_gl_get_glPointParameteriNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_point_sprite.glPointParameteriNV == (GdkGLProc_glPointParameteriNV) -1)
    _procs_GL_NV_point_sprite.glPointParameteriNV =
      (GdkGLProc_glPointParameteriNV) gdk_gl_get_proc_address ("glPointParameteriNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameteriNV () - %s",
               (_procs_GL_NV_point_sprite.glPointParameteriNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_point_sprite.glPointParameteriNV);
}

/* glPointParameterivNV */
GdkGLProc
gdk_gl_get_glPointParameterivNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_point_sprite.glPointParameterivNV == (GdkGLProc_glPointParameterivNV) -1)
    _procs_GL_NV_point_sprite.glPointParameterivNV =
      (GdkGLProc_glPointParameterivNV) gdk_gl_get_proc_address ("glPointParameterivNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPointParameterivNV () - %s",
               (_procs_GL_NV_point_sprite.glPointParameterivNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_point_sprite.glPointParameterivNV);
}

/* Get GL_NV_point_sprite functions */
GdkGL_GL_NV_point_sprite *
gdk_gl_get_GL_NV_point_sprite (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_point_sprite");

      if (supported)
        {
          supported &= (gdk_gl_get_glPointParameteriNV () != NULL);
          supported &= (gdk_gl_get_glPointParameterivNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_point_sprite () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_point_sprite;
}

/*
 * GL_EXT_stencil_two_side
 */

static GdkGL_GL_EXT_stencil_two_side _procs_GL_EXT_stencil_two_side = {
  (GdkGLProc_glActiveStencilFaceEXT) -1
};

/* glActiveStencilFaceEXT */
GdkGLProc
gdk_gl_get_glActiveStencilFaceEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_stencil_two_side.glActiveStencilFaceEXT == (GdkGLProc_glActiveStencilFaceEXT) -1)
    _procs_GL_EXT_stencil_two_side.glActiveStencilFaceEXT =
      (GdkGLProc_glActiveStencilFaceEXT) gdk_gl_get_proc_address ("glActiveStencilFaceEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glActiveStencilFaceEXT () - %s",
               (_procs_GL_EXT_stencil_two_side.glActiveStencilFaceEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_stencil_two_side.glActiveStencilFaceEXT);
}

/* Get GL_EXT_stencil_two_side functions */
GdkGL_GL_EXT_stencil_two_side *
gdk_gl_get_GL_EXT_stencil_two_side (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_stencil_two_side");

      if (supported)
        {
          supported &= (gdk_gl_get_glActiveStencilFaceEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_stencil_two_side () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_stencil_two_side;
}

/*
 * GL_APPLE_element_array
 */

static GdkGL_GL_APPLE_element_array _procs_GL_APPLE_element_array = {
  (GdkGLProc_glElementPointerAPPLE) -1,
  (GdkGLProc_glDrawElementArrayAPPLE) -1,
  (GdkGLProc_glDrawRangeElementArrayAPPLE) -1,
  (GdkGLProc_glMultiDrawElementArrayAPPLE) -1,
  (GdkGLProc_glMultiDrawRangeElementArrayAPPLE) -1
};

/* glElementPointerAPPLE */
GdkGLProc
gdk_gl_get_glElementPointerAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_element_array.glElementPointerAPPLE == (GdkGLProc_glElementPointerAPPLE) -1)
    _procs_GL_APPLE_element_array.glElementPointerAPPLE =
      (GdkGLProc_glElementPointerAPPLE) gdk_gl_get_proc_address ("glElementPointerAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glElementPointerAPPLE () - %s",
               (_procs_GL_APPLE_element_array.glElementPointerAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_element_array.glElementPointerAPPLE);
}

/* glDrawElementArrayAPPLE */
GdkGLProc
gdk_gl_get_glDrawElementArrayAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_element_array.glDrawElementArrayAPPLE == (GdkGLProc_glDrawElementArrayAPPLE) -1)
    _procs_GL_APPLE_element_array.glDrawElementArrayAPPLE =
      (GdkGLProc_glDrawElementArrayAPPLE) gdk_gl_get_proc_address ("glDrawElementArrayAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawElementArrayAPPLE () - %s",
               (_procs_GL_APPLE_element_array.glDrawElementArrayAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_element_array.glDrawElementArrayAPPLE);
}

/* glDrawRangeElementArrayAPPLE */
GdkGLProc
gdk_gl_get_glDrawRangeElementArrayAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_element_array.glDrawRangeElementArrayAPPLE == (GdkGLProc_glDrawRangeElementArrayAPPLE) -1)
    _procs_GL_APPLE_element_array.glDrawRangeElementArrayAPPLE =
      (GdkGLProc_glDrawRangeElementArrayAPPLE) gdk_gl_get_proc_address ("glDrawRangeElementArrayAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawRangeElementArrayAPPLE () - %s",
               (_procs_GL_APPLE_element_array.glDrawRangeElementArrayAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_element_array.glDrawRangeElementArrayAPPLE);
}

/* glMultiDrawElementArrayAPPLE */
GdkGLProc
gdk_gl_get_glMultiDrawElementArrayAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_element_array.glMultiDrawElementArrayAPPLE == (GdkGLProc_glMultiDrawElementArrayAPPLE) -1)
    _procs_GL_APPLE_element_array.glMultiDrawElementArrayAPPLE =
      (GdkGLProc_glMultiDrawElementArrayAPPLE) gdk_gl_get_proc_address ("glMultiDrawElementArrayAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawElementArrayAPPLE () - %s",
               (_procs_GL_APPLE_element_array.glMultiDrawElementArrayAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_element_array.glMultiDrawElementArrayAPPLE);
}

/* glMultiDrawRangeElementArrayAPPLE */
GdkGLProc
gdk_gl_get_glMultiDrawRangeElementArrayAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_element_array.glMultiDrawRangeElementArrayAPPLE == (GdkGLProc_glMultiDrawRangeElementArrayAPPLE) -1)
    _procs_GL_APPLE_element_array.glMultiDrawRangeElementArrayAPPLE =
      (GdkGLProc_glMultiDrawRangeElementArrayAPPLE) gdk_gl_get_proc_address ("glMultiDrawRangeElementArrayAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawRangeElementArrayAPPLE () - %s",
               (_procs_GL_APPLE_element_array.glMultiDrawRangeElementArrayAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_element_array.glMultiDrawRangeElementArrayAPPLE);
}

/* Get GL_APPLE_element_array functions */
GdkGL_GL_APPLE_element_array *
gdk_gl_get_GL_APPLE_element_array (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_APPLE_element_array");

      if (supported)
        {
          supported &= (gdk_gl_get_glElementPointerAPPLE () != NULL);
          supported &= (gdk_gl_get_glDrawElementArrayAPPLE () != NULL);
          supported &= (gdk_gl_get_glDrawRangeElementArrayAPPLE () != NULL);
          supported &= (gdk_gl_get_glMultiDrawElementArrayAPPLE () != NULL);
          supported &= (gdk_gl_get_glMultiDrawRangeElementArrayAPPLE () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_APPLE_element_array () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_APPLE_element_array;
}

/*
 * GL_APPLE_fence
 */

static GdkGL_GL_APPLE_fence _procs_GL_APPLE_fence = {
  (GdkGLProc_glGenFencesAPPLE) -1,
  (GdkGLProc_glDeleteFencesAPPLE) -1,
  (GdkGLProc_glSetFenceAPPLE) -1,
  (GdkGLProc_glIsFenceAPPLE) -1,
  (GdkGLProc_glTestFenceAPPLE) -1,
  (GdkGLProc_glFinishFenceAPPLE) -1,
  (GdkGLProc_glTestObjectAPPLE) -1,
  (GdkGLProc_glFinishObjectAPPLE) -1
};

/* glGenFencesAPPLE */
GdkGLProc
gdk_gl_get_glGenFencesAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glGenFencesAPPLE == (GdkGLProc_glGenFencesAPPLE) -1)
    _procs_GL_APPLE_fence.glGenFencesAPPLE =
      (GdkGLProc_glGenFencesAPPLE) gdk_gl_get_proc_address ("glGenFencesAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenFencesAPPLE () - %s",
               (_procs_GL_APPLE_fence.glGenFencesAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glGenFencesAPPLE);
}

/* glDeleteFencesAPPLE */
GdkGLProc
gdk_gl_get_glDeleteFencesAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glDeleteFencesAPPLE == (GdkGLProc_glDeleteFencesAPPLE) -1)
    _procs_GL_APPLE_fence.glDeleteFencesAPPLE =
      (GdkGLProc_glDeleteFencesAPPLE) gdk_gl_get_proc_address ("glDeleteFencesAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteFencesAPPLE () - %s",
               (_procs_GL_APPLE_fence.glDeleteFencesAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glDeleteFencesAPPLE);
}

/* glSetFenceAPPLE */
GdkGLProc
gdk_gl_get_glSetFenceAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glSetFenceAPPLE == (GdkGLProc_glSetFenceAPPLE) -1)
    _procs_GL_APPLE_fence.glSetFenceAPPLE =
      (GdkGLProc_glSetFenceAPPLE) gdk_gl_get_proc_address ("glSetFenceAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSetFenceAPPLE () - %s",
               (_procs_GL_APPLE_fence.glSetFenceAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glSetFenceAPPLE);
}

/* glIsFenceAPPLE */
GdkGLProc
gdk_gl_get_glIsFenceAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glIsFenceAPPLE == (GdkGLProc_glIsFenceAPPLE) -1)
    _procs_GL_APPLE_fence.glIsFenceAPPLE =
      (GdkGLProc_glIsFenceAPPLE) gdk_gl_get_proc_address ("glIsFenceAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsFenceAPPLE () - %s",
               (_procs_GL_APPLE_fence.glIsFenceAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glIsFenceAPPLE);
}

/* glTestFenceAPPLE */
GdkGLProc
gdk_gl_get_glTestFenceAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glTestFenceAPPLE == (GdkGLProc_glTestFenceAPPLE) -1)
    _procs_GL_APPLE_fence.glTestFenceAPPLE =
      (GdkGLProc_glTestFenceAPPLE) gdk_gl_get_proc_address ("glTestFenceAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTestFenceAPPLE () - %s",
               (_procs_GL_APPLE_fence.glTestFenceAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glTestFenceAPPLE);
}

/* glFinishFenceAPPLE */
GdkGLProc
gdk_gl_get_glFinishFenceAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glFinishFenceAPPLE == (GdkGLProc_glFinishFenceAPPLE) -1)
    _procs_GL_APPLE_fence.glFinishFenceAPPLE =
      (GdkGLProc_glFinishFenceAPPLE) gdk_gl_get_proc_address ("glFinishFenceAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFinishFenceAPPLE () - %s",
               (_procs_GL_APPLE_fence.glFinishFenceAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glFinishFenceAPPLE);
}

/* glTestObjectAPPLE */
GdkGLProc
gdk_gl_get_glTestObjectAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glTestObjectAPPLE == (GdkGLProc_glTestObjectAPPLE) -1)
    _procs_GL_APPLE_fence.glTestObjectAPPLE =
      (GdkGLProc_glTestObjectAPPLE) gdk_gl_get_proc_address ("glTestObjectAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTestObjectAPPLE () - %s",
               (_procs_GL_APPLE_fence.glTestObjectAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glTestObjectAPPLE);
}

/* glFinishObjectAPPLE */
GdkGLProc
gdk_gl_get_glFinishObjectAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_fence.glFinishObjectAPPLE == (GdkGLProc_glFinishObjectAPPLE) -1)
    _procs_GL_APPLE_fence.glFinishObjectAPPLE =
      (GdkGLProc_glFinishObjectAPPLE) gdk_gl_get_proc_address ("glFinishObjectAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFinishObjectAPPLE () - %s",
               (_procs_GL_APPLE_fence.glFinishObjectAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_fence.glFinishObjectAPPLE);
}

/* Get GL_APPLE_fence functions */
GdkGL_GL_APPLE_fence *
gdk_gl_get_GL_APPLE_fence (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_APPLE_fence");

      if (supported)
        {
          supported &= (gdk_gl_get_glGenFencesAPPLE () != NULL);
          supported &= (gdk_gl_get_glDeleteFencesAPPLE () != NULL);
          supported &= (gdk_gl_get_glSetFenceAPPLE () != NULL);
          supported &= (gdk_gl_get_glIsFenceAPPLE () != NULL);
          supported &= (gdk_gl_get_glTestFenceAPPLE () != NULL);
          supported &= (gdk_gl_get_glFinishFenceAPPLE () != NULL);
          supported &= (gdk_gl_get_glTestObjectAPPLE () != NULL);
          supported &= (gdk_gl_get_glFinishObjectAPPLE () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_APPLE_fence () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_APPLE_fence;
}

/*
 * GL_APPLE_vertex_array_object
 */

static GdkGL_GL_APPLE_vertex_array_object _procs_GL_APPLE_vertex_array_object = {
  (GdkGLProc_glBindVertexArrayAPPLE) -1,
  (GdkGLProc_glDeleteVertexArraysAPPLE) -1,
  (GdkGLProc_glGenVertexArraysAPPLE) -1,
  (GdkGLProc_glIsVertexArrayAPPLE) -1
};

/* glBindVertexArrayAPPLE */
GdkGLProc
gdk_gl_get_glBindVertexArrayAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_object.glBindVertexArrayAPPLE == (GdkGLProc_glBindVertexArrayAPPLE) -1)
    _procs_GL_APPLE_vertex_array_object.glBindVertexArrayAPPLE =
      (GdkGLProc_glBindVertexArrayAPPLE) gdk_gl_get_proc_address ("glBindVertexArrayAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBindVertexArrayAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_object.glBindVertexArrayAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_object.glBindVertexArrayAPPLE);
}

/* glDeleteVertexArraysAPPLE */
GdkGLProc
gdk_gl_get_glDeleteVertexArraysAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_object.glDeleteVertexArraysAPPLE == (GdkGLProc_glDeleteVertexArraysAPPLE) -1)
    _procs_GL_APPLE_vertex_array_object.glDeleteVertexArraysAPPLE =
      (GdkGLProc_glDeleteVertexArraysAPPLE) gdk_gl_get_proc_address ("glDeleteVertexArraysAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteVertexArraysAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_object.glDeleteVertexArraysAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_object.glDeleteVertexArraysAPPLE);
}

/* glGenVertexArraysAPPLE */
GdkGLProc
gdk_gl_get_glGenVertexArraysAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_object.glGenVertexArraysAPPLE == (GdkGLProc_glGenVertexArraysAPPLE) -1)
    _procs_GL_APPLE_vertex_array_object.glGenVertexArraysAPPLE =
      (GdkGLProc_glGenVertexArraysAPPLE) gdk_gl_get_proc_address ("glGenVertexArraysAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGenVertexArraysAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_object.glGenVertexArraysAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_object.glGenVertexArraysAPPLE);
}

/* glIsVertexArrayAPPLE */
GdkGLProc
gdk_gl_get_glIsVertexArrayAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_object.glIsVertexArrayAPPLE == (GdkGLProc_glIsVertexArrayAPPLE) -1)
    _procs_GL_APPLE_vertex_array_object.glIsVertexArrayAPPLE =
      (GdkGLProc_glIsVertexArrayAPPLE) gdk_gl_get_proc_address ("glIsVertexArrayAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsVertexArrayAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_object.glIsVertexArrayAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_object.glIsVertexArrayAPPLE);
}

/* Get GL_APPLE_vertex_array_object functions */
GdkGL_GL_APPLE_vertex_array_object *
gdk_gl_get_GL_APPLE_vertex_array_object (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_APPLE_vertex_array_object");

      if (supported)
        {
          supported &= (gdk_gl_get_glBindVertexArrayAPPLE () != NULL);
          supported &= (gdk_gl_get_glDeleteVertexArraysAPPLE () != NULL);
          supported &= (gdk_gl_get_glGenVertexArraysAPPLE () != NULL);
          supported &= (gdk_gl_get_glIsVertexArrayAPPLE () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_APPLE_vertex_array_object () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_APPLE_vertex_array_object;
}

/*
 * GL_APPLE_vertex_array_range
 */

static GdkGL_GL_APPLE_vertex_array_range _procs_GL_APPLE_vertex_array_range = {
  (GdkGLProc_glVertexArrayRangeAPPLE) -1,
  (GdkGLProc_glFlushVertexArrayRangeAPPLE) -1,
  (GdkGLProc_glVertexArrayParameteriAPPLE) -1
};

/* glVertexArrayRangeAPPLE */
GdkGLProc
gdk_gl_get_glVertexArrayRangeAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_range.glVertexArrayRangeAPPLE == (GdkGLProc_glVertexArrayRangeAPPLE) -1)
    _procs_GL_APPLE_vertex_array_range.glVertexArrayRangeAPPLE =
      (GdkGLProc_glVertexArrayRangeAPPLE) gdk_gl_get_proc_address ("glVertexArrayRangeAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexArrayRangeAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_range.glVertexArrayRangeAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_range.glVertexArrayRangeAPPLE);
}

/* glFlushVertexArrayRangeAPPLE */
GdkGLProc
gdk_gl_get_glFlushVertexArrayRangeAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_range.glFlushVertexArrayRangeAPPLE == (GdkGLProc_glFlushVertexArrayRangeAPPLE) -1)
    _procs_GL_APPLE_vertex_array_range.glFlushVertexArrayRangeAPPLE =
      (GdkGLProc_glFlushVertexArrayRangeAPPLE) gdk_gl_get_proc_address ("glFlushVertexArrayRangeAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFlushVertexArrayRangeAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_range.glFlushVertexArrayRangeAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_range.glFlushVertexArrayRangeAPPLE);
}

/* glVertexArrayParameteriAPPLE */
GdkGLProc
gdk_gl_get_glVertexArrayParameteriAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_array_range.glVertexArrayParameteriAPPLE == (GdkGLProc_glVertexArrayParameteriAPPLE) -1)
    _procs_GL_APPLE_vertex_array_range.glVertexArrayParameteriAPPLE =
      (GdkGLProc_glVertexArrayParameteriAPPLE) gdk_gl_get_proc_address ("glVertexArrayParameteriAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexArrayParameteriAPPLE () - %s",
               (_procs_GL_APPLE_vertex_array_range.glVertexArrayParameteriAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_array_range.glVertexArrayParameteriAPPLE);
}

/* Get GL_APPLE_vertex_array_range functions */
GdkGL_GL_APPLE_vertex_array_range *
gdk_gl_get_GL_APPLE_vertex_array_range (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_APPLE_vertex_array_range");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertexArrayRangeAPPLE () != NULL);
          supported &= (gdk_gl_get_glFlushVertexArrayRangeAPPLE () != NULL);
          supported &= (gdk_gl_get_glVertexArrayParameteriAPPLE () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_APPLE_vertex_array_range () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_APPLE_vertex_array_range;
}

/*
 * GL_ATI_draw_buffers
 */

static GdkGL_GL_ATI_draw_buffers _procs_GL_ATI_draw_buffers = {
  (GdkGLProc_glDrawBuffersATI) -1
};

/* glDrawBuffersATI */
GdkGLProc
gdk_gl_get_glDrawBuffersATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_draw_buffers.glDrawBuffersATI == (GdkGLProc_glDrawBuffersATI) -1)
    _procs_GL_ATI_draw_buffers.glDrawBuffersATI =
      (GdkGLProc_glDrawBuffersATI) gdk_gl_get_proc_address ("glDrawBuffersATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawBuffersATI () - %s",
               (_procs_GL_ATI_draw_buffers.glDrawBuffersATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_draw_buffers.glDrawBuffersATI);
}

/* Get GL_ATI_draw_buffers functions */
GdkGL_GL_ATI_draw_buffers *
gdk_gl_get_GL_ATI_draw_buffers (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_draw_buffers");

      if (supported)
        {
          supported &= (gdk_gl_get_glDrawBuffersATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_draw_buffers () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_draw_buffers;
}

/*
 * GL_NV_fragment_program
 */

static GdkGL_GL_NV_fragment_program _procs_GL_NV_fragment_program = {
  (GdkGLProc_glProgramNamedParameter4fNV) -1,
  (GdkGLProc_glProgramNamedParameter4dNV) -1,
  (GdkGLProc_glProgramNamedParameter4fvNV) -1,
  (GdkGLProc_glProgramNamedParameter4dvNV) -1,
  (GdkGLProc_glGetProgramNamedParameterfvNV) -1,
  (GdkGLProc_glGetProgramNamedParameterdvNV) -1
};

/* glProgramNamedParameter4fNV */
GdkGLProc
gdk_gl_get_glProgramNamedParameter4fNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fragment_program.glProgramNamedParameter4fNV == (GdkGLProc_glProgramNamedParameter4fNV) -1)
    _procs_GL_NV_fragment_program.glProgramNamedParameter4fNV =
      (GdkGLProc_glProgramNamedParameter4fNV) gdk_gl_get_proc_address ("glProgramNamedParameter4fNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramNamedParameter4fNV () - %s",
               (_procs_GL_NV_fragment_program.glProgramNamedParameter4fNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fragment_program.glProgramNamedParameter4fNV);
}

/* glProgramNamedParameter4dNV */
GdkGLProc
gdk_gl_get_glProgramNamedParameter4dNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fragment_program.glProgramNamedParameter4dNV == (GdkGLProc_glProgramNamedParameter4dNV) -1)
    _procs_GL_NV_fragment_program.glProgramNamedParameter4dNV =
      (GdkGLProc_glProgramNamedParameter4dNV) gdk_gl_get_proc_address ("glProgramNamedParameter4dNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramNamedParameter4dNV () - %s",
               (_procs_GL_NV_fragment_program.glProgramNamedParameter4dNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fragment_program.glProgramNamedParameter4dNV);
}

/* glProgramNamedParameter4fvNV */
GdkGLProc
gdk_gl_get_glProgramNamedParameter4fvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fragment_program.glProgramNamedParameter4fvNV == (GdkGLProc_glProgramNamedParameter4fvNV) -1)
    _procs_GL_NV_fragment_program.glProgramNamedParameter4fvNV =
      (GdkGLProc_glProgramNamedParameter4fvNV) gdk_gl_get_proc_address ("glProgramNamedParameter4fvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramNamedParameter4fvNV () - %s",
               (_procs_GL_NV_fragment_program.glProgramNamedParameter4fvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fragment_program.glProgramNamedParameter4fvNV);
}

/* glProgramNamedParameter4dvNV */
GdkGLProc
gdk_gl_get_glProgramNamedParameter4dvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fragment_program.glProgramNamedParameter4dvNV == (GdkGLProc_glProgramNamedParameter4dvNV) -1)
    _procs_GL_NV_fragment_program.glProgramNamedParameter4dvNV =
      (GdkGLProc_glProgramNamedParameter4dvNV) gdk_gl_get_proc_address ("glProgramNamedParameter4dvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glProgramNamedParameter4dvNV () - %s",
               (_procs_GL_NV_fragment_program.glProgramNamedParameter4dvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fragment_program.glProgramNamedParameter4dvNV);
}

/* glGetProgramNamedParameterfvNV */
GdkGLProc
gdk_gl_get_glGetProgramNamedParameterfvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fragment_program.glGetProgramNamedParameterfvNV == (GdkGLProc_glGetProgramNamedParameterfvNV) -1)
    _procs_GL_NV_fragment_program.glGetProgramNamedParameterfvNV =
      (GdkGLProc_glGetProgramNamedParameterfvNV) gdk_gl_get_proc_address ("glGetProgramNamedParameterfvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramNamedParameterfvNV () - %s",
               (_procs_GL_NV_fragment_program.glGetProgramNamedParameterfvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fragment_program.glGetProgramNamedParameterfvNV);
}

/* glGetProgramNamedParameterdvNV */
GdkGLProc
gdk_gl_get_glGetProgramNamedParameterdvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_fragment_program.glGetProgramNamedParameterdvNV == (GdkGLProc_glGetProgramNamedParameterdvNV) -1)
    _procs_GL_NV_fragment_program.glGetProgramNamedParameterdvNV =
      (GdkGLProc_glGetProgramNamedParameterdvNV) gdk_gl_get_proc_address ("glGetProgramNamedParameterdvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetProgramNamedParameterdvNV () - %s",
               (_procs_GL_NV_fragment_program.glGetProgramNamedParameterdvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_fragment_program.glGetProgramNamedParameterdvNV);
}

/* Get GL_NV_fragment_program functions */
GdkGL_GL_NV_fragment_program *
gdk_gl_get_GL_NV_fragment_program (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_fragment_program");

      if (supported)
        {
          supported &= (gdk_gl_get_glProgramNamedParameter4fNV () != NULL);
          supported &= (gdk_gl_get_glProgramNamedParameter4dNV () != NULL);
          supported &= (gdk_gl_get_glProgramNamedParameter4fvNV () != NULL);
          supported &= (gdk_gl_get_glProgramNamedParameter4dvNV () != NULL);
          supported &= (gdk_gl_get_glGetProgramNamedParameterfvNV () != NULL);
          supported &= (gdk_gl_get_glGetProgramNamedParameterdvNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_fragment_program () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_fragment_program;
}

/*
 * GL_NV_half_float
 */

static GdkGL_GL_NV_half_float _procs_GL_NV_half_float = {
  (GdkGLProc_glVertex2hNV) -1,
  (GdkGLProc_glVertex2hvNV) -1,
  (GdkGLProc_glVertex3hNV) -1,
  (GdkGLProc_glVertex3hvNV) -1,
  (GdkGLProc_glVertex4hNV) -1,
  (GdkGLProc_glVertex4hvNV) -1,
  (GdkGLProc_glNormal3hNV) -1,
  (GdkGLProc_glNormal3hvNV) -1,
  (GdkGLProc_glColor3hNV) -1,
  (GdkGLProc_glColor3hvNV) -1,
  (GdkGLProc_glColor4hNV) -1,
  (GdkGLProc_glColor4hvNV) -1,
  (GdkGLProc_glTexCoord1hNV) -1,
  (GdkGLProc_glTexCoord1hvNV) -1,
  (GdkGLProc_glTexCoord2hNV) -1,
  (GdkGLProc_glTexCoord2hvNV) -1,
  (GdkGLProc_glTexCoord3hNV) -1,
  (GdkGLProc_glTexCoord3hvNV) -1,
  (GdkGLProc_glTexCoord4hNV) -1,
  (GdkGLProc_glTexCoord4hvNV) -1,
  (GdkGLProc_glMultiTexCoord1hNV) -1,
  (GdkGLProc_glMultiTexCoord1hvNV) -1,
  (GdkGLProc_glMultiTexCoord2hNV) -1,
  (GdkGLProc_glMultiTexCoord2hvNV) -1,
  (GdkGLProc_glMultiTexCoord3hNV) -1,
  (GdkGLProc_glMultiTexCoord3hvNV) -1,
  (GdkGLProc_glMultiTexCoord4hNV) -1,
  (GdkGLProc_glMultiTexCoord4hvNV) -1,
  (GdkGLProc_glFogCoordhNV) -1,
  (GdkGLProc_glFogCoordhvNV) -1,
  (GdkGLProc_glSecondaryColor3hNV) -1,
  (GdkGLProc_glSecondaryColor3hvNV) -1,
  (GdkGLProc_glVertexWeighthNV) -1,
  (GdkGLProc_glVertexWeighthvNV) -1,
  (GdkGLProc_glVertexAttrib1hNV) -1,
  (GdkGLProc_glVertexAttrib1hvNV) -1,
  (GdkGLProc_glVertexAttrib2hNV) -1,
  (GdkGLProc_glVertexAttrib2hvNV) -1,
  (GdkGLProc_glVertexAttrib3hNV) -1,
  (GdkGLProc_glVertexAttrib3hvNV) -1,
  (GdkGLProc_glVertexAttrib4hNV) -1,
  (GdkGLProc_glVertexAttrib4hvNV) -1,
  (GdkGLProc_glVertexAttribs1hvNV) -1,
  (GdkGLProc_glVertexAttribs2hvNV) -1,
  (GdkGLProc_glVertexAttribs3hvNV) -1,
  (GdkGLProc_glVertexAttribs4hvNV) -1
};

/* glVertex2hNV */
GdkGLProc
gdk_gl_get_glVertex2hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertex2hNV == (GdkGLProc_glVertex2hNV) -1)
    _procs_GL_NV_half_float.glVertex2hNV =
      (GdkGLProc_glVertex2hNV) gdk_gl_get_proc_address ("glVertex2hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertex2hNV () - %s",
               (_procs_GL_NV_half_float.glVertex2hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertex2hNV);
}

/* glVertex2hvNV */
GdkGLProc
gdk_gl_get_glVertex2hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertex2hvNV == (GdkGLProc_glVertex2hvNV) -1)
    _procs_GL_NV_half_float.glVertex2hvNV =
      (GdkGLProc_glVertex2hvNV) gdk_gl_get_proc_address ("glVertex2hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertex2hvNV () - %s",
               (_procs_GL_NV_half_float.glVertex2hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertex2hvNV);
}

/* glVertex3hNV */
GdkGLProc
gdk_gl_get_glVertex3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertex3hNV == (GdkGLProc_glVertex3hNV) -1)
    _procs_GL_NV_half_float.glVertex3hNV =
      (GdkGLProc_glVertex3hNV) gdk_gl_get_proc_address ("glVertex3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertex3hNV () - %s",
               (_procs_GL_NV_half_float.glVertex3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertex3hNV);
}

/* glVertex3hvNV */
GdkGLProc
gdk_gl_get_glVertex3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertex3hvNV == (GdkGLProc_glVertex3hvNV) -1)
    _procs_GL_NV_half_float.glVertex3hvNV =
      (GdkGLProc_glVertex3hvNV) gdk_gl_get_proc_address ("glVertex3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertex3hvNV () - %s",
               (_procs_GL_NV_half_float.glVertex3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertex3hvNV);
}

/* glVertex4hNV */
GdkGLProc
gdk_gl_get_glVertex4hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertex4hNV == (GdkGLProc_glVertex4hNV) -1)
    _procs_GL_NV_half_float.glVertex4hNV =
      (GdkGLProc_glVertex4hNV) gdk_gl_get_proc_address ("glVertex4hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertex4hNV () - %s",
               (_procs_GL_NV_half_float.glVertex4hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertex4hNV);
}

/* glVertex4hvNV */
GdkGLProc
gdk_gl_get_glVertex4hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertex4hvNV == (GdkGLProc_glVertex4hvNV) -1)
    _procs_GL_NV_half_float.glVertex4hvNV =
      (GdkGLProc_glVertex4hvNV) gdk_gl_get_proc_address ("glVertex4hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertex4hvNV () - %s",
               (_procs_GL_NV_half_float.glVertex4hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertex4hvNV);
}

/* glNormal3hNV */
GdkGLProc
gdk_gl_get_glNormal3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glNormal3hNV == (GdkGLProc_glNormal3hNV) -1)
    _procs_GL_NV_half_float.glNormal3hNV =
      (GdkGLProc_glNormal3hNV) gdk_gl_get_proc_address ("glNormal3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormal3hNV () - %s",
               (_procs_GL_NV_half_float.glNormal3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glNormal3hNV);
}

/* glNormal3hvNV */
GdkGLProc
gdk_gl_get_glNormal3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glNormal3hvNV == (GdkGLProc_glNormal3hvNV) -1)
    _procs_GL_NV_half_float.glNormal3hvNV =
      (GdkGLProc_glNormal3hvNV) gdk_gl_get_proc_address ("glNormal3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNormal3hvNV () - %s",
               (_procs_GL_NV_half_float.glNormal3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glNormal3hvNV);
}

/* glColor3hNV */
GdkGLProc
gdk_gl_get_glColor3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glColor3hNV == (GdkGLProc_glColor3hNV) -1)
    _procs_GL_NV_half_float.glColor3hNV =
      (GdkGLProc_glColor3hNV) gdk_gl_get_proc_address ("glColor3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor3hNV () - %s",
               (_procs_GL_NV_half_float.glColor3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glColor3hNV);
}

/* glColor3hvNV */
GdkGLProc
gdk_gl_get_glColor3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glColor3hvNV == (GdkGLProc_glColor3hvNV) -1)
    _procs_GL_NV_half_float.glColor3hvNV =
      (GdkGLProc_glColor3hvNV) gdk_gl_get_proc_address ("glColor3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor3hvNV () - %s",
               (_procs_GL_NV_half_float.glColor3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glColor3hvNV);
}

/* glColor4hNV */
GdkGLProc
gdk_gl_get_glColor4hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glColor4hNV == (GdkGLProc_glColor4hNV) -1)
    _procs_GL_NV_half_float.glColor4hNV =
      (GdkGLProc_glColor4hNV) gdk_gl_get_proc_address ("glColor4hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4hNV () - %s",
               (_procs_GL_NV_half_float.glColor4hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glColor4hNV);
}

/* glColor4hvNV */
GdkGLProc
gdk_gl_get_glColor4hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glColor4hvNV == (GdkGLProc_glColor4hvNV) -1)
    _procs_GL_NV_half_float.glColor4hvNV =
      (GdkGLProc_glColor4hvNV) gdk_gl_get_proc_address ("glColor4hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glColor4hvNV () - %s",
               (_procs_GL_NV_half_float.glColor4hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glColor4hvNV);
}

/* glTexCoord1hNV */
GdkGLProc
gdk_gl_get_glTexCoord1hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord1hNV == (GdkGLProc_glTexCoord1hNV) -1)
    _procs_GL_NV_half_float.glTexCoord1hNV =
      (GdkGLProc_glTexCoord1hNV) gdk_gl_get_proc_address ("glTexCoord1hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord1hNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord1hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord1hNV);
}

/* glTexCoord1hvNV */
GdkGLProc
gdk_gl_get_glTexCoord1hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord1hvNV == (GdkGLProc_glTexCoord1hvNV) -1)
    _procs_GL_NV_half_float.glTexCoord1hvNV =
      (GdkGLProc_glTexCoord1hvNV) gdk_gl_get_proc_address ("glTexCoord1hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord1hvNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord1hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord1hvNV);
}

/* glTexCoord2hNV */
GdkGLProc
gdk_gl_get_glTexCoord2hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord2hNV == (GdkGLProc_glTexCoord2hNV) -1)
    _procs_GL_NV_half_float.glTexCoord2hNV =
      (GdkGLProc_glTexCoord2hNV) gdk_gl_get_proc_address ("glTexCoord2hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2hNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord2hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord2hNV);
}

/* glTexCoord2hvNV */
GdkGLProc
gdk_gl_get_glTexCoord2hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord2hvNV == (GdkGLProc_glTexCoord2hvNV) -1)
    _procs_GL_NV_half_float.glTexCoord2hvNV =
      (GdkGLProc_glTexCoord2hvNV) gdk_gl_get_proc_address ("glTexCoord2hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord2hvNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord2hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord2hvNV);
}

/* glTexCoord3hNV */
GdkGLProc
gdk_gl_get_glTexCoord3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord3hNV == (GdkGLProc_glTexCoord3hNV) -1)
    _procs_GL_NV_half_float.glTexCoord3hNV =
      (GdkGLProc_glTexCoord3hNV) gdk_gl_get_proc_address ("glTexCoord3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord3hNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord3hNV);
}

/* glTexCoord3hvNV */
GdkGLProc
gdk_gl_get_glTexCoord3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord3hvNV == (GdkGLProc_glTexCoord3hvNV) -1)
    _procs_GL_NV_half_float.glTexCoord3hvNV =
      (GdkGLProc_glTexCoord3hvNV) gdk_gl_get_proc_address ("glTexCoord3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord3hvNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord3hvNV);
}

/* glTexCoord4hNV */
GdkGLProc
gdk_gl_get_glTexCoord4hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord4hNV == (GdkGLProc_glTexCoord4hNV) -1)
    _procs_GL_NV_half_float.glTexCoord4hNV =
      (GdkGLProc_glTexCoord4hNV) gdk_gl_get_proc_address ("glTexCoord4hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord4hNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord4hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord4hNV);
}

/* glTexCoord4hvNV */
GdkGLProc
gdk_gl_get_glTexCoord4hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glTexCoord4hvNV == (GdkGLProc_glTexCoord4hvNV) -1)
    _procs_GL_NV_half_float.glTexCoord4hvNV =
      (GdkGLProc_glTexCoord4hvNV) gdk_gl_get_proc_address ("glTexCoord4hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTexCoord4hvNV () - %s",
               (_procs_GL_NV_half_float.glTexCoord4hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glTexCoord4hvNV);
}

/* glMultiTexCoord1hNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord1hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord1hNV == (GdkGLProc_glMultiTexCoord1hNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord1hNV =
      (GdkGLProc_glMultiTexCoord1hNV) gdk_gl_get_proc_address ("glMultiTexCoord1hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1hNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord1hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord1hNV);
}

/* glMultiTexCoord1hvNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord1hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord1hvNV == (GdkGLProc_glMultiTexCoord1hvNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord1hvNV =
      (GdkGLProc_glMultiTexCoord1hvNV) gdk_gl_get_proc_address ("glMultiTexCoord1hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1hvNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord1hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord1hvNV);
}

/* glMultiTexCoord2hNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord2hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord2hNV == (GdkGLProc_glMultiTexCoord2hNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord2hNV =
      (GdkGLProc_glMultiTexCoord2hNV) gdk_gl_get_proc_address ("glMultiTexCoord2hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2hNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord2hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord2hNV);
}

/* glMultiTexCoord2hvNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord2hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord2hvNV == (GdkGLProc_glMultiTexCoord2hvNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord2hvNV =
      (GdkGLProc_glMultiTexCoord2hvNV) gdk_gl_get_proc_address ("glMultiTexCoord2hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2hvNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord2hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord2hvNV);
}

/* glMultiTexCoord3hNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord3hNV == (GdkGLProc_glMultiTexCoord3hNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord3hNV =
      (GdkGLProc_glMultiTexCoord3hNV) gdk_gl_get_proc_address ("glMultiTexCoord3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3hNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord3hNV);
}

/* glMultiTexCoord3hvNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord3hvNV == (GdkGLProc_glMultiTexCoord3hvNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord3hvNV =
      (GdkGLProc_glMultiTexCoord3hvNV) gdk_gl_get_proc_address ("glMultiTexCoord3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3hvNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord3hvNV);
}

/* glMultiTexCoord4hNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord4hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord4hNV == (GdkGLProc_glMultiTexCoord4hNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord4hNV =
      (GdkGLProc_glMultiTexCoord4hNV) gdk_gl_get_proc_address ("glMultiTexCoord4hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4hNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord4hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord4hNV);
}

/* glMultiTexCoord4hvNV */
GdkGLProc
gdk_gl_get_glMultiTexCoord4hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glMultiTexCoord4hvNV == (GdkGLProc_glMultiTexCoord4hvNV) -1)
    _procs_GL_NV_half_float.glMultiTexCoord4hvNV =
      (GdkGLProc_glMultiTexCoord4hvNV) gdk_gl_get_proc_address ("glMultiTexCoord4hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4hvNV () - %s",
               (_procs_GL_NV_half_float.glMultiTexCoord4hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glMultiTexCoord4hvNV);
}

/* glFogCoordhNV */
GdkGLProc
gdk_gl_get_glFogCoordhNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glFogCoordhNV == (GdkGLProc_glFogCoordhNV) -1)
    _procs_GL_NV_half_float.glFogCoordhNV =
      (GdkGLProc_glFogCoordhNV) gdk_gl_get_proc_address ("glFogCoordhNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordhNV () - %s",
               (_procs_GL_NV_half_float.glFogCoordhNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glFogCoordhNV);
}

/* glFogCoordhvNV */
GdkGLProc
gdk_gl_get_glFogCoordhvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glFogCoordhvNV == (GdkGLProc_glFogCoordhvNV) -1)
    _procs_GL_NV_half_float.glFogCoordhvNV =
      (GdkGLProc_glFogCoordhvNV) gdk_gl_get_proc_address ("glFogCoordhvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFogCoordhvNV () - %s",
               (_procs_GL_NV_half_float.glFogCoordhvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glFogCoordhvNV);
}

/* glSecondaryColor3hNV */
GdkGLProc
gdk_gl_get_glSecondaryColor3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glSecondaryColor3hNV == (GdkGLProc_glSecondaryColor3hNV) -1)
    _procs_GL_NV_half_float.glSecondaryColor3hNV =
      (GdkGLProc_glSecondaryColor3hNV) gdk_gl_get_proc_address ("glSecondaryColor3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3hNV () - %s",
               (_procs_GL_NV_half_float.glSecondaryColor3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glSecondaryColor3hNV);
}

/* glSecondaryColor3hvNV */
GdkGLProc
gdk_gl_get_glSecondaryColor3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glSecondaryColor3hvNV == (GdkGLProc_glSecondaryColor3hvNV) -1)
    _procs_GL_NV_half_float.glSecondaryColor3hvNV =
      (GdkGLProc_glSecondaryColor3hvNV) gdk_gl_get_proc_address ("glSecondaryColor3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSecondaryColor3hvNV () - %s",
               (_procs_GL_NV_half_float.glSecondaryColor3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glSecondaryColor3hvNV);
}

/* glVertexWeighthNV */
GdkGLProc
gdk_gl_get_glVertexWeighthNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexWeighthNV == (GdkGLProc_glVertexWeighthNV) -1)
    _procs_GL_NV_half_float.glVertexWeighthNV =
      (GdkGLProc_glVertexWeighthNV) gdk_gl_get_proc_address ("glVertexWeighthNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexWeighthNV () - %s",
               (_procs_GL_NV_half_float.glVertexWeighthNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexWeighthNV);
}

/* glVertexWeighthvNV */
GdkGLProc
gdk_gl_get_glVertexWeighthvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexWeighthvNV == (GdkGLProc_glVertexWeighthvNV) -1)
    _procs_GL_NV_half_float.glVertexWeighthvNV =
      (GdkGLProc_glVertexWeighthvNV) gdk_gl_get_proc_address ("glVertexWeighthvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexWeighthvNV () - %s",
               (_procs_GL_NV_half_float.glVertexWeighthvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexWeighthvNV);
}

/* glVertexAttrib1hNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib1hNV == (GdkGLProc_glVertexAttrib1hNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib1hNV =
      (GdkGLProc_glVertexAttrib1hNV) gdk_gl_get_proc_address ("glVertexAttrib1hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1hNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib1hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib1hNV);
}

/* glVertexAttrib1hvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib1hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib1hvNV == (GdkGLProc_glVertexAttrib1hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib1hvNV =
      (GdkGLProc_glVertexAttrib1hvNV) gdk_gl_get_proc_address ("glVertexAttrib1hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib1hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib1hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib1hvNV);
}

/* glVertexAttrib2hNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib2hNV == (GdkGLProc_glVertexAttrib2hNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib2hNV =
      (GdkGLProc_glVertexAttrib2hNV) gdk_gl_get_proc_address ("glVertexAttrib2hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2hNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib2hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib2hNV);
}

/* glVertexAttrib2hvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib2hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib2hvNV == (GdkGLProc_glVertexAttrib2hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib2hvNV =
      (GdkGLProc_glVertexAttrib2hvNV) gdk_gl_get_proc_address ("glVertexAttrib2hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib2hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib2hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib2hvNV);
}

/* glVertexAttrib3hNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib3hNV == (GdkGLProc_glVertexAttrib3hNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib3hNV =
      (GdkGLProc_glVertexAttrib3hNV) gdk_gl_get_proc_address ("glVertexAttrib3hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3hNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib3hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib3hNV);
}

/* glVertexAttrib3hvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib3hvNV == (GdkGLProc_glVertexAttrib3hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib3hvNV =
      (GdkGLProc_glVertexAttrib3hvNV) gdk_gl_get_proc_address ("glVertexAttrib3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib3hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib3hvNV);
}

/* glVertexAttrib4hNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4hNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib4hNV == (GdkGLProc_glVertexAttrib4hNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib4hNV =
      (GdkGLProc_glVertexAttrib4hNV) gdk_gl_get_proc_address ("glVertexAttrib4hNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4hNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib4hNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib4hNV);
}

/* glVertexAttrib4hvNV */
GdkGLProc
gdk_gl_get_glVertexAttrib4hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttrib4hvNV == (GdkGLProc_glVertexAttrib4hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttrib4hvNV =
      (GdkGLProc_glVertexAttrib4hvNV) gdk_gl_get_proc_address ("glVertexAttrib4hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttrib4hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttrib4hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttrib4hvNV);
}

/* glVertexAttribs1hvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs1hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttribs1hvNV == (GdkGLProc_glVertexAttribs1hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttribs1hvNV =
      (GdkGLProc_glVertexAttribs1hvNV) gdk_gl_get_proc_address ("glVertexAttribs1hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs1hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttribs1hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttribs1hvNV);
}

/* glVertexAttribs2hvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs2hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttribs2hvNV == (GdkGLProc_glVertexAttribs2hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttribs2hvNV =
      (GdkGLProc_glVertexAttribs2hvNV) gdk_gl_get_proc_address ("glVertexAttribs2hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs2hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttribs2hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttribs2hvNV);
}

/* glVertexAttribs3hvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs3hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttribs3hvNV == (GdkGLProc_glVertexAttribs3hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttribs3hvNV =
      (GdkGLProc_glVertexAttribs3hvNV) gdk_gl_get_proc_address ("glVertexAttribs3hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs3hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttribs3hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttribs3hvNV);
}

/* glVertexAttribs4hvNV */
GdkGLProc
gdk_gl_get_glVertexAttribs4hvNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_half_float.glVertexAttribs4hvNV == (GdkGLProc_glVertexAttribs4hvNV) -1)
    _procs_GL_NV_half_float.glVertexAttribs4hvNV =
      (GdkGLProc_glVertexAttribs4hvNV) gdk_gl_get_proc_address ("glVertexAttribs4hvNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribs4hvNV () - %s",
               (_procs_GL_NV_half_float.glVertexAttribs4hvNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_half_float.glVertexAttribs4hvNV);
}

/* Get GL_NV_half_float functions */
GdkGL_GL_NV_half_float *
gdk_gl_get_GL_NV_half_float (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_half_float");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertex2hNV () != NULL);
          supported &= (gdk_gl_get_glVertex2hvNV () != NULL);
          supported &= (gdk_gl_get_glVertex3hNV () != NULL);
          supported &= (gdk_gl_get_glVertex3hvNV () != NULL);
          supported &= (gdk_gl_get_glVertex4hNV () != NULL);
          supported &= (gdk_gl_get_glVertex4hvNV () != NULL);
          supported &= (gdk_gl_get_glNormal3hNV () != NULL);
          supported &= (gdk_gl_get_glNormal3hvNV () != NULL);
          supported &= (gdk_gl_get_glColor3hNV () != NULL);
          supported &= (gdk_gl_get_glColor3hvNV () != NULL);
          supported &= (gdk_gl_get_glColor4hNV () != NULL);
          supported &= (gdk_gl_get_glColor4hvNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord1hNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord1hvNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord2hNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord2hvNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord3hNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord3hvNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord4hNV () != NULL);
          supported &= (gdk_gl_get_glTexCoord4hvNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1hNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1hvNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2hNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2hvNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3hNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3hvNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4hNV () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4hvNV () != NULL);
          supported &= (gdk_gl_get_glFogCoordhNV () != NULL);
          supported &= (gdk_gl_get_glFogCoordhvNV () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3hNV () != NULL);
          supported &= (gdk_gl_get_glSecondaryColor3hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexWeighthNV () != NULL);
          supported &= (gdk_gl_get_glVertexWeighthvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1hNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib1hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2hNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib2hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3hNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib3hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4hNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttrib4hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs1hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs2hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs3hvNV () != NULL);
          supported &= (gdk_gl_get_glVertexAttribs4hvNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_half_float () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_half_float;
}

/*
 * GL_NV_pixel_data_range
 */

static GdkGL_GL_NV_pixel_data_range _procs_GL_NV_pixel_data_range = {
  (GdkGLProc_glPixelDataRangeNV) -1,
  (GdkGLProc_glFlushPixelDataRangeNV) -1
};

/* glPixelDataRangeNV */
GdkGLProc
gdk_gl_get_glPixelDataRangeNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_pixel_data_range.glPixelDataRangeNV == (GdkGLProc_glPixelDataRangeNV) -1)
    _procs_GL_NV_pixel_data_range.glPixelDataRangeNV =
      (GdkGLProc_glPixelDataRangeNV) gdk_gl_get_proc_address ("glPixelDataRangeNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPixelDataRangeNV () - %s",
               (_procs_GL_NV_pixel_data_range.glPixelDataRangeNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_pixel_data_range.glPixelDataRangeNV);
}

/* glFlushPixelDataRangeNV */
GdkGLProc
gdk_gl_get_glFlushPixelDataRangeNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_pixel_data_range.glFlushPixelDataRangeNV == (GdkGLProc_glFlushPixelDataRangeNV) -1)
    _procs_GL_NV_pixel_data_range.glFlushPixelDataRangeNV =
      (GdkGLProc_glFlushPixelDataRangeNV) gdk_gl_get_proc_address ("glFlushPixelDataRangeNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFlushPixelDataRangeNV () - %s",
               (_procs_GL_NV_pixel_data_range.glFlushPixelDataRangeNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_pixel_data_range.glFlushPixelDataRangeNV);
}

/* Get GL_NV_pixel_data_range functions */
GdkGL_GL_NV_pixel_data_range *
gdk_gl_get_GL_NV_pixel_data_range (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_pixel_data_range");

      if (supported)
        {
          supported &= (gdk_gl_get_glPixelDataRangeNV () != NULL);
          supported &= (gdk_gl_get_glFlushPixelDataRangeNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_pixel_data_range () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_pixel_data_range;
}

/*
 * GL_NV_primitive_restart
 */

static GdkGL_GL_NV_primitive_restart _procs_GL_NV_primitive_restart = {
  (GdkGLProc_glPrimitiveRestartNV) -1,
  (GdkGLProc_glPrimitiveRestartIndexNV) -1
};

/* glPrimitiveRestartNV */
GdkGLProc
gdk_gl_get_glPrimitiveRestartNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_primitive_restart.glPrimitiveRestartNV == (GdkGLProc_glPrimitiveRestartNV) -1)
    _procs_GL_NV_primitive_restart.glPrimitiveRestartNV =
      (GdkGLProc_glPrimitiveRestartNV) gdk_gl_get_proc_address ("glPrimitiveRestartNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPrimitiveRestartNV () - %s",
               (_procs_GL_NV_primitive_restart.glPrimitiveRestartNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_primitive_restart.glPrimitiveRestartNV);
}

/* glPrimitiveRestartIndexNV */
GdkGLProc
gdk_gl_get_glPrimitiveRestartIndexNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_primitive_restart.glPrimitiveRestartIndexNV == (GdkGLProc_glPrimitiveRestartIndexNV) -1)
    _procs_GL_NV_primitive_restart.glPrimitiveRestartIndexNV =
      (GdkGLProc_glPrimitiveRestartIndexNV) gdk_gl_get_proc_address ("glPrimitiveRestartIndexNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPrimitiveRestartIndexNV () - %s",
               (_procs_GL_NV_primitive_restart.glPrimitiveRestartIndexNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_primitive_restart.glPrimitiveRestartIndexNV);
}

/* Get GL_NV_primitive_restart functions */
GdkGL_GL_NV_primitive_restart *
gdk_gl_get_GL_NV_primitive_restart (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_primitive_restart");

      if (supported)
        {
          supported &= (gdk_gl_get_glPrimitiveRestartNV () != NULL);
          supported &= (gdk_gl_get_glPrimitiveRestartIndexNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_primitive_restart () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_primitive_restart;
}

/*
 * GL_ATI_map_object_buffer
 */

static GdkGL_GL_ATI_map_object_buffer _procs_GL_ATI_map_object_buffer = {
  (GdkGLProc_glMapObjectBufferATI) -1,
  (GdkGLProc_glUnmapObjectBufferATI) -1
};

/* glMapObjectBufferATI */
GdkGLProc
gdk_gl_get_glMapObjectBufferATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_map_object_buffer.glMapObjectBufferATI == (GdkGLProc_glMapObjectBufferATI) -1)
    _procs_GL_ATI_map_object_buffer.glMapObjectBufferATI =
      (GdkGLProc_glMapObjectBufferATI) gdk_gl_get_proc_address ("glMapObjectBufferATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapObjectBufferATI () - %s",
               (_procs_GL_ATI_map_object_buffer.glMapObjectBufferATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_map_object_buffer.glMapObjectBufferATI);
}

/* glUnmapObjectBufferATI */
GdkGLProc
gdk_gl_get_glUnmapObjectBufferATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_map_object_buffer.glUnmapObjectBufferATI == (GdkGLProc_glUnmapObjectBufferATI) -1)
    _procs_GL_ATI_map_object_buffer.glUnmapObjectBufferATI =
      (GdkGLProc_glUnmapObjectBufferATI) gdk_gl_get_proc_address ("glUnmapObjectBufferATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glUnmapObjectBufferATI () - %s",
               (_procs_GL_ATI_map_object_buffer.glUnmapObjectBufferATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_map_object_buffer.glUnmapObjectBufferATI);
}

/* Get GL_ATI_map_object_buffer functions */
GdkGL_GL_ATI_map_object_buffer *
gdk_gl_get_GL_ATI_map_object_buffer (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_map_object_buffer");

      if (supported)
        {
          supported &= (gdk_gl_get_glMapObjectBufferATI () != NULL);
          supported &= (gdk_gl_get_glUnmapObjectBufferATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_map_object_buffer () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_map_object_buffer;
}

/*
 * GL_ATI_separate_stencil
 */

static GdkGL_GL_ATI_separate_stencil _procs_GL_ATI_separate_stencil = {
  (GdkGLProc_glStencilOpSeparateATI) -1,
  (GdkGLProc_glStencilFuncSeparateATI) -1
};

/* glStencilOpSeparateATI */
GdkGLProc
gdk_gl_get_glStencilOpSeparateATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_separate_stencil.glStencilOpSeparateATI == (GdkGLProc_glStencilOpSeparateATI) -1)
    _procs_GL_ATI_separate_stencil.glStencilOpSeparateATI =
      (GdkGLProc_glStencilOpSeparateATI) gdk_gl_get_proc_address ("glStencilOpSeparateATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glStencilOpSeparateATI () - %s",
               (_procs_GL_ATI_separate_stencil.glStencilOpSeparateATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_separate_stencil.glStencilOpSeparateATI);
}

/* glStencilFuncSeparateATI */
GdkGLProc
gdk_gl_get_glStencilFuncSeparateATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_separate_stencil.glStencilFuncSeparateATI == (GdkGLProc_glStencilFuncSeparateATI) -1)
    _procs_GL_ATI_separate_stencil.glStencilFuncSeparateATI =
      (GdkGLProc_glStencilFuncSeparateATI) gdk_gl_get_proc_address ("glStencilFuncSeparateATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glStencilFuncSeparateATI () - %s",
               (_procs_GL_ATI_separate_stencil.glStencilFuncSeparateATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_separate_stencil.glStencilFuncSeparateATI);
}

/* Get GL_ATI_separate_stencil functions */
GdkGL_GL_ATI_separate_stencil *
gdk_gl_get_GL_ATI_separate_stencil (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_separate_stencil");

      if (supported)
        {
          supported &= (gdk_gl_get_glStencilOpSeparateATI () != NULL);
          supported &= (gdk_gl_get_glStencilFuncSeparateATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_separate_stencil () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_separate_stencil;
}

/*
 * GL_ATI_vertex_attrib_array_object
 */

static GdkGL_GL_ATI_vertex_attrib_array_object _procs_GL_ATI_vertex_attrib_array_object = {
  (GdkGLProc_glVertexAttribArrayObjectATI) -1,
  (GdkGLProc_glGetVertexAttribArrayObjectfvATI) -1,
  (GdkGLProc_glGetVertexAttribArrayObjectivATI) -1
};

/* glVertexAttribArrayObjectATI */
GdkGLProc
gdk_gl_get_glVertexAttribArrayObjectATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_attrib_array_object.glVertexAttribArrayObjectATI == (GdkGLProc_glVertexAttribArrayObjectATI) -1)
    _procs_GL_ATI_vertex_attrib_array_object.glVertexAttribArrayObjectATI =
      (GdkGLProc_glVertexAttribArrayObjectATI) gdk_gl_get_proc_address ("glVertexAttribArrayObjectATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glVertexAttribArrayObjectATI () - %s",
               (_procs_GL_ATI_vertex_attrib_array_object.glVertexAttribArrayObjectATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_attrib_array_object.glVertexAttribArrayObjectATI);
}

/* glGetVertexAttribArrayObjectfvATI */
GdkGLProc
gdk_gl_get_glGetVertexAttribArrayObjectfvATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectfvATI == (GdkGLProc_glGetVertexAttribArrayObjectfvATI) -1)
    _procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectfvATI =
      (GdkGLProc_glGetVertexAttribArrayObjectfvATI) gdk_gl_get_proc_address ("glGetVertexAttribArrayObjectfvATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribArrayObjectfvATI () - %s",
               (_procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectfvATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectfvATI);
}

/* glGetVertexAttribArrayObjectivATI */
GdkGLProc
gdk_gl_get_glGetVertexAttribArrayObjectivATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectivATI == (GdkGLProc_glGetVertexAttribArrayObjectivATI) -1)
    _procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectivATI =
      (GdkGLProc_glGetVertexAttribArrayObjectivATI) gdk_gl_get_proc_address ("glGetVertexAttribArrayObjectivATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetVertexAttribArrayObjectivATI () - %s",
               (_procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectivATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_vertex_attrib_array_object.glGetVertexAttribArrayObjectivATI);
}

/* Get GL_ATI_vertex_attrib_array_object functions */
GdkGL_GL_ATI_vertex_attrib_array_object *
gdk_gl_get_GL_ATI_vertex_attrib_array_object (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_vertex_attrib_array_object");

      if (supported)
        {
          supported &= (gdk_gl_get_glVertexAttribArrayObjectATI () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribArrayObjectfvATI () != NULL);
          supported &= (gdk_gl_get_glGetVertexAttribArrayObjectivATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_vertex_attrib_array_object () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_vertex_attrib_array_object;
}

/*
 * GL_APPLE_texture_range
 */

static GdkGL_GL_APPLE_texture_range _procs_GL_APPLE_texture_range = {
  (GdkGLProc_glTextureRangeAPPLE) -1,
  (GdkGLProc_glGetTexParameterPointervAPPLE) -1
};

/* glTextureRangeAPPLE */
GdkGLProc
gdk_gl_get_glTextureRangeAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_texture_range.glTextureRangeAPPLE == (GdkGLProc_glTextureRangeAPPLE) -1)
    _procs_GL_APPLE_texture_range.glTextureRangeAPPLE =
      (GdkGLProc_glTextureRangeAPPLE) gdk_gl_get_proc_address ("glTextureRangeAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTextureRangeAPPLE () - %s",
               (_procs_GL_APPLE_texture_range.glTextureRangeAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_texture_range.glTextureRangeAPPLE);
}

/* glGetTexParameterPointervAPPLE */
GdkGLProc
gdk_gl_get_glGetTexParameterPointervAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_texture_range.glGetTexParameterPointervAPPLE == (GdkGLProc_glGetTexParameterPointervAPPLE) -1)
    _procs_GL_APPLE_texture_range.glGetTexParameterPointervAPPLE =
      (GdkGLProc_glGetTexParameterPointervAPPLE) gdk_gl_get_proc_address ("glGetTexParameterPointervAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetTexParameterPointervAPPLE () - %s",
               (_procs_GL_APPLE_texture_range.glGetTexParameterPointervAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_texture_range.glGetTexParameterPointervAPPLE);
}

/* Get GL_APPLE_texture_range functions */
GdkGL_GL_APPLE_texture_range *
gdk_gl_get_GL_APPLE_texture_range (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_APPLE_texture_range");

      if (supported)
        {
          supported &= (gdk_gl_get_glTextureRangeAPPLE () != NULL);
          supported &= (gdk_gl_get_glGetTexParameterPointervAPPLE () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_APPLE_texture_range () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_APPLE_texture_range;
}

/*
 * GL_APPLE_vertex_program_evaluators
 */

static GdkGL_GL_APPLE_vertex_program_evaluators _procs_GL_APPLE_vertex_program_evaluators = {
  (GdkGLProc_glEnableVertexAttribAPPLE) -1,
  (GdkGLProc_glDisableVertexAttribAPPLE) -1,
  (GdkGLProc_glIsVertexAttribEnabledAPPLE) -1,
  (GdkGLProc_glMapVertexAttrib1dAPPLE) -1,
  (GdkGLProc_glMapVertexAttrib1fAPPLE) -1,
  (GdkGLProc_glMapVertexAttrib2dAPPLE) -1,
  (GdkGLProc_glMapVertexAttrib2fAPPLE) -1
};

/* glEnableVertexAttribAPPLE */
GdkGLProc
gdk_gl_get_glEnableVertexAttribAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glEnableVertexAttribAPPLE == (GdkGLProc_glEnableVertexAttribAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glEnableVertexAttribAPPLE =
      (GdkGLProc_glEnableVertexAttribAPPLE) gdk_gl_get_proc_address ("glEnableVertexAttribAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEnableVertexAttribAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glEnableVertexAttribAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glEnableVertexAttribAPPLE);
}

/* glDisableVertexAttribAPPLE */
GdkGLProc
gdk_gl_get_glDisableVertexAttribAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glDisableVertexAttribAPPLE == (GdkGLProc_glDisableVertexAttribAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glDisableVertexAttribAPPLE =
      (GdkGLProc_glDisableVertexAttribAPPLE) gdk_gl_get_proc_address ("glDisableVertexAttribAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDisableVertexAttribAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glDisableVertexAttribAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glDisableVertexAttribAPPLE);
}

/* glIsVertexAttribEnabledAPPLE */
GdkGLProc
gdk_gl_get_glIsVertexAttribEnabledAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glIsVertexAttribEnabledAPPLE == (GdkGLProc_glIsVertexAttribEnabledAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glIsVertexAttribEnabledAPPLE =
      (GdkGLProc_glIsVertexAttribEnabledAPPLE) gdk_gl_get_proc_address ("glIsVertexAttribEnabledAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glIsVertexAttribEnabledAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glIsVertexAttribEnabledAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glIsVertexAttribEnabledAPPLE);
}

/* glMapVertexAttrib1dAPPLE */
GdkGLProc
gdk_gl_get_glMapVertexAttrib1dAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1dAPPLE == (GdkGLProc_glMapVertexAttrib1dAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1dAPPLE =
      (GdkGLProc_glMapVertexAttrib1dAPPLE) gdk_gl_get_proc_address ("glMapVertexAttrib1dAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapVertexAttrib1dAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1dAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1dAPPLE);
}

/* glMapVertexAttrib1fAPPLE */
GdkGLProc
gdk_gl_get_glMapVertexAttrib1fAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1fAPPLE == (GdkGLProc_glMapVertexAttrib1fAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1fAPPLE =
      (GdkGLProc_glMapVertexAttrib1fAPPLE) gdk_gl_get_proc_address ("glMapVertexAttrib1fAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapVertexAttrib1fAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1fAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib1fAPPLE);
}

/* glMapVertexAttrib2dAPPLE */
GdkGLProc
gdk_gl_get_glMapVertexAttrib2dAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2dAPPLE == (GdkGLProc_glMapVertexAttrib2dAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2dAPPLE =
      (GdkGLProc_glMapVertexAttrib2dAPPLE) gdk_gl_get_proc_address ("glMapVertexAttrib2dAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapVertexAttrib2dAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2dAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2dAPPLE);
}

/* glMapVertexAttrib2fAPPLE */
GdkGLProc
gdk_gl_get_glMapVertexAttrib2fAPPLE (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2fAPPLE == (GdkGLProc_glMapVertexAttrib2fAPPLE) -1)
    _procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2fAPPLE =
      (GdkGLProc_glMapVertexAttrib2fAPPLE) gdk_gl_get_proc_address ("glMapVertexAttrib2fAPPLE");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMapVertexAttrib2fAPPLE () - %s",
               (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2fAPPLE) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_APPLE_vertex_program_evaluators.glMapVertexAttrib2fAPPLE);
}

/* Get GL_APPLE_vertex_program_evaluators functions */
GdkGL_GL_APPLE_vertex_program_evaluators *
gdk_gl_get_GL_APPLE_vertex_program_evaluators (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_APPLE_vertex_program_evaluators");

      if (supported)
        {
          supported &= (gdk_gl_get_glEnableVertexAttribAPPLE () != NULL);
          supported &= (gdk_gl_get_glDisableVertexAttribAPPLE () != NULL);
          supported &= (gdk_gl_get_glIsVertexAttribEnabledAPPLE () != NULL);
          supported &= (gdk_gl_get_glMapVertexAttrib1dAPPLE () != NULL);
          supported &= (gdk_gl_get_glMapVertexAttrib1fAPPLE () != NULL);
          supported &= (gdk_gl_get_glMapVertexAttrib2dAPPLE () != NULL);
          supported &= (gdk_gl_get_glMapVertexAttrib2fAPPLE () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_APPLE_vertex_program_evaluators () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_APPLE_vertex_program_evaluators;
}

/*
 * GL_ATI_blend_equation_separate
 */

static GdkGL_GL_ATI_blend_equation_separate _procs_GL_ATI_blend_equation_separate = {
  (GdkGLProc_glBlendEquationSeparateATI) -1
};

/* glBlendEquationSeparateATI */
GdkGLProc
gdk_gl_get_glBlendEquationSeparateATI (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATI_blend_equation_separate.glBlendEquationSeparateATI == (GdkGLProc_glBlendEquationSeparateATI) -1)
    _procs_GL_ATI_blend_equation_separate.glBlendEquationSeparateATI =
      (GdkGLProc_glBlendEquationSeparateATI) gdk_gl_get_proc_address ("glBlendEquationSeparateATI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBlendEquationSeparateATI () - %s",
               (_procs_GL_ATI_blend_equation_separate.glBlendEquationSeparateATI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATI_blend_equation_separate.glBlendEquationSeparateATI);
}

/* Get GL_ATI_blend_equation_separate functions */
GdkGL_GL_ATI_blend_equation_separate *
gdk_gl_get_GL_ATI_blend_equation_separate (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATI_blend_equation_separate");

      if (supported)
        {
          supported &= (gdk_gl_get_glBlendEquationSeparateATI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATI_blend_equation_separate () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATI_blend_equation_separate;
}

/*
 * GL_ATIX_pn_triangles
 */

static GdkGL_GL_ATIX_pn_triangles _procs_GL_ATIX_pn_triangles = {
  (GdkGLProc_glPNTrianglesiATIX) -1,
  (GdkGLProc_glPNTrianglesfATIX) -1
};

/* glPNTrianglesiATIX */
GdkGLProc
gdk_gl_get_glPNTrianglesiATIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATIX_pn_triangles.glPNTrianglesiATIX == (GdkGLProc_glPNTrianglesiATIX) -1)
    _procs_GL_ATIX_pn_triangles.glPNTrianglesiATIX =
      (GdkGLProc_glPNTrianglesiATIX) gdk_gl_get_proc_address ("glPNTrianglesiATIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPNTrianglesiATIX () - %s",
               (_procs_GL_ATIX_pn_triangles.glPNTrianglesiATIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATIX_pn_triangles.glPNTrianglesiATIX);
}

/* glPNTrianglesfATIX */
GdkGLProc
gdk_gl_get_glPNTrianglesfATIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_ATIX_pn_triangles.glPNTrianglesfATIX == (GdkGLProc_glPNTrianglesfATIX) -1)
    _procs_GL_ATIX_pn_triangles.glPNTrianglesfATIX =
      (GdkGLProc_glPNTrianglesfATIX) gdk_gl_get_proc_address ("glPNTrianglesfATIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glPNTrianglesfATIX () - %s",
               (_procs_GL_ATIX_pn_triangles.glPNTrianglesfATIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_ATIX_pn_triangles.glPNTrianglesfATIX);
}

/* Get GL_ATIX_pn_triangles functions */
GdkGL_GL_ATIX_pn_triangles *
gdk_gl_get_GL_ATIX_pn_triangles (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_ATIX_pn_triangles");

      if (supported)
        {
          supported &= (gdk_gl_get_glPNTrianglesiATIX () != NULL);
          supported &= (gdk_gl_get_glPNTrianglesfATIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_ATIX_pn_triangles () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_ATIX_pn_triangles;
}

/*
 * GL_Autodesk_facet_normal
 */

static GdkGL_GL_Autodesk_facet_normal _procs_GL_Autodesk_facet_normal = {
  (GdkGLProc_glFacetNormal3b) -1,
  (GdkGLProc_glFacetNormal3d) -1,
  (GdkGLProc_glFacetNormal3f) -1,
  (GdkGLProc_glFacetNormal3i) -1,
  (GdkGLProc_glFacetNormal3s) -1,
  (GdkGLProc_glFacetNormal3bv) -1,
  (GdkGLProc_glFacetNormal3dv) -1,
  (GdkGLProc_glFacetNormal3fv) -1,
  (GdkGLProc_glFacetNormal3iv) -1,
  (GdkGLProc_glFacetNormal3sv) -1
};

/* glFacetNormal3b */
GdkGLProc
gdk_gl_get_glFacetNormal3b (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3b == (GdkGLProc_glFacetNormal3b) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3b =
      (GdkGLProc_glFacetNormal3b) gdk_gl_get_proc_address ("glFacetNormal3b");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3b () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3b) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3b);
}

/* glFacetNormal3d */
GdkGLProc
gdk_gl_get_glFacetNormal3d (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3d == (GdkGLProc_glFacetNormal3d) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3d =
      (GdkGLProc_glFacetNormal3d) gdk_gl_get_proc_address ("glFacetNormal3d");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3d () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3d) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3d);
}

/* glFacetNormal3f */
GdkGLProc
gdk_gl_get_glFacetNormal3f (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3f == (GdkGLProc_glFacetNormal3f) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3f =
      (GdkGLProc_glFacetNormal3f) gdk_gl_get_proc_address ("glFacetNormal3f");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3f () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3f) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3f);
}

/* glFacetNormal3i */
GdkGLProc
gdk_gl_get_glFacetNormal3i (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3i == (GdkGLProc_glFacetNormal3i) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3i =
      (GdkGLProc_glFacetNormal3i) gdk_gl_get_proc_address ("glFacetNormal3i");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3i () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3i) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3i);
}

/* glFacetNormal3s */
GdkGLProc
gdk_gl_get_glFacetNormal3s (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3s == (GdkGLProc_glFacetNormal3s) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3s =
      (GdkGLProc_glFacetNormal3s) gdk_gl_get_proc_address ("glFacetNormal3s");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3s () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3s) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3s);
}

/* glFacetNormal3bv */
GdkGLProc
gdk_gl_get_glFacetNormal3bv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3bv == (GdkGLProc_glFacetNormal3bv) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3bv =
      (GdkGLProc_glFacetNormal3bv) gdk_gl_get_proc_address ("glFacetNormal3bv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3bv () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3bv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3bv);
}

/* glFacetNormal3dv */
GdkGLProc
gdk_gl_get_glFacetNormal3dv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3dv == (GdkGLProc_glFacetNormal3dv) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3dv =
      (GdkGLProc_glFacetNormal3dv) gdk_gl_get_proc_address ("glFacetNormal3dv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3dv () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3dv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3dv);
}

/* glFacetNormal3fv */
GdkGLProc
gdk_gl_get_glFacetNormal3fv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3fv == (GdkGLProc_glFacetNormal3fv) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3fv =
      (GdkGLProc_glFacetNormal3fv) gdk_gl_get_proc_address ("glFacetNormal3fv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3fv () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3fv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3fv);
}

/* glFacetNormal3iv */
GdkGLProc
gdk_gl_get_glFacetNormal3iv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3iv == (GdkGLProc_glFacetNormal3iv) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3iv =
      (GdkGLProc_glFacetNormal3iv) gdk_gl_get_proc_address ("glFacetNormal3iv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3iv () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3iv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3iv);
}

/* glFacetNormal3sv */
GdkGLProc
gdk_gl_get_glFacetNormal3sv (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_facet_normal.glFacetNormal3sv == (GdkGLProc_glFacetNormal3sv) -1)
    _procs_GL_Autodesk_facet_normal.glFacetNormal3sv =
      (GdkGLProc_glFacetNormal3sv) gdk_gl_get_proc_address ("glFacetNormal3sv");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFacetNormal3sv () - %s",
               (_procs_GL_Autodesk_facet_normal.glFacetNormal3sv) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_facet_normal.glFacetNormal3sv);
}

/* Get GL_Autodesk_facet_normal functions */
GdkGL_GL_Autodesk_facet_normal *
gdk_gl_get_GL_Autodesk_facet_normal (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_Autodesk_facet_normal");

      if (supported)
        {
          supported &= (gdk_gl_get_glFacetNormal3b () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3d () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3f () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3i () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3s () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3bv () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3dv () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3fv () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3iv () != NULL);
          supported &= (gdk_gl_get_glFacetNormal3sv () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_Autodesk_facet_normal () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_Autodesk_facet_normal;
}

/*
 * GL_Autodesk_valid_back_buffer_hint
 */

static GdkGL_GL_Autodesk_valid_back_buffer_hint _procs_GL_Autodesk_valid_back_buffer_hint = {
  (GdkGLProc_glWindowBackBufferHint) -1,
  (GdkGLProc_glValidBackBufferHint) -1
};

/* glWindowBackBufferHint */
GdkGLProc
gdk_gl_get_glWindowBackBufferHint (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_valid_back_buffer_hint.glWindowBackBufferHint == (GdkGLProc_glWindowBackBufferHint) -1)
    _procs_GL_Autodesk_valid_back_buffer_hint.glWindowBackBufferHint =
      (GdkGLProc_glWindowBackBufferHint) gdk_gl_get_proc_address ("glWindowBackBufferHint");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glWindowBackBufferHint () - %s",
               (_procs_GL_Autodesk_valid_back_buffer_hint.glWindowBackBufferHint) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_valid_back_buffer_hint.glWindowBackBufferHint);
}

/* glValidBackBufferHint */
GdkGLProc
gdk_gl_get_glValidBackBufferHint (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_Autodesk_valid_back_buffer_hint.glValidBackBufferHint == (GdkGLProc_glValidBackBufferHint) -1)
    _procs_GL_Autodesk_valid_back_buffer_hint.glValidBackBufferHint =
      (GdkGLProc_glValidBackBufferHint) gdk_gl_get_proc_address ("glValidBackBufferHint");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glValidBackBufferHint () - %s",
               (_procs_GL_Autodesk_valid_back_buffer_hint.glValidBackBufferHint) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_Autodesk_valid_back_buffer_hint.glValidBackBufferHint);
}

/* Get GL_Autodesk_valid_back_buffer_hint functions */
GdkGL_GL_Autodesk_valid_back_buffer_hint *
gdk_gl_get_GL_Autodesk_valid_back_buffer_hint (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_Autodesk_valid_back_buffer_hint");

      if (supported)
        {
          supported &= (gdk_gl_get_glWindowBackBufferHint () != NULL);
          supported &= (gdk_gl_get_glValidBackBufferHint () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_Autodesk_valid_back_buffer_hint () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_Autodesk_valid_back_buffer_hint;
}

/*
 * GL_EXT_depth_bounds_test
 */

static GdkGL_GL_EXT_depth_bounds_test _procs_GL_EXT_depth_bounds_test = {
  (GdkGLProc_glDepthBoundsEXT) -1
};

/* glDepthBoundsEXT */
GdkGLProc
gdk_gl_get_glDepthBoundsEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_depth_bounds_test.glDepthBoundsEXT == (GdkGLProc_glDepthBoundsEXT) -1)
    _procs_GL_EXT_depth_bounds_test.glDepthBoundsEXT =
      (GdkGLProc_glDepthBoundsEXT) gdk_gl_get_proc_address ("glDepthBoundsEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDepthBoundsEXT () - %s",
               (_procs_GL_EXT_depth_bounds_test.glDepthBoundsEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_depth_bounds_test.glDepthBoundsEXT);
}

/* Get GL_EXT_depth_bounds_test functions */
GdkGL_GL_EXT_depth_bounds_test *
gdk_gl_get_GL_EXT_depth_bounds_test (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_depth_bounds_test");

      if (supported)
        {
          supported &= (gdk_gl_get_glDepthBoundsEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_depth_bounds_test () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_depth_bounds_test;
}

/*
 * GL_EXT_fragment_lighting
 */

static GdkGL_GL_EXT_fragment_lighting _procs_GL_EXT_fragment_lighting = {
  (GdkGLProc_glFragmentLightModelfEXT) -1,
  (GdkGLProc_glFragmentLightModelfvEXT) -1,
  (GdkGLProc_glFragmentLightModeliEXT) -1,
  (GdkGLProc_glFragmentLightModelivEXT) -1,
  (GdkGLProc_glFragmentLightfEXT) -1,
  (GdkGLProc_glFragmentLightfvEXT) -1,
  (GdkGLProc_glFragmentLightiEXT) -1,
  (GdkGLProc_glFragmentLightivEXT) -1,
  (GdkGLProc_glGetFragmentLightfvEXT) -1,
  (GdkGLProc_glGetFragmentLightivEXT) -1,
  (GdkGLProc_glFragmentMaterialfEXT) -1,
  (GdkGLProc_glFragmentMaterialfvEXT) -1,
  (GdkGLProc_glFragmentMaterialiEXT) -1,
  (GdkGLProc_glFragmentMaterialivEXT) -1,
  (GdkGLProc_glFragmentColorMaterialEXT) -1,
  (GdkGLProc_glGetFragmentMaterialfvEXT) -1,
  (GdkGLProc_glGetFragmentMaterialivEXT) -1,
  (GdkGLProc_glLightEnviEXT) -1
};

/* glFragmentLightModelfEXT */
GdkGLProc
gdk_gl_get_glFragmentLightModelfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightModelfEXT == (GdkGLProc_glFragmentLightModelfEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightModelfEXT =
      (GdkGLProc_glFragmentLightModelfEXT) gdk_gl_get_proc_address ("glFragmentLightModelfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModelfEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightModelfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightModelfEXT);
}

/* glFragmentLightModelfvEXT */
GdkGLProc
gdk_gl_get_glFragmentLightModelfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightModelfvEXT == (GdkGLProc_glFragmentLightModelfvEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightModelfvEXT =
      (GdkGLProc_glFragmentLightModelfvEXT) gdk_gl_get_proc_address ("glFragmentLightModelfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModelfvEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightModelfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightModelfvEXT);
}

/* glFragmentLightModeliEXT */
GdkGLProc
gdk_gl_get_glFragmentLightModeliEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightModeliEXT == (GdkGLProc_glFragmentLightModeliEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightModeliEXT =
      (GdkGLProc_glFragmentLightModeliEXT) gdk_gl_get_proc_address ("glFragmentLightModeliEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModeliEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightModeliEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightModeliEXT);
}

/* glFragmentLightModelivEXT */
GdkGLProc
gdk_gl_get_glFragmentLightModelivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightModelivEXT == (GdkGLProc_glFragmentLightModelivEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightModelivEXT =
      (GdkGLProc_glFragmentLightModelivEXT) gdk_gl_get_proc_address ("glFragmentLightModelivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightModelivEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightModelivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightModelivEXT);
}

/* glFragmentLightfEXT */
GdkGLProc
gdk_gl_get_glFragmentLightfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightfEXT == (GdkGLProc_glFragmentLightfEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightfEXT =
      (GdkGLProc_glFragmentLightfEXT) gdk_gl_get_proc_address ("glFragmentLightfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightfEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightfEXT);
}

/* glFragmentLightfvEXT */
GdkGLProc
gdk_gl_get_glFragmentLightfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightfvEXT == (GdkGLProc_glFragmentLightfvEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightfvEXT =
      (GdkGLProc_glFragmentLightfvEXT) gdk_gl_get_proc_address ("glFragmentLightfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightfvEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightfvEXT);
}

/* glFragmentLightiEXT */
GdkGLProc
gdk_gl_get_glFragmentLightiEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightiEXT == (GdkGLProc_glFragmentLightiEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightiEXT =
      (GdkGLProc_glFragmentLightiEXT) gdk_gl_get_proc_address ("glFragmentLightiEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightiEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightiEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightiEXT);
}

/* glFragmentLightivEXT */
GdkGLProc
gdk_gl_get_glFragmentLightivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentLightivEXT == (GdkGLProc_glFragmentLightivEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentLightivEXT =
      (GdkGLProc_glFragmentLightivEXT) gdk_gl_get_proc_address ("glFragmentLightivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentLightivEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentLightivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentLightivEXT);
}

/* glGetFragmentLightfvEXT */
GdkGLProc
gdk_gl_get_glGetFragmentLightfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glGetFragmentLightfvEXT == (GdkGLProc_glGetFragmentLightfvEXT) -1)
    _procs_GL_EXT_fragment_lighting.glGetFragmentLightfvEXT =
      (GdkGLProc_glGetFragmentLightfvEXT) gdk_gl_get_proc_address ("glGetFragmentLightfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentLightfvEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glGetFragmentLightfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glGetFragmentLightfvEXT);
}

/* glGetFragmentLightivEXT */
GdkGLProc
gdk_gl_get_glGetFragmentLightivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glGetFragmentLightivEXT == (GdkGLProc_glGetFragmentLightivEXT) -1)
    _procs_GL_EXT_fragment_lighting.glGetFragmentLightivEXT =
      (GdkGLProc_glGetFragmentLightivEXT) gdk_gl_get_proc_address ("glGetFragmentLightivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentLightivEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glGetFragmentLightivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glGetFragmentLightivEXT);
}

/* glFragmentMaterialfEXT */
GdkGLProc
gdk_gl_get_glFragmentMaterialfEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentMaterialfEXT == (GdkGLProc_glFragmentMaterialfEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentMaterialfEXT =
      (GdkGLProc_glFragmentMaterialfEXT) gdk_gl_get_proc_address ("glFragmentMaterialfEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialfEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentMaterialfEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentMaterialfEXT);
}

/* glFragmentMaterialfvEXT */
GdkGLProc
gdk_gl_get_glFragmentMaterialfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentMaterialfvEXT == (GdkGLProc_glFragmentMaterialfvEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentMaterialfvEXT =
      (GdkGLProc_glFragmentMaterialfvEXT) gdk_gl_get_proc_address ("glFragmentMaterialfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialfvEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentMaterialfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentMaterialfvEXT);
}

/* glFragmentMaterialiEXT */
GdkGLProc
gdk_gl_get_glFragmentMaterialiEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentMaterialiEXT == (GdkGLProc_glFragmentMaterialiEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentMaterialiEXT =
      (GdkGLProc_glFragmentMaterialiEXT) gdk_gl_get_proc_address ("glFragmentMaterialiEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialiEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentMaterialiEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentMaterialiEXT);
}

/* glFragmentMaterialivEXT */
GdkGLProc
gdk_gl_get_glFragmentMaterialivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentMaterialivEXT == (GdkGLProc_glFragmentMaterialivEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentMaterialivEXT =
      (GdkGLProc_glFragmentMaterialivEXT) gdk_gl_get_proc_address ("glFragmentMaterialivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentMaterialivEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentMaterialivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentMaterialivEXT);
}

/* glFragmentColorMaterialEXT */
GdkGLProc
gdk_gl_get_glFragmentColorMaterialEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glFragmentColorMaterialEXT == (GdkGLProc_glFragmentColorMaterialEXT) -1)
    _procs_GL_EXT_fragment_lighting.glFragmentColorMaterialEXT =
      (GdkGLProc_glFragmentColorMaterialEXT) gdk_gl_get_proc_address ("glFragmentColorMaterialEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFragmentColorMaterialEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glFragmentColorMaterialEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glFragmentColorMaterialEXT);
}

/* glGetFragmentMaterialfvEXT */
GdkGLProc
gdk_gl_get_glGetFragmentMaterialfvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glGetFragmentMaterialfvEXT == (GdkGLProc_glGetFragmentMaterialfvEXT) -1)
    _procs_GL_EXT_fragment_lighting.glGetFragmentMaterialfvEXT =
      (GdkGLProc_glGetFragmentMaterialfvEXT) gdk_gl_get_proc_address ("glGetFragmentMaterialfvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentMaterialfvEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glGetFragmentMaterialfvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glGetFragmentMaterialfvEXT);
}

/* glGetFragmentMaterialivEXT */
GdkGLProc
gdk_gl_get_glGetFragmentMaterialivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glGetFragmentMaterialivEXT == (GdkGLProc_glGetFragmentMaterialivEXT) -1)
    _procs_GL_EXT_fragment_lighting.glGetFragmentMaterialivEXT =
      (GdkGLProc_glGetFragmentMaterialivEXT) gdk_gl_get_proc_address ("glGetFragmentMaterialivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glGetFragmentMaterialivEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glGetFragmentMaterialivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glGetFragmentMaterialivEXT);
}

/* glLightEnviEXT */
GdkGLProc
gdk_gl_get_glLightEnviEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_fragment_lighting.glLightEnviEXT == (GdkGLProc_glLightEnviEXT) -1)
    _procs_GL_EXT_fragment_lighting.glLightEnviEXT =
      (GdkGLProc_glLightEnviEXT) gdk_gl_get_proc_address ("glLightEnviEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glLightEnviEXT () - %s",
               (_procs_GL_EXT_fragment_lighting.glLightEnviEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_fragment_lighting.glLightEnviEXT);
}

/* Get GL_EXT_fragment_lighting functions */
GdkGL_GL_EXT_fragment_lighting *
gdk_gl_get_GL_EXT_fragment_lighting (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_fragment_lighting");

      if (supported)
        {
          supported &= (gdk_gl_get_glFragmentLightModelfEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModelfvEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModeliEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightModelivEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightfEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightfvEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightiEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentLightivEXT () != NULL);
          supported &= (gdk_gl_get_glGetFragmentLightfvEXT () != NULL);
          supported &= (gdk_gl_get_glGetFragmentLightivEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialfEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialfvEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialiEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentMaterialivEXT () != NULL);
          supported &= (gdk_gl_get_glFragmentColorMaterialEXT () != NULL);
          supported &= (gdk_gl_get_glGetFragmentMaterialfvEXT () != NULL);
          supported &= (gdk_gl_get_glGetFragmentMaterialivEXT () != NULL);
          supported &= (gdk_gl_get_glLightEnviEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_fragment_lighting () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_fragment_lighting;
}

/*
 * GL_EXT_multitexture
 */

static GdkGL_GL_EXT_multitexture _procs_GL_EXT_multitexture = {
  (GdkGLProc_glMultiTexCoord1dEXT) -1,
  (GdkGLProc_glMultiTexCoord1dvEXT) -1,
  (GdkGLProc_glMultiTexCoord1fEXT) -1,
  (GdkGLProc_glMultiTexCoord1fvEXT) -1,
  (GdkGLProc_glMultiTexCoord1iEXT) -1,
  (GdkGLProc_glMultiTexCoord1ivEXT) -1,
  (GdkGLProc_glMultiTexCoord1sEXT) -1,
  (GdkGLProc_glMultiTexCoord1svEXT) -1,
  (GdkGLProc_glMultiTexCoord2dEXT) -1,
  (GdkGLProc_glMultiTexCoord2dvEXT) -1,
  (GdkGLProc_glMultiTexCoord2fEXT) -1,
  (GdkGLProc_glMultiTexCoord2fvEXT) -1,
  (GdkGLProc_glMultiTexCoord2iEXT) -1,
  (GdkGLProc_glMultiTexCoord2ivEXT) -1,
  (GdkGLProc_glMultiTexCoord2sEXT) -1,
  (GdkGLProc_glMultiTexCoord2svEXT) -1,
  (GdkGLProc_glMultiTexCoord3dEXT) -1,
  (GdkGLProc_glMultiTexCoord3dvEXT) -1,
  (GdkGLProc_glMultiTexCoord3fEXT) -1,
  (GdkGLProc_glMultiTexCoord3fvEXT) -1,
  (GdkGLProc_glMultiTexCoord3iEXT) -1,
  (GdkGLProc_glMultiTexCoord3ivEXT) -1,
  (GdkGLProc_glMultiTexCoord3sEXT) -1,
  (GdkGLProc_glMultiTexCoord3svEXT) -1,
  (GdkGLProc_glMultiTexCoord4dEXT) -1,
  (GdkGLProc_glMultiTexCoord4dvEXT) -1,
  (GdkGLProc_glMultiTexCoord4fEXT) -1,
  (GdkGLProc_glMultiTexCoord4fvEXT) -1,
  (GdkGLProc_glMultiTexCoord4iEXT) -1,
  (GdkGLProc_glMultiTexCoord4ivEXT) -1,
  (GdkGLProc_glMultiTexCoord4sEXT) -1,
  (GdkGLProc_glMultiTexCoord4svEXT) -1,
  (GdkGLProc_glInterleavedTextureCoordSetsEXT) -1,
  (GdkGLProc_glSelectTextureEXT) -1,
  (GdkGLProc_glSelectTextureCoordSetEXT) -1,
  (GdkGLProc_glSelectTextureTransformEXT) -1
};

/* glMultiTexCoord1dEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1dEXT == (GdkGLProc_glMultiTexCoord1dEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1dEXT =
      (GdkGLProc_glMultiTexCoord1dEXT) gdk_gl_get_proc_address ("glMultiTexCoord1dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1dEXT);
}

/* glMultiTexCoord1dvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1dvEXT == (GdkGLProc_glMultiTexCoord1dvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1dvEXT =
      (GdkGLProc_glMultiTexCoord1dvEXT) gdk_gl_get_proc_address ("glMultiTexCoord1dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1dvEXT);
}

/* glMultiTexCoord1fEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1fEXT == (GdkGLProc_glMultiTexCoord1fEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1fEXT =
      (GdkGLProc_glMultiTexCoord1fEXT) gdk_gl_get_proc_address ("glMultiTexCoord1fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1fEXT);
}

/* glMultiTexCoord1fvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1fvEXT == (GdkGLProc_glMultiTexCoord1fvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1fvEXT =
      (GdkGLProc_glMultiTexCoord1fvEXT) gdk_gl_get_proc_address ("glMultiTexCoord1fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1fvEXT);
}

/* glMultiTexCoord1iEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1iEXT == (GdkGLProc_glMultiTexCoord1iEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1iEXT =
      (GdkGLProc_glMultiTexCoord1iEXT) gdk_gl_get_proc_address ("glMultiTexCoord1iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1iEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1iEXT);
}

/* glMultiTexCoord1ivEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1ivEXT == (GdkGLProc_glMultiTexCoord1ivEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1ivEXT =
      (GdkGLProc_glMultiTexCoord1ivEXT) gdk_gl_get_proc_address ("glMultiTexCoord1ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1ivEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1ivEXT);
}

/* glMultiTexCoord1sEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1sEXT == (GdkGLProc_glMultiTexCoord1sEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1sEXT =
      (GdkGLProc_glMultiTexCoord1sEXT) gdk_gl_get_proc_address ("glMultiTexCoord1sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1sEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1sEXT);
}

/* glMultiTexCoord1svEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord1svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord1svEXT == (GdkGLProc_glMultiTexCoord1svEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord1svEXT =
      (GdkGLProc_glMultiTexCoord1svEXT) gdk_gl_get_proc_address ("glMultiTexCoord1svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1svEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord1svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord1svEXT);
}

/* glMultiTexCoord2dEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2dEXT == (GdkGLProc_glMultiTexCoord2dEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2dEXT =
      (GdkGLProc_glMultiTexCoord2dEXT) gdk_gl_get_proc_address ("glMultiTexCoord2dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2dEXT);
}

/* glMultiTexCoord2dvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2dvEXT == (GdkGLProc_glMultiTexCoord2dvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2dvEXT =
      (GdkGLProc_glMultiTexCoord2dvEXT) gdk_gl_get_proc_address ("glMultiTexCoord2dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2dvEXT);
}

/* glMultiTexCoord2fEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2fEXT == (GdkGLProc_glMultiTexCoord2fEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2fEXT =
      (GdkGLProc_glMultiTexCoord2fEXT) gdk_gl_get_proc_address ("glMultiTexCoord2fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2fEXT);
}

/* glMultiTexCoord2fvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2fvEXT == (GdkGLProc_glMultiTexCoord2fvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2fvEXT =
      (GdkGLProc_glMultiTexCoord2fvEXT) gdk_gl_get_proc_address ("glMultiTexCoord2fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2fvEXT);
}

/* glMultiTexCoord2iEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2iEXT == (GdkGLProc_glMultiTexCoord2iEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2iEXT =
      (GdkGLProc_glMultiTexCoord2iEXT) gdk_gl_get_proc_address ("glMultiTexCoord2iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2iEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2iEXT);
}

/* glMultiTexCoord2ivEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2ivEXT == (GdkGLProc_glMultiTexCoord2ivEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2ivEXT =
      (GdkGLProc_glMultiTexCoord2ivEXT) gdk_gl_get_proc_address ("glMultiTexCoord2ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2ivEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2ivEXT);
}

/* glMultiTexCoord2sEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2sEXT == (GdkGLProc_glMultiTexCoord2sEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2sEXT =
      (GdkGLProc_glMultiTexCoord2sEXT) gdk_gl_get_proc_address ("glMultiTexCoord2sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2sEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2sEXT);
}

/* glMultiTexCoord2svEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord2svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord2svEXT == (GdkGLProc_glMultiTexCoord2svEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord2svEXT =
      (GdkGLProc_glMultiTexCoord2svEXT) gdk_gl_get_proc_address ("glMultiTexCoord2svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2svEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord2svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord2svEXT);
}

/* glMultiTexCoord3dEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3dEXT == (GdkGLProc_glMultiTexCoord3dEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3dEXT =
      (GdkGLProc_glMultiTexCoord3dEXT) gdk_gl_get_proc_address ("glMultiTexCoord3dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3dEXT);
}

/* glMultiTexCoord3dvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3dvEXT == (GdkGLProc_glMultiTexCoord3dvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3dvEXT =
      (GdkGLProc_glMultiTexCoord3dvEXT) gdk_gl_get_proc_address ("glMultiTexCoord3dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3dvEXT);
}

/* glMultiTexCoord3fEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3fEXT == (GdkGLProc_glMultiTexCoord3fEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3fEXT =
      (GdkGLProc_glMultiTexCoord3fEXT) gdk_gl_get_proc_address ("glMultiTexCoord3fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3fEXT);
}

/* glMultiTexCoord3fvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3fvEXT == (GdkGLProc_glMultiTexCoord3fvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3fvEXT =
      (GdkGLProc_glMultiTexCoord3fvEXT) gdk_gl_get_proc_address ("glMultiTexCoord3fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3fvEXT);
}

/* glMultiTexCoord3iEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3iEXT == (GdkGLProc_glMultiTexCoord3iEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3iEXT =
      (GdkGLProc_glMultiTexCoord3iEXT) gdk_gl_get_proc_address ("glMultiTexCoord3iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3iEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3iEXT);
}

/* glMultiTexCoord3ivEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3ivEXT == (GdkGLProc_glMultiTexCoord3ivEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3ivEXT =
      (GdkGLProc_glMultiTexCoord3ivEXT) gdk_gl_get_proc_address ("glMultiTexCoord3ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3ivEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3ivEXT);
}

/* glMultiTexCoord3sEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3sEXT == (GdkGLProc_glMultiTexCoord3sEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3sEXT =
      (GdkGLProc_glMultiTexCoord3sEXT) gdk_gl_get_proc_address ("glMultiTexCoord3sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3sEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3sEXT);
}

/* glMultiTexCoord3svEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord3svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord3svEXT == (GdkGLProc_glMultiTexCoord3svEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord3svEXT =
      (GdkGLProc_glMultiTexCoord3svEXT) gdk_gl_get_proc_address ("glMultiTexCoord3svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3svEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord3svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord3svEXT);
}

/* glMultiTexCoord4dEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4dEXT == (GdkGLProc_glMultiTexCoord4dEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4dEXT =
      (GdkGLProc_glMultiTexCoord4dEXT) gdk_gl_get_proc_address ("glMultiTexCoord4dEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4dEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4dEXT);
}

/* glMultiTexCoord4dvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4dvEXT == (GdkGLProc_glMultiTexCoord4dvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4dvEXT =
      (GdkGLProc_glMultiTexCoord4dvEXT) gdk_gl_get_proc_address ("glMultiTexCoord4dvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4dvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4dvEXT);
}

/* glMultiTexCoord4fEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4fEXT == (GdkGLProc_glMultiTexCoord4fEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4fEXT =
      (GdkGLProc_glMultiTexCoord4fEXT) gdk_gl_get_proc_address ("glMultiTexCoord4fEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4fEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4fEXT);
}

/* glMultiTexCoord4fvEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fvEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4fvEXT == (GdkGLProc_glMultiTexCoord4fvEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4fvEXT =
      (GdkGLProc_glMultiTexCoord4fvEXT) gdk_gl_get_proc_address ("glMultiTexCoord4fvEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fvEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4fvEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4fvEXT);
}

/* glMultiTexCoord4iEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4iEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4iEXT == (GdkGLProc_glMultiTexCoord4iEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4iEXT =
      (GdkGLProc_glMultiTexCoord4iEXT) gdk_gl_get_proc_address ("glMultiTexCoord4iEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4iEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4iEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4iEXT);
}

/* glMultiTexCoord4ivEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4ivEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4ivEXT == (GdkGLProc_glMultiTexCoord4ivEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4ivEXT =
      (GdkGLProc_glMultiTexCoord4ivEXT) gdk_gl_get_proc_address ("glMultiTexCoord4ivEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4ivEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4ivEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4ivEXT);
}

/* glMultiTexCoord4sEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4sEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4sEXT == (GdkGLProc_glMultiTexCoord4sEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4sEXT =
      (GdkGLProc_glMultiTexCoord4sEXT) gdk_gl_get_proc_address ("glMultiTexCoord4sEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4sEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4sEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4sEXT);
}

/* glMultiTexCoord4svEXT */
GdkGLProc
gdk_gl_get_glMultiTexCoord4svEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glMultiTexCoord4svEXT == (GdkGLProc_glMultiTexCoord4svEXT) -1)
    _procs_GL_EXT_multitexture.glMultiTexCoord4svEXT =
      (GdkGLProc_glMultiTexCoord4svEXT) gdk_gl_get_proc_address ("glMultiTexCoord4svEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4svEXT () - %s",
               (_procs_GL_EXT_multitexture.glMultiTexCoord4svEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glMultiTexCoord4svEXT);
}

/* glInterleavedTextureCoordSetsEXT */
GdkGLProc
gdk_gl_get_glInterleavedTextureCoordSetsEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glInterleavedTextureCoordSetsEXT == (GdkGLProc_glInterleavedTextureCoordSetsEXT) -1)
    _procs_GL_EXT_multitexture.glInterleavedTextureCoordSetsEXT =
      (GdkGLProc_glInterleavedTextureCoordSetsEXT) gdk_gl_get_proc_address ("glInterleavedTextureCoordSetsEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glInterleavedTextureCoordSetsEXT () - %s",
               (_procs_GL_EXT_multitexture.glInterleavedTextureCoordSetsEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glInterleavedTextureCoordSetsEXT);
}

/* glSelectTextureEXT */
GdkGLProc
gdk_gl_get_glSelectTextureEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glSelectTextureEXT == (GdkGLProc_glSelectTextureEXT) -1)
    _procs_GL_EXT_multitexture.glSelectTextureEXT =
      (GdkGLProc_glSelectTextureEXT) gdk_gl_get_proc_address ("glSelectTextureEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSelectTextureEXT () - %s",
               (_procs_GL_EXT_multitexture.glSelectTextureEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glSelectTextureEXT);
}

/* glSelectTextureCoordSetEXT */
GdkGLProc
gdk_gl_get_glSelectTextureCoordSetEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glSelectTextureCoordSetEXT == (GdkGLProc_glSelectTextureCoordSetEXT) -1)
    _procs_GL_EXT_multitexture.glSelectTextureCoordSetEXT =
      (GdkGLProc_glSelectTextureCoordSetEXT) gdk_gl_get_proc_address ("glSelectTextureCoordSetEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSelectTextureCoordSetEXT () - %s",
               (_procs_GL_EXT_multitexture.glSelectTextureCoordSetEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glSelectTextureCoordSetEXT);
}

/* glSelectTextureTransformEXT */
GdkGLProc
gdk_gl_get_glSelectTextureTransformEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_multitexture.glSelectTextureTransformEXT == (GdkGLProc_glSelectTextureTransformEXT) -1)
    _procs_GL_EXT_multitexture.glSelectTextureTransformEXT =
      (GdkGLProc_glSelectTextureTransformEXT) gdk_gl_get_proc_address ("glSelectTextureTransformEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSelectTextureTransformEXT () - %s",
               (_procs_GL_EXT_multitexture.glSelectTextureTransformEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_multitexture.glSelectTextureTransformEXT);
}

/* Get GL_EXT_multitexture functions */
GdkGL_GL_EXT_multitexture *
gdk_gl_get_GL_EXT_multitexture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_multitexture");

      if (supported)
        {
          supported &= (gdk_gl_get_glMultiTexCoord1dEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1dvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1fEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1fvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1iEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1ivEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1sEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1svEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2dEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2dvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2fEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2fvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2iEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2ivEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2sEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2svEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3dEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3dvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3fEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3fvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3iEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3ivEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3sEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3svEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4dEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4dvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4fEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4fvEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4iEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4ivEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4sEXT () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4svEXT () != NULL);
          supported &= (gdk_gl_get_glInterleavedTextureCoordSetsEXT () != NULL);
          supported &= (gdk_gl_get_glSelectTextureEXT () != NULL);
          supported &= (gdk_gl_get_glSelectTextureCoordSetEXT () != NULL);
          supported &= (gdk_gl_get_glSelectTextureTransformEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_multitexture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_multitexture;
}

/*
 * GL_EXT_scene_marker
 */

static GdkGL_GL_EXT_scene_marker _procs_GL_EXT_scene_marker = {
  (GdkGLProc_glBeginSceneEXT) -1,
  (GdkGLProc_glEndSceneEXT) -1
};

/* glBeginSceneEXT */
GdkGLProc
gdk_gl_get_glBeginSceneEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_scene_marker.glBeginSceneEXT == (GdkGLProc_glBeginSceneEXT) -1)
    _procs_GL_EXT_scene_marker.glBeginSceneEXT =
      (GdkGLProc_glBeginSceneEXT) gdk_gl_get_proc_address ("glBeginSceneEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBeginSceneEXT () - %s",
               (_procs_GL_EXT_scene_marker.glBeginSceneEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_scene_marker.glBeginSceneEXT);
}

/* glEndSceneEXT */
GdkGLProc
gdk_gl_get_glEndSceneEXT (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_EXT_scene_marker.glEndSceneEXT == (GdkGLProc_glEndSceneEXT) -1)
    _procs_GL_EXT_scene_marker.glEndSceneEXT =
      (GdkGLProc_glEndSceneEXT) gdk_gl_get_proc_address ("glEndSceneEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glEndSceneEXT () - %s",
               (_procs_GL_EXT_scene_marker.glEndSceneEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_EXT_scene_marker.glEndSceneEXT);
}

/* Get GL_EXT_scene_marker functions */
GdkGL_GL_EXT_scene_marker *
gdk_gl_get_GL_EXT_scene_marker (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_EXT_scene_marker");

      if (supported)
        {
          supported &= (gdk_gl_get_glBeginSceneEXT () != NULL);
          supported &= (gdk_gl_get_glEndSceneEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_EXT_scene_marker () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_EXT_scene_marker;
}

/*
 * GL_IBM_static_data
 */

static GdkGL_GL_IBM_static_data _procs_GL_IBM_static_data = {
  (GdkGLProc_glFlushStaticDataIBM) -1
};

/* glFlushStaticDataIBM */
GdkGLProc
gdk_gl_get_glFlushStaticDataIBM (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_IBM_static_data.glFlushStaticDataIBM == (GdkGLProc_glFlushStaticDataIBM) -1)
    _procs_GL_IBM_static_data.glFlushStaticDataIBM =
      (GdkGLProc_glFlushStaticDataIBM) gdk_gl_get_proc_address ("glFlushStaticDataIBM");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glFlushStaticDataIBM () - %s",
               (_procs_GL_IBM_static_data.glFlushStaticDataIBM) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_IBM_static_data.glFlushStaticDataIBM);
}

/* Get GL_IBM_static_data functions */
GdkGL_GL_IBM_static_data *
gdk_gl_get_GL_IBM_static_data (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_IBM_static_data");

      if (supported)
        {
          supported &= (gdk_gl_get_glFlushStaticDataIBM () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_IBM_static_data () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_IBM_static_data;
}

/*
 * GL_KTX_buffer_region
 */

static GdkGL_GL_KTX_buffer_region _procs_GL_KTX_buffer_region = {
  (GdkGLProc_glBufferRegionEnabled) -1,
  (GdkGLProc_glNewBufferRegion) -1,
  (GdkGLProc_glDeleteBufferRegion) -1,
  (GdkGLProc_glReadBufferRegion) -1,
  (GdkGLProc_glDrawBufferRegion) -1
};

/* glBufferRegionEnabled */
GdkGLProc
gdk_gl_get_glBufferRegionEnabled (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_KTX_buffer_region.glBufferRegionEnabled == (GdkGLProc_glBufferRegionEnabled) -1)
    _procs_GL_KTX_buffer_region.glBufferRegionEnabled =
      (GdkGLProc_glBufferRegionEnabled) gdk_gl_get_proc_address ("glBufferRegionEnabled");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glBufferRegionEnabled () - %s",
               (_procs_GL_KTX_buffer_region.glBufferRegionEnabled) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_KTX_buffer_region.glBufferRegionEnabled);
}

/* glNewBufferRegion */
GdkGLProc
gdk_gl_get_glNewBufferRegion (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_KTX_buffer_region.glNewBufferRegion == (GdkGLProc_glNewBufferRegion) -1)
    _procs_GL_KTX_buffer_region.glNewBufferRegion =
      (GdkGLProc_glNewBufferRegion) gdk_gl_get_proc_address ("glNewBufferRegion");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glNewBufferRegion () - %s",
               (_procs_GL_KTX_buffer_region.glNewBufferRegion) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_KTX_buffer_region.glNewBufferRegion);
}

/* glDeleteBufferRegion */
GdkGLProc
gdk_gl_get_glDeleteBufferRegion (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_KTX_buffer_region.glDeleteBufferRegion == (GdkGLProc_glDeleteBufferRegion) -1)
    _procs_GL_KTX_buffer_region.glDeleteBufferRegion =
      (GdkGLProc_glDeleteBufferRegion) gdk_gl_get_proc_address ("glDeleteBufferRegion");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDeleteBufferRegion () - %s",
               (_procs_GL_KTX_buffer_region.glDeleteBufferRegion) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_KTX_buffer_region.glDeleteBufferRegion);
}

/* glReadBufferRegion */
GdkGLProc
gdk_gl_get_glReadBufferRegion (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_KTX_buffer_region.glReadBufferRegion == (GdkGLProc_glReadBufferRegion) -1)
    _procs_GL_KTX_buffer_region.glReadBufferRegion =
      (GdkGLProc_glReadBufferRegion) gdk_gl_get_proc_address ("glReadBufferRegion");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glReadBufferRegion () - %s",
               (_procs_GL_KTX_buffer_region.glReadBufferRegion) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_KTX_buffer_region.glReadBufferRegion);
}

/* glDrawBufferRegion */
GdkGLProc
gdk_gl_get_glDrawBufferRegion (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_KTX_buffer_region.glDrawBufferRegion == (GdkGLProc_glDrawBufferRegion) -1)
    _procs_GL_KTX_buffer_region.glDrawBufferRegion =
      (GdkGLProc_glDrawBufferRegion) gdk_gl_get_proc_address ("glDrawBufferRegion");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawBufferRegion () - %s",
               (_procs_GL_KTX_buffer_region.glDrawBufferRegion) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_KTX_buffer_region.glDrawBufferRegion);
}

/* Get GL_KTX_buffer_region functions */
GdkGL_GL_KTX_buffer_region *
gdk_gl_get_GL_KTX_buffer_region (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_KTX_buffer_region");

      if (supported)
        {
          supported &= (gdk_gl_get_glBufferRegionEnabled () != NULL);
          supported &= (gdk_gl_get_glNewBufferRegion () != NULL);
          supported &= (gdk_gl_get_glDeleteBufferRegion () != NULL);
          supported &= (gdk_gl_get_glReadBufferRegion () != NULL);
          supported &= (gdk_gl_get_glDrawBufferRegion () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_KTX_buffer_region () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_KTX_buffer_region;
}

/*
 * GL_NV_element_array
 */

static GdkGL_GL_NV_element_array _procs_GL_NV_element_array = {
  (GdkGLProc_glElementPointerNV) -1,
  (GdkGLProc_glDrawElementArrayNV) -1,
  (GdkGLProc_glDrawRangeElementArrayNV) -1,
  (GdkGLProc_glMultiDrawElementArrayNV) -1,
  (GdkGLProc_glMultiDrawRangeElementArrayNV) -1
};

/* glElementPointerNV */
GdkGLProc
gdk_gl_get_glElementPointerNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_element_array.glElementPointerNV == (GdkGLProc_glElementPointerNV) -1)
    _procs_GL_NV_element_array.glElementPointerNV =
      (GdkGLProc_glElementPointerNV) gdk_gl_get_proc_address ("glElementPointerNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glElementPointerNV () - %s",
               (_procs_GL_NV_element_array.glElementPointerNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_element_array.glElementPointerNV);
}

/* glDrawElementArrayNV */
GdkGLProc
gdk_gl_get_glDrawElementArrayNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_element_array.glDrawElementArrayNV == (GdkGLProc_glDrawElementArrayNV) -1)
    _procs_GL_NV_element_array.glDrawElementArrayNV =
      (GdkGLProc_glDrawElementArrayNV) gdk_gl_get_proc_address ("glDrawElementArrayNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawElementArrayNV () - %s",
               (_procs_GL_NV_element_array.glDrawElementArrayNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_element_array.glDrawElementArrayNV);
}

/* glDrawRangeElementArrayNV */
GdkGLProc
gdk_gl_get_glDrawRangeElementArrayNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_element_array.glDrawRangeElementArrayNV == (GdkGLProc_glDrawRangeElementArrayNV) -1)
    _procs_GL_NV_element_array.glDrawRangeElementArrayNV =
      (GdkGLProc_glDrawRangeElementArrayNV) gdk_gl_get_proc_address ("glDrawRangeElementArrayNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glDrawRangeElementArrayNV () - %s",
               (_procs_GL_NV_element_array.glDrawRangeElementArrayNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_element_array.glDrawRangeElementArrayNV);
}

/* glMultiDrawElementArrayNV */
GdkGLProc
gdk_gl_get_glMultiDrawElementArrayNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_element_array.glMultiDrawElementArrayNV == (GdkGLProc_glMultiDrawElementArrayNV) -1)
    _procs_GL_NV_element_array.glMultiDrawElementArrayNV =
      (GdkGLProc_glMultiDrawElementArrayNV) gdk_gl_get_proc_address ("glMultiDrawElementArrayNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawElementArrayNV () - %s",
               (_procs_GL_NV_element_array.glMultiDrawElementArrayNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_element_array.glMultiDrawElementArrayNV);
}

/* glMultiDrawRangeElementArrayNV */
GdkGLProc
gdk_gl_get_glMultiDrawRangeElementArrayNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_element_array.glMultiDrawRangeElementArrayNV == (GdkGLProc_glMultiDrawRangeElementArrayNV) -1)
    _procs_GL_NV_element_array.glMultiDrawRangeElementArrayNV =
      (GdkGLProc_glMultiDrawRangeElementArrayNV) gdk_gl_get_proc_address ("glMultiDrawRangeElementArrayNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawRangeElementArrayNV () - %s",
               (_procs_GL_NV_element_array.glMultiDrawRangeElementArrayNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_element_array.glMultiDrawRangeElementArrayNV);
}

/* Get GL_NV_element_array functions */
GdkGL_GL_NV_element_array *
gdk_gl_get_GL_NV_element_array (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_element_array");

      if (supported)
        {
          supported &= (gdk_gl_get_glElementPointerNV () != NULL);
          supported &= (gdk_gl_get_glDrawElementArrayNV () != NULL);
          supported &= (gdk_gl_get_glDrawRangeElementArrayNV () != NULL);
          supported &= (gdk_gl_get_glMultiDrawElementArrayNV () != NULL);
          supported &= (gdk_gl_get_glMultiDrawRangeElementArrayNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_element_array () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_element_array;
}

/*
 * GL_NV_stencil_two_side
 */

static GdkGL_GL_NV_stencil_two_side _procs_GL_NV_stencil_two_side = {
  (GdkGLProc_glActiveStencilFaceNV) -1
};

/* glActiveStencilFaceNV */
GdkGLProc
gdk_gl_get_glActiveStencilFaceNV (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_NV_stencil_two_side.glActiveStencilFaceNV == (GdkGLProc_glActiveStencilFaceNV) -1)
    _procs_GL_NV_stencil_two_side.glActiveStencilFaceNV =
      (GdkGLProc_glActiveStencilFaceNV) gdk_gl_get_proc_address ("glActiveStencilFaceNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glActiveStencilFaceNV () - %s",
               (_procs_GL_NV_stencil_two_side.glActiveStencilFaceNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_NV_stencil_two_side.glActiveStencilFaceNV);
}

/* Get GL_NV_stencil_two_side functions */
GdkGL_GL_NV_stencil_two_side *
gdk_gl_get_GL_NV_stencil_two_side (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_NV_stencil_two_side");

      if (supported)
        {
          supported &= (gdk_gl_get_glActiveStencilFaceNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_NV_stencil_two_side () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_NV_stencil_two_side;
}

/*
 * GL_SGIS_multitexture
 */

static GdkGL_GL_SGIS_multitexture _procs_GL_SGIS_multitexture = {
  (GdkGLProc_glMultiTexCoord1dSGIS) -1,
  (GdkGLProc_glMultiTexCoord1dvSGIS) -1,
  (GdkGLProc_glMultiTexCoord1fSGIS) -1,
  (GdkGLProc_glMultiTexCoord1fvSGIS) -1,
  (GdkGLProc_glMultiTexCoord1iSGIS) -1,
  (GdkGLProc_glMultiTexCoord1ivSGIS) -1,
  (GdkGLProc_glMultiTexCoord1sSGIS) -1,
  (GdkGLProc_glMultiTexCoord1svSGIS) -1,
  (GdkGLProc_glMultiTexCoord2dSGIS) -1,
  (GdkGLProc_glMultiTexCoord2dvSGIS) -1,
  (GdkGLProc_glMultiTexCoord2fSGIS) -1,
  (GdkGLProc_glMultiTexCoord2fvSGIS) -1,
  (GdkGLProc_glMultiTexCoord2iSGIS) -1,
  (GdkGLProc_glMultiTexCoord2ivSGIS) -1,
  (GdkGLProc_glMultiTexCoord2sSGIS) -1,
  (GdkGLProc_glMultiTexCoord2svSGIS) -1,
  (GdkGLProc_glMultiTexCoord3dSGIS) -1,
  (GdkGLProc_glMultiTexCoord3dvSGIS) -1,
  (GdkGLProc_glMultiTexCoord3fSGIS) -1,
  (GdkGLProc_glMultiTexCoord3fvSGIS) -1,
  (GdkGLProc_glMultiTexCoord3iSGIS) -1,
  (GdkGLProc_glMultiTexCoord3ivSGIS) -1,
  (GdkGLProc_glMultiTexCoord3sSGIS) -1,
  (GdkGLProc_glMultiTexCoord3svSGIS) -1,
  (GdkGLProc_glMultiTexCoord4dSGIS) -1,
  (GdkGLProc_glMultiTexCoord4dvSGIS) -1,
  (GdkGLProc_glMultiTexCoord4fSGIS) -1,
  (GdkGLProc_glMultiTexCoord4fvSGIS) -1,
  (GdkGLProc_glMultiTexCoord4iSGIS) -1,
  (GdkGLProc_glMultiTexCoord4ivSGIS) -1,
  (GdkGLProc_glMultiTexCoord4sSGIS) -1,
  (GdkGLProc_glMultiTexCoord4svSGIS) -1,
  (GdkGLProc_glMultiTexCoordPointerSGIS) -1,
  (GdkGLProc_glSelectTextureSGIS) -1,
  (GdkGLProc_glSelectTextureCoordSetSGIS) -1
};

/* glMultiTexCoord1dSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1dSGIS == (GdkGLProc_glMultiTexCoord1dSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1dSGIS =
      (GdkGLProc_glMultiTexCoord1dSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1dSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1dSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1dSGIS);
}

/* glMultiTexCoord1dvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1dvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1dvSGIS == (GdkGLProc_glMultiTexCoord1dvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1dvSGIS =
      (GdkGLProc_glMultiTexCoord1dvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1dvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1dvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1dvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1dvSGIS);
}

/* glMultiTexCoord1fSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1fSGIS == (GdkGLProc_glMultiTexCoord1fSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1fSGIS =
      (GdkGLProc_glMultiTexCoord1fSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1fSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1fSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1fSGIS);
}

/* glMultiTexCoord1fvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1fvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1fvSGIS == (GdkGLProc_glMultiTexCoord1fvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1fvSGIS =
      (GdkGLProc_glMultiTexCoord1fvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1fvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1fvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1fvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1fvSGIS);
}

/* glMultiTexCoord1iSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1iSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1iSGIS == (GdkGLProc_glMultiTexCoord1iSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1iSGIS =
      (GdkGLProc_glMultiTexCoord1iSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1iSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1iSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1iSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1iSGIS);
}

/* glMultiTexCoord1ivSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1ivSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1ivSGIS == (GdkGLProc_glMultiTexCoord1ivSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1ivSGIS =
      (GdkGLProc_glMultiTexCoord1ivSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1ivSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1ivSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1ivSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1ivSGIS);
}

/* glMultiTexCoord1sSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1sSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1sSGIS == (GdkGLProc_glMultiTexCoord1sSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1sSGIS =
      (GdkGLProc_glMultiTexCoord1sSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1sSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1sSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1sSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1sSGIS);
}

/* glMultiTexCoord1svSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord1svSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord1svSGIS == (GdkGLProc_glMultiTexCoord1svSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord1svSGIS =
      (GdkGLProc_glMultiTexCoord1svSGIS) gdk_gl_get_proc_address ("glMultiTexCoord1svSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord1svSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord1svSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord1svSGIS);
}

/* glMultiTexCoord2dSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2dSGIS == (GdkGLProc_glMultiTexCoord2dSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2dSGIS =
      (GdkGLProc_glMultiTexCoord2dSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2dSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2dSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2dSGIS);
}

/* glMultiTexCoord2dvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2dvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2dvSGIS == (GdkGLProc_glMultiTexCoord2dvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2dvSGIS =
      (GdkGLProc_glMultiTexCoord2dvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2dvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2dvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2dvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2dvSGIS);
}

/* glMultiTexCoord2fSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2fSGIS == (GdkGLProc_glMultiTexCoord2fSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2fSGIS =
      (GdkGLProc_glMultiTexCoord2fSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2fSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2fSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2fSGIS);
}

/* glMultiTexCoord2fvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2fvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2fvSGIS == (GdkGLProc_glMultiTexCoord2fvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2fvSGIS =
      (GdkGLProc_glMultiTexCoord2fvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2fvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2fvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2fvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2fvSGIS);
}

/* glMultiTexCoord2iSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2iSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2iSGIS == (GdkGLProc_glMultiTexCoord2iSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2iSGIS =
      (GdkGLProc_glMultiTexCoord2iSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2iSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2iSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2iSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2iSGIS);
}

/* glMultiTexCoord2ivSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2ivSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2ivSGIS == (GdkGLProc_glMultiTexCoord2ivSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2ivSGIS =
      (GdkGLProc_glMultiTexCoord2ivSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2ivSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2ivSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2ivSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2ivSGIS);
}

/* glMultiTexCoord2sSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2sSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2sSGIS == (GdkGLProc_glMultiTexCoord2sSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2sSGIS =
      (GdkGLProc_glMultiTexCoord2sSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2sSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2sSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2sSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2sSGIS);
}

/* glMultiTexCoord2svSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord2svSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord2svSGIS == (GdkGLProc_glMultiTexCoord2svSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord2svSGIS =
      (GdkGLProc_glMultiTexCoord2svSGIS) gdk_gl_get_proc_address ("glMultiTexCoord2svSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord2svSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord2svSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord2svSGIS);
}

/* glMultiTexCoord3dSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3dSGIS == (GdkGLProc_glMultiTexCoord3dSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3dSGIS =
      (GdkGLProc_glMultiTexCoord3dSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3dSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3dSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3dSGIS);
}

/* glMultiTexCoord3dvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3dvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3dvSGIS == (GdkGLProc_glMultiTexCoord3dvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3dvSGIS =
      (GdkGLProc_glMultiTexCoord3dvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3dvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3dvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3dvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3dvSGIS);
}

/* glMultiTexCoord3fSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3fSGIS == (GdkGLProc_glMultiTexCoord3fSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3fSGIS =
      (GdkGLProc_glMultiTexCoord3fSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3fSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3fSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3fSGIS);
}

/* glMultiTexCoord3fvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3fvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3fvSGIS == (GdkGLProc_glMultiTexCoord3fvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3fvSGIS =
      (GdkGLProc_glMultiTexCoord3fvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3fvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3fvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3fvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3fvSGIS);
}

/* glMultiTexCoord3iSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3iSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3iSGIS == (GdkGLProc_glMultiTexCoord3iSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3iSGIS =
      (GdkGLProc_glMultiTexCoord3iSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3iSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3iSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3iSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3iSGIS);
}

/* glMultiTexCoord3ivSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3ivSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3ivSGIS == (GdkGLProc_glMultiTexCoord3ivSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3ivSGIS =
      (GdkGLProc_glMultiTexCoord3ivSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3ivSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3ivSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3ivSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3ivSGIS);
}

/* glMultiTexCoord3sSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3sSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3sSGIS == (GdkGLProc_glMultiTexCoord3sSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3sSGIS =
      (GdkGLProc_glMultiTexCoord3sSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3sSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3sSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3sSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3sSGIS);
}

/* glMultiTexCoord3svSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord3svSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord3svSGIS == (GdkGLProc_glMultiTexCoord3svSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord3svSGIS =
      (GdkGLProc_glMultiTexCoord3svSGIS) gdk_gl_get_proc_address ("glMultiTexCoord3svSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord3svSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord3svSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord3svSGIS);
}

/* glMultiTexCoord4dSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4dSGIS == (GdkGLProc_glMultiTexCoord4dSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4dSGIS =
      (GdkGLProc_glMultiTexCoord4dSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4dSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4dSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4dSGIS);
}

/* glMultiTexCoord4dvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4dvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4dvSGIS == (GdkGLProc_glMultiTexCoord4dvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4dvSGIS =
      (GdkGLProc_glMultiTexCoord4dvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4dvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4dvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4dvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4dvSGIS);
}

/* glMultiTexCoord4fSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4fSGIS == (GdkGLProc_glMultiTexCoord4fSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4fSGIS =
      (GdkGLProc_glMultiTexCoord4fSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4fSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4fSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4fSGIS);
}

/* glMultiTexCoord4fvSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4fvSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4fvSGIS == (GdkGLProc_glMultiTexCoord4fvSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4fvSGIS =
      (GdkGLProc_glMultiTexCoord4fvSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4fvSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4fvSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4fvSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4fvSGIS);
}

/* glMultiTexCoord4iSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4iSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4iSGIS == (GdkGLProc_glMultiTexCoord4iSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4iSGIS =
      (GdkGLProc_glMultiTexCoord4iSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4iSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4iSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4iSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4iSGIS);
}

/* glMultiTexCoord4ivSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4ivSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4ivSGIS == (GdkGLProc_glMultiTexCoord4ivSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4ivSGIS =
      (GdkGLProc_glMultiTexCoord4ivSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4ivSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4ivSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4ivSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4ivSGIS);
}

/* glMultiTexCoord4sSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4sSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4sSGIS == (GdkGLProc_glMultiTexCoord4sSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4sSGIS =
      (GdkGLProc_glMultiTexCoord4sSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4sSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4sSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4sSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4sSGIS);
}

/* glMultiTexCoord4svSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoord4svSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoord4svSGIS == (GdkGLProc_glMultiTexCoord4svSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoord4svSGIS =
      (GdkGLProc_glMultiTexCoord4svSGIS) gdk_gl_get_proc_address ("glMultiTexCoord4svSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoord4svSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoord4svSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoord4svSGIS);
}

/* glMultiTexCoordPointerSGIS */
GdkGLProc
gdk_gl_get_glMultiTexCoordPointerSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glMultiTexCoordPointerSGIS == (GdkGLProc_glMultiTexCoordPointerSGIS) -1)
    _procs_GL_SGIS_multitexture.glMultiTexCoordPointerSGIS =
      (GdkGLProc_glMultiTexCoordPointerSGIS) gdk_gl_get_proc_address ("glMultiTexCoordPointerSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiTexCoordPointerSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glMultiTexCoordPointerSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glMultiTexCoordPointerSGIS);
}

/* glSelectTextureSGIS */
GdkGLProc
gdk_gl_get_glSelectTextureSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glSelectTextureSGIS == (GdkGLProc_glSelectTextureSGIS) -1)
    _procs_GL_SGIS_multitexture.glSelectTextureSGIS =
      (GdkGLProc_glSelectTextureSGIS) gdk_gl_get_proc_address ("glSelectTextureSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSelectTextureSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glSelectTextureSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glSelectTextureSGIS);
}

/* glSelectTextureCoordSetSGIS */
GdkGLProc
gdk_gl_get_glSelectTextureCoordSetSGIS (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIS_multitexture.glSelectTextureCoordSetSGIS == (GdkGLProc_glSelectTextureCoordSetSGIS) -1)
    _procs_GL_SGIS_multitexture.glSelectTextureCoordSetSGIS =
      (GdkGLProc_glSelectTextureCoordSetSGIS) gdk_gl_get_proc_address ("glSelectTextureCoordSetSGIS");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glSelectTextureCoordSetSGIS () - %s",
               (_procs_GL_SGIS_multitexture.glSelectTextureCoordSetSGIS) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIS_multitexture.glSelectTextureCoordSetSGIS);
}

/* Get GL_SGIS_multitexture functions */
GdkGL_GL_SGIS_multitexture *
gdk_gl_get_GL_SGIS_multitexture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIS_multitexture");

      if (supported)
        {
          supported &= (gdk_gl_get_glMultiTexCoord1dSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1dvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1fSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1fvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1iSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1ivSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1sSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord1svSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2dSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2dvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2fSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2fvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2iSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2ivSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2sSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord2svSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3dSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3dvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3fSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3fvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3iSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3ivSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3sSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord3svSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4dSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4dvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4fSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4fvSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4iSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4ivSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4sSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoord4svSGIS () != NULL);
          supported &= (gdk_gl_get_glMultiTexCoordPointerSGIS () != NULL);
          supported &= (gdk_gl_get_glSelectTextureSGIS () != NULL);
          supported &= (gdk_gl_get_glSelectTextureCoordSetSGIS () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIS_multitexture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIS_multitexture;
}

/*
 * GL_SGIX_fog_texture
 */

static GdkGL_GL_SGIX_fog_texture _procs_GL_SGIX_fog_texture = {
  (GdkGLProc_glTextureFogSGIX) -1
};

/* glTextureFogSGIX */
GdkGLProc
gdk_gl_get_glTextureFogSGIX (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SGIX_fog_texture.glTextureFogSGIX == (GdkGLProc_glTextureFogSGIX) -1)
    _procs_GL_SGIX_fog_texture.glTextureFogSGIX =
      (GdkGLProc_glTextureFogSGIX) gdk_gl_get_proc_address ("glTextureFogSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glTextureFogSGIX () - %s",
               (_procs_GL_SGIX_fog_texture.glTextureFogSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SGIX_fog_texture.glTextureFogSGIX);
}

/* Get GL_SGIX_fog_texture functions */
GdkGL_GL_SGIX_fog_texture *
gdk_gl_get_GL_SGIX_fog_texture (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SGIX_fog_texture");

      if (supported)
        {
          supported &= (gdk_gl_get_glTextureFogSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SGIX_fog_texture () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SGIX_fog_texture;
}

/*
 * GL_SUN_multi_draw_arrays
 */

static GdkGL_GL_SUN_multi_draw_arrays _procs_GL_SUN_multi_draw_arrays = {
  (GdkGLProc_glMultiDrawArraysSUN) -1,
  (GdkGLProc_glMultiDrawElementsSUN) -1
};

/* glMultiDrawArraysSUN */
GdkGLProc
gdk_gl_get_glMultiDrawArraysSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_multi_draw_arrays.glMultiDrawArraysSUN == (GdkGLProc_glMultiDrawArraysSUN) -1)
    _procs_GL_SUN_multi_draw_arrays.glMultiDrawArraysSUN =
      (GdkGLProc_glMultiDrawArraysSUN) gdk_gl_get_proc_address ("glMultiDrawArraysSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawArraysSUN () - %s",
               (_procs_GL_SUN_multi_draw_arrays.glMultiDrawArraysSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_multi_draw_arrays.glMultiDrawArraysSUN);
}

/* glMultiDrawElementsSUN */
GdkGLProc
gdk_gl_get_glMultiDrawElementsSUN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_SUN_multi_draw_arrays.glMultiDrawElementsSUN == (GdkGLProc_glMultiDrawElementsSUN) -1)
    _procs_GL_SUN_multi_draw_arrays.glMultiDrawElementsSUN =
      (GdkGLProc_glMultiDrawElementsSUN) gdk_gl_get_proc_address ("glMultiDrawElementsSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glMultiDrawElementsSUN () - %s",
               (_procs_GL_SUN_multi_draw_arrays.glMultiDrawElementsSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_SUN_multi_draw_arrays.glMultiDrawElementsSUN);
}

/* Get GL_SUN_multi_draw_arrays functions */
GdkGL_GL_SUN_multi_draw_arrays *
gdk_gl_get_GL_SUN_multi_draw_arrays (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_SUN_multi_draw_arrays");

      if (supported)
        {
          supported &= (gdk_gl_get_glMultiDrawArraysSUN () != NULL);
          supported &= (gdk_gl_get_glMultiDrawElementsSUN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_SUN_multi_draw_arrays () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_SUN_multi_draw_arrays;
}

/*
 * GL_WIN_swap_hint
 */

static GdkGL_GL_WIN_swap_hint _procs_GL_WIN_swap_hint = {
  (GdkGLProc_glAddSwapHintRectWIN) -1
};

/* glAddSwapHintRectWIN */
GdkGLProc
gdk_gl_get_glAddSwapHintRectWIN (void)
{
  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (_procs_GL_WIN_swap_hint.glAddSwapHintRectWIN == (GdkGLProc_glAddSwapHintRectWIN) -1)
    _procs_GL_WIN_swap_hint.glAddSwapHintRectWIN =
      (GdkGLProc_glAddSwapHintRectWIN) gdk_gl_get_proc_address ("glAddSwapHintRectWIN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glAddSwapHintRectWIN () - %s",
               (_procs_GL_WIN_swap_hint.glAddSwapHintRectWIN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GL_WIN_swap_hint.glAddSwapHintRectWIN);
}

/* Get GL_WIN_swap_hint functions */
GdkGL_GL_WIN_swap_hint *
gdk_gl_get_GL_WIN_swap_hint (void)
{
  static gint supported = -1;

  if (gdk_gl_context_get_current () == NULL)
    return NULL;

  if (supported == -1)
    {
      supported = gdk_gl_query_gl_extension ("GL_WIN_swap_hint");

      if (supported)
        {
          supported &= (gdk_gl_get_glAddSwapHintRectWIN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GL_WIN_swap_hint () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GL_WIN_swap_hint;
}

