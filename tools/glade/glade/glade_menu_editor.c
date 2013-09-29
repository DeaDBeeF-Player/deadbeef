
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

/*
 * The Menu Editor window, based on initial work by Javier Arriero PaÅÌs
 * and John Looney.
 */

#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkarrow.h>
#include <gtk/gtkaccellabel.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkcombo.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkeventbox.h>
#include <gtk/gtkfilechooserdialog.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkimagemenuitem.h>
#include <gtk/gtklist.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkradiomenuitem.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkseparatormenuitem.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtktearoffmenuitem.h>
#include <gtk/gtkvbox.h>

#include "gladeconfig.h"

#ifdef USE_GNOME
#include <gnome.h>
#include "glade_gnome.h"
#endif

#include "gb.h"
#include "glade_menu_editor.h"
#include "glade_keys_dialog.h"
#include "glade_project.h"
#include "gbwidget.h"
#include "tree.h"
#include "utils.h"

#define GladeMenuEditorIndexKey "glade-menu-editor-index-key"
#define GladeMenuEditorStockIDKey "glade-menu-editor-stock-id"

/* How many pixels to indent levels of the menu hierarchy in the clist. */
#define GB_INDENT	10

/* The text to display if the item is a separator. */
#define GB_SEPARATOR_TEXT "---"

/* This sets the order of the clist columns. */
#define GB_MENUED_NUM_COLS 8

#define GLD_COL_LABEL	0
#define GLD_COL_TYPE	1
#define GLD_COL_ACCEL	2
#define GLD_COL_NAME	3
#define GLD_COL_HANDLER	4
#define GLD_COL_ACTIVE	5
#define GLD_COL_GROUP	6
#define GLD_COL_ICON	7

typedef enum
  {
    GB_MENU_ITEM_NORMAL,
    GB_MENU_ITEM_CHECK,
    GB_MENU_ITEM_RADIO
  }
GbMenuItemType;


/* This holds information on one menu item. */
typedef struct _GbMenuItemData GbMenuItemData;
struct _GbMenuItemData
  {
    gint stock_item_index;	/* Stock menu item index, 0 == None. */
    gchar *label;		/* Text to display. */
    gchar *name;		/* Widget name. */
    gchar *handler;		/* Handler to call when selected. */
    time_t last_mod_time;	/* The time the handler was last updated. */
    gchar *icon;		/* Icon filename, or Stock icon name. */
    gchar *tooltip;		/* Tooltip text. */
    GbMenuItemType type;	/* Type - normal/check/radio. */
    gboolean active;		/* If the item is initially active. */
    GbMenuItemData *group;	/* Points to the item data of the first widget
				   in the radio group, or NULL if it is in its
				   own group or is the first item in group. */
    guint8 modifiers;		/* Control/Shift/Alt flags. */
    gchar *key;			/* Name of accelerator key. */

    gint level;			/* Level in menu hierarchy. */
    gboolean generate_name;	/* If the name should be auto-generated. */
    gboolean generate_handler;	/* If the handler should be auto-generated. */

    GladeWidgetData *wdata;	/* The widget data associated with the old
				   menu item, if there was one. */
  };


static void glade_menu_editor_class_init (GladeMenuEditorClass * klass);
static void glade_menu_editor_init (GladeMenuEditor * dialog);
static void glade_menu_editor_destroy (GtkObject *object);

/* If the menu widget we are editing is destroyed, the menu editor is destroyed
   as well. */
static void glade_menu_editor_on_menu_destroyed (GtkWidget *menu,
						 GtkWidget *menued);

static void on_menu_editor_ok (GtkWidget *button,
			       GladeMenuEditor *menued);
static void on_menu_editor_apply (GtkWidget *button,
				  GladeMenuEditor *menued);
static void on_menu_editor_close (GtkWidget *widget,
				  GladeMenuEditor *menued);

/* This sets the menubar/popup menu whose children are displayed in the
   menu editor, i.e. converted to items in the clist. */
static void glade_menu_editor_set_menu	 (GladeMenuEditor *menued,
					  GtkMenuShell    *menu);

/* This updates the given widget, based on the settings in the menu editor.
   It removes all the current children of the menu and recreates it. */
static void glade_menu_editor_update_menu (GladeMenuEditor *menued);


static gboolean on_key_press (GtkWidget * widget,
			      GdkEventKey * event,
			      gpointer user_data);
static void on_clist_select_row (GtkWidget * clist,
				 gint row,
				 gint column,
				 GdkEventButton * event,
				 gpointer user_data);
static void on_clist_unselect_row (GtkWidget * clist,
				   gint row,
				   gint column,
				   GdkEventButton * event,
				   gpointer user_data);
static void on_entry_changed (GtkWidget * entry,
			      gpointer user_data);
static void on_icon_button_clicked (GtkWidget * button,
				    gpointer user_data);
static void on_icon_filesel_response (GtkWidget * filesel,
				      gint response_id,
				      GladeMenuEditor *menued);
static void on_stock_item_entry_changed (GtkWidget * entry,
					 gpointer user_data);
static gboolean on_label_entry_key_press (GtkWidget * widget,
					  GdkEventKey * event,
					  gpointer user_data);
static void on_radiobutton_toggled (GtkWidget * togglebutton,
				    gpointer user_data);
static void on_checkbutton_toggled (GtkWidget * togglebutton,
				    gpointer user_data);
static void on_state_button_toggled (GtkToggleButton * togglebutton,
				     gpointer user_data);
static void on_accel_key_button_clicked (GtkButton * button,
					 gpointer user_data);
static void on_up_button_clicked (GtkButton * button,
				  gpointer user_data);
static void on_down_button_clicked (GtkButton * button,
				    gpointer user_data);
static void on_left_button_clicked (GtkButton * button,
				    gpointer user_data);
static void on_right_button_clicked (GtkButton * button,
				     gpointer user_data);
static void on_add_button_clicked (GtkWidget * button,
				   gpointer user_data);
static void on_add_child_button_clicked (GtkWidget * button,
					 gpointer user_data);
static void on_add_separator_button_clicked (GtkWidget * button,
					     gpointer user_data);
static void add_item (GladeMenuEditor * menued,
		      gboolean as_child,
		      gboolean separator);
static void on_delete_button_clicked (GtkWidget * widget,
				      gpointer user_data);

static void on_keys_dialog_clist_select (GtkWidget * widget,
					 gint row,
					 gint column,
					 GdkEventButton * bevent,
					 GladeMenuEditor * menued);
static void on_keys_dialog_response (GtkWidget * widget, gint response_id,
				     GladeMenuEditor * menued);

static gint get_selected_row (GladeMenuEditor * menued);
static GbMenuItemData* get_selected_item (GladeMenuEditor * menued);
static void set_interface_state (GladeMenuEditor * menued);
static gchar *get_accel_string (gchar * key,
				guint8 modifiers);
static void insert_item (GtkCList * clist,
			 GbMenuItemData * item,
			 gint row);
static void ensure_visible (GtkWidget *clist,
			    gint row);
static void update_current_item (GladeMenuEditor * menued);
static gboolean item_property_changed (gchar *new, gchar *old);
static gchar* copy_item_property (gchar *property);
static void clear_form (GladeMenuEditor * menued,
			gboolean full);
static void show_item_properties (GladeMenuEditor * menued);
static void insert_items (GtkWidget * clist,
			  GList * items,
			  gint row);
static GList *remove_item_and_children (GtkWidget * clist,
					gint row);
static GtkWidget* create_radio_menu_item (GtkMenuShell *menu,
					  GbMenuItemData *item,
					  GHashTable *group_hash);
static gchar* generate_name (GladeMenuEditor *menued,
			     gchar *label);
static gchar* generate_handler (GladeMenuEditor *menued,
				gint row,
				gchar *label,
				gchar *name);
static void check_generated_handlers (GladeMenuEditor *menued);
static gboolean is_parent (GladeMenuEditor *menued,
			   gint row);
static void set_submenu (GladeMenuEditor *menued,
			 GtkMenuShell    *menu,
			 gint	      level);
static void glade_menu_editor_reset (GladeMenuEditor *menued);
static void glade_menu_editor_free_item (GbMenuItemData *item);

static void update_radio_groups (GladeMenuEditor * menued);
static void normalize_radio_groups (GladeMenuEditor * menued);
static GbMenuItemData* find_radio_group (GtkRadioMenuItem *menuitem,
					 GList **groups,
					 GbMenuItemData *item);
static void remove_from_radio_group (GladeMenuEditor * menued,
				     GbMenuItemData *item);

static GtkWindowClass *parent_class = NULL;


GType
glade_menu_editor_get_type (void)
{
  static GType glade_menu_editor_type = 0;

  if (!glade_menu_editor_type)
    {
      GtkTypeInfo glade_menu_editor_info =
      {
	"GladeMenuEditor",
	sizeof (GladeMenuEditor),
	sizeof (GladeMenuEditorClass),
	(GtkClassInitFunc) glade_menu_editor_class_init,
	(GtkObjectInitFunc) glade_menu_editor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      glade_menu_editor_type = gtk_type_unique (gtk_window_get_type (),
						&glade_menu_editor_info);
    }

  return glade_menu_editor_type;
}

static void
glade_menu_editor_class_init (GladeMenuEditorClass * class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_window_get_type ());

  object_class->destroy = glade_menu_editor_destroy;
}


/* FIXME: This is mostly a temporary hack until GtkEntry is fixed in GTK+
   2.0.x so we don't get "changed" twice when we call gtk_entry_set_text().
   Though we also need this to set it to "" when NULL is passed in. */
static void
set_entry_text (GtkEntry *entry,
		const gchar *text)
{
  gint tmp_pos;

  GtkEditable *editable;

  g_return_if_fail (GTK_IS_ENTRY (entry));

  if (!text)
    text = "";

  editable = GTK_EDITABLE (entry);
  
  g_signal_handlers_block_by_func (editable, on_entry_changed, NULL);
  gtk_editable_delete_text (GTK_EDITABLE(entry), 0, -1);
  g_signal_handlers_unblock_by_func (editable, on_entry_changed, NULL);

  tmp_pos = 0;
  gtk_editable_insert_text (editable, text, strlen (text), &tmp_pos);
}


static void
glade_menu_editor_init (GladeMenuEditor * menued)
{
  menued->keys_dialog = NULL;
  menued->filesel = NULL;
  menued->project = NULL;
  menued->menu = NULL;
  menued->updating_widgets = FALSE;
  menued->gnome_support = FALSE;
}


static void
glade_menu_editor_construct (GladeMenuEditor * menued,
			     GladeProject * project)
{
  GtkWidget *vbox2, *vbox1, *scrolled_win;
  GtkWidget *hbox1;
  GtkWidget *vbox3;
  GtkWidget *table1;
  GtkWidget *eventbox3;
  GtkWidget *eventbox2;
  GtkWidget *eventbox1;
  GtkWidget *table2;
  GSList *table2_group = NULL;
  GtkWidget *table3;
  GtkWidget *accel_key_button;
  GtkWidget *hbox2;
  GtkWidget *label9;
  GtkWidget *label8;
  GtkWidget *hbuttonbox3;
  GtkWidget *arrow1;
  GtkWidget *arrow2;
  GtkWidget *arrow3;
  GtkWidget *arrow4;
  GtkWidget *button_table;
  GtkWidget *hseparator1;
  GtkWidget *hbuttonbox1;
  GtkWidget *listitem;
  GtkTooltips *tooltips;
  gchar *titles[GB_MENUED_NUM_COLS];
  gint row;
  GSList *elem;
  gchar *stock_id;
  GtkWidget *icon_hbox;
  gchar *label_text;
  gint idx;
#ifdef USE_GNOME
  GnomeUIInfo *uiinfo;
  GtkWidget *pixmap, *label, *hbox, *separator;
#endif

  menued->project = project;
#ifdef USE_GNOME
  if (glade_project_get_gnome_support (project))
    menued->gnome_support = TRUE;
#endif

  tooltips = gtk_tooltips_new ();

  gtk_container_set_border_width (GTK_CONTAINER (menued), 8);
  gtk_window_set_title (GTK_WINDOW (menued), _ ("Menu Editor"));
  gtk_window_set_policy (GTK_WINDOW (menued), FALSE, TRUE, FALSE);
  gtk_window_set_wmclass (GTK_WINDOW (menued), "menu_editor", "Glade");

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (menued), vbox2);

  hbox1 = gtk_hbox_new (FALSE, 6);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);

  vbox1 = gtk_vbox_new (FALSE, 4);
  gtk_widget_show (vbox1);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);

  titles[GLD_COL_LABEL]		= _("Label");
  titles[GLD_COL_TYPE]		= _("Type");
  titles[GLD_COL_ACCEL]		= _("Accelerator");
  titles[GLD_COL_NAME]		= _("Name");
  titles[GLD_COL_HANDLER]	= _("Handler");
  titles[GLD_COL_ACTIVE]	= _("Active");
  titles[GLD_COL_GROUP]		= _("Group");
  titles[GLD_COL_ICON]		= _("Icon");

  menued->clist = gtk_clist_new_with_titles (GB_MENUED_NUM_COLS, titles);
  gtk_widget_show (menued->clist);
  GTK_WIDGET_SET_FLAGS (menued->clist, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (menued->clist), "key_press_event",
		      GTK_SIGNAL_FUNC (on_key_press),
		      NULL);
  gtk_widget_set_usize (menued->clist, 300, -1);
  gtk_signal_connect (GTK_OBJECT (menued->clist), "select_row",
		      GTK_SIGNAL_FUNC (on_clist_select_row), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->clist), "unselect_row",
		      GTK_SIGNAL_FUNC (on_clist_unselect_row), NULL);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_LABEL, 144);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_TYPE, 42);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_ACCEL, 120);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_NAME, 100);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_HANDLER, 172);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_ICON, 172);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_ACTIVE, 42);
  gtk_clist_set_column_width (GTK_CLIST (menued->clist), GLD_COL_GROUP, 75);
  gtk_clist_column_titles_show (GTK_CLIST (menued->clist));
  gtk_clist_column_titles_passive (GTK_CLIST (menued->clist));

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_win), menued->clist);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  hbuttonbox3 = gtk_hbutton_box_new ();
  gtk_widget_show (hbuttonbox3);
  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox3, FALSE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox3),
			     GTK_BUTTONBOX_SPREAD);
  gtk_box_set_spacing (GTK_BOX (hbuttonbox3), 6);

  menued->up_button = gtk_button_new ();
  gtk_widget_show (menued->up_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), menued->up_button);
  gtk_tooltips_set_tip (tooltips, menued->up_button, _ ("Move the item and its children up one place in the list"), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->up_button), "clicked",
		      GTK_SIGNAL_FUNC (on_up_button_clicked),
		      NULL);

  arrow1 = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_OUT);
  gtk_widget_show (arrow1);
  gtk_container_add (GTK_CONTAINER (menued->up_button), arrow1);

  menued->down_button = gtk_button_new ();
  gtk_widget_show (menued->down_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), menued->down_button);
  gtk_tooltips_set_tip (tooltips, menued->down_button, _ ("Move the item and its children down one place in the list"), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->down_button), "clicked",
		      GTK_SIGNAL_FUNC (on_down_button_clicked),
		      NULL);

  arrow2 = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_widget_show (arrow2);
  gtk_container_add (GTK_CONTAINER (menued->down_button), arrow2);

  menued->left_button = gtk_button_new ();
  gtk_widget_show (menued->left_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), menued->left_button);
  gtk_tooltips_set_tip (tooltips, menued->left_button, _ ("Move the item and its children up one level"), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->left_button), "clicked",
		      GTK_SIGNAL_FUNC (on_left_button_clicked),
		      NULL);

  arrow3 = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_OUT);
  gtk_widget_show (arrow3);
  gtk_container_add (GTK_CONTAINER (menued->left_button), arrow3);

  menued->right_button = gtk_button_new ();
  gtk_widget_show (menued->right_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), menued->right_button);
  gtk_tooltips_set_tip (tooltips, menued->right_button, _ ("Move the item and its children down one level"), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->right_button), "clicked",
		      GTK_SIGNAL_FUNC (on_right_button_clicked),
		      NULL);

  arrow4 = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_widget_show (arrow4);
  gtk_container_add (GTK_CONTAINER (menued->right_button), arrow4);

  vbox3 = gtk_vbox_new (FALSE, 6);
  gtk_widget_show (vbox3);
  gtk_box_pack_start (GTK_BOX (hbox1), vbox3, FALSE, TRUE, 0);

  table1 = gtk_table_new (5, 3, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox3), table1, FALSE, TRUE, 0);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 4);
  row = 0;

  menued->stock_items = gtk_stock_list_ids ();
  menued->stock_items = g_slist_sort (menued->stock_items,
				      glade_util_compare_stock_labels);

  if (!menued->gnome_support)
    {
      eventbox1 = gtk_event_box_new ();
      gtk_widget_show (eventbox1);
      gtk_table_attach (GTK_TABLE (table1), eventbox1, 0, 1, row, row + 1,
			GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_tooltips_set_tip (tooltips, eventbox1, _ ("The stock item to use."),
			    NULL);

      menued->stock_label = gtk_label_new (_ ("Stock Item:"));
      gtk_widget_show (menued->stock_label);
      gtk_container_add (GTK_CONTAINER (eventbox1), menued->stock_label);
      gtk_misc_set_alignment (GTK_MISC (menued->stock_label), 0, 0.5);

      menued->stock_combo = gtk_combo_new ();
      gb_widget_set_usize (menued->stock_combo, 100, -1);
      gtk_table_attach (GTK_TABLE (table1), menued->stock_combo, 1, 3,
			row, row + 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_combo_set_value_in_list (GTK_COMBO (menued->stock_combo),
				   FALSE, FALSE);
      gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (menued->stock_combo)->entry),
			      FALSE);
      gtk_widget_show (menued->stock_combo);

      listitem = gtk_list_item_new_with_label (_("None"));
      gtk_widget_show (listitem);
      gtk_container_add (GTK_CONTAINER (GTK_COMBO (menued->stock_combo)->list),
			 listitem);

      for (elem = menued->stock_items, idx = 1; elem; elem = elem->next, idx++)
	{
	  GtkStockItem item;
	  GtkWidget *listitem, *hbox, *image, *label;
	  GtkIconSet *icon_set;
	  GtkIconSize *sizes;
	  gint n_sizes, i;
	  gboolean has_menu_size;

	  stock_id = elem->data;

#if 0
	  g_print ("Stock ID: %s\n", stock_id);
#endif

	  /* Only show GTK+ stock items. */
	  if (strncmp (stock_id, "gtk-", 4) != 0)
	    continue;

	  /* Check that the icon has a menu size. */
	  has_menu_size = FALSE;
	  icon_set = gtk_icon_factory_lookup_default (stock_id);
	  if (icon_set)
	    {
	      gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);
	      for (i = 0; i < n_sizes; i++)
		{
		  if (sizes[i] == GTK_ICON_SIZE_MENU)
		    has_menu_size = TRUE;
		}
	      g_free (sizes);
	    }

	  if (!has_menu_size)
	    {
#if 0
	      g_print ("Skipping: %s\n", stock_id);
#endif
	      continue;
	    }

	  if (gtk_stock_lookup (stock_id, &item))
	    {
	      listitem = gtk_list_item_new ();
	      gtk_object_set_data (GTK_OBJECT (listitem),
				   GladeMenuEditorIndexKey,
				   GINT_TO_POINTER (idx));
	      gtk_widget_show (listitem);
	      hbox = gtk_hbox_new (FALSE, 3);
	      gtk_container_add (GTK_CONTAINER (listitem), hbox);
	      gtk_widget_show (hbox);

	      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);

	      if (image)
		{
		  gtk_widget_show (image);
		  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
		}

	      label = gtk_type_new (GTK_TYPE_ACCEL_LABEL);
	      gtk_label_set_text_with_mnemonic (GTK_LABEL (label), item.label);
	      gtk_widget_show (label);
	      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	      gtk_combo_set_item_string (GTK_COMBO (menued->stock_combo),
					 GTK_ITEM (listitem), stock_id);

	      gtk_container_add (GTK_CONTAINER (GTK_COMBO (menued->stock_combo)->list), listitem);
	    }
	  else
	    {
#if 0
	      g_print ("Lookup failed for stock_id: %s (probably a stock image only\n", stock_id);
#endif
	    }
	}
      gtk_signal_connect (GTK_OBJECT (GTK_COMBO (menued->stock_combo)->entry),
			  "changed",
			  GTK_SIGNAL_FUNC (on_stock_item_entry_changed),
			  NULL);
      row++;
    }

#ifdef USE_GNOME
  if (menued->gnome_support)
    {
      /* Gnome stock item. */
      eventbox1 = gtk_event_box_new ();
      gtk_widget_show (eventbox1);
      gtk_table_attach (GTK_TABLE (table1), eventbox1, 0, 1, row, row + 1,
			GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_tooltips_set_tip (tooltips, eventbox1,
			    _ ("The stock Gnome item to use."), NULL);

      menued->stock_label = gtk_label_new (_ ("Stock Item:"));
      gtk_widget_show (menued->stock_label);
      gtk_container_add (GTK_CONTAINER (eventbox1), menued->stock_label);
      gtk_misc_set_alignment (GTK_MISC (menued->stock_label), 0, 0.5);

      menued->stock_combo = gtk_combo_new ();
      gb_widget_set_usize (menued->stock_combo, 100, -1);
      gtk_table_attach (GTK_TABLE (table1), menued->stock_combo, 1, 3,
			row, row + 1, GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_combo_set_value_in_list (GTK_COMBO (menued->stock_combo),
				   FALSE, FALSE);
      gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (menued->stock_combo)->entry),
			      FALSE);
      gtk_widget_show (menued->stock_combo);

      for (uiinfo = GladeStockMenuItemValues;
	   uiinfo->type != GNOME_APP_UI_ENDOFINFO;
	   uiinfo++)
	{
	  pixmap = NULL;
	  label = NULL;
	  label_text = NULL;

	  if (uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
	    gnome_app_ui_configure_configurable (uiinfo);

	  uiinfo->widget = NULL;

	  switch (uiinfo->type) {
	  case GNOME_APP_UI_SEPARATOR:
	    uiinfo->widget = gtk_list_item_new ();
	    gtk_widget_show (uiinfo->widget);
	    separator = gtk_hseparator_new();
	    gtk_widget_show (separator);
	    gtk_container_add (GTK_CONTAINER (uiinfo->widget), separator);
	    gtk_widget_set_sensitive (uiinfo->widget, FALSE);
	    gtk_combo_set_item_string (GTK_COMBO (menued->stock_combo),
				       GTK_ITEM (uiinfo->widget), "");
	    gtk_container_add (GTK_CONTAINER (GTK_COMBO (menued->stock_combo)->list), uiinfo->widget);
	    break;
	  case GNOME_APP_UI_ITEM:
	  case GNOME_APP_UI_SUBTREE_STOCK:
	    if (uiinfo->pixmap_type == GNOME_APP_PIXMAP_STOCK)
	      pixmap = gtk_image_new_from_stock (uiinfo->pixmap_info, GTK_ICON_SIZE_MENU);

	    if (uiinfo->label && uiinfo->label[0])
	      {
		uiinfo->widget = gtk_list_item_new ();
		gtk_widget_show (uiinfo->widget);
		hbox = gtk_hbox_new (FALSE, 3);
		gtk_container_add (GTK_CONTAINER (uiinfo->widget), hbox);
		gtk_widget_show (hbox);

		if (!pixmap && uiinfo->type != GNOME_APP_UI_SUBTREE_STOCK)
		  {
		    /* Create a dummy widget to fill in the space. */
		    pixmap = gtk_alignment_new (0, 0, 0, 0);
		    gtk_widget_set_size_request (pixmap, 16, -1);
		  }

		if (pixmap)
		  {
		    gtk_widget_show (pixmap);
		    gtk_box_pack_start (GTK_BOX (hbox), pixmap,
					FALSE, FALSE, 0);
		  }

		label = gtk_accel_label_new ("");
		/* Most of the label text is from Gnome, but 2 of them are
		   ours, so we use a utility function to find the translation.
		*/
		label_text = glade_gnome_gettext (uiinfo->label);
		gtk_label_set_text_with_mnemonic (GTK_LABEL (label),
						  label_text);
		gtk_widget_show (label);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

		gtk_combo_set_item_string (GTK_COMBO (menued->stock_combo),
					   GTK_ITEM (uiinfo->widget),
					   label_text);

		gtk_container_add (GTK_CONTAINER (GTK_COMBO (menued->stock_combo)->list), uiinfo->widget);
	      }
	    break;

	  default:
	    g_warning ("Invalid UIINFO item type");
	  }
	}
      gtk_signal_connect (GTK_OBJECT (GTK_COMBO (menued->stock_combo)->entry),
			  "changed",
			  GTK_SIGNAL_FUNC (on_stock_item_entry_changed),
			  NULL);

      row++;
    }
#endif /* USE_GNOME */

  /* Item Label. */
  eventbox1 = gtk_event_box_new ();
  gtk_widget_show (eventbox1);
  gtk_table_attach (GTK_TABLE (table1), eventbox1, 0, 1, row, row + 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox1, _ ("The text of the menu item, or empty for separators."), NULL);

  menued->label_label = gtk_label_new (_ ("Label:"));
  gtk_widget_show (menued->label_label);
  gtk_container_add (GTK_CONTAINER (eventbox1), menued->label_label);
  gtk_misc_set_alignment (GTK_MISC (menued->label_label), 0, 0.5);

  menued->label_entry = gtk_entry_new ();
  gtk_widget_show (menued->label_entry);
  gtk_table_attach (GTK_TABLE (table1), menued->label_entry, 1, 3,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (menued->label_entry), "changed",
		      GTK_SIGNAL_FUNC (on_entry_changed), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->label_entry), "key_press_event",
		      GTK_SIGNAL_FUNC (on_label_entry_key_press),
		      NULL);
  row++;

  /* Item Name. */
  eventbox2 = gtk_event_box_new ();
  gtk_widget_show (eventbox2);
  gtk_table_attach (GTK_TABLE (table1), eventbox2, 0, 1, row, row + 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox2, _ ("The name of the widget"),
			NULL);

  menued->name_label = gtk_label_new (_ ("Name:"));
  gtk_widget_show (menued->name_label);
  gtk_container_add (GTK_CONTAINER (eventbox2), menued->name_label);
  gtk_misc_set_alignment (GTK_MISC (menued->name_label), 0, 0.5);

  menued->name_entry = gtk_entry_new ();
  gtk_widget_show (menued->name_entry);
  gtk_table_attach (GTK_TABLE (table1), menued->name_entry, 1, 3, row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (menued->name_entry), "changed",
		      GTK_SIGNAL_FUNC (on_entry_changed), NULL);
  row++;

  /* Item Handler. */
  eventbox3 = gtk_event_box_new ();
  gtk_widget_show (eventbox3);
  gtk_table_attach (GTK_TABLE (table1), eventbox3, 0, 1, row, row + 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox3, _ ("The function to be called when the item is selected"), NULL);

  menued->handler_label = gtk_label_new (_ ("Handler:"));
  gtk_widget_show (menued->handler_label);
  gtk_container_add (GTK_CONTAINER (eventbox3), menued->handler_label);
  gtk_misc_set_alignment (GTK_MISC (menued->handler_label), 0, 0.5);

  menued->handler_entry = gtk_entry_new ();
  gtk_widget_show (menued->handler_entry);
  gtk_table_attach (GTK_TABLE (table1), menued->handler_entry, 1, 3,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (menued->handler_entry), "changed",
		      GTK_SIGNAL_FUNC (on_entry_changed), NULL);
  row++;

  /* Item Icon. */
  eventbox1 = gtk_event_box_new ();
  gtk_widget_show (eventbox1);
  gtk_table_attach (GTK_TABLE (table1), eventbox1, 0, 1, row, row + 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox1, _ ("An optional icon to show on the left of the menu item."), NULL);

  menued->icon_label = gtk_label_new (_ ("Icon:"));
  gtk_widget_show (menued->icon_label);
  gtk_container_add (GTK_CONTAINER (eventbox1), menued->icon_label);
  gtk_misc_set_alignment (GTK_MISC (menued->icon_label), 0, 0.5);

  icon_hbox = gtk_hbox_new (FALSE, 2);
  gtk_table_attach (GTK_TABLE (table1), icon_hbox, 1, 3, row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (icon_hbox);

  menued->icon_widget = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (menued->icon_widget)->entry),
			     FALSE);
  gb_widget_set_usize (menued->icon_widget, 100, -1);

  /* Add a "None" item first, so it is easy to reset the pixmap. */
  listitem = gtk_list_item_new_with_label (_("None"));
  gtk_widget_show (listitem);
  gtk_container_add (GTK_CONTAINER (GTK_COMBO (menued->icon_widget)->list),
		     listitem);

  for (elem = menued->stock_items; elem; elem = elem->next)
    {
      GtkStockItem item;
      GtkWidget *listitem, *hbox, *image, *label;
      GtkIconSet *icon_set;
      GtkIconSize *sizes;
      gint n_sizes, i;
      gboolean has_menu_size;

      stock_id = elem->data;

#if 0
      g_print ("Stock ID: %s\n", stock_id);
#endif

      /* Show only GTK+ stock items in GTK+ projects. */
      if (!menued->gnome_support && (strncmp (stock_id, "gtk-", 4) != 0))
	continue;
#if 0
      /* Only show GnomeDB stock items if GnomeDB support is on. */
      if (!menued->gnome_db_support && !strncmp (stock_id, "gnome-db-", 9))
	continue;
#endif

      /* Check that the icon has a menu size. */
      has_menu_size = FALSE;
      icon_set = gtk_icon_factory_lookup_default (stock_id);
      if (icon_set)
	{
	  gtk_icon_set_get_sizes (icon_set, &sizes, &n_sizes);
	  for (i = 0; i < n_sizes; i++)
	    {
	      if (sizes[i] == GTK_ICON_SIZE_MENU)
		has_menu_size = TRUE;
	    }
	  g_free (sizes);
	}

      if (!has_menu_size)
	{
#if 0
	  g_print ("Skipping: %s\n", stock_id);
#endif
	  continue;
	}

      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
      if (image)
	{
	  listitem = gtk_list_item_new ();
	  /* We store a pointer to the stock_id from the stock_items list,
	     so we don't need to free it, but if stock_items is destroyed it
	     will no longer be valid. */
	  gtk_object_set_data (GTK_OBJECT (listitem),
			       GladeMenuEditorStockIDKey, stock_id);
	  gtk_widget_show (listitem);
	  hbox = gtk_hbox_new (FALSE, 3);
	  gtk_container_add (GTK_CONTAINER (listitem), hbox);
	  gtk_widget_show (hbox);
	  gtk_widget_show (image);
	  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	  if (gtk_stock_lookup (stock_id, &item))
	    label_text = item.label;
	  else
	    /* FIXME: We have no name to use for the image. */
	    label_text = stock_id;

	  label = gtk_type_new (GTK_TYPE_ACCEL_LABEL);
	  gtk_label_set_text_with_mnemonic (GTK_LABEL (label), label_text);

	  gtk_widget_show (label);
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	  gtk_combo_set_item_string (GTK_COMBO (menued->icon_widget),
				     GTK_ITEM (listitem), stock_id);
	  gtk_container_add (GTK_CONTAINER (GTK_COMBO (menued->icon_widget)->list),
			     listitem);
	}
    }

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (menued->icon_widget)->entry),
		      "changed", GTK_SIGNAL_FUNC (on_entry_changed), NULL);

  gtk_widget_show (menued->icon_widget);
  gtk_box_pack_start (GTK_BOX (icon_hbox), menued->icon_widget,
		      TRUE, TRUE, 0);

  menued->icon_button = gtk_button_new_with_label ("...");
  gtk_widget_show (menued->icon_button);
  gtk_box_pack_start (GTK_BOX (icon_hbox), menued->icon_button,
		      FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (menued->icon_button), "clicked",
		      GTK_SIGNAL_FUNC (on_icon_button_clicked), NULL);
  row++;

  /* Tooltip. */
  eventbox3 = gtk_event_box_new ();
  gtk_widget_show (eventbox3);
  gtk_table_attach (GTK_TABLE (table1), eventbox3, 0, 1, row, row + 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox3, _ ("The tip to show when the mouse is over the item"), NULL);

  menued->tooltip_label = gtk_label_new (_ ("Tooltip:"));
  gtk_widget_show (menued->tooltip_label);
  gtk_container_add (GTK_CONTAINER (eventbox3), menued->tooltip_label);
  gtk_misc_set_alignment (GTK_MISC (menued->tooltip_label), 0, 0.5);

  menued->tooltip_entry = gtk_entry_new ();
  gtk_widget_show (menued->tooltip_entry);
  gtk_table_attach (GTK_TABLE (table1), menued->tooltip_entry, 1, 3,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_signal_connect (GTK_OBJECT (menued->tooltip_entry), "changed",
		      GTK_SIGNAL_FUNC (on_entry_changed), NULL);
  row++;

  /* Buttons to add/delete items. */
  button_table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (button_table), 1);
  gtk_table_set_col_spacings (GTK_TABLE (button_table), 2);
  gtk_widget_show (button_table);
  gtk_box_pack_start (GTK_BOX (vbox3), button_table, FALSE, TRUE, 0);

  menued->add_button = gtk_button_new_with_mnemonic (_("_Add"));
  gtk_widget_show (menued->add_button);
  gtk_table_attach (GTK_TABLE (button_table), menued->add_button,
		    0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, menued->add_button,
			_ ("Add a new item below the selected item."), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->add_button), "clicked",
		      GTK_SIGNAL_FUNC (on_add_button_clicked),
		      NULL);

  menued->add_child_button = gtk_button_new_with_mnemonic (_("Add _Child"));
  gtk_widget_show (menued->add_child_button);
  gtk_table_attach (GTK_TABLE (button_table), menued->add_child_button,
		    1, 2, 0, 1, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, menued->add_child_button,
			_ ("Add a new child item below the selected item."),
			NULL);
  gtk_signal_connect (GTK_OBJECT (menued->add_child_button), "clicked",
		      GTK_SIGNAL_FUNC (on_add_child_button_clicked),
		      NULL);

  menued->add_separator_button = gtk_button_new_with_mnemonic (_("Add _Separator"));
  gtk_widget_show (menued->add_separator_button);
  gtk_table_attach (GTK_TABLE (button_table), menued->add_separator_button,
		    0, 1, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, menued->add_separator_button,
			_ ("Add a separator below the selected item."), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->add_separator_button), "clicked",
		      GTK_SIGNAL_FUNC (on_add_separator_button_clicked),
		      NULL);

  menued->delete_button = gtk_button_new_with_mnemonic (_("_Delete"));
  gtk_widget_show (menued->delete_button);
  gtk_table_attach (GTK_TABLE (button_table), menued->delete_button,
		    1, 2, 1, 2, GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, menued->delete_button,
			_ ("Delete the current item"), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->delete_button), "clicked",
		      GTK_SIGNAL_FUNC (on_delete_button_clicked),
		      NULL);

  /* Type radio options and toggle options. */
  menued->type_frame = gtk_frame_new (_ ("Item Type:"));
  gtk_widget_show (menued->type_frame);
  gtk_box_pack_start (GTK_BOX (vbox3), menued->type_frame, FALSE, TRUE, 0);

  table2 = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table2), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table2), 1);
  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (menued->type_frame), table2);
  gtk_container_set_border_width (GTK_CONTAINER (table2), 4);

  eventbox1 = gtk_event_box_new ();
  gtk_widget_show (eventbox1);
  gtk_table_attach (GTK_TABLE (table2), eventbox1, 1, 2, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_tooltips_set_tip (tooltips, eventbox1,
			_ ("If the item is initially on."), NULL);

  menued->state_label = gtk_label_new (_("Active:"));
  gtk_misc_set_alignment (GTK_MISC (menued->state_label), 0, 0.5);
  gtk_widget_show (menued->state_label);
  gtk_container_add (GTK_CONTAINER (eventbox1), menued->state_label);

  menued->state_togglebutton = gtk_toggle_button_new_with_label (_("No"));
  gtk_widget_show (menued->state_togglebutton);
  gtk_table_attach (GTK_TABLE (table2), menued->state_togglebutton, 2, 3, 1, 2,
		    GTK_EXPAND | GTK_FILL, 0, 0, 0);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_widget_show (hbox2);
  gtk_table_attach (GTK_TABLE (table2), hbox2, 1, 3, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  eventbox1 = gtk_event_box_new ();
  gtk_widget_show (eventbox1);
  gtk_box_pack_start (GTK_BOX (hbox2), eventbox1, FALSE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, eventbox1,
			_ ("The radio menu item's group"), NULL);

  menued->group_label = gtk_label_new (_ ("Group:"));
  gtk_misc_set_alignment (GTK_MISC (menued->group_label), 0, 0.5);
  gtk_widget_show (menued->group_label);
  gtk_container_add (GTK_CONTAINER (eventbox1), menued->group_label);

  menued->group_combo = gtk_combo_new ();
  gtk_editable_set_editable (GTK_EDITABLE (GTK_COMBO (menued->group_combo)->entry),
			     FALSE);
  gtk_widget_set_usize (GTK_COMBO (menued->group_combo)->entry, 60, -1);
  gtk_widget_show (menued->group_combo);
  gtk_box_pack_start (GTK_BOX (hbox2), menued->group_combo, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (menued->group_combo)->entry),
		      "changed", GTK_SIGNAL_FUNC (on_entry_changed), NULL);

  menued->radio_radiobutton = gtk_radio_button_new_with_label (table2_group,
							       _ ("Radio"));
  table2_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (menued->radio_radiobutton));
  gtk_widget_show (menued->radio_radiobutton);
  gtk_table_attach (GTK_TABLE (table2), menued->radio_radiobutton, 0, 1, 2, 3,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  menued->check_radiobutton = gtk_radio_button_new_with_label (table2_group,
							       _ ("Check"));
  table2_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (menued->check_radiobutton));
  gtk_widget_show (menued->check_radiobutton);
  gtk_table_attach (GTK_TABLE (table2), menued->check_radiobutton, 0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  menued->normal_radiobutton = gtk_radio_button_new_with_label (table2_group,
							      _ ("Normal"));
  table2_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (menued->normal_radiobutton));
  gtk_widget_show (menued->normal_radiobutton);
  gtk_table_attach (GTK_TABLE (table2), menued->normal_radiobutton, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->normal_radiobutton), TRUE);


  /* Accelerator key options. */
  menued->accel_frame = gtk_frame_new (_ ("Accelerator:"));
  gtk_widget_show (menued->accel_frame);
  gtk_box_pack_start (GTK_BOX (vbox3), menued->accel_frame, FALSE, TRUE, 0);

  table3 = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (menued->accel_frame), table3);
  gtk_container_set_border_width (GTK_CONTAINER (table3), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table3), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table3), 4);

  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox2);
  gtk_table_attach (GTK_TABLE (table3), hbox2, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  menued->accel_key_entry = gtk_entry_new ();
  gtk_widget_set_usize (menued->accel_key_entry, 100, -1);
  gtk_widget_show (menued->accel_key_entry);
  gtk_box_pack_start (GTK_BOX (hbox2), menued->accel_key_entry,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (menued->accel_key_entry), "changed",
		      GTK_SIGNAL_FUNC (on_entry_changed), NULL);

  accel_key_button = gtk_button_new_with_label ("...");
  gtk_widget_show (accel_key_button);
  gtk_box_pack_start (GTK_BOX (hbox2), accel_key_button,
		      FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (accel_key_button), "clicked",
		      GTK_SIGNAL_FUNC (on_accel_key_button_clicked),
		      NULL);

  hbox2 = gtk_hbox_new (TRUE, 0);
  gtk_widget_show (hbox2);
  gtk_table_attach (GTK_TABLE (table3), hbox2, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  menued->accel_ctrl_checkbutton = gtk_check_button_new_with_label (_ ("Ctrl"));
  gtk_widget_show (menued->accel_ctrl_checkbutton);
  gtk_box_pack_start (GTK_BOX (hbox2), menued->accel_ctrl_checkbutton,
		      TRUE, TRUE, 0);

  menued->accel_shift_checkbutton = gtk_check_button_new_with_label (_ ("Shift"));
  gtk_widget_show (menued->accel_shift_checkbutton);
  gtk_box_pack_start (GTK_BOX (hbox2), menued->accel_shift_checkbutton,
		      TRUE, TRUE, 0);

  menued->accel_alt_checkbutton = gtk_check_button_new_with_label (_ ("Alt"));
  gtk_widget_show (menued->accel_alt_checkbutton);
  gtk_box_pack_start (GTK_BOX (hbox2), menued->accel_alt_checkbutton,
		      TRUE, TRUE, 0);

  label9 = gtk_label_new (_ ("Key:"));
  gtk_widget_show (label9);
  gtk_table_attach (GTK_TABLE (table3), label9, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label9), 0, 0.5);

  label8 = gtk_label_new (_ ("Modifiers:"));
  gtk_widget_show (label8);
  gtk_table_attach (GTK_TABLE (table3), label8, 0, 1, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label8), 0, 0.5);


  hseparator1 = gtk_hseparator_new ();
  gtk_widget_show (hseparator1);
  gtk_box_pack_start (GTK_BOX (vbox2), hseparator1, FALSE, TRUE, 8);


  /* OK, Apply & Cancel buttons. */
  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox2), hbuttonbox1, FALSE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (hbuttonbox1), 8);

  menued->cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_show (menued->cancel_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), menued->cancel_button);
  GTK_WIDGET_SET_FLAGS (menued->cancel_button, GTK_CAN_DEFAULT);

  menued->apply_button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  gtk_widget_show (menued->apply_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), menued->apply_button);
  GTK_WIDGET_SET_FLAGS (menued->apply_button, GTK_CAN_DEFAULT);

  menued->ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_show (menued->ok_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), menued->ok_button);
  GTK_WIDGET_SET_FLAGS (menued->ok_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (menued->ok_button);


  /* Now set up all the signal handlers. */
  gtk_signal_connect_after (GTK_OBJECT (menued->normal_radiobutton), "toggled",
			    GTK_SIGNAL_FUNC (on_radiobutton_toggled), NULL);
  gtk_signal_connect_after (GTK_OBJECT (menued->check_radiobutton), "toggled",
			    GTK_SIGNAL_FUNC (on_radiobutton_toggled), NULL);
  gtk_signal_connect_after (GTK_OBJECT (menued->radio_radiobutton), "toggled",
			    GTK_SIGNAL_FUNC (on_radiobutton_toggled), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->state_togglebutton), "toggled",
		      GTK_SIGNAL_FUNC (on_state_button_toggled), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->accel_ctrl_checkbutton), "toggled",
		      GTK_SIGNAL_FUNC (on_checkbutton_toggled), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->accel_shift_checkbutton), "toggled",
		      GTK_SIGNAL_FUNC (on_checkbutton_toggled), NULL);
  gtk_signal_connect (GTK_OBJECT (menued->accel_alt_checkbutton), "toggled",
		      GTK_SIGNAL_FUNC (on_checkbutton_toggled), NULL);

  gtk_signal_connect (GTK_OBJECT (menued->ok_button), "clicked",
		      GTK_SIGNAL_FUNC (on_menu_editor_ok), menued);
  gtk_signal_connect (GTK_OBJECT (menued->apply_button), "clicked",
		      GTK_SIGNAL_FUNC (on_menu_editor_apply), menued);
  gtk_signal_connect (GTK_OBJECT (menued->cancel_button), "clicked",
		      GTK_SIGNAL_FUNC (on_menu_editor_close), menued);

  set_interface_state (menued);
}


GtkWidget*
glade_menu_editor_new         (GladeProject    *project,
			       GtkMenuShell    *menu)
{
  GladeMenuEditor *menued;

  menued = gtk_type_new (glade_menu_editor_get_type ());
  glade_menu_editor_construct (menued, project);
  glade_menu_editor_set_menu (menued, menu);

  return GTK_WIDGET (menued);
}

static void
glade_menu_editor_destroy (GtkObject *object)
{
  GladeMenuEditor *menued;
  GSList *elem;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GLADE_IS_MENU_EDITOR (object));

  menued = GLADE_MENU_EDITOR (object);

  /* Free all the GbMenuItemData elements & disconnect our destroy handler
     on the menu widget. */
  glade_menu_editor_reset (menued);

  if (menued->keys_dialog)
    {
      gtk_widget_destroy (menued->keys_dialog);
      menued->keys_dialog = NULL;
    }

  if (menued->filesel)
    {
      gtk_widget_destroy (menued->filesel);
      menued->filesel = NULL;
    }

  for (elem = menued->stock_items; elem; elem = elem->next)
    g_free (elem->data);
  g_slist_free (menued->stock_items);
  menued->stock_items = NULL;
}


/**************************************************************************
 * Signal Handlers
 **************************************************************************/

static void
on_clist_select_row (GtkWidget * clist,
		     gint row,
		     gint column,
		     GdkEventButton * event,
		     gpointer user_data)
{
  GladeMenuEditor *menued;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (clist));

  show_item_properties (menued);

  if (event && !GTK_WIDGET_HAS_FOCUS (clist))
    gtk_widget_grab_focus (clist);

  set_interface_state (menued);
}

static void
on_clist_unselect_row (GtkWidget * clist,
		       gint row,
		       gint column,
		       GdkEventButton * event,
		       gpointer user_data)
{
  GladeMenuEditor *menued;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (clist));

  clear_form (menued, FALSE);

  if (event && !GTK_WIDGET_HAS_FOCUS (clist))
    gtk_widget_grab_focus (clist);

  set_interface_state (menued);
}

/* This will only call update_current_item if the text is different to the
   corresponding item field, since we don't want to propogate updates when
   we are setting the entry. */
static void
on_entry_changed (GtkWidget * entry,
		  gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkCList *clist;
  GbMenuItemData *item;
  gchar *text, *item_text;
  gboolean changed = FALSE;
  gint row;

#if 0
  g_print ("In on_entry_changed\n");
#endif

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (entry));

  /* If we are setting the widget values, just return. */
#if 1
  if (menued->updating_widgets)
    return;
#endif

  clist = GTK_CLIST (menued->clist);
  row = get_selected_row (menued);
#if 0
  g_print ("Selected row: %i\n", row);
#endif
  if (row == -1)
    return;
  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (menued->clist),
						    row);
#if 0
  g_print ("Item %p Label:%s Name:%s Handler:%s\n", item, item->label,
	   item->name, item->handler);
#endif

  text = (gchar*) gtk_entry_get_text (GTK_ENTRY (entry));

  if (entry == menued->label_entry)
    {
      item_text = item->label;
    }
  else if (entry == menued->name_entry)
    {    
      item_text = item->name;
    }
  else if (entry == menued->handler_entry)
    {
      item_text = item->handler;
    }
  else if (entry == GTK_COMBO (menued->icon_widget)->entry)
    {
      /* If the user selects the 'None' item from the combo, we reset the
	 text to "" and return. This callback will be called again. */
      if (!strcmp (text, _("None")))
	{
	  set_entry_text (GTK_ENTRY (entry), "");
	  return;
	}

      item_text = item->icon;
    }
  else if (entry == menued->tooltip_entry)
    {
      item_text = item->tooltip;
    }
  else if (entry == GTK_COMBO (menued->group_combo)->entry)
    {
      item_text = item->group ? item->group->name : item->name;
    }
  else if (entry == menued->accel_key_entry)
    {
      item_text = item->key;
    }
  else
    return;

  if (item_text == NULL)
    {
      if (strlen (text) > 0)
	changed = TRUE;
    }
  else
    {
      if (strcmp (text, item_text))
	changed = TRUE;
    }

  if (changed)
    {
      if (entry == menued->label_entry)
	{
	  if (item->generate_name)
	    {
	      glade_project_release_widget_name (menued->project, item->name);
	      g_free (item->name);
	      item->name = generate_name (menued, text);
	      set_entry_text (GTK_ENTRY (menued->name_entry),
			      item->name ? item->name : "");
	      gtk_clist_set_text (clist, row, GLD_COL_NAME,
				  item->name ? item->name : "");
	      if (item->generate_handler)
		{
		  g_free (item->handler);

		  item->handler = generate_handler (menued, row, text,
						    item->name);
		  set_entry_text (GTK_ENTRY (menued->handler_entry),
				  item->handler ? item->handler : "");
		  gtk_clist_set_text (clist, row, GLD_COL_HANDLER,
				      item->handler ? item->handler : "");
		}
	    }
	}
      else if (entry == menued->name_entry)
	{
	  item->generate_name = FALSE;
	  if (item->generate_handler)
	    {
	      g_free (item->handler);
	      item->handler = generate_handler (menued, row, item->label,
						text);
	      set_entry_text (GTK_ENTRY (menued->handler_entry),
			      item->handler ? item->handler : "");
	      gtk_clist_set_text (clist, row, GLD_COL_HANDLER,
				  item->handler ? item->handler : "");
	    }
	}
      else if (entry == menued->handler_entry)
	{
	  item->generate_handler = FALSE;
	}

      update_current_item (menued);
      set_interface_state (menued);
    }
}


/* This will only call update_current_item if the text is different to the
   corresponding item field, since we don't want to propogate updates when
   we are setting the entry. */
static void
on_stock_item_entry_changed (GtkWidget * entry,
			     gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkCList *clist;
  GbMenuItemData *item;
  gint row;
  GtkListItem *listitem;
  gint stock_item_index = 0;
  const gchar *text;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (entry));

  text = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (menued->stock_combo)->entry));
  /* FIXME GTK+ 1.3.x bug workaround. It emits "changed" twice, once when the
     existing text is deleted. So we just return if the text is empty. */
  if (text[0] == '\0')
    return;

  clist = GTK_CLIST (menued->clist);
  row = get_selected_row (menued);
  if (row == -1)
    return;
  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (menued->clist),
						    row);

  /* Find the index of the selected item. */
  listitem = glade_util_gtk_combo_find (GTK_COMBO (menued->stock_combo));
  g_return_if_fail (listitem != NULL);

  if (menued->gnome_support)
    {
#ifdef USE_GNOME
      stock_item_index = g_list_index (GTK_LIST (GTK_COMBO (menued->stock_combo)->list)->children, listitem);
#endif
    }
  else
    {
      GtkStockItem item;

      stock_item_index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (listitem), GladeMenuEditorIndexKey));
      if (gtk_stock_lookup (text, &item))
	text = item.label;
      else
	stock_item_index = 0;
    }

  if (item->stock_item_index != stock_item_index)
    {
      item->stock_item_index = stock_item_index;

      /* If the stock item is reset to 'None', get a new item name, and reset
	 generate_name/handler to TRUE. */
      if (stock_item_index == 0)
	{
	  item->generate_name = TRUE;
	  item->generate_handler = TRUE;

	  item->label = glade_project_new_widget_name (menued->project,
						       "item");
	  item->name = g_strdup (item->label);
	  item->handler = generate_handler (menued, row, item->label,
					    item->name);
	  show_item_properties (menued);

	  gtk_clist_set_text (clist, row, GLD_COL_LABEL,
			      item->label ? item->label : GB_SEPARATOR_TEXT);
	  gtk_clist_set_text (clist, row, GLD_COL_NAME,
			      item->name ? item->name : "");
	  gtk_clist_set_text (clist, row, GLD_COL_HANDLER,
			      item->handler ? item->handler : "");
	}
      else
	{
	  /* These will trigger callbacks, and will generate the name
	     and handler if appropriate. */
	  set_entry_text (GTK_ENTRY (menued->label_entry), text);

	  /* Stock menu items are all normal items. */
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->normal_radiobutton), TRUE);
	  set_entry_text (GTK_ENTRY (GTK_COMBO (menued->group_combo)->entry), "");

	  /* Reset the accelerator keys, as that is handled automatically. */
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_ctrl_checkbutton), FALSE);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_shift_checkbutton), FALSE);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_alt_checkbutton), FALSE);
	  set_entry_text (GTK_ENTRY (menued->accel_key_entry), "");
	}
    }

  set_interface_state (menued);
}


static gboolean
on_label_entry_key_press (GtkWidget * widget,
			  GdkEventKey * event,
			  gpointer user_data)
{
  GladeMenuEditor *menued;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (widget));

  /* If the Return key is pressed, we add a new item beneath the selected item.
     This makes it very easy to add several menus. If the Control key is
     pressed, we add the item as a child, otherwise we add it as a sibling. */
  if (event->keyval == GDK_Return)
    {
      if (event->state & GDK_CONTROL_MASK)
	{
	  add_item (menued, TRUE, FALSE);
	  /* Since we are added a child, we may need to set the parent's
	     handler to NULL if it has been auto-generated. */
	  check_generated_handlers (menued);
	}
      else
	{
	  add_item (menued, FALSE, FALSE);
	}
      return TRUE;
    }
  return FALSE;
}


static void
on_radiobutton_toggled (GtkWidget * togglebutton,
			gpointer user_data)
{
  GladeMenuEditor *menued;
  GbMenuItemData *item;
  GbMenuItemType type = 0;
  gboolean changed = FALSE;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (togglebutton)));
  item = get_selected_item (menued);
  if (item == NULL)
    return;

  if (togglebutton == menued->normal_radiobutton)
    type = GB_MENU_ITEM_NORMAL;
  else if (togglebutton == menued->check_radiobutton)
    type = GB_MENU_ITEM_CHECK;
  else if (togglebutton == menued->radio_radiobutton)
    type = GB_MENU_ITEM_RADIO;

  if (GTK_TOGGLE_BUTTON (togglebutton)->active)
    {
      if (type != item->type)
	changed = TRUE;
    }
  else
    {
      if (type == item->type)
	changed = TRUE;
    }

  if (changed)
    {
      update_current_item (menued);
      set_interface_state (menued);
    }
}

static void
on_checkbutton_toggled (GtkWidget * togglebutton,
			gpointer user_data)
{
  GladeMenuEditor *menued;
  GbMenuItemData *item;
  guint active;
  guint8 currently_active = 0;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (togglebutton)));
  active = GTK_TOGGLE_BUTTON (togglebutton)->active;
  item = get_selected_item (menued);
  if (item == NULL)
    return;
  if (togglebutton == menued->accel_ctrl_checkbutton)
    currently_active = item->modifiers & GDK_CONTROL_MASK;
  if (togglebutton == menued->accel_shift_checkbutton)
    currently_active = item->modifiers & GDK_SHIFT_MASK;
  if (togglebutton == menued->accel_alt_checkbutton)
    currently_active = item->modifiers & GDK_MOD1_MASK;

  if ((active && !currently_active) || (!active && currently_active))
    {
      update_current_item (menued);
      set_interface_state (menued);
    }
}

static void
on_state_button_toggled (GtkToggleButton * togglebutton,
			 gpointer user_data)
{
  GladeMenuEditor *menued;
  GbMenuItemData *item;
  GtkWidget *label;
  guint active;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (togglebutton)));
  active = GTK_TOGGLE_BUTTON (togglebutton)->active;
  label = GTK_BIN (togglebutton)->child;
  gtk_label_set_text (GTK_LABEL (label), active ? _("Yes") : _("No"));

  item = get_selected_item (menued);
  if (item == NULL)
    return;
  if ((item->active && !active) || (!item->active && active))
    update_current_item (menued);
}


/**************************************************************************
 * Accelerator Keys Dialog.
 **************************************************************************/
static void
on_accel_key_button_clicked (GtkButton * button,
			     gpointer user_data)
{
  GladeMenuEditor *menued;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));

  if (menued->keys_dialog == NULL)
    {
      menued->keys_dialog = glade_keys_dialog_new ();
      gtk_window_set_position (GTK_WINDOW (menued->keys_dialog),
			       GTK_WIN_POS_MOUSE);
      gtk_window_set_transient_for (GTK_WINDOW (menued->keys_dialog),
				    GTK_WINDOW (menued));
      gtk_signal_connect (GTK_OBJECT (GLADE_KEYS_DIALOG (menued->keys_dialog)->clist),
			  "select_row",
			  GTK_SIGNAL_FUNC (on_keys_dialog_clist_select),
			  menued);
      gtk_signal_connect (GTK_OBJECT (GLADE_KEYS_DIALOG (menued->keys_dialog)),
			  "response",
			  GTK_SIGNAL_FUNC (on_keys_dialog_response),
			  menued);
    }

  gtk_widget_show (GTK_WIDGET (menued->keys_dialog));
}

static void
on_keys_dialog_clist_select (GtkWidget * widget, gint row, gint column,
			     GdkEventButton * bevent, GladeMenuEditor * menued)
{
  if (bevent && bevent->type == GDK_2BUTTON_PRESS)
    on_keys_dialog_response (widget, GTK_RESPONSE_OK, menued);
}

static void
on_keys_dialog_response (GtkWidget * widget, gint response_id,
			 GladeMenuEditor * menued)
{
  gchar *key_symbol;

  if (response_id == GTK_RESPONSE_OK)
    {
      key_symbol = glade_keys_dialog_get_key_symbol (GLADE_KEYS_DIALOG (menued->keys_dialog));
      if (key_symbol)
	{
	  set_entry_text (GTK_ENTRY (menued->accel_key_entry), key_symbol);
	}
    }

  glade_util_close_window (menued->keys_dialog);
}


/**************************************************************************
 * Arrow Button callbacks.
 **************************************************************************/

static void
on_up_button_clicked (GtkButton * button,
		      gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkWidget *clist;
  GbMenuItemData *item, *prev_item;
  gint row, new_row, i, level;
  GList *items;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  clist = menued->clist;
  row = get_selected_row (menued);
  if (row == -1 || row == 0)
    return;

  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  level = item->level;

  /* Find the new position of the item and its children. */
  new_row = -1;
  for (i = row - 1; i >= 0; i--)
    {
      prev_item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist),
							     i);
      if (prev_item->level == level)
	{
	  new_row = i;
	  break;
	}
      else if (prev_item->level < level)
	break;
    }

  /* Return if we can't move the item up. */
  if (new_row == -1)
    return;

  /* Remove item and children. */
  items = remove_item_and_children (clist, row);

  /* Now insert at new position. */
  insert_items (clist, items, new_row);
  ensure_visible (clist, new_row);

  g_list_free (items);

  /* Make sure all items in the group point to the first one. */
  normalize_radio_groups (menued);

  gtk_clist_select_row (GTK_CLIST (clist), new_row, 0);
  set_interface_state (menued);
}

static void
on_down_button_clicked (GtkButton * button,
			gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkWidget *clist;
  GbMenuItemData *item, *next_item;
  gint row, new_row, i, level;
  gboolean found_next_item;
  GList *items;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  clist = menued->clist;
  row = get_selected_row (menued);
  if (row == -1 || row == GTK_CLIST (clist)->rows - 1)
    return;

  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  level = item->level;

  /* Find the new position of the item and its children. */
  new_row = -1;
  found_next_item = FALSE;
  for (i = row + 1; i < GTK_CLIST (clist)->rows; i++)
    {
      next_item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist),
							     i);
      /* We have to skip all the children of the next item as well. */
      if (next_item->level == level)
	{
	  if (found_next_item)
	    {
	      new_row = i;
	      break;
	    }
	  else
	    found_next_item = TRUE;
	}
      else if (next_item->level < level)
	break;
    }

  /* Return if we can't move the item up. */
  if (new_row == -1)
    {
      if (found_next_item)
	new_row = i;
      else
	return;
    }

  /* Remove item and children. */
  items = remove_item_and_children (clist, row);
  /* Remember that the new_row needs to be shifted because we deleted items. */
  new_row -= g_list_length (items);

  /* Now insert at new position. */
  insert_items (clist, items, new_row);
  ensure_visible (clist, new_row);

  g_list_free (items);

  /* Make sure all items in the group point to the first one. */
  normalize_radio_groups (menued);

  gtk_clist_select_row (GTK_CLIST (clist), new_row, 0);
  set_interface_state (menued);
}

static void
on_left_button_clicked (GtkButton * button,
			gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkWidget *clist;
  GbMenuItemData *item;
  gint row, i, level;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  clist = menued->clist;
  row = get_selected_row (menued);
  if (row == -1)
    return;
  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  level = item->level;
  if (item->level > 0)
    item->level--;
  gtk_clist_set_shift (GTK_CLIST (clist), row, 0, 0, item->level * GB_INDENT);

  for (i = row + 1; i < GTK_CLIST (clist)->rows; i++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), i);
      if (item->level <= level)
	break;
      item->level--;
      gtk_clist_set_shift (GTK_CLIST (clist), i, 0, 0,
			   item->level * GB_INDENT);
    }
  check_generated_handlers (menued);
  set_interface_state (menued);
}

static void
on_right_button_clicked (GtkButton * button,
			 gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkWidget *clist;
  GbMenuItemData *item, *prev_item;
  gint row, i, level;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  clist = menued->clist;
  row = get_selected_row (menued);
  if (row == -1 || row == 0)
    return;
  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  prev_item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist),
							 row - 1);
  if (prev_item->level < item->level)
    return;

  level = item->level;
  item->level++;
  gtk_clist_set_shift (GTK_CLIST (clist), row, 0, 0, item->level * GB_INDENT);

  for (i = row + 1; i < GTK_CLIST (clist)->rows; i++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), i);
      if (item->level <= level)
	break;
      item->level++;
      gtk_clist_set_shift (GTK_CLIST (clist), i, 0, 0,
			   item->level * GB_INDENT);
    }

  check_generated_handlers (menued);
  set_interface_state (menued);
}

static void
on_add_button_clicked (GtkWidget * button,
		       gpointer user_data)
{
  GladeMenuEditor *menued;
  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  add_item (menued, FALSE, FALSE);
}

static void
on_add_child_button_clicked (GtkWidget * button,
			     gpointer user_data)
{
  GladeMenuEditor *menued;
  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  add_item (menued, TRUE, FALSE);
}


/**************************************************************************
 * 
 **************************************************************************/

static void
on_add_separator_button_clicked (GtkWidget * button,
				 gpointer user_data)
{
  GladeMenuEditor *menued;
  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (button)));
  add_item (menued, FALSE, TRUE);
}

static gboolean
on_key_press (GtkWidget * widget,
	      GdkEventKey * event,
	      gpointer user_data)
{
  switch (event->keyval)
    {
    case GDK_Delete:
      on_delete_button_clicked (widget, NULL);
      break;
    }

  return FALSE;
}

static void
on_delete_button_clicked (GtkWidget * widget,
			  gpointer user_data)
{
  GladeMenuEditor *menued;
  GtkWidget *clist;
  GbMenuItemData *item;
  gint row, level, i;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (widget)));
  clist = menued->clist;
  row = get_selected_row (menued);
  if (row == -1)
    return;
  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  level = item->level;

  gtk_clist_remove (GTK_CLIST (clist), row);

  /* Update any other items in the same radio group. */
  if (item->type == GB_MENU_ITEM_RADIO)
    remove_from_radio_group (menued, item);

  glade_menu_editor_free_item (item);

  /* Move all children up a level */
  for (i = row; i < GTK_CLIST (clist)->rows; i++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), i);
      if (item->level <= level)
	break;
      item->level--;
      gtk_clist_set_shift (GTK_CLIST (clist), i, 0, 0,
			   item->level * GB_INDENT);
    }

  gtk_clist_select_row (GTK_CLIST (clist), row, 0);
  set_interface_state (menued);
}


/**************************************************************************
 * File Selection for selecting icon xpm files.
 **************************************************************************/
#define GLADE_RESPONSE_CLEAR	1

static void
on_icon_button_clicked (GtkWidget * widget,
			gpointer user_data)
{
  GladeMenuEditor *menued;
  gchar *icon;

  menued = GLADE_MENU_EDITOR (gtk_widget_get_toplevel (GTK_WIDGET (widget)));

  if (menued->filesel == NULL)
    {
      menued->filesel = gtk_file_chooser_dialog_new (_("Select icon"),
						     GTK_WINDOW (menued),
						     GTK_FILE_CHOOSER_ACTION_OPEN,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_CLEAR, GLADE_RESPONSE_CLEAR,
						     GTK_STOCK_OK, GTK_RESPONSE_OK,
						     NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (menued->filesel),
				       GTK_RESPONSE_OK);
 
      g_signal_connect (menued->filesel, "response",
			GTK_SIGNAL_FUNC (on_icon_filesel_response), menued);
      g_signal_connect (menued->filesel, "delete_event",
			G_CALLBACK (gtk_true), NULL);
    }

  icon = (gchar*) gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (menued->icon_widget)->entry));
  if (icon && *icon)
    glade_util_set_file_selection_filename (menued->filesel, icon);
  
  gtk_window_present (GTK_WINDOW (menued->filesel));
}


static void
on_icon_filesel_response (GtkWidget * filesel,
			  gint response_id,
			  GladeMenuEditor *menued)
{
  gchar *filename;

  if (response_id == GTK_RESPONSE_OK)
    {
      filename = glade_util_get_file_selection_filename (filesel);
      set_entry_text (GTK_ENTRY (GTK_COMBO (menued->icon_widget)->entry),
		      filename);
      g_free (filename);
    }
  else if (response_id == GLADE_RESPONSE_CLEAR)
    {
      set_entry_text (GTK_ENTRY (GTK_COMBO (menued->icon_widget)->entry),
		      "");
    }

  glade_util_close_window (menued->filesel);
}


/**************************************************************************
 * Utility functions
 **************************************************************************/


/* This returns the index of the currently selected row in the clist, or -1
   if no item is currently selected. */
static gint
get_selected_row (GladeMenuEditor * menued)
{
  if (GTK_CLIST (menued->clist)->selection == NULL)
    return -1;
  return GPOINTER_TO_INT (GTK_CLIST (menued->clist)->selection->data);
}

/* This returns the currently selected item, or NULL if no item is currently
   selected. */
static GbMenuItemData*
get_selected_item (GladeMenuEditor * menued)
{
  GbMenuItemData *item;
  gint row;

  row = get_selected_row (menued);
  if (row == -1)
    return NULL;
  item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), row);
  return item;
}

/* This set the sensitivity of the buttons according to the current state. */
static void
set_interface_state (GladeMenuEditor * menued)
{
  GbMenuItemData *item, *tmp_item;
  GtkCList *clist;
  gboolean up_button_sens = FALSE, down_button_sens = FALSE;
  gboolean left_button_sens = FALSE, right_button_sens = FALSE;
  gboolean add_button_sens = FALSE, delete_button_sens = FALSE;
  gboolean state_sens = FALSE, group_sens = FALSE;
  gboolean form_sens = FALSE, type_sens = FALSE, accel_sens = FALSE;
  gboolean label_sens = FALSE, icon_sens = FALSE;
  gint index;

  clist = GTK_CLIST (menued->clist);

  /* Figure out which of the arrow buttons should be sensitive. */

  /* The Add button is always sensitive, since empty labels are separators. */
  add_button_sens = TRUE;

  /* The Delete button and the entire form are sensitive if an item is
     selected in the clist. */
  index = get_selected_row (menued);
  if (index != -1)
    {
      form_sens = TRUE;
      type_sens = TRUE;
      label_sens = TRUE;
      icon_sens = TRUE;
      delete_button_sens = TRUE;

      if (index > 0)
	up_button_sens = TRUE;
      if (index < clist->rows - 1)
	down_button_sens = TRUE;

      item = (GbMenuItemData *) gtk_clist_get_row_data (clist, index);
      if (item->level > 0)
	left_button_sens = TRUE;

      /* The accelerator modifier and key are sensitive if this is not a
	 toplevel item on a menubar. */
      if (!GTK_IS_MENU_BAR (menued->menu) || item->level != 0)
	accel_sens = TRUE;

      if (index > 0)
	{
	  tmp_item = (GbMenuItemData *) gtk_clist_get_row_data (clist,
								index - 1);
	  if (tmp_item->level >= item->level)
	    right_button_sens = TRUE;
	}

      /* Figure out if the radio group widgets should be sensitive. */
      if (GTK_TOGGLE_BUTTON (menued->radio_radiobutton)->active)
	{
	  group_sens = TRUE;
	  state_sens = TRUE;
	  icon_sens = FALSE;
	}

      if (GTK_TOGGLE_BUTTON (menued->check_radiobutton)->active)
	{
	  state_sens = TRUE;
	  icon_sens = FALSE;
	}

      if (item->stock_item_index)
	{
	  /* For the 'New' menu item, a label and tooltip must be provided. */
	  if (menued->gnome_support)
	    {
#ifdef USE_GNOME
	      if (item->stock_item_index != GladeStockMenuItemNew)
		label_sens = FALSE;
#endif
	    }
	  else
	    {
	      /* We don't allow the "New" label to be changed for GTK+ stock
		 items now, to be compatable with libglade. It did make it a
		 bit too complicated anyway. */
#if 0
	      const char *stock_id = g_slist_nth_data (menued->stock_items,
						       item->stock_item_index - 1);
	      if (strcmp (stock_id, GTK_STOCK_NEW))
#endif
		label_sens = FALSE;
	    }

	  icon_sens = FALSE;
	  type_sens = FALSE;
	  accel_sens = FALSE;
	}
    }

  /* Now set the sensitivity of the widgets. */
  gtk_widget_set_sensitive (menued->stock_label, form_sens);
  gtk_widget_set_sensitive (menued->stock_combo, form_sens);

  gtk_widget_set_sensitive (menued->icon_label, icon_sens);
  gtk_widget_set_sensitive (menued->icon_widget, icon_sens);
  gtk_widget_set_sensitive (menued->icon_button, icon_sens);

  gtk_widget_set_sensitive (menued->name_label, form_sens);
  gtk_widget_set_sensitive (menued->name_entry, form_sens);
  gtk_widget_set_sensitive (menued->handler_label, form_sens);
  gtk_widget_set_sensitive (menued->handler_entry, form_sens);

  gtk_widget_set_sensitive (menued->label_label, label_sens);
  gtk_widget_set_sensitive (menued->label_entry, label_sens);
  gtk_widget_set_sensitive (menued->tooltip_label, label_sens);
  gtk_widget_set_sensitive (menued->tooltip_entry, label_sens);

  gtk_widget_set_sensitive (menued->add_button, add_button_sens);
  gtk_widget_set_sensitive (menued->add_separator_button, add_button_sens);
  gtk_widget_set_sensitive (menued->delete_button, delete_button_sens);

  gtk_widget_set_sensitive (menued->type_frame, type_sens);
  gtk_widget_set_sensitive (menued->state_label, state_sens);
  gtk_widget_set_sensitive (menued->state_togglebutton, state_sens);
  gtk_widget_set_sensitive (menued->group_label, group_sens);
  gtk_widget_set_sensitive (menued->group_combo, group_sens);

  gtk_widget_set_sensitive (menued->accel_frame, accel_sens);
}


/* This gets a string representing the accelerator key + modifiers.
   It returns a pointer to a static buffer. */
static gchar *
get_accel_string (gchar * key, guint8 modifiers)
{
  static gchar buffer[32];

  buffer[0] = '\0';
  if (modifiers & GDK_CONTROL_MASK)
    strcat (buffer, "C+");
  if (modifiers & GDK_SHIFT_MASK)
    strcat (buffer, "S+");
  if (modifiers & GDK_MOD1_MASK)
    strcat (buffer, "A+");
  if (key)
    strcat (buffer, key);
  return buffer;
}


static gchar*
get_stock_item_label (GladeMenuEditor * menued, gint stock_item_index)
{
  if (menued->gnome_support)
    {
#ifdef USE_GNOME
      /* Most of the label text is from Gnome, but 2 of them are ours,
	 so we use a utility function to find the translation. */
      return glade_gnome_gettext (GladeStockMenuItemValues[stock_item_index].label);
#else
      /* This shouldn't happen. */
      g_warning ("Trying to use GNOME stock items in GTK+ version of Glade");
      return NULL;
#endif
    }
  else
    {
      gchar *stock_id;

      if (stock_item_index <= 0)
	return _("None");

      stock_id = g_slist_nth_data (menued->stock_items, stock_item_index - 1);
      return stock_id;

#if 0
      GtkStockItem item;
      gtk_stock_lookup (stock_id, &item);
      return item.label;
#endif
    }
}


/* This shows the properties of the item currently selected in the clist. */
static void
show_item_properties (GladeMenuEditor * menued)
{
  GbMenuItemData *item;
  GtkWidget *clist;

  clist = menued->clist;
  item = get_selected_item (menued);
  if (item == NULL)
    return;

  menued->updating_widgets = TRUE;

  /* Now set them to the item's properties. */
  set_entry_text (GTK_ENTRY (GTK_COMBO (menued->stock_combo)->entry),
		  get_stock_item_label (menued, item->stock_item_index));
#if 0
  g_print ("Setting label_entry to: %s\n", item->label);
#endif
  set_entry_text (GTK_ENTRY (menued->label_entry), item->label);
#if 0
  g_print ("Setting name_entry to: %s\n", item->name);
#endif
  set_entry_text (GTK_ENTRY (menued->name_entry), item->name);
#if 0
  g_print ("Setting handler_entry to: %s\n", item->handler);
#endif
  set_entry_text (GTK_ENTRY (menued->handler_entry), item->handler);
  set_entry_text (GTK_ENTRY (GTK_COMBO (menued->icon_widget)->entry),
		  item->icon);
  set_entry_text (GTK_ENTRY (menued->tooltip_entry), item->tooltip);

  if (item->type == GB_MENU_ITEM_NORMAL)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->normal_radiobutton), TRUE);
      set_entry_text (GTK_ENTRY (GTK_COMBO (menued->group_combo)->entry), "");
    }
  else if (item->type == GB_MENU_ITEM_CHECK)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->check_radiobutton), TRUE);
      set_entry_text (GTK_ENTRY (GTK_COMBO (menued->group_combo)->entry), "");
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->radio_radiobutton), TRUE);
      set_entry_text (GTK_ENTRY (GTK_COMBO (menued->group_combo)->entry),
		      item->group ? item->group->name : item->name);
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->state_togglebutton), item->active);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_ctrl_checkbutton), (item->modifiers & GDK_CONTROL_MASK) ? TRUE : FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_shift_checkbutton), (item->modifiers & GDK_SHIFT_MASK) ? TRUE : FALSE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_alt_checkbutton), (item->modifiers & GDK_MOD1_MASK) ? TRUE : FALSE);
  set_entry_text (GTK_ENTRY (menued->accel_key_entry),
		  item->key ? item->key : "");

  update_radio_groups (menued);

  menued->updating_widgets = FALSE;
}

/* This adds a new menuitem. If separator is FALSE, it adds a normal item
   with the label 'New Item'. If separator is TRUE it adds a separator.
   It is added to the clist beneath the currently selected item, or at the
   end of the list if no item is selected. If as_child is TRUE it adds the
   item as a child of the selected item, else it adds it as a sibling.
*/
static void
add_item (GladeMenuEditor * menued,
	  gboolean as_child,
	  gboolean separator)
{
  GbMenuItemData *item, *selected_item;
  GtkWidget *clist;
  gint row;

  item = g_new0 (GbMenuItemData, 1);
  item->stock_item_index = 0;
  if (separator)
    {
      item->label = NULL;
      item->name = glade_project_new_widget_name (menued->project,
						  _("separator"));
    }
  else
    {
      item->label = glade_project_new_widget_name (menued->project, "item");
      item->name = g_strdup (item->label);
    }
  item->handler = generate_handler (menued, -1, item->label, item->name);
  /* This is a flag to indicate that the last_mod_time should be set when
     the 'Apply' button is clicked. */
  item->last_mod_time = (time_t) -2;
  item->icon = NULL;
  item->tooltip = NULL;
  item->type = GB_MENU_ITEM_NORMAL;
  item->active = FALSE;
  item->group = NULL;
  item->modifiers = 0;
  item->key = NULL;
  item->level = 0;
  item->generate_name = TRUE;
  item->generate_handler = TRUE;

  clist = menued->clist;
  row = get_selected_row (menued);
  if (row != -1)
    {
      selected_item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
      item->level = selected_item->level + (as_child ? 1 : 0);
      insert_item (GTK_CLIST (clist), item, row + 1);
      gtk_clist_select_row (GTK_CLIST (clist), row + 1, 0);
      ensure_visible (clist, row + 1);
    }
  else
    {
      item->level = 0;
      insert_item (GTK_CLIST (clist), item, -1);
      gtk_clist_select_row (GTK_CLIST (clist), GTK_CLIST (clist)->rows - 1, 0);
      ensure_visible (clist, GTK_CLIST (clist)->rows - 1);
    }

  set_interface_state (menued);
  gtk_widget_grab_focus (menued->label_entry);
  gtk_editable_select_region (GTK_EDITABLE (menued->label_entry), 0, -1);
}

/* This adds the item to the clist at the given position. */
static void
insert_item (GtkCList * clist, GbMenuItemData * item, gint row)
{
  gchar *rowdata[GB_MENUED_NUM_COLS];

  /* Empty labels are understood to be separators. */
  if (item->label && strlen (item->label) > 0)
    rowdata[GLD_COL_LABEL] = item->label;
  else
    rowdata[GLD_COL_LABEL] = GB_SEPARATOR_TEXT;
  if (item->type == GB_MENU_ITEM_NORMAL)
    rowdata[GLD_COL_TYPE] = "";
  else if (item->type == GB_MENU_ITEM_CHECK)
    rowdata[GLD_COL_TYPE] = _("Check");
  else if (item->type == GB_MENU_ITEM_RADIO)
    rowdata[GLD_COL_TYPE] = _("Radio");
  rowdata[GLD_COL_ACCEL] = get_accel_string (item->key, item->modifiers);
  rowdata[GLD_COL_NAME] = item->name ? item->name : "";
  rowdata[GLD_COL_HANDLER] = item->handler ? item->handler : "";
  rowdata[GLD_COL_ICON] = item->icon ? item->icon : "";
  rowdata[GLD_COL_ACTIVE] = item->active ? _("Yes") : "";
  rowdata[GLD_COL_GROUP] = item->group ? item->group->name : item->name;
  if (item->type != GB_MENU_ITEM_RADIO || !rowdata[GLD_COL_GROUP])
    rowdata[GLD_COL_GROUP] = "";

  if (row >= 0)
    gtk_clist_insert (clist, row, rowdata);
  else
    row = gtk_clist_append (GTK_CLIST (clist), rowdata);

  gtk_clist_set_row_data (GTK_CLIST (clist), row, item);
  gtk_clist_set_shift (GTK_CLIST (clist), row, 0, 0, item->level * GB_INDENT);
}

/* This makes sure the given row is visible. */
static void
ensure_visible (GtkWidget *clist,
		gint row)
{
  if (gtk_clist_row_is_visible (GTK_CLIST (clist), row)
      != GTK_VISIBILITY_FULL)
    gtk_clist_moveto(GTK_CLIST(clist), row, -1, 0.5, 0);
}


static GbMenuItemData*
find_group_item (GladeMenuEditor * menued, char *group_name)
{
  gint rows, row;

  if (!group_name || !group_name[0])
    return NULL;

  rows = GTK_CLIST (menued->clist)->rows;

  for (row = 0; row < rows; row++)
    {
      GbMenuItemData *item;
      item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), row);

      if (item->name && !strcmp (item->name, group_name))
	return item;
    }

  return NULL;
}


/* This updates the currently selected item in the clist, updating each field
   if it is different to the settings in the form elements. */
static void
update_current_item (GladeMenuEditor * menued)
{
  GbMenuItemData *item, *group_item;
  GtkCList *clist;
  gchar *name, *label, *handler, *tooltip, *group_name, *key;
  gchar *icon;
  GbMenuItemType type;
  gint row;
  guint8 modifiers;
  gboolean active, update_accelerator = FALSE;

  clist = GTK_CLIST (menued->clist);
  row = get_selected_row (menued);
  if (row == -1)
    return;
  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (menued->clist),
						    row);

  name = (gchar*) gtk_entry_get_text (GTK_ENTRY (menued->name_entry));
  if (item_property_changed (name, item->name))
    {
      g_free (item->name);
      item->name = copy_item_property (name);
      gtk_clist_set_text (clist, row, GLD_COL_NAME,
			  item->name ? item->name : "");
    }

  label = (gchar*) gtk_entry_get_text (GTK_ENTRY (menued->label_entry));
  if (item_property_changed (label, item->label))
    {
      g_free (item->label);
      item->label = copy_item_property (label);
      gtk_clist_set_text (clist, row, GLD_COL_LABEL,
			  item->label ? item->label : GB_SEPARATOR_TEXT);
    }

  handler = (gchar*) gtk_entry_get_text (GTK_ENTRY (menued->handler_entry));
  if (item_property_changed (handler, item->handler))
    {
      g_free (item->handler);
      item->handler = copy_item_property (handler);
      gtk_clist_set_text (clist, row, GLD_COL_HANDLER,
			  item->handler ? item->handler : "");

      /* This is a flag to indicate that the last_mod_time should be set when
	 the 'Apply' button is clicked. */
      item->last_mod_time = (time_t) -2;
    }

  icon = (gchar*) gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (menued->icon_widget)->entry));
  if (item_property_changed (icon, item->icon))
    {
      g_free (item->icon);
      item->icon = copy_item_property (icon);
      gtk_clist_set_text (clist, row, GLD_COL_ICON,
			  item->icon ? item->icon : "");
    }

  tooltip = (gchar*) gtk_entry_get_text (GTK_ENTRY (menued->tooltip_entry));
  if (item_property_changed (tooltip, item->tooltip))
    {
      g_free (item->tooltip);
      item->tooltip = copy_item_property (tooltip);
    }

  if (GTK_TOGGLE_BUTTON (menued->normal_radiobutton)->active)
    type = GB_MENU_ITEM_NORMAL;
  else if (GTK_TOGGLE_BUTTON (menued->check_radiobutton)->active)
    type = GB_MENU_ITEM_CHECK;
  else
    type = GB_MENU_ITEM_RADIO;
  if (item->type != type)
    {
      /* If the item is changing from a radio item to something else, make
	 sure other items in the same group no longer point to it. */
      if (item->type == GB_MENU_ITEM_RADIO)
	remove_from_radio_group (menued, item);

      item->type = type;
      if (type == GB_MENU_ITEM_NORMAL)
	gtk_clist_set_text (clist, row, GLD_COL_TYPE, "");
      else if (type == GB_MENU_ITEM_CHECK)
	gtk_clist_set_text (clist, row, GLD_COL_TYPE, _("Check"));
      else if (type == GB_MENU_ITEM_RADIO)
	gtk_clist_set_text (clist, row, GLD_COL_TYPE, _("Radio"));
    }

  active = GTK_TOGGLE_BUTTON (menued->state_togglebutton)->active
    ? TRUE : FALSE;
  if (active != item->active)
    {
      item->active = active;
      gtk_clist_set_text (clist, row, GLD_COL_ACTIVE, active ? _("Yes") : "");
    }

  group_name = (gchar*) gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (menued->group_combo)->entry));
  group_item = find_group_item (menued, group_name);
  if (group_item != item->group)
    {
      char *group_text = group_item ? group_item->name : item->name;
      if (item->type != GB_MENU_ITEM_RADIO)
	group_text = NULL;
      item->group = group_item;
      gtk_clist_set_text (clist, row, GLD_COL_GROUP,
			  group_text ? group_text : "");
    }

  key = (gchar*) gtk_entry_get_text (GTK_ENTRY (menued->accel_key_entry));
  if (item_property_changed (key, item->key))
    {
      g_free (item->key);
      item->key = copy_item_property (key);
      update_accelerator = TRUE;
    }

  modifiers = 0;
  if (GTK_TOGGLE_BUTTON (menued->accel_ctrl_checkbutton)->active)
    modifiers |= GDK_CONTROL_MASK;
  if (GTK_TOGGLE_BUTTON (menued->accel_shift_checkbutton)->active)
    modifiers |= GDK_SHIFT_MASK;
  if (GTK_TOGGLE_BUTTON (menued->accel_alt_checkbutton)->active)
    modifiers |= GDK_MOD1_MASK;
  if (modifiers != item->modifiers)
    {
      item->modifiers = modifiers;
      update_accelerator = TRUE;
    }

  if (update_accelerator)
    gtk_clist_set_text (clist, row, GLD_COL_ACCEL,
			get_accel_string (item->key, item->modifiers));

  set_interface_state (menued);
}

/* This checks if the new value is different to the old, but an empty string
   in new is taken to be equal to NULL as well. */
static gboolean
item_property_changed (gchar *new, gchar *old)
{
  if (old == NULL)
    return (new == NULL || new[0] == '\0') ? FALSE : TRUE;
  if (new == NULL)
    return TRUE;
  if (!strcmp (old, new))
    return FALSE;
  return TRUE;
}

/* This returns a copy of the given property string, or NULL if it is an
   empty string. */
static gchar*
copy_item_property (gchar *property)
{
  if (property[0] == '\0')
    return NULL;
  return g_strdup (property);
}


/* This clears the form, ready to add a new item. If full is TRUE it resets
   the type checkbuttons, group and accelerator modifiers. When adding items
   full is set to FALSE so the user can add several items of the same type. */
static void
clear_form (GladeMenuEditor * menued,
	    gboolean full)
{
  gtk_list_select_item (GTK_LIST (GTK_COMBO (menued->stock_combo)->list), 0);
  set_entry_text (GTK_ENTRY (GTK_COMBO (menued->icon_widget)->entry), "");
  set_entry_text (GTK_ENTRY (menued->label_entry), "");
  set_entry_text (GTK_ENTRY (menued->name_entry), "");
  set_entry_text (GTK_ENTRY (menued->handler_entry), "");
  set_entry_text (GTK_ENTRY (menued->tooltip_entry), "");
  set_entry_text (GTK_ENTRY (menued->accel_key_entry), "");

  if (full)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->normal_radiobutton), TRUE);
      set_entry_text (GTK_ENTRY (GTK_COMBO (menued->group_combo)->entry),
		      "");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_ctrl_checkbutton), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_shift_checkbutton), FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menued->accel_alt_checkbutton), FALSE);
    }
}


/* This removes an item and its children from the clist, and returns a list
   of the removed items. */
static GList *
remove_item_and_children (GtkWidget * clist,
			  gint row)
{
  GList *items = NULL;
  GbMenuItemData *item;
  gint level;

  item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
  level = item->level;
  items = g_list_append (items, item);
  gtk_clist_remove (GTK_CLIST (clist), row);

  while (row < GTK_CLIST (clist)->rows)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (clist), row);
      if (item->level > level)
	{
	  items = g_list_append (items, item);
	  gtk_clist_remove (GTK_CLIST (clist), row);
	}
      else
	break;
    }
  return items;
}

/* This inserts the given list of items at the given position in the clist. */
static void
insert_items (GtkWidget * clist,
	      GList * items,
	      gint row)
{
  GbMenuItemData *item;

  while (items)
    {
      item = (GbMenuItemData *) items->data;
      insert_item (GTK_CLIST (clist), item, row++);
      items = items->next;
    }
}


/* This returns the default name of the widget, given its label. The returned
   string should be freed at some point. */
static gchar*
generate_name (GladeMenuEditor *menued,
	       gchar *label)
{
  gchar *prefix, *name, *src, *dest;

  /* For empty labels, i.e. separators, use 'separator'. */
  if (label == NULL || label[0] == '\0')
    {
      return glade_project_new_widget_name (menued->project, _("separator"));
    }

  prefix = g_malloc (strlen (label) + 1);
  /* Convert spaces to underscores, and ignore periods (e.g. in "Open...")
     and underscores (e.g. in "_Open"). */
  for (src = label, dest = prefix; *src; src++)
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

  if (dest >= prefix + strlen (label) + 1)
    g_warning ("Buffer overflow");

  /* Get rid of any trailing digits. */
  dest--;
  while (*dest >= '0' && *dest <= '9')
    {
      *dest = '\0';
      dest--;
    }

  name = glade_project_new_widget_name (menued->project, prefix);
  g_free (prefix);

  return name;
}

/* This returns the default 'activate' handler name, given the name of the
   item. The returned string should be freed at some point. */
static gchar*
generate_handler (GladeMenuEditor *menued, gint row, gchar *label, gchar *name)
{
  gchar *handler, *start = "on_", *end = "_activate";

  /* For empty labels, i.e. separators, and items with submenus, there is no
     handler by default. */
  if (label == NULL || label[0] == '\0' || is_parent (menued, row))
    return NULL;

  handler = g_malloc (strlen (name) + strlen (start) + strlen (end) + 1);
  strcpy (handler, start);
  strcat (handler, name);
  strcat (handler, end);

  return handler;
}


/* This makes sure the default handlers are updated as items are moved around.
 */
static void
check_generated_handlers (GladeMenuEditor *menued)
{
  GtkCList *clist;
  GbMenuItemData *item;
  gint row;
  gchar *handler;

  clist = GTK_CLIST (menued->clist);
  for (row = 0; row < clist->rows; row++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (clist, row);
      if (item->generate_handler)
	{
	  handler = generate_handler (menued, row, item->label, item->name);
	  if (item_property_changed (handler, item->handler))
	    {
	      g_free (item->handler);
	      item->handler = handler;
	      gtk_clist_set_text (clist, row, GLD_COL_HANDLER, handler);
	    }
	  else
	    {
	      g_free (handler);
	    }
	}
    }
}


/* This returns TRUE id the item in the given row is a parent, or FALSE
   if the row doesn't exist or isn't a parent. */
static gboolean
is_parent (GladeMenuEditor *menued,
	   gint row)
{
  GtkCList *clist;
  GbMenuItemData *item, *next_item;

  clist = GTK_CLIST (menued->clist);
  if (row < 0 || row >= clist->rows - 1)
    return FALSE;
  item = (GbMenuItemData *) gtk_clist_get_row_data (clist, row);
  next_item = (GbMenuItemData *) gtk_clist_get_row_data (clist, row + 1);
  if (next_item->level > item->level)
    return TRUE;
  return FALSE;
}


static void
on_menu_editor_ok (GtkWidget *button,
		   GladeMenuEditor *menued)
{
  glade_menu_editor_update_menu (menued);
  gtk_widget_destroy (GTK_WIDGET (menued));
}

static void
on_menu_editor_apply (GtkWidget *button,
		      GladeMenuEditor *menued)
{
  glade_menu_editor_update_menu (menued);
}


static void
on_menu_editor_close (GtkWidget *widget,
		      GladeMenuEditor *menued)
{
  gtk_widget_destroy (GTK_WIDGET (menued));
}


/**************************************************************************
 * Public functions
 **************************************************************************/

/* This checks if the given icon string is a stock icon name, and if it is
   it returns the stock ID instead. If not, it just returns the icon. */
static gchar*
get_stock_id_from_icon_name (GladeMenuEditor    *menued,
			     gchar		*icon)
{
  if (!icon || *icon == '\0')
    return NULL;

  /* We use the stock id instead of the text now, so we don't need this. */
#if 0
  GList *clist;

  clist = GTK_LIST (GTK_COMBO (menued->icon_widget)->list)->children;

  while (clist && clist->data)
    {
      gchar* ltext = glade_util_gtk_combo_func (GTK_LIST_ITEM (clist->data));
      if (!ltext)
	continue;
      if (!strcmp (ltext, icon))
	{
	  gchar *stock_id = gtk_object_get_data (GTK_OBJECT (clist->data),
						 GladeMenuEditorStockIDKey);
	  return stock_id;
	}
      clist = clist->next;
    }
#endif

  return icon;
}


/* This updates the menu, based on the settings in the menu editor.
   It removes all the current children of the menu and recreates it.
   Note that it has to reload all the xpm files for pixmaps, so its not
   very efficient, but they're small so it shouldn't be too bad. */
static void
glade_menu_editor_update_menu (GladeMenuEditor    *menued)
{
  GbMenuItemData *item;
  GtkWidget *menuitem, *label, *prev_item = NULL, *child_menu;
  GtkCList *clist;
  GtkMenuShell *current_menu;
  GList *menus;
  GHashTable *group_hash;
  gchar *child_name;
  gint i, level;
  GbWidget *gbwidget;
  GladeWidgetData *wdata;
  GtkAccelGroup *accel_group;
  GtkWidget *pixmap = NULL;
  gboolean use_pixmap_menu_item;
  gchar *stock_id, *icon_name;
#ifdef USE_GNOME
  GnomeUIInfo *uiinfo = NULL;
#endif

  /* Remove existing children of the menu. Note that this will result in the
     old widget names being released, so we need to reserve the new names,
     even if they are the same. */
  while (menued->menu->children)
    {
      menuitem = menued->menu->children->data;
      gtk_widget_destroy (menuitem);
    }

  /* FIXME: This seems to be necessary to re-initialise the menu. I don't know
     why. */
  menued->menu->menu_flag = TRUE;

  clist = GTK_CLIST (menued->clist);

  /* Now reserve all the the new widget names. */
  for (i = 0; i < clist->rows; i++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (clist, i);

      if (item->name && item->name[0])
	glade_project_reserve_name (menued->project, item->name);
    }

  /* Now add widgets according to the items in the menu editor clist. */
  level = 0;
  menus = g_list_append (NULL, menued->menu);
  group_hash = g_hash_table_new (NULL, NULL);

  /* Make sure all items in the group point to the first one. */
  normalize_radio_groups (menued);

  for (i = 0; i < clist->rows; i++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (clist, i);

      icon_name = get_stock_id_from_icon_name (menued, item->icon);
      stock_id = NULL;

      if (item->level > level)
	{
	  child_menu = gb_widget_new_full ("GtkMenu", FALSE, NULL, NULL, 0, 0,
					   NULL, GB_CREATING, NULL);
	  child_name = g_strdup_printf ("%s_menu", prev_item->name);
	  gtk_widget_set_name (child_menu, child_name);
	  g_free (child_name);
	  level = item->level;
	  /* We use the menus GList as a stack, pushing menus onto the
	     front. */
	  menus = g_list_prepend (menus, child_menu);
	  gtk_menu_item_set_submenu (GTK_MENU_ITEM (prev_item), child_menu);
	  tree_add_widget (child_menu);
	}
      while (item->level < level)
	{
	  /* This removes/pops the first menu in the list. */
	  menus = g_list_remove_link (menus, menus);
	  level--;
	}
      current_menu = GTK_MENU_SHELL (menus->data);

      pixmap = NULL;
      if (item->label && strlen (item->label) > 0)
	{
	  label = gtk_accel_label_new ("");
	  gtk_label_set_text_with_mnemonic (GTK_LABEL (label), item->label);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_widget_show (label);

	  use_pixmap_menu_item = FALSE;

	  if (menued->gnome_support)
	    {
#ifdef USE_GNOME
	      if (item->stock_item_index)
		{
		  uiinfo = &GladeStockMenuItemValues[item->stock_item_index];
		  if (uiinfo->pixmap_type == GNOME_APP_PIXMAP_STOCK)
		    {
		      pixmap = gtk_image_new_from_stock (uiinfo->pixmap_info,
							 GTK_ICON_SIZE_MENU);
		      use_pixmap_menu_item = TRUE;
		    }
		}
#endif
	    }
	  else
	    {
	      if (item->stock_item_index)
		{
		  stock_id = g_slist_nth_data (menued->stock_items,
					       item->stock_item_index - 1);
#if 0
		  g_print ("Stock ID for icon: %s\n", stock_id);
#endif
		  pixmap = gtk_image_new_from_stock (stock_id,
						     GTK_ICON_SIZE_MENU);
		  use_pixmap_menu_item = TRUE;
		}
	    }

	  if (!item->stock_item_index && icon_name)
	    {
	      pixmap = gb_widget_new ("GtkImage", NULL);
	      if (glade_util_check_is_stock_id (icon_name))
		{
		  gtk_image_set_from_stock (GTK_IMAGE (pixmap), icon_name,
					    GTK_ICON_SIZE_MENU);
		}
	      else
		{
		  gtk_image_set_from_file (GTK_IMAGE (pixmap), icon_name);

		  /* Add the pixmap to the project, so the file is copied.
		     It should be removed from the project in the menuitem
		     GbWidget.destroy function. */
		  glade_project_add_pixmap (menued->project, icon_name);
		}
	      gb_widget_set_child_name (pixmap, GladeChildMenuItemImage);
	      use_pixmap_menu_item = TRUE;
	    }

	  if (item->type == GB_MENU_ITEM_NORMAL)
	    {
	      if (use_pixmap_menu_item)
		{
		  menuitem = gtk_image_menu_item_new ();
		  if (pixmap)
		    {
		      gtk_widget_show (pixmap);
		      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem),
						     pixmap);
		    }
		}
	      else
		{
		  menuitem = gtk_menu_item_new ();
		}
	      gtk_container_add (GTK_CONTAINER (menuitem), label);
	    }
	  else if (item->type == GB_MENU_ITEM_CHECK)
	    {
	      menuitem = gtk_check_menu_item_new ();
	      if (item->active)
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
						TRUE);
	      gtk_container_add (GTK_CONTAINER (menuitem), label);
	    }
	  else
	    {
	      menuitem = create_radio_menu_item (current_menu,
						 item, group_hash);
	      if (item->active)
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem),
						TRUE);
	      gtk_container_add (GTK_CONTAINER (menuitem), label);
	    }

	  gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), menuitem);
	}
      else
	{
	  /* This creates a separator. */
	  menuitem = gtk_separator_menu_item_new ();
	}
      gtk_widget_show (menuitem);

      /* Save the stock name and icon in the menuitem. */
      if (item->stock_item_index)
	{
	  if (menued->gnome_support)
	    {
#ifdef USE_GNOME
	      gtk_object_set_data (GTK_OBJECT (menuitem),
				   GladeMenuItemStockIndexKey,
				   GINT_TO_POINTER (item->stock_item_index));
#endif
	    }
	  else
	    {
	      gchar *stock_id;

	      stock_id = g_slist_nth_data (menued->stock_items,
					   item->stock_item_index - 1);

	      gtk_object_set_data_full (GTK_OBJECT (menuitem),
					GladeMenuItemStockIDKey,
					g_strdup (stock_id),
					g_free);
	    }
	}
      else if (item->icon && pixmap)
	{
	  gtk_object_set_data_full (GTK_OBJECT (pixmap), GladeIconKey,
				    g_strdup (icon_name), g_free);
	}


      /* Turn it into a GbWidget, and add the 'activate' handler and the
	 accelerator. */
      if (item->name == NULL || item->name[0] == '\0')
	{
	  g_free (item->name);
	  item->name = glade_project_new_widget_name (menued->project, "item");
	}

      gbwidget = gb_widget_lookup_class (gtk_type_name (GTK_OBJECT_TYPE (menuitem)));

      if (item->wdata)
	wdata = glade_widget_data_copy (item->wdata);
      else
	wdata = glade_widget_data_new (gbwidget);

      wdata->gbwidget = gbwidget;

      gb_widget_create_from_full (menuitem, NULL, wdata);
      gtk_widget_set_name (menuitem, item->name);

      if (item->active)
	wdata->flags |= GLADE_ACTIVE;
      else
	wdata->flags &= ~GLADE_ACTIVE;

      g_free (wdata->tooltip);
      wdata->tooltip = g_strdup (item->tooltip);
      /* FIXME: Should set tooltip? Or for Gnome install in appbar? */

      if (item->handler && strlen (item->handler) > 0)
	{
	  GladeSignal *signal = g_new (GladeSignal, 1);
	  signal->name = g_strdup ("activate");
	  signal->handler = g_strdup (item->handler);
	  signal->object = NULL;
	  signal->after = FALSE;
	  signal->data = NULL;

	  /* If the last mod time is set to the special value, we set it to
	     the current time now. */
	  if (item->last_mod_time == (time_t) -2) {
	    item->last_mod_time = time (NULL);
	    if (item->last_mod_time == (time_t) -1)
	      g_warning ("Can't get current time");
	  }

	  signal->last_modification_time = item->last_mod_time;
	  wdata->signals = g_list_append (wdata->signals, signal);
	}

      if (item->key && strlen (item->key) > 0)
	{
	  GladeAccelerator *accel = g_new (GladeAccelerator, 1);
	  guint key;
	  accel->modifiers = item->modifiers;
	  accel->key = g_strdup (item->key);
	  accel->signal = g_strdup ("activate");
	  wdata->accelerators = g_list_append (wdata->accelerators, accel);

	  /* We can only add accelerators to menus, not menubars. */
	  if (GTK_IS_MENU (current_menu))
	    {
	      key = glade_keys_dialog_find_key (item->key);
	      accel_group = GTK_MENU (current_menu)->accel_group;
	      gtk_widget_add_accelerator (menuitem, "activate", accel_group,
					  key, item->modifiers,
					  GTK_ACCEL_VISIBLE);
	    }
	}

      if (menued->gnome_support)
	{
#ifdef USE_GNOME
	  /* For stock menu items, we use the configured accelerator keys. */
	  if (GTK_IS_MENU (current_menu)
	      && item->stock_item_index && uiinfo->accelerator_key != 0)
	    {
	      accel_group = GTK_MENU (current_menu)->accel_group;
	      gtk_widget_add_accelerator (menuitem, "activate", accel_group,
					  uiinfo->accelerator_key,
					  uiinfo->ac_mods,
					  GTK_ACCEL_VISIBLE);
	    }
#endif
	}
      else
	{
	  /* For stock menu items, we use the configured accelerator keys. */
	  if (GTK_IS_MENU (current_menu) && item->stock_item_index && stock_id)
	    {
	      GtkStockItem item;

	      gtk_stock_lookup (stock_id, &item);
	      if (item.keyval)
		{
		  accel_group = GTK_MENU (current_menu)->accel_group;
		  gtk_widget_add_accelerator (menuitem, "activate",
					      accel_group,
					      item.keyval, item.modifier,
					      GTK_ACCEL_VISIBLE);
		}
	    }
	}

      /* Add the menuitem to the current menu. */
      gtk_menu_shell_append (current_menu, menuitem);
      tree_add_widget (menuitem);
      prev_item = menuitem;
    }
  g_list_free (menus);
  g_hash_table_destroy (group_hash);

  /* Make sure the displayed item is correct. */
  show_item_properties (menued);
}


/* This creates a radio menu item using the appropriate radio group.
   If the item's group is not NULL and doesn't points to the item itself,
   then it will search the list of groups to find the group. If it can't find
   a group, or it didn't have one it creates a new group.
   If it creates a new group it adds this item to the list of groups. */
static GtkWidget*
create_radio_menu_item (GtkMenuShell *menu,
			GbMenuItemData *item,
			GHashTable *group_hash)
{
  GtkWidget *menuitem;
  GSList *group = NULL;

  if (item->group && item->group != item)
    {
      GtkRadioMenuItem *group_widget = g_hash_table_lookup (group_hash,
							    item->group);
      if (group_widget)
	group = gtk_radio_menu_item_get_group (group_widget);
    }

  menuitem = gtk_radio_menu_item_new (group);
  if (!group)
    g_hash_table_insert (group_hash, item, menuitem);

  return menuitem;
}


static void
glade_menu_editor_on_menu_destroyed (GtkWidget *menu,
				     GtkWidget *menued)
{
  gtk_widget_destroy (menued);
}


/* This converts the given menu/menubar into the list of items displayed in
   the menu editor. */
static void
glade_menu_editor_set_menu	 (GladeMenuEditor *menued,
				  GtkMenuShell    *menu)
{
  /* First clear any current items and radio groups. */
  glade_menu_editor_reset (menued);

  /* Connect to the destroy signal of the menu widget, so we can destroy the
     menu editor when the widget is destroyed. */
  menued->menu_destroy_handler_id = gtk_signal_connect (GTK_OBJECT (menu),
							"destroy",
							GTK_SIGNAL_FUNC (glade_menu_editor_on_menu_destroyed),
							menued);

  /* Now add each of the menus/menuitems in the given menu. */
  menued->menu = menu;

  set_submenu (menued, menu, 0);
}


/* This checks if the given icon_name is a stock ID, and if it is it returns
   the text to display instead. If not, it returns icon_name. */
static gchar*
get_icon_name_from_stock_id (GladeMenuEditor *menued,
			     gchar	     *icon_name)
{
  /* We use the stock id instead of the text now. */
#if 0
  GtkStockItem item;
  if (icon_name && gtk_stock_lookup (icon_name, &item))
    return item.label;
  else
#endif

    return icon_name;
}


/* This recursively adds menus. */
static void
set_submenu (GladeMenuEditor *menued,
	     GtkMenuShell    *menu,
	     gint	      level)
{
  GbMenuItemData *item;
  GtkWidget *menuitem, *label;
  GList *child, *tmp_list;
  GladeWidgetData *wdata;
  gchar *icon_name;
  GList *groups = NULL;

  child = menu->children;
  while (child)
    {
      menuitem = GTK_WIDGET (child->data);

      if (!GTK_IS_MENU_ITEM (menuitem))
	{
	  g_warning ("Menu widget is not a menu item");
	  child = child->next;
	  continue;
	}
      /* FIXME: We can't handle tearoff menuitems at present. */
      if (GTK_IS_TEAROFF_MENU_ITEM (menuitem))
	{
	  child = child->next;
	  continue;
	}

      wdata = (GladeWidgetData*) gtk_object_get_data (GTK_OBJECT(menuitem),
						      GB_WIDGET_DATA_KEY);
      if (wdata == NULL)
	{
	  g_warning ("Menu widget has no GladeWidgetData");
	  child = child->next;
	  continue;
	}

      item = g_new0 (GbMenuItemData, 1);

      if (menued->gnome_support)
	{
#ifdef USE_GNOME
	  item->stock_item_index = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (menuitem), GladeMenuItemStockIndexKey));
#endif
	}
      else
	{
	  gchar *stock_id;
	  GSList *elem;
	  gint idx;

	  stock_id = gtk_object_get_data (GTK_OBJECT (menuitem),
					  GladeMenuItemStockIDKey);
	  item->stock_item_index = 0;
	  if (stock_id)
	    {
	      for (elem = menued->stock_items, idx = 1; elem;
		   elem = elem->next, idx++)
		{
		  if (!strcmp (elem->data, stock_id))
		    {
		      item->stock_item_index = idx;
		      break;
		    }
		}
	    }
	}

      item->name = g_strdup (gtk_widget_get_name (menuitem));
      item->label = NULL;
      item->handler = NULL;
      item->last_mod_time = 0;
      item->icon = NULL;

      /* If it isn't a stock item, we check if it has an icon set. */
      if (item->stock_item_index == 0 && GTK_IS_IMAGE_MENU_ITEM (menuitem))
	{
	  GtkWidget *image;
	  image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (menuitem));
	  if (image)
	    {
	      icon_name = gtk_object_get_data (GTK_OBJECT (image), GladeIconKey);
	      item->icon = g_strdup (get_icon_name_from_stock_id (menued, icon_name));
	    }
	}

      item->tooltip = g_strdup (wdata->tooltip);
      item->type = GB_MENU_ITEM_NORMAL;
      item->active = FALSE;
      item->group = NULL;
      item->modifiers = 0;
      item->key = NULL;
      item->level = level;
      item->generate_name = FALSE;
      item->generate_handler = FALSE;
      item->wdata = glade_widget_data_copy (wdata);

      if (GTK_IS_RADIO_MENU_ITEM (menuitem))
	{
	  item->type = GB_MENU_ITEM_RADIO;
	  item->group = find_radio_group (GTK_RADIO_MENU_ITEM (menuitem),
					  &groups, item);
	}
      else if (GTK_IS_CHECK_MENU_ITEM (menuitem))
	item->type = GB_MENU_ITEM_CHECK;

      label = GTK_BIN (menuitem)->child;
      if (label && GTK_IS_LABEL (label))
	item->label = glade_util_get_label_text (label);

      if (GTK_IS_CHECK_MENU_ITEM (menuitem)
	  && GTK_CHECK_MENU_ITEM (menuitem)->active)
	item->active = TRUE;
	
      /* Find 'activate' handler in widget data. */
      tmp_list = item->wdata->signals;
      while (tmp_list)
	{
	  GladeSignal *signal = (GladeSignal *) tmp_list->data;
	  if (!strcmp (signal->name, "activate"))
	    {
	      item->handler = g_strdup (signal->handler);
	      item->last_mod_time = signal->last_modification_time;

	      /* Remove the signal from our copy of the GladeWidgetData. */
	      glade_widget_data_free_signal (signal);
	      item->wdata->signals = g_list_remove (item->wdata->signals,
						    signal);
	      break;
	    }
	  tmp_list = tmp_list->next;
	}

      /* Find 'activate' accelerator in widget data. */
      tmp_list = item->wdata->accelerators;
      while (tmp_list)
	{
	  GladeAccelerator *accel = (GladeAccelerator *) tmp_list->data;
	  if (!strcmp (accel->signal, "activate"))
	    {
	      item->key = g_strdup (accel->key);
	      item->modifiers = accel->modifiers;

	      /* Remove the accel from our copy of the GladeWidgetData. */
	      glade_widget_data_free_accel (accel);
	      item->wdata->accelerators = g_list_remove (item->wdata->accelerators,
							 accel);
	      break;
	    }
	  tmp_list = tmp_list->next;
	}


      insert_item (GTK_CLIST (menued->clist), item, -1);

      if (GTK_MENU_ITEM (menuitem)->submenu)
	{
	  set_submenu (menued,
		       GTK_MENU_SHELL (GTK_MENU_ITEM (menuitem)->submenu),
		       level + 1);
	}

      child = child->next;
    }

  g_list_free (groups);
}


/* This clears the clist, freeing all GbMenuItemDatas, and resets the radio
   group combo, freeing all the current radio groups. */
static void
glade_menu_editor_reset (GladeMenuEditor *menued)
{
  GbMenuItemData *item;
  gint i;

  /* Disconnect our destroy handler on the menu widget. */
  if (menued->menu)
    {
      gtk_signal_disconnect (GTK_OBJECT (menued->menu),
			     menued->menu_destroy_handler_id);
      menued->menu = NULL;
    }

  for (i = 0; i < GTK_CLIST (menued->clist)->rows; i++)
    {
      item = (GbMenuItemData *) gtk_clist_get_row_data (GTK_CLIST (menued->clist), i);
      glade_menu_editor_free_item (item);

    }
  gtk_clist_clear (GTK_CLIST (menued->clist));

  gtk_list_clear_items (GTK_LIST (GTK_COMBO (menued->group_combo)->list),
			0, -1);
}


static void
glade_menu_editor_free_item (GbMenuItemData *item)
{
  g_free (item->name);
  g_free (item->label);
  g_free (item->handler);
  g_free (item->icon);
  g_free (item->tooltip);
  g_free (item->key);

  if (item->wdata)
    glade_widget_data_free (item->wdata);

  g_free (item);
}


/* Make sure all item group fields point to the first item in the group. */
static void
normalize_radio_groups (GladeMenuEditor * menued)
{
  GbMenuItemData *item, *group;
  GList *groups = NULL;
  gint rows, row;

  /* Step through each row checking each radio item. */
  rows = GTK_CLIST (menued->clist)->rows;
  for (row = 0; row < rows; row++)
    {
      item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), row);
      if (item->type != GB_MENU_ITEM_RADIO)
	continue;

      /* Follow the chain of groups until we get the real one. */
      group = item->group;
      while (group && group->group && group->group != group)
	group = group->group;

      /* If it is a new group, add it to the list. */
      if (!group || group == item)
	{
	  groups = g_list_prepend (groups, item);
	  item->group = NULL;
	}
      /* Else check if the group item is after the current one. If it is,
	 then we make the current item the group leader. We assume that if
	 we haven't seen the group item yet, it must be after this one. */
      else if (!g_list_find (groups, group))
	{
	  groups = g_list_prepend (groups, item);
	  group->group = item;
	  item->group = NULL;
	}
      else
	{
	  item->group = group;
	}
    }

  g_list_free (groups);
}


/* This recreates the list of available radio groups, and puts them in the
   combo's drop-down list. */
static void
update_radio_groups (GladeMenuEditor * menued)
{
  GbMenuItemData *item, *tmp_item;
  GList *groups = NULL, *elem;
  gint item_row, row, parent_row, rows;
  GtkCombo *combo;
  GtkList *list;
  GtkWidget *li;

  /* Make sure all item group fields point to the first item in the group. */
  normalize_radio_groups (menued);

  item_row = get_selected_row (menued);
  if (item_row == -1)
    {
      gtk_list_clear_items (GTK_LIST (GTK_COMBO (menued->group_combo)->list),
			    0, -1);
      return;
    }

  item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), item_row);

  /* Step backwards to find the parent item. */
  parent_row = -1;
  for (row = item_row - 1; row > 0; row--)
    {
      tmp_item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), row);
      if (tmp_item->level < item->level)
	{
	  parent_row = row;
	  break;
	}
    }

  /* Now step through the items, checking all items that are on the same
     level as the current one, until we reach the end of the list or find an
     item on a higher level. */
  rows = GTK_CLIST (menued->clist)->rows;
  for (row = parent_row + 1; row < rows; row++)
    {
      tmp_item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), row);
      if (tmp_item->level < item->level)
	break;

      if (tmp_item->level == item->level
	  && tmp_item->type == GB_MENU_ITEM_RADIO
	  && tmp_item->name && tmp_item->name[0])
	{
	  /* If the item has its group set to NULL or itself, then it is a new
	     group, so add its name. */
	  if (!tmp_item->group || tmp_item->group == tmp_item)
	    groups = g_list_prepend (groups, tmp_item->name);
	}
    }

  groups = g_list_sort (groups, (GCompareFunc)strcmp);

  combo = GTK_COMBO (menued->group_combo);
  list = GTK_LIST (combo->list);

  /* We have to block the combo's list's selection changed signal, or it
     causes problems. */
  gtk_signal_handler_block (GTK_OBJECT (list),
			    GTK_COMBO (menued->group_combo)->list_change_id);

  gtk_list_clear_items (list, 0, -1);

  /* Add the special 'New' item to create a new group. */
  li = gtk_list_item_new_with_label (_("New"));
  gtk_widget_show (li);
  gtk_container_add (GTK_CONTAINER (list), li);
  gtk_combo_set_item_string (combo, GTK_ITEM (li), "");

  /* Add a separator. */
  li = gtk_list_item_new ();
  gtk_widget_show (li);
  gtk_container_add (GTK_CONTAINER (list), li);

  for (elem = groups; elem; elem = elem->next)
    {
      li = gtk_list_item_new_with_label (elem->data);
      gtk_widget_show (li);
      gtk_container_add (GTK_CONTAINER (list), li);
    }

  gtk_signal_handler_unblock (GTK_OBJECT (list),
			    GTK_COMBO (menued->group_combo)->list_change_id);
  g_list_free (groups);
}


/* This finds the group item to use for the given radiomenuitem widget.
   It searches the list of group items to find the first one that is also in
   the radiomenuitem's group list. If none is found, it creates a new group
   and adds it to the list. */
static GbMenuItemData*
find_radio_group (GtkRadioMenuItem *menuitem, GList **groups,
		  GbMenuItemData *item)
{
  GSList *item_group_list;
  GList *elem;

  item_group_list = menuitem->group;

  /* The groups list contains pairs of GSList + GbMenuItemData*. */
  for (elem = *groups; elem; elem = elem->next->next)
    {
      GSList *elem_group = elem->data;
      GbMenuItemData *elem_item = elem->next->data;

      if (elem_group == item_group_list)
	return elem_item;
    }

  /* We couldn't find an existing group that matches, so we create a new one.
   */
  *groups = g_list_prepend (*groups, item);
  *groups = g_list_prepend (*groups, item_group_list);
  return NULL;
}


/* This is called to make sure that no items have the given item as their
   group leader. It is called when an item is removed, or when its type is
   changed from a radio item to something else.

   It steps through the list, checking for radio items with
   the group field set to the given item. The first time it finds one, it
   creates a new group. It sets the group field of all other items in the
   same group to the new first item in the group. */
static void
remove_from_radio_group (GladeMenuEditor * menued,
			 GbMenuItemData *item)
{
  GbMenuItemData *new_group_item = NULL;
  gint rows, row;

  rows = GTK_CLIST (menued->clist)->rows;
  for (row = 0; row < rows; row++)
    {
      GbMenuItemData *tmp_item;
      tmp_item = gtk_clist_get_row_data (GTK_CLIST (menued->clist), row);

      if (tmp_item->type == GB_MENU_ITEM_RADIO
	  && tmp_item->group == item && tmp_item != item)
	{
	  if (new_group_item)
	    {
	      tmp_item->group = new_group_item;
	    }
	  else
	    {
	      tmp_item->group = NULL;
	      new_group_item = tmp_item;
	    }
	}
    }
}

