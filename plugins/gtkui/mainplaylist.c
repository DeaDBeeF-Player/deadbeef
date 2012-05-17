/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "../../gettext.h"
#include "parser.h"
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

GdkPixbuf *play16_pixbuf;
GdkPixbuf *pause16_pixbuf;
GdkPixbuf *buffering16_pixbuf;


// HACK!!
extern GtkWidget *theme_treeview;

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
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.cursor.%d", deadbeef->plt_get_curr_idx ());
    deadbeef->conf_set_int (conf, cursor);
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
    deadbeef->plt_unref (plt);
    deadbeef->pl_unlock ();
    deadbeef->pl_save_all ();
}

void main_external_drag_n_drop (DdbListviewIter before, char *mem, int length) {
    gtkui_receive_fm_drop ((DB_playItem_t *)before, mem, length);
}

gboolean
playlist_tooltip_handler (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer unused)
{
    GtkWidget *pl = lookup_widget (mainwin, "playlist");
    DB_playItem_t *it = (DB_playItem_t *)ddb_listview_get_iter_from_coord (DDB_LISTVIEW (pl), 0, y);
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
    deadbeef->plt_sort (plt, PL_MAIN, c->id, c->format, sort_order-1);
    deadbeef->plt_unref (plt);
}
void main_handle_doubleclick (DdbListview *listview, DdbListviewIter iter, int idx) {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, idx, 0);
}

void main_selection_changed (DdbListviewIter it, int idx) {
    DdbListview *search = DDB_LISTVIEW (lookup_widget (searchwin, "searchlist"));
    if (idx == -1) {
        ddb_listview_refresh (search, DDB_REFRESH_LIST);
    }
    else {
        ddb_listview_draw_row (search, search_get_idx ((DB_playItem_t *)it), it);
    }
}

void main_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int x, int y, int width, int height) {
    if (group_by_str && group_by_str[0]) {
        char str[1024];
        deadbeef->pl_format_title ((DB_playItem_t *)it, -1, str, sizeof (str), -1, group_by_str);
        int theming = !gtkui_override_listview_colors ();
        if (theming) {
            GdkColor *clr = &gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
            float rgb[] = {clr->red/65535.f, clr->green/65535.f, clr->blue/65535.f};
            draw_set_fg_color (rgb);
        }
        else {
            GdkColor clr;
            gtkui_get_listview_text_color (&clr);
            float rgb[] = {clr.red/65535.f, clr.green/65535.f, clr.blue/65535.f};
            draw_set_fg_color (rgb);
        }
        int ew, eh;
        draw_get_text_extents (str, -1, &ew, &eh);
        draw_text (x + 5, y + height/2 - draw_get_font_size ()/2 - 2, ew+5, 0, str);
        draw_line (x + 5 + ew + 3, y+height/2, x + width, y+height/2);
    }
}
void
main_delete_selected (void) {
    deadbeef->pl_delete_selected ();
    deadbeef->pl_save_all ();
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

int
main_get_group (DdbListviewIter it, char *str, int size) {
    if (!group_by_str || !group_by_str[0]) {
        return -1;
    }
    deadbeef->pl_format_title ((DB_playItem_t *)it, -1, str, size, -1, group_by_str);
    return 0;
}

static int lock_column_config = 0;

void
main_columns_changed (DdbListview *listview) {
    if (!lock_column_config) {
        rewrite_column_config (listview, "playlist");
    }
}

void
main_column_size_changed (DdbListview *listview, int col) {
    const char *title;
    int width;
    int align_right;
    col_info_t *inf;
    int minheight;
    int res = ddb_listview_column_get_info (listview, col, &title, &width, &align_right, &minheight, (void **)&inf);
    if (res == -1) {
        return;
    }
    if (inf->id == DB_COLUMN_ALBUM_ART) {
        coverart_reset_queue ();
    }
}

void main_col_free_user_data (void *data) {
    if (data) {
        col_info_t *inf = data;
        if (inf->format) {
            free (inf->format);
        }
        free (data);
    }
}

void
main_vscroll_changed (int pos) {
    coverart_reset_queue ();
    int curr = deadbeef->plt_get_curr_idx ();
    char conf[100];
    snprintf (conf, sizeof (conf), "playlist.scroll.%d", curr);
    deadbeef->conf_set_int (conf, pos);
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

    .get_group = main_get_group,

    .drag_n_drop = main_drag_n_drop,
    .external_drag_n_drop = main_external_drag_n_drop,

    .draw_column_data = draw_column_data,
    .draw_group_title = main_draw_group_title,

    // columns
    .col_sort = main_col_sort,
    .columns_changed = main_columns_changed,
    .column_size_changed = main_column_size_changed,
    .col_free_user_data = main_col_free_user_data,

    // callbacks
    .handle_doubleclick = main_handle_doubleclick,
    .selection_changed = main_selection_changed,
    .header_context_menu = header_context_menu,
    .list_context_menu = list_context_menu,
    .delete_selected = main_delete_selected,
    .vscroll_changed = main_vscroll_changed,
    .modification_idx = gtkui_get_curr_playlist_mod,
};

void
main_playlist_init (GtkWidget *widget) {
    play16_pixbuf = create_pixbuf ("play_16.png");
    pause16_pixbuf = create_pixbuf ("pause_16.png");
    buffering16_pixbuf = create_pixbuf ("buffering_16.png");

    // make listview widget and bind it to data
    DdbListview *listview = DDB_LISTVIEW(widget);
    main_binding.ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    main_binding.unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    ddb_listview_set_binding (listview, &main_binding);
    lock_column_config = 1;
    DB_conf_item_t *col = deadbeef->conf_find ("playlist.column.", NULL);
    if (!col) {
        // create default set of columns
        add_column_helper (listview, _("Playing"), 50, DB_COLUMN_PLAYING, NULL, 0);
        add_column_helper (listview, _("Artist / Album"), 150, -1, "%a - %b", 0);
        add_column_helper (listview, _("Track No"), 50, -1, "%n", 1);
        add_column_helper (listview, _("Title"), 150, -1, "%t", 0);
        add_column_helper (listview, _("Duration"), 50, -1, "%l", 0);
    }
    else {
        while (col) {
            append_column_from_textdef (listview, col->value);
            col = deadbeef->conf_find ("playlist.column.", col);
        }
    }
    lock_column_config = 0;

    // FIXME: filepath should be in properties dialog, while tooltip should be
    // used to show text that doesn't fit in column width
    if (deadbeef->conf_get_int ("listview.showpathtooltip", 0)) {
        GValue value = {0, };
        g_value_init (&value, G_TYPE_BOOLEAN);
        g_value_set_boolean (&value, TRUE);
        g_object_set_property (G_OBJECT (widget), "has-tooltip", &value);
        g_signal_connect (G_OBJECT (widget), "query-tooltip", G_CALLBACK (playlist_tooltip_handler), NULL);
    }
    deadbeef->conf_lock ();
    strncpy (group_by_str, deadbeef->conf_get_str_fast ("playlist.group_by", ""), sizeof (group_by_str));
    deadbeef->conf_unlock ();
    group_by_str[sizeof (group_by_str)-1] = 0;
}

void
main_playlist_free (void) {
    g_object_unref (play16_pixbuf);
    g_object_unref (pause16_pixbuf);
    g_object_unref (buffering16_pixbuf);
}

void
main_refresh (void) {
    if (mainwin && gtk_widget_get_visible (mainwin)) {
        DdbListview *pl = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
        ddb_listview_refresh (pl, DDB_REFRESH_VSCROLL | DDB_REFRESH_LIST);
    }
}

