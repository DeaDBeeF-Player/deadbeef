
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

#include <string.h>

#include <gtk/gtkcheckbutton.h>
#include <gtk/gtklabel.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/checkbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *StockButton = "CheckButton|GtkButton::stock_button";
static gchar *Label = "CheckButton|GtkButton::label";
static gchar *Icon = "CheckButton|GtkButton::icon";
static gchar *FocusOnClick = "CheckButton|GtkButton::focus_on_click";

/* This is only used for normal/stock buttons, not special toolbar buttons,
   as the toolbar has its own relief setting. */
static gchar *Relief = "CheckButton|GtkButton::relief";

static gchar *State = "CheckButton|GtkToggleButton::active";
static gchar *Inconsistent = "CheckButton|GtkToggleButton::inconsistent";
static gchar *Indicator = "CheckButton|GtkToggleButton::draw_indicator";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkCheckButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_check_button_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_CREATING)
    new_widget = gtk_check_button_new_with_label (data->name);
  else
    {
      new_widget = gtk_check_button_new ();
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder());
    }
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_check_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				   data)
{
  property_add_stock_item (StockButton, _("Stock Button:"),
			   _("The stock button to use"),
			   GTK_ICON_SIZE_BUTTON);
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GTK_ICON_SIZE_BUTTON);
  property_add_choice (Relief, _("Button Relief:"),
		       _("The relief style of the button"),
		       GladeReliefChoices);

  property_add_bool (State, _("Initially On:"),
		     _("If the check button is initially on"));
  property_add_bool (Inconsistent, _("Inconsistent:"),
		     _("If the button is shown in an inconsistent state"));
  property_add_bool (Indicator, _("Indicator:"),
		     _("If the indicator is always drawn"));
  property_add_bool (FocusOnClick, _("Focus On Click:"), _("If the button grabs focus when it is clicked"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_check_button_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_button_get_standard_properties (widget, data, StockButton, Label, Icon,
				     Relief, FocusOnClick);

  gb_widget_output_bool (data, State, data->widget_data->flags & GLADE_ACTIVE);
  gb_widget_output_bool (data, Inconsistent,
			 GTK_TOGGLE_BUTTON (widget)->inconsistent);
  gb_widget_output_bool (data, Indicator,
			 GTK_TOGGLE_BUTTON (widget)->draw_indicator);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_check_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean state, indicator, inconsistent;

  gb_button_set_standard_properties (widget, data, StockButton, Label, Icon,
				     Relief, FocusOnClick);

  state = gb_widget_input_bool (data, State);
  if (data->apply)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), state);
      if (state)
	data->widget_data->flags |= GLADE_ACTIVE;
      else
	data->widget_data->flags &= ~GLADE_ACTIVE;
    }

  inconsistent = gb_widget_input_bool (data, Inconsistent);
  if (data->apply)
    gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (widget),
					inconsistent);

  indicator = gb_widget_input_bool (data, Indicator);
  if (data->apply)
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (widget), indicator);
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_check_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gb_button_write_standard_source (widget, data, Label);

  if (data->widget_data->flags & GLADE_ACTIVE)
    {
      source_add (data,
	  "  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (GTK_TOGGLE_BUTTON (widget)->inconsistent)
    {
      source_add (data,
      "  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (!GTK_TOGGLE_BUTTON (widget)->draw_indicator)
    {
      source_add (data,
	  "  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (%s), FALSE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_check_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_check_button_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = checkbutton_xpm;
  gbwidget.tooltip = _("Check Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_check_button_new;
  gbwidget.gb_widget_create_properties = gb_check_button_create_properties;
  gbwidget.gb_widget_get_properties = gb_check_button_get_properties;
  gbwidget.gb_widget_set_properties = gb_check_button_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_button_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_check_button_write_source;
  gbwidget.gb_widget_destroy = gb_button_destroy;

  return &gbwidget;
}
