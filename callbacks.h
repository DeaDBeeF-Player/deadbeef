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
on_quit1_activate                      (GtkMenuItem     *menuitem,
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
