
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

#include <gtk/gtkmenu.h>
#include "../gb.h"
#include "../glade_gnome.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/popupmenu.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;



/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkMenu, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget*
gb_menu_new(GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_menu_new ();

  /* We create an accelerator table/group so that we can install accels
     easily. */
  gtk_menu_set_accel_group (GTK_MENU (new_widget), gtk_accel_group_new ());
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
/*
   static void
   gb_menu_create_properties(GtkWidget *widget, GbWidgetCreateArgData *data)
   {

   }
 */



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
/*
   static void
   gb_menu_get_properties(GtkWidget *widget, GbWidgetGetArgData *data)
   {

   }
 */



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
/*
   static void
   gb_menu_set_properties(GtkWidget *widget, GbWidgetSetArgData *data)
   {

   }
 */



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkMenu, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_menu_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_menu_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
#ifdef USE_GNOME
  /* For Gnome projects the menus are created using GnomeUIInfo structs, so
     we just create the start of the struct here, and output code to fill
     the menu from the GnomeUIInfo structs. */
  if (data->project->gnome_support)
    {
      GtkWidget *attach_widget;

      glade_gnome_start_menu_source (GTK_MENU_SHELL (widget), data);

      /* We only need to create the toplevel menu. */
      attach_widget = gtk_menu_get_attach_widget (GTK_MENU (widget));
      if (attach_widget == NULL || GTK_IS_OPTION_MENU (attach_widget))
	{
	  if (data->create_widget)
	    {
	      source_add (data, "  %s = gtk_menu_new ();\n", data->wname);
	    }

	  gb_widget_write_standard_source (widget, data);

	  data->need_accel_group = TRUE;
	  source_add (data,
		      "  gnome_app_fill_menu (GTK_MENU_SHELL (%s), %s_uiinfo,\n"
		      "                       accel_group, FALSE, 0);\n",
		      data->wname, data->real_wname);
	}

      return;
    }
#endif

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_menu_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

#if 0
  gchar *accel_group_decl;

  /* I don't think we need this for GTK+ 2.0. */
  accel_group_decl = g_strdup_printf ("  GtkAccelGroup *%s_accels;\n",
				      data->real_wname);
  source_ensure_decl (data, accel_group_decl);
  source_add (data,
	      "  %s_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (%s));\n",
	      data->wname, data->wname);
  g_free (accel_group_decl);
#endif
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_menu_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_menu_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = popupmenu_xpm;
  gbwidget.tooltip = _("Popup Menu");

  /* Fill in any functions that this GbWidget has */
   gbwidget.gb_widget_new               = gb_menu_new;
   gbwidget.gb_widget_write_source = gb_menu_write_source;
/*
   gbwidget.gb_widget_create_properties = gb_menu_create_properties;
   gbwidget.gb_widget_get_properties    = gb_menu_get_properties;
   gbwidget.gb_widget_set_properties    = gb_menu_set_properties;
   gbwidget.gb_widget_create_popup_menu = gb_menu_create_popup_menu;
 */

  return &gbwidget;
}
