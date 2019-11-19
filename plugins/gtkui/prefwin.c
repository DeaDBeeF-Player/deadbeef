/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include "../../gettext.h"
#include "gtkui.h"
#include "support.h"
#include "interface.h"
#include "callbacks.h"
#include "drawing.h"
#include "eq.h"
#include "ddblistview.h"
#include "pluginconf.h"
#include "dspconfig.h"
#include "wingeom.h"
#include "hotkeys.h"

static GtkWidget *prefwin;

static GSList *output_device_names;

static const char *
_get_output_soundcard_conf_name (void) {
    static char name[100];
    snprintf (name, sizeof (name), "%s_soundcard", deadbeef->get_output ()->plugin.id);
    return name;
}

static void
gtk_enum_sound_callback (const char *name, const char *desc, void *userdata) {
    GtkComboBox *combobox = GTK_COMBO_BOX (userdata);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), desc);

    deadbeef->conf_lock ();
    const char *curr = deadbeef->conf_get_str_fast (_get_output_soundcard_conf_name(), "default");
    if (!strcmp (curr, name)) {
        gtk_combo_box_set_active (combobox, g_slist_length (output_device_names));
    }
    deadbeef->conf_unlock ();

    output_device_names = g_slist_append (output_device_names, g_strdup (name));
}

void
preferences_fill_soundcards (void) {
    if (!prefwin) {
        return;
    }
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (prefwin, "pref_soundcard"));
    GtkTreeModel *mdl = gtk_combo_box_get_model (combobox);
    gtk_list_store_clear (GTK_LIST_STORE (mdl));

    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), _("Default Audio Device"));

    deadbeef->conf_lock ();
    const char *s = deadbeef->conf_get_str_fast (_get_output_soundcard_conf_name(), "default");
    if (!strcmp (s, "default")) {
        gtk_combo_box_set_active (combobox, 0);
    }
    deadbeef->conf_unlock ();

    if (output_device_names) {
        for (GSList *dev = output_device_names; dev; dev = dev->next) {
            g_free (dev->data);
            dev->data = NULL;
        }
        g_slist_free (output_device_names);
        output_device_names = NULL;
    }
    output_device_names = g_slist_append (output_device_names, g_strdup ("default"));
    if (deadbeef->get_output ()->enum_soundcards) {
        deadbeef->get_output ()->enum_soundcards (gtk_enum_sound_callback, combobox);
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), TRUE);
    }
    else {
        gtk_widget_set_sensitive (GTK_WIDGET (combobox), FALSE);
    }
}

void
prefwin_init_theme_colors (void) {
    GdkColor clr;
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_background")), (gtkui_get_bar_background_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_foreground")), (gtkui_get_bar_foreground_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_dark")), (gtkui_get_tabstrip_dark_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_mid")), (gtkui_get_tabstrip_mid_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_light")), (gtkui_get_tabstrip_light_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_base")), (gtkui_get_tabstrip_base_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_text")), (gtkui_get_tabstrip_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_playing_text")), (gtkui_get_tabstrip_playing_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_selected_text")), (gtkui_get_tabstrip_selected_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_even_row")), (gtkui_get_listview_even_row_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_odd_row")), (gtkui_get_listview_odd_row_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_row")), (gtkui_get_listview_selection_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_text")), (gtkui_get_listview_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_text")), (gtkui_get_listview_selected_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_playing_text")), (gtkui_get_listview_playing_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_group_text")), (gtkui_get_listview_group_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_column_text")), (gtkui_get_listview_column_text_color (&clr), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_cursor")), (gtkui_get_listview_cursor_color (&clr), &clr));
}

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
set_scale (const char *scale_name, int value) {
    GtkWidget *scale = lookup_widget (prefwin, scale_name);
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
    gtk_range_set_value (GTK_RANGE (scale), value);
    g_signal_handlers_unblock_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
}

static void
set_combobox (GtkComboBox *combo, int i) {
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
    gtk_combo_box_set_active (combo, i);
    g_signal_handlers_unblock_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
}

static void
set_toggle_button (const char *button_name, int value) {
    GtkToggleButton *button = GTK_TOGGLE_BUTTON (lookup_widget (prefwin, button_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("toggled");
    g_signal_handlers_block_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
    gtk_toggle_button_set_active (button, value);
    g_signal_handlers_unblock_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
}

static void
set_entry_text (const char *entry_name, const char *text) {
    GtkEntry *entry = GTK_ENTRY (lookup_widget (prefwin, entry_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
    gtk_entry_set_text (entry, text);
    g_signal_handlers_unblock_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
}

static void
update_samplerate_widget_sensitivity (int override_sr, int dep_active) {
    gtk_widget_set_sensitive (lookup_widget (prefwin, "label_direct_sr"), override_sr);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "comboboxentry_direct_sr"), override_sr);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "checkbutton_dependent_sr"), override_sr);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "comboboxentry_sr_mult_48"), override_sr && dep_active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "comboboxentry_sr_mult_44"), override_sr && dep_active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "label_sr_mult_48"), override_sr && dep_active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "label_sr_mult_44"), override_sr && dep_active);
}


void
gtkui_run_preferences_dlg (void) {
    if (prefwin) {
        return;
    }
    deadbeef->conf_lock ();
    GtkWidget *w = prefwin = create_prefwin ();
    gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (mainwin));

    GtkComboBox *combobox = NULL;

    // output plugin selection
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_output_plugin"));

    const char *outplugname = deadbeef->conf_get_str_fast ("output_plugin", "alsa");
    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    for (int i = 0; out_plugs[i]; i++) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), out_plugs[i]->plugin.name);
        if (!strcmp (outplugname, out_plugs[i]->plugin.id)) {
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

    // 8_to_16
    set_toggle_button("convert8to16", deadbeef->conf_get_int ("streamer.8_to_16", 1));

    // 16_to_24
    set_toggle_button("convert16to24", deadbeef->conf_get_int ("streamer.16_to_24", 0));

    // override samplerate checkbox
    int override_sr = deadbeef->conf_get_int ("streamer.override_samplerate", 0);
    set_toggle_button ("checkbutton_sr_override", override_sr);

    // direct/dependent samplerate radio buttons
    int use_dependent_samplerate = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0);
    set_toggle_button ("checkbutton_dependent_sr", use_dependent_samplerate);

    // direct samplerate value
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup_widget (w, "comboboxentry_direct_sr")))), deadbeef->conf_get_str_fast ("streamer.samplerate", "44100"));
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup_widget (w, "comboboxentry_sr_mult_48")))), deadbeef->conf_get_str_fast ("streamer.samplerate_mult_48", "48000"));
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (lookup_widget (w, "comboboxentry_sr_mult_44")))), deadbeef->conf_get_str_fast ("streamer.samplerate_mult_44", "44100"));

    update_samplerate_widget_sensitivity (override_sr, use_dependent_samplerate);

    // replaygain_mode
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_replaygain_source_mode"));
    set_combobox (combobox, deadbeef->conf_get_int ("replaygain.source_mode", 0));

    // replaygain_processing
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_replaygain_processing"));
    int processing_idx = 0;
    int processing_flags = deadbeef->conf_get_int ("replaygain.processing_flags", 0);
    if (processing_flags == DDB_RG_PROCESSING_GAIN) {
        processing_idx = 1;
    }
    else if (processing_flags == (DDB_RG_PROCESSING_GAIN|DDB_RG_PROCESSING_PREVENT_CLIPPING)) {
        processing_idx = 2;
    }
    else if (processing_flags == DDB_RG_PROCESSING_PREVENT_CLIPPING) {
        processing_idx = 3;
    }

    set_combobox (combobox, processing_idx);

    // preamp with rg
    set_scale("replaygain_preamp", deadbeef->conf_get_int ("replaygain.preamp_with_rg", 0));

    // preamp without rg
    set_scale("global_preamp", deadbeef->conf_get_int ("replaygain.preamp_without_rg", 0));
    // dsp
    dsp_setup_init (prefwin);

    // minimize_on_startup
    set_toggle_button("minimize_on_startup", deadbeef->conf_get_int ("gtkui.start_hidden", 0));
    
    // close_send_to_tray
    set_toggle_button("pref_close_send_to_tray", deadbeef->conf_get_int ("close_send_to_tray", 0));

    // hide tray icon
    set_toggle_button("hide_tray_icon", deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0));

    // mmb_delete_playlist
    set_toggle_button("mmb_delete_playlist", deadbeef->conf_get_int ("gtkui.mmb_delete_playlist", 1));

    // hide_delete_from_disk
    set_toggle_button("hide_delete_from_disk", deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0));

    // auto-rename playlist from folder name
    set_toggle_button("auto_name_playlist_from_folder", deadbeef->conf_get_int ("gtkui.name_playlist_from_folder", 1));

    // refresh rate
    int val = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
    set_scale("gui_fps", val);

    // add from archives
    set_toggle_button("ignore_archives", deadbeef->conf_get_int ("ignore_archives", 1));

    // reset autostop
    set_toggle_button("reset_autostop", deadbeef->conf_get_int ("playlist.stop_after_current_reset", 0));

    // reset album autostop
    set_toggle_button("reset_autostopalbum", deadbeef->conf_get_int ("playlist.stop_after_album_reset", 0));

    // titlebar text
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "titlebar_format_playing")), deadbeef->conf_get_str_fast ("gtkui.titlebar_playing_tf", gtkui_default_titlebar_playing));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "titlebar_format_stopped")), deadbeef->conf_get_str_fast ("gtkui.titlebar_stopped_tf", gtkui_default_titlebar_stopped));

    // cli playlist
    int active = deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1);
    set_toggle_button("cli_add_to_playlist", active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "cli_playlist_name"), active);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (prefwin, "cli_playlist_name")), deadbeef->conf_get_str_fast ("cli_add_playlist_name", "Default"));

    // statusbar selection playback time
    set_toggle_button ("display_seltime", deadbeef->conf_get_int ("gtkui.statusbar_seltime", 0));

    // resume last session
    set_toggle_button("resume_last_session", deadbeef->conf_get_int ("resume_last_session", 1));

    // enable shift-jis recoding
    set_toggle_button("enable_shift_jis_recoding", deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0));

    // enable cp1251 recoding
    set_toggle_button("enable_cp1251_recoding", deadbeef->conf_get_int ("junk.enable_cp1251_detection", 1));

    // enable cp936 recoding
    set_toggle_button("enable_cp936_recoding", deadbeef->conf_get_int ("junk.enable_cp936_detection", 0));

    // enable auto-sizing of columns
    set_toggle_button("auto_size_columns", deadbeef->conf_get_int ("gtkui.autoresize_columns", 0));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON (lookup_widget (w, "listview_group_spacing")), deadbeef->conf_get_int ("playlist.groups.spacing", 0));

    // fill gui plugin list
    combobox = GTK_COMBO_BOX (lookup_widget (w, "gui_plugin"));
    const char **names = deadbeef->plug_get_gui_names ();
    for (int i = 0; names[i]; i++) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), names[i]);
        if (!strcmp (names[i], deadbeef->conf_get_str_fast ("gui_plugin", "GTK2"))) {
            set_combobox (combobox, i);
        }
    }

    // override bar colors
    int override = deadbeef->conf_get_int ("gtkui.override_bar_colors", 0);
    set_toggle_button("override_bar_colors", override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "bar_colors_group"), override);

    // override tabstrip colors
    override = deadbeef->conf_get_int ("gtkui.override_tabstrip_colors", 0);
    set_toggle_button("override_tabstrip_colors", override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "tabstrip_colors_group"), override);

    set_toggle_button("tabstrip_playing_bold", deadbeef->conf_get_int ("gtkui.tabstrip_embolden_playing", 0));
    set_toggle_button("tabstrip_playing_italic", deadbeef->conf_get_int ("gtkui.tabstrip_italic_playing", 0));
    set_toggle_button("tabstrip_selected_bold", deadbeef->conf_get_int ("gtkui.tabstrip_embolden_selected", 0));
    set_toggle_button("tabstrip_selected_italic", deadbeef->conf_get_int ("gtkui.tabstrip_italic_selected", 0));

    // get default gtk font
    GtkStyle *style = gtk_widget_get_style (mainwin);
    const char *gtk_style_font = pango_font_description_to_string (style->font_desc);

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "tabstrip_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.tabstrip_text", gtk_style_font));

    // override listview colors
    override = deadbeef->conf_get_int ("gtkui.override_listview_colors", 0);
    set_toggle_button("override_listview_colors", override);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "listview_colors_group"), override);

    // embolden/italic listview text
    set_toggle_button("listview_selected_text_bold", deadbeef->conf_get_int ("gtkui.embolden_selected_tracks", 0));
    set_toggle_button("listview_selected_text_italic", deadbeef->conf_get_int ("gtkui.italic_selected_tracks", 0));
    set_toggle_button("listview_playing_text_bold", deadbeef->conf_get_int ("gtkui.embolden_current_track", 0));
    set_toggle_button("listview_playing_text_italic", deadbeef->conf_get_int ("gtkui.italic_current_track", 0));

    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_text", gtk_style_font));
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_group_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_group_text", gtk_style_font));
    gtk_font_button_set_font_name (GTK_FONT_BUTTON (lookup_widget (w, "listview_column_text_font")), deadbeef->conf_get_str_fast ("gtkui.font.listview_column_text", gtk_style_font));

    // colors
    prefwin_init_theme_colors ();

    // network
    set_toggle_button("pref_network_enableproxy", deadbeef->conf_get_int ("network.proxy", 0));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyaddress")), deadbeef->conf_get_str_fast ("network.proxy.address", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "pref_network_proxyport")), deadbeef->conf_get_str_fast ("network.proxy.port", "8080"));
    combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_network_proxytype"));
    const char *type = deadbeef->conf_get_str_fast ("network.proxy.type", "HTTP");
    if (!strcasecmp (type, "HTTP")) {
        set_combobox (combobox, 0);
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        set_combobox (combobox, 1);
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        set_combobox (combobox, 2);
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        set_combobox (combobox, 3);
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        set_combobox (combobox, 4);
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        set_combobox (combobox, 5);
    }
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "proxyuser")), deadbeef->conf_get_str_fast ("network.proxy.username", ""));
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "proxypassword")), deadbeef->conf_get_str_fast ("network.proxy.password", ""));

    char ua[100];
    deadbeef->conf_get_str ("network.http_user_agent", "deadbeef", ua, sizeof (ua));
    set_entry_text("useragent", ua);

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
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Title"), rend_text, "text", 0, NULL);
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

    // hotkeys
    prefwin_init_hotkeys (prefwin);

    deadbeef->conf_unlock ();
    for (;;) {
        gtk_dialog_run (GTK_DIALOG (prefwin));
        if (gtkui_hotkeys_changed) {
            GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (prefwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You modified the hotkeys settings, but didn't save your changes."));
            gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
            gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Are you sure you want to continue without saving?"));
            gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
            int response = gtk_dialog_run (GTK_DIALOG (dlg));
            gtk_widget_destroy (dlg);
            if (response == GTK_RESPONSE_YES) {
                break;
            }
        }
        else {
            break;
        }
    }
    dsp_setup_free ();
    gtk_widget_destroy (prefwin);
    deadbeef->conf_save ();
    prefwin = NULL;
}

void
on_pref_soundcard_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    if (active >= 0 && active < g_slist_length(output_device_names)) {
        deadbeef->conf_lock ();
        const char *soundcard = deadbeef->conf_get_str_fast (_get_output_soundcard_conf_name(), "default");
        const char *active_name = g_slist_nth_data (output_device_names, active);
        if (strcmp (soundcard, active_name)) {
            deadbeef->conf_set_str (_get_output_soundcard_conf_name(), active_name);
            deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
        }
        deadbeef->conf_unlock ();
    }
}

void
on_pref_output_plugin_changed          (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);

    DB_output_t **out_plugs = deadbeef->plug_get_output_list ();
    DB_output_t *prev = NULL;
    DB_output_t *new = NULL;

    deadbeef->conf_lock ();
    const char *outplugname = deadbeef->conf_get_str_fast ("output_plugin", "alsa");
    for (int i = 0; out_plugs[i]; i++) {
        if (!strcmp (out_plugs[i]->plugin.id, outplugname)) {
            prev = out_plugs[i];
        }
        if (i == active) {
            new = out_plugs[i];
        }
    }
    deadbeef->conf_unlock ();

    if (!new) {
        fprintf (stderr, "failed to find output plugin selected in preferences window\n");
    }
    else {
        if (prev != new) {
            deadbeef->conf_set_str ("output_plugin", new->plugin.id);
            deadbeef->sendmessage (DB_EV_REINIT_SOUND, 0, 0, 0);
        }
    }
}

void
on_pref_replaygain_source_mode_changed (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int active = gtk_combo_box_get_active (combobox);
    deadbeef->conf_set_int ("replaygain.source_mode", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_replaygain_processing_changed  (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    uint32_t flags = 0;
    int idx = gtk_combo_box_get_active (combobox);
    if (idx == 1) {
        flags = DDB_RG_PROCESSING_GAIN;
    }
    if (idx == 2) {
        flags = DDB_RG_PROCESSING_GAIN | DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }
    if (idx == 3) {
        flags = DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }

    deadbeef->conf_set_int ("replaygain.processing_flags", flags);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_pref_replaygain_scale_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("replaygain_scale", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_replaygain_preamp_value_changed     (GtkRange        *range,
                                        gpointer         user_data)
{
    float val = gtk_range_get_value (range);
    deadbeef->conf_set_float ("replaygain.preamp_with_rg", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_global_preamp_value_changed     (GtkRange        *range,
                                    gpointer         user_data)
{
    float val = gtk_range_get_value (range);
    deadbeef->conf_set_float ("replaygain.preamp_without_rg", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_minimize_on_startup_clicked     (GtkButton       *button,
                                   gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("gtkui.start_hidden", active);
    if (active == 1) {
        set_toggle_button("hide_tray_icon", 0);
        deadbeef->conf_set_int ("gtkui.hide_tray_icon", 0);
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_pref_close_send_to_tray_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("close_send_to_tray", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_hide_tray_icon_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int ("gtkui.hide_tray_icon", active);
    if (active == 1) {
        set_toggle_button("minimize_on_startup", 0);
        deadbeef->conf_set_int ("gtkui.start_hidden", 0);
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
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
    GtkWidget *w = prefwin;
    assert (w);

    char s[20];
    snprintf (s, sizeof (s), "%d.%d", p->version_major, p->version_minor);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "plug_version")), s);

    if (p->descr) {
        GtkTextView *tv = GTK_TEXT_VIEW (lookup_widget (w, "plug_description"));

        GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

        gtk_text_buffer_set_text (buffer, p->descr, strlen(p->descr));
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (tv), buffer);
        g_object_unref (buffer);
    }

    GtkWidget *link = lookup_widget (w, "weblink");
    if (p->website) {
        gtk_link_button_set_uri (GTK_LINK_BUTTON(link), p->website);
        gtk_widget_set_sensitive (link, TRUE);
    }
    else {
        gtk_link_button_set_uri (GTK_LINK_BUTTON(link), "");
        gtk_widget_set_sensitive (link, FALSE);
    }

    GtkWidget *cpr = lookup_widget (w, "plug_copyright");
    if (p->copyright) {
        gtk_widget_set_sensitive (cpr, TRUE);
    }
    else {
        gtk_widget_set_sensitive (cpr, FALSE);
    }

    gtk_widget_set_sensitive (lookup_widget (prefwin, "configure_plugin"), p->configdialog ? TRUE : FALSE);
}

void
gtkui_conf_get_str (const char *key, char *value, int len, const char *def) {
    deadbeef->conf_get_str (key, def, value, len);
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
        ddb_dialog_t conf = {
            .title = p->name,
            .layout = p->configdialog,
            .set_param = deadbeef->conf_set_str,
            .get_param = gtkui_conf_get_str,
        };
        gtkui_run_dialog (prefwin, &conf, 0, NULL, NULL);
    }
}

void
on_pref_pluginlist_row_activated       (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
    GtkWidget *w = prefwin;
    GtkButton *btn = GTK_BUTTON (lookup_widget (w, "configure_plugin"));
    gtk_button_clicked(btn);
}

static void
override_set_helper (GtkToggleButton  *togglebutton, const char* conf_str, const char *group_name)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int (conf_str, active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, group_name), active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    gtkui_init_theme_colors ();
    prefwin_init_theme_colors ();
}

static void
font_set_helper (GtkFontButton *fontbutton, const char* conf_str)
{
    deadbeef->conf_set_str (conf_str, gtk_font_button_get_font_name (fontbutton));
    gtkui_init_theme_colors ();
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
}

static int
font_style_set_helper (GtkToggleButton  *togglebutton, const char* conf_str)
{
    int active = gtk_toggle_button_get_active (togglebutton);
    deadbeef->conf_set_int (conf_str, active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    return active;
}

static void
color_set_helper (GtkColorButton *colorbutton, const char* conf_str)
{
    if (conf_str == NULL) {
        return;
    }
    GdkColor clr;
    gtk_color_button_get_color (colorbutton, &clr);
    char str[100];
    snprintf (str, sizeof (str), "%d %d %d", clr.red, clr.green, clr.blue);
    deadbeef->conf_set_str (conf_str, str);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)conf_str, 0, 0);
    gtkui_init_theme_colors ();
}
void
on_tabstrip_light_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_light");
}


void
on_tabstrip_mid_color_set              (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_mid");
}


void
on_tabstrip_dark_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_dark");
}

void
on_tabstrip_base_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_base");
}

void
on_tabstrip_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_text");
}

void
on_tabstrip_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_selected_text");
}

void
on_tabstrip_playing_bold_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_embolden_playing = font_style_set_helper (togglebutton, "gtkui.tabstrip_embolden_playing");
}

void
on_tabstrip_playing_italic_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_italic_playing = font_style_set_helper (togglebutton, "gtkui.tabstrip_italic_playing");
}

void
on_tabstrip_selected_bold_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_embolden_selected = font_style_set_helper (togglebutton, "gtkui.tabstrip_embolden_selected");
}

void
on_tabstrip_selected_italic_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_tabstrip_italic_selected = font_style_set_helper (togglebutton, "gtkui.tabstrip_italic_selected");
}

void
on_tabstrip_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.tabstrip_text");
}

void
on_tabstrip_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.tabstrip_playing_text");
}

void
on_bar_foreground_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.bar_foreground");
    eq_redraw ();
}


void
on_bar_background_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.bar_background");
    eq_redraw ();
}

void
on_override_listview_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_listview_colors", "listview_colors_group");
}


void
on_listview_even_row_color_set         (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_even_row");
}

void
on_listview_odd_row_color_set          (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_odd_row");
}

void
on_listview_selected_row_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_selection");
}

void
on_listview_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_text");
}


void
on_listview_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_selected_text");
}

void
on_listview_cursor_color_set           (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_cursor");
}

void
on_listview_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_playing_text");
}

void
on_listview_group_text_color_set       (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_group_text");
}

void
on_listview_group_text_font_set        (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_group_text");
}

void
on_listview_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_text");
}

void
on_listview_playing_text_bold_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_embolden_current_track = font_style_set_helper (togglebutton, "gtkui.embolden_current_track");
}

void
on_listview_playing_text_italic_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_italic_current_track = font_style_set_helper (togglebutton, "gtkui.italic_current_track");
}

void
on_listview_selected_text_bold_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_embolden_selected_tracks = font_style_set_helper (togglebutton, "gtkui.embolden_selected_tracks");
}

void
on_listview_selected_text_italic_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gtkui_italic_selected_tracks = font_style_set_helper (togglebutton, "gtkui.italic_selected_tracks");
}

void
on_listview_column_text_color_set      (GtkColorButton  *colorbutton,
                                        gpointer         user_data)
{
    color_set_helper (colorbutton, "gtkui.color.listview_column_text");
}

void
on_listview_column_text_font_set       (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
    font_set_helper (fontbutton, "gtkui.font.listview_column_text");
}

void
on_override_bar_colors_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_bar_colors", "bar_colors_group");
    eq_redraw ();
}

void
on_override_tabstrip_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    override_set_helper (togglebutton, "gtkui.override_tabstrip_colors", "tabstrip_colors_group");
}

void
on_pref_network_proxyaddress_changed   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.address", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_pref_network_enableproxy_clicked    (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("network.proxy", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}


void
on_pref_network_proxyport_changed      (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("network.proxy.port", atoi (gtk_entry_get_text (GTK_ENTRY (editable))));
}


void
on_pref_network_proxytype_changed      (GtkComboBox     *combobox,
                                        gpointer         user_data)
{

    int active = gtk_combo_box_get_active (combobox);
    switch (active) {
    case 0:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP");
        break;
    case 1:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP_1_0");
        break;
    case 2:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS4");
        break;
    case 3:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS5");
        break;
    case 4:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS4A");
        break;
    case 5:
        deadbeef->conf_set_str ("network.proxy.type", "SOCKS5_HOSTNAME");
        break;
    default:
        deadbeef->conf_set_str ("network.proxy.type", "HTTP");
        break;
    }
}

void
on_proxyuser_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.username", gtk_entry_get_text (GTK_ENTRY (editable)));
}


void
on_proxypassword_changed               (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.proxy.password", gtk_entry_get_text (GTK_ENTRY (editable)));
}

void
on_hide_delete_from_disk_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("gtkui.hide_remove_from_disk", active);
}

void
on_skip_deleted_songs_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("gtkui.skip_deleted_songs", active);
}

void
on_titlebar_format_playing_changed     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("gtkui.titlebar_playing_tf", gtk_entry_get_text (GTK_ENTRY (editable)));
    gtkui_titlebar_tf_init ();
    gtkui_set_titlebar (NULL);
}


void
on_titlebar_format_stopped_changed     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("gtkui.titlebar_stopped_tf", gtk_entry_get_text (GTK_ENTRY (editable)));
    gtkui_titlebar_tf_init ();
    gtkui_set_titlebar (NULL);
}

void
on_cli_add_to_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("cli_add_to_specific_playlist", active);
    gtk_widget_set_sensitive (lookup_widget (prefwin, "cli_playlist_name"), active);
}


void
on_cli_playlist_name_changed           (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("cli_add_playlist_name", gtk_entry_get_text (GTK_ENTRY (editable)));
}

void
on_resume_last_session_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("resume_last_session", active);
}

void
on_enable_shift_jis_recoding_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("junk.enable_shift_jis_detection", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_enable_cp1251_recoding_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("junk.enable_cp1251_detection", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_enable_cp936_recoding_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("junk.enable_cp936_detection", active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_auto_name_playlist_from_folder_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("gtkui.name_playlist_from_folder", active);
}

static void
show_copyright_window (const char *text, const char *title, GtkWidget **pwindow) {
    if (*pwindow) {
        return;
    }
    GtkWidget *widget = *pwindow = create_helpwindow ();
    g_object_set_data (G_OBJECT (widget), "pointer", pwindow);
    g_signal_connect (widget, "delete_event", G_CALLBACK (on_gtkui_info_window_delete), pwindow);
    gtk_window_set_title (GTK_WINDOW (widget), title);
    gtk_window_set_transient_for (GTK_WINDOW (widget), GTK_WINDOW (prefwin));
    GtkWidget *txt = lookup_widget (widget, "helptext");
    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

    gtk_text_buffer_set_text (buffer, text, strlen(text));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (txt), buffer);
    g_object_unref (buffer);
    gtk_widget_show (widget);
}

static GtkWidget *copyright_window;

void
on_plug_copyright_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(lookup_widget (prefwin, "pref_pluginlist"));
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

    if (p->copyright) {
        show_copyright_window (p->copyright, "Copyright", &copyright_window);
    }
}

gboolean
on_prefwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "prefwin");
    return FALSE;
}


gboolean
on_prefwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "prefwin");
    return FALSE;
}


void
on_prefwin_realize                     (GtkWidget       *widget,
                                        gpointer         user_data)
{
    wingeom_restore (widget, "prefwin", -1, -1, -1, -1, 0);
}

void
on_gui_plugin_changed                  (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    gchar *txt = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (combobox));
    if (txt) {
        deadbeef->conf_set_str ("gui_plugin", txt);
        g_free (txt);
    }
}

void
on_gui_fps_value_changed           (GtkRange        *range,
                                        gpointer         user_data)
{
    int val = gtk_range_get_value (range);
    deadbeef->conf_set_int ("gtkui.refresh_rate", val);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_ignore_archives_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{

    deadbeef->conf_set_int ("ignore_archives", gtk_toggle_button_get_active (togglebutton));
}

void
on_reset_autostop_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_current_reset", gtk_toggle_button_get_active (togglebutton));
}

void
on_reset_autostopalbum_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_album_reset", gtk_toggle_button_get_active (togglebutton));
}



void
on_convert8to16_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("streamer.8_to_16", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_convert16to24_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("streamer.16_to_24", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_useragent_changed                   (GtkEditable     *editable,
                                        gpointer         user_data)
{
    deadbeef->conf_set_str ("network.http_user_agent", gtk_entry_get_text (GTK_ENTRY (editable)));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_auto_size_columns_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("gtkui.autoresize_columns", gtk_toggle_button_get_active (togglebutton));
}

void
on_display_seltime_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("gtkui.statusbar_seltime", gtk_toggle_button_get_active (togglebutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_listview_group_spacing_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.groups.spacing", gtk_spin_button_get_value_as_int (spinbutton));
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)"playlist.groups.spacing", 0, 0);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref(plt);
    }
}

void
on_checkbutton_dependent_sr_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int override_sr = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "checkbutton_sr_override")));
    int dep_active = gtk_toggle_button_get_active (togglebutton);
    update_samplerate_widget_sensitivity (override_sr, dep_active);
    deadbeef->conf_set_int ("streamer.use_dependent_samplerate", dep_active);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_checkbutton_sr_override_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int override_sr = gtk_toggle_button_get_active (togglebutton);
    int dep_active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (prefwin, "checkbutton_dependent_sr")));
    update_samplerate_widget_sensitivity (override_sr, dep_active);
    deadbeef->conf_set_int ("streamer.override_samplerate", override_sr);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

static int
clamp_samplerate (int val) {
    if (val < 8000) {
        return 8000;
    }
    else if (val > 768000) {
        return 768000;
    }
    return val;
}

void
on_comboboxentry_direct_sr_changed     (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int val = atoi(gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)))));
    int out = clamp_samplerate (val);
    deadbeef->conf_set_int ("streamer.samplerate", out);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_comboboxentry_sr_mult_48_changed    (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int val = atoi(gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)))));
    int out = clamp_samplerate (val);
    deadbeef->conf_set_int ("streamer.samplerate_mult_48", out);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


void
on_comboboxentry_sr_mult_44_changed    (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    int val = atoi(gtk_entry_get_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combobox)))));
    int out = clamp_samplerate (val);
    deadbeef->conf_set_int ("streamer.samplerate_mult_44", out);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}
