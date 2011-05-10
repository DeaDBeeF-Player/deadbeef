/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../gettext.h"

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "search.h"
#include "ddblistview.h"
#include "plcommon.h"
#include "../../deadbeef.h"
#include "mainplaylist.h"

#include "gtkui.h"

#include "wingeom.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

extern DB_functions_t *deadbeef; // defined in gtkui.c
//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

extern GtkWidget *searchwin;
extern GtkWidget *mainwin;

void
search_start (void) {
    wingeom_restore (searchwin, "searchwin", -1, -1, 450, 150, 0);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (searchwin, "searchentry")), "");
    gtk_widget_show (searchwin);
    gtk_window_present (GTK_WINDOW (searchwin));
    search_refresh ();
    main_refresh ();
}

void
search_process (const char *text) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_search_process (plt, text);
    deadbeef->plt_unref (plt);

    int row = deadbeef->pl_get_cursor (PL_SEARCH);
    if (row >= deadbeef->pl_getcount (PL_SEARCH)) {
        deadbeef->pl_set_cursor (PL_SEARCH, deadbeef->pl_getcount (PL_SEARCH) - 1);
    }
}

void
on_searchentry_changed                 (GtkEditable     *editable,
                                        gpointer         user_data)
{
    search_refresh ();
    main_refresh ();
}

void
search_refresh (void) {
    if (searchwin && gtk_widget_get_visible (searchwin)) {
        GtkEntry *entry = GTK_ENTRY (lookup_widget (searchwin, "searchentry"));
        const gchar *text = gtk_entry_get_text (entry);
        search_process (text);
        GtkWidget *pl = lookup_widget (searchwin, "searchlist");
        ddb_listview_refresh (DDB_LISTVIEW (pl), DDB_REFRESH_VSCROLL | DDB_REFRESH_LIST | DDB_LIST_CHANGED);
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
        if (deadbeef->pl_getcount (PL_SEARCH) > 0) {
            int row = deadbeef->pl_get_cursor (PL_SEARCH);
            DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (max (row, 0), PL_SEARCH);
            if (it) {
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of (it), 0);
                deadbeef->pl_item_unref (it);
            }
        }
    }
    else if (event->keyval != GDK_Delete && event->keyval != GDK_Home && event->keyval != GDK_End){
        GtkWidget *pl = lookup_widget (searchwin, "searchlist");
        if (!ddb_listview_handle_keypress (DDB_LISTVIEW (pl), event->keyval, event->state)) {
            return FALSE;
        }
    }
    else {
        return FALSE;
    }
    return TRUE;
}

gboolean
on_searchwin_configure_event           (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "searchwin");
    return FALSE;
}

gboolean
on_searchwin_window_state_event        (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "searchwin");
    return FALSE;
}

static int
search_get_count (void) {
    return deadbeef->pl_getcount (PL_SEARCH);
}

static int
search_get_sel_count (void) {
    int cnt = 0;
    DB_playItem_t *it = deadbeef->pl_get_first (PL_SEARCH);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            cnt++;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_SEARCH);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    return cnt;
}

static int
search_get_cursor (void) {
    return deadbeef->pl_get_cursor (PL_SEARCH);
}

static void
search_set_cursor (int cursor) {
    return deadbeef->pl_set_cursor (PL_SEARCH, cursor);
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

int search_get_idx (DdbListviewIter it) {
    DB_playItem_t *c = deadbeef->pl_get_first (PL_SEARCH);
    int idx = 0;
    while (c && c != it) {
        DB_playItem_t *next = deadbeef->pl_get_next (c, PL_SEARCH); 
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

int
search_is_selected (DdbListviewIter it) {
    return deadbeef->pl_is_selected ((DB_playItem_t *)it);
}

void
search_select (DdbListviewIter it, int sel) {
    deadbeef->pl_set_selected ((DB_playItem_t *)it, sel);
}

int
search_get_group (DdbListviewIter it, char *str, int size) {
    return -1;
}

void
search_col_sort (int col, int sort_order, void *user_data) {
    col_info_t *c = (col_info_t*)user_data;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort (plt, PL_SEARCH, c->id, c->format, sort_order-1);
    deadbeef->plt_unref (plt);
}

static int lock_column_config = 0;

void
search_columns_changed (DdbListview *listview) {
    if (!lock_column_config) {
        rewrite_column_config (listview, "search");
    }
}

void
search_column_size_changed (DdbListview *listview, int col) {
    const char *title;
    int width;
    int align_right;
    col_info_t *inf;
    int minheight;
    int res = ddb_listview_column_get_info (listview, col, &title, &width, &align_right, &minheight, (void **)&inf);
    if (res == -1) {
        return;
    }
}

void search_col_free_user_data (void *data) {
    if (data) {
        free (data);
    }
}

void search_handle_doubleclick (DdbListview *listview, DdbListviewIter iter, int idx) {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, deadbeef->pl_get_idx_of ((DB_playItem_t *)iter), 0);
}

void search_selection_changed (DdbListviewIter it, int idx) {
    DdbListview *main = DDB_LISTVIEW (lookup_widget (mainwin, "playlist"));
    if (idx == -1) {
        ddb_listview_refresh (main, DDB_REFRESH_LIST);
    }
    else {
        ddb_listview_draw_row (main, main_get_idx ((DB_playItem_t *)it), it);
    }
}

void
search_delete_selected (void) {
    deadbeef->pl_delete_selected ();
    main_refresh ();
    search_refresh ();
}

DdbListviewBinding search_binding = {
    // rows
    .count = search_get_count,
    .sel_count = search_get_sel_count,

    .cursor = search_get_cursor,
    .set_cursor = search_set_cursor,

    .head = search_head,
    .tail = search_tail,
    .next = search_next,
    .prev = search_prev,

    .get_for_idx = search_get_for_idx,
    .get_idx = search_get_idx,

    .is_selected = search_is_selected,
    .select = search_select,

    .get_group = search_get_group,

    .drag_n_drop = NULL,
    .external_drag_n_drop = NULL,

    .draw_column_data = draw_column_data,
    .draw_group_title = NULL,

    // columns
    .col_sort = search_col_sort,
    .columns_changed = search_columns_changed,
    .column_size_changed = search_column_size_changed,
    .col_free_user_data = search_col_free_user_data,

    // callbacks
    .handle_doubleclick = search_handle_doubleclick,
    .selection_changed = search_selection_changed,
    .header_context_menu = header_context_menu,
    .list_context_menu = list_context_menu,
    .delete_selected = search_delete_selected,
    .modification_idx = gtkui_get_curr_playlist_mod,
};

void
search_playlist_init (GtkWidget *widget) {
    DdbListview *listview = DDB_LISTVIEW(widget);
    search_binding.ref = (void (*) (DdbListviewIter))deadbeef->pl_item_ref;
    search_binding.unref = (void (*) (DdbListviewIter))deadbeef->pl_item_unref;
    search_binding.is_selected = (int (*) (DdbListviewIter))deadbeef->pl_is_selected;
    ddb_listview_set_binding (listview, &search_binding);
    lock_column_config = 1;
    // create default set of columns
    DB_conf_item_t *col = deadbeef->conf_find ("search.column.", NULL);
    if (!col) {
        add_column_helper (listview, _("Artist / Album"), 150, -1, "%a - %b", 0);
        add_column_helper (listview, _("Track No"), 50, -1, "%n", 1);
        add_column_helper (listview, _("Title"), 150, -1, "%t", 0);
        add_column_helper (listview, _("Duration"), 50, -1, "%l", 0);
    }
    else {
        while (col) {
            append_column_from_textdef (listview, col->value);
            col = deadbeef->conf_find ("search.column.", col);
        }
    }
    lock_column_config = 0;
}

