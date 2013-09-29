
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

#include <gtk/gtkarrow.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/arrow.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Direction = "GtkArrow::arrow_type";
static gchar *Shadow = "GtkArrow::shadow_type";
static gchar *XAlign = "Arrow|GtkMisc::xalign";
static gchar *YAlign = "Arrow|GtkMisc::yalign";
static gchar *XPad = "Arrow|GtkMisc::xpad";
static gchar *YPad = "Arrow|GtkMisc::ypad";

static const gchar *GbDirectionChoices[] =
{"Up", "Down", "Left", "Right", NULL};
static const gint GbDirectionValues[] =
{
  GTK_ARROW_UP,
  GTK_ARROW_DOWN,
  GTK_ARROW_LEFT,
  GTK_ARROW_RIGHT
};
static const gchar *GbDirectionSymbols[] =
{
  "GTK_ARROW_UP",
  "GTK_ARROW_DOWN",
  "GTK_ARROW_LEFT",
  "GTK_ARROW_RIGHT"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkArrow, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
/*
   GtkWidget*
   gb_arrow_new(GbWidgetNewData *data)
   {

   }
 */



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_arrow_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Direction, _("Direction:"), _("The direction of the arrow"),
		       GbDirectionChoices);
  property_add_choice (Shadow, _("Shadow:"), _("The shadow type of the arrow"),
		       GladeShadowChoices);
  property_add_float_range (XAlign, _("X Align:"),
			    _("The horizontal alignment of the arrow"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_float_range (YAlign, _("Y Align:"),
			    _("The vertical alignment of the arrow"),
			    0, 1, 0.01, 0.1, 0.01, 2);
  property_add_int_range (XPad, _("X Pad:"), _("The horizontal padding"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (YPad, _("Y Pad:"), _("The vertical padding"),
			  0, 1000, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_arrow_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;

  for (i = 0; i < sizeof (GbDirectionValues) / sizeof (GbDirectionValues[0]);
       i++)
    {
      if (GbDirectionValues[i] == GTK_ARROW (widget)->arrow_type)
	gb_widget_output_choice (data, Direction, i, GbDirectionSymbols[i]);
    }

  for (i = 0; i < GladeShadowChoicesSize; i++)
    {
      if (GladeShadowValues[i] == GTK_ARROW (widget)->shadow_type)
	gb_widget_output_choice (data, Shadow, i, GladeShadowSymbols[i]);
    }
  gb_widget_output_float (data, XAlign, GTK_MISC (widget)->xalign);
  gb_widget_output_float (data, YAlign, GTK_MISC (widget)->yalign);
  gb_widget_output_int (data, XPad, GTK_MISC (widget)->xpad);
  gb_widget_output_int (data, YPad, GTK_MISC (widget)->ypad);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_arrow_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *direction, *shadow;
  gboolean set_arrow = FALSE, set_alignment = FALSE, set_padding = FALSE;
  gfloat xalign, yalign;
  gint xpad, ypad, i;
  GtkArrowType arrow_type = GTK_ARROW_UP;
  GtkShadowType shadow_type = GTK_SHADOW_NONE;

  direction = gb_widget_input_choice (data, Direction);
  if (data->apply)
    {
      for (i = 0;
	   i < sizeof (GbDirectionValues) / sizeof (GbDirectionValues[0]);
	   i++)
	{
	  if (!strcmp (direction, GbDirectionChoices[i])
	      || !strcmp (direction, GbDirectionSymbols[i]))
	    {
	      arrow_type = GbDirectionValues[i];
	      set_arrow = TRUE;
	      break;
	    }
	}
    }
  else
    arrow_type = GTK_ARROW (widget)->arrow_type;

  shadow = gb_widget_input_choice (data, Shadow);
  if (data->apply)
    {
      for (i = 0; i < GladeShadowChoicesSize; i++)
	{
	  if (!strcmp (shadow, GladeShadowChoices[i])
	      || !strcmp (shadow, GladeShadowSymbols[i]))
	    {
	      shadow_type = GladeShadowValues[i];
	      set_arrow = TRUE;
	      break;
	    }
	}
    }
  else
    shadow_type = GTK_ARROW (widget)->shadow_type;

  if (set_arrow)
    gtk_arrow_set (GTK_ARROW (widget), arrow_type, shadow_type);

  xalign = gb_widget_input_float (data, XAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    xalign = GTK_MISC (widget)->xalign;

  yalign = gb_widget_input_float (data, YAlign);
  if (data->apply)
    set_alignment = TRUE;
  else
    yalign = GTK_MISC (widget)->yalign;

  if (set_alignment)
    gtk_misc_set_alignment (GTK_MISC (widget), xalign, yalign);

  xpad = gb_widget_input_int (data, XPad);
  if (data->apply)
    set_padding = TRUE;
  else
    xpad = GTK_MISC (widget)->xpad;

  ypad = gb_widget_input_int (data, YPad);
  if (data->apply)
    set_padding = TRUE;
  else
    ypad = GTK_MISC (widget)->ypad;

  if (set_padding)
    gtk_misc_set_padding (GTK_MISC (widget), xpad, ypad);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkArrow, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_arrow_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_arrow_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gfloat xalign, yalign;
  gint xpad, ypad, arrow_type = 0, shadow_type = 0, i;

  for (i = 0; i < sizeof (GbDirectionValues) / sizeof (GbDirectionValues[0]);
       i++)
    if (GbDirectionValues[i] == GTK_ARROW (widget)->arrow_type)
      {
	arrow_type = i;
	break;
      }
  for (i = 0; i < GladeShadowChoicesSize; i++)
    if (GladeShadowValues[i] == GTK_ARROW (widget)->shadow_type)
      {
	shadow_type = i;
	break;
      }
  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_arrow_new (%s, %s);\n", data->wname,
	      GbDirectionSymbols[arrow_type], GladeShadowSymbols[shadow_type]);
    }
  gb_widget_write_standard_source (widget, data);

  xalign = GTK_MISC (widget)->xalign;
  yalign = GTK_MISC (widget)->yalign;
  if (xalign != 0.5 || yalign != 0.5)
    {
      source_add (data, "  gtk_misc_set_alignment (GTK_MISC (%s), %g, %g);\n",
		  data->wname, xalign, yalign);
    }
  xpad = GTK_MISC (widget)->xpad;
  ypad = GTK_MISC (widget)->ypad;
  if (xpad || ypad)
    {
      source_add (data, "  gtk_misc_set_padding (GTK_MISC (%s), %d, %d);\n",
		  data->wname, xpad, ypad);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_arrow_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_arrow_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = arrow_xpm;
  gbwidget.tooltip = _("Arrow");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties = gb_arrow_create_properties;
  gbwidget.gb_widget_get_properties = gb_arrow_get_properties;
  gbwidget.gb_widget_set_properties = gb_arrow_set_properties;
/*
   gbwidget.gb_widget_new               = gb_arrow_new;
   gbwidget.gb_widget_create_popup_menu = gb_arrow_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_arrow_write_source;

  return &gbwidget;
}
