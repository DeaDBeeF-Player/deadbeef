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

#include <gtk/gtk.h>
#include "../gtkui.h"
#include "../support.h"
#include "prefwin.h"
#include "prefwinplayback.h"

static GtkWidget *prefwin;

void
prefwin_init_playback_tab (GtkWidget *_prefwin) {
    GtkWidget *w = prefwin = _prefwin;
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (w, "pref_replaygain_source_mode"));
    prefwin_set_combobox (combobox, deadbeef->conf_get_int ("replaygain.source_mode", 0));

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

    prefwin_set_combobox (combobox, processing_idx);

    // preamp with rg
    prefwin_set_scale("replaygain_preamp", deadbeef->conf_get_int ("replaygain.preamp_with_rg", 0));

    // preamp without rg
    prefwin_set_scale("global_preamp", deadbeef->conf_get_int ("replaygain.preamp_without_rg", 0));

    // cli playlist
    int active = deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1);
    prefwin_set_toggle_button("cli_add_to_playlist", active);
    gtk_widget_set_sensitive (lookup_widget (w, "cli_playlist_name"), active);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "cli_playlist_name")), deadbeef->conf_get_str_fast ("cli_add_playlist_name", "Default"));

    // resume last session
    prefwin_set_toggle_button("resume_last_session", deadbeef->conf_get_int ("resume_last_session", 1));

    // resume always paused
    prefwin_set_toggle_button("resume_always_paused", deadbeef->conf_get_int ("resume_always_paused", 0));

    // add from archives
    prefwin_set_toggle_button("ignore_archives", deadbeef->conf_get_int ("ignore_archives", 1));

    // reset autostop
    prefwin_set_toggle_button("reset_autostop", deadbeef->conf_get_int ("playlist.stop_after_current_reset", 0));

    // reset queue autostop
    prefwin_set_toggle_button("reset_autostopqueue", deadbeef->conf_get_int ("playlist.stop_after_queue_reset", 0));

    // reset album autostop
    prefwin_set_toggle_button("reset_autostopalbum", deadbeef->conf_get_int ("playlist.stop_after_album_reset", 0));
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
on_resume_always_paused_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    int active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton));
    deadbeef->conf_set_int ("resume_always_paused", active);
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
on_reset_autostopqueue_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_queue_reset", gtk_toggle_button_get_active (togglebutton));
}

void
on_reset_autostopalbum_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    deadbeef->conf_set_int ("playlist.stop_after_album_reset", gtk_toggle_button_get_active (togglebutton));
}
