/*
    Playlist browser widget plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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
#include <deadbeef/deadbeef.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include "../gtkui/gtkui_api.h"
#include "../../gettext.h"
#include "support.h"

DB_functions_t *deadbeef;
static ddb_gtkui_t *gtkui_plugin;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *tree;
    GtkTreeViewColumn *col_playing;
    GtkTreeViewColumn *col_items;
    GtkTreeViewColumn *col_duration;
    int last_selected;
    gulong cc_id;
    gulong ri_id;
} w_pltbrowser_t;

enum
{
    COL_PLAYING,
    COL_NAME,
    COL_ITEMS,
    COL_DURATION,
};

// copied from gtkui.c
static int
add_new_playlist (void) {
    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            strcpy (name, _("New Playlist"));
        }
        else {
            snprintf (name, sizeof (name), _("New Playlist (%d)"), idx);
        }
        deadbeef->pl_lock ();
        for (i = 0; i < cnt; i++) {
            char t[100];
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
            deadbeef->plt_get_title (plt, t, sizeof (t));
            deadbeef->plt_unref (plt);
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        deadbeef->pl_unlock ();
        if (i == cnt) {
            return deadbeef->plt_add (cnt, name);
        }
        idx++;
    }
    return -1;
}

static int
get_treeview_cursor_pos (GtkTreeView *tree)
{
    if (!tree) {
        return -1;
    }
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (tree), &path, &col);
    if (!path || !col) {
        return -1;
    }
    int result = -1;
    int *indices = gtk_tree_path_get_indices (path);
    if (indices) {
        result = indices[0];
        g_free (indices);
    }
    return result;
}

static void
on_pltbrowser_row_inserted (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data) {
    w_pltbrowser_t *plt = user_data;
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    if (idx > plt->last_selected) {
        idx--; // this is necessary, because in this case the rows will shift "up".
    }
    if (idx == plt->last_selected) {
        return;
    }

    deadbeef->plt_move (plt->last_selected, idx);
    plt->last_selected = idx;
    deadbeef->plt_set_curr_idx (idx);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_POSITION, 0);
}

static gboolean
update_pltbrowser_cb (gpointer data);

static void
on_pltbrowser_cursor_changed (GtkTreeView *treeview, gpointer user_data) {
    w_pltbrowser_t *w = user_data;
    int cursor = get_treeview_cursor_pos (treeview);
    if (cursor >= 0) {
        deadbeef->plt_set_curr_idx (cursor);
        w->last_selected = cursor;
    }
}

gboolean
on_pltbrowser_popup_menu (GtkWidget *widget, gpointer user_data);

static void
fill_pltbrowser_rows (gpointer user_data)
{
    w_pltbrowser_t *w = user_data;

    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (w->tree)));
    deadbeef->pl_lock ();
    int n = deadbeef->plt_get_count ();
    int plt_active = deadbeef->streamer_get_current_playlist ();
    int highlight_curr = deadbeef->conf_get_int ("gtkui.pltbrowser.highlight_curr_plt", 0);
    ddb_playback_state_t playback_state = deadbeef->get_output ()->state ();

    for (int i = 0; i < n; i++) {
        ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
        if (!plt) {
            continue;
        }
        GtkTreeIter iter;
        gtk_tree_model_iter_nth_child (gtk_tree_view_get_model (GTK_TREE_VIEW (w->tree)), &iter, NULL, i);
        GdkPixbuf *playing_pixbuf = NULL;
        char title[1000];
        char title_temp[1000];
        char num_items_str[100];
        deadbeef->plt_get_title (plt, title_temp, sizeof (title_temp));
        if (plt_active == i && highlight_curr) {
            if (playback_state == DDB_PLAYBACK_STATE_PAUSED) {
                snprintf (title, sizeof (title), "%s%s", title_temp, _(" (paused)"));
            }
            else if (playback_state == DDB_PLAYBACK_STATE_STOPPED) {
                snprintf (title, sizeof (title), "%s%s", title_temp, _(" (stopped)"));
            }
            else {
                snprintf (title, sizeof (title), "%s%s", title_temp, _(" (playing)"));
            }
        }
        else {
            snprintf (title, sizeof (title), "%s", title_temp);
        }

        if (plt_active == i) {
            GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
            if (icon_theme) {
                if (playback_state == DDB_PLAYBACK_STATE_PAUSED) {
                    playing_pixbuf = gtk_icon_theme_load_icon (icon_theme, "media-playback-pause", 16, 0, NULL);
                }
                else if (playback_state == DDB_PLAYBACK_STATE_STOPPED) {
                    playing_pixbuf = NULL;
                }
                else {
#if GTK_CHECK_VERSION(3,0,0)
                    playing_pixbuf = gtk_icon_theme_load_icon (icon_theme, "media-playback-start", 16, 0, NULL);
#else
                    playing_pixbuf = gtk_icon_theme_load_icon (icon_theme, "media-playback-start-ltr", 16, 0, NULL);
#endif
                }
            }
        }

        int num_items = deadbeef->plt_get_item_count (plt, PL_MAIN);
        snprintf (num_items_str, sizeof (num_items_str), "%d", num_items);

        float pl_totaltime = deadbeef->plt_get_totaltime (plt);
        int daystotal = (int)(int64_t)pl_totaltime / (3600 * 24);
        int hourtotal = (int)((int64_t)pl_totaltime / 3600) % 24;
        int mintotal = (int)((int64_t)pl_totaltime / 60) % 60;
        int sectotal = (int)((int64_t)pl_totaltime) % 60;

        char totaltime_str[512] = "";
        if (daystotal == 0) {
            snprintf (totaltime_str, sizeof (totaltime_str), "%d:%02d:%02d", hourtotal, mintotal, sectotal);
        }
        else {
            // NOTE: localizable because of the "d" suffix for days
            snprintf (totaltime_str, sizeof (totaltime_str), _("%dd %d:%02d:%02d"), daystotal, hourtotal, mintotal, sectotal);
        }

        gtk_list_store_set (store, &iter, COL_PLAYING, playing_pixbuf, COL_NAME, title, COL_ITEMS, num_items_str, COL_DURATION, totaltime_str, -1);
        deadbeef->plt_unref (plt);
    }
    deadbeef->pl_unlock ();
}

static gboolean
update_pltbrowser_cb (gpointer data) {
    fill_pltbrowser_rows (data);
    return FALSE;
}

static gboolean
fill_pltbrowser_cb (gpointer data) {
    w_pltbrowser_t *w = data;

    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (w->tree)));
    g_signal_handler_disconnect ((gpointer)w->tree, w->cc_id);
    g_signal_handler_disconnect ((gpointer)store, w->ri_id);
    w->cc_id = 0;
    w->ri_id = 0;

    deadbeef->pl_lock ();
    gtk_list_store_clear (store);
    int n = deadbeef->plt_get_count ();
    int curr = deadbeef->plt_get_curr_idx ();

    for (int i = 0; i < n; i++) {
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
    }
    if (curr != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (w->tree), path, NULL, FALSE);
        gtk_tree_path_free (path);
    }
    deadbeef->pl_unlock ();

    fill_pltbrowser_rows (data);

    w->ri_id = g_signal_connect ((gpointer)store, "row_inserted", G_CALLBACK (on_pltbrowser_row_inserted), w);
    w->cc_id = g_signal_connect ((gpointer)w->tree, "cursor_changed", G_CALLBACK (on_pltbrowser_cursor_changed), w);
    g_signal_connect ((gpointer) w->tree, "popup_menu", G_CALLBACK (on_pltbrowser_popup_menu), NULL);
    return FALSE;
}

static gboolean
update_treeview_cursor (gpointer user_data)
{
    w_pltbrowser_t *w = user_data;
    int curr = deadbeef->plt_get_curr_idx ();
    if (curr != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (w->tree), path, NULL, FALSE);
        gtk_tree_path_free (path);
    }
    return FALSE;
}

static int
pltbrowser_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGCHANGED:
        {
            int highlight_curr_plt = deadbeef->conf_get_int ("gtkui.pltbrowser.highlight_curr_plt", 0);
            // only fill when playlist changed
            if (highlight_curr_plt) {
                ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
                if (ev->from && ev->to) {
                    ddb_playlist_t *plt_from = deadbeef->pl_get_playlist (ev->from);
                    ddb_playlist_t *plt_to = deadbeef->pl_get_playlist (ev->to);
                    if (plt_from != plt_to) {
                        g_idle_add (update_pltbrowser_cb, w);
                    }
                    if (plt_from) {
                        deadbeef->plt_unref (plt_from);
                    }
                    if (plt_to) {
                        deadbeef->plt_unref (plt_to);
                    }
                }
                else if (!ev->from) {
                    g_idle_add (update_pltbrowser_cb, w);
                }
            }
        }
        break;
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (update_treeview_cursor, w);
        break;
    case DB_EV_CONFIGCHANGED:
    case DB_EV_PAUSED:
    case DB_EV_STOP:
    case DB_EV_TRACKINFOCHANGED:
        g_idle_add (update_pltbrowser_cb, w);
        break;
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT
            || p1 == DDB_PLAYLIST_CHANGE_TITLE)
            g_idle_add (update_pltbrowser_cb, w);
        else if (p1 == DDB_PLAYLIST_CHANGE_POSITION
            || p1 == DDB_PLAYLIST_CHANGE_DELETED
            || p1 == DDB_PLAYLIST_CHANGE_CREATED) {
            g_idle_add (fill_pltbrowser_cb, w);
        }
        break;
    }
    return 0;
}

// x, y must be relative to GtkTreeView's bin_window (see GtkTreeView documentation)
static int
get_treeview_row_at_pos (GtkTreeView *widget, int x, int y)
{
    GtkTreePath *path = NULL;
    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y, &path, NULL, NULL, NULL);

    int row = -1;
    if (!path) {
        // (probably) clicked empty area
        return row;
    }
    else {
        int *indices = gtk_tree_path_get_indices (path);
        if (indices) {
            if (indices[0] >= 0) {
                row = indices[0];
            }
            g_free (indices);
        }
    }
    return row;
}

static void
w_pltbrowser_init (struct ddb_gtkui_widget_s *w) {
    fill_pltbrowser_cb (w);
}

static GtkTreeViewColumn *
add_treeview_column (w_pltbrowser_t *w, GtkTreeView *tree, int pos, int expand, int align_right, const char *title, int is_pixbuf);

static void
on_popup_header_playing_clicked (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    w_pltbrowser_t *w = user_data;
    int active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    deadbeef->conf_set_int ("gtkui.pltbrowser.show_playing_column", active);
    if (active) {
        gtk_tree_view_column_set_visible (GTK_TREE_VIEW_COLUMN (w->col_playing), 1);
    }
    else if (w->col_playing) {
        gtk_tree_view_column_set_visible (GTK_TREE_VIEW_COLUMN (w->col_playing), 0);
    }
}

static void
on_popup_header_items_clicked (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    w_pltbrowser_t *w = user_data;
    int active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    deadbeef->conf_set_int ("gtkui.pltbrowser.show_items_column", active);
    if (active) {
        gtk_tree_view_column_set_visible (GTK_TREE_VIEW_COLUMN (w->col_items), 1);
    }
    else if (w->col_items) {
        gtk_tree_view_column_set_visible (GTK_TREE_VIEW_COLUMN (w->col_items), 0);
    }
}

static void
on_popup_header_duration_clicked (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
    w_pltbrowser_t *w = user_data;
    int active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    deadbeef->conf_set_int ("gtkui.pltbrowser.show_duration_column", active);
    if (active) {
        gtk_tree_view_column_set_visible (GTK_TREE_VIEW_COLUMN (w->col_duration), 1);
    }
    else if (w->col_duration) {
        gtk_tree_view_column_set_visible (GTK_TREE_VIEW_COLUMN (w->col_duration), 0);
    }
}

static gboolean
on_pltbrowser_header_popup_menu (GtkWidget *widget, gpointer user_data)
{
    w_pltbrowser_t *w = user_data;
    GtkWidget *popup = gtk_menu_new ();
    GtkWidget *playing = gtk_check_menu_item_new_with_mnemonic (_("Playing"));
    GtkWidget *items = gtk_check_menu_item_new_with_mnemonic (_("Items"));
    GtkWidget *duration = gtk_check_menu_item_new_with_mnemonic (_("Duration"));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (playing), deadbeef->conf_get_int ("gtkui.pltbrowser.show_playing_column", 0));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (items), deadbeef->conf_get_int ("gtkui.pltbrowser.show_items_column", 0));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (duration), deadbeef->conf_get_int ("gtkui.pltbrowser.show_duration_column", 0));
    gtk_container_add (GTK_CONTAINER (popup), playing);
    gtk_container_add (GTK_CONTAINER (popup), items);
    gtk_container_add (GTK_CONTAINER (popup), duration);
    gtk_widget_show (popup);
    gtk_widget_show (playing);
    gtk_widget_show (items);
    gtk_widget_show (duration);
    g_signal_connect_after ((gpointer) playing, "toggled", G_CALLBACK (on_popup_header_playing_clicked), w);
    g_signal_connect_after ((gpointer) items, "toggled", G_CALLBACK (on_popup_header_items_clicked), w);
    g_signal_connect_after ((gpointer) duration, "toggled", G_CALLBACK (on_popup_header_duration_clicked), w);
    gtk_menu_attach_to_widget (GTK_MENU (popup), GTK_WIDGET (widget), NULL);
    gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
    return TRUE;
}

static int
cmp_playlist_name_func (const void *a, const void *b) {
    ddb_playlist_t *aa = *((ddb_playlist_t **)a);
    ddb_playlist_t *bb = *((ddb_playlist_t **)b);
    char title0[1000];
    char title1[1000];
    deadbeef->plt_get_title (aa, title0, sizeof (title0));
    deadbeef->plt_get_title (bb, title1, sizeof (title1));
    return strcasecmp (title0, title1);
}

static int
cmp_playlist_items_func (const void *a, const void *b) {
    ddb_playlist_t *aa = *((ddb_playlist_t **)a);
    ddb_playlist_t *bb = *((ddb_playlist_t **)b);
    int num_items0 = deadbeef->plt_get_item_count (aa, PL_MAIN);
    int num_items1 = deadbeef->plt_get_item_count (bb, PL_MAIN);
    return num_items0 - num_items1;
}

static int
cmp_playlist_duration_func (const void *a, const void *b) {
    ddb_playlist_t *aa = *((ddb_playlist_t **)a);
    ddb_playlist_t *bb = *((ddb_playlist_t **)b);
    float pl_totaltime0 = deadbeef->plt_get_totaltime (aa);
    float pl_totaltime1 = deadbeef->plt_get_totaltime (bb);
    if (pl_totaltime0 > pl_totaltime1) {
        return 1;
    }
    else if (pl_totaltime0 == pl_totaltime1) {
        return 0;
    }
    else {
        return -1;
    }
}

static void
sort_playlists (int order, int (*qsort_cmp_func)(const void *, const void*))
{
    deadbeef->pl_lock ();
    int plt_count = deadbeef->plt_get_count ();
    ddb_playlist_t **array = malloc (plt_count * sizeof (ddb_playlist_t *));
    int idx = 0;
    for (ddb_playlist_t *plt = deadbeef->plt_get_for_idx (idx); plt; idx++, plt = deadbeef->plt_get_for_idx (idx)) {
        array[idx] = plt;
    }
    qsort (array, plt_count, sizeof (ddb_playlist_t *), qsort_cmp_func);
    deadbeef->pl_unlock ();

    for (int i = 0; i < plt_count; i++) {
        int idx = deadbeef->plt_get_idx (array[i]);
        if (order == GTK_SORT_ASCENDING) {
            deadbeef->plt_move (idx, i);
        }
        else {
            deadbeef->plt_move (idx, plt_count - i - 1);
        }
        deadbeef->plt_unref (array[i]);
    }

    free (array);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_POSITION, 0);
}

static void
sort_by_name (int order)
{
    sort_playlists (order, cmp_playlist_name_func);
    return;
}

static void
sort_by_items (int order)
{
    sort_playlists (order, cmp_playlist_items_func);
    return;
}

static void
sort_by_duration (int order)
{
    sort_playlists (order, cmp_playlist_duration_func);
    return;
}

static gint
get_col_number_from_tree_view_column (GtkTreeView *view, GtkTreeViewColumn *col)
{
    GList *cols = gtk_tree_view_get_columns(GTK_TREE_VIEW (view));
    gint num = g_list_index(cols, (gpointer) col);
    g_list_free(cols);
    return num;
}

static gboolean
on_pltbrowser_column_clicked (GtkTreeViewColumn     *col,
                              gpointer         user_data)
{
    GtkWidget *view = gtk_tree_view_column_get_tree_view (GTK_TREE_VIEW_COLUMN (col));
    int order = gtk_tree_view_column_get_sort_order (GTK_TREE_VIEW_COLUMN (col));

    // remove sort indicator from every column
    GList *list = gtk_tree_view_get_columns (GTK_TREE_VIEW (view));
    GList *node = list;
    GtkTreeViewColumn *column = node->data;
    while (node) {
        column = node->data;
        gtk_tree_view_column_set_sort_indicator (GTK_TREE_VIEW_COLUMN (column), FALSE);
        node = node->next;
    }
    g_list_free (list);

    // add sort indicator to current column
    gtk_tree_view_column_set_sort_indicator (GTK_TREE_VIEW_COLUMN (col), TRUE);
    gtk_tree_view_column_set_sort_order (GTK_TREE_VIEW_COLUMN (col), !order);

    // sort clicked column
    int col_id = get_col_number_from_tree_view_column (GTK_TREE_VIEW (view), GTK_TREE_VIEW_COLUMN (col));
    switch (col_id) {
        case COL_PLAYING:
            break;
        case COL_NAME:
            sort_by_name (order);
            break;
        case COL_ITEMS:
            sort_by_items (order);
            break;
        case COL_DURATION:
            sort_by_duration (order);
            break;
        default:
            sort_by_name (order);
            break;
    }
    return FALSE;
}

static gboolean
on_pltbrowser_header_clicked (GtkWidget       *widget,
                              GdkEventButton  *event,
                              gpointer         user_data)
{
    w_pltbrowser_t *w = user_data;
    if (gtkui_plugin->w_get_design_mode ()) {
        return FALSE;
    }
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 3) {
            return on_pltbrowser_header_popup_menu (widget, w);
        }
    }
    return FALSE;
}

static gboolean
on_pltbrowser_key_press_event (GtkWidget *widget,
                               GdkEventKey  *event,
                               gpointer   user_data)
{
    w_pltbrowser_t *w = user_data;
    if (event->state & GDK_CONTROL_MASK) {
        int row = get_treeview_cursor_pos (GTK_TREE_VIEW (w->tree));
        if (row >= 0) {
            deadbeef->pl_lock ();
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx (row);
            deadbeef->pl_unlock ();
            if (plt) {
                int res = 0;
                if (event->keyval == GDK_c) {
                    gtkui_plugin->copy_selection (plt, DDB_ACTION_CTX_PLAYLIST);
                    res = 1;
                }
                else if (event->keyval == GDK_v) {
                    gtkui_plugin->paste_selection (plt, DDB_ACTION_CTX_PLAYLIST);
                    res = 1;
                }
                else if (event->keyval == GDK_x) {
                    gtkui_plugin->cut_selection (plt, DDB_ACTION_CTX_PLAYLIST);
                    res = 1;
                }
                deadbeef->plt_unref (plt);
                return res;
            }
        }
    }
    return FALSE;
}

static GtkTreeViewColumn *
add_treeview_column (w_pltbrowser_t *w, GtkTreeView *tree, int pos, int expand, int align_right, const char *title, int is_pixbuf)
{
    GtkCellRenderer *rend = NULL;
    GtkTreeViewColumn *col = NULL;
    if (is_pixbuf) {
        rend = gtk_cell_renderer_pixbuf_new ();
        col = gtk_tree_view_column_new_with_attributes (title, rend, "pixbuf", pos, NULL);
    }
    else {
        rend = gtk_cell_renderer_text_new ();
        col = gtk_tree_view_column_new_with_attributes (title, rend, "text", pos, NULL);
    }
    if (align_right) {
        g_object_set (rend, "xalign", 1.0, NULL);
    }
    // GTK+ breaks row activation on editable rows, so we have to disable
    // inline editing for now
    /*
    if (pos == COL_NAME) {
        g_object_set (rend, "editable", TRUE, NULL);
        g_signal_connect (rend, "editing_started",
                G_CALLBACK (on_pltbrowser_cell_edititing_started),
                w);
        g_signal_connect (rend, "edited",
                G_CALLBACK (on_pltbrowser_cell_edited),
                w);
    }
    */
    gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_expand (col, expand);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (tree), col, pos);
    GtkWidget *label = gtk_label_new (title);
    gtk_tree_view_column_set_widget (col, label);
    gtk_widget_show (label);
    GtkWidget *col_button = gtk_widget_get_ancestor(label, GTK_TYPE_BUTTON);
    g_signal_connect (col_button, "button-press-event",
            G_CALLBACK (on_pltbrowser_header_clicked),
            w);
    g_signal_connect (col, "clicked",
            G_CALLBACK (on_pltbrowser_column_clicked),
            w);
    return col;
}

static gboolean drag_row_active = FALSE;

static gboolean
on_pltbrowser_drag_begin_event          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer user_data)
{
    drag_row_active = TRUE;
    return FALSE;
}

static gboolean
on_pltbrowser_drag_end_event          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer user_data)
{
    drag_row_active = FALSE;
    return FALSE;
}

static gboolean
on_pltbrowser_drag_motion_event          (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer user_data)
{
    w_pltbrowser_t *w = user_data;
    if (drag_row_active) {
        return FALSE;
    }
    GdkWindow *window = gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget));

    int bin_x = 0;
    int bin_y = 0;
    gdk_window_get_position (window, &bin_x, &bin_y);

    int row = get_treeview_row_at_pos (GTK_TREE_VIEW (widget), x - bin_x, y - bin_y);
    if (row >= 0) {
        deadbeef->plt_set_curr_idx (row);
        w->last_selected = row;
    }
    return FALSE;
}

gboolean
on_pltbrowser_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (gtkui_plugin->w_get_design_mode ()) {
        return FALSE;
    }
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 3) {
            int row_clicked = get_treeview_row_at_pos (GTK_TREE_VIEW (widget), event->x, event->y);
            GtkWidget *menu = gtkui_plugin->create_pltmenu (row_clicked);
            gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
            gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
            return TRUE;
        }
    }
    return FALSE;
}

gboolean
on_pltbrowser_button_press_end_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (gtkui_plugin->w_get_design_mode ()) {
        return FALSE;
    }
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 2) {
            int row_clicked = get_treeview_row_at_pos (GTK_TREE_VIEW (widget), event->x, event->y);
            if (row_clicked == -1) {
                // new tab
                int playlist = add_new_playlist ();
                if (playlist != -1) {
                    deadbeef->plt_set_curr_idx (playlist);
                }
                return TRUE;
            }
            else if (deadbeef->conf_get_int ("gtkui.pltbrowser.mmb_delete_playlist", 0)) {
                if (row_clicked != -1) {
                    deadbeef->plt_remove (row_clicked);
                }
            }
        }
    }
    else if (event->type == GDK_2BUTTON_PRESS) {
        if (event->button == 1) {
            int row_clicked = get_treeview_row_at_pos (GTK_TREE_VIEW (widget), event->x, event->y);
            if (row_clicked == -1) {
                // new tab
                int playlist = add_new_playlist ();
                if (playlist != -1) {
                    deadbeef->plt_set_curr_idx (playlist);
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

gboolean
on_pltbrowser_popup_menu (GtkWidget *widget, gpointer user_data) {
    int row = get_treeview_cursor_pos (GTK_TREE_VIEW (widget));
    if (row >= 0) {
        int plt_idx = row;
        GtkWidget *menu = gtkui_plugin->create_pltmenu (plt_idx);
        gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
        return TRUE;
    }
    return FALSE;
}

static void
on_pltbrowser_row_activated (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    if (deadbeef->conf_get_int ("gtkui.pltbrowser.play_on_double_click", 1)) {
        deadbeef->sendmessage (DB_EV_STOP, 0, 0, 0);
        deadbeef->sendmessage (DB_EV_NEXT, 0, 0, 0);
    }
}

static void
on_pltbrowser_showheaders_toggled (GtkCheckMenuItem *checkmenuitem, gpointer          user_data) {
    w_pltbrowser_t *w = user_data;
    int showheaders = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (checkmenuitem));
    deadbeef->conf_set_int ("gtkui.pltbrowser.show_headers", showheaders);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), showheaders);
}

static void
w_pltbrowser_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
    GtkWidget *item;
    item = gtk_check_menu_item_new_with_mnemonic (_("Show Column Headers"));
    gtk_widget_show (item);
    int showheaders = deadbeef->conf_get_int ("gtkui.pltbrowser.show_headers", 1);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), showheaders);
    gtk_container_add (GTK_CONTAINER (menu), item);
    g_signal_connect ((gpointer) item, "toggled",
            G_CALLBACK (on_pltbrowser_showheaders_toggled),
            w);
}

static ddb_gtkui_widget_t *
w_pltbrowser_create (void) {
    w_pltbrowser_t *w = malloc (sizeof (w_pltbrowser_t));
    memset (w, 0, sizeof (w_pltbrowser_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_pltbrowser_init;
    w->base.message = pltbrowser_message;
    w->base.initmenu = w_pltbrowser_initmenu;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->tree = gtk_tree_view_new ();
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (w->tree), TRUE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), TRUE);
    GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w->tree));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    gtk_widget_show (w->tree);

    gtk_container_add (GTK_CONTAINER (scroll), w->tree);

    GtkListStore *store = gtk_list_store_new (4, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));

    w->ri_id = g_signal_connect ((gpointer) store, "row_inserted", G_CALLBACK (on_pltbrowser_row_inserted), w);

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);
    add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_NAME, 1, 0, _("Name"), 0);

    int show_playing_column = deadbeef->conf_get_int ("gtkui.pltbrowser.show_playing_column", 0);
    w->col_playing = add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_PLAYING, 0, 1, _("♫"), 1);
    if (!show_playing_column) {
        gtk_tree_view_column_set_visible (w->col_playing, 0);
    }

    int show_item_column = deadbeef->conf_get_int ("gtkui.pltbrowser.show_items_column", 0);
    w->col_items = add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_ITEMS, 0, 1, _("Items"), 0);
    if (!show_item_column) {
        gtk_tree_view_column_set_visible (w->col_items, 0);
    }

    w->col_duration = add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_DURATION, 0, 1, _("Duration"), 0);
    int show_duration_column = deadbeef->conf_get_int ("gtkui.pltbrowser.show_duration_column", 0);
    if (!show_duration_column) {
        gtk_tree_view_column_set_visible (w->col_duration, 0);
    }

    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), TRUE);

    int showheaders = deadbeef->conf_get_int ("gtkui.pltbrowser.show_headers", 1);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), showheaders);

    w->cc_id = g_signal_connect ((gpointer) w->tree, "cursor_changed",
            G_CALLBACK (on_pltbrowser_cursor_changed),
            w);
    g_signal_connect ((gpointer) w->tree, "event_after",
            G_CALLBACK (on_pltbrowser_button_press_end_event),
            w);
    g_signal_connect ((gpointer) w->tree, "button-press-event",
            G_CALLBACK (on_pltbrowser_button_press_event),
            w);
    g_signal_connect ((gpointer) w->tree, "row_activated",
            G_CALLBACK (on_pltbrowser_row_activated),
            w);
    g_signal_connect ((gpointer) w->tree, "drag_begin",
            G_CALLBACK (on_pltbrowser_drag_begin_event),
            w);
    g_signal_connect ((gpointer) w->tree, "drag_end",
            G_CALLBACK (on_pltbrowser_drag_end_event),
            w);
    g_signal_connect ((gpointer) w->tree, "drag_motion",
            G_CALLBACK (on_pltbrowser_drag_motion_event),
            w);
    g_signal_connect ((gpointer) w->tree, "key_press_event",
            G_CALLBACK (on_pltbrowser_key_press_event),
            w);

    gtkui_plugin->w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}

static int
pltbrowser_connect (void) {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if(!gtkui_plugin) {
        return -1;
    }
    gtkui_plugin->w_reg_widget (_("Playlist browser"), 0, w_pltbrowser_create, "pltbrowser", NULL);

    return 0;
}

static int
pltbrowser_disconnect (void) {
    if (gtkui_plugin) {
        gtkui_plugin->w_unreg_widget ("pltbrowser");
    }
    return 0;
}

static const char pltbrowser_settings_dlg[] =
    "property \"Close playlists with middle mouse button\" checkbox gtkui.pltbrowser.mmb_delete_playlist 0;\n"
    "property \"Highlight current playlist\" checkbox gtkui.pltbrowser.highlight_curr_plt 0;\n"
    "property \"Play on double-click\" checkbox gtkui.pltbrowser.play_on_double_click 1;\n"
;

static DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "pltbrowser_gtk3",
#else
    .plugin.id = "pltbrowser_gtk2",
#endif
    .plugin.name = "Playlist Browser",
    .plugin.descr = "Use View -> Design Mode to add playlist browser into main window",
    .plugin.copyright = 
        "Playlist browser widget plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.connect = pltbrowser_connect,
    .plugin.disconnect = pltbrowser_disconnect,
    .plugin.configdialog = pltbrowser_settings_dlg,
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
pltbrowser_gtk3_load (DB_functions_t *api) {
#else
pltbrowser_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
