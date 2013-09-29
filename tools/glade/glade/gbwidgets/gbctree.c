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

#include <gtk/gtkctree.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkspinbutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/ctree.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Mode = "CTree|GtkCList::selection_mode";
static gchar *Titles = "CTree|GtkCList::show_titles";
static gchar *Shadow = "CTree|GtkCList::shadow_type";

/* This is only used for loading & saving - it isn't displayed in the property
   editor. */
static gchar *Cols = "GtkCTree::n_columns";
static gchar *ColumnWidths = "GtkCTree::column_widths";

static const gchar *GbModeChoices[] =
{"Single", "Browse", "Multiple", NULL};
static const gint GbModeValues[] =
{
  GTK_SELECTION_SINGLE,
  GTK_SELECTION_BROWSE,
  GTK_SELECTION_MULTIPLE
};
static const gchar *GbModeSymbols[] =
{
  "GTK_SELECTION_SINGLE",
  "GTK_SELECTION_BROWSE",
  "GTK_SELECTION_MULTIPLE"
};

static const gchar *GbShadowChoices[] =
{"None", "In", "Out",
 "Etched In", "Etched Out", NULL};
static const gint GbShadowValues[] =
{
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
};
static const gchar *GbShadowSymbols[] =
{
  "GTK_SHADOW_NONE",
  "GTK_SHADOW_IN",
  "GTK_SHADOW_OUT",
  "GTK_SHADOW_ETCHED_IN",
  "GTK_SHADOW_ETCHED_OUT"
};


static void show_ctree_dialog (GbWidgetNewData * data);
static void on_ctree_dialog_ok (GtkWidget * widget,
				GbWidgetNewData * data);
static void on_ctree_dialog_destroy (GtkWidget * widget,
				     GbWidgetNewData * data);
static GtkWidget *new_label (GtkWidget * parent);
static GtkWidget *new_unnamed_label (GtkWidget * parent);


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the funtion in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkCTree, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_ctree_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;
  gint cols = 0, i;

  if (data->action == GB_LOADING)
    {
      cols = load_int (data->loading_data, Cols);

      /* For backwards compatability with 1.1.1. */
      if (cols == 0)
	cols = load_int (data->loading_data, "columns");

      if (cols == 0)
	cols = 1;
      /* FIXME: Allow setting of tree column - 2nd arg */
      new_widget = gtk_ctree_new (cols, 0);

      /* GtkCList has problems if the title buttons aren't created. */
      for (i = 0; i < cols; i++)
	{
	  gtk_clist_set_column_widget (GTK_CLIST (new_widget), i,
				       new_unnamed_label (new_widget));
	  gtk_clist_set_column_width (GTK_CLIST (new_widget), i, 80);
          editor_add_mouse_signals_to_existing (GTK_CLIST (new_widget)->column[i].button);
	}

      return new_widget;
    }
  else
    {
      show_ctree_dialog (data);
      return NULL;
    }
}


static void
show_ctree_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New columned tree"), data->parent,
				     GTK_SIGNAL_FUNC (on_ctree_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_ctree_dialog_destroy), data);

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
on_ctree_dialog_ok (GtkWidget * widget,
		    GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *dialog, *label;
  gint cols, i;

  dialog = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (dialog), "cols");
      g_return_if_fail (spinbutton != NULL);
      cols = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      /* FIXME: Allow setting of tree column - 2nd arg */
      new_widget = gtk_ctree_new (cols, 0);
      gtk_clist_column_titles_show (GTK_CLIST (new_widget));
      for (i = 0; i < cols; i++)
	{
	  label = new_label (new_widget);
	  gtk_clist_set_column_widget (GTK_CLIST (new_widget), i,
				       label);
	  gtk_clist_set_column_width (GTK_CLIST (new_widget), i, 80);
	}

      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (dialog);
}


static void
on_ctree_dialog_destroy (GtkWidget * widget,
			 GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


GtkWidget *
new_label (GtkWidget * parent)
{
  GtkWidget *label;

  label = gb_widget_new ("GtkLabel", parent);
  g_return_val_if_fail (label != NULL, NULL);
  gb_widget_set_child_name (label, GladeChildCListTitle);
  return label;
}


GtkWidget *
new_unnamed_label (GtkWidget * parent)
{
  GtkWidget *label;

  label = gb_widget_new_full ("GtkLabel", FALSE, parent, NULL, 0, 0, NULL,
			      GB_CREATING, NULL);
  g_return_val_if_fail (label != NULL, NULL);
  gb_widget_set_child_name (label, GladeChildCListTitle);
  return label;
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_ctree_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_choice (Mode, _("Select Mode:"),
		       _("The selection mode of the columned tree"),
		       GbModeChoices);
  property_add_bool (Titles, _("Show Titles:"),
		     _("If the column titles are shown"));
  property_add_choice (Shadow, _("Shadow:"),
		       _("The type of shadow of the columned tree's border"),
		       GbShadowChoices);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_ctree_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gchar buffer[1024];
  gchar *pos;
  gboolean buffer_overrun = FALSE;
  gint i;

  if (data->action == GB_SAVING)
    {
      gb_widget_output_int (data, Cols, GTK_CLIST (widget)->columns);

      pos = buffer;
      for (i = 0; i < GTK_CLIST (widget)->columns; i++)
	{
	  if (i == 0)
	    sprintf (pos, "%i", GTK_CLIST (widget)->column[i].width);
	  else
	    sprintf (pos, ",%i", GTK_CLIST (widget)->column[i].width);
	  pos += strlen (pos);

	  /* Extra check to make sure we don't overrun the buffer. */
	  if (pos - buffer > 1000)
	    {
	      g_warning ("Buffer overflow");
	      buffer_overrun = TRUE;
	      break;
	    }
	}
      if (!buffer_overrun)
	gb_widget_output_string (data, ColumnWidths, buffer);
    }

  for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
    {
      if (GbModeValues[i] == GTK_CLIST (widget)->selection_mode)
	gb_widget_output_choice (data, Mode, i, GbModeSymbols[i]);
    }

  gb_widget_output_bool (data, Titles, GTK_CLIST_SHOW_TITLES (widget));

  for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i++)
    {
      if (GbShadowValues[i] == GTK_CLIST (widget)->shadow_type)
	gb_widget_output_choice (data, Shadow, i, GbShadowSymbols[i]);
    }
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_ctree_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *widths, *pos, *mode;
  gchar *shadow;
  gboolean titles;
  gint col, w, i;

  if (data->action == GB_LOADING)
    {
      widths = gb_widget_input_string (data, ColumnWidths);
      if (data->apply)
	{
	  pos = widths;
	  for (col = 0; col < GTK_CLIST (widget)->columns; col++)
	    {
	      w = atoi (pos);
	      gtk_clist_set_column_width (GTK_CLIST (widget), col, w);
	      pos = strchr (pos, ',');
	      if (!pos)
		break;
	      pos++;
	    }
	}
    }

  mode = gb_widget_input_choice (data, Mode);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (!strcmp (mode, GbModeChoices[i])
	      || !strcmp (mode, GbModeSymbols[i]))
	    {
	      gtk_clist_set_selection_mode (GTK_CLIST (widget), GbModeValues[i]);
	      break;
	    }
	}
    }

  titles = gb_widget_input_bool (data, Titles);
  if (data->apply)
    {
      if (titles)
	gtk_clist_column_titles_show (GTK_CLIST (widget));
      else
	gtk_clist_column_titles_hide (GTK_CLIST (widget));
    }

  shadow = gb_widget_input_choice (data, Shadow);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i
	   ++)
	{
	  if (!strcmp (shadow, GbShadowChoices[i])
	      || !strcmp (shadow, GbShadowSymbols[i]))
	    {
	      gtk_clist_set_shadow_type (GTK_CLIST (widget),
					 GbShadowValues[i]);
	      break;
	    }
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkCTree, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_ctree_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/


void
gb_ctree_add_child (GtkWidget *widget, GtkWidget *child,
		    GbWidgetSetArgData *data)
{
  /* We store the last column title read in 'last_child' */
  gint col = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
						   "last_child"));

  if (col >= GTK_CLIST (widget)->columns) {
    g_warning ("Too many column title widgets for GtkCTree - skipping");
    return;
  }

  gtk_clist_set_column_widget (GTK_CLIST (widget), col, child);
  gtk_object_set_data (GTK_OBJECT (widget), "last_child",
		       GINT_TO_POINTER (col + 1));

  /* We need to add signals to the clist button, just in case the
     title widget has no window and so doesn't get signals itself.
     Since Clist always creates 1 button initially, the signals would
     be added to this button in gb_widget_new, so we could skip it,
     but it doesn't hurt. */
  editor_add_mouse_signals_to_existing (GTK_CLIST (widget)->column[col].button);
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_ctree_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint col, i;

  if (data->create_widget)
    {
      /* FIXME: allow changing of 2nd arg - ctree column */
      source_add (data, "  %s = gtk_ctree_new (%i, 0);\n", data->wname,
		  GTK_CLIST (widget)->columns);
    }

  /* We reset the last_child index, so as the title widgets are written out
     they will start at column 0. */
  gtk_object_set_data (GTK_OBJECT (widget), "last_child", GINT_TO_POINTER (-1));

  gb_widget_write_standard_source (widget, data);

  for (col = 0; col < GTK_CLIST (widget)->columns; col++)
    {
      source_add (data,
		  "  gtk_clist_set_column_width (GTK_CLIST (%s), %i, %i);\n",
		  data->wname, col, GTK_CLIST (widget)->column[col].width);
    }

  if (GTK_CLIST (widget)->selection_mode != GTK_SELECTION_SINGLE)
    {
      for (i = 0; i < sizeof (GbModeValues) / sizeof (GbModeValues[0]); i++)
	{
	  if (GbModeValues[i] == GTK_CLIST (widget)->selection_mode)
	    source_add (data,
		   "  gtk_clist_set_selection_mode (GTK_CLIST (%s), %s);\n",
			data->wname, GbModeSymbols[i]);
	}
    }

  if (GTK_CLIST_SHOW_TITLES (widget))
    source_add (data, "  gtk_clist_column_titles_show (GTK_CLIST (%s));\n",
		data->wname);
  else
    source_add (data, "  gtk_clist_column_titles_hide (GTK_CLIST (%s));\n",
		data->wname);

  if (GTK_CLIST (widget)->shadow_type != GTK_SHADOW_IN)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i
	   ++)
	{
	  if (GbShadowValues[i] == GTK_CLIST (widget)->shadow_type)
	    source_add (data, "  gtk_clist_set_shadow_type (GTK_CLIST (%s), %s);\n",
			data->wname, GbShadowSymbols[i]);
	}
    }
}


/* Outputs source to add a child widget to a CTree. */
static void
gb_ctree_write_add_child_source (GtkWidget * parent,
				 const gchar *parent_name,
				 GtkWidget *child,
				 GbWidgetWriteSourceData * data)
{
  gchar *child_name;

  child_name = gb_widget_get_child_name (child);

  /* See if this is a title widget. */
  if (child_name && (!strcmp (child_name, GladeChildCListTitle)))
    {
      /* We store the last column title written in 'last_child' */
      gint col = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (parent),
						       "last_child"));
      source_add (data,
		  "  gtk_clist_set_column_widget (GTK_CLIST (%s), %i, %s);\n",
		  parent_name, col + 1, data->wname);

      gtk_object_set_data (GTK_OBJECT (parent), "last_child",
			   GINT_TO_POINTER (col + 1));
    }
  else
    {
      g_warning ("Unknown CTree child widgetL %s", data->wname);
      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		  parent_name, data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_ctree_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_ctree_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = ctree_xpm;
  gbwidget.tooltip = _("Columned Tree");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_ctree_new;
  gbwidget.gb_widget_add_child		= gb_ctree_add_child;
  gbwidget.gb_widget_create_properties	= gb_ctree_create_properties;
  gbwidget.gb_widget_get_properties	= gb_ctree_get_properties;
  gbwidget.gb_widget_set_properties	= gb_ctree_set_properties;
  gbwidget.gb_widget_write_source	= gb_ctree_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_ctree_write_add_child_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_ctree_create_popup_menu;
*/

  return &gbwidget;
}

