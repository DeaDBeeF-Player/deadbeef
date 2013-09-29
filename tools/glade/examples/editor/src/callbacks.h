#include <gtk/gtk.h>


gboolean
on_main_window_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_New_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Save_As_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Exit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Clear_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_Font_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_About_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_new_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_open_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_open_filesel_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_open_filesel_ok_button_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_open_filesel_cancel_button_clicked  (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_fontsel_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_fontsel_ok_button_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_fontsel_cancel_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_fontsel_apply_button_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_about_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_save_filesel_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_save_filesel_ok_button_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_filesel_cancel_button_clicked  (GtkButton       *button,
                                        gpointer         user_data);

void set_window_title (GtkWidget *main_window);

void
on_text_changed                        (GtkTextBuffer   *buffer,
                                        gpointer         user_data);
