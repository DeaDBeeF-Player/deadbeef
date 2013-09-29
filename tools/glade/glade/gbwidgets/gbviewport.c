
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

#include <gtk/gtkviewport.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/viewport.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Shadow = "GtkViewport::shadow_type";

/* I don't think there's any point in adding these. */
/*
   static gchar *HValues[]      = {
   "GtkViewport::hvalue",
   "GtkViewport::hlower",
   "GtkViewport::hupper",
   "GtkViewport::hstep",
   "GtkViewport::hpage",
   "GtkViewport::hpage_size",
   };

   static gchar *VValues[]      = {
   "GtkViewport::vvalue",
   "GtkViewport::vlower",
   "GtkViewport::vupper",
   "GtkViewport::vstep",
   "GtkViewport::vpage",
   "GtkViewport::vpage_size",
   };
 */

static const gchar *GbShadowChoices[] =
{"None", "In", "Out",
 "Etched In", "Etched Out", NULL};
static const gint GbShadowValues[] =
{
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
};
static const gchar *GbShadowSymbols[] =
{
  "GTK_SHADOW_NONE",
  "GTK_SHADOW_IN",
  "GTK_SHADOW_OUT",
  "GTK_SHADOW_ETCHED_IN",
  "GTK_SHADOW_ETCHED_OUT"
};

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkViewport, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_viewport_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_viewport_new (NULL, NULL);
  if (data->action != GB_LOADING)
    gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_viewport_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Shadow, _("Shadow:"), _("The type of shadow of the viewport"),
		       GbShadowChoices);
  /*
     property_add_adjustment(HValues, GB_ADJUST_H_LABELS);
     property_add_adjustment(VValues, GB_ADJUST_V_LABELS);
   */
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_viewport_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;
  for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i++)
    {
      if (GbShadowValues[i] == GTK_VIEWPORT (widget)->shadow_type)
	gb_widget_output_choice (data, Shadow, i, GbShadowSymbols[i]);
    }

  /*
     gb_widget_output_adjustment(data, HValues, GTK_VIEWPORT(widget)->hadjustment);
     gb_widget_output_adjustment(data, VValues, GTK_VIEWPORT(widget)->vadjustment);
   */
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_viewport_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gint i;
  gchar *shadow;

  shadow = gb_widget_input_choice (data, Shadow);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i
	   ++)
	{
	  if (!strcmp (shadow, GbShadowChoices[i])
	      || !strcmp (shadow, GbShadowSymbols[i]))
	    {
	      gtk_viewport_set_shadow_type (GTK_VIEWPORT (widget), GbShadowValues
					    [i]);
	      break;
	    }
	}
    }

  /*
     if (gb_widget_input_adjustment(data, HValues,
     GTK_VIEWPORT(widget)->hadjustment))
     gtk_signal_emit_by_name (GTK_OBJECT (GTK_VIEWPORT(widget)->hadjustment),
     "value_changed");
     if (gb_widget_input_adjustment(data, VValues,
     GTK_VIEWPORT(widget)->vadjustment))
     gtk_signal_emit_by_name (GTK_OBJECT (GTK_VIEWPORT(widget)->vadjustment),
     "value_changed");
   */
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkViewport, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_viewport_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_viewport_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_viewport_new (NULL, NULL);\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (GTK_VIEWPORT (widget)->shadow_type != GTK_SHADOW_IN)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i
	   ++)
	{
	  if (GbShadowValues[i] == GTK_VIEWPORT (widget)->shadow_type)
	    source_add (data,
		"  gtk_viewport_set_shadow_type (GTK_VIEWPORT (%s), %s);\n",
			data->wname, GbShadowSymbols[i]);
	}
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_viewport_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_viewport_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = viewport_xpm;
  gbwidget.tooltip = _("Viewport");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_viewport_new;
  gbwidget.gb_widget_create_properties = gb_viewport_create_properties;
  gbwidget.gb_widget_get_properties = gb_viewport_get_properties;
  gbwidget.gb_widget_set_properties = gb_viewport_set_properties;
  gbwidget.gb_widget_write_source = gb_viewport_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_viewport_create_popup_menu;
 */

  return &gbwidget;
}
