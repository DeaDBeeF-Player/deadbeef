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
#include <libgnomedb/gnome-db-connection-properties.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-db-connection-properties.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDbEditor, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */

GtkWidget*
gb_gnome_db_connection_properties_new(GbWidgetNewData *data)
{
  return (GtkWidget *) gnome_db_connection_properties_new(NULL);
}

/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_db_connection_properties_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{

  if (data->create_widget)
    {
      source_add (data, "  %s = gnome_db_connection_properties_new (NULL);\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

}

/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_gnome_db_connection_properties_init ()
{
  /* Initialise the GTK type */
  volatile GType type;
  type = gnome_db_connection_properties_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_db_connection_properties_xpm;
  gbwidget.tooltip = _("GnomeDbConnectionProperties");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new               = gb_gnome_db_connection_properties_new;
  gbwidget.gb_widget_write_source = gb_gnome_db_connection_properties_write_source;

  return &gbwidget;
}
