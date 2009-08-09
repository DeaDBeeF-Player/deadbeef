/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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

GtkWidget *searchwin = NULL;
struct playItem_s *search_current = NULL;
int search_count = 0;

void
search_start (void) {
    if (!searchwin) {
        searchwin = create_searchwin ();
        extern GtkWidget *mainwin;
        gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));
    }
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

    playlist_head[PS_SEARCH] = NULL;
    playlist_tail[PS_SEARCH] = NULL;
    search_count = 0;
    if (*text) {
        for (playItem_t *it = playlist_head[PS_MAIN]; it; it = it->next[PS_MAIN]) {
            for (metaInfo_t *m = it->meta; m; m = m->next) {
                if (strcasestr (m->value, text)) {
                    // add to list
                    it->next[PS_SEARCH] = NULL;
                    if (playlist_tail[PS_SEARCH]) {
                        playlist_tail[PS_SEARCH]->next[PS_SEARCH] = it;
                        playlist_tail[PS_SEARCH] = it;
                    }
                    else {
                        playlist_head[PS_SEARCH] = playlist_tail[PS_SEARCH] = it;
                    }
                    search_count++;
                    break;
                }
            }
        }
    }

    extern gtkplaylist_t search_playlist;
    gtkplaylist_t *ps = &search_playlist;
    gtkps_setup_scrollbar (ps);
    memset (ps->fmtcache, 0, sizeof (int16_t) * 3 * ps_ncolumns * ps->nvisiblerows);
    gtkps_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    gtkps_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
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
    return FALSE;
}


gboolean
on_searchlist_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_searchlist_configure_event          (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    extern void search_playlist_init (GtkWidget *widget);
    search_playlist_init (widget);
    GTKPS_PROLOGUE;
    gtkps_configure (ps);

  return FALSE;
}


gboolean
on_searchlist_expose_event             (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_searchlist_scroll_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  return FALSE;
}

///////////// searchwin scrollbar handlers

void
on_searchscroll_value_changed          (GtkRange        *range,
                                        gpointer         user_data)
{

}


