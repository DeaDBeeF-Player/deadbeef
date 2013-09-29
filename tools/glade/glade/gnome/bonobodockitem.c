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
#include <bonobo.h>
#include "../gb.h"
#include "../glade_gnome.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/bonobo-dock-item.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *ShadowType = "BonoboDockItem::shadow_type";
static gchar *Orientation = "BonoboDockItem::orientation";

/* These 5 are boolean flags in the property editor, but saved as one property,
   "behavior" in the XML. */
static gchar *Locked = "BonoboDockItem::locked";
static gchar *Exclusive = "BonoboDockItem::exclusive";
static gchar *NeverFloating = "BonoboDockItem::never_floating";
static gchar *NeverVertical = "BonoboDockItem::never_vertical";
static gchar *NeverHorizontal = "BonoboDockItem::never_horizontal";

/* These are only used for loading & saving - they are not displayed in the
   property editor. */
static gchar *Placement = "BonoboDockItem::placement";
static gchar *BandNum = "BonoboDockItem::band";
static gchar *Position = "BonoboDockItem::position";
static gchar *Offset = "BonoboDockItem::offset";
static gchar *Behavior = "BonoboDockItem::behavior";

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


static void gb_bonobo_dock_item_init_widget (GtkWidget *widget);
static void gb_bonobo_dock_item_drag_end (GtkWidget *widget,
					 gpointer user_data);

static void gb_bonobo_dock_item_add_item_before (GtkWidget * menuitem,
						BonoboDockItem * dock_item);
static void gb_bonobo_dock_item_add_item_after (GtkWidget * menuitem,
					       BonoboDockItem * dock_item);
static void gb_bonobo_dock_item_add_item (BonoboDockItem * existing_dock_item,
					 gboolean after);
static gboolean gb_bonobo_dock_item_find_position (BonoboDockItem * dock_item,
						  BonoboDockPlacement *placement,
						  gint *band_num,
						  gint *position,
						  gint *offset);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the funtion in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class BonoboDockItem, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
static GtkWidget*
gb_bonobo_dock_item_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = bonobo_dock_item_new (NULL, BONOBO_DOCK_ITEM_BEH_NORMAL);

  gb_bonobo_dock_item_init_widget (new_widget);

  return new_widget;
}


static void
gb_bonobo_dock_item_create_from_widget (GtkWidget *widget,
				       GbWidgetCreateFromData *data)
{
  gb_bonobo_dock_item_init_widget (widget);
}


static void
gb_bonobo_dock_item_init_widget (GtkWidget *widget)
{
  /* Connect a handler so we can update the properties if the item is moved. */
  gtk_signal_connect_after (GTK_OBJECT (widget), "dock_drag_end",
			    GTK_SIGNAL_FUNC (gb_bonobo_dock_item_drag_end),
			    NULL);
}


/* If the widgets properties are currently shown, we update them if necessary.
 */
static void
gb_bonobo_dock_item_drag_end (GtkWidget *widget, gpointer user_data)
{
  if (property_get_widget () == widget)
    {
      /* The orientation is only useful for floating items. */
      property_set_sensitive (Orientation,
			      BONOBO_DOCK_ITEM (widget)->is_floating);
    }
}


void
gb_bonobo_dock_item_add_child (GtkWidget *widget, GtkWidget * child,
			      GbWidgetSetArgData *data)
{
  /* Try to set up menubars and toolbars just like GnomeApp does.
     We don't do anything now. I'm not sure what we should do. */

  gtk_container_add (GTK_CONTAINER (widget), child);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_bonobo_dock_item_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (Locked, _("Locked:"),
		     _("If the dock item is locked in position"));
  property_add_bool (Exclusive, _("Exclusive:"),
		     _("If the dock item is always the only item in its band"));
  property_add_bool (NeverFloating, _("Never Floating:"),
		     _("If the dock item is never allowed to float in its own window"));
  property_add_bool (NeverVertical, _("Never Vertical:"),
		     _("If the dock item is never allowed to be vertical"));
  property_add_bool (NeverHorizontal, _("Never Horizontal:"),
		     _("If the dock item is never allowed to be horizontal"));

  property_add_choice (ShadowType, _("Shadow:"),
		       _("The type of shadow around the dock item"),
		       GbShadowChoices);
  property_add_choice (Orientation, _("Orientation:"),
		       _("The orientation of a floating dock item"),
		       GladeOrientationChoices);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_bonobo_dock_item_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gint i;

  if (BONOBO_DOCK_ITEM (widget)->is_floating)
    {
      /* Orientation is only relevant when floating. */
      for (i = 0; i < GladeOrientationSize; i++)
	{
	  if (GladeOrientationValues[i] == BONOBO_DOCK_ITEM (widget)->orientation)
	    {
	      gb_widget_output_choice (data, Orientation, i,
				       GladeOrientationSymbols[i]);
	    }
	}
    }

  if (data->action == GB_SHOWING)
    {
      gb_widget_output_bool (data, Locked, BONOBO_DOCK_ITEM (widget)->behavior & BONOBO_DOCK_ITEM_BEH_LOCKED);
      gb_widget_output_bool (data, Exclusive, BONOBO_DOCK_ITEM (widget)->behavior & BONOBO_DOCK_ITEM_BEH_EXCLUSIVE);
      gb_widget_output_bool (data, NeverFloating, BONOBO_DOCK_ITEM (widget)->behavior & BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING);
      gb_widget_output_bool (data, NeverVertical, BONOBO_DOCK_ITEM (widget)->behavior & BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL);
      gb_widget_output_bool (data, NeverHorizontal, BONOBO_DOCK_ITEM (widget)->behavior & BONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL);
    }

  for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]); i++)
    {
      if (GbShadowValues[i] == BONOBO_DOCK_ITEM (widget)->shadow_type)
	gb_widget_output_choice (data, ShadowType, i, GbShadowSymbols[i]);
    }

  if (data->action == GB_SHOWING)
    {
      /* The orientation is only useful for floating items. */
      property_set_sensitive (Orientation,
			      BONOBO_DOCK_ITEM (widget)->is_floating);
    }
}


/* SPECIAL CODE: "placement", "band", "position", "offset" and "behavior"
   are now packing properties, so we have this special function to save
   them, which is called directly in gb_widget_save(). */
void
gb_bonobo_dock_item_save_packing_properties (GtkWidget *parent,
					     GtkWidget *widget,
					     GbWidgetGetArgData * data)
{
  char *behavior;

  save_start_tag (data, "packing");

  if (BONOBO_DOCK_ITEM (widget)->is_floating)
    {
      gb_widget_output_string (data, Placement, "BONOBO_DOCK_FLOATING");
    }
  else
    {
      BonoboDockPlacement placement;
      gint band_num, position, offset, idx;

      if (gb_bonobo_dock_item_find_position (BONOBO_DOCK_ITEM (widget),
					     &placement, &band_num,
					     &position, &offset))
	{
	  idx = glade_util_int_array_index (GladePlacementValues,
					    GladePlacementSize,
					    placement);
	  if (idx != -1)
	    gb_widget_output_string (data, Placement,
				     GladePlacementSymbols[idx]);

	  gb_widget_output_int (data, BandNum, band_num);
	  gb_widget_output_int (data, Position, position);
	  gb_widget_output_int (data, Offset, offset);
	}
      else
	{
	  g_warning ("Dock band not found");
	}
    }

  behavior = glade_util_string_from_flags (BONOBO_TYPE_DOCK_ITEM_BEHAVIOR,
					   BONOBO_DOCK_ITEM (widget)->behavior);
  save_string (data, Behavior, behavior);
  g_free (behavior);

  save_end_tag (data, "packing");
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_bonobo_dock_item_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean locked, exclusive, never_floating, never_vertical, never_horizontal;
  gchar *shadow, *orientation;
  BonoboDockItemBehavior behavior;
  gint i;

  /* This is a bit of a hack, as some of the properties are now saved as
     packing properties instead of normal properties. */
  if (data->action == GB_LOADING)
    {
      gchar *behavior_string;

      data->loading_type = GB_CHILD_PROPERTIES;

      behavior_string = load_string (data, Behavior);
      behavior = glade_util_flags_from_string (BONOBO_TYPE_DOCK_ITEM_BEHAVIOR,
					       behavior_string);

      data->loading_type = GB_STANDARD_PROPERTIES;
    }
  else
    {
      behavior = BONOBO_DOCK_ITEM (widget)->behavior;

      locked = gb_widget_input_bool (data, Locked);
      if (data->apply)
	{
	  if (locked)
	    behavior |= BONOBO_DOCK_ITEM_BEH_LOCKED;
	  else
	    behavior &= ~BONOBO_DOCK_ITEM_BEH_LOCKED;

	  /* This avoids any problems with redrawing the selection. */
	  if (data->action == GB_APPLYING)
	    editor_clear_selection (NULL);

	  gtk_widget_queue_resize (widget);
	}

      exclusive = gb_widget_input_bool (data, Exclusive);
      if (data->apply)
	{
	  if (exclusive)
	    behavior |= BONOBO_DOCK_ITEM_BEH_EXCLUSIVE;
	  else
	    behavior &= ~BONOBO_DOCK_ITEM_BEH_EXCLUSIVE;
	}

      never_floating = gb_widget_input_bool (data, NeverFloating);
      if (data->apply)
	{
	  if (never_floating)
	    behavior |= BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING;
	  else
	    behavior &= ~BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING;
	}

      never_vertical = gb_widget_input_bool (data, NeverVertical);
      if (data->apply)
	{
	  if (never_vertical)
	    behavior |= BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL;
	  else
	    behavior &= ~BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL;
	}

      never_horizontal = gb_widget_input_bool (data, NeverHorizontal);
      if (data->apply)
	{
	  if (never_horizontal)
	    behavior |= BONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL;
	  else
	    behavior &= ~BONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL;
	}
    }

  /* BonoboDockItem has no method for setting the behavior. */
  BONOBO_DOCK_ITEM (widget)->behavior = behavior;


  shadow = gb_widget_input_choice (data, ShadowType);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbShadowValues) / sizeof (GbShadowValues[0]);
	   i++)
	{
	  if (!strcmp (shadow, GbShadowChoices[i])
	      || !strcmp (shadow, GbShadowSymbols[i]))
	    {
	      bonobo_dock_item_set_shadow_type (BONOBO_DOCK_ITEM (widget),
					       GbShadowValues[i]);
	      break;
	    }
	}
    }

  orientation = gb_widget_input_choice (data, Orientation);
  if (data->apply)
    {
      for (i = 0; i < GladeOrientationSize; i++)
	{
	  if (!strcmp (orientation, GladeOrientationChoices[i])
	      || !strcmp (orientation, GladeOrientationSymbols[i]))
	    {
	      bonobo_dock_item_set_orientation (BONOBO_DOCK_ITEM (widget),
					       GladeOrientationValues[i]);

	      /* This avoids any problems with redrawing the selection. */
	      if (data->action == GB_APPLYING)
		editor_clear_selection (NULL);

	      /* FIXME: A test to see if changing orientation works. */
	      if (BONOBO_DOCK_ITEM (widget)->bin.child)
		gtk_widget_queue_resize (BONOBO_DOCK_ITEM (widget)->bin.child);

	      break;
	    }
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a BonoboDockItem, with signals pointing to
 * other functions in this file.
 */
static void
gb_bonobo_dock_item_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;

  /* We can't add items next to floating items or items that have the
     BONOBO_DOCK_ITEM_BEH_EXCLUSIVE flag set. */
  if (!BONOBO_DOCK_ITEM (widget)->is_floating
      && !(BONOBO_DOCK_ITEM (widget)->behavior & BONOBO_DOCK_ITEM_BEH_EXCLUSIVE))
    {
      menuitem = gtk_menu_item_new_with_label (_("Add dock item before"));
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_bonobo_dock_item_add_item_before),
			  widget);

      menuitem = gtk_menu_item_new_with_label (_("Add dock item after"));
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_bonobo_dock_item_add_item_after),
			  widget);
    }
}


static void
gb_bonobo_dock_item_add_item_before (GtkWidget * menuitem,
				    BonoboDockItem * dock_item)
{
  gb_bonobo_dock_item_add_item (dock_item, FALSE);
}


static void
gb_bonobo_dock_item_add_item_after (GtkWidget * menuitem,
				   BonoboDockItem * dock_item)
{
  gb_bonobo_dock_item_add_item (dock_item, TRUE);
}


static void
gb_bonobo_dock_item_add_item (BonoboDockItem * existing_dock_item,
			     gboolean after)
{
  BonoboDock *dock;
  BonoboDockBand *dock_band;
  GtkWidget *dock_item, *placeholder;
  BonoboDockPlacement placement;
  gint band_num, position, offset;

  if (!gb_bonobo_dock_item_find_position (BONOBO_DOCK_ITEM (existing_dock_item),
					 &placement, &band_num,
					 &position, &offset))
    {
      g_warning ("Dock band not found");
      return;
    }

  dock_band = BONOBO_DOCK_BAND (GTK_WIDGET (existing_dock_item)->parent);
  dock = BONOBO_DOCK (GTK_WIDGET (dock_band)->parent);

  /* Create the new dock item. */
  dock_item = gb_widget_new ("BonoboDockItem", NULL);

  placeholder = editor_new_placeholder ();
  gtk_container_add (GTK_CONTAINER (dock_item), placeholder);

  /* Now add it at the required position. */
  if (after)
    position++;
  bonobo_dock_add_item (dock, BONOBO_DOCK_ITEM (dock_item), placement, band_num,
		       position, 0, FALSE);

  gtk_widget_show (dock_item);

  /* Show the properties of the new dock item. */
  gb_widget_show_properties (dock_item);
}


/* This gets the placement, band number, position and offset of dock items
   which are not floating. It returns TRUE if the dock item is found. */
static gboolean
gb_bonobo_dock_item_find_position (BonoboDockItem * dock_item,
				  BonoboDockPlacement *placement,
				  gint *band_num,
				  gint *position,
				  gint *offset)
{
  BonoboDock *dock;
  BonoboDockBand *dock_band;
  BonoboDockBandChild *dock_band_child;
  gint pos;
  GList *elem;

  g_return_val_if_fail (BONOBO_IS_DOCK_BAND (GTK_WIDGET (dock_item)->parent),
			FALSE);
  dock_band = BONOBO_DOCK_BAND (GTK_WIDGET (dock_item)->parent);

  g_return_val_if_fail (BONOBO_IS_DOCK (GTK_WIDGET (dock_band)->parent), FALSE);
  dock = BONOBO_DOCK (GTK_WIDGET (dock_band)->parent);

  /* First we find out which band the existing dock item is. */
  if ((*band_num = g_list_index (dock->top_bands, dock_band)) != -1)
    *placement = BONOBO_DOCK_TOP;
  else if ((*band_num = g_list_index (dock->bottom_bands, dock_band)) != -1)
    *placement = BONOBO_DOCK_BOTTOM;
  else if ((*band_num = g_list_index (dock->left_bands, dock_band)) != -1)
    *placement = BONOBO_DOCK_LEFT;
  else if ((*band_num = g_list_index (dock->right_bands, dock_band)) != -1)
    *placement = BONOBO_DOCK_RIGHT;
  else
    return FALSE;

  /* Now find the position of the existing dock item within the band. */
  for (elem = dock_band->children, pos = 0;
       elem != NULL;
       elem = elem->next, pos++)
    {
      dock_band_child = (BonoboDockBandChild*) elem->data;
      if (dock_band_child->widget == GTK_WIDGET (dock_item))
        {
	  *position = pos;
	  *offset = dock_band_child->offset;
	  return TRUE;
        }
    }
  return FALSE;
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_bonobo_dock_item_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  /* If we are in a GnomeApp's dock, we don't create the dock item here, or
     add it to the BonoboDock, since it is created automatically when the item
     is added to the GnomeApp. */
  if (glade_gnome_is_app_dock_item (widget)
      && !BONOBO_DOCK_ITEM (widget)->is_floating)
    {
      return;
    }

  if (data->create_widget)
    {
      BonoboDockItemBehavior behavior;
      const gchar *prefix = "\n                                ";
      const gchar *prefix2 = "\n                                | ";

      behavior = BONOBO_DOCK_ITEM (widget)->behavior;

      source_add (data,
		  "  %s = bonobo_dock_item_new (%s,",
		  data->wname, source_make_string (data->real_wname, FALSE));
      if (behavior == 0)
	{
	  source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NORMAL);\n", prefix);
	}
      else
	{
	  if (behavior & BONOBO_DOCK_ITEM_BEH_EXCLUSIVE)
	    {
	      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_EXCLUSIVE",
			  prefix);
	      prefix = prefix2;
	    }
	  if (behavior & BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING)
	    {
	      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NEVER_FLOATING",
			  prefix);
	      prefix = prefix2;
	    }
	  if (behavior & BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL)
	    {
	      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL",
			  prefix);
	      prefix = prefix2;
	    }
	  if (behavior & BONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL)
	    {
	      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL",
			  prefix);
	      prefix = prefix2;
	    }
	  if (behavior & BONOBO_DOCK_ITEM_BEH_LOCKED)
	    {
	      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_LOCKED",
			  prefix);
	    }

	  source_add (data, ");\n");
	}
    }

  gb_widget_write_standard_source (widget, data);

}


/* Outputs source to add a child menu to a BonoboDock. */
static void
gb_bonobo_dock_item_write_add_child_source (GtkWidget * parent,
					   const gchar *parent_name,
					   GtkWidget *child,
					   GbWidgetWriteSourceData * data)
{
  GnomeApp *app;

  /* If we're adding a dock item to a GnomeApp's dock, we use
     the special functions to add it here. */
  if ((app = glade_gnome_is_app_dock_item (parent)))
    {
      /* Children of floating items are added as normal. */
      if (BONOBO_DOCK_ITEM (parent)->is_floating)
	{
	  source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		      parent_name, data->wname);
	}
      else if (GTK_IS_MENU_BAR (child))
	{
	  source_add (data,
		      "  gnome_app_create_menus (GNOME_APP (%s), %s_uiinfo);\n",
		      data->component_name, data->real_wname);

	  /* Output the code to install the menu hints, if the GnomeApp has
	     a status bar. This must be output after the code to create the
	     GnomeAppBar is output, so we add it to the same buffer as the
	     signal connections. */
	  if (app->statusbar)
	    {
	      source_add_to_buffer (data, GLADE_SIGNAL_CONNECTIONS,
				    "  gnome_app_install_menu_hints (GNOME_APP (%s), %s_uiinfo);\n",
				    data->component_name, data->real_wname);
	    }
	}
      else
	{
	  BonoboDockPlacement placement;
	  BonoboDockItemBehavior behavior;
	  const gchar *placement_string;
	  gint idx, band_num, position, offset;
	  gchar *prefix, *prefix2;

	  if (gb_bonobo_dock_item_find_position (BONOBO_DOCK_ITEM (parent),
						&placement, &band_num,
						&position, &offset))
	    {
	      idx = glade_util_int_array_index (GladePlacementValues,
						GladePlacementSize,
						placement);
	      if (idx == -1)
		{
		  g_warning ("BonoboDock placement not found");
		  placement = 0;
		}
	      placement_string = GladePlacementSymbols[idx];

	      if (GTK_IS_TOOLBAR (child))
		{
		  source_add (data,
			      "  gnome_app_add_toolbar (GNOME_APP (%s), GTK_TOOLBAR (%s), %s,\n",
			      data->component_name, data->wname,
			      source_make_string (data->wname, FALSE));
		}
	      else
		{
		  source_add (data,
			      "  gnome_app_add_docked (GNOME_APP (%s), %s, %s,\n",
			      data->component_name, data->wname,
			      source_make_string (data->wname, FALSE));
		}

	      source_add (data, "                                ");
	      behavior = BONOBO_DOCK_ITEM (parent)->behavior;
	      prefix = "";
	      prefix2 = "\n                                | ";
	      if (behavior == BONOBO_DOCK_ITEM_BEH_NORMAL)
		{
		  source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NORMAL", prefix);
		}
	      else
		{
		  if (behavior & BONOBO_DOCK_ITEM_BEH_EXCLUSIVE)
		    {
		      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_EXCLUSIVE",
				  prefix);
		      prefix = prefix2;
		    }
		  if (behavior & BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING)
		    {
		      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NEVER_FLOATING",
				  prefix);
		      prefix = prefix2;
		    }
		  if (behavior & BONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL)
		    {
		      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NEVER_VERTICAL",
				  prefix);
		      prefix = prefix2;
		    }
		  if (behavior & BONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL)
		    {
		      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_NEVER_HORIZONTAL",
				  prefix);
		      prefix = prefix2;
		    }
		  if (behavior & BONOBO_DOCK_ITEM_BEH_LOCKED)
		    {
		      source_add (data, "%sBONOBO_DOCK_ITEM_BEH_LOCKED",
				  prefix);
		      prefix = prefix2;
		    }
		}

	      source_add (data,
			  ",\n"
			  "                                %s, %i, %i, %i);\n",
			  placement_string, band_num, position, offset);
	    }
	}
    }
  else
    {
      g_warning ("Skipping adding dock item to parent - unimplemented.");
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_bonobo_dock_item_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = bonobo_dock_item_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = bonobo_dock_item_xpm;
  gbwidget.tooltip = _("Gnome Dock Item");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_bonobo_dock_item_new;
  gbwidget.gb_widget_create_from_widget	= gb_bonobo_dock_item_create_from_widget;
  gbwidget.gb_widget_add_child		= gb_bonobo_dock_item_add_child;
  gbwidget.gb_widget_create_popup_menu	= gb_bonobo_dock_item_create_popup_menu;
  gbwidget.gb_widget_create_properties	= gb_bonobo_dock_item_create_properties;
  gbwidget.gb_widget_get_properties	= gb_bonobo_dock_item_get_properties;
  gbwidget.gb_widget_set_properties	= gb_bonobo_dock_item_set_properties;
  gbwidget.gb_widget_write_source	= gb_bonobo_dock_item_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_bonobo_dock_item_write_add_child_source;

  return &gbwidget;
}

