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

/* This is just for testing, to make sure we don't use deprecated calls. */
#if 0
#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkenums.h>
#include <gtk/gtktooltips.h>
#include <gtk/gtktoolitem.h>
#define GTK_DISABLE_DEPRECATED
#include <gtk/gtktoolbar.h>
#undef GTK_DISABLE_DEPRECATED
#endif

#ifdef USE_GNOME
#include <gnome.h>
#include "../glade_gnome.h"
#else
#include <gtk/gtkbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtktoolbutton.h>
#include <gtk/gtkvbox.h>
#endif

#include "../gb.h"
#include "../tree.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/toolbar.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static const gchar *Size = "GtkToolbar::size";
static const gchar *Orientation = "GtkToolbar::orientation";
static const gchar *Style = "GtkToolbar::toolbar_style";
static const gchar *Tooltips = "GtkToolbar::tooltips";
static const gchar *ShowArrow = "GtkToolbar::show_arrow";

/* For children of a toolbar */
static const gchar *Expand = "GtkToolbarChild::expand";
static const gchar *Homogeneous = "GtkToolbarChild::homogeneous";


static const gchar *GbOrientationChoices[] =
{"Horizontal", "Vertical", NULL};
static const gint GbOrientationValues[] =
{
  GTK_ORIENTATION_HORIZONTAL,
  GTK_ORIENTATION_VERTICAL
};
static const gchar *GbOrientationSymbols[] =
{
  "GTK_ORIENTATION_HORIZONTAL",
  "GTK_ORIENTATION_VERTICAL"
};

static const gchar *GbStyleChoices[] =
{"Icons", "Text", "Both", "Both Horizontal", NULL};
static const gint GbStyleValues[] =
{
  GTK_TOOLBAR_ICONS,
  GTK_TOOLBAR_TEXT,
  GTK_TOOLBAR_BOTH,
  GTK_TOOLBAR_BOTH_HORIZ
};
static const gchar *GbStyleSymbols[] =
{
  "GTK_TOOLBAR_ICONS",
  "GTK_TOOLBAR_TEXT",
  "GTK_TOOLBAR_BOTH",
  "GTK_TOOLBAR_BOTH_HORIZ"
};


static void show_toolbar_dialog (GbWidgetNewData * data);
static void on_toolbar_dialog_ok (GtkWidget * widget,
				  GbWidgetNewData * data);
static void on_toolbar_dialog_destroy (GtkWidget * widget,
				       GbWidgetNewData * data);

static void update_toolbar_size (GtkWidget * widget, gint size);

static void gb_toolbar_insert_before (GtkWidget * menuitem,
				      GtkWidget * child);
static void gb_toolbar_insert_after (GtkWidget * menuitem,
				     GtkWidget * child);
static void gb_toolbar_insert (GtkWidget * child,
			       gint offset);


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkToolbar, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget*
gb_toolbar_new(GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gtk_toolbar_new ();
      return new_widget;
    }
  else
    {
      show_toolbar_dialog (data);
      return NULL;
    }
}


void
gb_toolbar_add_child (GtkWidget *widget, GtkWidget *child, GbWidgetSetArgData *data)
{
  /* If the child is a placeholder, we need to insert a GtkToolItem. */
  if (GB_IS_PLACEHOLDER (child))
    {
      GtkWidget *toolitem = (GtkWidget*) gtk_tool_item_new ();
      gtk_widget_show (toolitem);
      gtk_container_add (GTK_CONTAINER (toolitem), child);
      gtk_toolbar_insert (GTK_TOOLBAR (widget),
			  GTK_TOOL_ITEM (toolitem), -1);
    }
  /* GtkToolItems can just be added as they are. */
  else if (GTK_IS_TOOL_ITEM (child))
    {
      gtk_toolbar_insert (GTK_TOOLBAR (widget),
			  GTK_TOOL_ITEM (child), -1);
    }
  /* Any other widgets are from the old GTK+ 2.2 files, and we need to insert
     a GtkToolItem. */
  else
    {
      GtkWidget *toolitem = gb_widget_new_full ("GtkToolItem", FALSE,
						NULL, NULL, 0, 0, NULL,
						GB_LOADING, data);
      gtk_toolbar_insert (GTK_TOOLBAR (widget),
			  GTK_TOOL_ITEM (toolitem), -1);
      tree_add_widget (toolitem);

      gtk_container_add (GTK_CONTAINER (toolitem), child);
    }
}


static void
show_toolbar_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New toolbar"), data->parent,
				     GTK_SIGNAL_FUNC (on_toolbar_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_toolbar_dialog_destroy), data);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Number of items:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "items",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 5);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


static void
on_toolbar_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window, *toolitem, *placeholder;
  gint items, i;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "items");
      g_return_if_fail (spinbutton != NULL);
      items = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gtk_toolbar_new ();
      for (i = 0; i < items; i++)
	{
	  toolitem = (GtkWidget*) gtk_tool_item_new ();
	  gtk_widget_show (toolitem);
	  gtk_toolbar_insert (GTK_TOOLBAR (new_widget),
			      GTK_TOOL_ITEM (toolitem), -1);

	  placeholder = editor_new_placeholder ();
	  gtk_container_add (GTK_CONTAINER (toolitem), placeholder);
	}
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_toolbar_dialog_destroy (GtkWidget * widget,
			   GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_toolbar_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_int_range (Size, _("Size:"),
			  _("The number of items in the toolbar"),
			  0, 1000, 1, 10, 1);
  property_add_choice (Orientation, _("Orientation:"),
		       _("The toolbar orientation"),
		       GbOrientationChoices);
  property_add_choice (Style, _("Style:"),
		       _("The toolbar style"),
		       GbStyleChoices);
  property_add_bool (Tooltips, _("Tooltips:"), _("If tooltips are enabled"));
  property_add_bool (ShowArrow, _("Show Arrow:"), _("If an arrow should be shown to popup a menu if the toolbar doesn't fit"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_toolbar_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint i;

  if (data->action != GB_SAVING)
    gb_widget_output_int (data, Size,
			  gtk_toolbar_get_n_items (GTK_TOOLBAR (widget)));

  for (i = 0; i < sizeof (GbOrientationValues)
	 / sizeof (GbOrientationValues[0]); i++)
    {
      if (GbOrientationValues[i] == GTK_TOOLBAR (widget)->orientation)
	gb_widget_output_choice (data, Orientation, i, GbOrientationSymbols[i]);
    }

  for (i = 0; i < sizeof (GbStyleValues) / sizeof (GbStyleValues[0]); i++)
    {
      if (GbStyleValues[i] == GTK_TOOLBAR (widget)->style)
	gb_widget_output_choice (data, Style, i, GbStyleSymbols[i]);
    }

  gb_widget_output_bool (data, Tooltips,
		    GTK_TOOLTIPS (GTK_TOOLBAR (widget)->tooltips)->enabled);

  gb_widget_output_bool (data, ShowArrow,
			 gtk_toolbar_get_show_arrow (GTK_TOOLBAR (widget)));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_toolbar_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *orientation, *style;
  gint i, size;
  gboolean tooltips, show_arrow;

  size = gb_widget_input_int (data, Size);
  if (data->apply)
    update_toolbar_size (widget, size);

  orientation = gb_widget_input_choice (data, Orientation);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbOrientationValues)
	   / sizeof (GbOrientationValues[0]); i++)
	{
	  if (!strcmp (orientation, GbOrientationChoices[i])
	      || !strcmp (orientation, GbOrientationSymbols[i]))
	    {
	      gtk_toolbar_set_orientation (GTK_TOOLBAR (widget),
					   GbOrientationValues[i]);
	      break;
	    }
	}
    }

  style = gb_widget_input_choice (data, Style);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbStyleValues) / sizeof (GbStyleValues[0]); i++
	)
	{
	  if (!strcmp (style, GbStyleChoices[i])
	      || !strcmp (style, GbStyleSymbols[i]))
	    {
	      /* This avoids any problems with redrawing the selection. */
	      if (data->action == GB_APPLYING)
		editor_clear_selection (NULL);
	      gtk_toolbar_set_style (GTK_TOOLBAR (widget), GbStyleValues[i]);
	      /*editor_refresh_widget (widget);*/
	      break;
	    }
	}
    }

  tooltips = gb_widget_input_bool (data, Tooltips);
  if (data->apply)
    gtk_toolbar_set_tooltips (GTK_TOOLBAR (widget), tooltips);

  show_arrow = gb_widget_input_bool (data, ShowArrow);
  if (data->apply)
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (widget), show_arrow);
}


/* This updates the toolbar size to the given value, adding buttons or
   deleting items as necessary. */
static void
update_toolbar_size (GtkWidget * widget, gint size)
{
  gint current_size;

  current_size = gtk_toolbar_get_n_items (GTK_TOOLBAR (widget));

  if (current_size < size)
    {
      /* FIXME: This avoids any problems with redrawing the selection. */
      editor_clear_selection (NULL);

      while (current_size < size)
	{
	  GtkWidget *toolitem, *placeholder;

	  toolitem = (GtkWidget*) gtk_tool_item_new ();
	  gtk_widget_show (toolitem);
	  gtk_toolbar_insert (GTK_TOOLBAR (widget),
			      GTK_TOOL_ITEM (toolitem), -1);

	  placeholder = editor_new_placeholder ();
	  gtk_container_add (GTK_CONTAINER (toolitem), placeholder);

	  current_size++;
	}
    }
  else if (current_size > size)
    {
      while (current_size > size)
	{
	  GtkToolItem *item;

	  item = gtk_toolbar_get_nth_item (GTK_TOOLBAR (widget), size);
	  gtk_container_remove (GTK_CONTAINER (widget), GTK_WIDGET (item));
	  current_size--;
	}
    }
}


static void
gb_toolbar_create_child_properties (GtkWidget * widget,
				    GbWidgetCreateChildArgData * data)
{
  property_add_bool (Expand, _("Expand:"),
		     _("Set True to let the widget expand"));
  property_add_bool (Homogeneous, _("Homogeneous:"),
		     _("If the item should be the same size as other homogeneous items"));
}


static void
gb_toolbar_get_child_properties (GtkWidget *widget, GtkWidget *child,
				 GbWidgetGetArgData *data)
{
  if (data->action == GB_SAVING)
    save_start_tag (data, "packing");

  gb_widget_output_bool (data, Expand,
			 gtk_tool_item_get_expand (GTK_TOOL_ITEM (child)));
  gb_widget_output_bool (data, Homogeneous,
			 gtk_tool_item_get_homogeneous (GTK_TOOL_ITEM (child)));

  if (data->action == GB_SAVING)
    save_end_tag (data, "packing");
}


/* Migrate old 2.0 XML files. We insert a separator tool item. */
static void
gb_toolbar_set_new_group (GtkWidget *widget, GtkWidget *child)
{
  gint pos;
  GtkWidget *separator;

  pos = gtk_toolbar_get_item_index (GTK_TOOLBAR (widget),
				    GTK_TOOL_ITEM (child));

  /* We use gb_widget_new_full() passing FALSE for create_default_name,
     since we are loading and we don't want to create any names while loading
     or they may clash. */
  separator = gb_widget_new_full ("GtkSeparatorToolItem", FALSE,
				  widget, NULL, 0, 0, NULL,
				  GB_LOADING, NULL);

  gtk_toolbar_insert (GTK_TOOLBAR (widget), GTK_TOOL_ITEM (separator), pos);
}


static void
gb_toolbar_set_child_properties (GtkWidget *widget, GtkWidget *child,
				 GbWidgetSetArgData *data)
{
  gboolean new_group, expand, homogeneous;

  /* Migrate old 2.0 XML files. */
  if (data->action == GB_LOADING)
    {
      new_group = gb_widget_input_bool (data, "new_group");
      if (data->apply && new_group)
	gb_toolbar_set_new_group (widget, child);
    }

  expand = gb_widget_input_bool (data, Expand);
  if (data->apply)
    gtk_tool_item_set_expand (GTK_TOOL_ITEM (child), expand);

  homogeneous = gb_widget_input_bool (data, Homogeneous);
  if (data->apply)
    gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (child), homogeneous);
}

/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkToolbar, with signals pointing to
 * other functions in this file.
 */
static void
gb_toolbar_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
{
  GtkWidget *menuitem;

  if (data->child == NULL)
    return;

  /* Commands for inserting new items. */
  menuitem = gtk_menu_item_new_with_label (_("Insert Item Before"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_toolbar_insert_before),
		      data->child);

  menuitem = gtk_menu_item_new_with_label (_("Insert Item After"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_toolbar_insert_after),
		      data->child);
}


static void
gb_toolbar_insert_before (GtkWidget * menuitem, GtkWidget * child)
{
  gb_toolbar_insert (child, -1);
}


static void
gb_toolbar_insert_after (GtkWidget * menuitem, GtkWidget * child)
{
  gb_toolbar_insert (child, 1);
}


static void
gb_toolbar_insert (GtkWidget * child, gint offset)
{
  GtkWidget *toolbar = child->parent, *toolitem, *placeholder;
  gint pos;

  pos = gtk_toolbar_get_item_index (GTK_TOOLBAR (toolbar),
				    GTK_TOOL_ITEM (child));

  if (offset > 0)
    pos++;

  toolitem = (GtkWidget*) gtk_tool_item_new ();
  gtk_widget_show (toolitem);
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), (GtkToolItem*) toolitem, pos);

  placeholder = editor_new_placeholder ();
  gtk_container_add (GTK_CONTAINER (toolitem), placeholder);
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_toolbar_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  const gchar *orientation = GbOrientationSymbols[0];
  const gchar *style = GbStyleSymbols[0];
  gint i;

  for (i = 0;
       i < sizeof (GbOrientationValues) / sizeof (GbOrientationValues[0]); i++)
    {
      if (GbOrientationValues[i] == GTK_TOOLBAR (widget)->orientation)
	orientation = GbOrientationSymbols[i];
    }

  for (i = 0; i < sizeof (GbStyleValues) / sizeof (GbStyleValues[0]); i++)
    {
      if (GbStyleValues[i] == GTK_TOOLBAR (widget)->style)
	style = GbStyleSymbols[i];
    }

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_toolbar_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  /* We have to set the style explicitly for now, as the default style depends
     on the user's theme settings. Ideally GTK+ should have a
     GTK_TOOLBAR_DEFAULT style. */
  source_add (data,
	      "  gtk_toolbar_set_style (GTK_TOOLBAR (%s), %s);\n",
	      data->wname, style);

  if (GTK_TOOLBAR (widget)->orientation != GTK_ORIENTATION_HORIZONTAL)
    source_add (data,
		"  gtk_toolbar_set_orientation (GTK_TOOLBAR (%s), %s);\n",
		data->wname, orientation);

  if (!GTK_TOOLTIPS (GTK_TOOLBAR (widget)->tooltips)->enabled)
    source_add (data,
		"  gtk_toolbar_set_tooltips (GTK_TOOLBAR (%s), FALSE);\n",
		data->wname);

  if (!gtk_toolbar_get_show_arrow (GTK_TOOLBAR (widget)))
    source_add (data,
		"  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (%s), FALSE);\n",
		data->wname);

  /* We set up a tmp_toolbar_icon_size variable that we can use when creating
     the tool items. */
  source_ensure_decl (data, "  GtkIconSize tmp_toolbar_icon_size;\n");
  source_add (data,
	      "  tmp_toolbar_icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (%s));\n",
	      data->wname);
}


static void
gb_toolbar_write_add_child_source (GtkWidget               *parent,
				   const char              *parent_name,
				   GtkWidget               *child,
				   GbWidgetWriteSourceData *data)
{
  gboolean homogeneous, output_homogeneous = FALSE;

  if (gtk_tool_item_get_expand (GTK_TOOL_ITEM (child)))
    {
      source_add (data,
		  "  gtk_tool_item_set_expand (GTK_TOOL_ITEM (%s), TRUE);\n",
		  data->wname);
    }

  /* Homogeneous defaults to FALSE for GtkToolItem, but TRUE for
     GtkToolButton and descendants. */
  homogeneous = gtk_tool_item_get_homogeneous (GTK_TOOL_ITEM (child));
  if (GTK_IS_TOOL_BUTTON (child))
    {
      if (!homogeneous)
	output_homogeneous = TRUE;
    }
  else
    {
      if (homogeneous)
	output_homogeneous = TRUE;
    }

  if (output_homogeneous)
    {
      source_add (data,
		  "  gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (%s), %s);\n",
		  data->wname,
		  homogeneous ? "TRUE" : "FALSE");
    }

  source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
	      parent_name, data->wname);
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_toolbar_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_toolbar_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = toolbar_xpm;
  gbwidget.tooltip = _("Toolbar");

  /* Fill in any functions that this GbWidget has */
   gbwidget.gb_widget_new               = gb_toolbar_new;
  gbwidget.gb_widget_add_child = gb_toolbar_add_child;
  gbwidget.gb_widget_create_properties = gb_toolbar_create_properties;
  gbwidget.gb_widget_get_properties = gb_toolbar_get_properties;
  gbwidget.gb_widget_set_properties = gb_toolbar_set_properties;
  gbwidget.gb_widget_create_child_properties = gb_toolbar_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_toolbar_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_toolbar_set_child_properties;
  gbwidget.gb_widget_create_popup_menu = gb_toolbar_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_toolbar_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_toolbar_write_add_child_source;

  return &gbwidget;
}
