/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
on_seekbar_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event);

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

gboolean
on_mainwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);
void
volumebar_draw (GtkWidget *widget);
void
volumebar_expose (GtkWidget *widget, int x, int y, int w, int h);


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
on_find_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);


void
on_help1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_helpwindow_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
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

void
on_pref_output_plugin_changed          (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_toggle_status_bar_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_toggle_menu_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_toggle_column_headers_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_after_current1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_after_current_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_stop_after_queue_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_to_playback_queue1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove_from_playback_queue1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_properties1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_prefwin_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_format_cancel_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_format_ok_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_cursor_follows_playback_activate    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_searchwin_configure_event           (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_searchwin_window_state_event        (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data);

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_trackproperties_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_column_id_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_changelog1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_gpl1_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_lgpl1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pref_dynsamplerate_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_pref_close_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_tabbar_button_press_event           (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_tabbar_button_release_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_tabbar_configure_event              (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_tabbar_expose_event                 (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

gboolean
on_tabbar_motion_notify_event          (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

void
on_load_playlist1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_playlist1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_all_playlists1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);


GtkWidget*
create_ddb_listview_widget (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);


GtkWidget*
create_tabstrip_widget (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

GtkWidget*
create_volumebar_widget (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_color_light_color_set               (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_mid_color_set                 (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_dark_color_set                (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_selection_color_set           (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_back_color_set                (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_override_gtk_colors_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_mainwin_realize                     (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_color_even_row_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_odd_row_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_text_color_set                (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_color_selected_text_color_set       (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_disable_playlist_theming_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_addhotkey_activate                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_toggle_tabs                         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_toggle_eq                           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_write_tags_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_write_id3v2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_write_id3v1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_write_apev2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_strip_id3v2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_strip_id3v1_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_strip_apev2_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_id3v2_version_changed               (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_id3v1_encoding_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_ape_write_id3v2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ape_write_apev2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ape_strip_id3v2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ape_strip_apev2_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_closebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_deselect_all1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_invert_selection1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_mmb_delete_playlist_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_mmb_delete_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_new_playlist1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_override_bar_colors_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_bar_foreground_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_bar_background_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_tabstrip_mid_color_set              (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_tabstrip_light_color_set            (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_tabstrip_dark_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_tabstrip_base_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_override_tabstrip_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_override_listview_colors_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_listview_even_row_color_set         (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_odd_row_color_set          (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_selected_row_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_cursor_color_set           (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_wv_write_apev2_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_wv_write_id3v1_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_wv_strip_apev2_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_wv_strip_id3v1_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

GtkWidget*
create_seekbar (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);


void
on_proxyuser_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_proxypassword_changed               (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_hide_tray_icon_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hide_delete_from_disk_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_skip_deleted_songs_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_titlebar_format_playing_changed     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_titlebar_format_stopped_changed     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cli_add_to_playlist_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cli_playlist_name_changed           (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_statusbar_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_resume_last_session_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_resume_always_paused_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_jump_to_current_track1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_translators1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);


GtkWidget*
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_album1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_artist1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_date1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_custom2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sortfmt_activate                    (GtkEntry        *entry,
                                        gpointer         user_data);

void
gtkui_dialog_response_ok               (GtkEntry        *entry,
                                        gpointer         user_data);


void
on_shuffle_albums1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_order_shuffle_albums_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_dsp_configure_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_dsp_up_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dsp_down_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_auto_name_playlist_from_folder_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_dsp_preset_changed                  (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_dsp_preset_save_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_dsp_preset_load_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_plug_copyright_clicked              (GtkButton       *button,
                                        gpointer         user_data);

GtkWidget*
create_plugin_weblink (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

gboolean
on_metalist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_tagwriter_settings_clicked          (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_trackproperties_configure_event     (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

void
on_trackproperties_state_changed       (GtkWidget       *widget,
                                        GtkStateType     state,
                                        gpointer         user_data);

gboolean
on_trackproperties_window_state_event  (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data);

gboolean
on_prefwin_configure_event             (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
on_prefwin_window_state_event          (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data);

void
on_prefwin_realize                     (GtkWidget       *widget,
                                        gpointer         user_data);

gboolean
on_prefwin_map_event                   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_replaygain_preamp_value_changed     (GtkRange        *range,
                                        gpointer         user_data);

void
on_global_preamp_value_changed         (GtkRange        *range,
                                        gpointer         user_data);

void
on_tabstrip_text_color_set             (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_gui_plugin_changed                  (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_seekbar_fps_value_changed           (GtkRange        *range,
                                        gpointer         user_data);

void
on_gui_fps_value_changed               (GtkRange        *range,
                                        gpointer         user_data);

void
on_add_from_archives_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_ignore_archives_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_sort_by_title_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sort_by_track_nr_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sort_by_album_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sort_by_artist_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sort_by_date_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sort_by_random_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_sort_by_custom_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_convert8to16_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_design_mode1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_convert16to24_toggled                (GtkToggleButton *togglebutton,
                                         gpointer       user_data);

void
on_reset_autostop_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_reset_autostopqueue_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_editcolumn_title_changed            (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_useragent_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_enable_cp1251_recoding_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_enable_cp936_recoding_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hotkeys_list_cursor_changed         (GtkTreeView     *treeview,
                                        gpointer         user_data);

void
on_hotkey_add_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_hotkey_remove_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_hotkeys_actions_cursor_changed      (GtkTreeView     *treeview,
                                        gpointer         user_data);

void
on_hotkey_keycombo_changed             (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_hotkey_is_global_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);


gboolean
on_hotkey_keycombo_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_hotkey_keycombo_focus_in_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_hotkey_keycombo_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_hotkey_keycombo_motion_notify_event (GtkWidget       *widget,
                                        GdkEventMotion  *event,
                                        gpointer         user_data);

gboolean
on_hotkey_keycombo_button_release_event
                                        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_hotkey_keycombo_focus_in_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_hotkeys_apply_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_hotkeys_revert_clicked              (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_hotkey_keycombo_focus_out_event     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_hotkeys_set_key_clicked             (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_hotkeys_set_key_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_hotkeys_set_key_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_menu_bar1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_edit_content_type_mapping_clicked   (GtkButton       *button,
                                        gpointer         user_data);

void
on_ctmapping_add_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_ctmapping_remove_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_ctmapping_edit_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_button3_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_ctmapping_reset_clicked             (GtkButton       *button,
                                        gpointer         user_data);


void
on_hotkeys_actions_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_hotkeys_defaults_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_auto_size_columns_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_searchentry_activate                (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_stop_after_album_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_reset_autostopalbum_toggled         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_mainwin_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_enable_shift_jis_recoding_toggled   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_listview_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_group_text_color_set       (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_group_text_font_set        (GtkFontButton   *fontbutton,
                                        gpointer         user_data);

void
on_listview_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data);

void
on_listview_playing_text_bold_toggled  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_listview_playing_text_italic_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_listview_selected_text_bold_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_listview_selected_text_italic_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tabstrip_selected_text_color_set    (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_tabstrip_playing_bold_toggled       (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tabstrip_playing_italic_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tabstrip_selected_bold_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tabstrip_selected_italic_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tabstrip_text_font_set              (GtkFontButton   *fontbutton,
                                        gpointer         user_data);

void
on_tabstrip_playing_text_color_set     (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_column_text_color_set      (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_listview_column_text_font_set       (GtkFontButton   *fontbutton,
                                        gpointer         user_data);

gboolean
on_prefwin_key_press_event             (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_trkpropertis_edit_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_edit_in_place_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_remove_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_cut_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_copy_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_paste_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_capitalize_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_clean_up_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_format_from_other_fields_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_add_new_field_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_paste_fields_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_automatically_fill_values_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_crop_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_trkproperties_edit_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pref_replaygain_source_mode_changed (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pref_replaygain_processing_changed  (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_view_log_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_log_clear_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_log_window_key_press_event          (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_display_seltime_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_pref_pluginlist_row_activated       (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data);
void
on_listview_group_spacing_value_changed
                                        (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_comboboxentry_direct_sr_changed     (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_comboboxentry_sr_mult_48_changed    (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_comboboxentry_sr_mult_44_changed    (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_checkbutton_sr_override_toggled     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);


void
on_checkbutton_dependent_sr_toggled    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_minimize_on_startup_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_move_to_trash_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_plugin_conf_tab_btn_clicked         (GtkRadioButton       *button,
                                        gpointer         user_data);

void
on_plugin_info_tab_btn_clicked         (GtkRadioButton       *button,
                                        gpointer         user_data);

void
on_plugin_license_tab_btn_clicked      (GtkRadioButton       *button,
                                        gpointer         user_data);


void
on_plugin_notebook_switch_page         (GtkNotebook     *notebook,
                                        GtkWidget       *page,
                                        guint            page_num,
                                        gpointer         user_data);

void
on_plugin_conf_reset_btn_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_copy_plugin_report_menuitem_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_pref_pluginlist_button_press_event  (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_only_show_plugins_with_configuration1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_use_visualization_base_color_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_visualization_custom_color_button_color_set
                                        (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_dsp_toolbtn_up_clicked              (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_dsp_toolbtn_down_clicked            (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_dsp_configure_toolbtn_clicked       (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_dsp_listview_row_activated          (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data);

void
on_dsp_remove_toolbtn_clicked          (GtkToolButton   *toolbutton,
                                        gpointer         user_data);

void
on_dsp_add_toolbtn_toggled             (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data);

void
on_sortcancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_sortok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_sortfmt_show                        (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_use_visualization_background_color_toggled
                                        (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_visualization_custom_background_color_button_color_set
                                        (GtkColorButton  *colorbutton,
                                        gpointer         user_data);

void
on_combo_bit_override_changed          (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_mainwin_undo_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_mainwin_redo_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_autoopen_button_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
