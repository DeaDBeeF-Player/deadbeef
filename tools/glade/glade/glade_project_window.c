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
#include <locale.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkstatusbar.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkvbox.h>

#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#endif

#include "glade.h"
#include "glade_clipboard.h"
#include "glade_palette.h"
#include "glade_project_options.h"
#include "glade_project_window.h"
#include "editor.h"
#include "gbwidget.h"
#include "load.h"
#include "property.h"
#include "tree.h"
#include "utils.h"

/* This is the main project window. */
static GladeProjectWindow *project_window;

/* This is used to store a pointer to a GladeProjectWindow in the main window.
 */
static gchar *GladeProjectWindowKey = "GladeProjectWindowKey";

/* The menuitem's to show/hide the various windows. */
static GtkWidget *palette_item = NULL;
static GtkWidget *property_editor_item = NULL;
static GtkWidget *widget_tree_item = NULL;
static GtkWidget *clipboard_item = NULL;

/* stuff for opening a file when dragged into glade */
enum
{
  TARGET_URI_LIST
};

static GtkTargetEntry drop_types[] =
{
  {"text/uri-list", 0, TARGET_URI_LIST}
};


static gboolean glade_project_window_delete_event (GtkWidget *widget,
						   GdkEvent *event,
						   GladeProjectWindow *project_window);
static void glade_project_window_destroy (GtkWidget *widget,
					  gpointer   data);

static void glade_project_window_new_project (GtkWidget *widget,
					      gpointer   data);
static void glade_project_window_on_new_project_ok (GladeProjectWindow *project_window);
static void glade_project_window_on_open_project (GtkWidget *widget,
					       gpointer   data);
static void glade_project_window_real_open_project (GtkWidget          *widget,
						    gint		response_id,
						    GladeProjectWindow *project_window);

static void glade_project_window_drag_data_received (GtkWidget *widget,
                                                     GdkDragContext *context,
                                                     gint x, gint y,
                                                     GtkSelectionData *selection_data,
                                                     guint info, guint time, gpointer data);
#if 0
static void glade_project_window_show_loading_errors (GladeProjectWindow *project_window,
						      GList		 *errors);
#endif
static GtkWidget* glade_project_window_new_errors_dialog (GladeProjectWindow *project_window,
							  GtkWidget **text_return);

static void glade_project_window_on_edit_options (GtkWidget *widget,
						  gpointer   data);
static void glade_project_window_edit_options (GladeProjectWindow *project_window,
					       GladeProjectOptionsAction action);
static void glade_project_window_save_project (GtkWidget *widget,
					       gpointer   data);
#if 0
static void glade_project_window_on_save_project_as (GtkWidget *widget,
						     gpointer   data);
static void glade_project_window_save_project_as (GladeProjectWindow *project_window);
static void glade_project_window_save_as_callback (GtkWidget          *widget,
						   gint		   response_id,
						   GladeProjectWindow *project_window);
#endif
static gboolean glade_project_window_real_save_project (GladeProjectWindow *project_window,
							gboolean            warn_before_overwrite);
static void glade_project_window_write_source (GtkWidget *widget,
					       gpointer   data);
static void glade_project_window_real_write_source (GladeProjectWindow *project_window);
static void glade_project_window_show_error (GladeProjectWindow *project_window,
					     GladeError         *error,
					     gchar              *title);

static void glade_project_window_quit (GtkWidget *widget,
				       gpointer   data);
static void glade_project_window_show_quit_dialog (GladeProjectWindow *project_window);

static void glade_project_window_cut (GtkWidget *widget,
				      gpointer   data);
static void glade_project_window_copy (GtkWidget *widget,
				       gpointer   data);
static void glade_project_window_paste (GtkWidget *widget,
					gpointer   data);
static void glade_project_window_delete (GtkWidget *widget,
					 gpointer   data);
static void glade_project_window_real_delete (GladeProjectWindow *project_window);

static void glade_project_window_toggle_palette (GtkWidget *widget,
					       gpointer   data);
static void glade_project_window_toggle_property_editor (GtkWidget *widget,
						       gpointer   data);
static void glade_project_window_toggle_widget_tree (GtkWidget *widget,
						   gpointer   data);
static void glade_project_window_toggle_clipboard (GtkWidget *widget,
						 gpointer   data);
static void glade_project_window_toggle_tooltips (GtkWidget *widget,
						  gpointer   data);
static void glade_project_window_toggle_grid (GtkWidget *widget,
					      gpointer   data);
static void glade_project_window_edit_grid_settings (GtkWidget *widget,
						     gpointer   data);
static void glade_project_window_toggle_snap (GtkWidget *widget,
					      gpointer   data);
static void glade_project_window_edit_snap_settings (GtkWidget *widget,
						     gpointer   data);

static void glade_project_window_about (GtkWidget *widget,
					gpointer   data);

static gint glade_project_window_key_press_event (GtkWidget * widget,
						  GdkEventKey * event,
						  gpointer data);

static void glade_project_window_options_ok (GtkWidget	    *widget,
					     GladeProjectWindow *project_window);
static void glade_project_window_update_title (GladeProjectWindow *project_window);



#ifdef USE_GNOME
static void
glade_project_window_show_help_doc (const gchar *docname,
				    GtkWidget *transient_widget)
{
  GError *error = NULL;

  gnome_help_display (docname, NULL, &error);
  if (error) {
    char *message;
    message = g_strdup_printf (_("Couldn't show help file: %s.\n\nError: %s"),
			       docname, error->message);
    glade_util_show_message_box	(message, transient_widget);
    g_free (message);
    g_error_free (error);
  }
}


static void
glade_project_window_show_glade_faq (GtkWidget *menuitem,
				     gpointer data)
{
  glade_project_window_show_help_doc ("glade-faq", menuitem);
}
#endif


/*
 * These are the menubar and toolbar definitions for Gnome.
 */
#ifdef USE_GNOME
static GnomeUIInfo FileMenu[] =
{
  GNOMEUIINFO_MENU_NEW_ITEM (N_("_New"), N_("Create a new project"),
			     glade_project_window_new_project, NULL),
  GNOMEUIINFO_MENU_OPEN_ITEM (glade_project_window_on_open_project, NULL),
  GNOMEUIINFO_MENU_SAVE_ITEM (glade_project_window_save_project, NULL),
  /*GNOMEUIINFO_MENU_SAVE_AS_ITEM (glade_project_window_on_save_project_as,
    NULL),*/
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("_Build"),
    N_("Output the project source code"),
    glade_project_window_write_source, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CONVERT,
    'B', GDK_CONTROL_MASK, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Op_tions..."),
    N_("Edit the project options"),
    glade_project_window_on_edit_options, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PROPERTIES,
    0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_QUIT_ITEM (glade_project_window_quit, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo EditMenu[] =
{
  GNOMEUIINFO_MENU_CUT_ITEM (glade_project_window_cut, NULL),
  GNOMEUIINFO_MENU_COPY_ITEM (glade_project_window_copy, NULL),
  GNOMEUIINFO_MENU_PASTE_ITEM (glade_project_window_paste, NULL),
  { GNOME_APP_UI_ITEM, N_("_Delete"), N_("Delete the selected widget"),
    glade_project_window_delete, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TRASH,
    GDK_DELETE, 0, NULL },
  GNOMEUIINFO_END
};


/* These must match the indices of the appropriate items in the settings
   GnomeUIInfo array. We need them to set the initial state. */
#define GLADE_PALETTE_ITEM 0
#define GLADE_PROPERTY_EDITOR_ITEM 1
#define GLADE_WIDGET_TREE_ITEM 2
#define GLADE_CLIPBOARD_ITEM 3

static GnomeUIInfo ViewMenu[] =
{
  {
    GNOME_APP_UI_TOGGLEITEM, N_("Show _Palette"), N_("Show the palette of widgets"),
    glade_project_window_toggle_palette, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_TOGGLEITEM, N_("Show Property _Editor"),
    N_("Show the property editor"),
    glade_project_window_toggle_property_editor, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_TOGGLEITEM, N_("Show Widget _Tree"),
    N_("Show the widget tree"),
    glade_project_window_toggle_widget_tree, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_TOGGLEITEM, N_("Show _Clipboard"),
    N_("Show the clipboard"),
    glade_project_window_toggle_clipboard, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};


/* These must match the indices of the appropriate items in the settings
   GnomeUIInfo array. We need them to set the initial state. */
#define GLADE_SHOW_GRID_ITEM 0
#define GLADE_SNAP_TO_GRID_ITEM 1
#define GLADE_SHOW_TOOLTIPS_ITEM 2

static GnomeUIInfo SettingsMenu[] =
{
  {
    GNOME_APP_UI_TOGGLEITEM, N_("Show _Grid"),
    N_("Show the grid (in fixed containers only)"),
    glade_project_window_toggle_grid, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_TOGGLEITEM, N_("_Snap to Grid"),
    N_("Snap widgets to the grid"),
    glade_project_window_toggle_snap, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_TOGGLEITEM, N_("Show _Widget Tooltips"),
    N_("Show the tooltips of created widgets"),
    glade_project_window_toggle_tooltips, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },

  GNOMEUIINFO_SEPARATOR,

  {
    GNOME_APP_UI_ITEM, N_("Set Grid _Options..."),
    N_("Set the grid style and spacing"),
    glade_project_window_edit_grid_settings, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Set Snap O_ptions..."),
    N_("Set options for snapping to the grid"),
    glade_project_window_edit_snap_settings, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GnomeUIInfo HelpMenu[] =
{
  GNOMEUIINFO_HELP ("glade-user-guide"),
  {
    GNOME_APP_UI_ITEM, N_("_FAQ"),
    N_("View the Glade FAQ"),
    glade_project_window_show_glade_faq, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL,
    0, 0, NULL
  },

  GNOMEUIINFO_SEPARATOR,

  GNOMEUIINFO_MENU_ABOUT_ITEM (glade_project_window_about, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo MainMenu[] =
{
  GNOMEUIINFO_SUBTREE (N_("_Project"), FileMenu),
  GNOMEUIINFO_MENU_EDIT_TREE (EditMenu),
  GNOMEUIINFO_MENU_VIEW_TREE (ViewMenu),
  GNOMEUIINFO_MENU_SETTINGS_TREE (SettingsMenu),
  GNOMEUIINFO_MENU_HELP_TREE (HelpMenu),
  GNOMEUIINFO_END
};

static GnomeUIInfo ToolBar[] =
{
  {
    GNOME_APP_UI_ITEM, N_("New"), N_("New Project"), 
    glade_project_window_new_project, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_NEW, 0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Open"), N_("Open Project"), 
    glade_project_window_on_open_project, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_OPEN, 0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Save"), N_("Save Project"), 
    glade_project_window_save_project, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_PIXMAP_SAVE, 0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  {
    GNOME_APP_UI_ITEM, N_("Options"), N_("Project Options"), 
    glade_project_window_on_edit_options, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PROPERTIES, 0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("Build"), N_("Build the Source Code"), 
    glade_project_window_write_source, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CONVERT, 0, 0, NULL
  },
  GNOMEUIINFO_END
};
#endif


#ifdef USE_GNOME
GladeProjectWindow*
glade_project_window_new (void)
{
  GtkWidget *scrolled_win;
  GtkWidget *show_grid_item, *snap_to_grid_item, *show_tooltips_item;

  project_window = g_new (GladeProjectWindow, 1);
  project_window->current_directory = NULL;

  project_window->window = gnome_app_new ("Glade", "Glade");
  gtk_widget_set_name (project_window->window, "GladeProjectWindow");
  gtk_window_set_wmclass (GTK_WINDOW (project_window->window),
			  "project", "Glade");

  /* We want all the keyboard accelerators from the menus to work anywhere in
     the Glade GUI, including windows being designed. So we add the accel
     group to every window. */
  GNOME_APP (project_window->window)->accel_group = glade_get_global_accel_group ();
  gtk_window_add_accel_group (GTK_WINDOW (project_window->window),
			      glade_get_global_accel_group ());

  /* Save a pointer to the GladeProjectWindow, so we can find it in callbacks.
   */
  gtk_object_set_data (GTK_OBJECT (project_window->window),
		       GladeProjectWindowKey, project_window);

  gtk_window_move (GTK_WINDOW (project_window->window), 0, 0);

  gtk_signal_connect (GTK_OBJECT (project_window->window), "destroy",
		      GTK_SIGNAL_FUNC (glade_project_window_destroy), NULL);
  gtk_signal_connect (GTK_OBJECT (project_window->window), "delete_event",
		      GTK_SIGNAL_FUNC (glade_project_window_delete_event),
		      project_window);
  gtk_signal_connect_after (GTK_OBJECT (project_window->window),
			    "key_press_event",
			    GTK_SIGNAL_FUNC (glade_project_window_key_press_event),
			    NULL);

  /* support for opening a file by dragging onto the project window */
  gtk_drag_dest_set (project_window->window,
                     GTK_DEST_DEFAULT_ALL,
                     drop_types, G_N_ELEMENTS (drop_types),
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  g_signal_connect (G_OBJECT (project_window->window), "drag-data-received",
                    G_CALLBACK (glade_project_window_drag_data_received), NULL);

  gnome_app_create_menus (GNOME_APP (project_window->window), MainMenu);

  /* Set the states of the toggle items. */
  palette_item = ViewMenu[GLADE_PALETTE_ITEM].widget;
  property_editor_item = ViewMenu[GLADE_PROPERTY_EDITOR_ITEM].widget;
  widget_tree_item = ViewMenu[GLADE_WIDGET_TREE_ITEM].widget;
  clipboard_item = ViewMenu[GLADE_CLIPBOARD_ITEM].widget;

  show_grid_item = SettingsMenu[GLADE_SHOW_GRID_ITEM].widget;
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (show_grid_item),
				 editor_get_show_grid ());

  snap_to_grid_item = SettingsMenu[GLADE_SNAP_TO_GRID_ITEM].widget;
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (snap_to_grid_item),
				  editor_get_snap_to_grid ());

  show_tooltips_item = SettingsMenu[GLADE_SHOW_TOOLTIPS_ITEM].widget;
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (show_tooltips_item),
				  gb_widget_get_show_tooltips ());


  gnome_app_create_toolbar (GNOME_APP (project_window->window), ToolBar);

  /* Create list of components */
  project_window->project_view = glade_project_view_new ();
  gtk_widget_show (project_window->project_view);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win),
		     project_window->project_view);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolled_win);

  gnome_app_set_contents (GNOME_APP (project_window->window), scrolled_win);

  /* Create status bar. */
  project_window->statusbar = gtk_statusbar_new ();
  gnome_app_set_statusbar (GNOME_APP (project_window->window),
			   project_window->statusbar);
  gnome_app_install_statusbar_menu_hints (GTK_STATUSBAR (project_window->statusbar), MainMenu);

  return project_window;
}

#else

#if 0
static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",				NULL,
    0,					0, "<Branch>" },
  { "/File/tearoff1",			NULL,
    0,					0, "<Tearoff>" },
  { "/File/_New Project",		NULL,
    glade_project_window_new_project,	0, "<StockItem>", GTK_STOCK_NEW },
  { "/File/_Open",			NULL,
    glade_project_window_on_open_project, 0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/_Save",			NULL,
    glade_project_window_save_project,	0, "<StockItem>", GTK_STOCK_SAVE },
  { "/File/sep1",			NULL,
    0,					0, "<Separator>" },
  { "/File/_Build Source Code",		NULL,
    glade_project_window_write_source,	0, "<StockItem>", GTK_STOCK_CONVERT },
  { "/File/_Project Options",		NULL,
    glade_project_window_on_edit_options, 0, "<StockItem>", GTK_STOCK_PROPERTIES },
  { "/File/_Quit",			NULL,
    glade_project_window_quit,		0, "<StockItem>", GTK_STOCK_QUIT },

  { "/_Edit",				NULL,
    0,					0, "<Branch>" },

  { "/_View",				NULL,
    0,					0, "<Branch>" },

  { "/_Help",				NULL,
    0,					0, "<LastBranch>" },
  { "/Help/_About",			NULL,
    glade_project_window_about,		0 },
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
#endif

static void
add_stock_menu_item	(GtkMenu *menu, gchar *stock_id, gchar *label, 
			 GtkSignalFunc callback, GtkAccelGroup *accel_group,
			 GtkTooltips *tooltips, gchar *tip)
{
  GtkWidget *menuitem;

  menuitem = gtk_image_menu_item_new_from_stock (stock_id, accel_group);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);

  if (label)
    gtk_label_set_text_with_mnemonic (GTK_LABEL (GTK_BIN (menuitem)->child),
				      label);

  gtk_signal_connect (GTK_OBJECT (menuitem), "activate", callback, NULL);

  gtk_tooltips_set_tip (tooltips, menuitem, tip, NULL);
  gtk_widget_show (menuitem);
}


GladeProjectWindow*
glade_project_window_new (void)
{
  GtkTooltips *tooltips;

  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *image;
  GtkWidget *toolbar;
  GdkColormap *colormap;
  GtkAccelGroup *accel_group;
  GtkWidget *scrolled_win;
  GtkWidget *vbox_main;
  GtkIconSize icon_size;
  GtkToolbarChild *toolbar_child;
  GList *elem;

  project_window = g_new (GladeProjectWindow, 1);
  project_window->current_directory = NULL;

  project_window->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name (project_window->window, "GladeProjectWindow");
  gtk_window_set_title (GTK_WINDOW (project_window->window), "Glade");
  gtk_window_set_wmclass (GTK_WINDOW (project_window->window),
			  "project", "Glade");

  /* Save a pointer to the GladeProjectWindow, so we can find it in callbacks.
   */
  gtk_object_set_data (GTK_OBJECT (project_window->window),
		       GladeProjectWindowKey, project_window);

  gtk_window_move (GTK_WINDOW (project_window->window), 0, 0);

  gtk_signal_connect (GTK_OBJECT (project_window->window), "destroy",
		      GTK_SIGNAL_FUNC (glade_project_window_destroy), NULL);
  gtk_signal_connect (GTK_OBJECT (project_window->window), "delete_event",
		      GTK_SIGNAL_FUNC (glade_project_window_delete_event),
		      project_window);
  gtk_signal_connect_after (GTK_OBJECT (project_window->window),
			    "key_press_event",
			    GTK_SIGNAL_FUNC (glade_project_window_key_press_event),
			    NULL);

  tooltips = gtk_tooltips_new ();

  vbox_main = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (project_window->window), vbox_main);
  gtk_widget_show (vbox_main);

  /* Create accelerator table */
  accel_group = glade_get_global_accel_group ();
  gtk_window_add_accel_group (GTK_WINDOW (project_window->window),
			      accel_group);

#if 0
  /* Create the menubar items, using the factory. */
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>",
				       accel_group);
  gtk_object_set_data_full (GTK_OBJECT (project_window->window), "<main>",
			    item_factory, (GtkDestroyNotify) gtk_object_unref);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  gtk_box_pack_start (GTK_BOX (vbox_main),
		      gtk_item_factory_get_widget (item_factory, "<main>"),
		      FALSE, FALSE, 0);
  gtk_widget_show_all (vbox_main);
#endif

  /* create menu bar */
  menubar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox_main), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);

  /* create File menu */
  menuitem = gtk_menu_item_new_with_mnemonic (_("_Project"));
  gtk_container_add (GTK_CONTAINER (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_NEW, _("_New"), 
		       GTK_SIGNAL_FUNC (glade_project_window_new_project),
		       accel_group, tooltips, _("Create a new project"));

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_OPEN, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_on_open_project),
		       accel_group, tooltips,  _("Open an existing project"));

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_SAVE, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_save_project),
		       accel_group, tooltips,  _("Save project"));

  /*
  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_SAVE_AS, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_save_project_as),
		       accel_group, tooltips,  _("Save project to a new file"));
  */

  menuitem = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_image_menu_item_new_with_mnemonic (_("_Build"));
  image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
  gtk_widget_show (image);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_write_source),
		      NULL);
  gtk_widget_add_accelerator (menuitem, "activate", accel_group,
			      'B', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Output the project source code"), NULL);
  gtk_widget_show (menuitem);

  menuitem = gtk_image_menu_item_new_with_mnemonic (_("Op_tions..."));
  image = gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
  gtk_widget_show (image);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_on_edit_options),
		      NULL);

  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem, _("Edit the project options"),
			NULL);
  gtk_widget_show (menuitem);

  menuitem = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_QUIT, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_quit),
		       accel_group, tooltips,  _("Quit Glade"));


  /* Create Edit menu */
  menuitem = gtk_menu_item_new_with_mnemonic (_("_Edit"));
  gtk_container_add (GTK_CONTAINER (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_CUT, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_cut),
		       accel_group, tooltips,
		       _("Cut the selected widget to the clipboard"));

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_COPY, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_copy),
		       accel_group, tooltips,
		       _("Copy the selected widget to the clipboard"));

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_PASTE, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_paste),
		       accel_group, tooltips,
		       _("Paste the widget from the clipboard over the selected widget"));

  add_stock_menu_item (GTK_MENU (menu), GTK_STOCK_DELETE, NULL, 
		       GTK_SIGNAL_FUNC (glade_project_window_delete),
		       accel_group, tooltips,
		       _("Delete the selected widget"));


  /* Create View menu */
  menuitem = gtk_menu_item_new_with_mnemonic (_("_View"));
  gtk_container_add (GTK_CONTAINER (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("Show _Palette"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_palette),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem, _("Show the palette of widgets"),
			NULL);
  palette_item = menuitem;
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("Show Property _Editor"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_property_editor),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Show the property editor"), NULL);
  property_editor_item = menuitem;
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("Show Widget _Tree"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_widget_tree),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Show the widget tree"), NULL);
  widget_tree_item = menuitem;
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("Show _Clipboard"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_clipboard),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Show the clipboard"), NULL);
  clipboard_item = menuitem;
  gtk_widget_show (menuitem);

  menuitem = gtk_separator_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("Show _Widget Tooltips"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
				 gb_widget_get_show_tooltips ());
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_tooltips),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Show the tooltips of created widgets"), NULL);
  gtk_widget_show (menuitem);


  menuitem = gtk_menu_item_new_with_mnemonic (_("_Grid"));
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Create Grid submenu */
  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("_Show Grid"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
				 editor_get_show_grid ());
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_grid),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Show the grid (in fixed containers only)"), NULL);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_mnemonic (_("Set Grid _Options..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_edit_grid_settings),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
			_("Set the spacing between grid lines"), NULL);
  gtk_widget_show (menuitem);

  menuitem = gtk_check_menu_item_new_with_mnemonic (_("S_nap to Grid"));
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
				  editor_get_snap_to_grid ());
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_toggle_snap),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
		      _("Snap widgets to the grid (in fixed containers only)"),
			NULL);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_mnemonic (_("Set Snap O_ptions..."));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_edit_snap_settings),
		      NULL);
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_tooltips_set_tip (tooltips, menuitem,
		      _("Set which parts of a widget snap to the grid"), NULL);
  gtk_widget_show (menuitem);

  /* Create Help menu */
  menuitem = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_container_add (GTK_CONTAINER (menubar), menuitem);
  gtk_widget_show (menuitem);

  menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), menu);

  /* Don't show these yet as we have no help pages.
  menuitem = gtk_menu_item_new_with_mnemonic (_("_Contents"));
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_mnemonic (_("_Index"));
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new ();
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_widget_show (menuitem);
  */

  menuitem = gtk_menu_item_new_with_mnemonic (_("_About..."));
  gtk_container_add (GTK_CONTAINER (menu), menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (glade_project_window_about),
		      NULL);
  gtk_widget_show (menuitem);

  /* Create toolbar */
  toolbar = gtk_toolbar_new ();
  /*gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_HALF);*/
  gtk_box_pack_start (GTK_BOX (vbox_main), toolbar, FALSE, TRUE, 0);
  gtk_widget_show (toolbar);

  colormap = gtk_widget_get_colormap (toolbar);

  /* I've taken this out to try to make the window smaller. */
#if 0
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New Project"), "",
			    GTK_SIGNAL_FUNC (glade_project_window_new_project),
			    NULL, -1);
#endif
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_OPEN,
			    _("Open Project"), "",
			    GTK_SIGNAL_FUNC (glade_project_window_on_open_project),
			    NULL, -1);
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_SAVE,
			    _("Save Project"), "",
			    GTK_SIGNAL_FUNC (glade_project_window_save_project),
			    NULL, -1);
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  
  icon_size = gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar));
  image = gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, icon_size);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "",
			   _("Project Options"), "", image,
			   GTK_SIGNAL_FUNC (glade_project_window_on_edit_options),
			   NULL);
  elem = g_list_last (GTK_TOOLBAR (toolbar)->children);
  toolbar_child = elem->data;
  gtk_label_set_text_with_mnemonic (GTK_LABEL (toolbar_child->label),
				    _("Optio_ns"));

  image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, icon_size);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "",
			   _("Write Source Code"), "", image,
			   GTK_SIGNAL_FUNC (glade_project_window_write_source),
			   NULL);
  elem = g_list_last (GTK_TOOLBAR (toolbar)->children);
  toolbar_child = elem->data;
  gtk_label_set_text_with_mnemonic (GTK_LABEL (toolbar_child->label),
				    _("_Build"));

  /* Create list of components */
  project_window->project_view = glade_project_view_new ();
  gtk_widget_show (project_window->project_view);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win),
		     project_window->project_view);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolled_win);
  gtk_box_pack_start (GTK_BOX (vbox_main), scrolled_win, TRUE, TRUE, 0);

  /* Create status bar. */
  project_window->statusbar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox_main), project_window->statusbar,
		      FALSE, FALSE, 0);
  gtk_widget_show (project_window->statusbar);

  return project_window;
}
#endif


/* This returns the GladeProjectWindow given any widget in the window,
   e.g. a menuitem or a toolbar item, or NULL if not found. */
static GladeProjectWindow*
get_glade_project_window (GtkWidget *widget)
{
  /* We just use a global now. */
  return project_window;

#if 0
  return (gtk_object_get_data (GTK_OBJECT (glade_util_get_toplevel (widget)),
			       GladeProjectWindowKey));
#endif
}


static gboolean
glade_project_window_delete_event (GtkWidget *widget,
				   GdkEvent *event,
				   GladeProjectWindow *project_window)
{
  glade_project_window_show_quit_dialog (project_window);
  return TRUE;
}


static void
glade_project_window_destroy (GtkWidget *widget,
			      gpointer   data)
{
  GladeProjectWindow *project_window;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  gtk_widget_destroy (project_window->window);
  g_free (project_window->current_directory);
  g_free (project_window);

  gtk_exit (0);
}


static void
glade_project_window_new_project (GtkWidget *widget,
				  gpointer   data)
{
  GladeProjectWindow *project_window;
  GtkWidget *dialog, *label;
  gint response;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  /* If we don't currently have a project, there's no point in prompting for
     confirmation. */
  if (current_project == NULL)
    {
      glade_project_window_on_new_project_ok (project_window);
      return;
    }

  dialog = gtk_dialog_new_with_buttons (_("Glade"),
					GTK_WINDOW (project_window->window),
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OK, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  label = gtk_label_new (_("Are you sure you want to create a new project?"));
  gtk_misc_set_padding (GTK_MISC (label), 20, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "new_project", "Glade");

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_OK)
    glade_project_window_on_new_project_ok (project_window);
}


/* This sets the palette sensitive if a project is open or insensitive if
   not, and shows/hides the GNOME widgets as appropriate. */
static void
glade_project_window_setup_interface (GladeProjectWindow *project_window)
{
  gboolean gnome_support = TRUE;
  gboolean gnome_db_support = TRUE;

  /* Make sure the arrow is selected in the palette. */
  glade_palette_reset_selection (GLADE_PALETTE (glade_palette), FALSE);

  if (current_project)
    {
      gnome_support = glade_project_get_gnome_support (current_project);
      gnome_db_support = glade_project_get_gnome_db_support (current_project);
    }

  glade_palette_set_show_gnome_widgets (GLADE_PALETTE (glade_palette),
					gnome_support, gnome_db_support);

  gtk_widget_set_sensitive (GTK_BIN (glade_palette)->child,
			    current_project ? TRUE : FALSE);
}


static void
glade_project_window_on_new_project_ok (GladeProjectWindow *project_window)
{
  GladeProject *project;

  /* In the GNOME version we need to ask the user to specify whether they
     want a GTK+ or GNOME project. The menu stock systems are different. */
#ifdef USE_GNOME
#define RESPONSE_GNOME	1
#define RESPONSE_GTK	2

  GtkWidget *dialog, *label;
  gint response;

  dialog = gtk_dialog_new_with_buttons (_("New Project"),
					GTK_WINDOW (project_window->window),
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					_("New _GTK+ Project"), RESPONSE_GTK,
					_("New G_NOME Project"), RESPONSE_GNOME,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), RESPONSE_GNOME);
  label = gtk_label_new (_("Which type of project do you want to create?"));
  gtk_misc_set_padding (GTK_MISC (label), 20, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "new_project_type", "Glade");

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  if (response == GTK_RESPONSE_CANCEL)
    return;

  project = glade_project_new ();

  /* In the GNOME version, gnome_support defaults to TRUE, so we turn it off
     if the user requested a GTK+ project. */
  if (response == RESPONSE_GTK)
    glade_project_set_gnome_support (project, FALSE);

#else

  project = glade_project_new ();

#endif

  glade_project_view_set_project (GLADE_PROJECT_VIEW (project_window->project_view), project);
  glade_project_window_update_title (project_window);

  gtk_statusbar_pop (GTK_STATUSBAR (project_window->statusbar), 1);
  gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
		      _("New project created."));

  /* Make sure the palette is sensitive now and shows the appropriate
     widgets. */
  glade_project_window_setup_interface (project_window);
}


static void
glade_project_window_on_open_project (GtkWidget *widget,
				      gpointer   data)
{
  GladeProjectWindow *project_window;
  GtkWidget *filesel;
  GtkFileFilter *filter;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  filesel = gtk_file_chooser_dialog_new (_("Open Project"),
					 GTK_WINDOW (project_window->window),
					 GTK_FILE_CHOOSER_ACTION_OPEN,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					 NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (filesel), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);

  g_signal_connect (filesel, "response",
		    GTK_SIGNAL_FUNC (glade_project_window_real_open_project),
		    project_window);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "Glade Files");
  gtk_file_filter_add_pattern (filter, "*.glade");
  gtk_file_filter_add_pattern (filter, "*.glade2");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filesel), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "All Files");
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filesel), filter);

  if (project_window->current_directory)
    glade_util_set_file_selection_filename (filesel, project_window->current_directory);

  gtk_widget_show (filesel);
}


static void
glade_project_window_real_open_project (GtkWidget          *filesel,
					gint		    response_id,
					GladeProjectWindow *project_window)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      gchar *filename = glade_util_get_file_selection_filename (filesel);
      if (filename)
	glade_project_window_open_project (project_window, filename);
      g_free (filename);
    }
  gtk_widget_destroy (filesel);
}


void
glade_project_window_open_project (GladeProjectWindow *project_window,
				   const gchar        *filename)
{
  GladeProject *project;
  gboolean status;

  /* Clear the project view. Otherwise we may show an invalid project for
     a while. */
  glade_project_view_set_project (GLADE_PROJECT_VIEW (project_window->project_view), NULL);

  if (filename)
    {
      g_free (project_window->current_directory);
      project_window->current_directory = glade_util_dirname (filename);
    }

  status = glade_project_open (filename, &project);

  gtk_statusbar_pop (GTK_STATUSBAR (project_window->statusbar), 1);
  if (status)
    {
      glade_project_view_set_project (GLADE_PROJECT_VIEW (project_window->project_view), project);
      gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
			  _("Project opened."));
    }
  else
    {
      /* For GNOME we set the current project to NULL - we can't create a new
	 project without asking if it is for GNOMR or GTK+. For GTK+ we can
	 create a new empty GTK+ project. */
#ifdef USE_GNOME
      project = NULL;
#else
      project = glade_project_new ();
#endif
      glade_project_view_set_project (GLADE_PROJECT_VIEW (project_window->project_view), project);
      gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
			  _("Error opening project."));
    }

  /* Make sure the palette is sensitive if needed and show the appropriate
     widgets. */
  glade_project_window_setup_interface (project_window);

  glade_project_window_update_title (project_window);
}


static void
glade_project_window_drag_data_received (GtkWidget *widget,
                                         GdkDragContext *context,
                                         gint x, gint y,
                                         GtkSelectionData *selection_data,
                                         guint info, guint time, gpointer data)
{
  GladeProjectWindow *project_window;
  gchar *uri_list;
  GList *list = NULL;

  if (info != TARGET_URI_LIST)
    return;

  /*
   * Mmh... it looks like on Windows selection_data->data is not
   * NULL terminated, so we need to make sure our local copy is.
   */
  uri_list = g_new (gchar, selection_data->length + 1);
  memcpy (uri_list, selection_data->data, selection_data->length);
  uri_list[selection_data->length] = 0;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  /* For now, if more than a file is dragged into glade we actually
   * only try the first one... we can open only one anyway.
   */
  list = glade_util_uri_list_parse (uri_list);
  if (list->data)
    glade_project_window_open_project (project_window, list->data);

  /* we can now free each string in the list */
  g_list_foreach (list, (GFunc) g_free, NULL);
  g_list_free (list);
}


/* This shows the errors in a dialog, and frees them. */
/* Currently unused, since we switched to using libxml. */
#if 0
static void
glade_project_window_show_loading_errors (GladeProjectWindow *project_window,
					  GList		     *errors)
{
  GtkWidget *dialog, *text;
  GtkTextBuffer *buffer;
  GList *element;
  gchar *message, buf[16];

  dialog = glade_project_window_new_errors_dialog (project_window, &text);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));

  gtk_window_set_title (GTK_WINDOW (dialog), _("Errors opening project file"));
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "error", "Glade");

  sprintf (buf, "\n%i", g_list_length (errors));
  gtk_text_buffer_insert_at_cursor (buffer, buf, -1);

  message = _(" errors opening project file:");
  gtk_text_buffer_insert_at_cursor (buffer, message, -1);
  gtk_text_buffer_insert_at_cursor (buffer, "\n\n", 2);

  element = errors;
  while (element)
    {
      message = (gchar*) element->data;
      gtk_text_buffer_insert_at_cursor (buffer, message, -1);
      g_free (message);
      element = element->next;
    }
  g_list_free (errors);

  gtk_widget_show (dialog);
}
#endif


static GtkWidget*
glade_project_window_new_errors_dialog (GladeProjectWindow *project_window,
					GtkWidget **text_return)
{
  GtkWidget *dialog, *vbox, *text, *hbbox, *ok_button, *scrolled_win;

  dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (dialog), vbox);

  text = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);
  gtk_widget_show (text);
  gtk_widget_set_usize (text, 400, 150);
  GTK_WIDGET_UNSET_FLAGS (text, GTK_CAN_FOCUS);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), text);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show(scrolled_win);

  hbbox = gtk_hbutton_box_new ();
  gtk_widget_show (hbbox);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbbox), GTK_BUTTONBOX_END);
  gtk_box_pack_start (GTK_BOX (vbox), hbbox, FALSE, TRUE, 0);

  ok_button = gtk_button_new_with_label (_("OK"));
  gtk_widget_show (ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);
  gtk_container_add (GTK_CONTAINER (hbbox), ok_button);
  gtk_widget_grab_default (ok_button);
  gtk_signal_connect_object (GTK_OBJECT (ok_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_signal_connect (GTK_OBJECT (dialog), "key_press_event",
		      GTK_SIGNAL_FUNC (glade_util_check_key_is_esc),
		      GINT_TO_POINTER (GladeEscDestroys));

  *text_return = text;
  return dialog;
}


/* This shows a warning dialog when the user tries to use a command like
   Save/Options/Build when we currently have no project. */
static void
glade_project_window_show_no_project_warning (GladeProjectWindow *project_window)
{
  glade_util_show_message_box (_("There is no project currently open.\n"
				 "Create a new project with the Project/New command."),
			       project_window->window);
}


static void
glade_project_window_on_edit_options (GtkWidget *widget,
				      gpointer   data)
{
  GladeProjectWindow *project_window;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  glade_project_window_edit_options (project_window,
				     GLADE_PROJECT_OPTIONS_ACTION_NORMAL);
}


/* The action specifies what to do when 'OK' is clicked. It can be NULL, to
   simply update the options, or 'Save' or 'Build' to save the XML file, or
   build the source code. */
static void
glade_project_window_edit_options (GladeProjectWindow *project_window,
				   GladeProjectOptionsAction action)
{
  GladeProject *project;
  GtkWidget *options;

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  if (project)
    {
      options = glade_project_options_new (project);
      gtk_signal_connect (GTK_OBJECT (GLADE_PROJECT_OPTIONS (options)->ok_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (glade_project_window_options_ok),
			  project_window);
      if (action)
	glade_project_options_set_action (GLADE_PROJECT_OPTIONS (options),
					  action);
      if (GTK_IS_WINDOW (project_window->window))
	gtk_window_set_transient_for (GTK_WINDOW (options),
				      GTK_WINDOW (project_window->window));
      				      
      gtk_widget_show (options);
    }
  else
    {
      glade_project_window_show_no_project_warning (project_window);
    }
}


static void
glade_project_window_options_ok (GtkWidget	    *widget,
				 GladeProjectWindow *project_window)
{
  GladeProjectOptions *options;
  GladeProjectOptionsAction action;
  gboolean saved;

  options = GLADE_PROJECT_OPTIONS (gtk_widget_get_toplevel (widget));
  action = options->action;

  glade_project_window_update_title (project_window);
  glade_project_window_setup_interface (project_window);

  gtk_widget_destroy (GTK_WIDGET (options));

  switch (action)
    {
    case GLADE_PROJECT_OPTIONS_ACTION_NORMAL:
      /* Don't need to do anything here. */
      break;
    case GLADE_PROJECT_OPTIONS_ACTION_SAVE:
      glade_project_window_real_save_project (project_window, FALSE);
      break;
    case GLADE_PROJECT_OPTIONS_ACTION_BUILD:
      saved = glade_project_window_real_save_project (project_window, FALSE);
      if (saved)
	glade_project_window_real_write_source (project_window);
      break;
    }
}


static void
glade_project_window_save_project (GtkWidget *widget,
				   gpointer   data)
{
  GladeProjectWindow *project_window;
  GladeProject *project;
  gchar *xml_filename = NULL;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  if (project)
    {
      xml_filename = glade_project_get_xml_filename (project);

      /* If the XML filename isn't set, show the project options dialog,
	 and tell it we are saving. */
      if (xml_filename == NULL || xml_filename[0] == '\0')
	glade_project_window_edit_options (project_window,
					   GLADE_PROJECT_OPTIONS_ACTION_SAVE);
      else
	glade_project_window_real_save_project (project_window, FALSE);
    }
  else
    {
      glade_project_window_show_no_project_warning (project_window);
    }
}


#if 0
static void
glade_project_window_on_save_project_as (GtkWidget *widget,
					 gpointer   data)
{
  GladeProjectWindow *project_window;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  glade_project_window_save_project_as (project_window);
}


static void
glade_project_window_save_project_as (GladeProjectWindow *project_window)
{
  GtkWidget *filesel;

  filesel = gtk_file_chooser_dialog_new (_("Save Project"),
					 GTK_WINDOW (project_window->window),
					 GTK_FILE_CHOOSER_ACTION_SAVE,
					 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					 GTK_STOCK_SAVE, GTK_RESPONSE_OK,
					 NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (filesel), GTK_RESPONSE_OK);
  gtk_window_set_position (GTK_WINDOW (filesel), GTK_WIN_POS_MOUSE);
 
  g_signal_connect (options->filesel, "response",
		    GTK_SIGNAL_FUNC (glade_project_window_save_as_callback),
		    project_window);

  if (project_window->current_directory)
    glade_util_set_file_selection_filename (filesel, project_window->current_directory);

  gtk_widget_show (filesel);
}


static void
glade_project_window_save_as_callback (GtkWidget          *filesel,
				       gint		   response_id,
				       GladeProjectWindow *project_window)
{
  GladeProject *project;
  gchar *filename;

  if (response_id == GTK_RESPONSE_OK)
    {
      filename = glade_util_get_file_selection_filename (filesel);

      g_free (project_window->current_directory);
      project_window->current_directory = glade_util_dirname (filename);

      project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
      if (project)
	{
	  glade_project_set_xml_filename (project, filename);
	  glade_project_window_real_save_project (project_window, TRUE);
	}
      g_free (filename);
    }

  gtk_widget_destroy (filesel);
}
#endif


/* Returns TRUE if the XML file is saved OK.
   FIXME: Doesn't use warn_before_overwrite. */
static gboolean
glade_project_window_real_save_project (GladeProjectWindow *project_window,
					gboolean            warn_before_overwrite)
{
  GladeProject *project;
  GladeError *error;

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  g_return_val_if_fail (project != NULL, GLADE_STATUS_ERROR);

  error = glade_project_save (project);

  gtk_statusbar_pop (GTK_STATUSBAR (project_window->statusbar), 1);
  if (error)
    {
      glade_project_window_show_error (project_window, error,
				       _("Error saving project"));
      gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
			  _("Error saving project."));
      glade_error_free (error);
      return FALSE;
    }
  else
    gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
			_("Project saved."));
  return TRUE;
}


static void
glade_project_window_write_source (GtkWidget *widget,
				   gpointer   data)
{
  GladeProjectWindow *project_window;
  GladeProject *project;
  gchar *directory, *xml_filename, *project_name, *program_name;
  gchar *source_directory, *pixmaps_directory;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  if (!project)
    {
      glade_project_window_show_no_project_warning (project_window);
      return;
    }

  /* First we need to make sure we have an XML file, a project name, and a
     program name. */
  directory = glade_project_get_directory (project);
  xml_filename = glade_project_get_xml_filename (project);
  project_name = glade_project_get_name (project);
  program_name = glade_project_get_program_name (project);
  source_directory = glade_project_get_source_directory (project);
  pixmaps_directory = glade_project_get_pixmaps_directory (project);

  /* First check we have all the options we need. If not show the project
     options dialog, and tell it we are building the source. */
  if (directory == NULL || directory[0] == '\0'
      || xml_filename == NULL || xml_filename[0] == '\0'
      || project_name == NULL || project_name[0] == '\0'
      || program_name == NULL || program_name[0] == '\0'
      || source_directory == NULL || source_directory[0] == '\0'
      || pixmaps_directory == NULL || pixmaps_directory[0] == '\0')
    {
      glade_project_window_edit_options (project_window,
					 GLADE_PROJECT_OPTIONS_ACTION_BUILD);
      return;
    }

  /* If there is an error saving the XML file we simply return. The user
     will already have been informed of the error. */
  if (!glade_project_window_real_save_project (project_window, FALSE))
    return;
  glade_project_window_real_write_source (project_window);
}


static void
glade_project_window_real_write_source (GladeProjectWindow *project_window)
{
  GladeProject *project;
  GladeError *error;

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  g_return_if_fail (project != NULL);

  error = glade_project_write_source (project);

  gtk_statusbar_pop (GTK_STATUSBAR (project_window->statusbar), 1);
  if (error)
    {
      glade_project_window_show_error (project_window, error,
				       _("Errors writing source code"));
      gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
			  _("Error writing source."));
      glade_error_free (error);
      return;
    }

  gtk_statusbar_push (GTK_STATUSBAR (project_window->statusbar), 1,
		      _("Source code written."));
}


/* This shows a single GladeError in a dialog. */
static void
glade_project_window_show_error (GladeProjectWindow *project_window,
				 GladeError         *error,
				 gchar		    *title)
{
  GtkWidget *dialog, *text;
  GtkTextBuffer *buffer;
  gchar *message;

  dialog = glade_project_window_new_errors_dialog (project_window, &text);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "error", "Glade");
  if (GTK_IS_WINDOW (project_window->window))
    gtk_window_set_transient_for (GTK_WINDOW (dialog),
				      GTK_WINDOW (project_window->window));

  gtk_text_buffer_insert_at_cursor (buffer, error->message, -1);

  /* For system errors, we also output the system error message. */
  if (error->status == GLADE_STATUS_SYSTEM_ERROR)
    {
      /* Insert a blank line between our message and the system message. */
      gtk_text_buffer_insert_at_cursor (buffer, "\n", 1);

      message = _("System error message:");
      gtk_text_buffer_insert_at_cursor (buffer, message, -1);

      /* Place the system error message on the next line, indented slightly. */
      gtk_text_buffer_insert_at_cursor (buffer, "\n  ", -1);

      message = (char*) g_strerror (error->system_errno);
      gtk_text_buffer_insert_at_cursor (buffer, message, -1);
    }

  gtk_widget_show (dialog);
}


static void
glade_project_window_quit (GtkWidget *widget,
			   gpointer   data)
{
  GladeProjectWindow *project_window;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  glade_project_window_show_quit_dialog (project_window);
}


static void
glade_project_window_show_quit_dialog (GladeProjectWindow *project_window)
{
  GtkWidget *dialog, *label;

  dialog = gtk_dialog_new_with_buttons (_("Glade"),
					GTK_WINDOW (project_window->window),
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_QUIT, GTK_RESPONSE_OK,
					NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  label = gtk_label_new (_("Are you sure you want to quit?"));
  gtk_misc_set_padding (GTK_MISC (label), 20, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label,
		      TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "quit", "Glade");

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      glade_save_settings (project_window, glade_palette, win_property,
			   win_tree, glade_clipboard);
      gtk_widget_destroy (project_window->window);
    }

  gtk_widget_destroy (dialog);
}


static void
glade_project_window_cut (GtkWidget *widget,
			  gpointer   user_data)
{
  GladeProjectWindow *project_window;
  GladeProject *project;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  if (project == NULL)
    return;

  glade_clipboard_cut (GLADE_CLIPBOARD (glade_clipboard), project, NULL);
}


static void
glade_project_window_copy (GtkWidget *widget,
			   gpointer   data)
{
  GladeProjectWindow *project_window;
  GladeProject *project;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  if (project == NULL)
    return;

  glade_clipboard_copy (GLADE_CLIPBOARD (glade_clipboard), project, NULL);
}


static void
glade_project_window_paste (GtkWidget *widget,
			    gpointer  user_data)
{
  GladeProjectWindow *project_window;
  GladeProject *project;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));
  if (project == NULL)
    return;

  glade_clipboard_paste (GLADE_CLIPBOARD (glade_clipboard), project, NULL);
}


static void
glade_project_window_delete (GtkWidget *widget,
			     gpointer   data)
{
  GladeProjectWindow *project_window;

  project_window = get_glade_project_window (widget);
  g_return_if_fail (project_window != NULL);

  glade_project_window_real_delete (project_window);
}


/* If one or more items in the project view is selected, we delete them.
   If not, we delete any widgets selected in the interface.
   FIXME: I'm not sure of the correct way to handle the Delete key. Should we
   be using X selections to determine what is currently selected? */
static void
glade_project_window_real_delete (GladeProjectWindow *project_window)
{
  if (glade_project_view_has_selection (GLADE_PROJECT_VIEW (project_window->project_view)))
    glade_project_view_delete_selection (GLADE_PROJECT_VIEW (project_window->project_view));
  else
    editor_on_delete ();
}


static void
refresh_menu_item (GtkWidget *menuitem, GtkWidget *window)
{
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
				  GTK_WIDGET_VISIBLE (window));
}

void
glade_project_window_refresh_menu_items (void)
{
  refresh_menu_item (palette_item, glade_palette);
  refresh_menu_item (property_editor_item, win_property);
  refresh_menu_item (widget_tree_item, win_tree);
  refresh_menu_item (clipboard_item, glade_clipboard);
}


void
glade_project_window_uncheck_palette_menu_item (void)
{
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (palette_item),
				  FALSE);
}


void
glade_project_window_uncheck_property_editor_menu_item (void)
{
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (property_editor_item),
				  FALSE);
}


void
glade_project_window_uncheck_widget_tree_menu_item (void)
{
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget_tree_item),
				  FALSE);
}


void
glade_project_window_uncheck_clipboard_menu_item (void)
{
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (clipboard_item),
				  FALSE);
}


static void
glade_project_window_toggle_palette (GtkWidget *widget,
				      gpointer   data)
{
  gboolean show_palette;

  show_palette = GTK_CHECK_MENU_ITEM (widget)->active;
  if (show_palette)
    glade_show_palette ();
  else
    glade_hide_palette ();
}


static void
glade_project_window_toggle_property_editor (GtkWidget *widget,
					   gpointer   data)
{
  gboolean show_property_editor;

  show_property_editor = GTK_CHECK_MENU_ITEM (widget)->active;
  if (show_property_editor)
    glade_show_property_editor ();
  else
    glade_hide_property_editor ();
}


static void
glade_project_window_toggle_widget_tree (GtkWidget *widget,
				       gpointer   data)
{
  gboolean show_widget_tree;

  show_widget_tree =  GTK_CHECK_MENU_ITEM (widget)->active;
  if (show_widget_tree)
    glade_show_widget_tree ();
  else
    glade_hide_widget_tree ();
}


static void
glade_project_window_toggle_clipboard (GtkWidget *widget,
				     gpointer   data)
{
  gboolean show_clipboard;

  show_clipboard =  GTK_CHECK_MENU_ITEM (widget)->active;
  if (show_clipboard)
    glade_show_clipboard ();
  else
    glade_hide_clipboard ();
}


static void
glade_project_window_toggle_tooltips (GtkWidget *widget,
				      gpointer   data)
{
  gboolean show_tooltips;

  show_tooltips =  GTK_CHECK_MENU_ITEM (widget)->active;
  gb_widget_set_show_tooltips (show_tooltips);
}


static void
glade_project_window_toggle_grid (GtkWidget *widget,
				  gpointer   data)
{
  gboolean show_grid;

  show_grid = GTK_CHECK_MENU_ITEM (widget)->active;
  editor_set_show_grid (show_grid);
}


static void
glade_project_window_edit_grid_settings (GtkWidget *widget,
					 gpointer   data)
{
  editor_show_grid_settings_dialog (widget);
}


static void
glade_project_window_toggle_snap (GtkWidget *widget,
				  gpointer   data)
{
  gboolean snap_to_grid;

  snap_to_grid = GTK_CHECK_MENU_ITEM (widget)->active;
  editor_set_snap_to_grid (snap_to_grid);
}


static void
glade_project_window_edit_snap_settings (GtkWidget *widget,
					 gpointer   data)
{
  editor_show_snap_settings_dialog (widget);
}


static void
glade_project_window_about (GtkWidget *widget,
			    gpointer   data)
{
  /* VERSION comes from configure.in - the only place it should be defined */

#ifdef USE_GNOME
  static GtkWidget *about = NULL;

  if (about == NULL )
    {
      const gchar *author[] = { "Damon Chaplin <damon@gnome.org>",
				"Martijn van Beers <martijn@earthling.net>",
				"Jacob Berkman <jacob@ximian.com>",
				NULL };
      const gchar *documenter[] = { NULL };
      GtkWidget *transient_parent;
      GdkPixbuf *pixbuf = NULL;
      char *filename;

      filename = gnome_program_locate_file (gnome_program_get (),
					    GNOME_FILE_DOMAIN_APP_PIXMAP,
					    "glade-2.png", /* "glade-2/glade_logo.png", */
					    TRUE, NULL);
      if (filename)
	pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

      about = gnome_about_new (_("Glade"),
			       VERSION,
			       _("(C) 1998-2002 Damon Chaplin"),
			       _("Glade is a User Interface Builder for GTK+ and GNOME."),
			       author, documenter, NULL, pixbuf);
      if (pixbuf)
	gdk_pixbuf_unref (pixbuf);
			   
      /* set the widget pointer to NULL when the widget is destroyed */
      g_signal_connect (G_OBJECT (about), "destroy",
			G_CALLBACK (gtk_widget_destroyed),
			&about);
      gtk_window_set_wmclass (GTK_WINDOW (about), "about", "Glade");
      transient_parent = glade_util_get_toplevel (widget);
      if (GTK_IS_WINDOW (transient_parent))
	gtk_window_set_transient_for (GTK_WINDOW (about),
				      GTK_WINDOW (transient_parent));

      gtk_widget_show_all (GTK_WIDGET (about));
    }
  gtk_window_present (GTK_WINDOW (about));

#else
  static  GtkWidget *about = NULL;

  if (about == NULL )
    {
      GtkWidget *vbox, *image, *label;
      GtkWindow *transient_parent;
      char *filename, *text;

      transient_parent = GTK_WINDOW (glade_util_get_toplevel (widget));
      about = gtk_dialog_new_with_buttons (_("About Glade"), 
					   transient_parent,
					   0, 
					   GTK_STOCK_OK, 
					   GTK_RESPONSE_OK, 
					   NULL);
      gtk_window_set_resizable (GTK_WINDOW (about), FALSE);
      g_signal_connect (G_OBJECT (about), "response",
			G_CALLBACK (gtk_widget_destroy), NULL);
      /* set the widget pointer to NULL when the widget is destroyed */
      g_signal_connect (G_OBJECT (about), "destroy",
			G_CALLBACK (gtk_widget_destroyed),
			&about);
      gtk_window_set_wmclass (GTK_WINDOW (about), "about", "Glade");

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about)->vbox), 
			  vbox, TRUE, TRUE, 0);

      filename = g_strdup_printf ("%s/pixmaps/glade-2.png", GLADE_DATADIR);
      image = gtk_image_new_from_file (filename);
      g_free (filename);
      gtk_box_pack_start (GTK_BOX (vbox), image, TRUE, TRUE, 8);

      text = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">Glade "
			  VERSION "</span>\n\n"
			 "%s\n\n"
			  "<span size=\"small\">%s</span>",
			  _("Glade is a User Interface Builder for GTK+ and GNOME."),
			  _("(C) 1998-2002 Damon Chaplin"));

      label = gtk_label_new (text);
      gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
      gtk_widget_show_all (GTK_WIDGET (about));
    }
  gtk_window_present (GTK_WINDOW (about));
#endif
}


static gint
glade_project_window_key_press_event (GtkWidget * widget,
				      GdkEventKey * event,
				      gpointer data)
{
  GladeProjectWindow *project_window;

  project_window = get_glade_project_window (widget);
  g_return_val_if_fail (project_window != NULL, FALSE);

  switch (event->keyval)
    {
    case GDK_Delete:
      glade_project_window_real_delete (project_window);
      break;
    }
  return TRUE;
}


void
glade_project_window_set_project	(GladeProjectWindow *project_window,
					 GladeProject       *project)
{
  glade_project_view_set_project (GLADE_PROJECT_VIEW (project_window->project_view),
				  project);
  glade_project_window_update_title (project_window);
}


static void
glade_project_window_update_title	(GladeProjectWindow *project_window)
{
  GladeProject *project;
  gchar *title, *project_name = NULL;

  project = glade_project_view_get_project (GLADE_PROJECT_VIEW (project_window->project_view));

  if (project)
    project_name = glade_project_get_name (project);

  if (!project_name || !project_name[0])
    project_name = _("<untitled>");

  title = g_strdup_printf ("Glade: %s", project_name);
  gtk_window_set_title (GTK_WINDOW (project_window->window), title);
  g_free (title);
}
