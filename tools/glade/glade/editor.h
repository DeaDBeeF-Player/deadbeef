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
#ifndef GLADE_EDITOR_H
#define GLADE_EDITOR_H

#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The size of the selection handles in the corners of widgets */
#define GB_CORNER_WIDTH   7
#define GB_CORNER_HEIGHT  7


void	    editor_init				(void);

/* Grid and snap settings */
gboolean    editor_get_show_grid		(void);
void	    editor_set_show_grid		(gboolean            show);
void	    editor_show_grid_settings_dialog	(GtkWidget	    *widget);

gboolean    editor_get_snap_to_grid		(void);
void	    editor_set_snap_to_grid		(gboolean            snap);
void	    editor_show_snap_settings_dialog	(GtkWidget	    *widget);

/* Signal handlers */
gint	    editor_close_window			(GtkWidget	    *widget,
						 GdkEvent	    *event,
						 gpointer	     data);

void	    editor_on_delete			(void);

/* These are from the popup context-sensitive menus */
void	    editor_on_select_activate		(GtkWidget	    *menuitem,
						 GtkWidget	    *widget);
void	    editor_on_cut_activate		(GtkWidget	    *menuitem,
						 GtkWidget	    *widget);
void	    editor_on_copy_activate		(GtkWidget	    *menuitem,
						 GtkWidget	    *widget);
void	    editor_on_paste_activate		(GtkWidget	    *menuitem,
						 GtkWidget	    *widget);
void	    editor_on_delete_activate		(GtkWidget	    *menuitem,
						 GtkWidget	    *widget);

/* General functions */
gboolean    editor_select_widget_control	(GtkWidget	    *widget);
gboolean    editor_select_widget		(GtkWidget	    *widget,
						 GdkEventButton	    *event,
						 gint		     x,
						 gint		     y);
gboolean    editor_is_selected			(GtkWidget	    *widget);
/* This sets the list of selected widgets, possibly NULL. It takes control
   of the GList, so don't free it. */
void	    editor_set_selection		(GList		    *new_selection);
void	    editor_dump_selection		(void);

/* Returns NULL if a widget can be deleted, or an error message. */
gchar*	    editor_can_delete_widget		(GtkWidget	    *widget);

void	    editor_delete_widget		(GtkWidget	    *widget);

GtkWidget*  editor_new_placeholder		(void);

void	    editor_add_key_signals		(GtkWidget	    *widget);
void	    editor_add_mouse_signals		(GtkWidget	    *widget);
void	    editor_add_mouse_signals_to_existing(GtkWidget	    *widget);
void	    editor_add_draw_signals		(GtkWidget	    *widget);

void	    editor_refresh_widget		(GtkWidget	    *widget);
void	    editor_refresh_widget_selection	(GtkWidget	    *widget);
void	    editor_refresh_widget_area		(GtkWidget	    *widget,
						 gint		     x,
						 gint		     y,
						 gint		     w,
						 gint		     h);

GList*	    editor_get_selection		(void);
gint	    editor_clear_selection		(GtkWidget	    *leave_widget);
void	    editor_remove_widget_from_selection	(GtkWidget	    *widget);
void	    editor_deselect_all_placeholders	(void);

/* Called when a GbWidget is destroyed so the editor can remove any references
   to it. */
void	    editor_on_widget_destroyed		(GtkWidget	    *widget);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* GLADE_EDITOR_H */
