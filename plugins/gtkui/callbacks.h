/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <gtk/gtk.h>


void
on_addbtn_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_volume_value_changed                (GtkRange        *range,
                                        gpointer         user_data);

void
on_playpos_value_changed               (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_playlist_configure_event            (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_playlist_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
on_playlist_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_playscroll_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_files_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_folder1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_mainwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_playlist_drag_begin                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
on_playlist_drag_motion                (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_playlist_drag_drop                  (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_drag_data_get              (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_drag_end                   (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
on_playlist_drag_failed                (GtkWidget       *widget,
                                        GdkDragContext  *arg1,
                                        GtkDragResult    arg2,
                                        gpointer         user_data);

void
on_playlist_drag_leave                 (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        guint            time,
                                        gpointer         user_data);

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

void
on_voice1_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_voice2_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_voice3_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_voice4_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_voice5_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_playlist_drag_data_received         (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gint             x,
                                        gint             y,
                                        GtkSelectionData *data,
                                        guint            info,
                                        guint            time,
                                        gpointer         user_data);

void
on_playlist_drag_data_delete           (GtkWidget       *widget,
                                        GdkDragContext  *drag_context,
                                        gpointer         user_data);

gboolean
on_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

void
on_header_realize                      (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_order_linear_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_order_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_loop_all_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_loop_single_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_loop_disable_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_searchwin_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_searchentry_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_searchheader_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_searchheader_button_release_event   (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_searchheader_configure_event        (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_searchheader_expose_event           (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_searchheader_motion_notify_event    (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_searchlist_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_searchlist_configure_event          (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_searchlist_expose_event             (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_searchlist_scroll_event             (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_searchscroll_value_changed          (GtkRange        *range,
                                        gpointer         user_data);

void
on_searchlist_realize                  (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_header_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_header_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_header_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_header_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_header_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_expose_event               (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_playlist_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_playlist_motion_notify_event        (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);


void
on_playscroll_value_changed            (GtkRange        *range,
                                        gpointer         user_data);

void
on_playlist_load_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_playlist_save_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_playlist_save_as_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_seekbar_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_seekbar_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_seekbar_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_seekbar_expose_event                (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_volumebar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_volumebar_button_release_event      (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_volumebar_configure_event           (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_volumebar_expose_event              (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_volumebar_motion_notify_event       (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

void
on_main_quit                           (GtkObject       *object,
                                        gpointer         user_data);

gboolean
on_mainwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);
void
volumebar_draw (GtkWidget *widget);
void
volumebar_expose (GtkWidget *widget, int x, int y, int w, int h);



void
on_progress_abort                      (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_addprogress_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_volumebar_scroll_event              (GtkWidget       *widget,
                                        GdkEventScroll        *event,
                                        gpointer         user_data);


void
on_order_shuffle_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_mainwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

void
on_cursor_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_scroll_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_find_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_folders_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_find_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);


void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_playhscroll_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_searchhscroll_value_changed         (GtkRange        *range,
                                        gpointer         user_data);

gboolean
on_helpwindow_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_add_audio_cd_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_preferences_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pref_soundcard_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pref_samplerate_changed             (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_pref_src_quality_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pref_replaygain_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_replaygain_scale_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_close_send_to_tray_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_plugin_configure_activate      (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_src_quality_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_conf_replaygain_mode_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pref_replaygain_mode_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pref_pluginlist_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data);

gboolean
on_header_popup_menu                   (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_artist_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_album_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_tracknum_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_duration_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_playing_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_title_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_custom_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pref_alsa_resampling_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_alsa_freewhenstopped_clicked   (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_soundcard_editing_done         (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_pref_soundcard_changed              (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pref_network_proxyaddress_changed   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_pref_network_enableproxy_clicked    (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_network_proxyport_changed      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_pref_network_proxytype_changed      (GtkComboBox     *combobox,
                                        gpointer         user_data);

gboolean
on_prefwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_addlocation_ok_activate             (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_addlocation_key_press_event         (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_add_location_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_location_activate                   (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_addlocation_ok_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_addlocation_entry_activate          (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_configure_plugin_clicked            (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_mainwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data);
