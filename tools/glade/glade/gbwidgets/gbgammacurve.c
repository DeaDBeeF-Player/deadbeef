
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

#include <gtk/gtkcurve.h>
#include <gtk/gtkgamma.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gammacurve.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

#if 0
static gchar *Type = "GammaCurve|GtkCurve::curve_type";
#endif
static gchar *XMin = "GammaCurve|GtkCurve::min_x";
static gchar *XMax = "GammaCurve|GtkCurve::max_x";
static gchar *YMin = "GammaCurve|GtkCurve::min_y";
static gchar *YMax = "GammaCurve|GtkCurve::max_y";

#if 0
static const gchar *GbTypeChoices[] =
{"Linear", "Spline", "Free", NULL};
static const gint GbTypeValues[] =
{
  GTK_CURVE_TYPE_LINEAR,	/* linear interpolation */
  GTK_CURVE_TYPE_SPLINE,	/* spline interpolation */
  GTK_CURVE_TYPE_FREE		/* free form curve */
};
static const gchar *GbTypeSymbols[] =
{
  "GTK_CURVE_TYPE_LINEAR",
  "GTK_CURVE_TYPE_SPLINE",
  "GTK_CURVE_TYPE_FREE"
};
#endif

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkGammaCurve, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
/*
   GtkWidget*
   gb_gamma_curve_new(GbWidgetNewData *data)
   {

   }
 */



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gamma_curve_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				  data)
{
#if 0
  property_add_choice (Type, _("Initial Type:"), _("The initial type of the curve"),
		       GbTypeChoices);
#endif
  property_add_float (XMin, _("X Min:"), _("The minimum horizontal value"));
  property_add_float (XMax, _("X Max:"), _("The maximum horizontal value"));
  property_add_float (YMin, _("Y Min:"), _("The minimum vertical value"));
  property_add_float (YMax, _("Y Max:"), _("The maximum vertical value"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gamma_curve_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GtkCurve *curve = GTK_CURVE (GTK_GAMMA_CURVE (widget)->curve);

#if 0
  gint i;

  for (i = 0; i < sizeof (GbTypeValues) / sizeof (GbTypeValues[0]); i++)
    {
      if (GbTypeValues[i] == curve->curve_type)
	gb_widget_output_choice (data, Type, i, GbTypeSymbols[i]);
    }
#endif
  gb_widget_output_float (data, XMin, curve->min_x);
  gb_widget_output_float (data, XMax, curve->max_x);
  gb_widget_output_float (data, YMin, curve->min_y);
  gb_widget_output_float (data, YMax, curve->max_y);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gamma_curve_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GtkCurve *curve = GTK_CURVE (GTK_GAMMA_CURVE (widget)->curve);
  gfloat min_x, max_x, min_y, max_y;
  gboolean set_range = FALSE;

#if 0
  gint i;
  gchar *type;

  type = gb_widget_input_choice (data, Type);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbTypeValues) / sizeof (GbTypeValues[0]); i++)
	{
	  if (!strcmp (type, GbTypeChoices[i])
	      || !strcmp (type, GbTypeSymbols[i]))
	    {
	      gtk_curve_set_curve_type (curve, GbTypeValues[i]);
	      break;
	    }
	}
    }
#endif

  min_x = gb_widget_input_float (data, XMin);
  if (data->apply)
    set_range = TRUE;
  else
    min_x = GTK_CURVE (widget)->min_x;

  max_x = gb_widget_input_float (data, XMax);
  if (data->apply)
    set_range = TRUE;
  else
    max_x = GTK_CURVE (widget)->max_x;

  min_y = gb_widget_input_float (data, YMin);
  if (data->apply)
    set_range = TRUE;
  else
    min_y = GTK_CURVE (widget)->min_y;

  max_y = gb_widget_input_float (data, YMax);
  if (data->apply)
    set_range = TRUE;
  else
    max_y = GTK_CURVE (widget)->max_y;

  if (set_range)
    gtk_curve_set_range (curve, min_x, max_x, min_y, max_y);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkGammaCurve, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_gamma_curve_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gamma_curve_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkCurve *curve = GTK_CURVE (GTK_GAMMA_CURVE (widget)->curve);
#if 0
  gint i;
#endif

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_gamma_curve_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

#if 0
  if (curve->curve_type != GTK_CURVE_TYPE_SPLINE)
    {
      for (i = 0; i < sizeof (GbTypeValues) / sizeof (GbTypeValues[0]); i++)
	{
	  if (GbTypeValues[i] == curve->curve_type)
	    source_add (data, "  gtk_curve_set_curve_type (GTK_CURVE (GTK_GAMMA_CURVE (%s)->curve), %s);\n",
			data->wname, GbTypeSymbols[i]);
	}
    }
#endif

  source_add (data, "  gtk_curve_set_range (GTK_CURVE (GTK_GAMMA_CURVE (%s)->curve), %g, %g, %g, %g);\n",
	      data->wname, curve->min_x, curve->max_x,
	      curve->min_y, curve->max_y);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_gamma_curve_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_gamma_curve_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gammacurve_xpm;
  gbwidget.tooltip = _("Gamma Curve");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties = gb_gamma_curve_create_properties;
  gbwidget.gb_widget_get_properties = gb_gamma_curve_get_properties;
  gbwidget.gb_widget_set_properties = gb_gamma_curve_set_properties;
  gbwidget.gb_widget_write_source = gb_gamma_curve_write_source;
/*
   gbwidget.gb_widget_new               = gb_gamma_curve_new;
   gbwidget.gb_widget_create_popup_menu = gb_gamma_curve_create_popup_menu;
 */

  return &gbwidget;
}
