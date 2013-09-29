
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

#ifndef GLADE_TREE_H
#define GLADE_TREE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern GtkWidget *win_tree;

/* This creates the widget tree and window, but doesn't show it. It needs
   to be called before any widgets are added to the interface, so they can
   be added to the tree. */
void	      tree_init				(void);

/* These show or hide the widget tree window. */
void          tree_show				(GtkWidget	*widget,
						 gpointer	 data);
gint          tree_hide				(GtkWidget	*widget,
						 gpointer	 data);

/* These are for adding/selecting/renaming/removing widgets. The widget must
   already have been added to the interface before calling tree_add_widget(),
   since we need to be able to determine its parent so we know where to add it
   in th widget tree. */
void	      tree_add_widget			(GtkWidget	*widget);
void          tree_select_widget		(GtkWidget	*widget,
						 gboolean	 select);
void          tree_rename_widget		(GtkWidget	*widget,
						 const gchar	*name);
void          tree_remove_widget		(GtkWidget	*widget);

/* These are for inserting widgets into the existing tree, and
   are used when adding/removing event boxes or alignment widgets. */
void          tree_insert_widget_parent		(GtkWidget	*parent,
						 GtkWidget	*widget);

/* This clears the entire widget tree, and is used when another project is
   about to be created or opened. */
void	      tree_clear			(void);

/* These are called when making major changes to the widget tree (i.e. when
   loading an entire interface), to make the updates more efficient. */
void	      tree_freeze			(void);
void	      tree_thaw				(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_TREE_H */
