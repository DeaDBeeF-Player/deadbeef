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

#include "gdkglprivate.h"
#include "gdkglconfig.h"

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
#include <gdk/gdkscreen.h>
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

gboolean _gdk_gl_config_no_standard_colormap = FALSE;

static void gdk_gl_config_class_init (GdkGLConfigClass *klass);
static void gdk_gl_config_finalize   (GObject          *object);

static gpointer parent_class = NULL;

GType
gdk_gl_config_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLConfigClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_config_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLConfig),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "GdkGLConfig",
                                     &type_info, 0);
    }

  return type;
}

static void
gdk_gl_config_class_init (GdkGLConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_config_finalize;
}

static void
gdk_gl_config_finalize (GObject *object)
{
  GDK_GL_NOTE_FUNC_PRIVATE ();

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
  list[n] = GDK_GL_ATTRIB_LIST_NONE;

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

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
      glconfig = gdk_gl_config_new_for_screen (screen, list);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
      glconfig = gdk_gl_config_new (list);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

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
  list[n] = GDK_GL_ATTRIB_LIST_NONE;

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  return gdk_gl_config_new_for_screen (screen, list);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  return gdk_gl_config_new (list);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
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
            glconfig->as_single_mode = TRUE;
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

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  screen = gdk_screen_get_default ();
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  screen = NULL;
#endif

  return gdk_gl_config_new_by_mode_common (screen, mode);
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

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

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

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

  return glconfig->layer_plane;
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

  return glconfig->n_aux_buffers;
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

  return glconfig->n_sample_buffers;
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

  return glconfig->is_rgba;
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

  return (glconfig->is_double_buffered && (!glconfig->as_single_mode));
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

  return glconfig->is_stereo;
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

  return glconfig->has_alpha;
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

  return glconfig->has_depth_buffer;
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

  return glconfig->has_stencil_buffer;
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

  return glconfig->has_accum_buffer;
}
