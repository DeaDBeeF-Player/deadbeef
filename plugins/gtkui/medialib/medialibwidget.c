//
//  medialibwidget.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 11/08/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//


#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../../../deadbeef.h"
#include "../../../gettext.h"
#include "../support.h"
#include "medialibwidget.h"
#include "../../medialib/medialib.h"

extern DB_functions_t *deadbeef;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkTreeView *tree;
    GtkComboBoxText *selector;
    GtkEntry *search_entry;
    ddb_medialib_plugin_t *plugin;
    ddb_mediasource_source_t source;
    ddb_mediasource_list_selector_t *selectors;
    int active_selector;
    char *search_text;
    int listener_id;
    GtkTreeIter root_iter;
    ddb_medialib_item_t *item_tree;
} w_medialib_viewer_t;

enum {
    COL_TITLE,
    COL_TRACK,
};

static void
_add_items (w_medialib_viewer_t *mlv, GtkTreeIter *iter, ddb_medialib_item_t *item) {
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));

    for (ddb_medialib_item_t *child_item = item->children; child_item; child_item = child_item->next) {
        GtkTreeIter child;
        gtk_tree_store_append (store, &child, iter);
        gtk_tree_store_set (store, &child, COL_TITLE, child_item->text, COL_TRACK, child_item->track, -1);

        if (child_item->children != NULL) {
            _add_items(mlv, &child, child_item);
        }
    }
}

static void
_reload_content (w_medialib_viewer_t *mlv) {
    // populate the tree
    if (mlv->item_tree != NULL) {
        mlv->plugin->plugin.free_item_tree (mlv->source, mlv->item_tree);
        mlv->item_tree = NULL;
    }
    mlv->item_tree = mlv->plugin->plugin.create_item_tree (mlv->source, mlv->selectors[mlv->active_selector], mlv->search_text);

    // clear
    GtkTreeIter iter;
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    if (gtk_tree_model_iter_children(GTK_TREE_MODEL(store), &iter, &mlv->root_iter)) {
        while (gtk_tree_store_remove(store, &iter));
    }

    _add_items(mlv, &mlv->root_iter, mlv->item_tree);

    GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
    gtk_tree_view_expand_row (mlv->tree, path, mlv->search_text != NULL);
    gtk_tree_path_free (path);
}

static gboolean
_medialib_event (void *user_data) {
    w_medialib_viewer_t *mlv = user_data;
    if (mlv->plugin == NULL) {
        return FALSE;
    }
//    if (event == DDB_MEDIASOURCE_EVENT_CONTENT_CHANGED || event == DDB_MEDIASOURCE_EVENT_SCAN_DID_COMPLETE) {
//        [self filterChanged];
//    }
//    else if (event == DDB_MEDIASOURCE_EVENT_STATE_CHANGED) {
//        int state = self.medialibPlugin->plugin.scanner_state (self.medialibSource);
//        if (state != DDB_MEDIASOURCE_STATE_IDLE) {
//            //            [_scannerActiveIndicator startAnimation:self];
//
//            [self updateMedialibStatus];
//
//            //            [_scannerActiveState setHidden:NO];
//        }
//        else {
//            //            [_scannerActiveIndicator stopAnimation:self];
//            //            [_scannerActiveState setHidden:YES];
//        }
//    }

    _reload_content(mlv);
    return FALSE;
}

static void
_medialib_listener(ddb_mediasource_event_type_t event, void *user_data) {
    if (event == DDB_MEDIASOURCE_EVENT_CONTENT_CHANGED) {
        g_idle_add(_medialib_event, user_data);
    }
}

static void
w_medialib_viewer_init (struct ddb_gtkui_widget_s *w) {
    // observe medialib source
    w_medialib_viewer_t *mlv = (w_medialib_viewer_t *)w;
    mlv->plugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id("medialib");
    if (mlv->plugin == NULL) {
        return;
    }
    mlv->source = mlv->plugin->plugin.create_source ("deadbeef");
    mlv->plugin->plugin.refresh(mlv->source);
    mlv->selectors = mlv->plugin->plugin.get_selectors_list (mlv->source);
    mlv->listener_id =  mlv->plugin->plugin.add_listener (mlv->source, _medialib_listener, mlv);

    for (int i = 0; mlv->selectors[i]; i++) {
        gtk_combo_box_text_append_text(mlv->selector, mlv->plugin->plugin.selector_name(mlv->source, mlv->selectors[i]));
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(mlv->selector), 0);
    mlv->active_selector = 0;

    // Root node
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    gtk_tree_store_append (store, &mlv->root_iter, NULL);
    gtk_tree_store_set (store, &mlv->root_iter, COL_TITLE, _("All Music"), -1);

    _reload_content(mlv);
}

static void
w_medialib_viewer_destroy (struct ddb_gtkui_widget_s *w) {
    w_medialib_viewer_t *mlv = (w_medialib_viewer_t *)w;
    if (mlv->item_tree != NULL) {
        mlv->plugin->plugin.free_item_tree (mlv->source, mlv->item_tree);
        mlv->item_tree = NULL;
    }
    if (mlv->selectors) {
        mlv->plugin->plugin.free_selectors_list (mlv->source, mlv->selectors);
        mlv->selectors = NULL;
    }
    free (mlv->search_text);
    mlv->search_text = NULL;
    if (mlv->source) {
        mlv->plugin->plugin.free_source (mlv->source);
        mlv->source = NULL;
    }
}

static int
w_medialib_viewer_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

static void
w_medialib_viewer_initmenu (struct ddb_gtkui_widget_s *w, GtkWidget *menu) {
//    GtkWidget *item;
//    item = gtk_check_menu_item_new_with_mnemonic (_("Show Column Headers"));
//    gtk_widget_show (item);
//    int showheaders = deadbeef->conf_get_int ("gtkui.pltbrowser.show_headers", 1);
//    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), showheaders);
//    gtk_container_add (GTK_CONTAINER (menu), item);
//    g_signal_connect ((gpointer) item, "toggled",
//                      G_CALLBACK (on_pltbrowser_showheaders_toggled),
//                      w);
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
//    GtkWidget *col_button = gtk_widget_get_ancestor(label, GTK_TYPE_BUTTON);
//    g_signal_connect (col_button, "button-press-event",
//                      G_CALLBACK (on_pltbrowser_header_clicked),
//                      w);
//    g_signal_connect (col, "clicked",
//                      G_CALLBACK (on_pltbrowser_column_clicked),
//                      w);
    return col;
}

static void
_active_selector_did_change (GtkComboBox* self, gpointer user_data) {
    w_medialib_viewer_t *mlv = user_data;
    int active_selector = gtk_combo_box_get_active(self);
    if (mlv->active_selector != active_selector) {
        mlv->active_selector = active_selector;
        _reload_content (mlv);
    }
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

static void
_treeview_row_did_activate (GtkTreeView* self, GtkTreePath* path, GtkTreeViewColumn* column, gpointer user_data) {
    w_medialib_viewer_t *mlv = user_data;

    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (mlv->tree));
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter (model, &iter, path)) {
        return;
    }

    ddb_playItem_t *track = NULL;

    if (!gtk_tree_model_iter_has_child(model, &iter)) {
        GValue value = {0};
        gtk_tree_model_get_value (model, &iter, COL_TRACK, &value);

        track = g_value_get_pointer(&value);

        g_value_unset (&value);
    }

    ddb_playlist_t *curr_plt = NULL;
    if (deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1)) {
        char str[200];
        deadbeef->conf_get_str ("cli_add_playlist_name", "Default", str, sizeof (str));
        curr_plt = deadbeef->plt_find_by_name (str);
        if (!curr_plt) {
            curr_plt = deadbeef->plt_append (str);
        }
    }
    if (!curr_plt) {
        return;
    }

    deadbeef->plt_set_curr (curr_plt);
    deadbeef->plt_clear(curr_plt);


    int count = 0;

    if (track != NULL) {
        ddb_playItem_t *it = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy (it, track);
        deadbeef->plt_insert_item (curr_plt, NULL, it);
        count = 1;
    }
    else {
        GtkTreeIter child;
        if (gtk_tree_model_iter_children (model, &child, &iter)) {
            ddb_playItem_t *prev = NULL;
            do {
                GValue value = {0};
                gtk_tree_model_get_value (model, &child, COL_TRACK, &value);

                ddb_playItem_t *track = g_value_get_pointer(&value);

                g_value_unset (&value);

                if (track) {
                    ddb_playItem_t *it = deadbeef->pl_item_alloc();
                    deadbeef->pl_item_copy (it, track);
                    deadbeef->plt_insert_item (curr_plt, prev, it);
                    if (prev != NULL) {
                        deadbeef->pl_item_unref (prev);
                    }
                    prev = it;
                    count += 1;
                }
            } while (gtk_tree_model_iter_next(model, &child));

            if (prev != NULL) {
                deadbeef->pl_item_unref (prev);
            }
            prev = NULL;
        }
    }

    if (count > 0) {
        deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, 0, 0);
    }
}

ddb_gtkui_widget_t *
w_medialib_viewer_create (void) {
    w_medialib_viewer_t *w = calloc (1, sizeof (w_medialib_viewer_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_medialib_viewer_init;
    w->base.destroy = w_medialib_viewer_destroy;
    w->base.message = w_medialib_viewer_message;
    w->base.initmenu = w_medialib_viewer_initmenu;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 8);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (w->base.widget), vbox);

    w->selector = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    gtk_widget_show (GTK_WIDGET(w->selector));
    gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(w->selector), FALSE, TRUE, 0);

    w->search_entry = GTK_ENTRY(gtk_entry_new());
#if GTK_CHECK_VERSION(3,2,0)
    gtk_entry_set_placeholder_text(w->search_entry, _("Search"));
#endif
    gtk_widget_show (GTK_WIDGET(w->search_entry));
    gtk_box_pack_start (GTK_BOX(vbox), GTK_WIDGET(w->search_entry), FALSE, TRUE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_box_pack_start (GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->tree = GTK_TREE_VIEW(gtk_tree_view_new ());
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (w->tree), TRUE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), TRUE);
    GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w->tree));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    gtk_widget_show (GTK_WIDGET(w->tree));

    gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(w->tree));

    GtkTreeStore *store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);
    add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_TITLE, 1, 0, _("Item"), 0);

    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), FALSE);

    g_signal_connect((gpointer)w->selector, "changed", G_CALLBACK (_active_selector_did_change), w);
    g_signal_connect((gpointer)w->search_entry, "changed", G_CALLBACK (_search_text_did_change), w);
    g_signal_connect((gpointer)w->tree, "row-activated", G_CALLBACK (_treeview_row_did_activate), w);

//    w->cc_id = g_signal_connect ((gpointer) w->tree, "cursor_changed",
//                                 G_CALLBACK (on_pltbrowser_cursor_changed),
//                                 w);
//    g_signal_connect ((gpointer) w->tree, "event_after",
//                      G_CALLBACK (on_pltbrowser_button_press_end_event),
//                      w);
//    g_signal_connect ((gpointer) w->tree, "button-press-event",
//                      G_CALLBACK (on_pltbrowser_button_press_event),
//                      w);
//    g_signal_connect ((gpointer) w->tree, "row_activated",
//                      G_CALLBACK (on_pltbrowser_row_activated),
//                      w);
//    g_signal_connect ((gpointer) w->tree, "drag_begin",
//                      G_CALLBACK (on_pltbrowser_drag_begin_event),
//                      w);
//    g_signal_connect ((gpointer) w->tree, "drag_end",
//                      G_CALLBACK (on_pltbrowser_drag_end_event),
//                      w);
//    g_signal_connect ((gpointer) w->tree, "drag_motion",
//                      G_CALLBACK (on_pltbrowser_drag_motion_event),
//                      w);
//    g_signal_connect ((gpointer) w->tree, "key_press_event",
//                      G_CALLBACK (on_pltbrowser_key_press_event),
//                      w);

    w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}
