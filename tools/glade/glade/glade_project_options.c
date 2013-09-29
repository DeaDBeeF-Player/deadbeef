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

#include <ctype.h>
#include <string.h>

#include <gtk/gtkalignment.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkvbox.h>

#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#endif

#include "glade_project_options.h"
#include "utils.h"


static GtkWindowClass *parent_class = NULL;


static void glade_project_options_class_init (GladeProjectOptionsClass * klass);
static void glade_project_options_init (GladeProjectOptions * project_options);
static void glade_project_options_destroy (GtkObject *object);

static void glade_project_options_set_project (GladeProjectOptions *options,
					       GladeProject *project);
static void glade_project_options_directory_changed (GtkWidget * entry,
						     GladeProjectOptions *options);
static void glade_project_options_name_changed (GtkWidget * entry,
						GladeProjectOptions *options);
static void glade_project_options_program_name_changed (GtkWidget * entry,
							GladeProjectOptions *options);
static void glade_project_options_xml_filename_changed (GtkWidget * entry,
							GladeProjectOptions *options);
static void glade_project_options_generate_name (GladeProjectOptions *options);
static void glade_project_options_generate_program_name (GladeProjectOptions *options);
static void glade_project_options_generate_xml_filename (GladeProjectOptions *options);
static gchar* glade_project_options_generate_project_name_from_directory (gchar *directory);
static gchar* glade_project_options_generate_program_name_from_project_name (gchar *project_name);
static gchar* glade_project_options_generate_xml_filename_from_program_name (gchar *program_name);
static void glade_project_options_translatable_strings_toggled (GtkWidget *widget,
								GladeProjectOptions *options);
static void glade_project_options_show_file_selection (GtkWidget *widget,
						       gpointer data);
static void on_filesel_response (GtkWidget *widget,
				 gint response_id,
				 GladeProjectOptions *options);
static void glade_project_options_ok (GtkWidget *widget,
				      GladeProjectOptions *options);
static gchar* get_entry_text (GtkWidget *entry);
static gboolean glade_project_options_check_valid	(GladeProjectOptions *options);

GType
glade_project_options_get_type (void)
{
  static GType glade_project_options_type = 0;

  if (!glade_project_options_type)
    {
      GtkTypeInfo glade_project_options_info =
      {
	"GladeProjectOptions",
	sizeof (GladeProjectOptions),
	sizeof (GladeProjectOptionsClass),
	(GtkClassInitFunc) glade_project_options_class_init,
	(GtkObjectInitFunc) glade_project_options_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      glade_project_options_type = gtk_type_unique (gtk_window_get_type (),
						  &glade_project_options_info);
    }
  return glade_project_options_type;
}


static void
glade_project_options_class_init (GladeProjectOptionsClass * klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (gtk_window_get_type ());

  object_class->destroy = glade_project_options_destroy;
}

static void
set_accessible_description (GtkWidget *widget, const gchar *desc)
{
  AtkObject *atk_widget;

  g_return_if_fail (GTK_IS_WIDGET (widget));

  atk_widget = gtk_widget_get_accessible (widget);

  if (desc)
    atk_object_set_description (atk_widget, desc);
}
 

static void
glade_project_options_init (GladeProjectOptions * options)
{
  GtkTooltips *tooltips;
  GtkWidget *main_vbox, *notebook;
  GtkWidget *general_options_vbox, *c_options_vbox, *libglade_options_vbox;
  GtkWidget *vbox, *table, *label, *button, *eventbox, *alignment;
  GtkWidget *hbox, *hbbox, *radio_button, *frame;
  gint row, language;
  GSList *group;

  options->generate_program_name = FALSE;
  options->generate_xml_filename = FALSE;
  options->language_buttons = NULL;
  options->filesel = NULL;
  options->auto_generation_level = 0;

  tooltips = gtk_tooltips_new ();

  gtk_container_set_border_width (GTK_CONTAINER (options), 4);
  gtk_window_set_position (GTK_WINDOW (options), GTK_WIN_POS_MOUSE);
  gtk_window_set_title (GTK_WINDOW (options), _("Project Options"));
  gtk_window_set_policy (GTK_WINDOW (options), FALSE, TRUE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (options), "project_options", "Glade");

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (options), main_vbox);
  gtk_widget_show (main_vbox);

  notebook = gtk_notebook_new ();
  gtk_widget_show (notebook);
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);


  /*
   * General Page.
   */

  label = gtk_label_new (_("General"));
  gtk_widget_show (label);

  general_options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (general_options_vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), general_options_vbox,
			    label);
  gtk_container_set_border_width (GTK_CONTAINER (general_options_vbox), 7);

  frame = gtk_frame_new (_("Basic Options:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (general_options_vbox), frame, FALSE, TRUE, 4);

  table = gtk_table_new (3, 4, TRUE);
  gtk_widget_show (table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);

  /* Project Directory. */
  row = 0;
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The project directory"), NULL);

  label = gtk_label_new (_("Project Directory:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_widget_show (hbox);
  gtk_table_attach (GTK_TABLE (table), hbox,
		    1, 4, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

  options->directory_entry = gtk_entry_new ();
  gtk_widget_show (options->directory_entry);
  gtk_box_pack_start (GTK_BOX (hbox), options->directory_entry,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (options->directory_entry), "changed",
		      GTK_SIGNAL_FUNC (glade_project_options_directory_changed),
		      options);

  button = gtk_button_new_with_label (_("Browse..."));
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 8, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (glade_project_options_show_file_selection),
		      NULL);

  /* Project Name. */
  row++;
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The name of the current project"), NULL);

  label = gtk_label_new (_("Project Name:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->name_entry, 80, -1);
  gtk_widget_show (options->name_entry);
  gtk_table_attach (GTK_TABLE (table), options->name_entry,
		    1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (options->name_entry), "changed",
		      GTK_SIGNAL_FUNC (glade_project_options_name_changed),
		      options);

  /* Program Name. */
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 2, 3, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The name of the program"), NULL);

  label = gtk_label_new (_("Program Name:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->program_name_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->program_name_entry, 80, -1);
  gtk_widget_show (options->program_name_entry);
  gtk_table_attach (GTK_TABLE (table), options->program_name_entry,
		    3, 4, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (options->program_name_entry), "changed",
		      GTK_SIGNAL_FUNC (glade_project_options_program_name_changed),
		      options);

  /* Project XML File. */
  row++;
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The project file"), NULL);

  label = gtk_label_new (_("Project File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->xml_filename_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->xml_filename_entry, 80, -1);
  gtk_widget_show (options->xml_filename_entry);
  gtk_table_attach (GTK_TABLE (table), options->xml_filename_entry,
		    1, 4, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_signal_connect (GTK_OBJECT (options->xml_filename_entry), "changed",
		      GTK_SIGNAL_FUNC (glade_project_options_xml_filename_changed),
		      options);


  /* Project Source Directory. */
  frame = gtk_frame_new (_("Subdirectories:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (general_options_vbox), frame, FALSE, TRUE, 4);

  table = gtk_table_new (1, 4, TRUE);
  gtk_widget_show (table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);

  row = 0;
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The directory to save generated source code"),
			NULL);

  label = gtk_label_new (_("Source Directory:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->source_directory_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->source_directory_entry, 80, -1);
  gtk_widget_show (options->source_directory_entry);
  gtk_table_attach (GTK_TABLE (table), options->source_directory_entry,
		    1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);


  /* Project Pixmaps Directory. */
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 2, 3, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The directory to store pixmaps"),
			NULL);

  label = gtk_label_new (_("Pixmaps Directory:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->pixmaps_directory_entry = gtk_entry_new ();
  gtk_widget_set_usize (options->pixmaps_directory_entry, 80, -1);
  gtk_widget_show (options->pixmaps_directory_entry);
  gtk_table_attach (GTK_TABLE (table), options->pixmaps_directory_entry,
		    3, 4, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);


  /* Project License. */
  /* Take this out until we actually use it. */
#if 0
  row++;
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The license which is added at the top of generated files"), NULL);

  label = gtk_label_new (_("License:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->license_option_menu = gtk_option_menu_new ();
  gtk_widget_show (options->license_option_menu);
  gtk_table_attach (GTK_TABLE (table), options->license_option_menu,
		    1, 3, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);

  menu = gtk_menu_new ();
  menuitem = gtk_menu_item_new_with_label ("GNU General Public License");
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (options->license_option_menu),
			    menu);
#endif

  /* Source Language. */
  frame = gtk_frame_new (_("Language:"));
  /* Change this to 0 if you don't want the language option (C/C++/Perl). */
#if 1
  gtk_widget_show (frame);
#endif
  gtk_box_pack_start (GTK_BOX (general_options_vbox), frame, FALSE, TRUE, 4);

  alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_widget_show (alignment);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_widget_show (hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (alignment), hbox);

  options->language_buttons = g_new (GtkWidget*, GladeNumLanguages);
  group = NULL;
  for (language = 0; language < GladeNumLanguages; language++)
    {
      radio_button = gtk_radio_button_new_with_label (group,
						      GladeLanguages[language]);
      gtk_widget_show (radio_button);
      gtk_box_pack_start (GTK_BOX (hbox), radio_button, FALSE, TRUE, 4);
      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_button));

      options->language_buttons[language] = radio_button;
    }

  /* Gnome Support. */
#ifdef USE_GNOME
  frame = gtk_frame_new (_("Gnome:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (general_options_vbox), frame, FALSE, TRUE, 4);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_widget_show (hbox);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  options->gnome_support = gtk_check_button_new_with_label (_("Enable Gnome Support"));
  /* This can't be changed now. */
  gtk_widget_set_sensitive (options->gnome_support, FALSE);
  gtk_widget_show (options->gnome_support);
  gtk_box_pack_start (GTK_BOX (hbox), options->gnome_support, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, options->gnome_support,
			_("If a Gnome application is to be built"), NULL);

#ifdef USE_GNOME_DB
  options->gnome_db_support = gtk_check_button_new_with_label (_("Enable Gnome DB Support"));
  gtk_widget_show (options->gnome_db_support);
  gtk_box_pack_start (GTK_BOX (hbox), options->gnome_db_support, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, options->gnome_db_support,
			_("If a Gnome DB application is to be built"), NULL);
#endif /* USE_GNOME_DB */

#endif /* USE_GNOME */


  /*
   * C Options Page.
   */

  label = gtk_label_new (_("C Options"));
  gtk_widget_show (label);

  c_options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (c_options_vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), c_options_vbox, label);
  gtk_container_set_border_width (GTK_CONTAINER (c_options_vbox), 7);

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Note:</b> for large applications the use of libglade is recommended."));
  gtk_widget_set_size_request (label, 400, -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (c_options_vbox), label, FALSE, TRUE, 4);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_widget_show (hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (c_options_vbox), hbox, FALSE, TRUE, 4);

  frame = gtk_frame_new (_("General Options:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Gettext Support. */
  options->gettext_support = gtk_check_button_new_with_label (_("Gettext Support"));
  gtk_widget_show (options->gettext_support);
  gtk_box_pack_start (GTK_BOX (vbox), options->gettext_support,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->gettext_support,
			_("If strings are marked for translation by gettext"),
			NULL);

  /* Setting widget names. */
  options->use_widget_names = gtk_check_button_new_with_label (_("Set Widget Names"));
  gtk_widget_show (options->use_widget_names);
  gtk_box_pack_start (GTK_BOX (vbox), options->use_widget_names,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->use_widget_names,
			_("If widget names are set in the source code"),
			NULL);

  /* Backing up source files. */
  options->backup_source_files = gtk_check_button_new_with_label (_("Backup Source Files"));
  gtk_widget_show (options->backup_source_files);
  gtk_box_pack_start (GTK_BOX (vbox), options->backup_source_files,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->backup_source_files,
			_("If copies of old source files are made"),
			NULL);

  /* Gnome Help System support. */
  options->gnome_help_support = gtk_check_button_new_with_label (_("Gnome Help Support"));
#ifdef USE_GNOME
  gtk_widget_show (options->gnome_help_support);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), options->gnome_help_support,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->gnome_help_support,
			_("If support for the Gnome Help system should be included"),
			NULL);

  frame = gtk_frame_new (_("File Output Options:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, TRUE, 0);

  vbox = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Outputting main file. */
  options->output_main_file = gtk_check_button_new_with_label (_("Output main.c File"));
  gtk_widget_show (options->output_main_file);
  gtk_box_pack_start (GTK_BOX (vbox), options->output_main_file,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->output_main_file,
			_("If a main.c file is output containing a main() function, if it doesn't already exist"),
			NULL);

  /* Outputting support files. */
  options->output_support_files = gtk_check_button_new_with_label (_("Output Support Functions"));
  gtk_widget_show (options->output_support_files);
  gtk_box_pack_start (GTK_BOX (vbox), options->output_support_files,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->output_support_files,
			_("If the support functions are output"),
			NULL);

  /* Outputting build files. */
  options->output_build_files = gtk_check_button_new_with_label (_("Output Build Files"));
  gtk_widget_show (options->output_build_files);
  gtk_box_pack_start (GTK_BOX (vbox), options->output_build_files,
		      FALSE, FALSE, 0);
  gtk_tooltips_set_tip (tooltips, options->output_build_files,
			_("If files for building the source code are output, including Makefile.am and configure.in, if they don't already exist"),
			NULL);

  /* Main source file. */
  frame = gtk_frame_new (_("Interface Creation Functions:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (c_options_vbox), frame, FALSE, TRUE, 4);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_widget_show (hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox, _("The file in which the functions to create the interface are written"), NULL);

  label = gtk_label_new (_("Source File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->main_source_entry = gtk_entry_new ();
  gtk_widget_show (options->main_source_entry);
  gtk_widget_set_usize (options->main_source_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (hbox), options->main_source_entry,
		      TRUE, TRUE, 0);

  /* Main header file. */
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox, _("The file in which the declarations of the functions to create the interface are written"), NULL);

  label = gtk_label_new (_("Header File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->main_header_entry = gtk_entry_new ();
  gtk_widget_show (options->main_header_entry);
  gtk_widget_set_usize (options->main_header_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (hbox), options->main_header_entry,
		      TRUE, TRUE, 0);

  set_accessible_description (options->main_source_entry, _("Source file for interface creation functions")); 
  set_accessible_description (options->main_header_entry, _("Header file for interface creation functions")); 

  /* Handler source file. */
  frame = gtk_frame_new (_("Signal Handler & Callback Functions:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (c_options_vbox), frame, FALSE, TRUE, 4);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_widget_show (hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox, _("The file in which the empty signal handler and callback functions are written"), NULL);

  label = gtk_label_new (_("Source File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->handler_source_entry = gtk_entry_new ();
  gtk_widget_show (options->handler_source_entry);
  gtk_widget_set_usize (options->handler_source_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (hbox), options->handler_source_entry,
		      TRUE, TRUE, 0);

  /* Handler header file. */
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox, _("The file in which the declarations of the signal handler and callback functions are written"), NULL);

  label = gtk_label_new (_("Header File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->handler_header_entry = gtk_entry_new ();
  gtk_widget_show (options->handler_header_entry);
  gtk_widget_set_usize (options->handler_header_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (hbox), options->handler_header_entry,
		      TRUE, TRUE, 0);

  set_accessible_description (options->handler_source_entry, _("Source file for signal handler and callback functions"));
  set_accessible_description (options->handler_header_entry, _("Header file for signal handler and callback functions"));

  /* Support source file. */
  frame = gtk_frame_new (_("Support Functions:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (c_options_vbox), frame, FALSE, TRUE, 4);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_widget_show (hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox, _("The file in which the support functions are written"), NULL);

  label = gtk_label_new (_("Source File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->support_source_entry = gtk_entry_new ();
  gtk_widget_show (options->support_source_entry);
  gtk_widget_set_usize (options->support_source_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (hbox), options->support_source_entry,
		      TRUE, TRUE, 0);

  /* Support header file. */
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_box_pack_start (GTK_BOX (hbox), eventbox, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox, _("The file in which the declarations of the support functions are written"), NULL);

  label = gtk_label_new (_("Header File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

  options->support_header_entry = gtk_entry_new ();
  gtk_widget_show (options->support_header_entry);
  gtk_widget_set_usize (options->support_header_entry, 80, -1);
  gtk_box_pack_start (GTK_BOX (hbox), options->support_header_entry,
		      TRUE, TRUE, 0);

  set_accessible_description (options->support_source_entry, _("Source file for support functions"));
  set_accessible_description (options->support_header_entry, _("Header file for support functions")); 

  /*
   * libglade Options Page.
   */

  label = gtk_label_new (_("LibGlade Options"));
  gtk_widget_show (label);

  libglade_options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (libglade_options_vbox);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), libglade_options_vbox,
			    label);
  gtk_container_set_border_width (GTK_CONTAINER (libglade_options_vbox), 7);

  frame = gtk_frame_new (_("Translatable Strings:"));
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (libglade_options_vbox), frame, FALSE, TRUE, 4);

  table = gtk_table_new (2, 3, FALSE);
  gtk_widget_show (table);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);

  row = 0;

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), _("<b>Note:</b> this option is deprecated - use intltool instead."));
  gtk_widget_set_size_request (label, 400, -1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label,
		    0, 3, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  row++;

  /* Output translatable strings. */
  options->output_translatable_strings = gtk_check_button_new_with_label (_("Save Translatable Strings"));
  gtk_widget_show (options->output_translatable_strings);
  gtk_table_attach (GTK_TABLE (table), options->output_translatable_strings,
		    0, 3, row, row + 1, GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, options->output_translatable_strings,
			_("If translatable strings are saved in a separate C source file, to enable translation of interfaces loaded by libglade"),
			NULL);

  /* Translatable Strings File. */
  row++;
  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_table_attach (GTK_TABLE (table), eventbox, 0, 1, row, row + 1,
		    GTK_FILL, 0, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox,
			_("The C source file to save all translatable strings in"), NULL);

  label = gtk_label_new (_("File:"));
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (eventbox), label);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  options->translatable_strings_filename_label = label;

  options->translatable_strings_filename_entry = gtk_entry_new ();
  gtk_widget_show (options->translatable_strings_filename_entry);
  gtk_table_attach (GTK_TABLE (table),
		    options->translatable_strings_filename_entry,
		    1, 2, row, row + 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);


  /* The button box with OK & Cancel buttons. */
  hbbox = gtk_hbutton_box_new ();
  gtk_widget_show (hbbox);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbbox, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbbox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (hbbox), 8);

  options->cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  GTK_WIDGET_SET_FLAGS (options->cancel_button, GTK_CAN_DEFAULT);
  gtk_widget_show (options->cancel_button);
  gtk_container_add (GTK_CONTAINER (hbbox), options->cancel_button);
  gtk_signal_connect_object (GTK_OBJECT (options->cancel_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (options));

  options->ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  GTK_WIDGET_SET_FLAGS (options->ok_button, GTK_CAN_DEFAULT);
  gtk_widget_show (options->ok_button);
  gtk_container_add (GTK_CONTAINER (hbbox), options->ok_button);
  gtk_widget_grab_default (options->ok_button);
  gtk_signal_connect (GTK_OBJECT (options->ok_button), "clicked",
		      GTK_SIGNAL_FUNC (glade_project_options_ok), options);

  gtk_signal_connect (GTK_OBJECT (options), "key_press_event",
		      GTK_SIGNAL_FUNC (glade_util_check_key_is_esc),
		      GINT_TO_POINTER (GladeEscDestroys));

  gtk_widget_grab_focus (options->directory_entry);
}


GtkWidget *
glade_project_options_new (GladeProject *project)
{
  GladeProjectOptions *options;

  g_return_val_if_fail (GLADE_IS_PROJECT (project), NULL);

  options = GLADE_PROJECT_OPTIONS (gtk_type_new (glade_project_options_get_type ()));
  glade_project_options_set_project (options, project);

  return GTK_WIDGET (options);
}


static void
glade_project_options_destroy (GtkObject *object)
{
  GladeProjectOptions *options;

  options = GLADE_PROJECT_OPTIONS (object);

  g_free (options->language_buttons);
  options->language_buttons = NULL;

  /* FIXME: Maybe we need to disconnect the destroy signal handler added to
     the project. */

  if (options->filesel)
    {
      gtk_widget_destroy (options->filesel);
      options->filesel = NULL;
    }
}


/* This sets which project which we show the options of. */
static void
glade_project_options_set_project (GladeProjectOptions *options,
				   GladeProject *project)
{
  GladeError *error;
  gchar *base = NULL, *next_project_dir = NULL;
  gint next_project_num = -1;

  g_return_if_fail (GLADE_IS_PROJECT (project));

  options->project = project;
  options->generate_name = FALSE;
  options->generate_program_name = FALSE;
  options->generate_xml_filename = FALSE;

  /* If the project directory has been set, we show it, else we show the
     next free project directory in $HOME/Projects as the default. */
  if (project->directory && project->directory[0])
    {
      base = project->directory;
      gtk_entry_set_text (GTK_ENTRY (options->directory_entry), base);
    }
  else
    {
      error = glade_util_get_next_free_project_directory (&next_project_dir,
							  &next_project_num);
      if (!error)
	{
	  base = next_project_dir;
	  gtk_entry_set_text (GTK_ENTRY (options->directory_entry), base);
	}
      else
	{
	  g_warning ("Couldn't find next free project directory.");
	  gtk_entry_set_text (GTK_ENTRY (options->directory_entry), "");
	}
    }

  /* Project name. */
  if (project->name && project->name[0])
    {
      gchar *generated_project_name;

      gtk_entry_set_text (GTK_ENTRY (options->name_entry), project->name);

      /* See if it matches what would have been auto-generated. */
      generated_project_name = glade_project_options_generate_project_name_from_directory (base);
      if (!strcmp (generated_project_name, project->name))
	options->generate_name = TRUE;
      g_free (generated_project_name);
    }
  else
    {
      glade_project_options_generate_name (options);
      options->generate_name = TRUE;
    }

  /* Program name. */
  if (project->program_name && project->program_name[0])
    {
      gchar *generated_program_name;

      gtk_entry_set_text (GTK_ENTRY (options->program_name_entry),
			  project->program_name);

      /* See if it matches what would have been auto-generated. */
      generated_program_name = glade_project_options_generate_program_name_from_project_name (project->name);
      if (!strcmp (generated_program_name, project->program_name))
	options->generate_program_name = TRUE;
      g_free (generated_program_name);
    }
  else
    {
      glade_project_options_generate_program_name (options);
      options->generate_program_name = TRUE;
    }

  /* XML filename. */
  if (project->xml_filename && project->xml_filename[0])
    {
      gchar *xml_filename_relative, *generated_filename;

      xml_filename_relative = glade_util_make_relative_path (base, project->xml_filename);
      gtk_entry_set_text (GTK_ENTRY (options->xml_filename_entry),
			  xml_filename_relative);

      /* See if it matches what would have been auto-generated. */
      generated_filename = glade_project_options_generate_xml_filename_from_program_name (project->program_name);
      if (!strcmp (generated_filename, xml_filename_relative))
	options->generate_xml_filename = TRUE;
      g_free (generated_filename);
      g_free (xml_filename_relative);
    }
  else
    {
      glade_project_options_generate_xml_filename (options);
      options->generate_xml_filename = TRUE;
    }

  /* Source directory. */
  if (project->source_directory && project->source_directory[0])
    {
      gchar *srcdir_relative;

      srcdir_relative = glade_util_make_relative_path (base, project->source_directory);
      gtk_entry_set_text (GTK_ENTRY (options->source_directory_entry),
			  srcdir_relative);
      g_free (srcdir_relative);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (options->source_directory_entry),
			  "src");
    }

  /* Pixmaps directory. */
  if (project->pixmaps_directory && project->pixmaps_directory[0])
    {
      gchar *pixmaps_relative;

      pixmaps_relative = glade_util_make_relative_path (base, project->pixmaps_directory);
      gtk_entry_set_text (GTK_ENTRY (options->pixmaps_directory_entry),
			  pixmaps_relative);
      g_free (pixmaps_relative);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (options->pixmaps_directory_entry),
			  "pixmaps");
    }


  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->language_buttons[project->language]), TRUE);

#ifdef USE_GNOME
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->gnome_support),
				project->gnome_support);
#ifdef USE_GNOME_DB
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->gnome_db_support),
				project->gnome_db_support);
#endif
#endif

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->gettext_support),
				project->gettext_support);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_widget_names),
				project->use_widget_names);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->output_main_file),
				project->output_main_file);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->output_support_files),
				project->output_support_files);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->output_build_files),
				project->output_build_files);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->backup_source_files),
				project->backup_source_files);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->gnome_help_support),
				project->gnome_help_support);

  gtk_entry_set_text (GTK_ENTRY (options->main_source_entry),
		      project->main_source_file
		        ? project->main_source_file : "");
  gtk_entry_set_text (GTK_ENTRY (options->main_header_entry),
		      project->main_header_file
		        ? project->main_header_file : "");
  gtk_entry_set_text (GTK_ENTRY (options->handler_source_entry),
		      project->handler_source_file
		        ? project->handler_source_file : "");
  gtk_entry_set_text (GTK_ENTRY (options->handler_header_entry),
		      project->handler_header_file
		        ? project->handler_header_file : "");

  gtk_entry_set_text (GTK_ENTRY (options->support_source_entry),
		      project->support_source_file
			? project->support_source_file : "");
  gtk_entry_set_text (GTK_ENTRY (options->support_header_entry),
		      project->support_header_file
			? project->support_header_file : "");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->output_translatable_strings),
				project->output_translatable_strings);
  if (project->translatable_strings_file
      && project->translatable_strings_file[0])
    {
      gchar *filename;

      filename = glade_util_make_relative_path (base, project->translatable_strings_file);
      gtk_entry_set_text (GTK_ENTRY (options->translatable_strings_filename_entry),
			  filename);
      g_free (filename);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (options->translatable_strings_filename_entry), "");
    }
  gtk_signal_connect (GTK_OBJECT (options->output_translatable_strings),
		      "toggled",
		      GTK_SIGNAL_FUNC (glade_project_options_translatable_strings_toggled),
		      options);

  /* Set the sensitivity of the options. */
  gtk_widget_set_sensitive (options->translatable_strings_filename_label,
			    project->output_translatable_strings);
  gtk_widget_set_sensitive (options->translatable_strings_filename_entry,
			    project->output_translatable_strings);


  /* Connect to the project's destroy signal so we can destroy ourself. */
  gtk_signal_connect_object (GTK_OBJECT (project), "destroy",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (options));

  g_free (next_project_dir);
}


static void
glade_project_options_directory_changed (GtkWidget * entry,
					 GladeProjectOptions *options)
{
  if (options->generate_name)
    glade_project_options_generate_name (options);
}


static void
glade_project_options_name_changed (GtkWidget * entry,
				    GladeProjectOptions *options)
{
  if (options->generate_program_name)
    glade_project_options_generate_program_name (options);

  /* If this option has been edited by the user, turn auto-generation off. */
  if (options->auto_generation_level == 0)
    options->generate_name = FALSE;
}


static void
glade_project_options_program_name_changed (GtkWidget * entry,
					    GladeProjectOptions *options)
{
  if (options->generate_xml_filename)
    glade_project_options_generate_xml_filename (options);

  /* If this option has been edited by the user, turn auto-generation off. */
  if (options->auto_generation_level == 0)
    options->generate_program_name = FALSE;
}


static void
glade_project_options_xml_filename_changed (GtkWidget * entry,
					    GladeProjectOptions *options)
{
  /* If this option has been edited by the user, turn auto-generation off. */
  if (options->auto_generation_level == 0)
    options->generate_xml_filename = FALSE;
}


static void
glade_project_options_generate_name (GladeProjectOptions *options)
{
  gchar *directory, *project_name;

  directory = (char*) gtk_entry_get_text (GTK_ENTRY (options->directory_entry));
  project_name = glade_project_options_generate_project_name_from_directory (directory);
  options->auto_generation_level++;
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), project_name);
  options->auto_generation_level--;
  g_free (project_name);
}


static gchar*
glade_project_options_generate_project_name_from_directory (gchar *directory)
{
  gchar *project_name;

  if (directory == NULL)
    return g_strdup ("");

  project_name = g_strdup (g_basename (directory));
  if (project_name[0] != '\0')
    project_name[0] = toupper (project_name[0]);
  return project_name;
}


static void
glade_project_options_generate_program_name (GladeProjectOptions *options)
{
  gchar *project_name, *program_name;

  project_name = (char*) gtk_entry_get_text (GTK_ENTRY (options->name_entry));
  program_name = glade_project_options_generate_program_name_from_project_name (project_name);
  options->auto_generation_level++;
  gtk_entry_set_text (GTK_ENTRY (options->program_name_entry), program_name);
  options->auto_generation_level--;
  g_free (program_name);
}


static gchar*
glade_project_options_generate_program_name_from_project_name (gchar *project_name)
{
  gchar *program_name, *pos;

  if (project_name == NULL)
    return g_strdup ("");

  program_name = g_strdup (project_name);
  g_strdown (program_name);
  /* Try to convert any characters which shouldn't be in a program name to '-'.
     We'll allow alphanumerics and characters in [.+-_] */
  for (pos = program_name; *pos; pos++)
    {
      if (!(isalnum (*pos) || *pos == '.' || *pos == '+' || *pos == '-'
	    || *pos == '_'))
	*pos = '-';
    }
  return program_name;
}


static void
glade_project_options_generate_xml_filename (GladeProjectOptions *options)
{
  gchar *program_name, *xml_filename;

  program_name = (char*) gtk_entry_get_text (GTK_ENTRY (options->program_name_entry));
  xml_filename = glade_project_options_generate_xml_filename_from_program_name (program_name);
  options->auto_generation_level++;
  gtk_entry_set_text (GTK_ENTRY (options->xml_filename_entry), xml_filename);
  options->auto_generation_level--;
  g_free (xml_filename);
}


static gchar*
glade_project_options_generate_xml_filename_from_program_name (gchar *program_name)
{
  gchar *xml_filename;

  if (program_name == NULL)
    return g_strdup ("");

  xml_filename = g_strdup_printf ("%s.glade", program_name);
  return xml_filename;
}


static void
glade_project_options_translatable_strings_toggled (GtkWidget *widget,
						    GladeProjectOptions *options)
{
  gboolean sensitive;

  sensitive = GTK_TOGGLE_BUTTON (options->output_translatable_strings)->active;

  /* Set the sensitivity of the options. */
  gtk_widget_set_sensitive (options->translatable_strings_filename_label,
			    sensitive);
  gtk_widget_set_sensitive (options->translatable_strings_filename_entry,
			    sensitive);
}


static void
glade_project_options_show_file_selection (GtkWidget *widget,
					   gpointer   data)
{
  GladeProjectOptions *options;
  gchar *filename;

  options = GLADE_PROJECT_OPTIONS (gtk_widget_get_toplevel (widget));

  if (options->filesel == NULL)
    {
      options->filesel = gtk_file_chooser_dialog_new (_("Select the Project Directory"),
						      GTK_WINDOW (options),
						      GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER,
						      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OK, GTK_RESPONSE_OK,
						      NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (options->filesel),
				       GTK_RESPONSE_OK);
 
      g_signal_connect (options->filesel, "response",
			GTK_SIGNAL_FUNC (on_filesel_response), options);
      g_signal_connect (options->filesel, "delete_event",
			G_CALLBACK (gtk_true), NULL);
    }

  filename = (char*) gtk_entry_get_text (GTK_ENTRY (options->directory_entry));
  if (glade_util_file_exists (filename))
    {
      glade_util_set_file_selection_filename (options->filesel, filename);
    }

  gtk_window_present (GTK_WINDOW (options->filesel));
}


static void
on_filesel_response (GtkWidget *widget,
		     gint response_id,
		     GladeProjectOptions *options)
{
  gchar *filename;

  if (response_id == GTK_RESPONSE_OK)
    {
      filename = glade_util_get_file_selection_filename (options->filesel);
      if (filename)
	gtk_entry_set_text (GTK_ENTRY (options->directory_entry), filename);
      g_free (filename);
    }

  glade_util_close_window (options->filesel);
}


static void
glade_project_options_ok (GtkWidget *widget,
			  GladeProjectOptions *options)
{
  GladeProject *project;
  gchar *xml_filename, *directory, *source_directory, *pixmaps_directory;
  gchar *strings_filename;
  gint language;

  project = options->project;

  /* First check that the options are valid, according to the requested action.
     If not, we need to stop the signal so the dialog isn't closed. */
  if (!glade_project_options_check_valid (options))
    {
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "clicked");
      return;
    }

  /* The project directory is the only aboslute path. Everything else is
     relative to it, so needs to be turned into an absolute path here. */
  directory = get_entry_text (options->directory_entry);
  glade_project_set_directory (project, directory);

  glade_project_set_name (project, get_entry_text (options->name_entry));
  glade_project_set_program_name (project, get_entry_text (options->program_name_entry));

  pixmaps_directory = get_entry_text (options->pixmaps_directory_entry);
  pixmaps_directory = glade_util_make_absolute_path (directory,
						     pixmaps_directory);
  glade_project_set_pixmaps_directory (project, pixmaps_directory);
  g_free (pixmaps_directory);

  source_directory = get_entry_text (options->source_directory_entry);
  source_directory = glade_util_make_absolute_path (directory,
						    source_directory);
  glade_project_set_source_directory (project, source_directory);
  g_free (source_directory);

  xml_filename = get_entry_text (options->xml_filename_entry);
  xml_filename = glade_util_make_absolute_path (directory, xml_filename);
  glade_project_set_xml_filename (project, xml_filename);
  g_free (xml_filename);

  for (language = 0; language < GladeNumLanguages; language++)
    {

      if (GTK_TOGGLE_BUTTON (options->language_buttons[language])->active)
	{
	  glade_project_set_language_name (project, GladeLanguages[language]);
	  break;
	}
    }


#ifdef USE_GNOME
  glade_project_set_gnome_support (project, GTK_TOGGLE_BUTTON (options->gnome_support)->active ? TRUE : FALSE);
#ifdef USE_GNOME_DB
  glade_project_set_gnome_db_support (project, GTK_TOGGLE_BUTTON (options->gnome_db_support)->active ? TRUE : FALSE);
#endif
#endif

  glade_project_set_gettext_support (project, GTK_TOGGLE_BUTTON (options->gettext_support)->active ? TRUE : FALSE);

  glade_project_set_use_widget_names (project, GTK_TOGGLE_BUTTON (options->use_widget_names)->active ? TRUE : FALSE);

  glade_project_set_output_main_file (project, GTK_TOGGLE_BUTTON (options->output_main_file)->active ? TRUE : FALSE);

  glade_project_set_output_support_files (project, GTK_TOGGLE_BUTTON (options->output_support_files)->active ? TRUE : FALSE);

  glade_project_set_output_build_files (project, GTK_TOGGLE_BUTTON (options->output_build_files)->active ? TRUE : FALSE);

  glade_project_set_backup_source_files (project, GTK_TOGGLE_BUTTON (options->backup_source_files)->active ? TRUE : FALSE);

  glade_project_set_gnome_help_support (project, GTK_TOGGLE_BUTTON (options->gnome_help_support)->active ? TRUE : FALSE);

  glade_project_set_source_files (project,
				  get_entry_text (options->main_source_entry),
				  get_entry_text (options->main_header_entry),
				  get_entry_text (options->handler_source_entry),
				  get_entry_text (options->handler_header_entry));
  glade_project_set_support_source_file (project, get_entry_text (options->support_source_entry));
  glade_project_set_support_header_file (project, get_entry_text (options->support_header_entry));

  glade_project_set_output_translatable_strings (project, GTK_TOGGLE_BUTTON (options->output_translatable_strings)->active ? TRUE : FALSE);

  strings_filename = get_entry_text (options->translatable_strings_filename_entry);
  strings_filename = glade_util_make_absolute_path (directory,
						    strings_filename);
  glade_project_set_translatable_strings_file (project, strings_filename);
  g_free (strings_filename);

  g_free (directory);
  gtk_widget_destroy (GTK_WIDGET (options));
}


/* This returns a pointer to the text in the given entry, or NULL if the entry
   is empty. */
static gchar*
get_entry_text (GtkWidget *entry)
{
  gchar *text;

  text = (char*) gtk_entry_get_text (GTK_ENTRY (entry));
  if (text && text[0] != '\0')
    return text;
  else
    return NULL;
}


void
glade_project_options_set_action	(GladeProjectOptions *options,
					 GladeProjectOptionsAction action)
{
  options->action = action;
}


/* This checks that all the options needed for the required action have been
   set, and returns TRUE if so. */
static gboolean
glade_project_options_check_valid	(GladeProjectOptions *options)
{
  gchar *error = NULL;
  gchar *directory, *xml_filename, *project_name, *program_name;
  gchar *source_directory, *pixmaps_directory;
  gboolean output_translatable_strings;
  gchar *translatable_strings_filename;

  directory = get_entry_text (options->directory_entry);
  xml_filename = get_entry_text (options->xml_filename_entry);
  project_name = get_entry_text (options->name_entry);
  program_name = get_entry_text (options->program_name_entry);
  source_directory = get_entry_text (options->source_directory_entry);
  pixmaps_directory = get_entry_text (options->pixmaps_directory_entry);
  output_translatable_strings = GTK_TOGGLE_BUTTON (options->output_translatable_strings)->active;
  translatable_strings_filename = get_entry_text (options->translatable_strings_filename_entry);

  switch (options->action)
    {
    case GLADE_PROJECT_OPTIONS_ACTION_NORMAL:
      if (output_translatable_strings
	  && (translatable_strings_filename == NULL
	      || translatable_strings_filename[0] == '\0'))
	error = _("You need to set the Translatable Strings File option");
      break;
    case GLADE_PROJECT_OPTIONS_ACTION_SAVE:
      if (directory == NULL || directory[0] == '\0')
	error = _("You need to set the Project Directory option");
      else if (xml_filename == NULL || xml_filename[0] == '\0')
	error = _("You need to set the Project File option");
      else if (output_translatable_strings
	       && (translatable_strings_filename == NULL
		   || translatable_strings_filename[0] == '\0'))
	error = _("You need to set the Translatable Strings File option");
      break;
    case GLADE_PROJECT_OPTIONS_ACTION_BUILD:
      if (directory == NULL || directory[0] == '\0')
	error = _("You need to set the Project Directory option");
      else if (xml_filename == NULL || xml_filename[0] == '\0')
	error = _("You need to set the Project File option");
      else if (output_translatable_strings
	       && (translatable_strings_filename == NULL
		   || translatable_strings_filename[0] == '\0'))
	error = _("You need to set the Translatable Strings File option");
      else if (project_name == NULL || project_name[0] == '\0')
	error = _("You need to set the Project Name option");
      else if (program_name == NULL || program_name[0] == '\0')
	error = _("You need to set the Program Name option");
#if 0
      else if (source_directory == NULL || source_directory[0] == '\0')
	error = _("You need to set the Source Directory option");
#endif
      else if (pixmaps_directory == NULL || pixmaps_directory[0] == '\0')
	error = _("You need to set the Pixmaps Directory option");
      break;
    }

  if (error)
    {
      glade_util_show_message_box (error, GTK_WIDGET (options));
      return FALSE;
    }

  return TRUE;
}
