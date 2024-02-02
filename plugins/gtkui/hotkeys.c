/*
    GTK hotkeys configuration for Deadbeef player
    Copyright (C) 2009-2013 Oleksiy Yakovenko and other contributors

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


// deadbeef core doesn't have any special hotkeys code,
// but we need some common hotkey definition to share between plugins
// so here is the example structure to use when implementing hotkeys support
/*
// corresponding line in the config file:
// hotkey.keyX "key combination" CONTEXT IS_GLOBAL ACTION_ID
// action contexts are defined in deadbeef.h
//
// example:
// hotkey.key1 "Super+n" 0 1 playback_random
// this would mean "execute playback_random action when Super+n is pressed globally"
//
// context can be main, selection, playlist or nowplaying
// TODO: do we need anything else, like widget contexts?..
typedef struct
{
    char *key_combination;
    int context; // NULL, selection, playlist, nowplaying
    DB_plugin_action_t *action;
    unsigned is_global : 1;
} ddb_hotkey_t;
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "../../gettext.h"
#include "support.h"
#include "gtkui.h"
#include "interface.h"
#include "../libparser/parser.h"
#include "../hotkeys/hotkeys.h"
#ifndef __APPLE__
#ifndef __MINGW32__
#include <X11/Xlib.h> // only for the KeySym type
#endif
#endif
#include "hotkeys.h"
#include <deadbeef/strdupa.h>

int gtkui_hotkeys_changed = 0;

void
on_hotkeys_actions_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data);

static GtkWidget *prefwin;
static guint last_accel_key = 0;
static guint last_accel_mask = 0;
static const char *ctx_names[DDB_ACTION_CTX_COUNT];

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

static void
prettify_forward_slash (const char *src, char *dst, int size) {
    const char arrow[] = " → ";
    size_t larrow = strlen (arrow);
    while (*src && size > 1) {
        if (*src == '\\' && *(src+1) == '/') {
            src++;
        }
        else if (*src == '/' && size > larrow) {
            memcpy (dst, arrow, larrow);
            src++;
            dst += larrow;
            size -= larrow;
            continue;
        }
        *dst++ = *src++;
        size--;
    }
    *dst = 0;
}

static const char *
get_display_action_title (const char *title) {
    const char *t = title + strlen (title) - 1;
    while (t > title) {
        if (*t != '/' || *(t-1) == '\\') {
            t--;
            continue;
        }
        t++;
        break;
    }
    return t;
}

DB_plugin_action_t *
find_action_by_name (const char *command) {
    // find action with this name, and add to list
    DB_plugin_action_t *actions = NULL;
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->get_actions) {
            actions = p->get_actions (NULL);
            while (actions) {
                if (actions->name && actions->title && !strcasecmp (actions->name, command)) {
                    break; // found
                }
                actions = actions->next;
            }
            if (actions) {
                break;
            }
        }
    }
    return actions;
}

static int
hotkeys_load (void) {
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkListStore *hkstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys)));
    gtk_list_store_clear (hkstore);
    int n_items = 0;
    DB_conf_item_t *item = deadbeef->conf_find ("hotkey.", NULL);
    while (item) {
        char token[MAX_TOKEN];
        char keycombo[MAX_TOKEN];
        int ctx;
        int isglobal;
        DB_plugin_action_t *action;
        const char *script = item->value;
        if ((script = gettoken (script, keycombo)) == 0) {
            goto out;
        }
        if ((script = gettoken (script, token)) == 0) {
            goto out;
        }
        ctx = atoi (token);
        if (ctx < 0 || ctx >= DDB_ACTION_CTX_COUNT) {
            goto out;
        }
        if ((script = gettoken (script, token)) == 0) {
            goto out;
        }
        isglobal = atoi (token);
        if ((script = gettoken (script, token)) == 0) {
            goto out;
        }
        action = find_action_by_name (token);
        if (!action) {
            goto out;
        }

        GtkTreeIter iter;
        gtk_list_store_append (hkstore, &iter);

        const char *t = get_display_action_title (action->title);
        char title[100];
        unescape_forward_slash (t, title, sizeof (title));
        gtk_list_store_set (hkstore, &iter, 0, keycombo, 1, title, 2, ctx_names[ctx], 3, isglobal, 4, action->name, 5, ctx, -1);
        n_items++;

out:
        item = deadbeef->conf_find ("hotkey.", item);
    }
    return n_items;
}

static void
hotkeys_save (void) {
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkListStore *hkstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys)));
    deadbeef->conf_remove_items ("hotkey.key");

    GtkTreeIter iter;
    gboolean res = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (hkstore), &iter);
    int i = 1;
    while (res) {
        GValue keycombo = {0,}, action = {0,}, context = {0,}, global = {0,};
        gtk_tree_model_get_value (GTK_TREE_MODEL (hkstore), &iter, 0, &keycombo);
        gtk_tree_model_get_value (GTK_TREE_MODEL (hkstore), &iter, 4, &action);
        gtk_tree_model_get_value (GTK_TREE_MODEL (hkstore), &iter, 5, &context);
        gtk_tree_model_get_value (GTK_TREE_MODEL (hkstore), &iter, 3, &global);
        char key[100];
        snprintf (key, sizeof (key), "hotkey.key%02d", i);
        char value[1000];
        snprintf (value, sizeof (value), "\"%s\" %d %d %s", g_value_get_string (&keycombo), g_value_get_int (&context), g_value_get_boolean (&global), g_value_get_string (&action));
        deadbeef->conf_set_str (key, value);

        res = gtk_tree_model_iter_next (GTK_TREE_MODEL (hkstore), &iter);
        i++;
    }
    // FIXME: should be done in a more generic sendmessage
    DB_plugin_t *hkplug = deadbeef->plug_get_for_id ("hotkeys");
    if (hkplug) {
        ((DB_hotkeys_plugin_t *)hkplug)->reset ();
    }
}

const char *
action_tree_append (const char *title, GtkTreeStore *store, GtkTreeIter *root_iter, GtkTreeIter *iter) {
    char *t = strdupa (title);
    char *p = t;
    GtkTreeIter i;
    GtkTreeIter newroot;
    for (;;) {
        char *s = strchr (p, '/');
        // find unescaped forward slash
        if (s == p) {
            p++;
            continue;
        }
        if (s && s > p && *(s-1) == '\\') {
            p = s + 1;
            continue;
        }
        if (!s) {
            break;
        }
        *s = 0;
        // find iter in the current root with name==p
        gboolean res = gtk_tree_model_iter_children (GTK_TREE_MODEL (store), &i, root_iter);
        if (!res) {
            gtk_tree_store_append (store, &i, root_iter);
            gtk_tree_store_set (store, &i, 0, p, 1, NULL, 2, -1, -1);
            memcpy (&newroot, &i, sizeof (GtkTreeIter));
            root_iter = &newroot;
        }
        else {
            int found = 0;
            do {
                GValue val = {0,};
                gtk_tree_model_get_value (GTK_TREE_MODEL (store), &i, 0, &val);
                const char *n = g_value_get_string (&val);
                if (n && !strcmp (n, p)) {
                    memcpy (&newroot, &i, sizeof (GtkTreeIter));
                    root_iter = &newroot;
                    found = 1;
                    break;
                }
            } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &i));
            if (!found) {
                gtk_tree_store_append (store, &i, root_iter);
                gtk_tree_store_set (store, &i, 0, p, 1, NULL, 2, -1, -1);
                memcpy (&newroot, &i, sizeof (GtkTreeIter));
                root_iter = &newroot;
            }
        }

        p = s+1;
    }
    gtk_tree_store_append (store, iter, root_iter);
    return get_display_action_title (title);
}

typedef struct {
    const char *name;
    int ctx;
    GtkWidget *treeview;
} actionbinding_t;

static gboolean
set_current_action (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    GValue val = {0,}, ctx_val = {0,};
    gtk_tree_model_get_value (model, iter, 1, &val);
    gtk_tree_model_get_value (model, iter, 2, &ctx_val);
    actionbinding_t *binding = data;
    const char *name = g_value_get_string (&val);
    if (name && binding->name && !strcmp (binding->name, name) && binding->ctx == g_value_get_int (&ctx_val)) {
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (binding->treeview), path);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (binding->treeview), path, NULL, FALSE);
        return TRUE;
    }
    return FALSE;
}

void
init_action_tree (GtkWidget *actions, const char *act, int ctx) {
    GtkTreeViewColumn *hk_act_col1 = gtk_tree_view_column_new_with_attributes (_("Action"), gtk_cell_renderer_text_new (), "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (actions), hk_act_col1);

    // traverse all plugins and collect all exported actions to treeview
    // column0: title
    // column1: ID (invisible)
    // column2: ctx (invisible
    GtkTreeStore *actions_store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    GtkTreeIter action_main_iter;
    gtk_tree_store_append (actions_store, &action_main_iter, NULL);
    gtk_tree_store_set (actions_store, &action_main_iter, 0, _("Main"), -1);

    GtkTreeIter action_selection_iter;
    gtk_tree_store_append (actions_store, &action_selection_iter, NULL);
    gtk_tree_store_set (actions_store, &action_selection_iter, 0, _("Selected track(s)"), -1);
    GtkTreeIter action_playlist_iter;
    gtk_tree_store_append (actions_store, &action_playlist_iter, NULL);
    gtk_tree_store_set (actions_store, &action_playlist_iter, 0, _("Current playlist"), -1);
    GtkTreeIter action_nowplaying_iter;
    gtk_tree_store_append (actions_store, &action_nowplaying_iter, NULL);
    gtk_tree_store_set (actions_store, &action_nowplaying_iter, 0, _("Now playing"), -1);

    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    for (int i = 0; plugins[i]; i++) {
        DB_plugin_t *p = plugins[i];
        if (p->get_actions) {
            DB_plugin_action_t *actions = p->get_actions (NULL);
            while (actions) {
                if (actions->name && actions->title) { // only add actions with both the name and the title
                    char title[100];

                    GtkTreeIter iter;
                    const char *t;
                    if (actions->flags & DB_ACTION_COMMON) {
                        t = action_tree_append (actions->title, actions_store, &action_main_iter, &iter);
                        unescape_forward_slash (t, title, sizeof (title));
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_MAIN, -1);
                    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                    if (actions->flags & (DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_CAN_MULTIPLE_TRACKS)) {
#pragma GCC diagnostic pop
                        t = action_tree_append (actions->title, actions_store, &action_selection_iter, &iter);
                        unescape_forward_slash (t, title, sizeof (title));
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_SELECTION, -1);
                        if (!(actions->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST)) {
                            t = action_tree_append (actions->title, actions_store, &action_playlist_iter, &iter);
                            unescape_forward_slash (t, title, sizeof (title));
                            gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_PLAYLIST, -1);
                        }
                        t = action_tree_append (actions->title, actions_store, &action_nowplaying_iter, &iter);
                        unescape_forward_slash (t, title, sizeof (title));
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_NOWPLAYING, -1);
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

    if (act && ctx != -1) {
        actionbinding_t binding = {
            .name = act,
            .ctx = ctx,
            .treeview = actions
        };
        gtk_tree_model_foreach (GTK_TREE_MODEL (actions_store), set_current_action, (void*)&binding);
    }
}

void
on_hotkeys_actions_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &path, NULL);
    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys));
    GtkTreeIter iter;
    if (!path || !gtk_tree_model_get_iter (model, &iter, path)) {
        return;
    }
    // get action name from iter
    GValue val_name = {0,}, val_ctx = {0,};
    gtk_tree_model_get_value (model, &iter, 4, &val_name);
    gtk_tree_model_get_value (model, &iter, 5, &val_ctx);
    const char *act = g_value_get_string (&val_name);
    int ctx = g_value_get_int (&val_ctx);

    GtkWidget *dlg = create_select_action ();
    GtkWidget *treeview = lookup_widget (dlg, "actions");
    init_action_tree (treeview, act, ctx);
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        on_hotkeys_actions_cursor_changed (GTK_TREE_VIEW (treeview), NULL);

        GtkTreePath *path;
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (treeview), &path, NULL);
        GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (treeview));
        GtkTreeIter iter;
        const char *name = NULL;
        int ctx = -1;
        if (path && gtk_tree_model_get_iter (model, &iter, path)) {
            GValue val = {0,};
            gtk_tree_model_get_value (model, &iter, 1, &val);
            name = g_value_get_string (&val);
            GValue val_ctx = {0,};
            gtk_tree_model_get_value (model, &iter, 2, &val_ctx);
            ctx = g_value_get_int (&val_ctx);
        }
        set_button_action_label (name, ctx, lookup_widget (prefwin, "hotkeys_actions"));
    }
    gtk_widget_destroy (dlg);
}

void
prefwin_init_hotkeys (GtkWidget *_prefwin) {
    DB_plugin_t *hkplug = deadbeef->plug_get_for_id ("hotkeys");
    if (!hkplug) {
        return;
    }

    gtkui_hotkeys_changed = 0;
    ctx_names[DDB_ACTION_CTX_MAIN] = _("Main");
    ctx_names[DDB_ACTION_CTX_SELECTION] = _("Selection");
    ctx_names[DDB_ACTION_CTX_PLAYLIST] = _("Playlist");
    ctx_names[DDB_ACTION_CTX_NOWPLAYING] = _("Now playing");

    prefwin = _prefwin;
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");

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
    // column0: keycombo string
    // column1: action title
    // column2: context title
    // column3: is_global
    // column4: action title id (hidden)
    // column5: context id (hidden)
    GtkListStore *hkstore = gtk_list_store_new (6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);

    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_actions"), FALSE);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), FALSE);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_set_key"), FALSE);

    gtk_tree_view_set_model (GTK_TREE_VIEW (hotkeys), GTK_TREE_MODEL (hkstore));

    (void)hotkeys_load ();
}

void
set_button_action_label (const char *act, int action_ctx, GtkWidget *button) {
    if (act && action_ctx >= 0) {
        DB_plugin_action_t *action = find_action_by_name (act);
        if (action) {
            const char *ctx_str = NULL;
            switch (action_ctx) {
            case DDB_ACTION_CTX_MAIN:
                break;
            case DDB_ACTION_CTX_SELECTION:
                ctx_str = _("Selected tracks");
                break;
            case DDB_ACTION_CTX_PLAYLIST:
                ctx_str = _("Tracks in current playlist");
                break;
            case DDB_ACTION_CTX_NOWPLAYING:
                ctx_str = _("Currently playing track");
                break;
            }
            char s[200];
            snprintf (s, sizeof (s), "%s%s%s", ctx_str ? ctx_str : "", ctx_str ? " ⇒ ": "", action->title);
            char s_fixed[200];
            prettify_forward_slash (s, s_fixed, sizeof (s_fixed));

            gtk_button_set_label (GTK_BUTTON (button), s_fixed);
            return;
        }
    }

    gtk_button_set_label (GTK_BUTTON (button), _("<Not set>"));
}


void
on_hotkeys_list_cursor_changed         (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    gtk_tree_view_get_cursor (treeview, &path, NULL);
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);
    GtkTreeIter iter;
    int changed = gtkui_hotkeys_changed;
    if (path && gtk_tree_model_get_iter (model, &iter, path)) {
        GtkWidget *actions = lookup_widget (prefwin, "hotkeys_actions");
        gtk_widget_set_sensitive (actions, TRUE);
        // get action name from iter
        GValue val_name = {0,}, val_ctx = {0,};
        gtk_tree_model_get_value (model, &iter, 4, &val_name);
        gtk_tree_model_get_value (model, &iter, 5, &val_ctx);
        const char *name = g_value_get_string (&val_name);

        set_button_action_label (name, g_value_get_int (&val_ctx), actions);

        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), TRUE);
        GValue val_isglobal = {0,};
        gtk_tree_model_get_value (model, &iter, 3, &val_isglobal);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "hotkey_is_global")), g_value_get_boolean (&val_isglobal));
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_set_key"), TRUE);
        GValue val_keycombo = {0,};
        gtk_tree_model_get_value (model, &iter, 0, &val_keycombo);
        const char *keycombo = g_value_get_string (&val_keycombo);
        gtk_button_set_label (GTK_BUTTON (lookup_widget (prefwin, "hotkeys_set_key")), keycombo ? keycombo : "");
    }
    else {
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_actions"), FALSE);
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "hotkey_is_global")), FALSE);
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_set_key"), FALSE);
        gtk_button_set_label (GTK_BUTTON (lookup_widget (prefwin, "hotkeys_set_key")), _("<Not set>"));
    }
    if (path) {
        gtk_tree_path_free (path);
    }
    gtkui_hotkeys_changed = changed;
}


void
on_hotkey_add_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkListStore *hkstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys)));
    GtkTreeIter iter;
    gtk_list_store_append (hkstore, &iter);
    gtk_list_store_set (hkstore, &iter, 0, _("<Not set>"), 1, _("<Not set>"), 2, _("<Not set>"), 3, 0, 4, NULL, 5, -1, -1);
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (hkstore), &iter);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (hotkeys), path, NULL, FALSE);
    gtk_tree_path_free (path);
    gtk_widget_grab_focus (hotkeys);
    gtkui_hotkeys_changed = 1;
}


void
on_hotkey_remove_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkTreePath *path;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &path, NULL);
    GtkListStore *hkstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys)));
    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (hkstore), &iter, path);
    gtk_list_store_remove (hkstore, &iter);
    set_button_action_label (NULL, 0, lookup_widget (prefwin, "hotkeys_actions"));
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "hotkey_is_global")), FALSE);
    gtk_button_set_label (GTK_BUTTON (lookup_widget (prefwin, "hotkeys_set_key")), _("<Not set>"));
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_actions"), FALSE);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), FALSE);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_set_key"), FALSE);
    gtkui_hotkeys_changed = 1;
}


void
on_hotkeys_actions_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    gtk_tree_view_get_cursor (treeview, &path, NULL);
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);
    GtkTreeIter iter;
    if (path && gtk_tree_model_get_iter (model, &iter, path)) {
        GValue val = {0,};
        gtk_tree_model_get_value (model, &iter, 1, &val);
        const gchar *name = g_value_get_string (&val);
        DB_plugin_action_t *action = NULL;
        int ctx = 0;
        if (name) {
            action = find_action_by_name (name);
            GValue val_ctx = {0,};
            gtk_tree_model_get_value (model, &iter, 2, &val_ctx);
            ctx = g_value_get_int (&val_ctx);
        }
        // update the tree
        {
            GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
            GtkTreePath *path;
            gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &path, NULL);
            GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys));
            GtkTreeIter iter;
            if (path && gtk_tree_model_get_iter (model, &iter, path)) {
                if (action) {
                    const char *t = get_display_action_title (action->title);
                    char title[100];
                    unescape_forward_slash (t, title, sizeof (title));
                    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, title, 4, action->name, 5, ctx, 2, ctx_names[ctx], -1);
                }
                else {
                    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, _("<Not set>"), 4, NULL, 2, _("<Not set>"), -1);
                }
            }
        }
    }
}


void
on_hotkey_is_global_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    // update the tree
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkTreePath *path;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &path, NULL);
    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys));
    GtkTreeIter iter;
    if (path && gtk_tree_model_get_iter (model, &iter, path)) {
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 3, gtk_toggle_button_get_active (togglebutton), -1);
    }
    gtkui_hotkeys_changed = 1;
}

typedef struct {
    const char *name;
    int keysym;
} xkey_t;

#define KEY(kname, kcode) { .name=kname, .keysym=kcode },

static const xkey_t keys[] = {
    #include "../hotkeys/keysyms.inc"
};

static const char *
get_name_for_keycode (int keycode) {
    for (int i = 0; keys[i].name; i++) {
        if (keycode == keys[i].keysym) {
            return keys[i].name;
        }
    }
    return NULL;
}


int gtkui_hotkey_grabbing = 0;

static void
get_keycombo_string (guint accel_key, GdkModifierType accel_mods, char *new_value) {
    // build value
    new_value[0] = 0;
    if (!accel_key) {
        strcpy (new_value, _("<Not set>"));
        return;
    }
    if (accel_mods & GDK_SHIFT_MASK) {
        strcat (new_value, "Shift ");
    }
    if (accel_mods & GDK_CONTROL_MASK) {
        strcat (new_value, "Ctrl ");
    }
    if (accel_mods & GDK_SUPER_MASK) {
        strcat (new_value, "Super ");
    }
    if (accel_mods & GDK_MOD1_MASK) {
        strcat (new_value, "Alt ");
    }

    // translate numlock keycodes into non-numlock codes
    switch (accel_key) {
    case GDK_KP_0:
        accel_key = GDK_KP_Insert;
        break;
    case GDK_KP_1:
        accel_key = GDK_KP_End;
        break;
    case GDK_KP_2:
        accel_key = GDK_KP_Down;
        break;
    case GDK_KP_3:
        accel_key = GDK_KP_Page_Down;
        break;
    case GDK_KP_4:
        accel_key = GDK_KP_Left;
        break;
    case GDK_KP_6:
        accel_key = GDK_KP_Right;
        break;
    case GDK_KP_7:
        accel_key = GDK_KP_Home;
        break;
    case GDK_KP_8:
        accel_key = GDK_KP_Up;
        break;
    case GDK_KP_9:
        accel_key = GDK_KP_Page_Up;
        break;
    }

    const char *name = get_name_for_keycode (accel_key);
    if (!name) {
        strcpy (new_value, _("<Not set>"));
        return;
    }
    strcat (new_value, name);
}

static GtkWidget *hotkey_grabber_button;
gboolean
on_hotkeys_set_key_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    widget = hotkey_grabber_button;
    GdkModifierType accel_mods = 0;
    guint accel_key;
    GdkModifierType consumed_modifiers;
    GdkDisplay *display;
    GtkTreePath *curpath;
    GtkTreeIter iter;

    if (!gtkui_hotkey_grabbing) {
        return FALSE;
    }

    display = gtk_widget_get_display (widget);

    if (event->is_modifier)
        return TRUE;

    accel_mods = event->state & gtk_accelerator_get_default_mod_mask ();

    gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (display),
            event->hardware_keycode, accel_mods & (~GDK_SHIFT_MASK),
            0, &accel_key, NULL, NULL, &consumed_modifiers);

    if (accel_key == GDK_ISO_Left_Tab) 
        accel_key = GDK_Tab;


    /* Filter consumed modifiers
    */
    accel_mods &= ~(consumed_modifiers&~GDK_SHIFT_MASK);

    char name[1000];
    gtk_button_set_label (GTK_BUTTON (widget), _(""));

    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys));

    // check if this key already registered
    get_keycombo_string (accel_key, accel_mods, name);

    gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &curpath, NULL);
    gboolean res = gtk_tree_model_get_iter_first (model, &iter);
    while (res) {
        GtkTreePath *iterpath = gtk_tree_model_get_path (model, &iter);

        if (!curpath || gtk_tree_path_compare (iterpath, curpath)) {
            GValue keycombo = {0,};
            gtk_tree_model_get_value (model, &iter, 0, &keycombo);
            const char *val = g_value_get_string (&keycombo);
            if (val && !strcmp (val, name)) {
                gtk_tree_path_free (iterpath);
                break;
            }
        }
        gtk_tree_path_free (iterpath);

        res = gtk_tree_model_iter_next (model, &iter);
    }

    if (res) {
        // duplicate
        gtk_button_set_label (GTK_BUTTON (widget), _("Duplicate key combination!"));
        gtk_widget_error_bell (widget);
        goto out;
    }

    last_accel_key = accel_key;
    last_accel_mask = accel_mods;
    get_keycombo_string (last_accel_key, last_accel_mask, name);
    gtk_button_set_label (GTK_BUTTON (widget), name);

    // update the tree
    if (curpath && gtk_tree_model_get_iter (model, &iter, curpath)) {
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, name, -1);
    }

out:
    if (curpath) {
        gtk_tree_path_free (curpath);
    }
    gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
    gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
    gtkui_hotkey_grabbing = 0;
    gtkui_hotkeys_changed = 1;
    return TRUE;
}

static void
hotkey_grab_focus (GtkWidget *widget) {
    GdkDisplay *display = gtk_widget_get_display (widget);
    if (gtkui_hotkey_grabbing) {
        return;
    }
    gtkui_hotkey_grabbing = 0;
    if (GDK_GRAB_SUCCESS != gdk_keyboard_grab (gtk_widget_get_window (widget), FALSE, GDK_CURRENT_TIME)) {
        return;
    }

    if (gdk_pointer_grab (gtk_widget_get_window (widget), FALSE,
                GDK_BUTTON_PRESS_MASK,
                NULL, NULL,
                GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
    {
        gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
        return;
    }
    gtk_button_set_label (GTK_BUTTON (widget), _("New key combination..."));
    gtkui_hotkey_grabbing = 1;

    // disable the window accelerators temporarily
    hotkey_grabber_button = widget;
}

void
on_hotkeys_set_key_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
    hotkey_grab_focus (GTK_WIDGET (button));
}

void
on_hotkeys_apply_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    hotkeys_save ();
    gtkui_hotkeys_changed = 0;
}


void
on_hotkeys_revert_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    hotkeys_load ();
    gtkui_hotkeys_changed = 0;
}

void
on_hotkeys_defaults_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (prefwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("All your custom-defined hotkeys will be lost."));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("This operation cannot be undone. Proceed?"));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    if (response != GTK_RESPONSE_YES) {
        return;
    }
    gtkui_set_default_hotkeys ();
    hotkeys_load ();
    gtkui_hotkeys_changed = 0;
}

void
gtkui_set_default_hotkeys (void) {
    deadbeef->conf_remove_items ("hotkey.key");
    deadbeef->conf_set_str ("hotkey.key01", "\"Ctrl f\" 0 0 find");
    deadbeef->conf_set_str ("hotkey.key02", "\"Ctrl o\" 0 0 open_files");
    deadbeef->conf_set_str ("hotkey.key03", "\"Ctrl q\" 0 0 quit");
    deadbeef->conf_set_str ("hotkey.key04", "\"Ctrl n\" 0 0 new_playlist");
    deadbeef->conf_set_str ("hotkey.key05", "\"Ctrl a\" 0 0 select_all");
    deadbeef->conf_set_str ("hotkey.key06", "\"Escape\" 0 0 deselect_all");
    deadbeef->conf_set_str ("hotkey.key07", "\"Ctrl m\" 0 0 toggle_stop_after_current");
    deadbeef->conf_set_str ("hotkey.key08", "\"Ctrl j\" 0 0 jump_to_current_track");
    deadbeef->conf_set_str ("hotkey.key09", "\"F1\" 0 0 help");
    deadbeef->conf_set_str ("hotkey.key10", "\"Delete\" 1 0 remove_from_playlist");
    deadbeef->conf_set_str ("hotkey.key11", "\"Ctrl w\" 0 0 remove_current_playlist");
    deadbeef->conf_set_str ("hotkey.key13", "\"Alt Return\" 1 0 track_properties");
    deadbeef->conf_set_str ("hotkey.key14", "\"Return\" 0 0 play");
    deadbeef->conf_set_str ("hotkey.key15", "\"Ctrl p\" 0 0 toggle_pause");
    deadbeef->conf_set_str ("hotkey.key16", "\"Alt 1\" 0 0 playlist1");
    deadbeef->conf_set_str ("hotkey.key17", "\"Alt 2\" 0 0 playlist2");
    deadbeef->conf_set_str ("hotkey.key18", "\"Alt 3\" 0 0 playlist3");
    deadbeef->conf_set_str ("hotkey.key19", "\"Alt 4\" 0 0 playlist4");
    deadbeef->conf_set_str ("hotkey.key20", "\"Alt 5\" 0 0 playlist5");
    deadbeef->conf_set_str ("hotkey.key21", "\"Alt 6\" 0 0 playlist6");
    deadbeef->conf_set_str ("hotkey.key22", "\"Alt 7\" 0 0 playlist7");
    deadbeef->conf_set_str ("hotkey.key23", "\"Alt 8\" 0 0 playlist8");
    deadbeef->conf_set_str ("hotkey.key24", "\"Alt 9\" 0 0 playlist9");
    deadbeef->conf_set_str ("hotkey.key25", "\"Alt 0\" 0 0 playlist10");
    deadbeef->conf_set_str ("hotkey.key26", "z 0 0 prev");
    deadbeef->conf_set_str ("hotkey.key27", "x 0 0 play");
    deadbeef->conf_set_str ("hotkey.key28", "c 0 0 toggle_pause");
    deadbeef->conf_set_str ("hotkey.key29", "v 0 0 stop");
    deadbeef->conf_set_str ("hotkey.key30", "b 0 0 next");
    deadbeef->conf_set_str ("hotkey.key31", "n 0 0 playback_random");
    deadbeef->conf_set_str ("hotkey.key32", "\"Ctrl k\" 0 0 toggle_stop_after_album");
    deadbeef->conf_set_str ("hotkey.key33", "\"Ctrl z\" 0 0 undo");
    deadbeef->conf_set_str ("hotkey.key34", "\"Ctrl Shift z\" 0 0 redo");
    deadbeef->conf_save ();
}

void
gtkui_import_0_5_global_hotkeys (void) {
    int n = 40;
    deadbeef->conf_lock ();
    DB_conf_item_t *item = deadbeef->conf_find ("hotkeys.key", NULL);
    while (item) {
        char *val = strdupa (item->value);
        char *colon = strchr (val, ':');
        if (colon) {
            *colon++ = 0;
            while (*colon && *colon == ' ') {
                colon++;
            }
            if (*colon) {
                char newkey[100];
                char newval[100];
                snprintf (newkey, sizeof (newkey), "hotkey.key%02d", n);
                snprintf (newval, sizeof (newval), "\"%s\" 0 1 %s", val, colon);
                deadbeef->conf_set_str (newkey, newval);
                n++;
            }
        }
        item = deadbeef->conf_find ("hotkeys.", item);
    }
    deadbeef->conf_unlock ();
}
