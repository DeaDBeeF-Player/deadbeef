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
#include "../graphics/cellview.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *BackgroundColor = "GtkCellView::background";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkCellView, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
/*
static GtkWidget*
gb_cell_view_new (GbWidgetNewData *data)
{

}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_cell_view_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_color (BackgroundColor, _("Back. Color:"),
		      _("The background color"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_cell_view_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gboolean background_set;
  GdkColor *background_color;

  g_object_get (G_OBJECT (widget),
		"background_set", &background_set,
		"background_gdk", &background_color,
		NULL);

  /* If the background color isn't currently set, set it to black just to
     make sure we don't get a random color. */
  if (!background_set) {
    background_color->red = 0;
    background_color->green = 0;
    background_color->blue = 0;
  }

  if (data->action == GB_SHOWING || background_set)
    gb_widget_output_color (data, BackgroundColor, background_color);

  gdk_color_free (background_color);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_cell_view_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GdkColor *color;

  color = gb_widget_input_color (data, BackgroundColor);
  if (data->apply)
    {
      gtk_cell_view_set_background_color (GTK_CELL_VIEW (widget), color);
      gtk_widget_queue_draw (widget);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkCellView, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_cell_view_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_cell_view_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gboolean background_set;
  GdkColor *background_color;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_cell_view_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  g_object_get (G_OBJECT (widget),
		"background_set", &background_set,
		"background_gdk", &background_color,
		NULL);

  if (background_set)
    {
      source_add_decl (data,
		       "  GdkColor %s_bg_color = { 0, %i, %i, %i };\n",
		       data->real_wname,
		       background_color->red, background_color->green,
		       background_color->blue);

      source_add (data,
		  "  gtk_cell_view_set_background_color (GTK_CELL_VIEW (%s), &%s_bg_color);\n",
		  data->wname, data->wname);
    }

  gdk_color_free (background_color);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_cell_view_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_cell_view_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = cellview_xpm;
  gbwidget.tooltip = _("Cell View");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties	= gb_cell_view_create_properties;
  gbwidget.gb_widget_get_properties	= gb_cell_view_get_properties;
  gbwidget.gb_widget_set_properties	= gb_cell_view_set_properties;
  gbwidget.gb_widget_write_source	= gb_cell_view_write_source;
/*
  gbwidget.gb_widget_new		= gb_cell_view_new;
  gbwidget.gb_widget_create_popup_menu	= gb_cell_view_create_popup_menu;
*/

  return &gbwidget;
}

