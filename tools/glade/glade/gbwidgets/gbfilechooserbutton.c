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
#include "../graphics/filechooserbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Action = "GtkFileChooserButton::action";
static gchar *LocalOnly = "GtkFileChooserButton::local_only";
static gchar *ShowHidden = "GtkFileChooserButton::show_hidden";
static gchar *Confirm = "GtkFileChooserButton::do_overwrite_confirmation";
static gchar *Title = "GtkFileChooserButton::title";
static gchar *WidthChars = "GtkFileChooserButton::width_chars";


/* Note that GtkFileChooserButton doesn't support "Save" or "Create Folder". */
static const gchar *GbActionChoices[] =
{"Open", "Select Folder", NULL};
static const gint GbActionValues[] =
{
  GTK_FILE_CHOOSER_ACTION_OPEN,
  GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
};
static const gchar *GbActionSymbols[] =
{
  "GTK_FILE_CHOOSER_ACTION_OPEN",
  "GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER"
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
gb_file_chooser_button_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_file_chooser_button_new (NULL,
					    GTK_FILE_CHOOSER_ACTION_OPEN);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_file_chooser_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_string (Title, _("Title:"),
		     _("The window title of the file chooser dialog"));
  property_add_choice (Action, _("Action:"),
		       _("The type of file operation being performed"),
		       GbActionChoices);
  property_add_bool (LocalOnly, _("Local Only:"),
		     _("Whether the selected files should be limited to local files"));
  property_add_bool (ShowHidden, _("Show Hidden:"),
		     _("Whether the hidden files and folders should be displayed"));
  property_add_bool (Confirm, _("Confirm:"),
		     _("Whether a confirmation dialog will be displayed if a file will be overwritten"));
  property_add_int_range (WidthChars, _("Width in Chars:"),
			  _("The width of the button in characters"),
			  -1, 1000, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_file_chooser_button_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gchar *title;
  GtkFileChooserAction action;
  gboolean local_only, show_hidden, confirm;
  gint i, width_chars;

  g_object_get (widget,
		"title", &title,
		"action", &action,
		"local_only", &local_only,
		"show_hidden", &show_hidden,
		"do_overwrite_confirmation", &confirm,
		"width_chars", &width_chars,
		NULL);

  gb_widget_output_translatable_string (data, Title, title);
  g_free (title);

  for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]); i++)
    {
      if (GbActionValues[i] == action)
	gb_widget_output_choice (data, Action, i, GbActionSymbols[i]);
    }

  gb_widget_output_bool (data, LocalOnly, local_only);
  gb_widget_output_bool (data, ShowHidden, show_hidden);
  gb_widget_output_bool (data, Confirm, confirm);
  gb_widget_output_int (data, WidthChars, width_chars);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_file_chooser_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *title, *action;
  gboolean local_only, show_hidden, confirm;
  gint i, width_chars;

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    g_object_set (widget, "title", title, NULL);

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

  show_hidden = gb_widget_input_bool (data, ShowHidden);
  if (data->apply)
    g_object_set (widget, "show_hidden", show_hidden, NULL);

  confirm = gb_widget_input_bool (data, Confirm);
  if (data->apply)
    g_object_set (widget, "do_overwrite_confirmation", confirm, NULL);

  width_chars = gb_widget_input_int (data, WidthChars);
  if (data->apply)
    g_object_set (widget, "width_chars", width_chars, NULL);
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
gb_file_chooser_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *title;
  GtkFileChooserAction action;
  gboolean local_only, show_hidden, confirm;
  gint width_chars;
  const gchar *action_symbol = GbActionSymbols[0];
  gint i;

  g_object_get (widget,
		"title", &title,
		"action", &action,
		"local_only", &local_only,
		"show_hidden", &show_hidden,
		"do_overwrite_confirmation", &confirm,
		"width_chars", &width_chars,
		NULL);

  for (i = 0; i < sizeof (GbActionValues) / sizeof (GbActionValues[0]); i++)
    {
      if (GbActionValues[i] == action)
	action_symbol = GbActionSymbols[i];
    }

  if (data->create_widget)
    {
      gboolean translatable, context;
      gchar *comments;

      glade_util_get_translation_properties (widget, Title, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      source_add (data,
		  "  %s = gtk_file_chooser_button_new (%s, %s);\n",
		  data->wname,
		  source_make_string_full (title, data->use_gettext && translatable, context),
		  action_symbol);
    }

  gb_widget_write_standard_source (widget, data);

  if (!local_only || show_hidden || width_chars != -1 || confirm)
    {
      source_add (data,   "  g_object_set (%s,\n", data->wname);

      if (!local_only)
	source_add (data, "                \"local-only\", FALSE,\n");

      if (show_hidden)
	source_add (data, "                \"show-hidden\", TRUE,\n");

      if (confirm)
	source_add (data, "                \"confirm\", TRUE,\n");

      if (width_chars != -1)
	source_add (data, "                \"width-chars\", %i,\n",
		    width_chars);

      source_add (data,   "                NULL);\n");
    }

  g_free (title);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_file_chooser_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_file_chooser_button_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = filechooserbutton_xpm;
  gbwidget.tooltip = _("File Chooser Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_file_chooser_button_new;
  gbwidget.gb_widget_create_properties	= gb_file_chooser_button_create_properties;
  gbwidget.gb_widget_get_properties	= gb_file_chooser_button_get_properties;
  gbwidget.gb_widget_set_properties	= gb_file_chooser_button_set_properties;
  gbwidget.gb_widget_write_source	= gb_file_chooser_button_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_file_chooser_widget_create_popup_menu;
*/

  return &gbwidget;
}
