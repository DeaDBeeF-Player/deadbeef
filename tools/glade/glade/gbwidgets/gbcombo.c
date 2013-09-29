
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

#include <gtk/gtkcombo.h>
#include <gtk/gtklabel.h>
#include <gtk/gtklist.h>
#include "../gb.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/combo.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

/* These 2 are stored in the widget's datalist since otherwise GtkCombo starts
   to grab the pointer if invalid values are entered which causes problems. */
static gchar *ValueInList = "GtkCombo::value_in_list";
static gchar *OKIfEmpty = "GtkCombo::allow_empty";

static gchar *Case = "GtkCombo::case_sensitive";
static gchar *Arrows = "GtkCombo::enable_arrow_keys";
static gchar *Always = "GtkCombo::enable_arrows_always";
static gchar *Items = "GtkCombo::items";

static void add_label (GtkWidget * widget, GString * items);

static gint is_simple_combo (GtkWidget * widget);
static void is_simple_combo_callback (GtkWidget * widget,
				      gint * num_children);
static void write_items_source_callback (GtkWidget * item,
					 GbWidgetWriteSourceData * data);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkCombo, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget*
gb_combo_new(GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_combo_new ();

  /* There is no way to move up from the popup list window to the combo, so
     we have to add a pointer here. It is used in glade_util_get_parent ().
     We don't add a ref to it, in case it messes up the normal widget code. */
  gtk_object_set_data (GTK_OBJECT (GTK_COMBO (new_widget)->popwin),
		       GladeParentKey, new_widget);

  gb_widget_create_from (GTK_COMBO (new_widget)->entry,
			 data->action == GB_CREATING ? "combo-entry" : NULL);
  gb_widget_set_child_name (GTK_COMBO (new_widget)->entry, GladeChildComboEntry);

  gb_widget_create_from (GTK_COMBO (new_widget)->list,
			 data->action == GB_CREATING ? "combo-list" : NULL);
  gb_widget_set_child_name (GTK_COMBO (new_widget)->list, GladeChildComboList);

  /* This defaults to TRUE. */
  gtk_object_set_data (GTK_OBJECT (new_widget), OKIfEmpty, "TRUE");

  /* FIXME: GTK+ 1.3 temporary hack to workaround problems. */
  gtk_signal_handler_block (GTK_OBJECT (GTK_COMBO (new_widget)->entry),
			    GTK_COMBO (new_widget)->entry_change_id);

  return new_widget;
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_combo_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (ValueInList, _("Value In List:"),
		     _("If the value must be in the list"));
  property_add_bool (OKIfEmpty, _("OK If Empty:"),
		     _("If an empty value is acceptable, when 'Value In List' is set"));
  property_add_bool (Case, _("Case Sensitive:"),
		     _("If the searching is case sensitive"));
  property_add_bool (Arrows, _("Use Arrows:"),
		     _("If arrows can be used to change the value"));
  property_add_bool (Always, _("Use Always:"),
		     _("If arrows work even if the value is not in the list"));
  property_add_text (Items, _("Items:"),
		     _("The items in the combo list, one per line"), 5);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_combo_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gb_widget_output_bool (data, ValueInList,
			 gtk_object_get_data (GTK_OBJECT (widget), ValueInList)
			 != NULL ? TRUE : FALSE);
  gb_widget_output_bool (data, OKIfEmpty,
			 gtk_object_get_data (GTK_OBJECT (widget), OKIfEmpty)
			 != NULL ? TRUE : FALSE);
  gb_widget_output_bool (data, Case, GTK_COMBO (widget)->case_sensitive);
  gb_widget_output_bool (data, Arrows, GTK_COMBO (widget)->use_arrows);
  gb_widget_output_bool (data, Always, GTK_COMBO (widget)->use_arrows_always);

  if (data->action == GB_SHOWING && is_simple_combo (widget) >= 0)
    {
      GString *items;

      items = g_string_new ("");
      gtk_container_foreach (GTK_CONTAINER (GTK_COMBO (widget)->list),
			     (GtkCallback) add_label, items);
      gb_widget_output_translatable_text_in_lines (data, Items, items->str);
      g_string_free (items, TRUE);
    }
}

/* Adds combo label text to buffer, or sets first char to -1 if it won't fit.
 */
static void
add_label (GtkWidget * widget, GString *items)
{
  const gchar *label_text;

  label_text = gtk_label_get_text (GTK_LABEL (GTK_BIN (widget)->child));
  g_string_append (items, label_text);
  g_string_append_c (items, '\n');
}


/* Returns number of items in combo list, or -1 if not a simple combo (not
   all children are labels). */
static gint
is_simple_combo (GtkWidget * widget)
{
  gint num_children = 0;

  gtk_container_foreach (GTK_CONTAINER (GTK_COMBO (widget)->list),
		     (GtkCallback) is_simple_combo_callback, &num_children);
  return num_children;
}


/* Sets flag to FALSE if list item's child widget is not a label. */
static void
is_simple_combo_callback (GtkWidget * widget, gint * num_children)
{
  /* If we've already found an item which isn't a simple label, just return. */
  if (*num_children == -1)
    return;

  /* If the items isn't a simple label, set num_children to -1, else add 1. */
  if (!GTK_IS_LABEL (GTK_BIN (widget)->child))
    *num_children = -1;
  else
    *num_children += 1;
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_combo_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean value_in_list, ok_if_empty, case_sensitive, arrows, arrows_always;
  gchar *items;

  value_in_list = gb_widget_input_bool (data, ValueInList);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), ValueInList,
			   value_in_list ? "TRUE" : NULL);
    }

  ok_if_empty = gb_widget_input_bool (data, OKIfEmpty);
  if (data->apply)
    {
      gtk_object_set_data (GTK_OBJECT (widget), OKIfEmpty,
			   ok_if_empty ? "TRUE" : NULL);
    }

  case_sensitive = gb_widget_input_bool (data, Case);
  if (data->apply)
    gtk_combo_set_case_sensitive (GTK_COMBO (widget), case_sensitive);
  arrows = gb_widget_input_bool (data, Arrows);
  if (data->apply)
    gtk_combo_set_use_arrows (GTK_COMBO (widget), arrows);
  arrows_always = gb_widget_input_bool (data, Always);
  if (data->apply)
    gtk_combo_set_use_arrows_always (GTK_COMBO (widget), arrows_always);

  if (data->action == GB_APPLYING)
    {
      items = gb_widget_input_text (data, Items);
      if (data->apply)
	{
	  GtkWidget *list = GTK_COMBO (widget)->list;
	  GtkWidget *listitem;
	  gchar *pos = items;
	  gchar *items_end = &items[strlen (items)];

	  gtk_list_clear_items (GTK_LIST (list), 0, -1);

	  while (pos < items_end)
	    {
	      gchar *item_end = strchr (pos, '\n');
	      if (item_end == NULL)
		item_end = items_end;
	      *item_end = '\0';

	      listitem = gb_widget_new ("GtkListItem", list);
	      gtk_label_set_text (GTK_LABEL (GTK_BIN (listitem)->child), pos);
	      gtk_widget_show (listitem);
	      gtk_container_add (GTK_CONTAINER (list), listitem);

	      if (item_end != items_end)
		*item_end = '\n';

	      pos = item_end + 1;
	    }

	  g_free (items);
	}
    }
}



/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkCombo, with signals pointing to
 * other functions in this file.
 */
/*
   static void
   gb_combo_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
   {

   }
 */



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_combo_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gchar *wname, *child_name;
  gboolean value_in_list, ok_if_empty;
  gboolean translatable, context;
  gchar *comments;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_combo_new ();\n", data->wname);
    }

  /* We store a pointer to the popup window's parent combo, so lookup_widget()
     will still work when the popup window is passed as the first arg. */
  source_add (data,
	      "  g_object_set_data (G_OBJECT (GTK_COMBO (%s)->popwin),\n"
	      "                     \"GladeParentKey\", %s);\n",
	      data->wname, data->wname);

  gb_widget_write_standard_source (widget, data);

  value_in_list = gtk_object_get_data (GTK_OBJECT (widget), ValueInList)
    != NULL ? TRUE : FALSE;
  if (value_in_list)
    {
      ok_if_empty = gtk_object_get_data (GTK_OBJECT (widget), OKIfEmpty)
	!= NULL ? TRUE : FALSE;
      source_add (data,
		  "  gtk_combo_set_value_in_list (GTK_COMBO (%s), %s, %s);\n",
		  data->wname,
		  value_in_list ? "TRUE" : "FALSE",
		  ok_if_empty ? "TRUE" : "FALSE");
    }


  if (GTK_COMBO (widget)->case_sensitive)
    {
      source_add (data,
		  "  gtk_combo_set_case_sensitive (GTK_COMBO (%s), TRUE);\n",
		  data->wname);
    }
  if (!GTK_COMBO (widget)->use_arrows)
    {
      source_add (data,
		  "  gtk_combo_set_use_arrows (GTK_COMBO (%s), FALSE);\n",
		  data->wname);
    }
  if (GTK_COMBO (widget)->use_arrows_always)
    {
      source_add (data,
		  "  gtk_combo_set_use_arrows_always (GTK_COMBO (%s), TRUE);\n",
		  data->wname);
    }

  if (is_simple_combo (widget) > 0)
    {
      gboolean old_use_gettext;

      source_add_decl (data, "  GList *%s_items = NULL;\n", data->real_wname);

      glade_util_get_translation_properties (widget, Items, &translatable,
					     &comments, &context);

      /* Temporarily set data->use_gettext to take the translatable flag for
	 this property into account, so our callback can use that. */
      old_use_gettext = data->use_gettext;
      data->use_gettext = data->use_gettext && translatable ? TRUE : FALSE;
      gtk_container_foreach (GTK_CONTAINER (GTK_COMBO (widget)->list),
			   (GtkCallback) write_items_source_callback, data);
      data->use_gettext = old_use_gettext;

      source_add (data,
		  "  gtk_combo_set_popdown_strings (GTK_COMBO (%s), %s_items);\n",
		  data->wname, data->real_wname);
      source_add (data, "  g_list_free (%s_items);\n", data->real_wname);
    }


  /* We output the source code for the children here, since the code should
     not include calls to create the widgets. We need to specify that the
     names used are like: "GTK_COMBO (<combo-name>)->entry".
     We need to remember the dialog's name since data->wname
     will be overwritten. */
  wname = g_strdup (data->wname);

  source_add (data, "\n");
  child_name = (gchar*) gtk_widget_get_name (GTK_COMBO (widget)->entry);
  child_name = source_create_valid_identifier (child_name);
  source_add (data, "  %s = GTK_COMBO (%s)->entry;\n",
	      child_name, wname);
  g_free (child_name);
  data->create_widget = FALSE;
  gb_widget_write_source (GTK_COMBO (widget)->entry, data);

  g_free (wname);
  data->write_children = FALSE;
}


static void
write_items_source_callback (GtkWidget * item, GbWidgetWriteSourceData * data)
{
  const gchar *label_text;
  label_text = gtk_label_get_text (GTK_LABEL (GTK_BIN (item)->child));
  /* The (gpointer) cast is just to keep g++ happy. */
  source_add (data, "  %s_items = g_list_append (%s_items, (gpointer) %s);\n",
	      data->real_wname, data->real_wname,
	      source_make_string (label_text, data->use_gettext));
}


static GtkWidget *
gb_combo_get_child (GtkWidget * widget,
		    const gchar * child_name)
{
  if (!strcmp (child_name, GladeChildComboEntry))
    return GTK_COMBO (widget)->entry;
  else if (!strcmp (child_name, GladeChildComboList))
    return GTK_COMBO (widget)->list;
  else
    return NULL;
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_combo_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_combo_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = combo_xpm;
  gbwidget.tooltip = _("Combo Box");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new               = gb_combo_new;
  gbwidget.gb_widget_create_properties = gb_combo_create_properties;
  gbwidget.gb_widget_get_properties = gb_combo_get_properties;
  gbwidget.gb_widget_set_properties = gb_combo_set_properties;
  gbwidget.gb_widget_get_child = gb_combo_get_child;
  gbwidget.gb_widget_write_source = gb_combo_write_source;
/*
   gbwidget.gb_widget_create_popup_menu = gb_combo_create_popup_menu;
 */

  return &gbwidget;
}
