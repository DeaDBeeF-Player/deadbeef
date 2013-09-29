
/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998  Damon Chaplin
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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <ctype.h>
#include <locale.h>

#include <gtk/gtkmenu.h>

#include "gladeconfig.h"

#include "gbwidget.h"
#include "glade_project.h"
#include "source.h"
#include "utils.h"
#ifdef HAVE_OS2_H
#include "source_os2.h"
#endif

/* Turn this on to add -DG_DISABLE_DEPRECATED etc. flags in generated code,
   so we can check if Glade is generating any deprecated code. */
#if 0
#define GLADE_ADD_DISABLE_DEPRECATED_FLAGS
#endif

/* An internal struct to pass data to the source_write_component() callback. */
typedef struct _GladeSourceCallbackData GladeSourceCallbackData;
struct _GladeSourceCallbackData
{
  GbWidgetWriteSourceData *write_source_data;
  FILE *interface_h_fp;
  FILE *interface_c_fp;
  FILE *callback_h_fp;
  FILE *callback_c_fp;
};


static GladeError* source_write_internal (GladeProject * project);

static void source_write_interface_and_callbacks (GbWidgetWriteSourceData * data);
static void source_begin_interface_and_callbacks_files (GbWidgetWriteSourceData * data,
							FILE **interface_h_fp,
							FILE **interface_c_fp,
							FILE **callback_h_fp,
							FILE **callback_c_fp);
static void source_output_interface_and_callbacks_source (GbWidgetWriteSourceData * data,
							  FILE *interface_h_fp,
							  FILE *interface_c_fp,
							  FILE *callback_h_fp,
							  FILE *callback_c_fp);
static void source_write_component (GtkWidget * component,
				    GladeSourceCallbackData * source_data);

static void source_write_interface_h_preamble (GbWidgetWriteSourceData * data,
					       FILE *fp);
static void source_write_interface_c_preamble (GbWidgetWriteSourceData * data,
					       FILE *fp);
static void source_write_callback_h_preamble (GbWidgetWriteSourceData * data,
					      FILE *fp);
static void source_write_callback_c_preamble (GbWidgetWriteSourceData * data,
					      FILE *fp);
static void source_write_preamble (gchar *project_name,
				   FILE * fp);

static void source_write_main_c (GbWidgetWriteSourceData * data);
static void source_write_component_create (GtkWidget * component,
					   GbWidgetWriteSourceData * data);

static void source_write_build_files (GbWidgetWriteSourceData * data);
static void source_write_autogen_sh (GbWidgetWriteSourceData * data);

static void source_write_gtk_build_files (GbWidgetWriteSourceData * data);
static void source_write_gtk_configure_in (GbWidgetWriteSourceData * data);
static void source_write_gtk_makefile_am (GbWidgetWriteSourceData * data);
static void source_write_gtk_makefile_am_pixmaps_targets (GbWidgetWriteSourceData * data,
							  gchar                   * directory,
							  FILE                    * fp);

static void source_write_gnome_build_files (GbWidgetWriteSourceData * data);
static void source_write_gnome_configure_in (GbWidgetWriteSourceData * data);
static void source_write_gnome_makefile_am (GbWidgetWriteSourceData * data);
static void source_write_gnome_makefile_am_pixmaps_targets (GbWidgetWriteSourceData * data,
							    gchar                   * directory,
							    FILE                    * fp);

static void source_write_common_build_files (GbWidgetWriteSourceData * data);
static void source_write_toplevel_makefile_am (GbWidgetWriteSourceData * data);
static void source_write_extra_dist (GbWidgetWriteSourceData * data,
				     const gchar *directory,
				     FILE *fp);
static GladeError* source_create_file_if_not_exist (const gchar *directory,
						    const gchar *filename,
						    const gchar *contents);
static void source_write_po_files (GbWidgetWriteSourceData * data);
#if 0
static void source_write_acconfig_h (GbWidgetWriteSourceData * data);
#endif
static void source_write_support_files (GbWidgetWriteSourceData * data);

static void source_write_gtk_create_pixmap_functions (GbWidgetWriteSourceData * data,
						      FILE *fp);
static void source_write_gnome_create_pixmap_functions (GbWidgetWriteSourceData * data,
							FILE *fp);

static gchar* source_is_valid_source_filename (const gchar * filename);
static GladeError* source_backup_file_if_exists (const gchar * filename);

static gchar * source_make_string_internal (const gchar * text,
					    gboolean translatable,
					    gboolean is_static,
					    gboolean context);
static void source_reset_code_buffers (GbWidgetWriteSourceData * data);
static void source_destroy_standard_widgets_callback (gchar * key,
						      GtkWidget * widget,
						      gpointer data);
static void source_free_hash_keys_callback (gchar * key,
					    gpointer value,
					    gpointer data);
static gchar* source_get_source_subdirectory (GbWidgetWriteSourceData * data);
static void source_write_no_editing_warning (FILE *fp);
static void source_write_include_files (FILE *fp);


/* We need this so that numbers are written in C syntax rather than the
   current locale, which may use ',' instead of '.' and then the code
   will not compile. This code is from glibc info docs. */
GladeError*
source_write (GladeProject *project)
{
  gchar *old_locale, *saved_locale;
  GladeError *error;
     
  old_locale = setlocale (LC_NUMERIC, NULL);
  saved_locale = g_strdup (old_locale);
  setlocale (LC_NUMERIC, "C");
  error = source_write_internal (project);
  setlocale (LC_NUMERIC, saved_locale);
  g_free (saved_locale);
  return error;
}


static GladeError*
source_write_internal (GladeProject * project)
{
  GbWidgetWriteSourceData data;
  gchar *source_directory, *interface_source_file, *interface_header_file;
  gchar *callback_source_file, *callback_header_file, *msg;
  gchar *support_source_file, *support_header_file;
  gint i;

  source_directory = glade_project_get_source_directory (project);
  glade_project_get_source_files (project,
				  &interface_source_file,
				  &interface_header_file,
				  &callback_source_file,
				  &callback_header_file);

  /* Check that any source filenames that we are going to use are valid. */
  if ((msg = source_is_valid_source_filename (interface_source_file)))
    return glade_error_new_general (GLADE_STATUS_ERROR, _("Invalid interface source filename: %s\n%s\n"), interface_source_file, msg);
  if ((msg = source_is_valid_source_filename (interface_header_file)))
    return glade_error_new_general (GLADE_STATUS_ERROR, _("Invalid interface header filename: %s\n%s\n"), interface_header_file, msg);

  if ((msg = source_is_valid_source_filename (callback_source_file)))
    return glade_error_new_general (GLADE_STATUS_ERROR, _("Invalid callbacks source filename: %s\n%s\n"), callback_source_file, msg);
  if ((msg = source_is_valid_source_filename (callback_header_file)))
    return glade_error_new_general (GLADE_STATUS_ERROR, _("Invalid callbacks header filename: %s\n%s\n"), callback_source_file, msg);

  if (glade_project_get_output_support_files (project)) {
    support_source_file = glade_project_get_support_source_file (project);
    support_header_file = glade_project_get_support_header_file (project);
    if ((msg = source_is_valid_source_filename (support_source_file)))
      return glade_error_new_general (GLADE_STATUS_ERROR, _("Invalid support source filename: %s\n%s\n"), support_source_file, msg);
    if ((msg = source_is_valid_source_filename (support_header_file)))
      return glade_error_new_general (GLADE_STATUS_ERROR, _("Invalid support header filename: %s\n%s\n"), support_header_file, msg);
  }

  /* Initialize the GbWidgetWriteSourceData fields.
     Note that the error is set to NULL. If anything sets it we know there is
     an error. */
  data.project = project;
  data.error = NULL;
  data.project_name = glade_project_get_name (project);
  data.program_name = glade_project_get_program_name (project);
  data.interface_c_filename = glade_util_make_absolute_path (source_directory,
							     interface_source_file);
  data.interface_h_filename = glade_util_make_absolute_path (source_directory,
							     interface_header_file);
  data.callback_c_filename = glade_util_make_absolute_path (source_directory,
							    callback_source_file);
  data.callback_h_filename = glade_util_make_absolute_path (source_directory,
							    callback_header_file);
  data.set_widget_names = glade_project_get_use_widget_names (project);
  data.use_gettext = glade_project_get_gettext_support (project);
  data.use_component_struct = FALSE;
  data.creating_callback_files = FALSE;
  data.standard_widgets = g_hash_table_new (g_str_hash, g_str_equal);
  data.handlers_output = g_hash_table_new (g_str_hash, g_str_equal);

  /* Create the empty source code buffers. */
  for (i = 0; i < GLADE_NUM_SOURCE_BUFFERS; i++)
    data.source_buffers[i] = g_string_sized_new (1024);

  /* If the callback.c file doesn't exists, we need to output all signal
     handlers & callbacks. If it exists, we only write handlers & callbacks
     that have been added/changed since the last time the interface.c file
     was written. */
  if (!glade_util_file_exists (data.callback_c_filename))
    {
      data.creating_callback_files = TRUE;
    }
  else
    {
      data.error = glade_util_file_last_mod_time (data.interface_c_filename,
						  &data.last_write_time);

      /* If the interface.c file doesn't exist, we have no way of knowing
	 which callbacks need to be output, so we set the last_write_time
	 to the current time, i.e. we don't output any callbacks. */
      if (data.error && data.error->status == GLADE_STATUS_SYSTEM_ERROR
	  && data.error->system_errno == ENOENT)
	{
	  data.last_write_time = time (NULL);
	  glade_error_free (data.error);
	  data.error = NULL;
	}
    }

  /* Make sure the project & source directories exist. */
  if (!data.error)
    data.error = glade_util_ensure_directory_exists (glade_project_get_directory (data.project));

  if (!data.error)
    data.error = glade_util_ensure_directory_exists (glade_project_get_source_directory (data.project));

  /* Now call the main functions to write the source code and support files. */
  if (!data.error)
    source_write_build_files (&data);
  if (!data.error)
    source_write_support_files (&data);
  if (!data.error)
    source_write_main_c (&data);
  if (!data.error)
    source_write_interface_and_callbacks (&data);

  /* Now free everything. */
  for (i = 0; i < GLADE_NUM_SOURCE_BUFFERS; i++)
    g_string_free (data.source_buffers[i], TRUE);

  g_hash_table_foreach (data.standard_widgets,
			(GHFunc) source_destroy_standard_widgets_callback,
			NULL);
  g_hash_table_destroy (data.standard_widgets);

  g_hash_table_foreach (data.handlers_output,
			(GHFunc) source_free_hash_keys_callback, NULL);
  g_hash_table_destroy (data.handlers_output);

  g_free (data.interface_c_filename);
  g_free (data.interface_h_filename);
  g_free (data.callback_c_filename);
  g_free (data.callback_h_filename);

  return data.error;
}


/*************************************************************************
 * Main source files - interface.[hc] & callbacks.[hc]
 *************************************************************************/

static void
source_write_interface_and_callbacks (GbWidgetWriteSourceData * data)
{
  FILE *interface_h_fp = NULL, *interface_c_fp = NULL;
  FILE *callback_h_fp = NULL, *callback_c_fp = NULL;

  /* Backup the two main files, if the backup option is selected.
     We don't backup the signals files since we only ever append to them. */
  if (glade_project_get_backup_source_files (data->project))
    {
      data->error = source_backup_file_if_exists (data->interface_c_filename);
      if (data->error)
	return;

      data->error = source_backup_file_if_exists (data->interface_h_filename);
      if (data->error)
	return;
    }

  /* Open all the files and add the standard license and #include stuff.
     Note that if an error occurs, data->error is set, and we simply drop
     through to close any files which were opened below. */
  source_begin_interface_and_callbacks_files (data,
					      &interface_h_fp,
					      &interface_c_fp,
					      &callback_h_fp,
					      &callback_c_fp);

  if (!data->error)
    {
      source_output_interface_and_callbacks_source (data,
						    interface_h_fp,
						    interface_c_fp,
						    callback_h_fp,
						    callback_c_fp);
    }

  /* Now close any files which were opened. */
  if (interface_h_fp)
    fclose (interface_h_fp);
  if (interface_c_fp)
    fclose (interface_c_fp);
  if (callback_h_fp)
    fclose (callback_h_fp);
  if (callback_c_fp)
    fclose (callback_c_fp);
}


/* Creates the interface.h & interface.c files, and created callbacks.h
   & callback.c or opens them for appending as appropriate. */
static void
source_begin_interface_and_callbacks_files (GbWidgetWriteSourceData * data,
					    FILE **interface_h_fp,
					    FILE **interface_c_fp,
					    FILE **callback_h_fp,
					    FILE **callback_c_fp)
{
  /* Create the interface.h file and output the standard license. */
  *interface_h_fp = glade_util_fopen (data->interface_h_filename, "w");
  if (*interface_h_fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    data->interface_h_filename);
      return;
    }

  source_write_interface_h_preamble (data, *interface_h_fp);
  if (data->error)
    return;


  /* Create the interface.c file and output the standard license and #include
     lines. */
  *interface_c_fp = glade_util_fopen (data->interface_c_filename, "w");
  if (*interface_c_fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    data->interface_c_filename);
      return;
    }

  source_write_interface_c_preamble (data, *interface_c_fp);
  if (data->error)
    return;


  /* If the callback.[hc] files are being created from scratch, create them
     and and output the standard license and #include stuff, else just open
     them for appending. */
  if (data->creating_callback_files)
    {
      *callback_h_fp = glade_util_fopen (data->callback_h_filename, "w");
      if (*callback_h_fp == NULL)
	{
	  data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
						data->callback_h_filename);
	  return;
	}

      source_write_callback_h_preamble (data, *callback_h_fp);
      if (data->error)
	return;


      *callback_c_fp = glade_util_fopen (data->callback_c_filename, "w");
      if (*callback_c_fp == NULL)
	{
	  data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
						data->callback_c_filename);
	  return;
	}

      source_write_callback_c_preamble (data, *callback_c_fp);
      if (data->error)
	return;
    }
  else
    {
      *callback_h_fp = glade_util_fopen (data->callback_h_filename, "a");
      if (*callback_h_fp == NULL)
	{
	  data->error = glade_error_new_system (_("Couldn't append to file:\n  %s\n"),
						data->callback_h_filename);
	  return;
	}

      *callback_c_fp = glade_util_fopen (data->callback_c_filename, "a");
      if (*callback_c_fp == NULL)
	{
	  data->error = glade_error_new_system (_("Couldn't append to file:\n  %s\n"),
						data->callback_c_filename);
	  return;
	}
    }
}


static void
source_output_interface_and_callbacks_source (GbWidgetWriteSourceData * data,
					      FILE *interface_h_fp,
					      FILE *interface_c_fp,
					      FILE *callback_h_fp,
					      FILE *callback_c_fp)
{
  GladeSourceCallbackData source_data;

  /* This outputs the code to create the components and the signal handler
     prototypes. */
  source_data.write_source_data = data;
  source_data.interface_h_fp = interface_h_fp;
  source_data.interface_c_fp = interface_c_fp;
  source_data.callback_h_fp = callback_h_fp;
  source_data.callback_c_fp = callback_c_fp;

  /* Iterate through the project components outputting the source code and
     declarations. */
  glade_project_foreach_component (data->project,
				   (GtkCallback) source_write_component,
				   &source_data);
}


/* This outputs the source code for one component (a window, dialog or popup
   menu). */
static void
source_write_component (GtkWidget * component,
			GladeSourceCallbackData * source_data)
{
  GbWidgetWriteSourceData * data;
  FILE *interface_h_fp, *interface_c_fp;
  FILE *callback_h_fp, *callback_c_fp;
  
  /* Get the data out of the callback data struct. */
  data = source_data->write_source_data;
  interface_h_fp = source_data->interface_h_fp;
  interface_c_fp = source_data->interface_c_fp;
  callback_h_fp = source_data->callback_h_fp;
  callback_c_fp = source_data->callback_c_fp;

  /* Reset the GbWidgetWriteSourceData, ready to begin a new component. */
  data->component = component;
  data->component_name = source_create_valid_identifier (gtk_widget_get_name (component));
  data->parent = NULL;
  data->need_tooltips = FALSE;
  data->need_accel_group = FALSE;
  data->create_widget = TRUE;
  data->write_children = TRUE;
  data->focus_widget = NULL;
  data->default_widget = NULL;

  /* Clear all the code from the previous component. */
  source_reset_code_buffers (data);

  /* Recursively write the source for all the widgets in the component. */
  gb_widget_write_source (component, data);

  /*
   * Output interface.h
   */

  /* Output the declaration of the function to create the component in the
     header file. */
  fprintf (interface_h_fp, "GtkWidget* create_%s (void);\n",
	   data->component_name);


  /*
   * Output interface.c
   */

  /* Output any GnomeUIInfo structs. */
  fprintf (interface_c_fp, "%s", data->source_buffers[GLADE_UIINFO]->str);

  fprintf (interface_c_fp,
	   "GtkWidget*\n"
	   "create_%s (void)\n"
	   "{\n",
	   data->component_name);

  /* Output the declarations of all the widgets and any temporary variables
     needed at the start of the function. */
  fprintf (interface_c_fp, "%s",
	   data->source_buffers[GLADE_DECLARATIONS]->str);

  /* Output a declaration of accel_group and tooltips if they are needed by
     the component. */
  if (data->need_accel_group)
    fprintf (interface_c_fp, "  GtkAccelGroup *accel_group;\n");
//  if (data->need_tooltips)
//      fprintf (interface_c_fp, "  GtkTooltips *tooltips;\n");

  /* Output a blank line between the declarations and the source. */
  fprintf (interface_c_fp, "\n");

  /* Create the tooltips object if needed. */
//  if (data->need_tooltips)
//      fprintf (interface_c_fp, "  tooltips = gtk_tooltips_new ();\n\n");

  /* Create the accel group if needed. */
  if (data->need_accel_group)
      fprintf (interface_c_fp, "  accel_group = gtk_accel_group_new ();\n\n");

  /* Output the source code to create the widgets in the component. */
  fprintf (interface_c_fp, "%s", data->source_buffers[GLADE_SOURCE]->str);

  /* Output the source code to connect the signal handlers. */
  if (data->source_buffers[GLADE_SIGNAL_CONNECTIONS]->len > 0)
    fprintf (interface_c_fp, "%s\n",
	     data->source_buffers[GLADE_SIGNAL_CONNECTIONS]->str);

  /* Output the source code to setup the accelerator keys. */
  if (data->source_buffers[GLADE_ACCELERATORS]->len > 0)
    fprintf (interface_c_fp, "%s\n",
	     data->source_buffers[GLADE_ACCELERATORS]->str);

  /* Output the source code to set ATK properties. */
  if (data->source_buffers[GLADE_ATK_SOURCE]->len > 0)
    fprintf (interface_c_fp, "%s\n",
	     data->source_buffers[GLADE_ATK_SOURCE]->str);

  /* Output the source code to set the pointers to the widgets. */
  if (data->source_buffers[GLADE_OBJECT_HOOKUP]->len > 0)
    {
      fprintf (interface_c_fp,
	       "  /* Store pointers to all widgets, for use by lookup_widget(). */\n");
      fprintf (interface_c_fp, "%s",
	       data->source_buffers[GLADE_OBJECT_HOOKUP]->str);
    }

  /* Store a pointer to the tooltips object, if we used one, so that it can
     be accessed in callbacks. */
//  if (data->need_tooltips)
//    {
//      fprintf (interface_c_fp,
//	       "  GLADE_HOOKUP_OBJECT_NO_REF (%s, tooltips, \"tooltips\");\n",
//	       data->component_name);
//    }

  fprintf (interface_c_fp, "\n");

  /* Set the focus widget, if there is one. */
  if (data->focus_widget)
    {
      fprintf (interface_c_fp, "  gtk_widget_grab_focus (%s);\n",
	       data->focus_widget);
      g_free (data->focus_widget);
      data->focus_widget = NULL;
    }

  /* Set the default widget, if there is one. */
  if (data->default_widget)
    {
      fprintf (interface_c_fp, "  gtk_widget_grab_default (%s);\n",
	       data->default_widget);
      g_free (data->default_widget);
      data->default_widget = NULL;
    }

  /* Add the accel group to the component. */
  if (data->need_accel_group)
    {
      if (GTK_IS_MENU (data->component))
	fprintf (interface_c_fp, 
		 "  gtk_menu_set_accel_group (GTK_MENU (%s), accel_group);\n\n",
		 data->component_name);
      else
	fprintf (interface_c_fp, 
		 "  gtk_window_add_accel_group (GTK_WINDOW (%s), accel_group);\n\n",
		 data->component_name);
    }

  /* Return the toplevel widget and finish the function. */
  fprintf (interface_c_fp, "  return %s;\n}\n\n", data->component_name);

  /*
   * Output callbacks.h
   */

  /* Output the signal handler declarations. */
  fprintf (callback_h_fp, "%s",
	   data->source_buffers[GLADE_CALLBACK_DECLARATIONS]->str);

  /*
   * Output callbacks.c
   */

  /* Output the signal handler functions. */
  fprintf (callback_c_fp, "%s",
	   data->source_buffers[GLADE_CALLBACK_SOURCE]->str);


  g_free (data->component_name);
  data->component_name = NULL;
}


/* Outputs the license and and other code needed at the top of the interface.h
   header file, before any function declarations are output. */
static void
source_write_interface_h_preamble (GbWidgetWriteSourceData * data, FILE *fp)
{
  source_write_no_editing_warning (fp);
  source_write_preamble (data->project_name, fp);
}

static void
source_write_include_files (FILE *fp)
{
    fprintf (fp,
	   "#ifdef HAVE_CONFIG_H\n"
	   "#  include <config.h>\n"
	   "#endif\n"
	   "\n"
	   "#include <sys/types.h>\n"
	   "#include <sys/stat.h>\n"
	   "#include <unistd.h>\n"
	   "#include <string.h>\n"
	   "#include <stdio.h>\n"
	   "\n");
}

/* Outputs the license and any include files needed at the top of the
   interface.c source file, before code for the components is output. */
static void
source_write_interface_c_preamble (GbWidgetWriteSourceData * data, FILE *fp)
{
  source_write_no_editing_warning (fp);
  source_write_preamble (data->project_name, fp);

  source_write_include_files (fp);

  if (glade_project_get_gnome_support (data->project))
    {
      fprintf (fp, "#include <bonobo.h>\n");
      fprintf (fp, "#include <gnome.h>\n");

      if (glade_project_get_gnome_db_support (data->project))
	{
	  fprintf (fp, "#include <libgnomedb/libgnomedb.h>\n");
	}
    }
  else
    {
      fprintf (fp,
	       "#include <gdk/gdkkeysyms.h>\n"
	       "#include <gtk/gtk.h>\n");
    }

  fprintf (fp,
	   "\n"
	   "#include \"%s\"\n"
	   "#include \"%s\"\n"
	   "#include \"%s\"\n\n",
	   g_basename (data->callback_h_filename),
	   g_basename (data->interface_h_filename),
	   glade_project_get_support_header_file (data->project));

  fprintf (fp,
	   "#define GLADE_HOOKUP_OBJECT(component,widget,name) \\\n"
	   "  g_object_set_data_full (G_OBJECT (component), name, \\\n"
	   "    g_object_ref(G_OBJECT(widget)), (GDestroyNotify) g_object_unref)\n\n");

  fprintf (fp,
	   "#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \\\n"
	   "  g_object_set_data (G_OBJECT (component), name, widget)\n\n");
}


/* Outputs the license and and other code needed at the top of the callback.h
   header file, before any signal handler and callback function declarations
   are output. */
static void
source_write_callback_h_preamble (GbWidgetWriteSourceData * data, FILE *fp)
{
  source_write_preamble (data->project_name, fp);

  if (glade_project_get_gnome_support (data->project))
    fprintf (fp, "#include <gnome.h>\n\n");
  else
    fprintf (fp, "#include <gtk/gtk.h>\n\n");

  if (glade_project_get_gnome_db_support (data->project))
    fprintf (fp, "#include <libgnomedb/libgnomedb.h>\n");
}


/* Outputs the license and any include files needed at the top of the
   callback.c source file, before code for the signal handlers and callback
   functions is output. */
static void
source_write_callback_c_preamble (GbWidgetWriteSourceData * data, FILE *fp)
{
  source_write_preamble (data->project_name, fp);

  fprintf (fp,
	   "#ifdef HAVE_CONFIG_H\n"
	   "#  include <config.h>\n"
	   "#endif\n"
	   "\n");

  if (glade_project_get_gnome_support (data->project))
    fprintf (fp, "#include <gnome.h>\n");
  else
    fprintf (fp, "#include <gtk/gtk.h>\n");

  fprintf (fp,
	   "\n"
	   "#include \"%s\"\n"
	   "#include \"%s\"\n"
	   "#include \"%s\"\n\n",
	   g_basename (data->callback_h_filename),
	   g_basename (data->interface_h_filename),
	   glade_project_get_support_header_file (data->project));
}


/* Output a license at the top of a source or header file.
   Note this will eventually be editable in the user interface, with an option
   to include a few standard licenses, e.g. GPL, which are then edited by the
   user. FIXME: I've taken this out until we support it fully. */
static void
source_write_preamble (gchar *project_name, FILE * fp)
{
#if 0
  fprintf (fp,
	   "/*  Note: You are free to use whatever license you want.\n"
	   "    Eventually you will be able to edit it within Glade. */\n"
	   "\n"
	   "/*  %s\n"
	   " *  Copyright (C) <YEAR> <AUTHORS>\n"
	   " *\n"
	   " *  This program is free software; you can redistribute it and/or modify\n"
	   " *  it under the terms of the GNU General Public License as published by\n"
	   " *  the Free Software Foundation; either version 2 of the License, or\n"
	   " *  (at your option) any later version.\n"
	   " *\n"
	   " *  This program is distributed in the hope that it will be useful,\n"
	   " *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	   " *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	   " *  GNU General Public License for more details.\n"
	   " *\n"
	   " *  You should have received a copy of the GNU General Public License\n"
	   " *  along with this program; if not, write to the Free Software\n"
	   " *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n"
	   "*/\n\n", project_name);
#endif
}


/*************************************************************************
 * main.c file.
 *************************************************************************/

void
source_write_main_c (GbWidgetWriteSourceData * data)
{
  gchar *directory, *source_directory, *filename;
  FILE *fp;

  /* See if the main.c file is wanted. */
  if (!glade_project_get_output_main_file (data->project))
    return;

  directory = glade_project_get_directory (data->project);
  source_directory = glade_project_get_source_directory (data->project);

  filename = glade_util_make_absolute_path (source_directory, "main.c");

  /* Return if it already exists. */
  if (glade_util_file_exists (filename))
    {
      g_free (filename);
      return;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  source_write_preamble (data->project_name, fp);

  fprintf (fp,
	   "/*\n"
	   " * Initial main.c file generated by Glade. Edit as required.\n"
	   " * Glade will not overwrite this file.\n"
	   " */\n\n");

  fprintf (fp,
	   "#ifdef HAVE_CONFIG_H\n"
	   "#  include <config.h>\n"
	   "#endif\n\n");

  if (glade_project_get_gnome_support (data->project))
    {
      fprintf (fp, "#include <gnome.h>\n\n");
    }
  else
    {
      fprintf (fp, "#include <gtk/gtk.h>\n\n");
    }

  /* Include the interface.h header to get declarations of the functions to
     create the components, and support.h so we include libintl.h if needed. */
  fprintf (fp,
	   "#include \"%s\"\n"
	   "#include \"%s\"\n\n",
	   g_basename (data->interface_h_filename),
	   glade_project_get_support_header_file (data->project));

  fprintf (fp,
	   "int\n"
	   "main (int argc, char *argv[])\n"
	   "{\n");

  source_reset_code_buffers (data);

  /* This outputs code in main() to create one of each component, just so that
     the user sees something after first building the project. */
  glade_project_foreach_component (data->project,
				   (GtkCallback) source_write_component_create,
				   data);
  fprintf (fp, "%s\n", data->source_buffers[GLADE_DECLARATIONS]->str);

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp,
	       "#ifdef ENABLE_NLS\n"
	       "  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);\n"
	       "  bind_textdomain_codeset (GETTEXT_PACKAGE, \"UTF-8\");\n"
	       "  textdomain (GETTEXT_PACKAGE);\n"
	       "#endif\n"
	       "\n");
    }

  /* Note that we don't mark the project name as translatable.
     I'm not entirely certain we should do that. */
  if (glade_project_get_gnome_support (data->project))
    {
      fprintf (fp,
	       "  gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,\n"
	       "                      argc, argv,\n"
	       "                      GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR,\n"
	       "                      NULL);\n");
    }
  else 
    {
      fprintf (fp,
	       "  gtk_set_locale ();\n"
	       "  gtk_init (&argc, &argv);\n"
	       "\n");

      fprintf (fp,
	       "  add_pixmap_directory (PACKAGE_DATA_DIR \"/\" PACKAGE \"/pixmaps\");\n");
    }

  fprintf (fp,
	   "\n"
	   "  /*\n"
	   "   * The following code was added by Glade to create one of each component\n"
	   "   * (except popup menus), just so that you see something after building\n"
	   "   * the project. Delete any components that you don't want shown initially.\n"
	   "   */\n");
  fprintf (fp, "%s", data->source_buffers[GLADE_SOURCE]->str);
  fprintf (fp, "\n  gtk_main ();\n  return 0;\n}\n\n");

  fclose (fp);

  g_free (filename);
}


static void
source_write_component_create (GtkWidget * component,
			       GbWidgetWriteSourceData * data)
{
  gchar *component_name;

  /* Don't output code to show popup menus. */
  if (GTK_IS_MENU (component))
    return;

  component_name = (gchar*) gtk_widget_get_name (component);
  component_name = source_create_valid_identifier (component_name);
  source_add_decl (data, "  GtkWidget *%s;\n", component_name);
  source_add (data,
	      "  %s = create_%s ();\n"
	      "  gtk_widget_show (%s);\n",
	      component_name, component_name, component_name);
  g_free (component_name);
}


/*************************************************************************
 * Build Files.
 *************************************************************************/

static void
source_write_build_files (GbWidgetWriteSourceData * data)
{
  /* If the build files aren't wanted, return. */
  if (!glade_project_get_output_build_files (data->project))
    return;

  /* Write the support files - Makefile.am, configure.in, etc. */
  if (glade_project_get_gnome_support (data->project))
    {
      source_write_gnome_build_files (data);
    }
  else
    {
      source_write_gtk_build_files (data);
    }
  if (data->error)
    return;

  source_write_common_build_files (data);
  if (data->error)
    return;

#ifdef HAVE_OS2_H
  source_write_os2_files(data);
#endif
}


/*
 * GTK+ build files.
 */

static void
source_write_gtk_build_files (GbWidgetWriteSourceData * data)
{
  source_write_autogen_sh (data);
  if (data->error)
    return;

  source_write_gtk_configure_in (data);
  if (data->error)
    return;

  source_write_gtk_makefile_am (data);
}


/* Copies the generic autogen.sh script which runs aclocal, automake
   & autoconf to create the Makefiles etc. */
static void
source_write_autogen_sh (GbWidgetWriteSourceData * data)
{
  static const gchar *data_dir = GLADE_DATADIR "/glade-2";
  gchar *project_dir, *srcbuffer, *destbuffer;
  const gchar *filename = "autogen.sh";
  gint old_umask;

  srcbuffer = g_malloc (strlen (data_dir) + 128);
  project_dir = glade_project_get_directory (data->project);
  destbuffer = g_malloc (strlen (project_dir) + 128);

  sprintf (srcbuffer, "%s/gtk/%s", data_dir, filename);
  sprintf (destbuffer, "%s/%s", project_dir, filename);

  if (!glade_util_file_exists (destbuffer))
    {
      data->error = glade_util_copy_file (srcbuffer, destbuffer);
    }

  /* We need to make the script executable, but we try to honour any umask. */
#ifndef _WIN32
  old_umask = umask (0666);
  chmod (destbuffer, 0777 & ~old_umask);
  umask (old_umask);
#endif

  g_free (srcbuffer);
  g_free (destbuffer);
}


static void
source_write_gtk_configure_in (GbWidgetWriteSourceData * data)
{
  FILE *fp;
  gchar *filename, *alt_filename, *source_subdir;

  filename = glade_util_make_absolute_path (glade_project_get_directory (data->project), "configure.in");
  alt_filename = glade_util_make_absolute_path (glade_project_get_directory (data->project), "configure.ac");

  /* FIXME: If configure.in exists, just leave it, for now. */
  if (glade_util_file_exists (filename)
      || glade_util_file_exists (alt_filename))
    {
      g_free (filename);
      g_free (alt_filename);
      return;
    }

  g_free (alt_filename);

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  /* FIXME: Using AC_INIT(configure.in) is not really correct - we should be
     using a file unique to the project. */
  fprintf (fp,
	   "dnl Process this file with autoconf to produce a configure script.\n"
	   "\n"
	   "AC_INIT(configure.in)\n"
	   "AM_INIT_AUTOMAKE(%s, 0.1)\n"
	   "AM_CONFIG_HEADER(config.h)\n"
	   "AM_MAINTAINER_MODE\n"
	   "\n"
	   "AC_ISC_POSIX\n"
	   "AC_PROG_CC\n"
	   "AM_PROG_CC_STDC\n"
	   "AC_HEADER_STDC\n"
	   "\n",
	   data->program_name);

  fprintf (fp,
	   "pkg_modules=\"gtk+-2.0 >= 2.0.0\"\n"
	   "PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])\n"
	   "AC_SUBST(PACKAGE_CFLAGS)\n"
	   "AC_SUBST(PACKAGE_LIBS)\n"
	   "\n");

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp,
	       "GETTEXT_PACKAGE=%s\n"
	       "AC_SUBST(GETTEXT_PACKAGE)\n"
	       "AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,\"$GETTEXT_PACKAGE\", [Gettext package.])\n\n",
	       data->program_name);
      fprintf (fp,
	       "dnl Add the languages which your application supports here.\n"
	       "ALL_LINGUAS=\"\"\n"
	       "AM_GLIB_GNU_GETTEXT\n"
	       "\n");
    }

  fprintf (fp,
	   "AC_OUTPUT([\n"
	   "Makefile\n");

  source_subdir = source_get_source_subdirectory (data);
  if (source_subdir)
    {
      fprintf (fp, "%s/Makefile\n", source_subdir);
      g_free (source_subdir);
    }

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp,
	       "po/Makefile.in\n");
    }

  fprintf (fp, "])\n\n");

  fclose (fp);

  g_free (filename);
}


static void
source_write_gtk_makefile_am (GbWidgetWriteSourceData * data)
{
  FILE *fp;
  gchar *directory, *source_directory, *filename, *program_name_as_target;
  gboolean is_toplevel;

  directory = glade_project_get_directory (data->project);
  source_directory = glade_project_get_source_directory (data->project);

  filename = glade_util_make_absolute_path (source_directory, "Makefile.am");

  /* FIXME: If Makefile.am exists, just leave it, for now. */
  if (glade_util_file_exists (filename))
    {
      g_free (filename);
      return;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  fprintf (fp,
	   "## Process this file with automake to produce Makefile.in\n\n");

  /* If the project directory is the source directory, we need to output
     SUBDIRS here. */
  is_toplevel = glade_util_directories_equivalent (directory,
						   source_directory);
  if (is_toplevel)
    {
      fprintf (fp, "SUBDIRS =");

      if (glade_project_get_gettext_support (data->project))
	fprintf (fp, " po");

      fprintf (fp, "\n\n");
    }

  fprintf (fp,
	   "INCLUDES = \\\n"
	   "\t-DPACKAGE_DATA_DIR=\\\"\"$(datadir)\"\\\" \\\n"
	   "\t-DPACKAGE_LOCALE_DIR=\\\"\"$(prefix)/$(DATADIRNAME)/locale\"\\\" \\\n"
#ifdef GLADE_ADD_DISABLE_DEPRECATED_FLAGS
	   "\t-DG_DISABLE_DEPRECATED \\\n"
	   "\t-DGDK_DISABLE_DEPRECATED \\\n"
	   "\t-DGTK_DISABLE_DEPRECATED \\\n"
#endif
	   "\t@PACKAGE_CFLAGS@\n"
	   "\n");

  program_name_as_target = g_strdup (data->program_name);
  g_strdelimit (program_name_as_target, "-", '_');

  fprintf (fp,
	   "bin_PROGRAMS = %s\n"
	   "\n",
	   data->program_name);

  fprintf (fp,
	   "%s_SOURCES = \\\n",
	   program_name_as_target);

  if (glade_project_get_output_main_file (data->project))
    fprintf (fp, "\tmain.c \\\n");

  if (glade_project_get_output_support_files (data->project))
    {
      fprintf (fp, "\t%s %s \\\n",
	       glade_project_get_support_source_file (data->project),
	       glade_project_get_support_header_file (data->project));
    }

  fprintf (fp,
	   "\t%s %s \\\n"
	   "\t%s %s\n\n",
	   g_basename (data->interface_c_filename),
	   g_basename (data->interface_h_filename),
	   g_basename (data->callback_c_filename),
	   g_basename (data->callback_h_filename));

  fprintf (fp,
	   "%s_LDADD = @PACKAGE_LIBS@",
	   program_name_as_target);

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp, " $(INTLLIBS)");
    }

  fprintf (fp, "\n\n");

  if (is_toplevel)
    source_write_extra_dist (data, source_directory, fp);

  source_write_gtk_makefile_am_pixmaps_targets (data, source_directory, fp);

  fclose (fp);

  g_free (program_name_as_target);
  g_free (filename);
}


/* This outputs targets to install pixmaps and to include them in the
   distribution, if the pixmaps are in a subdirectory of the given directory.
*/
static void
source_write_gtk_makefile_am_pixmaps_targets (GbWidgetWriteSourceData * data,
					      gchar                   * directory,
					      FILE                    * fp)
{
  gchar *pixmaps_directory, *subdir;
  gint subdir_len;

  pixmaps_directory = glade_project_get_pixmaps_directory (data->project);

  if (!glade_util_directory_contains_file (directory, pixmaps_directory))
    return;

  subdir = glade_util_make_relative_path (directory, pixmaps_directory);
  subdir_len = strlen (subdir);
  if (subdir_len > 0 && subdir[subdir_len - 1] == G_DIR_SEPARATOR)
    subdir[subdir_len - 1] = '\0';

  /* When installing we simply copy anything in the pixmaps directory into
     $(datadir)/pixmaps, if the pixmaps directory exists.
     FIXME: The @$(NORMAL_INSTALL) line comes from the GNU Makefile
     conventions, in the GNU coding standards info pages (under Releases).
     Using DESTDIR also allows installation into 'staging areas'.
     To comply fully the install commands should install to a particular file,
     rather than a directory, and we should probably also have an uninstall
     command. Kepp this in sync with source_write_gnome_makefile_am_... */
  fprintf (fp,
	   "install-data-local:\n"
	   "\t@$(NORMAL_INSTALL)\n"
	   "\tif test -d $(srcdir)/%s; then \\\n"
	   "\t  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \\\n"
	   "\t  for pixmap in $(srcdir)/%s/*; do \\\n"
	   "\t    if test -f $$pixmap; then \\\n"
	   "\t      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \\\n"
	   "\t    fi \\\n"
	   "\t  done \\\n"
	   "\tfi\n"
	   "\n",
	   subdir, subdir);

  /* When building the distribution we simply copy anything in the pixmaps
     directory into $(distdir)/subdir, if the pixmaps directory exists. */
  fprintf (fp,
	   "dist-hook:\n"
	   "\tif test -d %s; then \\\n"
	   "\t  mkdir $(distdir)/%s; \\\n"
	   "\t  for pixmap in %s/*; do \\\n"
	   "\t    if test -f $$pixmap; then \\\n"
	   "\t      cp -p $$pixmap $(distdir)/%s; \\\n"
	   "\t    fi \\\n"
	   "\t  done \\\n"
	   "\tfi\n"
	   "\n",
	   subdir, subdir, subdir, subdir);

  g_free (subdir);
}


/*
 * Gnome build files.
 */

static void
source_write_gnome_build_files (GbWidgetWriteSourceData * data)
{
  source_write_autogen_sh (data);
  if (data->error)
    return;

  source_write_gnome_configure_in (data);
  if (data->error)
    return;

  source_write_gnome_makefile_am (data);
  if (data->error)
    return;
}


static void
source_write_gnome_configure_in (GbWidgetWriteSourceData * data)
{
  FILE *fp;
  gchar *filename, *alt_filename, *source_subdir;

  filename = glade_util_make_absolute_path (glade_project_get_directory (data->project), "configure.in");
  alt_filename = glade_util_make_absolute_path (glade_project_get_directory (data->project), "configure.ac");

  /* FIXME: If configure.in exists, just leave it, for now. */
  if (glade_util_file_exists (filename)
      || glade_util_file_exists (alt_filename))
    {
      g_free (filename);
      g_free (alt_filename);
      return;
    }

  g_free (alt_filename);

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  /* FIXME: Using AC_INIT(configure.in) is not really correct - we should be
     using a file unique to the project. */
  fprintf (fp,
	   "dnl Process this file with autoconf to produce a configure script.\n"
	   "\n"
	   "AC_INIT(configure.in)\n"
	   "AM_INIT_AUTOMAKE(%s, 0.1)\n"
	   "AM_MAINTAINER_MODE\n"
	   "AM_CONFIG_HEADER(config.h)\n"
	   "\n"
	   "AC_ISC_POSIX\n"
	   "AC_PROG_CC\n"
	   "AM_PROG_CC_STDC\n"
	   "AC_HEADER_STDC\n"
	   "\n",
	   data->program_name);

  if (glade_project_get_gnome_db_support (data->project))
    fprintf (fp, "pkg_modules=\"libgnomedb\"\n");
  else
    fprintf (fp, "pkg_modules=\"libgnomeui-2.0\"\n");

  fprintf (fp,
	   "PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])\n"
	   "AC_SUBST(PACKAGE_CFLAGS)\n"
	   "AC_SUBST(PACKAGE_LIBS)\n"
	   "\n");

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp,
	       "GETTEXT_PACKAGE=%s\n"
	       "AC_SUBST(GETTEXT_PACKAGE)\n"
	       "AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,\"$GETTEXT_PACKAGE\", [Gettext package.])\n\n",
	       data->program_name);
      fprintf (fp,
	       "dnl Add the languages which your application supports here.\n"
	       "ALL_LINGUAS=\"\"\n"
	       "AM_GLIB_GNU_GETTEXT\n"
	       "\n");
    }

  fprintf (fp,
	   "AC_OUTPUT([\n"
	   "Makefile\n");

  source_subdir = source_get_source_subdirectory (data);
  if (source_subdir)
    {
      fprintf (fp, "%s/Makefile\n", source_subdir);
      g_free (source_subdir);
    }

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp,
	       "po/Makefile.in\n");
    }

  fprintf (fp, "])\n\n");

  fclose (fp);

  g_free (filename);
}


static void
source_write_gnome_makefile_am (GbWidgetWriteSourceData * data)
{
  FILE *fp;
  gchar *directory, *source_directory, *filename, *program_name_as_target;
  gboolean is_toplevel;

  directory = glade_project_get_directory (data->project);
  source_directory = glade_project_get_source_directory (data->project);

  filename = glade_util_make_absolute_path (source_directory, "Makefile.am");

  /* FIXME: If Makefile.am exists, just leave it, for now. */
  if (glade_util_file_exists (filename))
    {
      g_free (filename);
      return;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  fprintf (fp,
	   "## Process this file with automake to produce Makefile.in\n\n");

  /* If the project directory is the source directory, we need to output
     SUBDIRS here. */
  is_toplevel = glade_util_directories_equivalent (directory,
						   source_directory);
  if (is_toplevel)
    {
      if (glade_project_get_gettext_support (data->project))
	fprintf (fp, "SUBDIRS = po\n\n");
    }

  fprintf (fp,
	   "INCLUDES = \\\n"
	   "\t-DPACKAGE_DATA_DIR=\\\"\"$(datadir)\"\\\" \\\n"
	   "\t-DPACKAGE_LOCALE_DIR=\\\"\"$(prefix)/$(DATADIRNAME)/locale\"\\\" \\\n"
#ifdef GLADE_ADD_DISABLE_DEPRECATED_FLAGS
	   "\t-DG_DISABLE_DEPRECATED \\\n"
	   "\t-DGDK_DISABLE_DEPRECATED \\\n"
	   "\t-DGTK_DISABLE_DEPRECATED \\\n"
	   "\t-DGNOME_DISABLE_DEPRECATED \\\n"
#endif
	   "\t@PACKAGE_CFLAGS@\n"
	   "\n");

  program_name_as_target = g_strdup (data->program_name);
  g_strdelimit (program_name_as_target, "-", '_');

  fprintf (fp,
	   "bin_PROGRAMS = %s\n"
	   "\n",
	   data->program_name);

  fprintf (fp,
	   "%s_SOURCES = \\\n",
	   program_name_as_target);

  if (glade_project_get_output_main_file (data->project))
    fprintf (fp, "\tmain.c \\\n");

  if (glade_project_get_output_support_files (data->project))
    {
      fprintf (fp, "\t%s %s \\\n",
	       glade_project_get_support_source_file (data->project),
	       glade_project_get_support_header_file (data->project));
    }

  fprintf (fp,
	   "\t%s %s \\\n"
	   "\t%s %s\n\n",
	   g_basename (data->interface_c_filename),
	   g_basename (data->interface_h_filename),
	   g_basename (data->callback_c_filename),
	   g_basename (data->callback_h_filename));

  fprintf (fp,
	   "%s_LDADD = @PACKAGE_LIBS@",
	   program_name_as_target);

  if (glade_project_get_gettext_support (data->project))
    {
      fprintf (fp, " $(INTLLIBS)");
    }

  fprintf (fp, "\n\n");

  if (is_toplevel)
    source_write_extra_dist (data, source_directory, fp);

  source_write_gnome_makefile_am_pixmaps_targets (data, source_directory, fp);

  fclose (fp);

  g_free (program_name_as_target);
  g_free (filename);
}


/* This outputs targets to install pixmaps and to include them in the
   distribution, if the pixmaps are in a subdirectory of the given directory.
*/
static void
source_write_gnome_makefile_am_pixmaps_targets (GbWidgetWriteSourceData * data,
						gchar                   * directory,
						FILE                    * fp)
{
  gchar *pixmaps_directory, *subdir;
  gint subdir_len;

  pixmaps_directory = glade_project_get_pixmaps_directory (data->project);

  if (!glade_util_directory_contains_file (directory, pixmaps_directory))
    return;

  subdir = glade_util_make_relative_path (directory, pixmaps_directory);
  subdir_len = strlen (subdir);
  if (subdir_len > 0 && subdir[subdir_len - 1] == G_DIR_SEPARATOR)
    subdir[subdir_len - 1] = '\0';

  /* When installing we simply copy anything in the pixmaps directory into
     $(datadir)/pixmaps, if the pixmaps directory exists. */
  fprintf (fp,
	   "install-data-local:\n"
	   "\t@$(NORMAL_INSTALL)\n"
	   "\tif test -d $(srcdir)/%s; then \\\n"
	   "\t  $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \\\n"
	   "\t  for pixmap in $(srcdir)/%s/*; do \\\n"
	   "\t    if test -f $$pixmap; then \\\n"
	   "\t      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \\\n"
	   "\t    fi \\\n"
	   "\t  done \\\n"
	   "\tfi\n"
	   "\n",
	   subdir, subdir);

  /* When building the distribution we simply copy anything in the pixmaps
     directory into $(distdir)/subdir, if the pixmaps directory exists. */
  fprintf (fp,
	   "dist-hook:\n"
	   "\tif test -d %s; then \\\n"
	   "\t  mkdir $(distdir)/%s; \\\n"
	   "\t  for pixmap in %s/*; do \\\n"
	   "\t    if test -f $$pixmap; then \\\n"
	   "\t      cp -p $$pixmap $(distdir)/%s; \\\n"
	   "\t    fi \\\n"
	   "\t  done \\\n"
	   "\tfi\n"
	   "\n",
	   subdir, subdir, subdir, subdir);

  g_free (subdir);
}


/*
 * Common build files.
 */

/* This creates the standard files that automake expects you to have,
   i.e. NEWS, README, AUTHORS, ChangeLog.
   If they already exist, they are left as they are. */
static void
source_write_common_build_files (GbWidgetWriteSourceData * data)
{
  gchar *directory;

  directory = glade_project_get_directory (data->project);

  source_write_toplevel_makefile_am (data);
  if (data->error)
    return;

  data->error = source_create_file_if_not_exist (directory, "NEWS", NULL);
  if (data->error)
    return;

  data->error = source_create_file_if_not_exist (directory, "README", NULL);
  if (data->error)
    return;

  data->error = source_create_file_if_not_exist (directory, "AUTHORS", NULL);
  if (data->error)
    return;

  data->error = source_create_file_if_not_exist (directory, "ChangeLog", NULL);
  if (data->error)
    return;

  data->error = source_create_file_if_not_exist (directory, "stamp-h.in",
						 "timestamp\n");
  if (data->error)
    return;

  source_write_po_files (data);
  if (data->error)
    return;

#if 0
  source_write_acconfig_h (data);
#endif
}


/* This writes the toplevel Makefile.am, if the source directory is a
   subdirectory of the project directory, and the Makefile.am doesn't already
   exist. */
static void
source_write_toplevel_makefile_am (GbWidgetWriteSourceData * data)
{
  FILE *fp;
  gchar *directory, *source_subdir, *filename;

  directory = glade_project_get_directory (data->project);

  /* If the source directory isn't a subdirectory of the project directory,
     just return. */
  source_subdir = source_get_source_subdirectory (data);
  if (!source_subdir)
    return;

  filename = glade_util_make_absolute_path (directory, "Makefile.am");

  /* FIXME: If Makefile.am exists, just leave it, for now. */
  if (glade_util_file_exists (filename))
    {
      g_free (source_subdir);
      g_free (filename);
      return;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (source_subdir);
      g_free (filename);
      return;
    }

  fprintf (fp,
	   "## Process this file with automake to produce Makefile.in\n"
	   "\n"
	   "SUBDIRS = %s",
	   source_subdir);

  if (glade_project_get_gettext_support (data->project))
    fprintf (fp, " po");

  fprintf (fp, "\n\n");

  source_write_extra_dist (data, directory, fp);

  if (glade_project_get_gnome_support (data->project))
    source_write_gnome_makefile_am_pixmaps_targets (data, directory, fp);
  else
    source_write_gtk_makefile_am_pixmaps_targets (data, directory, fp);

  fclose (fp);

  g_free (source_subdir);
  g_free (filename);
}


static void
source_write_extra_dist (GbWidgetWriteSourceData * data,
			 const gchar *directory,
			 FILE *fp)
{
  gchar *xml_filename;

  xml_filename = glade_project_get_xml_filename (data->project);

  xml_filename = glade_util_make_relative_path (directory, xml_filename);

  fprintf (fp,
	   "EXTRA_DIST = \\\n"
	   "\tautogen.sh \\\n"
	   "\t%s \\\n"
	   "\t%sp\n\n",
	   xml_filename, xml_filename);

  g_free (xml_filename);
}


/* This creates an empty file in the given directory if it doesn't already
   exist. It is used to create the empty NEWS, README, AUTHORS & ChangeLog. */
static GladeError*
source_create_file_if_not_exist (const gchar *directory,
				 const gchar *filename,
				 const gchar *contents)
{
  gchar *pathname;
  FILE *fp;
  GladeError *error = NULL;
  gint bytes_written;

  pathname = glade_util_make_absolute_path (directory, filename);

  if (!glade_util_file_exists (pathname))
    {
      fp = glade_util_fopen (pathname, "w");
      if (fp)
	{
	  if (contents)
	    {
	      gint contents_len;

	      contents_len = strlen (contents);
	      bytes_written = fwrite (contents, 1, contents_len, fp);
	      if (bytes_written != contents_len)
		{
		  error = glade_error_new_system (_("Error writing to file:\n  %s\n"), pathname);
		}
	    }

	  fclose (fp);
	}
      else
	{
	  error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					  pathname);
	}
    }
  g_free (pathname);
  return error;
}


/* This creates the initial po/POTFILES.in & po/ChangeLog if they don't exist.
   If we don't create a ChangeLog the 'make dist' fails. */
static void
source_write_po_files (GbWidgetWriteSourceData * data)
{
  gchar *dirname = NULL, *filename = NULL, *prefix, *separator;
  gint prefix_len;
  FILE *fp;

  /* Returns if gettext support isn't wanted. */
  if (!glade_project_get_gettext_support (data->project))
    return;

  dirname = glade_util_make_absolute_path (glade_project_get_directory (data->project), "po");

  /* Create the po directory if it doesn't exist. */
  data->error = glade_util_ensure_directory_exists (dirname);
  if (data->error)
    {
      g_free (dirname);
      return;
    }

  /* Create ChangeLog if it doesn't exist. */
  data->error = source_create_file_if_not_exist (dirname, "ChangeLog", NULL);
  if (data->error)
    {
      g_free (dirname);
      return;
    }

  /* FIXME: If POTFILES.in exists, just leave it, for now. */
  filename = glade_util_make_absolute_path (dirname, "POTFILES.in");
  if (glade_util_file_exists (filename))
    {
      g_free (dirname);
      g_free (filename);
      return;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (dirname);
      g_free (filename);
      return;
    }

  /* We need the relative path from the project directory to the source
     directory, to prefix each source file. */
  prefix = glade_util_make_relative_path (glade_project_get_directory (data->project), glade_project_get_source_directory (data->project));

  /* See if we need a directory separator after the prefix. */
  prefix_len = strlen (prefix);
  if (prefix_len == 0 || prefix[prefix_len - 1] == G_DIR_SEPARATOR)
    separator = "";
  else
    separator = G_DIR_SEPARATOR_S;

  fprintf (fp, "# List of source files containing translatable strings.\n\n");

  /* Add the main.c file if we are outputting it. */
  if (glade_project_get_output_main_file (data->project))
    fprintf (fp, "%s%s%s\n", prefix, separator, "main.c");

  /* Add the interface.c & callbacks.c files. */
  fprintf (fp, 
	   "%s%s%s\n"
	   "%s%s%s\n",
	   prefix, separator, g_basename (data->interface_c_filename),
	   prefix, separator, g_basename (data->callback_c_filename));

  /* Add the support.c file if we are outputting it. */
  if (glade_project_get_output_support_files (data->project))
    fprintf (fp, "%s%s%s\n", prefix, separator,
	     glade_project_get_support_source_file (data->project));

  g_free (prefix);

  fclose (fp);

  g_free (dirname);
  g_free (filename);
}


#if 0
static void
source_write_acconfig_h (GbWidgetWriteSourceData * data)
{
  FILE *fp;
  gchar *filename;

  filename = glade_util_make_absolute_path (glade_project_get_directory (data->project), "acconfig.h");

  /* FIXME: If acconfig.h exists, just leave it, for now. */
  if (glade_util_file_exists (filename))
    {
      g_free (filename);
      return;
    }

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  /* If we aren't using gettext or Gnome some of these may not be necessary,
     but I don't think they cause problems. */
  fprintf (fp,
	   "#undef ENABLE_NLS\n"
	   "#undef HAVE_CATGETS\n"
	   "#undef HAVE_GETTEXT\n"
	   "#undef GETTEXT_PACKAGE\n"
	   "#undef HAVE_LC_MESSAGES\n"
	   "#undef HAVE_STPCPY\n"
	   "#undef HAVE_LIBSM\n");
  fclose (fp);

  g_free (filename);
}
#endif


/*************************************************************************
 * Support Files.
 *************************************************************************/

/* We copy a file containing support functions into the project, and a
   corresponding header.
   Note that we do overwrite these, in case an old version is currently
   being used. */
static void
source_write_support_files (GbWidgetWriteSourceData * data)
{
  gchar *filename;
  FILE *fp;

  /* If the support files aren't wanted, just return. */
  if (!glade_project_get_output_support_files (data->project))
    return;

  /* Create the support header file first. */
  filename = glade_util_make_absolute_path (glade_project_get_source_directory (data->project), glade_project_get_support_header_file (data->project));

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  source_write_no_editing_warning (fp);
  source_write_preamble (data->project_name, fp);

  if (glade_project_get_gnome_support (data->project))
    {
      fprintf (fp,
	       "#ifdef HAVE_CONFIG_H\n"
	       "#  include <config.h>\n"
	       "#endif\n"
	       "\n"
	       "#include <gnome.h>\n\n");

      if (glade_project_get_gettext_support (data->project))
	{
	  /* bonobo-i18n.h doesn't include the Q_ macro so we add it here. */
	  fprintf (fp,
		   "#undef Q_\n"
		   "#ifdef ENABLE_NLS\n"
		   "#  define Q_(String) g_strip_context ((String), gettext (String))\n"
		   "#else\n"
		   "#  define Q_(String) g_strip_context ((String), (String))\n"
		   "#endif\n\n\n");
	}
    }
  else
    {
      fprintf (fp,
	       "#ifdef HAVE_CONFIG_H\n"
	       "#  include <config.h>\n"
	       "#endif\n"
	       "\n"
	       "#include <gtk/gtk.h>\n\n");

      /* For GTK+ apps that want gettext support, we define the standard
	 macros here. */
      if (glade_project_get_gettext_support (data->project))
	{
	  fprintf (fp,
		   "/*\n"
		   " * Standard gettext macros.\n"
		   " */\n"
		   "#ifdef ENABLE_NLS\n"
		   "#  include <libintl.h>\n"
		   "#  undef _\n"
		   "#  define _(String) dgettext (PACKAGE, String)\n"
		   "#  define Q_(String) g_strip_context ((String), gettext (String))\n"
		   "#  ifdef gettext_noop\n"
		   "#    define N_(String) gettext_noop (String)\n"
		   "#  else\n"
		   "#    define N_(String) (String)\n"
		   "#  endif\n"
		   "#else\n"
		   "#  define textdomain(String) (String)\n"
		   "#  define gettext(String) (String)\n"
		   "#  define dgettext(Domain,Message) (Message)\n"
		   "#  define dcgettext(Domain,Message,Type) (Message)\n"
		   "#  define bindtextdomain(Domain,Directory) (Domain)\n"
		   "#  define _(String) (String)\n"
		   "#  define Q_(String) g_strip_context ((String), (String))\n"
		   "#  define N_(String) (String)\n"
		   "#endif\n\n\n");
	}
    }

  fprintf (fp,
	   "/*\n"
	   " * Public Functions.\n"
	   " */\n"
	   "\n");

  if (!data->use_component_struct)
    {
      fprintf (fp,
	       "/*\n"
	       " * This function returns a widget in a component created by Glade.\n"
	       " * Call it with the toplevel widget in the component (i.e. a window/dialog),\n"
	       " * or alternatively any widget in the component, and the name of the widget\n"
	       " * you want returned.\n"
	       " */\n"
	       "GtkWidget*  lookup_widget              (GtkWidget       *widget,\n"
	       "                                        const gchar     *widget_name);\n"
	       "\n"
	       "\n");
    }

  /* Gnome has its own function for looking for pixmaps, so we don't need this
     one. */
  if (!glade_project_get_gnome_support (data->project))
    {
      fprintf (fp,
	       "/* Use this function to set the directory containing installed pixmaps. */\n"
	       "void        add_pixmap_directory       (const gchar     *directory);\n"
	       "\n");
    }

  fprintf (fp,
	   "\n"
	   "/*\n"
	   " * Private Functions.\n"
	   " */\n"
	   "\n");

  fprintf (fp,
	   "/* This is used to create the pixmaps used in the interface. */\n"
	   "GtkWidget*  create_pixmap              (GtkWidget       *widget,\n"
	   "                                        const gchar     *filename);\n"
	   "\n");

  fprintf (fp,
	   "/* This is used to create the pixbufs used in the interface. */\n"
	   "GdkPixbuf*  create_pixbuf              (const gchar     *filename);\n"
	   "\n");

  fprintf (fp,
	   "/* This is used to set ATK action descriptions. */\n"
	   "void        glade_set_atk_action_description (AtkAction       *action,\n"
	   "                                              const gchar     *action_name,\n"
	   "                                              const gchar     *description);\n"
	   "\n");

  fclose (fp);

  g_free (filename);


  /* Now create the support source file. */
  filename = glade_util_make_absolute_path (glade_project_get_source_directory (data->project), glade_project_get_support_source_file (data->project));

  fp = glade_util_fopen (filename, "w");
  if (fp == NULL)
    {
      data->error = glade_error_new_system (_("Couldn't create file:\n  %s\n"),
					    filename);
      g_free (filename);
      return;
    }

  source_write_no_editing_warning (fp);
  source_write_preamble (data->project_name, fp);
  source_write_include_files (fp);
 
  if (glade_project_get_gnome_support (data->project))
    fprintf (fp, "#include <gnome.h>\n\n");
  else
    fprintf (fp, "#include <gtk/gtk.h>\n\n");

  fprintf (fp, "#include \"%s\"\n\n",
	   glade_project_get_support_header_file (data->project));

  /* Write a function to get a widget from the component's hash. */
  if (!data->use_component_struct)
    {
      fprintf (fp,
	       "GtkWidget*\n"
	       "lookup_widget                          (GtkWidget       *widget,\n"
	       "                                        const gchar     *widget_name)\n"
	       "{\n"
	       "  GtkWidget *parent, *found_widget;\n"
	       "\n"
	       "  for (;;)\n"
	       "    {\n"
	       "      if (GTK_IS_MENU (widget))\n"
	       "        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));\n"
	       "      else\n"
	       "        parent = gtk_widget_get_parent (widget);\n"
	       "      if (!parent)\n"
	       "        parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), \"GladeParentKey\");\n"
	       "      if (parent == NULL)\n"
	       "        break;\n"
	       "      widget = parent;\n"
	       "    }\n"
	       "\n"
	       "  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),\n"
	       "                                                 widget_name);\n"
	       "  if (!found_widget)\n"
	       "    g_warning (\"Widget not found: %%s\", widget_name);\n"
	       "  return found_widget;\n"
	       "}\n\n");
    }

  if (glade_project_get_gnome_support (data->project))
    {
      source_write_gnome_create_pixmap_functions (data, fp);
    }
  else
    {
      source_write_gtk_create_pixmap_functions (data, fp);
    }

  /* Output the support function to set AtkAction descriptions. */
  fprintf (fp,
	   "/* This is used to set ATK action descriptions. */\n"
	   "void\n"
	   "glade_set_atk_action_description       (AtkAction       *action,\n"
	   "                                        const gchar     *action_name,\n"
	   "                                        const gchar     *description)\n"
	   "{\n"
	   "  gint n_actions, i;\n"
	   "\n"
	   "  n_actions = atk_action_get_n_actions (action);\n"
	   "  for (i = 0; i < n_actions; i++)\n"
	   "    {\n"
	   "      if (!strcmp (atk_action_get_name (action, i), action_name))\n"
	   "        atk_action_set_description (action, i, description);\n"
	   "    }\n"
	   "}\n\n");

  fclose (fp);

  g_free (filename);
}


static void
source_write_gtk_create_pixmap_functions (GbWidgetWriteSourceData * data,
					  FILE *fp)
{
  /*
   * Write a function used to set the installed pixmaps directory.
   */
  fprintf (fp,
	   "static GList *pixmaps_directories = NULL;\n"
	   "\n"
	   "/* Use this function to set the directory containing installed pixmaps. */\n"
	   "void\n"
	   "add_pixmap_directory                   (const gchar     *directory)\n"
	   "{\n"
	   "  pixmaps_directories = g_list_prepend (pixmaps_directories,\n"
	   "                                        g_strdup (directory));\n"
	   "}\n\n");

  /*
   * Write a function used to find pixmap files. It returns the full pathname
   * of the found pixmap file, or NULL if it wasn't found.
   */
  fprintf (fp,
	   "/* This is an internally used function to find pixmap files. */\n"
	   "static gchar*\n"
	   "find_pixmap_file                       (const gchar     *filename)\n"
	   "{\n"
	   "  GList *elem;\n"
	   "\n");

  fprintf (fp,
	   "  /* We step through each of the pixmaps directory to find it. */\n"
	   "  elem = pixmaps_directories;\n"
	   "  while (elem)\n"
	   "    {\n"
	   "      gchar *pathname = g_strdup_printf (\"%%s%%s%%s\", (gchar*)elem->data,\n"
	   "                                         G_DIR_SEPARATOR_S, filename);\n"
	   "      if (g_file_test (pathname, G_FILE_TEST_EXISTS))\n"
	   "        return pathname;\n"
	   "      g_free (pathname);\n"
	   "      elem = elem->next;\n"
	   "    }\n"
	   "  return NULL;\n"
	   "}\n\n");



  /*
   * Write a function used for creating GtkImage widgets from pixmap files.
   * It will return an unset GtkImage if the pixmap file isn't found, so the
   * application will still run. Though it will output a warning if the
   * pixmap filename was set but wasn't found.
   */
  fprintf (fp,
	   "/* This is an internally used function to create pixmaps. */\n"
	   "GtkWidget*\n"
	   "create_pixmap                          (GtkWidget       *widget,\n"
	   "                                        const gchar     *filename)\n"
	   "{\n"
	   "  gchar *pathname = NULL;\n"
	   "  GtkWidget *pixmap;\n"
	   "\n");

  /* The developer may not have finished the interface yet, so we handle
     the filename not being set. */
  fprintf (fp,
	   "  if (!filename || !filename[0])\n"
	   "      return gtk_image_new ();\n"
	   "\n");

  fprintf (fp,
	   "  pathname = find_pixmap_file (filename);\n"
	   "\n");

  fprintf (fp,
	   "  if (!pathname)\n"
	   "    {\n"
	   "      g_warning (%s, filename);\n"
	   "      return gtk_image_new ();\n"
	   "    }\n"
	   "\n",
	   source_make_string ("Couldn't find pixmap file: %s",
			       data->use_gettext));

  fprintf (fp,
	   "  pixmap = gtk_image_new_from_file (pathname);\n"
	   "  g_free (pathname);\n"
	   "  return pixmap;\n"
	   "}\n\n");



  /*
   * Write a function used for creating GdkPixbufs from pixmap files.
   * It will return NULL if the pixmap file isn't found, so any calls to this
   * function may need to handle that. It will output a warning if the
   * pixmap filename was set but wasn't found.
   */
  fprintf (fp,
	   "/* This is an internally used function to create pixmaps. */\n"
	   "GdkPixbuf*\n"
	   "create_pixbuf                          (const gchar     *filename)\n"
	   "{\n"
	   "  gchar *pathname = NULL;\n"
	   "  GdkPixbuf *pixbuf;\n"
	   "  GError *error = NULL;\n"
	   "\n");

  /* The developer may not have finished the interface yet, so we handle
     the filename not being set. */
  fprintf (fp,
	   "  if (!filename || !filename[0])\n"
	   "      return NULL;\n"
	   "\n");

  fprintf (fp,
	   "  pathname = find_pixmap_file (filename);\n"
	   "\n");

  fprintf (fp,
	   "  if (!pathname)\n"
	   "    {\n"
	   "      g_warning (%s, filename);\n"
	   "      return NULL;\n"
	   "    }\n"
	   "\n",
	   source_make_string ("Couldn't find pixmap file: %s",
			       data->use_gettext));

  fprintf (fp,
	   "  pixbuf = gdk_pixbuf_new_from_file (pathname, &error);\n"
	   "  if (!pixbuf)\n"
	   "    {\n"
	   "      fprintf (stderr, \"Failed to load pixbuf file: %%s: %%s\\n\",\n"
	   "               pathname, error->message);\n"
	   "      g_error_free (error);\n"
	   "    }\n"
	   "  g_free (pathname);\n"
	   "  return pixbuf;\n"
	   "}\n\n");
}


static void
source_write_gnome_create_pixmap_functions (GbWidgetWriteSourceData * data,
					    FILE *fp)
{
  /*
   * Write a function used for creating GtkImage widgets from pixmap files.
   * It will return an unset GtkImage if the pixmap file isn't found, so the
   * application will still run. Though it will output a warning if the
   * pixmap filename was set but wasn't found.
   */
  fprintf (fp,
	   "/* This is an internally used function to create pixmaps. */\n"
	   "GtkWidget*\n"
	   "create_pixmap                          (GtkWidget       *widget,\n"
	   "                                        const gchar     *filename)\n"
	   "{\n"
	   "  GtkWidget *pixmap;\n"
	   "  gchar *pathname;\n"
	   "\n");

  /* The developer may not have finished the interface yet, so we handle
     the filename not being set. */
  fprintf (fp,
	   "  if (!filename || !filename[0])\n"
	   "      return gtk_image_new ();\n"
	   "\n");

  fprintf (fp,
	   "  pathname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,\n"
	   "                                        filename, TRUE, NULL);\n"
	   "  if (!pathname)\n"
	   "    {\n"
	   "      g_warning (%s, filename);\n"
	   "      return gtk_image_new ();\n"
	   "    }\n"
	   "\n",
	   source_make_string ("Couldn't find pixmap file: %s",
			       data->use_gettext));

  fprintf (fp,
	   "  pixmap = gtk_image_new_from_file (pathname);\n"
	   "  g_free (pathname);\n"
	   "  return pixmap;\n"
	   "}\n"
	   "\n");


  /*
   * Write a function used for creating GdkPixbufs from pixmap files.
   * It will return NULL if the pixmap file isn't found, so any calls to this
   * function may need to handle that. It will output a warning if the
   * pixmap filename was set but wasn't found.
   */
  fprintf (fp,
	   "/* This is an internally used function to create pixmaps. */\n"
	   "GdkPixbuf*\n"
	   "create_pixbuf                          (const gchar     *filename)\n"
	   "{\n"
	   "  gchar *pathname = NULL;\n"
	   "  GdkPixbuf *pixbuf;\n"
	   "  GError *error = NULL;\n"
	   "\n");

  /* The developer may not have finished the interface yet, so we handle
     the filename not being set. */
  fprintf (fp,
	   "  if (!filename || !filename[0])\n"
	   "      return NULL;\n"
	   "\n");

  fprintf (fp,
	   "  pathname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,\n"
	   "                                        filename, TRUE, NULL);\n"
	   "\n");

  fprintf (fp,
	   "  if (!pathname)\n"
	   "    {\n"
	   "      g_warning (%s, filename);\n"
	   "      return NULL;\n"
	   "    }\n"
	   "\n",
	   source_make_string ("Couldn't find pixmap file: %s",
			       data->use_gettext));

  fprintf (fp,
	   "  pixbuf = gdk_pixbuf_new_from_file (pathname, &error);\n"
	   "  if (!pixbuf)\n"
	   "    {\n"
	   "      fprintf (stderr, \"Failed to load pixbuf file: %%s: %%s\\n\",\n"
	   "               pathname, error->message);\n"
	   "      g_error_free (error);\n"
	   "    }\n"
	   "  g_free (pathname);\n"
	   "  return pixbuf;\n"
	   "}\n\n");
}


/*************************************************************************
 * Public Functions.
 *************************************************************************/

/* Adds some source code to one of the buffers, using printf-like format
   and arguments. */
void
source_add_to_buffer (GbWidgetWriteSourceData * data,
		      GladeSourceBuffer buffer,
		      const gchar *fmt,
		      ...)
{
  va_list args;

  va_start (args, fmt);
  source_add_to_buffer_v (data, buffer, fmt, args);
  va_end (args);
}


/* A va_list implementation of the above. */
void
source_add_to_buffer_v (GbWidgetWriteSourceData * data,
			GladeSourceBuffer buffer,
			const gchar *fmt,
			va_list args)
{
  gchar *buf;

  buf = g_strdup_vprintf (fmt, args);
  g_string_append (data->source_buffers[buffer], buf);
  g_free (buf);
}


/* Convenience functions to add to the 2 main source buffers, containing
   the code which creates the widgets and the declarations of the widgets. */
void
source_add (GbWidgetWriteSourceData * data, const gchar * fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  source_add_to_buffer_v (data, GLADE_SOURCE, fmt, args);
  va_end (args);
}


void
source_add_translator_comments (GbWidgetWriteSourceData *data,
				gboolean translatable,
				const gchar *comments)
{
  /* If the property isn't being translated we don't bother outputting the
     translator comments. */
  if (!translatable || !comments || comments[0] == '\0')
    return;

  /* We simply output it in a C comment.
     FIXME: If the comments contain an end of comment marker it won't
     compile. */
  source_add (data, "  /* %s */\n", comments);
}


void
source_add_translator_comments_to_buffer (GbWidgetWriteSourceData *data,
					  GladeSourceBuffer buffer,
					  gboolean translatable,
					  const gchar *comments)
{
  /* If the property isn't being translated we don't bother outputting the
     translator comments. */
  if (!translatable || !comments || comments[0] == '\0')
    return;

  /* We simply output it in a C comment.
     FIXME: If the comments contain an end of comment marker it won't
     compile. */
  source_add_to_buffer (data, buffer, "  /* %s */\n", comments);
}


void
source_add_decl (GbWidgetWriteSourceData * data, const gchar * fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  source_add_to_buffer_v (data, GLADE_DECLARATIONS, fmt, args);
  va_end (args);
}


/* This ensures that a temporary variable is declared, by adding the given
   declaration if it has not already been added. */
void
source_ensure_decl	(GbWidgetWriteSourceData *data,
			 const gchar		 *decl)
{
  if (!glade_util_strstr (data->source_buffers[GLADE_DECLARATIONS]->str, decl))
    source_add_decl (data, decl);
}


/* This creates a valid C identifier from a given string (usually the name of
   a widget or a signal handler function name). Currently all we do is convert
   any illegal characters to underscores.
   The returned string should be freed when no longer needed. */
gchar *
source_create_valid_identifier (const gchar * name)
{
  gint name_len, i, j;
  gchar *identifier;

  name_len = strlen (name);
  /* allocate extra space for _ prefix if the identifier starts with
     a number */
  identifier = g_malloc (name_len + 2);
  j = 1;

  /* The first character of an identifier must be in [a-zA-Z_] */
  if ((name[0] >= 'a' && name[0] <= 'z')
      || (name[0] >= 'A' && name[0] <= 'Z')
      || name[0] == '_')
    identifier[0] = name[0];
  else
    {
      if (name[0] >= '0' && name[0] <= '9')
	{
	  /* prepend the _ instead of overwriting, so you'll still have
	     unique names */
	  identifier[0] = '_';
	  identifier[1] = name[0];
	  j++;
	}
      else
	identifier[0] = '_';
    }

  /* The remaining characters must be in [a-zA-Z0-9_] */
  for (i = 1; i < name_len; i++, j++)
    {
      if ((name[i] >= 'a' && name[i] <= 'z')
	  || (name[i] >= 'A' && name[i] <= 'Z')
	  || (name[i] >= '0' && name[i] <= '9')
	  || name[i] == '_')
	identifier[j] = name[i];
      else
	identifier[j] = '_';
    }

  identifier[j] = '\0';

  return identifier;
}


/* This converts a string so that it can be output as part of the C source
 * code. It converts non-printable characters to escape codes.
 * Note that it uses one dynamically allocated buffer, so the result is only
 * valid until the next call to the function.
 *
 * FIXME: There is a limit to the length of literal strings in ANSI C - ~500
 * chars. What should we do when that is exceeded?
 */
gchar *
source_make_string (const gchar * text,
		    gboolean translatable)
{
  return source_make_string_internal (text, translatable, FALSE, FALSE);
}


gchar *
source_make_string_full (const gchar * text,
			 gboolean translatable,
			 gboolean context)
{
  return source_make_string_internal (text, translatable, FALSE, context);
}


gchar *
source_make_static_string (const gchar * text,
			   gboolean translatable)
{
  return source_make_string_internal (text, translatable, TRUE, FALSE);
}


/* This converts a string so that it can be output as part of the C source
 * code. It converts non-printable characters to escape codes.
 * If is_static is TRUE it uses "N_" to mark translatable strings.
 * Note that it uses one dynamically allocated buffer, so the result is only
 * valid until the next call to the function.
 */
static gchar *
source_make_string_internal (const gchar * text,
			     gboolean translatable,
			     gboolean is_static,
			     gboolean context)
{
  static GString *buffer = NULL;

  gchar escape_buffer[16];
  const gchar *p;

  /* If the text is empty, we return an empty string without the _ macro. */
  if (!text || text[0] == '\0')
    return "\"\"";

  /* Create the buffer if it hasn't already been created. */
  if (buffer == NULL)
    {
      buffer = g_string_sized_new (1024);
    }

  /* Clear any previous string. */
  g_string_truncate (buffer, 0);

  /* Output the C code to start the string. */
  if (translatable)
    {
      /* Static "N_" overrides any context setting, though they shouldn't
	 really both be passed in as TRUE anyway. */
      if (is_static)
	g_string_append (buffer, "N_(\"");
      else if (context)
	g_string_append (buffer, "Q_(\"");
      else
	g_string_append (buffer, "_(\"");
    }
  else
    {
      g_string_append (buffer, "\"");
    }

  /* Step through each character of the given string, adding it to our GString
     buffer, converting it so that it is valid in a literal C string. */
  for (p = text; *p; p++)
    {
      switch (*p)
	{
	case '\n':
	  g_string_append (buffer, "\\n");
	  break;
	case '\r':
	  g_string_append (buffer, "\\r");
	  break;
	case '\t':
	  g_string_append (buffer, "\\t");
	  break;
	case '\\':
	  g_string_append (buffer, "\\\\");
	  break;
	case '"':
	  g_string_append (buffer, "\\\"");
	  break;
	default:
	  if (isprint (*p))
	    {
	      g_string_append_c (buffer, *p);
	    }
	  else
	    {
	      sprintf (escape_buffer, "\\%02o", (guchar) *p);
	      g_string_append (buffer, escape_buffer);
	    }
	  break;
	}
    }

  /* Output the C code to end the string. */
  g_string_append (buffer, translatable ? "\")" : "\"");

  return buffer->str;
}


/* This outputs code to create a GtkImage widget with the given identifier,
   and using the given filename (only the basename is used). If filename NULL
   or "" an empty GtkImage is created. */
void
source_create_pixmap (GbWidgetWriteSourceData * data,
		      const gchar             * identifier,
		      const gchar             * filename)
{
  gboolean empty_filename = FALSE;

  if (!filename || filename[0] == '\0')
    empty_filename = TRUE;

  /* We use the basename of the pixmap file. The create_pixmap() support
     function is responsible for finding the pixmap.
     If it can't find the pixmap, it outputs a warning messages and returns
     a simple dummy pixmap, so that the app can continue without crashing. */
  if (glade_project_get_gnome_support (data->project))
    {
      /* FIXME: Should convert filename to a valid C string? */
      if (!empty_filename)
	source_add (data, "  %s = create_pixmap (%s, \"%s/%s\");\n",
		    identifier, data->component_name,
		    data->program_name, g_basename (filename));
      else
	source_add (data, "  %s = create_pixmap (%s, NULL);\n",
		    identifier, data->component_name);
    }
  else
    {
      if (!empty_filename)
	source_add (data, "  %s = create_pixmap (%s, \"%s\");\n",
		    identifier, data->component_name, g_basename (filename));
      else
	source_add (data, "  %s = create_pixmap (%s, NULL);\n",
		    identifier, data->component_name);
    }
}


/* This outputs code to create a GdkPixbuf with the given identifier,
   and using the given filename (only the basename is used). filename must not
   be NULL or "". */
void
source_create_pixbuf (GbWidgetWriteSourceData * data,
		      const gchar             * identifier,
		      const gchar             * filename)
{
  g_return_if_fail (filename && filename[0]);

  /* We use the basename of the pixmap file. The create_pixmap() support
     function is responsible for finding the pixmap.
     If it can't find the pixmap, it outputs a warning messages and returns
     a simple dummy pixmap, so that the app can continue without crashing. */
  if (glade_project_get_gnome_support (data->project))
    {
      /* FIXME: Should convert filename to a valid C string? */
      source_add (data, "  %s = create_pixbuf (\"%s/%s\");\n",
		  identifier, data->program_name, g_basename (filename));
    }
  else
    {
      source_add (data, "  %s = create_pixbuf (\"%s\");\n",
		  identifier, g_basename (filename));
    }
}


/*************************************************************************
 * Utility Functions.
 *************************************************************************/

/* This empties all the source code buffers, but doesn't free them. */
static void
source_reset_code_buffers (GbWidgetWriteSourceData * data)
{
  gint i;

  for (i = 0; i < GLADE_NUM_SOURCE_BUFFERS; i++)
    g_string_truncate (data->source_buffers[i], 0);
}


/* A callback used to free the 'standard widgets' (widgets we use to determine
   default property values) when iterating over their GHashTable. */
static void
source_destroy_standard_widgets_callback (gchar * key, GtkWidget * widget,
					  gpointer data)
{
  gtk_widget_destroy (widget);
}


/* A callback used to free GHashTable keys when iterating over them */
static void
source_free_hash_keys_callback (gchar * key, gpointer value, gpointer data)
{
  g_free (key);
}


/* Checks if the given file is a valid source filename, i.e. not NULL and
   not absolute. If not it returns an appropriate error message. */
static gchar*
source_is_valid_source_filename (const gchar * filename)
{
  if (filename == NULL || filename[0] == '\0')
    return _("The filename must be set in the Project Options dialog.");

  if (g_path_is_absolute (filename))
    return _("The filename must be a simple relative filename.\n"
	     "Use the Project Options dialog to set it.");

  return NULL;
}


/* Checks if the given file exists, and if so renames it to file.bak. */
static GladeError*
source_backup_file_if_exists (const gchar * filename)
{
  GladeError *error = NULL;
  gchar *backup_filename;
  int status;

  if (glade_util_file_exists (filename))
    {
      backup_filename = g_strdup_printf ("%s.bak", filename);
#if defined (__EMX__) || defined (_WIN32)
      remove (backup_filename);
#endif
      status = rename (filename, backup_filename);

      if (status == -1)
	{
	  error = glade_error_new_system (_("Couldn't rename file:\n  %s\nto:\n  %s\n"), filename, backup_filename);
	}
      g_free (backup_filename);
    }
  return error;
}


/* This returns the relative path from the project directory to the source
   directory, or NULL if the source directory is the project directory.
   The returned string will not have any trailing '/', and should be freed
   when no longer needed. */
static gchar*
source_get_source_subdirectory (GbWidgetWriteSourceData * data)
{
  gchar *directory, *source_directory, *subdir;
  gint subdir_len;

  directory = glade_project_get_directory (data->project);
  source_directory = glade_project_get_source_directory (data->project);

  /* Check if the source directory is a subdirectory of the project directory.
   */
  if (!glade_util_directory_contains_file (directory, source_directory))
    return NULL;

  subdir = glade_util_make_relative_path (directory, source_directory);
  subdir_len = strlen (subdir);
  if (subdir_len > 0 && subdir[subdir_len - 1] == G_DIR_SEPARATOR)
    subdir[subdir_len - 1] = '\0';

  return subdir;
}


/* Outputs a warning to the given file, telling the user not to edit the file
   since it is generated by Glade. */
static void
source_write_no_editing_warning (FILE *fp)
{
  /*
   * Output a 3-line comment like this one, makes it easy for translators.
   * Actually I've turned translation off now, as UTF-8 may cause problems
   * in the C code. See bug #95435.
   */
  fprintf (fp, "/*\n * %s\n */\n\n",
	   "DO NOT EDIT THIS FILE - it is generated by Glade.");
}
