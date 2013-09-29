
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

#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkspinbutton.h>
#include "../gb.h"
#include "../tree.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/notebook.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *ShowTabs = "GtkNotebook::show_tabs";
static gchar *ShowBorder = "GtkNotebook::show_border";
static gchar *TabPos = "GtkNotebook::tab_pos";
static gchar *Scrollable = "GtkNotebook::scrollable";
static gchar *Popups = "GtkNotebook::enable_popup";

#if 0
/* These seem to be deprecated. */
static gchar *TabHBorder = "GtkNotebook::tab_hborder";
static gchar *TabVBorder = "GtkNotebook::tab_vborder";
#endif

/* This isn't save in the XML as it is implicit. */
static gchar *NumPages = "GtkNotebook::num_pages";

/* This isn't save in the XML as it is implicit. */
static gchar *ChildPosition = "GtkNotebook::position";

static gchar *ChildExpand = "GtkNotebook::tab_expand";
static gchar *ChildFill = "GtkNotebook::tab_fill";
static gchar *ChildPack = "GtkNotebook::tab_pack";
static gchar *ChildMenuLabel = "GtkNotebook::menu_label";


static const gchar *GbTabPosChoices[] =
{"Left", "Right", "Top", "Bottom", NULL};
static const gint GbTabPosValues[] =
{
  GTK_POS_LEFT,
  GTK_POS_RIGHT,
  GTK_POS_TOP,
  GTK_POS_BOTTOM
};
static const gchar *GbTabPosSymbols[] =
{
  "GTK_POS_LEFT",
  "GTK_POS_RIGHT",
  "GTK_POS_TOP",
  "GTK_POS_BOTTOM"
};


static void show_notebook_dialog (GbWidgetNewData * data);
static void on_notebook_dialog_ok (GtkWidget * widget,
				   GbWidgetNewData * data);
static void on_notebook_dialog_destroy (GtkWidget * widget,
					GbWidgetNewData * data);
static GtkWidget *gb_notebook_new_tab_label ();
static void gb_notebook_next_page (GtkWidget * menuitem, GtkNotebook * notebook);
static void gb_notebook_prev_page (GtkWidget * menuitem, GtkNotebook * notebook);
static void gb_notebook_update_num_pages (GtkNotebook *notebook);

/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GtkNotebook, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget *
gb_notebook_new (GbWidgetNewData * data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gtk_notebook_new ();
      return new_widget;
    }
  else
    {
      show_notebook_dialog (data);
      return NULL;
    }
}


void
gb_notebook_add_child (GtkWidget *widget, GtkWidget *child,
		       GbWidgetSetArgData *data)
{
  gboolean is_tab = FALSE;

  /* See if this is a tab widget. We use a special "type" packing property set
     to "tab".*/
  if (data->child_info)
    {
      int j;

      for (j = 0; j < data->child_info->n_properties; j++)
	{
	  if (!strcmp (data->child_info->properties[j].name, "type")
	      && !strcmp (data->child_info->properties[j].value, "tab"))
	    {
	      is_tab = TRUE;
	      break;
	    }
	}
    }

  if (is_tab)
    {
      /* We store the last tab read in 'last_child' */
      GtkWidget *notebook_page;
      gint pos = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget),
                                                       "last_child"));

      MSG1 ("Adding notebook tab: %i", pos);
      /* SPECIAL CODE to replace the notebooks default tab label with the
         loaded widget. We remove the page and add it with the new tab,
         just like in gb_widget_replace_child(). */

      notebook_page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), pos);
      if (notebook_page)
	{
	  gtk_notebook_set_tab_label (GTK_NOTEBOOK (widget), notebook_page,
				      child);
	}
      else
	{
	  g_warning ("Notebook tab found for non-existent page");
	  gtk_notebook_append_page (GTK_NOTEBOOK (widget),
				    editor_new_placeholder (), child);
	}
      gtk_object_set_data (GTK_OBJECT (widget), "last_child",
			   GINT_TO_POINTER (pos + 1));
    }
  else
    {
      /* We create a label, in case it does not appear in the XML file,
         or we are pasting into the notebook. */
      GtkWidget *label = gb_widget_new_full ("GtkLabel", FALSE, widget,
                                             NULL, 0, 0, NULL, GB_CREATING,
                                             NULL);
      g_return_if_fail (label != NULL);

      gtk_notebook_append_page (GTK_NOTEBOOK (widget), child, label);
    }
}


static void
show_notebook_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New notebook"), data->parent,
				     GTK_SIGNAL_FUNC (on_notebook_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_notebook_dialog_destroy), data);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Number of pages:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 1, 100, 1, 10, 10);
  spinbutton = glade_util_spin_button_new (GTK_OBJECT (dialog), "pages",
					   GTK_ADJUSTMENT (adjustment), 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, TRUE, TRUE, 5);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  gtk_widget_show (dialog);
  gtk_grab_add (dialog);
}


static void
on_notebook_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window;
  gint pages, i;

  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "pages");
      g_return_if_fail (spinbutton != NULL);
      pages = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gtk_notebook_new ();
      for (i = 0; i < pages; i++)
	{
	  gtk_notebook_append_page (GTK_NOTEBOOK (new_widget),
				    editor_new_placeholder (),
				    gb_notebook_new_tab_label ());
	}
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_notebook_dialog_destroy (GtkWidget * widget,
			    GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


GtkWidget *
gb_notebook_new_tab_label ()
{
  GtkWidget *label;

  label = gb_widget_new ("GtkLabel", NULL);
  g_return_val_if_fail (label != NULL, NULL);
  return label;
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_notebook_create_properties (GtkWidget * widget, GbWidgetCreateArgData * data)
{
  property_add_bool (ShowTabs, _("Show Tabs:"), _("If the notebook tabs are shown"));
  property_add_bool (ShowBorder, _("Show Border:"),
		     _("If the notebook border is shown, when the tabs are not shown"));
  property_add_choice (TabPos, _("Tab Pos:"),
		       _("The position of the notebook tabs"),
		       GbTabPosChoices);
  property_add_bool (Scrollable, _("Scrollable:"),
		     _("If the notebook tabs are scrollable"));
#if 0
/* These seem to be deprecated. */
  property_add_int_range (TabHBorder, _("Tab Horz. Border:"),
			  _("The size of the notebook tabs' horizontal border"),
			  0, 1000, 1, 10, 1);
  property_add_int_range (TabVBorder, _("Tab Vert. Border:"),
			  _("The size of the notebook tabs' vertical border"),
			  0, 1000, 1, 10, 1);
#endif
  property_add_bool (Popups, _("Show Popup:"), _("If the popup menu is enabled"));
  property_add_int_range (NumPages, _("Number of Pages:"),
		          _("The number of notebook pages"),
		          1, 100, 1, 10, 1);
}



/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_notebook_get_properties (GtkWidget * widget, GbWidgetGetArgData * data)
{
  gint tab_pos, i;

  gb_widget_output_bool (data, ShowTabs, GTK_NOTEBOOK (widget)->show_tabs);
  gb_widget_output_bool (data, ShowBorder, GTK_NOTEBOOK (widget)->show_border);

  tab_pos = GTK_NOTEBOOK (widget)->tab_pos;
  for (i = 0; i < sizeof (GbTabPosValues) / sizeof (GbTabPosValues[0]); i++)
    {
      if (GbTabPosValues[i] == tab_pos)
	gb_widget_output_choice (data, TabPos, i, GbTabPosSymbols[i]);
    }

  gb_widget_output_bool (data, Scrollable, GTK_NOTEBOOK (widget)->scrollable);

#if 0
/* These seem to be deprecated. */
  gb_widget_output_int (data, TabHBorder, GTK_NOTEBOOK (widget)->tab_hborder);
  gb_widget_output_int (data, TabVBorder, GTK_NOTEBOOK (widget)->tab_vborder);
#endif

  gb_widget_output_bool (data, Popups,
			 (GTK_NOTEBOOK (widget)->menu) ? TRUE : FALSE);

  /* Don't save the number of pages. */
  if (data->action != GB_SAVING)
    gb_widget_output_int (data, NumPages,
			  g_list_length (GTK_NOTEBOOK (widget)->children));
}



/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_notebook_set_properties (GtkWidget * widget, GbWidgetSetArgData * data)
{
  gboolean show_tabs, show_border, scrollable, popups;
  gchar *tab_pos;
  gint i, num_pages;
  GtkWidget *new_label;

  show_tabs = gb_widget_input_bool (data, ShowTabs);
  if (data->apply)
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (widget), show_tabs);

  show_border = gb_widget_input_bool (data, ShowBorder);
  if (data->apply)
    gtk_notebook_set_show_border (GTK_NOTEBOOK (widget), show_border);

  tab_pos = gb_widget_input_choice (data, TabPos);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbTabPosValues) / sizeof (GbTabPosValues[0]); i
	   ++)
	{
	  if (!strcmp (tab_pos, GbTabPosChoices[i])
	      || !strcmp (tab_pos, GbTabPosSymbols[i]))
	    {
	      gtk_notebook_set_tab_pos (GTK_NOTEBOOK (widget), GbTabPosValues
					[i]);
	      break;
	    }
	}
    }

  scrollable = gb_widget_input_bool (data, Scrollable);
  if (data->apply)
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (widget), scrollable);

#if 0
/* These seem to be deprecated. */
  tab_border = gb_widget_input_int (data, TabHBorder);
  if (data->apply)
    gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (widget), tab_border);

  tab_border = gb_widget_input_int (data, TabVBorder);
  if (data->apply)
    gtk_notebook_set_tab_vborder (GTK_NOTEBOOK (widget), tab_border);
#endif

  popups = gb_widget_input_bool (data, Popups);
  if (data->apply)
    {
      if (popups)
	gtk_notebook_popup_enable (GTK_NOTEBOOK (widget));
      else
	gtk_notebook_popup_disable (GTK_NOTEBOOK (widget));
    }
  
  /* Don't adjust the size of a notebook if loading a project 
   * as it is handled by other routines. */
  if (data->action != GB_LOADING)
    {
      num_pages = gb_widget_input_int (data, NumPages);
      if (data->apply)
	{
	  if (num_pages != g_list_length (GTK_NOTEBOOK (widget)->children))
	    {
	      if (num_pages > g_list_length (GTK_NOTEBOOK (widget)->children))
		{
		  while (num_pages > g_list_length (GTK_NOTEBOOK (widget)->children))
		    {
		      new_label = gb_notebook_new_tab_label ();
		      gtk_notebook_append_page (GTK_NOTEBOOK (widget),
						editor_new_placeholder (),
						new_label);
		      tree_add_widget (new_label);
		    }
		}
	      else
		{
		  while (num_pages < g_list_length (GTK_NOTEBOOK (widget)->children))
		    {
		      gtk_notebook_remove_page (GTK_NOTEBOOK (widget),
						num_pages);
		    }
		}
	    }
	}
    }
}



static void
gb_notebook_next_page (GtkWidget * menuitem, GtkNotebook * notebook)
{
  gtk_notebook_next_page (notebook);
}

static void
gb_notebook_prev_page (GtkWidget * menuitem, GtkNotebook * notebook)
{
  gtk_notebook_prev_page (notebook);
}

static void
gb_notebook_switch_prev (GtkWidget *menuitem, GtkNotebook *notebook)
{
  GtkWidget       *child;
  gint             current_page;

  current_page = gtk_notebook_get_current_page (notebook);
  child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), current_page);

  if (child)
    {
      gtk_notebook_reorder_child (notebook, child, current_page - 1);
    }
}

static void
gb_notebook_switch_next (GtkWidget *menuitem, GtkNotebook *notebook)
{
  GtkWidget       *child;
  gint             current_page;

  current_page = gtk_notebook_get_current_page (notebook);
  child = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), current_page);

  if (child)
    {
      gtk_notebook_reorder_child (notebook, child, current_page + 1);
    }
}

static void
gb_notebook_insert_next (GtkWidget *menuitem, GtkNotebook *notebook)
{
  gint current_page;
  GtkWidget *new_label;

  current_page = gtk_notebook_get_current_page (notebook);

  new_label = gb_notebook_new_tab_label ();
  gtk_notebook_insert_page (notebook, editor_new_placeholder (),
			    new_label, current_page + 1);
  tree_add_widget (new_label);
  gb_notebook_update_num_pages (notebook);
}

static void
gb_notebook_insert_prev (GtkWidget *menuitem, GtkNotebook *notebook)
{
  gint current_page;
  GtkWidget *new_label;
	
  current_page = gtk_notebook_get_current_page (notebook);

  new_label = gb_notebook_new_tab_label ();
  gtk_notebook_insert_page (notebook, editor_new_placeholder (),
			    new_label, current_page);
  tree_add_widget (new_label);
  gb_notebook_update_num_pages (notebook);
}

static void
gb_notebook_delete_page (GtkWidget *menuitem, GtkNotebook *notebook)
{
  gtk_notebook_remove_page (notebook,
			    gtk_notebook_get_current_page (notebook));
  gb_notebook_update_num_pages (notebook);
}

 
/* This updates the number of pages property, if the notebook's properties are
   currently shown. */
static void
gb_notebook_update_num_pages (GtkNotebook *notebook)
{
  if (property_get_widget () == GTK_WIDGET (notebook))
    {
      property_set_auto_apply (FALSE);
      property_set_int (NumPages, g_list_length (notebook->children));
      property_set_auto_apply (TRUE);
    }
}

/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkNotebook, with signals pointing to
 * other functions in this file.
 */
static void
gb_notebook_create_popup_menu (GtkWidget * widget, GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;
  gint current_page, num_pages;

  current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (widget));
  num_pages = g_list_length (GTK_NOTEBOOK (widget)->children);

  menuitem = gtk_menu_item_new_with_label (_("Previous Page"));
  gtk_widget_show (menuitem);
  if (current_page == 0)
    gtk_widget_set_sensitive (menuitem, FALSE);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_prev_page), GTK_NOTEBOOK (widget));

  menuitem = gtk_menu_item_new_with_label (_("Next Page"));
  gtk_widget_show (menuitem);
  if (current_page == num_pages - 1)
    gtk_widget_set_sensitive (menuitem, FALSE);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_next_page), GTK_NOTEBOOK (widget));
  
  menuitem = gtk_menu_item_new_with_label (_("Delete Page"));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_delete_page), GTK_NOTEBOOK (widget));
  
  menuitem = gtk_menu_item_new_with_label (_("Switch Next"));
  gtk_widget_show (menuitem);
  if (current_page == num_pages - 1)
    gtk_widget_set_sensitive (menuitem, FALSE);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_switch_next), GTK_NOTEBOOK (widget));
  
  menuitem = gtk_menu_item_new_with_label (_("Switch Previous"));
  gtk_widget_show (menuitem);
  if (current_page == 0)
    gtk_widget_set_sensitive (menuitem, FALSE);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_switch_prev), GTK_NOTEBOOK (widget));
  
  menuitem = gtk_menu_item_new_with_label (_("Insert Page After"));
  gtk_widget_show (menuitem);
  /*  if (current_page == num_pages - 1)
      gtk_widget_set_sensitive (menuitem, FALSE);*/
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_insert_next), GTK_NOTEBOOK (widget));
  
  menuitem = gtk_menu_item_new_with_label (_("Insert Page Before"));
  gtk_widget_show (menuitem);
  /*if (current_page == 0)
    gtk_widget_set_sensitive (menuitem, FALSE);*/
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
	    GTK_SIGNAL_FUNC (gb_notebook_insert_prev), GTK_NOTEBOOK (widget));
}



/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_notebook_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gint i;

  if (data->create_widget)
    {
      source_add (data, "  %s = gtk_notebook_new ();\n", data->wname);
    }

  /* We reset the last_child index, so as the tab widgets are written out
     they will start at page 0. */
  gtk_object_set_data (GTK_OBJECT (widget), "last_child",
		       GINT_TO_POINTER (-1));

  gb_widget_write_standard_source (widget, data);

  if (!GTK_NOTEBOOK (widget)->show_tabs)
    source_add (data,
		"  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (%s), FALSE);\n",
		data->wname);
  if (!GTK_NOTEBOOK (widget)->show_border)
    source_add (data,
		"  gtk_notebook_set_show_border (GTK_NOTEBOOK (%s), FALSE);\n",
		data->wname);
  if (GTK_NOTEBOOK (widget)->tab_pos != GTK_POS_TOP)
    {
      for (i = 0; i < sizeof (GbTabPosValues) / sizeof (GbTabPosValues[0]); i
	   ++)
	{
	  if (GbTabPosValues[i] == GTK_NOTEBOOK (widget)->tab_pos)
	    source_add (data,
		    "  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (%s), %s);\n",
			data->wname, GbTabPosSymbols[i]);
	}
    }
  if (GTK_NOTEBOOK (widget)->scrollable)
    source_add (data,
		"  gtk_notebook_set_scrollable (GTK_NOTEBOOK (%s), TRUE);\n",
		data->wname);

#if 0
/* These seem to be deprecated. */
  if (GTK_NOTEBOOK (widget)->tab_hborder != 2)
    source_add (data,
		"  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (%s), %i);\n",
		data->wname, GTK_NOTEBOOK (widget)->tab_hborder);
  if (GTK_NOTEBOOK (widget)->tab_vborder != 2)
    source_add (data,
		"  gtk_notebook_set_tab_vborder (GTK_NOTEBOOK (%s), %i);\n",
		data->wname, GTK_NOTEBOOK (widget)->tab_vborder);
#endif

  if (GTK_NOTEBOOK (widget)->menu)
    source_add (data,
		"  gtk_notebook_popup_enable (GTK_NOTEBOOK (%s));\n",
		data->wname);
}


/*
 * Creates the child packing properties for children of this widget.
 */
void
gb_notebook_create_child_properties (GtkWidget * widget,
				     GbWidgetCreateChildArgData * data)
{
  property_add_int_range (ChildPosition, _("Position:"),
			  _("The page's position in the list of pages"),
			  0, 10000, 1, 10, 1);
  property_add_bool (ChildExpand, _("Expand:"),
		     _("Set True to let the tab expand"));
  property_add_bool (ChildFill, _("Fill:"),
		     _("Set True to let the tab fill its allocated area"));
  property_add_bool (ChildPack, _("Pack Start:"),
		     _("Set True to pack the tab at the start of the notebook"));
  property_add_string (ChildMenuLabel, _("Menu Label:"),
		     _("The text to display in the popup menu"));
}


static gboolean
gb_notebook_get_child_info (GtkNotebook *notebook, GtkWidget *child,
			    GtkWidget **notebook_child,
			    GtkWidget **notebook_tab,
			    gint *position, gboolean *expand, gboolean *fill,
			    GtkPackType *pack_type)
{
  gint nchildren, page;

  nchildren = g_list_length (notebook->children);

  for (page = 0; page < nchildren; page++)
    {
      *notebook_child = gtk_notebook_get_nth_page (notebook, page);
      *notebook_tab = gtk_notebook_get_tab_label (notebook, *notebook_child);

      if (*notebook_child == child || *notebook_tab == child)
	{
	  gtk_notebook_query_tab_label_packing (notebook, *notebook_child,
						expand, fill, pack_type);
	  *position = page;
	  return TRUE;
	}
    }

  return FALSE;
}


void
gb_notebook_get_child_properties (GtkWidget *widget, GtkWidget *child,
				  GbWidgetGetArgData *data)
{
  GtkWidget *notebook_child, *notebook_tab;
  gboolean expand, fill;
  gint position;
  GtkPackType pack_type;
  const gchar *menu_label;

  /* If we can't find the child, output a warning and return. */
  if (!gb_notebook_get_child_info (GTK_NOTEBOOK (widget), child,
				   &notebook_child, &notebook_tab,
				   &position, &expand, &fill, &pack_type))
    {
      g_warning ("Notebook child not found");
      return;
    }

  /* If we are saving, we save the packing properties with the main child
     widget, and not with the tab label. But we show the packing properties
     for the tab label as well, as that is easier for the user.
     For the tab label we save the special "type" packing property. */
  if (data->action == GB_SAVING && child == notebook_tab)
    {
      save_start_tag (data, "packing");
      gb_widget_output_string (data, "type", "tab");
      save_end_tag (data, "packing");
      return;
    }

  if (data->action == GB_SAVING)
    save_start_tag (data, "packing");

  if (data->action == GB_SHOWING)
    gb_widget_output_int (data, ChildPosition, position);

  gb_widget_output_bool (data, ChildExpand, expand);
  gb_widget_output_bool (data, ChildFill, fill);

  if (data->action == GB_SAVING)
    {
      /* Save pack type as an enum symbol rather than a bool */
      if (pack_type == GTK_PACK_END)
	{
	  gb_widget_output_string (data, ChildPack, "GTK_PACK_END");
	}
    }
  else
    {
      gb_widget_output_bool (data, ChildPack, pack_type == GTK_PACK_START);
    }

  menu_label = gtk_notebook_get_menu_label_text (GTK_NOTEBOOK (widget),
						 notebook_child);
  gb_widget_output_translatable_string (data, ChildMenuLabel, menu_label);

  if (data->action == GB_SAVING)
    save_end_tag (data, "packing");
}


/* Applies or loads the child properties of a child of a hbox/vbox. */
void
gb_notebook_set_child_properties (GtkWidget *widget, GtkWidget *child,
				  GbWidgetSetArgData *data)
{
  GtkWidget *notebook_child, *notebook_tab;
  gint position, old_position;
  gboolean expand, fill, pack, set_child_packing = FALSE;
  gboolean old_expand, old_fill;
  GtkPackType old_pack_type;
  char *menu_label;

  /* If we can't find the child, output a warning and return. */
  if (!gb_notebook_get_child_info (GTK_NOTEBOOK (widget), child,
				   &notebook_child, &notebook_tab,
				   &old_position, &old_expand, &old_fill,
				   &old_pack_type))
    {
      g_warning ("Notebook child not found");
      return;
    }

  /* If we are loading, don't set the packing properties for the tab. */
  if (data->action == GB_LOADING && child == notebook_tab)
    return;

  position = gb_widget_input_int (data, ChildPosition);
  if (data->apply)
    {
      gtk_notebook_reorder_child (GTK_NOTEBOOK (widget), notebook_child,
				  position);
    }

  expand = gb_widget_input_bool (data, ChildExpand);
  if (data->apply)
    set_child_packing = TRUE;
  else
    expand = old_expand;

  fill = gb_widget_input_bool (data, ChildFill);
  if (data->apply)
    set_child_packing = TRUE;
  else
    fill = old_fill;

  if (data->action == GB_APPLYING)
    {
      pack = gb_widget_input_bool (data, ChildPack);
    }
  else
    {
      gchar *pack_symbol = gb_widget_input_string (data, ChildPack);
      pack = pack_symbol && !strcmp (pack_symbol, "GTK_PACK_START");
    }
  if (data->apply)
    set_child_packing = TRUE;
  else
    pack = (old_pack_type == GTK_PACK_START) ? TRUE : FALSE;

  if (set_child_packing)
    gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (widget), notebook_child,
					expand, fill,
					pack ? GTK_PACK_START : GTK_PACK_END);

  menu_label = gb_widget_input_string (data, ChildMenuLabel);
  if (data->apply)
    {
      gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (widget), notebook_child,
					menu_label);
    }
}

/* Outputs source to add a child widget to a GtkNotebook. */
static void
gb_notebook_write_add_child_source (GtkWidget * parent,
				    const gchar *parent_name,
				    GtkWidget *child,
				    GbWidgetWriteSourceData * data)
{
  GtkWidget *notebook_child, *notebook_tab;
  gboolean expand, fill;
  gint position;
  GtkPackType pack_type;

  if (!gb_notebook_get_child_info (GTK_NOTEBOOK (parent), child,
				   &notebook_child, &notebook_tab,
				   &position, &expand, &fill, &pack_type))
    {
      g_warning ("Notebook child not found");
      return;
    }

  /* See if this is a notebook tab widget. */
  if (child == notebook_tab)
    {
      /* We store the last tab written in 'last_child' */
      gint col = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (parent),
						       "last_child"));
      /* We use the special function to add the tab to the notebook. */
      source_add (data,
		  "  gtk_notebook_set_tab_label (GTK_NOTEBOOK (%s), gtk_notebook_get_nth_page (GTK_NOTEBOOK (%s), %i), %s);\n",
		  parent_name, parent_name, col + 1, data->wname);

      gtk_object_set_data (GTK_OBJECT (parent), "last_child",
			   GINT_TO_POINTER (col + 1));
    }
  else
    {
      const gchar *menu_label;

      source_add (data, "  gtk_container_add (GTK_CONTAINER (%s), %s);\n",
		  parent_name, data->wname);

      /* Set the packing properties, if different to the defaults. */
      if (expand || !fill || pack_type != GTK_PACK_START)
	{
	  source_add (data,
	"  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (%s), %s,\n"
	"                                      %s, %s, %s);\n",
		      parent_name, data->wname,
		      expand ? "TRUE" : "FALSE",
		      fill ? "TRUE" : "FALSE",
		      pack_type == GTK_PACK_START ? "GTK_PACK_START" : "GTK_PACK_END");
	}

      menu_label = gtk_notebook_get_menu_label_text (GTK_NOTEBOOK (parent),
						     notebook_child);
      if (menu_label && *menu_label)
	{
	  gboolean translatable, context;
	  gchar *comments;

	  glade_util_get_translation_properties (notebook_child,
						 ChildMenuLabel, &translatable,
						 &comments, &context);
	  source_add_translator_comments (data, translatable, comments);

	  source_add (data,
	"  gtk_notebook_set_menu_label_text (GTK_NOTEBOOK (%s), %s, %s);\n",
		      parent_name, data->wname,
		      source_make_string_full (menu_label, data->use_gettext && translatable, context));
	}
    }
}


/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_notebook_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_notebook_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = notebook_xpm;
  gbwidget.tooltip = _("Notebook");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new = gb_notebook_new;
  gbwidget.gb_widget_add_child = gb_notebook_add_child;

  gbwidget.gb_widget_create_properties = gb_notebook_create_properties;
  gbwidget.gb_widget_get_properties = gb_notebook_get_properties;
  gbwidget.gb_widget_set_properties = gb_notebook_set_properties;
  gbwidget.gb_widget_write_source = gb_notebook_write_source;
  gbwidget.gb_widget_create_child_properties = gb_notebook_create_child_properties;
  gbwidget.gb_widget_get_child_properties = gb_notebook_get_child_properties;
  gbwidget.gb_widget_set_child_properties = gb_notebook_set_child_properties;
  gbwidget.gb_widget_write_add_child_source = gb_notebook_write_add_child_source;
  gbwidget.gb_widget_create_popup_menu = gb_notebook_create_popup_menu;

  return &gbwidget;
}
