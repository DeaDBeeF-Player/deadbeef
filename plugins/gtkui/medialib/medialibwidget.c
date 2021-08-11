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
    ddb_medialib_plugin_t *plugin;
    ddb_mediasource_source_t source;
    ddb_mediasource_list_selector_t *selectors;
    int listener_id;
    GtkTreeIter root_iter;
    ddb_medialib_item_t *item_tree;
} w_medialib_viewer_t;

enum {
    COL_ITEM,
};

static void
_add_items (w_medialib_viewer_t *mlv, GtkTreeIter *iter, ddb_medialib_item_t *item) {
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));

    for (ddb_medialib_item_t *child_item = item->children; child_item; child_item = child_item->next) {
        GtkTreeIter child;
        gtk_tree_store_append (store, &child, iter);
        gtk_tree_store_set (store, &child, COL_ITEM, child_item->text, -1);

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
    mlv->item_tree = mlv->plugin->plugin.create_item_tree (mlv->source, mlv->selectors[/*index*/ 0], /*self.searchString ? self.searchString.UTF8String : */NULL);

    _add_items(mlv, &mlv->root_iter, mlv->item_tree);
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

    // Root node
    GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (mlv->tree));
    gtk_tree_store_append (store, &mlv->root_iter, NULL);
    gtk_tree_store_set (store, &mlv->root_iter, COL_ITEM, "All Music", -1);

//    GtkTreeIter child;
//    gtk_tree_store_append (store, &child, &toplevel);
//    gtk_tree_store_set (store, &child, COL_ITEM, "Album1", -1);
//    gtk_tree_store_append (store, &child, &toplevel);
//    gtk_tree_store_set (store, &child, COL_ITEM, "Album2", -1);
//    gtk_tree_store_append (store, &child, &toplevel);
//    gtk_tree_store_set (store, &child, COL_ITEM, "Album3", -1);
//

    _reload_content(mlv);
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

ddb_gtkui_widget_t *
w_medialib_viewer_create (void) {
    w_medialib_viewer_t *w = calloc (1, sizeof (w_medialib_viewer_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_medialib_viewer_init;
    w->base.message = w_medialib_viewer_message;
    w->base.initmenu = w_medialib_viewer_initmenu;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->tree = GTK_TREE_VIEW(gtk_tree_view_new ());
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (w->tree), TRUE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), TRUE);
    GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w->tree));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    gtk_widget_show (GTK_WIDGET(w->tree));

    gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(w->tree));

    GtkTreeStore *store = gtk_tree_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);
    add_treeview_column (w, GTK_TREE_VIEW (w->tree), COL_ITEM, 1, 0, _("Item"), 0);

    gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), FALSE);

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
