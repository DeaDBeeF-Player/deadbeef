
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

#include <gtk/gtkhpaned.h>
#include <gtk/gtktogglebutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/hpaned.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Position = "HPaned|GtkPaned::position";

/* For children of a paned */
static const gchar *Shrink = "GtkPanedChild::shrink";
static const gchar *Resize = "GtkPanedChild::resize";

static void on_toggle_position (GtkWidget * widget, gpointer value);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkHPaned, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_hpaned_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget = gtk_hpaned_new ();
  if (data->action != GB_LOADING)
    {
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder ());
    }
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_hpaned_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_optional_int_range (Position, _("Position:"),
				   _("The position of the divider"),
				   1, 1000, 1, 10, 1,
				   on_toggle_position);
}


static void
on_toggle_position (GtkWidget * button, gpointer value)
{
  GtkWidget *widget;
  gboolean value_set;
  gint position;

  widget = property_get_widget ();
  if (widget == NULL)
    return;

  value_set = GTK_TOGGLE_BUTTON (button)->active ? TRUE : FALSE;
  gtk_widget_set_sensitive (GTK_WIDGET (value), value_set);

  position = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						   Position));
  gtk_paned_set_position (GTK_PANED (widget),
			  value_set ? position : -1);
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_hpaned_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint position;

  position = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						   Position));
  gb_widget_output_optional_int (data, Position, position,
				 GTK_PANED (widget)->position_set);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_hpaned_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gint position;

  position = gb_widget_input_int (data, Position);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), Position,
			   GINT_TO_POINTER (position));
      gtk_paned_set_position (GTK_PANED (widget), position);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkHPaned, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_hpaned_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_hpaned_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint position;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_hpaned_new ();\n", data->wname);
    }
  gb_widget_write_standard_source (widget, data);

  if (GTK_PANED (widget)->position_set)
    {
      position = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						       Position));
      source_add (data, "  gtk_paned_set_position (GTK_PANED (%s), %d);\n",
		  data->wname, position);
    }
}


/*
 * Creates the child packing properties for children of this widget.
 */
void
gb_paned_create_child_properties (GtkWidget * widget,
				  GbWidgetCreateChildArgData * data)
{
  property_add_bool (Shrink, _("Shrink:"),
		     _("Set True to let the widget shrink"));
  property_add_bool (Resize, _("Resize:"),
		     _("Set True to let the widget resize"));
}


void
gb_paned_get_child_properties (GtkWidget *widget, GtkWidget *child,
			       GbWidgetGetArgData *data)
{
  gboolean shrink, resize;

  if (child == GTK_PANED (widget)->child1)
    {
      shrink = GTK_PANED (widget)->child1_shrink;
      resize = GTK_PANED (widget)->child1_resize;
    }
  else if (child == GTK_PANED (widget)->child2)
    {
      shrink = GTK_PANED (widget)->child2_shrink;
      resize = GTK_PANED (widget)->child2_resize;
    }
  else
    {
      /* This shouldn't happen. */
      g_warning ("Couldn't find child of GtkPaned container");
      return;
    }

  if (data->action == GB_SAVING)
    save_start_tag (data, "packing");

  gb_widget_output_bool (data, Shrink, shrink);
  gb_widget_output_bool (data, Resize, resize);

  if (data->action == GB_SAVING)
    save_end_tag (data, "packing");
}


/* Applies or loads the child properties of a child of a hbox/vbox. */
void
gb_paned_set_child_properties (GtkWidget *widget, GtkWidget *child,
			       GbWidgetSetArgData *data)
{
  gboolean shrink, resize, is_child1, need_resize = FALSE;

  if (child == GTK_PANED (widget)->child1)
    is_child1 = TRUE;
  else if (child == GTK_PANED (widget)->child2)
    is_child1 = FALSE;
  else
    {
      /* This shouldn't happen. */
      g_warning ("Couldn't find child of GtkPaned container");
      return;
    }

  shrink = gb_widget_input_bool (data, Shrink);
  if (data->apply)
    {
      if (is_child1)
	GTK_PANED (widget)->child1_shrink = shrink;
      else
	GTK_PANED (widget)->child2_shrink = shrink;
      need_resize = TRUE;
    }

  resize = gb_widget_input_bool (data, Resize);
  if (data->apply)
    {
      if (is_child1)
	GTK_PANED (widget)->child1_resize = resize;
      else
	GTK_PANED (widget)->child2_resize = resize;
      need_resize = TRUE;
    }

  if (need_resize)
    gtk_widget_queue_resize (widget);
}


void
gb_paned_write_add_child_source (GtkWidget * parent,
				 const gchar *parent_name,
				 GtkWidget *child,
				 GbWidgetWriteSourceData * data)
{
  gint child_num;
  gchar *resize, *shrink;

  if (child == GTK_PANED (parent)->child1) {
    child_num = 1;
    resize = GTK_PANED (parent)->child1_resize ? "TRUE" : "FALSE";
    shrink = GTK_PANED (parent)->child1_shrink ? "TRUE" : "FALSE";
  } else if (child == GTK_PANED (parent)->child2) {
    child_num = 2;
    resize = GTK_PANED (parent)->child2_resize ? "TRUE" : "FALSE";
    shrink = GTK_PANED (parent)->child2_shrink ? "TRUE" : "FALSE";
  } else {
    g_warning ("Paned child not found");
    return;
  }

  source_add (data, "  gtk_paned_pack%i (GTK_PANED (%s), %s, %s, %s);\n",
	      child_num, parent_name, data->wname, resize, shrink);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_hpaned_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_hpaned_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = hpaned_xpm;
  gbwidget.tooltip = _("Horizontal Panes");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_hpaned_new;
  gbwidget.gb_widget_create_properties = gb_hpaned_create_properties;
  gbwidget.gb_widget_get_properties = gb_hpaned_get_properties;
  gbwidget.gb_widget_set_properties = gb_hpaned_set_properties;
  gbwidget.gb_widget_create_child_properties = gb_paned_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_paned_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_paned_set_child_properties;
/*
   gbwidget.gb_widget_create_popup_menu = gb_hpaned_create_popup_menu;
 */
  gbwidget.gb_widget_write_source = gb_hpaned_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_paned_write_add_child_source;

  return &gbwidget;
}
