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

#include "gdkglprivate.h"
#include "gdkglconfig.h"

#include <gdk/gdk.h>

#ifdef GDKGLEXT_WINDOWING_X11
#include "x11/gdkx11glconfig.h"
#include "x11/gdkglconfig-x11.h"
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
#include "win32/gdkwin32glconfig.h"
#include "win32/gdkglconfig-win32.h"
#endif

G_DEFINE_TYPE (GdkGLConfig,     \
               gdk_gl_config,   \
               G_TYPE_OBJECT)

static void
gdk_gl_config_init (GdkGLConfig *self)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static void
gdk_gl_config_class_init (GdkGLConfigClass *klass)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();
}

static GdkGLConfig *
gdk_gl_config_new_ci (GdkScreen       *screen,
                      GdkGLConfigMode  mode)
{
  GdkGLConfig *glconfig = NULL;
  static const int buf_size_list[] = { 16, 12, 8, 4, 2, 1, 0 };
  int list[32];
  int n = 0;
  int i;

  list[n++] = GDK_GL_BUFFER_SIZE;
  list[n++] = 1;
  if (mode & GDK_GL_MODE_DOUBLE)
    {
      list[n++] = GDK_GL_DOUBLEBUFFER;
    }
  if (mode & GDK_GL_MODE_STEREO)
    {
      list[n++] = GDK_GL_STEREO;
    }
  if (mode & GDK_GL_MODE_DEPTH)
    {
      list[n++] = GDK_GL_DEPTH_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_STENCIL)
    {
      list[n++] = GDK_GL_STENCIL_SIZE;
      list[n++] = 1;
    }

  /* from GLUT */
  /* glXChooseVisual specify GLX_BUFFER_SIZE prefers the
     "smallest index buffer of at least the specified size".
     This would be reasonable if GLUT allowed the user to
     specify the required buffe size, but GLUT's display mode
     is too simplistic (easy to use?). GLUT should try to find
     the "largest".  So start with a large buffer size and
     shrink until we find a matching one that exists. */

  for (i = 0; buf_size_list[i]; i++)
    {
      /* XXX Assumes list[1] is where GDK_GL_BUFFER_SIZE parameter is. */
      list[1] = buf_size_list[i];

      glconfig = gdk_gl_config_new_for_screen (screen, list, n);

      if (glconfig != NULL)
        return glconfig;
    }

  return NULL;
}

static GdkGLConfig *
gdk_gl_config_new_rgb (GdkScreen       *screen,
                       GdkGLConfigMode  mode)
{
  int list[32];
  int n = 0;

  list[n++] = GDK_GL_RGBA;
  list[n++] = GDK_GL_RED_SIZE;
  list[n++] = 1;
  list[n++] = GDK_GL_GREEN_SIZE;
  list[n++] = 1;
  list[n++] = GDK_GL_BLUE_SIZE;
  list[n++] = 1;
  if (mode & GDK_GL_MODE_ALPHA)
    {
      list[n++] = GDK_GL_ALPHA_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_DOUBLE)
    {
      list[n++] = GDK_GL_DOUBLEBUFFER;
    }
  if (mode & GDK_GL_MODE_STEREO)
    {
      list[n++] = GDK_GL_STEREO;
    }
  if (mode & GDK_GL_MODE_DEPTH)
    {
      list[n++] = GDK_GL_DEPTH_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_STENCIL)
    {
      list[n++] = GDK_GL_STENCIL_SIZE;
      list[n++] = 1;
    }
  if (mode & GDK_GL_MODE_ACCUM)
    {
      list[n++] = GDK_GL_ACCUM_RED_SIZE;
      list[n++] = 1;
      list[n++] = GDK_GL_ACCUM_GREEN_SIZE;
      list[n++] = 1;
      list[n++] = GDK_GL_ACCUM_BLUE_SIZE;
      list[n++] = 1;
      if (mode & GDK_GL_MODE_ALPHA)
        {
          list[n++] = GDK_GL_ACCUM_ALPHA_SIZE;
          list[n++] = 1;
        }
    }

  return gdk_gl_config_new_for_screen (screen, list, n);
}

static GdkGLConfig *
gdk_gl_config_new_by_mode_common (GdkScreen       *screen,
                                  GdkGLConfigMode  mode)
{
  GdkGLConfig *glconfig;

#define _GL_CONFIG_NEW_BY_MODE(__screen, __mode)        \
  ( ((__mode) & GDK_GL_MODE_INDEX) ?                    \
    gdk_gl_config_new_ci (__screen, __mode) :           \
    gdk_gl_config_new_rgb (__screen, __mode) )

  glconfig = _GL_CONFIG_NEW_BY_MODE (screen, mode);
  if (glconfig == NULL)
    {
      /* Fallback cases when can't get exactly what was asked for... */
      if (!(mode & GDK_GL_MODE_DOUBLE))
        {
          /* If we can't find a single buffered visual, try looking
             for a double buffered visual.  We can treat a double
             buffered visual as a single buffered visual by changing
             the draw buffer to GL_FRONT and treating any swap
             buffers as no-ops. */
          mode |= GDK_GL_MODE_DOUBLE;
          glconfig = _GL_CONFIG_NEW_BY_MODE (screen, mode);
          if (glconfig != NULL)
            glconfig->impl->as_single_mode = TRUE;
        }
    }

#undef _GL_CONFIG_NEW_BY_MODE

  return glconfig;
}

/**
 * gdk_gl_config_new_by_mode:
 * @mode: display mode bit mask.
 *
 * Returns an OpenGL frame buffer configuration that match the specified
 * display mode.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_config_new_by_mode (GdkGLConfigMode mode)
{
  GdkScreen *screen;

  screen = gdk_screen_get_default ();

  return gdk_gl_config_new_by_mode_common (screen, mode);
}

/**
 * gdk_gl_config_new_by_mode_for_screen:
 * @screen: target screen.
 * @mode: display mode bit mask.
 *
 * Returns an OpenGL frame buffer configuration that match the specified
 * display mode.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_config_new_by_mode_for_screen (GdkScreen       *screen,
                                      GdkGLConfigMode  mode)
{
  return gdk_gl_config_new_by_mode_common (screen, mode);
}

/**
 * gdk_gl_config_get_layer_plane:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets the layer plane (level) of the frame buffer.
 * Zero is the default frame buffer.
 * Positive layer planes correspond to frame buffers that overlay the default
 * buffer, and negative layer planes correspond to frame buffers that underlie
 * the default frame buffer.
 *
 * Return value: layer plane.
 **/
gint
gdk_gl_config_get_layer_plane (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), 0);

  return glconfig->impl->layer_plane;
}

/**
 * gdk_gl_config_get_n_aux_buffers:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets the number of auxiliary color buffers.
 *
 * Return value: number of auxiliary color buffers.
 **/
gint
gdk_gl_config_get_n_aux_buffers (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), 0);

  return glconfig->impl->n_aux_buffers;
}

/**
 * gdk_gl_config_get_n_sample_buffers:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets the number of multisample buffers.
 *
 * Return value: number of multisample buffers.
 **/
gint
gdk_gl_config_get_n_sample_buffers (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), 0);

  return glconfig->impl->n_sample_buffers;
}

/**
 * gdk_gl_config_is_rgba:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configured frame buffer is RGBA mode.
 *
 * Return value: TRUE if the configured frame buffer is RGBA mode, FALSE
 *               otherwise.
 **/
gboolean
gdk_gl_config_is_rgba (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return glconfig->impl->is_rgba;
}

/**
 * gdk_gl_config_is_double_buffered:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configuration supports the double-buffered visual.
 *
 * Return value: TRUE if the double-buffered visual is supported, FALSE
 *               otherwise.
 **/
gboolean
gdk_gl_config_is_double_buffered (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return (glconfig->impl->is_double_buffered && (!glconfig->impl->as_single_mode));
}

/**
 * gdk_gl_config_is_stereo:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configuration supports the stereo visual.
 *
 * Return value: TRUE if the stereo visual is supported, FALSE otherwise.
 **/
gboolean
gdk_gl_config_is_stereo (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return glconfig->impl->is_stereo;
}

/**
 * gdk_gl_config_has_alpha:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configured color buffer has alpha bits.
 *
 * Return value: TRUE if the color buffer has alpha bits, FALSE otherwise.
 **/
gboolean
gdk_gl_config_has_alpha (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return glconfig->impl->has_alpha;
}

/**
 * gdk_gl_config_has_depth_buffer:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configured frame buffer has depth buffer.
 *
 * Return value: TRUE if the frame buffer has depth buffer, FALSE otherwise.
 **/
gboolean
gdk_gl_config_has_depth_buffer (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return glconfig->impl->has_depth_buffer;
}

/**
 * gdk_gl_config_has_stencil_buffer:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configured frame buffer has stencil buffer.
 *
 * Return value: TRUE if the frame buffer has stencil buffer, FALSE otherwise.
 **/
gboolean
gdk_gl_config_has_stencil_buffer (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return glconfig->impl->has_stencil_buffer;
}

/**
 * gdk_gl_config_has_accum_buffer:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the configured frame buffer has accumulation buffer.
 *
 * Return value: TRUE if the frame buffer has accumulation buffer, FALSE
 *               otherwise.
 **/
gboolean
gdk_gl_config_has_accum_buffer (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return glconfig->impl->has_accum_buffer;
}

/**
 * gdk_gl_config_new:
 * @attrib_list: (array length=n_attribs): a list of attribute/value pairs.
 * @n_attribs: the number of attributes and values in attrib_list.
 *
 * Returns an OpenGL frame buffer configuration that match the specified
 * attributes.
 *
 * attrib_list is a int array that contains the attribute/value pairs.
 * Available attributes are:
 * GDK_GL_USE_GL, GDK_GL_BUFFER_SIZE, GDK_GL_LEVEL, GDK_GL_RGBA,
 * GDK_GL_DOUBLEBUFFER, GDK_GL_STEREO, GDK_GL_AUX_BUFFERS,
 * GDK_GL_RED_SIZE, GDK_GL_GREEN_SIZE, GDK_GL_BLUE_SIZE, GDK_GL_ALPHA_SIZE,
 * GDK_GL_DEPTH_SIZE, GDK_GL_STENCIL_SIZE, GDK_GL_ACCUM_RED_SIZE,
 * GDK_GL_ACCUM_GREEN_SIZE, GDK_GL_ACCUM_BLUE_SIZE, GDK_GL_ACCUM_ALPHA_SIZE.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_config_new (const int *attrib_list, gsize n_attribs)
{
  return gdk_gl_config_new_for_display(gdk_display_get_default(),
                                       attrib_list,
                                       n_attribs);
}

/**
 * gdk_gl_config_new_for_display:
 * @screen: target display.
 * @attrib_list: (array length=n_attribs): a list of attribute/value pairs.
 * @n_attribs: the number of attributes and values in attrib_list.
 *
 * Returns an OpenGL frame buffer configuration that match the specified
 * attributes.
 *
 * attrib_list is a int array that contains the attribute/value pairs.
 * Available attributes are:
 * GDK_GL_USE_GL, GDK_GL_BUFFER_SIZE, GDK_GL_LEVEL, GDK_GL_RGBA,
 * GDK_GL_DOUBLEBUFFER, GDK_GL_STEREO, GDK_GL_AUX_BUFFERS,
 * GDK_GL_RED_SIZE, GDK_GL_GREEN_SIZE, GDK_GL_BLUE_SIZE, GDK_GL_ALPHA_SIZE,
 * GDK_GL_DEPTH_SIZE, GDK_GL_STENCIL_SIZE, GDK_GL_ACCUM_RED_SIZE,
 * GDK_GL_ACCUM_GREEN_SIZE, GDK_GL_ACCUM_BLUE_SIZE, GDK_GL_ACCUM_ALPHA_SIZE.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_config_new_for_display (GdkDisplay *display,
                               const int *attrib_list,
                               gsize n_attribs)
{
  GdkGLConfig *glconfig = NULL;

#ifdef GDKGLEXT_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
    {
      glconfig = gdk_x11_gl_config_new_for_display(display,
                                                   attrib_list,
                                                   n_attribs);
    }
  else
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
    {
      glconfig = gdk_win32_gl_config_new_for_display(display,
                                                     attrib_list,
                                                     n_attribs);
    }
  else
#endif
    {
      g_warning("Unsupported GDK backend");
    }

  return glconfig;
}

/**
 * gdk_gl_config_new_for_screen:
 * @screen: target screen.
 * @attrib_list: (array length=n_attribs): a list of attribute/value pairs.
 * @n_attribs: the number of attributes and values in attrib_list.
 *
 * Returns an OpenGL frame buffer configuration that match the specified
 * attributes.
 *
 * attrib_list is a int array that contains the attribute/value pairs.
 * Available attributes are:
 * GDK_GL_USE_GL, GDK_GL_BUFFER_SIZE, GDK_GL_LEVEL, GDK_GL_RGBA,
 * GDK_GL_DOUBLEBUFFER, GDK_GL_STEREO, GDK_GL_AUX_BUFFERS,
 * GDK_GL_RED_SIZE, GDK_GL_GREEN_SIZE, GDK_GL_BLUE_SIZE, GDK_GL_ALPHA_SIZE,
 * GDK_GL_DEPTH_SIZE, GDK_GL_STENCIL_SIZE, GDK_GL_ACCUM_RED_SIZE,
 * GDK_GL_ACCUM_GREEN_SIZE, GDK_GL_ACCUM_BLUE_SIZE, GDK_GL_ACCUM_ALPHA_SIZE.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_config_new_for_screen (GdkScreen *screen,
                              const int *attrib_list,
                              gsize n_attribs)
{
  GdkDisplay *display;
  GdkGLConfig *glconfig = NULL;

  /* The linker returns undefined symbol '_gdk_win32_screen_get_type'
   * for win32 builds when using GDK_IS_WIN32_SCREEN. Thus we lookup
   * the screen's display and test the display instead.
   */

  display = gdk_screen_get_display(screen);
  g_return_val_if_fail(display != NULL, NULL);

#ifdef GDKGLEXT_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY(display))
    {
      glconfig = gdk_x11_gl_config_new_for_screen(screen,
                                                  attrib_list,
                                                  n_attribs);
    }
  else
#endif
#ifdef GDKGLEXT_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY(display))
    {
      glconfig = gdk_win32_gl_config_new_for_screen(screen,
                                                    attrib_list,
                                                    n_attribs);
    }
  else
#endif
    {
      g_warning("Unsupported GDK backend");
    }

  return glconfig;
}

/**
 * gdk_gl_config_get_screen:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets #GdkScreen.
 *
 * Return value: the #GdkScreen.
 **/
GdkScreen *
gdk_gl_config_get_screen (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return GDK_GL_CONFIG_IMPL_GET_CLASS (glconfig->impl)->get_screen (glconfig);
}

/**
 * gdk_gl_config_get_attrib:
 * @glconfig: a #GdkGLConfig.
 * @attribute: the attribute to be returned.
 * @value: returns the requested value.
 *
 * Gets information about a OpenGL frame buffer configuration.
 *
 * Return value: TRUE if it succeeded, FALSE otherwise.
 **/
gboolean
gdk_gl_config_get_attrib (GdkGLConfig *glconfig,
                          int          attribute,
                          int         *value)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return GDK_GL_CONFIG_IMPL_GET_CLASS (glconfig->impl)->get_attrib (glconfig, attribute, value);
}

/**
 * gdk_gl_config_get_visual:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets the #GdkVisual that is appropriate for the OpenGL frame buffer
 * configuration.
 *
 * Return value: the appropriate #GdkVisual.
 **/
GdkVisual *
gdk_gl_config_get_visual (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return GDK_GL_CONFIG_IMPL_GET_CLASS (glconfig->impl)->get_visual (glconfig);
}

/**
 * gdk_gl_config_get_depth:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets the color depth of the OpenGL-capable visual.
 *
 * Return value: number of bits per pixel
 **/
gint
gdk_gl_config_get_depth (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG (glconfig), FALSE);

  return GDK_GL_CONFIG_IMPL_GET_CLASS (glconfig->impl)->get_depth (glconfig);
}
