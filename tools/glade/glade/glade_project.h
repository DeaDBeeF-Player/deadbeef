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
#ifndef GLADE_PROJECT_H
#define GLADE_PROJECT_H

#include "gbwidget.h"
#include "glade.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * GladeProject is a subclass of GtkObject and contains a project's data,
 * which is viewed in a GladeProjectView.
 */

#define GLADE_PROJECT(obj)          GTK_CHECK_CAST (obj, glade_project_get_type (), GladeProject)
#define GLADE_PROJECT_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_project_get_type (), GladeProjectClass)
#define GLADE_IS_PROJECT(obj)       GTK_CHECK_TYPE (obj, glade_project_get_type ())


typedef enum
{
  GLADE_LANGUAGE_C,
  GLADE_LANGUAGE_CPP,
  GLADE_LANGUAGE_ADA95,
  GLADE_LANGUAGE_PERL,
  GLADE_LANGUAGE_EIFFEL
} GladeLanguageType;

extern gchar *GladeLanguages[];
extern gint GladeNumLanguages;

typedef struct _GladeProjectClass  GladeProjectClass;

/* All directories are absolute paths in GladeProject, but some are saved in
   the XML file as relative paths so the project can be placed in different
   directories (e.g. it should still work if multiple developers check it out
   from a CVS repository into different directories.) */
struct _GladeProject
{
  GtkObject object;

  gchar *name;

  /* The name of the executable to build. */
  gchar *program_name;

  /* This is the XML file. */
  gchar *xml_filename;

  /* This is the project directory, which defaults to the directory which the
     XML file is in. */
  gchar *directory;

  /* These are the directories for the source code and pixmaps files, and
     default to the project directory and the 'pixmaps' subdirectory. */
  gchar *source_directory;
  gchar *pixmaps_directory;

  /* The currently selected language of the project, an index into
     GladeLanguages[]. */
  GladeLanguageType language;

  /* If the project has changed since the last time it was saved.
     FIXME: this isn't completely supported yet. */
  gboolean changed;

  /* The windows, dialogs & popup menus making up the project. */
  GList *components;

  /* A hash table used to ensure that all widget names are unique. */
  GHashTable *unique_id_hash;

  /* A list of absolute pixmap filenames used in the project. The same file
     may appear more than once, so we don't need to use refcounting. */
  GList *pixmap_filenames;

  /* This is the project's 'current directory', used when opening/saving
     files. We may want separate directories for opening/saving/writing
     source. */
  gchar *current_directory;

  /* This is TRUE if we are building a Gnome application. It defaults to
     TRUE if Glade has been compiled with Gnome support, though it can be
     set to FALSE. If Glade doesn't have Gnome support it must be FALSE,
     and we need to check that any projects loaded aren't Gnome projects. */
  gboolean gnome_support;

  /* This is TRUE to add support for Gnome DB. */
  gboolean gnome_db_support;


  /*
   * C Source code options.
   */

  /* This is TRUE if translatable strings are output using the standard
     gettext macros, _("") and N_(""). */
  gboolean gettext_support;

  /* This is TRUE if gtk_widget_set_name () is called to set the name of
     each widget when they are created. Useful when using rc files. */
  gboolean use_widget_names;

  /* This is TRUE if a main.c file is output with a main() function.
     Though an existing main.c will not be overwritten. */
  gboolean output_main_file;

  /* This is TRUE if a support file (support.c by default) is output with the
     get_widget() function and other support functions. */
  gboolean output_support_files;

  /* This is TRUE if Makefile.am, configure.in, autogen.sh, m4 macros etc.
     are output. */
  gboolean output_build_files;

  /* This is TRUE if backups are made of the source files. */
  gboolean backup_source_files;

  /* This is TRUE to add support for Gnome Help. Currently we just output
     the GNOME_UIINFO_HELP macro at the top of the Help GnomeUIInfo structs,
     but we will output a template help file with build files in future. */
  gboolean gnome_help_support;

  /* FIXME: These will be deleted soon, when we support better code output. */
  gchar *main_source_file;
  gchar *main_header_file;
  gchar *handler_source_file;
  gchar *handler_header_file;

  gchar *support_source_file;
  gchar *support_header_file;


  /*
   * libglade options.
   */

  /* This is TRUE to output a file containing all the translatable strings in
     the interface, wrapped in gettext macros. When using libglade, this file
     can be added to an app's POTFILES.in so the interface is translated. */
  gboolean output_translatable_strings;

  /* This is the file in which translatable strings are saved, useful when
     XML interfaces are loaded dynamically by libglade. */
  gchar *translatable_strings_file;
};


struct _GladeProjectClass
{
  GtkObjectClass parent_class;

  void   (*add_component)        (GladeProject   *project,
				  GtkWidget      *component);
  void   (*remove_component)     (GladeProject   *project,
				  GtkWidget      *component);
  void   (*component_changed)    (GladeProject   *project,
				  GtkWidget      *component);
};


/* FIXME: Currently we only support one project open at once, and this is it.
   But we will support multiple projects in future, so try not to use this
   too much. */
extern GladeProject *current_project;


GType       glade_project_get_type		(void);
GladeProject*   glade_project_new		(void);

/* Note that even if this command succeeds, there may be some error messages
   which should be displayed (e.g. some widget properties may be invalid).
   The list of error messages should be freed, as well as each message. */
gboolean    glade_project_open			(const gchar   *xml_filename,
						 GladeProject **project);
GladeError* glade_project_save			(GladeProject  *project);

/* If an error occurs a GladeError will be returned, which should be freed. */
GladeError* glade_project_write_source		(GladeProject  *project);

/* This sets/resets the project's changed flag. */
void	    glade_project_set_changed		(GladeProject  *project,
						 gboolean	changed);

/*
 * These are for ensuring that widget names are unique.
 */

/* This returns a new unique widget name, based on the given base name
   (often a widget class name). Any leading 'Gtk' or 'Gnome' is stripped,
   it is converted to lower case, and a number is appended to ensure it is
   unique. */
gchar*	    glade_project_new_widget_name	(GladeProject  *project,
						 const gchar   *base_name);

/* This releases the given name, so it may possibly be reused in future.
   It is used when a widget is destroyed or its name is changed. */
void	    glade_project_release_widget_name	(GladeProject  *project,
						 const gchar   *name);

/* This reserves the given name, so that no other widgets will be given it.
   It is used when loading the XML file. */
void	    glade_project_reserve_name		(GladeProject  *project,
						 const gchar   *name);

/* This ensures that the given widget and all its descendants have been named,
   creating default names for them if necessary. */
void	    glade_project_ensure_widgets_named  (GladeProject *project,
						 GtkWidget    *widget);

/*
 * These are for loading & saving the project options to the XML file.
 */
gboolean    glade_project_load_options		(GladeProject  *project);
GladeError* glade_project_save_options		(GladeProject  *project,
						 FILE	       *fp);


/*
 * Accessor functions.
 */
gchar*	    glade_project_get_name		(GladeProject  *project);
void	    glade_project_set_name		(GladeProject  *project,
						 const gchar   *name);

gchar*	    glade_project_get_program_name	(GladeProject  *project);
void	    glade_project_set_program_name	(GladeProject  *project,
						 const gchar   *program_name);

gchar*	    glade_project_get_xml_filename	(GladeProject  *project);
void	    glade_project_set_xml_filename	(GladeProject  *project,
						 const gchar   *filename);

gchar*	    glade_project_get_directory		(GladeProject  *project);
void	    glade_project_set_directory		(GladeProject  *project,
						 const gchar   *directory);

gchar*	    glade_project_get_source_directory	(GladeProject  *project);
void	    glade_project_set_source_directory	(GladeProject  *project,
						 const gchar   *directory);

gchar*	    glade_project_get_pixmaps_directory	(GladeProject  *project);
void	    glade_project_set_pixmaps_directory	(GladeProject  *project,
						 const gchar   *directory);

gint	    glade_project_get_language		(GladeProject  *project);
void	    glade_project_set_language		(GladeProject  *project,
						 GladeLanguageType language);
gboolean    glade_project_set_language_name	(GladeProject  *project,
						 const gchar   *language_name);

gboolean    glade_project_get_gnome_support	(GladeProject  *project);
void	    glade_project_set_gnome_support	(GladeProject  *project,
						 gboolean       gnome_support);

gboolean    glade_project_get_gnome_db_support	(GladeProject  *project);
void	    glade_project_set_gnome_db_support	(GladeProject  *project,
						 gboolean       gnome_db_support);

void	    glade_project_add_component		(GladeProject  *project,
						 GtkWidget     *component);
void	    glade_project_show_component	(GladeProject	*project,
						 GtkWidget	*component);
void	    glade_project_component_changed	(GladeProject  *project,
						 GtkWidget     *component);
void	    glade_project_foreach_component	(GladeProject  *project,
						 GtkCallback    callback,
						 gpointer       callback_data);
void	    glade_project_remove_component	(GladeProject  *project,
						 GtkWidget     *component);

void	    glade_project_add_associated_window	(GladeProject  *project,
						 GtkWidget     *window);
void	    glade_project_remove_associated_window (GladeProject *project,
						    GtkWidget    *window);

/* These add/remove pixmaps to the project. The same filename can appear
   more than once in the project's list of pixmaps, so refcounting isn't
   needed. The filename is copied. */
void	    glade_project_add_pixmap		(GladeProject  *project,
						 const gchar   *filename);
void	    glade_project_remove_pixmap		(GladeProject  *project,
						 const gchar   *filename);

/* This ensures that all pixmaps are in the project's pixmaps directory,
   by copying them if necessary. */
GladeError* glade_project_copy_all_pixmaps	(GladeProject  *project);

gchar*	    glade_project_get_current_directory	(GladeProject  *project);
void	    glade_project_set_current_directory	(GladeProject  *project,
						 const gchar   *directory);


/*
 * C Output options.
 */
gboolean    glade_project_get_gettext_support	(GladeProject  *project);
void	    glade_project_set_gettext_support	(GladeProject  *project,
						 gboolean       gettext_support);
gboolean    glade_project_get_use_widget_names	(GladeProject  *project);
void	    glade_project_set_use_widget_names	(GladeProject  *project,
						 gboolean       use_widget_names);
gboolean    glade_project_get_output_main_file	(GladeProject  *project);
void	    glade_project_set_output_main_file	(GladeProject  *project,
						 gboolean       output_main_file);
gboolean    glade_project_get_output_support_files (GladeProject *project);
void	    glade_project_set_output_support_files (GladeProject *project,
						    gboolean      output_support_files);
gboolean    glade_project_get_output_build_files (GladeProject  *project);
void	    glade_project_set_output_build_files (GladeProject  *project,
						  gboolean       output_build_files);
gboolean    glade_project_get_backup_source_files (GladeProject  *project);
void	    glade_project_set_backup_source_files (GladeProject  *project,
						   gboolean       backup_source_files);
gboolean    glade_project_get_gnome_help_support (GladeProject  *project);
void	    glade_project_set_gnome_help_support (GladeProject  *project,
						  gboolean       gnome_help_support);

/* These will be removed when the source code output is improved. */
void	    glade_project_get_source_files (GladeProject *project,
					    gchar	**main_source_file,
					    gchar	**main_header_file,
					    gchar	**handler_source_file,
					    gchar	**handler_header_file);
void	    glade_project_set_source_files (GladeProject *project,
					    const gchar	 *main_source_file,
					    const gchar	 *main_header_file,
					    const gchar	 *handler_source_file,
					    const gchar	 *handler_header_file);

gchar*	    glade_project_get_support_source_file (GladeProject  *project);
void	    glade_project_set_support_source_file (GladeProject  *project,
						   const gchar*   support_source_file);

gchar*	    glade_project_get_support_header_file (GladeProject  *project);
void	    glade_project_set_support_header_file (GladeProject  *project,
						  const gchar*   support_header_file);


/*
 * libglade options.
 */
gboolean    glade_project_get_output_translatable_strings (GladeProject  *project);
void	    glade_project_set_output_translatable_strings (GladeProject  *project,
							   gboolean       output_translatable_strings);

gchar*	    glade_project_get_translatable_strings_file (GladeProject  *project);
void	    glade_project_set_translatable_strings_file (GladeProject  *project,
							 const gchar*   file);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_PROJECT_H */
