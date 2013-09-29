
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

#include <gtk/gtkhandlebox.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/handlebox.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Shadow = "GtkHandleBox::shadow_type";
static gchar *Position = "GtkHandleBox::handle_position";
static gchar *SnapEdge = "GtkHandleBox::snap_edge";


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

static const gchar *GbPositionChoices[] =
{"Left", "Right", "Top", "Bottom", NULL};
static const gint GbPositionValues[] =
{
  GTK_POS_LEFT,
  GTK_POS_RIGHT,
  GTK_POS_TOP,
  GTK_POS_BOTTOM
};
static const gchar *GbPositionSymbols[] =
{
  "GTK_POS_LEFT",
  "GTK_POS_RIGHT",
  "GTK_POS_TOP",
  "GTK_POS_BOTTOM"
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkHandleBox, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_handle_box_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_handle_box_new ();

  /* We set the snap edge to top, which matches the default handle position,
     since we don't support the default value of -1. */
  gtk_handle_box_set_snap_edge (GTK_HANDLE_BOX (new_widget), GTK_POS_TOP);

  if (data->action != GB_LOADING)
    gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_handle_box_create_properties(GtkWidget *widget, GbWidgetCreateArgData *data)
{
  property_add_choice (Shadow, _("Shadow:"),
		       _("The type of shadow around the handle box"),
		       GbShadowChoices);

  property_add_choice (Position, _("Handle Pos:"),
		       _("The position of the handle"),
		       GbPositionChoices);
  property_add_choice (SnapEdge, _("Snap Edge:"),
		       _("The edge of the handle box which snaps into position"),
		       GbPositionChoices);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_handle_box_get_properties(GtkWidget *widget, GbWidgetGetArgData *data)
{
  gint i;

  for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i++)
    {
      if (GbShadowValues[i] == GTK_HANDLE_BOX (widget)->shadow_type)
	gb_widget_output_choice (data, Shadow, i, GbShadowSymbols[i]);
    }

  for (i = 0; i < sizeof (GbPositionValues) / sizeof (GbPositionValues[0]);
       i++)
    {
      if (GbPositionValues[i] == GTK_HANDLE_BOX (widget)->handle_position)
	gb_widget_output_choice (data, Position, i, GbPositionSymbols[i]);
    }

  for (i = 0; i < sizeof (GbPositionValues) / sizeof (GbPositionValues[0]);
       i++)
    {
      if (GbPositionValues[i] == GTK_HANDLE_BOX (widget)->snap_edge)
	gb_widget_output_choice (data, SnapEdge, i, GbPositionSymbols[i]);
    }
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_handle_box_set_properties(GtkWidget *widget, GbWidgetSetArgData *data)
{
  gchar *shadow, *position, *snap_edge;
  gint i;

  shadow = gb_widget_input_choice (data, Shadow);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]);
	   i++)
	{
	  if (!strcmp (shadow, GbShadowChoices[i])
	      || !strcmp (shadow, GbShadowSymbols[i]))
	    {
	      gtk_handle_box_set_shadow_type (GTK_HANDLE_BOX (widget),
					      GbShadowValues[i]);
	      break;
	    }
	}
    }

  position = gb_widget_input_choice (data, Position);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbPositionValues) / sizeof (GbPositionValues[0]);
	   i++)
	{
	  if (!strcmp (position, GbPositionChoices[i])
	      || !strcmp (position, GbPositionSymbols[i]))
	    {
	      gtk_handle_box_set_handle_position (GTK_HANDLE_BOX (widget),
						  GbPositionValues[i]);
	      break;
	    }
	}
    }

  snap_edge = gb_widget_input_choice (data, SnapEdge);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbPositionValues) / sizeof (GbPositionValues[0]);
	   i++)
	{
	  if (!strcmp (snap_edge, GbPositionChoices[i])
	      || !strcmp (snap_edge, GbPositionSymbols[i]))
	    {
	      gtk_handle_box_set_snap_edge (GTK_HANDLE_BOX (widget),
					    GbPositionValues[i]);
	      break;
	    }
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkHandleBox, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_handle_box_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_handle_box_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_handle_box_new ();\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  if (GTK_HANDLE_BOX (widget)->shadow_type != GTK_SHADOW_OUT)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]);
	   i++)
	if (GbShadowValues[i] == GTK_HANDLE_BOX (widget)->shadow_type)
	  {
	    source_add (data,
			"  gtk_handle_box_set_shadow_type (GTK_HANDLE_BOX (%s), %s);\n",
			data->wname, GbShadowSymbols[i]);
	    break;
	  }
    }

  if (GTK_HANDLE_BOX (widget)->handle_position != GTK_POS_LEFT)
    {
      for (i = 0; i < sizeof (GbPositionValues) / sizeof (GbPositionValues[0]);
	   i++)
	{
	  if (GbPositionValues[i] == GTK_HANDLE_BOX (widget)->handle_position)
	    source_add (data,
		    "  gtk_handle_box_set_handle_position (GTK_HANDLE_BOX (%s), %s);\n",
			data->wname, GbPositionSymbols[i]);
	}
    }

  if (GTK_HANDLE_BOX (widget)->snap_edge != GTK_POS_TOP)
    {
      for (i = 0; i < sizeof (GbPositionValues) / sizeof (GbPositionValues[0]);
	   i++)
	{
	  if (GbPositionValues[i] == GTK_HANDLE_BOX (widget)->snap_edge)
	    source_add (data,
		    "  gtk_handle_box_set_snap_edge (GTK_HANDLE_BOX (%s), %s);\n",
			data->wname, GbPositionSymbols[i]);
	}
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_handle_box_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_handle_box_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = handlebox_xpm;
  gbwidget.tooltip = _("Handle Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_handle_box_new;
  gbwidget.gb_widget_create_properties = gb_handle_box_create_properties;
  gbwidget.gb_widget_get_properties    = gb_handle_box_get_properties;
  gbwidget.gb_widget_set_properties    = gb_handle_box_set_properties;
/*
   gbwidget.gb_widget_create_popup_menu = gb_handle_box_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_handle_box_write_source;

  return &gbwidget;
}
