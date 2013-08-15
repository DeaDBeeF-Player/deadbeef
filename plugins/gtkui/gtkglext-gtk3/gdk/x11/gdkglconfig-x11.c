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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglwindow-x11.h"

#include <gdk/gdk.h>

#ifdef HAVE_LIBXMU

#include <X11/Xatom.h>  /* for XA_RGB_DEFAULT_MAP atom */

#ifdef HAVE_XMU_STDCMAP_H
#include <Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#else
#include <X11/Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#endif

#endif /* HAVE_LIBXMU */

static Display      *_gdk_x11_gl_config_impl_get_xdisplay       (GdkGLConfig *glconfig);
static int           _gdk_x11_gl_config_impl_get_screen_number  (GdkGLConfig *glconfig);
static XVisualInfo  *_gdk_x11_gl_config_impl_get_xvinfo         (GdkGLConfig *glconfig);
static GdkGLWindow  *_gdk_x11_gl_config_impl_create_gl_window   (GdkGLConfig *glconfig,
                                                                 GdkWindow   *window,
                                                                 const int   *attrib_list);
static GdkScreen    *_gdk_x11_gl_config_impl_get_screen         (GdkGLConfig *glconfig);
static gboolean      _gdk_x11_gl_config_impl_get_attrib         (GdkGLConfig *glconfig,
                                                                 int          attribute,
                                                                 int         *value);
static GdkVisual    *_gdk_x11_gl_config_impl_get_visual         (GdkGLConfig *glconfig);
static gint          _gdk_x11_gl_config_impl_get_depth          (GdkGLConfig *glconfig);

G_DEFINE_TYPE (GdkGLConfigImplX11,              \
               gdk_gl_config_impl_x11,          \
               GDK_TYPE_GL_CONFIG_IMPL)

static void
gdk_gl_config_impl_x11_init (GdkGLConfigImplX11 *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  self->xdisplay = NULL;
  self->screen_num = 0;
  self->xvinfo = NULL;
  self->screen = 0;
}

static void
gdk_gl_config_impl_x11_finalize (GObject *object)
{
  GdkGLConfigImplX11 *x11_impl = GDK_GL_CONFIG_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  XFree (x11_impl->xvinfo);

  G_OBJECT_CLASS (gdk_gl_config_impl_x11_parent_class)->finalize (object);
}

static void
gdk_gl_config_impl_x11_class_init (GdkGLConfigImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  klass->get_xdisplay      = _gdk_x11_gl_config_impl_get_xdisplay;
  klass->get_screen_number = _gdk_x11_gl_config_impl_get_screen_number;
  klass->get_xvinfo        = _gdk_x11_gl_config_impl_get_xvinfo;

  klass->parent_class.create_gl_window = _gdk_x11_gl_config_impl_create_gl_window;
  klass->parent_class.get_screen       = _gdk_x11_gl_config_impl_get_screen;
  klass->parent_class.get_attrib       = _gdk_x11_gl_config_impl_get_attrib;
  klass->parent_class.get_visual       = _gdk_x11_gl_config_impl_get_visual;
  klass->parent_class.get_depth        = _gdk_x11_gl_config_impl_get_depth;

  object_class->finalize = gdk_gl_config_impl_x11_finalize;
}

static void
gdk_x11_gl_config_impl_init_attrib (GdkGLConfig *glconfig)
{
  GdkGLConfigImplX11 *x11_impl;
  int value;

  x11_impl = GDK_GL_CONFIG_IMPL_X11 (glconfig->impl);

#define _GET_CONFIG(__attrib) \
  glXGetConfig (x11_impl->xdisplay, x11_impl->xvinfo, __attrib, &value)

  /* RGBA mode? */
  _GET_CONFIG (GLX_RGBA);
  glconfig->impl->is_rgba = value ? TRUE : FALSE;

  /* Layer plane. */
  _GET_CONFIG (GLX_LEVEL);
  glconfig->impl->layer_plane = value;

  /* Double buffering is supported? */
  _GET_CONFIG (GLX_DOUBLEBUFFER);
  glconfig->impl->is_double_buffered = value ? TRUE : FALSE;

  /* Stereo is supported? */
  _GET_CONFIG (GLX_STEREO);
  glconfig->impl->is_stereo = value ? TRUE : FALSE;

  /* Number of aux buffers */
  _GET_CONFIG (GLX_AUX_BUFFERS);
  glconfig->impl->n_aux_buffers = value;

  /* Has alpha bits? */
  _GET_CONFIG (GLX_ALPHA_SIZE);
  glconfig->impl->has_alpha = value ? TRUE : FALSE;

  /* Has depth buffer? */
  _GET_CONFIG (GLX_DEPTH_SIZE);
  glconfig->impl->has_depth_buffer = value ? TRUE : FALSE;

  /* Has stencil buffer? */
  _GET_CONFIG (GLX_STENCIL_SIZE);
  glconfig->impl->has_stencil_buffer = value ? TRUE : FALSE;

  /* Has accumulation buffer? */
  _GET_CONFIG (GLX_ACCUM_RED_SIZE);
  glconfig->impl->has_accum_buffer = value ? TRUE : FALSE;

  /* Number of multisample buffers (not supported yet) */
  glconfig->impl->n_sample_buffers = 0;

#undef _GET_CONFIG
}

static int *
glx_attrib_list_from_attrib_list (const gint *attrib_list, gsize n_attribs)
{
  static const guchar has_param[] =
    {
      [GDK_GL_BUFFER_SIZE]      = 1,
      [GDK_GL_LEVEL]            = 1,
      [GDK_GL_AUX_BUFFERS]      = 1,
      [GDK_GL_RED_SIZE]         = 1,
      [GDK_GL_GREEN_SIZE]       = 1,
      [GDK_GL_BLUE_SIZE]        = 1,
      [GDK_GL_ALPHA_SIZE]       = 1,
      [GDK_GL_DEPTH_SIZE]       = 1,
      [GDK_GL_STENCIL_SIZE]     = 1,
      [GDK_GL_ACCUM_RED_SIZE]   = 1,
      [GDK_GL_ACCUM_GREEN_SIZE] = 1,
      [GDK_GL_ACCUM_BLUE_SIZE]  = 1,
      [GDK_GL_ACCUM_ALPHA_SIZE] = 1
    };

  static const int glx_attrib_of_attrib[] =
    {
      [GDK_GL_USE_GL]           = GLX_USE_GL,
      [GDK_GL_BUFFER_SIZE]      = GLX_BUFFER_SIZE,
      [GDK_GL_LEVEL]            = GLX_LEVEL,
      [GDK_GL_RGBA]             = GLX_RGBA,
      [GDK_GL_DOUBLEBUFFER]     = GLX_DOUBLEBUFFER,
      [GDK_GL_STEREO]           = GLX_STEREO,
      [GDK_GL_AUX_BUFFERS]      = GLX_AUX_BUFFERS,
      [GDK_GL_RED_SIZE]         = GLX_RED_SIZE,
      [GDK_GL_GREEN_SIZE]       = GLX_GREEN_SIZE,
      [GDK_GL_BLUE_SIZE]        = GLX_BLUE_SIZE,
      [GDK_GL_ALPHA_SIZE]       = GLX_ALPHA_SIZE,
      [GDK_GL_DEPTH_SIZE]       = GLX_DEPTH_SIZE,
      [GDK_GL_STENCIL_SIZE]     = GLX_STENCIL_SIZE,
      [GDK_GL_ACCUM_RED_SIZE]   = GLX_ACCUM_RED_SIZE,
      [GDK_GL_ACCUM_GREEN_SIZE] = GLX_ACCUM_GREEN_SIZE,
      [GDK_GL_ACCUM_BLUE_SIZE]  = GLX_ACCUM_BLUE_SIZE,
      [GDK_GL_ACCUM_ALPHA_SIZE] = GLX_ACCUM_ALPHA_SIZE
    };

  int *glx_attrib_list;
  gsize attrib_index;
  gsize glx_attrib_index;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  glx_attrib_list = g_malloc( sizeof(*glx_attrib_list)*(n_attribs+3) );

  if (!glx_attrib_list)
    goto err_g_malloc;

  for (attrib_index = 0, glx_attrib_index = 0; (attrib_index < n_attribs) && attrib_list[attrib_index]; ++attrib_index)
    {
      switch (attrib_list[attrib_index])
        {
          case GDK_GL_USE_GL:
            /* legacy from GLX 1.2 and always true; will be removed */
          case GDK_GL_RGBA:
            /* not supported anymore */
            break;

          default:
            glx_attrib_list[glx_attrib_index++] = glx_attrib_of_attrib[attrib_list[attrib_index]];
            if ( has_param[attrib_list[attrib_index]] )
              {
                ++attrib_index;
                if (attrib_index == n_attribs)
                  goto err_n_attribs;
                glx_attrib_list[glx_attrib_index++] = attrib_list[attrib_index];
              }
            break;
        }
    }

  glx_attrib_list[glx_attrib_index++] = GLX_RGBA;
  glx_attrib_list[glx_attrib_index++] = GLX_USE_GL;
  glx_attrib_list[glx_attrib_index++] = None;

  return glx_attrib_list;

err_n_attribs:
  g_free(glx_attrib_list);
err_g_malloc:
  return NULL;
}

static GdkGLConfig *
gdk_x11_gl_config_impl_new_common (GdkGLConfig *glconfig,
                                   GdkScreen *screen,
                                   const int *attrib_list,
                                   gsize n_attribs)
{
  GdkGLConfigImplX11 *x11_impl;

  Display *xdisplay;
  int screen_num;
  int *glx_attrib_list;
  XVisualInfo *xvinfo;
  int is_rgba;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  xdisplay = GDK_SCREEN_XDISPLAY (screen);
  screen_num = GDK_SCREEN_XNUMBER (screen);

  GDK_GL_NOTE (MISC, _gdk_x11_gl_print_glx_info (xdisplay, screen_num));

  /*
   * Find an OpenGL-capable visual.
   */

  glx_attrib_list = glx_attrib_list_from_attrib_list(attrib_list, n_attribs);

  if (glx_attrib_list == NULL)
    goto err_glx_attrib_list_from_attrib_list;

  GDK_GL_NOTE_FUNC_IMPL ("glXChooseVisual");

  xvinfo = glXChooseVisual (xdisplay, screen_num, glx_attrib_list);

  if (xvinfo == NULL)
    goto err_glXChooseVisual;

  GDK_GL_NOTE (MISC,
    g_message (" -- glXChooseVisual: screen number = %d", xvinfo->screen));
  GDK_GL_NOTE (MISC,
    g_message (" -- glXChooseVisual: visual id = 0x%lx", xvinfo->visualid));

  /*
   * Instantiate the GdkGLConfigImplX11 object.
   */

  x11_impl = g_object_new (GDK_TYPE_GL_CONFIG_IMPL_X11, NULL);

  x11_impl->xdisplay = xdisplay;
  x11_impl->screen_num = screen_num;
  x11_impl->xvinfo = xvinfo;

  x11_impl->screen = screen;

  /* RGBA mode? */
  glXGetConfig (xdisplay, xvinfo, GLX_RGBA, &is_rgba);

  /*
   * Init GdkGLConfig
   */
  glconfig->impl = GDK_GL_CONFIG_IMPL (x11_impl);

  /*
   * Init configuration attributes.
   */

  gdk_x11_gl_config_impl_init_attrib (glconfig);

  g_free(glx_attrib_list);

  return glconfig;

err_glXChooseVisual:
  g_free(glx_attrib_list);
err_glx_attrib_list_from_attrib_list:
  return NULL;
}

GdkGLConfig *
_gdk_x11_gl_config_impl_new (GdkGLConfig *glconfig,
                             const int *attrib_list,
                             gsize n_attribs)
{
  GdkScreen *screen;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG(glconfig), NULL);
  g_return_val_if_fail (attrib_list != NULL, NULL);

  screen = gdk_screen_get_default ();

  return gdk_x11_gl_config_impl_new_common (glconfig, screen, attrib_list, n_attribs);
}

GdkGLConfig *
_gdk_x11_gl_config_impl_new_for_screen (GdkGLConfig *glconfig,
                                        GdkScreen *screen,
                                        const int *attrib_list,
                                        gsize n_attribs)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG(glconfig), NULL);
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (attrib_list != NULL, NULL);

  return gdk_x11_gl_config_impl_new_common (glconfig, screen, attrib_list, n_attribs);
}

/*
 * XVisualInfo returned by this function should be freed by XFree ().
 */
static XVisualInfo *
gdk_x11_gl_get_xvinfo (Display  *xdisplay,
                       int       screen_num,
                       VisualID  xvisualid)
{
  XVisualInfo xvinfo_template;
  XVisualInfo *xvinfo_list;
  int nitems_return;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  xvinfo_template.visualid = xvisualid;
  xvinfo_template.screen = screen_num;

  xvinfo_list = XGetVisualInfo (xdisplay,
                                VisualIDMask | VisualScreenMask,
                                &xvinfo_template,
                                &nitems_return);

  /* Returned XVisualInfo needs to be unique */
  g_assert (xvinfo_list != NULL && nitems_return == 1);

  return xvinfo_list;
}

static GdkGLConfig *
gdk_x11_gl_config_impl_new_from_visualid_common (GdkGLConfig *glconfig,
                                                 GdkScreen   *screen,
                                                 VisualID     xvisualid)
{
  GdkGLConfigImplX11 *x11_impl;

  Display *xdisplay;
  int screen_num;
  XVisualInfo *xvinfo;
  int is_rgba;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  xdisplay = GDK_SCREEN_XDISPLAY (screen);
  screen_num = GDK_SCREEN_XNUMBER (screen);

  GDK_GL_NOTE (MISC,
               g_message (" -- GLX_VENDOR     : %s",
                          glXGetClientString (xdisplay, GLX_VENDOR)));
  GDK_GL_NOTE (MISC,
               g_message (" -- GLX_VERSION    : %s",
                          glXGetClientString (xdisplay, GLX_VERSION)));
  GDK_GL_NOTE (MISC,
               g_message (" -- GLX_EXTENSIONS : %s",
                          glXGetClientString (xdisplay, GLX_EXTENSIONS)));

  /*
   * Get XVisualInfo.
   */

  xvinfo = gdk_x11_gl_get_xvinfo (xdisplay, screen_num, xvisualid);
  if (xvinfo == NULL)
    return NULL;

  GDK_GL_NOTE (MISC,
    g_message (" -- gdk_x11_gl_get_xvinfo: screen number = %d", xvinfo->screen));
  GDK_GL_NOTE (MISC,
    g_message (" -- gdk_x11_gl_get_xvinfo: visual id = 0x%lx", xvinfo->visualid));

  /*
   * Instantiate the GdkGLConfigImplX11 object.
   */

  x11_impl = g_object_new (GDK_TYPE_GL_CONFIG_IMPL_X11, NULL);

  g_return_val_if_fail(x11_impl != NULL, NULL);

  x11_impl->xdisplay = xdisplay;
  x11_impl->screen_num = screen_num;
  x11_impl->xvinfo = xvinfo;
  x11_impl->screen = screen;

  /* RGBA mode? */
  glXGetConfig (xdisplay, xvinfo, GLX_RGBA, &is_rgba);

  /*
   * Init GdkGLConfig
   */

  glconfig->impl = GDK_GL_CONFIG_IMPL (x11_impl);

  /*
   * Init configuration attributes.
   */

  gdk_x11_gl_config_impl_init_attrib (glconfig);

  return glconfig;
}

GdkGLConfig *
_gdk_x11_gl_config_impl_new_from_visualid_for_screen (GdkGLConfig *glconfig,
                                                      GdkScreen   *screen,
                                                      VisualID     xvisualid)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG(glconfig), NULL);
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return gdk_x11_gl_config_impl_new_from_visualid_common (glconfig, screen, xvisualid);
}

static Display *
_gdk_x11_gl_config_impl_get_xdisplay (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->xdisplay;
}

static int
_gdk_x11_gl_config_impl_get_screen_number (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), 0);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->screen_num;
}

static XVisualInfo *
_gdk_x11_gl_config_impl_get_xvinfo (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->xvinfo;
}

static GdkGLWindow *
_gdk_x11_gl_config_impl_create_gl_window (GdkGLConfig *glconfig,
                                          GdkWindow   *window,
                                          const int   *attrib_list)
{
  GdkGLWindow *glwindow;
  GdkGLWindow *impl;

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  glwindow = g_object_new (GDK_TYPE_X11_GL_WINDOW, NULL);

  g_return_val_if_fail(glwindow != NULL, NULL);

  impl = _gdk_x11_gl_window_impl_new(glwindow,
                                     glconfig,
                                     window,
                                     attrib_list);
  if (impl == NULL)
    g_object_unref(glwindow);

  g_return_val_if_fail(impl != NULL, NULL);

  return glwindow;
}

static GdkScreen *
_gdk_x11_gl_config_impl_get_screen (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->screen;
}

static gboolean
_gdk_x11_gl_config_impl_get_attrib (GdkGLConfig *glconfig,
                                    int          attribute,
                                    int         *value)
{
  GdkGLConfigImplX11 *x11_impl;
  int ret;

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), FALSE);

  x11_impl = GDK_GL_CONFIG_IMPL_X11 (glconfig->impl);

  ret = glXGetConfig (x11_impl->xdisplay, x11_impl->xvinfo, attribute, value);

  return (ret == Success);
}

static GdkVisual *
_gdk_x11_gl_config_impl_get_visual (GdkGLConfig *glconfig)
{
  GdkGLConfigImplX11 *x11_impl;

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), NULL);

  x11_impl = GDK_GL_CONFIG_IMPL_X11 (glconfig->impl);

  return gdk_x11_screen_lookup_visual(x11_impl->screen, x11_impl->xvinfo->visualid);
}

static gint
_gdk_x11_gl_config_impl_get_depth (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), 0);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig->impl)->xvinfo->depth;
}
