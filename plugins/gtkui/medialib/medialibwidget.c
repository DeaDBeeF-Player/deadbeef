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
#include "../../artwork/artwork.h"
#include "../covermanager/covermanager.h"
#include "../prefwin/prefwin.h"
#include "../support.h"
#include "../playlist/ddblistview.h"
#include "medialibwidget.h"
#include "medialibmanager.h"
#include "plmenu.h"
#include "../../../shared/scriptable/scriptable.h"
#include "../scriptable/gtkScriptableSelectViewController.h"
#include "mlcellrendererpixbuf.h"
#include "../gtkui.h"
#include "undo/undobuffer.h"
#include "undo/undomanager.h"

extern DB_functions_t *deadbeef;
static DB_mediasource_t *plugin;

typedef struct {
    ddb_gtkui_widget_t base;
    gtkScriptableSelectViewController_t *selectViewController;
    gtkScriptableSelectViewControllerDelegate_t scriptableSelectDelegate;
    GtkTreeView *tree;
    GtkEntry *search_entry;
    ddb_mediasource_source_t *source;
    char *search_text;
    int listener_id;
    GtkTreeIter root_iter;
    GtkTreeStore *store;
    ddb_medialib_item_t *item_tree;
    gint collapse_expand_select_timeout;
    int is_reloading;
    MlCellRendererPixbufDelegate pixbuf_cell_delegate;

    GdkPixbuf *folder_icon;
    int reload_index;
    int64_t artwork_source_id;
    dispatch_queue_t background_queue;
} w_medialib_viewer_t;

enum {
    COL_ICON,
    COL_TITLE,
    COL_TRACK,
    COL_ITEM,
    COL_PATH,
};

static void
_restore_selected_expanded_state_for_iter (w_medialib_viewer_t *mlv, GtkTreeStore *store, GtkTreeIter *iter);

static void
_add_items (w_medialib_viewer_t *mlv, GtkTreeIter *iter, const ddb_medialib_item_t *item, GtkTreePath *parent_path) {
    if (item == NULL) {
        return;
    }
    GtkTreeStore *store = mlv->store;

    const ddb_medialib_item_t *child_item = plugin->tree_item_get_children (item);
    int index = 0;
    while (child_item != NULL) {
        GtkTreeIter child;
        gtk_tree_store_append (store, &child, iter);
        int child_numchildren = plugin->tree_item_get_children_count (child_item);
        const char *item_text = plugin->tree_item_get_text (child_item);
        if (child_numchildren > 0) {
            size_t len = strlen (item_text) + 20;
            char *text = malloc (len + 20);
            snprintf (text, len, "%s (%d)", item_text, child_numchildren);
            gtk_tree_store_set (store, &child, COL_TITLE, text, -1);
            free (text);
        }
        else {
            gtk_tree_store_set (store, &child, COL_TITLE, item_text, -1);
        }

        ddb_playItem_t *track = plugin->tree_item_get_track (child_item);
        gtk_tree_store_set (store, &child, COL_TRACK, track, -1);
        gtk_tree_store_set (store, &child, COL_ITEM, child_item, -1);

        GtkTreePath *path = gtk_tree_path_copy (parent_path);
        gtk_tree_path_append_index (path, index++);

        gchar *strpath = gtk_tree_path_to_string (path);
        gtk_tree_store_set (store, &child, COL_PATH, strpath, -1);
        g_free (strpath);

        const ddb_medialib_item_t *child_children = plugin->tree_item_get_children (child_item);
        if (child_children != NULL) {
            _add_items (mlv, &child, child_item, path);
        }

        gtk_tree_path_free (path);

        child_item = plugin->tree_item_get_next (child_item);
    }
}

static gboolean
_medialib_state_did_change (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    ddb_mediasource_state_t state = plugin->scanner_state (mlv->source);
    int enabled = plugin->is_source_enabled (mlv->source);
    GtkTreeStore *store = mlv->store;
    switch (state) {
    case DDB_MEDIASOURCE_STATE_IDLE:
        if (enabled) {
            char text[200];
            int count = 0;
            if (mlv->item_tree != NULL) {
                count = plugin->tree_item_get_children_count (mlv->item_tree);
            }
            snprintf (text, sizeof (text), "%s (%d)", _ ("All Music"), mlv->item_tree ? count : 0);
            gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, text, -1);
        }
        else {
            gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _ ("Media library is disabled"), -1);
        }
        break;
    case DDB_MEDIASOURCE_STATE_LOADING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _ ("Loading..."), -1);
        break;
    case DDB_MEDIASOURCE_STATE_SCANNING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _ ("Scanning..."), -1);
        break;
    case DDB_MEDIASOURCE_STATE_INDEXING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _ ("Indexing..."), -1);
        break;
    case DDB_MEDIASOURCE_STATE_SAVING:
        gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _ ("Saving..."), -1);
        break;
    }

    return FALSE;
}

static void
_reload_content (w_medialib_viewer_t *mlv) {
    // populate the tree
    mlv->reload_index++;

    GtkTreeStore *store = mlv->store;

    if (mlv->item_tree != NULL) {
        plugin->free_item_tree (mlv->source, mlv->item_tree);
        mlv->item_tree = NULL;
    }

    scriptableItem_t *preset = NULL;

    scriptableItem_t *presets = plugin->get_queries_scriptable (mlv->source);
    if (presets) {
        scriptableModel_t *model = gtkui_medialib_get_model ();

        char *curr_preset = scriptableModelGetAPI (model)->get_active_name (model);
        if (curr_preset) {
            preset = scriptableItemSubItemForName (presets, curr_preset);
        }
        if (preset == NULL) {
            preset = scriptableItemChildren (presets);
        }
        mlv->item_tree = plugin->create_item_tree (mlv->source, preset, mlv->search_text);
        free (curr_preset);
    }

    mlv->is_reloading = 1;
    // clear
    GtkTreeIter iter;
    if (gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &iter, &mlv->root_iter)) {
        while (gtk_tree_store_remove (store, &iter))
            ;
    }

    GtkTreePath *root_path = gtk_tree_path_new_from_indices (0, -1);
    _add_items (mlv, &mlv->root_iter, mlv->item_tree, root_path);

    gtk_tree_view_expand_row (mlv->tree, root_path, mlv->search_text != NULL);
    gtk_tree_path_free (root_path);

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

static gboolean
_selection_func (
    GtkTreeSelection *selection,
    GtkTreeModel *model,
    GtkTreePath *path,
    gboolean path_currently_selected,
    gpointer data) {
    gint *indices = gtk_tree_path_get_indices (path);

    int count = gtk_tree_path_get_depth (path);

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
    GtkTreeModel *model = GTK_TREE_MODEL (store);

    GValue value = { 0 };
    gtk_tree_model_get_value (model, iter, COL_ITEM, &value);
    ddb_medialib_item_t *medialibItem = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (medialibItem != NULL) {
        GtkTreePath *path = gtk_tree_model_get_path (model, iter);
        if (path != NULL) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);
            gboolean selected = gtk_tree_selection_iter_is_selected (selection, iter);
            gboolean expanded = gtk_tree_view_row_expanded (mlv->tree, path);
            plugin->set_tree_item_selected (mlv->source, medialibItem, selected ? 1 : 0);
            plugin->set_tree_item_expanded (mlv->source, medialibItem, expanded ? 1 : 0);
        }
    }

    GtkTreeIter child;
    if (gtk_tree_model_iter_children (model, &child, iter)) {
        do {
            _save_selection_state_with_iter (mlv, store, &child);
        } while (gtk_tree_model_iter_next (model, &child));
    }
}

static void
_restore_selected_expanded_state_for_iter (w_medialib_viewer_t *mlv, GtkTreeStore *store, GtkTreeIter *iter) {
    GtkTreeModel *model = GTK_TREE_MODEL (store);

    GValue value = { 0 };
    gtk_tree_model_get_value (model, iter, COL_ITEM, &value);
    ddb_medialib_item_t *medialibItem = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (medialibItem != NULL) {
        int selected = plugin->is_tree_item_selected (mlv->source, medialibItem);
        int expanded = plugin->is_tree_item_expanded (mlv->source, medialibItem);

        GtkTreePath *path = gtk_tree_model_get_path (model, iter);
        if (expanded) {
            gtk_tree_view_expand_row (mlv->tree, path, FALSE);
        }
        else {
            gtk_tree_view_collapse_row (mlv->tree, path);
        }

        GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);
        if (selected) {
            gtk_tree_selection_select_iter (selection, iter);
        }
    }

    GtkTreeIter child;
    if (gtk_tree_model_iter_children (model, &child, iter)) {
        do {
            _restore_selected_expanded_state_for_iter (mlv, store, &child);
        } while (gtk_tree_model_iter_next (model, &child));
    }
}

static gboolean
_row_collapse_expand_selection_did_change (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    GtkTreeStore *store = mlv->store;
    _save_selection_state_with_iter (mlv, store, &mlv->root_iter);
    mlv->collapse_expand_select_timeout = 0;
    return FALSE;
}

static void
_selection_did_change (GtkTreeSelection *self, w_medialib_viewer_t *mlv) {
    if (mlv->collapse_expand_select_timeout != 0) {
        g_source_remove (mlv->collapse_expand_select_timeout);
    }
    if (mlv->is_reloading) {
        return;
    }
    mlv->collapse_expand_select_timeout = g_timeout_add (50, _row_collapse_expand_selection_did_change, mlv);
}

static void
_row_did_collapse_expand (GtkTreeView *self, GtkTreeIter *iter, GtkTreePath *path, w_medialib_viewer_t *mlv) {
    if (mlv->collapse_expand_select_timeout != 0) {
        g_source_remove (mlv->collapse_expand_select_timeout);
    }
    if (mlv->is_reloading) {
        return;
    }
    mlv->collapse_expand_select_timeout = g_timeout_add (50, _row_collapse_expand_selection_did_change, mlv);
}

static void
_scriptableSelectSelectionDidChange (gtkScriptableSelectViewController_t *vc, scriptableItem_t *item, void *context) {
    w_medialib_viewer_t *mlv = context;

    _reload_content (mlv);
}

static void
_scriptableSelectScriptableDidChange (
    gtkScriptableSelectViewController_t *view_controller,
    gtkScriptableChange_t change_type,
    void *context) {
    w_medialib_viewer_t *mlv = context;
    scriptableItem_t *presets = plugin->get_queries_scriptable (mlv->source);
    scriptableItemSave (presets);

    // refresh the tree
    int index = gtkScriptableSelectViewControllerIndexOfSelectedItem (mlv->selectViewController);
    scriptableItem_t *item = scriptableItemChildAtIndex (presets, (unsigned int)index);
    _scriptableSelectSelectionDidChange (mlv->selectViewController, item, context);
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
    mlv->listener_id = plugin->add_listener (mlv->source, _medialib_listener, mlv);

    scriptableItem_t *presets = plugin->get_queries_scriptable (mlv->source);

    mlv->scriptableSelectDelegate.selection_did_change = _scriptableSelectSelectionDidChange;
    mlv->scriptableSelectDelegate.scriptable_did_change = _scriptableSelectScriptableDidChange;
    gtkScriptableSelectViewControllerSetScriptable (mlv->selectViewController, presets);
    gtkScriptableSelectViewControllerSetModel (mlv->selectViewController, gtkui_medialib_get_model ());
    gtkScriptableSelectViewControllerSetDelegate (mlv->selectViewController, &mlv->scriptableSelectDelegate, mlv);

    // Root node
    GtkTreeStore *store = mlv->store;
    gtk_tree_store_append (store, &mlv->root_iter, NULL);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (mlv->tree);
    gtk_tree_selection_set_select_function (selection, _selection_func, mlv, NULL);
    g_signal_connect (selection, "changed", G_CALLBACK (_selection_did_change), w);
    g_signal_connect (mlv->tree, "row-collapsed", G_CALLBACK (_row_did_collapse_expand), w);
    g_signal_connect (mlv->tree, "row-expanded", G_CALLBACK (_row_did_collapse_expand), w);

    _reload_content (mlv);
}

static void
w_medialib_viewer_destroy (struct ddb_gtkui_widget_s *w) {
    w_medialib_viewer_t *mlv = (w_medialib_viewer_t *)w;
    if (mlv->background_queue != NULL) {
        dispatch_release (mlv->background_queue);
        mlv->background_queue = NULL;
    }
    if (mlv->selectViewController != NULL) {
        gtkScriptableSelectViewControllerFree (mlv->selectViewController);
    }
    if (mlv->source != NULL) {
        plugin->remove_listener (mlv->source, mlv->listener_id);
    }
    if (mlv->item_tree != NULL) {
        plugin->free_item_tree (mlv->source, mlv->item_tree);
        mlv->item_tree = NULL;
    }
    free (mlv->search_text);
    mlv->search_text = NULL;
    if (mlv->folder_icon) {
        g_object_unref (mlv->folder_icon);
        mlv->folder_icon = NULL;
    }
}

static int
w_medialib_viewer_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
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
    GValue value = { 0 };
    gtk_tree_model_get_value (model, iter, COL_TRACK, &value);
    ddb_playItem_t *track = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (track) {
        if (tracks != NULL) {
            ddb_playItem_t *it = deadbeef->pl_item_alloc ();
            deadbeef->pl_item_copy (it, track);
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
_collect_selected_tracks (
    GtkTreeModel *model,
    GtkTreeSelection *selection,
    ddb_playItem_t **tracks,
    int append_position) {
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

    g_list_free (selected_rows);
    return count;
}

static void
_append_tracks_to_playlist (ddb_playItem_t **tracks, int count, ddb_playlist_t *plt) {
    ddb_undobuffer_t *undobuffer = ddb_undomanager_get_buffer (ddb_undomanager_shared ());
    ddb_undobuffer_group_begin (undobuffer);

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

    ddb_undobuffer_group_end (undobuffer);
    deadbeef->undo_set_action_name (_("Add Files"));

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
_treeview_row_did_activate (GtkTreeView *self, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    w_medialib_viewer_t *mlv = user_data;

    GtkTreeModel *model = GTK_TREE_MODEL (mlv->store);
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
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    GtkTreePath *path;
    if (!gtk_tree_view_get_path_at_pos (treeview, x, y, &path, NULL, NULL, NULL)) {
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
_treeview_row_mousedown (GtkWidget *self, GdkEventButton *event, gpointer user_data) {
    if (w_get_design_mode ()) {
        return FALSE;
    }

    if (event->type != GDK_BUTTON_PRESS || (event->button != 3 && event->button != 2)) {
        return FALSE;
    }

    w_medialib_viewer_t *mlv = user_data;
    GtkTreeModel *model = GTK_TREE_MODEL (mlv->store);
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

        ddb_playlist_t *plt = deadbeef->plt_alloc ("MediaLib Action Playlist");

        ddb_playItem_t *after = NULL;
        for (int i = 0; i < count; i++) {
            after = deadbeef->plt_insert_item (plt, after, tracks[i]);
        }
        deadbeef->plt_select_all (plt);

        list_context_menu_with_dynamic_track_list (plt, &_trkproperties_delegate);

        deadbeef->plt_unref (plt);
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
_configure_did_activate (GtkButton *self, gpointer user_data) {
    prefwin_run (PREFWIN_TAB_INDEX_MEDIALIB);
}

static void
_drag_data_get (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint info, guint time) {
    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (widget)));
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));

    int count = _collect_selected_tracks (model, selection, NULL, 0);
    if (count == 0) {
        return;
    }
    ddb_playItem_t **tracks = calloc (count, sizeof (ddb_playItem_t *));
    _collect_selected_tracks (model, selection, tracks, 0);

    GdkAtom type = gtk_selection_data_get_target (selection_data);
    gtk_selection_data_set (
        selection_data,
        type,
        sizeof (void *) * 8,
        (const guchar *)tracks,
        count * sizeof (ddb_playItem_t *));

    free (tracks);
    tracks = NULL;
}

static void
_receive_cover (w_medialib_viewer_t *mlv, GtkTreePath *path, GdkPixbuf *img) {
    g_object_ref (img);
    dispatch_async (mlv->background_queue, ^{
        // scale
        GtkAllocation a;
        a.x = 0;
        a.y = 0;
        a.width = ML_CELL_RENDERER_PIXBUF_SIZE;
        a.height = ML_CELL_RENDERER_PIXBUF_SIZE;
        GdkPixbuf *scaled_img = covermanager_create_scaled_image (covermanager_shared (), img, a);

        gtkui_dispatch_on_main (^{
            GtkTreeStore *store = mlv->store;
            GtkTreeIter iter;
            GtkTreeModel *model = GTK_TREE_MODEL (mlv->store);
            gtk_tree_model_get_iter (model, &iter, path);
            gtk_tree_store_set (store, &iter, COL_ICON, scaled_img, -1);
            g_object_unref (scaled_img);
            gtk_tree_path_free (path);
            g_object_unref (img);
        });
    });
}

static GdkPixbuf *
_pixbuf_cell_did_become_visible (void *ctx, const char *pathstr) {
    if (pathstr == NULL) {
        return NULL;
    }
    GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);
    if (path == NULL) {
        return NULL;
    }

    w_medialib_viewer_t *mlv = ctx;
    if (mlv->is_reloading) {
        return NULL;
    }
    GtkTreeModel *model = GTK_TREE_MODEL (mlv->store);

    GtkTreeIter iter;
    gtk_tree_model_get_iter (model, &iter, path);
    if (!gtk_tree_model_iter_has_child (model, &iter)) {
        return NULL; // leaf
    }

    // get first child
    GtkTreePath *child_path = gtk_tree_path_copy (path);
    gtk_tree_path_append_index (child_path, 0);
    gtk_tree_model_get_iter (model, &iter, child_path);
    gtk_tree_path_free (child_path);
    child_path = NULL;

    GValue value = { 0 };
    gtk_tree_model_get_value (model, &iter, COL_TRACK, &value);
    ddb_playItem_t *track = g_value_get_pointer (&value);
    g_value_unset (&value);

    if (track == NULL) {
        return mlv->folder_icon;
    }

    if (mlv->artwork_source_id == 0) {
        return mlv->folder_icon;
    }

    int64_t reload_index = mlv->reload_index;
    GdkPixbuf *cached_cover = covermanager_cover_for_track_no_default (
        covermanager_shared (),
        track,
        mlv->artwork_source_id,
        ^(GdkPixbuf *img) {
            if (reload_index == mlv->reload_index && img != NULL) {
                _receive_cover (mlv, path, img);
            }
        });

    if (cached_cover != NULL) {
        _receive_cover (mlv, path, cached_cover);
    }
    return mlv->folder_icon;
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
        GtkWidget *label = gtk_label_new (_ ("Media Library plugin is unavailable."));
        gtk_widget_show (label);
        gtk_container_add (GTK_CONTAINER (w->base.widget), label);
        return (ddb_gtkui_widget_t *)w;
    }

    GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (w->base.widget), vbox);

    GtkWidget *headerbox = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (headerbox);
    gtk_box_pack_start (GTK_BOX (vbox), headerbox, FALSE, FALSE, 8);

    w->selectViewController = gtkScriptableSelectViewControllerNew ();

    GtkWidget *selectViewControllerWidget = gtkScriptableSelectViewControllerGetView (w->selectViewController);

    GtkWidget *buttons_padding_hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (buttons_padding_hbox);
    gtk_box_pack_start (GTK_BOX (headerbox), buttons_padding_hbox, FALSE, TRUE, 0);

    GtkWidget *buttons_container_hbox = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (buttons_container_hbox);
    gtk_box_pack_start (GTK_BOX (buttons_padding_hbox), buttons_container_hbox, TRUE, TRUE, 20);
    gtk_box_pack_start (GTK_BOX (buttons_container_hbox), selectViewControllerWidget, TRUE, TRUE, 0);

    GtkWidget *configure_button = gtk_button_new_with_label (_ ("Configure"));
    gtk_widget_show (configure_button);
    gtk_box_pack_start (GTK_BOX (buttons_container_hbox), configure_button, FALSE, FALSE, 0);

    GtkWidget *search_hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (search_hbox);
    gtk_box_pack_start (GTK_BOX (headerbox), search_hbox, FALSE, TRUE, 0);

    w->search_entry = GTK_ENTRY (gtk_entry_new ());
#if GTK_CHECK_VERSION(3, 2, 0)
    gtk_entry_set_placeholder_text (w->search_entry, _ ("Search"));
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

    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    // NOTE: using "folder-music" icon doesn't yield a good result,
    // since it doesn't look like a folder across the icon themes.
    w->folder_icon = gtk_icon_theme_load_icon (icon_theme, "folder", ML_CELL_RENDERER_PIXBUF_SIZE, 0, NULL);
    ddb_artwork_plugin_t *artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    if (artwork_plugin != NULL) {
        w->artwork_source_id = artwork_plugin->allocate_source_id ();
    }
    w->pixbuf_cell_delegate.ctx = w;
    w->pixbuf_cell_delegate.cell_did_became_visible = _pixbuf_cell_did_become_visible;
    w->background_queue = dispatch_queue_create ("MedialibBackgroundQueue", NULL);

    GtkTreeStore *store = gtk_tree_store_new (
        5,
        GDK_TYPE_PIXBUF, // COL_ICON
        G_TYPE_STRING, // COL_TITLE
        G_TYPE_POINTER, // COL_TRACK
        G_TYPE_POINTER, // COL_ITEM
        G_TYPE_STRING // COL_PATH
    );
    w->store = store;

    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);
    GtkCellRenderer *rend_pixbuf = GTK_CELL_RENDERER (ml_cell_renderer_pixbuf_new (&w->pixbuf_cell_delegate));
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();

    GtkTreeViewColumn *col = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (w->tree, col);
    gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_pack_start (col, rend_pixbuf, FALSE);
    gtk_tree_view_column_pack_start (col, rend_text, FALSE);

    gtk_tree_view_column_add_attribute (col, rend_pixbuf, "path", COL_PATH);
    gtk_tree_view_column_add_attribute (col, rend_pixbuf, "pixbuf", COL_ICON);
    gtk_tree_view_column_add_attribute (col, rend_text, "text", COL_TITLE);

    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), FALSE);

    GtkTreeSelection *selection = gtk_tree_view_get_selection (w->tree);
    gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

    g_signal_connect ((gpointer)w->search_entry, "changed", G_CALLBACK (_search_text_did_change), w);
    g_signal_connect ((gpointer)w->tree, "row-activated", G_CALLBACK (_treeview_row_did_activate), w);
    g_signal_connect ((gpointer)w->tree, "button_press_event", G_CALLBACK (_treeview_row_mousedown), w);
    g_signal_connect ((gpointer)configure_button, "clicked", G_CALLBACK (_configure_did_activate), w);

    GtkTargetEntry entry = { .target = TARGET_PLAYITEM_POINTERS, .flags = GTK_TARGET_SAME_APP, .info = 0 };
    gtk_drag_source_set (GTK_WIDGET (w->tree), GDK_BUTTON1_MASK, &entry, 1, GDK_ACTION_COPY);

    g_signal_connect ((gpointer)w->tree, "drag_data_get", G_CALLBACK (_drag_data_get), w);

    w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}
