/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-2002  Damon Chaplin
 *  Copyright (C) 2002 Sun Microsystems, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "gladeconfig.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <gtk/gtkmain.h>
#include <gtk/gtkrc.h>

#ifdef USE_GNOME
#include <gnome.h>
#include <bonobo.h>
#include <libgnomeui/gnome-window-icon.h>
#endif

#include "glade.h"
#include "glade_palette.h"
#include "glade_project.h"
#include "glade_project_window.h"
#include "property.h"
#include "save.h"
#include "tree.h"
#include "utils.h"


/* These are the arguments parsed from the command line. */
static gchar *arg_filename     = NULL;	/* The XML file to load on start-up. */
static int    arg_write_source = 0;	/* Set to write the source & exit. */
static int    arg_hide_palette = 0;
static int    arg_hide_property_editor = 0;
static int    arg_show_widget_tree = 0;
static int    arg_show_clipboard = 0;


static void parse_command_line (int argc, char *argv[]);
static guint final_setup_from_main_loop (gpointer data);
static void write_source (void);
static void usage (void);


#ifdef USE_GNOME
static poptContext pctx;

static struct poptOption options[] = {
  {
    "write-source", 'w', POPT_ARG_NONE, &arg_write_source, 0,
    N_("Write the source code and exit"), NULL
  },
  {
    "hide-palette", '\0', POPT_ARG_NONE, &arg_hide_palette, 0,
    N_("Start with the palette hidden"), NULL
  },
  {
    "hide-property-editor", '\0', POPT_ARG_NONE, &arg_hide_property_editor, 0,
    N_("Start with the property editor hidden"), NULL
  },
  {
    "show-widget-tree", '\0', POPT_ARG_NONE, &arg_show_widget_tree, 0,
    N_("Show the widget tree"), NULL
  },
  {
    "show-clipboard", '\0', POPT_ARG_NONE, &arg_show_clipboard, 0,
    N_("Show the clipboard"), NULL
  },
  {
    NULL, '\0', 0, NULL, 0, NULL, NULL
  }
};

static gint session_save_yourself_cb (GnomeClient *client, gint phase,
				      GnomeRestartStyle save_style,
				      gboolean shutdown,
				      GnomeInteractStyle interact_style,
				      gboolean fast,
				      gpointer client_data);
static void session_die_cb (GnomeClient *client, gpointer client_data);
#else
static GOptionEntry options[] = {
    {
        "write-source", 'w', 0, G_OPTION_ARG_NONE, &arg_write_source,
        N_("Write the source code and exit"), NULL
    },
    {
        "hide-palette", '\0', 0, G_OPTION_ARG_NONE, &arg_hide_palette,
        N_("Start with the palette hidden"), NULL
    },
    {
        "hide-property-editor", '\0', 0, G_OPTION_ARG_NONE, &arg_hide_property_editor,
        N_("Start with the property editor hidden"), NULL
    },
    {
        "show-widget-tree", '\0', 0, G_OPTION_ARG_NONE, &arg_show_widget_tree,
        N_("Show the widget tree"), NULL
    },
    {
        "show-clipboard", '\0', 0, G_OPTION_ARG_NONE, &arg_show_clipboard,
        N_("Show the clipboard"), NULL
    },
    { NULL }
};
#endif


#ifdef USE_GNOME
GnomeClient *GladeClient;
#endif


static void
glade_log_handler (const gchar *log_domain,
		   GLogLevelFlags log_level,
		   const gchar *message,
		   gpointer user_data)
{
  /* We just want to ignore this warning as it happens in normal circumstances
     and just confuses users. */
  if (!strcmp (message, "gtk_scrolled_window_add(): cannot add non scrollable widget use gtk_scrolled_window_add_with_viewport() instead"))
    return;

  g_log_default_handler (log_domain, log_level, message, user_data);
}


int
main (int argc, char *argv[])
{
  gchar *home_dir, *rc_path, *modules, *modules_needed, *new_modules;
#ifdef USE_GNOME
  GnomeProgram *program;
  char *icon;
#endif

  home_dir = NULL;
  rc_path = NULL;

  /* We need to ensure that gail is loaded, so that we can query accessibility
     info. I can't see a GTK+ function to do that. For now we just add the
     modules we need to the GTK_MODULES environment variable. It doesn't
     matter if modules appear twice. */
  modules = (char*) g_getenv ("GTK_MODULES");
#ifdef USE_GNOME
  modules_needed = "gail" G_SEARCHPATH_SEPARATOR_S "gail-gnome";
#else
  modules_needed = "gail";
#endif
  new_modules = g_strdup_printf ("GTK_MODULES=%s%s%s",
				 modules ? modules : "",
				 modules ? G_SEARCHPATH_SEPARATOR_S : "",
				 modules_needed);
  putenv (new_modules);

#ifdef USE_GNOME
    /* Gnome sets the locale and parses rc files automatically. */
   program = gnome_program_init ("glade-2", VERSION,
				 LIBGNOMEUI_MODULE,
				 argc, argv, 
				 GNOME_PARAM_POPT_TABLE, options,
				 GNOME_PARAM_APP_DATADIR, GLADE_DATADIR,
				 NULL);
   icon = gnome_program_locate_file (program, GNOME_FILE_DOMAIN_APP_PIXMAP,
				     "glade-2.png", TRUE, NULL);

   if (icon)
	   gnome_window_icon_set_default_from_file (icon);
   g_free (icon);
				     
   g_object_get (G_OBJECT (program),
		 GNOME_PARAM_POPT_CONTEXT, &pctx,
		 NULL);

   /* Set up session management*/
   GladeClient = gnome_master_client ();		 
			 
   g_signal_connect (GladeClient, "save_yourself",
		     G_CALLBACK (session_save_yourself_cb), (gpointer)argv[0]);
   g_signal_connect (GladeClient, "die", G_CALLBACK (session_die_cb), NULL);

#else

  gtk_set_locale ();

  /* For GTK 1.2, default GTK rc files are parsed automatically. */
  home_dir = (gchar*) g_get_home_dir ();
  if (home_dir)
    {
      rc_path = g_strdup_printf ("%s/.gladerc", home_dir);
      gtk_rc_add_default_file (rc_path);
      g_free (rc_path);
    }
  gtk_init_with_args (&argc, &argv, "file", options, NULL, NULL);
#endif

  /* Ignore Ctrl-C. */
  /*signal (SIGINT, SIG_IGN);*/

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, GLADE_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  glade_app_init ();

  parse_command_line (argc, argv);

  /* If the --write-source option is passed, we just write the source and exit
     without even entering the GTK+ main loop. */
  if (arg_write_source)
    write_source ();

  /* We can't make any CORBA calls unless we're in the main loop.
     So we delay loading of files until then. */
  gtk_idle_add ((GtkFunction) final_setup_from_main_loop, NULL);

  g_log_set_handler ("Gtk", G_LOG_LEVEL_WARNING, glade_log_handler, NULL);

#ifdef USE_GNOME
  bonobo_main ();
#else
  gtk_main ();
#endif
  return 0;
}


/* This looks for a .glade file to load, after all the popt options have
   been removed. */
#ifdef USE_GNOME
static void
parse_command_line (int argc, char *argv[])
{
  const gchar **args;
  gint i;

  args = poptGetArgs (pctx);

  for (i = 0; args && args[i]; i++)
    {
      if (arg_filename == NULL)
	arg_filename = (gchar*) args[i];
      else
	usage ();
    }

  poptFreeContext (pctx);
}

#else
static void
parse_command_line (int argc, char *argv[])
{
  gint i;

  /* GTK parses argc & argv and sets arguments to NULL if it has used them. */
  for (i = 1; i < argc; i++)
    {
      if (!argv[i]) continue;
      if (!strcmp (argv[i], "-w") || !strcmp (argv[i], "--write-source"))
	arg_write_source = 1;
      else if (!strcmp (argv[i], "--version"))
	{
	  printf ("Glade (GTK+) %s\n", VERSION);
	  exit (0);
	}
      else if (arg_filename == NULL)
	arg_filename = (gchar*) argv[i];
      else
	usage ();
    }
}
#endif


#ifdef USE_GNOME
/* Returns the session file, in on-disk encoding (We are assuming that
   gnome_client_get_config_prefix() returns on-disk encoded filenames). */
static char*
get_session_file (GnomeClient *client)
{
  const char *config_prefix;
  char *prefix, *session_file;

  /* Get the config prefix, copy it, and remove the '/' at each end. */
  config_prefix = gnome_client_get_config_prefix (client);
  prefix = g_strdup (config_prefix + 1);
  prefix[strlen (prefix) - 1] = '\0';

  /* Create the directory containing all the sessions, if necessary.
     gnome-client.h suggests using ~/.gnome2/<app>.d/session/ to save session
     files, so that is what we do. */
  session_file = g_build_filename (gnome_user_dir_get(), "glade-2.d",
				   "sessions", prefix, "project.glade", NULL);
  g_free (prefix);

  return session_file;
}
#endif


static void
show_window (GtkWidget *window, gboolean show)
{
  if (show)
    gtk_widget_show (window);
  else
    gtk_widget_hide (window);
}


/* This creates the main GUI windows and loads any XML file specified from the
   command-line. We do this here because we can't make any Bonob calls until
   the main loop is running. */
static guint
final_setup_from_main_loop (gpointer data)
{
  GladeProjectWindow *project_window;
  gchar *directory, *pathname = NULL, *pathname_utf8 = NULL;
#ifdef USE_GNOME
  gchar *session_file = NULL, *session_file_utf8 = NULL;
#endif
  gboolean loaded_file = FALSE;
  gboolean show_palette = TRUE, show_property_editor = TRUE;
  gboolean show_widget_tree = FALSE, show_clipboard = FALSE;

  project_window = glade_project_window_new ();

  /* Load the window geometries. */
  glade_load_settings (project_window,
		       glade_palette, &show_palette,
		       win_property, &show_property_editor,
		       win_tree, &show_widget_tree,
		       glade_clipboard, &show_clipboard);

  /* Always show the main project window. */
  gtk_widget_show (project_window->window);

  /* Command-line args override the settings file. */
  if (arg_hide_palette)
    show_palette = FALSE;
  if (arg_hide_property_editor)
    show_property_editor = FALSE;
  if (arg_show_widget_tree)
    show_widget_tree = TRUE;
  if (arg_show_clipboard)
    show_clipboard = TRUE;

  show_window (glade_palette, show_palette);
  show_window (win_property, show_property_editor);
  show_window (win_tree, show_widget_tree);
  show_window (glade_clipboard, show_clipboard);

   if (arg_filename)
    {
      directory = g_get_current_dir ();
      pathname = glade_util_make_absolute_path (directory, arg_filename);
      pathname_utf8 = g_filename_to_utf8 (pathname, -1, NULL, NULL, NULL);
      g_free (directory);
    }

  /* First we check for a saved session, else we check for a normal project
     file on the command-line. */
#ifdef USE_GNOME
  session_file = get_session_file (GladeClient);
  session_file_utf8 = g_filename_to_utf8 (session_file, -1, NULL, NULL,
					  NULL);
  if (g_file_test (session_file, G_FILE_TEST_EXISTS))
    {
      struct stat pathname_stat, session_file_stat;
      int status1 = -1, status2 = -1;

      /* Note that pathname may be NULL, if the project hasn't been
	 saved yet. NOTE: This could cause problems if the user explicitly
	 saves a session without saving the Glade project, and then keeps
	 using that session on startup. Glade will always think the project
	 is a new one, even if the user saves it at some point. (Sessions
	 shouldn't be used this way really.) */
      if (pathname)
	{
	  status1 = stat (pathname, &pathname_stat);
	  status2 = stat (session_file, &session_file_stat);
	}

      /* We only want to load the session file if it is newer than the
	 real project XML file. i.e. its mtime is greater. */
      if (status1 == -1 || status2 == -1
	  || session_file_stat.st_mtime >= pathname_stat.st_mtime)
	{
	  GladeSessionFile = session_file_utf8;
	  glade_project_window_open_project (project_window, pathname_utf8);
	  GladeSessionFile = NULL;
	  loaded_file = TRUE;
	}
    }
  g_free (session_file);
  g_free (session_file_utf8);
#endif

   if (!loaded_file && pathname_utf8)
    {
      glade_project_window_open_project (project_window, pathname_utf8);
    }

  g_free (pathname);
  g_free (pathname_utf8);

  /* For GNOME, we start with no current project, as the user needs to select
     between GTK+ and GNOME when creating a project. For GTK+ we can just
     create a GTK+ project. */
#ifdef USE_GNOME
  if (current_project == NULL)
    gtk_widget_set_sensitive (GTK_BIN (glade_palette)->child, FALSE);
#else
  if (current_project == NULL)
    glade_project_new ();
#endif

  glade_project_window_set_project (project_window, current_project);

  glade_project_window_refresh_menu_items ();

  return FALSE;
}


/* Outputs the source code for the project, for when the --write-source option
   is used. This function will not return. It exits with 0 if OK, or 1 if
   there was an error writing the source. */
static void
write_source (void)
{
  GladeProject *project;
  gboolean status;
  GladeError *error;
  gchar *directory, *directory_utf8, *filename;

  if (!arg_filename) {
    g_printerr (_("glade: The XML file must be set for the '-w' or '--write-source' option.\n"));
    exit (1);
  }

  directory = g_get_current_dir ();
  directory_utf8 = g_filename_to_utf8 (directory, -1, NULL, NULL, NULL);
  filename = glade_util_make_absolute_path (directory_utf8, arg_filename);
  g_free (directory_utf8);
  g_free (directory);

  status = glade_project_open (filename, &project);
  g_free (filename);

  if (!status) {
    g_printerr (_("glade: Error loading XML file.\n"));
    /* The errors aren't freed, but it doesn't really matter. */
    exit (1);
  }

  error = glade_project_write_source (project);
  if (error) {
    g_printerr (_("glade: Error writing source.\n"));
    glade_error_free (error);
    exit (1);
  }

  exit (0);
}


/* Display the available command-line options and exit. Used when an invalid
   argument was passed in. */
static void
usage (void)
{
  fprintf (stderr, "Usage: glade [-w|--write-source] [<filename>]\n");
  exit (0);
}


#if USE_GNOME
static void
glade_write_session_files (GnomeClient *client)
{
  GladeError* error;
  char *session_file = NULL, *session_dir = NULL;
  char *session_file_utf8 = NULL, *session_dir_utf8 = NULL;
  gchar *argv[] = { "rm", "-r", NULL };

  session_file = get_session_file (client);
  session_dir = g_dirname (session_file);
  session_file_utf8 = g_filename_to_utf8 (session_file, -1, NULL, NULL, NULL);
  session_dir_utf8 = g_filename_to_utf8 (session_dir, -1, NULL, NULL, NULL);

  error = glade_util_ensure_directory_exists (session_dir_utf8);
  if (error)
    {
      fprintf (stderr, error->message);
      glade_error_free (error);
      goto out;
    }

  /* Set a global variable to the session file to save. */
  GladeSessionFile = session_file_utf8;
  error = save_project_file (current_project);
  GladeSessionFile = NULL;
  if (error)
    {
      fprintf (stderr, error->message);
      glade_error_free (error);
    }

  /* Set the discard command, so the session files will be deleted when the
     session is. It copies all the args, so we can free our copies.
     Note that we use on-disk encoding here, as I assume that is what 'rm'
     wants. */
  argv[2] = session_dir;
  gnome_client_set_discard_command (client, 3, argv);

 out:

  g_free (session_dir);
  g_free (session_file);
  g_free (session_dir_utf8);
  g_free (session_file_utf8);
}


static gint
session_save_yourself_cb (GnomeClient *client, gint phase,
			  GnomeRestartStyle save_style, gboolean shutdown,
			  GnomeInteractStyle interact_style, gboolean fast,
			  gpointer client_data)
{
  char *argv[10]; /* Make sure this has enough space for all possible args. */
  int arg = 0;

  /* FIXME: Hardcoded path just for testing. */
#if 0
  argv[arg++] = "/home/damon/cvs/glade-gtk2/glade/glade-2";
#else
  argv[arg++] = "glade-2";
#endif

  if (!glade_palette || !GTK_WIDGET_VISIBLE (glade_palette))
    {
      argv[arg++] = "--hide-palette";
    }

  if (!win_property || !GTK_WIDGET_VISIBLE (win_property))
    {
      argv[arg++] = "--hide-property-editor";
    }

  if (win_tree && GTK_WIDGET_VISIBLE (win_tree))
    {
      argv[arg++] = "--show-widget-tree";
    }

  if (glade_clipboard && GTK_WIDGET_VISIBLE (glade_clipboard))
    {
      argv[arg++] = "--show-clipboard";
    }

  /* Check if we need to save the project files. */
  if (current_project && current_project->components)
    {
      /* We save the XML filename as an arg, but the saved session files will
	 be loaded instead. Though we need to know the XML filename for saving
	 the project later. */
      if (current_project->xml_filename
	  && *current_project->xml_filename)
	argv[arg++] = current_project->xml_filename;

      glade_write_session_files (client);
    }

  gnome_client_set_restart_command (client, arg, argv);

  return TRUE;
}
 

static void
session_die_cb (GnomeClient *client, gpointer client_data)
{
  gtk_main_quit ();
}
#endif
