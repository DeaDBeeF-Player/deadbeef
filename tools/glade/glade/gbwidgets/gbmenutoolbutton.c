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
#include "../graphics/menutoolbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *StockButton = "GtkMenuToolButton|GtkToolButton::stock_id";
static gchar *Label = "GtkMenuToolButton|GtkToolButton::label";
static gchar *Icon = "GtkMenuToolButton|GtkToolButton::icon";
static gchar *VisibleHorz = "GtkMenuToolButton|GtkToolItem::visible_horizontal";
static gchar *VisibleVert = "GtkMenuToolButton|GtkToolItem::visible_vertical";
static gchar *IsImportant = "GtkMenuToolButton|GtkToolItem::is_important";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkMenuToolButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_menu_tool_button_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget, *image;
  GbWidget *pixmap_gbwidget;

  /* Place the pixmap icon in the button initially (even when loading). */
  pixmap_gbwidget = gb_widget_lookup_class ("GtkImage");
  if (pixmap_gbwidget)
    {
      image = gtk_image_new_from_pixmap (pixmap_gbwidget->gdkpixmap,
					 pixmap_gbwidget->mask);
    }
  else
    {
      image = gtk_image_new ();
      g_warning ("Couldn't find GtkPixmap data");
    }
  gtk_widget_show (image);

  new_widget = (GtkWidget*) gtk_menu_tool_button_new (image, "");

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_menu_tool_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_stock_item (StockButton, _("Stock Button:"),
			   _("The stock button to use"),
			   GTK_ICON_SIZE_LARGE_TOOLBAR);
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GTK_ICON_SIZE_LARGE_TOOLBAR);
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
gb_menu_tool_button_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_tool_button_get_standard_properties (widget, data,
					  StockButton, Label, Icon,
					  VisibleHorz, VisibleVert,
					  IsImportant);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_menu_tool_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gb_tool_button_set_standard_properties (widget, data,
					  StockButton, Label, Icon,
					  VisibleHorz, VisibleVert,
					  IsImportant);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkMenuToolButton, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_menu_tool_button_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_menu_tool_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *stock_id, *label, *icon_name;
  gboolean translatable, context;
  gchar *comments;

  stock_id = gtk_object_get_data (GTK_OBJECT (widget),
				  GladeToolButtonStockIDKey);
  icon_name = gtk_object_get_data (GTK_OBJECT (widget),
				   GladeToolButtonIconKey);
  label = (gchar*) gtk_tool_button_get_label (GTK_TOOL_BUTTON (widget));

  glade_util_get_translation_properties (widget, Label, &translatable,
					 &comments, &context);

  if (data->create_widget)
    {
      if (stock_id)
	{
	  /* Stock Button */
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_menu_tool_button_new_from_stock (%s);\n",
		      data->wname, source_make_string (stock_id, FALSE));
	}
      else if (icon_name)
	{
	  /* Icon and Label */
	  source_ensure_decl (data, "  GtkWidget *tmp_image;\n");

	  if (glade_util_check_is_stock_id (icon_name))
	    {
	      source_add (data,
			  "  tmp_image = gtk_image_new_from_stock (\"%s\", tmp_toolbar_icon_size);\n",
			  icon_name);
	    }
	  else
	    {
	      source_create_pixmap (data, "tmp_image", icon_name);
	    }

	  source_add (data, "  gtk_widget_show (tmp_image);\n");

	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_menu_tool_button_new (tmp_image, %s);\n",
		      data->wname,
		      label ? source_make_string_full (label, data->use_gettext && translatable, context) : "NULL");
	}
      else
	{
	  /* Just a Label */
	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_menu_tool_button_new (NULL, %s);\n",
		      data->wname,
		      label ? source_make_string_full (label, data->use_gettext && translatable, context) : "NULL");
	}
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
gb_menu_tool_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_menu_tool_button_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = menutoolbutton_xpm;
  gbwidget.tooltip = _("Toolbar Button with Menu");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_menu_tool_button_new;
  gbwidget.gb_widget_create_properties	= gb_menu_tool_button_create_properties;
  gbwidget.gb_widget_get_properties	= gb_menu_tool_button_get_properties;
  gbwidget.gb_widget_set_properties	= gb_menu_tool_button_set_properties;
  gbwidget.gb_widget_write_source	= gb_menu_tool_button_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_menu_tool_button_create_popup_menu;
*/

  return &gbwidget;
}

