/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

#include "common.h"
#include "search.h"
#include "gtkplaylist.h"
#include "messagepump.h"
#include "messages.h"

extern GtkWidget *searchwin;
struct playItem_s *search_current = NULL;
int search_count = 0;

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

    playlist_head[PL_SEARCH] = NULL;
    playlist_tail[PL_SEARCH] = NULL;
    search_count = 0;
    if (*text) {
        for (playItem_t *it = playlist_head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
            for (metaInfo_t *m = it->meta; m; m = m->next) {
                if (strcasestr (m->value, text)) {
                    // add to list
                    it->next[PL_SEARCH] = NULL;
                    if (playlist_tail[PL_SEARCH]) {
                        playlist_tail[PL_SEARCH]->next[PL_SEARCH] = it;
                        playlist_tail[PL_SEARCH] = it;
                    }
                    else {
                        playlist_head[PL_SEARCH] = playlist_tail[PL_SEARCH] = it;
                    }
                    search_count++;
                    break;
                }
            }
        }
    }

    extern gtkplaylist_t search_playlist;
    gtkplaylist_t *ps = &search_playlist;
    gtkpl_setup_scrollbar (ps);
    memset (ps->fmtcache, 0, sizeof (int16_t) * 3 * pl_ncolumns * ps->nvisiblerows);
    gtkpl_draw_playlist (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
    gtkpl_expose (ps, 0, 0, ps->playlist->allocation.width, ps->playlist->allocation.height);
}

void
search_refresh (void) {
    if (searchwin) {
        on_searchentry_changed (GTK_EDITABLE (lookup_widget (searchwin, "searchentry")), NULL);
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
        if (search_count > 0) {
            playItem_t *it = gtkpl_get_for_idx (ps, max (ps->row, 0));
            if (it) {
                messagepump_push (M_PLAYSONGNUM, 0, pl_get_idx_of (it), 0);
            }
        }
    }
    return FALSE;
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


