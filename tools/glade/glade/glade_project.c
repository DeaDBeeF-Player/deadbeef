/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
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
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>

#include <dirent.h>

#include <libxml/parser.h>

#include <gtk/gtkmenu.h>
#include <gtk/gtk.h>
#include "gladeconfig.h"

#include "editor.h"
#include "glade_menu_editor.h"
#include "glade_project.h"
#include "load.h"
#include "property.h"
#include "save.h"
#include "source.h"
#include "tree.h"
#include "utils.h"

/* FIXME: This is the current project. We only support one open at present. */
GladeProject *current_project = NULL;


/* The order must match the GladeLanguageType enum in glade_project.h. */
gchar *GladeLanguages[] = { "C", "C++" , "Ada 95", "Perl", "Eiffel" };
#if 1
gint GladeNumLanguages = 3; /* Only C, C++ and Ada ported to GTK+ 2. */
#else
gint GladeNumLanguages = sizeof (GladeLanguages) / sizeof (GladeLanguages[0]);
#endif

static void glade_project_class_init (GladeProjectClass * klass);
static void glade_project_init (GladeProject *project);
static void glade_project_destroy (GtkObject *object);

static GladeError* glade_project_write_c_source (GladeProject *project);
static GladeError* glade_project_write_cxx_source (GladeProject *project);
static GladeError* glade_project_write_ada95_source (GladeProject *project);
static GladeError* glade_project_write_perl_source (GladeProject *project);
static GladeError* glade_project_write_eiffel_source (GladeProject *project);

static void free_key (gchar	*key,
		      gchar	*value,
		      gpointer	 data);

static gchar* glade_project_find_id (GladeProject *project,
				     const gchar *name);
static void glade_project_real_ensure_widgets_named (GtkWidget    *widget,
						     GladeProject *project);

enum
{
  ADD_COMPONENT,
  REMOVE_COMPONENT,
  COMPONENT_CHANGED,
  LAST_SIGNAL
};

static guint glade_project_signals[LAST_SIGNAL] = {0};

static GtkObjectClass *parent_class = NULL;


GType
glade_project_get_type (void)
{
  static GType glade_project_type = 0;

  if (!glade_project_type)
    {
      GtkTypeInfo glade_project_info =
      {
	"GladeProject",
	sizeof (GladeProject),
	sizeof (GladeProjectClass),
	(GtkClassInitFunc) glade_project_class_init,
	(GtkObjectInitFunc) glade_project_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      glade_project_type = gtk_type_unique (gtk_object_get_type (),
					    &glade_project_info);
    }

  return glade_project_type;
}

static void
glade_project_class_init (GladeProjectClass * klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (gtk_object_get_type ());

  glade_project_signals[ADD_COMPONENT] =
    gtk_signal_new ("add_component",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (GladeProjectClass, add_component),
		    gtk_marshal_VOID__OBJECT,
		    GTK_TYPE_NONE, 1, GTK_TYPE_OBJECT);
  glade_project_signals[REMOVE_COMPONENT] =
    gtk_signal_new ("remove_component",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (GladeProjectClass, remove_component),
		    gtk_marshal_VOID__OBJECT,
		    GTK_TYPE_NONE, 1, GTK_TYPE_OBJECT);
  glade_project_signals[COMPONENT_CHANGED] =
    gtk_signal_new ("component_changed",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (GladeProjectClass, component_changed),
		    gtk_marshal_VOID__OBJECT,
		    GTK_TYPE_NONE, 1, GTK_TYPE_OBJECT);

  klass->add_component = NULL;
  klass->remove_component = NULL;
  klass->component_changed = NULL;

  object_class->destroy = glade_project_destroy;
}


static void
glade_project_init (GladeProject * project)
{
  project->xml_filename = NULL;

  /* The default project name, used when new projects are created. */
  project->name = NULL;
  project->program_name = NULL;
  project->directory = NULL;
  project->source_directory = NULL;
  project->pixmaps_directory = NULL;

  project->language = GLADE_LANGUAGE_C;
  property_show_lang_specific_page (project->language);

  project->changed = FALSE;

  project->unique_id_hash = g_hash_table_new (g_str_hash, g_str_equal);

  project->components = NULL;

  project->pixmap_filenames = NULL;

  project->current_directory = NULL;

  project->gettext_support = TRUE;

#ifdef USE_GNOME
  project->gnome_support = TRUE;
#else
  project->gnome_support = FALSE;
#endif

  project->gnome_db_support = FALSE;
  project->gnome_help_support = FALSE;

  project->use_widget_names = FALSE;
  project->output_main_file = TRUE;
  project->output_support_files = TRUE;
  project->output_build_files = TRUE;
  project->backup_source_files = TRUE;

  project->main_source_file = g_strdup ("interface.c");
  project->main_header_file = g_strdup ("interface.h");
  project->handler_source_file = g_strdup ("callbacks.c");
  project->handler_header_file = g_strdup ("callbacks.h");

  project->support_source_file = g_strdup ("support.c");
  project->support_header_file = g_strdup ("support.h");

  project->output_translatable_strings = FALSE;
  project->translatable_strings_file = NULL;
}


GladeProject*
glade_project_new (void)
{
  /* FIXME: Currently we only support one project open, so we have to destroy
     any existing project, and reset everything. */
  if (current_project)
    {
      property_set_widget (NULL);
      editor_clear_selection (NULL);
      gtk_object_destroy (GTK_OBJECT (current_project));
      /* Delete all GbStyles and reset hash table. Do this after all widgets
	 are destroyed. */
#ifdef GLADE_STYLE_SUPPORT
      gb_widget_reset_gb_styles ();
#endif
    }

  current_project = GLADE_PROJECT (gtk_type_new (glade_project_get_type()));
  return current_project;
}


static void
glade_project_destroy (GtkObject *object)
{
  GladeProject *project;
  GList *tmp_list;

  project = GLADE_PROJECT (object);

  g_free (project->xml_filename);
  project->xml_filename = NULL;

  g_free (project->name);
  project->name = NULL;

  g_free (project->program_name);
  project->program_name = NULL;

  g_free (project->directory);
  project->directory = NULL;

  g_free (project->source_directory);
  project->source_directory = NULL;

  g_free (project->pixmaps_directory);
  project->pixmaps_directory = NULL;

  /* Destroy all project components. */
  tmp_list = project->components;
  while (tmp_list)
    {
      gtk_widget_destroy (GTK_WIDGET (tmp_list->data));
      tmp_list = tmp_list->next;
    }
  g_list_free (project->components);
  project->components = NULL;

  /* Empty the tree. Do this after the widgets are destroyed as it is a bit
     safer since the widgets won't ever have invalid pointers to the tree
     nodes. */
  tree_clear ();

  /* Destroy the unique id hash. */
  if (project->unique_id_hash)
    {
      g_hash_table_foreach (project->unique_id_hash, (GHFunc) free_key, NULL);
      g_hash_table_destroy (project->unique_id_hash);
      project->unique_id_hash = NULL;
    }

  /* Free all pixmap filenames. */
  tmp_list = project->pixmap_filenames;
  while (tmp_list)
    {
      g_free (tmp_list->data);
      tmp_list = tmp_list->next;
    }
  g_list_free (project->pixmap_filenames);
  project->pixmap_filenames = NULL;

  g_free (project->current_directory);
  project->current_directory = NULL;

  g_free (project->main_source_file);
  project->main_source_file = NULL;

  g_free (project->main_header_file);
  project->main_header_file = NULL;

  g_free (project->handler_source_file);
  project->handler_source_file = NULL;

  g_free (project->handler_header_file);
  project->handler_header_file = NULL;

  g_free (project->support_source_file);
  project->support_source_file = NULL;

  g_free (project->support_header_file);
  project->support_header_file = NULL;
}


/* This is called for each id in the unique id hash. We have to free the
   id strings, since they were g_strdup'ed. */
static void
free_key (gchar		*key,
	  gchar		*value,
	  gpointer	 data)
{
  g_free (key);
}


gboolean
glade_project_open	(const gchar   *xml_filename,
			 GladeProject **project_return)
{
  gboolean status;
  GladeProject *project;

  project = glade_project_new ();
  project->xml_filename = g_strdup (xml_filename);

  tree_freeze ();

  status = load_project_file (project);
  
  tree_thaw ();

  if (!status)
    {
      gtk_object_destroy (GTK_OBJECT (project));
      current_project = NULL;
    }
  else
    {
      *project_return = project;
    }
  return status;
}


GladeError*
glade_project_save (GladeProject *project)
{
  GladeError *error;

  error = glade_util_ensure_directory_exists (project->directory);
  if (error)
    return error;

  /* Copy any pixmaps to the project's pixmaps directory. */
  error = glade_project_copy_all_pixmaps (project);
  if (error)
    return error;

  error = save_project_file (project);
  if (error == NULL)
    glade_project_set_changed (project, FALSE);

  return error;
}


GladeError*
glade_project_write_source (GladeProject *project)
{
  GladeError *error;

  /* First we check that we have a project directory, a source directory,
     and a pixmaps directory, and that the source directory is the same as
     the project directory or is a subdirectory of it. */
  if (project->directory == NULL)
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("The project directory is not set.\n"
					"Please set it using the Project Options dialog.\n"));
    }

  if (project->source_directory == NULL)
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("The source directory is not set.\n"
					"Please set it using the Project Options dialog.\n"));
    }

  if (!glade_util_directories_equivalent (project->directory,
					  project->source_directory)
      && !glade_util_directory_contains_file (project->directory,
					      project->source_directory))
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("Invalid source directory:\n\n"
					"The source directory must be the project directory\n"
					"or a subdirectory of the project directory.\n"));
    }

  if (project->pixmaps_directory == NULL)
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("The pixmaps directory is not set.\n"
					"Please set it using the Project Options dialog.\n"));
    }

  /* Copy any pixmaps to the project's pixmaps directory. */
  error = glade_project_copy_all_pixmaps (project);
  if (error)
    return error;

  /* We call a function according to the project's language. */
  switch (project->language)
    {
    case GLADE_LANGUAGE_C:
      return glade_project_write_c_source (project);
    case GLADE_LANGUAGE_CPP:
      return glade_project_write_cxx_source (project);
    case GLADE_LANGUAGE_ADA95:
      return glade_project_write_ada95_source (project);
    case GLADE_LANGUAGE_PERL:
      return glade_project_write_perl_source (project);
    case GLADE_LANGUAGE_EIFFEL:
      return glade_project_write_eiffel_source (project);
    default:
      break;
    }

  /* Shouldn't get here. */
  return glade_error_new_general (GLADE_STATUS_ERROR,
				  _("Sorry - generating source for %s is not implemented yet"),
				  GladeLanguages[project->language]);
}


/* C source code output is built in to Glade, so we just call the main
   C source code generation function here. */
static GladeError*
glade_project_write_c_source (GladeProject *project)
{
  return source_write (project);
}

/*
 * Iterate through the list of widgets to ensure we're not using any
 * deprecated widgets in the project when emitting C++ code.
 */
static gboolean
check_deprecated_widget (GList* list)
{
    GtkWidget* widget;
    GList* tmpList;
    gboolean status;

    for (; list != NULL; list = list->next )
      {
        widget = GTK_WIDGET (list->data);

        if (GTK_IS_CONTAINER (widget))
	  {
            tmpList = gtk_container_get_children (GTK_CONTAINER (widget));
            status = check_deprecated_widget (tmpList);
            g_list_free (tmpList);

            if (!status)
	      {
                return FALSE;
	      }
	  }

	/* Check if any deprecated widgets are being used. Note that we only
	   check for widgets created by Glade, since GtkCombo uses a GtkList
	   internally and we don't want to disallow that. */
        if (GB_IS_GB_WIDGET (widget)
	    && (GTK_IS_CLIST        (widget) ||
		GTK_IS_CTREE        (widget) ||
		GTK_IS_LIST         (widget) ||
		GTK_IS_PIXMAP       (widget) ||
		GTK_IS_PREVIEW      (widget) ))
	  {
            return FALSE;
	  }
      }

    return TRUE;
}

/* Use system() to run glade-- on the XML file to generate C++ source code. */
static GladeError*
glade_project_write_cxx_source (GladeProject *project)
{
    gchar *command_buffer;
    gint status;

    /*
     * Ensure that our project does not have deprecated widgets
     * that gtkmm-2 doesn't support.
     */
    if ( !check_deprecated_widget (project->components))
      {
        return glade_error_new_general (GLADE_STATUS_ERROR,
		_("Your project uses deprecated widgets that Gtkmm-2\n"
		  "does not support.  Check your project for these\n"
		  "widgets, and use their replacements."));
      }

  command_buffer = g_strdup_printf ("glade-- %s", project->xml_filename);
  status = system (command_buffer);
  g_free (command_buffer);

  if (status != 0)
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("Error running glade-- to generate the C++ source code.\n"
					"Check that you have glade-- installed and that it is in your PATH.\n"
					"Then try running 'glade-- <project_file.glade>' in a terminal."));
    }

  return NULL;
}


/* Use system() to run gate on the XML file to generate Ada95 source code. */
static GladeError*
glade_project_write_ada95_source (GladeProject *project)
{
  gchar *command_buffer;
  gint status;

#ifdef _WIN32
  chdir(project->directory);
#endif
  
  command_buffer = g_strdup_printf ("gate %s", project->xml_filename);
  status = system (command_buffer);
  g_free (command_buffer);

  if (status != 0)
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("Error running gate to generate the Ada95 source code.\n"
					"Check that you have gate installed and that it is in your PATH.\n"
					"Then try running 'gate <project_file.glade>' in a terminal."));
    }

  return NULL;
}


/* Use system() to run gate on the XML file to generate Ada95 source code. */
static GladeError*
glade_project_write_perl_source (GladeProject *project)
{
  gchar *command_buffer;
  gint status;
 
  command_buffer = g_strdup_printf ("glade2perl %s", project->xml_filename);
  status = system (command_buffer);
  g_free (command_buffer);

  if (status != 0)
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("Error running glade2perl to generate the Perl source code.\n"
					"Check that you have glade2perl installed and that it is in your PATH.\n"
					"Then try running 'glade2perl <project_file.glade>' in a terminal."));
    }

  return NULL;
}


/* Use system() to run eglade on the XML file to generate Eiffel source code. */
static GladeError*
glade_project_write_eiffel_source (GladeProject *project)
{
	gchar *command_buffer;
	gint status;

	command_buffer = g_strdup_printf ("eglade %s", project->xml_filename);
	status = system (command_buffer);
	g_free (command_buffer);

	if (status != 0)
	{
		return glade_error_new_general (GLADE_STATUS_ERROR,
			_("Error running eglade to generate the Eiffel source code.\n"
			  "Check that you have eglade installed and that it is in your PATH.\n"
			  "Then try running 'eglade <project_file.glade>' in a terminal."));
	}

	return NULL;
}


/* This should be called by anything which changes the project - adding or
   removing widgets, changing properties, setting project options etc.
   FIXME: It's not used throughout Glade yet. */
void
glade_project_set_changed		(GladeProject  *project,
					 gboolean	changed)
{
  project->changed = changed;
}


gchar*
glade_project_get_name	(GladeProject  *project)
{
  return project->name;
}


void
glade_project_set_name	(GladeProject  *project,
			 const gchar   *name)
{
  if (glade_util_strings_equivalent (project->name, name))
    return;
  g_free (project->name);
  project->name = g_strdup (name);
  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_program_name	(GladeProject  *project)
{
  return project->program_name;
}


void
glade_project_set_program_name	(GladeProject  *project,
				 const gchar   *program_name)
{
  if (glade_util_strings_equivalent (project->program_name, program_name))
    return;
  g_free (project->program_name);
  project->program_name = g_strdup (program_name);
  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_xml_filename	(GladeProject  *project)
{
  return project->xml_filename;
}


void
glade_project_set_xml_filename	(GladeProject  *project,
				 const gchar   *filename)
{
  gchar *xml_directory;

  if (glade_util_strings_equivalent (project->xml_filename, filename))
    return;
  g_free (project->xml_filename);
  project->xml_filename = g_strdup (filename);
  xml_directory = glade_util_dirname (filename);

  /* If the project directories are not set, set them to defaults based on
     the directory the XML file is in. */
  if (!project->directory || project->directory[0] == '\0')
    {
      project->directory = g_strdup (xml_directory);
    }

  if (!project->source_directory || project->source_directory[0] == '\0')
    {
      project->source_directory = glade_util_make_path (project->directory,
							"src");
    }

  if (!project->pixmaps_directory || project->pixmaps_directory[0] == '\0')
    {
      project->pixmaps_directory = glade_util_make_path (project->directory,
							"pixmaps");
    }

  g_free (xml_directory);
  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_directory	(GladeProject  *project)
{
  return project->directory;
}


void
glade_project_set_directory	(GladeProject  *project,
				 const gchar   *directory)
{
  if (glade_util_strings_equivalent (project->directory, directory))
    return;
  g_free (project->directory);
  project->directory = g_strdup (directory);
  glade_project_set_changed (project, TRUE);

  /* If the pixmaps directory is not set, set it to defaults based on
     the source directory. */
  if (directory && directory[0] != '\0')
    {
      if (!project->pixmaps_directory || project->pixmaps_directory[0] == '\0')
	{
	  project->pixmaps_directory = glade_util_make_path (directory,
							     "pixmaps");
	}
    }
}


gchar*
glade_project_get_source_directory	(GladeProject  *project)
{
  return project->source_directory;
}


void
glade_project_set_source_directory	(GladeProject  *project,
					 const gchar   *directory)
{
  if (glade_util_strings_equivalent (project->source_directory, directory))
    return;
  g_free (project->source_directory);
  project->source_directory = g_strdup (directory);

  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_pixmaps_directory	(GladeProject  *project)
{
  return project->pixmaps_directory;
}


void
glade_project_set_pixmaps_directory	(GladeProject  *project,
					 const gchar   *directory)
{
  if (glade_util_strings_equivalent (project->pixmaps_directory, directory))
    return;
  g_free (project->pixmaps_directory);
  project->pixmaps_directory = g_strdup (directory);
  glade_project_set_changed (project, TRUE);
}


gint
glade_project_get_language	(GladeProject  *project)
{
  return project->language;
}


void
glade_project_set_language	(GladeProject     *project,
				 GladeLanguageType language)
{
  if (project->language == language)
    return;
  project->language = language;
  property_show_lang_specific_page (language);
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_set_language_name	(GladeProject  *project,
				 const gchar   *language_name)
{
  gint language;

  if (language_name == NULL || language_name[0] == '\0')
    return FALSE;

  for (language = 0; language < GladeNumLanguages; language++)
    {
      if (!strcmp (language_name, GladeLanguages[language]))
	{
	  glade_project_set_language (project, language);
	  return TRUE;
	}
    }
  return FALSE;
}


void
glade_project_add_component		(GladeProject  *project,
					 GtkWidget     *component)
{
  project->components = g_list_append (project->components, component);
  tree_add_widget (component);
  gtk_signal_emit (GTK_OBJECT (project),
		   glade_project_signals[ADD_COMPONENT], component);
  glade_project_set_changed (project, TRUE);
}


void
glade_project_show_component		(GladeProject	*project,
					 GtkWidget	*component)
{
  /* Popup menus are shown in the menu editor. */
  if (GTK_IS_MENU (component))
    {
      GtkWidget *menued;

      menued = glade_menu_editor_new (project, GTK_MENU_SHELL (component));
      gtk_widget_show (menued);
    }
  else if (GTK_IS_WINDOW (component))
    {
      gtk_widget_show (component);
      /* This maps the window, which de-iconifies it according to the ICCCM. */
      gdk_window_show (component->window);
      /* This raises is to the top, in case it was hidden. */
      gdk_window_raise (component->window);
    }
  else
    g_warning ("Don't know how to show component.");
}


void
glade_project_component_changed	(GladeProject  *project,
				 GtkWidget     *component)
{
  gtk_signal_emit (GTK_OBJECT (project),
		   glade_project_signals[COMPONENT_CHANGED], component);
  glade_project_set_changed (project, TRUE);
}


void
glade_project_foreach_component		(GladeProject  *project,
					 GtkCallback    callback,
					 gpointer       callback_data)
{
  GList *tmp_list;

  g_return_if_fail (project != NULL);

  tmp_list = project->components;
  while (tmp_list)
    {
      (*callback) (GTK_WIDGET (tmp_list->data), callback_data);
      tmp_list = tmp_list->next;
    }
}


void
glade_project_remove_component		(GladeProject  *project,
					 GtkWidget     *component)
{
  project->components = g_list_remove (project->components, component);
  gtk_signal_emit (GTK_OBJECT (project),
		   glade_project_signals[REMOVE_COMPONENT], component);

  /* FIXME: These could be better. */
  property_set_widget (NULL);
  editor_clear_selection (NULL);

  gtk_widget_destroy (component);
  glade_project_set_changed (project, TRUE);
}


/* These add/remove pixmaps to the project. The same filename can appear
   more than once in the project's list of pixmaps, so refcounting isn't
   needed. The filename is copied. */
void
glade_project_add_pixmap		(GladeProject  *project,
					 const gchar   *filename)
{
  if (filename && filename[0])
    {
#if 0
      g_print ("Adding pixmap: %s\n", filename);
#endif
      project->pixmap_filenames = g_list_prepend (project->pixmap_filenames,
						  g_strdup (filename));
      glade_project_set_changed (project, TRUE);
    }
}


/* This removes the given pixmap from the project. If the pixmap isn't in
   the project it is ignored. */
void
glade_project_remove_pixmap		(GladeProject  *project,
					 const gchar   *filename)
{
  GList *element;

  if (!filename)
    return;

#if 0
  g_print ("Removing pixmap: %s\n", filename);
#endif

  element = project->pixmap_filenames;
  while (element)
    {
      if (!strcmp (element->data, filename))
	{
	  project->pixmap_filenames = g_list_remove_link (project->pixmap_filenames, element);
	  g_free (element->data);
	  g_list_free (element);

	  glade_project_set_changed (project, TRUE);
	  break;
	}
      element = element->next;
    }
}


/* This ensures that all pixmaps are in the project's pixmaps directory,
   by copying them if necessary. Note that it doesn't change the pixmap
   filenames in the list. They will be updated the next time the project is
   opened. */
GladeError*
glade_project_copy_all_pixmaps (GladeProject  *project)
{
  gchar *pixmaps_dir, *filename, *new_filename;
  gint pixmaps_dir_len;
  GList *element;
  GladeError *error = NULL;
  gboolean checked_pixmaps_dir = FALSE;

  pixmaps_dir = glade_project_get_pixmaps_directory (project);
  if (!pixmaps_dir || pixmaps_dir[0] == '\0')
    {
      return glade_error_new_general (GLADE_STATUS_ERROR,
				      _("The pixmap directory is not set.\n"
					"Please set it using the Project Options dialog.\n"));
    }

  pixmaps_dir_len = strlen (pixmaps_dir);

  element = project->pixmap_filenames;
  while (element)
    {
      filename = (gchar*) element->data;

      /* If the start of the pixmap filename doesn't match the pixmaps
	 directory, we make sure it is copied there. */
      if (!glade_util_directory_contains_file (pixmaps_dir, filename))
	{
	  new_filename = glade_util_make_path (pixmaps_dir,
					       g_basename (filename));

	  /* Check if it already exists, and copy if it doesn't. */
	  if (!glade_util_file_exists (new_filename))
	    {
	      /* We only want to do this once. */
	      if (!checked_pixmaps_dir)
		{
		  checked_pixmaps_dir = TRUE;
		  error = glade_util_ensure_directory_exists (project->pixmaps_directory);
		  if (error)
		    {
		      g_free (new_filename);
		      break;
		    }
		}

	      error = glade_util_copy_file (filename, new_filename);
	      g_free (new_filename);
	      if (error)
		break;
	    }
	}
      element = element->next;
    }

  return error;
}


gchar*
glade_project_get_current_directory	(GladeProject  *project)
{
  return project->current_directory;
}


void
glade_project_set_current_directory	(GladeProject  *project,
					 const gchar   *directory)
{
  g_free (project->current_directory);
  project->current_directory = g_strdup (directory);
}


gboolean
glade_project_get_gnome_support	(GladeProject  *project)
{
  return project->gnome_support;
}


void
glade_project_set_gnome_support	(GladeProject  *project,
				 gboolean       gnome_support)
{
  if (project->gnome_support == gnome_support)
    return;

  /* If we don't have Gnome support compiled-in, we can't build a Gnome app. */
#ifndef USE_GNOME
  if (gnome_support == TRUE)
    return;
#endif

  project->gnome_support = gnome_support;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_gnome_db_support	(GladeProject  *project)
{
  return project->gnome_db_support;
}


void
glade_project_set_gnome_db_support	(GladeProject  *project,
					 gboolean       gnome_db_support)
{
  if (project->gnome_db_support == gnome_db_support)
    return;

  project->gnome_db_support = gnome_db_support;
  glade_project_set_changed (project, TRUE);
}


/*
 * C Output options.
 */
gboolean
glade_project_get_gettext_support	(GladeProject  *project)
{
  return project->gettext_support;
}


void
glade_project_set_gettext_support	(GladeProject  *project,
					 gboolean       gettext_support)
{
  if (project->gettext_support == gettext_support)
    return;
  project->gettext_support = gettext_support;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_use_widget_names	(GladeProject  *project)
{
  return project->use_widget_names;
}


void
glade_project_set_use_widget_names	(GladeProject  *project,
					 gboolean       use_widget_names)
{
  if (project->use_widget_names == use_widget_names)
    return;
  project->use_widget_names = use_widget_names;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_output_main_file	(GladeProject  *project)
{
  return project->output_main_file;
}


void
glade_project_set_output_main_file	(GladeProject  *project,
					 gboolean       output_main_file)
{
  if (project->output_main_file == output_main_file)
    return;
  project->output_main_file = output_main_file;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_output_support_files	(GladeProject  *project)
{
  return project->output_support_files;
}


void
glade_project_set_output_support_files	(GladeProject  *project,
					 gboolean       output_support_files)
{
  if (project->output_support_files == output_support_files)
    return;
  project->output_support_files = output_support_files;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_output_build_files	(GladeProject  *project)
{
  return project->output_build_files;
}


void
glade_project_set_output_build_files	(GladeProject  *project,
					 gboolean       output_build_files)
{
  if (project->output_build_files == output_build_files)
    return;
  project->output_build_files = output_build_files;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_backup_source_files	(GladeProject  *project)
{
  return project->backup_source_files;
}


void
glade_project_set_backup_source_files	(GladeProject  *project,
					 gboolean       backup_source_files)
{
  if (project->backup_source_files == backup_source_files)
    return;
  project->backup_source_files = backup_source_files;
  glade_project_set_changed (project, TRUE);
}


gboolean
glade_project_get_gnome_help_support	(GladeProject  *project)
{
  return project->gnome_help_support;
}


void
glade_project_set_gnome_help_support	(GladeProject  *project,
					 gboolean       gnome_help_support)
{
  if (project->gnome_help_support == gnome_help_support)
    return;

  project->gnome_help_support = gnome_help_support;
  glade_project_set_changed (project, TRUE);
}


/* FIXME: These will be removed when the source code output is improved. */
void
glade_project_get_source_files (GladeProject *project,
				gchar	**main_source_file,
				gchar	**main_header_file,
				gchar	**handler_source_file,
				gchar	**handler_header_file)
{
  *main_source_file = project->main_source_file;
  *main_header_file = project->main_header_file;
  *handler_source_file = project->handler_source_file;
  *handler_header_file = project->handler_header_file;
}


void
glade_project_set_source_files (GladeProject *project,
				const gchar  *main_source_file,
				const gchar  *main_header_file,
				const gchar  *handler_source_file,
				const gchar  *handler_header_file)
{
  if (glade_util_strings_equivalent (project->main_source_file,
				     main_source_file)
      && glade_util_strings_equivalent (project->main_header_file,
					main_header_file)
      && glade_util_strings_equivalent (project->handler_source_file,
					handler_source_file)
      && glade_util_strings_equivalent (project->handler_header_file,
					handler_header_file))
    return;

  g_free (project->main_source_file);
  g_free (project->main_header_file);
  g_free (project->handler_source_file);
  g_free (project->handler_header_file);
  project->main_source_file = g_strdup (main_source_file);
  project->main_header_file = g_strdup (main_header_file);
  project->handler_source_file = g_strdup (handler_source_file);
  project->handler_header_file = g_strdup (handler_header_file);
  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_support_source_file	(GladeProject  *project)
{
  return project->support_source_file;
}


void
glade_project_set_support_source_file	(GladeProject  *project,
					 const gchar*   support_source_file)
{
  if (glade_util_strings_equivalent (project->support_source_file,
				     support_source_file))
    return;
  g_free (project->support_source_file);
  project->support_source_file = g_strdup (support_source_file);
  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_support_header_file	(GladeProject  *project)
{
  return project->support_header_file;
}


void
glade_project_set_support_header_file	(GladeProject  *project,
					 const gchar*   support_header_file)
{
  if (glade_util_strings_equivalent (project->support_header_file,
				     support_header_file))
    return;
  g_free (project->support_header_file);
  project->support_header_file = g_strdup (support_header_file);
  glade_project_set_changed (project, TRUE);
}


/*
 * libglade options.
 */

gboolean
glade_project_get_output_translatable_strings (GladeProject  *project)
{
  return project->output_translatable_strings;
}


void
glade_project_set_output_translatable_strings (GladeProject  *project,
					       gboolean       output_translatable_strings)
{
  if (project->output_translatable_strings == output_translatable_strings)
    return;
  project->output_translatable_strings = output_translatable_strings;
  glade_project_set_changed (project, TRUE);
}


gchar*
glade_project_get_translatable_strings_file (GladeProject  *project)
{
  return project->translatable_strings_file;
}


void
glade_project_set_translatable_strings_file (GladeProject  *project,
					     const gchar*   file)
{
  if (glade_util_strings_equivalent (project->translatable_strings_file, file))
    return;
  g_free (project->translatable_strings_file);
  project->translatable_strings_file = g_strdup (file);
  glade_project_set_changed (project, TRUE);
}


/*
 * Loading Project Options.
 */
typedef enum {
    PARSER_OPTION,
    PARSER_UNKNOWN
} OptionsParserState;

typedef struct _GladeOptionsParseState GladeOptionsParseState;
struct _GladeOptionsParseState {
  OptionsParserState state;

  GladeProject *project;

  gchar *base_directory;

  GString *option_name;
  GString *option_value;
};


/* This sets one project option, as it is loaded by the SAX parser. */
void
glade_project_load_option (GladeOptionsParseState *state)
{
  GladeProject *project;
  gchar *base, *option_name, *option_value;

  project = state->project;
  base = state->base_directory;
  option_name = state->option_name->str;
  option_value = state->option_value->str;

#if 0
  g_print ("Setting Option:'%s' to '%s'\n", option_name, option_value);
#endif

  if (!strcmp (option_name, "name"))
    {
      g_free (project->name);
      project->name = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "program_name"))
    {
      g_free (project->program_name);
      project->program_name = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "directory") && base)
    {
      g_free (project->directory);
      project->directory = glade_util_make_absolute_path (base, option_value);
    }
  else if (!strcmp (option_name, "source_directory") && base)
    {
      g_free (project->source_directory);
      project->source_directory = glade_util_make_absolute_path (base,
								 option_value);
    }
  else if (!strcmp (option_name, "pixmaps_directory") && base)
    {
      g_free (project->pixmaps_directory);
      project->pixmaps_directory = glade_util_make_absolute_path (base,
								  option_value);
    }
  else if (!strcmp (option_name, "language"))
    {
      if (!glade_project_set_language_name (project, option_value))
	g_warning ("Invalid source language");
    }
  else if (!strcmp (option_name, "gnome_support"))
    {
      gboolean gnome_support = load_parse_bool (NULL, option_value);
#ifndef	USE_GNOME
      if (gnome_support == TRUE)
	{
	  g_warning ("Glade has been compiled without support for Gnome.");
	}
#endif
      project->gnome_support = gnome_support;
    }
  else if (!strcmp (option_name, "gnome_db_support"))
    {
      gboolean gnome_db_support = load_parse_bool (NULL, option_value);
#ifndef	USE_GNOME_DB
      if (gnome_db_support == TRUE)
	{
	  g_warning ("Glade has been compiled without support for Gnome DB.");
	}
#endif
      project->gnome_db_support = gnome_db_support;
    }
  else if (!strcmp (option_name, "gettext_support"))
    {
      project->gettext_support = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "use_widget_names"))
    {
      project->use_widget_names = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "output_main_file"))
    {
      project->output_main_file = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "output_support_files"))
    {
      project->output_support_files = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "output_build_files"))
    {
      project->output_build_files = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "backup_source_files"))
    {
      project->backup_source_files = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "gnome_help_support"))
    {
      project->gnome_help_support = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "main_source_file"))
    {
      g_free (project->main_source_file);
      project->main_source_file = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "main_header_file"))
    {
      g_free (project->main_header_file);
      project->main_header_file = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "handler_source_file"))
    {
      g_free (project->handler_source_file);
      project->handler_source_file = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "handler_header_file"))
    {
      g_free (project->handler_header_file);
      project->handler_header_file = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "support_source_file"))
    {
      g_free (project->support_source_file);
      project->support_source_file = glade_util_copy_string (option_value);
    }
  else if (!strcmp (option_name, "support_header_file"))
    {
      g_free (project->support_header_file);
      project->support_header_file = glade_util_copy_string (option_value);
    }
  
  else if (!strcmp (option_name, "output_translatable_strings"))
    {
      project->output_translatable_strings = load_parse_bool (NULL, option_value);
    }
  else if (!strcmp (option_name, "translatable_strings_file") && base)
    {
      g_free (project->translatable_strings_file);
      project->translatable_strings_file = glade_util_make_absolute_path (base, option_value);
    }

  else
    {
      g_warning ("Unknown project option: %s\n", option_name);
    }
}


static xmlEntityPtr
glade_options_parser_get_entity(GladeOptionsParseState *state, const xmlChar *name)
{
    return xmlGetPredefinedEntity(name);
}

static void
glade_options_parser_warning(GladeOptionsParseState *state, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_WARNING, msg, args);
    va_end(args);
}

static void
glade_options_parser_error(GladeOptionsParseState *state, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_CRITICAL, msg, args);
    va_end(args);
}

static void
glade_options_parser_fatal_error(GladeOptionsParseState *state, const char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    g_logv("XML", G_LOG_LEVEL_ERROR, msg, args);
    va_end(args);
}

static void
glade_options_parser_start_document(GladeOptionsParseState *state)
{
  state->state = PARSER_UNKNOWN;

  state->option_name = g_string_sized_new (128);
  state->option_value = g_string_sized_new (1024);
}

static void
glade_options_parser_end_document(GladeOptionsParseState *state)
{
  g_string_free (state->option_name, TRUE);
  g_string_free (state->option_value, TRUE);
}

static void
glade_options_parser_start_element(GladeOptionsParseState *state,
			   const xmlChar *name, const xmlChar **attrs)
{
  g_string_assign (state->option_name, name);
  g_string_truncate (state->option_value, 0);

  state->state = PARSER_OPTION;
}

static void
glade_options_parser_end_element(GladeOptionsParseState *state, const xmlChar *name)
{
  if (state->state != PARSER_OPTION)
    return;

  glade_project_load_option (state);

  state->state = PARSER_UNKNOWN;
}

static void
glade_options_parser_characters(GladeOptionsParseState *state, const xmlChar *chars, int len)
{
    switch (state->state) {
    case PARSER_OPTION:
	g_string_append_len (state->option_value, chars, len);
	break;
    default:
	/* don't care about content in any other states */
	break;
    }
}

static xmlSAXHandler glade_options_parser = {
    0, /* internalSubset */
    0, /* isStandalone */
    0, /* hasInternalSubset */
    0, /* hasExternalSubset */
    0, /* resolveEntity */
    (getEntitySAXFunc)glade_options_parser_get_entity, /* getEntity */
    0, /* entityDecl */
    0, /* notationDecl */
    0, /* attributeDecl */
    0, /* elementDecl */
    0, /* unparsedEntityDecl */
    0, /* setDocumentLocator */
    (startDocumentSAXFunc)glade_options_parser_start_document, /* startDocument */
    (endDocumentSAXFunc)glade_options_parser_end_document, /* endDocument */
    (startElementSAXFunc)glade_options_parser_start_element, /* startElement */
    (endElementSAXFunc)glade_options_parser_end_element, /* endElement */
    0, /* reference */
    (charactersSAXFunc)glade_options_parser_characters, /* characters */
    0, /* ignorableWhitespace */
    0, /* processingInstruction */
    (commentSAXFunc)0, /* comment */
    (warningSAXFunc)glade_options_parser_warning, /* warning */
    (errorSAXFunc)glade_options_parser_error, /* error */
    (fatalErrorSAXFunc)glade_options_parser_fatal_error, /* fatalError */
};




/* Returns TRUE if the options file was found and loaded OK. */
gboolean
glade_project_load_options (GladeProject *project)
{
  gchar *filename, *base;
  gboolean retval = FALSE;

  /* Check if the options file exists. If it doesn't we use defaults for
     everything. */
  filename = g_strdup_printf ("%sp", GladeSessionFile ? GladeSessionFile : project->xml_filename);
  base = project->xml_filename ? g_dirname (project->xml_filename) : NULL;

  if (glade_util_file_exists (filename))
    {
      GladeOptionsParseState state = { 0 };

      state.project = project;
      state.base_directory = base;

      if (xmlSAXUserParseFile (&glade_options_parser, &state, filename) < 0)
	{
	  g_warning("document not well formed!");
	}
      else
	{
	  retval = TRUE;
	}
    }

  /* Check that the directory options are set to defaults, but only if we
     have a project filename set. We may not have one when loading a session
     file. */
  if (base)
    {
      if (project->directory == NULL)
	project->directory = g_strdup (base);
      if (project->source_directory == NULL)
	project->source_directory = glade_util_make_absolute_path (base, "src");
      if (project->pixmaps_directory == NULL)
	project->pixmaps_directory = glade_util_make_absolute_path (base,
								    "pixmaps");
    }

  g_free (filename);
  g_free (base);

  return retval;
}


/*
 * Saving Project Options.
 */
static void
save_option (GString *buffer, gint indent, gchar *tag_name, gchar *tag_value)
{
  gint i;

  for (i = 0; i < indent; i++)
    g_string_append (buffer, "  ");

  g_string_append_printf (buffer, "<%s>%s</%s>\n",
			  tag_name, tag_value ? tag_value : "", tag_name);
}


static void
save_bool_option (GString *buffer, gint indent, gchar *tag_name,
		  gboolean tag_value)
{
  save_option (buffer, indent, tag_name, tag_value ? "TRUE" : "FALSE");
}


GladeError*
glade_project_save_options (GladeProject *project,
			    FILE *fp)
{
  GladeError *error = NULL;
  GString *buffer;
  gchar *base_dir, *dir, *file;
  gint indent = 1, bytes_written;

  buffer = g_string_sized_new (1024);

  g_string_append (buffer, "<glade-project>\n");

  save_option (buffer, indent, "name", project->name);
  save_option (buffer, indent, "program_name", project->program_name);

  /* All directories are saved relative to the xml file's directory. */
  base_dir = glade_util_dirname (project->xml_filename);

  /* We use defaults for most properties so only a few properties need to be
     saved. */

  dir = glade_util_make_relative_path (base_dir, project->directory);
  if (strcmp (dir, ""))
    save_option (buffer, indent, "directory", dir);
  g_free (dir);

  dir = glade_util_make_relative_path (base_dir, project->source_directory);
  if (strcmp (dir, "src"))
    save_option (buffer, indent, "source_directory", dir);
  g_free (dir);

  dir = glade_util_make_relative_path (base_dir, project->pixmaps_directory);
  if (strcmp (dir, "pixmaps"))
    save_option (buffer, indent, "pixmaps_directory", dir);
  g_free (dir);

  if (project->language != GLADE_LANGUAGE_C)
    save_option (buffer, indent, "language", GladeLanguages[project->language]);
  if (!project->gnome_support)
    save_bool_option (buffer, indent, "gnome_support", project->gnome_support);

  if (project->gnome_db_support)
    save_bool_option (buffer, indent, "gnome_db_support", project->gnome_db_support);


  /*
   * C Options.
   */
  if (!project->gettext_support)
    save_bool_option (buffer, indent, "gettext_support", project->gettext_support);
  if (project->use_widget_names)
    save_bool_option (buffer, indent, "use_widget_names", project->use_widget_names);
  if (!project->output_main_file)
    save_bool_option (buffer, indent, "output_main_file", project->output_main_file);
  if (!project->output_support_files)
    save_bool_option (buffer, indent, "output_support_files", project->output_support_files); 
  if (!project->output_build_files)
    save_bool_option (buffer, indent, "output_build_files", project->output_build_files);
  if (!project->backup_source_files)
    save_bool_option (buffer, indent, "backup_source_files", project->backup_source_files);
  if (project->gnome_help_support)
    save_bool_option (buffer, indent, "gnome_help_support", project->gnome_help_support);

  if (!project->main_source_file
      || strcmp (project->main_source_file, "interface.c"))
    save_option (buffer, indent, "main_source_file", project->main_source_file);
  if (!project->main_header_file
      || strcmp (project->main_header_file, "interface.h"))
    save_option (buffer, indent, "main_header_file", project->main_header_file);
  if (!project->handler_source_file
      || strcmp (project->handler_source_file, "callbacks.c"))
    save_option (buffer, indent, "handler_source_file", project->handler_source_file);
  if (!project->handler_header_file
      || strcmp (project->handler_header_file, "callbacks.h"))
    save_option (buffer, indent, "handler_header_file", project->handler_header_file);
  if (!project->support_source_file
      || strcmp (project->support_source_file, "support.c"))
    save_option (buffer, indent, "support_source_file", project->support_source_file);
  if (!project->support_header_file
      || strcmp (project->support_header_file, "support.h"))
    save_option (buffer, indent, "support_header_file", project->support_header_file);

  if (project->output_translatable_strings)
    save_bool_option (buffer, indent, "output_translatable_strings", TRUE);
  if (project->translatable_strings_file
      && project->translatable_strings_file[0])
    {
      file = glade_util_make_relative_path (base_dir, project->translatable_strings_file);
      if (strcmp (file, ""))
	save_option (buffer, indent, "translatable_strings_file", file);
      g_free (file);
    }

  g_free (base_dir);

  g_string_append (buffer, "</glade-project>\n");

  bytes_written = fwrite (buffer->str, sizeof (gchar), buffer->len, fp);
  if (bytes_written != buffer->len)
    error = glade_error_new_system (_("Error writing project XML file\n"));

  g_string_free (buffer, TRUE);

  return error;
}


/*
 * Functions to do with ensuring widget names are unique.
 */

/* This returns a unique name for a widget using the given base name, often
   a widget class name. If the base name starts with 'Gtk' or 'Gnome' then
   that is taken off. The base name is converted to lower case and a number is
   added on to the end of it to ensure that no other widget in the project
   uses the same name. */
gchar*
glade_project_new_widget_name (GladeProject *project,
			       const gchar  *base_name)
{
  char new_widget_name[128], *id_start;
  gint widget_id, i, new_widget_name_len;

  /* Check we won't overflow the buffer. */
  g_return_val_if_fail (strlen (base_name) < 100, g_strdup (base_name));

  /* Skip 'Gtk' or 'Gnome' at the start of all class names. */
  if (!strncmp (base_name, "Gtk", 3))
    base_name += 3;
  else if (!strncmp (base_name, "Gnome", 5))
    base_name += 5;

  strcpy (new_widget_name, base_name);

  /* Remove any id number at the end of the name. */
  id_start = glade_project_find_id (project, new_widget_name);
  if (id_start)
    *id_start = '\0';

  /* convert name to lower case (only normal ASCII chars) */
  new_widget_name_len = strlen (new_widget_name);
  for (i = 0; i < new_widget_name_len; i++)
    {
      if ((new_widget_name[i] >= 'A') && (new_widget_name[i] <= 'Z'))
	new_widget_name[i] += 'a' - 'A';
    }

  widget_id = GPOINTER_TO_INT (g_hash_table_lookup (project->unique_id_hash,
						    new_widget_name));
  if (widget_id == 0)
    {
      widget_id = 1;
      g_hash_table_insert (project->unique_id_hash, g_strdup (new_widget_name),
			   GINT_TO_POINTER (widget_id));
    }
  else
    {
      widget_id++;
      /* We don't need to g_strdup new_widget_name since it is already in the
         hash. */
      g_hash_table_insert (project->unique_id_hash, new_widget_name,
			   GINT_TO_POINTER (widget_id));
    }

  /* Add the ID onto the end of the name. */
  sprintf (new_widget_name + strlen (new_widget_name), "%i", widget_id);

  MSG1 ("Allocating new widget name: %s", new_widget_name);
  return g_strdup (new_widget_name);
}


/* This releases a widget name, so that it can possibly be used again.
   It will only be reused if it is the last ID with the same prefix. This is
   still useful as it means that if a name is generated based on something
   the user edits (e.g. a menuitem label), only the final name is reserved -
   all the intermediate names get released so we don't waste IDs. */
void
glade_project_release_widget_name (GladeProject *project,
				   const gchar  *name)
{
  gchar buffer[128];
  const gchar *id_start;
  gint id = 0, current_id;
  gpointer hash_key, hash_value;
  gboolean found;

  id_start = glade_project_find_id (project, name);
  if (id_start == NULL)
    return;

  /* Make sure we won't overflow the buffer. */
  g_return_if_fail (id_start - name < 127);

  id = atoi (id_start);
  if (id == 0)
    return;

  strncpy (buffer, name, id_start - name);
  buffer[id_start - name] = '\0';

  found = g_hash_table_lookup_extended (project->unique_id_hash, buffer,
					&hash_key, &hash_value);
  if (found)
    {
      current_id = GPOINTER_TO_INT (hash_value);
      /* We only release the ID if it is the last one. */
      if (current_id == id)
	{
	  /* If the current ID is 1, remove it from the hash completely. */
	  if (current_id == 1)
	    {
	      g_hash_table_remove (project->unique_id_hash, buffer);
	      g_free (hash_key);
	    }
	  else
	    {
	      /* We don't need to g_strdup buffer since it is already in the
		 hash. */
	      g_hash_table_insert (project->unique_id_hash, buffer,
				   GINT_TO_POINTER (id - 1));
	    }
	}
    }
}


/* Check if the name has a trailing ID number, and if so compare it to the
   current maximum in the ID hash and update if necessary. */
void
glade_project_reserve_name (GladeProject *project,
			    const gchar	 *name)
{
  gchar buffer[128];
  const gchar *id_start;
  gint id = 0, current_id;

  id_start = glade_project_find_id (project, name);
  if (id_start == NULL)
    return;

  /* Make sure we won't overflow the buffer. */
  g_return_if_fail (id_start - name < 127);

  id = atoi (id_start);
  if (id == 0)
    return;

  strncpy (buffer, name, id_start - name);
  buffer[id_start - name] = '\0';

  current_id = GPOINTER_TO_INT (g_hash_table_lookup (project->unique_id_hash,
						     buffer));
  if (current_id == 0)
    {
      g_hash_table_insert (project->unique_id_hash, g_strdup (buffer),
			   GINT_TO_POINTER (id));
    }
  else if (id > current_id)
    {
      /* We don't need to g_strdup buffer since it is already in the hash. */
      g_hash_table_insert (project->unique_id_hash, buffer,
			   GINT_TO_POINTER (id));
    }
}


/* This returns the start of the ID number at the end of the widget name,
   or NULL if there is no number. */
static gchar*
glade_project_find_id (GladeProject *project,
		       const gchar *name)
{
  gint pos;

  pos = strlen (name) - 1;
  if (pos <= 0)
    return NULL;

  /* Step back from the end of the string to find the first digit. */
  while (pos > 0 && name[pos] >= '0' && name[pos] <= '9')
    pos--;

  /* We may have gone too far, so check. */
  if (!(name[pos] >= '0' && name[pos] <= '9'))
    pos++;

  /* If there is no trailing number return NULL. */
  if (name[pos] == '\0')
    return NULL;

  return (gchar*) &name[pos];
}


/* This recursively descends a widget tree, ensuring that all widgets have
   names. If any names are NULL, default names are created for them. */
void
glade_project_ensure_widgets_named (GladeProject *project,
				    GtkWidget    *widget)
{
  /* We have to switch the arguments for the recursive calls to work. */
  glade_project_real_ensure_widgets_named (widget, project);
}


static void
glade_project_real_ensure_widgets_named (GtkWidget    *widget,
					 GladeProject *project)
{
  if (GB_IS_GB_WIDGET (widget) && widget->name == NULL)
    {
      gchar *class = gb_widget_get_class_id (widget);
      gtk_widget_set_name (widget,
			   glade_project_new_widget_name (project, class));
      MSG1 ("set default name for widget: %s\n", gtk_widget_get_name (widget));
    }

  gb_widget_children_foreach (widget, (GtkCallback) glade_project_real_ensure_widgets_named, project);
}
