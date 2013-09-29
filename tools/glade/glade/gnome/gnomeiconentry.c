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
#include "../graphics/gnome-iconentry.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *HistoryID = "GnomeIconEntry|GnomeEntry::history_id";
static gchar *MaxSaved = "GnomeIconEntry|GnomeEntry::max_saved";
static gchar *Title = "GnomeIconEntry::browse_dialog_title";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeIconEntry, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_gnome_icon_entry_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gnome_icon_entry_new (NULL, NULL);

  /* Set MaxSaved to the default. */
  gtk_object_set_data (GTK_OBJECT (new_widget), MaxSaved,
		       GINT_TO_POINTER (GLADE_DEFAULT_MAX_HISTORY_SAVED));

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_icon_entry_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_string (Title, _("Title:"),
		       _("The title of the file selection dialog"));
  property_add_string (HistoryID, _("History ID:"),
		       _("The ID to save the history entries under"));
  property_add_int_range (MaxSaved, _("Max Saved:"),
			  _("The maximum number of history entries saved"),
			  0, 10000, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_icon_entry_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_translatable_string (data, Title, gtk_object_get_data (GTK_OBJECT (widget), Title));
  gb_widget_output_string (data, HistoryID, gtk_object_get_data (GTK_OBJECT (widget), HistoryID));
  gb_widget_output_int (data, MaxSaved, GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), MaxSaved)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_icon_entry_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *title, *history_id;
  gint max_saved;

  title = gb_widget_input_string (data, Title);
  if (data->apply)
    {
      gtk_object_set_data_full (GTK_OBJECT (widget), Title, g_strdup (title),
				title ? g_free : NULL);
      gnome_icon_entry_set_browse_dialog_title (GNOME_ICON_ENTRY (widget),
						title && title[0] ? title : "");
    }

  history_id = gb_widget_input_string (data, HistoryID);
  if (data->apply)
    gtk_object_set_data_full (GTK_OBJECT (widget), HistoryID,
			      g_strdup (history_id),
			      history_id ? g_free : NULL);

  max_saved = gb_widget_input_int (data, MaxSaved);
  if (data->apply)
    gtk_object_set_data (GTK_OBJECT (widget), MaxSaved,
			 GINT_TO_POINTER (max_saved));
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeIconEntry, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_icon_entry_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_icon_entry_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *title, *history_id;
  gint max_saved;
  gboolean translatable, context;
  gchar *comments;

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

      source_add (data, "  %s = gnome_icon_entry_new (%s, ",
		  data->wname,
		  history_id ? source_make_string (history_id, FALSE) : "NULL");

      source_add (data, "%s);\n",
		  title ? source_make_string_full (title, data->use_gettext && translatable, context) : "NULL");
    }
  gb_widget_write_standard_source (widget, data);

  /* Note that GLADE_DEFAULT_MAX_HISTORY_SAVED is copied from gnome-entry.c */
  max_saved = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						    MaxSaved));
  if (max_saved != GLADE_DEFAULT_MAX_HISTORY_SAVED)
    source_add (data, "  gnome_entry_set_max_saved (GNOME_ENTRY (gnome_icon_entry_gnome_entry (GNOME_ICON_ENTRY (%s))), %i);\n",
		data->wname, max_saved);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_icon_entry_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_icon_entry_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_iconentry_xpm;
  gbwidget.tooltip = _("Gnome Icon Entry");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_icon_entry_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_icon_entry_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_icon_entry_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_icon_entry_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_icon_entry_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_icon_entry_create_popup_menu;
*/

  return &gbwidget;
}

