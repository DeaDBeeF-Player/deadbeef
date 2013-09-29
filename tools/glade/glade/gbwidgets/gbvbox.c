
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

#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkvbox.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/vbox.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Size = "VBox|GtkBox::size";
static gchar *Homogeneous = "VBox|GtkBox::homogeneous";
static gchar *Spacing = "VBox|GtkBox::spacing";

static void show_vbox_dialog (GbWidgetNewData * data);
static void on_vbox_dialog_ok (GtkWidget * widget,
			       GbWidgetNewData * data);
static void on_vbox_dialog_destroy (GtkWidget * widget,
				    GbWidgetNewData * data);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkVBox, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_vbox_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gtk_vbox_new (FALSE, 0);
      return new_widget;
    }
  else
    {
      show_vbox_dialog (data);
      return NULL;
    }
}


static void
show_vbox_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New vertical box"), data->parent,
				     GTK_SIGNAL_FUNC (on_vbox_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_vbox_dialog_destroy), data);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Number of rows:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "rows",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 5);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


static void
on_vbox_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window, *placeholder;
  gint rows, i;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "rows");
      g_return_if_fail (spinbutton != NULL);
      rows = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gtk_vbox_new (FALSE, 0);
      for (i = 0; i < rows; i++)
	{
	  placeholder = editor_new_placeholder ();
	  /*gtk_widget_set_usize(placeholder, 80, 60); */
	  gtk_box_pack_start (GTK_BOX (new_widget), placeholder, TRUE, TRUE, 0);
	}
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_vbox_dialog_destroy (GtkWidget * widget,
			GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_vbox_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_int_range (Size, _("Size:"), _("The number of widgets in the box"),
			  0, 1000, 1, 10, 1);
  property_add_bool (Homogeneous, _("Homogeneous:"),
		     _("If the children should be the same size"));
  property_add_int_range (Spacing, _("Spacing:"), _("The space between each child"),
			  0, 1000, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_vbox_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  if (data->action != GB_SAVING)
    gb_widget_output_int (data, Size, g_list_length (GTK_BOX (widget)->children));
  gb_widget_output_bool (data, Homogeneous, GTK_BOX (widget)->homogeneous);
  gb_widget_output_int (data, Spacing, GTK_BOX (widget)->spacing);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_vbox_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean homogeneous;
  gint spacing, size;

  if (data->action != GB_LOADING)
    {
      size = gb_widget_input_int (data, Size);
      if (data->apply)
	gb_box_set_size (widget, size);
    }

  homogeneous = gb_widget_input_bool (data, Homogeneous);
  if (data->apply)
    gtk_box_set_homogeneous (GTK_BOX (widget), homogeneous);

  spacing = gb_widget_input_int (data, Spacing);
  if (data->apply)
    gtk_box_set_spacing (GTK_BOX (widget), spacing);
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_vbox_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_vbox_new (%s, %i);\n", data->wname,
		  GTK_BOX (widget)->homogeneous ? "TRUE" : "FALSE",
		  GTK_BOX (widget)->spacing);
    }

  gb_widget_write_standard_source (widget, data);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_vbox_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_vbox_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = vbox_xpm;
  gbwidget.tooltip = _("Vertical Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_vbox_new;
  gbwidget.gb_widget_create_properties = gb_vbox_create_properties;
  gbwidget.gb_widget_get_properties = gb_vbox_get_properties;
  gbwidget.gb_widget_set_properties = gb_vbox_set_properties;
  gbwidget.gb_widget_write_source = gb_vbox_write_source;

  gbwidget.gb_widget_create_child_properties = gb_box_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_box_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_box_set_child_properties;
  gbwidget.gb_widget_create_popup_menu = gb_box_create_popup_menu;
  gbwidget.gb_widget_write_add_child_source = gb_box_write_add_child_source;

  return &gbwidget;
}
