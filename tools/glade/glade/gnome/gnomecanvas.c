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
#include "../graphics/gnome-canvas.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *AntiAliased = "GnomeCanvas::aa";
static gchar *ScrollX1 = "GnomeCanvas::scroll_x1";
static gchar *ScrollY1 = "GnomeCanvas::scroll_y1";
static gchar *ScrollX2 = "GnomeCanvas::scroll_x2";
static gchar *ScrollY2 = "GnomeCanvas::scroll_y2";
static gchar *PixelsPerUnit = "GnomeCanvas::pixels_per_unit";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeCanvas, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
/*
static GtkWidget*
gb_gnome_canvas_new (GbWidgetNewData *data)
{

}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_canvas_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (AntiAliased, _("Anti-Aliased:"),
		     _("If the canvas is anti-aliased, to smooth the edges of text and graphics"));
  property_add_float (ScrollX1, _("X1:"), _("The minimum x coordinate"));
  property_add_float (ScrollY1, _("Y1:"), _("The minimum y coordinate"));
  property_add_float (ScrollX2, _("X2:"), _("The maximum x coordinate"));
  property_add_float (ScrollY2, _("Y2:"), _("The maximum y coordinate"));

  property_add_float (PixelsPerUnit, _("Pixels Per Unit:"),
		      _("The number of pixels corresponding to one unit"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_canvas_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_bool (data, AntiAliased,
			 GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), AntiAliased)));

  gb_widget_output_float (data, ScrollX1, GNOME_CANVAS (widget)->scroll_x1);
  gb_widget_output_float (data, ScrollY1, GNOME_CANVAS (widget)->scroll_y1);
  gb_widget_output_float (data, ScrollX2, GNOME_CANVAS (widget)->scroll_x2);
  gb_widget_output_float (data, ScrollY2, GNOME_CANVAS (widget)->scroll_y2);

  gb_widget_output_float (data, PixelsPerUnit,
			  GNOME_CANVAS (widget)->pixels_per_unit);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_canvas_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean antialiased;
  gfloat value;
  gfloat scroll_x1, scroll_y1, scroll_x2, scroll_y2;
  gfloat old_scroll_x1, old_scroll_y1, old_scroll_x2, old_scroll_y2;
  gfloat pixels_per_unit;

  antialiased = gb_widget_input_bool (data, AntiAliased);
  if (data->apply)
    gtk_object_set_data (GTK_OBJECT (widget), AntiAliased,
			 GINT_TO_POINTER (antialiased));

  /* Also check for the old name we used for this. */
  if (data->action == GB_LOADING)
    {
      antialiased = gb_widget_input_bool (data, "anti_aliased");
      if (data->apply)
	gtk_object_set_data (GTK_OBJECT (widget), AntiAliased,
			     GINT_TO_POINTER (antialiased));
    }

  scroll_x1 = old_scroll_x1 = GNOME_CANVAS (widget)->scroll_x1;
  scroll_y1 = old_scroll_y1 = GNOME_CANVAS (widget)->scroll_y1;
  scroll_x2 = old_scroll_x2 = GNOME_CANVAS (widget)->scroll_x2;
  scroll_y2 = old_scroll_y2 = GNOME_CANVAS (widget)->scroll_y2;

  value = gb_widget_input_float (data, ScrollX1);
  if (data->apply)
    scroll_x1 = value;
  value = gb_widget_input_float (data, ScrollY1);
  if (data->apply)
    scroll_y1 = value;
  value = gb_widget_input_float (data, ScrollX2);
  if (data->apply)
    scroll_x2 = value;
  value = gb_widget_input_float (data, ScrollY2);
  if (data->apply)
    scroll_y2 = value;

  if (scroll_x1 != old_scroll_x1 || scroll_y1 != old_scroll_y1
      || scroll_x2 != old_scroll_x2 || scroll_y2 != old_scroll_y2)
    {
      gnome_canvas_set_scroll_region (GNOME_CANVAS (widget),
				      scroll_x1, scroll_y1,
				      scroll_x2, scroll_y2);
    }

  pixels_per_unit = gb_widget_input_float (data, PixelsPerUnit);
  if (data->apply)
    gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (widget), pixels_per_unit);
}


/*
 * Sets the properties for the child widget specific to this type of
 * parent widget.
 */
/*
static void
gb_gnome_canvas_set_child_props (GtkWidget * widget, GtkWidget * child, GbWidgetSetArgData * data)
{

}
*/


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeCanvas, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_canvas_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_canvas_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      if (gtk_object_get_data (GTK_OBJECT (widget), AntiAliased))
	{
	  source_add (data,
		      "  %s = gnome_canvas_new_aa ();\n",
		      data->wname);
	}
      else
	{
	  source_add (data,
		      "  %s = gnome_canvas_new ();\n",
		      data->wname);
	}
    }

  gb_widget_write_standard_source (widget, data);

  source_add (data,
	      "  gnome_canvas_set_scroll_region (GNOME_CANVAS (%s), %g, %g, %g, %g);\n",
	      data->wname,
	      GNOME_CANVAS (widget)->scroll_x1,
	      GNOME_CANVAS (widget)->scroll_y1,
	      GNOME_CANVAS (widget)->scroll_x2,
	      GNOME_CANVAS (widget)->scroll_y2);

  if (GNOME_CANVAS (widget)->pixels_per_unit != 1.0)
    source_add (data,
		"  gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (%s), %g);\n",
	      data->wname,
	      GNOME_CANVAS (widget)->pixels_per_unit);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_canvas_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_canvas_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_canvas_xpm;
  gbwidget.tooltip = _("GnomeCanvas");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties	= gb_gnome_canvas_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_canvas_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_canvas_set_properties;
  gbwidget.gb_widget_write_source	= gb_gnome_canvas_write_source;
/*
  gbwidget.gb_widget_new		= gb_gnome_canvas_new;
  gbwidget.gb_widget_set_child_props	= gb_gnome_canvas_set_child_props;
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_canvas_create_popup_menu;
*/

  return &gbwidget;
}

