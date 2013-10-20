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
 * This is a generated file.  Please modify "gen-gdkglglxext-c.pl".
 */

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglglxext.h"

/*
 * GLX_VERSION_1_3
 */

static GdkGL_GLX_VERSION_1_3 _procs_GLX_VERSION_1_3 = {
  (GdkGLProc_glXGetFBConfigs) -1,
  (GdkGLProc_glXChooseFBConfig) -1,
  (GdkGLProc_glXGetFBConfigAttrib) -1,
  (GdkGLProc_glXGetVisualFromFBConfig) -1,
  (GdkGLProc_glXCreateWindow) -1,
  (GdkGLProc_glXDestroyWindow) -1,
  (GdkGLProc_glXCreatePixmap) -1,
  (GdkGLProc_glXDestroyPixmap) -1,
  (GdkGLProc_glXCreatePbuffer) -1,
  (GdkGLProc_glXDestroyPbuffer) -1,
  (GdkGLProc_glXQueryDrawable) -1,
  (GdkGLProc_glXCreateNewContext) -1,
  (GdkGLProc_glXMakeContextCurrent) -1,
  (GdkGLProc_glXGetCurrentReadDrawable) -1,
  (GdkGLProc_glXGetCurrentDisplay) -1,
  (GdkGLProc_glXQueryContext) -1,
  (GdkGLProc_glXSelectEvent) -1,
  (GdkGLProc_glXGetSelectedEvent) -1
};

/* glXGetFBConfigs */
GdkGLProc
gdk_gl_get_glXGetFBConfigs (void)
{
  if (_procs_GLX_VERSION_1_3.glXGetFBConfigs == (GdkGLProc_glXGetFBConfigs) -1)
    _procs_GLX_VERSION_1_3.glXGetFBConfigs =
      (GdkGLProc_glXGetFBConfigs) gdk_gl_get_proc_address ("glXGetFBConfigs");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetFBConfigs () - %s",
               (_procs_GLX_VERSION_1_3.glXGetFBConfigs) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXGetFBConfigs);
}

/* glXChooseFBConfig */
GdkGLProc
gdk_gl_get_glXChooseFBConfig (void)
{
  if (_procs_GLX_VERSION_1_3.glXChooseFBConfig == (GdkGLProc_glXChooseFBConfig) -1)
    _procs_GLX_VERSION_1_3.glXChooseFBConfig =
      (GdkGLProc_glXChooseFBConfig) gdk_gl_get_proc_address ("glXChooseFBConfig");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXChooseFBConfig () - %s",
               (_procs_GLX_VERSION_1_3.glXChooseFBConfig) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXChooseFBConfig);
}

/* glXGetFBConfigAttrib */
GdkGLProc
gdk_gl_get_glXGetFBConfigAttrib (void)
{
  if (_procs_GLX_VERSION_1_3.glXGetFBConfigAttrib == (GdkGLProc_glXGetFBConfigAttrib) -1)
    _procs_GLX_VERSION_1_3.glXGetFBConfigAttrib =
      (GdkGLProc_glXGetFBConfigAttrib) gdk_gl_get_proc_address ("glXGetFBConfigAttrib");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetFBConfigAttrib () - %s",
               (_procs_GLX_VERSION_1_3.glXGetFBConfigAttrib) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXGetFBConfigAttrib);
}

/* glXGetVisualFromFBConfig */
GdkGLProc
gdk_gl_get_glXGetVisualFromFBConfig (void)
{
  if (_procs_GLX_VERSION_1_3.glXGetVisualFromFBConfig == (GdkGLProc_glXGetVisualFromFBConfig) -1)
    _procs_GLX_VERSION_1_3.glXGetVisualFromFBConfig =
      (GdkGLProc_glXGetVisualFromFBConfig) gdk_gl_get_proc_address ("glXGetVisualFromFBConfig");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetVisualFromFBConfig () - %s",
               (_procs_GLX_VERSION_1_3.glXGetVisualFromFBConfig) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXGetVisualFromFBConfig);
}

/* glXCreateWindow */
GdkGLProc
gdk_gl_get_glXCreateWindow (void)
{
  if (_procs_GLX_VERSION_1_3.glXCreateWindow == (GdkGLProc_glXCreateWindow) -1)
    _procs_GLX_VERSION_1_3.glXCreateWindow =
      (GdkGLProc_glXCreateWindow) gdk_gl_get_proc_address ("glXCreateWindow");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateWindow () - %s",
               (_procs_GLX_VERSION_1_3.glXCreateWindow) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXCreateWindow);
}

/* glXDestroyWindow */
GdkGLProc
gdk_gl_get_glXDestroyWindow (void)
{
  if (_procs_GLX_VERSION_1_3.glXDestroyWindow == (GdkGLProc_glXDestroyWindow) -1)
    _procs_GLX_VERSION_1_3.glXDestroyWindow =
      (GdkGLProc_glXDestroyWindow) gdk_gl_get_proc_address ("glXDestroyWindow");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXDestroyWindow () - %s",
               (_procs_GLX_VERSION_1_3.glXDestroyWindow) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXDestroyWindow);
}

/* glXCreatePixmap */
GdkGLProc
gdk_gl_get_glXCreatePixmap (void)
{
  if (_procs_GLX_VERSION_1_3.glXCreatePixmap == (GdkGLProc_glXCreatePixmap) -1)
    _procs_GLX_VERSION_1_3.glXCreatePixmap =
      (GdkGLProc_glXCreatePixmap) gdk_gl_get_proc_address ("glXCreatePixmap");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreatePixmap () - %s",
               (_procs_GLX_VERSION_1_3.glXCreatePixmap) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXCreatePixmap);
}

/* glXDestroyPixmap */
GdkGLProc
gdk_gl_get_glXDestroyPixmap (void)
{
  if (_procs_GLX_VERSION_1_3.glXDestroyPixmap == (GdkGLProc_glXDestroyPixmap) -1)
    _procs_GLX_VERSION_1_3.glXDestroyPixmap =
      (GdkGLProc_glXDestroyPixmap) gdk_gl_get_proc_address ("glXDestroyPixmap");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXDestroyPixmap () - %s",
               (_procs_GLX_VERSION_1_3.glXDestroyPixmap) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXDestroyPixmap);
}

/* glXCreatePbuffer */
GdkGLProc
gdk_gl_get_glXCreatePbuffer (void)
{
  if (_procs_GLX_VERSION_1_3.glXCreatePbuffer == (GdkGLProc_glXCreatePbuffer) -1)
    _procs_GLX_VERSION_1_3.glXCreatePbuffer =
      (GdkGLProc_glXCreatePbuffer) gdk_gl_get_proc_address ("glXCreatePbuffer");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreatePbuffer () - %s",
               (_procs_GLX_VERSION_1_3.glXCreatePbuffer) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXCreatePbuffer);
}

/* glXDestroyPbuffer */
GdkGLProc
gdk_gl_get_glXDestroyPbuffer (void)
{
  if (_procs_GLX_VERSION_1_3.glXDestroyPbuffer == (GdkGLProc_glXDestroyPbuffer) -1)
    _procs_GLX_VERSION_1_3.glXDestroyPbuffer =
      (GdkGLProc_glXDestroyPbuffer) gdk_gl_get_proc_address ("glXDestroyPbuffer");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXDestroyPbuffer () - %s",
               (_procs_GLX_VERSION_1_3.glXDestroyPbuffer) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXDestroyPbuffer);
}

/* glXQueryDrawable */
GdkGLProc
gdk_gl_get_glXQueryDrawable (void)
{
  if (_procs_GLX_VERSION_1_3.glXQueryDrawable == (GdkGLProc_glXQueryDrawable) -1)
    _procs_GLX_VERSION_1_3.glXQueryDrawable =
      (GdkGLProc_glXQueryDrawable) gdk_gl_get_proc_address ("glXQueryDrawable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryDrawable () - %s",
               (_procs_GLX_VERSION_1_3.glXQueryDrawable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXQueryDrawable);
}

/* glXCreateNewContext */
GdkGLProc
gdk_gl_get_glXCreateNewContext (void)
{
  if (_procs_GLX_VERSION_1_3.glXCreateNewContext == (GdkGLProc_glXCreateNewContext) -1)
    _procs_GLX_VERSION_1_3.glXCreateNewContext =
      (GdkGLProc_glXCreateNewContext) gdk_gl_get_proc_address ("glXCreateNewContext");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateNewContext () - %s",
               (_procs_GLX_VERSION_1_3.glXCreateNewContext) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXCreateNewContext);
}

/* glXMakeContextCurrent */
GdkGLProc
gdk_gl_get_glXMakeContextCurrent (void)
{
  if (_procs_GLX_VERSION_1_3.glXMakeContextCurrent == (GdkGLProc_glXMakeContextCurrent) -1)
    _procs_GLX_VERSION_1_3.glXMakeContextCurrent =
      (GdkGLProc_glXMakeContextCurrent) gdk_gl_get_proc_address ("glXMakeContextCurrent");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXMakeContextCurrent () - %s",
               (_procs_GLX_VERSION_1_3.glXMakeContextCurrent) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXMakeContextCurrent);
}

/* glXGetCurrentReadDrawable */
GdkGLProc
gdk_gl_get_glXGetCurrentReadDrawable (void)
{
  if (_procs_GLX_VERSION_1_3.glXGetCurrentReadDrawable == (GdkGLProc_glXGetCurrentReadDrawable) -1)
    _procs_GLX_VERSION_1_3.glXGetCurrentReadDrawable =
      (GdkGLProc_glXGetCurrentReadDrawable) gdk_gl_get_proc_address ("glXGetCurrentReadDrawable");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetCurrentReadDrawable () - %s",
               (_procs_GLX_VERSION_1_3.glXGetCurrentReadDrawable) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXGetCurrentReadDrawable);
}

/* glXGetCurrentDisplay */
GdkGLProc
gdk_gl_get_glXGetCurrentDisplay (void)
{
  if (_procs_GLX_VERSION_1_3.glXGetCurrentDisplay == (GdkGLProc_glXGetCurrentDisplay) -1)
    _procs_GLX_VERSION_1_3.glXGetCurrentDisplay =
      (GdkGLProc_glXGetCurrentDisplay) gdk_gl_get_proc_address ("glXGetCurrentDisplay");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetCurrentDisplay () - %s",
               (_procs_GLX_VERSION_1_3.glXGetCurrentDisplay) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXGetCurrentDisplay);
}

/* glXQueryContext */
GdkGLProc
gdk_gl_get_glXQueryContext (void)
{
  if (_procs_GLX_VERSION_1_3.glXQueryContext == (GdkGLProc_glXQueryContext) -1)
    _procs_GLX_VERSION_1_3.glXQueryContext =
      (GdkGLProc_glXQueryContext) gdk_gl_get_proc_address ("glXQueryContext");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryContext () - %s",
               (_procs_GLX_VERSION_1_3.glXQueryContext) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXQueryContext);
}

/* glXSelectEvent */
GdkGLProc
gdk_gl_get_glXSelectEvent (void)
{
  if (_procs_GLX_VERSION_1_3.glXSelectEvent == (GdkGLProc_glXSelectEvent) -1)
    _procs_GLX_VERSION_1_3.glXSelectEvent =
      (GdkGLProc_glXSelectEvent) gdk_gl_get_proc_address ("glXSelectEvent");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXSelectEvent () - %s",
               (_procs_GLX_VERSION_1_3.glXSelectEvent) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXSelectEvent);
}

/* glXGetSelectedEvent */
GdkGLProc
gdk_gl_get_glXGetSelectedEvent (void)
{
  if (_procs_GLX_VERSION_1_3.glXGetSelectedEvent == (GdkGLProc_glXGetSelectedEvent) -1)
    _procs_GLX_VERSION_1_3.glXGetSelectedEvent =
      (GdkGLProc_glXGetSelectedEvent) gdk_gl_get_proc_address ("glXGetSelectedEvent");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetSelectedEvent () - %s",
               (_procs_GLX_VERSION_1_3.glXGetSelectedEvent) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_3.glXGetSelectedEvent);
}

/* Get GLX_VERSION_1_3 functions */
GdkGL_GLX_VERSION_1_3 *
gdk_gl_get_GLX_VERSION_1_3 (void)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported =  (gdk_gl_get_glXGetFBConfigs () != NULL);
      supported &= (gdk_gl_get_glXChooseFBConfig () != NULL);
      supported &= (gdk_gl_get_glXGetFBConfigAttrib () != NULL);
      supported &= (gdk_gl_get_glXGetVisualFromFBConfig () != NULL);
      supported &= (gdk_gl_get_glXCreateWindow () != NULL);
      supported &= (gdk_gl_get_glXDestroyWindow () != NULL);
      supported &= (gdk_gl_get_glXCreatePixmap () != NULL);
      supported &= (gdk_gl_get_glXDestroyPixmap () != NULL);
      supported &= (gdk_gl_get_glXCreatePbuffer () != NULL);
      supported &= (gdk_gl_get_glXDestroyPbuffer () != NULL);
      supported &= (gdk_gl_get_glXQueryDrawable () != NULL);
      supported &= (gdk_gl_get_glXCreateNewContext () != NULL);
      supported &= (gdk_gl_get_glXMakeContextCurrent () != NULL);
      supported &= (gdk_gl_get_glXGetCurrentReadDrawable () != NULL);
      supported &= (gdk_gl_get_glXGetCurrentDisplay () != NULL);
      supported &= (gdk_gl_get_glXQueryContext () != NULL);
      supported &= (gdk_gl_get_glXSelectEvent () != NULL);
      supported &= (gdk_gl_get_glXGetSelectedEvent () != NULL);
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_VERSION_1_3 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_VERSION_1_3;
}

/*
 * GLX_VERSION_1_4
 */

static GdkGL_GLX_VERSION_1_4 _procs_GLX_VERSION_1_4 = {
  (GdkGLProc_glXGetProcAddress) -1
};

/* glXGetProcAddress */
GdkGLProc
gdk_gl_get_glXGetProcAddress (void)
{
  if (_procs_GLX_VERSION_1_4.glXGetProcAddress == (GdkGLProc_glXGetProcAddress) -1)
    _procs_GLX_VERSION_1_4.glXGetProcAddress =
      (GdkGLProc_glXGetProcAddress) gdk_gl_get_proc_address ("glXGetProcAddress");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetProcAddress () - %s",
               (_procs_GLX_VERSION_1_4.glXGetProcAddress) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_VERSION_1_4.glXGetProcAddress);
}

/* Get GLX_VERSION_1_4 functions */
GdkGL_GLX_VERSION_1_4 *
gdk_gl_get_GLX_VERSION_1_4 (void)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported =  (gdk_gl_get_glXGetProcAddress () != NULL);
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_VERSION_1_4 () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_VERSION_1_4;
}

/*
 * GLX_ARB_get_proc_address
 */

static GdkGL_GLX_ARB_get_proc_address _procs_GLX_ARB_get_proc_address = {
  (GdkGLProc_glXGetProcAddressARB) -1
};

/* glXGetProcAddressARB */
GdkGLProc
gdk_gl_get_glXGetProcAddressARB (void)
{
  if (_procs_GLX_ARB_get_proc_address.glXGetProcAddressARB == (GdkGLProc_glXGetProcAddressARB) -1)
    _procs_GLX_ARB_get_proc_address.glXGetProcAddressARB =
      (GdkGLProc_glXGetProcAddressARB) gdk_gl_get_proc_address ("glXGetProcAddressARB");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetProcAddressARB () - %s",
               (_procs_GLX_ARB_get_proc_address.glXGetProcAddressARB) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_ARB_get_proc_address.glXGetProcAddressARB);
}

/* Get GLX_ARB_get_proc_address functions */
GdkGL_GLX_ARB_get_proc_address *
gdk_gl_get_GLX_ARB_get_proc_address (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_ARB_get_proc_address");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetProcAddressARB () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_ARB_get_proc_address () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_ARB_get_proc_address;
}

/*
 * GLX_SGI_swap_control
 */

static GdkGL_GLX_SGI_swap_control _procs_GLX_SGI_swap_control = {
  (GdkGLProc_glXSwapIntervalSGI) -1
};

/* glXSwapIntervalSGI */
GdkGLProc
gdk_gl_get_glXSwapIntervalSGI (void)
{
  if (_procs_GLX_SGI_swap_control.glXSwapIntervalSGI == (GdkGLProc_glXSwapIntervalSGI) -1)
    _procs_GLX_SGI_swap_control.glXSwapIntervalSGI =
      (GdkGLProc_glXSwapIntervalSGI) gdk_gl_get_proc_address ("glXSwapIntervalSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXSwapIntervalSGI () - %s",
               (_procs_GLX_SGI_swap_control.glXSwapIntervalSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGI_swap_control.glXSwapIntervalSGI);
}

/* Get GLX_SGI_swap_control functions */
GdkGL_GLX_SGI_swap_control *
gdk_gl_get_GLX_SGI_swap_control (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGI_swap_control");

      if (supported)
        {
          supported &= (gdk_gl_get_glXSwapIntervalSGI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGI_swap_control () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGI_swap_control;
}

/*
 * GLX_SGI_video_sync
 */

static GdkGL_GLX_SGI_video_sync _procs_GLX_SGI_video_sync = {
  (GdkGLProc_glXGetVideoSyncSGI) -1,
  (GdkGLProc_glXWaitVideoSyncSGI) -1
};

/* glXGetVideoSyncSGI */
GdkGLProc
gdk_gl_get_glXGetVideoSyncSGI (void)
{
  if (_procs_GLX_SGI_video_sync.glXGetVideoSyncSGI == (GdkGLProc_glXGetVideoSyncSGI) -1)
    _procs_GLX_SGI_video_sync.glXGetVideoSyncSGI =
      (GdkGLProc_glXGetVideoSyncSGI) gdk_gl_get_proc_address ("glXGetVideoSyncSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetVideoSyncSGI () - %s",
               (_procs_GLX_SGI_video_sync.glXGetVideoSyncSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGI_video_sync.glXGetVideoSyncSGI);
}

/* glXWaitVideoSyncSGI */
GdkGLProc
gdk_gl_get_glXWaitVideoSyncSGI (void)
{
  if (_procs_GLX_SGI_video_sync.glXWaitVideoSyncSGI == (GdkGLProc_glXWaitVideoSyncSGI) -1)
    _procs_GLX_SGI_video_sync.glXWaitVideoSyncSGI =
      (GdkGLProc_glXWaitVideoSyncSGI) gdk_gl_get_proc_address ("glXWaitVideoSyncSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXWaitVideoSyncSGI () - %s",
               (_procs_GLX_SGI_video_sync.glXWaitVideoSyncSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGI_video_sync.glXWaitVideoSyncSGI);
}

/* Get GLX_SGI_video_sync functions */
GdkGL_GLX_SGI_video_sync *
gdk_gl_get_GLX_SGI_video_sync (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGI_video_sync");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetVideoSyncSGI () != NULL);
          supported &= (gdk_gl_get_glXWaitVideoSyncSGI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGI_video_sync () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGI_video_sync;
}

/*
 * GLX_SGI_make_current_read
 */

static GdkGL_GLX_SGI_make_current_read _procs_GLX_SGI_make_current_read = {
  (GdkGLProc_glXMakeCurrentReadSGI) -1,
  (GdkGLProc_glXGetCurrentReadDrawableSGI) -1
};

/* glXMakeCurrentReadSGI */
GdkGLProc
gdk_gl_get_glXMakeCurrentReadSGI (void)
{
  if (_procs_GLX_SGI_make_current_read.glXMakeCurrentReadSGI == (GdkGLProc_glXMakeCurrentReadSGI) -1)
    _procs_GLX_SGI_make_current_read.glXMakeCurrentReadSGI =
      (GdkGLProc_glXMakeCurrentReadSGI) gdk_gl_get_proc_address ("glXMakeCurrentReadSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXMakeCurrentReadSGI () - %s",
               (_procs_GLX_SGI_make_current_read.glXMakeCurrentReadSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGI_make_current_read.glXMakeCurrentReadSGI);
}

/* glXGetCurrentReadDrawableSGI */
GdkGLProc
gdk_gl_get_glXGetCurrentReadDrawableSGI (void)
{
  if (_procs_GLX_SGI_make_current_read.glXGetCurrentReadDrawableSGI == (GdkGLProc_glXGetCurrentReadDrawableSGI) -1)
    _procs_GLX_SGI_make_current_read.glXGetCurrentReadDrawableSGI =
      (GdkGLProc_glXGetCurrentReadDrawableSGI) gdk_gl_get_proc_address ("glXGetCurrentReadDrawableSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetCurrentReadDrawableSGI () - %s",
               (_procs_GLX_SGI_make_current_read.glXGetCurrentReadDrawableSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGI_make_current_read.glXGetCurrentReadDrawableSGI);
}

/* Get GLX_SGI_make_current_read functions */
GdkGL_GLX_SGI_make_current_read *
gdk_gl_get_GLX_SGI_make_current_read (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGI_make_current_read");

      if (supported)
        {
          supported &= (gdk_gl_get_glXMakeCurrentReadSGI () != NULL);
          supported &= (gdk_gl_get_glXGetCurrentReadDrawableSGI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGI_make_current_read () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGI_make_current_read;
}

/*
 * GLX_SGIX_video_source
 */

#ifdef _VL_H

static GdkGL_GLX_SGIX_video_source _procs_GLX_SGIX_video_source = {
  (GdkGLProc_glXCreateGLXVideoSourceSGIX) -1,
  (GdkGLProc_glXDestroyGLXVideoSourceSGIX) -1
};

/* glXCreateGLXVideoSourceSGIX */
GdkGLProc
gdk_gl_get_glXCreateGLXVideoSourceSGIX (void)
{
  if (_procs_GLX_SGIX_video_source.glXCreateGLXVideoSourceSGIX == (GdkGLProc_glXCreateGLXVideoSourceSGIX) -1)
    _procs_GLX_SGIX_video_source.glXCreateGLXVideoSourceSGIX =
      (GdkGLProc_glXCreateGLXVideoSourceSGIX) gdk_gl_get_proc_address ("glXCreateGLXVideoSourceSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateGLXVideoSourceSGIX () - %s",
               (_procs_GLX_SGIX_video_source.glXCreateGLXVideoSourceSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_source.glXCreateGLXVideoSourceSGIX);
}

/* glXDestroyGLXVideoSourceSGIX */
GdkGLProc
gdk_gl_get_glXDestroyGLXVideoSourceSGIX (void)
{
  if (_procs_GLX_SGIX_video_source.glXDestroyGLXVideoSourceSGIX == (GdkGLProc_glXDestroyGLXVideoSourceSGIX) -1)
    _procs_GLX_SGIX_video_source.glXDestroyGLXVideoSourceSGIX =
      (GdkGLProc_glXDestroyGLXVideoSourceSGIX) gdk_gl_get_proc_address ("glXDestroyGLXVideoSourceSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXDestroyGLXVideoSourceSGIX () - %s",
               (_procs_GLX_SGIX_video_source.glXDestroyGLXVideoSourceSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_source.glXDestroyGLXVideoSourceSGIX);
}

/* Get GLX_SGIX_video_source functions */
GdkGL_GLX_SGIX_video_source *
gdk_gl_get_GLX_SGIX_video_source (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_video_source");

      if (supported)
        {
          supported &= (gdk_gl_get_glXCreateGLXVideoSourceSGIX () != NULL);
          supported &= (gdk_gl_get_glXDestroyGLXVideoSourceSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_video_source () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_video_source;
}

#endif /* _VL_H */

/*
 * GLX_EXT_import_context
 */

static GdkGL_GLX_EXT_import_context _procs_GLX_EXT_import_context = {
  (GdkGLProc_glXGetCurrentDisplayEXT) -1,
  (GdkGLProc_glXQueryContextInfoEXT) -1,
  (GdkGLProc_glXGetContextIDEXT) -1,
  (GdkGLProc_glXImportContextEXT) -1,
  (GdkGLProc_glXFreeContextEXT) -1
};

/* glXGetCurrentDisplayEXT */
GdkGLProc
gdk_gl_get_glXGetCurrentDisplayEXT (void)
{
  if (_procs_GLX_EXT_import_context.glXGetCurrentDisplayEXT == (GdkGLProc_glXGetCurrentDisplayEXT) -1)
    _procs_GLX_EXT_import_context.glXGetCurrentDisplayEXT =
      (GdkGLProc_glXGetCurrentDisplayEXT) gdk_gl_get_proc_address ("glXGetCurrentDisplayEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetCurrentDisplayEXT () - %s",
               (_procs_GLX_EXT_import_context.glXGetCurrentDisplayEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_EXT_import_context.glXGetCurrentDisplayEXT);
}

/* glXQueryContextInfoEXT */
GdkGLProc
gdk_gl_get_glXQueryContextInfoEXT (void)
{
  if (_procs_GLX_EXT_import_context.glXQueryContextInfoEXT == (GdkGLProc_glXQueryContextInfoEXT) -1)
    _procs_GLX_EXT_import_context.glXQueryContextInfoEXT =
      (GdkGLProc_glXQueryContextInfoEXT) gdk_gl_get_proc_address ("glXQueryContextInfoEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryContextInfoEXT () - %s",
               (_procs_GLX_EXT_import_context.glXQueryContextInfoEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_EXT_import_context.glXQueryContextInfoEXT);
}

/* glXGetContextIDEXT */
GdkGLProc
gdk_gl_get_glXGetContextIDEXT (void)
{
  if (_procs_GLX_EXT_import_context.glXGetContextIDEXT == (GdkGLProc_glXGetContextIDEXT) -1)
    _procs_GLX_EXT_import_context.glXGetContextIDEXT =
      (GdkGLProc_glXGetContextIDEXT) gdk_gl_get_proc_address ("glXGetContextIDEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetContextIDEXT () - %s",
               (_procs_GLX_EXT_import_context.glXGetContextIDEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_EXT_import_context.glXGetContextIDEXT);
}

/* glXImportContextEXT */
GdkGLProc
gdk_gl_get_glXImportContextEXT (void)
{
  if (_procs_GLX_EXT_import_context.glXImportContextEXT == (GdkGLProc_glXImportContextEXT) -1)
    _procs_GLX_EXT_import_context.glXImportContextEXT =
      (GdkGLProc_glXImportContextEXT) gdk_gl_get_proc_address ("glXImportContextEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXImportContextEXT () - %s",
               (_procs_GLX_EXT_import_context.glXImportContextEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_EXT_import_context.glXImportContextEXT);
}

/* glXFreeContextEXT */
GdkGLProc
gdk_gl_get_glXFreeContextEXT (void)
{
  if (_procs_GLX_EXT_import_context.glXFreeContextEXT == (GdkGLProc_glXFreeContextEXT) -1)
    _procs_GLX_EXT_import_context.glXFreeContextEXT =
      (GdkGLProc_glXFreeContextEXT) gdk_gl_get_proc_address ("glXFreeContextEXT");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXFreeContextEXT () - %s",
               (_procs_GLX_EXT_import_context.glXFreeContextEXT) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_EXT_import_context.glXFreeContextEXT);
}

/* Get GLX_EXT_import_context functions */
GdkGL_GLX_EXT_import_context *
gdk_gl_get_GLX_EXT_import_context (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_EXT_import_context");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetCurrentDisplayEXT () != NULL);
          supported &= (gdk_gl_get_glXQueryContextInfoEXT () != NULL);
          supported &= (gdk_gl_get_glXGetContextIDEXT () != NULL);
          supported &= (gdk_gl_get_glXImportContextEXT () != NULL);
          supported &= (gdk_gl_get_glXFreeContextEXT () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_EXT_import_context () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_EXT_import_context;
}

/*
 * GLX_SGIX_fbconfig
 */

static GdkGL_GLX_SGIX_fbconfig _procs_GLX_SGIX_fbconfig = {
  (GdkGLProc_glXGetFBConfigAttribSGIX) -1,
  (GdkGLProc_glXChooseFBConfigSGIX) -1,
  (GdkGLProc_glXCreateGLXPixmapWithConfigSGIX) -1,
  (GdkGLProc_glXCreateContextWithConfigSGIX) -1,
  (GdkGLProc_glXGetVisualFromFBConfigSGIX) -1,
  (GdkGLProc_glXGetFBConfigFromVisualSGIX) -1
};

/* glXGetFBConfigAttribSGIX */
GdkGLProc
gdk_gl_get_glXGetFBConfigAttribSGIX (void)
{
  if (_procs_GLX_SGIX_fbconfig.glXGetFBConfigAttribSGIX == (GdkGLProc_glXGetFBConfigAttribSGIX) -1)
    _procs_GLX_SGIX_fbconfig.glXGetFBConfigAttribSGIX =
      (GdkGLProc_glXGetFBConfigAttribSGIX) gdk_gl_get_proc_address ("glXGetFBConfigAttribSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetFBConfigAttribSGIX () - %s",
               (_procs_GLX_SGIX_fbconfig.glXGetFBConfigAttribSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_fbconfig.glXGetFBConfigAttribSGIX);
}

/* glXChooseFBConfigSGIX */
GdkGLProc
gdk_gl_get_glXChooseFBConfigSGIX (void)
{
  if (_procs_GLX_SGIX_fbconfig.glXChooseFBConfigSGIX == (GdkGLProc_glXChooseFBConfigSGIX) -1)
    _procs_GLX_SGIX_fbconfig.glXChooseFBConfigSGIX =
      (GdkGLProc_glXChooseFBConfigSGIX) gdk_gl_get_proc_address ("glXChooseFBConfigSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXChooseFBConfigSGIX () - %s",
               (_procs_GLX_SGIX_fbconfig.glXChooseFBConfigSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_fbconfig.glXChooseFBConfigSGIX);
}

/* glXCreateGLXPixmapWithConfigSGIX */
GdkGLProc
gdk_gl_get_glXCreateGLXPixmapWithConfigSGIX (void)
{
  if (_procs_GLX_SGIX_fbconfig.glXCreateGLXPixmapWithConfigSGIX == (GdkGLProc_glXCreateGLXPixmapWithConfigSGIX) -1)
    _procs_GLX_SGIX_fbconfig.glXCreateGLXPixmapWithConfigSGIX =
      (GdkGLProc_glXCreateGLXPixmapWithConfigSGIX) gdk_gl_get_proc_address ("glXCreateGLXPixmapWithConfigSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateGLXPixmapWithConfigSGIX () - %s",
               (_procs_GLX_SGIX_fbconfig.glXCreateGLXPixmapWithConfigSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_fbconfig.glXCreateGLXPixmapWithConfigSGIX);
}

/* glXCreateContextWithConfigSGIX */
GdkGLProc
gdk_gl_get_glXCreateContextWithConfigSGIX (void)
{
  if (_procs_GLX_SGIX_fbconfig.glXCreateContextWithConfigSGIX == (GdkGLProc_glXCreateContextWithConfigSGIX) -1)
    _procs_GLX_SGIX_fbconfig.glXCreateContextWithConfigSGIX =
      (GdkGLProc_glXCreateContextWithConfigSGIX) gdk_gl_get_proc_address ("glXCreateContextWithConfigSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateContextWithConfigSGIX () - %s",
               (_procs_GLX_SGIX_fbconfig.glXCreateContextWithConfigSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_fbconfig.glXCreateContextWithConfigSGIX);
}

/* glXGetVisualFromFBConfigSGIX */
GdkGLProc
gdk_gl_get_glXGetVisualFromFBConfigSGIX (void)
{
  if (_procs_GLX_SGIX_fbconfig.glXGetVisualFromFBConfigSGIX == (GdkGLProc_glXGetVisualFromFBConfigSGIX) -1)
    _procs_GLX_SGIX_fbconfig.glXGetVisualFromFBConfigSGIX =
      (GdkGLProc_glXGetVisualFromFBConfigSGIX) gdk_gl_get_proc_address ("glXGetVisualFromFBConfigSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetVisualFromFBConfigSGIX () - %s",
               (_procs_GLX_SGIX_fbconfig.glXGetVisualFromFBConfigSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_fbconfig.glXGetVisualFromFBConfigSGIX);
}

/* glXGetFBConfigFromVisualSGIX */
GdkGLProc
gdk_gl_get_glXGetFBConfigFromVisualSGIX (void)
{
  if (_procs_GLX_SGIX_fbconfig.glXGetFBConfigFromVisualSGIX == (GdkGLProc_glXGetFBConfigFromVisualSGIX) -1)
    _procs_GLX_SGIX_fbconfig.glXGetFBConfigFromVisualSGIX =
      (GdkGLProc_glXGetFBConfigFromVisualSGIX) gdk_gl_get_proc_address ("glXGetFBConfigFromVisualSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetFBConfigFromVisualSGIX () - %s",
               (_procs_GLX_SGIX_fbconfig.glXGetFBConfigFromVisualSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_fbconfig.glXGetFBConfigFromVisualSGIX);
}

/* Get GLX_SGIX_fbconfig functions */
GdkGL_GLX_SGIX_fbconfig *
gdk_gl_get_GLX_SGIX_fbconfig (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_fbconfig");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetFBConfigAttribSGIX () != NULL);
          supported &= (gdk_gl_get_glXChooseFBConfigSGIX () != NULL);
          supported &= (gdk_gl_get_glXCreateGLXPixmapWithConfigSGIX () != NULL);
          supported &= (gdk_gl_get_glXCreateContextWithConfigSGIX () != NULL);
          supported &= (gdk_gl_get_glXGetVisualFromFBConfigSGIX () != NULL);
          supported &= (gdk_gl_get_glXGetFBConfigFromVisualSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_fbconfig () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_fbconfig;
}

/*
 * GLX_SGIX_pbuffer
 */

static GdkGL_GLX_SGIX_pbuffer _procs_GLX_SGIX_pbuffer = {
  (GdkGLProc_glXCreateGLXPbufferSGIX) -1,
  (GdkGLProc_glXDestroyGLXPbufferSGIX) -1,
  (GdkGLProc_glXQueryGLXPbufferSGIX) -1,
  (GdkGLProc_glXSelectEventSGIX) -1,
  (GdkGLProc_glXGetSelectedEventSGIX) -1
};

/* glXCreateGLXPbufferSGIX */
GdkGLProc
gdk_gl_get_glXCreateGLXPbufferSGIX (void)
{
  if (_procs_GLX_SGIX_pbuffer.glXCreateGLXPbufferSGIX == (GdkGLProc_glXCreateGLXPbufferSGIX) -1)
    _procs_GLX_SGIX_pbuffer.glXCreateGLXPbufferSGIX =
      (GdkGLProc_glXCreateGLXPbufferSGIX) gdk_gl_get_proc_address ("glXCreateGLXPbufferSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateGLXPbufferSGIX () - %s",
               (_procs_GLX_SGIX_pbuffer.glXCreateGLXPbufferSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_pbuffer.glXCreateGLXPbufferSGIX);
}

/* glXDestroyGLXPbufferSGIX */
GdkGLProc
gdk_gl_get_glXDestroyGLXPbufferSGIX (void)
{
  if (_procs_GLX_SGIX_pbuffer.glXDestroyGLXPbufferSGIX == (GdkGLProc_glXDestroyGLXPbufferSGIX) -1)
    _procs_GLX_SGIX_pbuffer.glXDestroyGLXPbufferSGIX =
      (GdkGLProc_glXDestroyGLXPbufferSGIX) gdk_gl_get_proc_address ("glXDestroyGLXPbufferSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXDestroyGLXPbufferSGIX () - %s",
               (_procs_GLX_SGIX_pbuffer.glXDestroyGLXPbufferSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_pbuffer.glXDestroyGLXPbufferSGIX);
}

/* glXQueryGLXPbufferSGIX */
GdkGLProc
gdk_gl_get_glXQueryGLXPbufferSGIX (void)
{
  if (_procs_GLX_SGIX_pbuffer.glXQueryGLXPbufferSGIX == (GdkGLProc_glXQueryGLXPbufferSGIX) -1)
    _procs_GLX_SGIX_pbuffer.glXQueryGLXPbufferSGIX =
      (GdkGLProc_glXQueryGLXPbufferSGIX) gdk_gl_get_proc_address ("glXQueryGLXPbufferSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryGLXPbufferSGIX () - %s",
               (_procs_GLX_SGIX_pbuffer.glXQueryGLXPbufferSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_pbuffer.glXQueryGLXPbufferSGIX);
}

/* glXSelectEventSGIX */
GdkGLProc
gdk_gl_get_glXSelectEventSGIX (void)
{
  if (_procs_GLX_SGIX_pbuffer.glXSelectEventSGIX == (GdkGLProc_glXSelectEventSGIX) -1)
    _procs_GLX_SGIX_pbuffer.glXSelectEventSGIX =
      (GdkGLProc_glXSelectEventSGIX) gdk_gl_get_proc_address ("glXSelectEventSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXSelectEventSGIX () - %s",
               (_procs_GLX_SGIX_pbuffer.glXSelectEventSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_pbuffer.glXSelectEventSGIX);
}

/* glXGetSelectedEventSGIX */
GdkGLProc
gdk_gl_get_glXGetSelectedEventSGIX (void)
{
  if (_procs_GLX_SGIX_pbuffer.glXGetSelectedEventSGIX == (GdkGLProc_glXGetSelectedEventSGIX) -1)
    _procs_GLX_SGIX_pbuffer.glXGetSelectedEventSGIX =
      (GdkGLProc_glXGetSelectedEventSGIX) gdk_gl_get_proc_address ("glXGetSelectedEventSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetSelectedEventSGIX () - %s",
               (_procs_GLX_SGIX_pbuffer.glXGetSelectedEventSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_pbuffer.glXGetSelectedEventSGIX);
}

/* Get GLX_SGIX_pbuffer functions */
GdkGL_GLX_SGIX_pbuffer *
gdk_gl_get_GLX_SGIX_pbuffer (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_pbuffer");

      if (supported)
        {
          supported &= (gdk_gl_get_glXCreateGLXPbufferSGIX () != NULL);
          supported &= (gdk_gl_get_glXDestroyGLXPbufferSGIX () != NULL);
          supported &= (gdk_gl_get_glXQueryGLXPbufferSGIX () != NULL);
          supported &= (gdk_gl_get_glXSelectEventSGIX () != NULL);
          supported &= (gdk_gl_get_glXGetSelectedEventSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_pbuffer () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_pbuffer;
}

/*
 * GLX_SGI_cushion
 */

static GdkGL_GLX_SGI_cushion _procs_GLX_SGI_cushion = {
  (GdkGLProc_glXCushionSGI) -1
};

/* glXCushionSGI */
GdkGLProc
gdk_gl_get_glXCushionSGI (void)
{
  if (_procs_GLX_SGI_cushion.glXCushionSGI == (GdkGLProc_glXCushionSGI) -1)
    _procs_GLX_SGI_cushion.glXCushionSGI =
      (GdkGLProc_glXCushionSGI) gdk_gl_get_proc_address ("glXCushionSGI");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCushionSGI () - %s",
               (_procs_GLX_SGI_cushion.glXCushionSGI) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGI_cushion.glXCushionSGI);
}

/* Get GLX_SGI_cushion functions */
GdkGL_GLX_SGI_cushion *
gdk_gl_get_GLX_SGI_cushion (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGI_cushion");

      if (supported)
        {
          supported &= (gdk_gl_get_glXCushionSGI () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGI_cushion () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGI_cushion;
}

/*
 * GLX_SGIX_video_resize
 */

static GdkGL_GLX_SGIX_video_resize _procs_GLX_SGIX_video_resize = {
  (GdkGLProc_glXBindChannelToWindowSGIX) -1,
  (GdkGLProc_glXChannelRectSGIX) -1,
  (GdkGLProc_glXQueryChannelRectSGIX) -1,
  (GdkGLProc_glXQueryChannelDeltasSGIX) -1,
  (GdkGLProc_glXChannelRectSyncSGIX) -1
};

/* glXBindChannelToWindowSGIX */
GdkGLProc
gdk_gl_get_glXBindChannelToWindowSGIX (void)
{
  if (_procs_GLX_SGIX_video_resize.glXBindChannelToWindowSGIX == (GdkGLProc_glXBindChannelToWindowSGIX) -1)
    _procs_GLX_SGIX_video_resize.glXBindChannelToWindowSGIX =
      (GdkGLProc_glXBindChannelToWindowSGIX) gdk_gl_get_proc_address ("glXBindChannelToWindowSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXBindChannelToWindowSGIX () - %s",
               (_procs_GLX_SGIX_video_resize.glXBindChannelToWindowSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_resize.glXBindChannelToWindowSGIX);
}

/* glXChannelRectSGIX */
GdkGLProc
gdk_gl_get_glXChannelRectSGIX (void)
{
  if (_procs_GLX_SGIX_video_resize.glXChannelRectSGIX == (GdkGLProc_glXChannelRectSGIX) -1)
    _procs_GLX_SGIX_video_resize.glXChannelRectSGIX =
      (GdkGLProc_glXChannelRectSGIX) gdk_gl_get_proc_address ("glXChannelRectSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXChannelRectSGIX () - %s",
               (_procs_GLX_SGIX_video_resize.glXChannelRectSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_resize.glXChannelRectSGIX);
}

/* glXQueryChannelRectSGIX */
GdkGLProc
gdk_gl_get_glXQueryChannelRectSGIX (void)
{
  if (_procs_GLX_SGIX_video_resize.glXQueryChannelRectSGIX == (GdkGLProc_glXQueryChannelRectSGIX) -1)
    _procs_GLX_SGIX_video_resize.glXQueryChannelRectSGIX =
      (GdkGLProc_glXQueryChannelRectSGIX) gdk_gl_get_proc_address ("glXQueryChannelRectSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryChannelRectSGIX () - %s",
               (_procs_GLX_SGIX_video_resize.glXQueryChannelRectSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_resize.glXQueryChannelRectSGIX);
}

/* glXQueryChannelDeltasSGIX */
GdkGLProc
gdk_gl_get_glXQueryChannelDeltasSGIX (void)
{
  if (_procs_GLX_SGIX_video_resize.glXQueryChannelDeltasSGIX == (GdkGLProc_glXQueryChannelDeltasSGIX) -1)
    _procs_GLX_SGIX_video_resize.glXQueryChannelDeltasSGIX =
      (GdkGLProc_glXQueryChannelDeltasSGIX) gdk_gl_get_proc_address ("glXQueryChannelDeltasSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryChannelDeltasSGIX () - %s",
               (_procs_GLX_SGIX_video_resize.glXQueryChannelDeltasSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_resize.glXQueryChannelDeltasSGIX);
}

/* glXChannelRectSyncSGIX */
GdkGLProc
gdk_gl_get_glXChannelRectSyncSGIX (void)
{
  if (_procs_GLX_SGIX_video_resize.glXChannelRectSyncSGIX == (GdkGLProc_glXChannelRectSyncSGIX) -1)
    _procs_GLX_SGIX_video_resize.glXChannelRectSyncSGIX =
      (GdkGLProc_glXChannelRectSyncSGIX) gdk_gl_get_proc_address ("glXChannelRectSyncSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXChannelRectSyncSGIX () - %s",
               (_procs_GLX_SGIX_video_resize.glXChannelRectSyncSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_video_resize.glXChannelRectSyncSGIX);
}

/* Get GLX_SGIX_video_resize functions */
GdkGL_GLX_SGIX_video_resize *
gdk_gl_get_GLX_SGIX_video_resize (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_video_resize");

      if (supported)
        {
          supported &= (gdk_gl_get_glXBindChannelToWindowSGIX () != NULL);
          supported &= (gdk_gl_get_glXChannelRectSGIX () != NULL);
          supported &= (gdk_gl_get_glXQueryChannelRectSGIX () != NULL);
          supported &= (gdk_gl_get_glXQueryChannelDeltasSGIX () != NULL);
          supported &= (gdk_gl_get_glXChannelRectSyncSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_video_resize () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_video_resize;
}

/*
 * GLX_SGIX_dmbuffer
 */

#ifdef _DM_BUFFER_H_

static GdkGL_GLX_SGIX_dmbuffer _procs_GLX_SGIX_dmbuffer = {
  (GdkGLProc_glXAssociateDMPbufferSGIX) -1
};

/* glXAssociateDMPbufferSGIX */
GdkGLProc
gdk_gl_get_glXAssociateDMPbufferSGIX (void)
{
  if (_procs_GLX_SGIX_dmbuffer.glXAssociateDMPbufferSGIX == (GdkGLProc_glXAssociateDMPbufferSGIX) -1)
    _procs_GLX_SGIX_dmbuffer.glXAssociateDMPbufferSGIX =
      (GdkGLProc_glXAssociateDMPbufferSGIX) gdk_gl_get_proc_address ("glXAssociateDMPbufferSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXAssociateDMPbufferSGIX () - %s",
               (_procs_GLX_SGIX_dmbuffer.glXAssociateDMPbufferSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_dmbuffer.glXAssociateDMPbufferSGIX);
}

/* Get GLX_SGIX_dmbuffer functions */
GdkGL_GLX_SGIX_dmbuffer *
gdk_gl_get_GLX_SGIX_dmbuffer (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_dmbuffer");

      if (supported)
        {
          supported &= (gdk_gl_get_glXAssociateDMPbufferSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_dmbuffer () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_dmbuffer;
}

#endif /* _DM_BUFFER_H_ */

/*
 * GLX_SGIX_swap_group
 */

static GdkGL_GLX_SGIX_swap_group _procs_GLX_SGIX_swap_group = {
  (GdkGLProc_glXJoinSwapGroupSGIX) -1
};

/* glXJoinSwapGroupSGIX */
GdkGLProc
gdk_gl_get_glXJoinSwapGroupSGIX (void)
{
  if (_procs_GLX_SGIX_swap_group.glXJoinSwapGroupSGIX == (GdkGLProc_glXJoinSwapGroupSGIX) -1)
    _procs_GLX_SGIX_swap_group.glXJoinSwapGroupSGIX =
      (GdkGLProc_glXJoinSwapGroupSGIX) gdk_gl_get_proc_address ("glXJoinSwapGroupSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXJoinSwapGroupSGIX () - %s",
               (_procs_GLX_SGIX_swap_group.glXJoinSwapGroupSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_swap_group.glXJoinSwapGroupSGIX);
}

/* Get GLX_SGIX_swap_group functions */
GdkGL_GLX_SGIX_swap_group *
gdk_gl_get_GLX_SGIX_swap_group (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_swap_group");

      if (supported)
        {
          supported &= (gdk_gl_get_glXJoinSwapGroupSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_swap_group () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_swap_group;
}

/*
 * GLX_SGIX_swap_barrier
 */

static GdkGL_GLX_SGIX_swap_barrier _procs_GLX_SGIX_swap_barrier = {
  (GdkGLProc_glXBindSwapBarrierSGIX) -1,
  (GdkGLProc_glXQueryMaxSwapBarriersSGIX) -1
};

/* glXBindSwapBarrierSGIX */
GdkGLProc
gdk_gl_get_glXBindSwapBarrierSGIX (void)
{
  if (_procs_GLX_SGIX_swap_barrier.glXBindSwapBarrierSGIX == (GdkGLProc_glXBindSwapBarrierSGIX) -1)
    _procs_GLX_SGIX_swap_barrier.glXBindSwapBarrierSGIX =
      (GdkGLProc_glXBindSwapBarrierSGIX) gdk_gl_get_proc_address ("glXBindSwapBarrierSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXBindSwapBarrierSGIX () - %s",
               (_procs_GLX_SGIX_swap_barrier.glXBindSwapBarrierSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_swap_barrier.glXBindSwapBarrierSGIX);
}

/* glXQueryMaxSwapBarriersSGIX */
GdkGLProc
gdk_gl_get_glXQueryMaxSwapBarriersSGIX (void)
{
  if (_procs_GLX_SGIX_swap_barrier.glXQueryMaxSwapBarriersSGIX == (GdkGLProc_glXQueryMaxSwapBarriersSGIX) -1)
    _procs_GLX_SGIX_swap_barrier.glXQueryMaxSwapBarriersSGIX =
      (GdkGLProc_glXQueryMaxSwapBarriersSGIX) gdk_gl_get_proc_address ("glXQueryMaxSwapBarriersSGIX");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXQueryMaxSwapBarriersSGIX () - %s",
               (_procs_GLX_SGIX_swap_barrier.glXQueryMaxSwapBarriersSGIX) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SGIX_swap_barrier.glXQueryMaxSwapBarriersSGIX);
}

/* Get GLX_SGIX_swap_barrier functions */
GdkGL_GLX_SGIX_swap_barrier *
gdk_gl_get_GLX_SGIX_swap_barrier (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SGIX_swap_barrier");

      if (supported)
        {
          supported &= (gdk_gl_get_glXBindSwapBarrierSGIX () != NULL);
          supported &= (gdk_gl_get_glXQueryMaxSwapBarriersSGIX () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SGIX_swap_barrier () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SGIX_swap_barrier;
}

/*
 * GLX_SUN_get_transparent_index
 */

static GdkGL_GLX_SUN_get_transparent_index _procs_GLX_SUN_get_transparent_index = {
  (GdkGLProc_glXGetTransparentIndexSUN) -1
};

/* glXGetTransparentIndexSUN */
GdkGLProc
gdk_gl_get_glXGetTransparentIndexSUN (void)
{
  if (_procs_GLX_SUN_get_transparent_index.glXGetTransparentIndexSUN == (GdkGLProc_glXGetTransparentIndexSUN) -1)
    _procs_GLX_SUN_get_transparent_index.glXGetTransparentIndexSUN =
      (GdkGLProc_glXGetTransparentIndexSUN) gdk_gl_get_proc_address ("glXGetTransparentIndexSUN");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetTransparentIndexSUN () - %s",
               (_procs_GLX_SUN_get_transparent_index.glXGetTransparentIndexSUN) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_SUN_get_transparent_index.glXGetTransparentIndexSUN);
}

/* Get GLX_SUN_get_transparent_index functions */
GdkGL_GLX_SUN_get_transparent_index *
gdk_gl_get_GLX_SUN_get_transparent_index (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_SUN_get_transparent_index");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetTransparentIndexSUN () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_SUN_get_transparent_index () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_SUN_get_transparent_index;
}

/*
 * GLX_MESA_copy_sub_buffer
 */

static GdkGL_GLX_MESA_copy_sub_buffer _procs_GLX_MESA_copy_sub_buffer = {
  (GdkGLProc_glXCopySubBufferMESA) -1
};

/* glXCopySubBufferMESA */
GdkGLProc
gdk_gl_get_glXCopySubBufferMESA (void)
{
  if (_procs_GLX_MESA_copy_sub_buffer.glXCopySubBufferMESA == (GdkGLProc_glXCopySubBufferMESA) -1)
    _procs_GLX_MESA_copy_sub_buffer.glXCopySubBufferMESA =
      (GdkGLProc_glXCopySubBufferMESA) gdk_gl_get_proc_address ("glXCopySubBufferMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCopySubBufferMESA () - %s",
               (_procs_GLX_MESA_copy_sub_buffer.glXCopySubBufferMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_MESA_copy_sub_buffer.glXCopySubBufferMESA);
}

/* Get GLX_MESA_copy_sub_buffer functions */
GdkGL_GLX_MESA_copy_sub_buffer *
gdk_gl_get_GLX_MESA_copy_sub_buffer (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_MESA_copy_sub_buffer");

      if (supported)
        {
          supported &= (gdk_gl_get_glXCopySubBufferMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_MESA_copy_sub_buffer () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_MESA_copy_sub_buffer;
}

/*
 * GLX_MESA_pixmap_colormap
 */

static GdkGL_GLX_MESA_pixmap_colormap _procs_GLX_MESA_pixmap_colormap = {
  (GdkGLProc_glXCreateGLXPixmapMESA) -1
};

/* glXCreateGLXPixmapMESA */
GdkGLProc
gdk_gl_get_glXCreateGLXPixmapMESA (void)
{
  if (_procs_GLX_MESA_pixmap_colormap.glXCreateGLXPixmapMESA == (GdkGLProc_glXCreateGLXPixmapMESA) -1)
    _procs_GLX_MESA_pixmap_colormap.glXCreateGLXPixmapMESA =
      (GdkGLProc_glXCreateGLXPixmapMESA) gdk_gl_get_proc_address ("glXCreateGLXPixmapMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXCreateGLXPixmapMESA () - %s",
               (_procs_GLX_MESA_pixmap_colormap.glXCreateGLXPixmapMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_MESA_pixmap_colormap.glXCreateGLXPixmapMESA);
}

/* Get GLX_MESA_pixmap_colormap functions */
GdkGL_GLX_MESA_pixmap_colormap *
gdk_gl_get_GLX_MESA_pixmap_colormap (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_MESA_pixmap_colormap");

      if (supported)
        {
          supported &= (gdk_gl_get_glXCreateGLXPixmapMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_MESA_pixmap_colormap () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_MESA_pixmap_colormap;
}

/*
 * GLX_MESA_release_buffers
 */

static GdkGL_GLX_MESA_release_buffers _procs_GLX_MESA_release_buffers = {
  (GdkGLProc_glXReleaseBuffersMESA) -1
};

/* glXReleaseBuffersMESA */
GdkGLProc
gdk_gl_get_glXReleaseBuffersMESA (void)
{
  if (_procs_GLX_MESA_release_buffers.glXReleaseBuffersMESA == (GdkGLProc_glXReleaseBuffersMESA) -1)
    _procs_GLX_MESA_release_buffers.glXReleaseBuffersMESA =
      (GdkGLProc_glXReleaseBuffersMESA) gdk_gl_get_proc_address ("glXReleaseBuffersMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXReleaseBuffersMESA () - %s",
               (_procs_GLX_MESA_release_buffers.glXReleaseBuffersMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_MESA_release_buffers.glXReleaseBuffersMESA);
}

/* Get GLX_MESA_release_buffers functions */
GdkGL_GLX_MESA_release_buffers *
gdk_gl_get_GLX_MESA_release_buffers (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_MESA_release_buffers");

      if (supported)
        {
          supported &= (gdk_gl_get_glXReleaseBuffersMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_MESA_release_buffers () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_MESA_release_buffers;
}

/*
 * GLX_MESA_set_3dfx_mode
 */

static GdkGL_GLX_MESA_set_3dfx_mode _procs_GLX_MESA_set_3dfx_mode = {
  (GdkGLProc_glXSet3DfxModeMESA) -1
};

/* glXSet3DfxModeMESA */
GdkGLProc
gdk_gl_get_glXSet3DfxModeMESA (void)
{
  if (_procs_GLX_MESA_set_3dfx_mode.glXSet3DfxModeMESA == (GdkGLProc_glXSet3DfxModeMESA) -1)
    _procs_GLX_MESA_set_3dfx_mode.glXSet3DfxModeMESA =
      (GdkGLProc_glXSet3DfxModeMESA) gdk_gl_get_proc_address ("glXSet3DfxModeMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXSet3DfxModeMESA () - %s",
               (_procs_GLX_MESA_set_3dfx_mode.glXSet3DfxModeMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_MESA_set_3dfx_mode.glXSet3DfxModeMESA);
}

/* Get GLX_MESA_set_3dfx_mode functions */
GdkGL_GLX_MESA_set_3dfx_mode *
gdk_gl_get_GLX_MESA_set_3dfx_mode (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_MESA_set_3dfx_mode");

      if (supported)
        {
          supported &= (gdk_gl_get_glXSet3DfxModeMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_MESA_set_3dfx_mode () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_MESA_set_3dfx_mode;
}

/*
 * GLX_OML_sync_control
 */

static GdkGL_GLX_OML_sync_control _procs_GLX_OML_sync_control = {
  (GdkGLProc_glXGetSyncValuesOML) -1,
  (GdkGLProc_glXGetMscRateOML) -1,
  (GdkGLProc_glXSwapBuffersMscOML) -1,
  (GdkGLProc_glXWaitForMscOML) -1,
  (GdkGLProc_glXWaitForSbcOML) -1
};

/* glXGetSyncValuesOML */
GdkGLProc
gdk_gl_get_glXGetSyncValuesOML (void)
{
  if (_procs_GLX_OML_sync_control.glXGetSyncValuesOML == (GdkGLProc_glXGetSyncValuesOML) -1)
    _procs_GLX_OML_sync_control.glXGetSyncValuesOML =
      (GdkGLProc_glXGetSyncValuesOML) gdk_gl_get_proc_address ("glXGetSyncValuesOML");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetSyncValuesOML () - %s",
               (_procs_GLX_OML_sync_control.glXGetSyncValuesOML) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_OML_sync_control.glXGetSyncValuesOML);
}

/* glXGetMscRateOML */
GdkGLProc
gdk_gl_get_glXGetMscRateOML (void)
{
  if (_procs_GLX_OML_sync_control.glXGetMscRateOML == (GdkGLProc_glXGetMscRateOML) -1)
    _procs_GLX_OML_sync_control.glXGetMscRateOML =
      (GdkGLProc_glXGetMscRateOML) gdk_gl_get_proc_address ("glXGetMscRateOML");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetMscRateOML () - %s",
               (_procs_GLX_OML_sync_control.glXGetMscRateOML) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_OML_sync_control.glXGetMscRateOML);
}

/* glXSwapBuffersMscOML */
GdkGLProc
gdk_gl_get_glXSwapBuffersMscOML (void)
{
  if (_procs_GLX_OML_sync_control.glXSwapBuffersMscOML == (GdkGLProc_glXSwapBuffersMscOML) -1)
    _procs_GLX_OML_sync_control.glXSwapBuffersMscOML =
      (GdkGLProc_glXSwapBuffersMscOML) gdk_gl_get_proc_address ("glXSwapBuffersMscOML");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXSwapBuffersMscOML () - %s",
               (_procs_GLX_OML_sync_control.glXSwapBuffersMscOML) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_OML_sync_control.glXSwapBuffersMscOML);
}

/* glXWaitForMscOML */
GdkGLProc
gdk_gl_get_glXWaitForMscOML (void)
{
  if (_procs_GLX_OML_sync_control.glXWaitForMscOML == (GdkGLProc_glXWaitForMscOML) -1)
    _procs_GLX_OML_sync_control.glXWaitForMscOML =
      (GdkGLProc_glXWaitForMscOML) gdk_gl_get_proc_address ("glXWaitForMscOML");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXWaitForMscOML () - %s",
               (_procs_GLX_OML_sync_control.glXWaitForMscOML) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_OML_sync_control.glXWaitForMscOML);
}

/* glXWaitForSbcOML */
GdkGLProc
gdk_gl_get_glXWaitForSbcOML (void)
{
  if (_procs_GLX_OML_sync_control.glXWaitForSbcOML == (GdkGLProc_glXWaitForSbcOML) -1)
    _procs_GLX_OML_sync_control.glXWaitForSbcOML =
      (GdkGLProc_glXWaitForSbcOML) gdk_gl_get_proc_address ("glXWaitForSbcOML");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXWaitForSbcOML () - %s",
               (_procs_GLX_OML_sync_control.glXWaitForSbcOML) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_OML_sync_control.glXWaitForSbcOML);
}

/* Get GLX_OML_sync_control functions */
GdkGL_GLX_OML_sync_control *
gdk_gl_get_GLX_OML_sync_control (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_OML_sync_control");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetSyncValuesOML () != NULL);
          supported &= (gdk_gl_get_glXGetMscRateOML () != NULL);
          supported &= (gdk_gl_get_glXSwapBuffersMscOML () != NULL);
          supported &= (gdk_gl_get_glXWaitForMscOML () != NULL);
          supported &= (gdk_gl_get_glXWaitForSbcOML () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_OML_sync_control () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_OML_sync_control;
}

/*
 * GLX_MESA_agp_offset
 */

static GdkGL_GLX_MESA_agp_offset _procs_GLX_MESA_agp_offset = {
  (GdkGLProc_glXGetAGPOffsetMESA) -1
};

/* glXGetAGPOffsetMESA */
GdkGLProc
gdk_gl_get_glXGetAGPOffsetMESA (void)
{
  if (_procs_GLX_MESA_agp_offset.glXGetAGPOffsetMESA == (GdkGLProc_glXGetAGPOffsetMESA) -1)
    _procs_GLX_MESA_agp_offset.glXGetAGPOffsetMESA =
      (GdkGLProc_glXGetAGPOffsetMESA) gdk_gl_get_proc_address ("glXGetAGPOffsetMESA");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXGetAGPOffsetMESA () - %s",
               (_procs_GLX_MESA_agp_offset.glXGetAGPOffsetMESA) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_MESA_agp_offset.glXGetAGPOffsetMESA);
}

/* Get GLX_MESA_agp_offset functions */
GdkGL_GLX_MESA_agp_offset *
gdk_gl_get_GLX_MESA_agp_offset (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_MESA_agp_offset");

      if (supported)
        {
          supported &= (gdk_gl_get_glXGetAGPOffsetMESA () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_MESA_agp_offset () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_MESA_agp_offset;
}

/*
 * GLX_NV_vertex_array_range
 */

static GdkGL_GLX_NV_vertex_array_range _procs_GLX_NV_vertex_array_range = {
  (GdkGLProc_glXAllocateMemoryNV) -1,
  (GdkGLProc_glXFreeMemoryNV) -1
};

/* glXAllocateMemoryNV */
GdkGLProc
gdk_gl_get_glXAllocateMemoryNV (void)
{
  if (_procs_GLX_NV_vertex_array_range.glXAllocateMemoryNV == (GdkGLProc_glXAllocateMemoryNV) -1)
    _procs_GLX_NV_vertex_array_range.glXAllocateMemoryNV =
      (GdkGLProc_glXAllocateMemoryNV) gdk_gl_get_proc_address ("glXAllocateMemoryNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXAllocateMemoryNV () - %s",
               (_procs_GLX_NV_vertex_array_range.glXAllocateMemoryNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_NV_vertex_array_range.glXAllocateMemoryNV);
}

/* glXFreeMemoryNV */
GdkGLProc
gdk_gl_get_glXFreeMemoryNV (void)
{
  if (_procs_GLX_NV_vertex_array_range.glXFreeMemoryNV == (GdkGLProc_glXFreeMemoryNV) -1)
    _procs_GLX_NV_vertex_array_range.glXFreeMemoryNV =
      (GdkGLProc_glXFreeMemoryNV) gdk_gl_get_proc_address ("glXFreeMemoryNV");

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_glXFreeMemoryNV () - %s",
               (_procs_GLX_NV_vertex_array_range.glXFreeMemoryNV) ? "supported" : "not supported"));

  return (GdkGLProc) (_procs_GLX_NV_vertex_array_range.glXFreeMemoryNV);
}

/* Get GLX_NV_vertex_array_range functions */
GdkGL_GLX_NV_vertex_array_range *
gdk_gl_get_GLX_NV_vertex_array_range (GdkGLConfig *glconfig)
{
  static gint supported = -1;

  if (supported == -1)
    {
      supported = gdk_x11_gl_query_glx_extension (glconfig, "GLX_NV_vertex_array_range");

      if (supported)
        {
          supported &= (gdk_gl_get_glXAllocateMemoryNV () != NULL);
          supported &= (gdk_gl_get_glXFreeMemoryNV () != NULL);
        }
    }

  GDK_GL_NOTE (MISC,
    g_message (" - gdk_gl_get_GLX_NV_vertex_array_range () - %s",
               (supported) ? "supported" : "not supported"));

  if (!supported)
    return NULL;

  return &_procs_GLX_NV_vertex_array_range;
}

