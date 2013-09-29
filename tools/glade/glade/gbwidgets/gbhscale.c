
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

#include <gtk/gtkhscale.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/hscale.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *DrawValue = "HScale|GtkScale::draw_value";
static gchar *ValuePos = "HScale|GtkScale::value_pos";
static gchar *Digits = "HScale|GtkRange::digits";
static gchar *Policy = "HScale|GtkRange::update_policy";
static gchar *Inverted = "HScale|GtkRange::inverted";

static const gchar *Values[] =
{
  "GtkHScale::value",
  "GtkHScale::lower",
  "GtkHScale::upper",
  "GtkHScale::step",
  "GtkHScale::page",
  "GtkHScale::page_size",
};

static const gchar *GbValuePosChoices[] =
{"Left", "Right", "Top", "Bottom", NULL};
static const gint GbValuePosValues[] =
{
  GTK_POS_LEFT,
  GTK_POS_RIGHT,
  GTK_POS_TOP,
  GTK_POS_BOTTOM
};
static const gchar *GbValuePosSymbols[] =
{
  "GTK_POS_LEFT",
  "GTK_POS_RIGHT",
  "GTK_POS_TOP",
  "GTK_POS_BOTTOM"
};

static const gchar *GbPolicyChoices[] =
{"Continuous", "Discontinuous", "Delayed",
 NULL};
static const gint GbPolicyValues[] =
{
  GTK_UPDATE_CONTINUOUS,
  GTK_UPDATE_DISCONTINUOUS,
  GTK_UPDATE_DELAYED
};
static const gchar *GbPolicySymbols[] =
{
  "GTK_UPDATE_CONTINUOUS",
  "GTK_UPDATE_DISCONTINUOUS",
  "GTK_UPDATE_DELAYED"
};

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkHScale, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_hscale_new (GbWidgetNewData * data)
{
  return gtk_hscale_new (NULL);
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_hscale_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (DrawValue, _("Show Value:"), _("If the scale's value is shown"));
  property_add_int_range (Digits, _("Digits:"), _("The number of digits to show"),
			  0, 13, 1, 10, 1);
  property_add_choice (ValuePos, _("Value Pos:"),
		       _("The position of the value"),
		       GbValuePosChoices);
  property_add_choice (Policy, _("Policy:"),
		       _("The update policy of the scale"),
		       GbPolicyChoices);
  property_add_bool (Inverted, _("Inverted:"), _("If the range values are inverted"));
  property_add_adjustment (Values, GB_ADJUST_DEFAULT_LABELS);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_hscale_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;

  gb_widget_output_bool (data, DrawValue, GTK_SCALE (widget)->draw_value);

  for (i = 0; i < sizeof (GbValuePosValues) / sizeof (GbValuePosValues[0]);
       i++)
    {
      if (GbValuePosValues[i] == GTK_SCALE (widget)->value_pos)
	gb_widget_output_choice (data, ValuePos, i, GbValuePosSymbols[i]);
    }
  gb_widget_output_int (data, Digits, GTK_SCALE (widget)->digits);

  for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i++)
    {
      if (GbPolicyValues[i] == GTK_RANGE (widget)->update_policy)
	gb_widget_output_choice (data, Policy, i, GbPolicySymbols[i]);
    }

  gb_widget_output_bool (data, Inverted, GTK_RANGE (widget)->inverted);

  gb_widget_output_adjustment (data, Values, GTK_RANGE (widget)->adjustment,
			       "adjustment");
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_hscale_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean draw_value, inverted;
  gint digits, i;
  gchar *valuepos, *policy;
  GtkAdjustment *adj;

  draw_value = gb_widget_input_bool (data, DrawValue);
  if (data->apply)
    {
      gtk_scale_set_draw_value (GTK_SCALE (widget), draw_value);
      /* Shouldn't really need to do this */
      editor_refresh_widget (widget);
    }

  valuepos = gb_widget_input_choice (data, ValuePos);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbValuePosValues) / sizeof (GbValuePosValues[0]);
	   i++)
	{
	  if (!strcmp (valuepos, GbValuePosChoices[i])
	      || !strcmp (valuepos, GbValuePosSymbols[i]))
	    {
	      gtk_scale_set_value_pos (GTK_SCALE (widget), GbValuePosValues[i]);
	      break;
	    }
	}
    }

  digits = gb_widget_input_int (data, Digits);
  if (data->apply)
    gtk_scale_set_digits (GTK_SCALE (widget), digits);

  policy = gb_widget_input_choice (data, Policy);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i
	   ++)
	{
	  if (!strcmp (policy, GbPolicyChoices[i])
	      || !strcmp (policy, GbPolicySymbols[i]))
	    {
	      gtk_range_set_update_policy (GTK_RANGE (widget), GbPolicyValues
					   [i]);
	      break;
	    }
	}
    }

  inverted = gb_widget_input_bool (data, Inverted);
  if (data->apply)
    {
      gtk_range_set_inverted (GTK_RANGE (widget), inverted);
    }

  adj = GTK_RANGE (widget)->adjustment;
  if (gb_widget_input_adjustment (data, Values, adj, "adjustment"))
    {
      gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkHScale, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_hscale_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_hscale_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkAdjustment *adj = GTK_RANGE (widget)->adjustment;
  gint i;

  if (data->create_widget)
    {
      source_add (data,
		  "  %s = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (%g, %g, %g, %g, %g, %g)));\n",
		  data->wname, adj->value, adj->lower, adj->upper,
		  adj->step_increment, adj->page_increment, adj->page_size);
    }

  gb_widget_write_standard_source (widget, data);

  if (!GTK_SCALE (widget)->draw_value)
    source_add (data, "  gtk_scale_set_draw_value (GTK_SCALE (%s), FALSE);\n",
		data->wname);

  if (GTK_SCALE (widget)->value_pos != GTK_POS_TOP)
    {
      for (i = 0; i < sizeof (GbValuePosValues) / sizeof (GbValuePosValues[0]);
	   i++)
	{
	  if (GbValuePosValues[i] == GTK_SCALE (widget)->value_pos)
	    source_add (data, "  gtk_scale_set_value_pos (GTK_SCALE (%s), %s);\n",
			data->wname, GbValuePosSymbols[i]);
	}
    }
  if (GTK_SCALE (widget)->digits != 1)
    source_add (data, "  gtk_scale_set_digits (GTK_SCALE (%s), %i);\n",
		data->wname, GTK_SCALE (widget)->digits);

  if (GTK_RANGE (widget)->update_policy != GTK_UPDATE_CONTINUOUS)
    {
      for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i
	   ++)
	{
	  if (GbPolicyValues[i] == GTK_RANGE (widget)->update_policy)
	    source_add (data,
		    "  gtk_range_set_update_policy (GTK_RANGE (%s), %s);\n",
			data->wname, GbPolicySymbols[i]);
	}
    }

  if (GTK_RANGE (widget)->inverted)
    {
      source_add (data,
		  "  gtk_range_set_inverted (GTK_RANGE (%s), TRUE);\n",
		  data->wname);

    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_hscale_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_hscale_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = hscale_xpm;
  gbwidget.tooltip = _("Horizontal Scale");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_hscale_new;
  gbwidget.gb_widget_create_properties = gb_hscale_create_properties;
  gbwidget.gb_widget_get_properties = gb_hscale_get_properties;
  gbwidget.gb_widget_set_properties = gb_hscale_set_properties;
  gbwidget.gb_widget_write_source = gb_hscale_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_hscale_create_popup_menu;
 */

  return &gbwidget;
}
