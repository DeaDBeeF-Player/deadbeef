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

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include "../gb.h"
#include "../glade_gnome.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/checkmenuitem.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Label = "CheckMenuItem|GtkItem::label";
static gchar *State = "GtkCheckMenuItem::active";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkCheckMenuItem, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_check_menu_item_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_CREATING)
    new_widget = gtk_check_menu_item_new_with_label (data->name);
  else
    new_widget = gtk_check_menu_item_new ();
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_check_menu_item_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_bool (State, _("Initially On:"),
		     _("If the check menu item is initially on"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_check_menu_item_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_widget_output_child_label (widget, data, Label);

  gb_widget_output_bool (data, State, data->widget_data->flags & GLADE_ACTIVE);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_check_menu_item_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean state;

  gb_widget_input_child_label (widget, data, Label);

  state = gb_widget_input_bool (data, State);
  if (data->apply)
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), state);
      if (state)
	data->widget_data->flags |= GLADE_ACTIVE;
      else
	data->widget_data->flags &= ~GLADE_ACTIVE;
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkCheckMenuItem, with signals pointing to
 * other functions in this file.
 */
static void
gb_check_menu_item_create_popup_menu (GtkWidget * widget,
				      GbWidgetCreateMenuData * data)
{
  /* Add command to remove child label. */
#if 0
  gb_widget_create_child_label_popup_menu (widget, data);
#endif
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_check_menu_item_write_source (GtkWidget * widget, GbWidgetWriteSourceData *
				 data)
{
  GtkWidget *child = GTK_BIN (widget)->child;
  gchar *label_text;
  gboolean translatable, context;
  gchar *comments;

#ifdef USE_GNOME
  if (data->project->gnome_support)
    {
      glade_gnome_write_menu_item_source (GTK_MENU_ITEM (widget), data);
      return;
    }
#endif

  if (child && GTK_IS_LABEL (child) && !GB_IS_GB_WIDGET (child))
    {
      glade_util_get_translation_properties (widget, Label, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      label_text = glade_util_get_label_text (child);
      source_add (data,
		  "  %s = gtk_check_menu_item_new_with_mnemonic (%s);\n",
		  data->wname,
		  source_make_string_full (label_text, data->use_gettext && translatable, context));
      g_free (label_text);
    }
  else
    {
      source_add (data, "  %s = gtk_check_menu_item_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (data->widget_data->flags & GLADE_ACTIVE)
    {
      source_add (data,
      "  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (%s), TRUE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_check_menu_item_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_check_menu_item_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = checkmenuitem_xpm;
  gbwidget.tooltip = _("Check Menu Item");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_check_menu_item_new;
  gbwidget.gb_widget_create_properties = gb_check_menu_item_create_properties;
  gbwidget.gb_widget_get_properties = gb_check_menu_item_get_properties;
  gbwidget.gb_widget_set_properties = gb_check_menu_item_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_check_menu_item_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_check_menu_item_write_source;

  return &gbwidget;
}
