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

#include <gtk/gtkalignment.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/alignment.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *XAlign = "GtkAlignment::xalign";
static gchar *YAlign = "GtkAlignment::yalign";
static gchar *XScale = "GtkAlignment::xscale";
static gchar *YScale = "GtkAlignment::yscale";

static gchar *TopPadding = "GtkAlignment::top_padding";
static gchar *BottomPadding = "GtkAlignment::bottom_padding";
static gchar *LeftPadding = "GtkAlignment::left_padding";
static gchar *RightPadding = "GtkAlignment::right_padding";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkAlignment, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_alignment_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  if (data->action != GB_LOADING)
    gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_alignment_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_float_range (XAlign, _("X Align:"),
			    _("The horizontal alignment of the child widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (YAlign, _("Y Align:"),
			    _("The vertical alignment of the child widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (XScale, _("X Scale:"),
			    _("The horizontal scale of the child widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (YScale, _("Y Scale:"),
			    _("The vertical scale of the child widget"),
			    0, 1, 0.01, 0.1, 0.01, 2);

  property_add_int_range (TopPadding,
			  _("Top Padding:"),
			  _("Space to put above the child widget"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (BottomPadding,
			  _("Bottom Padding:"),
			  _("Space to put below the child widget"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (LeftPadding,
			  _("Left Padding:"),
			  _("Space to put to the left of the child widget"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (RightPadding,
			  _("Right Padding:"),
			  _("Space to put to the right of the child widget"),
			  0, 1000, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_alignment_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  guint padding_top, padding_bottom, padding_left, padding_right;

  gb_widget_output_float (data, XAlign, GTK_ALIGNMENT (widget)->xalign);
  gb_widget_output_float (data, YAlign, GTK_ALIGNMENT (widget)->yalign);
  gb_widget_output_float (data, XScale, GTK_ALIGNMENT (widget)->xscale);
  gb_widget_output_float (data, YScale, GTK_ALIGNMENT (widget)->yscale);

  gtk_alignment_get_padding (GTK_ALIGNMENT (widget),
			     &padding_top, &padding_bottom,
			     &padding_left, &padding_right);

  gb_widget_output_int (data, TopPadding, padding_top);
  gb_widget_output_int (data, BottomPadding, padding_bottom);
  gb_widget_output_int (data, LeftPadding, padding_left);
  gb_widget_output_int (data, RightPadding, padding_right);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_alignment_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gfloat xalign, yalign, xscale, yscale;
  guint padding_top, padding_bottom, padding_left, padding_right;
  gboolean set_alignment = FALSE, set_padding = FALSE;

  xalign = gb_widget_input_float (data, XAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    xalign = GTK_ALIGNMENT (widget)->xalign;

  yalign = gb_widget_input_float (data, YAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    yalign = GTK_ALIGNMENT (widget)->yalign;

  xscale = gb_widget_input_float (data, XScale);
  if (data->apply)
    set_alignment = TRUE;
  else
    xscale = GTK_ALIGNMENT (widget)->xscale;

  yscale = gb_widget_input_float (data, YScale);
  if (data->apply)
    set_alignment = TRUE;
  else
    yscale = GTK_ALIGNMENT (widget)->yscale;

  if (set_alignment)
    gtk_alignment_set (GTK_ALIGNMENT (widget), xalign, yalign, xscale, yscale);

  gtk_alignment_get_padding (GTK_ALIGNMENT (widget),
			     &padding_top, &padding_bottom,
			     &padding_left, &padding_right);

  padding_top = gb_widget_input_int (data, TopPadding);
  if (data->apply)
    set_padding = TRUE;

  padding_bottom = gb_widget_input_int (data, BottomPadding);
  if (data->apply)
    set_padding = TRUE;

  padding_left = gb_widget_input_int (data, LeftPadding);
  if (data->apply)
    set_padding = TRUE;

  padding_right = gb_widget_input_int (data, RightPadding);
  if (data->apply)
    set_padding = TRUE;

  if (set_padding)
    gtk_alignment_set_padding (GTK_ALIGNMENT (widget),
			       padding_top, padding_bottom,
			       padding_left, padding_right);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkAlignment, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_alignment_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */

static void
gb_alignment_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  guint padding_top, padding_bottom, padding_left, padding_right;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_alignment_new (%g, %g, %g, %g);\n",
		  data->wname,
	     GTK_ALIGNMENT (widget)->xalign, GTK_ALIGNMENT (widget)->yalign,
	    GTK_ALIGNMENT (widget)->xscale, GTK_ALIGNMENT (widget)->yscale);
    }
  gb_widget_write_standard_source (widget, data);

  gtk_alignment_get_padding (GTK_ALIGNMENT (widget),
			     &padding_top, &padding_bottom,
			     &padding_left, &padding_right);

  if (padding_top != 0 || padding_bottom != 0
      || padding_left != 0 || padding_right != 0)
    {
      source_add (data,
		  "  gtk_alignment_set_padding (GTK_ALIGNMENT (%s), %i, %i, %i, %i);\n",
		  data->wname,
		  padding_top, padding_bottom, padding_left, padding_right);

    }
}

/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_alignment_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_alignment_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = alignment_xpm;
  gbwidget.tooltip = _("Alignment");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_alignment_new;
  gbwidget.gb_widget_create_properties = gb_alignment_create_properties;
  gbwidget.gb_widget_get_properties = gb_alignment_get_properties;
  gbwidget.gb_widget_set_properties = gb_alignment_set_properties;
/*
   gbwidget.gb_widget_create_popup_menu = gb_alignment_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_alignment_write_source;

  return &gbwidget;
}
