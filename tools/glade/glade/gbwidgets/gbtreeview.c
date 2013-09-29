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

#include <gtk/gtk.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/ctree.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *HeadersVisible	= "GtkTreeView::headers_visible";
static gchar *RulesHint		= "GtkTreeView::rules_hint";
static gchar *Reorderable	= "GtkTreeView::reorderable";
static gchar *EnableSearch	= "GtkTreeView::enable_search";

static gchar *FixedHeightMode	= "GtkTreeView::fixed_height_mode";
static gchar *HoverSelection	= "GtkTreeView::hover_selection";
static gchar *HoverExpand	= "GtkTreeView::hover_expand";


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkTreeView, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_tree_view_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;
  GtkListStore *store;
  GtkTreeIter iter;
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  char buf[256];
  gint i;

  store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  new_widget = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

  /* We set the columns to a fixed size, since the "fixed_height_mode"
     property requires that. */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Column 1"), rend,
						  "text", 0,
						  NULL);
  gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (col, 100);
  gtk_tree_view_append_column (GTK_TREE_VIEW (new_widget), col);

  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Column 2"), rend,
						  "text", 1,
						  NULL);
  gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (col, 100);
  gtk_tree_view_append_column (GTK_TREE_VIEW (new_widget), col);

  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Column 3"), rend,
						  "text", 2,
						  NULL);
  gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (col, 100);
  gtk_tree_view_append_column (GTK_TREE_VIEW (new_widget), col);

  for (i = 1; i <= 8; i++)
    {
      gtk_list_store_append (store, &iter);
      sprintf (buf, _("Row %i"), i);
      gtk_list_store_set (store, &iter, 0, buf, 1, "...", 2, "...", -1);
    }

  g_object_unref (G_OBJECT (store));

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_tree_view_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (HeadersVisible, _("Headers Visible:"),
		     _("If the column header buttons are shown"));
  property_add_bool (RulesHint, _("Rules Hint:"),
		     _("If a hint is set so the theme engine should draw rows in alternating colors"));
  property_add_bool (Reorderable, _("Reorderable:"),
		     _("If the view is reorderable"));
  property_add_bool (EnableSearch, _("Enable Search:"),
		     _("If the user can search through columns interactively"));

  property_add_bool (FixedHeightMode, _("Fixed Height Mode:"),
		     _("Sets all rows to the same height to improve performance"));
  property_add_bool (HoverSelection, _("Hover Selection:"),
		     _("Whether the selection should follow the pointer"));
  property_add_bool (HoverExpand, _("Hover Expand:"),
		     _("Whether rows should be expanded or collapsed when the pointer moves over them"));
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_tree_view_get_properties (GtkWidget *widget, GbWidgetGetArgData * data)
{
  gboolean headers_visible, rules_hint, reorderable, enable_search;
  gboolean fixed_height_mode, hover_selection, hover_expand;

  g_object_get (G_OBJECT (widget),
		"headers_visible", &headers_visible,
		"rules_hint", &rules_hint,
		"reorderable", &reorderable,
		"enable_search", &enable_search,
		"fixed_height_mode", &fixed_height_mode,
		"hover_selection", &hover_selection,
		"hover_expand", &hover_expand,
		NULL);

  gb_widget_output_bool (data, HeadersVisible, headers_visible);
  gb_widget_output_bool (data, RulesHint, rules_hint);
  gb_widget_output_bool (data, Reorderable, reorderable);
  gb_widget_output_bool (data, EnableSearch, enable_search);
  gb_widget_output_bool (data, FixedHeightMode, fixed_height_mode);
  gb_widget_output_bool (data, HoverSelection, hover_selection);
  gb_widget_output_bool (data, HoverExpand, hover_expand);
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_tree_view_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean headers_visible, rules_hint, reorderable, enable_search;
  gboolean fixed_height_mode, hover_selection, hover_expand;

  headers_visible = gb_widget_input_bool (data, HeadersVisible);
  if (data->apply)
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (widget), headers_visible);

  rules_hint = gb_widget_input_bool (data, RulesHint);
  if (data->apply)
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (widget), rules_hint);

  reorderable = gb_widget_input_bool (data, Reorderable);
  if (data->apply)
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (widget), reorderable);

  enable_search = gb_widget_input_bool (data, EnableSearch);
  if (data->apply)
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (widget), enable_search);

  fixed_height_mode = gb_widget_input_bool (data, FixedHeightMode);
  if (data->apply)
    gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (widget), fixed_height_mode);

  hover_selection = gb_widget_input_bool (data, HoverSelection);
  if (data->apply)
    gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (widget), hover_selection);

  hover_expand = gb_widget_input_bool (data, HoverExpand);
  if (data->apply)
    gtk_tree_view_set_hover_expand (GTK_TREE_VIEW (widget), hover_expand);
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkTreeView, with signals pointing to
 * other functions in this file.
 */
/*
static void
gb_tree_view_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{

}
*/



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_tree_view_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gboolean headers_visible, rules_hint, reorderable, enable_search;
  gboolean fixed_height_mode, hover_selection, hover_expand;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_tree_view_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  g_object_get (G_OBJECT (widget),
		"headers_visible", &headers_visible,
		"rules_hint", &rules_hint,
		"reorderable", &reorderable,
		"enable_search", &enable_search,
		"fixed_height_mode", &fixed_height_mode,
		"hover_selection", &hover_selection,
		"hover_expand", &hover_expand,
		NULL);

  if (!headers_visible)
    {
      source_add (data,
	"  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (%s), FALSE);\n",
		  data->wname);
    }

  if (rules_hint)
    {
      source_add (data,
	"  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (%s), TRUE);\n",
		  data->wname);
    }

  if (reorderable)
    {
      source_add (data,
	"  gtk_tree_view_set_reorderable (GTK_TREE_VIEW (%s), TRUE);\n",
		  data->wname);
    }

  if (!enable_search)
    {
      source_add (data,
	"  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (%s), FALSE);\n",
		  data->wname);
    }

  if (fixed_height_mode)
    {
      source_add (data,
	"  gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (%s), TRUE);\n",
		  data->wname);
    }

  if (hover_selection)
    {
      source_add (data,
	"  gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (%s), TRUE);\n",
		  data->wname);
    }

  if (hover_expand)
    {
      source_add (data,
	"  gtk_tree_view_set_hover_expand (GTK_TREE_VIEW (%s), TRUE);\n",
		  data->wname);
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_tree_view_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_tree_view_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = ctree_xpm;
  gbwidget.tooltip = _("List or Tree View");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_tree_view_new;
  gbwidget.gb_widget_create_properties	= gb_tree_view_create_properties;
  gbwidget.gb_widget_get_properties	= gb_tree_view_get_properties;
  gbwidget.gb_widget_set_properties	= gb_tree_view_set_properties;
  gbwidget.gb_widget_write_source	= gb_tree_view_write_source;
/*
  gbwidget.gb_widget_create_popup_menu	= gb_tree_view_create_popup_menu;
*/

  return &gbwidget;
}

