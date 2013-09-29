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

#include <gtk/gtk.h>
#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#endif

#include "editor.h"
#include "gb.h"
#include "gbwidget.h"
#include "glade.h"
#include "glade_atk.h"
#include "glade_gnome.h"
#include "glade_gtk12lib.h"
#include "glade_plugin.h"
#include "glade_project.h"
#include "load.h"
#include "palette.h"
#include "property.h"
#include "save.h"
#include "tree.h"
#include "utils.h"

#ifdef USE_GNOME
#include "glade_gnomelib.h"
#endif

#ifdef USE_GNOME_DB
#include "glade_gnomedblib.h"
#endif

/*
 * The hash table associates a Gtk widget class name with a GbWidget struct,
 * which contains a table of functions used by the builder, e.g. creating a
 * new widget, saving the widget etc.
 */
static GHashTable *gb_widget_table = NULL;

/* Tooltips for the created widgets */
static GtkTooltips *gb_widget_tooltips;


/* This is the GTK+ stock id string, used in GtkImageMenuItem widgets. */
const gchar *GladeMenuItemStockIDKey = "GladeMenuItemStockIDKey";

/* This is the old Gnome stock index key. */
const gchar *GladeMenuItemStockIndexKey = "GladeMenuItemStockIndexKey";

/* This is the key we use to store the stock icon name or full pathname. */
const gchar *GladeIconKey = "GladeIconKey";

/* This is used to save a pointer to the GladeWidgetInfo inside a widget while
   loading, so we can resolve ATK relations afterwards. */
const gchar *GladeWidgetInfoKey = "GladeWidgetInfoKey";


static void gb_widget_init_widget_lib (GladePaletteSectionData *sections);

static void on_widget_destroy (GtkWidget * widget,
			       gpointer data);

#ifdef GLADE_STYLE_SUPPORT
static void show_color_properties (GdkColor colors[],
				   gchar * name);
#endif
static void show_accelerators (GtkWidget * widget,
			       GbWidgetGetArgData * data);
static void show_signals (GtkWidget * widget,
			  GbWidgetGetArgData * data);

static void set_standard_properties (GtkWidget * widget,
				     GbWidgetSetArgData * data);
static void set_position_properties (GtkWidget * widget,
				     GbWidgetSetArgData * data);
static void set_special_child_properties (GtkWidget * widget,
					  GbWidgetSetArgData * data);
static void set_lang_specific_properties (GtkWidget * widget,
					  GbWidgetSetArgData * data);
#if 0
static void apply_style (GtkWidget * widget,
			 GbWidgetSetArgData * data);
static gboolean apply_colors (GtkWidget * widget,
			      GbWidgetSetArgData * data,
			      GdkColor colors[],
			      GdkColor new_colors[],
			      gchar * name);
#endif
static void apply_accelerators (GtkWidget * widget,
				GbWidgetSetArgData * data);
static void apply_signals (GtkWidget * widget,
			   GbWidgetSetArgData * data);

static void add_standard_top_menu_items (GtkWidget * widget,
					 GbWidgetCreateMenuData * data);
static void add_standard_bottom_menu_items (GtkWidget * widget,
					    GbWidgetCreateMenuData * data);

static void get_standard_properties (GtkWidget * widget,
				     GbWidgetGetArgData * data);

static void get_position_properties (GtkWidget * widget,
				     GbWidgetGetArgData * data);
static void get_lang_specific_properties (GtkWidget * widget,
					  GbWidgetGetArgData * data);

static void save_accelerators (GtkWidget * widget,
			       GbWidgetGetArgData * data);
static void save_signals (GtkWidget * widget,
			  GbWidgetGetArgData * data);


static void gb_widget_add_alignment (GtkWidget * menuitem,
				     GtkWidget * widget);
static void gb_widget_remove_alignment (GtkWidget * menuitem,
					GtkWidget * widget);
static void gb_widget_add_event_box (GtkWidget * menuitem,
				     GtkWidget * widget);
static void gb_widget_remove_event_box (GtkWidget * menuitem,
					GtkWidget * widget);
static void gb_widget_redisplay_window (GtkWidget * menuitem,
					GtkWidget * widget);
static void gb_widget_add_scrolled_window (GtkWidget * menuitem,
					   GtkWidget * widget);
static void gb_widget_remove_scrolled_window (GtkWidget * menuitem,
					      GtkWidget * widget);

static void table_foreach (GtkTable * table,
			   GtkCallback callback,
			   gpointer callback_data);
static void box_foreach (GtkBox *box,
			 GtkCallback callback,
			 gpointer callback_data);

static gint find_notebook_page (GtkNotebook * notebook,
				GtkWidget * current_child,
				GtkWidget **page,
				GtkWidget **tab_label);


/* These aren't included in the Bonobo headers, so we declare them here to
   avoid warnings. */
#ifdef USE_GNOME
void bonobo_dock_item_get_floating_position (BonoboDockItem *item,
					     gint *x, gint *y);
gboolean bonobo_dock_item_detach (BonoboDockItem *item, gint x, gint y);
#endif

/*************************************************************************
 * Initialization functions
 *************************************************************************/

void
gb_widgets_init ()
{
  gb_widget_table = g_hash_table_new (g_str_hash, g_str_equal);

#ifdef GLADE_STYLE_SUPPORT
  gb_widget_reset_gb_styles ();
#endif

  /* Create tooltips */
  gb_widget_tooltips = gtk_tooltips_new ();

  gb_widget_init_widget_lib (get_gtk_widgets());
#ifdef USE_GNOME
  gb_widget_init_widget_lib (get_gnome_widgets());
#endif
#ifdef USE_GNOME_DB
  gb_widget_init_widget_lib (get_gnome_db_widgets());
#endif

  glade_plugin_load_plugins ();
}

static void
gb_widget_init_widget_lib (GladePaletteSectionData *sections)
{
  gint index, j;
  GladeWidgetInitData *gwid;
  GladePaletteSectionData *palsec;
  GbWidget *(*init_func) ();
  GbWidget *gbwidget;

  index = 0;
  while (1)
    {
      j = 0;
      palsec = &sections[index];
      index++;
      if (!palsec->section)
        break;
      while (1)
        {
          gwid = &palsec->widgets[j];
          j++;
          if (!gwid->name)
            break;  
          init_func = gwid->init_func;
          gbwidget = (*init_func) ();
	  gb_widget_register_gbwidget (gwid->name, gbwidget);
	  palette_add_gbwidget (gbwidget, palsec->section, gwid->name);
        }
    }
}


/* Adds a GbWidget to the hash of known GbWidgets. The class_id is copied. */
void
gb_widget_register_gbwidget (const gchar *class_id,
			     GbWidget    *gbwidget)
{
  gbwidget->class_id = g_strdup (class_id);
  g_hash_table_insert (gb_widget_table, gbwidget->class_id, gbwidget);
}


/* This returns the GbWidget struct corresponding to the given class name. */
GbWidget *
gb_widget_lookup_class (const gchar *class_id)
{
  GbWidget *gbwidget;

  gbwidget = (GbWidget *) g_hash_table_lookup (gb_widget_table, class_id);

  return gbwidget;
}


/* This returns the GbWidget struct corresponding to the given widget. */
GbWidget *
gb_widget_lookup (GtkWidget *widget)
{
  GladeWidgetData *wdata;

  wdata = gtk_object_get_data (GTK_OBJECT (widget),
			       GB_WIDGET_DATA_KEY);

  if (!wdata) {
    const gchar *class_name;

    MSG ("Widget has no associated GladeWidgetData");

    /* Fall back to the original way we used to get the GbWidget*, since
       some widgets currently don't have a GladeWidgetData attached. */
    class_name = gtk_type_name (GTK_OBJECT_TYPE (widget));
    return (GbWidget *) g_hash_table_lookup (gb_widget_table, class_name);
  }

  return wdata->gbwidget;
}


gchar*
gb_widget_get_class_id (GtkWidget *widget)
{
  GbWidget *gbwidget;

  gbwidget = gb_widget_lookup (widget);

  if (gbwidget)
    return gbwidget->class_id;
  else
    return (char*) gtk_type_name (GTK_OBJECT_TYPE (widget));
}


void
gb_widget_init_struct (GbWidget * gbwidget)
{
  gbwidget->pixmap_struct = NULL;
  gbwidget->class_id = NULL;
  gbwidget->gdkpixmap = NULL;
  gbwidget->mask = NULL;
  gbwidget->tooltip = NULL;
  gbwidget->pixbuf = NULL;
  gbwidget->properties_page_number = GB_PROPERTIES_NOT_CREATED;
  gbwidget->child_properties_page_number = GB_PROPERTIES_NOT_CREATED;

  gbwidget->gb_widget_new = NULL;
  gbwidget->gb_widget_create_from_widget = NULL;

  gbwidget->gb_widget_create_properties = NULL;
  gbwidget->gb_widget_get_properties = NULL;
  gbwidget->gb_widget_set_properties = NULL;

  gbwidget->gb_widget_add_child = NULL;
  gbwidget->gb_widget_get_child = NULL;

  gbwidget->gb_widget_create_child_properties = NULL;
  gbwidget->gb_widget_get_child_properties = NULL;
  gbwidget->gb_widget_set_child_properties = NULL;

  gbwidget->gb_widget_write_add_child_source = NULL;

  gbwidget->gb_widget_create_popup_menu = NULL;

  gbwidget->gb_widget_write_source = NULL;

  gbwidget->gb_widget_destroy = NULL;
}



/*************************************************************************
 * Functions for creating & destroying GbWidgets
 *************************************************************************/

GtkWidget *
gb_widget_new (const gchar * class_id, GtkWidget * parent)
{
  return gb_widget_new_full (class_id, TRUE, parent, NULL, 0, 0, NULL,
			     GB_CREATING, NULL);
}


/* Creates a new widget.
 * class_id is the name of the widget class, e.g. 'GtkLabel'
 * create_default_name is TRUE if you want a default name to be created,
 *   e.g. 'label1'.
 * parent is the widget that the new widget will be added beneath, so that
 *   the callback function knows where to put the widget.
 * current_child is the widget that the new widget will replace, or NULL
 *   if the new widget is just being added. It is used when replacing
 *   placeholders.
 * x & y are the coordinates of the new widget if it is being added to a
 *   GtkFixed container.
 * callback is the function to call once the widget is created to actually
 *   add it to the parent. Some widgets require dialog boxes for creating
 *   them (e.g. the dialog box to set the number of rows/cols in a table).
 *   So we need to provide a function to be called after this is done
 *   (we could have possibly chosen to use modal dialogs instead.)
 * action is either GB_CREATING or GB_LOADING. When loading widgets we don't
 *   want dialog boxes to pop up when they are being created.
 * loading_data is set when action is GB_LOADING, and contains the data used
 *   while loading, so that the GbWidgets can get any properties they need to
 *   create the widget without popping up a dialog box.
 */
GtkWidget *
gb_widget_new_full (const gchar * class_id, gboolean create_default_name,
		    GtkWidget * parent, GtkWidget * current_child,
		    gint x, gint y, GbWidgetNewCallback callback,
		    GbWidgetAction action, GbWidgetSetArgData * loading_data)
{
  GbWidgetNewData *data;
  GtkWidget *new_widget;
  GbWidget *gbwidget;
  GType type;

  gbwidget = gb_widget_lookup_class (class_id);
  g_return_val_if_fail (gbwidget != NULL, NULL);

  /* Note that for custom widgets this won't be found, and so will be 0. */
  type = g_type_from_name (class_id);
#if 0
  g_print ("Class Name: %s Type: %i\n", class_id, type);
#endif

  data = g_new (GbWidgetNewData, 1);
  /* Don't set data->name to NULL, since many widgets use it to set the label
     of the new widget. */
  data->project = current_project;
  data->name = create_default_name ? glade_project_new_widget_name (data->project, class_id) : g_strdup ("");
  data->callback = callback;
  data->parent = parent;
  if (parent) {
    gtk_widget_ref (parent);
    if (!gb_widget_lookup (parent))
       MSG2 ("Registering unknown widget '%s' as parent of '%s'",
	     G_OBJECT_TYPE_NAME (parent), class_id);
  }
  data->current_child = current_child;
  if (current_child)
    gtk_widget_ref (current_child);
  data->x = x;
  data->y = y;
  data->widget_data = glade_widget_data_new (gbwidget);
  data->action = action;
  data->loading_data = loading_data;

  if (gbwidget->gb_widget_new)
    new_widget = (gbwidget->gb_widget_new) (data);
  else if (type != 0)
    new_widget = gtk_widget_new (type, NULL);
  else
    g_return_val_if_fail ((new_widget = NULL), NULL);

  /* If the widget has been created immediately, then we can finish it off now,
     and free the GbWidgetNewData struct, otherwise we leave that to the
     dialog. */
  if (new_widget)
    {
      gb_widget_initialize (new_widget, data);
      if (data->callback)
	(*data->callback) (new_widget, data);
      gb_widget_free_new_data (data);
    }

  return new_widget;
}

static void
gb_widget_real_initialize (GtkWidget * widget, GladeWidgetData * wdata)
{
  g_return_if_fail (wdata != NULL);
  g_return_if_fail (wdata->gbwidget != NULL);

  /* Make sure GtkMenu widgets have visible set to FALSE. */
  if (GTK_IS_MENU (widget) && wdata)
      wdata->flags &= ~GLADE_VISIBLE;

  gtk_object_set_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY, wdata);
  gtk_signal_connect (GTK_OBJECT (widget), "destroy",
		      GTK_SIGNAL_FUNC (on_widget_destroy), NULL);

  editor_add_mouse_signals (widget);
  editor_add_draw_signals (widget);
  editor_add_key_signals (widget);

  if (GTK_IS_WINDOW (widget))
    gtk_window_add_accel_group (GTK_WINDOW (widget),
				glade_get_global_accel_group ());

#ifdef USE_GNOME
  /* FIXME: GnomeLibs 1.0.1 workaround - floating GnomeDockItem's don't work
     properly if we show them before adding to the GnomeDock. */
  if (!GTK_IS_WINDOW (widget) && !GTK_IS_MENU (widget)
      && !BONOBO_IS_DOCK_ITEM (widget))
    gtk_widget_show (widget);
#else
  if (!GTK_IS_WINDOW (widget) && !GTK_IS_MENU (widget))
    gtk_widget_show (widget);
#endif
}

/* This turns a normal widget into a GbWidget, adding a GladeWidgetData struct
   and the necessary signals. The widget must have its parent set already.
   If name is non-NULL, the widget's name will be set to the name with a
   unique ID added on to it, e.g. "ok_button1".
   NOTE: when loading, you should not create the names of any widgets,
   since it may clash with a widget loaded later. Instead leave the name as
   NULL. glade_project_ensure_widgets_named () will be called after the
   project is loaded, and any widgets without names will have names
   generated for them. */
void
gb_widget_create_from		(GtkWidget		*widget,
				 const gchar		*name)
{
  gb_widget_create_from_full (widget, name, NULL);
}


void
gb_widget_create_from_full	(GtkWidget		*widget,
				 const gchar		*name,
				 GladeWidgetData	*wdata)
{
  GbWidget *gbwidget;
  const char *class_id;

  MSG1 ("In create_from, widget name: %s", gtk_widget_get_name (widget));
  if (name)
    {
      char *wname = glade_project_new_widget_name (current_project, name);
      gtk_widget_set_name (widget, wname);
      g_free (wname);
    }

  if (GLADE_IS_CUSTOM_WIDGET (widget))
    class_id = "Custom";
  else
    class_id = gtk_type_name (GTK_OBJECT_TYPE (widget));

  gbwidget = (GbWidget *) g_hash_table_lookup (gb_widget_table, class_id);
  g_return_if_fail (gbwidget != NULL);

  if (!wdata)
    wdata = glade_widget_data_new (gbwidget);
  gb_widget_real_initialize (widget, wdata);

  gbwidget = gb_widget_lookup (widget);
  g_return_if_fail (gbwidget != NULL);

  /* Call any function the GbWidget has for setting up the widget to be used
     within Glade. */
  if (gbwidget->gb_widget_create_from_widget)
    {
      GbWidgetCreateFromData data;

      data.project = current_project;
      (gbwidget->gb_widget_create_from_widget) (widget, &data);
    }
}


void
gb_widget_initialize (GtkWidget * widget, GbWidgetNewData * data)
{
  if (data->name && data->name[0] != '\0')
    gtk_widget_set_name (widget, data->name);
  gb_widget_real_initialize (widget, data->widget_data);

  /* Now we set the widget's real style */
  /* FIXME: check if style should be propagated down from an ancestor? */
#if 0
  if (widget->style != data->widget_data->gbstyle->style)
    {
      gtk_widget_set_style (widget, data->widget_data->gbstyle->style);
    }
#endif

  /* FIXME: GTK workarounds to make sure that some widgets have reasonable
     sizes initially. Quite a few widgets default to a width and height of 0,
     which means that if there is little space available they will disappear,
     so we may need to do more here. */
  if (GTK_IS_ARROW (widget))
    gtk_widget_set_usize (widget, 16, 16);

  /* Set this to NULL so we don't try to free it later. */
  data->widget_data = NULL;
}

/* This returns TRUE if it is OK to complete the new() procedure, i.e. that
   the widget to replace or the parent widget still exist. It is used after
   the OK button is pressed in the dialog boxes for creating new tables/boxes.
   FIXME: I'm not too sure what we should do here. */
gboolean
gb_widget_can_finish_new (GbWidgetNewData * data)
{
  if (data->current_child)
    {
      if (data->current_child->parent == NULL)
	return FALSE;
    }
  else if (data->parent)
    {
      if (data->parent->parent == NULL && !GTK_IS_WINDOW (data->parent))
	return FALSE;
    }
  return TRUE;
}


void
gb_widget_free_new_data (GbWidgetNewData * data)
{
  g_free (data->name);
  g_free (data->widget_data);
  if (data->parent)
    gtk_widget_unref (data->parent);
  if (data->current_child)
    gtk_widget_unref (data->current_child);
  g_free (data);
}


static void
on_widget_destroy (GtkWidget * widget, gpointer user_data)
{
  GbWidget *gbwidget;
  GladeWidgetData *widget_data;
  GbWidgetDestroyData data;

  MSG1 ("IN on_widget_destroy widget:%s", gtk_widget_get_name (widget));

  /* Make sure we don't try to show its properties after it is destroyed. */
  if (property_get_widget () == widget)
    property_set_widget (NULL);

  /* If the entire project is being destroyed, we don't need to update the
     selection or the widget tree. */
  if (!(GTK_OBJECT_FLAGS (current_project) & GTK_IN_DESTRUCTION))
    {
      editor_remove_widget_from_selection (widget);
      tree_remove_widget (widget);
    }

  editor_on_widget_destroyed (widget);

  gbwidget = gb_widget_lookup (widget);
  g_return_if_fail (gbwidget != NULL);

  /* Call the GbWidget destroy function, if it has one. */
  data.project = current_project;
  if (gbwidget->gb_widget_destroy)
    (gbwidget->gb_widget_destroy) (widget, &data);

  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);

  /* Release the ID. */
  if (widget->name)
    glade_project_release_widget_name (current_project, widget->name);

  glade_widget_data_free (widget_data);

  g_free (gb_widget_get_child_name (widget));

  MSG1 ("OUT on_widget_destroy widget:%s", gtk_widget_get_name (widget));
}


/*************************************************************************
 * Functions for getting/setting the child name of a widget.
 * The child name is used to identify special widgets which have to
 * be treated differently. e.g. Dialog buttons.
 *************************************************************************/

static const gchar *glade_child_name_key  = "glade-child-name";
static GQuark       glade_child_name_key_id = 0;


/* Returns the child name of the widget. */
gchar*
gb_widget_get_child_name (GtkWidget *widget)
{
  if (!glade_child_name_key_id)
    glade_child_name_key_id = g_quark_from_static_string (glade_child_name_key);
  return gtk_object_get_data_by_id (GTK_OBJECT (widget),
				    glade_child_name_key_id);
}


/* Sets the child name of the widget. The child_name string is duplicated. */
void
gb_widget_set_child_name (GtkWidget *widget, const gchar *child_name)
{
  if (!glade_child_name_key_id)
    glade_child_name_key_id = g_quark_from_static_string (glade_child_name_key);
  /* Free any existing child name. */
  g_free (gtk_object_get_data_by_id (GTK_OBJECT (widget),
				     glade_child_name_key_id));
  gtk_object_set_data_by_id (GTK_OBJECT (widget), glade_child_name_key_id,
			     g_strdup (child_name));
}


/*************************************************************************
 * Functions for creating the page of properties specific to this widget
 *************************************************************************/

/* Returns the page number of the new page in the notebook which contains the
   widget's specific properties, or GB_PROPERTIES_NOT_NEEDED if it has none. */
gint
gb_widget_create_properties (GtkWidget * widget)
{
  GtkWidget *page;
  gint page_number;
  GbWidgetCreateArgData data;
  GbWidget *gbwidget;
  gboolean add_border_width = FALSE;

  gbwidget = gb_widget_lookup (widget);
  g_return_val_if_fail (gbwidget != NULL, GB_PROPERTIES_NOT_NEEDED);

  /* We always create the border width property, but hide it when it isn't
     needed. Some specific widgets don't need it, e.g. GtkDialog action areas.
  */
  /*if (glade_util_uses_border_width (widget))*/
  add_border_width = TRUE;

  if (gbwidget->gb_widget_create_properties || add_border_width)
    {
      /* Create skeleton of properties page, so gbwidget just has to add
         properties */
      page = gtk_table_new (1, 3, FALSE);
      gtk_table_set_row_spacings (GTK_TABLE (page), 1);
      gtk_widget_show (page);
      page_number = property_add_gbwidget_page (page);
      property_set_table_position (page, 0);

      /* If widget is a container add a border width property */
      if (add_border_width)
	{
	  gchar *class_id, buf[128];

	  class_id = gb_widget_get_class_id (widget);
	  sprintf (buf, "%s::border_width", class_id);
	  property_add_int_range (buf, _("Border Width:"),
			     _("The width of the border around the container"),
				  0, 1000, 1, 10, 1);
	}

      data.project = current_project;
      if (gbwidget->gb_widget_create_properties)
	(gbwidget->gb_widget_create_properties) (widget, &data);
      return page_number;
    }
  else
    {
      return GB_PROPERTIES_NOT_NEEDED;
    }
}


/*************************************************************************
 * Functions for creating the page of place properties specific to this
 * widget.
 *************************************************************************/

/* Returns the page number of the new page in the notebook which contains the
   widget's properties which applt to any children of the widget,
   or GB_PROPERTIES_NOT_NEEDED if no extra properties are needed for its
   children. */
gint
gb_widget_create_child_properties (GtkWidget * widget)
{
  static GHashTable *page_hash_table = NULL;

  GtkWidget *page;
  gint page_number;
  GbWidgetCreateChildArgData data;
  GbWidget *gbwidget;

  /* Create a hash table to contain functions already called to create child
     packing properties together with the page numbers they returned.
     This lets us use the same functions for multiple widgets, e.g. GtkHBox
     and GtkVBox, GtkHPaned and GtkVPaned. */
  if (page_hash_table == NULL)
    page_hash_table = g_hash_table_new (NULL, NULL);

  gbwidget = gb_widget_lookup (widget);
  g_return_val_if_fail (gbwidget != NULL, GB_PROPERTIES_NOT_NEEDED);

  if (gbwidget->gb_widget_create_child_properties)
    {
      /* First see if the function has already been called. Note that the
	 page numbers in the hash have 1 added to them so we can detect empty
	 values (they won't clash with page 0). */
      /* FIXME: ANSI forbids casting function pointer to data pointer. */
      page_number = GPOINTER_TO_INT (g_hash_table_lookup (page_hash_table, (gconstpointer) gbwidget->gb_widget_create_child_properties));
      if (page_number)
	return page_number - 1;

      /* Create skeleton of properties page, so gbwidget just has to add
         properties */
      page = gtk_table_new (10, 3, FALSE);
      gtk_widget_show (page);
      page_number = property_add_child_packing_page (page);
      /* FIXME: ANSI forbids casting function pointer to data pointer. */
      g_hash_table_insert (page_hash_table,
			   (gpointer) gbwidget->gb_widget_create_child_properties,
			   GINT_TO_POINTER (page_number + 1));
      property_set_table_position (page, 0);

      data.project = current_project;
      (gbwidget->gb_widget_create_child_properties) (widget, &data);
      return page_number;
    }
  else
    {
      return GB_PROPERTIES_NOT_NEEDED;
    }
}


/*************************************************************************
 * Functions for showing the widget's properties
 *************************************************************************/

void
gb_widget_show_properties (GtkWidget * widget)
{
  GbWidgetGetArgData data;
  GbWidget *gbwidget, *parent_gbwidget = NULL;
  GladeWidgetData *widget_data;
  gint page, child_packing_page;

  /* If properties of widget are already shown, just return */
  if (property_get_widget () == widget)
    return;

  /* If widget is a placeholder reset the properties window and return */
  if (GB_IS_PLACEHOLDER (widget))
    {
      property_set_widget (NULL);
      return;
    }

  gbwidget = gb_widget_lookup (widget);
  g_return_if_fail (gbwidget != NULL);

  /* Turn off auto-apply so we can set properties without the 'changed'
     callbacks calling gb_widget_apply_properties (). */
  property_set_auto_apply (FALSE);

  /* Need this here to make sure properties notebook is sensitive */
  property_set_widget (widget);

  page = gbwidget->properties_page_number;
  /* If widget's properties page hasn't been created, create it now */
  if (page == GB_PROPERTIES_NOT_CREATED)
    {
      page = gb_widget_create_properties (widget);
      gbwidget->properties_page_number = page;
    }

  /* Show the widget's own properties page if it has one.
     Need to show the page before setting properties because of the
     Text widget - it must be realized before setting the text :-( */
  if (page == GB_PROPERTIES_NOT_NEEDED)
    property_hide_gbwidget_page ();
  else
    property_show_gbwidget_page (page);

  /* Now see if the parent has child packing properties that need to be
     created or shown. */
  if (widget->parent)
    {
      parent_gbwidget = gb_widget_lookup (widget->parent);

      /* parent_gbwidget may be NULL, e.g. for GnomeDockItems. */
      if (parent_gbwidget)
	{
	  child_packing_page = parent_gbwidget->child_properties_page_number;
	  /* If widget's properties page hasn't been created, create it now */
	  if (child_packing_page == GB_PROPERTIES_NOT_CREATED)
	    {
	      child_packing_page = gb_widget_create_child_properties (widget->parent);
	      parent_gbwidget->child_properties_page_number = child_packing_page;
	    }
      
	  if (child_packing_page == GB_PROPERTIES_NOT_NEEDED)
	    property_hide_child_packing_page ();
	  else
	    property_show_child_packing_page (child_packing_page);
	}
    }
  else
    {
      property_hide_child_packing_page ();
    }

  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (widget_data != NULL);
  data.project = current_project;
  data.action = GB_SHOWING;
  data.widget_data = widget_data;
  data.widget = widget;

  get_standard_properties (widget, &data);

  if (gbwidget->gb_widget_get_properties)
    (gbwidget->gb_widget_get_properties) (widget, &data);

  if (parent_gbwidget && parent_gbwidget->gb_widget_get_child_properties)
    (parent_gbwidget->gb_widget_get_child_properties) (widget->parent, widget,
						       &data);

  /* Turn auto-apply back on again */
  property_set_auto_apply (TRUE);
}


/* This is called when the widget's size or position has changed, so that we
   should update the values shown in the properties editor. */
void
gb_widget_show_position_properties (GtkWidget * widget)
{
  GbWidgetGetArgData data;
  GladeWidgetData *widget_data;

  /* Make sure this is the widget shown in the properties editor. */
  if (property_get_widget () != widget)
    return;

  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (widget_data != NULL);
  data.project = current_project;
  data.action = GB_SHOWING;
  data.widget_data = widget_data;

  property_set_auto_apply (FALSE);
  get_position_properties (widget, &data);
  property_set_auto_apply (TRUE);
}



#ifdef GLADE_STYLE_SUPPORT
void
gb_widget_show_style (GtkWidget * widget)
{
  GladeWidgetData *wdata;
  GbStyle *gbstyle;
  GtkStyle *style = widget->style;
  gchar buffer[128];
  gint i;

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (wdata != NULL);
  gbstyle = wdata->gbstyle;

  property_set_bool (GbStylePropagate, wdata->flags & GLADE_STYLE_PROPAGATE);

  property_set_dialog (GbStyleName, wdata->flags & GLADE_STYLE_IS_UNNAMED ?
		       "" : gbstyle->name, NULL);
  property_set_font (GbStyleFont, style->font, gbstyle->xlfd_fontname);

  /* Colors */
  show_color_properties (style->fg, "fg");
  show_color_properties (style->bg, "bg");
  show_color_properties (style->text, "text");
  show_color_properties (style->base, "base");

  /* Background pixmaps */
  for (i = 0; i < GB_NUM_STYLE_STATES; i++)
    {
      sprintf (buffer, "GtkStyle::%s[%s]", GbBgPixmapName, GbStateNames[i]);
      property_set_bgpixmap (buffer, style->bg_pixmap[i],
			     gbstyle->bg_pixmap_filenames[i]);
    }
}


static void
show_color_properties (GdkColor colors[], gchar * name)
{
  gint state;
  gchar buf[128];

  for (state = 0; state < GB_NUM_STYLE_STATES; state++)
    {
      sprintf (buf, "GtkStyle::%s[%s]", name, GbStateNames[state]);
      property_set_color (buf, &colors[state]);
    }
}
#endif


static void
show_accelerators (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GList *element = data->widget_data->accelerators;
  property_clear_accelerators ();
  while (element)
    {
      property_add_accelerator ((GladeAccelerator *) element->data);
      element = element->next;
    }
}


static void
show_signals (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GList *element = data->widget_data->signals;
  property_clear_signals ();
  MSG1 ("Num signals: %i", g_list_length (element));
  while (element)
    {
      property_add_signal ((GladeSignal *) element->data);
      element = element->next;
    }
}



/*************************************************************************
 * Functions for applying properties to a widget
 *************************************************************************/

void
gb_widget_apply_properties (GtkWidget * widget, GtkWidget * property)
{
  GbWidgetSetArgData data;
  GbWidget *gbwidget;
  GladeWidgetData *widget_data;

  MSG1 ("Applying properties: %s", gtk_widget_get_name (widget));

  gbwidget = gb_widget_lookup (widget);
  g_return_if_fail (gbwidget != NULL);

  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  g_return_if_fail (widget_data != NULL);

  data.project = current_project;
  data.action = GB_APPLYING;
  data.widget_data = widget_data;
  data.widget = widget;
  data.property_to_apply = property;

  set_standard_properties (widget, &data);

  MSG ("Calling widget's own apply_properties");
  if (gbwidget->gb_widget_set_properties)
    (gbwidget->gb_widget_set_properties) (widget, &data);
  MSG ("Called widget's own apply_properties");
}


/* Copies the signals from the GladeWidgetInfo into a list of GladeSignal
   structs. */
static GList*
copy_signals (GbWidgetSetArgData * data, GladeWidgetInfo *widget_info)
{
  GList *signals = NULL;
  GladeSignalInfo *signal_info;
  gint i;

  signal_info = widget_info->signals;
  for (i = 0; i < widget_info->n_signals; i++)
    {
      GladeSignal *signal = g_new0 (GladeSignal, 1);

      signal->name = g_strdup (signal_info[i].name);
      signal->handler = g_strdup (signal_info[i].handler);
      signal->object = g_strdup (signal_info[i].object);
      signal->after = signal_info[i].after ? TRUE : FALSE;
      signal->data = NULL; /* Not supported anymore. */
      signal->last_modification_time = load_parse_date (data, signal_info[i].last_modification_time);

      if (data->status == GLADE_STATUS_INVALID_VALUE)
	{
	  g_warning ("Invalid date value: %s", signal_info[i].last_modification_time);
	  data->status = GLADE_STATUS_OK;
	}

      signals = g_list_prepend (signals, signal);
    }

  /* Reverse the list so it stays in the original order. */
  return g_list_reverse (signals);
}


/* Copies the accelerators from the GladeWidgetInfo into a list of
   GladeAccelerator structs. */
static GList*
copy_accels (GladeWidgetInfo *widget_info)
{
  GList *accels = NULL;
  GladeAccelInfo *accel_info;
  gint i;

  accel_info = widget_info->accels;
  for (i = 0; i < widget_info->n_accels; i++)
    {
      GladeAccelerator *accel = g_new0 (GladeAccelerator, 1);

      accel->key = g_strdup (gdk_keyval_name (accel_info[i].key));
      accel->modifiers = accel_info[i].modifiers;
      accel->signal = g_strdup (accel_info[i].signal);

      accels = g_list_prepend (accels, accel);
    }

  /* Reverse the list so it stays in the original order. */
  return g_list_reverse (accels);
}


static void
set_standard_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GladeWidgetData *wdata = data->widget_data;
  gchar *name, *tooltip, *events_str, *ext_events;
  gboolean visible, sensitive, can_default, has_default, can_focus, has_focus;
  gint events, i;

  /* Properties on widget page */
  /* When pasting, we may want to discard the names from the XML, and generate
     a new name instead. */
  if (data->action == GB_LOADING)
    {
      if  (data->xml_buffer && data->discard_names)
	{
	  gchar *class_id = gb_widget_get_class_id (widget);
	  name = glade_project_new_widget_name (data->project, class_id);
	  gtk_widget_set_name (widget, name);
	  g_free (name);
	}
      else
	{
	  gtk_widget_set_name (widget, data->widget_info->name);
	}

      if (glade_util_is_component (widget))
	glade_project_component_changed (data->project, widget);

      /* We have to check if the name has a trailing ID and if so we reserve
	 it so no other widget can use it. */
      glade_project_reserve_name (data->project, gtk_widget_get_name (widget));
    }
  else
    {
      name = gb_widget_input_string (data, GbName);
      if (data->apply)
	{
	  tree_rename_widget (widget, name);
	  gtk_widget_set_name (widget, name);
	  property_update_title ();

	  /* If widget is a toplevel window/dialog set the component's name in
	     the project window */
	  if (glade_util_is_component (widget))
	    glade_project_component_changed (data->project, widget);
	}
    }

  /* If widget is a container, show the border width */
  if (glade_util_uses_border_width (widget))
    {
      gchar buf[128];
      gint border_width;
      gchar *class_id;

      class_id = gb_widget_get_class_id (widget);
      sprintf (buf, "%s::border_width", class_id);
      border_width = gb_widget_input_int (data, buf);
      if (data->apply && GTK_CONTAINER (widget)->border_width != border_width)
	{
	  gtk_container_set_border_width (GTK_CONTAINER (widget),
					  border_width);
	  if (data->action == GB_APPLYING)
	    editor_refresh_widget (widget);
	}
    }

  /* Language-specific properties. */
  set_lang_specific_properties (widget, data);

  /* Special child properties page */
  set_special_child_properties (widget, data);

  /* Properties on standard page */
  set_position_properties (widget, data);

  /* Visible property. Note that we create widgets with visible set to TRUE
     by default, but the property is FALSE by default in the XML file. So
     when loading we make sure we set the flag to the appropriate value. */
  visible = gb_widget_input_bool (data, GbVisible);
  if (data->apply || data->action == GB_LOADING)
    {
      if (!data->apply)
	visible = FALSE;
      if (visible)
	wdata->flags |= GLADE_VISIBLE;
      else
	wdata->flags &= ~GLADE_VISIBLE;
    }

  sensitive = gb_widget_input_bool (data, GbSensitive);
  if (data->apply)
    {
      if (sensitive)
	wdata->flags |= GLADE_SENSITIVE;
      else
	wdata->flags &= ~GLADE_SENSITIVE;
    }

  tooltip = gb_widget_input_string (data, GbTooltip);
  if (data->apply)
    {
      g_free (wdata->tooltip);
      if (tooltip && tooltip[0] == '\0')
	tooltip = NULL;
      wdata->tooltip = g_strdup (tooltip);

      /* SPECIAL CODE: toolitems have a special function. */
      if (GTK_IS_TOOL_ITEM (widget))
	gtk_tool_item_set_tooltip (GTK_TOOL_ITEM (widget),
				   gb_widget_tooltips, tooltip, NULL);
      else
	gtk_tooltips_set_tip (gb_widget_tooltips, widget, tooltip, NULL);
    }

  can_default = gb_widget_input_bool (data, GbCanDefault);
  if (data->apply)
    {
      if (can_default)
	GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_DEFAULT);
      else
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_DEFAULT);
      if (data->action == GB_APPLYING)
	editor_refresh_widget (widget);
    }

  has_default = gb_widget_input_bool (data, GbHasDefault);
  if (data->apply)
    {
      if (has_default)
	wdata->flags |= GLADE_GRAB_DEFAULT;
      else
	wdata->flags &= ~GLADE_GRAB_DEFAULT;
    }

  /* Different widgets have different default values for CAN_FOCUS, so when
     we load an XML file we must make sure that we always set it or unset it.
     Also, since we don't save the can_focus flag if it is false, we must make
     sure that we apply it anyway when loading. */
  can_focus = gb_widget_input_bool (data, GbCanFocus);
  if (!data->apply)
    can_focus = FALSE;
  if (data->apply || data->action == GB_LOADING)
    {
      if (can_focus)
	GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
      else
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_CAN_FOCUS);
    }
  has_focus = gb_widget_input_bool (data, GbHasFocus);
  if (data->apply)
    {
      if (has_focus)
	wdata->flags |= GLADE_GRAB_FOCUS;
      else
	wdata->flags &= ~GLADE_GRAB_FOCUS;
    }

  /* Events & ext events. */
  if (!GTK_WIDGET_NO_WINDOW (widget))
    {
      if (data->action == GB_APPLYING)
	{
	  events = property_events_string_to_value (gb_widget_input_string (data,
								 GbEvents));
	  if (data->apply)
	    wdata->events = events;
	}
      else
	{
	  events_str = gb_widget_input_string (data, GbEvents);
	  if (data->apply)
	    {
	      for (i = 0; i < GB_EVENT_MASKS_COUNT; i++)
		{
		  if (glade_util_strstr (events_str, GbEventMaskSymbols[i]))
		    wdata->events |= GbEventMaskValues[i];
		}
	    }
	}

      ext_events = gb_widget_input_choice (data, GbExtEvents);
      if (data->apply)
	{
	  for (i = 0; GbExtensionModeChoices[i]; i++)
	    {
	      if (!strcmp (ext_events, GbExtensionModeChoices[i])
		  || !strcmp (ext_events, GbExtensionModeSymbols[i]))
		{
		  gtk_widget_set_extension_events (widget, GbExtensionModeValues
						   [i]);
		  break;
		}
	    }
	}
    }

  if (data->action == GB_APPLYING)
    {
#if 0
      apply_style (widget, data);
#endif
      apply_accelerators (widget, data);
      apply_signals (widget, data);

      glade_atk_set_properties (widget, data);
    }
  else
    {
      data->widget_data->signals = copy_signals (data, data->widget_info);
      data->widget_data->accelerators = copy_accels (data->widget_info);

      /* ATK properties are loaded later, after all widgets are created. */
    }
}


static void
set_special_child_properties (GtkWidget * widget,
			      GbWidgetSetArgData * data)
{
  GtkWidget *parent = widget->parent;
  GbWidget *gbparent;

  if (!parent)
    return;

  /* When pasting a widget to replace an existing widget, the child properties
     will already have been set, so we just return. */
  if (data->action == GB_LOADING && data->xml_buffer && data->replacing_widget)
    return;

  gbparent = gb_widget_lookup (parent);
  if (!gbparent)
    {
      MSG1 ("Unknown parent type %s", G_OBJECT_TYPE_NAME (parent));
    }

  /* Tell the load functions to use the child properties array. */
  data->loading_type = GB_CHILD_PROPERTIES;

  if (gbparent && gbparent->gb_widget_set_child_properties)
    {
      (gbparent->gb_widget_set_child_properties) (parent, widget, data);
    }

  data->loading_type = GB_STANDARD_PROPERTIES;
}


static void
set_position_properties (GtkWidget * widget,
			 GbWidgetSetArgData * data)
{
  GladeWidgetData *wdata = data->widget_data;
  gint w, h;
  gboolean applyWidth, applyHeight;
  gboolean set_usize = FALSE;

  w = gb_widget_input_int (data, GbWidth);
  applyWidth = data->apply;
  h = gb_widget_input_int (data, GbHeight);
  applyHeight = data->apply;

  /* When loading we need to remember which values have been set explicitly. */
  if (data->action == GB_LOADING)
    {
      if (applyWidth)
	wdata->flags |= GLADE_WIDTH_SET;
      if (applyHeight)
	wdata->flags |= GLADE_HEIGHT_SET;
    }

#if 0
  g_print ("In set_position_properties X:%i Y:%i W:%i H:%i\n", x, y, w, h);
#endif
  if (GTK_IS_WINDOW (widget))
    {
      if (applyWidth && wdata->width != w)
	{
	  wdata->width = w;
	  set_usize = TRUE;
	}
      if (applyHeight && wdata->height != h)
	{
	  wdata->height = h;
	  set_usize = TRUE;
	}

      if (set_usize)
	{
	  gint w = wdata->flags & GLADE_WIDTH_SET ? wdata->width : -1;
	  gint h = wdata->flags & GLADE_HEIGHT_SET ? wdata->height : -1;
	  gb_widget_set_usize (widget, w, h);
	}
    }
  else if (widget->parent && (GTK_IS_FIXED (widget->parent)
			      || GTK_IS_LAYOUT (widget->parent)))
    {
      /* When pasting a widget to replace an existing widget, the size &
	 position will be set in the replace_child function. */
      if (data->action == GB_LOADING && data->xml_buffer
	  && data->replacing_widget)
	{
	  return;
	}


      if (applyWidth && wdata->width != w)
	{
	  wdata->width = w;
	  set_usize = TRUE;
	}
      if (applyHeight && wdata->height != h)
	{
	  wdata->height = h;
	  set_usize = TRUE;
	}
      if (set_usize)
	gb_widget_set_usize (widget, wdata->width, wdata->height);
    }
  else
    {
      if (applyWidth && wdata->width != w)
	{
	  wdata->width = w;
	  set_usize = TRUE;
	}
      if (applyHeight && wdata->height != h)
	{
	  wdata->height = h;
	  set_usize = TRUE;
	}

      if (set_usize)
	{
	  gint w = wdata->flags & GLADE_WIDTH_SET ? wdata->width : -1;
	  gint h = wdata->flags & GLADE_HEIGHT_SET ? wdata->height : -1;
	  gb_widget_set_usize (widget, w, h);
	}
      MSG2 ("*** Width set:%i Height set:%i", wdata->flags & GLADE_WIDTH_SET,
	    wdata->flags & GLADE_HEIGHT_SET);
    }
}


#if 0
static void
apply_style (GtkWidget * widget,
	     GbWidgetSetArgData * data)
{
  GladeWidgetData *wdata = data->widget_data;
  GtkStyle *style = widget->style, *old_style = NULL;
  GbStyle *gbstyle = wdata->gbstyle, *new_gbstyle;
  GdkFont *font = NULL;
  gchar *style_name, *xlfd_fontname;
  GdkColor fg[GB_NUM_STYLE_STATES];
  GdkColor bg[GB_NUM_STYLE_STATES];
  GdkColor text[GB_NUM_STYLE_STATES];
  GdkColor base[GB_NUM_STYLE_STATES];
  GdkPixmap *bg_pixmap[GB_NUM_STYLE_STATES];
  gchar *bg_pixmap_filenames[GB_NUM_STYLE_STATES];
  gint recreate = FALSE, redraw = FALSE, i;
  gchar buf[128], *filename;
  gboolean named_style;

  style_name = gb_widget_input_dialog (data, GbStyleName);
  named_style = (style_name[0] == '\0') ? FALSE : TRUE;
  if (data->apply)
    {
      if (named_style)
	{
	  new_gbstyle = (GbStyle *) g_hash_table_lookup (gb_style_hash, style_name);
	  g_return_if_fail (new_gbstyle != NULL);
	  if (new_gbstyle != gbstyle)
	    {
	      gbstyle = new_gbstyle;
	      gb_widget_set_gb_style (widget, gbstyle);
	      wdata->flags &= ~GLADE_STYLE_IS_UNNAMED;
	      redraw = TRUE;
	    }
	}
      else
	{
	  wdata->flags |= GLADE_STYLE_IS_UNNAMED;
	}
    }

  font = gb_widget_input_font (data, GbStyleFont, &xlfd_fontname);
  if (data->apply)
    {
      if (font != style->font)
	recreate = TRUE;
    }

  recreate |= apply_colors (widget, data, style->fg, fg, "fg");
  recreate |= apply_colors (widget, data, style->bg, bg, "bg");
  recreate |= apply_colors (widget, data, style->text, text, "text");
  recreate |= apply_colors (widget, data, style->base, base, "base");

  /* Background pixmaps */
  for (i = 0; i < GB_NUM_STYLE_STATES; i++)
    {
      sprintf (buf, "GtkStyle::%s[%s]", GbBgPixmapName, GbStateNames[i]);
      bg_pixmap[i] = gb_widget_input_bgpixmap (data, buf, &filename);
      bg_pixmap_filenames[i] = filename;
      if (data->apply)
	{
	  if (bg_pixmap[i] != style->bg_pixmap[i])
	    recreate = TRUE;
	}
    }

  if (recreate)
    {
      old_style = style;

      /* If the widget is supposedly using an unnamed GbStyle, but currently is
         actually using a named GbStyle (for convenience), then we need to
         create a copy of the GbStyle and place our new style in it. */
      if ((wdata->flags & GLADE_STYLE_IS_UNNAMED) && gbstyle->name)
	{
	  gbstyle = gb_widget_copy_gb_style (gbstyle);
	  g_free (gbstyle->name);
	  gbstyle->name = NULL;
	}

      style = gtk_style_new ();
      for (i = 0; i < GB_NUM_STYLE_STATES; i++)
	{
	  style->fg[i] = fg[i];
	  style->bg[i] = bg[i];
	  style->text[i] = text[i];
	  style->base[i] = base[i];
	  style->bg_pixmap[i] = bg_pixmap[i];
	  if (bg_pixmap[i])
	    gdk_pixmap_ref (bg_pixmap[i]);

	  if (gbstyle->bg_pixmap_filenames[i] != bg_pixmap_filenames[i])
	    {
	      g_free (gbstyle->bg_pixmap_filenames[i]);
	      gbstyle->bg_pixmap_filenames[i] = g_strdup (bg_pixmap_filenames
							  [i]);
	    }
	}
      if (font)
	{
	  gdk_font_unref (style->font);
	  style->font = font;
	  gdk_font_ref (style->font);
	}
      if (strcmp (gbstyle->xlfd_fontname, xlfd_fontname))
	{
	  g_free (gbstyle->xlfd_fontname);
	  gbstyle->xlfd_fontname = g_strdup (xlfd_fontname);
	}

      gbstyle->style = style;
      gtk_style_ref (style);
      gb_widget_set_gb_style (widget, gbstyle);
    }


  /* If a named style has been changed/recreated we have to update all
     widget's that use it. */
  if (recreate || redraw)
    {
      if (named_style)
	{
	  gb_widget_update_gb_styles (gbstyle, gbstyle);
	}
      else
	{
	  editor_refresh_widget (widget);
	}
    }

  if (old_style)
    gtk_style_unref (old_style);
}


/* This makes sure a widget's gbstyle & its style are up to date, and
   if the propagate flag is set it also updates any children.
   But it won't change a descendant's style if it has been set explicitly.
   FIXME: only propagates one level at present, and always sets child's style,
   even if it has a different GbStyle! */
void
gb_widget_set_gb_style (GtkWidget * widget,
			GbStyle * gbstyle)
{
  GladeWidgetData *wdata;

  if (!GB_IS_PLACEHOLDER (widget))
    {
      if (widget->style != gbstyle->style)
	gtk_widget_set_style (widget, gbstyle->style);
    }

  wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
  if (wdata)
    {
      if (wdata->gbstyle != gbstyle)
	{
	  gb_widget_unref_gb_style (wdata->gbstyle);
	  wdata->gbstyle = gbstyle;
	  gb_widget_ref_gb_style (gbstyle);
	}
      /* If propagate style flags is set, propagate style to children */
      if (wdata->flags & GLADE_STYLE_PROPAGATE)
	gb_widget_children_foreach (widget, (GtkCallback) gb_widget_set_gb_style,
			   gbstyle);
    }
}


static gboolean
apply_colors (GtkWidget * widget, GbWidgetSetArgData * data, GdkColor colors[],
	      GdkColor new_colors[], gchar * name)
{
  gint state;
  gchar buf[128];
  GdkColor *color;
  gboolean need_redraw = FALSE;

  for (state = 0; state < GB_NUM_STYLE_STATES; state++)
    {
      sprintf (buf, "GtkStyle::%s[%s]", name, GbStateNames[state]);

      color = gb_widget_input_color (data, buf);
      if (data->apply)
	{
	  new_colors[state] = *color;
	  if (!gdk_color_equal (&new_colors[state], &colors[state]))
	    need_redraw = TRUE;
	}
      else
      {
	/* Copy the old values across. */
	new_colors[state] = colors[state];
      }
    }
  return need_redraw;
}
#endif


/* Currently this frees all the GladeAccelerators and creates them from scratch */
static void
apply_accelerators (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GladeWidgetData *wdata;

  if (data->property_to_apply == NULL
      || property_is_accel_clist (data->property_to_apply))
    {
      wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
      g_return_if_fail (wdata != NULL);

      glade_widget_data_set_accels (wdata, property_get_accelerators ());
    }
}


/* Currently this frees all the GladeSignals and creates them from scratch. */
static void
apply_signals (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GladeWidgetData *wdata;

  if (data->property_to_apply == NULL
      || property_is_signal_clist (data->property_to_apply))
    {
      wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
      g_return_if_fail (wdata != NULL);

      glade_widget_data_set_signals (wdata, property_get_signals ());
    }
}



/*************************************************************************
 * Functions for showing the popup context-sensitive menu of a widget
 *************************************************************************/

static void
popup_done (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (widget);
}

void
gb_widget_show_popup_menu (GtkWidget * widget,
			   GdkEventButton * event)
{
  GtkWidget *menu = NULL;
  GbWidget *gbwidget;
  const gchar *name;
  GtkWidget *menuitem, *ancestor, *submenu, *child;
  GbWidgetCreateMenuData data;

  gbwidget = gb_widget_lookup (widget);
  g_return_if_fail (gbwidget != NULL);

  name = gtk_widget_get_name (widget);
  if (GB_IS_PLACEHOLDER (widget))
    name = "Placeholder";

  menu = gtk_menu_new ();
  menuitem = gtk_menu_item_new_with_label (name);
  gtk_widget_show (menuitem);
  gtk_widget_set_sensitive (menuitem, FALSE);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);

  data.project = current_project;
  data.menu = menu;
  data.child = NULL;
  add_standard_top_menu_items (widget, &data);

  if (gbwidget->gb_widget_create_popup_menu)
    (gbwidget->gb_widget_create_popup_menu) (widget, &data);

  add_standard_bottom_menu_items (widget, &data);

  child = widget;
  ancestor = widget->parent;
  while (ancestor)
    {
      name = gtk_widget_get_name (ancestor);
      if (GB_IS_PLACEHOLDER (ancestor))
	name = "Placeholder";

      /* Skip widgets which aren't GbWidgets */
      if (GB_IS_GB_WIDGET (ancestor))
	{
	  /* Add a separator */
	  menuitem = gtk_menu_item_new ();
	  gtk_container_add (GTK_CONTAINER (menu), menuitem);
	  gtk_widget_show (menuitem);

	  menuitem = gtk_menu_item_new_with_label (name);
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (menu), menuitem);

	  /* Create submenu */
	  submenu = gtk_menu_new ();
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);

	  data.menu = submenu;
	  data.child = child;
	  add_standard_top_menu_items (ancestor, &data);

	  /* Call ancestors function to add any menu items */
	  gbwidget = gb_widget_lookup (ancestor);
	  if (gbwidget != NULL && gbwidget->gb_widget_create_popup_menu)
	    (gbwidget->gb_widget_create_popup_menu) (ancestor, &data);

	  add_standard_bottom_menu_items (ancestor, &data);
	}
      child = ancestor;
      ancestor = ancestor->parent;
    }

  /* Automatically destroy the menu when it is hidden. */
  gtk_signal_connect_after (GTK_OBJECT (menu), "selection-done",
			    GTK_SIGNAL_FUNC (popup_done), menu);

  MSG ("showing popup menu");
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
		  event->button, event->time);
}


static void
add_standard_top_menu_items (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (_("Select"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (editor_on_select_activate), widget);
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
}


static void
add_standard_bottom_menu_items (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;
  gboolean can_delete;

  can_delete = (editor_can_delete_widget (widget) == NULL) ? TRUE : FALSE;

  /* For widgets which can handle scrolling, we add commands to add or remove
     a parent scrolled window. */
  if (GTK_WIDGET_CLASS (G_OBJECT_GET_CLASS (widget))->set_scroll_adjustments_signal)
    {
      if (widget->parent && GTK_IS_SCROLLED_WINDOW (widget->parent))
	{
	  menuitem = gtk_menu_item_new_with_label (_("Remove Scrolled Window"));
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      GTK_SIGNAL_FUNC (gb_widget_remove_scrolled_window),
			      widget);
	}
      else
	{
	  menuitem = gtk_menu_item_new_with_label (_("Add Scrolled Window"));
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      GTK_SIGNAL_FUNC (gb_widget_add_scrolled_window),
			      widget);
	}
    }

  /* We assume that we can only insert/remove alignments and event boxes if
     the widget can be deleted. */
  if (can_delete
      && !GTK_IS_WINDOW (widget)
      && !GTK_IS_MISC (widget)
      && !GTK_IS_ALIGNMENT (widget)
      && !GTK_IS_MENU (widget)
      && !GTK_IS_MENU_ITEM (widget)
      && !GB_IS_PLACEHOLDER (widget))
    {
      if (GTK_IS_ALIGNMENT (widget->parent))
	{
	  menuitem = gtk_menu_item_new_with_label (_("Remove Alignment"));
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_widget_remove_alignment), widget);
	}
      else
	{
	  menuitem = gtk_menu_item_new_with_label (_("Add Alignment"));
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			 GTK_SIGNAL_FUNC (gb_widget_add_alignment), widget);
	}
    }

  if (can_delete
      && GTK_WIDGET_NO_WINDOW (widget)
      && !GTK_IS_EVENT_BOX (widget)
      && !GB_IS_PLACEHOLDER (widget))
    {
      if (GTK_IS_EVENT_BOX (widget->parent))
	{
	  menuitem = gtk_menu_item_new_with_label (_("Remove Event Box"));
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_widget_remove_event_box), widget);
	}
      else
	{
	  menuitem = gtk_menu_item_new_with_label (_("Add Event Box"));
	  gtk_widget_show (menuitem);
	  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			 GTK_SIGNAL_FUNC (gb_widget_add_event_box), widget);
	}
    }

  if (GTK_IS_WINDOW (widget))
    {
      menuitem = gtk_menu_item_new_with_label (_("Redisplay"));
      gtk_widget_show (menuitem);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_widget_redisplay_window),
			  widget);
    }

  menuitem = gtk_separator_menu_item_new ();
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);

  /* Only show the 'Cut' item if the widget can be deleted. */
  if (can_delete)
    {
      menuitem = gtk_menu_item_new_with_label (_("Cut"));
      gtk_widget_show (menuitem);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (editor_on_cut_activate), widget);
    }
 
  menuitem = gtk_menu_item_new_with_label (_("Copy"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (editor_on_copy_activate), widget);
 
  /* Only show the 'Paste' item if the widget can be deleted. */
  if (can_delete)
    {
      menuitem = gtk_menu_item_new_with_label (_("Paste"));
      gtk_widget_show (menuitem);
      if (!widget->parent)
	gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (editor_on_paste_activate), widget);
    }
 
  /* Only show the 'Delete' item if the widget can be deleted. */
  if (can_delete)
    {
      menuitem = gtk_menu_item_new_with_label (_("Delete"));
      gtk_widget_show (menuitem);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (editor_on_delete_activate), widget);
    }
}



/*************************************************************************
 * Functions for loading project files
 *************************************************************************/

/* This reads in a widget, and calls itself recursively to read it's children.
   It assumes a '<widget>' has just been read, and reads up to and including
   '</widget>'. The newly-created widget is returned, mainly so that
   glade_clipboard_paste () can show any windows which are pasted. */
GtkWidget*
gb_widget_load (GtkWidget * widget, GbWidgetSetArgData * data, GtkWidget * parent)
{
  gchar *class_id;
  const gchar *child_name;
  GbWidget *gbwidget, *ancestor_gbwidget;
  GtkWidget *ancestor;
  GladeWidgetData *wdata;
  GladeChildInfo *child_info;
  GladeWidgetInfo *widget_info;
  gint i;
  gboolean skip_child;

  data->action = GB_LOADING;
  data->loading_type = GB_STANDARD_PROPERTIES;

  child_info = data->child_info;
  widget_info = data->widget_info;

  class_id = widget_info ? widget_info->class : NULL;
  MSG1 ("Reading Class: %s", class_id ? class_id : "(placeholder)");

  child_name = child_info ? child_info->internal_child : NULL;

  /* Migrate toolbar buttons from old XML files. See the set_properties
     functions in gbtoolbutton.c etc. for the code that reads in the old
     properties. */
  if (class_id)
    {
      if (!strcmp (class_id, "button"))
	{
	  class_id = "GtkToolButton";
	}
      else if (!strcmp (class_id, "toggle"))
	{
	  class_id = "GtkToggleToolButton";
	}
      else if (!strcmp (class_id, "radio"))
	{
	  class_id = "GtkRadioToolButton";
	}
    }

  /* SPECIAL CODE: CList/CTree title buttons don't save the child name now, so
     we have to add it. */
  if (parent && GTK_IS_CLIST (parent))
    child_name = GladeChildCListTitle;

  /* SPECIAL CODE: Ignore the old 'BonoboDock:contents' child name. */
  if (child_name && !strcmp (child_name, "BonoboDock:contents"))
    child_name = NULL;

  /* SPECIAL CODE: when pasting a widget to replace an existing widget, check
     if the child name should be transferred to the new widget. */
  if (data->xml_buffer && data->replacing_widget && child_name)
    {
      /* These child names should be removed from the widget when it is pasted,
	 since they identify the old position the widget. */
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
	  || !strcmp (child_name, GladeChildGnomeAppDock)
	  || !strcmp (child_name, GladeChildGnomeAppBar)
	  || !strcmp (child_name, GladeChildGnomeEntry)
	  )
	{
	  child_name = NULL;
	}
    }

  if (!class_id)
    {
      MSG ("found placeholder");
      widget = editor_new_placeholder ();
      if (!gb_widget_add_child (parent, data, widget))
	{
	  data->status = GLADE_STATUS_ERROR;
	  return NULL;
	}
      if (child_name)
	gb_widget_set_child_name (widget, child_name);
      return NULL;
    }

  gbwidget = gb_widget_lookup_class (class_id);
  if (gbwidget == NULL)
    {
      MSG ("Load error");
      data->status = GLADE_STATUS_CLASS_UNKNOWN;
      g_warning ("Unknown widget class: %s", class_id);
      return NULL;
    }

  /* If this is a special child of a widget, step through the ancestors of
     the widget and try to find it. Note that some special child widgets
     have to be created normally, and then added by the parent container,
     e.g. notebook tabs and clist titles - see gb_widget_add_child(). */
  if (child_name)
    {
      MSG1 ("Child name: %s", child_name);
      ancestor = parent;
      while (ancestor)
	{
	  ancestor_gbwidget = gb_widget_lookup (ancestor);
	  if (ancestor_gbwidget && ancestor_gbwidget->gb_widget_get_child)
	    {
	      widget = (ancestor_gbwidget->gb_widget_get_child) (ancestor,
								 child_name);
	      if (widget)
		break;
	    }

	  ancestor = ancestor->parent;
	}

#if 0
      if (!widget)
	g_print ("Child widget %s not found - may be a problem\n", child_name);
#endif
    }

  /* If this is a standard widget, we need to create it and add it to its
     parent. If the widget has already been created by its parent, we can
     just set the properties. */
  if (widget == NULL)
    {
      MSG ("widget == NULL, has not been created by parent.");
      widget = gb_widget_new_full (class_id, FALSE, parent, NULL,
				   0, 0, NULL, GB_LOADING, data);
      if (!widget)
	{
	  data->status = GLADE_STATUS_ERROR;
	  return NULL;
	}

      wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
      data->widget_data = wdata;
      data->widget = widget;

      /* We get the gbwidget from the widget data, as it may be different from
	 the original one. Currently only Bonobo controls do this. */
      gbwidget = wdata->gbwidget;
      g_assert (gbwidget);

      if (parent == NULL)
	{
	  glade_project_add_component (data->project, widget);
	}
      else
	{
	  if (!gb_widget_add_child (parent, data, widget))
	    {
	      data->status = GLADE_STATUS_ERROR;
	      return NULL;
	    }
	}

      /* This must come after add_child so we can set special child properties,
         and also we may need to realize the widget. It must also come after
	 glade_project_add_component(), in case setting any properties results
	 in a signal being emitted from the project. */
      set_standard_properties (widget, data);

      tree_add_widget (widget);
    }
  else
    {
      MSG ("widget != NULL, has been created by parent.");
      wdata = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);
      if (wdata == NULL)
	g_warning ("Null widget_data\n");
      data->widget_data = wdata;
      data->widget = widget;
      set_standard_properties (widget, data);
      tree_add_widget (widget);
    }

  if (child_name)
    gb_widget_set_child_name (widget, child_name);

  if (data->status != GLADE_STATUS_OK)
    {
      MSG ("Load error");
      return NULL;
    }

  MSG ("Calling widgets set_properties()");
  if (gbwidget->gb_widget_set_properties)
    (gbwidget->gb_widget_set_properties) (widget, data);
  MSG ("Called widgets set_properties()");

  if (data->status != GLADE_STATUS_OK)
    {
      MSG ("Load error");
      return NULL;
    }

  /* Save a pointer to the widget in the all_widgets hash, when loading and
     when pasting. Note that when pasting we use the name before any renaming
     is done, so ATK relation targets within the widget hierarchy being
     pasted will still be correct. */
  if (data->all_widgets)
    {
      const gchar *widget_name = widget_info->name;
      if (widget_name && *widget_name)
	{
#if 0
	  g_print ("Saving pointer to widget: %s, %p\n", widget_name, widget);
#endif
	  g_hash_table_insert (data->all_widgets, (gpointer) widget_name,
			       widget);
	}
    }

  /* Save a pointer to the GladeWidgetInfo in the widget, which we use to
     load the ATK properties after all widgets are created. */
  gtk_object_set_data (GTK_OBJECT (widget), GladeWidgetInfoKey, widget_info);

  /* When pasting, we only ever replace the top widget. All children are added
     as normal. */
  data->replacing_widget = NULL;

  MSG ("Loading children");

  /* load children. */
  for (i = 0; i < widget_info->n_children; i++)
    {
      data->child_info = &widget_info->children[i];
      data->widget_info = data->child_info->child;

      /* We have to reset the widget_data since this may have been changed
	 while loading the last child (and its children) */
      data->widget_data = wdata;
      if (widget->parent && !gb_widget_lookup (widget->parent))
	{
	  MSG1 ("Unusual widget here of type '%s'", 
		G_OBJECT_TYPE_NAME (widget->parent));
	}

      /* Skip the hscrollbar & vscrollbar internal children, from
	 GtkScrolledWindow. libglade-convert outputs these, but we don't
	 use them. */
      skip_child = FALSE;
      if (data->child_info && data->child_info->internal_child)
	{
	  if (!strcmp (data->child_info->internal_child, "vscrollbar")
	      || !strcmp (data->child_info->internal_child, "hscrollbar"))
	    skip_child = TRUE;
	}

      if (!skip_child)
	gb_widget_load (NULL, data, widget);

      /* Reset these, just in case. */
      data->child_info = child_info;
      data->widget_info = widget_info;

      if (data->status != GLADE_STATUS_OK)
	{
	  MSG ("Load error");
	  return NULL;
	}
    }

  /* If the widget is a table, we make sure all empty cells have placeholders
     in them. */
  if (GTK_IS_TABLE (widget))
    {
      gb_table_update_placeholders (widget, -1, -1);
    }

  return widget;
}


/* Adds a widget to a parent, when loading or pasting from the clipboard.
   Returns TRUE on success. */
gboolean
gb_widget_add_child (GtkWidget * parent,
		     GbWidgetSetArgData * data,
		     GtkWidget * child)
{
  GbWidget *gbwidget;

  MSG2 ("Adding %s to %s", gtk_widget_get_name (child),
	parent ? gtk_widget_get_name (parent) : "NULL");

  g_return_val_if_fail (parent != NULL, FALSE);

  /* If we are pasting, the top widget replaces the parent rather than being
     added to it. */
  if (data->xml_buffer && data->replacing_widget)
    {
      return gb_widget_replace_child (parent, data->replacing_widget, child);
    }

  gbwidget = gb_widget_lookup (parent);
  g_return_val_if_fail (gbwidget != NULL, FALSE);

  if (gbwidget->gb_widget_add_child)
    {
      gbwidget->gb_widget_add_child (parent, child, data);
    }
  else if (GTK_IS_BIN (parent))
    {
      if (GTK_BIN (parent)->child)
	gtk_container_remove (GTK_CONTAINER (parent), GTK_BIN (parent)->child);
      gtk_container_add (GTK_CONTAINER (parent), child);
    }
  else if (GTK_IS_CONTAINER (parent))
    {
      gtk_container_add (GTK_CONTAINER (parent), child);
    }

  return TRUE;
}


/*************************************************************************
 * Functions for saving project files
 *************************************************************************/

void
gb_widget_save (GtkWidget * widget,
		GbWidgetGetArgData * data)
{
  GbWidget *gbwidget, *parent_gbwidget;
  GladeWidgetData *widget_data;
  gchar *child_name, *class_id, *id, *class_to_save;
  gboolean is_toplevel;

  class_id = class_to_save = gb_widget_get_class_id (widget);
  child_name = gb_widget_get_child_name (widget);
  is_toplevel = glade_util_is_component (widget);

  /* Bonobo Controls are save with a class of 'BonoboWidget'. */
#ifdef USE_GNOME
  if (gtk_object_get_data (GTK_OBJECT (widget), Moniker))
    class_to_save = "BonoboWidget";
#endif

  /* SPECIAL CODE: CList/CTree title buttons don't save the child name. */
  if (child_name && !strcmp (child_name, GladeChildCListTitle))
    {
      child_name = NULL;
    }

  /* SPECIAL CODE: Don't save a placeholder if its parent is a table, since we
     don't really want placeholders in the XML when the interface is finished,
     but it is quite possible that some table cells will be left blank. */
  if (GB_IS_PLACEHOLDER (widget))
    {
      if (!GTK_IS_TABLE (widget->parent))
	{
	  save_newline (data);

	  if (!is_toplevel)
	    save_child_start_tag (data, child_name);

	  save_placeholder (data);

	  if (!is_toplevel)
	    save_end_tag (data, "child");
	}
      return;
    }

  /* Write out widget data and any child widgets */
  widget_data = gtk_object_get_data (GTK_OBJECT (widget), GB_WIDGET_DATA_KEY);

  /* If this isn't a gbwidget, skip it, but save any child gbwidgets. */
  if (!widget_data)
    {
      /* Recursively save children. */
      gb_widget_children_foreach (widget, (GtkCallback) gb_widget_save, data);
      return;
    }

  data->action = GB_SAVING;
  data->widget_data = widget_data;
  data->widget = widget;

  save_newline (data);
  if (!is_toplevel)
    save_child_start_tag (data, child_name);

  id = (char*) gtk_widget_get_name (widget);
  save_widget_start_tag (data, class_to_save, id);

  get_standard_properties (widget, data);

  /* Call gbwidgets save function for any extra properties */
  gbwidget = gb_widget_lookup_class (class_id);
  g_return_if_fail (gbwidget != NULL);

  if (gbwidget->gb_widget_get_properties)
    (gbwidget->gb_widget_get_properties) (widget, data);

  glade_atk_save_properties (widget, data);

  save_signals (widget, data);
  save_accelerators (widget, data);

  /* Recursively save children. */
  gb_widget_children_foreach (widget, (GtkCallback) gb_widget_save, data);

  save_end_tag (data, "widget");

  /* Call parent widget's function to save child packing properties. */
  if (widget->parent)
    {
      parent_gbwidget = gb_widget_lookup (widget->parent);
      /* parent_gbwidget may be NULL, e.g. for GnomeDockItems. */
      if (parent_gbwidget
	  && parent_gbwidget->gb_widget_get_child_properties)
	(parent_gbwidget->gb_widget_get_child_properties) (widget->parent,
							   widget, data);
#ifdef USE_GNOME
      else if (BONOBO_IS_DOCK_ITEM (widget))
	{
	  gb_bonobo_dock_item_save_packing_properties (widget->parent, widget,
						       data);
	}
#endif
    }

  if (!is_toplevel)
    save_end_tag (data, "child");
}


/* This outputs all the standard widget properties, for showing or saving.
   Note that when saving we try not to save default values. */
static void
get_standard_properties (GtkWidget * widget,
			 GbWidgetGetArgData * data)
{
  gboolean can_default, has_default, can_focus, has_focus, visible, sensitive;
  GladeWidgetData *wdata = data->widget_data;
  gchar *name, *class_id;
  GbWidgetAction action = data->action;
  gchar border_width_property_buf[128];
  gboolean border_width_visible = FALSE;

  /* The class and name (id) are saved in gb_widget_save, so don't save them
     here. */
  name = (char*) gtk_widget_get_name (widget);
  class_id = gb_widget_get_class_id (widget);
  if (action != GB_SAVING)
    {
      gb_widget_output_string (data, GbName, name);
      gb_widget_output_string (data, GbClass, class_id);
    }

  /* If widget is a container, save the border width */
  sprintf (border_width_property_buf, "%s::border_width", class_id);
  if (glade_util_uses_border_width (widget))
    {
      gint border_width;
      border_width_visible = TRUE;
      border_width = GTK_CONTAINER (widget)->border_width;
      if (action != GB_SAVING || border_width > 0)
	{
	  gb_widget_output_int (data, border_width_property_buf, border_width);
	}
    }
  if (action == GB_SHOWING)
    {
      property_set_visible (border_width_property_buf, border_width_visible);
    }

  get_position_properties (widget, data);
  get_lang_specific_properties (widget, data);

  visible = wdata->flags & GLADE_VISIBLE;
  if (action != GB_SAVING || visible)
    gb_widget_output_bool (data, GbVisible, visible);
  sensitive = wdata->flags & GLADE_SENSITIVE;
  if (action != GB_SAVING || !sensitive)
    gb_widget_output_bool (data, GbSensitive, sensitive);

  /* Only widgets with windows can have tooltips at present. Though buttons
     seem to be a special case, as they are NO_WINDOW widgets but have
     InputOnly windows, so tooltip still work. In GTK+ 2 menuitems are like
     buttons. */
  if (!GTK_WIDGET_NO_WINDOW (widget) || GTK_IS_BUTTON (widget)
      || GTK_IS_MENU_ITEM (widget) || GTK_IS_TOOL_BUTTON (widget)
      || GTK_IS_EVENT_BOX (widget))
    {
      if (action != GB_SAVING || wdata->tooltip)
	gb_widget_output_translatable_string (data, GbTooltip, wdata->tooltip);
      if (action == GB_SHOWING)
	property_set_sensitive (GbTooltip, TRUE);
    }
  else if (action == GB_SHOWING)
    {
      /* N/A stands for 'Not Applicable'. It is used when a standard widget
	 property does not apply to the current widget. e.g. widgets without
	 windows can't use the Events property. This appears in the property
	 editor and so should be a short abbreviation. */
      gb_widget_output_string (data, GbTooltip, _("N/A"));
      property_set_sensitive (GbTooltip, FALSE);
    }

  can_default = GTK_WIDGET_CAN_DEFAULT (widget);
  if (action != GB_SAVING || can_default)
    gb_widget_output_bool (data, GbCanDefault, can_default);
  has_default = wdata ? wdata->flags & GLADE_GRAB_DEFAULT : 0;
  if (action != GB_SAVING || has_default)
    gb_widget_output_bool (data, GbHasDefault, has_default);

  can_focus = GTK_WIDGET_CAN_FOCUS (widget);
  if (action != GB_SAVING || can_focus)
    gb_widget_output_bool (data, GbCanFocus, can_focus);
  has_focus = wdata ? wdata->flags & GLADE_GRAB_FOCUS : 0;
  if (action != GB_SAVING || has_focus)
    gb_widget_output_bool (data, GbHasFocus, has_focus);

  if (!GTK_WIDGET_NO_WINDOW (widget))
    {
      GdkExtensionMode ext_mode = gtk_widget_get_extension_events (widget);
      gint i;

      if (action == GB_SAVING)
	{
	  if (wdata && wdata->events)
	    {
	      gchar buffer[1024];
	      gboolean written_first = FALSE;

	      buffer[0] = '\0';
	      for (i = 0; i < GB_EVENT_MASKS_COUNT; i++)
		{
		  if (wdata->events & GbEventMaskValues[i])
		    {
		      if (written_first)
			strcat (buffer, " | ");
		      strcat (buffer, GbEventMaskSymbols[i]);
		      written_first = TRUE;
		    }
		}

	      gb_widget_output_string (data, GbEvents, buffer);
	    }
	}
      else
	{
	  gb_widget_output_string (data, GbEvents,
			   property_events_value_to_string (wdata->events));
	}

      /* Don't save default extension mode ('None') */
      if (action != GB_SAVING || ext_mode != GDK_EXTENSION_EVENTS_NONE)
	{
	  for (i = 0; GbExtensionModeChoices[i]; i++)
	    {
	      if (GbExtensionModeValues[i] == ext_mode)
		gb_widget_output_choice (data, GbExtEvents, i,
					 GbExtensionModeSymbols[i]);
	    }
	}
      if (action == GB_SHOWING)
	{
	  property_set_sensitive (GbEvents, TRUE);
	  property_set_sensitive (GbExtEvents, TRUE);
	}
    }
  else if (action == GB_SHOWING)
    {
      gb_widget_output_dialog (data, GbEvents, _("N/A"), NULL);
      property_set_sensitive (GbEvents, FALSE);
      gb_widget_output_choice (data, GbExtEvents, 0, GbExtensionModeSymbols[0]);
      property_set_sensitive (GbExtEvents, FALSE);
    }

  if (action == GB_SHOWING)
    {
#ifdef GLADE_STYLE_SUPPORT
      gb_widget_show_style (widget);
#endif
      show_accelerators (widget, data);
      show_signals (widget, data);

      glade_atk_get_properties (widget, data);
    }
  else
    {
#ifdef GLADE_STYLE_SUPPORT
      save_style (widget, data);
#endif

      /* These need to be saved in gb_widget_save(), since they must be after
	 all the widget properties. */
#if 0
      save_signals (widget, data);
      save_accelerators (widget, data);
#endif
    }
}


/* This is used when saving or displaying properties. */
static void
get_lang_specific_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{ guint i;

  GladeWidgetData *wdata = data->widget_data;
  GbWidgetAction action = data->action;

  /* Note that when saving we only save the property if it is not the
     default value. Otherwise we get lots of unnecessary XML output. */

  /* C options. */
  if (action != GB_SAVING || wdata->source_file)
    gb_widget_output_filename (data, GbCSourceFile, wdata->source_file);
  if (action != GB_SAVING || wdata->public_field == 0)
    gb_widget_output_bool (data, GbCPublic, wdata->public_field);
  if (data->action == GB_SHOWING)
    {
      /* We only want the source file set for toplevel widgets. */
      if (glade_util_is_component (widget))
	property_set_sensitive (GbCSourceFile, TRUE);
      else
	property_set_sensitive (GbCSourceFile, FALSE);
    }

  /* C++ options. */
  data->agent = "glademm";
  if (action != GB_SAVING || wdata->cxx_separate_class == 1)
    gb_widget_output_bool (data, GbCxxSeparateClass, wdata->cxx_separate_class);
  if (action == GB_SHOWING)
    property_set_sensitive (GbCxxSeparateFile, wdata->cxx_separate_class);
  if (action != GB_SAVING || wdata->cxx_separate_file == 1)
    gb_widget_output_bool (data, GbCxxSeparateFile, wdata->cxx_separate_file);
  if (action != GB_SAVING || wdata->cxx_visibility != 0)
    for (i = 0; GbCxxVisibilityChoices[i]; i++)
	{
	  if (GbCxxVisibilityValues[i] == wdata->cxx_visibility)
	    gb_widget_output_choice (data, GbCxxVisibility, i,
				     GbCxxVisibilitySymbols[i]);
	}
  data->agent = NULL;
}


/* This is used when loading or applying properties. */
static void
set_lang_specific_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  GladeWidgetData *wdata = data->widget_data;
  gchar *filename;
  gboolean public_field;
  gboolean cxx_separate_file, cxx_separate_class;
  gchar *cxx_visibility;

  /* C options. */
  filename = gb_widget_input_filename (data, GbCSourceFile);
  if (data->apply)
    {
      g_free (wdata->source_file);
      wdata->source_file = g_strdup (filename);
    }
  public_field = gb_widget_input_bool (data, GbCPublic);
  if (data->apply)
    wdata->public_field = public_field ? 1 : 0;

  /* C++ options. */
  data->agent = "glademm";
  cxx_separate_class = gb_widget_input_bool (data, GbCxxSeparateClass);
  if (data->apply)
    {
      wdata->cxx_separate_class = cxx_separate_class ? 1 : 0;
      if (!wdata->cxx_separate_class)
	wdata->cxx_separate_file = 0;

      if (property_get_widget () == widget)
	{
	  property_set_sensitive (GbCxxSeparateFile, wdata->cxx_separate_class);
	  property_set_auto_apply (FALSE);
	  property_set_bool (GbCxxSeparateFile, FALSE);
	  property_set_auto_apply (TRUE);
	}
    }

  cxx_separate_file = gb_widget_input_bool (data, GbCxxSeparateFile);
  if (data->apply)
    wdata->cxx_separate_file = cxx_separate_file ? 1 : 0;

  cxx_visibility = gb_widget_input_choice (data, GbCxxVisibility);
  if (data->apply)
    {
      guint i;
      for (i = 0; GbCxxVisibilityChoices[i]; i++)
	{
	  if (!strcmp (cxx_visibility, GbCxxVisibilityChoices[i])
	      || !strcmp (cxx_visibility, GbCxxVisibilitySymbols[i]))
	    {
	      wdata->cxx_visibility = GbCxxVisibilityValues[i];
	      break;
	    }
	}
    }
  data->agent = NULL;
}


static void
get_position_properties (GtkWidget * widget,
			 GbWidgetGetArgData * data)
{
  GladeWidgetData *wdata = data->widget_data;
  gboolean wh_applicable = TRUE;
  gboolean show_wh_buttons = TRUE;

#if 0
  g_print ("In get_pos: %s WS:%s HS:%s ",
	gtk_widget_get_name (widget),
	wdata->flags & GLADE_WIDTH_SET ? "1" : "0",
	wdata->flags & GLADE_HEIGHT_SET ?  "1" : "0");
#endif

  /* Don't bother if widget's area hasn't been allocated yet, unless it is a
     window. */
  /* Note: When using the widget tree to view widget properties the widget's
     area may not have been allocated but we mustn't just return here. */
#if 0
  if (data->action == GB_SHOWING && (wdata->flags & GLADE_SIZE_NOT_ALLOCATED)
      && !GTK_IS_WINDOW (widget))
      return;
#endif

  if (GTK_IS_MENU (widget))
    {
      if (data->action == GB_SHOWING)
	{
	  gb_widget_output_int (data, GbWidth, widget->requisition.width);
	  gb_widget_output_int (data, GbHeight, widget->requisition.height);
	}
      wh_applicable = FALSE;
    }
  else if (GTK_IS_WINDOW (widget))
    {
      /* Toplevel window widgets */
      if (data->action == GB_SHOWING)
	{
	  gb_widget_output_int (data, GbWidth, wdata->width);
	  gb_widget_output_int (data, GbHeight, wdata->height);
	}
      else
	{
	  if (wdata->flags & GLADE_WIDTH_SET)
	    gb_widget_output_int (data, GbWidth, wdata->width);
	  if (wdata->flags & GLADE_HEIGHT_SET)
	    gb_widget_output_int (data, GbHeight, wdata->height);
	}
    }
  else if (widget->parent && (GTK_IS_FIXED (widget->parent)
			      || GTK_IS_LAYOUT (widget->parent)))
    {
      /* Widgets in fixed or layout containers. Note that for widgets in a 
       layout the allocation is relative to the window origin and changes as
       the layout is scrolled, so don't output that as the x & y coords. */
      gb_widget_output_int (data, GbWidth, wdata->width);
      gb_widget_output_int (data, GbHeight, wdata->height);
      show_wh_buttons = FALSE;
    }
  else
    {
      /* Widgets in standard containers */
      if (data->action == GB_SHOWING)
	{
	  /* If the width or height has been set explicitly we show the size
	     set, else we show the current requisition. We always remember
	     what we have shown in wdata->width & height so we know if the
	     user changes it. */
	  if (!(wdata->flags & GLADE_WIDTH_SET))
	    wdata->width = widget->requisition.width;
	  gb_widget_output_int (data, GbWidth, wdata->width);

	  if (!(wdata->flags & GLADE_HEIGHT_SET))
	    wdata->height = widget->requisition.height;
	  gb_widget_output_int (data, GbHeight, wdata->height);

	}
      else
	{
	  /* Only save if user has set it explicitly. */
	  if (wdata->flags & GLADE_WIDTH_SET)
	    gb_widget_output_int (data, GbWidth, wdata->width);
	  if (wdata->flags & GLADE_HEIGHT_SET)
	    gb_widget_output_int (data, GbHeight, wdata->height);
	}
    }

  /* Show the buttons for setting the size & positions explicitly, if
     applicable, and set the values sensitive if they are currently set. */
  if (data->action == GB_SHOWING)
    {
      if (wh_applicable)
	{
	  gboolean wsensitive = (wdata->flags & GLADE_WIDTH_SET) ? TRUE : FALSE;
	  gboolean hsensitive = (wdata->flags & GLADE_HEIGHT_SET) ? TRUE : FALSE;

	  property_set_sensitive_full (GbWidth, TRUE, wsensitive,
				       show_wh_buttons);
	  property_set_sensitive_full (GbHeight, TRUE, hsensitive,
				       show_wh_buttons);
	}
      else
	{
	  property_set_sensitive_full (GbWidth, FALSE, FALSE, FALSE);
	  property_set_sensitive_full (GbHeight, FALSE, FALSE, FALSE);
	}
    }

#if 0
  g_print ("\n");
#endif
}


static void
save_accelerators (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GList *item;
  GladeAccelerator *accel;

  if (data->widget_data == NULL)
    return;

  item = data->widget_data->accelerators;
  while (item)
    {
      accel = (GladeAccelerator *) item->data;

      save_accelerator (data, accel->modifiers, accel->key, accel->signal);

      item = item->next;
    }
}


static void
save_signals (GtkWidget * widget, GbWidgetGetArgData * data)
{
  GList *item;
  GladeSignal *signal;

  if (data->widget_data == NULL)
    return;

  item = data->widget_data->signals;
  while (item)
    {
      signal = (GladeSignal *) item->data;

      save_signal (data, signal->name, signal->handler, signal->after,
		   signal->object, signal->last_modification_time);

      item = item->next;
    }
}



/*************************************************************************
 * Functions for replacing widgets in the user interface
 *************************************************************************/

/*
 * Replace a child with a new child. Used to replace placeholders with
 * a widget when adding widgets, and also to replace widgets with
 * placeholders when deleting. Returns TRUE on success.
 * NOTE: gbwidgets do not currently have their own function for this,
 * but we'll probably add it at some point.
 */
gboolean
gb_widget_replace_child (GtkWidget * widget,
			 GtkWidget * current_child,
			 GtkWidget * new_child)
{
  gchar *child_name, *new_child_name;

  /* Copy the child name to the new widget if necessary. */
  child_name = gb_widget_get_child_name (current_child);
  new_child_name = gb_widget_get_child_name (new_child);

  /* We never copy these child names to the new child widget.
     The "vscrollbar" and "hscrollbar" child names come from libglade-convert,
     but we don't use them and we need to get rid of them. */
  if (child_name && (!strcmp (child_name, "vscrollbar")
		     || !strcmp (child_name, "hscrollbar")))
    {
      child_name = NULL;
    }
	  
  /* Copy the old child name across. */
  gb_widget_set_child_name (new_child, child_name);

#ifdef USE_GNOME
  if (BONOBO_IS_DOCK_ITEM (widget))
    {
      gboolean is_floating;
      gint x, y;

      /* GnomeDockItem widgets which are floating automatically disappear when
	 the child is removed, so we remember the position and try to show it
	 again in the same place after adding the new widget. */
      is_floating = BONOBO_DOCK_ITEM (widget)->is_floating ? TRUE : FALSE;
      if (is_floating)
	bonobo_dock_item_get_floating_position (BONOBO_DOCK_ITEM (widget),
					       &x, &y);
      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      gtk_widget_hide (new_child);
      gtk_container_add (GTK_CONTAINER (widget), new_child);
      gtk_widget_show (new_child);

      if (is_floating)
	bonobo_dock_item_detach (BONOBO_DOCK_ITEM (widget), x, y);
    }
  else if (GTK_IS_FRAME (widget))
#else
  if (GTK_IS_FRAME (widget))
#endif
    {
      /* If this is the frame's label widget, we replace that. */
      if (gtk_frame_get_label_widget (GTK_FRAME (widget)) == current_child)
	{
	  gtk_frame_set_label_widget (GTK_FRAME (widget), new_child);
	}
      else
	{
	  gtk_container_remove (GTK_CONTAINER (widget), current_child);
	  gtk_container_add (GTK_CONTAINER (widget), new_child);
	}
    }
  else if (GTK_IS_TOOL_ITEM (widget))
    {
      /* For a GtkToolItem, if the current child is a placeholder and the
	 GtkToolItem is not a GbWidget or the new child is a GtkToolItem,
	 we replace the GtkToolItem instead. Otherwise we replace the
	 placeholder as usual. */
      if (GB_IS_PLACEHOLDER (current_child)
	  && (!GB_IS_GB_WIDGET (widget) || GTK_IS_TOOL_ITEM (new_child)))
	{
	  return gb_widget_replace_child (widget->parent, widget, new_child);
	}
      else
	{
	  gtk_container_remove (GTK_CONTAINER (widget), current_child);
	  gtk_container_add (GTK_CONTAINER (widget), new_child);
	}
    }
  else if (GTK_IS_EXPANDER (widget))
    {
      /* If this is the expander's label widget, we replace that. */
      if (gtk_expander_get_label_widget (GTK_EXPANDER (widget)) == current_child)
	{
	  gtk_expander_set_label_widget (GTK_EXPANDER (widget), new_child);
	}
      else
	{
	  gtk_container_remove (GTK_CONTAINER (widget), current_child);
	  gtk_container_add (GTK_CONTAINER (widget), new_child);
	}
    }
  else if (GTK_IS_BIN (widget))
    {
      /* For a bin, we just delete the existing child and add the new one. */
      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      gtk_container_add (GTK_CONTAINER (widget), new_child);
    }
  else if (GTK_IS_BOX (widget))
    {
      /* For a box, we find out the position of the current child, delete it,
         add the new one, and move it into position with reorder_child().
	 If the existing child is a placeholder and the new one is a menubar
	 or toolbar, we set the packing so it doesn't expand, as that is
	 probably what the user wants. */
      gboolean expand, fill;
      guint padding;
      GtkPackType pack_type;
      gint pos = glade_util_get_box_pos (GTK_BOX (widget), current_child);
      g_return_val_if_fail (pos != -1, FALSE);
      gtk_box_query_child_packing (GTK_BOX (widget), current_child,
				   &expand, &fill, &padding, &pack_type);

      /* If we are replacing a placeholder (i.e. we are adding a new widget),
	 we try to set the expand & fill options to reasonable defaults. */
      if (GB_IS_PLACEHOLDER (current_child))
	{
	  if (GTK_IS_LABEL (new_child)
	      || GTK_IS_BUTTON (new_child)
	      || GTK_IS_OPTION_MENU (new_child)
	      || GTK_IS_PROGRESS (new_child)
	      || GTK_IS_PROGRESS_BAR (new_child)
	      || GTK_IS_MENU_BAR (new_child)
	      || GTK_IS_TOOLBAR (new_child)
	      || GTK_IS_STATUSBAR (new_child))
	    {
	      expand = FALSE;
	      fill = FALSE;
	    }

	  /* In a vbox, entry & combo widgets should not expand/fill either. */
	  if (GTK_IS_VBOX (widget))
	    {
	      if (GTK_IS_ENTRY (new_child)
		  || GTK_IS_COMBO (new_child)
		  || GTK_IS_SPIN_BUTTON (new_child)
#ifdef USE_GNOME
		   || GNOME_IS_DATE_EDIT (new_child)
		   || GNOME_IS_FILE_ENTRY (new_child)
		   || GNOME_IS_PIXMAP_ENTRY (new_child)
#endif
		  )
		{
		  expand = FALSE;
		  fill = FALSE;
		}
	    }
	}

      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      gtk_container_add (GTK_CONTAINER (widget), new_child);

      gtk_box_set_child_packing (GTK_BOX (widget), new_child, expand, fill,
				 padding, pack_type);
      gtk_box_reorder_child (GTK_BOX (widget), new_child, pos);

    }
  else if (GTK_IS_TOOLBAR (widget))
    {
      gint pos;

      pos = gtk_toolbar_get_item_index (GTK_TOOLBAR (widget),
					GTK_TOOL_ITEM (current_child));
      g_return_val_if_fail (pos != -1, FALSE);

      /* FIXME: GTK+/GNOME 2 bug workaround - something keeps a ref to the
	 initial buttons added to a GnomeApp, which causes when they eventually
	 get destroyed later and we try to remove them from the tree. So we
	 remove them from the tree here. */
      tree_remove_widget (current_child);

      gtk_container_remove (GTK_CONTAINER (widget), current_child);

      /* If the new child is a GtkToolItem, we can simply add it at the old
	 position. */
      if (GTK_IS_TOOL_ITEM (new_child))
	{
	  gtk_toolbar_insert (GTK_TOOLBAR (widget),
			      GTK_TOOL_ITEM (new_child), pos);
	}
      /* If the new child is a placeholder, we need to insert a
	 GtkToolItem above it (but not a GbWidget). */
      else if (GB_IS_PLACEHOLDER (new_child))
	{
	  GtkWidget *toolitem = (GtkWidget*) gtk_tool_item_new ();
	  gtk_widget_show (toolitem);
	  gtk_container_add (GTK_CONTAINER (toolitem), new_child);
	  gtk_toolbar_insert (GTK_TOOLBAR (widget),
			      GTK_TOOL_ITEM (toolitem), pos);
	}
      /* If the new child is not a GtkToolItem, we need to insert a
	 GtkToolItem above it, but use a GbWidget so its properties can be
	 set as required. */
      else
	{
	  GtkWidget *toolitem = gb_widget_new ("GtkToolItem", NULL);
	  gtk_widget_show (toolitem);
	  gtk_container_add (GTK_CONTAINER (toolitem), new_child);
	  gtk_toolbar_insert (GTK_TOOLBAR (widget),
			      GTK_TOOL_ITEM (toolitem), pos);
	  tree_add_widget (toolitem);
	}
    }
  else if (GTK_IS_LIST (widget))
    {
      /* For a list, we find out the position of the current child, delete it,
         and add the new one at the same position. */
      gint pos = gtk_list_child_position (GTK_LIST (widget), current_child);
      GList glist =
      {NULL, NULL, NULL};
      glist.data = current_child;
      gtk_list_remove_items (GTK_LIST (widget), &glist);
      glist.data = new_child;
      gtk_list_insert_items (GTK_LIST (widget), &glist, pos);

    }
  else if (GTK_IS_NOTEBOOK (widget))
    {
      /* For a notebook, we find out the position of the current child, delete
         it, and add the new one at the same position. If the current_child is
         a notebook tab, just replace it. */
      GtkWidget *page, *tab_label;
      gint pos;

      pos = find_notebook_page (GTK_NOTEBOOK (widget), current_child,
				&page, &tab_label);
      g_return_val_if_fail (pos != -1, FALSE);

      if (page == current_child)
	{
	  gtk_widget_ref (tab_label);
	  gtk_notebook_remove_page (GTK_NOTEBOOK (widget), pos);
	  gtk_notebook_insert_page (GTK_NOTEBOOK (widget),
				    new_child, tab_label, pos);
	  gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), pos);
	  gtk_widget_unref (tab_label);
	}
      else
	{
	  gtk_widget_ref (page);
	  gtk_notebook_remove_page (GTK_NOTEBOOK (widget), pos);
	  gtk_notebook_insert_page (GTK_NOTEBOOK (widget),
				    page, new_child, pos);
	  gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), pos);
	  gtk_widget_unref (page);
	}

    }
  else if (GTK_IS_PANED (widget))
    {
      /* For paned, we find out the position of the current child, delete it,
         and add the new one at the same position. */
      gint pos = (GTK_PANED (widget)->child1 == current_child) ? 1 : 2;
      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      if (pos == 1)
	gtk_paned_add1 (GTK_PANED (widget), new_child);
      else
	gtk_paned_add2 (GTK_PANED (widget), new_child);

    }
  else if (GTK_IS_TABLE (widget))
    {
      /* For a table, we find out the position & size of the current child,
         delete it, and add the new one in the same place. */
      gint left, right, top, bottom;
      GtkTableChild *tchild;
      GtkAttachOptions xoptions, yoptions;

      tchild = glade_util_find_table_child (GTK_TABLE (widget), current_child);
      g_return_val_if_fail (tchild != NULL, FALSE);

      left = tchild->left_attach;
      right = tchild->right_attach;
      top = tchild->top_attach;
      bottom = tchild->bottom_attach;

      xoptions = 0;
      if (tchild->xexpand)
	xoptions |= GTK_EXPAND;
      if (tchild->xshrink)
	xoptions |= GTK_SHRINK;
      if (tchild->xfill)
	xoptions |= GTK_FILL;

      yoptions = 0;
      if (tchild->yexpand)
	yoptions |= GTK_EXPAND;
      if (tchild->yshrink)
	yoptions |= GTK_SHRINK;
      if (tchild->yfill)
	yoptions |= GTK_FILL;

      /* If we are replacing a placeholder (i.e. we are adding a new widget),
	 we try to set the expand & fill options to reasonable defaults. */
      if (GB_IS_PLACEHOLDER (current_child))
	{
	  if (GTK_IS_LABEL (new_child)
	      || GTK_IS_BUTTON (new_child)
	      || GTK_IS_OPTION_MENU (new_child)
	      || GTK_IS_PROGRESS (new_child)
	      || GTK_IS_PROGRESS_BAR (new_child)
	      || GTK_IS_MENU_BAR (new_child)
	      || GTK_IS_TOOLBAR (new_child)
	      || GTK_IS_STATUSBAR (new_child))
	    {
	      xoptions = GTK_FILL;
	      yoptions = 0;
	    }
	  else if (GTK_IS_ENTRY (new_child)
		   || GTK_IS_COMBO (new_child)
		   || GTK_IS_SPIN_BUTTON (new_child)
#ifdef USE_GNOME
		   || GNOME_IS_DATE_EDIT (new_child)
		   || GNOME_IS_FILE_ENTRY (new_child)
		   || GNOME_IS_PIXMAP_ENTRY (new_child)
#endif
		   )
	    {
	      xoptions = GTK_EXPAND | GTK_FILL;
	      yoptions = 0;
	    }
	}

      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      gtk_table_attach (GTK_TABLE (widget), new_child,
			left, right, top, bottom, xoptions, yoptions, 0, 0);

      /* Note that if we have just added a placeholder, but there is already
	 a widget at the same position in the table, the placeholder will be
	 destroyed immediately. Thus don't rely on the new widget still being
	 alive after calling gb_widget_replace_child(). */
      gb_table_update_placeholders (widget, -1, -1);
    }
#if GLADE_SUPPORTS_GTK_TREE
  else if (GTK_IS_TREE (widget))
    {
      /* For a tree, we find out the position of the current child, delete it,
         and add the new one at the same position. */
      gint pos = gtk_tree_child_position (GTK_TREE (widget), current_child);
      GList glist =
      {NULL, NULL, NULL};
      glist.data = current_child;
      gtk_tree_remove_items (GTK_TREE (widget), &glist);
      gtk_tree_insert (GTK_TREE (widget), new_child, pos);

    }
#endif
  else if (GTK_IS_CLIST (widget))
    {
      /* For a clist, we check if the widget is a column title and if it is
	 we replace it. */
	gint pos;
	GtkCList *clist;

	clist = GTK_CLIST(widget);

	for (pos = 0; pos < clist->columns; pos++)
	{
	    if (clist->column[pos].button == current_child)
	    {
		gtk_clist_set_column_widget(clist, pos, new_child);
 	    }
	}
    }
  else if (GTK_IS_FIXED (widget) || GTK_IS_LAYOUT (widget))
    {
      GladeWidgetData *wdata;
      gint x, y, w, h;

      /* For a fixed, we find the position and size of the existing child,
	 remove it, and add the new one in the same place. */
      wdata = gtk_object_get_data (GTK_OBJECT (current_child),
				   GB_WIDGET_DATA_KEY);
      g_return_val_if_fail (wdata != NULL, FALSE);

      gtk_container_child_get (GTK_CONTAINER (widget), current_child,
			       "x", &x,
			       "y", &y,
			       NULL);
      w = wdata->width;
      h = wdata->height;

      wdata = gtk_object_get_data (GTK_OBJECT (new_child), GB_WIDGET_DATA_KEY);
      g_return_val_if_fail (wdata != NULL, FALSE);
      wdata->flags |= GLADE_WIDTH_SET | GLADE_HEIGHT_SET;
      wdata->width = w;
      wdata->height = h;

      /* Reset the widget's uposition, just in case it gets added to a standard
	 container. I don't think we need this for GTK+ 2. */
      /*gtk_widget_set_uposition (current_child, -1, -1);*/

      /* FIXME: GTK+ 1.2.3 bug workaround. We need to ref the widget to stop
	 gtk_layout_remove() from issuing a warning. */
      gtk_widget_ref (current_child);
      gtk_container_remove (GTK_CONTAINER (widget), current_child);
      gtk_widget_unref (current_child);

      if (GTK_IS_FIXED (widget))
	{
	  gtk_fixed_put (GTK_FIXED (widget), new_child, x, y);
	  /*gtk_widget_set_uposition (new_child, x, y);*/
	}
      else
	{
	  gtk_layout_put (GTK_LAYOUT (widget), new_child, x, y);
	}
      gb_widget_set_usize (new_child, w, h);
    }

#ifdef USE_GNOME
  else if (BONOBO_IS_DOCK (widget))
    {
      /* For a GnomeDock, we call bonobo_dock_set_client_area (). It removes
	 the existing child automatically. */
      bonobo_dock_set_client_area (BONOBO_DOCK (widget), new_child);
    }

#endif

  else if (GTK_IS_CONTAINER (widget))
    {
      /* General code for container - has to remove all children and add back
         NOTE: this may not work for specialised containers.
         NOTE: need to ref widgets? */
      g_warning (_("replacing child of container - not implemented yet\n"));
      return FALSE;
    }

  return TRUE;
}


/*************************************************************************
 * Functions for showing/hiding tooltips of the widgets
 *************************************************************************/

gboolean
gb_widget_get_show_tooltips ()
{
  return GTK_TOOLTIPS (gb_widget_tooltips)->enabled;
}


void
gb_widget_set_show_tooltips (gboolean show)
{
  if (show)
    gtk_tooltips_enable (gb_widget_tooltips);
  else
    gtk_tooltips_disable (gb_widget_tooltips);
}


void
gb_widget_reset_tooltips ()
{
  gtk_object_destroy (GTK_OBJECT (gb_widget_tooltips));
  gb_widget_tooltips = gtk_tooltips_new ();
}



/*************************************************************************
 * Misc. Functions
 *************************************************************************/


/* This is a GTK bug workaround for combo widgets. They should manage the
   size of their GtkEntry, but they don't at present, so we have to set its
   size explicitly (and also in the source code output).
   I think this has been fixed. It seems to be OK in GTK+ 1.2.3. */
void
gb_widget_set_usize (GtkWidget *widget,
		     gint w,
		     gint h)
{
#if 0
  if (GTK_IS_COMBO (widget))
    gtk_widget_set_usize (GTK_COMBO (widget)->entry,
			  w - 16 < 0 ? -1 : w - 16, h);
#endif
  gtk_widget_set_usize (widget, w, h);
}


static gint
find_notebook_page (GtkNotebook * notebook, GtkWidget * current_child,
		    GtkWidget **page, GtkWidget **tab_label)
{
  gint nchildren, i;
  GtkWidget *tmp_page, *tmp_tab_label;

  nchildren = g_list_length (notebook->children);
  for (i = 0; i < nchildren; i++)
    {
      tmp_page = gtk_notebook_get_nth_page (notebook, i);
      tmp_tab_label = gtk_notebook_get_tab_label (notebook, tmp_page);

      if (tmp_page == current_child || tmp_tab_label == current_child)
	{
	  *page = tmp_page;
	  *tab_label = tmp_tab_label;
	  return i;
	}
    }

  return -1;
}

static void
dummy_detach (GtkWidget *attach_widget,
	      GtkMenu *menu)
{
  ;
}

/*
 * FIXME: MAJOR HACK.
 *
 * GtkOptionMenu places the currently selected item inside its button, and
 * removes it from the menu. So we have to hack around that here so that the
 * menu items are all output correctly, in the XML and the source code.
 *
 * We remove the menu from the original option menu and add it to a temporary
 * one with the same name.
 */
static void
option_menu_foreach (GtkOptionMenu *option,
		     GtkCallback callback,
		     gpointer data)
{
  GtkWidget *menu = option->menu;
  GtkWidget *temp;
  int history;

  if (!menu)
    return;

  temp = gtk_option_menu_new ();
  gtk_widget_set_name (temp, gtk_widget_get_name (GTK_WIDGET (option)));
  gtk_object_ref (GTK_OBJECT (temp));
  gtk_object_sink (GTK_OBJECT (temp));

  history = gtk_option_menu_get_history (option);

  gtk_object_ref (GTK_OBJECT (menu));

  gtk_option_menu_set_menu (option, gtk_menu_new ());
  gtk_menu_attach_to_widget (GTK_MENU (menu), temp, dummy_detach);
  (*callback) (menu, data);
  gtk_menu_detach (GTK_MENU (menu));
  gtk_option_menu_set_menu (option, menu);

  gtk_object_unref (GTK_OBJECT (menu));

  gtk_option_menu_set_history (option, history);

  gtk_object_unref (GTK_OBJECT (temp));
}

static void
combo_foreach (GtkCombo *combo,
	       GtkCallback callback,
	       gpointer data)
{
  (*callback) (combo->entry, data);
  (*callback) (combo->list, data);
  (*callback) (combo->button, data);
}


/* This calls the given callback for each child of a widget. It gets round
   some of the quirks of the different versions of GTK, and descends menus
   as well. */
void
gb_widget_children_foreach (GtkWidget *widget,
			    GtkCallback callback,
			    gpointer data)
{
  /* SPECIAL CODE: for table, so we output in the reverse order. */
  if (GTK_IS_TABLE (widget))
    table_foreach (GTK_TABLE (widget), callback, data);
  else if (GTK_IS_COMBO (widget))
    combo_foreach (GTK_COMBO (widget), callback, data);
  else if (GTK_IS_BOX (widget))
    box_foreach (GTK_BOX (widget), callback, data);
  else if (GTK_IS_OPTION_MENU (widget))
    option_menu_foreach (GTK_OPTION_MENU (widget), callback, data);
  else if (GTK_IS_CONTAINER (widget))
    gtk_container_forall (GTK_CONTAINER (widget), callback, data);

  /* SPECIAL CODE: for menu items, descend to child menus. */
  if (GTK_IS_MENU_ITEM (widget) && GTK_MENU_ITEM (widget)->submenu)
    (*callback) (GTK_MENU_ITEM (widget)->submenu, data);
}


/* This function is used to iterate through the table children in reverse.
   It is needed so we output the XML file in the same order each time. */
static void
table_foreach (GtkTable * table,
	       GtkCallback callback,
	       gpointer callback_data)
{
  GList *children;
  GtkTableChild *child;

  children = g_list_last (table->children);
  while (children)
    {
      child = children->data;
      children = children->prev;

      (*callback) (child->widget, callback_data);
    }
}


static void
box_foreach (GtkBox *box,
	     GtkCallback callback,
	     gpointer callback_data)
{
  GList *children;
  GtkBoxChild *child;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}

/*************************************************************************
 * Common popup menu callbacks
 *************************************************************************/

static void
gb_widget_add_alignment (GtkWidget * menuitem,
			 GtkWidget * widget)
{
  GtkWidget *alignment, *parent;

  parent = widget->parent;
  alignment = gb_widget_new ("GtkAlignment", parent);

  gtk_widget_ref (widget);
  if (!gb_widget_replace_child (parent, widget, alignment))
    {
      glade_util_show_message_box (_("Couldn't insert GtkAlignment widget."),
				   parent);
      gtk_widget_destroy (alignment);
      gtk_widget_unref (widget);
      return;
    }
  if (GTK_BIN (alignment)->child)
    gtk_container_remove (GTK_CONTAINER (alignment),
			  GTK_BIN (alignment)->child);
  gtk_container_add (GTK_CONTAINER (alignment), widget);
  gtk_widget_unref (widget);
  tree_insert_widget_parent (alignment, widget);
}


static void
gb_widget_remove_alignment (GtkWidget * menuitem,
			    GtkWidget * widget)
{
  GtkWidget *alignment, *parent;

  alignment = widget->parent;
  g_return_if_fail (GTK_IS_ALIGNMENT (alignment));
  parent = alignment->parent;

  gtk_widget_ref (widget);
  gtk_widget_ref (alignment);

  /* Remove the alignment and all children from the tree. */
  tree_remove_widget (alignment);

  gtk_container_remove (GTK_CONTAINER (alignment), widget);

  if (gb_widget_replace_child (parent, alignment, widget))
    {
      /* Now add the widget and its children back to the tree. */
      tree_add_widget (widget);
    }
  else
    {
      glade_util_show_message_box (_("Couldn't remove GtkAlignment widget."),
				   parent);
      /* Try to put it back as it was. */
      gtk_container_add (GTK_CONTAINER (alignment), widget);
      tree_add_widget (alignment);
    }

  gtk_widget_unref (alignment);
  gtk_widget_unref (widget);
}


static void
gb_widget_add_event_box (GtkWidget * menuitem,
			 GtkWidget * widget)
{
  GtkWidget *event_box, *parent;

  parent = widget->parent;
  event_box = gb_widget_new ("GtkEventBox", parent);

  gtk_widget_ref (widget);
  if (!gb_widget_replace_child (parent, widget, event_box))
    {
      glade_util_show_message_box (_("Couldn't insert GtkEventBox widget."),
				   parent);
      gtk_widget_destroy (event_box);
      gtk_widget_unref (widget);
      return;
    }
  if (GTK_BIN (event_box)->child)
    gtk_container_remove (GTK_CONTAINER (event_box),
			  GTK_BIN (event_box)->child);
  gtk_container_add (GTK_CONTAINER (event_box), widget);
  gtk_widget_unref (widget);
  tree_insert_widget_parent (event_box, widget);
}

static void
gb_widget_remove_event_box (GtkWidget * menuitem,
			    GtkWidget * widget)
{
  GtkWidget *event_box, *parent;

  event_box = widget->parent;
  g_return_if_fail (GTK_IS_EVENT_BOX (event_box));
  parent = event_box->parent;

  gtk_widget_ref (widget);
  gtk_widget_ref (event_box);

  /* Remove the event box and all children from the tree. */
  tree_remove_widget (event_box);

  gtk_container_remove (GTK_CONTAINER (event_box), widget);

  if (gb_widget_replace_child (parent, event_box, widget))
    {
      /* Now add the widget and its children back to the tree. */
      tree_add_widget (widget);
    }
  else
    {
      glade_util_show_message_box (_("Couldn't remove GtkEventBox widget."),
				   parent);
      /* Try to put it back as it was. */
      gtk_container_add (GTK_CONTAINER (event_box), widget);
      tree_add_widget (event_box);
    }
  gtk_widget_unref (event_box);
  gtk_widget_unref (widget);
}


static void
gb_widget_redisplay_window (GtkWidget * menuitem,
			    GtkWidget * widget)
{
  g_return_if_fail (GTK_IS_WINDOW (widget));

  /* See also editor_on_key_press_event() in editor.c. */
  glade_util_close_window (widget);
  gtk_window_reshow_with_initial_size (GTK_WINDOW (widget));
}


static void
gb_widget_add_scrolled_window (GtkWidget * menuitem,
			       GtkWidget * widget)
{
  GtkWidget *scrolledwin, *parent;

  parent = widget->parent;
  scrolledwin = gb_widget_new ("GtkScrolledWindow", parent);

  gtk_widget_ref (widget);
  if (!gb_widget_replace_child (parent, widget, scrolledwin))
    {
      glade_util_show_message_box (_("Couldn't insert GtkScrolledWindow widget."), parent);
      gtk_widget_destroy (scrolledwin);
      gtk_widget_unref (widget);
      return;
    }
  if (GTK_BIN (scrolledwin)->child)
    gtk_container_remove (GTK_CONTAINER (scrolledwin),
			  GTK_BIN (scrolledwin)->child);
  gtk_container_add (GTK_CONTAINER (scrolledwin), widget);
  gtk_widget_unref (widget);
  tree_insert_widget_parent (scrolledwin, widget);
}


static void
gb_widget_remove_scrolled_window (GtkWidget * menuitem,
				  GtkWidget * widget)
{
  GtkWidget *scrolledwin, *parent;

  scrolledwin = widget->parent;
  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (scrolledwin));
  parent = scrolledwin->parent;

  gtk_widget_ref (widget);
  gtk_widget_ref (scrolledwin);

  /* Remove the alignment and all children from the tree. */
  tree_remove_widget (scrolledwin);

  gtk_container_remove (GTK_CONTAINER (scrolledwin), widget);

  if (gb_widget_replace_child (parent, scrolledwin, widget))
    {
      /* Now add the widget and its children back to the tree. */
      tree_add_widget (widget);
    }
  else
    {
      glade_util_show_message_box (_("Couldn't remove GtkScrolledWindow widget."), parent);
      /* Try to put it back as it was. */
      gtk_container_add (GTK_CONTAINER (scrolledwin), widget);
      tree_add_widget (scrolledwin);
    }

  gtk_widget_unref (scrolledwin);
  gtk_widget_unref (widget);
}


/*************************************************************************
 * Common functions used by gbwidgets.
 *************************************************************************/

/* This gets the child label for buttons or menuitems or subclasses.
   This is for showing or saving. */
void
gb_widget_output_child_label (GtkWidget * widget, GbWidgetGetArgData * data,
			      const gchar * Label)
{
  GtkWidget *child;
  gchar *label_text;

  child = GTK_BIN (widget)->child;

  /* Note that we don't want to save the child label if it is a GbWidget,
     since it will be saved as a separate widget. */
  if (child && GTK_IS_LABEL (child) && !GB_IS_GB_WIDGET (child))
    {
      label_text = glade_util_get_label_text (child);
      gb_widget_output_translatable_text (data, Label, label_text);
      g_free (label_text);

      if (data->action == GB_SHOWING)
	property_set_sensitive (Label, TRUE);

      /* All our menuitems use underlined accelerators. */
      if (data->action == GB_SAVING && GTK_IS_MENU_ITEM (widget))
	gb_widget_output_bool (data, "use_underline", TRUE);
    }
  else
    {
      if (data->action == GB_SHOWING)
	{
	  gb_widget_output_translatable_text (data, Label, "");
	  property_set_sensitive (Label, FALSE);
	}
    }
}


/* This sets the child label for buttons/items/menuitems or subclasses.
   This is for applying or loading. */
void
gb_widget_input_child_label (GtkWidget * widget, GbWidgetSetArgData * data,
			     const gchar * Label)
{
  GtkWidget *child, *label;
  gchar *label_text;

  child = GTK_BIN (widget)->child;

  label_text = gb_widget_input_text (data, Label);
  if (data->apply)
    {
      if (child && GTK_IS_LABEL (child))
	{
	  gtk_label_set_text_with_mnemonic (GTK_LABEL (child), label_text);
	}
      else
	{
	  if (child != NULL)
	    gtk_container_remove (GTK_CONTAINER (widget), child);
	  if (GTK_IS_MENU_ITEM (widget))
	    {
	      label = gtk_accel_label_new ("");
	      gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label),
						widget);
	    }
	  else
	    {
	      label = gtk_label_new ("");
	    }
	  gtk_container_add (GTK_CONTAINER (widget), label);
	  gtk_label_set_text_with_mnemonic (GTK_LABEL (label), label_text);
	  gtk_widget_show (label);

	  /* Simple child labels are given different alignments according to
	     the parent. GtkButton and GtkToggleButton are centred. All the
	     others are aligned left. See the GTK _new_with_label() fns. */
	  if (data->action == GB_LOADING)
	    {
	      if (GTK_IS_CHECK_BUTTON (widget)
		  || GTK_IS_ITEM (widget))
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	    }
	}
    }
  /* This isn't very nice. When a text property is got from the property
     editor (i.e. when action is GB_APPLYING) it needs to be freed after. */
  if (data->action == GB_APPLYING)
    g_free (label_text);
}


void
gb_widget_create_child_label_popup_menu (GtkWidget	       *widget,
					 GbWidgetCreateMenuData *data)
{
  GtkWidget *menuitem;

  if (GTK_IS_LABEL (GTK_BIN (widget)->child))
    {
      menuitem = gtk_menu_item_new_with_label (_("Remove Label"));
      gtk_widget_show (menuitem);
      gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_widget_remove_label), widget);
    }
}


void
gb_widget_remove_label (GtkWidget * menuitem,
			GtkWidget * widget)
{
  GtkWidget *child;

  g_return_if_fail (GTK_IS_BIN (widget));

  child = GTK_BIN (widget)->child;
  if (child && GTK_IS_LABEL (child))
    editor_delete_widget (child);
}


/*************************************************************************
 * Adjustment convenience functions - handles all 6 values.
 *************************************************************************/
void
gb_widget_output_adjustment (GbWidgetGetArgData * data,
			     const gchar * Values[],
			     GtkAdjustment * adjustment,
			     gchar *saved_property_name)
{
  /* Adjustments are now saved as a single property with all 6 values in a
     string, e.g. '0 0 100 1 10 10'. */
  if (data->action == GB_SAVING)
    {
      gchar buffer[256];

      sprintf (buffer, "%.12g %.12g %.12g %.12g %.12g %.12g",
	       adjustment->value,
	       adjustment->lower,
	       adjustment->upper,
	       adjustment->step_increment,
	       adjustment->page_increment,
	       adjustment->page_size);
      save_string (data, saved_property_name, buffer);
    }
  else
    {
      if (Values[0])
	gb_widget_output_float (data, Values[0], adjustment->value);
      if (Values[1])
	gb_widget_output_float (data, Values[1], adjustment->lower);
      if (Values[2])
	gb_widget_output_float (data, Values[2], adjustment->upper);
      if (Values[3])
	gb_widget_output_float (data, Values[3], adjustment->step_increment);
      if (Values[4])
	gb_widget_output_float (data, Values[4], adjustment->page_increment);
      if (Values[5])
	gb_widget_output_float (data, Values[5], adjustment->page_size);
    }
}


gboolean
gb_widget_input_adjustment (GbWidgetSetArgData * data,
			    const gchar * Values[],
			    GtkAdjustment * adjustment,
			    gchar *saved_property_name)
{
  gfloat value, lower, upper, step_inc, page_inc, page_size, tmp;

  /* Adjustments are now saved as a single property with all 6 values in a
     string, e.g. '0 0 100 1 10 10'. */
  if (data->action == GB_LOADING)
    {
      gchar *value = gb_widget_input_string (data, saved_property_name);
      if (data->apply)
	{
	  gchar *ptr = value;

	  adjustment->value = g_strtod (ptr, &ptr);
	  adjustment->lower = g_strtod (ptr, &ptr);
	  adjustment->upper = g_strtod (ptr, &ptr);
	  adjustment->step_increment = g_strtod (ptr, &ptr);
	  adjustment->page_increment = g_strtod (ptr, &ptr);
	  adjustment->page_size = g_strtod (ptr, &ptr);

	  return TRUE;
	}
      else
	return FALSE;
    }

  value = adjustment->value;
  lower = adjustment->lower;
  upper = adjustment->upper;
  step_inc = adjustment->step_increment;
  page_inc = adjustment->page_increment;
  page_size = adjustment->page_size;

  if (Values[0])
    {
      tmp = gb_widget_input_float (data, Values[0]);
      if (data->apply)
	value = tmp;
    }

  if (Values[1])
    {
      tmp = gb_widget_input_float (data, Values[1]);
      if (data->apply)
	lower = tmp;
    }

  if (Values[2])
    {
      tmp = gb_widget_input_float (data, Values[2]);
      if (data->apply)
	upper = tmp;
    }

  if (Values[3])
    {
      tmp = gb_widget_input_float (data, Values[3]);
      if (data->apply)
	step_inc = tmp;
    }

  if (Values[4])
    {
      tmp = gb_widget_input_float (data, Values[4]);
      if (data->apply)
	page_inc = tmp;
    }

  if (Values[5])
    {
      tmp = gb_widget_input_float (data, Values[5]);
      if (data->apply)
	page_size = tmp;
    }

  /* Only return TRUE if one or more of the properties have changed. */
  if (adjustment->value != value
      || adjustment->lower != lower
      || adjustment->upper != upper
      || adjustment->step_increment != step_inc
      || adjustment->page_increment != page_inc
      || adjustment->page_size != page_size)
    {
      adjustment->value = value;
      adjustment->lower = lower;
      adjustment->upper = upper;
      adjustment->step_increment = step_inc;
      adjustment->page_increment = page_inc;
      adjustment->page_size = page_size;
      return TRUE;
    }
  return FALSE;
}


/*************************************************************************
 * Functions to input/output properties.
 *************************************************************************/

/* Inputting Properties - Loading or Applying. */

gchar*
gb_widget_input_string (GbWidgetSetArgData *data,
			const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_string (data, property);
  else
    return property_get_string (property, data->widget,
				data->property_to_apply, &data->apply);
}


/* NOTE: You must free the returned string if data->action == GB_APPLYING */
gchar*
gb_widget_input_text (GbWidgetSetArgData *data,
		      const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_text (data, property);
  else
    return property_get_text (property, data->widget,
			      data->property_to_apply, &data->apply);
}


gint
gb_widget_input_int (GbWidgetSetArgData *data,
		     const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_int (data, property);
  else
    return property_get_int (property, data->property_to_apply,
			     &data->apply);
}


gint
gb_widget_input_optional_int (GbWidgetSetArgData *data,
			      const gchar *property,
			      gboolean *is_set)
{
  gint value;

  if (data->action == GB_LOADING)
    {
      value = load_int (data, property);
      if (is_set)
	*is_set = data->apply;
      return value;
    }
  else
    return property_get_optional_int (property, data->property_to_apply,
				      &data->apply, is_set);
}


gfloat
gb_widget_input_float (GbWidgetSetArgData *data,
		       const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_float (data, property);
  else
    return property_get_float (property, data->property_to_apply,
			       &data->apply);
}


gboolean
gb_widget_input_bool (GbWidgetSetArgData *data,
		      const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_bool (data, property);
  else
    return property_get_bool (property, data->property_to_apply,
			      &data->apply);
}


gchar*
gb_widget_input_choice (GbWidgetSetArgData *data,
			const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_choice (data, property);
  else
    return property_get_choice (property, data->property_to_apply,
				&data->apply);
}


gchar*
gb_widget_input_combo (GbWidgetSetArgData *data,
		       const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_combo (data, property);
  else
    return property_get_combo (property, data->property_to_apply,
			       &data->apply);
}


GdkColor*
gb_widget_input_color (GbWidgetSetArgData *data,
		       const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_color (data, property);
  else
    return property_get_color (property, data->property_to_apply,
			       &data->apply);
}


GdkPixmap*
gb_widget_input_bgpixmap (GbWidgetSetArgData *data,
			  const gchar *property,
			  gchar **filename)
{
  if (data->action == GB_LOADING)
    return load_bgpixmap (data, property, filename);
  else
    return property_get_bgpixmap (property, data->property_to_apply,
				  &data->apply, filename);
}


gpointer
gb_widget_input_dialog (GbWidgetSetArgData *data,
			const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_dialog (data, property);
  else
    return property_get_dialog (property, data->property_to_apply,
				&data->apply);
}


gchar*
gb_widget_input_filename (GbWidgetSetArgData *data,
			  const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_filename (data, property);
  else
    return property_get_filename (property, data->property_to_apply,
				  &data->apply);
}


gchar*
gb_widget_input_pixmap_filename (GbWidgetSetArgData *data,
				 const gchar *property)
{
  if (data->action == GB_LOADING)
    return load_pixmap_filename (data, property);
  else
    return property_get_filename (property, data->property_to_apply,
				  &data->apply);
}


GdkFont*
gb_widget_input_font (GbWidgetSetArgData *data,
		      const gchar *property,
		      gchar **xlfd_fontname)
{
  if (data->action == GB_LOADING)
    return load_font (data, property, xlfd_fontname);
  else
    return property_get_font (property, data->property_to_apply,
			      &data->apply, xlfd_fontname);
}


gchar*
gb_widget_input_stock_item	(GbWidgetSetArgData	*data,
				 const gchar		*property)
{
  if (data->action == GB_LOADING)
    return load_string (data, property);
  else
    return property_get_stock_item (property, data->property_to_apply,
				    &data->apply);
}


gchar*
gb_widget_input_icon	(GbWidgetSetArgData	*data,
			 const gchar		*property)
{
  if (data->action == GB_LOADING)
    return load_icon (data, property);
  else
    return property_get_icon (property, data->property_to_apply,
			      &data->apply);
}

gchar*
gb_widget_input_named_icon	(GbWidgetSetArgData	*data,
				 const gchar		*property)
{
  if (data->action == GB_LOADING)
    return load_string (data, property);
  else
    return property_get_named_icon (property, data->property_to_apply,
				    &data->apply);
}

gint
gb_widget_input_enum (GbWidgetSetArgData        *data,
		      GType                      enum_type,
		      const char               **labels,
		      int                       *values,
		      const gchar               *property)
{
  gint retval = 0;
  int i;
  char *s;

  if (data->action == GB_LOADING) 
    {
      s = load_string (data, property);
      if (data->apply && s)
	retval = glade_enum_from_string (enum_type, s);
    }
  else
    {
      s = property_get_choice (property, data->property_to_apply,
			       &data->apply);
      if (data->apply && s)
	{
	  for (i = 0; labels[i]; i++)
	    {
	      if (!strcmp (labels[i], s))
		{
		  retval = values[i];
		  break;
		}
	    }
	}
    }
  return retval;
}

/* Outputting Properties - Saving or Showing. */
void
gb_widget_output_string (GbWidgetGetArgData *data,
			 const gchar *property,
			 const gchar *value)
{
  if (data->action == GB_SAVING)
    save_string (data, property, value);
  else
    property_set_string (property, value);
}


void
gb_widget_output_translatable_string (GbWidgetGetArgData *data,
				      const gchar *property,
				      const gchar *value)
{
  if (data->action == GB_SAVING)
    save_translatable_string (data, property, value);
  else
    property_set_translatable_string (property, value, data->widget);
}


void
gb_widget_output_text (GbWidgetGetArgData *data,
		       const gchar *property,
		       const gchar *value)
{
  if (data->action == GB_SAVING)
    save_text (data, property, value);
  else
    property_set_text (property, value);
}


void
gb_widget_output_translatable_text (GbWidgetGetArgData *data,
				    const gchar *property,
				    const gchar *value)
{
  if (data->action == GB_SAVING)
    save_translatable_text (data, property, value);
  else
    property_set_translatable_text (property, value, data->widget);
}


void
gb_widget_output_translatable_text_in_lines (GbWidgetGetArgData *data,
					     const gchar *property,
					     const gchar *value)
{
  if (data->action == GB_SAVING)
    save_translatable_text_in_lines (data, property, value);
  else
    property_set_translatable_text (property, value, data->widget);
}


void
gb_widget_output_int (GbWidgetGetArgData *data,
		      const gchar *property,
		      gint value)
{
#if 0
  if (property == GbX)
    g_print ("X: %i ", value);
  else if (property == GbY)
    g_print ("Y: %i ", value);
  else if (property == GbWidth)
    g_print ("W: %i ", value);
  else if (property == GbHeight)
    g_print ("H: %i ", value);
#endif

  if (data->action == GB_SAVING)
    save_int (data, property, value);
  else
    property_set_int (property, value);
}


void
gb_widget_output_optional_int (GbWidgetGetArgData *data,
			       const gchar *property,
			       gint value,
			       gboolean is_set)
{
  if (data->action == GB_SAVING)
    {
      if (is_set)
	save_int (data, property, value);
    }
  else
    property_set_optional_int (property, value, is_set);
}


void
gb_widget_output_float (GbWidgetGetArgData *data,
			const gchar *property,
			gfloat value)
{
  if (data->action == GB_SAVING)
    save_float (data, property, value);
  else
    property_set_float (property, value);
}


void
gb_widget_output_bool (GbWidgetGetArgData *data,
		       const gchar *property,
		       gint value)
{
  if (data->action == GB_SAVING)
    save_bool (data, property, value);
  else
    property_set_bool (property, value);
}


void
gb_widget_output_choice (GbWidgetGetArgData *data,
			 const gchar *property,
			 gint value,
			 const gchar *symbol)
{
  if (data->action == GB_SAVING)
    save_choice (data, property, symbol);
  else
    property_set_choice (property, value);
}


void
gb_widget_output_combo (GbWidgetGetArgData *data,
			const gchar *property,
			const gchar *value)
{
  if (data->action == GB_SAVING)
    save_combo (data, property, value);
  else
    property_set_combo (property, value);
}


void
gb_widget_output_color (GbWidgetGetArgData *data,
			const gchar *property,
			GdkColor *value)
{
  if (data->action == GB_SAVING)
    save_color (data, property, value);
  else
    property_set_color (property, value);
}


void
gb_widget_output_bgpixmap (GbWidgetGetArgData *data,
			   const gchar *property,
			   GdkPixmap *value,
			   const gchar *filename)
{
  if (data->action == GB_SAVING)
    save_bgpixmap (data, property, filename);
  else
    property_set_bgpixmap (property, value, filename);
}


void
gb_widget_output_dialog (GbWidgetGetArgData *data,
			 const gchar *property,
			 const gchar *string,
			 gconstpointer value)
{
  if (data->action == GB_SAVING)
    save_dialog (data, property, value);
  else
    property_set_dialog (property, string, value);
}


/* FIXME: I think this is broken. It should save it relative to the XML file.*/
void
gb_widget_output_filename (GbWidgetGetArgData *data,
			   const gchar *property,
			   const gchar *value)
{
  if (data->action == GB_SAVING)
    save_filename (data, property, value);
  else
    property_set_filename (property, value);
}


void
gb_widget_output_pixmap_filename (GbWidgetGetArgData *data,
				  const gchar *property,
				  const gchar *value)
{
  if (data->action == GB_SAVING)
    save_pixmap_filename (data, property, value);
  else
    property_set_filename (property, value);
}


void
gb_widget_output_font (GbWidgetGetArgData *data,
		       const gchar *property,
		       GdkFont *value,
		       const gchar *xlfd_fontname)
{
  if (data->action == GB_SAVING)
    save_font (data, property, xlfd_fontname);
  else
    property_set_font (property, value, xlfd_fontname);
}


void
gb_widget_output_stock_item (GbWidgetGetArgData *data,
			     const gchar *property,
			     const gchar *value)
{
  if (data->action == GB_SAVING)
    save_string (data, property, value);
  else
    property_set_stock_item (property, value);
}


void
gb_widget_output_icon (GbWidgetGetArgData *data,
		       const gchar *property,
		       const gchar *value)
{
  if (data->action == GB_SAVING)
    save_icon (data, property, value);
  else
    property_set_icon (property, value);
}

void
gb_widget_output_named_icon (GbWidgetGetArgData *data,
			     const gchar *property,
			     const gchar *value)
{
  if (data->action == GB_SAVING)
    save_string (data, property, value);
  else
    property_set_named_icon (property, value);
}

void
gb_widget_output_enum (GbWidgetGetArgData        *data,
		       GType                      enum_type,
		       int                       *values,
		       int                        n_values,
		       const gchar               *property,
		       gint                       value)
{
  const char *s;
  int i;

  if (data->action == GB_SAVING)
    {
      s = glade_string_from_enum (enum_type, value);
      save_string (data, property, s);
    }
  else
    {
      for (i = 0; i < n_values; i++)
	if (values[i] == value)
	  break;

      property_set_choice (property, i < n_values ? values[i] : 0);
    }
}
