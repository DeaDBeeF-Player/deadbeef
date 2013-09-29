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
#include "../graphics/gnome-entry.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *HistoryID = "GnomeEntry::history_id";
static gchar *MaxSaved = "GnomeEntry::max_saved";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeEntry, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_gnome_entry_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gnome_entry_new (NULL);

  gb_widget_create_from (GTK_COMBO (new_widget)->entry,
			 data->action == GB_CREATING ? "combo-entry" : NULL);
  gb_widget_set_child_name (GTK_COMBO (new_widget)->entry, GladeChildGnomeEntry);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_entry_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
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
gb_gnome_entry_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_string (data, HistoryID, gtk_object_get_data (GTK_OBJECT (widget), HistoryID));
  gb_widget_output_int (data, MaxSaved, gnome_entry_get_max_saved (GNOME_ENTRY (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_entry_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *history_id;
  gint max_saved;

  history_id = gb_widget_input_string (data, HistoryID);
  if (data->apply)
    gtk_object_set_data_full (GTK_OBJECT (widget), HistoryID,
			      g_strdup (history_id),
			      history_id ? g_free : NULL);

  max_saved = gb_widget_input_int (data, MaxSaved);
  if (data->apply)
    gnome_entry_set_max_saved (GNOME_ENTRY (widget), max_saved);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeEntry, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_entry_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_entry_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *wname, *child_name;

  if (data->create_widget)
    {
      gchar *history_id;

      history_id = gtk_object_get_data (GTK_OBJECT (widget), HistoryID);
      if (history_id && history_id[0])
	source_add (data, "  %s = gnome_entry_new (%s);\n",
		    data->wname, source_make_string (history_id, FALSE));
      else
	source_add (data, "  %s = gnome_entry_new (NULL);\n",
		    data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  /* Note that GLADE_DEFAULT_MAX_HISTORY_SAVED is copied from gnome-entry.c */
  if (gnome_entry_get_max_saved (GNOME_ENTRY (widget)) != GLADE_DEFAULT_MAX_HISTORY_SAVED)
    source_add (data, "  gnome_entry_set_max_saved (GNOME_ENTRY (%s), %i);\n",
		data->wname, gnome_entry_get_max_saved (GNOME_ENTRY (widget)));


  /* We output the source code for the children here, since the code should
     not include calls to create the widgets. We need to specify that the
     names used are like: "GTK_COMBO (<combo-name>)->entry".
     We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  child_name = (char*) gtk_widget_get_name (GTK_COMBO (widget)->entry);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = gnome_entry_gtk_entry (GNOME_ENTRY (%s));\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GTK_COMBO (widget)->entry, data);

  g_free (wname);
  data->write_children = FALSE;
}


static GtkWidget *
gb_gnome_entry_get_child (GtkWidget * widget,
			  const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildGnomeEntry))
    return GTK_COMBO (widget)->entry;
  else
    return NULL;
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_entry_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_entry_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_entry_xpm;
  gbwidget.tooltip = _("Gnome Entry");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_entry_new;
  gbwidget.gb_widget_create_properties	= gb_gnome_entry_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_entry_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_entry_set_properties;
  gbwidget.gb_widget_get_child		= gb_gnome_entry_get_child;
  gbwidget.gb_widget_write_source	= gb_gnome_entry_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_entry_create_popup_menu;
*/

  return &gbwidget;
}

