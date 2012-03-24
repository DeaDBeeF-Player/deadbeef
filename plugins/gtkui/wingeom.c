/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include "wingeom.h"
#include "../../deadbeef.h"
#include "gtkui.h"

void
wingeom_save (GtkWidget *widget, const char *name) {
#if GTK_CHECK_VERSION(2,2,0)
	GdkWindowState window_state = gdk_window_get_state (gtk_widget_get_window (widget));
#else
	GdkWindowState window_state = gdk_window_get_state (G_OBJECT (widget));
#endif
    if (!(window_state & GDK_WINDOW_STATE_MAXIMIZED) && gtk_widget_get_visible (widget)) {
        int x, y;
        int w, h;
        char key[100];
        gtk_window_get_position (GTK_WINDOW (widget), &x, &y);
        gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
        snprintf (key, sizeof (key), "%s.geometry.x", name);
        deadbeef->conf_set_int (key, x);
        snprintf (key, sizeof (key), "%s.geometry.y", name);
        deadbeef->conf_set_int (key, y);
        snprintf (key, sizeof (key), "%s.geometry.w", name);
        deadbeef->conf_set_int (key, w);
        snprintf (key, sizeof (key), "%s.geometry.h", name);
        deadbeef->conf_set_int (key, h);
    }
    deadbeef->conf_save ();
}

void
wingeom_save_max (GdkEventWindowState *event, GtkWidget *widget, const char *name) {
    if (!gtk_widget_get_visible (widget)) {
        return;
    }
    char key[100];
    snprintf (key, sizeof (key), "%s.geometry.maximized", name);
    // based on pidgin maximization handler
#if GTK_CHECK_VERSION(2,2,0)
    if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
        if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
            deadbeef->conf_set_int (key, 1);
        }
        else {
            deadbeef->conf_set_int (key, 0);
        }
    }
#else
	GdkWindowState new_window_state = gdk_window_get_state(G_OBJECT(widget));

	if (new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        deadbeef->conf_set_int (key, 1);
    }
	else {
        deadbeef->conf_set_int (key, 0);
    }
#endif
}

void
wingeom_restore (GtkWidget *win, const char *name, int dx, int dy, int dw, int dh, int dmax) {
    char key[100];
    snprintf (key, sizeof (key), "%s.geometry.x", name);
    int x = deadbeef->conf_get_int (key, dx);
    snprintf (key, sizeof (key), "%s.geometry.y", name);
    int y = deadbeef->conf_get_int (key, dy);
    snprintf (key, sizeof (key), "%s.geometry.w", name);
    int w = deadbeef->conf_get_int (key, dw);
    snprintf (key, sizeof (key), "%s.geometry.h", name);
    int h = deadbeef->conf_get_int (key, dh);
    if (x != -1 && y != -1) {
        gtk_window_move (GTK_WINDOW (win), x, y);
    }
    if (w != -1 && h != -1) {
        gtk_window_resize (GTK_WINDOW (win), w, h);
    }
    snprintf (key, sizeof (key), "%s.geometry.maximized", name);
    if (deadbeef->conf_get_int (key, dmax)) {
        gtk_window_maximize (GTK_WINDOW (win));
    }
}
