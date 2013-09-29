/*  Gtk+ User Interface Builder
 *  Copyright (C) 1999-2002  Damon Chaplin
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

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/radiotoolbutton.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *StockButton = "GtkRadioToolButton|GtkToolButton::stock_id";
static gchar *Label = "GtkRadioToolButton|GtkToolButton::label";
static gchar *Icon = "GtkRadioToolButton|GtkToolButton::icon";
static gchar *VisibleHorz = "GtkRadioToolButton|GtkToolItem::visible_horizontal";
static gchar *VisibleVert = "GtkRadioToolButton|GtkToolItem::visible_vertical";
static gchar *IsImportant = "GtkRadioToolButton|GtkToolItem::is_important";

static gchar *Group = "GtkRadioToolButton::group";
static gchar *Active = "GtkRadioToolButton|GtkToggleToolButton::active";


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

  if (GTK_IS_RADIO_TOOL_BUTTON (widget))
    *group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (widget));
}


/*
 * Creates a new GtkWidget of class GtkRadioToolButton, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_radio_tool_button_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget, *image;
  GbWidget *pixmap_gbwidget;
  GSList *group_list = NULL;

  /* When creating a radiotoolbutton we try to place it in the same group
     as other radiotoolbuttons in the same toolbar. */
  if (data->action == GB_CREATING)
    {
      GtkWidget *parent = data->parent;
      while (parent && !GTK_IS_TOOLBAR (parent))
	parent = parent->parent;
      if (parent)
	gb_widget_children_foreach (parent,
				    (GtkCallback) find_parents_group,
				    &group_list);
    }

  /* Place the pixmap icon in the button initially (even when loading). */
  pixmap_gbwidget = gb_widget_lookup_class ("GtkImage");
  if (pixmap_gbwidget)
    {
      image = gtk_image_new_from_pixmap (pixmap_gbwidget->gdkpixmap,
					 pixmap_gbwidget->mask);
    }
  else
    {
      image = gtk_image_new ();
      g_warning ("Couldn't find GtkPixmap data");
    }
  gtk_widget_show (image);

  new_widget = (GtkWidget*) gtk_radio_tool_button_new (group_list);

  gtk_tool_button_set_label (GTK_TOOL_BUTTON (new_widget), "");
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (new_widget), image);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_radio_tool_button_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  GtkWidget *combo;

  property_add_stock_item (StockButton, _("Stock Button:"),
			   _("The stock button to use"),
			   GTK_ICON_SIZE_LARGE_TOOLBAR);
  property_add_text (Label, _("Label:"), _("The text to display"), 2);
  property_add_icon (Icon, _("Icon:"),
		     _("The icon to display"),
		     GTK_ICON_SIZE_LARGE_TOOLBAR);

  property_add_combo (Group, _("Group:"),
		      _("The radio tool button group (the default is all radio tool buttons in the toolbar)"),
		      NULL);
  combo = property_get_value_widget (Group);
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (combo)->entry), FALSE);

  property_add_bool (Active, _("Initially On:"),
		     _("If the radio button is initially on"));

  property_add_bool (VisibleHorz, _("Show Horizontal:"),
		     _("If the item is visible when the toolbar is horizontal"));
  property_add_bool (VisibleVert, _("Show Vertical:"),
		     _("If the item is visible when the toolbar is vertical"));
  property_add_bool (IsImportant, _("Is Important:"),
		     _("If the item's text should be shown when the toolbar's mode is GTK_TOOLBAR_BOTH_HORIZ"));
}


/* Note that this must walk the widget tree in exactly the same way that we
   save the widgets, so we know which widget in the group will be the first
   output. */
static void
get_radio_button_groups (GtkWidget * widget, GladeFindGroupsData *find_data)
{
  if (GTK_IS_RADIO_TOOL_BUTTON (widget) && GB_IS_GB_WIDGET (widget))
    {
      GSList *group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (widget));

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
}


static void
find_radio_group (GtkWidget *widget, GladeFindGroupData *find_data)
{
  if (find_data->found_widget)
    return;

  if (GTK_IS_RADIO_TOOL_BUTTON (widget) && GB_IS_GB_WIDGET (widget))
    {
      if (gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (widget)) == find_data->group)
	{
	  find_data->found_widget = widget;
	}
    }
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_radio_tool_button_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  GladeFindGroupData find_data;

  gb_tool_button_get_standard_properties (widget, data,
					  StockButton, Label, Icon,
					  VisibleHorz, VisibleVert,
					  IsImportant);

  gb_widget_output_bool (data, Active,
			 data->widget_data->flags & GLADE_ACTIVE);

  /* If we're showing we need to display the list of groups to choose from.
     We walk the tree of widgets in this component, and if a widget is
     a radio button, we see if it has a group and if it is already in the
     list and if not we add it. */
  if (data->action == GB_SHOWING)
    {
      GladeFindGroupsData find_groups_data;

      find_groups_data.groups_found = NULL;
      find_groups_data.group_names = NULL;
      gb_widget_children_foreach (widget->parent,
				  (GtkCallback) get_radio_button_groups,
				  &find_groups_data);

      find_groups_data.group_names = g_list_prepend (find_groups_data.group_names,
						     _("New Group"));
      property_set_combo_strings (Group, find_groups_data.group_names);

      g_list_free (find_groups_data.groups_found);
      g_list_free (find_groups_data.group_names);
    }

  find_data.group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (widget));
  find_data.found_widget = NULL;
  gb_widget_children_foreach (widget->parent,
			      (GtkCallback) find_radio_group,
			      &find_data);

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
      g_warning ("Radiotoolbutton has no group");
      gb_widget_output_combo (data, Group, "");
    }
}



static void
find_group_widget (GtkWidget *widget, GladeFindGroupWidgetData *find_data)
{
  if (find_data->found_widget)
    return;

  if (GTK_IS_RADIO_TOOL_BUTTON (widget) && GB_IS_GB_WIDGET (widget))
    {
      if (!strcmp (gtk_widget_get_name (widget), find_data->name))
	{
#if 0
	  g_print ("Found widget: %s\n", find_data->name);
#endif
	  find_data->found_widget = widget;
	}
    }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_radio_tool_button_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean active;
  gchar *group_name;

  gb_tool_button_set_standard_properties (widget, data,
					  StockButton, Label, Icon,
					  VisibleHorz, VisibleVert,
					  IsImportant);

  active = gb_widget_input_bool (data, Active);
  if (data->apply)
    {
      gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (widget),
					 active);
      if (active)
	data->widget_data->flags |= GLADE_ACTIVE;
      else
	data->widget_data->flags &= ~GLADE_ACTIVE;
    }

  /* Find any widgets in given group and set this widgets group.
     If group is NULL try to find radiobuttons with same parent and use
     their group. If these don't succeed, set group to NULL. */
  group_name = gb_widget_input_combo (data, Group);
  if (data->apply)
    {
      GSList *old_group, *new_group = NULL;

      old_group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (widget));

      if (group_name && (group_name[0] == '\0'
			 || !strcmp (group_name, _("New Group"))))
	group_name = NULL;

      if (group_name)
	{
	  GladeFindGroupWidgetData find_data;

	  find_data.name = group_name;
	  find_data.found_widget = NULL;
	  gb_widget_children_foreach (widget->parent,
				      (GtkCallback) find_group_widget,
				      &find_data);

	  if (find_data.found_widget)
	    new_group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (find_data.found_widget));
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
	  gtk_radio_tool_button_set_group (GTK_RADIO_TOOL_BUTTON (widget),
					   new_group);
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkRadioToolButton, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_radio_tool_button_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_radio_tool_button_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  GladeFindGroupData find_data;
  GtkWidget *group_widget;
  gchar *stock_id, *label, *icon_name;
  gchar buffer[256], *group_name;
  gboolean translatable, context;
  gchar *comments;

  stock_id = gtk_object_get_data (GTK_OBJECT (widget),
				  GladeToolButtonStockIDKey);
  icon_name = gtk_object_get_data (GTK_OBJECT (widget),
				   GladeToolButtonIconKey);
  label = (gchar*) gtk_tool_button_get_label (GTK_TOOL_BUTTON (widget));

  glade_util_get_translation_properties (widget, Label, &translatable,
					 &comments, &context);

  find_data.group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (widget));
  find_data.found_widget = NULL;
  gb_widget_children_foreach (widget->parent,
			      (GtkCallback) find_radio_group,
			      &find_data);

  group_widget = find_data.found_widget;
  if (!group_widget)
    {
      g_warning ("Radiotoolbutton has no group");
      group_widget = widget;
    }
  group_name = (char*) gtk_widget_get_name (group_widget);
  group_name = source_create_valid_identifier (group_name);
  sprintf (buffer, "  GSList *%s_group = NULL;\n", group_name);
  source_ensure_decl (data, buffer);

  if (data->create_widget)
    {
      if (stock_id)
	{
	  /* Stock Button */
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_radio_tool_button_new_from_stock (NULL, %s);\n",
		      data->wname, source_make_string (stock_id, FALSE));
	}
      else if (icon_name)
	{
	  /* Icon and Label */
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_radio_tool_button_new (NULL);\n",
		      data->wname);

	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  gtk_tool_button_set_label (GTK_TOOL_BUTTON (%s), %s);\n",
		      data->wname,
		      label ? source_make_string_full (label, data->use_gettext && translatable, context) : "NULL");

	  source_ensure_decl (data, "  GtkWidget *tmp_image;\n");

	  if (glade_util_check_is_stock_id (icon_name))
	    {
	      source_add (data,
			  "  tmp_image = gtk_image_new_from_stock (\"%s\", tmp_toolbar_icon_size);\n",
			  icon_name);
	    }
	  else
	    {
	      source_create_pixmap (data, "tmp_image", icon_name);
	    }

	  source_add (data, "  gtk_widget_show (tmp_image);\n");

	  source_add (data,
		      "  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (%s), tmp_image);\n",
		      data->wname);
	}
      else
	{
	  /* Just a Label */
	  source_add (data,
		      "  %s = (GtkWidget*) gtk_radio_tool_button_new (NULL);\n",
		      data->wname);

	  source_add_translator_comments (data, translatable, comments);
	  source_add (data,
		      "  gtk_tool_button_set_label (GTK_TOOL_BUTTON (%s), %s);\n",
		      data->wname,
		      label ? source_make_string_full (label, data->use_gettext && translatable, context) : "NULL");
	}
    }

  gb_widget_write_standard_source (widget, data);

  source_add (data,
	      "  gtk_radio_tool_button_set_group (GTK_RADIO_TOOL_BUTTON (%s), %s_group);\n",
	      data->wname, group_name);
  source_add (data,
	      "  %s_group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (%s));\n",
	      group_name, data->wname);

  if (data->widget_data->flags & GLADE_ACTIVE)
    {
      source_add (data,
	  "  gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (%s), TRUE);\n",
		  data->wname);
    }

  if (gtk_object_get_data (GTK_OBJECT (widget), VisibleHorz) != NULL)
    {
      source_add (data,
		  "  gtk_tool_item_set_visible_horizontal (GTK_TOOL_ITEM (%s), FALSE);\n",
		  data->wname);
    }

  if (gtk_object_get_data (GTK_OBJECT (widget), VisibleVert) != NULL)
    {
      source_add (data,
		  "  gtk_tool_item_set_visible_vertical (GTK_TOOL_ITEM (%s), FALSE);\n",
		  data->wname);
    }

  if (gtk_tool_item_get_is_important (GTK_TOOL_ITEM (widget)))
    {
      source_add (data,
		  "  gtk_tool_item_set_is_important (GTK_TOOL_ITEM (%s), TRUE);\n",
		  data->wname);
    }

  g_free (group_name);
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_radio_tool_button_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_radio_tool_button_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = radiotoolbutton_xpm;
  gbwidget.tooltip = _("Toolbar Radio Button");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_radio_tool_button_new;
  gbwidget.gb_widget_create_properties	= gb_radio_tool_button_create_properties;
  gbwidget.gb_widget_get_properties	= gb_radio_tool_button_get_properties;
  gbwidget.gb_widget_set_properties	= gb_radio_tool_button_set_properties;
  gbwidget.gb_widget_write_source	= gb_radio_tool_button_write_source;
  gbwidget.gb_widget_destroy		= gb_tool_button_destroy;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_radio_tool_button_create_popup_menu;
*/

  return &gbwidget;
}

