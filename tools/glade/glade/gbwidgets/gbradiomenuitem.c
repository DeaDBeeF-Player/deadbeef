
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
#include <gtk/gtkradiomenuitem.h>
#include "../gb.h"
#include "../glade_gnome.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/radiomenuitem.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *Label = "RadioMenuItem|GtkItem::label";
static gchar *State = "RadioMenuItem|GtkCheckMenuItem::active";
static gchar *Group = "GtkRadioMenuItem::group";


typedef struct _GladeFindGroupWidgetData GladeFindGroupWidgetData;
struct _GladeFindGroupWidgetData {
  gchar *name;
  GtkWidget *found_widget;
};


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

static void
find_parents_group (GtkWidget * widget, GSList ** group)
{
  /* If a group has already been found, return. */
  if (*group)
    return;

  if (GTK_IS_RADIO_MENU_ITEM (widget))
    {
      *group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (widget));
    }
}


/*
 * Creates a new GtkWidget of class GtkRadioMenuItem, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_radio_menu_item_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;
  GSList *group_list = NULL;

  /* When creating a radiomenuitem we try to place it in the same group as
     other radiomenuitems with the same parent. */
  if (data->parent && data->action == GB_CREATING)
    gtk_container_foreach (GTK_CONTAINER (data->parent),
			   (GtkCallback) find_parents_group, &group_list);

  if (data->action == GB_CREATING)
    new_widget = gtk_radio_menu_item_new_with_label (group_list, data->name);
  else
    new_widget = gtk_radio_menu_item_new (group_list);
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_radio_menu_item_create_properties (GtkWidget * widget, GbWidgetCreateArgData
				      * data)
{
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_bool (State, _("Initially On:"),
		     _("If the radio menu item is initially on"));
  property_add_combo (Group, _("Group:"),
		      _("The radio menu item group (the default is all radio menu items with the same parent)"),
		      NULL);
  /* The Group property is only used for loading and saving at present. */
  property_set_visible (Group, FALSE);
}



void
gb_radio_menu_item_find_radio_group (GtkWidget *widget,
				     GladeFindGroupData *find_data)
{
  if (find_data->found_widget)
    return;

  if (GTK_IS_RADIO_MENU_ITEM (widget) && GB_IS_GB_WIDGET (widget))
    {
      if (gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (widget)) == find_data->group)
	{
	  find_data->found_widget = widget;
	  return;
	}
    }

  if (GTK_IS_CONTAINER (widget))
    gb_widget_children_foreach (widget,
				(GtkCallback) gb_radio_menu_item_find_radio_group,
				find_data);
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_radio_menu_item_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_widget_output_child_label (widget, data, Label);

  gb_widget_output_bool (data, State, data->widget_data->flags & GLADE_ACTIVE);

  /* We only output the group when saving, for now. */
  if (data->action == GB_SAVING)
    {
      GladeFindGroupData find_data;
      GtkWidget *component;

      component = glade_util_get_toplevel (widget);

      find_data.group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (widget));
      find_data.found_widget = NULL;
      gb_radio_menu_item_find_radio_group (component, &find_data);

      if (find_data.found_widget)
	{
	  /* We don't output the group if this widget is the first widget in
	     the group. */
	  if (find_data.found_widget != widget)
	    {
	      const char *name;
	      name = gtk_widget_get_name (find_data.found_widget);
	      gb_widget_output_combo (data, Group, name);
	    }
	}
    }
}


/* This recursively steps though a complete component's hierarchy to find
   a radio button with a particular group name set. When found the radio
   button's group list is returned in find_group_data->group. */
static void
find_group_widget (GtkWidget *widget, GladeFindGroupWidgetData *find_data)
{
  if (find_data->found_widget)
    return;

  if (GTK_IS_RADIO_MENU_ITEM (widget) && GB_IS_GB_WIDGET (widget))
    {
      if (!strcmp (gtk_widget_get_name (widget), find_data->name))
	{
#if 0
	  g_print ("Found widget: %s\n", find_data->name);
#endif
	  find_data->found_widget = widget;
	  return;
	}
    }

  if (GTK_IS_CONTAINER (widget))
    gb_widget_children_foreach (widget, (GtkCallback) find_group_widget,
				find_data);
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_radio_menu_item_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean state;

  gb_widget_input_child_label (widget, data, Label);

  state = gb_widget_input_bool (data, State);
  if (data->apply)
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), state);
      if (state)
	data->widget_data->flags |= GLADE_ACTIVE;
      else
	data->widget_data->flags &= ~GLADE_ACTIVE;
    }

  /* We only set the group when loading, for now. */
  if (data->action == GB_LOADING)
    {
      gchar *group_name;

      /* Find any widgets in given group and set this widgets group.
	 If group is NULL try to find radiomenuitems with same parent and use
	 their group. If these don't succeed, set group to NULL. */
      group_name = gb_widget_input_combo (data, Group);
      if (data->apply)
	{
	  GSList *old_group, *new_group = NULL;

	  old_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (widget));

	  if (group_name && group_name[0] == '\0')
	    group_name = NULL;

	  if (group_name)
	    {
	      GladeFindGroupWidgetData find_data;
	      GtkWidget *component;

	      component = glade_util_get_toplevel (widget);

	      find_data.name = group_name;
	      find_data.found_widget = NULL;
	      find_group_widget (component, &find_data);

	      if (find_data.found_widget)
		new_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (find_data.found_widget));
	      else if (data->action == GB_LOADING)
		g_warning ("Invalid radio group: %s\n   (Note that forward references are not allowed in Glade files)", group_name);
	    }

	  if (new_group != old_group)
	    {
#if 0
	      g_print ("##### setting radio group: %s\n",
		       group_name ? group_name : "NULL");
#endif
	      gtk_radio_menu_item_set_group (GTK_RADIO_MENU_ITEM (widget),
					     new_group);
	    }
	}
    }
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkRadioMenuItem, with signals pointing to
 * other functions in this file.
 */
static void
gb_radio_menu_item_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData
				      * data)
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
gb_radio_menu_item_write_source (GtkWidget * widget, GbWidgetWriteSourceData *
				 data)
{
  GladeFindGroupData find_data;
  GtkWidget *group_widget;
  GtkWidget *child = GTK_BIN (widget)->child;
  gchar buffer[256], *group_name, *label_text;
  gboolean translatable, context;
  gchar *comments;

#ifdef USE_GNOME
  if (data->project->gnome_support)
    {
      glade_gnome_write_menu_item_source (GTK_MENU_ITEM (widget), data);
      return;
    }
#endif

  find_data.group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (widget));
  find_data.found_widget = NULL;
  gb_radio_menu_item_find_radio_group (glade_util_get_toplevel (widget),
				       &find_data);

  group_widget = find_data.found_widget;
  if (!group_widget)
    {
      g_warning ("Radiobutton has no group");
      group_widget = widget;
    }

  if (data->create_widget)
    {
      /* Make sure the temporary group list variable is declared. */
      group_name = (char*) gtk_widget_get_name (group_widget);
      group_name = source_create_valid_identifier (group_name);
      sprintf (buffer, "  GSList *%s_group = NULL;\n", group_name);
      source_ensure_decl (data, buffer);

      if (child && GTK_IS_LABEL (child) && !GB_IS_GB_WIDGET (child))
	{
	  label_text = glade_util_get_label_text (child);

	  glade_util_get_translation_properties (widget, Label, &translatable,
						 &comments, &context);
	  source_add_translator_comments (data, translatable, comments);

	  source_add (data,
		      "  %s = gtk_radio_menu_item_new_with_mnemonic (%s_group, %s);\n",
		      data->wname, group_name,
		      source_make_string_full (label_text, data->use_gettext && translatable, context));
	  g_free (label_text);
	}
      else
	{
	  source_add (data, "  %s = gtk_radio_menu_item_new (%s_group);\n",
		      data->wname, group_name);
	}
      source_add (data,
      "  %s_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (%s));\n",
		  group_name, data->wname);
      g_free (group_name);
    }

  gb_widget_write_standard_source (widget, data);

  if (data->widget_data->flags & GLADE_ACTIVE)
    {
      source_add (data,
      "  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (%s), TRUE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_radio_menu_item_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_radio_menu_item_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = radiomenuitem_xpm;
  gbwidget.tooltip = _("Radio Menu Item");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_radio_menu_item_new;
  gbwidget.gb_widget_create_properties = gb_radio_menu_item_create_properties;
  gbwidget.gb_widget_get_properties = gb_radio_menu_item_get_properties;
  gbwidget.gb_widget_set_properties = gb_radio_menu_item_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_radio_menu_item_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_radio_menu_item_write_source;

  return &gbwidget;
}
