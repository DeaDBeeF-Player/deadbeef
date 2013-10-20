/* GtkGLExt - OpenGL Extension to GTK+
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
#include <stdlib.h>

#include "gtkglprivate.h"
#include "gtkglinit.h"

static gboolean gtk_gl_initialized = FALSE;

guint gtk_gl_debug_flags = 0;   /* Global GtkGLExt debug flag */

#ifdef G_ENABLE_DEBUG

static const GDebugKey gtk_gl_debug_keys[] = {
  {"misc", GTK_GL_DEBUG_MISC},
  {"func", GTK_GL_DEBUG_FUNC}
};

static const guint gtk_gl_ndebug_keys = G_N_ELEMENTS (gtk_gl_debug_keys);

#endif /* G_ENABLE_DEBUG */

/**
 * gtk_gl_parse_args:
 * @argc: the number of command line arguments.
 * @argv: the array of command line arguments.
 * 
 * Parses command line arguments, and initializes global
 * attributes of GtkGLExt.
 *
 * Any arguments used by GtkGLExt are removed from the array and
 * @argc and @argv are updated accordingly.
 *
 * You shouldn't call this function explicitely if you are using
 * gtk_gl_init(), or gtk_gl_init_check().
 *
 * Return value: %TRUE if initialization succeeded, otherwise %FALSE.
 **/
gboolean
gtk_gl_parse_args (int    *argc,
                   char ***argv)
{
  const gchar *env_string;

  if (gtk_gl_initialized)
    return TRUE;

  /*
   * If window manager doesn't watch the WM_COLORMAP_WINDOWS property on
   * the top-level window, we have to set OpenGL window's colormap to the
   * top-level window, especially in color index mode (color index mode
   * uses own private colormap).
   */
  env_string = g_getenv ("GTK_GL_INSTALL_COLORMAP");
  if (env_string != NULL)
    {
      _gtk_gl_widget_install_toplevel_cmap = (atoi (env_string) != 0);
      env_string = NULL;
    }

#ifdef G_ENABLE_DEBUG
  env_string = g_getenv ("GTK_GL_DEBUG");
  if (env_string != NULL)
    {
      gtk_gl_debug_flags = g_parse_debug_string (env_string,
                                                 gtk_gl_debug_keys,
                                                 gtk_gl_ndebug_keys);
      env_string = NULL;
    }
#endif	/* G_ENABLE_DEBUG */

  if (argc && argv)
    {
      gint i, j, k;
      
      for (i = 1; i < *argc;)
	{
          if (strcmp ("--gtk-gl-install-colormap", (*argv)[i]) == 0)
            {
              _gtk_gl_widget_install_toplevel_cmap = TRUE;
              (*argv)[i] = NULL;
            }
#ifdef G_ENABLE_DEBUG
          else if ((strcmp ("--gtk-gl-debug", (*argv)[i]) == 0) ||
                   (strncmp ("--gtk-gl-debug=", (*argv)[i], 15) == 0))
	    {
	      gchar *equal_pos = strchr ((*argv)[i], '=');
	      
	      if (equal_pos != NULL)
		{
		  gtk_gl_debug_flags |= g_parse_debug_string (equal_pos+1,
                                                              gtk_gl_debug_keys,
                                                              gtk_gl_ndebug_keys);
		}
	      else if ((i + 1) < *argc && (*argv)[i + 1])
		{
		  gtk_gl_debug_flags |= g_parse_debug_string ((*argv)[i+1],
                                                              gtk_gl_debug_keys,
                                                              gtk_gl_ndebug_keys);
		  (*argv)[i] = NULL;
		  i += 1;
		}
	      (*argv)[i] = NULL;
	    }
	  else if ((strcmp ("--gtk-gl-no-debug", (*argv)[i]) == 0) ||
		   (strncmp ("--gtk-gl-no-debug=", (*argv)[i], 18) == 0))
	    {
	      gchar *equal_pos = strchr ((*argv)[i], '=');
	      
	      if (equal_pos != NULL)
		{
		  gtk_gl_debug_flags &= ~g_parse_debug_string (equal_pos+1,
                                                               gtk_gl_debug_keys,
                                                               gtk_gl_ndebug_keys);
		}
	      else if ((i + 1) < *argc && (*argv)[i + 1])
		{
		  gtk_gl_debug_flags &= ~g_parse_debug_string ((*argv)[i+1],
                                                               gtk_gl_debug_keys,
                                                               gtk_gl_ndebug_keys);
		  (*argv)[i] = NULL;
		  i += 1;
		}
	      (*argv)[i] = NULL;
	    }
#endif /* G_ENABLE_DEBUG */
	  i += 1;
	}
      
      for (i = 1; i < *argc; i++)
	{
	  for (k = i; k < *argc; k++)
	    if ((*argv)[k] != NULL)
	      break;
	  
	  if (k > i)
	    {
	      k -= i;
	      for (j = i + k; j < *argc; j++)
		(*argv)[j-k] = (*argv)[j];
	      *argc -= k;
	    }
	}

    }

  /* Set the 'initialized' flag. */
  gtk_gl_initialized = TRUE;

  return TRUE;
}

/**
 * gtk_gl_init_check:
 * @argc: Address of the <parameter>argc</parameter> parameter of your 
 *        <function>main()</function> function. Changed if any arguments
 *        were handled.
 * @argv: Address of the <parameter>argv</parameter> parameter of 
 *        <function>main()</function>. Any parameters understood by
 *        gtk_gl_init() are stripped before return.
 * 
 * This function does the same work as gtk_gl_init() with only 
 * a single change: It does not terminate the program if the library can't be 
 * initialized. Instead it returns %FALSE on failure.
 *
 * This way the application can fall back to some other means of communication 
 * with the user - for example a curses or command line interface.
 * 
 * Return value: %TRUE if the GUI has been successfully initialized, 
 *               %FALSE otherwise.
 **/
gboolean
gtk_gl_init_check (int    *argc,
                   char ***argv)
{
  /* Init GdkGLExt library. */
  if (!gdk_gl_init_check (argc, argv))
    return FALSE;

  /* Parse args and init GtkGLExt library. */
  if (!gtk_gl_parse_args (argc, argv))
    {
      g_warning ("GtkGLExt library initialization fails.");
      return FALSE;
    }

  return TRUE;
}

/**
 * gtk_gl_init:
 * @argc: Address of the <parameter>argc</parameter> parameter of your 
 *        <function>main()</function> function. Changed if any arguments
 *        were handled.
 * @argv: Address of the <parameter>argv</parameter> parameter of 
 *        <function>main()</function>. Any parameters understood by
 *        gtk_gl_init() are stripped before return.
 * 
 * Call this function before using any other GtkGLExt functions in your 
 * applications.  It will initialize everything needed to operate the library
 * and parses some standard command line options. @argc and 
 * @argv are adjusted accordingly so your own code will 
 * never see those standard arguments.
 *
 * <note><para>
 * This function will terminate your program if it was unable to initialize 
 * the library for some reason. If you want your program to fall back to a 
 * textual interface you want to call gtk_gl_init_check() instead.
 * </para></note>
 **/
void
gtk_gl_init (int    *argc,
             char ***argv)
{
  if (!gtk_gl_init_check (argc, argv))
    exit (1);
}
