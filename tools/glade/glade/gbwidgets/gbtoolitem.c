/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999-2002  Damon Chaplin
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

#include <config.h>

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/toolitem.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *VisibleHorz = "GtkToolItem::visible_horizontal";
static gchar *VisibleVert = "GtkToolItem::visible_vertical";
static gchar *IsImportant = "GtkToolItem::is_important";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkToolItem, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
/*
static GtkWidget*
gb_tool_item_new (GbWidgetNewData *data)
{

}
*/



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_tool_item_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (VisibleHorz, _("Show Horizontal:"),
		     _("If the item is visible when the toolbar is horizontal"));
  property_add_bool (VisibleVert, _("Show Vertical:"),
		     _("If the item is visible when the toolbar is vertical"));
  property_add_bool (IsImportant, _("Is Important:"),
		     _("If the item's text should be shown when the toolbar's mode is GTK_TOOLBAR_BOTH_HORIZ"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_tool_item_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_bool (data, VisibleHorz,
			 gtk_object_get_data (GTK_OBJECT (widget), VisibleHorz)
			 != NULL ? FALSE : TRUE);

  gb_widget_output_bool (data, VisibleVert,
			 gtk_object_get_data (GTK_OBJECT (widget), VisibleVert)
			 != NULL ? FALSE : TRUE);

  gb_widget_output_bool (data, IsImportant,
			 gtk_tool_item_get_is_important (GTK_TOOL_ITEM (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_tool_item_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean visible_horz, visible_vert, is_important;

  visible_horz = gb_widget_input_bool (data, VisibleHorz);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), VisibleHorz,
			   visible_horz ? NULL : "FALSE");
    }

  visible_vert = gb_widget_input_bool (data, VisibleVert);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), VisibleVert,
			   visible_vert ? NULL : "FALSE");
    }

  is_important = gb_widget_input_bool (data, IsImportant);
  if (data->apply)
    {
      gtk_tool_item_set_is_important (GTK_TOOL_ITEM (widget), is_important);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkToolItem, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_tool_item_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_tool_item_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      source_add (data,
		  "  %s = (GtkWidget*) gtk_tool_item_new ();\n",
		  data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (gtk_object_get_data (GTK_OBJECT (widget), VisibleHorz) != NULL)
    {
      source_add (data,
		  "  gtk_tool_item_set_visible_horizontal (GTK_TOOL_ITEM (%s), FALSE);\n",
		  data->wname);
    }

  if (gtk_object_get_data (GTK_OBJECT (widget), VisibleVert) != NULL)
    {
      source_add (data,
		  "  gtk_tool_item_set_visible_vertical (GTK_TOOL_ITEM (%s), FALSE);\n",
		  data->wname);
    }

  if (gtk_tool_item_get_is_important (GTK_TOOL_ITEM (widget)))
    {
      source_add (data,
		  "  gtk_tool_item_set_is_important (GTK_TOOL_ITEM (%s), TRUE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_tool_item_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_tool_item_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = toolitem_xpm;
  gbwidget.tooltip = _("Toolbar Item");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_create_properties	= gb_tool_item_create_properties;
  gbwidget.gb_widget_get_properties	= gb_tool_item_get_properties;
  gbwidget.gb_widget_set_properties	= gb_tool_item_set_properties;
  gbwidget.gb_widget_write_source	= gb_tool_item_write_source;
/*
  gbwidget.gb_widget_new		= gb_tool_item_new;
  gbwidget.gb_widget_create_popup_menu	= gb_tool_item_create_popup_menu;
*/

  return &gbwidget;
}

