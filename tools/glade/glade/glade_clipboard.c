/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
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

#include "config.h"

#include <string.h>
#include <locale.h>
#include <stdlib.h>

#ifdef USE_GNOME
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif

#include "gladeconfig.h"

#include "editor.h"
#include "glade_clipboard.h"
#include "gbwidget.h"
#include "load.h"
#include "tree.h"
#include "utils.h"
#include "gb.h"
#include "glade_project_window.h"

typedef struct _GladeClipboardItem  GladeClipboardItem;

struct _GladeClipboardItem
{
  GladeProject *project;
  GtkType type;
  gchar *xml_data;
  gboolean names_unique;
};


static GtkWindowClass *parent_class = NULL;

static void glade_clipboard_class_init (GladeClipboardClass * klass);
static void glade_clipboard_init (GladeClipboard * clipboard);

static void glade_clipboard_cut_or_copy (GladeClipboard *clipboard,
					 GladeProject   *project,
					 GtkWidget      *widget,
					 gboolean	 cut);
static void glade_clipboard_add	(GladeClipboard *clipboard,
				 GladeProject   *project,
				 gboolean	 names_unique,
				 GtkWidget	*widget,
				 gchar		*xml_data);
static GladeClipboardItem* glade_clipboard_get_current_item (GladeClipboard *clipboard);

static void glade_clipboard_on_project_destroy (GladeProject *project,
						GladeClipboardItem *item);


#define GLADE_PASTE_BUFFER_INCREMENT	1024


GType
glade_clipboard_get_type (void)
{
  static GType glade_clipboard_type = 0;

  if (!glade_clipboard_type)
    {
      GtkTypeInfo glade_clipboard_info =
      {
	"GladeClipboard",
	sizeof (GladeClipboard),
	sizeof (GladeClipboardClass),
	(GtkClassInitFunc) glade_clipboard_class_init,
	(GtkObjectInitFunc) glade_clipboard_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      glade_clipboard_type = gtk_type_unique (gtk_window_get_type (),
					      &glade_clipboard_info);
    }
  return glade_clipboard_type;
}


static void
glade_clipboard_class_init (GladeClipboardClass * klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (gtk_window_get_type ());
}


static void
glade_clipboard_init (GladeClipboard * clipboard)
{
  GtkWidget *vbox, *scrolled_win;

  gtk_window_set_title (GTK_WINDOW (clipboard), _ ("Clipboard"));
  gtk_window_set_policy (GTK_WINDOW (clipboard), FALSE, TRUE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (clipboard), "clipboard", "Glade");
  gtk_window_set_default_size (GTK_WINDOW (clipboard), 150, 200);
  gtk_window_add_accel_group (GTK_WINDOW (clipboard),
			      glade_get_global_accel_group ());

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (clipboard), vbox);

  clipboard->clist = gtk_clist_new (1);
  gtk_clist_set_row_height (GTK_CLIST (clipboard->clist), 23);
  gtk_widget_set_usize (clipboard->clist, 100, 100);
  gtk_clist_set_column_width (GTK_CLIST (clipboard->clist), 0, 100);
  gtk_clist_set_selection_mode (GTK_CLIST (clipboard->clist),
				GTK_SELECTION_BROWSE);
  gtk_widget_show (clipboard->clist);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), clipboard->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  gtk_signal_connect (GTK_OBJECT (clipboard), "delete_event",
		      GTK_SIGNAL_FUNC (glade_util_close_window_on_delete),
		      NULL);

  gtk_signal_connect_after (GTK_OBJECT (clipboard), "hide",
                      GTK_SIGNAL_FUNC (glade_project_window_uncheck_clipboard_menu_item),
                      NULL);
}


GtkWidget *
glade_clipboard_new (void)
{
  return GTK_WIDGET (gtk_type_new (glade_clipboard_get_type ()));
}


void
glade_clipboard_cut (GladeClipboard *clipboard,
		     GladeProject   *project,
		     GtkWidget      *widget)
{
  glade_clipboard_cut_or_copy (clipboard, project, widget, TRUE);
}


void
glade_clipboard_copy (GladeClipboard *clipboard,
		      GladeProject   *project,
		      GtkWidget      *widget)
{
  glade_clipboard_cut_or_copy (clipboard, project, widget, FALSE);
}


static void
glade_clipboard_cut_or_copy (GladeClipboard *clipboard,
			     GladeProject   *project,
			     GtkWidget      *widget,
			     gboolean	     cut)
{
  GbWidgetGetArgData data = { 0 };
  gchar *old_locale, *saved_locale;
  GList *selection;
  gboolean is_component;

  if (widget == NULL)
    {
      selection = editor_get_selection ();
      if (selection)
	widget = GTK_WIDGET (selection->data);
    }

  if (widget == NULL || GB_IS_PLACEHOLDER (widget))
    return;

  data.project = project;
  data.action = GB_SAVING;
  data.copying_to_clipboard = TRUE;
  data.error = NULL;

  /* Initialize the output buffer. */
  data.buffer = g_string_sized_new (1024);
  data.indent = 0;

  /* We don't need the translatable strings. */
  data.save_translatable_strings = FALSE;
  data.translatable_strings = NULL;

  old_locale = setlocale (LC_NUMERIC, NULL);
  saved_locale = g_strdup (old_locale);
  setlocale (LC_NUMERIC, "C");

  g_string_append (data.buffer,
		   "<?xml version=\"1.0\" standalone=\"no\"?> <!--*- mode: xml -*-->\n"
		   "<!DOCTYPE glade-interface SYSTEM \"http://glade.gnome.org/glade-project-2.0.dtd\">\n"
		   "\n"
		   "<glade-interface>\n");

  /* If the widget is not a toplevel widget, we add a dummy parent to it.
     This is so that we keep the packing properties of the widget. */
  is_component = glade_util_is_component (widget);
  if (!is_component)
    {
      g_string_append (data.buffer,
		       "<widget class=\"GtkWindow\" id=\"glade-dummy-container\">\n");
      data.indent++;
    }

  gb_widget_save (widget, &data);

  if (!is_component)
    {
      g_string_append (data.buffer,
		       "</widget>\n");
    }

  g_string_append (data.buffer,
		   "\n</glade-interface>\n");

  setlocale (LC_NUMERIC, saved_locale);
  g_free (saved_locale);

  if (data.error == NULL)
    {
#if 0
      g_print ("Adding to clipboard:\n%s\n", data.buffer->str);
#endif
      glade_clipboard_add (GLADE_CLIPBOARD (clipboard), project, cut,
			   widget, data.buffer->str);
      if (cut)
	editor_delete_widget (widget);
    }
  else
    /* This shouldn't happen. */
    g_warning ("Error saving widget to clipboard");

  g_string_free (data.buffer, TRUE);
}


static void
initialize_all_widgets_cb (GtkWidget *widget, GbWidgetSetArgData *data)
{
  const gchar *widget_name;

  /* If this is the widget we are replacing, just return. We don't want to
     add it or its descendants to all_widgets. They will no longer exist, so
     ATK relations can't use them. */
  if (widget == data->replacing_widget)
    return;

  /* Add this widget to the all_widgets hash. */
  if (GB_IS_GB_WIDGET (widget))
    {
      widget_name = gtk_widget_get_name (widget);
      if (widget_name && *widget_name)
	{
#if 0
	  g_print ("Initializing pointer to widget: %s, %p\n",
		   widget_name, widget);
#endif
	  g_hash_table_insert (data->all_widgets, (gpointer) widget_name,
			       widget);
	}
    }

  /* Now recursively add the children. */
  gb_widget_children_foreach (widget, (GtkCallback) initialize_all_widgets_cb,
			      data);
}


/* This adds all widgets that are in the window being pasted into to the
   all_widgets hash, so ATK relations to other widgets will be set when
   pasting. */
static void
glade_clipboard_initialize_all_widgets (GbWidgetSetArgData *data)
{
  GtkWidget *toplevel;

  /* If we aren't pasting into a component, just return. */
  if (!data->replacing_widget)
    return;

  toplevel = glade_util_get_toplevel (data->replacing_widget);

  initialize_all_widgets_cb (toplevel, data);
}


/* The widget argument is the widget to be pasted over, i.e. the widget in
   the current interface that will be replaced by the clipboard item.
   If it is NULL, the currently selected widget will be used. */
void
glade_clipboard_paste (GladeClipboard *clipboard,
		       GladeProject   *project,
		       GtkWidget      *widget)
{
  GladeClipboardItem *item;
  GbWidgetSetArgData data = { 0 };
  GtkWidget *parent, *new_widget = NULL;
  gchar *saved_locale, *saved_timezone, *child_name;
  GList *selection;
  gboolean is_component = FALSE;

  item = glade_clipboard_get_current_item (clipboard);
  if (item == NULL)
    return;

  if (widget == NULL)
    {
      selection = editor_get_selection ();
      if (selection)
	widget = GTK_WIDGET (selection->data);
    }

  if (gtk_type_is_a (item->type, gtk_window_get_type ())
      || gtk_type_is_a (item->type, gtk_menu_get_type ()))
    is_component = TRUE;

  /* We can only paste toplevel components, i.e. windows and menus, into the
     project, and not into other widgets. */
  if (widget == NULL)
    {
      if (!is_component)
	{
	  glade_util_show_message_box (_("You need to select a widget to paste into"), widget);
	  return;
	}
    }
  else
    {
      if (is_component)
	widget = NULL;
    }

  if (widget)
    {
      /* Don't allow pasting into any windows/dialogs. */
      if (GTK_IS_WINDOW (widget)
	  || GTK_IS_DIALOG (widget)
	  || GTK_IS_COLOR_SELECTION_DIALOG (widget)
	  || GTK_IS_INPUT_DIALOG (widget)
	  || GTK_IS_FONT_SELECTION_DIALOG (widget)
	  || GTK_IS_FILE_SELECTION (widget)
#ifdef USE_GNOME
	  || GNOME_IS_APP (widget)
	  || GNOME_IS_DIALOG (widget)
#endif
	  )
	{
	  glade_util_show_message_box (_("You can't paste into windows or dialogs."), widget);
	  return;
	}

      /* SPECIAL CODE: Don't allow pasting into dialog widgets. */
      child_name = gb_widget_get_child_name (widget);
      if (child_name)
	{
	  if (!strcmp (child_name, GladeChildOKButton)
	      || !strcmp (child_name, GladeChildCancelButton)
	      || !strcmp (child_name, GladeChildHelpButton)
	      || !strcmp (child_name, GladeChildApplyButton)
	      || !strcmp (child_name, GladeChildSaveButton)
	      || !strcmp (child_name, GladeChildCloseButton)
	      || !strcmp (child_name, GladeChildDialogVBox)
	      || !strcmp (child_name, GladeChildDialogActionArea)
	      || !strcmp (child_name, GladeChildComboEntry)
	      || !strcmp (child_name, GladeChildComboList)
	      || !strcmp (child_name, GladeChildFontSelection)
	      || !strcmp (child_name, GladeChildColorSelection)
	      || !strcmp (child_name, GladeChildGnomeEntry)
	      )
	    {
	      glade_util_show_message_box (_("You can't paste into the selected widget, since\nit is created automatically by its parent."), widget);
	      return;
	    }
	}

      /* Only allow menuitems to be pasted into menus or menubars. */
      if (GTK_IS_MENU_SHELL (widget)
	  && !gtk_type_is_a (item->type, gtk_menu_item_get_type ()))
	{
	  glade_util_show_message_box (_("Only menu items can be pasted into a menu or menu bar."), widget);
	  return;
	}

      /* Only allow menuitems to replace menuitems. */
      if (GTK_IS_MENU_ITEM (widget)
	  && !gtk_type_is_a (item->type, gtk_menu_item_get_type ()))
	{
	  glade_util_show_message_box (_("Only menu items can be pasted into a menu or menu bar."), widget);
	  return;
	}

      /* Only buttons can be pasted into a dialog action area. */
      if (widget->parent)
	{
	  child_name = gb_widget_get_child_name (widget->parent);
	  if (child_name && !strcmp (child_name, GladeChildDialogActionArea)
	      && item->type != GTK_TYPE_BUTTON)
	    {
	      glade_util_show_message_box (_("Only buttons can be pasted into a dialog action area."), widget);
	      return;
	    }
	}

#ifdef USE_GNOME
      if (BONOBO_IS_DOCK (widget))
	{
	  if (!gtk_type_is_a (item->type, bonobo_dock_item_get_type ()))
	    {
	      glade_util_show_message_box (_("Only GnomeDockItem widgets can be pasted into a GnomeDock."), widget);
	      return;
	    }
	}

      if (BONOBO_IS_DOCK_ITEM (widget))
	{
	  if (!gtk_type_is_a (item->type, bonobo_dock_item_get_type ()))
	    {
	      glade_util_show_message_box (_("Only GnomeDockItem widgets can be pasted over a GnomeDockItem."), widget);
	      return;
	    }
	  glade_util_show_message_box (_("Sorry - pasting over a GnomeDockItem is not implemented yet."), widget);
	  return;
	}

      if (gtk_type_is_a (item->type, bonobo_dock_item_get_type ()))
	{
	  if (!BONOBO_IS_DOCK (widget) && !BONOBO_IS_DOCK_ITEM (widget))
	    {
	      glade_util_show_message_box (_("GnomeDockItem widgets can only be pasted into a GnomeDock."), widget);
	      return;
	    }
	}
#endif
    }

  data.project = project;
  data.filename = NULL;
  data.xml_buffer = item->xml_data;
  data.status = GLADE_STATUS_OK;
  data.all_widgets = g_hash_table_new (g_str_hash, g_str_equal);

  /* We normally create new names for the widgets pasted, discarding the names
     in the XML data. However, if the widgets were cut to the clipboard, and
     we are pasting into the same project for the first time, then we use
     the original names. */
  data.discard_names = TRUE;
  if (project == item->project)
    {
      if (item->names_unique)
	{
	  data.discard_names = FALSE;
	  item->names_unique = FALSE;
	}
    }

  /* We always try to replace the selected widget, unless it is a GtkFixed,
     GtkLayout, or GtkPacker, in which case we add the pasted widget as a
     child. If we didn't do this, then it would be quite difficult to paste
     children into these widgets. I did consider adding the widget as a child
     when pasting into other containers like boxes and tables, but then we
     have a problem with composite widgets, where we may accidentally paste
     inside the composite (e.g. inside its toplevel vbox).
     For Gnome, we also allow pasting GnomeDockItems into GnomeDocks. */
  parent = NULL;
  data.replacing_widget = NULL;
  if (widget)
    {
      if (GTK_IS_FIXED (widget)
	  || GTK_IS_LAYOUT (widget)
#ifdef USE_GNOME
	  || BONOBO_IS_DOCK (widget)
#endif
	  )
	{
	  parent = widget;
	}
      else
	{
	  parent = widget->parent;
	  data.replacing_widget = widget;
	}
    }

  /* Initialize the all_widgets hash with all widget names in the component,
     except the widget being replaced and any of its descendants. This is
     done so that ATK relations to targets in the component will still be OK.
  */
  glade_clipboard_initialize_all_widgets (&data);

  tree_freeze ();

  /* Set the locale to "C". */
  saved_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
  setlocale (LC_NUMERIC, "C");

  /* Set the timezone to "UTC". */
  saved_timezone = glade_util_set_timezone ("UTC");

  /* Now parse the clipboard data. */
  data.interface = glade_parser_parse_buffer (data.xml_buffer,
					      strlen (data.xml_buffer), NULL);
  if (!data.interface || data.interface->n_toplevels != 1)
    {
      /* I don't think this should happen, except due to bugs. */
      g_warning ("Error pasting from clipboard");
      glade_interface_destroy (data.interface);
      g_hash_table_destroy (data.all_widgets);
      return;
    }

  /* If the widget is a toplevel component, we use the top widget info and
     no child info. If not, we use the first child, as the toplevel will be
     a dummy container. */
  data.child_info = NULL;
  data.widget_info = data.interface->toplevels[0];

  if (!is_component)
    {
      if (data.widget_info->n_children != 1
	  || data.widget_info->children == NULL)
	{
	  data.status = GLADE_STATUS_ERROR;
	}
      else
	{
	  data.child_info = &data.widget_info->children[0];
	  data.widget_info = data.child_info->child;
	}
    }

  /* Create the widget. */
  if (data.status == GLADE_STATUS_OK)
    new_widget = gb_widget_load (NULL, &data, parent);

  /* Destroy the parse data. */
  glade_interface_destroy (data.interface);

  /* Reset the timezone. */
  glade_util_reset_timezone (saved_timezone);

  /* Reset the locale. */
  setlocale (LC_NUMERIC, saved_locale);
  g_free (saved_locale);

  tree_thaw ();

  g_hash_table_destroy (data.all_widgets);

  if (data.status != GLADE_STATUS_OK)
    {
      /* I don't think this should happen, except due to bugs. */
      g_warning ("Error pasting from clipboard");
      return;
    }

  /* If a window was pasted, show it. */
  if (GTK_IS_WINDOW (new_widget))
    glade_project_show_component (project, new_widget);
}


/* This adds the XML for a widget (and any descendants) to the clipboard.
   If the widget has been cut and is later pasted into the same project,
   then the same widget names can be used. Otherwise new widget names have
   to be created. The widget parameter is used to get the widget's name and
   class and to look up its pixmap to display in the clipboard.
   The xml_data parameter is copied. */
static void
glade_clipboard_add	(GladeClipboard *clipboard,
			 GladeProject   *project,
			 gboolean	 names_unique,
			 GtkWidget	*widget,
			 gchar		*xml_data)
{
  GladeClipboardItem *item;
  GbWidget *gbwidget;
  gchar *name;

  name = (char*) gtk_widget_get_name (widget);
  gbwidget = gb_widget_lookup (widget);
  g_return_if_fail (gbwidget != NULL);

  item = g_new (GladeClipboardItem, 1);
  item->project = project;
  item->type = GTK_OBJECT_TYPE (widget);
  item->xml_data = g_strdup (xml_data);
  item->names_unique = names_unique;

  /* Connect to the project's destroy signal to set the pointer to NULL, so
     we never have invalid pointers. */
  gtk_signal_connect (GTK_OBJECT (project), "destroy",
		      GTK_SIGNAL_FUNC (glade_clipboard_on_project_destroy),
		      item);

  gtk_clist_insert (GTK_CLIST (clipboard->clist), 0, &name);
  gtk_clist_set_row_data (GTK_CLIST (clipboard->clist), 0, item);

  gtk_clist_set_pixtext (GTK_CLIST (clipboard->clist), 0, 0, name, 3,
			 gbwidget->gdkpixmap, gbwidget->mask);

  gtk_clist_select_row (GTK_CLIST (clipboard->clist), 0, 0);
}


/* This returns the currently-selected GladeClipboardItem, or NULL if no item
   is currently selected (i.e. the clipboard is empty). */
static GladeClipboardItem*
glade_clipboard_get_current_item	(GladeClipboard *clipboard)
{
  GList *selection;

  selection = GTK_CLIST (clipboard->clist)->selection;
  if (selection == NULL)
    return NULL;

  return (GladeClipboardItem*) gtk_clist_get_row_data (GTK_CLIST (clipboard->clist), GPOINTER_TO_INT (selection->data));
}


static void
glade_clipboard_on_project_destroy (GladeProject *project,
				    GladeClipboardItem *item)
{
  if (item)
    item->project = NULL;
}
