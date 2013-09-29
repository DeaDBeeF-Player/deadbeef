/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999-2002  Damon Chaplin
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

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/filechooserwidget.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Action = "GtkFileChooserWidget::action";
static gchar *LocalOnly = "GtkFileChooserWidget::local_only";
static gchar *SelectMultiple = "GtkFileChooserWidget::select_multiple";
static gchar *ShowHidden = "GtkFileChooserWidget::show_hidden";


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
 * Creates a new GtkWidget of class GtkFileChooser, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_file_chooser_widget_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_file_chooser_widget_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Action, _("Action:"),
		       _("The type of file operation being performed"),
		       GbActionChoices);
  property_add_bool (LocalOnly, _("Local Only:"),
		     _("Whether the selected files should be limited to local files"));
  property_add_bool (SelectMultiple, _("Select Multiple:"),
		     _("Whether to allow multiple files to be selected"));
  property_add_bool (ShowHidden, _("Show Hidden:"),
		     _("Whether the hidden files and folders should be displayed"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_file_chooser_widget_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GtkFileChooserAction action;
  gboolean local_only, select_multiple, show_hidden;
  gint i;

  g_object_get (widget,
		"action", &action,
		"local_only", &local_only,
		"select_multiple", &select_multiple,
		"show_hidden", &show_hidden,
		NULL);

  for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]); i++)
    {
      if (GbActionValues[i] == action)
	gb_widget_output_choice (data, Action, i, GbActionSymbols[i]);
    }

  gb_widget_output_bool (data, LocalOnly, local_only);
  gb_widget_output_bool (data, SelectMultiple, select_multiple);
  gb_widget_output_bool (data, ShowHidden, show_hidden);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_file_chooser_widget_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *action;
  gboolean local_only, select_multiple, show_hidden;
  gint i;

  action = gb_widget_input_choice (data, Action);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]);
	   i++)
	{
	  if (!strcmp (action, GbActionChoices[i])
	      || !strcmp (action, GbActionSymbols[i]))
	    {
	      g_object_set (widget, "action", GbActionValues[i], NULL);
	      break;
	    }
	}
    }

  local_only = gb_widget_input_bool (data, LocalOnly);
  if (data->apply)
    g_object_set (widget, "local_only", local_only, NULL);

  select_multiple = gb_widget_input_bool (data, SelectMultiple);
  if (data->apply)
    g_object_set (widget, "select_multiple", select_multiple, NULL);

  show_hidden = gb_widget_input_bool (data, ShowHidden);
  if (data->apply)
    g_object_set (widget, "show_hidden", show_hidden, NULL);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkFileChooser, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_file_chooser_widget_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_file_chooser_widget_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkFileChooserAction action;
  gboolean local_only, select_multiple, show_hidden;

  g_object_get (widget,
		"action", &action,
		"local_only", &local_only,
		"select_multiple", &select_multiple,
		"show_hidden", &show_hidden,
		NULL);

  if (data->create_widget)
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
		  "  %s = gtk_file_chooser_widget_new (%s);\n",
		  data->wname,
		  action_symbol);
    }

  gb_widget_write_standard_source (widget, data);

  if (!local_only || select_multiple || show_hidden)
    {
      source_add (data,   "  g_object_set (%s,\n", data->wname);

      if (!local_only)
	source_add (data, "                \"local-only\", FALSE,\n");

      if (select_multiple)
	source_add (data, "                \"select-multiple\", TRUE,\n");

      if (show_hidden)
	source_add (data, "                \"show-hidden\", TRUE,\n");

      source_add (data,   "                NULL);\n");
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_file_chooser_widget_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_file_chooser_widget_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = filechooserwidget_xpm;
  gbwidget.tooltip = _("File Chooser");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_file_chooser_widget_new;
  gbwidget.gb_widget_create_properties	= gb_file_chooser_widget_create_properties;
  gbwidget.gb_widget_get_properties	= gb_file_chooser_widget_get_properties;
  gbwidget.gb_widget_set_properties	= gb_file_chooser_widget_set_properties;
  gbwidget.gb_widget_write_source	= gb_file_chooser_widget_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_file_chooser_widget_create_popup_menu;
*/

  return &gbwidget;
}

