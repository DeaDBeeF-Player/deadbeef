/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999  Damon Chaplin
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

#include <config.h>

#include <gnome.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-fileentry.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Directory = "GnomeFileEntry::directory_entry";
static gchar *HistoryID = "GnomeFileEntry|GnomeEntry::history_id";
static gchar *MaxSaved = "GnomeFileEntry|GnomeEntry::max_saved";
static gchar *Title = "GnomeFileEntry::browse_dialog_title";
static gchar *Modal = "GnomeFileEntry::modal";
static gchar *FileChooser = "GnomeFileEntry::use_filechooser";

static gchar *Action = "GnomeFileEntry::filechooser_action";


static const gchar *GbActionChoices[] =
{"Open", "Save", "Select Folder", "Create Folder", NULL};
static const gint GbActionValues[] =
{
  GTK_FILE_CHOOSER_ACTION_OPEN,
  GTK_FILE_CHOOSER_ACTION_SAVE,
  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
  GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER
};
static const gchar *GbActionSymbols[] =
{
  "GTK_FILE_CHOOSER_ACTION_OPEN",
  "GTK_FILE_CHOOSER_ACTION_SAVE",
  "GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER",
  "GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeFileEntry, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_gnome_file_entry_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget, *entry;

  new_widget = gnome_file_entry_new (NULL, NULL);

  entry = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (new_widget));
  gb_widget_create_from (entry,
			 data->action == GB_CREATING ? "combo-entry" : NULL);
  gb_widget_set_child_name (entry, GladeChildGnomeEntry);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_file_entry_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_string (HistoryID, _("History ID:"),
		       _("The ID to save the history entries under"));
  property_add_int_range (MaxSaved, _("Max Saved:"),
			  _("The maximum number of history entries saved"),
			  0, 10000, 1, 10, 1);
  property_add_string (Title, _("Title:"),
		       _("The title of the file selection dialog"));
  property_add_bool (Directory, _("Directory:"),
		     _("If a directory is needed rather than a file"));
  property_add_bool (Modal, _("Modal:"),
		     _("If the file selection dialog should be modal"));
  property_add_bool (FileChooser, _("Use FileChooser:"),
		     _("Use the new GtkFileChooser widget instead of GtkFileSelection"));
  property_add_choice (Action, _("Action:"),
		       _("The type of file operation being performed"),
		       GbActionChoices);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_file_entry_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GtkWidget *gentry;
  gboolean   use_filechooser = FALSE;
  GtkFileChooserAction action;
  gint i;

  gentry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (widget));

  gb_widget_output_string (data, HistoryID, gtk_object_get_data (GTK_OBJECT (widget), HistoryID));
  gb_widget_output_int (data, MaxSaved, gnome_entry_get_max_saved (GNOME_ENTRY (gentry)));
  gb_widget_output_translatable_string (data, Title, gtk_object_get_data (GTK_OBJECT (widget), Title));
  gb_widget_output_bool (data, Directory,
			 gnome_file_entry_get_directory_entry (GNOME_FILE_ENTRY (widget)));
  gb_widget_output_bool (data, Modal,
			 gnome_file_entry_get_modal (GNOME_FILE_ENTRY (widget)));
  g_object_get (G_OBJECT (widget),
		"use_filechooser", &use_filechooser,
		"filechooser_action", &action,
		NULL); 
  gb_widget_output_bool (data, FileChooser, use_filechooser);

  for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]); i++)
    {
      if (GbActionValues[i] == action)
	gb_widget_output_choice (data, Action, i, GbActionSymbols[i]);
    }
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_file_entry_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GtkWidget *gentry;
  gchar *history_id, *title, *action;
  gint max_saved, i;
  gboolean directory, modal, use_filechooser;

  gentry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (widget));

  history_id = gb_widget_input_string (data, HistoryID);
  if (data->apply)
    gtk_object_set_data_full (GTK_OBJECT (widget), HistoryID,
			      g_strdup (history_id),
			      history_id ? g_free : NULL);

  max_saved = gb_widget_input_int (data, MaxSaved);
  if (data->apply)
    gnome_entry_set_max_saved (GNOME_ENTRY (gentry), max_saved);

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    {
      gtk_object_set_data_full (GTK_OBJECT (widget), Title, g_strdup (title),
				title ? g_free : NULL);
      gnome_file_entry_set_title (GNOME_FILE_ENTRY (widget),
				  title && title[0] ? title : NULL);
    }

  directory = gb_widget_input_bool (data, Directory);
  if (data->apply)
    gnome_file_entry_set_directory_entry (GNOME_FILE_ENTRY (widget), directory);

  modal = gb_widget_input_bool (data, Modal);
  if (data->apply)
    gnome_file_entry_set_modal (GNOME_FILE_ENTRY (widget), modal);

  use_filechooser = gb_widget_input_bool (data, FileChooser);
  if (data->apply)
    g_object_set (G_OBJECT (widget), "use_filechooser", use_filechooser, NULL);

  action = gb_widget_input_choice (data, Action);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]);
	   i++)
	{
	  if (!strcmp (action, GbActionChoices[i])
	      || !strcmp (action, GbActionSymbols[i]))
	    {
	      g_object_set (widget, "filechooser_action", GbActionValues[i], NULL);
	      break;
	    }
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeFileEntry, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_file_entry_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_file_entry_write_source (GtkWidget * widget,
				  GbWidgetWriteSourceData * data)
{
  GtkWidget *gentry, *entry;
  gchar *title, *history_id;
  gchar *wname, *child_name;
  gboolean use_filechooser = FALSE;
  GtkFileChooserAction action;
  gboolean translatable, context;
  gchar *comments;

  gentry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (widget));

  title = gtk_object_get_data (GTK_OBJECT (widget), Title);
  if (title && title[0] == '\0')
    title = NULL;

  history_id = gtk_object_get_data (GTK_OBJECT (widget), HistoryID);
  if (history_id && history_id[0] == '\0')
    history_id = NULL;

  if (data->create_widget)
    {
      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data, "  %s = gnome_file_entry_new (%s, ",
		  data->wname,
		  history_id ? source_make_string (history_id, FALSE) : "NULL");

      source_add (data, "%s);\n",
		  title ? source_make_string_full (title, data->use_gettext && translatable, context) : "NULL");
    }

  gb_widget_write_standard_source (widget, data);

  /* Note that GLADE_DEFAULT_MAX_HISTORY_SAVED is copied from gnome-entry.c */
  if (gnome_entry_get_max_saved (GNOME_ENTRY (gentry)) != GLADE_DEFAULT_MAX_HISTORY_SAVED)
    source_add (data, "  gnome_entry_set_max_saved (GNOME_ENTRY (gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (%s))), %i);\n",
		data->wname,
		gnome_entry_get_max_saved (GNOME_ENTRY (gentry)));

  if (gnome_file_entry_get_directory_entry (GNOME_FILE_ENTRY (widget)))
    source_add (data,
		"  gnome_file_entry_set_directory_entry (GNOME_FILE_ENTRY (%s), TRUE);\n",
		data->wname);
  
  if (gnome_file_entry_get_modal (GNOME_FILE_ENTRY (widget)))
    source_add (data,
		"  gnome_file_entry_set_modal (GNOME_FILE_ENTRY (%s), TRUE);\n",
		data->wname);

  g_object_get (G_OBJECT (widget),
		"use_filechooser", &use_filechooser,
		"filechooser_action", &action,
		NULL); 

  if (use_filechooser)
    {
      const gchar *action_symbol = GbActionSymbols[0];
      gint i;

      for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]);
	   i++)
	{
	  if (GbActionValues[i] == action)
	    action_symbol = GbActionSymbols[i];
	}

      source_add (data,
		  "  g_object_set (G_OBJECT (%s),\n"
		  "                \"use_filechooser\", TRUE,\n"
		  "                \"filechooser_action\", %s,\n"
		  "                NULL);\n",
		  data->wname, action_symbol);
    }


  /* We output the source code for the children here, since the code should
     not include calls to create the widgets. We need to specify that the
     names used are like: "GTK_COMBO (<combo-name>)->entry".
     We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  entry = GTK_COMBO (gentry)->entry;
  child_name = (char*) gtk_widget_get_name (entry);
  child_name = source_create_valid_identifier (child_name);
  source_add (data,
	      "  %s = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (%s));\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (entry, data);

  g_free (wname);
  data->write_children = FALSE;
}


static GtkWidget *
gb_gnome_file_entry_get_child (GtkWidget * widget,
			       const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildGnomeEntry))
    return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (widget));
  else
    return NULL;
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_file_entry_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_file_entry_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_fileentry_xpm;
  gbwidget.tooltip = _("Gnome File Entry");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_file_entry_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_file_entry_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_file_entry_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_file_entry_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_file_entry_write_source;
  gbwidget.gb_widget_get_child		= gb_gnome_file_entry_get_child;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_file_entry_create_popup_menu;
*/

  return &gbwidget;
}

