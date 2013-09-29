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

#include <gtk/gtkhbox.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtklabel.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtktable.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtksignal.h>

#include "graphics/selector.xpm"
#include "glade_palette.h"
#include "glade.h"

GtkWidget *glade_palette = NULL;

#define GLADE_CLASS_ID_KEY "GladeClassID"

static void glade_palette_class_init (GladePaletteClass * klass);
static void glade_palette_init (GladePalette * glade_palette);

static GladePaletteSection *new_section (GladePalette * palette,
					 const gchar * section);
static void rebuild_page (gpointer key,
			  GladePaletteSection * page,
			  GladePalette * palette);
static void remove_button (gpointer button,
			   gpointer table);
static void add_button (GtkWidget * button,
			GladePaletteSection * sect);
static void on_palette_button_toggled (GtkWidget * button,
				       GladePalette * palette);
static void on_section_button_clicked (GtkWidget * button,
				       GladePalette * palette);

enum
{
  SELECT_ITEM,
  UNSELECT_ITEM,
  LAST_SIGNAL
};

static guint glade_palette_signals[LAST_SIGNAL] = {0};

static GtkWindowClass *parent_class = NULL;


GType
glade_palette_get_type (void)
{
  static GType palette_type = 0;

  if (!palette_type)
    {
      GtkTypeInfo palette_info =
      {
	"GladePalette",
	sizeof (GladePalette),
	sizeof (GladePaletteClass),
	(GtkClassInitFunc) glade_palette_class_init,
	(GtkObjectInitFunc) glade_palette_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      palette_type = gtk_type_unique (gtk_window_get_type (), &palette_info);
    }
  return palette_type;
}


static void
glade_palette_class_init (GladePaletteClass * klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (gtk_window_get_type ());

  glade_palette_signals[SELECT_ITEM] =
    gtk_signal_new ("select_item",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (GladePaletteClass, select_item),
		    gtk_marshal_VOID__STRING,
		    GTK_TYPE_NONE, 1, GTK_TYPE_STRING);
  glade_palette_signals[UNSELECT_ITEM] =
    gtk_signal_new ("unselect_item",
		    GTK_RUN_LAST,
		    GTK_CLASS_TYPE (object_class),
		    GTK_SIGNAL_OFFSET (GladePaletteClass, unselect_item),
		    gtk_marshal_VOID__STRING,
		    GTK_TYPE_NONE, 1, GTK_TYPE_STRING);

  klass->select_item = NULL;
  klass->unselect_item = NULL;
}


/* Ensure the correct button is depressed when the page is switched with
   keyboard accelerators. */
static void
on_notebook_switch_page (GtkNotebook *notebook, GtkNotebookPage *page,
			 guint page_num, GladePalette *palette)
{
  GSList *elem;
  gint last_page;

  last_page = g_list_length (notebook->children) - 1;

  for (elem = palette->section_button_group; elem; elem = elem->next)
    {
      GladePaletteSection *sect = gtk_object_get_data (GTK_OBJECT (elem->data), "section");

      if ((sect->page == -1 && page_num == last_page)
	  || sect->page == page_num)
	{
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (elem->data), TRUE);
	  break;
	}
    }
}


static void
glade_palette_init (GladePalette * palette)
{
  GtkWidget *seperator, *hbox;
  GdkPixmap *gdkpixmap;
  GdkBitmap *gdkmask;
  GtkWidget *pixmap;
  GtkReliefStyle relief_style;

  palette->width = 4;
  palette->selected_widget = NULL;
  palette->hold_selected_widget = FALSE;
  palette->sections = g_hash_table_new (g_str_hash, g_str_equal);
  palette->tooltips = gtk_tooltips_new ();

  palette->vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (palette), palette->vbox);
  gtk_widget_show (palette->vbox);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (palette->vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  relief_style = GTK_RELIEF_NONE;
  
  palette->selector = gtk_radio_button_new (NULL);
  gtk_button_set_relief (GTK_BUTTON (palette->selector), relief_style);

  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gtk_widget_get_colormap (GTK_WIDGET (palette)), &gdkmask, NULL, selector_xpm);
  pixmap = gtk_pixmap_new (gdkpixmap, gdkmask);
  gtk_container_add (GTK_CONTAINER (GLADE_PALETTE (palette)->selector),
		     pixmap);
  gtk_widget_show (pixmap);

  palette->widget_button_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (palette->selector));
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (palette->selector), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), palette->selector, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (palette->selector), "toggled",
		      (GtkSignalFunc) on_palette_button_toggled, palette);
  gtk_widget_show (palette->selector);
  /* Don't translate 'Selector' here, since it is not displayed, and we test
     for it later. */
  gtk_object_set_data (GTK_OBJECT (palette->selector), GLADE_CLASS_ID_KEY,
		       "Selector");
  gtk_tooltips_set_tip (palette->tooltips, palette->selector,
			_ ("Selector"), NULL);

  palette->selected_item_label = gtk_label_new (_ ("Selector"));
  gtk_misc_set_alignment (GTK_MISC (palette->selected_item_label), 0, 0.5);
  gtk_widget_show (palette->selected_item_label);

  gtk_widget_set_usize (palette->selected_item_label, 10, -1);
  gtk_box_pack_start (GTK_BOX (hbox), palette->selected_item_label,
		      TRUE, TRUE, 0);

  seperator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (palette->vbox), seperator, FALSE, TRUE, 3);
  gtk_widget_show (seperator);

  palette->notebook = gtk_notebook_new ();
  /* We use gtk_box_pack_end, so we can later add buttons for new categories
     at the right place with gtk_box_pack_start */
  gtk_box_pack_end (GTK_BOX (palette->vbox), palette->notebook,
		    TRUE, TRUE, 0);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (palette->notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (palette->notebook), FALSE);
  gtk_widget_show (palette->notebook);

  gtk_signal_connect (GTK_OBJECT (palette->notebook), "switch_page",
		      GTK_SIGNAL_FUNC (on_notebook_switch_page), palette);

  seperator = gtk_hseparator_new ();
  gtk_box_pack_end (GTK_BOX (palette->vbox), seperator, FALSE, TRUE, 3);
  gtk_widget_show (seperator);
}


GtkWidget *
glade_palette_new ()
{
  return GTK_WIDGET (gtk_type_new (glade_palette_get_type ()));
}


void
glade_palette_add_widget (GladePalette *palette,
			  const gchar  *section,
			  const gchar  *name,
			  GdkPixmap    *gdkpixmap,
			  GdkBitmap    *mask,
			  const gchar  *tooltip)
{
  GladePaletteSection *sect;
  GtkWidget *button;
  GtkWidget *pixmap;
  GtkReliefStyle relief_style;
  
  g_return_if_fail (palette != NULL);
  g_return_if_fail (GLADE_IS_PALETTE (palette));
  g_return_if_fail (gdkpixmap != NULL);

  if (!(sect = g_hash_table_lookup (palette->sections, section)))
    sect = new_section (palette, section);

  relief_style = GTK_RELIEF_NONE;

  button = gtk_radio_button_new (palette->widget_button_group);
  gtk_button_set_relief (GTK_BUTTON (button), relief_style);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  palette->widget_button_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gtk_container_add (GTK_CONTAINER (button), pixmap);
  gtk_widget_show (pixmap);

  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (button), GLADE_CLASS_ID_KEY,
		       (gpointer) name);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) on_palette_button_toggled, palette);

  gtk_tooltips_set_tip (palette->tooltips, button, tooltip, NULL);

  sect->buttons = g_list_append (sect->buttons, button);
  g_hash_table_insert (sect->buttonhash, (gpointer) name, button);
  /* so we can use the same code as from rebuild_page */
  gtk_widget_ref (button);
  add_button (button, sect);
}


void
glade_palette_remove_widget (GladePalette * palette,
			     const gchar * section,
			     const gchar * name)
{
  GladePaletteSection *sect;
  GtkWidget *button;

  sect = g_hash_table_lookup (palette->sections, section);
  g_return_if_fail (sect != NULL);
  button = GTK_WIDGET (g_hash_table_lookup (sect->buttonhash, name));
  g_return_if_fail (button != NULL);
  g_hash_table_remove (sect->buttonhash, name);
  sect->buttons = g_list_remove (sect->buttons, button);
  gtk_container_remove (GTK_CONTAINER (sect->table), button);
  rebuild_page ((gpointer)section, sect, palette);
}


void
glade_palette_set_width (GladePalette * palette, guint columns)
{
  palette->width = columns;
  g_hash_table_foreach (palette->sections, (GHFunc) rebuild_page, palette);
}


static GladePaletteSection *
new_section (GladePalette * palette,
	     const gchar * section)
{
  GladePaletteSection *sect;
  GtkWidget *button;
  gboolean deprecated_page = FALSE;

  if (!strcmp (section, _("Dep_recated")))
    deprecated_page = TRUE;

  sect = g_new (GladePaletteSection, 1);
  sect->x = sect->y = 0;
  sect->table = gtk_table_new (1, palette->width, TRUE);
  gtk_widget_show (sect->table);
  sect->buttons = NULL;
  sect->buttonhash = g_hash_table_new (g_str_hash, g_str_equal);

  sect->section_button = button = gtk_radio_button_new_with_mnemonic (palette->section_button_group, section);
  gtk_widget_set_name (button, section);
  gtk_widget_set_usize (button, 20, -1);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
  palette->section_button_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_widget_show (button);
  gtk_object_set_data (GTK_OBJECT (button), "section", sect);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_section_button_clicked), palette);
  g_hash_table_insert (palette->sections, (gpointer) section, sect);

  /* Place the deprecated section last. */
  if (deprecated_page)
    {
      sect->page = -1;
      gtk_notebook_append_page (GTK_NOTEBOOK (palette->notebook), sect->table,
				NULL);
      gtk_box_pack_end (GTK_BOX (palette->vbox), button, FALSE, TRUE, 0);
    }
  else
    {
      sect->page = palette->next_page_to_add++;
      gtk_notebook_insert_page (GTK_NOTEBOOK (palette->notebook), sect->table,
				NULL, sect->page);
      gtk_box_pack_start (GTK_BOX (palette->vbox), button, FALSE, TRUE, 0);
    }

  return sect;
}

static void
rebuild_page (gpointer key,
	      GladePaletteSection * sect,
	      GladePalette * palette)
{
  g_list_foreach (g_list_first (sect->buttons), remove_button, sect->table);
  sect->x = 0;
  sect->y = 0;
  gtk_table_resize (GTK_TABLE (sect->table), 1, palette->width);
  g_list_foreach (g_list_first (sect->buttons), (GFunc) add_button, sect);
}

static void
remove_button (gpointer button,
	       gpointer table)
{
  g_return_if_fail (button != NULL);
  g_return_if_fail (table != NULL);
  g_return_if_fail (GTK_IS_CONTAINER (table));
  g_return_if_fail (GTK_IS_WIDGET (button));
  gtk_widget_ref (GTK_WIDGET (button));
  gtk_container_remove (GTK_CONTAINER (table), GTK_WIDGET (button));
}

static void
add_button (GtkWidget * button,
	    GladePaletteSection * sect)
{
  GladePalette *palette;

  /*                                                notebook  vbox   palette */
  palette = GLADE_PALETTE (GTK_WIDGET (sect->table)->parent->parent->parent);
  if (sect->x >= palette->width)
    {
      /* new row */
      sect->x = 0;
      sect->y++;
    }
  gtk_table_attach (GTK_TABLE (sect->table), button, sect->x, sect->x + 1,
		    sect->y, sect->y + 1, 0, 0, 0, 0);
  gtk_widget_unref (button);
  sect->x++;
}


static void
on_palette_button_toggled (GtkWidget * button,
			   GladePalette * palette)
{
  GdkModifierType modifiers;

  if ((GTK_TOGGLE_BUTTON (button)->active))
    {
      if (button == palette->selector)
	{
	  palette->selected_widget = NULL;
	  gtk_label_set_text (GTK_LABEL (palette->selected_item_label),
			      _("Selector"));
	  palette->hold_selected_widget = FALSE;
	}
      else
	{
	  palette->selected_widget = gtk_object_get_data (GTK_OBJECT (button),
							  GLADE_CLASS_ID_KEY);
	  gtk_label_set_text (GTK_LABEL (palette->selected_item_label),
			      palette->selected_widget);
	  gdk_window_get_pointer (button->window, NULL, NULL, &modifiers);
	  palette->hold_selected_widget = (modifiers & GDK_CONTROL_MASK)
	    ? TRUE : FALSE;
	}
      gtk_signal_emit (GTK_OBJECT (palette),
		       glade_palette_signals[SELECT_ITEM],
		       palette->selected_widget);
    }
  else
    {
      gtk_signal_emit (GTK_OBJECT (palette),
		       glade_palette_signals[UNSELECT_ITEM],
		       palette->selected_widget);
    }
}


static void
on_section_button_clicked (GtkWidget * button,
			   GladePalette * palette)
{
  GladePaletteSection *sect;

  sect = (GladePaletteSection*) gtk_object_get_data (GTK_OBJECT (button),
						     "section");
  gtk_notebook_set_current_page (GTK_NOTEBOOK (palette->notebook), sect->page);
}


/* Resets the palette so that the selector is selected, unless the existing
   item was selected while holding the Ctrl key. */
void
glade_palette_reset_selection (GladePalette *palette,
			       gboolean allow_hold)
{
  if (!palette->hold_selected_widget || !allow_hold)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (palette->selector), TRUE);
}


/* Returns TRUE if the selector is currently selected. */
gboolean
glade_palette_is_selector_on (GladePalette *palette)
{
  return (palette->selected_widget == NULL) ? TRUE : FALSE;
}


/* Returns the class name of the currently selected widget. */
gchar *
glade_palette_get_widget_class (GladePalette *palette)
{
  return palette->selected_widget;
}


static void
glade_palette_set_show_section (GladePalette        *palette,
				GladePaletteSection *sect,
				gboolean	     show)
{
  if (show)
    gtk_widget_show (sect->section_button);
  else
    gtk_widget_hide (sect->section_button);
}


static void
set_button_visibility (GladePaletteSection *sect,
		       const gchar         *name,
		       gboolean             show)
{
  GtkWidget *button = g_hash_table_lookup (sect->buttonhash, name);
  if (button)
    {
      if (show)
	gtk_widget_show (button);
      else
	gtk_widget_hide (button);
    }
}


/* Shows or hides the GNOME widgets. */
void
glade_palette_set_show_gnome_widgets (GladePalette *palette,
				      gboolean	    show_gnome,
				      gboolean	    show_gnome_db)
{
  GladePaletteSection *sect;

  /* Show the main page, and make the selector active. */
  gtk_notebook_set_current_page (GTK_NOTEBOOK (palette->notebook), 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (palette->selector), TRUE);

  /* Show or hide the Gnome page if it exists. */
  sect = g_hash_table_lookup (palette->sections, "_Gnome");
  if (sect)
    glade_palette_set_show_section (palette, sect, show_gnome);

  /* Show or hide the GnomeDB page if it exists. */
  sect = g_hash_table_lookup (palette->sections, "Gnome _DB");
  if (sect)
    glade_palette_set_show_section (palette, sect, show_gnome_db);

  /* Show or hide the GNOME widgets on the Deprecated page if they exist. */
  sect = g_hash_table_lookup (palette->sections, "Dep_recated");
  if (sect)
    {
      set_button_visibility (sect, "GnomeDialog", show_gnome);
      set_button_visibility (sect, "GnomeMessageBox", show_gnome);
      set_button_visibility (sect, "GnomePropertyBox", show_gnome);
      set_button_visibility (sect, "GnomePixmap", show_gnome);
      set_button_visibility (sect, "GnomeColorPicker", show_gnome);
      set_button_visibility (sect, "GnomeFontPicker", show_gnome);
      set_button_visibility (sect, "GnomeAbout", show_gnome);
      set_button_visibility (sect, "GnomeIconList", show_gnome);
      set_button_visibility (sect, "GnomeEntry", show_gnome);
      set_button_visibility (sect, "GnomeFileEntry", show_gnome);
      set_button_visibility (sect, "GnomePixmapEntry", show_gnome);
    }
}

