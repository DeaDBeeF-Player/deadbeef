/*
    Playlist browser widget plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Alexey Yakovenko

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
#include "../../deadbeef.h"
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
    int last_selected;
    gulong cc_id;
    gulong ri_id;
} w_pltbrowser_t;

static void
on_pltbrowser_row_inserted (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data) {
    w_pltbrowser_t *plt = user_data;
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    if (idx > plt->last_selected) {
        idx--;
    }
    if (idx == plt->last_selected) {
        return;
    }

    deadbeef->plt_move (plt->last_selected, idx);
    plt->last_selected = idx;
    deadbeef->plt_set_curr_idx (idx);
    deadbeef->sendmessage (DB_EV_PLAYLISTSWITCHED, 0, 0, 0);
}

static void
on_pltbrowser_cursor_changed (GtkTreeView *treeview, gpointer user_data) {
    w_pltbrowser_t *w = user_data;
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path) {
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    if (indices) {
        if (indices[0] >= 0) {
            deadbeef->plt_set_curr_idx (indices[0]);
            w->last_selected = indices[0];
        }
        g_free (indices);
    }
}

gboolean
on_pltbrowser_popup_menu (GtkWidget *widget, gpointer user_data);

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
        ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
        GtkTreeIter iter;
        gtk_list_store_append (store, &iter);
        char buf[1000];
        deadbeef->plt_get_title (plt, buf, sizeof (buf));
        gtk_list_store_set (store, &iter, 0, buf, -1);
    }
    if (curr != -1) {
        GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (w->tree), path, NULL, FALSE);
        gtk_tree_path_free (path);
    }
    deadbeef->pl_unlock ();
    w->ri_id = g_signal_connect ((gpointer)store, "row_inserted", G_CALLBACK (on_pltbrowser_row_inserted), w);
    w->cc_id = g_signal_connect ((gpointer)w->tree, "cursor_changed", G_CALLBACK (on_pltbrowser_cursor_changed), w);
    g_signal_connect ((gpointer) w->tree, "popup_menu", G_CALLBACK (on_pltbrowser_popup_menu), NULL);
    return FALSE;
}

static int
pltbrowser_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_PLAYLISTSWITCHED:
        g_idle_add (fill_pltbrowser_cb, w);
        break;
    }
    return 0;
}


static void
w_pltbrowser_init (struct ddb_gtkui_widget_s *w) {
    fill_pltbrowser_cb (w);
}

gboolean
on_pltbrowser_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (gtkui_plugin->w_get_design_mode ()) {
        return FALSE;
    }
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        return on_pltbrowser_popup_menu (widget, user_data);
    }
    return FALSE;
}

gboolean
on_pltbrowser_popup_menu (GtkWidget *widget, gpointer user_data) {
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW(widget), &path, &col);
    if (!path || !col) {
        // reset
        return FALSE;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int plt_idx;
    if (indices) {
        plt_idx = indices[0];
        g_free (indices);
    }
    else {
        return FALSE;
    }

    GtkWidget *menu = gtkui_plugin->create_pltmenu (plt_idx);
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, 0, gtk_get_current_event_time());
    return TRUE;
}

static void
on_pltbrowser_row_activated (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data) {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, 0, 0);
}

static ddb_gtkui_widget_t *
w_pltbrowser_create (void) {
    w_pltbrowser_t *w = malloc (sizeof (w_pltbrowser_t));
    memset (w, 0, sizeof (w_pltbrowser_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_pltbrowser_init;
    w->base.message = pltbrowser_message;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    w->tree = gtk_tree_view_new ();
    gtk_tree_view_set_reorderable (GTK_TREE_VIEW (w->tree), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (w->tree), FALSE);
    gtk_tree_view_set_enable_search (GTK_TREE_VIEW (w->tree), TRUE);
    GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (w->tree));
    gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
    gtk_widget_show (w->tree);

    gtk_container_add (GTK_CONTAINER (scroll), w->tree);

    GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (w->tree), GTK_TREE_MODEL (store));

    w->ri_id = g_signal_connect ((gpointer) store, "row_inserted", G_CALLBACK (on_pltbrowser_row_inserted), w);

    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (w->tree), TRUE);

    GtkCellRenderer *rend1 = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Name"), rend1, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (w->tree), col1);


    w->cc_id = g_signal_connect ((gpointer) w->tree, "cursor_changed",
            G_CALLBACK (on_pltbrowser_cursor_changed),
            w);
    g_signal_connect ((gpointer) w->tree, "event_after",
            G_CALLBACK (on_pltbrowser_button_press_event),
            w);
    g_signal_connect ((gpointer) w->tree, "row_activated",
            G_CALLBACK (on_pltbrowser_row_activated),
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

static DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "pltbrowser_gtk3",
#else
    .plugin.id = "pltbrowser_gtk2",
#endif
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.name = "Playlist browser GTK3",
#else
    .plugin.name = "Playlist browser GTK2",
#endif
    .plugin.descr = "Use View -> Design Mode to add playlist browser into main window",
    .plugin.copyright = 
        "Playlist browser widget plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Alexey Yakovenko\n"
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
