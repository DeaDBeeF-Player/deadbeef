/*
    GTK hotkeys configuration for Deadbeef player
    Copyright (C) 2009-2012 Alexey Yakovenko and other contributors

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
#include <gtk/gtk.h>
#include "../../gettext.h"
#include "support.h"
#include "gtkui.h"

static void
unescape_forward_slash (const char *src, char *dst, int size) {
    char *start = dst;
    while (*src) {
        if (dst - start >= size - 1) {
            break;
        }
        if (*src == '\\' && *(src+1) == '/') {
            src++;
        }
        *dst++ = *src++;
    }
    *dst = 0;
}

void
prefwin_init_hotkeys (GtkWidget *prefwin) {
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkWidget *actions = lookup_widget (prefwin, "hotkeys_actions");

    // setup hotkeys list
    GtkTreeViewColumn *hk_col1 = gtk_tree_view_column_new_with_attributes (_("Key combination"), gtk_cell_renderer_text_new (), "text", 0, NULL);
    gtk_tree_view_column_set_resizable (hk_col1, TRUE);
    GtkTreeViewColumn *hk_col2 = gtk_tree_view_column_new_with_attributes (_("Action"), gtk_cell_renderer_text_new (), "text", 1, NULL);
    gtk_tree_view_column_set_resizable (hk_col2, TRUE);
    GtkTreeViewColumn *hk_col3 = gtk_tree_view_column_new_with_attributes (_("Context"), gtk_cell_renderer_text_new (), "text", 2, NULL);
    gtk_tree_view_column_set_resizable (hk_col3, TRUE);
    GtkTreeViewColumn *hk_col4 = gtk_tree_view_column_new_with_attributes (_("Is global"), gtk_cell_renderer_text_new (), "text", 3, NULL);
    gtk_tree_view_column_set_resizable (hk_col4, TRUE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (hotkeys), hk_col1);
    gtk_tree_view_append_column (GTK_TREE_VIEW (hotkeys), hk_col2);
    gtk_tree_view_append_column (GTK_TREE_VIEW (hotkeys), hk_col3);
    gtk_tree_view_append_column (GTK_TREE_VIEW (hotkeys), hk_col4);
    GtkListStore *hkstore = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (hotkeys), GTK_TREE_MODEL (hkstore));

    // setup action tree
    GtkTreeViewColumn *hk_act_col1 = gtk_tree_view_column_new_with_attributes (_("Action"), gtk_cell_renderer_text_new (), "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (actions), hk_act_col1);

    // traverse all plugins and collect all exported actions to dropdown
    // column0: title
    // column1: ID (invisible)
    GtkTreeStore *actions_store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter action_main_iter;
    gtk_tree_store_append (actions_store, &action_main_iter, NULL);
    gtk_tree_store_set (actions_store, &action_main_iter, 0, _("Main"), 1, NULL, -1);
    GtkTreeIter action_selection_iter;
    gtk_tree_store_append (actions_store, &action_selection_iter, NULL);
    gtk_tree_store_set (actions_store, &action_selection_iter, 0, _("Selected track(s)"), 1, NULL, -1);
    GtkTreeIter action_playlist_iter;
    gtk_tree_store_append (actions_store, &action_playlist_iter, NULL);
    gtk_tree_store_set (actions_store, &action_playlist_iter, 0, _("Current playlist"), 1, NULL, -1);
    GtkTreeIter action_nowplaying_iter;
    gtk_tree_store_append (actions_store, &action_nowplaying_iter, NULL);
    gtk_tree_store_set (actions_store, &action_nowplaying_iter, 0, _("Now playing"), 1, NULL, -1);

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->get_actions) {
            DB_plugin_action_t *actions = p->get_actions (NULL);
            while (actions) {
                if (actions->name && actions->title) { // only add actions with both the name and the title
                    char title[100];
                    unescape_forward_slash (actions->title, title, sizeof (title));

                    GtkTreeIter iter;
                    if (actions->flags & DB_ACTION_COMMON) {
                        gtk_tree_store_append (actions_store, &iter, &action_main_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, -1);
                    }
                    if (actions->flags & (DB_ACTION_SINGLE_TRACK | DB_ACTION_ALLOW_MULTIPLE_TRACKS | DB_ACTION_CAN_MULTIPLE_TRACKS)) {
                        gtk_tree_store_append (actions_store, &iter, &action_selection_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, -1);
                        gtk_tree_store_append (actions_store, &iter, &action_nowplaying_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, -1);
                        gtk_tree_store_append (actions_store, &iter, &action_playlist_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, -1);
                    }
                }
                else {
//                    fprintf (stderr, "WARNING: action %s/%s from plugin %s is missing name and/or title\n", actions->name, actions->title, p->name);
                }
                actions = actions->next;
            }
        }
    }

    gtk_tree_view_set_model (GTK_TREE_VIEW (actions), GTK_TREE_MODEL (actions_store));
}

void
on_hotkeys_list_cursor_changed         (GtkTreeView     *treeview,
                                        gpointer         user_data)
{

}


void
on_hotkey_add_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_hotkey_remove_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_hotkeys_actions_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{

}


void
on_hotkey_keycombo_changed             (GtkEditable     *editable,
                                        gpointer         user_data)
{

}


void
on_hotkey_is_global_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

}

gboolean
on_hotkey_keycombo_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{

  return FALSE;
}

