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

void
session_capture_window_attrs (uintptr_t window) {
    GtkWindow *wnd = GTK_WINDOW (window);
    extern int session_win_attrs[5];
    gtk_window_get_position (wnd, &session_win_attrs[0], &session_win_attrs[1]);
    gtk_window_get_size (wnd, &session_win_attrs[2], &session_win_attrs[3]);
    //printf ("attrs: %d %d %d %d\n", session_win_attrs[0], session_win_attrs[1],  session_win_attrs[2], session_win_attrs[3]);
}

void
session_restore_window_attrs (uintptr_t window) {
    GtkWindow *wnd = GTK_WINDOW (window);
    extern int session_win_attrs[5];
    gtk_window_move (wnd, session_win_attrs[0], session_win_attrs[1]);
    gtk_window_resize (wnd, session_win_attrs[2], session_win_attrs[3]);
}
