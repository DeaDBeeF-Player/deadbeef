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

#include "gdkglwin32.h"
#include "gdkglprivate-win32.h"
#include "gdkglquery.h"

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT
#include <gdk/gdkdisplay.h>
#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

gboolean
gdk_gl_query_extension (void)
{
  return TRUE;

#if 0
  DWORD version = GetVersion ();
  DWORD major = (DWORD) (LOBYTE (LOWORD (version)));

  if (version < 0x80000000)     /* Windows NT/2000/XP */
    {
      if (major >= 3)
        return TRUE;
    }
  else                          /* Windows 95/98/Me */
    {
      if (major >= 4)
        return TRUE;
    }

  return FALSE;
#endif
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

gboolean
gdk_gl_query_extension_for_display (GdkDisplay *display)
{
  g_return_val_if_fail (display == gdk_display_get_default (), FALSE);

  return TRUE;

#if 0
  DWORD version = GetVersion ();
  DWORD major = (DWORD) (LOBYTE (LOWORD (version)));

  if (version < 0x80000000)     /* Windows NT/2000/XP */
    {
      if (major >= 3)
        return TRUE;
    }
  else                          /* Windows 95/98/Me */
    {
      if (major >= 4)
        return TRUE;
    }

  return FALSE;
#endif
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

gboolean
gdk_gl_query_version (int *major,
                      int *minor)
{
  DWORD version = GetVersion ();

  /* return Windows version. */
  *major = (int) (LOBYTE (LOWORD (version)));
  *minor = (int) (HIBYTE (LOWORD (version)));

  return TRUE;
}

#ifdef GDKGLEXT_MULTIHEAD_SUPPORT

gboolean
gdk_gl_query_version_for_display (GdkDisplay *display,
                                  int        *major,
                                  int        *minor)
{
  DWORD version;

  g_return_val_if_fail (display == gdk_display_get_default (), FALSE);

  version = GetVersion ();

  /* return Windows version. */
  *major = (int) (LOBYTE (LOWORD (version)));
  *minor = (int) (HIBYTE (LOWORD (version)));

  return TRUE;
}

#endif /* GDKGLEXT_MULTIHEAD_SUPPORT */

gboolean
gdk_win32_gl_query_wgl_extension (GdkGLConfig *glconfig,
                                  const char  *extension)
{
  typedef const char * (WINAPI * __wglGetExtensionsStringARBProc) (HDC);
  typedef const char * (WINAPI * __wglGetExtensionsStringEXTProc) (void);
  __wglGetExtensionsStringARBProc wgl_get_extensions_string_arb;
  __wglGetExtensionsStringEXTProc wgl_get_extensions_string_ext;

  static const char *extensions = NULL;
  const char *start;
  char *where, *terminator;
  HDC hdc;

  /* Extension names should not have spaces. */
  where = strchr (extension, ' ');
  if (where || *extension == '\0')
    return FALSE;

  if (extensions == NULL)
    {
      /* Try wglGetExtensionsStringARB. */

      wgl_get_extensions_string_arb =
        (__wglGetExtensionsStringARBProc) gdk_gl_get_proc_address ("wglGetExtensionsStringARB");

      if (wgl_get_extensions_string_arb)
        {
          hdc = GetDC (NULL);
          if (hdc == NULL)
            {
              g_warning ("cannot get DC");
            }
          else
            {
              extensions = wgl_get_extensions_string_arb (hdc);
              ReleaseDC (NULL, hdc);
            }
        }

      if (extensions == NULL)
        {
          /* Try wglGetExtensionsStringEXT. */

          wgl_get_extensions_string_ext =
            (__wglGetExtensionsStringEXTProc) gdk_gl_get_proc_address ("wglGetExtensionsStringEXT");

          if (wgl_get_extensions_string_ext)
            extensions = wgl_get_extensions_string_ext ();
        }

      if (extensions == NULL)
        return FALSE;
    }

  /* It takes a bit of care to be fool-proof about parsing
     the WGL extensions string.  Don't be fooled by
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
gdk_gl_get_proc_address (const char *proc_name)
{
  GdkGLProc proc_address = NULL;
  HMODULE hmodule;

  GDK_GL_NOTE_FUNC ();

  if (strncmp ("glu", proc_name, 3) != 0)
    {
      /* Try wglGetProcAddress () */

      proc_address = (GdkGLProc) wglGetProcAddress (proc_name);

      GDK_GL_NOTE (IMPL, g_message (" ** wglGetProcAddress () - %s",
                                    proc_address ? "succeeded" : "failed"));

      if (proc_address != NULL)
        return proc_address;

      /* Try GetProcAddress () */

      /* opengl32.dll */

      GDK_GL_NOTE (MISC, g_message (" - Get opengl32.dll module handle"));

      hmodule = GetModuleHandle ("opengl32.dll");
      if (hmodule == NULL)
        g_warning ("Cannot get opengl32.dll module handle");
      else
        proc_address = (GdkGLProc) GetProcAddress (hmodule, proc_name);

      GDK_GL_NOTE (IMPL, g_message (" ** GetProcAddress () - %s",
                                    proc_address ? "succeeded" : "failed"));
    }
  else
    {
      /* glu32.dll */

      GDK_GL_NOTE (MISC, g_message (" - Get glu32.dll module handle"));

      hmodule = GetModuleHandle ("glu32.dll");
      if (hmodule == NULL)
        g_warning ("Cannot get glu32.dll module handle");
      else
        proc_address = (GdkGLProc) GetProcAddress (hmodule, proc_name);

      GDK_GL_NOTE (IMPL, g_message (" ** GetProcAddress () - %s",
                                    proc_address ? "succeeded" : "failed"));
    }

  return proc_address;
}
