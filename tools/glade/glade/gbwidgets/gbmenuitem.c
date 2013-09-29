
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

#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkmenuitem.h>
#include "../gb.h"
#include "../glade_gnome.h"
#include "../glade_keys_dialog.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/menuitem.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Label = "MenuItem|GtkItem::label";
static gchar *Icon = "GtkMenuItem::icon";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkMenuItem, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_menu_item_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_CREATING)
    new_widget = gtk_menu_item_new_with_label (data->name);
  else
    new_widget = gtk_menu_item_new ();
  return new_widget;
}

void
gb_menu_item_add_child (GtkWidget * widget, GtkWidget * child, GbWidgetSetArgData *data)
{
  if (GTK_IS_MENU (child))
    {
      MSG ("Trying to add a menu to a menu item");
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), child);
    }
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_menu_item_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
#if 0
  /* For now we don't support editing the menuitem properties in the property
     editor. The menu editor has to be used instead. */
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
#endif
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_menu_item_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gboolean output_label = TRUE;

  /* We only support saving the properties here. */
  if (data->action != GB_SAVING)
    return;

  /* If we are saving, we check for a stock Gnome menu item, and if so, we
     just save that. GTK+ stock items are all GtkImageMenuItems, so we don't
     have to worry about those here. */
#ifdef USE_GNOME
  if (glade_project_get_gnome_support (data->project))
    {
      gint stock_item_index;

      stock_item_index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), GladeMenuItemStockIndexKey));
      /* The 'New' item is special. If it has a child menu, it must be a
	 GNOMEUIINFO_MENU_NEW_SUBTREE. If not, it is a
	 GNOMEUIINFO_MENU_NEW_ITEM, in which case the label is also output. */
      if (stock_item_index == GladeStockMenuItemNew)
	{
	  if (GTK_MENU_ITEM (widget)->submenu)
	    {
	      gb_widget_output_string (data, "stock_item",
				       "GNOMEUIINFO_MENU_NEW_SUBTREE");
	      output_label = FALSE;
	    }
	  else
	    {
	      gb_widget_output_string (data, "stock_item",
				       "GNOMEUIINFO_MENU_NEW_ITEM");
	    }
	}
      else if (stock_item_index != 0)
	{
	  gb_widget_output_string (data, "stock_item", GladeStockMenuItemSymbols[stock_item_index]);
	  output_label = FALSE;
	}
    }
#endif

  if (output_label)
    gb_widget_output_child_label (widget, data, Label);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_menu_item_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean input_label = TRUE, input_rest = TRUE;
  GtkAccelGroup *accel_group;
  guint key;
  GdkModifierType modifiers;

  /* We only support loading the properties here. */
  if (data->action != GB_LOADING)
    return;

#ifdef USE_GNOME
  /* Check for a stock menu item. */
  if (glade_project_get_gnome_support (data->project))
    {
      GnomeUIInfo *uiinfo;
      gchar *stock_item;
      GtkWidget *label;
      gint stock_item_index;

      stock_item = gb_widget_input_string (data, "stock_item");
      if (stock_item && stock_item[0])
	{
	  /* Special case for the NEW_SUBTREE. */
	  if (!strcmp (stock_item, "GNOMEUIINFO_MENU_NEW_SUBTREE"))
	    {
	      stock_item_index = GladeStockMenuItemNew;
	    }
	  else
	    {
	      stock_item_index = glade_util_string_array_index (GladeStockMenuItemSymbols, GladeStockMenuItemSize, stock_item);
	    }

	  if (stock_item_index != -1)
	    {
	      uiinfo = &GladeStockMenuItemValues[stock_item_index];
	      if (uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
		gnome_app_ui_configure_configurable (uiinfo);

	      /* Note that we don't have to worry about the pixmap, since if
		 it had a pixmap it would be a GtkImageMenuItem. */

	      label = gtk_accel_label_new ("");
	      gtk_label_set_text_with_mnemonic (GTK_LABEL (label),
						uiinfo->label);
	      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	      gtk_widget_show (label);
	      gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label),
						widget);
	      gtk_container_add (GTK_CONTAINER (widget), label);

	      /* Add the configured accelerator key. */
	      if (uiinfo->accelerator_key != 0 && widget->parent
		  && GTK_IS_MENU (widget->parent))
		{
		  accel_group = GTK_MENU (widget->parent)->accel_group;
		  gtk_widget_add_accelerator (widget, "activate", accel_group,
					      uiinfo->accelerator_key,
					      uiinfo->ac_mods,
					      GTK_ACCEL_VISIBLE);
		}

	      /* Remember the index of the stock item. */
	      gtk_object_set_data (GTK_OBJECT (widget),
				   GladeMenuItemStockIndexKey,
				   GINT_TO_POINTER (stock_item_index));

	      /* The 'New' item can have a label. The rest can't. */
	      if (stock_item_index != GladeStockMenuItemNew)
		input_label = FALSE;
	      input_rest = FALSE;
	    }
	  else
	    {
#ifdef FIXME
	      load_add_error_message_with_tag (data,
					       GLADE_LINE_PROPERTY,
					       _("Invalid stock menu item"),
					       "stock_item", stock_item);
#endif
	    }
	}
    }
#endif

  if (input_label)
    gb_widget_input_child_label (widget, data, Label);

  if (input_rest)
    {
      /* FIXME: should this be somewhere else? */
      /* If we are loading, install the 'activate' accelerator, if it has one,
	 so that is is visible. */
      if (data->action == GB_LOADING && widget->parent
	  && GTK_IS_MENU (widget->parent))
	{
	  int i;

	  for (i = 0; i < data->widget_info->n_accels; i++)
	    {
	      if (!strcmp (data->widget_info->accels[i].signal, "activate"))
		{
		  key = data->widget_info->accels[i].key;
		  modifiers = data->widget_info->accels[i].modifiers;
		  accel_group = GTK_MENU (widget->parent)->accel_group;
		  gtk_widget_add_accelerator (widget, "activate", accel_group,
					      key, modifiers,
					      GTK_ACCEL_VISIBLE);
		  break;
		}
	    }
	}
    }
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkMenuItem, with signals pointing to
 * other functions in this file.
 */
static void
gb_menu_item_create_popup_menu (GtkWidget * widget,
				GbWidgetCreateMenuData * data)
{
  /* Add command to remove child label. */
#if 0
  gb_widget_create_child_label_popup_menu (widget, data);
#endif
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_menu_item_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GtkWidget *child = GTK_BIN (widget)->child;
  gchar *label_text;
  gboolean translatable, context;
  gchar *comments;

#ifdef USE_GNOME
  if (data->project->gnome_support)
    {
      glade_gnome_write_menu_item_source (GTK_MENU_ITEM (widget), data);
      return;
    }
#endif

  if (child && GTK_IS_LABEL (child) && !GB_IS_GB_WIDGET (child))
    {
      glade_util_get_translation_properties (widget, Label, &translatable,
					     &comments, &context);
      source_add_translator_comments (data, translatable, comments);

      label_text = glade_util_get_label_text (child);
      source_add (data, "  %s = gtk_menu_item_new_with_mnemonic (%s);\n",
		  data->wname,
		  source_make_string_full (label_text, data->use_gettext && translatable, context));
      g_free (label_text);
    }
  else
    {
      source_add (data, "  %s = gtk_menu_item_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);
}


/* Outputs source to add a child menu to a menu item. */
static void
gb_menu_item_write_add_child_source (GtkWidget * parent,
				     const gchar *parent_name,
				     GtkWidget *child,
				     GbWidgetWriteSourceData * data)
{
  if (GTK_IS_MENU (child))
    {
      source_add (data,
		  "  gtk_menu_item_set_submenu (GTK_MENU_ITEM (%s), %s);\n",
		  parent_name, data->wname);
    }
  else
    {
      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		  parent_name, data->wname);
    }
}


static void
gb_menu_item_destroy (GtkWidget * widget, GbWidgetDestroyData * data)
{
  /* We need to remove the icon pixmap from the project.
     Note that it may be a stock icon name, in which case it will be ignored
     by glade_project_remove_pixmap. */
  gchar *filename = gtk_object_get_data (GTK_OBJECT (widget), Icon);
  if (filename)
    {
      glade_project_remove_pixmap (data->project, filename);
      g_free (filename);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_menu_item_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_menu_item_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = menuitem_xpm;
  gbwidget.tooltip = _("Menu Item");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_menu_item_new;
  gbwidget.gb_widget_add_child = gb_menu_item_add_child;
  gbwidget.gb_widget_create_properties = gb_menu_item_create_properties;
  gbwidget.gb_widget_get_properties = gb_menu_item_get_properties;
  gbwidget.gb_widget_set_properties = gb_menu_item_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_menu_item_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_menu_item_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_menu_item_write_add_child_source;
  gbwidget.gb_widget_destroy = gb_menu_item_destroy;

  return &gbwidget;
}
