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

#include "gladeconfig.h"

#include <string.h>

#include <gtk/gtkcellrendererpixbuf.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkwindow.h>


#include "gbwidget.h"
#include "editor.h"
#include "tree.h"
#include "utils.h"
#include "glade_project_window.h"

/* This key is used to store a pointer from each widget to its corresponding 
   node in the widget tree. */
#define GLADE_TREE_NODE_KEY "GLADE_TREE_NODE_KEY"

enum {
  COLUMN_ICON,
  COLUMN_TEXT,
  COLUMN_OBJECT
};

GtkWidget *win_tree = NULL;
static GtkWidget *widget_tree = NULL;
static GtkTreeStore *tree_store = NULL;
static GtkTreeSelection *tree_selection = NULL;


static void tree_on_selection_changed (GtkTreeSelection *selection,
				       gpointer data);
static gint tree_on_button_press (GtkWidget * tree,
				  GdkEventButton * event,
				  gpointer widget);



/* This creates the widget tree window, with just the root tree. */
void
tree_init ()
{
  GtkWidget *scrolled_win;
  GtkTreeViewColumn *col;
  GtkCellRenderer *icon_rend, *text_rend;

  win_tree = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_move (GTK_WINDOW (win_tree), 530, 0);
  gtk_widget_set_name (win_tree, "GladeWidgetTree");
  gtk_signal_connect (GTK_OBJECT (win_tree), "delete_event",
		      GTK_SIGNAL_FUNC (tree_hide), NULL);
  gtk_signal_connect_after (GTK_OBJECT (win_tree), "hide",
                      GTK_SIGNAL_FUNC (glade_project_window_uncheck_widget_tree_menu_item),
                      NULL);
  gtk_window_set_title (GTK_WINDOW (win_tree), _("Widget Tree"));
  gtk_container_set_border_width (GTK_CONTAINER (win_tree), 0);
  gtk_window_set_wmclass (GTK_WINDOW (win_tree), "widget_tree", "Glade");
  gtk_window_add_accel_group (GTK_WINDOW (win_tree),
			      glade_get_global_accel_group ());

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (win_tree), scrolled_win);
  gtk_widget_show (scrolled_win);

  tree_store = gtk_tree_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING,
				   G_TYPE_POINTER);
  widget_tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (widget_tree), FALSE);
  gtk_container_add (GTK_CONTAINER (scrolled_win), widget_tree);
  gtk_widget_set_usize (widget_tree, 250, 320);
  gtk_widget_show (widget_tree);

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget_tree));
  gtk_tree_selection_set_mode (GTK_TREE_SELECTION (tree_selection),
			       GTK_SELECTION_SINGLE);

  col = gtk_tree_view_column_new ();

  icon_rend = gtk_cell_renderer_pixbuf_new ();
  text_rend = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, icon_rend, FALSE);
  gtk_tree_view_column_pack_start (col, text_rend, TRUE);
  gtk_tree_view_column_set_attributes (col, icon_rend,
				       "pixbuf", COLUMN_ICON,
				       NULL);
  gtk_tree_view_column_set_attributes (col, text_rend,
				       "text", COLUMN_TEXT,
				       NULL);
  gtk_tree_view_column_set_resizable (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget_tree), col);

  gtk_signal_connect (GTK_OBJECT (widget_tree), "button_press_event",
		      GTK_SIGNAL_FUNC (tree_on_button_press), NULL);
  g_signal_connect (G_OBJECT (tree_selection), "changed",
		    GTK_SIGNAL_FUNC (tree_on_selection_changed), NULL);
}

/* This shows the widget tree window. */
void
tree_show (GtkWidget * widget, gpointer data)
{
  gtk_widget_show (win_tree);
  gdk_window_show (GTK_WIDGET (win_tree)->window);
  gdk_window_raise (GTK_WIDGET (win_tree)->window);
}

/* This hides the widget tree window. */
gint
tree_hide (GtkWidget * widget, gpointer data)
{
  gtk_widget_hide (win_tree);
  return TRUE;
}


/* This adds a widget to the tree (if it is a widget that we are interested in,
 * i.e. a GbWidget). The widget must have already been added to the interface,
 * so we can determine its parent. It also recursively adds any children. */
void
tree_add_widget (GtkWidget * widget)
{
  /* We only add it to the tree if it is a GbWidget and it hasn't already been
     added. */
  if (GB_IS_GB_WIDGET (widget))
    {
      if (!gtk_object_get_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY))
	{
	  GbWidget *gbwidget;
	  GtkWidget *parent;
	  GNode *parent_node = NULL;
	  GtkTreeIter iter, tmp_iter = { 0 }, *parent_iter = NULL;

	  gbwidget = gb_widget_lookup (widget);
	  g_return_if_fail (gbwidget != NULL);

	  parent = glade_util_get_parent (widget);
	  if (parent)
	    parent_node = gtk_object_get_data (GTK_OBJECT (parent),
					       GLADE_TREE_NODE_KEY);
	  if (parent_node)
	    {
	      /* I'm not sure if this is a bad hack. */
	      tmp_iter.stamp = tree_store->stamp;
	      tmp_iter.user_data = parent_node;
	      parent_iter = &tmp_iter;
	    }

	  /* Create the pixbuf for the icon, if necessary. */
	  if (gbwidget->pixbuf == NULL && gbwidget->pixmap_struct)
	    {
	      gbwidget->pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) gbwidget->pixmap_struct);
	    }

	  gtk_tree_store_append (tree_store, &iter, parent_iter);
	  gtk_tree_store_set (tree_store, &iter,
			      COLUMN_ICON, gbwidget->pixbuf,
			      COLUMN_TEXT, gtk_widget_get_name (widget),
			      COLUMN_OBJECT, widget,
			      -1);

	  /* Save a pointer to the GNode inside the widget. */
	  gtk_object_set_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY,
			       iter.user_data);
	}
      else
	{
	  /* Update the name if necessary. This is needed because we may have
	     added this widget before (if it was created by its parent
	     automatically), but we have only just loaded its name. */
	  tree_rename_widget (widget, gtk_widget_get_name (widget));
	}
    }

  /* Now add any children. */
  gb_widget_children_foreach (widget, (GtkCallback) tree_add_widget, NULL);
}


/* Recursively removes any pointers to tree GNodes in the widgets. */
void
tree_remove_widget_cb (GtkWidget * widget)
{
#if 0
  g_print ("Removing node key from widget: %s\n", gtk_widget_get_name (widget));
#endif
  gtk_object_remove_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY);
  gb_widget_children_foreach (widget, (GtkCallback) tree_remove_widget_cb,
			      NULL);
}


void
tree_remove_widget (GtkWidget * widget)
{
  GNode *node;
  GtkTreeIter iter = { 0 };

#if 0
  g_print ("In tree_remove_widget: %s\n", gtk_widget_get_name (widget));
#endif

  node = gtk_object_get_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY);
  if (node == NULL)
    return;

  /* I'm not sure if this is a bad hack. */
  iter.stamp = tree_store->stamp;
  iter.user_data = node;

#if 0
  g_print ("Removing widget: %s\n", gtk_widget_get_name (widget));
#endif

  gtk_object_remove_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY);

  /* Remove the GNode's stored in any child widgets, as these GNodes are also
     about to be removed. */
  gb_widget_children_foreach (widget, (GtkCallback) tree_remove_widget_cb,
			      NULL);

  gtk_tree_store_remove (tree_store, &iter);
}


void
tree_clear (void)
{
  gtk_tree_store_clear (tree_store);
}


void
tree_freeze (void)
{
#if 0
  /* I don't think GtkTreeView has a corresponding function. */
  gtk_clist_freeze (GTK_CLIST (widget_tree));
#endif
}


void
tree_thaw (void)
{
#if 0
  /* I don't think GtkTreeView has a corresponding function. */
  gtk_clist_thaw (GTK_CLIST (widget_tree));
#endif
}


void
tree_rename_widget (GtkWidget * widget, const gchar * name)
{
  GNode *node;
  GtkTreeIter iter = { 0 };
  gchar *old_name;

  node = gtk_object_get_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY);
  if (node == NULL)
    return;

  /* I'm not sure if this is a bad hack. */
  iter.stamp = tree_store->stamp;
  iter.user_data = node;

  gtk_tree_model_get (GTK_TREE_MODEL (tree_store), &iter,
		      COLUMN_TEXT, &old_name, -1);

  /* Only update it if the name has really changed. */
  if (strcmp (old_name, name))
    {
#if 0
      g_print ("Changing name from: %s to: %s\n", old_name, name);
#endif
      gtk_tree_store_set (tree_store, &iter, COLUMN_TEXT, name, -1);
    }
}


/* This is used when inserting an alignment or event box above a widget. */
void
tree_insert_widget_parent (GtkWidget * parent, GtkWidget * widget)
{
  /* Remove the widget if it is currently in the tree. */
  tree_remove_widget (widget);

  /* Now add the parent. Since widgets are added recursively, the widget
     will get added back as well. */
  tree_add_widget (parent);
}


/* Selects a node and makes sure it is visible on the tree */
static void
tree_select_iter (GtkTreeIter *iter)
{
  GtkTreePath *path;
  GList *ancestors = NULL, *elem;

  /* Make sure all ancestors are expanded. This makes the GUI easier to use,
     but is also necessary to be able to select the widget in the GtkTreeView.
     We have to expand the rows from the top down, so first we step up the
     tree pushing them onto a list. */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_store), iter);
  for (;;)
    {
      if (!gtk_tree_path_up (path))
	break;

      ancestors = g_list_prepend (ancestors, path);
      path = gtk_tree_path_copy (path);
    }
  gtk_tree_path_free (path);

  for (elem = ancestors; elem; elem = elem->next)
    {
      GtkTreePath *tmp_path = elem->data;

      if (!gtk_tree_view_row_expanded (GTK_TREE_VIEW (widget_tree), tmp_path))
	gtk_tree_view_expand_row (GTK_TREE_VIEW (widget_tree), tmp_path,
				  FALSE);
      gtk_tree_path_free (tmp_path);
    }
  g_list_free (ancestors);

  /* Select the row. */
  gtk_tree_selection_select_iter (tree_selection, iter);

  /* Make sure the row is visible. */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_store), iter);
  gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget_tree), path, NULL,
				TRUE, 0, 0);
  gtk_tree_path_free (path);
}


static void
tree_get_selection_cb (GtkTreeModel      *model,
		       GtkTreePath       *path,
		       GtkTreeIter       *iter,
		       GList	       **selection)
{
  GObject *object;

  gtk_tree_model_get (model, iter, COLUMN_OBJECT, &object, -1);
  if (object)
    *selection = g_list_prepend (*selection, object);
}


/* Returns a list of selected widgets. g_list_free() it. */
static GList*
tree_get_selection (void)
{
  GList *selection = NULL;

  /* Create a GList of all the widgets selected in the tree. */
  gtk_tree_selection_selected_foreach (tree_selection,
				       (GtkTreeSelectionForeachFunc) tree_get_selection_cb,
				       &selection);
  return selection;
}


void
tree_select_widget (GtkWidget * widget, gboolean select)
{
  GNode *node;
  GtkTreeIter iter = { 0 };
  GList *selection;
  gboolean selected = FALSE;

  if (!GB_IS_GB_WIDGET (widget))
    return;

  node = gtk_object_get_data (GTK_OBJECT (widget), GLADE_TREE_NODE_KEY);
  if (node == NULL)
    return;

  /* I'm not sure if this is a bad hack. */
  iter.stamp = tree_store->stamp;
  iter.user_data = node;

  /* Check if the item is currently selected. */
  selection = tree_get_selection ();
  if (g_list_find (selection, widget))
    selected = TRUE;
  g_list_free (selection);

  if (select && !selected)
    tree_select_iter (&iter);
  else if (!select && selected)
    gtk_tree_selection_unselect_iter (tree_selection, &iter);
}


/* We set the widget selection in the GUI to match the selection in the tree.
 */
static void
tree_on_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  /* Note that we may get callbacks as widgets are selected or deselected in
     the GUI, but we will check if the tree needs updating and it shouldn't
     so we won't get into infinite loops. */
  editor_set_selection (tree_get_selection ());
}


/* This is called when a button is pressed in the tree. If it was the
   right mouse button we show the popup menu for the widget. */
static gint
tree_on_button_press (GtkWidget      *tree,
		      GdkEventButton *event,
		      gpointer        data)
{
  GtkTreePath *path;
  GtkTreeIter iter;

  if (event->button != 3
      || event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (tree)))
    return FALSE;

  if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree),
				      event->x, event->y,
				      &path, NULL, NULL, NULL))
    return FALSE;

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (tree_store), &iter, path))
    {
      GtkWidget *widget;

      gtk_tree_model_get (GTK_TREE_MODEL (tree_store), &iter,
			  COLUMN_OBJECT, &widget,
			  -1);

      if (widget)
	gb_widget_show_popup_menu (GTK_WIDGET (widget), event);

      gtk_tree_path_free (path);
    }

  return TRUE;
}


