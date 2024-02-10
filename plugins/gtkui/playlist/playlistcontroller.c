/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <Block.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>
#include "../gtkui.h"
#include <deadbeef/deadbeef.h>
#include "../../artwork/artwork.h"
#include "mainplaylist.h"
#include "playlistcontroller.h"
#include "plcommon.h"
#include "searchplaylist.h"

extern DB_functions_t *deadbeef;

struct playlist_controller_s {
    ddb_artwork_plugin_t *artwork_plugin;
    DdbListview *listview;
    gboolean is_search;
    ddb_listview_datasource_t datasource;
    ddb_listview_renderer_t renderer;
    ddb_listview_delegate_t delegate;
};

static gboolean
_dispatch_on_main_wrapper (void *context) {
    void (^block)(void) = context;
    block ();
    Block_release(block);
    return FALSE;
}

static void
_dispatch_on_main(void (^block)(void)) {
    dispatch_block_t copy_block = Block_copy(block);
    g_idle_add(_dispatch_on_main_wrapper, copy_block);
}

static void
_artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    _dispatch_on_main(^{
        playlist_controller_t *ctl = user_data;
        ddb_listview_reset_artwork (ctl->listview);
    });
}

playlist_controller_t *
playlist_controller_new(DdbListview *listview, gboolean is_search) {
    playlist_controller_t *ctl = calloc (1, sizeof (playlist_controller_t));
    ctl->is_search = is_search;

    ctl->artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id("artwork2");

    if (ctl->artwork_plugin != NULL) {
        ctl->artwork_plugin->add_listener(_artwork_listener, ctl);
    }

    g_object_ref_sink(listview);

    ctl->listview = listview;
    ctl->listview->delegate = &ctl->delegate;
    ctl->listview->datasource = &ctl->datasource;
    ctl->listview->renderer = &ctl->renderer;

    if (is_search) {
        search_init_listview_api(listview);
    }
    else {
        main_init_listview_api(listview);
    }

    return ctl;
}

void
playlist_controller_free(playlist_controller_t *ctl) {
    if (ctl->artwork_plugin) {
        ctl->artwork_plugin->remove_listener(_artwork_listener, ctl);
    }
    g_object_unref(ctl->listview);
    free (ctl);
}

typedef struct {
    DdbListview *listview;
    DB_playItem_t *trk;
} w_trackdata_t;

static w_trackdata_t *
playlist_trackdata (DdbListview *listview, DB_playItem_t *track) {
    w_trackdata_t *td = malloc (sizeof (w_trackdata_t));
    td->listview = listview;
    g_object_ref(listview);
    td->trk = track;
    deadbeef->pl_item_ref(track);
    return td;
}

static gboolean
trackinfochanged_cb (gpointer data) {
    w_trackdata_t *d = data;
    int idx = deadbeef->pl_get_idx_of (d->trk);
    if (idx != -1) {
        ddb_listview_draw_row (d->listview, idx, d->trk);
    }
    g_object_unref(d->listview);
    deadbeef->pl_item_unref (d->trk);
    free (d);
    return FALSE;
}

static gboolean
paused_cb (gpointer data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        int idx = deadbeef->pl_get_idx_of (it);
        if (idx != -1) {
            ddb_listview_draw_row (DDB_LISTVIEW(data), idx, it);
        }
        deadbeef->pl_item_unref (it);
    }
    g_object_unref(DDB_LISTVIEW(data));
    return FALSE;
}

static gboolean
playlist_config_changed_cb (gpointer data) {
    ddb_listview_refresh (DDB_LISTVIEW(data), DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_CONFIG);
    return FALSE;
}

static gboolean
songfinished_cb (gpointer data) {
    w_trackdata_t *d = data;
    int idx = deadbeef->pl_get_idx_of (d->trk);
    if (idx != -1) {
        ddb_listview_draw_row (d->listview, idx, d->trk);
    }
    g_object_unref(d->listview);
    deadbeef->pl_item_unref (d->trk);
    free (data);
    return FALSE;
}

// This only actually does anything if the track is in the current playlist, otherwise gtkui.c will handle it
static gboolean
songstarted_cb (gpointer data) {
    w_trackdata_t *d = data;
    int idx = deadbeef->pl_get_idx_of (d->trk);
    if (idx != -1) {
        if (!gtkui_listview_busy) {
            if (deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)) {
                ddb_listview_select_single (d->listview, idx);
                deadbeef->pl_set_cursor (PL_MAIN, idx);
            }
            if (deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)) {
                ddb_listview_scroll_to (d->listview, idx);
            }
        }
        ddb_listview_draw_row (d->listview, idx, d->trk);
    }
    g_object_unref(d->listview);
    deadbeef->pl_item_unref (d->trk);
    free (data);
    return FALSE;
}

static void
playlist_set_cursor (DdbListview *listview, DB_playItem_t *it) {
    int new_cursor = deadbeef->pl_get_idx_of_iter (it, PL_MAIN);
    if (new_cursor != -1) {
        int cursor = deadbeef->pl_get_cursor (PL_MAIN);
        if (new_cursor != cursor) {
            deadbeef->pl_set_cursor (PL_MAIN, new_cursor);
            ddb_listview_draw_row (listview, new_cursor, NULL);
            if (cursor != -1) {
                ddb_listview_draw_row (listview, cursor, NULL);
            }
        }
        ddb_listview_scroll_to (listview, new_cursor);
    }
}

static gboolean
cursor_moved_cb (gpointer data) {
    w_trackdata_t *d = data;
    playlist_set_cursor (d->listview, d->trk);
    g_object_unref(d->listview);
    deadbeef->pl_item_unref (d->trk);
    free (data);
    return FALSE;
}

static gboolean
focus_selection_cb (gpointer data) {
    DdbListview *listview = data;
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it && !deadbeef->pl_is_selected (it)) {
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    if (it) {
        playlist_set_cursor (listview, it);
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
    return FALSE;
}

// This only actually does anything if the track is in the current playlist
// Otherwise gtkui.c will handle it and send a PLAYLISTSWITCHED message
static gboolean
trackfocus_cb (gpointer data) {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
    if (it) {
        deadbeef->pl_lock ();
        int cursor = deadbeef->pl_get_idx_of (it);
        if (cursor != -1) {
            ddb_listview_select_single (data, cursor);
            deadbeef->pl_set_cursor (PL_MAIN, cursor);
            ddb_listview_scroll_to (data, cursor);
        }
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
    }
    return FALSE;
}

static gboolean
playlist_sort_reset_cb (gpointer data) {
    ddb_listview_col_sort_update (DDB_LISTVIEW(data));
    return FALSE;
}

static gboolean
playlist_list_refresh_cb (gpointer data) {
    ddb_listview_refresh (DDB_LISTVIEW(data), DDB_REFRESH_LIST);
    return FALSE;
}

static gboolean
playlist_header_refresh_cb (gpointer data) {
    ddb_listview_refresh (DDB_LISTVIEW(data), DDB_REFRESH_COLUMNS);
    return FALSE;
}

static gboolean
playlist_setup_cb (gpointer data) {
    DdbListview *listview = DDB_LISTVIEW(data);
    ddb_listview_clear_sort (listview);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int scroll = deadbeef->plt_get_scroll (plt);
        if (!ddb_listview_list_setup(listview, scroll)) {
            deadbeef->plt_unref (plt);
            return TRUE;
        }

        int cursor = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cursor != -1) {
            DB_playItem_t *it = deadbeef->pl_get_for_idx (cursor);
            if (it) {
                deadbeef->pl_set_selected (it, 1);
                deadbeef->pl_item_unref (it);
            }
        }
        deadbeef->plt_unref (plt);

        if (scroll < 0) {
            ddb_listview_scroll_to (listview, scroll * -1);
        }

        ddb_listview_refresh(listview, DDB_REFRESH_LIST);
    }
    return FALSE;
}

void
playlist_controller_init (playlist_controller_t *ctl, gboolean show_headers, int width) {
    ddb_listview_show_header (ctl->listview, show_headers);
    ddb_listview_init_autoresize (ctl->listview, width);
    g_idle_add (playlist_setup_cb, ctl->listview);
}

void
playlist_controller_message (playlist_controller_t *ctl, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    if (ctl->is_search) {
        return; // search has its own message handler
    }

    switch (id) {
    case DB_EV_PAUSED:
        g_object_ref(ctl->listview);
        g_idle_add (paused_cb, ctl->listview);
        break;
    case DB_EV_SONGFINISHED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                g_idle_add (songfinished_cb, playlist_trackdata(ctl->listview, ev->track));
            }
            break;
        }
    case DB_EV_SONGSTARTED:
        {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                g_idle_add (songstarted_cb, playlist_trackdata(ctl->listview, ev->track));
            }
            break;
        }
    case DB_EV_TRACKINFOCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            g_idle_add (playlist_sort_reset_cb, ctl->listview);
        }
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || (p1 == DDB_PLAYLIST_CHANGE_SELECTION && p2 != PL_MAIN) || p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                g_idle_add (trackinfochanged_cb, playlist_trackdata(ctl->listview, ev->track));
            }
        }
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT ||
            (p1 == DDB_PLAYLIST_CHANGE_SELECTION && (p2 != PL_MAIN || (DdbListview *)ctx != ctl->listview)) ||
            p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
            g_idle_add (playlist_list_refresh_cb, ctl->listview);
        }
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (playlist_setup_cb, ctl->listview);
        break;
    case DB_EV_FOCUS_SELECTION:
        g_idle_add (focus_selection_cb, ctl->listview);
        break;
    case DB_EV_TRACKFOCUSCURRENT:
        g_idle_add (trackfocus_cb, ctl->listview);
        break;
    case DB_EV_CURSOR_MOVED:
        if (p1 != PL_MAIN) {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            if (ev->track) {
                g_idle_add (cursor_moved_cb, playlist_trackdata(ctl->listview, ev->track));
            }
        }
        break;
    case DB_EV_CONFIGCHANGED:
        if (ctx) {
            char *conf_str = (char *)ctx;
            if (gtkui_listview_override_conf(conf_str) || gtkui_listview_font_conf(conf_str)) {
                g_idle_add (playlist_config_changed_cb, ctl->listview);
            }
            else if (gtkui_listview_colors_conf(conf_str)) {
                g_idle_add (playlist_list_refresh_cb, ctl->listview);
                g_idle_add (playlist_header_refresh_cb, ctl->listview);
            }
            else if (gtkui_listview_font_style_conf(conf_str) || !strcmp (conf_str, "playlist.pin.groups") ||
                     !strcmp (conf_str, "playlist.groups.spacing") ) {
                g_idle_add (playlist_list_refresh_cb, ctl->listview);
            }
            else if (gtkui_tabstrip_override_conf(conf_str) || gtkui_tabstrip_colors_conf(conf_str)) {
                g_idle_add (playlist_header_refresh_cb, ctl->listview);
            }
        }
        break;
    }
}
