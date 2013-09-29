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

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/layout.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *AreaWidth = "GtkLayout::width";
static gchar *AreaHeight = "GtkLayout::height";

/* Child properties. */
const gchar *GladeLayoutChildX = "GtkLayout::x";
const gchar *GladeLayoutChildY = "GtkLayout::y";



/* The default step increment for new layouts. */
#define GLADE_DEFAULT_STEP_INCREMENT 10

/* We only use the step increment property since the other are set
   automatically by the GtkLayout, or aren't relevant. */
static const gchar *HValues[] = {
  NULL, NULL, NULL, "GtkLayout::hstep", NULL, NULL,
};

static const gchar *VValues[] = {
  NULL, NULL, NULL, "GtkLayout::vstep", NULL, NULL,
};

static void gb_layout_adjustment_changed (GtkAdjustment *adjustment,
					  GtkWidget     *widget);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkLayout, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_layout_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_layout_new (NULL, NULL);
  gtk_layout_set_size (GTK_LAYOUT (new_widget), 400, 400);

  return new_widget;
}


static void
gb_layout_adjustment_changed (GtkAdjustment *adjustment,
			      GtkWidget     *widget)
{
  /* We check that this is a widget in the interface being created rather
     than part of Glade's interface. */
  if (GB_IS_GB_WIDGET (widget))
    gtk_widget_queue_clear (widget);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_layout_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_int_range (AreaWidth, _("Area Width:"),
			  _("The width of the layout area"),
			  1, 1000, 1, 10, 1);
  property_add_int_range (AreaHeight, _("Area Height:"),
			  _("The height of the layout area"),
			  1, 1000, 1, 10, 1);

  property_add_adjustment (HValues, GB_ADJUST_H_LABELS);
  property_add_adjustment (VValues, GB_ADJUST_V_LABELS);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_layout_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_int (data, AreaWidth, GTK_LAYOUT (widget)->width);
  gb_widget_output_int (data, AreaHeight, GTK_LAYOUT (widget)->height);

  gb_widget_output_adjustment (data, HValues,
			       GTK_LAYOUT (widget)->hadjustment,
			       "hadjustment");
  gb_widget_output_adjustment (data, VValues,
			       GTK_LAYOUT (widget)->vadjustment,
			       "vadjustment");
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_layout_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gint width, height;
  gboolean set_size = FALSE;

  width = gb_widget_input_int (data, AreaWidth);
  if (data->apply)
    set_size = TRUE;
  else
    width = GTK_LAYOUT (widget)->width;

  height = gb_widget_input_int (data, AreaHeight);
  if (data->apply)
    set_size = TRUE;
  else
    height = GTK_LAYOUT (widget)->height;

  if (set_size)
    gtk_layout_set_size (GTK_LAYOUT (widget), width, height);

  gb_widget_input_adjustment (data, HValues, GTK_LAYOUT (widget)->hadjustment,
			      "hadjustment");
  gb_widget_input_adjustment (data, VValues, GTK_LAYOUT (widget)->vadjustment,
			      "vadjustment");
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkLayout, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_layout_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/


void
gb_layout_add_child (GtkWidget *widget, GtkWidget *child,
		     GbWidgetSetArgData *data)
{
  gtk_layout_put (GTK_LAYOUT (widget), child, 0, 0);
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_layout_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_layout_new (NULL, NULL);\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  source_add (data, "  gtk_layout_set_size (GTK_LAYOUT (%s), %i, %i);\n",
	      data->wname,
	      GTK_LAYOUT (widget)->width,
	      GTK_LAYOUT (widget)->height);

  source_add (data,
	      "  GTK_ADJUSTMENT (GTK_LAYOUT (%s)->hadjustment)->step_increment = %g;\n",
	      data->wname,
	      GTK_ADJUSTMENT (GTK_LAYOUT (widget)->hadjustment)->step_increment);
  source_add (data,
	      "  GTK_ADJUSTMENT (GTK_LAYOUT (%s)->vadjustment)->step_increment = %g;\n",
	      data->wname,
	      GTK_ADJUSTMENT (GTK_LAYOUT (widget)->vadjustment)->step_increment);
}


/*
 * Creates the child packing properties for children of this widget.
 */
void
gb_layout_create_child_properties (GtkWidget * widget,
				   GbWidgetCreateChildArgData * data)
{
  property_add_int_range (GladeLayoutChildX, _("X:"),
			  _("The X coordinate of the widget in the GtkLayout"),
			  0, 10000, 1, 10, 1);
  property_add_int_range (GladeLayoutChildY, _("Y:"),
			  _("The Y coordinate of the widget in the GtkLayout"),
			  0, 10000, 1, 10, 1);
}


/* Shows or saves the child properties of a child of a layout. */
void
gb_layout_get_child_properties (GtkWidget *widget, GtkWidget *child,
				GbWidgetGetArgData *data)
{
  gint x, y;

  if (data->action == GB_SAVING)
    save_start_tag (data, "packing");

  gtk_container_child_get (GTK_CONTAINER (widget), child,
			   "x", &x,
			   "y", &y,
			   NULL);

  gb_widget_output_int (data, GladeLayoutChildX, x);
  gb_widget_output_int (data, GladeLayoutChildY, y);

  if (data->action == GB_SAVING)
    save_end_tag (data, "packing");
}


/* Applies or loads the child properties of a child of a layout. */
void
gb_layout_set_child_properties (GtkWidget *widget, GtkWidget *child,
				GbWidgetSetArgData *data)
{
  gint x, y;

  x = gb_widget_input_int (data, GladeLayoutChildX);
  if (data->apply)
    gtk_container_child_set (GTK_CONTAINER (widget), child,
			     "x", x,
			     NULL);

  y = gb_widget_input_int (data, GladeLayoutChildY);
  if (data->apply)
    gtk_container_child_set (GTK_CONTAINER (widget), child,
			     "y", y,
			     NULL);
}


/* Outputs source to add a child widget to a layout. */
static void
gb_layout_write_add_child_source (GtkWidget * parent,
				  const gchar *parent_name,
				  GtkWidget *child,
				  GbWidgetWriteSourceData * data)
{
  gint x, y;

  gtk_container_child_get (GTK_CONTAINER (parent), child,
			   "x", &x,
			   "y", &y,
			   NULL);
  source_add (data,
	      "  gtk_layout_put (GTK_LAYOUT (%s), %s, %i, %i);\n",
	      parent_name, data->wname, x, y);
}


static gboolean
gb_layout_emission_hook (GSignalInvocationHint  *ihint,
			 guint			n_param_values,
			 const GValue	       *param_values,
			 gpointer		data)
{
  GtkObject *object, *hadjustment, *vadjustment;
  GtkObject *old_hadjustment, *old_vadjustment;

  object = g_value_get_object (param_values);
  g_return_val_if_fail (GTK_IS_LAYOUT (object), FALSE);

  hadjustment = g_value_get_object (param_values + 1);
  vadjustment = g_value_get_object (param_values + 2);

  old_hadjustment = gtk_object_get_data (object, "scrollhadjustment");
  if (hadjustment != old_hadjustment)
    {
      gtk_object_set_data (object, "scrollhadjustment", hadjustment);

      if (hadjustment)
	{
	  gtk_signal_connect (hadjustment, "value_changed",
			      (GtkSignalFunc) gb_layout_adjustment_changed,
			      object);

	  GTK_ADJUSTMENT (hadjustment)->step_increment = GLADE_DEFAULT_STEP_INCREMENT;
	  if (property_get_widget () == GTK_WIDGET (object))
	    {
	      property_set_auto_apply (FALSE);
	      property_set_float (HValues[3], GLADE_DEFAULT_STEP_INCREMENT);
	      property_set_auto_apply (TRUE);
	    }
	}
    }

  old_vadjustment = gtk_object_get_data (object, "scrollvadjustment");
  if (vadjustment != old_vadjustment)
    {
      gtk_object_set_data (object, "scrollvadjustment", vadjustment);

      if (vadjustment)
	{
	  gtk_signal_connect (vadjustment, "value_changed",
			      (GtkSignalFunc) gb_layout_adjustment_changed,
			      object);

	  GTK_ADJUSTMENT (vadjustment)->step_increment = GLADE_DEFAULT_STEP_INCREMENT;
	  if (property_get_widget () == GTK_WIDGET (object))
	    {
	      property_set_auto_apply (FALSE);
	      property_set_float (VValues[3], GLADE_DEFAULT_STEP_INCREMENT);
	      property_set_auto_apply (TRUE);
	    }
	}
    }

  return TRUE;
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_layout_init ()
{
  GtkWidgetClass *klass;

  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_layout_get_type();

  /* Add a signal emission hook so we can connect signal handlers to the
     scrollbar adjustments to redraw the layout when necessary. This will also
     work for subclasses of GtkLayout. */
  klass = gtk_type_class (gtk_layout_get_type ());
  g_signal_add_emission_hook (klass->set_scroll_adjustments_signal, 0,
			      gb_layout_emission_hook, NULL, NULL);

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = layout_xpm;
  gbwidget.tooltip = _("Layout");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_layout_new;
  gbwidget.gb_widget_create_properties	= gb_layout_create_properties;
  gbwidget.gb_widget_get_properties	= gb_layout_get_properties;
  gbwidget.gb_widget_set_properties	= gb_layout_set_properties;
  gbwidget.gb_widget_write_source	= gb_layout_write_source;
  gbwidget.gb_widget_create_child_properties = gb_layout_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_layout_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_layout_set_child_properties;
  gbwidget.gb_widget_write_add_child_source = gb_layout_write_add_child_source;
  gbwidget.gb_widget_add_child		= gb_layout_add_child;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_layout_create_popup_menu;
*/

  return &gbwidget;
}

