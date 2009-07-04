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
