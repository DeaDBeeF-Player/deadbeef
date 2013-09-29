
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
#ifndef GLADE_GNOME_H
#define GLADE_GNOME_H

/*
 * This file contains general Gnome-related declarations & code.
 */

#ifdef USE_GNOME

#include <gnome.h>

#include "gbwidget.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The domain used for libgnomeui, used when we need to get translated strings
   from there. */
#define GLADE_LIBGNOMEUI_GETTEXT_PACKAGE "libgnomeui-2.0"

/*************************************************************************
 * Common choice properties.
 *************************************************************************/

/* Choices for the GnomeDockItem placement. */
extern const gint GladePlacementValues[];
extern const gint GladePlacementSize;
extern const gchar *GladePlacementSymbols[];

/* Choices for the GnomeDockItem placement. */
extern const gint GladeOrientationValues[];
extern const gint GladeOrientationSize;
extern const gchar *GladeOrientationChoices[];
extern const gchar *GladeOrientationSymbols[];


/*************************************************************************
 * Stock Gnome Menu Items.
 *************************************************************************/

/* These are the stock menu items, and the symbols we use to denote them.
   Note that Gnome doesn't have symbols to identify these so we have to make
   our own up. */
extern GnomeUIInfo GladeStockMenuItemValues[];
extern const gchar *GladeStockMenuItemSymbols[];
extern const gint GladeStockMenuItemSize;
/* This is the index of the 'New' item, which is treated specially. If it
   has children it is assumed to be GNOMEUIINFO_MENU_NEW_SUBTREE. If not,
   it is assumed to be GNOMEUIINFO_MENU_NEW_ITEM, and the label can be set,
   e.g. '_New Project'. */
extern const gint GladeStockMenuItemNew;


/*************************************************************************
 * Common functions for outputting source code.
 *************************************************************************/

/* This outputs the GnomeUIInfo structs for menu items when building a Gnome
   application. */
void	  glade_gnome_start_menu_source      (GtkMenuShell	      *widget,
					      GbWidgetWriteSourceData *data);
void	  glade_gnome_finish_menu_source     (GtkMenuShell	      *widget,
					      GbWidgetWriteSourceData *data);
void	  glade_gnome_write_menu_item_source (GtkMenuItem	      *widget,
					      GbWidgetWriteSourceData *data);


/*************************************************************************
 * Utility functions.
 *************************************************************************/

/* Returns the GnomeApp if the widget is a dock item within a GnomeApp. */
GnomeApp* glade_gnome_is_app_dock_item	     (GtkWidget		      *widget);

/* This sets up defaults menus for GtkMenuBar and GnomeApp widgets. */
void	  glade_gnome_setup_initial_menus    (GtkWidget		      *widget);

/* Tries to translate the text in the gnome-libs domain, and if there is no
   translation try Glade's domain. */
gchar*	  glade_gnome_gettext		     (const gchar	      *text);

void	  gb_bonobo_dock_item_save_packing_properties (GtkWidget *parent,
						       GtkWidget *widget,
						       GbWidgetGetArgData * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* USE_GNOME */

#endif	/* GLADE_GNOME_H */
