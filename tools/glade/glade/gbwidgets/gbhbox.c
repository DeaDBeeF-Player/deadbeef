
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

#include <gtk/gtkbbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkspinbutton.h>
#include "../gb.h"
#include "../tree.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/hbox.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/* Size is not saved to the XML. */
static const gchar *Size = "HBox|GtkBox::size";
static const gchar *Homogeneous = "HBox|GtkBox::homogeneous";
static const gchar *Spacing = "HBox|GtkBox::spacing";

/* For children of a box */
static const gchar *GbPadding = "GtkBoxChild::padding";
static const gchar *GbExpand = "GtkBoxChild::expand";
static const gchar *GbFill = "GtkBoxChild::fill";
static const gchar *GbPack = "GtkBoxChild::pack_type";

/* This is only used internally - it isn't save in the XML. */
static const gchar *GbPosition = "GtkBoxChild::position";


static void show_hbox_dialog (GbWidgetNewData * data);
static void on_hbox_dialog_ok (GtkWidget * widget,
			       GbWidgetNewData * data);
static void on_hbox_dialog_destroy (GtkWidget * widget,
				    GbWidgetNewData * data);

static void gb_box_insert_before (GtkWidget * menuitem, GtkWidget * child);
static void gb_box_insert_after (GtkWidget * menuitem, GtkWidget * child);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkHBox, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_hbox_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gtk_hbox_new (FALSE, 0);
      return new_widget;
    }
  else
    {
      show_hbox_dialog (data);
      return NULL;
    }
}


static void
show_hbox_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New horizontal box"), data->parent,
				     GTK_SIGNAL_FUNC (on_hbox_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_hbox_dialog_destroy), data);

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
on_hbox_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window, *placeholder;
  gint cols, i;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "cols");
      g_return_if_fail (spinbutton != NULL);
      cols = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gtk_hbox_new (FALSE, 0);
      for (i = 0; i < cols; i++)
	{
	  placeholder = editor_new_placeholder ();
	  /*gtk_widget_set_usize(placeholder, 60, 80); */
	  gtk_box_pack_start (GTK_BOX (new_widget), placeholder, TRUE, TRUE, 0);
	}
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_hbox_dialog_destroy (GtkWidget * widget,
			GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_hbox_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
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
gb_hbox_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
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
gb_hbox_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
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
gb_hbox_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_hbox_new (%s, %i);\n", data->wname,
		  GTK_BOX (widget)->homogeneous ? "TRUE" : "FALSE",
		  GTK_BOX (widget)->spacing);
    }

  gb_widget_write_standard_source (widget, data);
}


/*
 * Functions common to GtkHBox & GtkVBox (and possibly GtkHButtonBox
 * & GtkVButtonBox).
 */


/* This updates the box size to the given value, adding placeholders or
   deleting widgets as necessary. */
void
gb_box_set_size (GtkWidget * widget, gint size)
{
  GtkWidget *new_child;
  gint current_size = g_list_length (GTK_BOX (widget)->children);
  gint i;

  if (current_size < size)
    {
      /* This avoids any problems with redrawing the selection. */
      editor_clear_selection (NULL);

      for (i = 0; i < size - current_size; i++)
	{
	  if (GTK_IS_BUTTON_BOX (widget))
	    {
	      new_child = gb_widget_new ("GtkButton", widget);
	      GTK_WIDGET_SET_FLAGS (new_child, GTK_CAN_DEFAULT);
	      gtk_box_pack_start (GTK_BOX (widget), new_child, TRUE, TRUE, 0);
	      tree_add_widget (new_child);
	    }
	  else
	    {
	      new_child = editor_new_placeholder ();
	      gtk_box_pack_start (GTK_BOX (widget), new_child, TRUE, TRUE, 0);
	    }
	}
    }
  else if (current_size > size)
    {
      GList *children, *elem;
      GtkWidget *child;
      gchar *error = NULL;

      /* Get a list of children in the order they appear in the box, start at
	 the end and move backwards until we find a widget that can be
	 destroyed. If we can't find any, show a message box. */
      children = gtk_container_get_children (GTK_CONTAINER (widget));
      elem = g_list_last (children);

      while (elem)
	{
	  child = elem->data;
	  error = editor_can_delete_widget (child);
	  if (!error)
	    {
	      gtk_container_remove (GTK_CONTAINER (widget), child);
	      current_size--;
	      if (current_size == size)
		break;
	    }
	  elem = elem->prev;
	}

      g_list_free (children);

      /* If we show an error dialog it causes weird problems with the
	 spinbutton used to change the size. So we don't do this. */
#if 0
      if (current_size > size)
	{
	  glade_util_show_message_box (error ? error
				       : _("Can't delete any children."),
				       widget);
	}
#endif
    }
}


/*
 * Creates the child packing properties for children of this widget.
 */
void
gb_box_create_child_properties (GtkWidget * widget,
				GbWidgetCreateChildArgData * data)
{
  property_add_int_range (GbPosition, _("Position:"),
			  _("The widget's position relative to its siblings"),
			  0, 10000, 1, 10, 1);
  property_add_int_range (GbPadding, _("Padding:"),
			  _("The widget's padding"),
			  0, 10000, 1, 10, 1);
  property_add_bool (GbExpand, _("Expand:"),
		     _("Set True to let the widget expand"));
  property_add_bool (GbFill, _("Fill:"),
		     _("Set True to let the widget fill its allocated area"));
  property_add_bool (GbPack, _("Pack Start:"),
		     _("Set True to pack the widget at the start of the box"));
}


void
gb_box_get_child_properties (GtkWidget *widget, GtkWidget *child,
			     GbWidgetGetArgData *data)
{
  gboolean expand, fill;
  gint position;
  guint padding;
  GtkPackType pack_type;

  if (data->action == GB_SAVING)
    save_start_tag (data, "packing");

  if (data->action == GB_SHOWING)
    {
      position = glade_util_get_box_pos (GTK_BOX (widget), child);
      gb_widget_output_int (data, GbPosition, position);
    }

  gtk_box_query_child_packing (GTK_BOX (widget), child,
			       &expand, &fill, &padding, &pack_type);
  gb_widget_output_int (data, GbPadding, padding);
  gb_widget_output_bool (data, GbExpand, expand);
  gb_widget_output_bool (data, GbFill, fill);

  if (data->action == GB_SAVING)
    {
      /* Save pack type as an enum symbol rather than a bool */
      if (pack_type == GTK_PACK_END)
	{
	  gb_widget_output_string (data, GbPack, "GTK_PACK_END");
	}
    }
  else
    {
      gb_widget_output_bool (data, GbPack, pack_type == GTK_PACK_START);
    }

  if (data->action == GB_SAVING)
    save_end_tag (data, "packing");
}


/* Applies or loads the child properties of a child of a hbox/vbox. */
void
gb_box_set_child_properties (GtkWidget *widget, GtkWidget *child,
			     GbWidgetSetArgData *data)
{
  gint position, padding;
  guint old_padding;
  gboolean expand, fill, pack, set_child_packing = FALSE;
  gboolean old_expand, old_fill;
  GtkPackType old_pack_type;

  position = gb_widget_input_int (data, GbPosition);
  if (data->apply)
    {
      gtk_box_reorder_child (GTK_BOX (widget), child, position);
    }

  gtk_box_query_child_packing (GTK_BOX (widget), child,
			       &old_expand, &old_fill, &old_padding,
			       &old_pack_type);

  padding = gb_widget_input_int (data, GbPadding);
  if (data->apply)
    set_child_packing = TRUE;
  else
    padding = old_padding;

  expand = gb_widget_input_bool (data, GbExpand);
  if (data->apply)
    set_child_packing = TRUE;
  else
    expand = old_expand;

  fill = gb_widget_input_bool (data, GbFill);
  if (data->apply)
    set_child_packing = TRUE;
  else
    fill = old_fill;

  if (data->action == GB_APPLYING)
    {
      pack = gb_widget_input_bool (data, GbPack);
    }
  else
    {
      gchar *pack_symbol = gb_widget_input_string (data, GbPack);
      pack = pack_symbol && !strcmp (pack_symbol, "GTK_PACK_START");
    }
  if (data->apply)
    set_child_packing = TRUE;
  else
    pack = (old_pack_type == GTK_PACK_START) ? TRUE : FALSE;

  if (set_child_packing)
    gtk_box_set_child_packing (GTK_BOX (widget), child, expand, fill, padding,
                               pack ? GTK_PACK_START : GTK_PACK_END);
}

/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkHBox, with signals pointing to
 * other functions in this file.
 */
void
gb_box_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;

  if (data->child == NULL)
    return;

  menuitem = gtk_menu_item_new_with_label (_("Insert Before"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_box_insert_before), data->child);

  menuitem = gtk_menu_item_new_with_label (_("Insert After"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_box_insert_after), data->child);
}


static void
gb_box_insert_before (GtkWidget * menuitem, GtkWidget * child)
{
  GtkWidget *box, *newchild;
  guint pos;
  gboolean expand, fill;
  guint padding;
  GtkPackType pack_type;

  box = child->parent;
  pos = glade_util_get_box_pos (GTK_BOX (box), child);
  g_return_if_fail (pos != -1);
  newchild = editor_new_placeholder ();
  gtk_box_query_child_packing (GTK_BOX (box), child,
			       &expand, &fill, &padding, &pack_type);
  if (pack_type == GTK_PACK_START)
    {
      gtk_box_pack_start (GTK_BOX (box), newchild, TRUE, TRUE, 0);
      gtk_box_reorder_child (GTK_BOX (box), newchild, pos);
    }
  else
    {
      gtk_box_pack_end (GTK_BOX (box), newchild, TRUE, TRUE, 0);
      gtk_box_reorder_child (GTK_BOX (box), newchild, pos + 1);
    }
}

static void
gb_box_insert_after (GtkWidget * menuitem, GtkWidget * child)
{
  GtkWidget *box, *newchild;
  guint pos;
  gboolean expand, fill;
  guint padding;
  GtkPackType pack_type;

  box = child->parent;
  pos = glade_util_get_box_pos (GTK_BOX (box), child);
  g_return_if_fail (pos != -1);
  newchild = editor_new_placeholder ();
  gtk_box_query_child_packing (GTK_BOX (box), child,
			       &expand, &fill, &padding, &pack_type);
  if (pack_type == GTK_PACK_START)
    {
      gtk_box_pack_start (GTK_BOX (box), newchild, TRUE, TRUE, 0);
      gtk_box_reorder_child (GTK_BOX (box), newchild, pos + 1);
    }
  else
    {
      gtk_box_pack_end (GTK_BOX (box), newchild, TRUE, TRUE, 0);
      gtk_box_reorder_child (GTK_BOX (box), newchild, pos);
    }
}


/* Outputs source to add a child widget to a hbox/vbox. */
void
gb_box_write_add_child_source (GtkWidget * parent,
			       const gchar *parent_name,
			       GtkWidget *child,
			       GbWidgetWriteSourceData * data)
{
  gboolean expand, fill;
  guint padding;
  GtkPackType pack_type;

  gtk_box_query_child_packing (GTK_BOX (parent), child,
			       &expand, &fill, &padding, &pack_type);
  if (pack_type == GTK_PACK_START)
    {
      source_add (data,
		  "  gtk_box_pack_start (GTK_BOX (%s), %s, %s, %s, %i);\n",
		  parent_name, data->wname,
		  expand ? "TRUE" : "FALSE", fill ? "TRUE" : "FALSE", padding);
    }
  else
    {
      source_add (data,
		  "  gtk_box_pack_end (GTK_BOX (%s), %s, %s, %s, %i);\n",
		  parent_name, data->wname,
		  expand ? "TRUE" : "FALSE", fill ? "TRUE" : "FALSE", padding);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_hbox_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_hbox_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = hbox_xpm;
  gbwidget.tooltip = _("Horizontal Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_hbox_new;
  gbwidget.gb_widget_create_properties = gb_hbox_create_properties;
  gbwidget.gb_widget_get_properties = gb_hbox_get_properties;
  gbwidget.gb_widget_set_properties = gb_hbox_set_properties;
  gbwidget.gb_widget_write_source = gb_hbox_write_source;

  gbwidget.gb_widget_create_child_properties = gb_box_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_box_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_box_set_child_properties;
  gbwidget.gb_widget_create_popup_menu = gb_box_create_popup_menu;
  gbwidget.gb_widget_write_add_child_source = gb_box_write_add_child_source;

  return &gbwidget;
}
