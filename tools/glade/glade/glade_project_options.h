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
#ifndef GLADE_PROJECT_OPTIONS_H
#define GLADE_PROJECT_OPTIONS_H

#include <gtk/gtkwindow.h>

#include "glade_project.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * GladeProjectOptions is a subclass of GtkWindow, used to set the project
 * options.
 */

#define GLADE_PROJECT_OPTIONS(obj)          GTK_CHECK_CAST (obj, glade_project_options_get_type (), GladeProjectOptions)
#define GLADE_PROJECT_OPTIONS_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_project_options_get_type (), GladeProjectOptionsClass)
#define GLADE_IS_PROJECT_OPTIONS(obj)       GTK_CHECK_TYPE (obj, glade_project_options_get_type ())

typedef enum
{
  GLADE_PROJECT_OPTIONS_ACTION_NORMAL,
  GLADE_PROJECT_OPTIONS_ACTION_SAVE,
  GLADE_PROJECT_OPTIONS_ACTION_BUILD
} GladeProjectOptionsAction;


typedef struct _GladeProjectOptions       GladeProjectOptions;
typedef struct _GladeProjectOptionsClass  GladeProjectOptionsClass;

struct _GladeProjectOptions
{
  GtkWindow window;

  /* This is the project whose options we are editing. */
  GladeProject *project;

  /* This is the action which will be taken after the dialog is closed.
     We will ensure that any options needed for the action are set properly
     before the dialog is closed. */
  GladeProjectOptionsAction action;

  /*
   * General options page.
   */
  GtkWidget *name_entry;
  gboolean generate_name;
  GtkWidget *program_name_entry;
  gboolean generate_program_name;
  GtkWidget *xml_filename_entry;
  gboolean generate_xml_filename;
  GtkWidget *directory_entry;
  GtkWidget *source_directory_entry;
  GtkWidget *pixmaps_directory_entry;

  GtkWidget **language_buttons;

  /* Take this out until we support it. */
  /*GtkWidget *license_option_menu;*/

  GtkWidget *gnome_support;
  GtkWidget *gnome_db_support;

  /*
   * C options page.
   */
  GtkWidget *gettext_support;
  GtkWidget *use_widget_names;
  GtkWidget *output_main_file;
  GtkWidget *output_support_files;
  GtkWidget *output_build_files;
  GtkWidget *backup_source_files;
  GtkWidget *gnome_help_support;

  GtkWidget *main_source_entry;
  GtkWidget *main_header_entry;
  GtkWidget *handler_source_entry;
  GtkWidget *handler_header_entry;
  GtkWidget *support_source_entry;
  GtkWidget *support_header_entry;

  /*
   * libglade options page.
   */
  GtkWidget *output_translatable_strings;
  GtkWidget *translatable_strings_filename_label;
  GtkWidget *translatable_strings_filename_entry;


  /*
   * C options page.
   */
  GtkWidget *ok_button;
  GtkWidget *cancel_button;

  /* This is the file selection dialog we use for all file selections. */
  GtkWidget *filesel;

  /* This is incremented when we are auto-generating the project name, program
     name & xml filename. It is used to tell whether an options is being
     auto-generated or the user has edited it. */
  gint auto_generation_level;
};


struct _GladeProjectOptionsClass
{
  GtkWindowClass parent_class;
};




GType      glade_project_options_get_type	(void);
GtkWidget* glade_project_options_new		(GladeProject *project);

void	   glade_project_options_set_action	(GladeProjectOptions *options,
						 GladeProjectOptionsAction action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_PROJECT_OPTIONS_H */
