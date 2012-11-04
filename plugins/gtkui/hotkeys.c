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
#include "parser.h"
#include "../hotkeys/hotkeys.h"
#include <X11/Xlib.h> // only for the KeySym type

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

static DB_plugin_action_t *
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

static void
hotkeys_load (void) {
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkListStore *hkstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys)));
    gtk_list_store_clear (hkstore);
    int has_items = 0;
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

        gtk_list_store_set (hkstore, &iter, 0, keycombo, 1, action->title, 2, ctx_names[ctx], 3, isglobal, 4, action->name, 5, ctx, -1);
        has_items = 1;

out:
        item = deadbeef->conf_find ("hotkey.", item);
    }
}

static void
hotkeys_save (void) {
    GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
    GtkListStore *hkstore = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys)));
//    deadbeef->conf_remove_items ("hotkey.key");

    GtkTreePath *path = gtk_tree_path_new_first ();
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
        snprintf (key, sizeof (key), "hotkey.key%d", i);
        char value[1000];
        snprintf (value, sizeof (value), "\"%s\" %d %d %s", g_value_get_string (&keycombo), g_value_get_int (&context), g_value_get_boolean (&global), g_value_get_string (&action));
        deadbeef->conf_set_str (key, value);

        res = gtk_tree_model_iter_next (GTK_TREE_MODEL (hkstore), &iter);
        i++;
    }
}

void

prefwin_init_hotkeys (GtkWidget *_prefwin) {
    ctx_names[DDB_ACTION_CTX_MAIN] = _("Main");
    ctx_names[DDB_ACTION_CTX_SELECTION] = _("Selection");
    ctx_names[DDB_ACTION_CTX_PLAYLIST] = _("Playlist");
    ctx_names[DDB_ACTION_CTX_NOWPLAYING] = _("Now playing");

    prefwin = _prefwin;
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
    // column0: keycombo string
    // column1: action title
    // column2: context title
    // column3: is_global
    // column4: action title id (hidden)
    // column5: context id (hidden)
    GtkListStore *hkstore = gtk_list_store_new (6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_INT);

    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_actions"), FALSE);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), FALSE);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_keycombo"), FALSE);

    gtk_tree_view_set_model (GTK_TREE_VIEW (hotkeys), GTK_TREE_MODEL (hkstore));

    hotkeys_load ();

    // setup action tree
    GtkTreeViewColumn *hk_act_col1 = gtk_tree_view_column_new_with_attributes (_("Action"), gtk_cell_renderer_text_new (), "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (actions), hk_act_col1);

    // traverse all plugins and collect all exported actions to dropdown
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
                    unescape_forward_slash (actions->title, title, sizeof (title));

                    GtkTreeIter iter;
                    if (actions->flags & DB_ACTION_COMMON) {
                        gtk_tree_store_append (actions_store, &iter, &action_main_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_MAIN, -1);
                    }
                    if (actions->flags & (DB_ACTION_SINGLE_TRACK | DB_ACTION_ALLOW_MULTIPLE_TRACKS | DB_ACTION_CAN_MULTIPLE_TRACKS)) {
                        gtk_tree_store_append (actions_store, &iter, &action_selection_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_SELECTION, -1);
                        gtk_tree_store_append (actions_store, &iter, &action_playlist_iter);
                        gtk_tree_store_set (actions_store, &iter, 0, title, 1, actions->name, 2, DDB_ACTION_CTX_PLAYLIST, -1);
                        gtk_tree_store_append (actions_store, &iter, &action_nowplaying_iter);
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

    GtkTreePath *path = gtk_tree_path_new_first ();
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (hotkeys), path, NULL, FALSE);
    gtk_tree_path_free (path);
}

typedef struct {
    const char *name;
    int ctx;
} actionbinding_t;

static gboolean
set_current_action (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    GValue val = {0,}, ctx_val = {0,};
    gtk_tree_model_get_value (model, iter, 1, &val);
    gtk_tree_model_get_value (model, iter, 2, &ctx_val);
    actionbinding_t *binding = data;
    const char *name = g_value_get_string (&val);
    if (name && binding->name && !strcmp (binding->name, name) && binding->ctx == g_value_get_int (&ctx_val)) {
        gtk_tree_view_expand_to_path (GTK_TREE_VIEW (lookup_widget (prefwin, "hotkeys_actions")), path);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (lookup_widget (prefwin, "hotkeys_actions")), path, NULL, FALSE);
        return TRUE;
    }
    return FALSE;
}

void
on_hotkeys_list_cursor_changed         (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    gtk_tree_view_get_cursor (treeview, &path, NULL);
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);
    GtkTreeIter iter;
    if (path && gtk_tree_model_get_iter (model, &iter, path)) {
        GtkWidget *actions = lookup_widget (prefwin, "hotkeys_actions");
        gtk_widget_set_sensitive (actions, TRUE);
        // get action name from iter
        GValue val_name = {0,}, val_ctx = {0,};
        gtk_tree_model_get_value (model, &iter, 4, &val_name);
        gtk_tree_model_get_value (model, &iter, 5, &val_ctx);
        const char *name = g_value_get_string (&val_name);
        // find in the action list and set as current
        GtkTreeModel *actmodel = gtk_tree_view_get_model (GTK_TREE_VIEW (actions));
        actionbinding_t binding = {
            .name = name,
            .ctx = g_value_get_int (&val_ctx)
        };
        gtk_tree_model_foreach (actmodel, set_current_action, (void*)&binding);

        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), TRUE);
        GValue val_isglobal = {0,};
        gtk_tree_model_get_value (model, &iter, 3, &val_isglobal);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "hotkey_is_global")), g_value_get_boolean (&val_isglobal));
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_keycombo"), TRUE);
        GValue val_keycombo = {0,};
        gtk_tree_model_get_value (model, &iter, 0, &val_keycombo);
        const char *keycombo = g_value_get_string (&val_keycombo);
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (prefwin, "hotkey_keycombo")), keycombo ? keycombo : "");
    }
    else {
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkeys_actions"), FALSE);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (lookup_widget (prefwin, "hotkeys_actions")), NULL, NULL, FALSE);
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_is_global"), FALSE);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "hotkey_is_global")), FALSE);
        gtk_widget_set_sensitive (lookup_widget (prefwin, "hotkey_keycombo"), FALSE);
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (prefwin, "hotkey_keycombo")), "");
    }
    if (path) {
        gtk_tree_path_free (path);
    }
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
                    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, action->title, 4, action->name, 5, ctx, 2, ctx_names[ctx], -1);
                }
                else {
                    gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, NULL, 4, NULL, 2, 0, -1);
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
    {
        GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
        GtkTreePath *path;
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &path, NULL);
        GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys));
        GtkTreeIter iter;
        if (path && gtk_tree_model_get_iter (model, &iter, path)) {
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, 3, gtk_toggle_button_get_active (togglebutton), -1);
        }
    }
}

typedef struct {
    const char *name;
    KeySym keysym;
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


static int grabbed = 0;

static void
get_keycombo_string (guint accel_key, GdkModifierType accel_mods, char *new_value) {
    // build value
    new_value[0] = 0;
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
    strcat (new_value, name);
}

gboolean
on_hotkey_keycombo_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    GdkModifierType accel_mods = 0;
    guint accel_key;
    gchar *path;
    gboolean edited;
    gboolean cleared;
    GdkModifierType consumed_modifiers;
    GdkDisplay *display;

    if (!grabbed) {
        return TRUE;
    }

    display = gtk_widget_get_display (widget);

    if (event->is_modifier)
        return TRUE;

    edited = FALSE;
    cleared = FALSE;

    gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (display),
            event->hardware_keycode,
            event->state,
            event->group,
            NULL, NULL, NULL, &consumed_modifiers);

    accel_key = gdk_keyval_to_lower (event->keyval);
    if (accel_key == GDK_ISO_Left_Tab) 
        accel_key = GDK_Tab;

    accel_mods = event->state & gtk_accelerator_get_default_mod_mask ();

    /* Filter consumed modifiers 
    */
    accel_mods &= ~consumed_modifiers;

    /* Put shift back if it changed the case of the key, not otherwise.
    */
    if (accel_key != event->keyval)
        accel_mods |= GDK_SHIFT_MASK;

    char name[1000];
    gtk_entry_set_text (GTK_ENTRY (widget), _(""));
    if (accel_mods == 0)
    {
        switch (event->keyval)
        {
        case GDK_Escape:
            get_keycombo_string (last_accel_key, last_accel_mask, name);
            gtk_entry_set_text (GTK_ENTRY (widget), name);
            goto out; /* cancel */
        case GDK_BackSpace:
            gtk_entry_set_text (GTK_ENTRY (widget), "");
            last_accel_key = 0;
            last_accel_mask = 0;
            /* clear the accelerator on Backspace */
            cleared = TRUE;
            goto out;
        default:
            break;
        }
    }

    if (!gtk_accelerator_valid (accel_key, accel_mods))
    {
        gtk_widget_error_bell (widget);

        return TRUE;
    }
    last_accel_key = accel_key;
    last_accel_mask = accel_mods;
    get_keycombo_string (last_accel_key, last_accel_mask, name);
    gtk_entry_set_text (GTK_ENTRY (widget), name);

    // update the tree
    {
        GtkWidget *hotkeys = lookup_widget (prefwin, "hotkeys_list");
        GtkTreePath *path;
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (hotkeys), &path, NULL);
        GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (hotkeys));
        GtkTreeIter iter;
        if (path && gtk_tree_model_get_iter (model, &iter, path)) {
            gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, name, -1);
        }
    }

out:
    gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
    gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
    grabbed = 0;
    return TRUE;
}

static void
hotkey_grab_focus (GtkWidget *widget, GdkEvent *event) {
    GdkDisplay *display = gtk_widget_get_display (widget);
    if (grabbed) {
        return;
    }
    grabbed = 0;
    if (GDK_GRAB_SUCCESS != gdk_keyboard_grab (gtk_widget_get_window (widget), FALSE, gdk_event_get_time ((GdkEvent*)event))) {
        return;
    }

    if (gdk_pointer_grab (gtk_widget_get_window (widget), FALSE,
                GDK_BUTTON_PRESS_MASK,
                NULL, NULL,
                gdk_event_get_time ((GdkEvent *)event)) != GDK_GRAB_SUCCESS)
    {
        gdk_display_keyboard_ungrab (display, gdk_event_get_time ((GdkEvent *)event));
        return;
    }
    gtk_entry_set_text (GTK_ENTRY (widget), _("New key combination..."));
    grabbed = 1;
}

gboolean
on_hotkey_keycombo_focus_in_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
    hotkey_grab_focus (widget, (GdkEvent *)event);
    return TRUE;
}

gboolean
on_hotkey_keycombo_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    hotkey_grab_focus (widget, (GdkEvent *)event);
    return FALSE;
}

gboolean
on_hotkey_keycombo_motion_notify_event (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data)
{
    return TRUE;
}


gboolean
on_hotkey_keycombo_button_release_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    return TRUE;
}


void
on_hotkeys_apply_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    hotkeys_save ();
}


void
on_hotkeys_revert_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    hotkeys_load ();
}

