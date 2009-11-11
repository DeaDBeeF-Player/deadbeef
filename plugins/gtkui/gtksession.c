/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <stdio.h>
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "session.h"
#include "conf.h"

void
session_capture_window_attrs (uintptr_t window) {
    GtkWindow *wnd = GTK_WINDOW (window);
    int win_attrs[4];
    gtk_window_get_position (wnd, &win_attrs[0], &win_attrs[1]);
    gtk_window_get_size (wnd, &win_attrs[2], &win_attrs[3]);
    conf_set_int ("mainwin.geometry.x", win_attrs[0]);
    conf_set_int ("mainwin.geometry.y", win_attrs[1]);
    conf_set_int ("mainwin.geometry.w", win_attrs[2]);
    conf_set_int ("mainwin.geometry.h", win_attrs[3]);
}

void
session_restore_window_attrs (uintptr_t window) {
    GtkWindow *wnd = GTK_WINDOW (window);
    gtk_window_move (wnd, conf_get_int ("mainwin.geometry.x", 40), conf_get_int ("mainwin.geometry.y", 40));
    gtk_window_resize (wnd, conf_get_int ("mainwin.geometry.w", 500), conf_get_int ("mainwin.geometry.h", 300));
}
