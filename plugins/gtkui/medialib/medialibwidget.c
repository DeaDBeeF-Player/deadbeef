//
//  medialibwidget.c
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 11/08/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//


#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include "../../../gettext.h"
#include "../prefwin/prefwin.h"
#include "../support.h"
#include "../playlist/ddblistview.h"
#include "medialibwidget.h"
#include "medialibmanager.h"
#include "plmenu.h"
#include "../../../shared/scriptable/scriptable.h"
#include "../scriptable/gtkScriptableSelectViewController.h"

extern DB_functions_t *deadbeef;
static DB_mediasource_t *plugin;

typedef struct {
    ddb_gtkui_widget_t base;
    gtkScriptableSelectViewController_t *selectViewController;
    gtkScriptableSelectViewControllerDelegate_t scriptableSelectDelegate;
    GtkTreeView *tree;
    GtkEntry *search_entry;
    ddb_mediasource_source_t *source;
    char *preset;
    char *search_text;
    int listener_id;
    GtkTreeIter root_iter;
    ddb_medialib_item_t *item_tree;
    gint collapse_expand_select_timeout;
    int is_reloading;
} w_medialib_viewer_t;

enum {
    COL_TITLE,
    COL_TRACK,
    COL_ITEM,
};

static void
_restore_selected_expanded_state_for_iter (w_medialib_viewer_t *mlv, GtkTreeStore *store, GtkTreeIter *iter);

static int
_item_comparator (const void *a, const void *b) {
    const ddb_medialib_item_t *item1 = *((ddb_medialib_item_t **)a);
    const ddb_medialib_item_t *item2 = *((ddb_medialib_item_t **)b);

    ddb_playItem_t *track1 = plugin->tree_item_get_track(item1);
    ddb_playItem_t *track2 = plugin->tree_item_get_track(item2);

    if (!track1 || !track2) {
        const char *text1 = plugin->tree_item_get_text(item1);
        const char *text2 = plugin->tree_item_get_text(item2);
        return strcasecmp (text1, text2);
    }

    int n1 = atoi (deadbeef->pl_find_meta (track1, "track") ?: "0");
    int n2 = atoi (deadbeef->pl_find_meta (track2, "track") ?: "0");
    int d1 = atoi (deadbeef->pl_find_meta (track1, "disc") ?: "0") + 1;
    int d2 = atoi (deadbeef->pl_find_meta (track2, "disc") ?: "0") + 1;
    n1 = d1 * 10000 + n1;
    n2 = d2 * 10000 + n2;

    return n1-n2;
}

static const ddb_medialib_item_t **
_sorted_children_from_item (const ddb_medialib_item_t *item) {
    int count = plugin->tree_item_get_children_count(item);
    const ddb_medialib_item_t **children = calloc (count, sizeof (ddb_medialib_item_t *));
    const ddb_medialib_item_t *c = plugin->tree_item_get_children(item);
    for (int i = 0; i < count; i++) {
        children[i] = c;
        c = plugin->tree_item_get_next(c);
    }

    qsort (children, count, sizeof (ddb_medialib_item_t *), _item_comparator);

    return children;
}

static void
_add_items (w_medialib_viewer_t *mlv, GtkTreeIter *iter, const ddb_medialib_item_t *item) {
    if (item == NULL) {
        return;
    }
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));

    const ddb_medialib_item_t **sorted_items = _sorted_children_from_item (item);

    int count = plugin->tree_item_get_children_count(item);
    for (int i = 0; i < count; i++) {
        const ddb_medialib_item_t *child_item = sorted_items[i];
        GtkTreeIter child;
        gtk_tree_store_append (store, &child, iter);
        int child_numchildren = plugin->tree_item_get_children_count(child_item);
        const char *item_text = plugin->tree_item_get_text(child_item);
        if (child_numchildren > 0) {
            size_t len = strlen(item_text) + 20;
            char *text = malloc (len + 20);
            snprintf (text, len, "%s (%d)", item_text, child_numchildren);
            gtk_tree_store_set (store, &child, COL_TITLE, text, -1);
            free (text);
        }
        else {
            gtk_tree_store_set (store, &child, COL_TITLE, item_text, -1);
        }

        ddb_playItem_t *track = plugin->tree_item_get_track(child_item);
        gtk_tree_store_set(store, &child, COL_TRACK, track, -1);
        gtk_tree_store_set(store, &child, COL_ITEM, child_item, -1);

        const ddb_medialib_item_t *child_children = plugin->tree_item_get_children(child_item);
        if (child_children != NULL) {
            _add_items (mlv, &child, child_item);
        }
    }

    free (sorted_items);
}

static gboolean
_medialib_state_did_change (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    ddb_mediasource_state_t state = plugin->scanner_state (mlv->source);
    int enabled = plugin->is_source_enabled (mlv->source);
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    switch (state) {
    case DDB_MEDIASOURCE_STATE_IDLE:
        if (enabled) {
            char text[200];
            int count = plugin->tree_item_get_children_count(mlv->item_tree);
            snprintf (text, sizeof (text), "%s (%d)", _("All Music"), mlv->item_tree ? count : 0);
            gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, text, -1);
        }
        else {
            gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _("Media library is disabled"), -1);
        }
        break;
    case DDB_MEDIASOURCE_STATE_LOADING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _("Loading..."), -1);
        break;
    case DDB_MEDIASOURCE_STATE_SCANNING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _("Scanning..."), -1);
        break;
    case DDB_MEDIASOURCE_STATE_INDEXING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _("Indexing..."), -1);
        break;
    case DDB_MEDIASOURCE_STATE_SAVING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _("Saving..."), -1);
        break;
    }

    return FALSE;
}

static void
_reload_content (w_medialib_viewer_t *mlv) {
    // populate the tree
    if (mlv->item_tree != NULL) {
        plugin->free_item_tree (mlv->source, mlv->item_tree);
        mlv->item_tree = NULL;
    }

    scriptableItem_t *preset = NULL;

    scriptableItem_t *presets = plugin->get_queries_scriptable(mlv->source);
    if (presets) {
        if (mlv->preset) {
            preset = scriptableItemSubItemForName(presets, mlv->preset);
        }
        if (preset == NULL) {
            preset = scriptableItemChildren(presets);
        }
        mlv->item_tree = plugin->create_item_tree (mlv->source, preset, mlv->search_text);
    }

    mlv->is_reloading = 1;
    // clear
    GtkTreeIter iter;
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &iter, &mlv->root_iter)) {
        while (gtk_tree_store_remove (store, &iter));
    }

    _add_items (mlv, &mlv->root_iter, mlv->item_tree);

    GtkTreePath *path = gtk_tree_path_new_from_indices (0, -1);
    gtk_tree_view_expand_row (mlv->tree, path, mlv->search_text != NULL);
    gtk_tree_path_free (path);

    _medialib_state_did_change (mlv);

    // restore selected/expanded state
    _restore_selected_expanded_state_for_iter (mlv, store, &mlv->root_iter);

    mlv->is_reloading = 0;
}

static gboolean
_medialib_content_did_change (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    if (plugin == NULL) {
        return FALSE;
    }
    _reload_content (mlv);
    return FALSE;
}

static void
_medialib_listener (ddb_mediasource_event_type_t event, void *user_data) {
    switch (event) {
    case DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE:
        g_idle_add (_medialib_content_did_change, user_data);
        break;
    case DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE:
    case DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE:
        g_idle_add (_medialib_state_did_change, user_data);
        break;
    default:
        break;
    }
}

static gboolean _selection_func (
                                 GtkTreeSelection  *selection,
                                 GtkTreeModel      *model,
                                 GtkTreePath       *path,
                                 gboolean           path_currently_selected,
                                 gpointer           data
                                 ) {
    gint *indices = gtk_tree_path_get_indices(path);

    int count = gtk_tree_path_get_depth(path);

    // don't select root
    if (count == 1 && indices[0] == 0) {
        return FALSE;
    }

    return TRUE;
}

static void
_save_selection_state_with_iter (w_medialib_viewer_t *mlv, GtkTreeStore *store, GtkTreeIter *iter) {
    if (mlv->is_reloading) {
        return;
    }
    GtkTreeModel *model = GTK_TREE_MODEL(store);

    GValue value = {0};
    gtk_tree_model_get_value (model, iter, COL_ITEM, &value);
    ddb_medialib_item_t *medialibItem = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (medialibItem != NULL) {
        GtkTreePath *path = gtk_tree_model_get_path(model, iter);
        if (path != NULL) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);
            gboolean selected = gtk_tree_selection_iter_is_selected(selection, iter);
            gboolean expanded = gtk_tree_view_row_expanded(mlv->tree, path);
            plugin->set_tree_item_selected (mlv->source, medialibItem, selected ? 1 : 0);
            plugin->set_tree_item_expanded (mlv->source, medialibItem, expanded ? 1 : 0);
        }
    }

    GtkTreeIter child;
    if (gtk_tree_model_iter_children (model, &child, iter)) {
        do {
            _save_selection_state_with_iter(mlv, store, &child);
        } while (gtk_tree_model_iter_next (model, &child));
    }
}

static void
_restore_selected_expanded_state_for_iter (w_medialib_viewer_t *mlv, GtkTreeStore *store, GtkTreeIter *iter) {
    GtkTreeModel *model = GTK_TREE_MODEL(store);

    GValue value = {0};
    gtk_tree_model_get_value (model, iter, COL_ITEM, &value);
    ddb_medialib_item_t *medialibItem = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (medialibItem != NULL) {
        int selected = plugin->is_tree_item_selected (mlv->source, medialibItem);
        int expanded = plugin->is_tree_item_expanded (mlv->source, medialibItem);

        GtkTreePath *path = gtk_tree_model_get_path(model, iter);
        if (expanded) {
            gtk_tree_view_expand_row(mlv->tree, path, FALSE);
        }
        else {
            gtk_tree_view_collapse_row(mlv->tree, path);
        }

        GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);
        if (selected) {
            gtk_tree_selection_select_iter(selection, iter);
        }
    }

    GtkTreeIter child;
    if (gtk_tree_model_iter_children (model, &child, iter)) {
        do {
            _restore_selected_expanded_state_for_iter(mlv, store, &child);
        } while (gtk_tree_model_iter_next (model, &child));
    }
}

static gboolean
_row_collapse_expand_selection_did_change (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    _save_selection_state_with_iter(mlv, store, &mlv->root_iter);
    mlv->collapse_expand_select_timeout = 0;
    return FALSE;
}

static void
_selection_did_change (GtkTreeSelection* self, w_medialib_viewer_t *mlv) {
    if (mlv->collapse_expand_select_timeout != 0) {
        g_source_remove (mlv->collapse_expand_select_timeout);
    }
    if (mlv->is_reloading) {
        return;
    }
    mlv->collapse_expand_select_timeout = g_timeout_add(50, _row_collapse_expand_selection_did_change, mlv);
}

static void
_row_did_collapse_expand (GtkTreeView* self, GtkTreeIter* iter, GtkTreePath* path, w_medialib_viewer_t *mlv) {
    if (mlv->collapse_expand_select_timeout != 0) {
        g_source_remove (mlv->collapse_expand_select_timeout);
    }
    if (mlv->is_reloading) {
        return;
    }
    mlv->collapse_expand_select_timeout = g_timeout_add(50, _row_collapse_expand_selection_did_change, mlv);
}

static void
_scriptableSelectSelectionDidChange(gtkScriptableSelectViewController_t *vc, scriptableItem_t *item, void *context) {
    w_medialib_viewer_t *mlv = context;

    const char *name = scriptableItemPropertyValueForKey(item, "name");

    if (mlv->preset == NULL || strcmp(mlv->preset, name)) {
        free (mlv->preset);
        mlv->preset = strdup(name);
        _reload_content (mlv);
    }
}

static void
w_medialib_viewer_init (struct ddb_gtkui_widget_s *w) {
    // observe medialib source
    w_medialib_viewer_t *mlv = (w_medialib_viewer_t *)w;
    if (plugin == NULL) {
        plugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    }
    if (plugin == NULL) {
        return;
    }

    mlv->source = gtkui_medialib_get_source ();
    mlv->listener_id =  plugin->add_listener (mlv->source, _medialib_listener, mlv);

    scriptableItem_t *presets = plugin->get_queries_scriptable(mlv->source);

    mlv->scriptableSelectDelegate.selectionDidChange = _scriptableSelectSelectionDidChange;
    gtkScriptableSelectViewControllerSetScriptable(mlv->selectViewController, presets);
    gtkScriptableSelectViewControllerSetDelegate(mlv->selectViewController, &mlv->scriptableSelectDelegate, mlv);
    gtkScriptableSelectViewControllerSelectItem(mlv->selectViewController, scriptableItemChildren(presets));

    // Root node
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    gtk_tree_store_append (store, &mlv->root_iter, NULL);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);
    gtk_tree_selection_set_select_function(selection, _selection_func, mlv, NULL);
    g_signal_connect(selection, "changed", G_CALLBACK(_selection_did_change), w);
    g_signal_connect(mlv->tree, "row-collapsed", G_CALLBACK(_row_did_collapse_expand), w);
    g_signal_connect(mlv->tree, "row-expanded", G_CALLBACK(_row_did_collapse_expand), w);

    _reload_content (mlv);
}

static void
w_medialib_viewer_destroy (struct ddb_gtkui_widget_s *w) {
    w_medialib_viewer_t *mlv = (w_medialib_viewer_t *)w;
    if (mlv->selectViewController != NULL) {
        gtkScriptableSelectViewControllerFree(mlv->selectViewController);
    }
    if (mlv->source != NULL) {
        plugin->remove_listener (mlv->source, mlv->listener_id);
    }
    if (mlv->item_tree != NULL) {
        plugin->free_item_tree (mlv->source, mlv->item_tree);
        mlv->item_tree = NULL;
    }
    free (mlv->preset);
    free (mlv->search_text);
    mlv->search_text = NULL;
}

static int
w_medialib_viewer_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

static GtkTreeViewColumn *
add_treeview_column (w_medialib_viewer_t *w, GtkTreeView *tree, int pos, int expand, int align_right, const char *title, int is_pixbuf)
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

    gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_expand (col, expand);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (tree), col, pos);
    GtkWidget *label = gtk_label_new (title);
    gtk_tree_view_column_set_widget (col, label);
    gtk_widget_show (label);
    return col;
}

static void
_search_text_did_change (GtkEditable *editable, gpointer user_data) {
    w_medialib_viewer_t *mlv = user_data;

    const gchar *text = gtk_entry_get_text (mlv->search_entry);

    free (mlv->search_text);
    mlv->search_text = NULL;
    if (*text) {
        mlv->search_text = strdup (text);
    }

    _reload_content (mlv);
}

static int
_collect_tracks_from_iter (GtkTreeModel *model, GtkTreeIter *iter, ddb_playItem_t **tracks, int append_position) {
    // is it a track?
    GValue value = {0};
    gtk_tree_model_get_value (model, iter, COL_TRACK, &value);
    ddb_playItem_t *track = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (track) {
        if (tracks != NULL) {
            ddb_playItem_t *it = deadbeef->pl_item_alloc();
            deadbeef->pl_item_copy(it, track);
            tracks[append_position] = it;
        }
        return 1;
    }

    int count = 0;
    // recurse into children
    GtkTreeIter child;
    if (gtk_tree_model_iter_children (model, &child, iter)) {
        do {
            int appended_count = _collect_tracks_from_iter (model, &child, tracks, append_position);
            count += appended_count;
            append_position += appended_count;
        } while (gtk_tree_model_iter_next (model, &child));
    }

    return count;
}

static int
_collect_selected_tracks (GtkTreeModel *model, GtkTreeSelection *selection, ddb_playItem_t **tracks, int append_position) {
    GList *selected_rows = gtk_tree_selection_get_selected_rows (selection, NULL);

    int count = 0;

    for (GList *row = selected_rows; row; row = row->next) {
        GtkTreePath *path = (GtkTreePath *)row->data;
        GtkTreeIter iter;
        if (!gtk_tree_model_get_iter (model, &iter, path)) {
            gtk_tree_path_free (path);
            continue;
        }

        int appended_count = _collect_tracks_from_iter (model, &iter, tracks, append_position);
        count += appended_count;
        append_position += appended_count;
        gtk_tree_path_free (path);
    }

    g_list_free(selected_rows);
    return count;
}

static void _append_tracks_to_playlist (ddb_playItem_t **tracks, int count, ddb_playlist_t *plt) {
    ddb_playItem_t *prev = deadbeef->plt_get_tail_item (plt, PL_MAIN);
    for (int i = 0; i < count; i++) {
        ddb_playItem_t *it = deadbeef->pl_item_alloc ();
        deadbeef->pl_item_copy (it, tracks[i]);
        deadbeef->plt_insert_item (plt, prev, it);
        if (prev != NULL) {
            deadbeef->pl_item_unref (prev);
        }
        prev = it;
    }
    if (prev != NULL) {
        deadbeef->pl_item_unref (prev);
    }
    prev = NULL;
}

static ddb_playlist_t *
_get_target_playlist (void) {
    ddb_playlist_t *plt = NULL;
    if (deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1)) {
        char str[200];
        deadbeef->conf_get_str ("cli_add_playlist_name", "Default", str, sizeof (str));
        plt = deadbeef->plt_find_by_name (str);
        if (plt == NULL) {
            plt = deadbeef->plt_append (str);
        }
    }
    return plt;
}

static void
_treeview_row_did_activate (GtkTreeView* self, GtkTreePath* path, GtkTreeViewColumn* column, gpointer user_data) {
    w_medialib_viewer_t *mlv = user_data;

    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (mlv->tree));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter (model, &iter, path)) {
        return;
    }

    ddb_playlist_t *curr_plt = _get_target_playlist ();
    if (curr_plt == NULL) {
        return;
    }

    deadbeef->plt_set_curr (curr_plt);
    deadbeef->plt_clear (curr_plt);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (self);

    // count selected tracks
    int count = _collect_selected_tracks (model, selection, NULL, 0);

    if (count > 0) {
        // create array of tracks
        ddb_playItem_t **tracks = NULL;
        tracks = calloc (count, sizeof (ddb_playItem_t *));
        _collect_selected_tracks (model, selection, tracks, 0);

        _append_tracks_to_playlist (tracks, count, curr_plt);

        for (int i = 0; i < count; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }

        free (tracks);

        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, 0, 0);
    }

    deadbeef->plt_unref (curr_plt);
}

static gboolean
_select_at_position (GtkTreeView *treeview, gint x, gint y) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (treeview));

    GtkTreePath *path;
    if (!gtk_tree_view_get_path_at_pos (treeview,
                                       x, y,
                                       &path, NULL, NULL, NULL)) {
        gtk_tree_selection_unselect_all (selection);
        return FALSE;
    }

    GtkTreeIter iter;
    gtk_tree_model_get_iter (model, &iter, path);
    if (!gtk_tree_selection_iter_is_selected (selection, &iter)) {
        gtk_tree_selection_unselect_all (selection);
        gtk_tree_selection_select_path (selection, path);
    }
    gtk_tree_path_free (path);
    return TRUE;
}

static void
_trkproperties_did_change_tracks (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    plugin->refresh (mlv->source);
}

static void
_trkproperties_did_delete_files (void *user_data, int cancelled) {
    if (!cancelled) {
        w_medialib_viewer_t *mlv = user_data;
        plugin->refresh (mlv->source);
    }
}

static trkproperties_delegate_t _trkproperties_delegate = {
    .trkproperties_did_update_tracks = _trkproperties_did_change_tracks,
    .trkproperties_did_reload_metadata = _trkproperties_did_change_tracks,
    .trkproperties_did_delete_files = _trkproperties_did_delete_files,
};

gboolean
_treeview_row_mousedown (GtkWidget* self, GdkEventButton *event, gpointer user_data) {
    if (w_get_design_mode ()) {
        return FALSE;
    }

    if (event->type != GDK_BUTTON_PRESS || (event->button != 3 && event->button != 2)) {
        return FALSE;
    }

    w_medialib_viewer_t *mlv = user_data;
    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (mlv->tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);

    if (!_select_at_position (mlv->tree, event->x, event->y)) {
        return FALSE;
    }

    // count selected tracks
    int count = _collect_selected_tracks (model, selection, NULL, 0);

    // create array of tracks
    ddb_playItem_t **tracks = NULL;
    if (count == 0) {
        return TRUE;
    }

    tracks = calloc (count, sizeof (ddb_playItem_t *));
    _collect_selected_tracks (model, selection, tracks, 0);

    // context menu
    if (event->button == 3) {
        _trkproperties_delegate.user_data = mlv;

        ddb_playlist_t *plt = deadbeef->plt_alloc("MediaLib Action Playlist");

        ddb_playItem_t *after = NULL;
        for (int i = 0; i < count; i++) {
            after = deadbeef->plt_insert_item(plt, after, tracks[i]);
        }
        deadbeef->plt_select_all(plt);

        list_context_menu_with_dynamic_track_list (plt, &_trkproperties_delegate);

        deadbeef->plt_unref(plt);
    }

    // append to playlist
    else if (event->button == 2 && count > 0) {
        ddb_playlist_t *curr_plt = _get_target_playlist ();
        if (curr_plt != NULL) {
            deadbeef->plt_set_curr (curr_plt);
            _append_tracks_to_playlist (tracks, count, curr_plt);
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        }
    }
    for (int i = 0; i < count; i++) {
        deadbeef->pl_item_unref (tracks[i]);
    }
    free (tracks);

    return TRUE;
}

static void
_configure_did_activate (GtkButton* self, gpointer user_data) {
    prefwin_run (PREFWIN_TAB_INDEX_MEDIALIB);
}

static void
_drag_data_get (
               GtkWidget* widget,
               GdkDragContext* context,
               GtkSelectionData* selection_data,
               guint info,
               guint time_
                ) {
    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

    int count = _collect_selected_tracks (model, selection, NULL, 0);
    if (count == 0) {
        return;
    }
    ddb_playItem_t **tracks = calloc (count, sizeof (ddb_playItem_t *));
    _collect_selected_tracks (model, selection, tracks, 0);

    GdkAtom type = gtk_selection_data_get_target (selection_data);
    gtk_selection_data_set (selection_data,
                           type,
                           sizeof (void *) * 8,
                           (const guchar *)tracks,
                           count * sizeof (ddb_playItem_t *));

    free (tracks);
    tracks = NULL;
}

ddb_gtkui_widget_t *
w_medialib_viewer_create (void) {
    w_medialib_viewer_t *w = calloc (1, sizeof (w_medialib_viewer_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_medialib_viewer_init;
    w->base.destroy = w_medialib_viewer_destroy;
    w->base.message = w_medialib_viewer_message;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    DB_mediasource_t *plugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    if (plugin == NULL) {
        GtkWidget *label = gtk_label_new(_("Media Library plugin is unavailable."));
        gtk_widget_show (label);
        gtk_container_add (GTK_CONTAINER (w->base.widget), label);
        return (ddb_gtkui_widget_t *)w;
    }

    GtkWidget *vbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (w->base.widget), vbox);

    w->selectViewController = gtkScriptableSelectViewControllerNew();
    GtkWidget *selectViewControllerWidget = gtkScriptableSelectViewControllerGetView(w->selectViewController);

    GtkWidget *select_view_wrap_hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (select_view_wrap_hbox);
    gtk_box_pack_start (GTK_BOX (vbox), select_view_wrap_hbox, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (select_view_wrap_hbox), selectViewControllerWidget, TRUE, TRUE, 20);

    GtkWidget *configure_button = gtk_button_new_with_label (_("Configure"));
    gtk_widget_show (configure_button);
    gtk_box_pack_start (GTK_BOX (select_view_wrap_hbox), configure_button, TRUE, TRUE, 0);

    GtkWidget *search_hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (search_hbox);
    gtk_box_pack_start (GTK_BOX (vbox), search_hbox, FALSE, TRUE, 0);

    w->search_entry = GTK_ENTRY (gtk_entry_new ());
#if GTK_CHECK_VERSION (3,2,0)
    gtk_entry_set_placeholder_text (w->search_entry, _("Search"));
#endif
    gtk_widget_show (GTK_WIDGET (w->search_entry));
    gtk_box_pack_start (GTK_BOX (search_hbox), GTK_WIDGET (w->search_entry), TRUE, TRUE, 20);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->tree = GTK_TREE_VIEW (gtk_tree_view_new ());
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), TRUE);
    GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w->tree));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    gtk_widget_show (GTK_WIDGET (w->tree));

    gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (w->tree));

    GtkTreeStore *store = gtk_tree_store_new (
                                              3,
                                              G_TYPE_STRING,  // COL_TITLE
                                              G_TYPE_POINTER, // COL_TRACK
                                              G_TYPE_POINTER  // COL_ITEM
                                              );
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);
    add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_TITLE, 1, 0, "", 0);

    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), FALSE);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (w->tree);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

    g_signal_connect ((gpointer)w->search_entry, "changed", G_CALLBACK (_search_text_did_change), w);
    g_signal_connect ((gpointer)w->tree, "row-activated", G_CALLBACK (_treeview_row_did_activate), w);
    g_signal_connect ((gpointer)w->tree, "button_press_event", G_CALLBACK (_treeview_row_mousedown), w);
    g_signal_connect ((gpointer)configure_button, "clicked", G_CALLBACK (_configure_did_activate), w);

    GtkTargetEntry entry = {
        .target = TARGET_PLAYITEM_POINTERS,
        .flags = GTK_TARGET_SAME_APP,
        .info = 0
    };
    gtk_drag_source_set (GTK_WIDGET (w->tree), GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_COPY);

    g_signal_connect ((gpointer) w->tree, "drag_data_get", G_CALLBACK (_drag_data_get), w);

    w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}
