/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999  Damon Chaplin
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

#include <gnome.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-propertybox.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Title = "GnomePropertyBox|GtkWindow::title";
static gchar *Position = "GnomePropertyBox|GtkWindow::window_position";
static gchar *Modal = "GnomePropertyBox|GtkWindow::modal";
static gchar *DefaultWidth = "GnomePropertyBox|GtkWindow::default_width";
static gchar *DefaultHeight = "GnomePropertyBox|GtkWindow::default_height";
static gchar *Shrink = "GnomePropertyBox|GtkWindow::allow_shrink";
static gchar *Grow = "GnomePropertyBox|GtkWindow::allow_grow";
static gchar *AutoShrink = "GnomePropertyBox|GtkWindow::auto_shrink";
static gchar *IconName = "GnomePropertyBox|GtkWindow::icon_name";
static gchar *FocusOnMap = "GnomePropertyBox|GtkWindow::focus_on_map";

static gchar *Resizable = "GnomePropertyBox|GtkWindow::resizable";
static gchar *DestroyWithParent = "GnomePropertyBox|GtkWindow::destroy_with_parent";
static gchar *Icon = "GnomePropertyBox|GtkWindow::icon";

static gchar *Role = "GnomePropertyBox|GtkWindow::role";
static gchar *TypeHint = "GnomePropertyBox|GtkWindow::type_hint";
static gchar *SkipTaskbar = "GnomePropertyBox|GtkWindow::skip_taskbar_hint";
static gchar *SkipPager = "GnomePropertyBox|GtkWindow::skip_pager_hint";
static gchar *Decorated = "GnomePropertyBox|GtkWindow::decorated";
static gchar *Gravity = "GnomePropertyBox|GtkWindow::gravity";
static gchar *Urgency = "GnomePropertyBox|GtkWindow::urgency_hint";

static void show_gnome_property_box_dialog (GbWidgetNewData * data);
static void on_gnome_property_box_dialog_ok (GtkWidget * widget,
					     GbWidgetNewData * data);
static void on_gnome_property_box_dialog_destroy (GtkWidget * widget,
						  GbWidgetNewData * data);
GtkWidget * gnome_property_box_new_tab_label ();

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomePropertyBox, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_property_box_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gnome_property_box_new ();

      gb_widget_create_from (GNOME_PROPERTY_BOX (new_widget)->notebook,
			     data->action == GB_CREATING ? "notebook" : NULL);
      gb_widget_set_child_name (GNOME_PROPERTY_BOX (new_widget)->notebook,
				GladeChildGnomePBoxNotebook);

      /* We connect a close signal handler which always returns TRUE so that
	 the built-in close functionality is skipped. */
      gtk_signal_connect (GTK_OBJECT (new_widget), "close",
			  GTK_SIGNAL_FUNC (gtk_true), NULL);

      /* Now we connect our normal delete_event handler. */
      gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
			  GTK_SIGNAL_FUNC (editor_close_window), NULL);

      return new_widget;
    }
  else
    {
      show_gnome_property_box_dialog (data);
      return NULL;
    }
}


static void
show_gnome_property_box_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New GnomePropertyBox"), data->parent,
				     GTK_SIGNAL_FUNC (on_gnome_property_box_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_gnome_property_box_dialog_destroy),
		      data);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Number of pages:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "pages",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 5);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


static void
on_gnome_property_box_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window, *placeholder;
  gint pages, i;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "pages");
      g_return_if_fail (spinbutton != NULL);
      pages = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gnome_property_box_new ();

      gb_widget_create_from (GNOME_PROPERTY_BOX (new_widget)->notebook,
			     "notebook");
      gb_widget_set_child_name (GNOME_PROPERTY_BOX (new_widget)->notebook,
				GladeChildGnomePBoxNotebook);

      for (i = 0; i < pages; i++)
	{
	  placeholder = editor_new_placeholder ();
	  gtk_widget_set_usize (placeholder, 300, 150);
	  gnome_property_box_append_page (GNOME_PROPERTY_BOX (new_widget),
					  placeholder,
					  gnome_property_box_new_tab_label ());
	}

      /* We connect a close signal handler which always returns TRUE so that
	 the built-in close functionality is skipped. */
      gtk_signal_connect (GTK_OBJECT (new_widget), "close",
			  GTK_SIGNAL_FUNC (gtk_true), NULL);

      /* Now we connect our normal delete_event handler. */
      gtk_signal_connect (GTK_OBJECT (new_widget), "delete_event",
			  GTK_SIGNAL_FUNC (editor_close_window), NULL);

      gtk_object_set_data (GTK_OBJECT (new_widget), TypeHint,
			   GINT_TO_POINTER (GLADE_TYPE_HINT_DIALOG_INDEX));

      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_gnome_property_box_dialog_destroy (GtkWidget * widget,
				      GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


GtkWidget *
gnome_property_box_new_tab_label ()
{
  GtkWidget *label;

  label = gb_widget_new ("GtkLabel", NULL);
  g_return_val_if_fail (label != NULL, NULL);
  return label;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */

static void
gb_gnome_property_box_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  gb_window_create_standard_properties (widget, data,
		  Title, NULL, Position, Modal,
		  DefaultWidth, DefaultHeight,
		  Shrink, Grow, AutoShrink,
		  IconName, FocusOnMap,
		  Resizable, DestroyWithParent, Icon,
		  Role, TypeHint, SkipTaskbar,
		  SkipPager, Decorated, Gravity, Urgency);
}




/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */

static void
gb_gnome_property_box_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
    gb_window_get_standard_properties (widget, data,
		Title, NULL, Position, Modal,
		DefaultWidth, DefaultHeight,
		Shrink, Grow, AutoShrink,
		IconName, FocusOnMap,
		Resizable, DestroyWithParent, Icon,
		Role, TypeHint, SkipTaskbar,
		SkipPager, Decorated, Gravity, Urgency);
}




/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */

static void
gb_gnome_property_box_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gb_window_set_standard_properties (widget, data,
		Title, NULL, Position, Modal,
		DefaultWidth, DefaultHeight,
		Shrink, Grow, AutoShrink,
		IconName, FocusOnMap,
		Resizable, DestroyWithParent, Icon,
		Role, TypeHint, SkipTaskbar,
		SkipPager, Decorated, Gravity, Urgency);
}




/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomePropertyBox, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_gnome_property_box_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_property_box_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *wname, *child_name;

  if (data->create_widget)
    {
      source_add (data, "  %s = gnome_property_box_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  gb_window_write_standard_source (widget, data,
				   Title, NULL, Position, Modal,
				   DefaultWidth, DefaultHeight,
				   Shrink, Grow, AutoShrink,
				   IconName, FocusOnMap,
				   Resizable, DestroyWithParent, Icon,
				   Role, TypeHint, SkipTaskbar,
				   SkipPager, Decorated, Gravity, Urgency);

  /* We output the source code for the children here, since the code should
     not include calls to create the widgets. We need to specify that the
     names used are like: "GTK_DIALOG (<dialog-name>)->vbox".
     We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  child_name = (char*) gtk_widget_get_name (GNOME_PROPERTY_BOX (widget)->notebook);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = GNOME_PROPERTY_BOX (%s)->notebook;\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GNOME_PROPERTY_BOX (widget)->notebook, data);

  g_free (wname);
  data->write_children = FALSE;
}


static GtkWidget *
gb_gnome_property_box_get_child (GtkWidget * widget,
				 const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildGnomePBoxNotebook))
    return GNOME_PROPERTY_BOX (widget)->notebook;
  else
    return NULL;
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_property_box_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_property_box_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_propertybox_xpm;
  gbwidget.tooltip = _("Property Dialog Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_property_box_new;
  gbwidget.gb_widget_write_source	= gb_gnome_property_box_write_source;
  gbwidget.gb_widget_get_child		= gb_gnome_property_box_get_child;

  gbwidget.gb_widget_create_properties	= gb_gnome_property_box_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_property_box_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_property_box_set_properties;
  gbwidget.gb_widget_destroy = gb_window_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_property_box_create_popup_menu;
*/

  return &gbwidget;
}

