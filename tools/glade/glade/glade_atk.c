/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-2002  Damon Chaplin
 *  Copyright (C) 2002 Sun Microsystems, Inc.
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

/*
 * This file contains most of the ATK-related code in Glade.
 * It handles the properties, actions and relations in the property editor.
 */

#include <config.h>
#include <gladeconfig.h>

#include <string.h>

#include <atk/atk.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkmain.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkvbox.h>

#include "property.h"
#include "save.h"
#include "source.h"
#include "utils.h"
#include "glade_atk.h"

#include "graphics/glade-atk.xpm"

const gchar *GladeATKName	  = "AtkObject::accessible_name";
const gchar *GladeATKDescription  = "AtkObject::accessible_description";
/* There is a problem with this as AtkObject says it is a string property
   but it really is an AtkObject. I've taken it out for now. */
#if 0
const gchar *GladeATKTableCaption = "AtkObject::accessible_table_caption";
#endif

/* These are keys we use to store data in the property value widget - the
   entry in the property editor that shows relations. */
const gchar *GladeATKPropertyName = "GladeATKPropertyName";
const gchar *GladeATKRelationType = "GladeATKRelationType";
const gchar *GladeATKRelationsDialog = "GladeATKRelationsDialog";

/* These are keys we use to store data in the relations dialog. */
const gchar *GladeATKValueWidget = "GladeATKValueWidget";
const gchar *GladeATKRelationsListStore = "GladeATKRelationsListStore";
const gchar *GladeATKRelationsWidgets = "GladeATKRelationsWidgets";


/* These are the GtkTables used in the property editor to contain the action
   properties and the relation properties. */
static GtkWidget *actions_table = NULL;
static GtkWidget *relations_table = NULL;

/* This is a list of the relation property value widgets, i.e. the GtkEntry
   widgets in the property editor for each relation. */
static GList *relations_properties = NULL;

/* A hash of action property widgets. The keys are the Glade property names
   ('AtkAction:<action-name>') and the values are the widgets in the property
   editor. */
static GHashTable *action_properties = NULL;
static gint n_action_properties = 0;

enum
{
  COLUMN_SELECTED,
  COLUMN_WIDGET_NAME
};


/***************************************************************************
 * Utility functions.
 ***************************************************************************/

/* Returns the property name to use in the property editor given a relation
   type name. We need to add a prefix to make sure we don't clash with other
   types of properties. The returned string must be freed. */
static gchar *
glade_atk_get_relation_property_name (const char *name)
{
  return g_strdup_printf ("AtkRelation:%s", name);
}


typedef struct _GladePropertyLabel GladePropertyLabel;
struct _GladePropertyLabel
{
  const char *name;
  const char *label;
};


/* This table lets us translate relation names to show in the property editor.
   These are the standard relations from from atkrelation.h. */
GladePropertyLabel relation_names_table[] = {
  { "controlled-by", N_("Controlled By") },
  { "controller-for", N_("Controller For") },
  { "label-for", N_("Label For") },
  { "labelled-by", N_("Labelled By") },
  { "member-of", N_("Member Of") },
  { "node-child-of", N_("Node Child Of") },
  { "flows-to", N_("Flows To") },
  { "flows-from", N_("Flows From") },
  { "subwindow-of", N_("Subwindow Of") },
  { "embeds", N_("Embeds") },
  { "embedded-by", N_("Embedded By") },
  { "popup-for", N_("Popup For") },
  { "parent-window-of", N_("Parent Window Of") }
};

/* This returns the translated relation name. We hardcode some strings here so
   we can translate them. The returned string should not be freed. */
static const gchar*
glade_atk_get_relation_name_for_display (const gchar *name)
{
  const char *label = name;
  gint i;

  for (i = 0; i < G_N_ELEMENTS (relation_names_table); i++)
    {
      if (!strcmp (name, relation_names_table[i].name))
	label = _(relation_names_table[i].label);
    }

  return label;
}

/* This returns the label to use for the given relation name. We hardcode some
   strings here so we can translate the action names. The returned string
   should be freed. */
static gchar*
glade_atk_get_relation_property_label (const gchar *name)
{
  const char *label;

  label = glade_atk_get_relation_name_for_display (name);
  return g_strdup_printf ("%s:", label);
}


/***************************************************************************
 * The Relations Dialog.
 ***************************************************************************/

typedef struct _GladeRelationsApplyData GladeRelationsApplyData;
struct _GladeRelationsApplyData
{
  gint row;
  GList *widget_pointers;

  GList *targets;
  GString *buffer;
};

static gboolean
relations_dialog_apply_cb (GtkTreeModel *model,
			   GtkTreePath *path,
			   GtkTreeIter *iter,
			   gpointer user_data)
{
  GladeRelationsApplyData *data = user_data;
  gboolean selected;

  gtk_tree_model_get (model, iter, COLUMN_SELECTED, &selected, -1);

  if (selected)
    {
      GtkWidget *widget = data->widget_pointers->data;

      if (widget)
	{
	  data->targets = g_list_prepend (data->targets, widget);

	  /* We add a weak pointer to the widget, so the element data will be
	     set to NULL if the widget is destroyed. */
#if 0
	  g_print ("Adding weak pointer Object:%p Pointer:%p\n",
		   widget, &data->targets->data);
#endif
	  g_object_add_weak_pointer (G_OBJECT (widget),
				     &data->targets->data);

	  /* Add a comma if the string isn't empty. */
	  if (data->buffer->len > 0)
	    g_string_append_c (data->buffer, ',');
	  g_string_append (data->buffer, gtk_widget_get_name (widget));
	}
    }

  if (data->widget_pointers)
    data->widget_pointers = data->widget_pointers->next;
  data->row++;
  return FALSE;
}


static void
relations_dialog_apply (GtkWidget *relations_dialog)
{
  GtkWidget *widget, *value_widget;
  GladeWidgetData *wdata;
  GtkTreeModel *model;
  AtkRelationType relationship;
  GList *widget_pointers;
  GladeRelationsApplyData data;

  widget = property_get_widget ();
  if (!widget)
    return;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);

  value_widget = gtk_object_get_data (GTK_OBJECT (relations_dialog),
				      GladeATKValueWidget);
  g_return_if_fail (value_widget != NULL);

  model = gtk_object_get_data (GTK_OBJECT (relations_dialog),
			       GladeATKRelationsListStore);
  g_return_if_fail (model != NULL);

  widget_pointers = gtk_object_get_data (GTK_OBJECT (relations_dialog),
					 GladeATKRelationsWidgets);

  relationship = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (value_widget), GladeATKRelationType));

  /* Step through the list of widgets in the dialog, adding selected widgets
     to a list of targets. */
  data.row = 0;
  data.widget_pointers = widget_pointers;
  data.buffer = g_string_sized_new (256);
  data.targets = NULL;
  gtk_tree_model_foreach (model, relations_dialog_apply_cb, &data);

  /* Update the field in the property editor. */
  gtk_entry_set_text (GTK_ENTRY (value_widget), data.buffer->str);
  g_string_free (data.buffer, TRUE);

  /* Now set the relation in the GladeWidgetData. */
  glade_widget_data_set_relation (wdata, relationship, data.targets);
}


static void
relations_dialog_response_cb (GtkWidget *dialog, gint response_id)
{

  if (response_id == GTK_RESPONSE_APPLY || response_id == GTK_RESPONSE_OK)
    relations_dialog_apply (dialog);

  if (response_id == GTK_RESPONSE_APPLY)
    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
				       GTK_RESPONSE_APPLY, FALSE);

  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide (dialog);
}


static void
relations_widget_toggled_cb (GtkWidget *widget, gchar *path_str,
			     GtkWidget *relations_dialog)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean selected;

  model = gtk_object_get_data (GTK_OBJECT (relations_dialog),
			       GladeATKRelationsListStore);

  /* Toggle the selected flag. */
  path = gtk_tree_path_new_from_string (path_str);
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (model, &iter, COLUMN_SELECTED, &selected, -1);
  selected = selected ? FALSE : TRUE;
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SELECTED,
		      selected, -1);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (relations_dialog),
				     GTK_RESPONSE_APPLY, TRUE);
}


static GtkWidget*
create_relations_dialog (GtkWidget *value_widget)
{
  GtkWindow *property_editor;
  GtkWidget *relations_dialog, *scrolledwin, *treeview;
  GtkListStore *list_store;
  GtkTreeViewColumn *col;
  GtkCellRenderer *rend;
  AtkRelationType relationship;
  const gchar *relation_name, *name;
  gchar *title;

  property_editor = GTK_WINDOW (gtk_widget_get_toplevel (value_widget));

  relationship = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (value_widget), GladeATKRelationType));
  relation_name = atk_relation_type_get_name (relationship);
  name = glade_atk_get_relation_name_for_display (relation_name);

  /* I don't think we should set the transient parent as the dialog could be
     left open if desired. */
  title = g_strdup_printf (_("Relationship: %s"), name);
  relations_dialog = gtk_dialog_new_with_buttons (title,
						  NULL /*property_editor*/,
						  GTK_DIALOG_NO_SEPARATOR,
						  GTK_STOCK_CLOSE,
						  GTK_RESPONSE_CLOSE,
						  GTK_STOCK_APPLY,
						  GTK_RESPONSE_APPLY,
						  GTK_STOCK_OK,
						  GTK_RESPONSE_OK,
						  NULL);
  g_free (title);
  gtk_window_set_default_size (GTK_WINDOW (relations_dialog), 250, 300);
  /* We say it is a normal window as it may be left open if desired. */
  gtk_window_set_type_hint (GTK_WINDOW (relations_dialog),
			    GDK_WINDOW_TYPE_HINT_NORMAL);

  scrolledwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwin);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwin),
				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (relations_dialog)->vbox),
		      scrolledwin, TRUE, TRUE, 0);

  list_store = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);

  treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  g_object_unref (G_OBJECT (list_store));
  gtk_widget_show (treeview);
  gtk_container_add (GTK_CONTAINER (scrolledwin), treeview);

  rend = gtk_cell_renderer_toggle_new ();
  col = gtk_tree_view_column_new_with_attributes ("", rend,
						  "active", COLUMN_SELECTED,
						  NULL);
  gtk_tree_view_column_set_resizable (col, FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
  gtk_signal_connect (GTK_OBJECT (rend), "toggled",
		      GTK_SIGNAL_FUNC (relations_widget_toggled_cb),
		      relations_dialog);

  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Widget"), rend,
						  "text", COLUMN_WIDGET_NAME,
						  NULL);
  gtk_tree_view_column_set_resizable (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  gtk_signal_connect (GTK_OBJECT (relations_dialog), "response",
		      GTK_SIGNAL_FUNC (relations_dialog_response_cb),
		      value_widget);
  gtk_signal_connect (GTK_OBJECT (relations_dialog), "delete-event",
		      GTK_SIGNAL_FUNC (gtk_true), NULL);

  /* Save a pointer to the dialog in the value widget and vice versa, and save
     a pointer to the list store in the dialog. */
  gtk_object_set_data (GTK_OBJECT (value_widget),
		       GladeATKRelationsDialog, relations_dialog);
  gtk_object_set_data (GTK_OBJECT (relations_dialog),
		       GladeATKValueWidget, value_widget);
  gtk_object_set_data (GTK_OBJECT (relations_dialog),
		       GladeATKRelationsListStore, list_store);

  return relations_dialog;
}


/* This is a recursive function to traverse the widget hierarchy and add all
   the GbWidgets to the list. */
void
scan_widgets (GtkWidget *widget, GList **widget_list)
{
  /* First add this widget, if it is a GbWidget. */
  if (GB_IS_GB_WIDGET (widget))
    {
      /* GbWidget names shouldn't be empty, but check just in case. */
      const gchar *name = gtk_widget_get_name (widget);
      if (name && *name)
	*widget_list = g_list_prepend (*widget_list, widget);
    }

  /* Now add child widgets recursively. */
  gb_widget_children_foreach (widget, (GtkCallback) scan_widgets,
			      widget_list);
}


gint
compare_widget_names (GtkWidget *a, GtkWidget *b)
{
  const char *namea = gtk_widget_get_name (a);
  const char *nameb = gtk_widget_get_name (b);
  return g_utf8_collate (namea, nameb);
}


/* Remove all the weak pointers to the widgets. */
static void
clear_relations_dialog_weak_pointers (GList *widget_pointers)
{
  GList *elem;

  for (elem = widget_pointers; elem; elem = elem->next)
    {
      GtkWidget *widget = elem->data;
      if (widget)
	{
#if 0
	  g_print ("Removing weak pointer Object:%p Pointer:%p\n",
		   widget, &elem->data);
#endif
	  g_object_remove_weak_pointer (G_OBJECT (widget), &elem->data);
	}
    }

  g_list_free (widget_pointers);
}


/* This refreshes the contents of a relations dialog. It clears the list,
   then adds all widgets in the same component as the widget shown in the
   property editor, selecting those that are in this relation. */
/* FIXME: Note that we could share the list of widgets in the component among
   all the relations dialogs. That would be more efficient, though it should
   only make a difference when several relations dialogs are left open. */
static void
glade_atk_refresh_relations_dialog (GtkWidget *relations_dialog)
{
  GtkWidget *widget, *value_widget, *toplevel;
  GladeWidgetData *wdata;
  GList *widget_list = NULL, *elem, *targets = NULL;
  GtkListStore *list_store;
  GList *widget_pointers;
  AtkRelationType relationship;

  widget = property_get_widget ();
  g_return_if_fail (widget != NULL);

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);

  value_widget = gtk_object_get_data (GTK_OBJECT (relations_dialog),
				      GladeATKValueWidget);
  g_return_if_fail (value_widget != NULL);

  /* Get a pointer to the list store that we set when creating the dialog. */
  list_store = gtk_object_get_data (GTK_OBJECT (relations_dialog),
				    GladeATKRelationsListStore);
  g_return_if_fail (list_store != NULL);

  /* Clear the list. */
  gtk_list_store_clear (list_store);

  /* Remove all the weak pointers to the widgets. */
  widget_pointers = gtk_object_get_data (GTK_OBJECT (relations_dialog),
					 GladeATKRelationsWidgets);
  clear_relations_dialog_weak_pointers (widget_pointers);
  widget_pointers = NULL;

  /* Traverse the widget hierarchy of the window the widget is in, adding
     all GbWidgets to a list, and sort it by their names. */
  toplevel = glade_util_get_toplevel (widget);
  scan_widgets (toplevel, &widget_list);
  widget_list = g_list_sort (widget_list, (GCompareFunc) compare_widget_names);

  /* Get the current relations, so we can check them in the list. */
  relationship = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (value_widget), GladeATKRelationType));
  for (elem = wdata->relations; elem; elem = elem->next)
    {
      GladeRelation *relation = elem->data;
      if (relation->relationship == relationship)
	{
	  targets = relation->targets;
	  break;
	}
    }

  /* Now add the widgets to the list store. If the widget is already
     in the relation, check the checkbox. */
  for (elem = widget_list; elem; elem = elem->next)
    {
      GtkWidget *widget_to_add = elem->data;
      GtkTreeIter iter;
      gboolean selected = FALSE;

      if (g_list_find (targets, widget_to_add))
	selected = TRUE;

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
			  0, selected,
			  1, gtk_widget_get_name (widget_to_add),
			  -1);

      widget_pointers = g_list_prepend (widget_pointers, widget_to_add);
      /* Add a weak pointer, so it gets set to NULL if the widget is
	 destroyed. */
#if 0
      g_print ("Adding weak pointer Object:%p Pointer:%p\n",
	       widget_to_add, &widget_pointers->data);
#endif
      g_object_add_weak_pointer (G_OBJECT (widget_to_add),
				 &widget_pointers->data);
    }

  widget_pointers = g_list_reverse (widget_pointers);
  gtk_object_set_data (GTK_OBJECT (relations_dialog),
		       GladeATKRelationsWidgets, widget_pointers);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (relations_dialog),
				     GTK_RESPONSE_APPLY, FALSE);
}


/* This is the callback that is called when the little button on the right
   of each relation property is clicked. value_button is the little button
   that was clicked. value_widget is the entry to the left of it. */
static void
show_relations_dialog (GtkWidget *value_button,
		       gpointer value_widget)
{
  GtkWidget *relations_dialog;

  /* Create the dialog if it we don't already have one for this relation. */
  relations_dialog = gtk_object_get_data (GTK_OBJECT (value_widget),
					  GladeATKRelationsDialog);
  if (!relations_dialog)
    relations_dialog = create_relations_dialog (value_widget);

  glade_atk_refresh_relations_dialog (relations_dialog);

  /* Show the dialog, or raise it if it is already shown. */
  gtk_widget_show (relations_dialog);
  gdk_window_show (GTK_WIDGET (relations_dialog)->window);
  gdk_window_raise (GTK_WIDGET (relations_dialog)->window);
}


/* This updates any relations dialogs which are currently visible, when the
   widget being shown in the property editor changes (maybe to NULL). */
void
glade_atk_update_relation_dialogs (void)
{
  GList *elem;

  /* Iterate over the list of relations value widgets (the entry fields in
     the property editor). Get the associated dialog, if it has been created,
     and check if it is visible. If it is, refresh it. */
  for (elem = relations_properties; elem; elem = elem->next)
    {
      GtkWidget *value_widget = elem->data;
      GtkWidget *relations_dialog;

      relations_dialog = gtk_object_get_data (GTK_OBJECT (value_widget),
					      GladeATKRelationsDialog);
      if (relations_dialog && GTK_WIDGET_DRAWABLE (relations_dialog))
	{
	  glade_atk_refresh_relations_dialog (relations_dialog);
	}
    }
}


/***************************************************************************
 * Creating the initial properties in the property editor.
 ***************************************************************************/

/* This creates the ATK properties page of the property editor.
   We add the standard atk properties and relations here. Actions and other
   relations may get added as needed. */
void
glade_atk_create_property_page	(GtkNotebook *notebook)
{
  GtkWidget *page, *icon, *vbox, *table;
  GdkPixbuf *pixbuf;
  /* Don't use AtkRelationType here since new values can be registered
     dynamically, which can cause problems with optimizing compilers. */
  int relationship;

  page = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (page),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (page);

  /* We use an icon for the notebook tab, as there isn't much room for text. */
  pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**) glade_atk_xpm);
  icon = gtk_image_new_from_pixbuf (pixbuf);
  gdk_pixbuf_unref (pixbuf);
  gtk_widget_show (icon);

  gtk_notebook_append_page (notebook, page, icon);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (page), vbox);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (GTK_BIN (page)->child),
				GTK_SHADOW_NONE);
  gtk_widget_show (vbox);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);

  property_set_table_position (table, 0);

  property_add_string (GladeATKName, _("Name:"),
		       _("The name of the widget to pass to assistive technologies"));
  property_add_text (GladeATKDescription, _("Description:"),
		       _("The description of the widget to pass to assistive technologies"), 4);
#if 0
  property_add_text (GladeATKTableCaption, _("Table Caption:"),
		       _("The table caption to pass to assistive technologies"), 2);
#endif

  /* We create separate tables for the actions and relations as we need to be
     able to add properties to them as they are needed. */
  actions_table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (actions_table), 1);
  gtk_widget_show (actions_table);
  gtk_box_pack_start (GTK_BOX (vbox), actions_table, FALSE, TRUE, 0);

  action_properties = g_hash_table_new (g_str_hash, g_str_equal);

  relations_table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (relations_table), 1);
  gtk_widget_show (relations_table);
  gtk_box_pack_start (GTK_BOX (vbox), relations_table, FALSE, TRUE, 0);

  /* We assume that all relation types will have already been registered by
     now. I hope that is correct. */
  property_set_table_position (relations_table, 0);
  for (relationship = 0; ; relationship++)
    {
      /* Skip these 2 as they aren't real relations. */
      if (relationship != ATK_RELATION_NULL
	  && relationship != ATK_RELATION_LAST_DEFINED)
	{
	  const gchar *name = atk_relation_type_get_name (relationship);
	  char *property_name, *property_label;
	  GtkWidget *value_widget;

	  if (!name)
	    break;

	  property_name = glade_atk_get_relation_property_name (name);
	  property_label = glade_atk_get_relation_property_label (name);

	  property_add_dialog (property_name, property_label,
			       _("Select the widgets with this relationship"),
			       FALSE, show_relations_dialog);

	  /* Store the property name and the relation type in the value widget,
	     so we can find them later. */
	  value_widget = property_get_value_widget (property_name);
	  relations_properties = g_list_prepend (relations_properties,
						 value_widget);
	  gtk_object_set_data_full (GTK_OBJECT (value_widget),
				    GladeATKPropertyName, property_name,
				    g_free);
	  gtk_object_set_data (GTK_OBJECT (value_widget),
			       GladeATKRelationType,
			       GINT_TO_POINTER (relationship));

	  g_free (property_label);
	}
    }
}


/***************************************************************************
 * Showing properties in the property editor.
 ***************************************************************************/

static void
glade_atk_get_atk_properties	(GtkWidget * widget,
				 AtkObject * atko,
				 GbWidgetGetArgData * data)
{
  const char *name, *description;
#if 0
  const char *table_caption;
  gboolean show_table_caption = FALSE;
#endif

  name = atk_object_get_name (atko);
  gb_widget_output_translatable_string (data, GladeATKName, name);

  description = atk_object_get_description (atko);
  gb_widget_output_translatable_text (data, GladeATKDescription, description);

#if 0
  if (GTK_IS_TREE_VIEW (widget))
    {
      show_table_caption = TRUE;

      g_object_get (G_OBJECT (atko),
		    "AtkObject::accessible-table-caption", &table_caption,
		    NULL);
      gb_widget_output_translatable_text (data, GladeATKTableCaption,
					  table_caption);
    }

  if (data->action == GB_SHOWING)
    property_set_visible (GladeATKTableCaption, show_table_caption);
#endif
}


/* Returns the property name to use in the property editor given an action
   name. We need to add a prefix to make sure we don't clash with other
   types of properties. The returned string must be freed. */
static gchar *
glade_atk_get_action_property_name (const char *name)
{
  return g_strdup_printf ("AtkAction:%s", name);
}


static void
hide_action_properties (const gchar * key, gpointer data, gpointer user_data)
{
  property_set_visible (key, FALSE);
}


/* This table lets us translate action names to show in the property editor.
   These are the only 3 actions I can find at present. */
GladePropertyLabel action_names_table[] = {
  { "click", N_("Click") },
  { "press", N_("Press") },
  { "release", N_("Release") }
};

/* This returns the label to use for the given action name. We hardcode some
   strings here so we can translate the action names. The returned string
   should be freed. */
static gchar*
get_action_property_label (const gchar *name)
{
  const char *label = name;
  gint i;

  for (i = 0; i < G_N_ELEMENTS (action_names_table); i++)
    {
      if (!strcmp (name, action_names_table[i].name))
	label = _(action_names_table[i].label);
    }

  return g_strdup_printf ("%s:", label);
}


static void
glade_atk_get_actions	(GtkWidget * widget,
			 AtkAction * action,
			 GbWidgetGetArgData * data)
{
  gint n_actions, i;

  /* Hide all the action properties. We'll show the ones the widget supports
     after. */
  g_hash_table_foreach (action_properties, (GHFunc) hide_action_properties,
			NULL);

  n_actions = atk_action_get_n_actions (action);
  for (i = 0; i < n_actions; i++)
    {
      const char *name, *description;
      char *property_name, *property_label;
      GtkWidget *value;
      gboolean free_property_name;

      name = atk_action_get_name (action, i);
      description = atk_action_get_description (action, i);
      property_name = glade_atk_get_action_property_name (name);
      free_property_name = TRUE;

      /* If we already have a property widget for this action, then show it,
	 else create a new property widget to edit the description. */
      value = property_get_value_widget (property_name);
      if (value)
	{
	  property_set_visible (property_name, TRUE);
	}
      else
	{
	  property_set_table_position (actions_table, n_action_properties);
	  property_label = get_action_property_label (name);
	  property_add_text (property_name, property_label,
			     _("Enter the description of the action to pass to assistive technologies"), 2);
	  g_free (property_label);
	  n_action_properties++;

	  /* Add to our hash. */
	  value = property_get_value_widget (property_name);
	  g_hash_table_insert (action_properties, property_name, value);
	  free_property_name = FALSE;
	}

      /* Now set the current value. */
      property_set_text (property_name, description);

      if (free_property_name)
	g_free (property_name);
    }
}


static void
glade_atk_get_relations		(GtkWidget * widget,
				 AtkObject * atko,
				 GbWidgetGetArgData * data)
{
  GladeWidgetData *wdata;
  GList *elem;
  GString *buffer;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);

  buffer = g_string_sized_new (256);

  /* Clear all the relations properties. */
  for (elem = relations_properties; elem; elem = elem->next)
    {
      GtkWidget *value_widget = elem->data;
      gtk_entry_set_text (GTK_ENTRY (value_widget), "");
    }

  /* Step through each of the relations in the GladeWidgetData. */
  for (elem = wdata->relations; elem; elem = elem->next)
    {
      GladeRelation *relation = elem->data;
      const gchar *relation_name;
      char *property_name;
      GtkWidget *value_widget;

      /* Find the property for the relationship. */
      relation_name = atk_relation_type_get_name (relation->relationship);
      property_name = glade_atk_get_relation_property_name (relation_name);

      value_widget = property_get_value_widget (property_name);
      if (value_widget)
	{
	  GList *target;

	  /* Convert the list of target widgets to a comma-separated string. */
	  for (target = relation->targets; target; target = target->next)
	    {
	      GtkWidget *target_widget = target->data;

	      /* Remember this is a weak pointer so may be NULL. */
	      if (target_widget)
		{
		  const char *target_name = gtk_widget_get_name (target_widget);
		  if (target_name && *target_name)
		    {
		      /* Add a comma if the string isn't empty. */
		      if (buffer->len > 0)
			g_string_append_c (buffer, ',');
		      g_string_append (buffer, target_name);
		    }
		}
	    }

	  gtk_entry_set_text (GTK_ENTRY (value_widget), buffer->str);
	  g_string_truncate (buffer, 0);
	}

      g_free (property_name);
    }

  g_string_free (buffer, TRUE);
}


/* This shows the widget's ATK properties in the property editor. */
void
glade_atk_get_properties	(GtkWidget * widget,
				 GbWidgetGetArgData * data)
{
  AtkObject *atko;

  atko = gtk_widget_get_accessible (widget);

  glade_atk_get_atk_properties (widget, atko, data);

  if (ATK_IS_ACTION (atko))
    {
      glade_atk_get_actions (widget, ATK_ACTION (atko), data);
      gtk_widget_show (actions_table);
    }
  else
    {
      gtk_widget_hide (actions_table);
    }

  glade_atk_get_relations (widget, atko, data);
}


/***************************************************************************
 * Applying properties from the property editor.
 ***************************************************************************/

static void
glade_atk_set_atk_properties	(GtkWidget * widget,
				 AtkObject * atko,
				 GbWidgetSetArgData * data)
{
  char *name, *description;

  name = gb_widget_input_string (data, GladeATKName);
  if (data->apply)
    {
      /* Set a flag to indicate the name has been set explicitly. */
      g_object_set_data (G_OBJECT (atko), GladeATKName, "T");
      atk_object_set_name (atko, name);
    }

  description = gb_widget_input_text (data, GladeATKDescription);
  if (data->apply)
    {
      g_object_set_data (G_OBJECT (atko), GladeATKDescription, "T");
      atk_object_set_description (atko, description);
    }
  g_free (description);

#if 0
  if (GTK_IS_TREE_VIEW (widget))
    {
      char *table_caption;

      table_caption = gb_widget_input_text (data, GladeATKTableCaption);
      if (data->apply)
	{
	  g_object_set_data (G_OBJECT (atko), GladeATKTableCaption, "T");
	  g_object_set (G_OBJECT (atko),
			"AtkObject::accessible-table-caption", table_caption,
			NULL);
	}
      g_free (table_caption);
    }
#endif
}


static void
glade_atk_set_actions	(GtkWidget * widget,
			 AtkAction * action,
			 GbWidgetSetArgData * data)
{
  gint n_actions, i;

  n_actions = atk_action_get_n_actions (action);
  for (i = 0; i < n_actions; i++)
    {
      const char *name;
      char *property_name, *description;

      name = atk_action_get_name (action, i);
      property_name = glade_atk_get_action_property_name (name);

      description = gb_widget_input_text (data, property_name);
      if (data->apply)
	atk_action_set_description (action, i, description);
      g_free (description);

      g_free (property_name);
    }
}


/* This applies the ATK properties from the property editor. */
void
glade_atk_set_properties	(GtkWidget * widget,
				 GbWidgetSetArgData * data)
{
  AtkObject *atko;

  atko = gtk_widget_get_accessible (widget);

  glade_atk_set_atk_properties (widget, atko, data);

  if (ATK_IS_ACTION (atko))
    glade_atk_set_actions (widget, ATK_ACTION (atko), data);

  /* Note that we don't have to set the relations here, since that is done
     when the relations dialog is closed. */
}


/***************************************************************************
 * Saving properties in the XML.
 ***************************************************************************/

static void
save_atk_property (GbWidgetGetArgData * data, GString *buffer, gint indent,
		   const gchar * tag_name, const gchar * tag_value)
{
  gboolean translatable, context;
  gchar *comments;

  if (tag_value == NULL)
    return;

  glade_util_get_translation_properties (data->widget, tag_name, &translatable,
					 &comments, &context);

  save_buffer_add_indent (buffer, indent);
  g_string_append (buffer, "<atkproperty name=\"");
  save_buffer_add_string (buffer, tag_name);
  g_string_append_c (buffer, '"');

  if (translatable)
    {
      g_string_append (buffer, " translatable=\"yes\"");

      if (context)
	g_string_append (buffer, " context=\"yes\"");

      if (comments && *comments)
	{
	  g_string_append (buffer, " comments=\"");
	  save_buffer_add_string (buffer, comments);
	  g_string_append_c (buffer, '"');
	}
    }

  g_string_append_c (buffer, '>');

  save_buffer_add_string (buffer, tag_value);
  g_string_append (buffer, "</atkproperty>\n");

  if (data->save_translatable_strings)
    save_add_translatable_string (data, tag_value);
}


static void
glade_atk_save_atk_properties	(GtkWidget * widget,
				 AtkObject * atko,
				 GbWidgetGetArgData * data,
				 GString   * buffer,
				 gint	     indent)
{
  const char *name, *description;

  /* We only save the name, description and caption if they are set explicitly
     by the user, and are not empty. There may be default settings. */
  name = atk_object_get_name (atko);
  if (name && *name && g_object_get_data (G_OBJECT (atko), GladeATKName))
    save_atk_property (data, buffer, indent, GladeATKName, name);

  description = atk_object_get_description (atko);
  if (description && *description
      && g_object_get_data (G_OBJECT (atko), GladeATKDescription))
    save_atk_property (data, buffer, indent, GladeATKDescription, description);

#if 0
  if (GTK_IS_TREE_VIEW (widget))
    {
      const char *table_caption;

      g_object_get (G_OBJECT (atko),
		    "AtkObject::accessible-table-caption", &table_caption,
		    NULL);
      if (table_caption && *table_caption
	  && g_object_get_data (G_OBJECT (atko), GladeATKTableCaption))
	save_atk_property (data, buffer, indent, GladeATKTableCaption,
			   table_caption);
    }
#endif
}


static void
glade_atk_save_actions		(GtkWidget * widget,
				 AtkAction * action,
				 GbWidgetGetArgData * data,
				 GString   * buffer,
				 gint	     indent)
{
  gint n_actions, i;

  n_actions = atk_action_get_n_actions (action);
  for (i = 0; i < n_actions; i++)
    {
      const char *name, *description;

      name = atk_action_get_name (action, i);
      description = atk_action_get_description (action, i);

      if (description && *description)
	{
	  save_buffer_add_indent (buffer, indent);
	  g_string_append (buffer, "<atkaction action_name=\"");
	  save_buffer_add_string (buffer, name);
	  g_string_append (buffer, "\" description=\"");
	  save_buffer_add_string (buffer, description);
	  g_string_append (buffer, "\"/>\n");

	  if (data->save_translatable_strings)
	    save_add_translatable_string (data, description);
	}
    }
}


static void
glade_atk_save_relations	(GtkWidget * widget,
				 AtkObject * atko,
				 GbWidgetGetArgData * data,
				 GString   * buffer,
				 gint	     indent)
{
  GladeWidgetData *wdata;
  GList *elem;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);

  for (elem = wdata->relations; elem; elem = elem->next)
    {
      GladeRelation *relation = elem->data;
      const gchar *relation_name;
      GList *target;

      relation_name = atk_relation_type_get_name (relation->relationship);

      /* We output a separate <atkrelation> element for each target. */
      for (target = relation->targets; target; target = target->next)
	{
	  GtkWidget *widget = target->data;
	  if (widget)
	    {
	      const gchar *target_name = gtk_widget_get_name (widget);
	      if (target_name && *target_name)
		{
		  save_buffer_add_indent (buffer, indent);
		  g_string_append (buffer, "<atkrelation target=\"");
		  save_buffer_add_string (buffer, target_name);
		  g_string_append (buffer, "\" type=\"");
		  save_buffer_add_string (buffer, relation_name);
		  g_string_append (buffer, "\"/>\n");
		}
	    }
	}
    }
}


/* This saves the ATK properties in the XML file. */
void
glade_atk_save_properties	(GtkWidget * widget,
				 GbWidgetGetArgData * data)
{
  AtkObject *atko;
  GString *buffer;
  gint indent;

  atko = gtk_widget_get_accessible (widget);

  /* We output all the ATK properties into a buffer. Then if it isn't empty
     we output an <accessibility> element with the buffer contents. */
  buffer = g_string_sized_new (1024);
  indent = data->indent + 1;

  glade_atk_save_atk_properties (widget, atko, data, buffer, indent);

  if (ATK_IS_ACTION (atko))
    glade_atk_save_actions (widget, ATK_ACTION (atko), data, buffer, indent);

  glade_atk_save_relations (widget, atko, data, buffer, indent);

  if (buffer->len > 0)
    {
      save_start_tag (data, "accessibility");
      g_string_append (data->buffer, buffer->str);
      save_end_tag (data, "accessibility");
    }

  g_string_free (buffer, TRUE);
}


/***************************************************************************
 * Loading properties from the XML.
 ***************************************************************************/

static void
glade_atk_load_atk_properties	(GtkWidget * widget,
				 AtkObject * atko,
				 GladeWidgetInfo * info)
{
  gint i;

  for (i = 0; i < info->n_atk_props; i++)
    {
      GladeProperty *prop = &info->atk_props[i];

      if (!strcmp (prop->name, GladeATKName))
	{
	  /* Set a flag to indicate the name has been set explicitly. */
	  g_object_set_data (G_OBJECT (atko), GladeATKName, "T");
	  atk_object_set_name (atko, prop->value);

	  glade_util_set_translation_properties (widget, GladeATKName,
						 prop->translatable,
						 prop->translator_comments,
						 prop->context_prefix);
	}
      else if (!strcmp (prop->name, GladeATKDescription))
	{
	  g_object_set_data (G_OBJECT (atko), GladeATKDescription, "T");
	  atk_object_set_description (atko, prop->value);

	  glade_util_set_translation_properties (widget, GladeATKDescription,
						 prop->translatable,
						 prop->translator_comments,
						 prop->context_prefix);
	}
#if 0
      else if (GTK_IS_TREE_VIEW (widget)
	       && !strcmp (prop->name, GladeATKTableCaption))
	{
	  g_object_set_data (G_OBJECT (atko), GladeATKTableCaption, "T");
	  g_object_set (G_OBJECT (atko),
			"AtkObject::accessible-table-caption", prop->value,
			NULL);

	  glade_util_set_translation_properties (widget, GladeATKTableCaption,
						 prop->translatable,
						 prop->translator_comments,
						 prop->context_prefix);
	}
#endif
    }
}


static void
glade_atk_load_actions		(GtkWidget * widget,
				 AtkAction * action,
				 GladeWidgetInfo * info)
{
  gint n_actions, i, j;

  n_actions = atk_action_get_n_actions (action);

  for (j = 0; j < info->n_atk_actions; j++)
    {
      GladeAtkActionInfo *action_info = &info->atk_actions[j];

      for (i = 0; i < n_actions; i++)
	{
	  const char *name;

	  name = atk_action_get_name (action, i);
	  if (!strcmp (name, action_info->action_name))
	    {
	      atk_action_set_description (action, i, action_info->description);
	      break;
	    }
	}
    }
}


static void
glade_atk_load_relations	(GtkWidget * widget,
				 AtkObject * atko,
				 GladeWidgetInfo *info,
				 GHashTable *all_widgets)
{
  GladeWidgetData *wdata;
  gint i;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);

  for (i = 0; i < info->n_relations; i++)
    {
      GladeAtkRelationInfo *relation = &info->relations[i];
      AtkRelationType relationship = ATK_RELATION_NULL;
      GtkWidget *target = NULL;

      /* Convert the type string to an integer. */
      if (relation->type && relation->type[0])
	{
	  relationship = atk_relation_type_for_name (relation->type);
	  if (relationship == ATK_RELATION_NULL)
	    g_warning ("Couldn't find relation type: %s", relation->type);
	}
      else
	g_warning ("<atkrelation> element has no type");

      /* Try to find the target widget. */
      if (relation->target && relation->target[0])
	{
	  target = g_hash_table_lookup (all_widgets, relation->target);
	  if (!target)
	    g_warning ("Couldn't find relation target: %s", relation->target);
	}
      else
	g_warning ("<atkrelation> element has no target");

      /* Add the target to the widget's GladeWidgetData. */
      if (relationship != ATK_RELATION_NULL && target)
	{
	  glade_widget_data_add_relation (wdata, relationship, target);
	}
    }
}


/* This loads the ATK properties from the XML file. It is called after the
   entire XML file is loaded and all the widgets are created, so that it
   can resolve relation targets properly. */
void
glade_atk_load_properties	(GtkWidget * widget,
				 GladeWidgetInfo * info,
				 GHashTable *all_widgets)
{
  AtkObject *atko;

  atko = gtk_widget_get_accessible (widget);

  glade_atk_load_atk_properties (widget, atko, info);

  if (ATK_IS_ACTION (atko))
    glade_atk_load_actions (widget, ATK_ACTION (atko), info);

  glade_atk_load_relations (widget, atko, info, all_widgets);
}





/***************************************************************************
 * Generating source code.
 ***************************************************************************/

static void
glade_gtk_source_add_v		(GString *buffer,
				 const gchar *format,
				 va_list args)
{
  gchar *buf;

  buf = g_strdup_vprintf (format, args);
  g_string_append (buffer, buf);
  g_free (buf);
}


static void
glade_atk_source_add		(GString *buffer,
				 const gchar *format,
				 ...)
{
  va_list args;

  va_start (args, format);
  glade_gtk_source_add_v (buffer, format, args);
  va_end (args);
}


void
glade_atk_source_add_translator_comments (GString *buffer,
					  gboolean translatable,
					  const gchar *comments)
{
  /* If the property isn't being translated we don't bother outputting the
     translator comments. */
  if (!translatable || !comments || comments[0] == '\0')
    return;

  /* We simply output it in a C comment.
     FIXME: If the comments contain an end of comment marker it won't
     compile. */
  g_string_append (buffer, "  /* ");
  g_string_append (buffer, comments);
  g_string_append (buffer, " */\n");
}


static void
glade_atk_write_atk_properties_source 	(GtkWidget * widget,
					 AtkObject * atko,
					 GbWidgetWriteSourceData * data,
					 GString   * buffer)
{
  const char *name, *description;
  gboolean translatable, context;
  gchar *comments;

  /* We only generate code for setting the name, description and caption if
     they are set explicitly by the user, and are not empty. There may be
     default settings. */
  name = atk_object_get_name (atko);
  if (name && *name && g_object_get_data (G_OBJECT (atko), GladeATKName))
    {
      glade_util_get_translation_properties (widget, GladeATKName,
					     &translatable,
					     &comments, &context);
      glade_atk_source_add_translator_comments (buffer, translatable,
						comments);

      glade_atk_source_add (buffer,
			    "  atk_object_set_name (atko, %s);\n",
			    source_make_string_full (name, data->use_gettext && translatable, context));
    }

  description = atk_object_get_description (atko);
  if (description && *description
      && g_object_get_data (G_OBJECT (atko), GladeATKDescription))
    {
      glade_util_get_translation_properties (widget, GladeATKDescription,
					     &translatable,
					     &comments, &context);
      glade_atk_source_add_translator_comments (buffer, translatable,
						comments);

      glade_atk_source_add (buffer,
			    "  atk_object_set_description (atko, %s);\n",
			    source_make_string_full (description, data->use_gettext && translatable, context));
    }

#if 0
  if (GTK_IS_TREE_VIEW (widget))
    {
      const char *table_caption;

      g_object_get (G_OBJECT (atko),
		    "AtkObject::accessible-table-caption", &table_caption,
		    NULL);
      if (table_caption && *table_caption
	  && g_object_get_data (G_OBJECT (atko), GladeATKTableCaption))
	{
	  glade_util_get_translation_properties (widget, GladeATKTableCaption,
						 &translatable,
						 &comments, &context);
	  glade_atk_source_add_translator_comments (buffer, translatable,
						    comments);

	  glade_atk_source_add (buffer,
				"  g_object_set (G_OBJECT (atko), \"AtkObject::accessible-table-caption\", %s,\n"
				"                NULL);\n",
			    source_make_string_full (table_caption, data->use_gettext && translatable, context));

	}
    }
#endif
}


static void
glade_atk_write_actions_source 	(GtkWidget * widget,
				 AtkAction * action,
				 GbWidgetWriteSourceData * data,
				 GString   * buffer)
{
  gint n_actions, i;

  n_actions = atk_action_get_n_actions (action);
  for (i = 0; i < n_actions; i++)
    {
      const char *name, *description;

      name = atk_action_get_name (action, i);
      description = atk_action_get_description (action, i);

      if (description && *description)
	{
	  /* We use a support function to set the action description. */
	  glade_atk_source_add (buffer,
				"  glade_set_atk_action_description (ATK_ACTION (atko), %s,\n",
				source_make_string (name, FALSE));

	  glade_atk_source_add (buffer,
				"    %s);\n",
				source_make_string (description,
						    data->use_gettext));
	}
    }
}


static void
glade_atk_write_relations_source 	(GtkWidget * widget,
					 AtkObject * atko,
					 GbWidgetWriteSourceData * data,
					 GString   * buffer)
{
  GladeWidgetData *wdata;
  gint max_targets = 0;
  GList *elem;
  gchar *decl;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);

  if (!wdata->relations)
    return;

  /* Find out the maximum number of targets in the relations. We'll use one
     temporary targets array for all of them. */
  for (elem = wdata->relations; elem; elem = elem->next)
    {
      GladeRelation *relation = elem->data;
      gint n_targets = 0;
      GList *target;

      for (target = relation->targets; target; target = target->next)
	{
	  if (target->data)
	    n_targets++;
	}

      max_targets = MAX (max_targets, n_targets);
    }

  if (max_targets == 0)
    return;

  source_ensure_decl (data, "  AtkRelationSet *tmp_relation_set;\n");
  source_ensure_decl (data, "  AtkRelationType tmp_relationship;\n");
  source_ensure_decl (data, "  AtkRelation *tmp_relation;\n");
  glade_atk_source_add (buffer,
			"  tmp_relation_set = atk_object_ref_relation_set (atko);\n");

  /* Output the declaration of the temporary targets array. */
  decl = g_strdup_printf ("  AtkObject *%s_relation_targets[%i];\n",
			  data->wname, max_targets);
  source_ensure_decl (data, decl);
  g_free (decl);

  for (elem = wdata->relations; elem; elem = elem->next)
    {
      GladeRelation *relation = elem->data;
      gint n_targets = 0, target_num = 0;
      GList *target;
      const gchar *relation_name;

      /* Remember that target widgets may be NULL if the target widget is
	 destroyed, so we count how many non-NULL targets there are. */
      for (target = relation->targets; target; target = target->next)
	{
	  if (target->data)
	    n_targets++;
	}

      if (n_targets == 0)
	continue;

      /* We have to get a reference to all the AtkObjects and add them to the
	 targets array. */
      for (target = relation->targets; target; target = target->next)
	{
	  GtkWidget *target_widget = target->data;
	  gchar *target_name;

	  target_name = source_create_valid_identifier (gtk_widget_get_name (target_widget));
	  glade_atk_source_add (buffer,
				"  %s_relation_targets[%i] = gtk_widget_get_accessible (%s);\n",
				data->wname, target_num, target_name);
	  g_free (target_name);
	  target_num++;
	}

      /* We have to output code to lookup the relationship code from the
	 string name, as it may be an externally-defined relationship so the
	 code may change. */
      relation_name = atk_relation_type_get_name (relation->relationship);

      glade_atk_source_add (buffer,
    "  tmp_relationship = atk_relation_type_for_name (%s);\n",
			    source_make_string (relation_name, FALSE));
      glade_atk_source_add (buffer,
    "  tmp_relation = atk_relation_new (%s_relation_targets, %i, tmp_relationship);\n",
			    data->wname, target_num);


      glade_atk_source_add (buffer,
			    "  atk_relation_set_add (tmp_relation_set, tmp_relation);\n"
			    "  g_object_unref (G_OBJECT (tmp_relation));\n");
    }

  glade_atk_source_add (buffer, "  g_object_unref (G_OBJECT (tmp_relation_set));\n");
}


/* This generates the source code to set the ATK properties, event descriptions
   and relations. */
void
glade_atk_write_source		(GtkWidget * widget,
				 GbWidgetWriteSourceData * data)
{
  AtkObject *atko;
  GString *buffer;

  atko = gtk_widget_get_accessible (widget);

  /* We output all the ATK code into a buffer. Then if it isn't empty
     we output the declaration for the AtkObject and the get_accessible()
     call. */
  buffer = g_string_sized_new (1024);

  glade_atk_write_atk_properties_source (widget, atko, data, buffer);

  if (ATK_IS_ACTION (atko))
    glade_atk_write_actions_source (widget, ATK_ACTION (atko), data, buffer);

  glade_atk_write_relations_source (widget, atko, data, buffer);

  if (buffer->len > 0)
    {
      source_ensure_decl (data, "  AtkObject *atko;\n");

      source_add_to_buffer (data, GLADE_ATK_SOURCE,
			    "  atko = gtk_widget_get_accessible (%s);\n%s\n",
			    data->wname, buffer->str);
    }

  g_string_free (buffer, TRUE);
}

