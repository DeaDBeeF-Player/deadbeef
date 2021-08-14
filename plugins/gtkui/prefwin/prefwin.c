/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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
#include <assert.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "../../../gettext.h"
#include "../callbacks.h"
#include "../ctmapping.h"
#include "../ddblistview.h"
#include "../drawing.h"
#include "../dspconfig.h"
#include "../eq.h"
#include "../gtkui.h"
#include "../hotkeys.h"
#include "../interface.h"
#include "../pluginconf.h"
#include "../support.h"
#include "../wingeom.h"
#include "prefwin.h"
#include "prefwinappearance.h"
#include "prefwinmisc.h"
#include "prefwinnetwork.h"
#include "prefwinplayback.h"
#include "prefwinplugins.h"
#include "prefwinsound.h"

int PREFWIN_TAB_INDEX_SOUND = 0;
int PREFWIN_TAB_INDEX_PLAYBACK = 1;
int PREFWIN_TAB_INDEX_DSP = 2;
int PREFWIN_TAB_INDEX_GUI = 3;
int PREFWIN_TAB_INDEX_APPEARANCE = 4;
int PREFWIN_TAB_INDEX_MEDIALIB = 5;
int PREFWIN_TAB_INDEX_NETWORK = 6;
int PREFWIN_TAB_INDEX_HOTKEYS = 7;
int PREFWIN_TAB_INDEX_PLUGINS = 8;

static GtkWidget *prefwin;

void
prefwin_init_theme_colors (void) {
    GdkColor clr;
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_background")), ((void)(gtkui_get_bar_background_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "bar_foreground")), ((void)(gtkui_get_bar_foreground_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_dark")), ((void)(gtkui_get_tabstrip_dark_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_mid")), ((void)(gtkui_get_tabstrip_mid_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_light")), ((void)(gtkui_get_tabstrip_light_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_base")), ((void)(gtkui_get_tabstrip_base_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_text")), ((void)(gtkui_get_tabstrip_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_playing_text")), ((void)(gtkui_get_tabstrip_playing_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "tabstrip_selected_text")), ((void)(gtkui_get_tabstrip_selected_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_even_row")), ((void)(gtkui_get_listview_even_row_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_odd_row")), ((void)(gtkui_get_listview_odd_row_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_row")), ((void)(gtkui_get_listview_selection_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_text")), ((void)(gtkui_get_listview_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_selected_text")), ((void)(gtkui_get_listview_selected_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_playing_text")), ((void)(gtkui_get_listview_playing_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_group_text")), ((void)(gtkui_get_listview_group_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_column_text")), ((void)(gtkui_get_listview_column_text_color (&clr)), &clr));
    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (prefwin, "listview_cursor")), ((void)(gtkui_get_listview_cursor_color (&clr)), &clr));
}

void
prefwin_set_scale (const char *scale_name, int value) {
    GtkWidget *scale = lookup_widget (prefwin, scale_name);
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("value_changed");
    g_signal_handlers_block_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
    gtk_range_set_value (GTK_RANGE (scale), value);
    g_signal_handlers_unblock_matched ((gpointer)scale, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_combobox (GtkComboBox *combo, int i) {
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
    gtk_combo_box_set_active (combo, i);
    g_signal_handlers_unblock_matched ((gpointer)combo, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_toggle_button (const char *button_name, int value) {
    GtkToggleButton *button = GTK_TOGGLE_BUTTON (lookup_widget (prefwin, button_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("toggled");
    g_signal_handlers_block_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
    gtk_toggle_button_set_active (button, value);
    g_signal_handlers_unblock_matched ((gpointer)button, mask, detail, 0, NULL, NULL, NULL);
}

void
prefwin_set_entry_text (const char *entry_name, const char *text) {
    GtkEntry *entry = GTK_ENTRY (lookup_widget (prefwin, entry_name));
    GSignalMatchType mask = G_SIGNAL_MATCH_DETAIL | G_SIGNAL_MATCH_DATA;
    GQuark detail = g_quark_from_static_string("changed");
    g_signal_handlers_block_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
    gtk_entry_set_text (entry, text);
    g_signal_handlers_unblock_matched ((gpointer)entry, mask, detail, 0, NULL, NULL, NULL);
}

void
on_prefwin_response_cb (GtkDialog *dialog,
                        int        response_id,
                        gpointer   user_data) {
    if (response_id != GTK_RESPONSE_CLOSE && response_id != GTK_RESPONSE_DELETE_EVENT) {
        return;
    }

    if (gtkui_hotkeys_changed) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (prefwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You modified the hotkeys settings, but didn't save your changes."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (prefwin));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Are you sure you want to continue without saving?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));
        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response == GTK_RESPONSE_NO) {
            return;
        }
    }

    dsp_setup_free ();
    ctmapping_setup_free ();
    gtk_widget_destroy (prefwin);
    deadbeef->conf_save ();
    prefwin_free_plugins ();
    prefwin = NULL;
}

static void
_init_prefwin() {
    if (prefwin != NULL) {
        return;
    }
    GtkWidget *w = prefwin = create_prefwin ();

    // hide unavailable tabs
    if (!deadbeef->plug_get_for_id ("hotkeys")) {
        gtk_notebook_remove_page (GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), 7);
        PREFWIN_TAB_INDEX_HOTKEYS = -1;
    }
    if (!deadbeef->plug_get_for_id ("medialib")) {
        gtk_notebook_remove_page (GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), 5);
        PREFWIN_TAB_INDEX_MEDIALIB = -1;
    }

    gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (mainwin));

    deadbeef->conf_lock ();

    // output plugin selection
    prefwin_init_sound_tab (prefwin);

    // replaygain_mode
    prefwin_init_playback_tab (prefwin);

    // dsp
    dsp_setup_init (prefwin);

    // minimize_on_startup
    prefwin_init_gui_misc_tab (prefwin);

    // override bar colors
    prefwin_init_appearance_tab (prefwin);

    // network
    ctmapping_setup_init (w);
    prefwin_init_network_tab (prefwin);

    // list of plugins
    prefwin_init_plugins_tab (prefwin);

    // hotkeys
    if (PREFWIN_TAB_INDEX_HOTKEYS != -1) {
        prefwin_init_hotkeys (prefwin);
    }

    deadbeef->conf_unlock ();

    g_signal_connect (GTK_DIALOG (prefwin), "response", G_CALLBACK (on_prefwin_response_cb), NULL);

    gtk_window_set_modal (GTK_WINDOW (prefwin), FALSE);
    gtk_window_set_position (GTK_WINDOW (prefwin), GTK_WIN_POS_CENTER_ON_PARENT);
}

void
prefwin_run (int tab_index) {
    _init_prefwin();

    if (tab_index != -1) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK (lookup_widget (prefwin, "notebook")), tab_index);
    }

#if GTK_CHECK_VERSION(2,28,0)
    gtk_window_present_with_time (GTK_WINDOW(prefwin), (guint32)(g_get_monotonic_time() / 1000));
#else
    gtk_window_present_with_time (prefwin, GDK_CURRENT_TIME);
#endif
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
        prefwin_set_toggle_button("hide_tray_icon", 0);
        deadbeef->conf_set_int ("gtkui.hide_tray_icon", 0);
    }
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

void
on_move_to_trash_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
    deadbeef->conf_set_int ("gtkui.move_to_trash", active);
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
        prefwin_set_toggle_button("minimize_on_startup", 0);
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

    gtk_text_buffer_set_text (buffer, text, (gint)strlen(text));
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
