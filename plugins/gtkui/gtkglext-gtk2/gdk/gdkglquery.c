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

#include <stdlib.h>
#include <string.h>

#include "gdkglprivate.h"
#include "gdkglquery.h"

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#include <GL/gl.h>

/*
 * This code is based on glutExtensionSupported().
 */

/**
 * gdk_gl_query_gl_extension:
 * @extension: name of OpenGL extension.
 *
 * Determines whether a given OpenGL extension is supported.
 *
 * There must be a valid current rendering context to call
 * gdk_gl_query_gl_extension().
 *
 * gdk_gl_query_gl_extension() returns information about OpenGL extensions
 * only. This means that window system dependent extensions (for example,
 * GLX extensions) are not reported by gdk_gl_query_gl_extension().
 *
 * Return value: TRUE if the OpenGL extension is supported, FALSE if not 
 *               supported.
 **/
gboolean
gdk_gl_query_gl_extension (const char *extension)
{
  static const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr (extension, ' ');
  if (where || *extension == '\0')
    return FALSE;

  if (extensions == NULL)
    extensions = glGetString (GL_EXTENSIONS);

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string.  Don't be fooled by sub-strings,
     etc. */
  start = extensions;
  for (;;)
    {
      /* If your application crashes in the strstr routine below,
         you are probably calling gdk_gl_query_gl_extension without
         having a current window.  Calling glGetString without
         a current OpenGL context has unpredictable results.
         Please fix your program. */
      where = (GLubyte *) strstr ((const char *) start, extension);
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

/*< private >*/
void
_gdk_gl_print_gl_info (void)
{
  static gboolean done = FALSE;

  if (!done)
    {
      g_message (" -- GL_VENDOR     : %s", glGetString (GL_VENDOR));
      g_message (" -- GL_RENDERER   : %s", glGetString (GL_RENDERER));
      g_message (" -- GL_VERSION    : %s", glGetString (GL_VERSION));
      g_message (" -- GL_EXTENSIONS : %s", glGetString (GL_EXTENSIONS));

      done = TRUE;
    }
}
