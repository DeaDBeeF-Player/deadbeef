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
#include <string.h>
#include "../gtkui.h"
#include "../support.h"
#include "prefwin.h"
#include "prefwinmisc.h"

void
prefwin_init_gui_misc_tab (GtkWidget *_prefwin) {
    GtkWidget *w = _prefwin;
    prefwin_set_toggle_button("minimize_on_startup", deadbeef->conf_get_int ("gtkui.start_hidden", 0));

    // close_send_to_tray
    prefwin_set_toggle_button("pref_close_send_to_tray", deadbeef->conf_get_int ("close_send_to_tray", 0));

    // hide tray icon
    prefwin_set_toggle_button("hide_tray_icon", deadbeef->conf_get_int ("gtkui.hide_tray_icon", 0));

    // move_to_trash
    prefwin_set_toggle_button("move_to_trash", deadbeef->conf_get_int ("gtkui.move_to_trash", 1));

    // mmb_delete_playlist
    prefwin_set_toggle_button("mmb_delete_playlist", deadbeef->conf_get_int ("gtkui.mmb_delete_playlist", 1));

    // hide_delete_from_disk
    prefwin_set_toggle_button("hide_delete_from_disk", deadbeef->conf_get_int ("gtkui.hide_remove_from_disk", 0));

    // play next song when currently played is deleted
    prefwin_set_toggle_button("skip_deleted_songs", deadbeef->conf_get_int ("gtkui.skip_deleted_songs", 0));

    // auto-rename playlist from folder name
    prefwin_set_toggle_button("auto_name_playlist_from_folder", deadbeef->conf_get_int ("gtkui.name_playlist_from_folder", 1));

    // refresh rate
    int val = deadbeef->conf_get_int ("gtkui.refresh_rate", 10);
    prefwin_set_scale("gui_fps", val);

    // titlebar text
    char titlebar[1024];
    deadbeef->conf_get_str ("gtkui.titlebar_playing_tf", gtkui_default_titlebar_playing, titlebar, sizeof (titlebar));
    if (*titlebar == 0) {
        strncat (titlebar, gtkui_default_titlebar_playing, sizeof (titlebar)-1);
    }

    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "titlebar_format_playing")), titlebar);

    deadbeef->conf_get_str ("gtkui.titlebar_stopped_tf", gtkui_default_titlebar_stopped, titlebar, sizeof (titlebar));
    if (*titlebar == 0) {
        strncat (titlebar, gtkui_default_titlebar_stopped, sizeof (titlebar)-1);
    }

    gtk_entry_set_text (GTK_ENTRY (lookup_widget (w, "titlebar_format_stopped")), titlebar);

    // statusbar selection playback time
    prefwin_set_toggle_button ("display_seltime", deadbeef->conf_get_int ("gtkui.statusbar_seltime", 0));

    // enable shift-jis recoding
    prefwin_set_toggle_button("enable_shift_jis_recoding", deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0));

    // enable cp1251 recoding
    prefwin_set_toggle_button("enable_cp1251_recoding", deadbeef->conf_get_int ("junk.enable_cp1251_detection", 1));

    // enable cp936 recoding
    prefwin_set_toggle_button("enable_cp936_recoding", deadbeef->conf_get_int ("junk.enable_cp936_detection", 0));

    // enable auto-sizing of columns
    prefwin_set_toggle_button("auto_size_columns", deadbeef->conf_get_int ("gtkui.autoresize_columns", 0));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON (lookup_widget (w, "listview_group_spacing")), deadbeef->conf_get_int ("playlist.groups.spacing", 0));

    // fill gui plugin list
    GtkComboBox *combobox = GTK_COMBO_BOX (lookup_widget (w, "gui_plugin"));
    const char **names = deadbeef->plug_get_gui_names ();
    for (int i = 0; names[i]; i++) {
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combobox), names[i]);
        if (!strcmp (names[i], deadbeef->conf_get_str_fast ("gui_plugin", "GTK3"))) {
            prefwin_set_combobox (combobox, i);
        }
    }
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
    const char *text = gtk_entry_get_text (GTK_ENTRY (editable));
    if (*text == 0) {
        text = NULL;
    }
    deadbeef->conf_set_str ("gtkui.titlebar_playing_tf", text);
    gtkui_titlebar_tf_init ();
    gtkui_set_titlebar (NULL);
}


void
on_titlebar_format_stopped_changed     (GtkEditable     *editable,
                                        gpointer         user_data)
{
    const char *text = gtk_entry_get_text (GTK_ENTRY (editable));
    if (*text == 0) {
        text = NULL;
    }
    deadbeef->conf_set_str ("gtkui.titlebar_stopped_tf", text);
    gtkui_titlebar_tf_init ();
    gtkui_set_titlebar (NULL);
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
