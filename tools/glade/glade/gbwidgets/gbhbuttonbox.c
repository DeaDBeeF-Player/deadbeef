
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

#include <config.h>

#ifdef USE_GNOME
#include <gnome.h>
#include "../glade_gnome.h"
#else
#include <gtk/gtkbutton.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkspinbutton.h>
#endif

#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/hbuttonbox.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Size = "HBBox|GtkBox::size";
static gchar *Layout = "HBBox|GtkButtonBox::layout_style";
static gchar *Spacing = "HBBox|GtkButtonBox::spacing";

static const gchar *GbLayoutChoices[] =
{"Default", "Spread", "Edge",
 "Start", "End", NULL};
static const gint GbLayoutValues[] =
{
  GTK_BUTTONBOX_DEFAULT_STYLE,
  GTK_BUTTONBOX_SPREAD,
  GTK_BUTTONBOX_EDGE,
  GTK_BUTTONBOX_START,
  GTK_BUTTONBOX_END
};
static const gchar *GbLayoutSymbols[] =
{
  "GTK_BUTTONBOX_DEFAULT_STYLE",
  "GTK_BUTTONBOX_SPREAD",
  "GTK_BUTTONBOX_EDGE",
  "GTK_BUTTONBOX_START",
  "GTK_BUTTONBOX_END"
};


/* FIXME: Hack - copied from gtkbbox.c. Should use GParam query instead,
   or use an optional int property instead. */
#define DEFAULT_CHILD_MIN_WIDTH 85
#define DEFAULT_CHILD_MIN_HEIGHT 27
#define DEFAULT_CHILD_IPAD_X 4
#define DEFAULT_CHILD_IPAD_Y 0


static void show_hbbox_dialog (GbWidgetNewData * data);
static void on_hbbox_dialog_ok (GtkWidget * widget,
				GbWidgetNewData * data);
static void on_hbbox_dialog_destroy (GtkWidget * widget,
				     GbWidgetNewData * data);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkHButtonBox, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_hbutton_box_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gtk_hbutton_box_new ();
      return new_widget;
    }
  else
    {
      show_hbbox_dialog (data);
      return NULL;
    }
}


static void
show_hbbox_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New horizontal button box"),
				     data->parent,
				     GTK_SIGNAL_FUNC (on_hbbox_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_hbbox_dialog_destroy), data);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Number of columns:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "cols",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 5);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


static void
on_hbbox_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window, *new_child;
  gint cols, i;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "cols");
      g_return_if_fail (spinbutton != NULL);
      cols = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gtk_hbutton_box_new ();
      for (i = 0; i < cols; i++)
	{
	  new_child = gb_widget_new ("GtkButton", new_widget);
	  GTK_WIDGET_SET_FLAGS (new_child, GTK_CAN_DEFAULT);
	  gtk_container_add (GTK_CONTAINER (new_widget), new_child);
	}
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_hbbox_dialog_destroy (GtkWidget * widget,
			 GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_hbutton_box_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				  data)
{
  property_add_int_range (Size, _("Size:"), _("The number of buttons"),
			  0, 1000, 1, 10, 1);
  property_add_choice (Layout, _("Layout:"),
		       _("The layout style of the buttons"),
		       GbLayoutChoices);
  property_add_int_range (Spacing, _("Spacing:"), _("The space between the buttons"),
			  0, 1000, 1, 10, 1);
}



static gboolean
gb_hbutton_box_is_dialog_action_area (GtkWidget *widget)
{
  char *child_name = gb_widget_get_child_name (widget);
  if (child_name && !strcmp (child_name, GladeChildDialogActionArea))
    return TRUE;
  return FALSE;
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_hbutton_box_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GtkButtonBoxStyle layout;
  gint i, spacing;
  gboolean spacing_visible = FALSE;

  if (data->action != GB_SAVING)
    gb_widget_output_int (data, Size, g_list_length (GTK_BOX (widget)->children));

  layout = gtk_button_box_get_layout (GTK_BUTTON_BOX (widget));
  for (i = 0; i < sizeof (GbLayoutValues) / sizeof (GbLayoutValues[0]); i++)
    {
      if (GbLayoutValues[i] == layout)
	gb_widget_output_choice (data, Layout, i, GbLayoutSymbols[i]);
    }

  if (!gb_hbutton_box_is_dialog_action_area (widget))
    {
      spacing_visible = TRUE;
      spacing = gtk_box_get_spacing (GTK_BOX (widget));
      gb_widget_output_int (data, Spacing, spacing);
    }

  if (data->action == GB_SHOWING)
    {
      property_set_visible (Spacing, spacing_visible);
    }
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_hbutton_box_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gint size, i, spacing;
  gchar *layout;
  gboolean queue_resize = FALSE;

  if (data->action != GB_LOADING)
    {
      size = gb_widget_input_int (data, Size);
      if (data->apply)
	gb_box_set_size (widget, size);
    }

  layout = gb_widget_input_choice (data, Layout);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbLayoutValues) / sizeof (GbLayoutValues[0]);
	   i++)
	{
	  if (!strcmp (layout, GbLayoutChoices[i])
	      || !strcmp (layout, GbLayoutSymbols[i]))
	    {
	      gtk_button_box_set_layout (GTK_BUTTON_BOX (widget), GbLayoutValues
					 [i]);
	      queue_resize = TRUE;
	      break;
	    }
	}
    }

  if (!gb_hbutton_box_is_dialog_action_area (widget))
    {
      spacing = gb_widget_input_int (data, Spacing);
      if (data->apply)
	{
	  gtk_box_set_spacing (GTK_BOX (widget), spacing);
	  queue_resize = TRUE;
	}
    }

  if (queue_resize)
    gtk_widget_queue_resize (widget);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkHButtonBox, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_hbutton_box_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_hbutton_box_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkButtonBoxStyle layout_style;
  gint spacing, i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_hbutton_box_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  layout_style = GTK_BUTTON_BOX (widget)->layout_style;
  if (layout_style != GTK_BUTTONBOX_DEFAULT_STYLE)
    {
      for (i = 0; i < sizeof (GbLayoutValues) / sizeof (GbLayoutValues[0]); i
	   ++)
	{
	  if (GbLayoutValues[i] == layout_style)
	    source_add (data,
		 "  gtk_button_box_set_layout (GTK_BUTTON_BOX (%s), %s);\n",
			data->wname, GbLayoutSymbols[i]);
	}
    }

  if (!gb_hbutton_box_is_dialog_action_area (widget))
    {
      spacing = gtk_box_get_spacing (GTK_BOX (widget));
      if (spacing != 0)
	{
	  source_add (data,
		      "  gtk_box_set_spacing (GTK_BOX (%s), %i);\n",
		      data->wname, spacing);
	}
    }
}


/* Outputs source to add a child widget to a hbuttonbox. We need to check if
   the hbuttonbox is a GtkDialog action area, and if it is we use the special
   gtk_dialog_add_action_widget() function to add it. */
void
gb_hbutton_box_write_add_child_source (GtkWidget * parent,
				       const gchar *parent_name,
				       GtkWidget *child,
				       GbWidgetWriteSourceData * data)
{
  if (gb_hbutton_box_is_dialog_action_area (parent)
      && G_OBJECT_TYPE (child) == GTK_TYPE_BUTTON)
    {
      gint response_id;
      char *response_name, *dialog_name;

      response_id = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (child), GladeDialogResponseIDKey));
      response_name = gb_dialog_response_id_to_string (response_id);

      dialog_name = (char*) gtk_widget_get_name (parent->parent->parent);
      dialog_name = source_create_valid_identifier (dialog_name);

      source_add (data,
		  "  gtk_dialog_add_action_widget (GTK_DIALOG (%s), %s, %s);\n",
		  dialog_name, data->wname, response_name);

      g_free (dialog_name);
    }
  else
    {
      /* Use the standard gtk_container_add(). */
      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		  parent_name, data->wname);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_hbutton_box_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_hbutton_box_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = hbuttonbox_xpm;
  gbwidget.tooltip = _("Horizontal Button Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_hbutton_box_new;
  gbwidget.gb_widget_create_properties = gb_hbutton_box_create_properties;
  gbwidget.gb_widget_get_properties = gb_hbutton_box_get_properties;
  gbwidget.gb_widget_set_properties = gb_hbutton_box_set_properties;
  gbwidget.gb_widget_write_source = gb_hbutton_box_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_hbutton_box_write_add_child_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_hbutton_box_create_popup_menu;
 */

  return &gbwidget;
}
