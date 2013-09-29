
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

#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkradiobutton.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/radiobutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *StockButton = "RadioButton|GtkButton::stock_button";
static gchar *Label = "RadioButton|GtkButton::label";
static gchar *Icon = "RadioButton|GtkButton::icon";
static gchar *FocusOnClick = "RadioButton|GtkButton::focus_on_click";

/* This is only used for normal/stock buttons, not special toolbar buttons,
   as the toolbar has its own relief setting. */
static gchar *Relief = "RadioButton|GtkButton::relief";

static gchar *State = "RadioButton|GtkToggleButton::active";
static gchar *Inconsistent = "RadioButton|GtkToggleButton::inconsistent";
static gchar *Group = "GtkRadioButton::group";
static gchar *Indicator = "RadioButton|GtkToggleButton::draw_indicator";

typedef struct _GladeFindGroupsData GladeFindGroupsData;
struct _GladeFindGroupsData {
  GList *groups_found;
  GList *group_names;
};


typedef struct _GladeFindGroupWidgetData GladeFindGroupWidgetData;
struct _GladeFindGroupWidgetData {
  gchar *name;
  GtkWidget *found_widget;
};


static void get_radio_button_groups (GtkWidget * widget,
				     GladeFindGroupsData *find_data);



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

  if (GTK_IS_RADIO_BUTTON (widget))
    *group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
}


/*
 * Creates a new GtkWidget of class GtkRadioButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_radio_button_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;
  GSList *group_list = NULL;

  /* When creating a radiobutton we try to place it in the same group as other
     radio buttons with the same parent. */
  if (data->parent && data->action == GB_CREATING)
    gb_widget_children_foreach (data->parent, (GtkCallback) find_parents_group,
				&group_list);

  if (data->action == GB_CREATING)
    new_widget = gtk_radio_button_new_with_label (group_list, data->name);
  else
    {
      new_widget = gtk_radio_button_new (group_list);
      gtk_container_add (GTK_CONTAINER (new_widget), editor_new_placeholder());
    }
  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_radio_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData *
				   data)
{
  GtkWidget *combo;

  property_add_stock_item (StockButton, _("Stock Button:"),
			   _("The stock button to use"),
			   GTK_ICON_SIZE_BUTTON);
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GTK_ICON_SIZE_BUTTON);
  property_add_choice (Relief, _("Button Relief:"),
		       _("The relief style of the button"),
		       GladeReliefChoices);

  property_add_bool (State, _("Initially On:"),
		     _("If the radio button is initially on"));
  property_add_bool (Inconsistent, _("Inconsistent:"),
		     _("If the button is shown in an inconsistent state"));
  property_add_bool (Indicator, _("Indicator:"),
		     _("If the indicator is always drawn"));
  property_add_combo (Group, _("Group:"),
		      _("The radio button group (the default is all radio buttons with the same parent)"),
		      NULL);
  combo = property_get_value_widget (Group);
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (combo)->entry), FALSE);
  property_add_bool (FocusOnClick, _("Focus On Click:"), _("If the button grabs focus when it is clicked"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_radio_button_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GtkWidget *component;
  GladeFindGroupData find_data;

  component = glade_util_get_toplevel (widget);

  gb_button_get_standard_properties (widget, data, StockButton, Label, Icon,
				     Relief, FocusOnClick);

  gb_widget_output_bool (data, State, data->widget_data->flags & GLADE_ACTIVE);

  gb_widget_output_bool (data, Inconsistent,
			 GTK_TOGGLE_BUTTON (widget)->inconsistent);

  gb_widget_output_bool (data, Indicator,
			 GTK_TOGGLE_BUTTON (widget)->draw_indicator);

  /* If we're showing we need to display the list of groups to choose from.
     We walk the tree of widgets in this component, and if a widget is
     a radio button, we see if it has a group and if it is already in the
     list and if not we add it. */
  if (data->action == GB_SHOWING)
    {
      GladeFindGroupsData find_groups_data;

      find_groups_data.groups_found = NULL;
      find_groups_data.group_names = NULL;
      get_radio_button_groups (component, &find_groups_data);

      find_groups_data.group_names = g_list_prepend (find_groups_data.group_names,
						     _("New Group"));
      property_set_combo_strings (Group, find_groups_data.group_names);

      g_list_free (find_groups_data.groups_found);
      g_list_free (find_groups_data.group_names);

      /*property_set_visible (Indicator, !is_toolbar_button);*/
    }

  find_data.group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
  find_data.found_widget = NULL;
  gb_button_find_radio_group (component, &find_data);

  if (find_data.found_widget)
    {
      /* If we are saving, we don't output the group if this widget is the
	 first widget in the group. */
      if (data->action == GB_SHOWING || find_data.found_widget != widget)
	{
	  const char *name;
	  name = gtk_widget_get_name (find_data.found_widget);
	  gb_widget_output_combo (data, Group, name);
	}
    }
  else
    {
      g_warning ("Radiobutton has no group");
      gb_widget_output_combo (data, Group, "");
    }
}


/* Note that this must walk the widget tree in exactly the same way that we
   save the widgets, so we know which widget in the group will be the first
   output. */
static void
get_radio_button_groups (GtkWidget * widget, GladeFindGroupsData *find_data)
{
  if (GTK_IS_RADIO_BUTTON (widget) && GB_IS_GB_WIDGET (widget))
    {
      GSList *group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));

      /* See if we've already found a widget in this group. */
      if (!g_list_find (find_data->groups_found, group))
	{
	  const char *name = gtk_widget_get_name (GTK_WIDGET (widget));

	  /* Remember that we've already seen this group. */
	  find_data->groups_found = g_list_prepend (find_data->groups_found,
						    group);

	  /* Add the widget's name to the list. */
	  find_data->group_names = g_list_insert_sorted (find_data->group_names, (char*)name, (GCompareFunc) g_utf8_collate);
	}
    }

  if (GTK_IS_CONTAINER (widget))
    gb_widget_children_foreach (widget, (GtkCallback) get_radio_button_groups,
				find_data);
}


/* This tries to ensure that one and only one radio button in the given
   group is selected. It's used because GtkRadioButton doesn't handle this
   when groups are changed. */
void
gb_radio_button_update_radio_group (GSList * group)
{
  GtkRadioButton *button;
  GSList *slist;
  gboolean found_selected = FALSE;

  if (group == NULL)
    return;

  for (slist = group; slist; slist = slist->next)
    {
      button = GTK_RADIO_BUTTON (slist->data);
      if (GTK_TOGGLE_BUTTON (button)->active)
	{
	  if (found_selected)
	    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
	  else
	    found_selected = TRUE;
	}
    }
  /* If none are currently selected, select the first. */
  if (!found_selected)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (group->data), TRUE);
}


/* This recursively steps though a complete component's hierarchy to find
   a radio button with a particular group name set. When found the radio
   button's group list is returned in find_group_data->group. */
static void
find_group_widget (GtkWidget *widget, GladeFindGroupWidgetData *find_data)
{
  if (find_data->found_widget)
    return;

  if (GTK_IS_RADIO_BUTTON (widget) && GB_IS_GB_WIDGET (widget))
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
gb_radio_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean state, indicator, inconsistent;
  gchar *group_name;

  gb_button_set_standard_properties (widget, data, StockButton, Label, Icon,
				     Relief, FocusOnClick);

  state = gb_widget_input_bool (data, State);
  if (data->apply)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), state);
      if (state)
	data->widget_data->flags |= GLADE_ACTIVE;
      else
	data->widget_data->flags &= ~GLADE_ACTIVE;
    }

  inconsistent = gb_widget_input_bool (data, Inconsistent);
  if (data->apply)
    gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (widget),
					inconsistent);

  indicator = gb_widget_input_bool (data, Indicator);
  if (data->apply)
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (widget), indicator);

  /* Find any widgets in given group and set this widgets group.
     If group is NULL try to find radiobuttons with same parent and use
     their group. If these don't succeed, set group to NULL. */
  group_name = gb_widget_input_combo (data, Group);
  if (data->apply)
    {
      GSList *old_group, *new_group = NULL;

      old_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));

      if (group_name && (group_name[0] == '\0'
			 || !strcmp (group_name, _("New Group"))))
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
	    new_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (find_data.found_widget));
	  else if (data->action == GB_LOADING)
	    g_warning ("Invalid radio group: %s\n   (Note that forward references are not allowed in Glade files)", group_name);
	}

#if 0
      g_print ("New Group: %p Old Group: %p\n", new_group, old_group);
#endif

      if (new_group != old_group)
	{
#if 0
	  g_print ("##### setting radio group: %s\n",
		   group_name ? group_name : "NULL");
#endif
	  gtk_radio_button_set_group (GTK_RADIO_BUTTON (widget), new_group);
	}
    }
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_radio_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GladeFindGroupData find_data;
  GtkWidget *group_widget;
  gchar buffer[256], *group_name;

  find_data.group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (widget));
  find_data.found_widget = NULL;
  gb_button_find_radio_group (glade_util_get_toplevel (widget), &find_data);

  group_widget = find_data.found_widget;
  if (!group_widget)
    {
      g_warning ("Radiobutton has no group");
      group_widget = widget;
    }

  gb_button_write_standard_source (widget, data, Label);

  /* Make sure the temporary group list variable is declared. */
  group_name = (char*) gtk_widget_get_name (group_widget);
  group_name = source_create_valid_identifier (group_name);
  sprintf (buffer, "  GSList *%s_group = NULL;\n", group_name);
  source_ensure_decl (data, buffer);

  source_add (data,
	      "  gtk_radio_button_set_group (GTK_RADIO_BUTTON (%s), %s_group);\n",
	      data->wname, group_name);
  source_add (data,
	      "  %s_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (%s));\n",
	      group_name, data->wname);
  g_free (group_name);

  if (data->widget_data->flags & GLADE_ACTIVE)
    {
      source_add (data,
	  "  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (GTK_TOGGLE_BUTTON (widget)->inconsistent)
    {
      source_add (data,
      "  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (!GTK_TOGGLE_BUTTON (widget)->draw_indicator)
    {
      source_add (data,
	  "  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (%s), FALSE);\n",
		  data->wname);
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_radio_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_radio_button_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = radiobutton_xpm;
  gbwidget.tooltip = _("Radio Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_radio_button_new;
  gbwidget.gb_widget_create_properties = gb_radio_button_create_properties;
  gbwidget.gb_widget_get_properties = gb_radio_button_get_properties;
  gbwidget.gb_widget_set_properties = gb_radio_button_set_properties;
  gbwidget.gb_widget_create_popup_menu = gb_button_create_popup_menu;
  gbwidget.gb_widget_write_source = gb_radio_button_write_source;
  gbwidget.gb_widget_destroy = gb_button_destroy;

  return &gbwidget;
}
