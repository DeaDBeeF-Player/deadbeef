
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

#include <gtk/gtkbutton.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkstock.h>
#include "../gb.h"
#include "../glade_gnome.h"
#include "../glade_menu_editor.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/menubar.xpm"

/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;

static gchar *PackDirection = "GtkMenuBar::pack_direction";
static gchar *ChildPackDirection = "GtkMenuBar::child_pack_direction";


static const gchar *GbPackDirectionChoices[] =
{
  "Left to Right",
  "Right to Left",
  "Top to Bottom",
  "Bottom to Top",
  NULL
};
static const gint GbPackDirectionValues[] =
{
  GTK_PACK_DIRECTION_LTR,
  GTK_PACK_DIRECTION_RTL,
  GTK_PACK_DIRECTION_TTB,
  GTK_PACK_DIRECTION_BTT
};
static const gchar *GbPackDirectionSymbols[] =
{
  "GTK_PACK_DIRECTION_LTR",
  "GTK_PACK_DIRECTION_RTL",
  "GTK_PACK_DIRECTION_TTB",
  "GTK_PACK_DIRECTION_BTT"
};

static void on_menu_bar_size_request (GtkWidget * widget,
				      GtkRequisition *requisition,
				      gpointer data);
static void gb_menu_bar_on_edit_menu (GtkWidget *button,
				      gpointer data);
static void gb_menu_bar_on_edit_menu_activate (GtkWidget *menuitem,
					       GtkWidget *menubar);



/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/


static GtkWidget*
gb_menu_bar_add_menu (GtkWidget *menu, const gchar *label)
{
  GtkWidget *menuitem, *child_menu;

  menuitem = gb_widget_new ("GtkMenuItem", NULL);
  gtk_label_set_text_with_mnemonic (GTK_LABEL (GTK_BIN (menuitem)->child),
				    label);
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);

  child_menu = gb_widget_new ("GtkMenu", NULL);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), child_menu);

  return child_menu;
}


/* This returns the name to use given the widget's label. Spaces are converted
   to underscores, and '.' and '_' chars are ignore, e.g. '_Open...' -> 'Open'.
   The returned string should be freed. */
static char*
gb_menu_bar_generate_item_name (const gchar *label)
{
  char *name, *dest;
  const char *src;

  /* We never add characters, so name is never longer than label. I think this
   is OK for UTF-8. */
  name = g_malloc (strlen (label) + 1);
  for (src = label, dest = name; *src; src++)
    {
      if (*src == ' ')
	*dest++ = '_';
      else if (*src == '.')
	continue;
      else if (*src == '_')
	continue;
      else
	*dest++ = *src;
    }
  *dest = '\0';

  return name;
}


/* This adds an "activate" signal handler to the menuitem. */
static void
gb_menu_bar_add_item_handler (GtkWidget *menuitem)
{
  GladeWidgetData *wdata;
  GladeSignal *signal;
  const char *name;

  wdata = (GladeWidgetData*) gtk_object_get_data (GTK_OBJECT(menuitem),
						  GB_WIDGET_DATA_KEY);
  if (wdata == NULL)
    {
      g_warning ("Widget has no GladeWidgetData attached");
      return;
    }


  name = gtk_widget_get_name (menuitem);

  signal = g_new (GladeSignal, 1);
  signal->name = g_strdup ("activate");
  signal->handler = g_strdup_printf ("on_%s_activate", name);
  signal->object = NULL;
  signal->after = FALSE;
  signal->data = NULL;
  signal->last_modification_time = time (NULL);
  wdata->signals = g_list_append (wdata->signals, signal);
}

static void
gb_menu_bar_add_stock_item (GtkWidget *menu, const gchar *stock_id)
{
  GtkWidget *menuitem;
  char *label_text, *name;

  menuitem = gtk_image_menu_item_new_from_stock (stock_id, NULL);
  label_text = glade_util_get_label_text (GTK_BIN (menuitem)->child);
  name = gb_menu_bar_generate_item_name (label_text);
  gb_widget_create_from (menuitem, name);
  g_free (name);
  g_free (label_text);
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);

  /* For stock items we also have to store the stock_id. */
  gtk_object_set_data_full (GTK_OBJECT (menuitem), GladeMenuItemStockIDKey,
			    g_strdup (stock_id), g_free);

  gb_menu_bar_add_item_handler (menuitem);
}


static void
gb_menu_bar_add_item (GtkWidget *menu, const gchar *label)
{
  GtkWidget *menuitem;
  char *name;

  menuitem = gtk_menu_item_new_with_mnemonic (label);
  name = gb_menu_bar_generate_item_name (label);
  gb_widget_create_from (menuitem, name);
  g_free (name);
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);

  gb_menu_bar_add_item_handler (menuitem);
}


static void
gb_menu_bar_add_separator (GtkWidget *menu)
{
  GtkWidget *menuitem;

  menuitem = gb_widget_new ("GtkSeparatorMenuItem", NULL);
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
}


static void
gb_menu_bar_setup_initial_menus (GtkWidget *widget)
{
  GtkWidget *menu;

  /* FIXME: I'm not sure if we should translate the non-stock labels or not. */
  menu = gb_menu_bar_add_menu (widget, _("_File"));
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_NEW);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_OPEN);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_SAVE);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_SAVE_AS);
  gb_menu_bar_add_separator (menu);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_QUIT);

  menu = gb_menu_bar_add_menu (widget, _("_Edit"));
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_CUT);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_COPY);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_PASTE);
  gb_menu_bar_add_stock_item (menu, GTK_STOCK_DELETE);

  menu = gb_menu_bar_add_menu (widget, _("_View"));

  menu = gb_menu_bar_add_menu (widget, _("_Help"));
  gb_menu_bar_add_item (menu, _("_About"));
}


/*
 * Creates a new GtkWidget of class GtkMenuBar, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 * If the widget needs a special destroy handler, add a signal here.
 */
GtkWidget*
gb_menu_bar_new(GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  new_widget = gtk_menu_bar_new ();

  gtk_signal_connect_after (GTK_OBJECT (new_widget), "size_request",
			    GTK_SIGNAL_FUNC (on_menu_bar_size_request),
			    NULL);

  if (data->action == GB_CREATING)
    {
#ifdef USE_GNOME
      if (glade_project_get_gnome_support (data->project))
	glade_gnome_setup_initial_menus (new_widget);
      else
	gb_menu_bar_setup_initial_menus (new_widget);
#else
      gb_menu_bar_setup_initial_menus (new_widget);
#endif
    }

  return new_widget;
}


static void
on_menu_bar_size_request (GtkWidget * widget,
			  GtkRequisition *requisition,
			  gpointer data)
{
  /* Make sure we request a decent size. If we don't do this, when a menubar
     is created it appears about 3 pixels high which is not very good. */
  requisition->width = MAX (requisition->width, 32);
  requisition->height = MAX (requisition->height, 24);
}


/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_menu_bar_create_properties(GtkWidget *widget, GbWidgetCreateArgData *data)
{
  GtkWidget *property_table, *button;
  gint property_table_row;

  property_add_choice (PackDirection, _("Pack Direction:"),
		       _("The pack direction of the menubar"),
		       GbPackDirectionChoices);
  property_add_choice (ChildPackDirection, _("Child Direction:"),
		       _("The child pack direction of the menubar"),
		       GbPackDirectionChoices);

  /* Add a button for editing the menubar. */
  property_table = property_get_table_position (&property_table_row);
  button = gtk_button_new_with_label (_("Edit Menus..."));
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gb_menu_bar_on_edit_menu), NULL);
  gtk_table_attach (GTK_TABLE (property_table), button, 0, 3,
		    property_table_row, property_table_row + 1,
		    GTK_FILL, GTK_FILL, 10, 10);
}


/* Make window behave like a dialog */
static void
dialogize (GtkWidget *menued, GtkWidget *parent_widget)
{
  GtkWidget *transient_parent;

  gtk_signal_connect (GTK_OBJECT (menued), "key_press_event",
		      GTK_SIGNAL_FUNC (glade_util_check_key_is_esc),
		      GINT_TO_POINTER (GladeEscDestroys));
  transient_parent = glade_util_get_toplevel (parent_widget);
  if (GTK_IS_WINDOW (transient_parent))
    {
      gtk_window_set_transient_for (GTK_WINDOW (menued),
				    GTK_WINDOW (transient_parent));
    }
}

static void
gb_menu_bar_on_edit_menu (GtkWidget *button,
			  gpointer data)
{
  GtkWidget *menubar, *menued;

  menubar = property_get_widget ();
  g_return_if_fail (GTK_IS_MENU_BAR (menubar));

  menued = glade_menu_editor_new (current_project, GTK_MENU_SHELL (menubar));
  dialogize (menued, button);
  gtk_widget_show (GTK_WIDGET (menued));
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_menu_bar_get_properties(GtkWidget *widget, GbWidgetGetArgData *data)
{
  GtkPackDirection pack_direction, child_pack_direction;
  gint i;

  pack_direction = gtk_menu_bar_get_pack_direction (GTK_MENU_BAR (widget));
  child_pack_direction = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (widget));

  for (i = 0; i < sizeof (GbPackDirectionValues) / sizeof (GbPackDirectionValues[0]); i++)
    {
      if (GbPackDirectionValues[i] == pack_direction)
	gb_widget_output_choice (data, PackDirection, i, GbPackDirectionSymbols[i]);

      if (GbPackDirectionValues[i] == child_pack_direction)
	gb_widget_output_choice (data, ChildPackDirection, i, GbPackDirectionSymbols[i]);
    }
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_menu_bar_set_properties(GtkWidget *widget, GbWidgetSetArgData *data)
{
  gchar *pack_direction, *child_pack_direction;
  gint i;

  pack_direction = gb_widget_input_choice (data, PackDirection);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbPackDirectionValues) / sizeof (GbPackDirectionValues[0]); i++)
	{
	  if (!strcmp (pack_direction, GbPackDirectionChoices[i])
	      || !strcmp (pack_direction, GbPackDirectionSymbols[i]))
	    {
	      gtk_menu_bar_set_pack_direction (GTK_MENU_BAR (widget),
					       GbPackDirectionValues[i]);
	      break;
	    }
	}
    }

  child_pack_direction = gb_widget_input_choice (data, ChildPackDirection);
  if (data->apply)
    {
      for (i = 0; i < sizeof (GbPackDirectionValues) / sizeof (GbPackDirectionValues[0]); i++)
	{
	  if (!strcmp (child_pack_direction, GbPackDirectionChoices[i])
	      || !strcmp (child_pack_direction, GbPackDirectionSymbols[i]))
	    {
	      gtk_menu_bar_set_child_pack_direction (GTK_MENU_BAR (widget),
						     GbPackDirectionValues[i]);
	      break;
	    }
	}
    }
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GtkMenuBar, with signals pointing to
 * other functions in this file.
 */
static void
gb_menu_bar_create_popup_menu(GtkWidget *widget, GbWidgetCreateMenuData *data)
{
  GtkWidget *menuitem;

  menuitem = gtk_menu_item_new_with_label (_("Edit Menus..."));
  gtk_widget_show (menuitem);
  gtk_container_add (GTK_CONTAINER (data->menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (gb_menu_bar_on_edit_menu_activate),
		      widget);
}


static void
gb_menu_bar_on_edit_menu_activate (GtkWidget *menuitem,
				   GtkWidget *menubar)
{
  GtkWidget *menued;

  menued = glade_menu_editor_new (current_project, GTK_MENU_SHELL (menubar));
  dialogize (menued, menubar);
  gtk_widget_show (GTK_WIDGET (menued));
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_menu_bar_write_source (GtkWidget * widget, GbWidgetWriteSourceData * data)
{
  gboolean created_menu = FALSE;
  GtkPackDirection pack_direction, child_pack_direction;
  gint i;

  pack_direction = gtk_menu_bar_get_pack_direction (GTK_MENU_BAR (widget));
  child_pack_direction = gtk_menu_bar_get_child_pack_direction (GTK_MENU_BAR (widget));

#ifdef USE_GNOME
  /* For Gnome projects the menus are created using GnomeUIInfo structs, so
     we just create the start of the struct here. In a GnomeApp dock item,
     the code to add the menu to the GnomeApp is in output in gnomedockitem.c.
     If the menubar is not in a GnomeApp, we have to output the code to create
     it here. */
  if (data->project->gnome_support)
    {
      glade_gnome_start_menu_source (GTK_MENU_SHELL (widget), data);

      if (widget->parent && glade_gnome_is_app_dock_item (widget->parent))
	{
	  /* FIXME: should we set some standard properties? */
	  gb_widget_write_add_child_source (widget, data);
	  return;
	}
      else
	{
	  if (data->create_widget)
	    {
	      source_add (data, "  %s = gtk_menu_bar_new ();\n", data->wname);
	    }

	  gb_widget_write_standard_source (widget, data);

	  data->need_accel_group = TRUE;
	  source_add (data,
		      "  gnome_app_fill_menu (GTK_MENU_SHELL (%s), %s_uiinfo,\n"
		      "                       accel_group, FALSE, 0);\n",
		      data->wname, data->real_wname);
	  created_menu = TRUE;
	}
    }
#endif

  if (!created_menu)
    {
      if (data->create_widget)
	{
	  source_add (data, "  %s = gtk_menu_bar_new ();\n", data->wname);
	}

      gb_widget_write_standard_source (widget, data);
    }

  if (pack_direction != GTK_PACK_DIRECTION_LTR)
    {
      for (i = 0; i < sizeof (GbPackDirectionValues) / sizeof (GbPackDirectionValues[0]); i++)
	{
	  if (GbPackDirectionValues[i] == pack_direction)
	    source_add (data,
		"  gtk_menu_bar_set_pack_direction (GTK_MENU_BAR (%s), %s);\n",
			data->wname, GbPackDirectionSymbols[i]);
	}
    }

  if (child_pack_direction != GTK_PACK_DIRECTION_LTR)
    {
      for (i = 0; i < sizeof (GbPackDirectionValues) / sizeof (GbPackDirectionValues[0]); i++)
	{
	  if (GbPackDirectionValues[i] == child_pack_direction)
	    source_add (data,
		"  gtk_menu_bar_set_child_pack_direction (GTK_MENU_BAR (%s), %s);\n",
			data->wname, GbPackDirectionSymbols[i]);
	}
    }
}



/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget *
gb_menu_bar_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gtk_menu_bar_get_type ();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct (&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = menubar_xpm;
  gbwidget.tooltip = _("Menu Bar");

  /* Fill in any functions that this GbWidget has */
   gbwidget.gb_widget_new               = gb_menu_bar_new;
   gbwidget.gb_widget_create_properties = gb_menu_bar_create_properties;
   gbwidget.gb_widget_get_properties    = gb_menu_bar_get_properties;
   gbwidget.gb_widget_set_properties    = gb_menu_bar_set_properties;
   gbwidget.gb_widget_create_popup_menu = gb_menu_bar_create_popup_menu;
   gbwidget.gb_widget_write_source	= gb_menu_bar_write_source;

  return &gbwidget;
}
