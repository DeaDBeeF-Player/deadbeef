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

#include <gnome.h>
#include "../gb.h"
#include "../tree.h"

/* Include the 21x21 icon pixmap for this widget, to be used in the palette */
#include "../graphics/gnome-druid.xpm"

const char *ShowHelp = "GnomeDruid::show_help";


/*
 * This is the GbWidget struct for this widget (see ../gbwidget.h).
 * It is initialized in the init() function at the end of this file
 */
static GbWidget gbwidget;


static void show_druid_dialog (GbWidgetNewData * data);
static void on_druid_dialog_ok (GtkWidget * widget,
			       GbWidgetNewData * data);
static void on_druid_dialog_destroy (GtkWidget * widget,
				    GbWidgetNewData * data);

static void gb_gnome_druid_add_start_page (GtkWidget * menuitem,
					   GnomeDruid *druid);
static void gb_gnome_druid_add_finish_page (GtkWidget * menuitem,
					    GnomeDruid *druid);
static void gb_gnome_druid_insert_page_before (GtkWidget * menuitem,
					       GnomeDruidPage *page);
static void gb_gnome_druid_insert_page_after (GtkWidget * menuitem,
					      GnomeDruidPage *page);
static void gb_gnome_druid_show_page (GtkWidget *parent,
				      GtkWidget *new_page);


/******
 * NOTE: To use these functions you need to uncomment them AND add a pointer
 * to the function in the GbWidget struct at the end of this file.
 ******/

/*
 * Creates a new GtkWidget of class GnomeDruid, performing any specialized
 * initialization needed for the widget to work correctly in this environment.
 * If a dialog box is used to initialize the widget, return NULL from this
 * function, and call data->callback with your new widget when it is done.
 */
static GtkWidget*
gb_gnome_druid_new (GbWidgetNewData *data)
{
  GtkWidget *new_widget;

  if (data->action == GB_LOADING)
    {
      new_widget = gnome_druid_new ();
      return new_widget;
    }
  else
    {
      show_druid_dialog (data);
      return NULL;
    }
}


static void
show_druid_dialog (GbWidgetNewData * data)
{
  GtkWidget *dialog, *vbox, *hbox, *label, *spinbutton;
  GtkObject *adjustment;

  dialog = glade_util_create_dialog (_("New Gnome Druid"), data->parent,
				     GTK_SIGNAL_FUNC (on_druid_dialog_ok),
				     data, &vbox);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (on_druid_dialog_destroy), data);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Number of Pages:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 5);
  gtk_widget_show (label);

  adjustment = gtk_adjustment_new (3, 2, 100, 1, 10, 10);
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
on_druid_dialog_ok (GtkWidget * widget, GbWidgetNewData * data)
{
  GtkWidget *new_widget, *spinbutton, *window, *page;
  gint pages, i;
	
  window = gtk_widget_get_toplevel (widget);

  /* Only call callback if placeholder/fixed widget is still there */
  if (gb_widget_can_finish_new (data))
    {
      spinbutton = gtk_object_get_data (GTK_OBJECT (window), "pages");
      g_return_if_fail (spinbutton != NULL);
      pages = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinbutton));

      new_widget = gnome_druid_new ();
      page = gnome_druid_page_edge_new_aa (GNOME_EDGE_START);
      gb_widget_create_from (page, "GnomeDruidPageStart");
      gnome_druid_append_page (GNOME_DRUID (new_widget),
			       GNOME_DRUID_PAGE (page));
      gnome_druid_set_page (GNOME_DRUID (new_widget), GNOME_DRUID_PAGE (page));

      for (i = 0; i < pages - 2; i++)
	{
	  page = gb_widget_new ("GnomeDruidPageStandard", new_widget);
	  gnome_druid_append_page (GNOME_DRUID (new_widget),
				   GNOME_DRUID_PAGE (page));
	}

      if (pages >= 2)
	{
	  page = gnome_druid_page_edge_new_aa (GNOME_EDGE_FINISH);
	  gb_widget_create_from (page, "GnomeDruidPageFinish");
	  gnome_druid_append_page (GNOME_DRUID (new_widget),
				   GNOME_DRUID_PAGE (page));
	}

      gtk_widget_show_all (new_widget);
      gb_widget_initialize (new_widget, data);
      (*data->callback) (new_widget, data);
    }
  gtk_widget_destroy (window);
}


static void
on_druid_dialog_destroy (GtkWidget * widget,
			 GbWidgetNewData * data)
{
  gb_widget_free_new_data (data);
  gtk_grab_remove (widget);
}


void
gb_gnome_druid_add_child (GtkWidget *widget,
			  GtkWidget *child,
			  GbWidgetSetArgData *data)
{
  gnome_druid_append_page (GNOME_DRUID (widget), GNOME_DRUID_PAGE (child));
}



/*
 * Creates the components needed to edit the extra properties of this widget.
 */
static void
gb_gnome_druid_create_properties (GtkWidget *widget,
				  GbWidgetCreateArgData *data)
{
  property_add_bool (ShowHelp, _("Show Help"), _("Display the help button."));
}


/*
 * Gets the properties of the widget. This is used for both displaying the
 * properties in the property editor, and also for saving the properties.
 */
static void
gb_gnome_druid_get_properties (GtkWidget *widget,
			       GbWidgetGetArgData *data)
{
  gboolean show_help;

  g_object_get (G_OBJECT (widget),
		"show-help", &show_help,
		NULL);

  gb_widget_output_bool (data, ShowHelp, show_help);
}


/*
 * Sets the properties of the widget. This is used for both applying the
 * properties changed in the property editor, and also for loading.
 */
static void
gb_gnome_druid_set_properties (GtkWidget *widget,
			       GbWidgetSetArgData *data)
{
  gboolean show_help;

  show_help = gb_widget_input_bool (data, ShowHelp);
  if (data->apply)
    g_object_set (G_OBJECT (widget), "show-help", show_help, NULL);
}


/*
 * Adds menu items to a context menu which is just about to appear!
 * Add commands to aid in editing a GnomeDruid, with signals pointing to
 * other functions in this file.
 */
static void
gb_gnome_druid_create_popup_menu (GtkWidget * widget,
				  GbWidgetCreateMenuData * data)
{
  GtkWidget *menuitem;
  GnomeDruid *druid;
  GList *children, *elem;

  if (data->child == NULL)
    return;

  g_return_if_fail (GNOME_IS_DRUID (data->child->parent));
  druid = GNOME_DRUID (data->child->parent);

  children = gtk_container_get_children (GTK_CONTAINER (widget));

  /* 'Add Start Page' is added if the druid has no pages or the first one
     is not a start page. */
  if (!children || 
      !(GNOME_IS_DRUID_PAGE_EDGE (children->data) && 
	GNOME_DRUID_PAGE_EDGE (children->data)->position == GNOME_EDGE_START))
    { 
      menuitem = gtk_menu_item_new_with_label (_("Add Start Page"));
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_gnome_druid_add_start_page),
			  druid);
    }

  /* 'Add Finish Page' is added if the druid has no pages or the last one
     is not a finish page. */
  elem = g_list_last (children);
  if (!elem || 
      !(GNOME_IS_DRUID_PAGE_EDGE (elem->data) && 
	GNOME_DRUID_PAGE_EDGE (elem->data)->position == GNOME_EDGE_FINISH))
    { 
      menuitem = gtk_menu_item_new_with_label (_("Add Finish Page"));
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_gnome_druid_add_finish_page),
			  druid);
    }

  g_list_free (children);

  /* 'Insert Page Before' is added if the current page is not the start page.
   */
  if (!(GNOME_IS_DRUID_PAGE_EDGE (data->child) && 
	GNOME_DRUID_PAGE_EDGE (data->child)->position == GNOME_EDGE_START))
    { 
      menuitem = gtk_menu_item_new_with_label (_("Insert Page Before"));
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_gnome_druid_insert_page_before),
			  data->child);
    }

  /* 'Insert Page After' is added if the current page is not the finish page.
   */
  if (!(GNOME_IS_DRUID_PAGE_EDGE (data->child) &&
	GNOME_DRUID_PAGE_EDGE (data->child)->position == GNOME_EDGE_FINISH))
    { 
      menuitem = gtk_menu_item_new_with_label (_("Insert Page After"));
      gtk_widget_show (menuitem);
      gtk_menu_append (GTK_MENU (data->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gb_gnome_druid_insert_page_after),
			  data->child);
    }
}


static void
gb_gnome_druid_add_start_page (GtkWidget * menuitem,
			       GnomeDruid *druid)
{
  GtkWidget *new_page;

  new_page = gnome_druid_page_edge_new_aa (GNOME_EDGE_START);
  gb_widget_create_from (new_page, "GnomeDruidPageStart");
  gtk_widget_show_all (new_page);

  gnome_druid_prepend_page (druid, GNOME_DRUID_PAGE (new_page));

  gb_gnome_druid_show_page (GTK_WIDGET (druid), new_page);

  gnome_druid_set_page (druid, GNOME_DRUID_PAGE (new_page));
  tree_add_widget (GTK_WIDGET (new_page));
}


static void
gb_gnome_druid_add_finish_page (GtkWidget * menuitem,
				GnomeDruid *druid)
{
  GtkWidget *new_page;

  new_page = gnome_druid_page_edge_new_aa (GNOME_EDGE_FINISH);
  gb_widget_create_from (new_page, "GnomeDruidPageFinish");
  gtk_widget_show_all (new_page);

  gnome_druid_append_page (druid, GNOME_DRUID_PAGE (new_page));

  gb_gnome_druid_show_page (GTK_WIDGET (druid), new_page);

  gnome_druid_set_page (druid, GNOME_DRUID_PAGE (new_page));
  tree_add_widget (GTK_WIDGET (new_page));
}


static void
gb_gnome_druid_insert_page_before (GtkWidget * menuitem,
				   GnomeDruidPage *page)
{
  GtkWidget *parent, *new_page;
  GnomeDruidPage *prev_page;
  GList *children, *elem;

  parent = GTK_WIDGET (page)->parent;
  g_return_if_fail (GNOME_IS_DRUID (parent));

  children = gtk_container_get_children (GTK_CONTAINER (parent));
  elem = g_list_find (children, page);
  g_return_if_fail (elem != NULL);

  new_page = gb_widget_new ("GnomeDruidPageStandard", parent);
  gtk_widget_show_all (new_page);

  if (elem->prev)
    prev_page = GNOME_DRUID_PAGE (elem->prev->data);
  else
    prev_page = NULL;

  g_list_free (children);

  gnome_druid_insert_page (GNOME_DRUID (parent), prev_page,
			   GNOME_DRUID_PAGE (new_page));

  gb_gnome_druid_show_page (parent, new_page);

  gnome_druid_set_page (GNOME_DRUID (parent), GNOME_DRUID_PAGE (new_page));
  tree_add_widget (GTK_WIDGET (new_page));
}


static void
gb_gnome_druid_insert_page_after (GtkWidget * menuitem,
				  GnomeDruidPage *page)
{
  GtkWidget *parent, *new_page;

  parent = GTK_WIDGET (page)->parent;
  g_return_if_fail (GNOME_IS_DRUID (parent));

  new_page = gb_widget_new ("GnomeDruidPageStandard", parent);
  gtk_widget_show_all (new_page);

  gnome_druid_insert_page (GNOME_DRUID (parent),
			   GNOME_DRUID_PAGE (page),
			   GNOME_DRUID_PAGE (new_page));

  gb_gnome_druid_show_page (parent, new_page);

  gnome_druid_set_page (GNOME_DRUID (parent), GNOME_DRUID_PAGE (new_page));
  tree_add_widget (GTK_WIDGET (new_page));
}


/* FIXME: GnomeDruid bug workaround. */
static void
gb_gnome_druid_show_page (GtkWidget *parent, GtkWidget *new_page)
{
  /* Hopefully we don't need this for GNOME 2. */
#if 0
  if (GTK_WIDGET_REALIZED (parent))
    gtk_widget_realize (new_page);

  if (GTK_WIDGET_VISIBLE (parent) && GTK_WIDGET_VISIBLE (new_page))
    {
      if (GTK_WIDGET_MAPPED (parent))
	gtk_widget_map (new_page);

      gtk_widget_queue_resize (new_page);
    }
#endif
}


/*
 * Writes the source code needed to create this widget.
 * You have to output everything necessary to create the widget here, though
 * there are some convenience functions to help.
 */
static void
gb_gnome_druid_write_source (GtkWidget * widget,
			     GbWidgetWriteSourceData * data)
{
  gboolean show_help;

  if (data->create_widget)
    {
      source_add (data, "  %s = gnome_druid_new ();\n", data->wname);
    }

  gb_widget_write_standard_source (widget, data);

  g_object_get (G_OBJECT (widget),
		"show-help", &show_help,
		NULL);

  if (show_help)
    {
      source_add (data,
		  "  gnome_druid_set_show_help (GNOME_DRUID (%s), TRUE);\n",
		  data->wname);
    }
}


/* Outputs source to add a child widget to a table. */
static void
gb_gnome_druid_write_add_child_source (GtkWidget * parent,
				       const gchar *parent_name,
				       GtkWidget *child,
				       GbWidgetWriteSourceData * data)
{
  source_add (data,
	      "  gnome_druid_append_page (GNOME_DRUID (%s),\n"
	      "                           GNOME_DRUID_PAGE (%s));\n",
	      parent_name, data->wname);
}

/*
 * Initializes the GbWidget structure.
 * I've placed this at the end of the file so we don't have to include
 * declarations of all the functions.
 */
GbWidget*
gb_gnome_druid_init ()
{
  /* Initialise the GTK type */
  volatile GtkType type;
  type = gnome_druid_get_type();

  /* Initialize the GbWidget structure */
  gb_widget_init_struct(&gbwidget);

  /* Fill in the pixmap struct & tooltip */
  gbwidget.pixmap_struct = gnome_druid_xpm;
  gbwidget.tooltip = _("Druid");

  /* Fill in any functions that this GbWidget has */
  gbwidget.gb_widget_new		= gb_gnome_druid_new;
  gbwidget.gb_widget_add_child          = gb_gnome_druid_add_child;
  gbwidget.gb_widget_create_popup_menu	= gb_gnome_druid_create_popup_menu;
  gbwidget.gb_widget_write_source	= gb_gnome_druid_write_source;
  gbwidget.gb_widget_write_add_child_source = gb_gnome_druid_write_add_child_source;
  gbwidget.gb_widget_create_properties	= gb_gnome_druid_create_properties;
  gbwidget.gb_widget_get_properties	= gb_gnome_druid_get_properties;
  gbwidget.gb_widget_set_properties	= gb_gnome_druid_set_properties;

  return &gbwidget;
}

