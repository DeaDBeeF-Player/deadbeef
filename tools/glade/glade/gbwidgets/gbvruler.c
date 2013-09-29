
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

#include <gtk/gtkvruler.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/vruler.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Metric = "VRuler|GtkRuler::metric";
static gchar *Lower = "VRuler|GtkRuler::lower";
static gchar *Upper = "VRuler|GtkRuler::upper";
static gchar *Pos = "VRuler|GtkRuler::position";
static gchar *Max = "VRuler|GtkRuler::max_size";

static const gchar *GbMetricChoices[] =
{"Pixels", "Inches", "Centimeters", NULL};
static const gint GbMetricValues[] =
{
  GTK_PIXELS,
  GTK_INCHES,
  GTK_CENTIMETERS
};
static const gchar *GbMetricSymbols[] =
{
  "GTK_PIXELS",
  "GTK_INCHES",
  "GTK_CENTIMETERS"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkVRuler, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_vruler_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_vruler_new ();
  gtk_ruler_set_range (GTK_RULER (new_widget), 0, 10, 0, 10);
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_vruler_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Metric, _("Metric:"),
		       _("The units of the ruler"),
		       GbMetricChoices);
  property_add_float (Lower, _("Lower Value:"),
		      _("The low value of the ruler"));
  property_add_float (Upper, _("Upper Value:"),
		      _("The low value of the ruler"));
  property_add_float (Pos, _("Position:"),
		      _("The current position on the ruler"));
  property_add_float (Max, _("Max:"),
		      _("The maximum value of the ruler"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_vruler_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gchar *metric_name;
  gint i;

  metric_name = GTK_RULER (widget)->metric->metric_name;
  for (i = 0; i < sizeof (GbMetricValues) / sizeof (GbMetricValues[0]); i++)
    {
      if (!strcmp (GbMetricChoices[i], metric_name))
	gb_widget_output_choice (data, Metric, i, GbMetricSymbols[i]);
    }

  gb_widget_output_float (data, Lower, GTK_RULER (widget)->lower);
  gb_widget_output_float (data, Upper, GTK_RULER (widget)->upper);
  gb_widget_output_float (data, Pos, GTK_RULER (widget)->position);
  gb_widget_output_float (data, Max, GTK_RULER (widget)->max_size);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_vruler_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *metric;
  gint i;
  gfloat lower, upper, pos, max;
  gboolean set_range = FALSE;

  metric = gb_widget_input_choice (data, Metric);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbMetricValues) / sizeof (GbMetricValues[0]); i
	   ++)
	{
	  if (!strcmp (metric, GbMetricChoices[i])
	      || !strcmp (metric, GbMetricSymbols[i]))
	    {
	      gtk_ruler_set_metric (GTK_RULER (widget), GbMetricValues[i]);
	      break;
	    }
	}
    }

  lower = gb_widget_input_float (data, Lower);
  if (data->apply)
    set_range = TRUE;
  else
    lower = GTK_RULER (widget)->lower;

  upper = gb_widget_input_float (data, Upper);
  if (data->apply)
    set_range = TRUE;
  else
    upper = GTK_RULER (widget)->upper;

  pos = gb_widget_input_float (data, Pos);
  if (data->apply)
    set_range = TRUE;
  else
    pos = GTK_RULER (widget)->position;

  max = gb_widget_input_float (data, Max);
  if (data->apply)
    set_range = TRUE;
  else
    max = GTK_RULER (widget)->max_size;

  if (set_range)
    gtk_ruler_set_range (GTK_RULER (widget), lower, upper, pos, max);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkVRuler, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_vruler_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_vruler_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *metric_name;
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_vruler_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  metric_name = GTK_RULER (widget)->metric->metric_name;
  if (strcmp (metric_name, "Pixels"))
    {
      for (i = 0; i < sizeof (GbMetricValues) / sizeof (GbMetricValues[0]); i
	   ++)
	{
	  if (!strcmp (GbMetricChoices[i], metric_name))
	    source_add (data, "  gtk_ruler_set_metric (GTK_RULER (%s), %s);\n",
			data->wname, GbMetricSymbols[i]);
	}
    }
  source_add (data, "  gtk_ruler_set_range (GTK_RULER (%s), %g, %g, %g, %g);\n",
	  data->wname, GTK_RULER (widget)->lower, GTK_RULER (widget)->upper,
	      GTK_RULER (widget)->position, GTK_RULER (widget)->max_size);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_vruler_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_vruler_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = vruler_xpm;
  gbwidget.tooltip = _("Vertical Ruler");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_vruler_new;
  gbwidget.gb_widget_create_properties = gb_vruler_create_properties;
  gbwidget.gb_widget_get_properties = gb_vruler_get_properties;
  gbwidget.gb_widget_set_properties = gb_vruler_set_properties;
  gbwidget.gb_widget_write_source = gb_vruler_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_vruler_create_popup_menu;
 */

  return &gbwidget;
}
