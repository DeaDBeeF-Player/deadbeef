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

#ifdef __APPLE__
#include <mach-o/dyld.h>
#else  /* __APPLE__ */
#include <gmodule.h>
#endif /* __APPLE__ */

#include <gdk/gdk.h>

#include "gdkglx.h"
#include "gdkglprivate-x11.h"
#include "gdkglconfig-x11.h"
#include "gdkglquery.h"

#include "gdkglquery-x11.h"

gboolean
_gdk_x11_gl_query_extension_for_display (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  return glXQueryExtension (GDK_DISPLAY_XDISPLAY (display),
                            NULL, NULL);
}

gboolean
_gdk_x11_gl_query_version_for_display (GdkDisplay *display,
                                       int        *major,
                                       int        *minor)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  return glXQueryVersion (GDK_DISPLAY_XDISPLAY (display),
                          major, minor);
}

/*
 * This code is based on __glutIsSupportedByGLX().
 */

/**
 * gdk_x11_gl_query_glx_extension:
 * @glconfig: a #GdkGLConfig.
 * @extension: name of GLX extension.
 *
 * Determines whether a given GLX extension is supported.
 *
 * Return value: TRUE if the GLX extension is supported, FALSE if not
 *               supported.
 **/
gboolean
gdk_x11_gl_query_glx_extension (GdkGLConfig *glconfig,
                                const char  *extension)
{
  static const char *extensions = NULL;
  const char *start;
  char *where, *terminator;
  int major, minor;

  g_return_val_if_fail (GDK_IS_X11_GL_CONFIG (glconfig), FALSE);

  /* Extension names should not have spaces. */
  where = strchr (extension, ' ');
  if (where || *extension == '\0')
    return FALSE;

  if (extensions == NULL)
    {
      /* Be careful not to call glXQueryExtensionsString if it
         looks like the server doesn't support GLX 1.1.
         Unfortunately, the original GLX 1.0 didn't have the notion
         of GLX extensions. */

      glXQueryVersion (GDK_GL_CONFIG_XDISPLAY (glconfig),
                       &major, &minor);

      if ((major == 1 && minor < 1) || (major < 1))
        return FALSE;

      extensions = glXQueryExtensionsString (GDK_GL_CONFIG_XDISPLAY (glconfig),
                                             GDK_GL_CONFIG_SCREEN_XNUMBER (glconfig));
    }

  /* It takes a bit of care to be fool-proof about parsing
     the GLX extensions string.  Don't be fooled by
     sub-strings,  etc. */
  start = extensions;
  for (;;)
    {
      where = strstr (start, extension);
      if (where == NULL)
        break;

      terminator = where + strlen (extension);

      if (where == start || *(where - 1) == ' ')
        if (*terminator == ' ' || *terminator == '\0')
          {
            GDK_GL_NOTE (MISC, g_message (" - %s - supported", extension));
            return TRUE;
          }

      start = terminator;
    }

  GDK_GL_NOTE (MISC, g_message (" - %s - not supported", extension));

  return FALSE;
}

GdkGLProc
_gdk_x11_gl_get_proc_address (const char *proc_name)
{
#ifdef __APPLE__

  #define _GDK_GL_LIBGL_PATH  "/usr/X11R6/lib/libGL.1.dylib"
  #define _GDK_GL_LIBGLU_PATH "/usr/X11R6/lib/libGLU.1.dylib"

  typedef GdkGLProc (*__glXGetProcAddressProc) (const GLubyte *);
  static __glXGetProcAddressProc glx_get_proc_address = (__glXGetProcAddressProc) -1;
  const char *image_name;
  static const struct mach_header *libgl_image = NULL;
  static const struct mach_header *libglu_image = NULL;
  NSSymbol symbol;
  char *symbol_name;
  GdkGLProc proc_address;

  GDK_GL_NOTE_FUNC ();

  if (strncmp ("glu", proc_name, 3) != 0)
    {
      /* libGL */

      if (libgl_image == NULL)
        {
          image_name = g_getenv ("GDK_GL_LIBGL_PATH");
          if (image_name == NULL)
            image_name = _GDK_GL_LIBGL_PATH;

          GDK_GL_NOTE (MISC, g_message (" - Add Mach-O image %s", image_name));

          libgl_image = NSAddImage (image_name, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
          if (libgl_image == NULL)
            {
              g_warning ("Cannot add Mach-O image %s", image_name);
              return NULL;
            }
        }

      if (glx_get_proc_address == (__glXGetProcAddressProc) -1)
        {
          /*
           * Look up glXGetProcAddress () function.
           */

          symbol = NSLookupSymbolInImage (libgl_image,
                                          "_glXGetProcAddress",
                                          NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                          NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
          if (symbol == NULL)
            {
              symbol = NSLookupSymbolInImage (libgl_image,
                                              "_glXGetProcAddressARB",
                                              NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                              NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
              if (symbol == NULL)
                {
                  symbol = NSLookupSymbolInImage (libgl_image,
                                                  "_glXGetProcAddressEXT",
                                                  NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                                  NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
                }
            }
          GDK_GL_NOTE (MISC, g_message (" - glXGetProcAddress () - %s",
                                        symbol ? "supported" : "not supported"));
          if (symbol != NULL)
            glx_get_proc_address = NSAddressOfSymbol (symbol);
          else
            glx_get_proc_address = NULL;
        }

      /* Try glXGetProcAddress () */

      if (glx_get_proc_address != NULL)
        {
          proc_address = glx_get_proc_address ((unsigned char *) proc_name);
          GDK_GL_NOTE (IMPL, g_message (" ** glXGetProcAddress () - %s",
                                        proc_address ? "succeeded" : "failed"));
          if (proc_address != NULL)
            return proc_address;
        }

      /* Try Mach-O dyld */

      symbol_name = g_strconcat ("_", proc_name, NULL);

      symbol = NSLookupSymbolInImage (libgl_image,
                                      symbol_name,
                                      NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                      NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
      GDK_GL_NOTE (MISC, g_message (" - NSLookupSymbolInImage () - %s",
                                    symbol ? "succeeded" : "failed"));

      g_free (symbol_name);

      if (symbol != NULL)
        return NSAddressOfSymbol (symbol);
    }
  else
    {
      /* libGLU */

      if (libglu_image == NULL)
        {
          image_name = g_getenv ("GDK_GL_LIBGLU_PATH");
          if (image_name == NULL)
            image_name = _GDK_GL_LIBGLU_PATH;

          GDK_GL_NOTE (MISC, g_message (" - Add Mach-O image %s", image_name));

          libglu_image = NSAddImage (image_name, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
          if (libglu_image == NULL)
            {
              g_warning ("Cannot add Mach-O image %s", image_name);
              return NULL;
            }
        }

      symbol_name = g_strconcat ("_", proc_name, NULL);

      symbol = NSLookupSymbolInImage (libglu_image,
                                      symbol_name,
                                      NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                      NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
      GDK_GL_NOTE (MISC, g_message (" - NSLookupSymbolInImage () - %s",
                                    symbol ? "succeeded" : "failed"));

      g_free (symbol_name);

      if (symbol != NULL)
        return NSAddressOfSymbol (symbol);
    }

  return NULL;

#else  /* __APPLE__ */

  typedef GdkGLProc (*__glXGetProcAddressProc) (const GLubyte *);
  static __glXGetProcAddressProc glx_get_proc_address = (__glXGetProcAddressProc) -1;
  gchar *file_name;
  GModule *module;
  GdkGLProc proc_address = NULL;

  GDK_GL_NOTE_FUNC ();

  if (strncmp ("glu", proc_name, 3) != 0)
    {
      if (glx_get_proc_address == (__glXGetProcAddressProc) -1)
        {
          /*
           * Look up glXGetProcAddress () function.
           */

          file_name = g_module_build_path (NULL, "GL");
          GDK_GL_NOTE (MISC, g_message (" - Open %s", file_name));
          module = g_module_open (file_name, G_MODULE_BIND_LAZY);
          g_free (file_name);

          if (module != NULL)
            {
              g_module_symbol (module, "glXGetProcAddress",
                               (gpointer) &glx_get_proc_address);
              if (glx_get_proc_address == NULL)
                {
                  g_module_symbol (module, "glXGetProcAddressARB",
                                   (gpointer) &glx_get_proc_address);
                  if (glx_get_proc_address == NULL)
                    {
                      g_module_symbol (module, "glXGetProcAddressEXT",
                                       (gpointer) &glx_get_proc_address);
                    }
                }
              GDK_GL_NOTE (MISC, g_message (" - glXGetProcAddress () - %s",
                                            glx_get_proc_address ? "supported" : "not supported"));
              g_module_close (module);
            }
          else
            {
              g_warning ("Cannot open %s", file_name);
              glx_get_proc_address = NULL;
              return NULL;
            }
        }

      /* Try glXGetProcAddress () */

      if (glx_get_proc_address != NULL)
        {
          proc_address = glx_get_proc_address ((unsigned char *) proc_name);
          GDK_GL_NOTE (IMPL, g_message (" ** glXGetProcAddress () - %s",
                                        proc_address ? "succeeded" : "failed"));
          if (proc_address != NULL)
            return proc_address;
        }

      /* Try g_module_symbol () */

      /* libGL */
      file_name = g_module_build_path (NULL, "GL");
      GDK_GL_NOTE (MISC, g_message (" - Open %s", file_name));
      module = g_module_open (file_name, G_MODULE_BIND_LAZY);
      g_free (file_name);

      if (module != NULL)
        {
          g_module_symbol (module, proc_name, (gpointer) &proc_address);
          GDK_GL_NOTE (MISC, g_message (" - g_module_symbol () - %s",
                                        proc_address ? "succeeded" : "failed"));
          g_module_close (module);
        }
      else
        {
          g_warning ("Cannot open %s", file_name);
        }

      if (proc_address == NULL)
        {
          /* libGLcore */
          file_name = g_module_build_path (NULL, "GLcore");
          GDK_GL_NOTE (MISC, g_message (" - Open %s", file_name));
          module = g_module_open (file_name, G_MODULE_BIND_LAZY);
          g_free (file_name);

          if (module != NULL)
            {
              g_module_symbol (module, proc_name, (gpointer) &proc_address);
              GDK_GL_NOTE (MISC, g_message (" - g_module_symbol () - %s",
                                            proc_address ? "succeeded" : "failed"));
              g_module_close (module);
            }
        }
    }
  else
    {
      /* libGLU */
      file_name = g_module_build_path (NULL, "GLU");
      GDK_GL_NOTE (MISC, g_message (" - Open %s", file_name));
      module = g_module_open (file_name, G_MODULE_BIND_LAZY);
      g_free (file_name);

      if (module != NULL)
        {
          g_module_symbol (module, proc_name, (gpointer) &proc_address);
          GDK_GL_NOTE (MISC, g_message (" - g_module_symbol () - %s",
                                        proc_address ? "succeeded" : "failed"));
          g_module_close (module);
        }
      else
        {
          g_warning ("Cannot open %s", file_name);
        }
    }

  return proc_address;

#endif /* __APPLE__ */
}

/*< private >*/
void
_gdk_x11_gl_print_glx_info (Display *xdisplay,
                            int      screen_num)
{
  static gboolean done = FALSE;

  if (!done)
    {
      g_message (" -- Server GLX_VENDOR     : %s",
                 glXQueryServerString (xdisplay, screen_num, GLX_VENDOR));
      g_message (" -- Server GLX_VERSION    : %s",
                 glXQueryServerString (xdisplay, screen_num, GLX_VERSION));
      g_message (" -- Server GLX_EXTENSIONS : %s",
                 glXQueryServerString (xdisplay, screen_num, GLX_EXTENSIONS));

      g_message (" -- Client GLX_VENDOR     : %s",
                 glXGetClientString (xdisplay, GLX_VENDOR));
      g_message (" -- Client GLX_VERSION    : %s",
                 glXGetClientString (xdisplay, GLX_VERSION));
      g_message (" -- Client GLX_EXTENSIONS : %s",
                 glXGetClientString (xdisplay, GLX_EXTENSIONS));

      done = TRUE;
    }
}
