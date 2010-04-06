/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <gtk/gtk.h>
#include <string.h>
#include <assert.h>
#include "gtkui.h"
#include "support.h"
#include "interface.h"
#include "callbacks.h"
#include "drawing.h"
#include "../hotkeys/hotkeys.h"

#pragma GCC optimize("O0")

static GtkWidget *prefwin;

static char alsa_device_names[100][64];
static int num_alsa_devices;

static void
gtk_enum_sound_callback (const char *name, const char *desc, void *userdata) {
    if (num_alsa_devices >= 100) {
        fprintf (stderr, "wtf!! more than 100 alsa devices??\n");
        return;
    }
    GtkComboBox *combobox = GTK_COMBO_BOX (userdata);
    gtk_combo_box_append_text (combobox, desc);

    if (!strcmp (deadbeef->conf_get_str ("alsa_soundcard", "default"), name)) {
        gtk_combo_box_set_active (combobox, num_alsa_devices);
    }

    strncpy (alsa_device_names[num_alsa_devices], name, 63);
    alsa_device_names[num_alsa_devices][63] = 0;
    num_alsa_devices++;
}

void
on_plugin_active_toggled (GtkCellRendererToggle *cell_renderer, gchar *path, GtkTreeModel *model) {
    GtkTreePath *p = gtk_tree_path_new_from_string (path);
    if (p) {
        int *indices = gtk_tree_path_get_indices (p);
        //gtk_tree_path_free (p); // wtf?? gtk crashes on this
        if (indices) {
            DB_plugin_t **plugins = deadbeef->plug_get_list ();
            DB_plugin_t *plug = plugins[*indices];
            gboolean state;
            GtkTreeIter iter;
            gtk_tree_model_get_iter (model, &iter, p);
            gtk_tree_model_get (model, &iter, 0, &state, -1);
            if (!deadbeef->plug_activate (plug, !state)) {
                gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, !state, -1);
            }
        }
        g_free (indices);
    }
}

void
preferences_fill_soundcards (void) {
    if (!prefwin) {
        return;
    }
    const char *s = deadbeef->conf_get_str ("alsa_soundcard", "default");
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (prefwin, "pref_soundcard"));
    GtkTreeModel *mdl = gtk_combo_box_get_model (combobox);
    gtk_list_store_clear (GTK_LIST_STORE (mdl));

    gtk_combo_box_append_text (combobox, "Default Audio Device");
    if (!strcmp (s, "default")) {
        gtk_combo_box_set_active (combobox, 0);
    }
    num_alsa_devices = 1;
    strcpy (alsa_device_names[0], "default");
    if (deadbeef->get_output ()->enum_soundcards) {
        deadbeef->get_output ()->enum_soundcards (gtk_enum_sound_callback, combobox);
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), TRUE);
    }
    else {
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), FALSE);
    }
}

void
on_hk_slot_edited (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
    if (!new_text || !new_text[0]) {
        return;
    }
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, treepath);
    gtk_tree_path_free (treepath);
    gtk_list_store_set (store, &iter, 0, new_text, -1);
}

void
on_hk_binding_edited (GtkCellRendererAccel *accel, gchar *path, guint accel_key, GdkModifierType accel_mods, guint hardware_keycode, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, treepath);
    gtk_tree_path_free (treepath);

    // build value
    char new_value[1000] = "";
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

    // find key name from hotkeys plugin
    DB_plugin_t **plugs = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugs[i]; i++) {
        if (plugs[i]->id && !strcmp (plugs[i]->id, "hotkeys")) {
            const char *name = ((DB_hotkeys_plugin_t *)plugs[i])->get_name_for_keycode (accel_key);
            strcat (new_value, name);
            gtk_list_store_set (store, &iter, 1, new_value, -1);
            break;
        }
    }
//    if (!plugs[i]) {
//        return;
//    }
}

void
on_addhotkey_clicked                     (GtkButton *button, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, "toggle_pause", 1, "", -1);
}

void
on_removehotkey_clicked                     (GtkButton *button, gpointer user_data) {
    GtkTreeView *tree = GTK_TREE_VIEW (user_data);
    GtkTreeModel *model = gtk_tree_view_get_model (tree);
    if (model) {
        GtkTreeSelection *sel = gtk_tree_view_get_selection (tree);
        if (sel) {
            GtkTreeIter iter;
            if (gtk_tree_selection_get_selected (sel, NULL, &iter)) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
            }
        }
    }
}

static gboolean
add_hotkey_to_config (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    int *counter = (int *)data;
    GValue key = {0,}, value = {0,};
    gtk_tree_model_get_value (model, iter, 0, &key);
    gtk_tree_model_get_value (model, iter, 1, &value);
    const char *skey = g_value_get_string (&key);
    const char *svalue = g_value_get_string (&value);

    char conf_name[100];
    char conf_value[100];
    snprintf (conf_name, sizeof (conf_name), "hotkeys.key%d", *counter);
    (*counter)++;
    snprintf (conf_value, sizeof (conf_value), "%s: %s", svalue, skey);
    deadbeef->conf_set_str (conf_name, conf_value);
    return FALSE;
}

void
on_applyhotkeys_clicked                     (GtkButton *button, gpointer user_data) {
    DB_plugin_t **plugs = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugs[i]; i++) {
        if (plugs[i]->id && !strcmp (plugs[i]->id, "hotkeys")) {
            // rebuild config
            deadbeef->conf_remove_items ("hotkeys.key");
            int counter = 1;
            GtkTreeModel *model = GTK_TREE_MODEL (user_data);
            gtk_tree_model_foreach (model, add_hotkey_to_config, &counter);

            ((DB_hotkeys_plugin_t *)plugs[i])->reset ();
            break;
        }
    }
}

void
prefwin_init_theme_colors (void) {
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_background")), gtkui_get_bar_background_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_foreground")), gtkui_get_bar_foreground_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_dark")), gtkui_get_tabstrip_dark_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_mid")), gtkui_get_tabstrip_mid_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_light")), gtkui_get_tabstrip_light_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_base")), gtkui_get_tabstrip_base_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_even_row")), gtkui_get_listview_even_row_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_odd_row")), gtkui_get_listview_odd_row_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_row")), gtkui_get_listview_selection_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_text")), gtkui_get_listview_text_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_text")), gtkui_get_listview_selected_text_color ());
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_cursor")), gtkui_get_listview_cursor_color ());

}

void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (prefwin) {
        return;
    }
    GtkWidget *w = prefwin = create_prefwin ();
    gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (mainwin));

    GtkComboBox *combobox = NULL;

    // output plugin selection
    const char *outplugname = deadbeef->conf_get_str ("output_plugin", "ALSA output plugin");
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_output_plugin"));

    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    for (int i = 0; out_plugs[i]; i++) {
        gtk_combo_box_append_text (combobox, out_plugs[i]->plugin.name);
        if (!strcmp (outplugname, out_plugs[i]->plugin.name)) {
            gtk_combo_box_set_active (combobox, i);
        }
    }

    // soundcard (output device) selection
    preferences_fill_soundcards ();

    g_signal_connect ((gpointer) combobox, "changed",
            G_CALLBACK (on_pref_output_plugin_changed),
            NULL);
    GtkWidget *pref_soundcard = lookup_widget (prefwin, "pref_soundcard");
    g_signal_connect ((gpointer) pref_soundcard, "changed",
            G_CALLBACK (on_pref_soundcard_changed),
            NULL);

    // alsa resampling
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_dynsamplerate")), deadbeef->conf_get_int ("playback.dynsamplerate", 0));

    // src_quality
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_src_quality"));
    gtk_combo_box_set_active (combobox, deadbeef->conf_get_int ("src_quality", 2));

    // replaygain_mode
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_replaygain_mode"));
    gtk_combo_box_set_active (combobox, deadbeef->conf_get_int ("replaygain_mode", 0));

    // replaygain_scale
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_replaygain_scale")), deadbeef->conf_get_int ("replaygain_scale", 1));

    // close_send_to_tray
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_close_send_to_tray")), deadbeef->conf_get_int ("close_send_to_tray", 0));

    // mmb_delete_playlist
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "mmb_delete_playlist")), deadbeef->conf_get_int ("gtkui.mmb_delete_playlist", 0));

    // override bar colors
    int override = deadbeef->conf_get_int ("gtkui.override_bar_colors", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "override_bar_colors")), override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "bar_colors_group"), override);

    // override tabstrip colors
    override = deadbeef->conf_get_int ("gtkui.override_tabstrip_colors", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "override_tabstrip_colors")), override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "tabstrip_colors_group"), override);

    // override listview colors
    override = deadbeef->conf_get_int ("gtkui.override_listview_colors", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "override_listview_colors")), override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "listview_colors_group"), override);

    // colors
    prefwin_init_theme_colors ();

    // network
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (w, "pref_network_enableproxy")), deadbeef->conf_get_int ("network.proxy", 0));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyaddress")), deadbeef->conf_get_str ("network.proxy.address", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyport")), deadbeef->conf_get_str ("network.proxy.port", "8080"));
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_network_proxytype"));
    const char *type = deadbeef->conf_get_str ("network.proxy.type", "HTTP");
    if (!strcasecmp (type, "HTTP")) {
        gtk_combo_box_set_active (combobox, 0);
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        gtk_combo_box_set_active (combobox, 1);
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        gtk_combo_box_set_active (combobox, 2);
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        gtk_combo_box_set_active (combobox, 3);
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        gtk_combo_box_set_active (combobox, 4);
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        gtk_combo_box_set_active (combobox, 5);
    }

    // list of plugins
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
#if 0
    GtkCellRenderer *rend_toggle = gtk_cell_renderer_toggle_new ();
    GtkListStore *store = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_BOOLEAN);
    g_signal_connect ((gpointer)rend_toggle, "toggled",
            G_CALLBACK (on_plugin_active_toggled),
            store);
    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes ("Active", rend_toggle, "active", 0, "activatable", 2, NULL);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes ("Title", rend_text, "text", 1, NULL);
    gtk_tree_view_append_column (tree, col1);
    gtk_tree_view_append_column (tree, col2);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        GtkTreeIter it;
        gtk_list_store_append (store, &it);
        gtk_list_store_set (store, &it, 0, plugins[i]->inactive ? FALSE : TRUE, 1, plugins[i]->name, 2, plugins[i]->nostop ? FALSE : TRUE, -1);
    }
#else
    GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes ("Title", rend_text, "text", 0, NULL);
    gtk_tree_view_append_column (tree, col2);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    int i;
    for (i = 0; plugins[i]; i++) {
        GtkTreeIter it;
        gtk_list_store_append (store, &it);
        gtk_list_store_set (store, &it, 0, plugins[i]->name, -1);
    }
#endif
    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));

    gtk_widget_set_sensitive (lookup_widget (prefwin, "configure_plugin"), FALSE);
//    gtk_widget_show (w);

    // hotkeys
    GtkTreeView *hktree = GTK_TREE_VIEW (lookup_widget (prefwin, "hotkeystree"));
    GtkListStore *hkstore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    GtkCellRenderer *rend_hk_slot = gtk_cell_renderer_combo_new ();

    // model for hotkey slots
    const char *slots[] = {
        "toggle_pause",
        "play",
        "prev",
        "next",
        "stop",
        "play_random",
        "seek_fwd",
        "seek_back",
        "volume_up",
        "volume_down",
        "toggle_stop_after_current",
        NULL
    };
    GtkListStore *slots_store = gtk_list_store_new (1, G_TYPE_STRING);
    for (int i = 0; slots[i]; i++) {
        GtkTreeIter iter;
        gtk_list_store_append (slots_store, &iter);
        gtk_list_store_set (slots_store, &iter, 0, slots[i], -1);
    }
    g_object_set (G_OBJECT (rend_hk_slot), "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
    g_object_set (G_OBJECT (rend_hk_slot), "has-entry", FALSE, NULL);
    g_object_set (G_OBJECT (rend_hk_slot), "text-column", 0, NULL);
    g_object_set (G_OBJECT (rend_hk_slot), "model", slots_store, NULL);
    g_object_set (G_OBJECT (rend_hk_slot), "editable", TRUE, NULL);

    g_signal_connect ((gpointer)rend_hk_slot, "edited",
            G_CALLBACK (on_hk_slot_edited),
            hkstore);

    GtkCellRenderer *rend_hk_binding = gtk_cell_renderer_accel_new ();
    g_object_set (G_OBJECT (rend_hk_binding), "editable", TRUE, NULL);

    g_signal_connect ((gpointer)rend_hk_binding, "accel-edited",
            G_CALLBACK (on_hk_binding_edited),
            hkstore);


    GtkTreeViewColumn *hk_col1 = gtk_tree_view_column_new_with_attributes ("Slot", rend_hk_slot, "text", 0, NULL);
    GtkTreeViewColumn *hk_col2 = gtk_tree_view_column_new_with_attributes ("Key combination", rend_hk_binding, "text", 1, NULL);
    gtk_tree_view_append_column (hktree, hk_col1);
    gtk_tree_view_append_column (hktree, hk_col2);

    // fetch hotkeys from config
    DB_conf_item_t *item = deadbeef->conf_find ("hotkeys.", NULL);
    while (item) {
        size_t l = strlen (item->value);
        char param[l+1];
        memcpy (param, item->value, l+1);
        
        char* colon = strchr (param, ':');
        if (!colon)
        {
            fprintf (stderr, "hotkeys: bad config option %s %s\n", item->key, item->value);
            continue;
        }
        char* command = colon+1;
        *colon = 0;
        while (*command && ((uint8_t)*command) <= 0x20) {
            command++;
        }
        if (*command) {
            GtkTreeIter iter;
            gtk_list_store_append (hkstore, &iter);
            gtk_list_store_set (hkstore, &iter, 0, command, 1, param, -1);
            item = deadbeef->conf_find ("hotkeys.", item);
        }
    }
    gtk_tree_view_set_model (hktree, GTK_TREE_MODEL (hkstore));

    g_signal_connect ((gpointer)lookup_widget (prefwin, "addhotkey"), "clicked", G_CALLBACK (on_addhotkey_clicked), hkstore);
    g_signal_connect ((gpointer)lookup_widget (prefwin, "removehotkey"), "clicked", G_CALLBACK (on_removehotkey_clicked), hktree);
    g_signal_connect ((gpointer)lookup_widget (prefwin, "applyhotkeys"), "clicked", G_CALLBACK (on_applyhotkeys_clicked), hkstore);

    // tag writer
    int strip_id3v2 = deadbeef->conf_get_int ("mp3.strip_id3v2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("mp3.strip_id3v1", 0);
    int strip_apev2 = deadbeef->conf_get_int ("mp3.strip_apev2", 0);
    int write_id3v2 = deadbeef->conf_get_int ("mp3.write_id3v2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("mp3.write_id3v1", 0);
    int write_apev2 = deadbeef->conf_get_int ("mp3.write_apev2", 1);
    int id3v2_version = deadbeef->conf_get_int ("mp3.id3v2_version", 3);
    const char *id3v1_encoding = deadbeef->conf_get_str ("mp3.id3v1_encoding", "iso8859-1");
    int ape_strip_id3v2 = deadbeef->conf_get_int ("ape.strip_id3v2", 0);
    int ape_strip_apev2 = deadbeef->conf_get_int ("ape.strip_apev2", 0);
    int ape_write_id3v2 = deadbeef->conf_get_int ("ape.write_id3v2", 0);
    int ape_write_apev2 = deadbeef->conf_get_int ("ape.write_apev2", 1);
    int wv_strip_apev2 = deadbeef->conf_get_int ("wv.strip_apev2", 0);
    int wv_strip_id3v1 = deadbeef->conf_get_int ("wv.strip_id3v1", 0);
    int wv_write_apev2 = deadbeef->conf_get_int ("wv.write_apev2", 1);
    int wv_write_id3v1 = deadbeef->conf_get_int ("wv.write_id3v1", 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "strip_id3v2")), strip_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "strip_id3v1")), strip_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "strip_apev2")), strip_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "write_id3v2")), write_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "write_id3v1")), write_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "write_apev2")), write_apev2);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (prefwin, "id3v2_version")), id3v2_version != 4 ? 0 : 1);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (prefwin, "id3v1_encoding")), id3v1_encoding);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "ape_strip_id3v2")), ape_strip_id3v2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "ape_strip_apev2")), ape_strip_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "ape_write_apev2")), ape_write_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "ape_write_id3v2")), ape_write_id3v2);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "wv_strip_id3v1")), wv_strip_id3v1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "wv_strip_apev2")), wv_strip_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "wv_write_apev2")), wv_write_apev2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "wv_write_id3v1")), wv_write_id3v1);

    gtk_dialog_run (GTK_DIALOG (prefwin));
    gtk_widget_destroy (prefwin);
    prefwin = NULL;
}


void
on_pref_soundcard_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    if (active >= 0 && active < num_alsa_devices) {
        const char *soundcard = deadbeef->conf_get_str ("alsa_soundcard", "default");
        if (strcmp (soundcard, alsa_device_names[active])) {
            deadbeef->conf_set_str ("alsa_soundcard", alsa_device_names[active]);
            deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
        }
    }
}

void
on_pref_output_plugin_changed          (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    const char *outplugname = deadbeef->conf_get_str ("output_plugin", "ALSA output plugin");
    int active = gtk_combo_box_get_active (combobox);

    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    DB_output_t *prev = NULL;
    DB_output_t *new = NULL;

    for (int i = 0; out_plugs[i]; i++) {
        if (!strcmp (out_plugs[i]->plugin.name, outplugname)) {
            prev = out_plugs[i];
        }
        if (i == active) {
            new = out_plugs[i];
        }
    }

    if (!new) {
        fprintf (stderr, "failed to find output plugin selected in preferences window\n");
    }
    else {
        if (prev != new) {
            deadbeef->conf_set_str ("output_plugin", new->plugin.name);
            deadbeef->sendmessage (M_REINIT_SOUND, 0, 0, 0);
        }
    }
}

void
on_pref_dynsamplerate_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("playback.dynsamplerate", active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_src_quality_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("src_quality", active == -1 ? 2 : active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_replaygain_mode_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("replaygain_mode", active == -1 ? 0 : active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}

void
on_pref_replaygain_scale_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("replaygain_scale", active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_close_send_to_tray_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("close_send_to_tray", active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
}

void
on_mmb_delete_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("gtkui.mmb_delete_playlist", gtk_toggle_button_get_active (togglebutton));
}

void
on_pref_pluginlist_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[*indices];
    g_free (indices);
    assert (p);
    GtkWidget *w = prefwin;//GTK_WIDGET (gtk_widget_get_parent_window (GTK_WIDGET (treeview)));
    assert (w);
    GtkEntry *e = GTK_ENTRY (lookup_widget (w, "pref_plugin_descr"));
    gtk_entry_set_text (e, p->descr ? p->descr : "");
    e = GTK_ENTRY (lookup_widget (w, "pref_plugin_author"));
    gtk_entry_set_text (e, p->author ? p->author : "");
    e = GTK_ENTRY (lookup_widget (w, "pref_plugin_email"));
    gtk_entry_set_text (e, p->email ? p->email : "");
    e = GTK_ENTRY (lookup_widget (w, "pref_plugin_website"));
    gtk_entry_set_text (e, p->website ? p->website : "");

    gtk_widget_set_sensitive (lookup_widget (prefwin, "configure_plugin"), p->configdialog ? TRUE : FALSE);
}

gboolean
on_prefwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    prefwin = NULL;
    return FALSE;
}

void
on_pref_close_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    gtk_widget_hide (prefwin);
    gtk_widget_destroy (prefwin);
}

void
on_configure_plugin_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (w, "pref_pluginlist"));
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        // reset
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    DB_plugin_t **plugins = deadbeef->plug_get_list ();
    DB_plugin_t *p = plugins[*indices];
    if (p->configdialog) {
        plugin_configure (prefwin, p);
    }
}

void
on_tabstrip_light_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.tabstrip_light", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    tabstrip_redraw ();
}


void
on_tabstrip_mid_color_set              (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.tabstrip_mid", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    tabstrip_redraw ();
}


void
on_tabstrip_dark_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.tabstrip_dark", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    tabstrip_redraw ();
}

void
on_tabstrip_base_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.tabstrip_base", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    tabstrip_redraw ();
}


void
on_bar_foreground_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.bar_foreground", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    seekbar_redraw ();
    volumebar_redraw ();
}


void
on_bar_background_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.bar_background", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    seekbar_redraw ();
    volumebar_redraw ();
}

void
on_override_listview_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.override_listview_colors", active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "listview_colors_group"), active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}


void
on_listview_even_row_color_set         (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.listview_even_row", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}

void
on_listview_odd_row_color_set          (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.listview_odd_row", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}

void
on_listview_selected_row_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.listview_selection", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}

void
on_listview_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.listview_text", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}


void
on_listview_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.listview_selected_text", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}

void
on_listview_cursor_color_set           (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str ("gtkui.color.listview_cursor", str);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    playlist_refresh ();
}


void
on_override_bar_colors_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.override_bar_colors", active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "bar_colors_group"), active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    prefwin_init_theme_colors ();
    seekbar_redraw ();
    volumebar_redraw ();
}


void
on_override_tabstrip_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.override_tabstrip_colors", active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "tabstrip_colors_group"), active);
    deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    gtkui_init_theme_colors ();
    prefwin_init_theme_colors ();
    tabstrip_redraw ();
}

void
on_write_id3v2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.write_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_write_id3v1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.write_id3v1", gtk_toggle_button_get_active (togglebutton));
}


void
on_write_apev2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.write_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_strip_id3v2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.strip_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_strip_id3v1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.strip_id3v1", gtk_toggle_button_get_active (togglebutton));
}


void
on_strip_apev2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("mp3.strip_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_id3v2_version_changed               (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int version = 3;
    int active = gtk_combo_box_get_active (combobox);
    if (active == 1) {
        version = 4;
    }
    deadbeef->conf_set_int ("mp3.id3v2_version", version);
}


void
on_id3v1_encoding_changed              (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("mp3.id3v1_encoding", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_ape_write_id3v2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.write_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_ape_write_apev2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.write_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_ape_strip_id3v2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.strip_id3v2", gtk_toggle_button_get_active (togglebutton));
}


void
on_ape_strip_apev2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("ape.strip_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_wv_write_apev2_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.write_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_wv_write_id3v1_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.write_id3v1", gtk_toggle_button_get_active (togglebutton));
}

void
on_wv_strip_apev2_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.strip_apev2", gtk_toggle_button_get_active (togglebutton));
}


void
on_wv_strip_id3v1_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("wv.strip_id3v1", gtk_toggle_button_get_active (togglebutton));
}

