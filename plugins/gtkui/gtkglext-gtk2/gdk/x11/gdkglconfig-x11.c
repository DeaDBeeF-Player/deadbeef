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
#include "gdkgloverlay-x11.h"
#include "gdkglconfig-x11.h"

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
#include <gdk/gdkscreen.h>
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

#ifdef HAVE_LIBXMU

#include <X11/Xatom.h>  /* for XA_RGB_DEFAULT_MAP atom */

#ifdef HAVE_XMU_STDCMAP_H
#include <Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#else
#include <X11/Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#endif

#endif /* HAVE_LIBXMU */

static void gdk_gl_config_impl_x11_class_init (GdkGLConfigImplX11Class *klass);
static void gdk_gl_config_impl_x11_finalize   (GObject                 *object);

static gpointer parent_class = NULL;

GType
gdk_gl_config_impl_x11_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info = {
        sizeof (GdkGLConfigImplX11Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gl_config_impl_x11_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                   /* class_data */
        sizeof (GdkGLConfigImplX11),
        0,                      /* n_preallocs */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GDK_TYPE_GL_CONFIG,
                                     "GdkGLConfigImplX11",
                                     &type_info, 0);
    }

  return type;
}

static void
gdk_gl_config_impl_x11_class_init (GdkGLConfigImplX11Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gl_config_impl_x11_finalize;
}

static void
gdk_gl_config_impl_x11_finalize (GObject *object)
{
  GdkGLConfigImplX11 *impl = GDK_GL_CONFIG_IMPL_X11 (object);

  GDK_GL_NOTE_FUNC_PRIVATE ();

  XFree (impl->xvinfo);

  g_object_unref (G_OBJECT (impl->colormap));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* 
 * Get standard RGB colormap
 */

#ifdef HAVE_GDK_X11_COLORMAP_FOREIGN_NEW

static GdkColormap *
gdk_gl_config_get_std_rgb_colormap (GdkScreen   *screen,
                                    XVisualInfo *xvinfo,
                                    gboolean     is_mesa_glx)
{
  GdkDisplay *display;
  Display *xdisplay;
  int screen_num;
  Window xroot_window;
  Status status;
  Colormap xcolormap = None;
  XStandardColormap *standard_cmaps;
  int i, num_cmaps;
  GdkVisual *visual;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  display = gdk_screen_get_display (screen);
  xdisplay = GDK_DISPLAY_XDISPLAY (display);
  screen_num = xvinfo->screen;
  xroot_window = RootWindow (xdisplay, screen_num);

  /*
   * (ripped from GLUT)
   * Hewlett-Packard supports a feature called "HP Color Recovery".
   * Mesa has code to use HP Color Recovery.  For Mesa to use this feature,
   * the atom _HP_RGB_SMOOTH_MAP_LIST must be defined on the root window AND
   * the colormap obtainable by XGetRGBColormaps for that atom must be set on
   * the window.  If that colormap is not set, the output will look stripy.
   */

  if (is_mesa_glx &&
      xvinfo->visual->class == TrueColor &&
      xvinfo->depth == 8)
    {
      Atom xa_hp_cr_maps;

      GDK_GL_NOTE (MISC,
        g_message (" -- Try to find a standard RGB colormap with HP Color Recovery"));

      xa_hp_cr_maps = gdk_x11_get_xatom_by_name_for_display (display,
                                                             "_HP_RGB_SMOOTH_MAP_LIST");

      status = XGetRGBColormaps (xdisplay, xroot_window,
                                 &standard_cmaps, &num_cmaps,
                                 xa_hp_cr_maps);
      if (status)
        {
          for (i = 0; i < num_cmaps; i++)
            {
              if (standard_cmaps[i].visualid == xvinfo->visualid)
                {
                  xcolormap = standard_cmaps[i].colormap;
                  break;
                }
            }

          XFree (standard_cmaps);

          if (xcolormap != None)
            {
              GDK_GL_NOTE (MISC,
                g_message (" -- Colormap: standard RGB with HP Color Recovery"));

              visual = gdk_x11_screen_lookup_visual (screen, xvinfo->visualid);
              return gdk_x11_colormap_foreign_new (visual, xcolormap);
            }
        }
    }

#if defined(HAVE_LIBXMU) && !defined(_DISABLE_STANDARD_RGB_CMAP)

  /*
   * (ripped from GLUT)
   * Solaris 2.4 and 2.5 have a bug in their XmuLookupStandardColormap
   * implementations.  Please compile your Solaris 2.4 or 2.5 version of
   * GtkGLExt with -D_DISABLE_STANDARD_RGB_CMAP to work around this bug.
   * The symptom of the bug is that programs will get a BadMatch error
   * from XCreateWindow when creating a window because Solaris 2.4 and 2.5
   * create a corrupted RGB_DEFAULT_MAP property.  Note that this workaround
   * prevents colormap sharing between applications, perhaps leading
   * unnecessary colormap installations or colormap flashing.  Sun fixed
   * this bug in Solaris 2.6.
   */

  if (!_gdk_gl_config_no_standard_colormap)
    {
      GDK_GL_NOTE (MISC,
        g_message (" -- Try to find a standard RGB colormap"));

      status = XmuLookupStandardColormap (xdisplay, screen_num,
                                          xvinfo->visualid, xvinfo->depth,
                                          XA_RGB_DEFAULT_MAP,
                                          False, True);
      if (status)
        {
          status = XGetRGBColormaps (xdisplay, xroot_window,
                                     &standard_cmaps, &num_cmaps,
                                     XA_RGB_DEFAULT_MAP);
          if (status)
            {
              for (i = 0; i < num_cmaps; i++)
                {
                  if (standard_cmaps[i].visualid == xvinfo->visualid)
                    {
                      xcolormap = standard_cmaps[i].colormap;
                      break;
                    }
                }

              XFree (standard_cmaps);

              if (xcolormap != None)
                {
                  GDK_GL_NOTE (MISC, g_message (" -- Colormap: standard RGB"));

                  visual = gdk_x11_screen_lookup_visual (screen, xvinfo->visualid);
                  return gdk_x11_colormap_foreign_new (visual, xcolormap);
                }
            }
        }
    }

#endif /* defined(HAVE_LIBXMU) && !defined(_DISABLE_STANDARD_RGB_CMAP) */

  return NULL;
}

#endif /* HAVE_GDK_X11_COLORMAP_FOREIGN_NEW */

/* 
 * Setup colormap.
 */

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

static GdkColormap *
gdk_gl_config_setup_colormap (GdkScreen   *screen,
                              XVisualInfo *xvinfo,
                              gboolean     is_rgba,
                              gboolean     is_mesa_glx)
{
  GdkColormap *colormap;
  GdkVisual *visual;
  GdkGLOverlayInfo overlay_info;
  gboolean overlay_supported;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (is_rgba)
    {
      /*
       * For RGBA mode.
       */

      /* Try default colormap. */

      colormap = gdk_screen_get_default_colormap (screen);
      visual = gdk_colormap_get_visual (colormap);
      if (GDK_VISUAL_XVISUAL (visual)->visualid == xvinfo->visualid)
        {
          GDK_GL_NOTE (MISC, g_message (" -- Colormap: screen default"));
          g_object_ref (G_OBJECT (colormap));
          return colormap;
        }

      /* Try standard RGB colormap. */

#ifdef HAVE_GDK_X11_COLORMAP_FOREIGN_NEW
      colormap = gdk_gl_config_get_std_rgb_colormap (screen, xvinfo, is_mesa_glx);
      if (colormap)
        return colormap;
#endif /* HAVE_GDK_X11_COLORMAP_FOREIGN_NEW */

      /* New colormap. */

      GDK_GL_NOTE (MISC, g_message (" -- Colormap: new"));
      visual = gdk_x11_screen_lookup_visual (screen, xvinfo->visualid);
      colormap = gdk_colormap_new (visual, FALSE);
      return colormap;

    }
  else
    {
      /*
       * For color index mode.
       */

      visual = gdk_x11_screen_lookup_visual (screen, xvinfo->visualid);

      overlay_supported = _gdk_x11_gl_overlay_get_info (visual, &overlay_info);
      if (overlay_supported &&
          overlay_info.transparent_type == GDK_GL_OVERLAY_TRANSPARENT_PIXEL &&
          overlay_info.value < (guint32) xvinfo->visual->map_entries)
        {

          /*
           * On machines where zero (or some other value in the range
           * of 0 through map_entries-1), BadAlloc may be generated
           * when an AllocAll overlay colormap is allocated since the
           * transparent pixel precludes all the cells in the colormap
           * being allocated (the transparent pixel is pre-allocated).
           * So in this case, use XAllocColorCells to allocate
           * map_entries-1 pixels (that is, all but the transparent pixel).
           */

          GDK_GL_NOTE (MISC, g_message (" -- Colormap: new"));
          colormap = gdk_colormap_new (visual, FALSE);
        }
      else
        {

          /*
           * If there is no transparent pixel or if the transparent
           * pixel is outside the range of valid colormap cells (HP
           * can implement their overlays this smart way since their
           * transparent pixel is 255), we can AllocAll the colormap.
           * See note above.
           */

          GDK_GL_NOTE (MISC, g_message (" -- Colormap: new allocated writable"));
          colormap = gdk_colormap_new (visual, TRUE);
        }

      return colormap;

    }

  /* not reached */
  return NULL;
}

#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */

static GdkColormap *
gdk_gl_config_setup_colormap (GdkScreen   *screen,
                              XVisualInfo *xvinfo,
                              gboolean     is_rgba,
                              gboolean     is_mesa_glx)
{
  GdkColormap *colormap;
  GdkVisual *visual;
  GdkGLOverlayInfo overlay_info;
  gboolean overlay_supported;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  if (is_rgba)
    {
      /*
       * For RGBA mode.
       */

      /* Try default colormap. */

      colormap = gdk_colormap_get_system ();
      visual = gdk_colormap_get_visual (colormap);
      if (GDK_VISUAL_XVISUAL (visual)->visualid == xvinfo->visualid)
        {
          GDK_GL_NOTE (MISC, g_message (" -- Colormap: system default"));

          g_object_ref (G_OBJECT (colormap));
          return colormap;
        }

      /* New colormap. */

      GDK_GL_NOTE (MISC, g_message (" -- Colormap: new"));

      visual = gdkx_visual_get (xvinfo->visualid);
      colormap = gdk_colormap_new (visual, FALSE);
      return colormap;

    }
  else
    {
      /*
       * For color index mode.
       */

      visual = gdkx_visual_get (xvinfo->visualid);

      overlay_supported = _gdk_x11_gl_overlay_get_info (visual, &overlay_info);
      if (overlay_supported &&
          overlay_info.transparent_type == GDK_GL_OVERLAY_TRANSPARENT_PIXEL &&
          overlay_info.value < xvinfo->visual->map_entries)
        {

          /*
           * On machines where zero (or some other value in the range
           * of 0 through map_entries-1), BadAlloc may be generated
           * when an AllocAll overlay colormap is allocated since the
           * transparent pixel precludes all the cells in the colormap
           * being allocated (the transparent pixel is pre-allocated).
           * So in this case, use XAllocColorCells to allocate
           * map_entries-1 pixels (that is, all but the transparent pixel).
           */

          GDK_GL_NOTE (MISC, g_message (" -- Colormap: new"));
          colormap = gdk_colormap_new (visual, FALSE);
        }
      else
        {

          /*
           * If there is no transparent pixel or if the transparent
           * pixel is outside the range of valid colormap cells (HP
           * can implement their overlays this smart way since their
           * transparent pixel is 255), we can AllocAll the colormap.
           * See note above.
           */

          GDK_GL_NOTE (MISC, g_message (" -- Colormap: new allocated writable"));
          colormap = gdk_colormap_new (visual, TRUE);
        }

      return colormap;

    }

  /* not reached */
  return NULL;
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

static void
gdk_gl_config_init_attrib (GdkGLConfig *glconfig)
{
  GdkGLConfigImplX11 *impl;
  int value;

  impl = GDK_GL_CONFIG_IMPL_X11 (glconfig);

#define _GET_CONFIG(__attrib) \
  glXGetConfig (impl->xdisplay, impl->xvinfo, __attrib, &value)

  /* RGBA mode? */
  _GET_CONFIG (GLX_RGBA);
  glconfig->is_rgba = value ? TRUE : FALSE;

  /* Layer plane. */
  _GET_CONFIG (GLX_LEVEL);
  glconfig->layer_plane = value;

  /* Double buffering is supported? */
  _GET_CONFIG (GLX_DOUBLEBUFFER);
  glconfig->is_double_buffered = value ? TRUE : FALSE;

  /* Stereo is supported? */
  _GET_CONFIG (GLX_STEREO);
  glconfig->is_stereo = value ? TRUE : FALSE;

  /* Number of aux buffers */
  _GET_CONFIG (GLX_AUX_BUFFERS);
  glconfig->n_aux_buffers = value;

  /* Has alpha bits? */
  _GET_CONFIG (GLX_ALPHA_SIZE);
  glconfig->has_alpha = value ? TRUE : FALSE;

  /* Has depth buffer? */
  _GET_CONFIG (GLX_DEPTH_SIZE);
  glconfig->has_depth_buffer = value ? TRUE : FALSE;

  /* Has stencil buffer? */
  _GET_CONFIG (GLX_STENCIL_SIZE);
  glconfig->has_stencil_buffer = value ? TRUE : FALSE;

  /* Has accumulation buffer? */
  _GET_CONFIG (GLX_ACCUM_RED_SIZE);
  glconfig->has_accum_buffer = value ? TRUE : FALSE;

  /* Number of multisample buffers (not supported yet) */
  glconfig->n_sample_buffers = 0;

#undef _GET_CONFIG
}

static GdkGLConfig *
gdk_gl_config_new_common (GdkScreen *screen,
                          const int *attrib_list)
{
  GdkGLConfig *glconfig;
  GdkGLConfigImplX11 *impl;

  Display *xdisplay;
  int screen_num;
  XVisualInfo *xvinfo;
  int is_rgba;

  GDK_GL_NOTE_FUNC_PRIVATE ();

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  xdisplay = GDK_SCREEN_XDISPLAY (screen);
  screen_num = GDK_SCREEN_XNUMBER (screen);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  xdisplay = gdk_x11_get_default_xdisplay ();
  screen_num = gdk_x11_get_default_screen ();
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  GDK_GL_NOTE (MISC, _gdk_x11_gl_print_glx_info (xdisplay, screen_num));

  /*
   * Find an OpenGL-capable visual.
   */

  GDK_GL_NOTE_FUNC_IMPL ("glXChooseVisual");

  xvinfo = glXChooseVisual (xdisplay, screen_num, (int *) attrib_list);
  if (xvinfo == NULL)
    return NULL;

  GDK_GL_NOTE (MISC,
    g_message (" -- glXChooseVisual: screen number = %d", xvinfo->screen));
  GDK_GL_NOTE (MISC,
    g_message (" -- glXChooseVisual: visual id = 0x%lx", xvinfo->visualid));

  /*
   * Instantiate the GdkGLConfigImplX11 object.
   */

  glconfig = g_object_new (GDK_TYPE_GL_CONFIG_IMPL_X11, NULL);
  impl = GDK_GL_CONFIG_IMPL_X11 (glconfig);

  impl->xdisplay = xdisplay;
  impl->screen_num = screen_num;
  impl->xvinfo = xvinfo;

  impl->screen = screen;

  /* Using Mesa? */
  if (strstr (glXQueryServerString (xdisplay, screen_num, GLX_VERSION), "Mesa"))
    impl->is_mesa_glx = TRUE;
  else
    impl->is_mesa_glx = FALSE;

  /*
   * Get an appropriate colormap.
   */

  /* RGBA mode? */
  glXGetConfig (xdisplay, xvinfo, GLX_RGBA, &is_rgba);

  impl->colormap = gdk_gl_config_setup_colormap (impl->screen,
                                                 impl->xvinfo,
                                                 is_rgba,
                                                 impl->is_mesa_glx);

  GDK_GL_NOTE (MISC,
    g_message (" -- Colormap: visual id = 0x%lx",
               GDK_VISUAL_XVISUAL (impl->colormap->visual)->visualid));

  /*
   * Init configuration attributes.
   */

  gdk_gl_config_init_attrib (glconfig);

  return glconfig;
}

/**
 * gdk_gl_config_new:
 * @attrib_list: a list of attribute/value pairs. The last attribute must
 *               be GDK_GL_ATTRIB_LIST_NONE.
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
gdk_gl_config_new (const int *attrib_list)
{
  GdkScreen *screen;

  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (attrib_list != NULL, NULL);

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  screen = gdk_screen_get_default ();
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  screen = NULL;
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  return gdk_gl_config_new_common (screen, attrib_list);
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

/**
 * gdk_gl_config_new_for_screen:
 * @screen: target screen.
 * @attrib_list: a list of attribute/value pairs. The last attribute must
 *               be GDK_GL_ATTRIB_LIST_NONE.
 *
 * Returns an OpenGL frame buffer configuration that match the specified
 * attributes.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_gl_config_new_for_screen (GdkScreen *screen,
                              const int *attrib_list)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (attrib_list != NULL, NULL);

  return gdk_gl_config_new_common (screen, attrib_list);
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

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
gdk_x11_gl_config_new_from_visualid_common (GdkScreen *screen,
                                            VisualID   xvisualid)
{
  GdkGLConfig *glconfig;
  GdkGLConfigImplX11 *impl;

  Display *xdisplay;
  int screen_num;
  XVisualInfo *xvinfo;
  int is_rgba;

  GDK_GL_NOTE_FUNC_PRIVATE ();

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  xdisplay = GDK_SCREEN_XDISPLAY (screen);
  screen_num = GDK_SCREEN_XNUMBER (screen);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  xdisplay = gdk_x11_get_default_xdisplay ();
  screen_num = gdk_x11_get_default_screen ();
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

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

  glconfig = g_object_new (GDK_TYPE_GL_CONFIG_IMPL_X11, NULL);
  impl = GDK_GL_CONFIG_IMPL_X11 (glconfig);

  impl->xdisplay = xdisplay;
  impl->screen_num = screen_num;
  impl->xvinfo = xvinfo;

  impl->screen = screen;

  /* Using Mesa? */
  if (strstr (glXQueryServerString (xdisplay, screen_num, GLX_VERSION), "Mesa"))
    impl->is_mesa_glx = TRUE;
  else
    impl->is_mesa_glx = FALSE;

  /*
   * Get an appropriate colormap.
   */

  /* RGBA mode? */
  glXGetConfig (xdisplay, xvinfo, GLX_RGBA, &is_rgba);

  impl->colormap = gdk_gl_config_setup_colormap (impl->screen,
                                                 impl->xvinfo,
                                                 is_rgba,
                                                 impl->is_mesa_glx);

  GDK_GL_NOTE (MISC,
    g_message (" -- Colormap: visual id = 0x%lx",
               GDK_VISUAL_XVISUAL (impl->colormap->visual)->visualid));

  /*
   * Init configuration attributes.
   */

  gdk_gl_config_init_attrib (glconfig);

  return glconfig;
}

/**
 * gdk_x11_gl_config_new_from_visualid:
 * @xvisualid: visual ID.
 *
 * Creates #GdkGLConfig from given visual ID that specifies the OpenGL-capable
 * visual.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_x11_gl_config_new_from_visualid (VisualID xvisualid)
{
  GdkScreen *screen;

  GDK_GL_NOTE_FUNC ();

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  screen = gdk_screen_get_default ();
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  screen = NULL;
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  return gdk_x11_gl_config_new_from_visualid_common (screen, xvisualid);
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

/**
 * gdk_x11_gl_config_new_from_visualid_for_screen:
 * @screen: target screen.
 * @xvisualid: visual ID.
 *
 * Creates #GdkGLConfig from given visual ID that specifies the OpenGL-capable
 * visual.
 *
 * Return value: the new #GdkGLConfig.
 **/
GdkGLConfig *
gdk_x11_gl_config_new_from_visualid_for_screen (GdkScreen *screen,
                                                VisualID   xvisualid)
{
  GDK_GL_NOTE_FUNC ();

  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return gdk_x11_gl_config_new_from_visualid_common (screen, xvisualid);
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

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
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->screen;
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
  GdkGLConfigImplX11 *impl;
  int ret;

  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), FALSE);

  impl = GDK_GL_CONFIG_IMPL_X11 (glconfig);

  ret = glXGetConfig (impl->xdisplay, impl->xvinfo, attribute, value);

  return (ret == Success);
}

/**
 * gdk_gl_config_get_colormap:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets the #GdkColormap that is appropriate for the OpenGL frame buffer
 * configuration.
 *
 * Return value: the appropriate #GdkColormap.
 **/
GdkColormap *
gdk_gl_config_get_colormap (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->colormap;
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
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);

  return gdk_colormap_get_visual (GDK_GL_CONFIG_IMPL_X11 (glconfig)->colormap);
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
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), 0);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->xvinfo->depth;
}

/**
 * gdk_x11_gl_config_get_xdisplay:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets X Display.
 *
 * Return value: pointer to the Display.
 **/
Display *
gdk_x11_gl_config_get_xdisplay (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->xdisplay;
}

/**
 * gdk_x11_gl_config_get_screen_number:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets X screen number.
 *
 * Return value: the screen number.
 **/
int
gdk_x11_gl_config_get_screen_number (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), 0);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->screen_num;
}

/**
 * gdk_x11_gl_config_get_xvinfo:
 * @glconfig: a #GdkGLConfig.
 *
 * Gets XVisualInfo data.
 *
 * Return value: pointer to the XVisualInfo data.
 **/
XVisualInfo *
gdk_x11_gl_config_get_xvinfo (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), NULL);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->xvinfo;
}

/**
 * gdk_x11_gl_config_is_mesa_glx:
 * @glconfig: a #GdkGLConfig.
 *
 * Returns whether the server's GLX entension is Mesa.
 *
 * Return value: TRUE if Mesa GLX, FALSE otherwise.
 **/
gboolean
gdk_x11_gl_config_is_mesa_glx (GdkGLConfig *glconfig)
{
  g_return_val_if_fail (GDK_IS_GL_CONFIG_IMPL_X11 (glconfig), FALSE);

  return GDK_GL_CONFIG_IMPL_X11 (glconfig)->is_mesa_glx;
}
