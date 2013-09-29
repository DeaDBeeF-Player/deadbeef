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
#include "../glade_gnome.h"
#include "../tree.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/bonobo-dock.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *AllowFloating = "BonoboDock::allow_floating";


static void gb_bonobo_dock_add_band_on_left (GtkWidget * menuitem,
					    GtkWidget * dock);
static void gb_bonobo_dock_add_band_on_right (GtkWidget * menuitem,
					     GtkWidget * dock);
static void gb_bonobo_dock_add_band_on_top (GtkWidget * menuitem,
					   GtkWidget * dock);
static void gb_bonobo_dock_add_band_on_bottom (GtkWidget * menuitem,
					      GtkWidget * dock);
static void gb_bonobo_dock_add_band_floating (GtkWidget * menuitem,
					     GtkWidget * dock);
static void gb_bonobo_dock_add_band (GtkWidget * dock,
				    BonoboDockPlacement placement);

static void gb_bonobo_dock_init_widget (GtkWidget *widget);
static void gb_bonobo_dock_size_request (GtkWidget *widget,
					GtkRequisition *requisition,
					gpointer user_data);
static void gb_bonobo_dock_size_allocate (GtkWidget *widget,
					 GtkAllocation *allocation,
					 gpointer user_data);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the funtion in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class BonoboDock, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_bonobo_dock_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = bonobo_dock_new ();

  /* FIXME: GnomeLibs 1.0.1 bug workaround to resize floating items. */
  gb_bonobo_dock_init_widget (new_widget);

  return new_widget;
}


/* FIXME: GnomeLibs 1.0.1 bug workaround to resize floating items. */
static void
gb_bonobo_dock_create_from_widget (GtkWidget *widget,
				  GbWidgetCreateFromData *data)
{
  gb_bonobo_dock_init_widget (widget);
}


/* FIXME: GnomeLibs 1.0.1 bug workaround to resize floating items. */
static void
gb_bonobo_dock_init_widget (GtkWidget *widget)
{
  gtk_signal_connect_after (GTK_OBJECT (widget), "size_request",
			    GTK_SIGNAL_FUNC (gb_bonobo_dock_size_request),
			    NULL);
  gtk_signal_connect_after (GTK_OBJECT (widget), "size_allocate",
			    GTK_SIGNAL_FUNC (gb_bonobo_dock_size_allocate),
			    NULL);
}


/* FIXME: GnomeLibs 1.0.1 bug workaround to resize floating items.
   Calls gtk_widget_size_request() for all floating items, just so that they
   calculate their requisition. */
static void
gb_bonobo_dock_size_request (GtkWidget *widget, GtkRequisition *requisition,
			    gpointer user_data)
{
  GList *lp;
  GtkWidget *w;
  GtkRequisition float_item_requisition;

  lp = BONOBO_DOCK (widget)->floating_children;
  while (lp != NULL)
    {
      w = lp->data;
      lp = lp->next;
      gtk_widget_size_request (w, &float_item_requisition);
    }
}


/* FIXME: GnomeLibs 1.0.1 bug workaround to resize floating items.
   Calls gtk_widget_size_allocate() for all floating items, allocating
   whatever the item wants in its requisition. */
static void
gb_bonobo_dock_size_allocate (GtkWidget *widget, GtkAllocation *allocation,
			     gpointer user_data)
{
  GList *lp;
  GtkWidget *w;
  GtkAllocation float_item_allocation;

  lp = BONOBO_DOCK (widget)->floating_children;
  while (lp != NULL)
    {
      w = lp->data;
      lp = lp->next;

      float_item_allocation.x = 0;
      float_item_allocation.y = 0;
      float_item_allocation.width = w->requisition.width;
      float_item_allocation.height = w->requisition.height;
      gtk_widget_size_allocate (w, &float_item_allocation);
    }
}


void
gb_bonobo_dock_add_child (GtkWidget *widget, GtkWidget * child,
			 GbWidgetSetArgData *data)
{
  gchar *orientation_string, *placement_string;
  BonoboDockPlacement placement;
  GtkOrientation orientation;
  gint band_num, position, offset, placement_index, orientation_index;

  if (BONOBO_IS_DOCK_ITEM (child))
    {
      data->loading_type = GB_CHILD_PROPERTIES;

      placement_string = load_choice (data, "placement");
      placement = BONOBO_DOCK_TOP;
      if (placement_string && placement_string[0])
	{
	  placement_index = glade_util_string_array_index (GladePlacementSymbols,
							   GladePlacementSize,
							   placement_string);
	  if (placement_index != -1)
	    placement = GladePlacementValues[placement_index];
	}

      if (placement == BONOBO_DOCK_FLOATING)
	{
	  orientation_string = load_choice (data, "orientation");
	  orientation = GTK_ORIENTATION_HORIZONTAL;
	  if (orientation_string && orientation_string[0])
	    {
	      orientation_index = glade_util_string_array_index (GladeOrientationSymbols, GladeOrientationSize, orientation_string);
	      if (orientation_index != -1)
		orientation = GladeOrientationValues[orientation_index];
	    }

	  /* FIXME: Where fo we put the floating item? Use 300,300 for now. */
	  bonobo_dock_add_floating_item (BONOBO_DOCK (widget),
					BONOBO_DOCK_ITEM (child),
					300, 300, orientation);
	}
      else
	{
	  gboolean new_band = FALSE;

	  band_num = load_int (data, "band");
	  position = load_int (data, "position");
	  offset = load_int (data, "offset");

	  /* When loading, we don't want to create a new band explicitly,
	     but when pasting we do (though I'm not sure yet). */
	  if (data->xml_buffer)
	    new_band = TRUE;

	  bonobo_dock_add_item (BONOBO_DOCK (widget), BONOBO_DOCK_ITEM (child),
			       placement, band_num, position, offset,
			       new_band /* FIXME: what should this be. */);
	}

      data->loading_type = GB_STANDARD_PROPERTIES;
    }
  else
    {
      bonobo_dock_set_client_area (BONOBO_DOCK (widget), child);
    }

  /* Floating dock items must be shown after adding to the dock, or they do
     not appear (GnomeLibs 1.0.1). */
  gtk_widget_show (child);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_bonobo_dock_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (AllowFloating, _("Allow Floating:"),
		     _("If floating dock items are allowed"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_bonobo_dock_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gb_widget_output_bool (data, AllowFloating,
			 BONOBO_DOCK (widget)->floating_items_allowed);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_bonobo_dock_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean allow_floating;

  allow_floating = gb_widget_input_bool (data, AllowFloating);
  if (data->apply)
    {
      bonobo_dock_allow_floating_items (BONOBO_DOCK (widget), allow_floating);
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a BonoboDock, with signals pointing to
 * other functions in this file.
 */
static void
gb_bonobo_dock_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (_("Add dock band on top"));
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_bonobo_dock_add_band_on_top),
		      widget);

  menuitem = gtk_menu_item_new_with_label (_("Add dock band on bottom"));
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_bonobo_dock_add_band_on_bottom),
		      widget);

  menuitem = gtk_menu_item_new_with_label (_("Add dock band on left"));
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_bonobo_dock_add_band_on_left),
		      widget);

  menuitem = gtk_menu_item_new_with_label (_("Add dock band on right"));
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_bonobo_dock_add_band_on_right),
		      widget);

  menuitem = gtk_menu_item_new_with_label (_("Add floating dock item"));
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_bonobo_dock_add_band_floating),
		      widget);
}

static void
gb_bonobo_dock_add_band_on_left (GtkWidget * menuitem, GtkWidget * dock)
{
  gb_bonobo_dock_add_band (dock, BONOBO_DOCK_LEFT);
}

static void
gb_bonobo_dock_add_band_on_right (GtkWidget * menuitem, GtkWidget * dock)
{
  gb_bonobo_dock_add_band (dock, BONOBO_DOCK_RIGHT);
}

static void
gb_bonobo_dock_add_band_on_top (GtkWidget * menuitem, GtkWidget * dock)
{
  gb_bonobo_dock_add_band (dock, BONOBO_DOCK_TOP);
}

static void
gb_bonobo_dock_add_band_on_bottom (GtkWidget * menuitem, GtkWidget * dock)
{
  gb_bonobo_dock_add_band (dock, BONOBO_DOCK_BOTTOM);
}

static void
gb_bonobo_dock_add_band_floating (GtkWidget * menuitem, GtkWidget * dock)
{
  gb_bonobo_dock_add_band (dock, BONOBO_DOCK_FLOATING);
}

static void
gb_bonobo_dock_add_band (GtkWidget * dock, BonoboDockPlacement placement)
{
  GtkWidget *dock_item, *placeholder;
  gint x, y;

  dock_item = gb_widget_new ("BonoboDockItem", NULL);

  placeholder = editor_new_placeholder ();
  gtk_container_add (GTK_CONTAINER (dock_item), placeholder);

  if (placement == BONOBO_DOCK_FLOATING)
    {
      gdk_window_get_pointer (NULL, &x, &y, NULL);
      /* Place the floating item slightly to the left and above the pointer,
	 but make sure it is on the screen. */
      x = MAX (0, x - 50);
      y = MAX (0, y - 50);
      bonobo_dock_add_floating_item (BONOBO_DOCK (dock),
				    BONOBO_DOCK_ITEM (dock_item),
				    x, y, GTK_ORIENTATION_HORIZONTAL);
    }
  else
    {
      bonobo_dock_add_item (BONOBO_DOCK (dock), BONOBO_DOCK_ITEM (dock_item),
			   placement, -1, 0, 0, TRUE);
    }

  /* Floating dock items must be shown after adding to the dock, or they do
     not appear (GnomeLibs 1.0.1). */
  gtk_widget_show (dock_item);

  /* Show the properties of the new dock item. */
  gb_widget_show_properties (dock_item);

  tree_add_widget (dock_item);
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_bonobo_dock_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  /* If our parent is a GnomeApp we don't create the BonoboDock here. */
  if (widget->parent && GTK_IS_VBOX (widget->parent)
      && widget->parent->parent && GNOME_IS_APP (widget->parent->parent))
    {
      data->create_widget = FALSE;
      source_add (data, "  %s = GNOME_APP (%s)->dock;\n",
		  data->wname, data->component_name);
    }

  if (data->create_widget)
    {
      source_add (data, "  %s = bonobo_dock_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (!BONOBO_DOCK (widget)->floating_items_allowed)
    {
      source_add (data,
		  "  bonobo_dock_allow_floating_items (BONOBO_DOCK (%s), FALSE);\n",
		  data->wname);
    }

  /* Set the initial positions for any floating dock items. */
  gtk_object_set_data (GTK_OBJECT (widget), "glade-dock-item-x",
		       GINT_TO_POINTER (100));
  gtk_object_set_data (GTK_OBJECT (widget), "glade-dock-item-y",
		       GINT_TO_POINTER (100));
}


/* Outputs source to add a child menu to a BonoboDock. */
static void
gb_bonobo_dock_write_add_child_source (GtkWidget * parent,
				      const gchar *parent_name,
				      GtkWidget *child,
				      GbWidgetWriteSourceData * data)
{
  if (BONOBO_IS_DOCK_ITEM (child))
    {
      gchar *orientation;
      gint x, y;

      /* FIXME: We should support adding normal dock items eventually. */
      if (!glade_gnome_is_app_dock_item (child))
	g_warning ("Can't add normal dock items to a BonoboDock: %s",
		   data->wname);

      if (!BONOBO_DOCK_ITEM (child)->is_floating)
	g_warning ("Can't add non-floating dock items to a BonoboDock: %s",
		   data->wname);

      if (BONOBO_DOCK_ITEM (child)->orientation == GTK_ORIENTATION_HORIZONTAL)
	orientation = "GTK_ORIENTATION_HORIZONTAL";
      else
	orientation = "GTK_ORIENTATION_VERTICAL";

      /* We add the floating item to the layout. I think that is OK.
	 FIXME: Ideally we don't want the position of the floating item to
	 be set explicitly, so the user has to position in. But I don't know
	 if that can be done. So what should we do? */

      x = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (parent),
						"glade-dock-item-x"));
      y = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (parent),
						"glade-dock-item-y"));

      source_add (data,
		  "  bonobo_dock_layout_add_floating_item (BONOBO_DOCK_LAYOUT (GNOME_APP (%s)->layout),\n"
		  "                                       BONOBO_DOCK_ITEM (%s), %i, %i,\n"
		  "                                       %s);\n",
		  data->component_name, data->wname, x, y, orientation);

      gtk_object_set_data (GTK_OBJECT (parent), "glade-dock-item-x",
			   GINT_TO_POINTER (x + 50));
      gtk_object_set_data (GTK_OBJECT (parent), "glade-dock-item-y",
			   GINT_TO_POINTER (y + 50));
    }
  else
    {
      source_add (data,
		  "  gnome_app_set_contents (GNOME_APP (%s), %s);\n",
		  data->component_name, data->wname);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_bonobo_dock_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = bonobo_dock_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = bonobo_dock_xpm;
  gbwidget.tooltip = _("Gnome Dock");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_bonobo_dock_new;
  gbwidget.gb_widget_create_from_widget	= gb_bonobo_dock_create_from_widget;
  gbwidget.gb_widget_add_child = gb_bonobo_dock_add_child;
  gbwidget.gb_widget_create_properties	= gb_bonobo_dock_create_properties;
  gbwidget.gb_widget_get_properties	= gb_bonobo_dock_get_properties;
  gbwidget.gb_widget_set_properties	= gb_bonobo_dock_set_properties;
  gbwidget.gb_widget_create_popup_menu	= gb_bonobo_dock_create_popup_menu;
  gbwidget.gb_widget_write_source	= gb_bonobo_dock_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_bonobo_dock_write_add_child_source;

  return &gbwidget;
}

