/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#include <stdlib.h>
#include <string.h>
#include "../../../gettext.h"
#include "../../../shared/parser.h"
#include "../drawing.h"
#include "../gtkui.h"
#include "../interface.h"
#include "../plmenu.h"
#include "../search.h"
#include "../support.h"
#include "../trkproperties.h"
#include "../undointegration.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "playlistrenderer.h"
#include "plcommon.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

static int
main_get_count (void) {
    return deadbeef->pl_getcount (PL_MAIN);
}

static int
main_get_sel_count (void) {
    return deadbeef->pl_getselcount ();
}

static int
main_get_cursor (void) {
    return deadbeef->pl_get_cursor (PL_MAIN);
}

static void
main_set_cursor (int cursor) {
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
    DB_playItem_t *it = deadbeef->pl_get_for_idx (cursor);
    if (it) {
        ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_CURSOR_MOVED);
        event->track = it;
        deadbeef->event_send ((ddb_event_t *)event, PL_MAIN, 0);
    }
}

static DdbListviewIter main_head (void) {
    return (DdbListviewIter)deadbeef->pl_get_first (PL_MAIN);
}

static DdbListviewIter main_tail (void) {
    return (DdbListviewIter)deadbeef->pl_get_last(PL_MAIN);
}

static DdbListviewIter main_next (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_next(it, PL_MAIN);
}

static DdbListviewIter main_prev (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_prev(it, PL_MAIN);
}

void
main_drag_n_drop (DdbListviewIter before, DdbPlaylistHandle from_playlist, uint32_t *indices, int length, int copy) {
    deadbeef->pl_lock ();
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (copy) {
        deadbeef->plt_copy_items (plt, PL_MAIN, (ddb_playlist_t *)from_playlist, (DB_playItem_t *)before, indices, length);
    }
    else {
        deadbeef->plt_move_items (plt, PL_MAIN, (ddb_playlist_t *)from_playlist, (DB_playItem_t *)before, indices, length);
    }
    if (!copy && from_playlist != plt) {
        deadbeef->plt_save_config (from_playlist);
    }
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->pl_unlock ();
    ddb_undo->set_action_name (_("Add Files"));
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
main_tracks_copy_drag_n_drop (DdbListviewIter before, DdbListviewIter *tracks, int count) {
    deadbeef->pl_lock ();
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();

    ddb_playItem_t *prev = NULL;
    if (before != NULL) {
        prev = deadbeef->pl_get_prev(before, PL_MAIN);
    }
    else {
        prev = deadbeef->plt_get_last (plt, PL_MAIN);
    }

    for (int i = 0; i < count; i++) {
        ddb_playItem_t *it = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy (it, tracks[i]);
        deadbeef->plt_insert_item(plt, prev, it);
        if (prev) {
            deadbeef->pl_item_unref(prev);
        }
        prev = it;
    }
    if (prev) {
        deadbeef->pl_item_unref(prev);
    }
    deadbeef->plt_unref (plt);
    deadbeef->pl_unlock ();
    ddb_undo->set_action_name (_("Add Files"));
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}


void main_external_drag_n_drop (DdbListviewIter before, char *mem, int length) {
    gtkui_receive_fm_drop ((DB_playItem_t *)before, mem, length);
}

// columns

static void
main_col_sort (DdbListviewColumnSortOrder sort_order, void *user_data) {
    if (sort_order) {
        pl_common_col_sort (sort_order, PL_MAIN, user_data);
    }
}

static void main_handle_doubleclick (DdbListview *listview, DdbListviewIter iter, int idx) {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, idx, 0);
}

static void main_selection_changed (DdbListview *ps, DdbListviewIter it, int idx) {
    pl_common_selection_changed (ps, PL_MAIN, it);
}

static void
main_groups_changed (const char* format) {
    deadbeef->conf_set_str ("gtkui.playlist.group_by_tf", format);
}

static void
main_columns_changed_before_loaded (DdbListview *listview) {
}

static void
main_columns_changed (DdbListview *listview) {
    pl_common_rewrite_column_config (listview, "gtkui.columns.playlist");
}

static void
main_vscroll_changed (int pos) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_set_scroll (plt, pos);
        deadbeef->plt_unref (plt);
    }
}

void
main_init_listview_api (DdbListview *listview) {
    listview->datasource->count = main_get_count;
    listview->datasource->sel_count = main_get_sel_count;
    listview->datasource->cursor = main_get_cursor;
    listview->datasource->set_cursor = main_set_cursor;
    listview->datasource->head = main_head;
    listview->datasource->tail = main_tail;
    listview->datasource->next = main_next;
    listview->datasource->prev = main_prev;
    listview->datasource->is_album_art_column = pl_common_is_album_art_column;
    listview->datasource->modification_idx = gtkui_get_curr_playlist_mod;
    listview->datasource->get_group_text = pl_common_get_group_text;
    listview->datasource->ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    listview->datasource->unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    listview->datasource->is_selected = (int (*) (DdbListviewIter))deadbeef->pl_is_selected;
    listview->datasource->select = (void (*) (DdbListviewIter, int))deadbeef->pl_set_selected;
    listview->datasource->get_for_idx = (DdbListviewIter)deadbeef->pl_get_for_idx;
    listview->datasource->get_idx = (int (*) (DdbListviewIter))deadbeef->pl_get_idx_of;

    listview->renderer->draw_column_data = main_draw_column_data;
    listview->renderer->draw_album_art = pl_common_draw_album_art;
    listview->renderer->draw_group_title = main_draw_group_title;
    listview->delegate->groups_changed = main_groups_changed;
    listview->delegate->drag_n_drop = main_drag_n_drop;
    listview->delegate->external_drag_n_drop = main_external_drag_n_drop;
    listview->delegate->tracks_copy_drag_n_drop = main_tracks_copy_drag_n_drop;
    listview->delegate->col_sort = main_col_sort;
    listview->delegate->columns_changed = main_columns_changed_before_loaded;
    listview->delegate->col_free_user_data = pl_common_free_col_info;
    listview->delegate->handle_doubleclick = main_handle_doubleclick;
    listview->delegate->list_handle_keypress = list_handle_keypress;
    listview->delegate->selection_changed = main_selection_changed;
    listview->delegate->header_context_menu = pl_common_header_context_menu;
    listview->delegate->list_context_menu = list_context_menu;
    listview->delegate->vscroll_changed = main_vscroll_changed;
    listview->delegate->columns_changed = main_columns_changed;

    ddb_listview_set_artwork_subgroup_level(listview, deadbeef->conf_get_int ("gtkui.playlist.group_artwork_level", 0));
    ddb_listview_set_subgroup_title_padding(listview, deadbeef->conf_get_int ("gtkui.playlist.subgroup_title_padding", 10));
    deadbeef->conf_lock();
    char *format = strdup(deadbeef->conf_get_str_fast ("gtkui.playlist.group_by_tf", ""));
    deadbeef->conf_unlock();
    pl_common_set_group_format (listview, format);
    free (format);

    // create default set of columns
    if (pl_common_load_column_config (listview, "gtkui.columns.playlist") < 0) {
        pl_common_add_column_helper (listview, "♫", 50, DB_COLUMN_PLAYING, "%playstatus%", NULL, 0);
        pl_common_add_column_helper (listview, _("Artist / Album"), 150, DB_COLUMN_STANDARD, COLUMN_FORMAT_ARTISTALBUM, NULL, 0);
        pl_common_add_column_helper (listview, _("Track No"), 50, DB_COLUMN_STANDARD, COLUMN_FORMAT_TRACKNUMBER, NULL, 1);
        pl_common_add_column_helper (listview, _("Title"), 150, DB_COLUMN_STANDARD, COLUMN_FORMAT_TITLE, NULL, 0);
        pl_common_add_column_helper (listview, _("Duration"), 50, DB_COLUMN_STANDARD, COLUMN_FORMAT_LENGTH, NULL, 0);
    }
}
