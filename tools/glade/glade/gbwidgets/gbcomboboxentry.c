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
#include "../graphics/comboboxentry.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/* This isn't a real property, but since we can't set up a proper tree model
   we just support simple text like we did for GtkCombo. */
static gchar *Items = "GtkComboBoxEntry::items";

static gchar *AddTearoffs = "GtkComboBoxEntry|GtkComboBox::add_tearoffs";
static gchar *HasFrame = "GtkComboBoxEntry|GtkComboBox::has_frame";
static gchar *FocusOnClick = "GtkComboBoxEntry|GtkComboBox::focus_on_click";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkComboBoxEntry, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_combo_box_entry_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_combo_box_entry_new_text ();

  /* Force the combobox to create the child widgets, so that we can connect
     to all the "event" signals so the user can select the widget. */
  gtk_widget_ensure_style (new_widget);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_combo_box_entry_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_text (Items, _("Items:"),
		     _("The items in the combo list, one per line"), 5);

  property_add_bool (AddTearoffs, _("Add Tearoffs:"),
		     _("Whether dropdowns should have a tearoff menu item"));
  property_add_bool (HasFrame, _("Has Frame:"),
		     _("Whether the combo box draws a frame around the child"));
  property_add_bool (FocusOnClick, _("Focus On Click:"),
		     _("Whether the combo box grabs focus when it is clicked"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_combo_box_entry_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gchar *items;
  gboolean add_tearoffs, has_frame, focus_on_click;

  items = gtk_object_get_data (GTK_OBJECT (widget), Items);
  gb_widget_output_translatable_text_in_lines (data, Items, items);

  g_object_get (G_OBJECT (widget),
		"add_tearoffs", &add_tearoffs,
		"has_frame", &has_frame,
		"focus_on_click", &focus_on_click,
		NULL);

  gb_widget_output_bool (data, AddTearoffs, add_tearoffs);
  gb_widget_output_bool (data, HasFrame, has_frame);
  gb_widget_output_bool (data, FocusOnClick, focus_on_click);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_combo_box_entry_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gchar *items;
  gboolean add_tearoffs, has_frame, focus_on_click;

  items = gb_widget_input_text (data, Items);
  if (data->apply)
    {
      GtkTreeModel *model;
      gchar *pos = items;
      gchar *items_end = &items[strlen (items)];

      /* Save a copy so it is easy to get out later. */
      gtk_object_set_data_full (GTK_OBJECT (widget), Items,
				g_strdup (items), g_free);

      /* Clear the list. */
      model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
      gtk_list_store_clear (GTK_LIST_STORE (model));

      /* Now add the items one at a time. */
      while (pos < items_end)
	{
	  gchar *item_end = strchr (pos, '\n');
	  if (item_end == NULL)
	    item_end = items_end;
	  *item_end = '\0';

	  gtk_combo_box_append_text (GTK_COMBO_BOX (widget), pos);

	  if (item_end != items_end)
	    *item_end = '\n';

	  pos = item_end + 1;
	}
    }
  if (data->action == GB_APPLYING)
    g_free (items);

  add_tearoffs = gb_widget_input_bool (data, AddTearoffs);
  if (data->apply)
    gtk_combo_box_set_add_tearoffs (GTK_COMBO_BOX (widget), add_tearoffs);

  has_frame = gb_widget_input_bool (data, HasFrame);
  if (data->apply)
    g_object_set (widget, "has_frame", has_frame, NULL);

  focus_on_click = gb_widget_input_bool (data, FocusOnClick);
  if (data->apply)
    gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (widget), focus_on_click);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkComboBoxEntry, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_combo_box_entry_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_combo_box_entry_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *items, *pos, *items_end;
  gboolean translatable, context;
  gchar *comments;
  gboolean add_tearoffs, has_frame, focus_on_click;

  items = pos = gtk_object_get_data (GTK_OBJECT (widget), Items);

  glade_util_get_translation_properties (widget, Items, &translatable,
					 &comments, &context);

  if (data->create_widget)
    {
      /* If any items have been entered, we create a simple text combo,
	 otherwise we create a bare combo without a model, so the user can
	 setup the model in their code. */
      /* NOTE: Creating it without a model causes problems so we always create
	 a model now. Users can still set the model to something else. */
#if 0
      if (items && items[0])
	{
	  source_add (data, "  %s = gtk_combo_box_entry_new_text ();\n",
		      data->wname);
	}
      else
	{
	  source_add (data, "  %s = gtk_combo_box_entry_new ();\n",
		      data->wname);
	}
#endif

      source_add (data, "  %s = gtk_combo_box_entry_new_text ();\n",
		  data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  if (items && items[0])
    {
      items_end = &items[strlen (items)];

      while (pos < items_end)
	{
	  gchar *item_end = strchr (pos, '\n');
	  if (item_end == NULL)
	    item_end = items_end;
	  *item_end = '\0';

	  source_add (data, "  gtk_combo_box_append_text (GTK_COMBO_BOX_TEXT (%s), %s);\n",
		      data->wname,
		      source_make_string (pos,
					  data->use_gettext && translatable));

	  if (item_end != items_end)
	    *item_end = '\n';

	  pos = item_end + 1;
	}
    }

  g_object_get (G_OBJECT (widget),
		"add_tearoffs", &add_tearoffs,
		"has_frame", &has_frame,
		"focus_on_click", &focus_on_click,
		NULL);

  if (add_tearoffs)
    {
      source_add (data,
	"  gtk_combo_box_set_add_tearoffs (GTK_COMBO_BOX (%s), TRUE);\n",
		  data->wname);
    }

  if (!has_frame)
    {
      source_add (data,
		  "  g_object_set (%s, \"has_frame\", FALSE, NULL);\n",
		  data->wname);
    }

  if (!focus_on_click)
    {
      source_add (data,
	"  gtk_combo_box_set_focus_on_click (GTK_COMBO_BOX (%s), FALSE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_combo_box_entry_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_combo_box_entry_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = comboboxentry_xpm;
  gbwidget.tooltip = _("Combo Box Entry");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_combo_box_entry_new;
  gbwidget.gb_widget_create_properties	= gb_combo_box_entry_create_properties;
  gbwidget.gb_widget_get_properties	= gb_combo_box_entry_get_properties;
  gbwidget.gb_widget_set_properties	= gb_combo_box_entry_set_properties;
  gbwidget.gb_widget_write_source	= gb_combo_box_entry_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_combo_box_entry_create_popup_menu;
*/

  return &gbwidget;
}

