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
#ifndef GLADE_PROJECT_WINDOW_H
#define GLADE_PROJECT_WINDOW_H

#include "glade_project_view.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * GladeProjectWindow is the main project window, containing a mennubar,
 * toolbar, statusbar and a GladeProjectView to show the project components.
 * It is not a widget, it just creates a window. I did it this way because
 * for Gnome we need to use a GnomeApp instead of a GtkWindow, and I didn't
 * want to use conditional code to subsclass the different widgets.
 */

typedef struct _GladeProjectWindow       GladeProjectWindow;

struct _GladeProjectWindow
{
  /* The main GtkWindow/GnomeApp. */
  GtkWidget *window;

  /* The GladeProjectView, showing the components in the project. */
  GtkWidget *project_view;

  /* The statusbar, for status messages, e.g. 'Project Saved'. */
  GtkWidget *statusbar;

  /* The current directory, for opening projects. */
  gchar *current_directory;
};


GladeProjectWindow* glade_project_window_new	(void);

void	    glade_project_window_open_project	(GladeProjectWindow *project_window,
						 const gchar        *filename);

void	    glade_project_window_set_project	(GladeProjectWindow *project_window,
						 GladeProject       *project);


void	    glade_project_window_refresh_menu_items		(void);

void        glade_project_window_uncheck_palette_menu_item	(void);

void        glade_project_window_uncheck_property_editor_menu_item	(void);

void        glade_project_window_uncheck_widget_tree_menu_item	(void);

void        glade_project_window_uncheck_clipboard_menu_item	(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_PROJECT_WINDOW_H */
