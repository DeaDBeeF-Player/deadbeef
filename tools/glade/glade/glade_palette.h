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
#ifndef GLADE_PALETTE_H
#define GLADE_PALETTE_H

#include <gtk/gtkwindow.h>
#include <gtk/gtktooltips.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* This is the only palette. */
extern GtkWidget *glade_palette;


#define GLADE_PALETTE(obj)          GTK_CHECK_CAST (obj, glade_palette_get_type (), GladePalette)
#define GLADE_PALETTE_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, glade_palette_get_type (), GladePaletteClass)
#define GLADE_IS_PALETTE(obj)       GTK_CHECK_TYPE (obj, glade_palette_get_type ())


typedef struct _GladePaletteSection	GladePaletteSection;
typedef struct _GladePalette		GladePalette;
typedef struct _GladePaletteClass	GladePaletteClass;

struct _GladePaletteSection
{
  /* The notebook page number for the section. */
  gint page;

  /* The button to switch to this section. */
  GtkWidget *section_button;

  /* The table containing all the widget icons for the section. */
  GtkWidget *table;

  /* The position in the table where the next button for this section will be
     added. */
  guint x;
  guint y;

  /* A list of the buttons in the section. */
  GList *buttons;

  /* A hash of the buttons in the section, so you can look a button up by
     name. */
  GHashTable *buttonhash;
};

struct _GladePalette
{
  GtkWindow window;
  
  GtkTooltips *tooltips;
  GtkWidget *vbox;
  GtkWidget *selector;
  GtkWidget *selected_item_label;
  GtkWidget *notebook;

  /* The number of columns of widgets. */
  guint width;

  /* The name of the class of the currently selected widget, or 'Selector'. */
  gchar *selected_widget;

  /* Whether the widget was selected with the 'Ctrl' key, which means that it
     will remain selected until the selector is selected again. */
  gboolean hold_selected_widget;

  /* The radio group of all the widget buttons and the selector. */
  GSList *widget_button_group;

  /* The radio group of the section buttons. */
  GSList *section_button_group;

  /* A hash table of GladePaletteSection's, keyed by section name. */
  GHashTable *sections;

  /* The number of the next notebook page to add. Note that the deprecated
     page is always last. */
  gint next_page_to_add;
};

struct _GladePaletteClass
{
  GtkWindowClass parent_class;

  void   (*select_item)          (GladePalette   *palette,
				  gchar          *item_name);
  void   (*unselect_item)        (GladePalette   *palette,
				  gchar          *item_name);
};


GType	    glade_palette_get_type		(void);
GtkWidget*  glade_palette_new			(void);

/* Sets the number of columns of widgets in the palette. */
void	    glade_palette_set_width		(GladePalette	*palette,
						 guint		 columns);

/* Adds a widget to a section of the palette. */
void	    glade_palette_add_widget		(GladePalette	*palette,
						 const gchar	*section,
						 const gchar	*name,
						 GdkPixmap	*gdkpixmap,
						 GdkBitmap	*mask,
						 const gchar	*tooltip);

/* Removes a widget from the palette. */
void	    glade_palette_remove_widget		(GladePalette	*palette,
						 const gchar	*section,
						 const gchar	*name);

/* Returns TRUE if the selector is currently selected. */
gboolean    glade_palette_is_selector_on	(GladePalette	*palette);

/* Returns the class name of the selected widget. */
gchar*	    glade_palette_get_widget_class	(GladePalette   *palette);

/* Resets the palette selection to the Selector, unless the currently selected
   item was selected with the Ctrl key held down (and allow_hold is TRUE). */
void	    glade_palette_reset_selection	(GladePalette	*palette,
						 gboolean        allow_hold);

/* Shows or hides the GNOME widgets. */
void	    glade_palette_set_show_gnome_widgets(GladePalette	*palette,
						 gboolean	 show_gnome,
						 gboolean	 show_gnome_db);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_PALETTE_H */
