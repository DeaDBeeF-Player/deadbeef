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

#include <gtk/gtkspinbutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/spinbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *ClimbRate = "GtkSpinButton::climb_rate";
static gchar *Digits = "GtkSpinButton::digits";
static gchar *Numeric = "GtkSpinButton::numeric";
static gchar *Policy = "GtkSpinButton::update_policy";
static gchar *Snap = "GtkSpinButton::snap_to_ticks";
static gchar *Wrap = "GtkSpinButton::wrap";


static const gchar *Values[] =
{
  "GtkSpinButton::value",
  "GtkSpinButton::lower",
  "GtkSpinButton::upper",
  "GtkSpinButton::step",
  "GtkSpinButton::page",
  "GtkSpinButton::page_size",
};

static const gchar *GbPolicyChoices[] =
{"Always", "If Valid", NULL};
static const gint GbPolicyValues[] =
{
  GTK_UPDATE_ALWAYS,
  GTK_UPDATE_IF_VALID
};
static const gchar *GbPolicySymbols[] =
{
  "GTK_UPDATE_ALWAYS",
  "GTK_UPDATE_IF_VALID"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkSpinButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_spin_button_new (GbWidgetNewData * data)
{
  GtkObject *adjustment = gtk_adjustment_new (1, 0, 100, 1, 10, 10);
  return gtk_spin_button_new (GTK_ADJUSTMENT (adjustment), 1, 0);
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_spin_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				  data)
{
  property_add_float (ClimbRate, _("Climb Rate:"),
		      _("The climb rate of the spinbutton, used in conjunction with the Page Increment"));
  property_add_int_range (Digits, _("Digits:"),
			  _("The number of decimal digits to show"),
			  0, 5, 1, 1, 0);
  property_add_bool (Numeric, _("Numeric:"),
		     _("If only numeric entry is allowed"));
  property_add_choice (Policy, _("Update Policy:"),
		       _("When value_changed signals are emitted"),
		       GbPolicyChoices);
  property_add_bool (Snap, _("Snap:"),
	      _("If the value is snapped to multiples of the step increment"));
  property_add_bool (Wrap, _("Wrap:"),
		     _("If the value is wrapped at the limits"));
  property_add_adjustment (Values, GB_ADJUST_DEFAULT_LABELS);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_spin_button_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i, update_policy;
  gb_widget_output_float (data, ClimbRate, GTK_SPIN_BUTTON (widget)->climb_rate);
  gb_widget_output_int (data, Digits, GTK_SPIN_BUTTON (widget)->digits);
  gb_widget_output_bool (data, Numeric, GTK_SPIN_BUTTON (widget)->numeric);

  /* This is a slight kludge since the spin_button's update policy is
     a set of flags rather than integer values */
  update_policy = GTK_SPIN_BUTTON (widget)->update_policy
    & (GTK_UPDATE_ALWAYS | GTK_UPDATE_IF_VALID);
  for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i++)
    {
      if (GbPolicyValues[i] == update_policy)
	gb_widget_output_choice (data, Policy, i, GbPolicySymbols[i]);
    }
  /* In GTK 1.1 snap_to_ticks is given its own variable. */
  gb_widget_output_bool (data, Snap, GTK_SPIN_BUTTON (widget)->snap_to_ticks);

  gb_widget_output_bool (data, Wrap, GTK_SPIN_BUTTON (widget)->wrap);
  gb_widget_output_adjustment (data, Values,
			       GTK_SPIN_BUTTON (widget)->adjustment,
			       "adjustment");
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_spin_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gfloat climb_rate;
  gint digits, policy_value = GTK_UPDATE_ALWAYS, i;
  gchar *policy;
  gboolean numeric, snap, wrap;
  GtkAdjustment *adj;

  climb_rate = gb_widget_input_float (data, ClimbRate);
  /* No set function for this */
  if (data->apply)
    GTK_SPIN_BUTTON (widget)->climb_rate = climb_rate;

  digits = gb_widget_input_int (data, Digits);
  if (data->apply)
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (widget), digits);

  numeric = gb_widget_input_bool (data, Numeric);
  if (data->apply)
    gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), numeric);

  snap = gb_widget_input_bool (data, Snap);
  if (data->apply)
    gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (widget), snap);

  policy = gb_widget_input_choice (data, Policy);
  for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i++)
    {
      if (!strcmp (policy, GbPolicyChoices[i])
	  || !strcmp (policy, GbPolicySymbols[i]))
	{
	  policy_value = GbPolicyValues[i];
	  break;
	}
    }
  if (data->apply)
    gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (widget), policy_value);

  wrap = gb_widget_input_bool (data, Wrap);
  if (data->apply)
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), wrap);

  adj = GTK_SPIN_BUTTON (widget)->adjustment;
  if (gb_widget_input_adjustment (data, Values, adj, "adjustment"))
    {
      gtk_signal_emit_by_name (GTK_OBJECT (adj), "value_changed");
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkSpinButton, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_spin_button_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_spin_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint update_policy;
  GtkAdjustment *adj = GTK_SPIN_BUTTON (widget)->adjustment;

  if (data->create_widget)
    {
      source_add_decl (data, "  GObject *%s_adj;\n", data->real_wname);
      source_add (data,
		"  %s_adj = G_OBJECT(gtk_adjustment_new (%.12g, %.12g, %.12g, %.12g, %.12g, %.12g));\n",
		  data->real_wname, adj->value, adj->lower, adj->upper,
		  adj->step_increment, adj->page_increment, adj->page_size);
      source_add (data,
		  "  %s = gtk_spin_button_new (GTK_ADJUSTMENT (%s_adj), %.12g, %d);\n",
		  data->wname, data->real_wname,
		  GTK_SPIN_BUTTON (widget)->climb_rate,
		  GTK_SPIN_BUTTON (widget)->digits);
    }
  gb_widget_write_standard_source (widget, data);

  if (GTK_SPIN_BUTTON (widget)->numeric)
    {
      source_add (data,
		  "  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  update_policy = GTK_SPIN_BUTTON (widget)->update_policy;
  if (update_policy != GTK_UPDATE_ALWAYS)
    {
      source_add (data,
		  "  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (%s), GTK_UPDATE_IF_VALID);\n",
		  data->wname);
    }

  if (GTK_SPIN_BUTTON (widget)->snap_to_ticks)
      source_add (data,
		  "  gtk_spin_button_set_snap_to_ticks (GTK_SPIN_BUTTON (%s), TRUE);\n",
		  data->wname);

  if (GTK_SPIN_BUTTON (widget)->wrap)
    source_add (data,
		"  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (%s), TRUE);\n",
		data->wname);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_spin_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_spin_button_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = spinbutton_xpm;
  gbwidget.tooltip = _("Spin Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_spin_button_new;
  gbwidget.gb_widget_create_properties = gb_spin_button_create_properties;
  gbwidget.gb_widget_get_properties = gb_spin_button_get_properties;
  gbwidget.gb_widget_set_properties = gb_spin_button_set_properties;
/*
   gbwidget.gb_widget_create_popup_menu = gb_spin_button_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_spin_button_write_source;

  return &gbwidget;
}
