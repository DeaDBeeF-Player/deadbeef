/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
on_add_to_playback_queue1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove_from_playback_queue1_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove2_activate                    (GtkMenuItem     *menuitem,
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

void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_edit_column_activate                (GtkMenuItem     *menuitem,
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
on_rename_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_remove_playlist1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_new_playlist1_activate          (GtkMenuItem     *menuitem,
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

gboolean
on_mainwin_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_mainwin_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_mainwin_scroll_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_mainwin_motion_notify_event         (GtkWidget       *widget,
                                        GdkEventMotion  *event,
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
on_embolden_current_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_hide_delete_from_disk_toggled       (GtkToggleButton *togglebutton,
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
