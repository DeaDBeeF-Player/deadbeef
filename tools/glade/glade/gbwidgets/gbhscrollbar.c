
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

#include <gtk/gtkhscrollbar.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/hscrollbar.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Policy = "HScrollbar|GtkRange::update_policy";
static gchar *Inverted = "HScrollbar|GtkRange::inverted";

static const gchar *Values[] =
{
  "GtkHScrollbar::value",
  "GtkHScrollbar::lower",
  "GtkHScrollbar::upper",
  "GtkHScrollbar::step",
  "GtkHScrollbar::page",
  "GtkHScrollbar::page_size",
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
 * Creates a new GtkWidget of class GtkHScrollbar, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_hscrollbar_new (GbWidgetNewData * data)
{
  return gtk_hscrollbar_new (NULL);
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_hscrollbar_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Policy, _("Policy:"),
		       _("The update policy of the scrollbar"),
		       GbPolicyChoices);
  property_add_bool (Inverted, _("Inverted:"), _("If the range values are inverted"));
  property_add_adjustment (Values, GB_ADJUST_DEFAULT_LABELS);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_hscrollbar_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;
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
gb_hscrollbar_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *policy;
  gint i;
  gboolean inverted;
  GtkAdjustment *adj;

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
      gtk_signal_emit_by_name (GTK_OBJECT (adj), "changed");
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkHScrollbar, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_hscrollbar_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_hscrollbar_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkAdjustment *adj = GTK_RANGE (widget)->adjustment;
  gint i;

  if (data->create_widget)
    {
      source_add (data,
		  "  %s = gtk_hscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (%g, %g, %g, %g, %g, %g)));\n",
		  data->wname, adj->value, adj->lower, adj->upper,
		  adj->step_increment, adj->page_increment, adj->page_size);
    }

  gb_widget_write_standard_source (widget, data);

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
gb_hscrollbar_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_hscrollbar_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = hscrollbar_xpm;
  gbwidget.tooltip = _("Horizontal Scrollbar");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_hscrollbar_new;
  gbwidget.gb_widget_create_properties = gb_hscrollbar_create_properties;
  gbwidget.gb_widget_get_properties = gb_hscrollbar_get_properties;
  gbwidget.gb_widget_set_properties = gb_hscrollbar_set_properties;
  gbwidget.gb_widget_write_source = gb_hscrollbar_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_hscrollbar_create_popup_menu;
 */

  return &gbwidget;
}
