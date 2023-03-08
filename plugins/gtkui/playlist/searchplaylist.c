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

#include <string.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>
#include "../../../gettext.h"
#include "gtkui.h"
#include "plcommon.h"
#include "plmenu.h"
#include "search.h"
#include "searchplaylist.h"

extern DB_functions_t *deadbeef;

static DB_playItem_t *
next_playitem (DB_playItem_t *it) {
    DB_playItem_t *next = deadbeef->pl_get_next (it, PL_SEARCH);
    deadbeef->pl_item_unref (it);
    return next;
}

static int
search_get_sel_count (void) {
    int cnt = 0;
    for (DB_playItem_t *it = deadbeef->pl_get_first (PL_SEARCH); it; it = next_playitem (it)) {
        if (deadbeef->pl_is_selected (it)) {
            cnt++;
        }
    }
    return cnt;
}

static int search_get_count (void) {
    return deadbeef->pl_getcount (PL_SEARCH);
}

static int search_get_cursor (void) {
    return deadbeef->pl_get_cursor (PL_SEARCH);
}

static void search_set_cursor (int cursor) {
    deadbeef->pl_set_cursor (PL_SEARCH, cursor);
    DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_SEARCH);
    if (it) {
        ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_CURSOR_MOVED);
        event->track = it;
        deadbeef->event_send ((ddb_event_t *)event, PL_SEARCH, 0);
    }
}

static DdbListviewIter search_head (void) {
    return (DdbListviewIter)deadbeef->pl_get_first (PL_SEARCH);
}

static DdbListviewIter search_tail (void) {
    return (DdbListviewIter)deadbeef->pl_get_last(PL_SEARCH);
}

static DdbListviewIter search_next (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_next(it, PL_SEARCH);
}

static DdbListviewIter search_prev (DdbListviewIter it) {
    return (DdbListviewIter)deadbeef->pl_get_prev(it, PL_SEARCH);
}

static DdbListviewIter search_get_for_idx (int idx) {
    return deadbeef->pl_get_for_idx_and_iter (idx, PL_SEARCH);
}

static int search_get_idx (DdbListviewIter it) {
    return deadbeef->pl_get_idx_of_iter(it, PL_SEARCH);
}

static void
search_col_sort (DdbListviewColumnSortOrder sort_order, void *user_data) {
    if (sort_order != DdbListviewColumnSortOrderNone) {
        pl_common_col_sort (sort_order, PL_SEARCH, user_data);
    }
    else {
        search_submit_refresh ();
    }
}

static void
search_groups_changed (const char *format) {
    deadbeef->conf_set_str ("gtkui.search.group_by_tf", format);
}

static void
search_columns_changed_before_loaded (DdbListview *listview) {
}

static void
search_columns_changed (DdbListview *listview) {
    pl_common_rewrite_column_config (listview, "gtkui.columns.search");
}

static void
search_handle_doubleclick (DdbListview *listview, DdbListviewIter iter, int idx) {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of ((DB_playItem_t *)iter), 0);
}

static void
search_selection_changed (DdbListview *ps, DdbListviewIter it, int idx) {
    pl_common_selection_changed (ps, PL_SEARCH, it);
}

static void
search_draw_column_data (DdbListview *listview, cairo_t *cr, DdbListviewIter it, int idx, int align, void *user_data, GdkColor *fg_clr, int x, int y, int width, int height, int even)
{
    pl_common_draw_column_data (listview, cr, it, idx, PL_SEARCH, align, user_data, fg_clr, x, y, width, height, even);
}

static void
search_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int x, int y, int width, int height, int group_depth)
{
    pl_common_draw_group_title (listview, drawable, it, PL_SEARCH, x, y, width, height, group_depth);
}

static void
search_list_context_menu (ddb_playlist_t *playlist, int iter) {
    list_context_menu (playlist, PL_SEARCH);
}

static gboolean
search_list_handle_keypress (DdbListview *ps, int keyval, int state, int iter) {
    return list_handle_keypress (ps, keyval, state, PL_SEARCH);
}

void
search_init_listview_api(DdbListview *listview) {
    listview->datasource->count = search_get_count;
    listview->datasource->sel_count = search_get_sel_count;
    listview->datasource->cursor = search_get_cursor;
    listview->datasource->set_cursor = search_set_cursor;
    listview->datasource->head = search_head;
    listview->datasource->tail = search_tail;
    listview->datasource->next = search_next;
    listview->datasource->prev = search_prev;
    listview->datasource->get_for_idx = search_get_for_idx;
    listview->datasource->get_idx = search_get_idx;
    listview->datasource->is_album_art_column = pl_common_is_album_art_column;
    listview->datasource->modification_idx = gtkui_get_curr_playlist_mod;
    listview->datasource->get_group_text = pl_common_get_group_text;
    listview->datasource->ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    listview->datasource->unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    listview->datasource->is_selected = (int (*) (DdbListviewIter))deadbeef->pl_is_selected;
    listview->datasource->select = (void (*) (DdbListviewIter, int))deadbeef->pl_set_selected;

    listview->renderer->draw_column_data = search_draw_column_data;
    listview->renderer->draw_album_art = pl_common_draw_album_art;
    listview->renderer->draw_group_title = search_draw_group_title;

    listview->delegate->groups_changed = search_groups_changed;
    listview->delegate->drag_n_drop = NULL;
    listview->delegate->external_drag_n_drop = NULL;
    listview->delegate->col_sort = search_col_sort;
    listview->delegate->columns_changed = search_columns_changed_before_loaded;
    listview->delegate->col_free_user_data = pl_common_free_col_info;
    listview->delegate->handle_doubleclick = search_handle_doubleclick;
    listview->delegate->list_handle_keypress = search_list_handle_keypress;
    listview->delegate->selection_changed = search_selection_changed;
    listview->delegate->header_context_menu = pl_common_header_context_menu;
    listview->delegate->list_context_menu = search_list_context_menu;
    listview->delegate->columns_changed = search_columns_changed;

    // create default set of columns
    if (pl_common_load_column_config (listview, "gtkui.columns.search") < 0) {
        pl_common_add_column_helper (listview, _("Artist / Album"), 150, DB_COLUMN_STANDARD, COLUMN_FORMAT_ARTISTALBUM, NULL, 0);
        pl_common_add_column_helper (listview, _("Track No"), 50, DB_COLUMN_STANDARD, COLUMN_FORMAT_TRACKNUMBER, NULL, 1);
        pl_common_add_column_helper (listview, _("Title"), 150, DB_COLUMN_STANDARD, COLUMN_FORMAT_TITLE, NULL, 0);
        pl_common_add_column_helper (listview, _("Duration"), 50, DB_COLUMN_STANDARD, COLUMN_FORMAT_LENGTH, NULL, 0);
    }

    ddb_listview_set_artwork_subgroup_level(listview, deadbeef->conf_get_int ("gtkui.search.group_artwork_level", 0));
    ddb_listview_set_subgroup_title_padding(listview, deadbeef->conf_get_int ("gtkui.search.subgroup_title_padding", 10));

    deadbeef->conf_lock();
    char *format = strdup(deadbeef->conf_get_str_fast ("gtkui.search.group_by_tf", ""));
    deadbeef->conf_unlock();
    pl_common_set_group_format (listview, format);
    free (format);
}
