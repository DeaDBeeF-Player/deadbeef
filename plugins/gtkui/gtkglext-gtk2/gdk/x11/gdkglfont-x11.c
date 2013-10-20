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

#include <string.h>

#include <pango/pangox.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglfont.h"

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
#include <gdk/gdkdisplay.h>
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

/*
 * This code is ripped from gdk/x11/gdkfont-x11.c in GTK+.
 */
static char *
gdk_gl_font_charset_for_locale (void)
{
  static char *charset_map[][2] = {
    { "ANSI_X3.4-1968", "iso8859-1" },
    { "US-ASCII",   "iso8859-1" },
    { "ISO-8859-1", "iso8859-1" },
    { "ISO-8859-2", "iso8859-2" },
    { "ISO-8859-3", "iso8859-3" },
    { "ISO-8859-4", "iso8859-4" },
    { "ISO-8859-5", "iso8859-5" },
    { "ISO-8859-6", "iso8859-6" },
    { "ISO-8859-7", "iso8859-7" },
    { "ISO-8859-8", "iso8859-8" },
    { "ISO-8859-9", "iso8859-9" },
    { "UTF-8",      "iso8859-1" }
  };

  const char *codeset;
  char *result = NULL;
  gsize i;

  g_get_charset (&codeset);
  
  for (i = 0; i < G_N_ELEMENTS (charset_map); i++)
    if (strcmp (charset_map[i][0], codeset) == 0)
      {
	result = charset_map[i][1];
	break;
      }

  if (result != NULL)
    return g_strdup (result);
  else
    return g_strdup ("iso8859-1");
}

static PangoFont *
gdk_gl_font_use_pango_font_common (PangoFontMap               *font_map,
                                   const PangoFontDescription *font_desc,
                                   int                         first,
                                   int                         count,
                                   int                         list_base)
{
  PangoFont *font = NULL;
  gchar *charset = NULL;
  PangoXSubfont subfont_id;
  gchar *xlfd = NULL;
  PangoXFontCache *font_cache;
  XFontStruct *fs;

  GDK_GL_NOTE_FUNC_PRIVATE ();

  font = pango_font_map_load_font (font_map, NULL, font_desc);
  if (font == NULL)
    {
      g_warning ("cannot load PangoFont");
      goto FAIL;
    }

  charset = gdk_gl_font_charset_for_locale ();
  if (!pango_x_find_first_subfont (font, &charset, 1, &subfont_id))
    {
      g_warning ("cannot find PangoXSubfont");
      font = NULL;
      goto FAIL;
    }

  xlfd = pango_x_font_subfont_xlfd (font, subfont_id);
  if (xlfd == NULL)
    {
      g_warning ("cannot get XLFD");
      font = NULL;
      goto FAIL;
    }

  font_cache = pango_x_font_map_get_font_cache (font_map);

  fs = pango_x_font_cache_load (font_cache, xlfd);

  glXUseXFont (fs->fid, first, count, list_base);

  pango_x_font_cache_unload (font_cache, fs);

 FAIL:

  if (charset != NULL)
    g_free (charset);

  if (xlfd != NULL)
    g_free (xlfd);

  return font;
}

/**
 * gdk_gl_font_use_pango_font:
 * @font_desc: a #PangoFontDescription describing the font to use.
 * @first: the index of the first glyph to be taken.
 * @count: the number of glyphs to be taken.
 * @list_base: the index of the first display list to be generated.
 *
 * Creates bitmap display lists from a #PangoFont.
 *
 * Return value: the #PangoFont used, or NULL if no font matched.
 **/
PangoFont *
gdk_gl_font_use_pango_font (const PangoFontDescription *font_desc,
                            int                         first,
                            int                         count,
                            int                         list_base)
{
  PangoFontMap *font_map;

  g_return_val_if_fail (font_desc != NULL, NULL);

  GDK_GL_NOTE_FUNC ();

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
  font_map = pango_x_font_map_for_display (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
#else  /* GDKGLEXT_MULTIHEAD_SUPPORT */
  font_map = pango_x_font_map_for_display (gdk_x11_get_default_xdisplay ());
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

  return gdk_gl_font_use_pango_font_common (font_map, font_desc,
                                            first, count, list_base);
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

/**
 * gdk_gl_font_use_pango_font_for_display:
 * @display: a #GdkDisplay.
 * @font_desc: a #PangoFontDescription describing the font to use.
 * @first: the index of the first glyph to be taken.
 * @count: the number of glyphs to be taken.
 * @list_base: the index of the first display list to be generated.
 *
 * Creates bitmap display lists from a #PangoFont.
 *
 * Return value: the #PangoFont used, or NULL if no font matched.
 **/
PangoFont *
gdk_gl_font_use_pango_font_for_display (GdkDisplay                 *display,
                                        const PangoFontDescription *font_desc,
                                        int                         first,
                                        int                         count,
                                        int                         list_base)
{
  PangoFontMap *font_map;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (font_desc != NULL, NULL);

  GDK_GL_NOTE_FUNC ();

  font_map = pango_x_font_map_for_display (GDK_DISPLAY_XDISPLAY (display));

  return gdk_gl_font_use_pango_font_common (font_map, font_desc,
                                            first, count, list_base);
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */
