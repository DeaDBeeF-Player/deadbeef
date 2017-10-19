#include <gtk/gtk.h>

void
on_converter_encoder_changed           (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_presets_cursor_changed              (GtkTreeView     *treeview,
                                        gpointer         user_data);

void
on_dsp_preset_add_plugin_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_dsp_preset_remove_plugin_clicked    (GtkButton       *button,
                                        gpointer         user_data);

void
on_converter_dsp_preset_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_dsp_preset_plugin_configure_clicked (GtkButton       *button,
                                        gpointer         user_data);

void
on_dsp_preset_plugin_up_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_dsp_preset_plugin_down_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_converter_output_format_changed     (GtkComboBox     *combobox,
                                        gpointer         user_data);

GtkWidget*
encoder_cmdline_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);


void
on_edit_encoder_presets_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_edit_dsp_presets_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_converter_output_browse_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_converter_cancel_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_converter_ok_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_converterdlg_close                  (GtkDialog       *dialog,
                                        gpointer         user_data);

void
on_converterdlg_response               (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

gboolean
on_converterdlg_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

GtkWidget*
title_formatting_help_link_create (gchar *widget_name, gchar *string1, gchar *string2,
                gint int1, gint int2);

void
on_output_folder_changed               (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_numthreads_changed                  (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_overwrite_action_changed            (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_encoder_changed                     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_preserve_folder_browse_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_fname_changed                       (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_checkbutton1_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_preserve_folders_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_output_file_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_preserve_folder_root_changed        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_preserve_root_folder_changed        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_write_to_source_folder_toggled      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_bypass_same_format_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_retag_after_copy_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
