
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

#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktable.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/table.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static const gchar *Rows = "GtkTable::n_rows";
static const gchar *Columns = "GtkTable::n_columns";
static const gchar *Homogeneous = "GtkTable::homogeneous";
static const gchar *RowSpacing = "GtkTable::row_spacing";
static const gchar *ColSpacing = "GtkTable::column_spacing";

/* For children of a table */
static const gchar *GbCellX = "GtkTableChild::cell_x";
static const gchar *GbCellY = "GtkTableChild::cell_y";
static const gchar *GbColSpan = "GtkTableChild::col_span";
static const gchar *GbRowSpan = "GtkTableChild::row_span";
static const gchar *GbXPad = "GtkTableChild::x_padding";
static const gchar *GbYPad = "GtkTableChild::y_padding";
static const gchar *GbXExpand = "GtkTableChild::xexpand";
static const gchar *GbYExpand = "GtkTableChild::yexpand";
static const gchar *GbXShrink = "GtkTableChild::xshrink";
static const gchar *GbYShrink = "GtkTableChild::yshrink";
static const gchar *GbXFill = "GtkTableChild::xfill";
static const gchar *GbYFill = "GtkTableChild::yfill";

/* The Expand, Shrink and Fill get merged into one property when saved. */
static const gchar *GbXOptions = "GtkTableChild::x_options";
static const gchar *GbYOptions = "GtkTableChild::y_options";

/* These are used to return what is in a table cell */
#define GB_CELL_EMPTY		1
#define GB_CELL_WIDGET		2
#define GB_CELL_PLACEHOLDER	3

static void show_table_dialog (GbWidgetNewData * data);
static void on_table_dialog_ok (GtkWidget * widget,
				GbWidgetNewData * data);
static void on_table_dialog_destroy (GtkWidget * widget,
				     GbWidgetNewData * data);

static void update_table_size (GtkWidget * table, gint rows, gint cols);

static void gb_table_insert_row_before (GtkWidget * menuitem,
					GtkWidget * widget);
static void gb_table_insert_row_after (GtkWidget * menuitem,
				       GtkWidget * widget);
static void gb_table_insert_column_before (GtkWidget * menuitem,
					   GtkWidget * widget);
static void gb_table_insert_column_after (GtkWidget * menuitem,
					  GtkWidget * widget);
static void gb_table_insert_row_or_col (GtkWidget * table, gint row, gint col);
static void gb_table_delete_row (GtkWidget * menuitem, GtkWidget * widget);
static void gb_table_delete_column (GtkWidget * menuitem, GtkWidget * widget);

static gint gb_table_is_cell_occupied (GtkWidget * table, gint row, gint col);
static void gb_table_remove_placeholders (GtkWidget * table,
					  gint row, gint col);
static void gb_table_split_placeholder (GtkWidget * table,
					GtkWidget * placeholder,
					gint left, gint right,
					gint top, gint bottom,
					gint skip_row, gint skip_col);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkTable, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_table_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;
  gint rows, cols;

  if (data->action == GB_LOADING)
    {
      rows = load_int (data->loading_data, Rows);
      cols = load_int (data->loading_data, Columns);
      new_widget = gtk_table_new (rows, cols, FALSE);
      return new_widget;
    }
  else
    {
      show_table_dialog (data);
      return NULL;
    }
}


static void
show_table_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *table, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New table"), data->parent,
				     GTK_SIGNAL_FUNC (on_table_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_table_dialog_destroy), data);

  /* Rows label & entry */
  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_widget_show (table);

  label = gtk_label_new (_("Number of rows:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "rows",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 4);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  /* Columns label & entry */
  label = gtk_label_new (_("Number of columns:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "cols",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 4, 4);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_show (spinbutton);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


static void
on_table_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window;
  gint rows, cols, row, col;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "rows");
      g_return_if_fail (spinbutton != NULL);
      rows = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "cols");
      g_return_if_fail (spinbutton != NULL);
      cols = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gtk_table_new (rows, cols, FALSE);
      for (row = 0; row < rows; row++)
	{
	  for (col = 0; col < cols; col++)
	    {
	      gtk_table_attach (GTK_TABLE (new_widget),
				editor_new_placeholder (),
				col, col + 1, row, row + 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);
	    }
	}
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_table_dialog_destroy (GtkWidget * widget,
			 GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_table_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_int_range (Rows, _("Rows:"),
			  _("The number of rows in the table"),
			  1, 1000, 1, 10, 1);
  property_add_int_range (Columns, _("Columns:"),
			  _("The number of columns in the table"),
			  1, 1000, 1, 10, 1);
  property_add_bool (Homogeneous, _("Homogeneous:"),
		     _("If the children should all be the same size"));
  property_add_int_range (RowSpacing, _("Row Spacing:"),
			  _("The space between each row"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (ColSpacing, _("Col Spacing:"),
			  _("The space between each column"),
			  0, 1000, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_table_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_widget_output_int (data, Rows, GTK_TABLE (widget)->nrows);
  gb_widget_output_int (data, Columns, GTK_TABLE (widget)->ncols);
  gb_widget_output_bool (data, Homogeneous, GTK_TABLE (widget)->homogeneous);
  gb_widget_output_int (data, RowSpacing, GTK_TABLE (widget)->row_spacing);
  gb_widget_output_int (data, ColSpacing, GTK_TABLE (widget)->column_spacing);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_table_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean homogeneous, update_size = FALSE;
  gint rows, cols, row_spacing, column_spacing;

  rows = gb_widget_input_int (data, Rows);
  if (data->apply)
    update_size = TRUE;
  else
    rows = GTK_TABLE (widget)->nrows;

  cols = gb_widget_input_int (data, Columns);
  if (data->apply)
    update_size = TRUE;
  else
    cols = GTK_TABLE (widget)->ncols;

  if (update_size)
    update_table_size (widget, rows, cols);

  homogeneous = gb_widget_input_bool (data, Homogeneous);
  if (data->apply)
    gtk_table_set_homogeneous (GTK_TABLE (widget), homogeneous);

  row_spacing = gb_widget_input_int (data, RowSpacing);
  if (data->apply)
    gtk_table_set_row_spacings (GTK_TABLE (widget), row_spacing);

  column_spacing = gb_widget_input_int (data, ColSpacing);
  if (data->apply)
    gtk_table_set_col_spacings (GTK_TABLE (widget), column_spacing);
}


/* This changes the size of the table to the given dimensions. Placeholders
   are added in empty cells, and widgets are destroyed if they fall outside
   the new dimensions.
   NOTE: this changes the table data structures, which we would prefer not to
   do, but there is no alternative. */
static void
update_table_size (GtkWidget * table, gint rows, gint cols)
{
  GList *children;
  GtkTableChild *child;

  g_return_if_fail (rows > 0);
  g_return_if_fail (cols > 0);

  if (GTK_TABLE (table)->nrows == rows && GTK_TABLE (table)->ncols == cols)
    return;

  children = GTK_TABLE (table)->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      /* Remove the widget if it doesn't fit into the new dimensions,
         or crop it if it extends past the edges. */
      if (child->left_attach >= cols || child->top_attach >= rows)
	{
	  gtk_container_remove (GTK_CONTAINER (table), child->widget);
	}
      else
	{
	  if (child->right_attach > cols)
	    child->right_attach = cols;
	  if (child->bottom_attach > rows)
	    child->bottom_attach = rows;
	}
    }

  /* This is in ../gbwidget.c. It will expand the dimensions if necessary. */
  gb_table_update_placeholders (table, rows, cols);

  /* This bit is especially naughty. It makes sure the table shrinks,
     so we don't get extra spacings on the end. */
  GTK_TABLE (table)->nrows = rows;
  GTK_TABLE (table)->ncols = cols;

  /* We clear the selection since it isn't displayed properly. */
  editor_clear_selection (NULL);
}


/*
 * Creates the child packing properties for children of this widget.
 */
static void
gb_table_create_child_properties (GtkWidget * widget,
				  GbWidgetCreateChildArgData * data)
{
  property_add_int_range (GbCellX, _("Cell X:"),
			  _("The left edge of the widget in the table"),
			  0, 10000, 1, 10, 1);
  property_add_int_range (GbCellY, _("Cell Y:"),
			  _("The top edge of the widget in the table"),
			  0, 10000, 1, 10, 1);

  property_add_int_range (GbColSpan, _("Col Span:"),
		 _("The number of columns spanned by the widget in the table"),
			  1, 10000, 1, 10, 1);
  property_add_int_range (GbRowSpan, _("Row Span:"),
		    _("The number of rows spanned by the widget in the table"),
			  1, 10000, 1, 10, 1);
  property_add_int_range (GbXPad, _("H Padding:"),
			  _("The horizontal padding"),
			  0, 10000, 1, 10, 1);
  property_add_int_range (GbYPad, _("V Padding:"),
			  _("The vertical padding"),
			  0, 10000, 1, 10, 1);
  property_add_bool (GbXExpand, _("X Expand:"),
		     _("Set True to let the widget expand horizontally"));
  property_add_bool (GbYExpand, _("Y Expand:"),
		     _("Set True to let the widget expand vertically"));
  property_add_bool (GbXShrink, _("X Shrink:"),
		     _("Set True to let the widget shrink horizontally"));
  property_add_bool (GbYShrink, _("Y Shrink:"),
		     _("Set True to let the widget shrink vertically"));
  property_add_bool (GbXFill, _("X Fill:"),
	   _("Set True to let the widget fill its horizontal allocated area"));
  property_add_bool (GbYFill, _("Y Fill:"),
	     _("Set True to let the widget fill its vertical allocated area"));
}


static void
gb_table_get_child_properties (GtkWidget *widget, GtkWidget *child,
			       GbWidgetGetArgData *data)
{
  GtkTableChild *table_child;

  table_child = glade_util_find_table_child (GTK_TABLE (widget), child);
  g_return_if_fail (table_child != NULL);

  if (data->action == GB_SAVING)
    save_start_tag (data, "packing");

  if (data->action == GB_SAVING)
    {
      /* We use left/right/top/bottom_attach here as they are used in GTK */
      gb_widget_output_int (data, "GtkWidget::left_attach",
			    table_child->left_attach);
      gb_widget_output_int (data, "GtkWidget::right_attach",
			    table_child->right_attach);
      gb_widget_output_int (data, "GtkWidget::top_attach",
			    table_child->top_attach);
      gb_widget_output_int (data, "GtkWidget::bottom_attach",
			    table_child->bottom_attach);
    }
  else
    {
      gb_widget_output_int (data, GbCellX, table_child->left_attach);
      gb_widget_output_int (data, GbCellY, table_child->top_attach);
      gb_widget_output_int (data, GbColSpan,
			    table_child->right_attach
			    - table_child->left_attach);
      gb_widget_output_int (data, GbRowSpan,
			    table_child->bottom_attach
			    - table_child->top_attach);
    }

  /* Default X & Y padding is 0. */
  if (data->action != GB_SAVING || table_child->xpadding)
    gb_widget_output_int (data, GbXPad, table_child->xpadding);
  if (data->action != GB_SAVING || table_child->ypadding)
    gb_widget_output_int (data, GbYPad, table_child->ypadding);

  if (data->action == GB_SAVING)
    {
      char buffer[32];

      /* Default packing is GTK_EXPAND | GTK_FILL. */
      if (!table_child->xexpand || !table_child->xfill || table_child->xshrink)
	{
	  buffer[0] = '\0';
	  if (table_child->xexpand)
	    strcat (buffer, "expand");
	  if (table_child->xshrink)
	    {
	      if (buffer[0])
		strcat (buffer, "|");
	      strcat (buffer, "shrink");
	    }
	  if (table_child->xfill)
	    {
	      if (buffer[0])
		strcat (buffer, "|");
	      strcat (buffer, "fill");
	    }
	  gb_widget_output_string (data, GbXOptions, buffer);
	}

      if (!table_child->yexpand || !table_child->yfill || table_child->yshrink)
	{
	  buffer[0] = '\0';
	  if (table_child->yexpand)
	    strcat (buffer, "expand");
	  if (table_child->yshrink)
	    {
	      if (buffer[0])
		strcat (buffer, "|");
	      strcat (buffer, "shrink");
	    }
	  if (table_child->yfill)
	    {
	      if (buffer[0])
		strcat (buffer, "|");
	      strcat (buffer, "fill");
	    }
	  gb_widget_output_string (data, GbYOptions, buffer);
	}
    }
  else
    {
      gb_widget_output_bool (data, GbXExpand, table_child->xexpand);
      gb_widget_output_bool (data, GbYExpand, table_child->yexpand);
      gb_widget_output_bool (data, GbXShrink, table_child->xshrink);
      gb_widget_output_bool (data, GbYShrink, table_child->yshrink);
      gb_widget_output_bool (data, GbXFill, table_child->xfill);
      gb_widget_output_bool (data, GbYFill, table_child->yfill);
    }

  if (data->action == GB_SAVING)
    save_end_tag (data, "packing");
}


static void
gb_table_set_child_properties (GtkWidget *widget, GtkWidget *child,
			       GbWidgetSetArgData *data)
{
  GtkTableChild *tchild;
  gint xpad, ypad, left_attach, right_attach, top_attach, bottom_attach;
  gint xexpand, yexpand, xshrink, yshrink, xfill, yfill;

  tchild = glade_util_find_table_child (GTK_TABLE (widget), child);
  g_return_if_fail (tchild != NULL);

  xpad = gb_widget_input_int (data, GbXPad);
  if (!data->apply)
    xpad = tchild->xpadding;

  ypad = gb_widget_input_int (data, GbYPad);
  if (!data->apply)
    ypad = tchild->ypadding;

  if (data->action == GB_APPLYING)
    {
      xexpand = (gb_widget_input_bool (data, GbXExpand)) ? GTK_EXPAND : 0;
      if (!data->apply)
	xexpand = tchild->xexpand ? GTK_EXPAND : 0;

      yexpand = (gb_widget_input_bool (data, GbYExpand)) ? GTK_EXPAND : 0;
      if (!data->apply)
	yexpand = tchild->yexpand ? GTK_EXPAND : 0;

      xshrink = (gb_widget_input_bool (data, GbXShrink)) ? GTK_SHRINK : 0;
      if (!data->apply)
	xshrink = tchild->xshrink ? GTK_SHRINK : 0;

      yshrink = (gb_widget_input_bool (data, GbYShrink)) ? GTK_SHRINK : 0;
      if (!data->apply)
	yshrink = tchild->yshrink ? GTK_SHRINK : 0;

      xfill = (gb_widget_input_bool (data, GbXFill)) ? GTK_FILL : 0;
      if (!data->apply)
	xfill = tchild->xfill ? GTK_FILL : 0;

      yfill = (gb_widget_input_bool (data, GbYFill)) ? GTK_FILL : 0;
      if (!data->apply)
	yfill = tchild->yfill ? GTK_FILL : 0;
    }
  else
    {
      char *xoptions, *yoptions;

      xoptions = gb_widget_input_string (data, GbXOptions);
      if (data->apply)
	{
	  xexpand = (strstr (xoptions, "expand")) ? GTK_EXPAND : 0;
	  xfill   = (strstr (xoptions, "fill")) ? GTK_FILL : 0;
	  xshrink = (strstr (xoptions, "shrink")) ? GTK_SHRINK : 0;
	}
      else
	{
	  xexpand = GTK_EXPAND;
	  xfill = GTK_FILL;
	  xshrink = 0;
	}

      yoptions = gb_widget_input_string (data, GbYOptions);
      if (data->apply)
	{
	  yexpand = (strstr (yoptions, "expand")) ? GTK_EXPAND : 0;
	  yfill   = (strstr (yoptions, "fill")) ? GTK_FILL : 0;
	  yshrink = (strstr (yoptions, "shrink")) ? GTK_SHRINK : 0;
	}
      else
	{
	  yexpand = GTK_EXPAND;
	  yfill = GTK_FILL;
	  yshrink = 0;
	}
    }

  if (data->action == GB_APPLYING)
    {
      left_attach = gb_widget_input_int (data, GbCellX);
      if (!data->apply)
	left_attach = tchild->left_attach;

      top_attach = gb_widget_input_int (data, GbCellY);
      if (!data->apply)
	top_attach = tchild->top_attach;

      right_attach = gb_widget_input_int (data, GbColSpan) + left_attach;
      if (!data->apply)
	right_attach = left_attach + (tchild->right_attach
				      - tchild->left_attach);

      bottom_attach = gb_widget_input_int (data, GbRowSpan) + top_attach;
      if (!data->apply)
	bottom_attach = top_attach + (tchild->bottom_attach
				      - tchild->top_attach);
    }
  else
    {
      left_attach = gb_widget_input_int (data, "GtkWidget::left_attach");
      if (!data->apply)
	left_attach = tchild->left_attach;

      top_attach = gb_widget_input_int (data, "GtkWidget::top_attach");
      if (!data->apply)
	top_attach = tchild->top_attach;

      right_attach = gb_widget_input_int (data, "GtkWidget::right_attach");
      if (!data->apply)
	right_attach = tchild->right_attach;

      bottom_attach = gb_widget_input_int (data, "GtkWidget::bottom_attach");
      if (!data->apply)
	bottom_attach = tchild->bottom_attach;
    }

  if (right_attach <= left_attach)
    right_attach = left_attach + 1;
  if (bottom_attach <= top_attach)
    bottom_attach = top_attach + 1;

  if (xpad != tchild->xpadding || ypad != tchild->ypadding
      || (xexpand && !tchild->xexpand) || (!xexpand && tchild->xexpand)
      || (yexpand && !tchild->yexpand) || (!yexpand && tchild->yexpand)
      || (xshrink && !tchild->xshrink) || (!xshrink && tchild->xshrink)
      || (yshrink && !tchild->yshrink) || (!yshrink && tchild->yshrink)
      || (xfill   && !tchild->xfill)   || (!xfill   && tchild->xfill)
      || (yfill   && !tchild->yfill)   || (!yfill   && tchild->yfill)
      || left_attach   != tchild->left_attach
      || right_attach  != tchild->right_attach
      || top_attach    != tchild->top_attach
      || bottom_attach != tchild->bottom_attach)
    {
      gtk_widget_ref (child);
      gtk_container_remove (GTK_CONTAINER (widget), child);
      gtk_table_attach (GTK_TABLE (widget), child,
			left_attach, right_attach, top_attach, bottom_attach,
			xexpand | xshrink | xfill, yexpand | yshrink | yfill,
			xpad, ypad);
      gtk_widget_unref (child);
      if (data->action == GB_APPLYING)
	gb_table_update_placeholders (widget, -1, -1);
    }
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkTable, with signals pointing to
 * other functions in this file.
 */
static void
gb_table_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;

  /* It is possible that the mouse button was clicked outside a child
     (e.g. beneath all the children), in which case we return. We could
     still support adding rows/cols. */
  if (data->child == NULL)
    return;

  menuitem = gtk_menu_item_new_with_label (_("Insert Row Before"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_table_insert_row_before),
		      data->child);

  menuitem = gtk_menu_item_new_with_label (_("Insert Row After"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_table_insert_row_after),
		      data->child);

  menuitem = gtk_menu_item_new_with_label (_("Insert Column Before"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_table_insert_column_before),
		      data->child);

  menuitem = gtk_menu_item_new_with_label (_("Insert Column After"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_table_insert_column_after),
		      data->child);

  menuitem = gtk_menu_item_new_with_label (_("Delete Row"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_table_delete_row), data->child);

  menuitem = gtk_menu_item_new_with_label (_("Delete Column"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_table_delete_column), data->child);
}


static void
gb_table_insert_row_before (GtkWidget * menuitem, GtkWidget * widget)
{
  GtkWidget *table;
  GtkTableChild *tchild;

  table = widget->parent;
  g_return_if_fail (GTK_IS_TABLE (table));
  tchild = glade_util_find_table_child (GTK_TABLE (table), widget);
  g_return_if_fail (tchild != NULL);
  gb_table_insert_row_or_col (table, tchild->top_attach, -1);
}


static void
gb_table_insert_row_after (GtkWidget * menuitem, GtkWidget * widget)
{
  GtkWidget *table;
  GtkTableChild *tchild;

  table = widget->parent;
  g_return_if_fail (GTK_IS_TABLE (table));
  tchild = glade_util_find_table_child (GTK_TABLE (table), widget);
  g_return_if_fail (tchild != NULL);
  gb_table_insert_row_or_col (table, tchild->bottom_attach, -1);
}


static void
gb_table_insert_column_before (GtkWidget * menuitem, GtkWidget * widget)
{
  GtkWidget *table;
  GtkTableChild *tchild;

  table = widget->parent;
  g_return_if_fail (GTK_IS_TABLE (table));
  tchild = glade_util_find_table_child (GTK_TABLE (table), widget);
  g_return_if_fail (tchild != NULL);
  gb_table_insert_row_or_col (table, -1, tchild->left_attach);
}


static void
gb_table_insert_column_after (GtkWidget * menuitem, GtkWidget * widget)
{
  GtkWidget *table;
  GtkTableChild *tchild;

  table = widget->parent;
  g_return_if_fail (GTK_IS_TABLE (table));
  tchild = glade_util_find_table_child (GTK_TABLE (table), widget);
  g_return_if_fail (tchild != NULL);
  gb_table_insert_row_or_col (table, -1, tchild->right_attach);
}


/* This inserts a row or column into the table at the given position.
   Use -1 for the other unused argument. */
static void
gb_table_insert_row_or_col (GtkWidget * table, gint row, gint col)
{
  GtkTableChild *tchild;
  GList *child;
  GtkWidget *widget, *tmp_label;
  gint rows, cols;

  /* This relies on knowing the internals of GtkTable, so it is fast.
     First it adds a simple label at the bottom right of the new table size,
     to ensure that the table grows. Then it removes the label, and moves
     all the widgets down/right as appropriate, and adds any necessary
     placeholders. */
  tmp_label = gtk_label_new ("");
  rows = GTK_TABLE (table)->nrows + (row != -1 ? 1 : 0);
  cols = GTK_TABLE (table)->ncols + (col != -1 ? 1 : 0);
  gtk_table_attach_defaults (GTK_TABLE (table), tmp_label,
			     cols - 1, cols, rows - 1, rows);

  child = GTK_TABLE (table)->children;
  while (child)
    {
      tchild = child->data;
      child = child->next;
      widget = tchild->widget;

      if ((row != -1 && tchild->top_attach >= row)
	  || (col != -1 && tchild->left_attach >= col))
	{
	  /* If we're inserting a row, we move the widget down.
	     If we're inserting a col, we move the widget right. */
	  if (row != -1)
	    {
	      tchild->top_attach++;
	      tchild->bottom_attach++;
	    }
	  else
	    {
	      tchild->left_attach++;
	      tchild->right_attach++;
	    }
	}
    }

  /* Now remove the temporary label. */
  gtk_container_remove (GTK_CONTAINER (table), tmp_label);

  /* This fills any empty cells with placeholders. */
  gb_table_update_placeholders (table, -1, -1);

  /* If the tables properties are currently shown, update rows/cols. */
  if (property_get_widget () == table)
    {
      property_set_auto_apply (FALSE);
      if (row != -1)
	property_set_int (Rows, GTK_TABLE (table)->nrows);
      else
	property_set_int (Columns, GTK_TABLE (table)->ncols);
      property_set_auto_apply (TRUE);
    }
}


static void
gb_table_delete_row (GtkWidget * menuitem, GtkWidget * widget)
{
  GtkWidget *table;
  GtkTableChild *tchild;
  GList *children;
  guint16 nrows, ncols, row;

  table = widget->parent;
  nrows = GTK_TABLE (table)->nrows - 1;
  if (nrows == 0)
    return;
  ncols = GTK_TABLE (table)->ncols;
  tchild = glade_util_find_table_child (GTK_TABLE (table), widget);
  g_return_if_fail (tchild != NULL);
  row = tchild->top_attach;

  children = GTK_TABLE (table)->children;
  while (children)
    {
      tchild = children->data;
      children = children->next;
      if (tchild->top_attach == row && tchild->bottom_attach == row + 1)
	{
	  gtk_container_remove (GTK_CONTAINER (table), tchild->widget);
	}
      else if (tchild->top_attach <= row && tchild->bottom_attach > row)
	{
	  tchild->bottom_attach -= 1;
	}
      else if (tchild->top_attach > row)
	{
	  tchild->top_attach -= 1;
	  tchild->bottom_attach -= 1;
	}
    }
  update_table_size (table, nrows, ncols);

  if (property_get_widget () == table)
    {
      property_set_auto_apply (FALSE);
      property_set_int (Rows, nrows);
      property_set_auto_apply (TRUE);
    }
}


static void
gb_table_delete_column (GtkWidget * menuitem, GtkWidget * widget)
{
  GtkWidget *table;
  GtkTableChild *tchild;
  GList *children;
  guint16 nrows, ncols, col;

  table = widget->parent;
  nrows = GTK_TABLE (table)->nrows;
  ncols = GTK_TABLE (table)->ncols - 1;
  if (ncols == 0)
    return;
  tchild = glade_util_find_table_child (GTK_TABLE (table), widget);
  g_return_if_fail (tchild != NULL);
  col = tchild->left_attach;

  children = GTK_TABLE (table)->children;
  while (children)
    {
      tchild = children->data;
      children = children->next;
      if (tchild->left_attach == col && tchild->right_attach == col + 1)
	{
	  gtk_container_remove (GTK_CONTAINER (table), tchild->widget);
	}
      else if (tchild->left_attach <= col && tchild->right_attach > col)
	{
	  tchild->right_attach -= 1;
	}
      else if (tchild->left_attach > col)
	{
	  tchild->left_attach -= 1;
	  tchild->right_attach -= 1;
	}
    }
  update_table_size (table, nrows, ncols);

  if (property_get_widget () == table)
    {
      property_set_auto_apply (FALSE);
      property_set_int (Columns, ncols);
      property_set_auto_apply (TRUE);
    }
}


/* This ensures that placeholders are placed in every unoccupied cell.
   If rows and cols are not -1, then they are the new dimensions of the table.
   FIXME: This is very inefficient.
 */
void
gb_table_update_placeholders (GtkWidget * table, gint rows, gint cols)
{
  gint row, col, cell_contents;
  guchar *rows_occupied, *cols_occupied;
  GList *children;
  GtkTableChild *child;

  if (rows == -1)
    rows = GTK_TABLE (table)->nrows;
  if (cols == -1)
    cols = GTK_TABLE (table)->ncols;

  /* These hold flags to indicate which rows/cols have widgets in them. */
  rows_occupied = g_new0 (guchar, rows);
  cols_occupied = g_new0 (guchar, cols);

  for (row = 0; row < rows; row++)
    {
      for (col = 0; col < cols; col++)
	{
	  /* Find out what is in the cell */
	  cell_contents = gb_table_is_cell_occupied (table, row, col);
	  if (cell_contents == GB_CELL_WIDGET)
	    {
	      /* If cell is occupied, delete any placeholders there. If a
		 placeholder occupies the cell but spans multiple rows/cols
		 split it into single cells but without a placeholder in this
		 cell */
	      gb_table_remove_placeholders (table, row, col);
	      rows_occupied[row] = 1;
	      cols_occupied[col] = 1;
	    }
	  else if (cell_contents == GB_CELL_EMPTY)
	    {
	      /* If the cell is empty, put a placeholder in it */
	      gtk_table_attach (GTK_TABLE (table),
				editor_new_placeholder (),
				col, col + 1, row, row + 1,
				GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
				0, 0);
	    }
	}
    }

  /* Now make sure that if a row/col is occupied, any placeholders in that
     row/col do not expand, and vice versa. */
  children = GTK_TABLE (table)->children;
  while (children)
    {
      child = children->data;
      if (GB_IS_PLACEHOLDER (child->widget))
	{
	  gboolean xexpand = TRUE, yexpand = TRUE;
	  gint row, col;

	  /* Check if any widgets are in the same rows as the placeholder. */
	  for (row = child->top_attach; row < child->bottom_attach; row++)
	    {
	      if (rows_occupied[row])
		yexpand = FALSE;
	    }
	  child->yexpand = yexpand;

	  /* Check if any widgets are in the same cols as the placeholder. */
	  for (col = child->left_attach; col < child->right_attach; col++)
	    {
	      if (cols_occupied[col])
		xexpand = FALSE;
	    }
	  child->xexpand = xexpand;

	  child->xfill = TRUE;
	  child->yfill = TRUE;
	}
      children = children->next;
    }

  g_free (rows_occupied);
  g_free (cols_occupied);

  gtk_widget_queue_resize (table);
}


/* Finds out if cell is occupied by a real widget. If not, returns whether
   the cell is empty or whether a placeholder is in it */
static gint
gb_table_is_cell_occupied (GtkWidget * table, gint row, gint col)
{
  GList *children;
  GtkTableChild *child;
  gint return_val = GB_CELL_EMPTY;

  children = GTK_TABLE (table)->children;
  while (children)
    {
      child = children->data;
      if (child->top_attach <= row && child->bottom_attach > row
	  && child->left_attach <= col && child->right_attach > col)
	{
	  if (GB_IS_PLACEHOLDER (child->widget))
	    return_val = GB_CELL_PLACEHOLDER;
	  else
	    return GB_CELL_WIDGET;
	}
      children = children->next;
    }
  return return_val;
}


static void
gb_table_remove_placeholders (GtkWidget * table, gint row, gint col)
{
  GList *children, *next;
  GtkTableChild *child;
  gint left, right, top, bottom;

  children = GTK_TABLE (table)->children;
  while (children)
    {
      next = children->next;
      child = children->data;
      left = child->left_attach;
      right = child->right_attach;
      top = child->top_attach;
      bottom = child->bottom_attach;

      if (top <= row && bottom > row && left <= col && right > col)
	{
	  /* If the widget is a placeholder remove it */
	  if (GB_IS_PLACEHOLDER (child->widget))
	    {
	      if (bottom - top > 1 || right - left > 1)
		{
		  gb_table_split_placeholder (table, child->widget,
					      left, right, top, bottom,
					      row, col);
		}
	      else
		{
		  gtk_container_remove (GTK_CONTAINER (table), child->widget);
		}
	    }
	}
      children = next;
    }
}


static void
gb_table_split_placeholder (GtkWidget * table, GtkWidget * placeholder,
			    gint left, gint right, gint top, gint bottom,
			    gint skip_row, gint skip_col)
{
  gint row, col;

  gtk_container_remove (GTK_CONTAINER (table), placeholder);
  for (row = top; row < bottom; row++)
    {
      for (col = left; col < right; col++)
	{
	  if (!(row == skip_row && col == skip_col))
	    {
	      gtk_table_attach (GTK_TABLE (table),
				editor_new_placeholder (),
				col, col + 1, row, row + 1,
				GTK_FILL, GTK_FILL, 0, 0);
	    }
	}
    }
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_table_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_table_new (%i, %i, %s);\n", data->wname,
		  GTK_TABLE (widget)->nrows, GTK_TABLE (widget)->ncols,
		  GTK_TABLE (widget)->homogeneous ? "TRUE" : "FALSE");
    }

  gb_widget_write_standard_source (widget, data);

  if (GTK_TABLE (widget)->row_spacing != 0)
    {
      source_add (data, "  gtk_table_set_row_spacings (GTK_TABLE (%s), %i);\n",
		  data->wname, GTK_TABLE (widget)->row_spacing);
    }
  if (GTK_TABLE (widget)->column_spacing != 0)
    {
      source_add (data, "  gtk_table_set_col_spacings (GTK_TABLE (%s), %i);\n",
		  data->wname, GTK_TABLE (widget)->column_spacing);
    }
}


/* Outputs source to add a child widget to a table. */
static void
gb_table_write_add_child_source (GtkWidget * parent,
				 const gchar *parent_name,
				 GtkWidget *child,
				 GbWidgetWriteSourceData * data)
{
  gchar xoptions[48], yoptions[48];
  GtkTableChild *tchild = glade_util_find_table_child (GTK_TABLE (parent),
						       child);
  g_return_if_fail (tchild != NULL);

  xoptions[0] = yoptions[0] = '\0';
  if (tchild->xexpand)
    strcpy (xoptions, "GTK_EXPAND");
  if (tchild->xshrink)
    {
      if (xoptions[0] != '\0')
	strcat (xoptions, " | ");
      strcat (xoptions, "GTK_SHRINK");
    }
  if (tchild->xfill)
    {
      if (xoptions[0] != '\0')
	strcat (xoptions, " | ");
      strcat (xoptions, "GTK_FILL");
    }
  if (xoptions[0] == '\0')
    strcpy (xoptions, "0");

  if (tchild->yexpand)
    strcpy (yoptions, "GTK_EXPAND");
  if (tchild->yshrink)
    {
      if (yoptions[0] != '\0')
	strcat (yoptions, " | ");
      strcat (yoptions, "GTK_SHRINK");
    }
  if (tchild->yfill)
    {
      if (yoptions[0] != '\0')
	strcat (yoptions, " | ");
      strcat (yoptions, "GTK_FILL");
    }
  if (yoptions[0] == '\0')
    strcpy (yoptions, "0");

  source_add (data,
	      "  gtk_table_attach (GTK_TABLE (%s), %s, %i, %i, %i, %i,\n"
	      "                    (GtkAttachOptions) (%s),\n"
	      "                    (GtkAttachOptions) (%s), %i, %i);\n",
	      parent_name, data->wname,
	      tchild->left_attach, tchild->right_attach,
	      tchild->top_attach, tchild->bottom_attach,
	      xoptions, yoptions, tchild->xpadding, tchild->ypadding);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_table_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_table_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = table_xpm;
  gbwidget.tooltip = _("Table");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_table_new;
  gbwidget.gb_widget_create_properties = gb_table_create_properties;
  gbwidget.gb_widget_get_properties = gb_table_get_properties;
  gbwidget.gb_widget_set_properties = gb_table_set_properties;
  gbwidget.gb_widget_create_child_properties = gb_table_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_table_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_table_set_child_properties;
  gbwidget.gb_widget_write_source = gb_table_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_table_write_add_child_source;
  gbwidget.gb_widget_create_popup_menu = gb_table_create_popup_menu;

  return &gbwidget;
}
