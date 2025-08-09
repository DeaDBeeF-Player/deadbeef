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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <dispatch/dispatch.h>
#include <gtk/gtk.h>
#include "../gtkui.h"
#include "../support.h"
#include "../medialib/medialibmanager.h"
#include "../../medialib/medialib.h"
#include "prefwin.h"
#include "prefwinmedialib.h"

static GtkWidget *prefwin;
static DB_mediasource_t *medialib_plugin;
static ddb_medialib_plugin_api_t *medialib_api;
static GtkTreeView *treeview;
static int _listener_id;

static void
_reload_data (void) {
    ddb_mediasource_source_t *source = gtkui_medialib_get_source ();
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

    gtk_list_store_clear(store);

    int count = medialib_api->folder_count (source);
    for (int i = 0; i < count; i++) {
        char path[PATH_MAX];
        medialib_api->folder_at_index (source, i, path, sizeof (path));
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter, 0, path, -1);
    }
}

static void
_enable_did_toggle (GtkToggleButton *togglebutton, gpointer user_data) {
    gboolean active = gtk_toggle_button_get_active (togglebutton);
    ddb_mediasource_source_t *source = gtkui_medialib_get_source ();
    medialib_plugin->set_source_enabled (source, active);
    medialib_plugin->refresh (source);
}

static void
_add_did_activate (GtkButton* self, gpointer user_data) {
    GSList *folders = show_file_chooser(_("Select music folders to add"), GTKUI_FILECHOOSER_OPENFOLDER, TRUE);

    if (folders == NULL) {
        return;
    }

    int count = g_slist_length(folders);
    if (count == 0) {
        return;
    }

    ddb_mediasource_source_t *source = gtkui_medialib_get_source ();
    GSList *curr = folders;

    for (int i = 0; i < count; i++) {
        medialib_api->append_folder(source, curr->data);
        curr = curr->next;
    }
    g_slist_free(folders);
    folders = NULL;
    medialib_plugin->refresh (source);
}

static void
_remove_did_activate (GtkButton* self, gpointer user_data) {
    ddb_mediasource_source_t *source = gtkui_medialib_get_source ();
    int count = medialib_api->folder_count (source);
    if (count == 0) {
        return;
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
    GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model(treeview));
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
        return;
    }
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
    gint *indices = gtk_tree_path_get_indices (path);
    gint index_count = gtk_tree_path_get_depth(path);
    if (index_count != 1) {
        return;
    }

    medialib_api->remove_folder_at_index(source, indices[0]);
    medialib_plugin->refresh (source);
}

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
_listener (ddb_mediasource_event_type_t _event, void *user_data) {
    _dispatch_on_main(^{
        ddb_mediasource_source_t *source = gtkui_medialib_get_source();
        if ((int)_event < 1000) {
            switch (_event) {
            case DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE:
                {
                    GtkWidget *enable_button = lookup_widget(prefwin, "toggle_medialib_on");
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_button), medialib_plugin->is_source_enabled(source));
                }
                break;
            default:
                break;
            }
            return;
        }

        ddb_medialib_mediasource_event_type_t event = (ddb_medialib_mediasource_event_type_t)_event;
        switch (event) {
        case DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE:
            _reload_data();
            break;
        }
    });
}

void
prefwin_init_medialib (GtkWidget *_prefwin) {
    prefwin = _prefwin;
    medialib_plugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    if (medialib_plugin == NULL) {
        return;
    }

    medialib_api = (ddb_medialib_plugin_api_t *)medialib_plugin->get_extended_api();

    ddb_mediasource_source_t *source = gtkui_medialib_get_source();
    if (source == NULL) {
        return;
    }

    _listener_id = medialib_plugin->add_listener(source, _listener, prefwin);

    int enabled = medialib_plugin->is_source_enabled(source);
    GtkWidget *enable_button = lookup_widget(prefwin, "toggle_medialib_on");
    prefwin_set_toggle_button("toggle_medialib_on", enabled);

    treeview = GTK_TREE_VIEW(lookup_widget(prefwin, "treeview_medialib_folders"));
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Path", renderer, "text", 0, NULL);
    gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (treeview), column, 0);

    GtkWidget *button_add = lookup_widget(prefwin, "button_medialib_add_folder");
    GtkWidget *button_remove = lookup_widget(prefwin, "button_medialib_remove_folder");

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_button_set_image(GTK_BUTTON(button_add), gtk_image_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_BUTTON));
    gtk_button_set_image(GTK_BUTTON(button_remove), gtk_image_new_from_icon_name("list-remove-symbolic", GTK_ICON_SIZE_BUTTON));
#endif

    g_signal_connect((gpointer)enable_button, "toggled", G_CALLBACK (_enable_did_toggle), prefwin);
    g_signal_connect((gpointer)button_add, "clicked", G_CALLBACK (_add_did_activate), prefwin);
    g_signal_connect((gpointer)button_remove, "clicked", G_CALLBACK (_remove_did_activate), prefwin);

    _reload_data ();
}

void
prefwin_free_medialib (void) {
    if (medialib_plugin == NULL) {
        return;
    }
    ddb_mediasource_source_t *source = gtkui_medialib_get_source();
    if (source == NULL) {
        return;
    }

    medialib_plugin->remove_listener (source, _listener_id);
    _listener_id = 0;
}
