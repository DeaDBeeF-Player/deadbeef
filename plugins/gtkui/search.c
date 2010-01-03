/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "search.h"
#include "gtkplaylist.h"
#include "deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define PL_HEAD(iter) (deadbeef->pl_get_first(iter))
#define PL_TAIL(iter) (deadbeef->pl_get_last(iter))
#define PL_NEXT(it, iter) (deadbeef->pl_get_next(it, iter))
#define PL_PREV(it, iter) (deadbeef->pl_get_prev(it, iter))
#define SELECTED(it) (deadbeef->pl_is_selected(it))
#define SELECT(it, sel) (deadbeef->pl_set_selected(it,sel))

extern DB_functions_t *deadbeef; // defined in gtkui.c
//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern GtkWidget *searchwin;
struct playItem_s *search_current = NULL;

void
search_restore_attrs (void) {
    int x = deadbeef->conf_get_int ("searchwin.geometry.x", -1);
    int y = deadbeef->conf_get_int ("searchwin.geometry.y", -1);
    int w = deadbeef->conf_get_int ("searchwin.geometry.w", 500);
    int h = deadbeef->conf_get_int ("searchwin.geometry.h", 300);
    gtk_widget_show (searchwin);
    if (x != -1 && y != -1) {
        gtk_window_move (GTK_WINDOW (searchwin), x, y);
        gtk_window_resize (GTK_WINDOW (searchwin), w, h);
        if (deadbeef->conf_get_int ("searchwin.geometry.maximized", 0)) {
            gtk_window_maximize (GTK_WINDOW (searchwin));
        }
        gtk_window_present (GTK_WINDOW (searchwin));
    }
}

void
search_start (void) {
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (searchwin, "searchentry")), "");
    gtk_widget_show (searchwin);
    gtk_window_present (GTK_WINDOW (searchwin));
}

void
on_searchentry_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
    // final implementation must work in separate thread, and catch up when
    // value was changed
    // but for alpha, let's do it in GTK thread
    
    // walk playlist starting with playlist_head, and populate list starting
    // with search_head

    const gchar *text = gtk_entry_get_text (GTK_ENTRY (editable));
    deadbeef->pl_search_process (text);

    extern gtkplaylist_t search_playlist;
    gtkplaylist_t *ps = &search_playlist;
    int row = deadbeef->pl_get_cursor (ps->iterator);
    if (row >= ps->get_count ()) {
        deadbeef->pl_set_cursor (ps->iterator, ps->get_count () - 1);
    }
    gtkpl_setup_scrollbar (ps);
    //memset (ps->fmtcache, 0, sizeof (int16_t) * 3 * pl_ncolumns * ps->nvisiblerows);
    gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);

    // redraw main playlist to be in sync selection-wise
    ps = &main_playlist;
    gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
}

void
search_refresh (void) {
    if (searchwin && GTK_WIDGET_VISIBLE (searchwin)) {
        gtkplaylist_t *ps = &search_playlist;
        gtkpl_setup_scrollbar (ps);
        gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
        gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    }
}

///////// searchwin header handlers

gboolean
on_searchheader_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_searchheader_button_release_event   (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_searchheader_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    return FALSE;
}


gboolean
on_searchheader_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_searchheader_motion_notify_event    (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


///////// searchwin playlist navigation and rendering

gboolean
on_searchwin_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    // that's for when user attempts to navigate list while entry has focus
    if (event->keyval == GDK_Escape) {
        gtk_widget_hide (widget);
    }
    else if (event->keyval == GDK_Return) {
        extern gtkplaylist_t search_playlist;
        gtkplaylist_t *ps = &search_playlist;
        if (deadbeef->pl_getcount (ps->iterator) > 0) {
            int row = deadbeef->pl_get_cursor (ps->iterator);
            DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (max (row, 0), ps->iterator);
            if (it) {
                deadbeef->sendmessage (M_PLAYSONGNUM, 0, deadbeef->pl_get_idx_of (it), 0);
            }
        }
    }
    else if (event->keyval != GDK_Delete && event->keyval != GDK_Home && event->keyval != GDK_End){
        if (!gtkpl_keypress (&search_playlist, event->keyval, event->state)) {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }
    return TRUE;
}



gboolean
on_searchlist_configure_event          (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    GTKPL_PROLOGUE;
    gtkpl_configure (ps);
    return FALSE;
}

gboolean
on_searchwin_configure_event           (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
#if GTK_CHECK_VERSION(2,2,0)
	GdkWindowState window_state = gdk_window_get_state (GDK_WINDOW (widget->window));
#else
	GdkWindowState window_state = gdk_window_get_state (G_OBJECT (widget));
#endif
    if (!(window_state & GDK_WINDOW_STATE_MAXIMIZED) && GTK_WIDGET_VISIBLE (widget)) {
        int x, y;
        int w, h;
        gtk_window_get_position (GTK_WINDOW (widget), &x, &y);
        gtk_window_get_size (GTK_WINDOW (widget), &w, &h);
        deadbeef->conf_set_int ("searchwin.geometry.x", x);
        deadbeef->conf_set_int ("searchwin.geometry.y", y);
        deadbeef->conf_set_int ("searchwin.geometry.w", w);
        deadbeef->conf_set_int ("searchwin.geometry.h", h);
    }
    return FALSE;
}

gboolean
on_searchwin_window_state_event        (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    if (!GTK_WIDGET_VISIBLE (widget)) {
        return FALSE;
    }
    // based on pidgin maximization handler
#if GTK_CHECK_VERSION(2,2,0)
    if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
        if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
            deadbeef->conf_set_int ("searchwin.geometry.maximized", 1);
        }
        else {
            deadbeef->conf_set_int ("searchwin.geometry.maximized", 0);
        }
    }
#else
	GdkWindowState new_window_state = gdk_window_get_state(G_OBJECT(widget));

    if ()
	if (new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        deadbeef->conf_set_int ("searchwin.geometry.maximized", 1);
    }
	else {
        deadbeef->conf_set_int ("searchwin.geometry.maximized", 0);
    }
#endif
    return FALSE;
}

