/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include <stdlib.h>
#include <string.h>
#include "gtkui.h"
#include "widgets.h"
#include "ddbtabstrip.h"
#include "ddblistview.h"
#include "mainplaylist.h"

typedef struct {
    ddb_gtkui_widget_t base;
} w_splitter_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_box_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_tabstrip_t;

typedef struct {
    ddb_gtkui_widget_t base;
    DdbTabStrip *tabstrip;
    DdbListview *list;
} w_tabbed_playlist_t;

typedef struct {
    ddb_gtkui_widget_t base;
} w_playlist_t;


void
w_append (ddb_gtkui_widget_t *cont, ddb_gtkui_widget_t *child) {
    child->next = cont->children;
    cont->children = child;
}

ddb_gtkui_widget_t *
w_hsplitter_create (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->base.widget = gtk_hpaned_new ();
    return (ddb_gtkui_widget_t*)w;
}

ddb_gtkui_widget_t *
w_create_vsplitter (void) {
    w_splitter_t *w = malloc (sizeof (w_splitter_t));
    memset (w, 0, sizeof (w_splitter_t));
    w->base.widget = gtk_vpaned_new ();
    return (ddb_gtkui_widget_t*)w;
}

ddb_gtkui_widget_t *
w_create_box (void) {
    w_box_t *w = malloc (sizeof (w_box_t));
    memset (w, 0, sizeof (w_box_t));
    w->base.widget = gtk_vbox_new (FALSE, 0);
    return (ddb_gtkui_widget_t*)w;
}

ddb_gtkui_widget_t *
w_create_tabstrip (void) {
    w_tabstrip_t *w = malloc (sizeof (w_tabstrip_t));
    memset (w, 0, sizeof (w_tabstrip_t));
    w->base.widget = ddb_tabstrip_new ();
    return (ddb_gtkui_widget_t*)w;
}

typedef struct {
    ddb_gtkui_widget_t *w;
    DB_playItem_t *trk;
} w_trackdata_t;

static gboolean
trackinfochanged_cb (gpointer p) {
    w_trackdata_t *d = p;
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)d->w;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int idx = deadbeef->plt_get_item_idx (plt, (DB_playItem_t *)d->trk, PL_MAIN);
        if (idx != -1) {
            ddb_listview_draw_row (tp->list, idx, (DdbListviewIter)d->trk);
        }
        deadbeef->plt_unref (plt);
    }
    deadbeef->pl_item_unref (d->trk);
    free (d);
    return FALSE;
}

static gboolean
paused_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
    if (curr) {
        int idx = deadbeef->pl_get_idx_of (curr);
        ddb_listview_draw_row (tp->list, idx, (DdbListviewIter)curr);
        deadbeef->pl_item_unref (curr);
    }
    return FALSE;
}

static gboolean
refresh_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    ddb_listview_clear_sort (tp->list);
    ddb_listview_refresh (tp->list, DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    return FALSE;
}


static gboolean
playlistswitch_cb (gpointer p) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)p;
    int curr = deadbeef->plt_get_curr_idx ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    int scroll = deadbeef->conf_get_int (conf, 0);
    snprintf (conf, sizeof (conf), "playlist.cursor.%d", curr);
    int cursor = deadbeef->conf_get_int (conf, -1);
    ddb_tabstrip_refresh (tp->tabstrip);
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
    if (cursor != -1) {
        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
        if (it) {
            deadbeef->pl_set_selected (it, 1);
            deadbeef->pl_item_unref (it);
        }
    }

    ddb_listview_refresh (tp->list, DDB_LIST_CHANGED | DDB_REFRESH_LIST | DDB_REFRESH_VSCROLL);
    ddb_listview_set_vscroll (tp->list, scroll);
    return FALSE;
}

struct fromto_t {
    ddb_gtkui_widget_t *w;
    DB_playItem_t *from;
    DB_playItem_t *to;
};

static gboolean
songchanged_cb (gpointer p) {
    struct fromto_t *ft = p;
    DB_playItem_t *from = ft->from;
    DB_playItem_t *to = ft->to;
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)ft->w;
    int to_idx = -1;
    if (!ddb_listview_is_scrolling (tp->list) && to) {
        int cursor_follows_playback = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 0);
        int scroll_follows_playback = deadbeef->conf_get_int ("playlist.scroll.followplayback", 0);
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != -1) {
            if (cursor_follows_playback && plt != deadbeef->plt_get_curr_idx ()) {
                deadbeef->plt_set_curr_idx (plt);
            }
            to_idx = deadbeef->pl_get_idx_of (to);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    ddb_listview_set_cursor_noscroll (tp->list, to_idx);
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr_idx ()) {
                    ddb_listview_scroll_to (tp->list, to_idx);
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            ddb_listview_draw_row (tp->list, idx, from);
        }
    }
    if (to && to_idx != -1) {
        ddb_listview_draw_row (tp->list, to_idx, to);
    }
    if (ft->from) {
        deadbeef->pl_item_unref (ft->from);
    }
    if (ft->to) {
        deadbeef->pl_item_unref (ft->to);
    }
    free (ft);
    return FALSE;
}

static int
w_tabbed_playlist_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)w;
    switch (id) {
    case DB_EV_SONGCHANGED:
        g_idle_add (redraw_queued_tracks_cb, tp->list);
        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
        struct fromto_t *ft = malloc (sizeof (struct fromto_t));
        ft->from = ev->from;
        ft->to = ev->to;
        if (ft->from) {
            deadbeef->pl_item_ref (ft->from);
        }
        if (ft->to) {
            deadbeef->pl_item_ref (ft->to);
        }
        ft->w = w;
        g_idle_add (songchanged_cb, ft);
        break;
    case DB_EV_TRACKINFOCHANGED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                deadbeef->pl_item_ref (ev->track);
            }
            w_trackdata_t *d = malloc (sizeof (w_trackdata_t));
            memset (d, 0, sizeof (w_trackdata_t));
            d->w = w;
            d->trk = ev->track;
            g_idle_add (trackinfochanged_cb, d);
        }
        break;
    case DB_EV_PAUSED:
        g_idle_add (paused_cb, w);
        break;
    case DB_EV_PLAYLISTCHANGED:
        g_idle_add (refresh_cb, w);
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlistswitch_cb, w);
        break;
    }
    return 0;
}

static void
w_tabbed_playlist_destroy (ddb_gtkui_widget_t *w) {
    w_tabbed_playlist_t *tp = (w_tabbed_playlist_t *)w;
    gtk_widget_destroy (tp->base.widget);
    free (tp);
}

ddb_gtkui_widget_t *
w_tabbed_playlist_create (void) {
    w_tabbed_playlist_t *w = malloc (sizeof (w_tabbed_playlist_t));
    memset (w, 0, sizeof (w_tabbed_playlist_t));

    w->base.widget = gtk_vbox_new (FALSE, 0);

    GtkWidget *tabstrip = ddb_tabstrip_new ();
    w->tabstrip = (DdbTabStrip *)tabstrip;
    gtk_widget_show (tabstrip);
    GtkWidget *list = ddb_listview_new ();
    w->list = (DdbListview *)list;
    gtk_widget_show (list);
    GtkWidget *frame = gtk_frame_new (NULL);
    gtk_widget_show (frame);

    gtk_box_pack_start (GTK_BOX (w->base.widget), tabstrip, FALSE, TRUE, 0);
    gtk_widget_set_size_request (tabstrip, -1, 24);
    GTK_WIDGET_UNSET_FLAGS (tabstrip, GTK_CAN_FOCUS);
    GTK_WIDGET_UNSET_FLAGS (tabstrip, GTK_CAN_DEFAULT);

    gtk_box_pack_start (GTK_BOX (w->base.widget), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 1);

    gtk_container_add (GTK_CONTAINER (frame), list);
//    GTK_WIDGET_SET_FLAGS (list, GTK_CAN_FOCUS);
//    GTK_WIDGET_SET_FLAGS (list, GTK_CAN_DEFAULT);
    printf ("can_focus: %d\n", gtk_widget_get_can_focus (list));
    main_playlist_init (list);
    if (deadbeef->conf_get_int ("gtkui.headers.visible", 1)) {
        ddb_listview_show_header (w->list, 1);
    }
    else {
        ddb_listview_show_header (w->list, 0);
    }

    w->base.message = w_tabbed_playlist_message;
    w->base.destroy = w_tabbed_playlist_destroy;
    return (ddb_gtkui_widget_t*)w;
}

ddb_gtkui_widget_t *
w_create_playlist (void) {
    w_playlist_t *w = malloc (sizeof (w_playlist_t));
    memset (w, 0, sizeof (w_playlist_t));
    w->base.widget = ddb_listview_new ();
    return (ddb_gtkui_widget_t*)w;
}
