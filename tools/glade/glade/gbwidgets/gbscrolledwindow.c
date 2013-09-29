
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

#include <gtk/gtkscrolledwindow.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/scrolledwindow.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *HPolicy = "GtkScrolledWindow::hscrollbar_policy";
static gchar *VPolicy = "GtkScrolledWindow::vscrollbar_policy";

static gchar *WindowPlacement = "GtkScrolledWindow::window_placement";
static gchar *ShadowType = "GtkScrolledWindow::shadow_type";

/* I don't think there's any point in adding these. */
/*
   static gchar *HValues[]      = {
   "GtkScrolledWindow::hvalue",
   "GtkScrolledWindow::hlower",
   "GtkScrolledWindow::hupper",
   "GtkScrolledWindow::hstep",
   "GtkScrolledWindow::hpage",
   "GtkScrolledWindow::hpage_size",
   };

   static gchar *VValues[]      = {
   "GtkScrolledWindow::vvalue",
   "GtkScrolledWindow::vlower",
   "GtkScrolledWindow::vupper",
   "GtkScrolledWindow::vstep",
   "GtkScrolledWindow::vpage",
   "GtkScrolledWindow::vpage_size",
   };
 */


static const gchar *GbPolicyChoices[] =
{
  "Always",
  "Automatic",
  "Never",
  NULL
};
static const gint GbPolicyValues[] =
{
  GTK_POLICY_ALWAYS,
  GTK_POLICY_AUTOMATIC,
  GTK_POLICY_NEVER
};
static const gchar *GbPolicySymbols[] =
{
  "GTK_POLICY_ALWAYS",
  "GTK_POLICY_AUTOMATIC",
  "GTK_POLICY_NEVER"
};

/* These aren't really useful so we'll remove them. They should be in the
   scrollbar internal children anyway. */
/*
static const gchar *GbUpdatePolicyChoices[] =
{"Continuous", "Discontinuous",
 "Delayed", NULL};
static const gint GbUpdatePolicyValues[] =
{
  GTK_UPDATE_CONTINUOUS,
  GTK_UPDATE_DISCONTINUOUS,
  GTK_UPDATE_DELAYED
};
static const gchar *GbUpdatePolicySymbols[] =
{
  "GTK_UPDATE_CONTINUOUS",
  "GTK_UPDATE_DISCONTINUOUS",
  "GTK_UPDATE_DELAYED"
};
*/


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkScrolledWindow, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_scrolled_window_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_scrolled_window_new (NULL, NULL);
  if (data->action != GB_LOADING)
    gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_scrolled_window_create_properties (GtkWidget * widget, GbWidgetCreateArgData
				      * data)
{
  property_add_choice (HPolicy, _("H Policy:"),
		       _("When the horizontal scrollbar will be shown"),
		       GbPolicyChoices);
  property_add_choice (VPolicy, _("V Policy:"),
		       _("When the vertical scrollbar will be shown"),
		       GbPolicyChoices);
  property_add_choice (WindowPlacement, _("Window Pos:"),
		       _("Where the child window is located with respect to the scrollbars"),
		       GladeCornerChoices);
  property_add_choice (ShadowType, _("Shadow Type:"),
		       _("The update policy of the vertical scrollbar"),
		       GladeShadowChoices);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_scrolled_window_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;

  for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i++)
    {
      if (GbPolicyValues[i] == GTK_SCROLLED_WINDOW (widget)->hscrollbar_policy)
	gb_widget_output_choice (data, HPolicy, i, GbPolicySymbols[i]);
    }
  for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]); i++)
    {
      if (GbPolicyValues[i] == GTK_SCROLLED_WINDOW (widget)->vscrollbar_policy)
	gb_widget_output_choice (data, VPolicy, i, GbPolicySymbols[i]);
    }

  for (i = 0; i < GladeShadowChoicesSize; i++)
    {
      if (GladeShadowValues[i] == GTK_SCROLLED_WINDOW (widget)->shadow_type)
	gb_widget_output_choice (data, ShadowType, i, GladeShadowSymbols[i]);
    }

  for (i = 0; i < GladeCornerChoicesSize; i++)
    {
      if (GladeCornerValues[i] == GTK_SCROLLED_WINDOW (widget)->window_placement)
	gb_widget_output_choice (data, WindowPlacement, i,
				 GladeCornerSymbols[i]);
    }
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_scrolled_window_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gint i;
  gchar *hpolicy, *vpolicy;
  gboolean set_policy = FALSE;
  GtkPolicyType hpolicy_value = GTK_POLICY_AUTOMATIC;
  GtkPolicyType vpolicy_value = GTK_POLICY_AUTOMATIC;
  gchar *shadow, *placement;

  hpolicy = gb_widget_input_choice (data, HPolicy);
  if (data->apply)
    {
      set_policy = TRUE;
      for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]);
	   i++)
	{
	  if (!strcmp (hpolicy, GbPolicyChoices[i])
	      || !strcmp (hpolicy, GbPolicySymbols[i]))
	    hpolicy_value = GbPolicyValues[i];
	}
    }
  else
    hpolicy_value = GTK_SCROLLED_WINDOW (widget)->hscrollbar_policy;

  vpolicy = gb_widget_input_choice (data, VPolicy);
  if (data->apply)
    {
      set_policy = TRUE;
      for (i = 0; i < sizeof (GbPolicyValues) / sizeof (GbPolicyValues[0]);
	   i++)
	{
	  if (!strcmp (vpolicy, GbPolicyChoices[i])
	      || !strcmp (vpolicy, GbPolicySymbols[i]))
	    vpolicy_value = GbPolicyValues[i];
	}
    }
  else
    vpolicy_value = GTK_SCROLLED_WINDOW (widget)->vscrollbar_policy;

  if (set_policy)
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
				    hpolicy_value, vpolicy_value);

  shadow = gb_widget_input_choice (data, ShadowType);
  if (data->apply)
    {
      for (i = 0; i < GladeShadowChoicesSize; i++)
	{
	  if (!strcmp (shadow, GladeShadowChoices[i])
	      || !strcmp (shadow, GladeShadowSymbols[i]))
	    {
	      gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GladeShadowValues[i]);
	      break;
	    }
	}
    }

  placement = gb_widget_input_choice (data, WindowPlacement);
  if (data->apply)
    {
      for (i = 0; i < GladeCornerChoicesSize; i++)
	{
	  if (!strcmp (placement, GladeCornerChoices[i])
	      || !strcmp (placement, GladeCornerSymbols[i]))
	    {
	      gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (widget),
						 GladeCornerValues[i]);
	      break;
	    }
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkScrolledWindow, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_scrolled_window_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_scrolled_window_write_source (GtkWidget * widget, GbWidgetWriteSourceData *
				 data)
{
  const gchar *hpolicy = GbPolicySymbols[0], *vpolicy = GbPolicySymbols[0];
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_scrolled_window_new (NULL, NULL);\n",
		  data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (GTK_SCROLLED_WINDOW (widget)->hscrollbar_policy != GTK_POLICY_ALWAYS
    || GTK_SCROLLED_WINDOW (widget)->vscrollbar_policy != GTK_POLICY_ALWAYS)
    {
      for (i = 0; i < sizeof (GbPolicyValues)
	   / sizeof (GbPolicyValues[0]); i++)
	{
	  if (GbPolicyValues[i] == GTK_SCROLLED_WINDOW (widget)->hscrollbar_policy)
	    hpolicy = GbPolicySymbols[i];
	  if (GbPolicyValues[i] == GTK_SCROLLED_WINDOW (widget)->vscrollbar_policy)
	    vpolicy = GbPolicySymbols[i];
	}
      source_add (data, "  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (%s), %s, %s);\n",
		  data->wname, hpolicy, vpolicy);
    }

  if (GTK_SCROLLED_WINDOW (widget)->shadow_type != GTK_SHADOW_NONE)
    {
      for (i = 0; i < GladeShadowChoicesSize; i++)
	{
	  if (GladeShadowValues[i] == GTK_SCROLLED_WINDOW (widget)->shadow_type)
	    source_add (data, "  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (%s), %s);\n",
			data->wname, GladeShadowSymbols[i]);
	}
    }

  if (GTK_SCROLLED_WINDOW (widget)->window_placement != GTK_CORNER_TOP_LEFT)
    {
      for (i = 0; i < GladeCornerChoicesSize; i++)
	{
	  if (GladeCornerValues[i] == GTK_SCROLLED_WINDOW (widget)->window_placement)
	    source_add (data, "  gtk_scrolled_window_set_placement (GTK_SCROLLED_WINDOW (%s), %s);\n",
			data->wname, GladeCornerSymbols[i]);
	}
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_scrolled_window_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_scrolled_window_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = scrolledwindow_xpm;
  gbwidget.tooltip = _("Scrolled Window");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_scrolled_window_new;
  gbwidget.gb_widget_create_properties = gb_scrolled_window_create_properties;
  gbwidget.gb_widget_get_properties = gb_scrolled_window_get_properties;
  gbwidget.gb_widget_set_properties = gb_scrolled_window_set_properties;
  gbwidget.gb_widget_write_source = gb_scrolled_window_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_scrolled_window_create_popup_menu;
 */

  return &gbwidget;
}
