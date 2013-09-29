
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

#include <gtk/gtkvpaned.h>
#include <gtk/gtktogglebutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/vpaned.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Position = "VPaned|GtkPaned::position";

static void on_toggle_position (GtkWidget * widget, gpointer value);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkVPaned, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_vpaned_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_vpaned_new ();
  if (data->action != GB_LOADING)
    {
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
    }
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_vpaned_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_optional_int_range (Position, _("Position:"),
				   _("The position of the divider"),
				   1, 1000, 1, 10, 1,
				   on_toggle_position);
}


static void
on_toggle_position (GtkWidget * button, gpointer value)
{
  GtkWidget *widget;
  gboolean value_set;
  gint position;

  widget = property_get_widget ();
  if (widget == NULL)
    return;

  value_set = GTK_TOGGLE_BUTTON (button)->active ? TRUE : FALSE;
  gtk_widget_set_sensitive (GTK_WIDGET (value), value_set);

  position = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						   Position));
  gtk_paned_set_position (GTK_PANED (widget),
			  value_set ? position : -1);
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_vpaned_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint position;

  position = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						   Position));
  gb_widget_output_optional_int (data, Position, position,
				 GTK_PANED (widget)->position_set);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_vpaned_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gint position;

  position = gb_widget_input_int (data, Position);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), Position,
			   GINT_TO_POINTER (position));
      gtk_paned_set_position (GTK_PANED (widget), position);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkVPaned, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_vpaned_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_vpaned_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint position;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_vpaned_new ();\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  if (GTK_PANED (widget)->position_set)
    {
      position = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						       Position));
      source_add (data, "  gtk_paned_set_position (GTK_PANED (%s), %d);\n",
		  data->wname, position);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_vpaned_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_vpaned_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = vpaned_xpm;
  gbwidget.tooltip = _("Vertical Panes");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_vpaned_new;
  gbwidget.gb_widget_create_properties = gb_vpaned_create_properties;
  gbwidget.gb_widget_get_properties = gb_vpaned_get_properties;
  gbwidget.gb_widget_set_properties = gb_vpaned_set_properties;
  gbwidget.gb_widget_create_child_properties = gb_paned_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_paned_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_paned_set_child_properties;
/*
   gbwidget.gb_widget_create_popup_menu = gb_vpaned_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_vpaned_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_paned_write_add_child_source;

  return &gbwidget;
}
