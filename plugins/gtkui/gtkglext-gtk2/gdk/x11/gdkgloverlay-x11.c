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

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkgloverlay-x11.h"

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
#include <gdk/gdkscreen.h>
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

#include <X11/Xmd.h>

/*
 * SERVER_OVERLAY_VISUALS property entry format
 *
 * format: 32
 *
 * <Name>              <Type>      <Description>
 * overlay_visual      VisualID    Visual ID of visual.
 * transparent_type    CARD32      None (0).
 *                                 TransparentPixel (1).
 *                                 TransparentMask (2).
 * value               CARD32      Pixel value or transparency mask.
 * layer               INT32       The layer the visual resides in.
 */

/*
 * SOV property.
 * (format is 32: the returned data is represented as a long array)
 */

typedef struct
{
  long overlay_visual;
  long transparent_type;
  long value;
  long layer;
} __SOVProp;

/* SOV properties data. */

typedef struct
{
  __SOVProp *prop;
  unsigned long num;
} __SOVPropArray;

static const gchar quark_sov_props_string[] = "gdk-gl-overlay-sov-props";
static GQuark quark_sov_props = 0;

static void
sov_prop_array_destroy (__SOVPropArray *sov_props)
{
  if (sov_props->prop != NULL)
    XFree (sov_props->prop);

  g_free (sov_props);
}

static __SOVPropArray *
gdk_gl_overlay_get_sov_props (GdkScreen *screen)
{
  __SOVPropArray *sov_props;
  GdkWindow *root_window;
#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  GdkDisplay *display;
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
  Display *xdisplay;
  Atom xa_sov;
  Status status;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  GDK_GL_NOTE_FUNC_PRIVATE ();

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  root_window = gdk_screen_get_root_window (screen);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  root_window = gdk_get_default_root_window ();
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  if (quark_sov_props == 0)
    quark_sov_props = g_quark_from_static_string (quark_sov_props_string);

  sov_props = g_object_get_qdata (G_OBJECT (root_window), quark_sov_props);
  if (sov_props != NULL)
    return sov_props;

  sov_props = g_malloc (sizeof (__SOVPropArray));

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  display = gdk_screen_get_display (screen);
  xdisplay = GDK_DISPLAY_XDISPLAY (display);
  xa_sov = gdk_x11_get_xatom_by_name_for_display (display, "SERVER_OVERLAY_VISUALS");
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  xdisplay = gdk_x11_get_default_xdisplay ();
  xa_sov = gdk_x11_get_xatom_by_name ("SERVER_OVERLAY_VISUALS");
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  status = XGetWindowProperty (xdisplay, GDK_WINDOW_XWINDOW (root_window),
                               xa_sov, 0L, 1000000L, False, AnyPropertyType,
                               &actual_type, &actual_format,
                               &nitems, &bytes_after, &prop);
  if (status != Success ||
      actual_type == None ||
      actual_format != 32 ||
      nitems < 4)
    {
      GDK_GL_NOTE (MISC, g_message (" -- SERVER_OVERLAY_VISUALS: not supported"));

      if (prop != NULL)
        XFree (prop);

      sov_props->prop = NULL;
      sov_props->num = 0;
    }
  else
    {
      GDK_GL_NOTE (MISC, g_message (" -- SERVER_OVERLAY_VISUALS: supported"));

      sov_props->prop = (__SOVProp *) prop;
      sov_props->num = nitems / (sizeof (__SOVProp) / 4);
    }

  g_object_set_qdata_full (G_OBJECT (root_window), quark_sov_props, sov_props,
                           (GDestroyNotify) sov_prop_array_destroy);

#ifdef G_ENABLE_DEBUG
  if (gdk_gl_debug_flags & GDK_GL_DEBUG_MISC)
    {
#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
      int screen_num = GDK_SCREEN_XNUMBER (screen);
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
      int screen_num = gdk_x11_get_default_screen ();
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
      int i;

      g_message (" -- SERVER_OVERLAY_VISUALS: properties");
      g_print ("screen\tvisual\ttype\tvalue\tlayer\n");
      for (i = 0; i < sov_props->num; i++)
        {
          g_print ("%d\t0x%lx\t%lu\t%lu\t%ld\n",
                   screen_num,
                   (VisualID) (sov_props->prop[i].overlay_visual),
                   (CARD32)   (sov_props->prop[i].transparent_type),
                   (CARD32)   (sov_props->prop[i].value),
                   (INT32)    (sov_props->prop[i].layer));
        }
    }
#endif /* G_ENABLE_DEBUG */

  return sov_props;
}

/* private at present... */
gboolean
_gdk_x11_gl_overlay_get_info (GdkVisual        *visual,
                              GdkGLOverlayInfo *overlay_info)
{
  __SOVPropArray *sov_props;
  VisualID xvisualid;
  int i;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  g_return_val_if_fail (GDK_IS_VISUAL (visual), FALSE);
  g_return_val_if_fail (overlay_info != NULL, FALSE);

  /* Get SOV properties. */

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  sov_props = gdk_gl_overlay_get_sov_props (gdk_visual_get_screen (visual));
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  sov_props = gdk_gl_overlay_get_sov_props (NULL);
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  /* Look up SOV property for the visual. */

  xvisualid = GDK_VISUAL_XVISUAL (visual)->visualid;

  for (i = 0; i < sov_props->num; i++)
    {
      if ((VisualID) (sov_props->prop[i].overlay_visual) == xvisualid)
        {
          overlay_info->visual           = visual;
          overlay_info->transparent_type = sov_props->prop[i].transparent_type;
          overlay_info->value            = sov_props->prop[i].value;
          overlay_info->layer            = sov_props->prop[i].layer;

          GDK_GL_NOTE (MISC, g_message (" -- overlay visual"));
          GDK_GL_NOTE (MISC, g_print ("transparent_type = %d\n",
                                      overlay_info->transparent_type));
          GDK_GL_NOTE (MISC, g_print ("value = %u\n",
                                      overlay_info->value));
          GDK_GL_NOTE (MISC, g_print ("layer = %d\n",
                                      overlay_info->layer));

          return TRUE;
        }
    }

  /* meaningless */
  overlay_info->visual           = visual;
  overlay_info->transparent_type = GDK_GL_OVERLAY_TRANSPARENT_NONE;
  overlay_info->value            = 0;
  overlay_info->layer            = 0;

  GDK_GL_NOTE (MISC, g_message (" -- not overlay visual"));
      
  return FALSE;
}
