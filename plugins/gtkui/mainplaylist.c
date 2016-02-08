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
#include <stdlib.h>
#include <string.h>
#include "../../gettext.h"
#include "../libparser/parser.h"
#include "gtkui.h"
#include "ddblistview.h"
#include "mainplaylist.h"
#include "search.h"
#include "interface.h"
#include "support.h"
#include "drawing.h"
#include "trkproperties.h"
#include "coverart.h"
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
    return deadbeef->pl_set_cursor (PL_MAIN, cursor);
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

static DdbListviewIter main_get_for_idx (int idx) {
    return deadbeef->pl_get_for_idx_and_iter (idx, PL_MAIN);
}

int main_get_idx (DdbListviewIter it) {
    DB_playItem_t *c = deadbeef->pl_get_first (PL_MAIN);
    int idx = 0;
    while (c && c != it) {
        DB_playItem_t *next = deadbeef->pl_get_next (c, PL_MAIN); 
        deadbeef->pl_item_unref (c);
        c = next;
        idx++;
    }
    if (!c) {
        return -1;
    }
    deadbeef->pl_item_unref (c);
    return idx;
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
}

void main_external_drag_n_drop (DdbListviewIter before, char *mem, int length) {
    gtkui_receive_fm_drop ((DB_playItem_t *)before, mem, length);
}

gboolean
playlist_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer unused)
{
    DdbListview *pl = DDB_LISTVIEW (g_object_get_data (G_OBJECT (widget), "owner"));
    DB_playItem_t *it = (DB_playItem_t *)ddb_listview_get_iter_from_coord (pl, 0, y);
    if (it) {
        deadbeef->pl_lock ();
        gtk_tooltip_set_text (tooltip, deadbeef->pl_find_meta (it, ":URI"));
        deadbeef->pl_unlock ();
        deadbeef->pl_item_unref (it);
        return TRUE;
    }
    return FALSE;
}

// columns

void
main_col_sort (int col, int sort_order, void *user_data) {
    col_info_t *c = (col_info_t*)user_data;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, c->id, c->format, sort_order-1);
    deadbeef->plt_unref (plt);
}
void main_handle_doubleclick (DdbListview *listview, DdbListviewIter iter, int idx) {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, idx, 0);
}

void main_selection_changed (DdbListview *ps, DdbListviewIter it, int idx) {
    DdbListview *search = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    if (idx == -1) {
        ddb_listview_refresh (search, DDB_REFRESH_LIST);
    }
    else {
        ddb_listview_draw_row (search, search_get_idx ((DB_playItem_t *)it), it);
    }
    deadbeef->sendmessage (DB_EV_SELCHANGED, (uintptr_t)ps, deadbeef->plt_get_curr_idx (), PL_MAIN);
}

void
main_delete_selected (void) {
    deadbeef->pl_delete_selected ();
    deadbeef->pl_save_current();
    main_refresh ();
    search_refresh ();
}

void
main_select (DdbListviewIter it, int sel) {
    deadbeef->pl_set_selected ((DB_playItem_t *)it, sel);
}

int
main_is_selected (DdbListviewIter it) {
    return deadbeef->pl_is_selected ((DB_playItem_t *)it);
}

void
main_groups_changed (DdbListview *listview, const char* format) {
    if (!format) {
        return;
    }
    if (listview->group_format) {
        free (listview->group_format);
    }
    if (listview->group_title_bytecode) {
        free (listview->group_title_bytecode);
        listview->group_title_bytecode = NULL;
    }
    deadbeef->conf_set_str ("gtkui.playlist.group_by_tf", format);
    listview->group_format = strdup (format);
    listview->group_title_bytecode = deadbeef->tf_compile (listview->group_format);
}

static int lock_column_config = 0;

void
main_columns_changed (DdbListview *listview) {
    if (!lock_column_config) {
        rewrite_column_config (listview, "gtkui.columns.playlist");
    }
}

void main_col_free_user_data (void *data) {
    if (data) {
        col_info_t *inf = data;
        if (inf->format) {
            free (inf->format);
        }
        if (inf->bytecode) {
            free (inf->bytecode);
        }
        free (data);
    }
}

void
main_vscroll_changed (int pos) {
    coverart_reset_queue ();
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_set_scroll (plt, pos);
        deadbeef->plt_unref (plt);
    }
}

void
main_header_context_menu (DdbListview *ps, int column) {
    GtkWidget *menu = create_headermenu (1);
    set_last_playlist_cm (ps); // playlist ptr for context menu
    set_active_column_cm (column);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, ps, 3, gtk_get_current_event_time());
}

DdbListviewBinding main_binding = {
    // rows
    .count = main_get_count,
    .sel_count = main_get_sel_count,

    .cursor = main_get_cursor,
    .set_cursor = main_set_cursor,

    .head = main_head,
    .tail = main_tail,
    .next = main_next,
    .prev = main_prev,

    .get_for_idx = main_get_for_idx,
    .get_idx = main_get_idx,

    .is_selected = main_is_selected,
    .select = main_select,

    .get_group = pl_common_get_group,
    .groups_changed = main_groups_changed,

    .drag_n_drop = main_drag_n_drop,
    .external_drag_n_drop = main_external_drag_n_drop,

    .draw_column_data = draw_column_data,
    .draw_album_art = draw_album_art,
    .draw_group_title = pl_common_draw_group_title,

    // columns
    .col_sort = main_col_sort,
    .columns_changed = main_columns_changed,
    .col_free_user_data = main_col_free_user_data,

    // callbacks
    .handle_doubleclick = main_handle_doubleclick,
    .selection_changed = main_selection_changed,
    .header_context_menu = main_header_context_menu,
    .list_context_menu = list_context_menu,
    .delete_selected = main_delete_selected,
    .vscroll_changed = main_vscroll_changed,
    .modification_idx = gtkui_get_curr_playlist_mod,
};

void
main_playlist_init (GtkWidget *widget) {
    // make listview widget and bind it to data
    DdbListview *listview = DDB_LISTVIEW(widget);
    main_binding.ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    main_binding.unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    ddb_listview_set_binding (listview, &main_binding);
    lock_column_config = 1;
    deadbeef->conf_lock ();
    if (!deadbeef->conf_get_str_fast ("gtkui.columns.playlist", NULL)) {
        import_column_config_0_6 ("playlist.column.", "gtkui.columns.playlist");
    }
    deadbeef->conf_unlock ();
    if (load_column_config (listview, "gtkui.columns.playlist") < 0) {
        // create default set of columns
        add_column_helper (listview, "♫", 50, DB_COLUMN_PLAYING, "%playstatus%", 0);
        add_column_helper (listview, _("Artist / Album"), 150, -1, "%artist% - %album%", 0);
        add_column_helper (listview, _("Track No"), 50, -1, "%tracknumber%", 1);
        add_column_helper (listview, _("Title"), 150, -1, "%title%", 0);
        add_column_helper (listview, _("Duration"), 50, -1, "%length%", 0);
    }
    lock_column_config = 0;

    deadbeef->conf_lock ();
    listview->group_format = strdup (deadbeef->conf_get_str_fast ("gtkui.playlist.group_by_tf", ""));
    deadbeef->conf_unlock ();
    listview->group_title_bytecode = deadbeef->tf_compile (listview->group_format);

    // FIXME: filepath should be in properties dialog, while tooltip should be
    // used to show text that doesn't fit in column width
    if (deadbeef->conf_get_int ("listview.showpathtooltip", 0)) {
        GValue value = {0, };
        g_value_init (&value, G_TYPE_BOOLEAN);
        g_value_set_boolean (&value, TRUE);
        DdbListview *pl = DDB_LISTVIEW (widget);
        g_object_set_property (G_OBJECT (pl->list), "has-tooltip", &value);
        g_signal_connect (G_OBJECT (pl->list), "query-tooltip", G_CALLBACK (playlist_tooltip_handler), NULL);
    }
}

void
main_refresh (void) {
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

