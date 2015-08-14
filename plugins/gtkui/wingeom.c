/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include "wingeom.h"
#include "../../deadbeef.h"
#include "gtkui.h"
#include "support.h"

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
