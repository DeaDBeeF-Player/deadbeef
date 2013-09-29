
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

#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkoptionmenu.h>
#include "../gb.h"
#include "../glade_menu_editor.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/optionmenu.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static const char *History = "GtkOptionMenu::history";

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkOptionMenu, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_option_menu_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;
  new_widget = gtk_option_menu_new ();
  return new_widget;
}

/* Make window behave like a dialog */
static void
dialogize (GtkWidget *menued, GtkWidget *parent_widget)
{
  GtkWidget *transient_parent;

  gtk_signal_connect (GTK_OBJECT (menued), "key_press_event",
		      GTK_SIGNAL_FUNC (glade_util_check_key_is_esc),
		      GINT_TO_POINTER (GladeEscDestroys));
  transient_parent = glade_util_get_toplevel (parent_widget);
  if (GTK_IS_WINDOW (transient_parent))
    {
      gtk_window_set_transient_for (GTK_WINDOW (menued),
				    GTK_WINDOW (transient_parent));
    }
}

static void
set_menu (GladeMenuEditor *menued, GtkOptionMenu *option)
{
  int history;

  g_return_if_fail (GLADE_IS_MENU_EDITOR (menued));
  g_return_if_fail (GTK_IS_OPTION_MENU (option));
  
  history = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (option), History));
  gtk_option_menu_set_menu (option, GTK_WIDGET (menued->menu));	
  gtk_option_menu_set_history (option, history);
}

static void
gb_menu_bar_on_edit_menu (GtkWidget *button,
			  gpointer data)
{
  GtkWidget *option, *menued, *menu;

  option = property_get_widget ();
  g_return_if_fail (GTK_IS_OPTION_MENU (option));

  /* 
   * we need to remove the menu from the option menu otherwise there
   * will be a separator where the selected menu is
   */
  g_object_set_data (G_OBJECT (option), 
		     History,
		     GINT_TO_POINTER (gtk_option_menu_get_history (GTK_OPTION_MENU (option))));
  menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option));
  if (!menu)
    menu = gb_widget_new ("GtkMenu", option);
  g_object_ref (menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), gtk_menu_new ());

  menued = glade_menu_editor_new (current_project, GTK_MENU_SHELL (menu));
  g_signal_connect (menued, "destroy", G_CALLBACK (set_menu), option);

  /* I think this was hidden as it doesn't call set_menu() to reset the
     history. */
  gtk_widget_hide (GLADE_MENU_EDITOR (menued)->apply_button);

  dialogize (menued, button);
  gtk_widget_show (GTK_WIDGET (menued));

  gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
  g_object_unref (menu);
}

/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_option_menu_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				  data)
{
  GtkWidget *property_table, *button;
  gint property_table_row;

  /* Add a button for editing the menubar. */
  property_table = property_get_table_position (&property_table_row);
  button = gtk_button_new_with_label (_("Edit Menus..."));
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gb_menu_bar_on_edit_menu), NULL);
  gtk_table_attach (GTK_TABLE (property_table), button, 0, 3,
		    property_table_row, property_table_row + 1,
		    GTK_FILL, GTK_FILL, 10, 10);

}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_option_menu_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  if (data->action == GB_SAVING)
    gb_widget_output_int (data, History, 
			  gtk_option_menu_get_history (GTK_OPTION_MENU (widget)));
}

/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_option_menu_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  int history;

  if (data->action == GB_LOADING)
    {
      history = gb_widget_input_int (data, History);
      if (data->apply)
	gtk_option_menu_set_history (GTK_OPTION_MENU (widget), history);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkOptionMenu, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_option_menu_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_option_menu_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint history;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_option_menu_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  history = gtk_option_menu_get_history (GTK_OPTION_MENU (widget));
  if (history > 0)
    source_add (data,
	      "  gtk_option_menu_set_history (GTK_OPTION_MENU (%s), %i);\n",
		data->wname, history);
}


static void
gb_option_menu_add_child (GtkWidget *widget,
			  GtkWidget *child,
			  GbWidgetSetArgData *data)
{
  if (GTK_IS_MENU (child))
    gtk_option_menu_set_menu (GTK_OPTION_MENU (widget), child);
  else
    g_warning (_("Cannot add a %s to a GtkOptionMenu."),
	       g_type_name (G_OBJECT_TYPE (child)));
}


/* Outputs source to add a child widget to a hbox/vbox. */
void
gb_option_menu_write_add_child_source (GtkWidget * parent,
				       const gchar *parent_name,
				       GtkWidget *child,
				       GbWidgetWriteSourceData * data)
{
  /* We don't output anything here. The code to add the menu to the option
     menu must be output after all the menuitems are added, so we have some
     special code in gb_widget_write_source() to do that. */
#if 0
  source_add (data,
	      "  gtk_option_menu_set_menu (GTK_OPTION_MENU (%s), %s);\n",
	      parent_name, data->wname);
#endif
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_option_menu_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_option_menu_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = optionmenu_xpm;
  gbwidget.tooltip = _("Option Menu");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_option_menu_new;
  gbwidget.gb_widget_create_properties = gb_option_menu_create_properties;
  gbwidget.gb_widget_get_properties = gb_option_menu_get_properties;
  gbwidget.gb_widget_set_properties = gb_option_menu_set_properties;
  gbwidget.gb_widget_add_child = gb_option_menu_add_child;
  gbwidget.gb_widget_write_source = gb_option_menu_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_option_menu_write_add_child_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_option_menu_create_popup_menu;
 */

  return &gbwidget;
}
