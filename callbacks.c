#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "common.h"

#include "psdl.h"
#include "playlist.h"
#include "gtkplaylist.h"
#include "messagepump.h"
#include "messages.h"
#include "codec.h"

extern GtkWidget *mainwin;

static void
addfile_func (gpointer data, gpointer userdata) {
    ps_add_file (data);
    g_free (data);
}


void
on_volume_value_changed                (GtkRange        *range,
        gpointer         user_data)
{
//    float db = -(60 - (gtk_range_get_value (range) * 0.6f))
    float a = gtk_range_get_value (range) / 100;
    psdl_set_volume (a*a);
}

int g_disable_seekbar_handler = 0;
void
on_playpos_value_changed               (GtkRange        *range,
        gpointer         user_data)
{
    if (g_disable_seekbar_handler) {
        return;
    }
    if (playlist_current.codec) {
        if (playlist_current.codec->info.duration > 0) {
            int val = gtk_range_get_value (range);
            int upper = gtk_adjustment_get_upper (gtk_range_get_adjustment (range));
            float time = playlist_current.codec->info.duration / (float)upper * (float)val;
            messagepump_push (M_SONGSEEK, 0, (int)time * 1000, 0);
        }
    }
}


// change properties
gboolean
on_playlist_configure_event            (GtkWidget       *widget,
        GdkEventConfigure *event,
        gpointer         user_data)
{
    gtkps_reconf (widget);
    return FALSE;
}

// redraw
gboolean
on_playlist_expose_event               (GtkWidget       *widget,
        GdkEventExpose  *event,
        gpointer         user_data)
{
    // draw visible area of playlist
    gtkps_expose (widget, event->area.x, event->area.y, event->area.width, event->area.height);

    return FALSE;
}


void
on_playlist_realize                    (GtkWidget       *widget,
        gpointer         user_data)
{

}


gboolean
on_playlist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1) {
        gtkps_mouse1_clicked (widget, event->state, event->x, event->y, event->time);
    }
    return FALSE;
}


void
on_playscroll_value_changed            (GtkRange        *range,
                                        gpointer         user_data)
{
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    gtkps_scroll (newscroll);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
}


void
on_add_files_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add file(s) to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    GtkFileFilter* flt;
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Supported music files");
    gtk_file_filter_add_pattern (flt, "*.ogg");
    gtk_file_filter_add_pattern (flt, "*.mod");
    gtk_file_filter_add_pattern (flt, "*.wav");
    gtk_file_filter_add_pattern (flt, "*.mp3");
    gtk_file_filter_add_pattern (flt, "*.nsf");
    gtk_file_filter_add_pattern (flt, "*.flac");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dlg), flt);
    flt = gtk_file_filter_new ();
    gtk_file_filter_set_name (flt, "Other files (*)");
    gtk_file_filter_add_pattern (flt, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dlg), flt);
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        GSList *lst = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dlg));
        g_slist_foreach(lst, addfile_func, NULL);
        g_slist_free (lst);
    }
    gtk_widget_destroy (dlg);
    gtkps_setup_scrollbar ();
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}


void
on_add_folder1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

    GtkWidget *dlg = gtk_file_chooser_dialog_new ("Add folder to playlist...", GTK_WINDOW (mainwin), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        gchar *folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        if (folder) {
            ps_add_dir (folder);
            g_free (folder);
        }
    }
    gtk_widget_destroy (dlg);
    gtkps_setup_scrollbar ();
    GtkWidget *widget = lookup_widget (mainwin, "playlist");
    draw_playlist (widget, 0, 0, widget->allocation.width, widget->allocation.height);
    gtkps_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_clear1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_select_all1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_remove1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_crop1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


gboolean
on_playlist_scroll_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    // pass event to scrollbar
    GtkWidget *range = lookup_widget (mainwin, "playscroll");
	GdkEventScroll *ev = (GdkEventScroll*)event;
    GtkAdjustment* adj = gtk_range_get_adjustment (GTK_RANGE (range));
    int newscroll = gtk_range_get_value (GTK_RANGE (range));
    if (ev->direction == GDK_SCROLL_UP) {
        newscroll -= 10;//gtk_adjustment_get_page_increment (adj);
    }
    else if (ev->direction == GDK_SCROLL_DOWN) {
        newscroll += 10;//gtk_adjustment_get_page_increment (adj);
    }
    gtk_range_set_value (GTK_RANGE (range), newscroll);
    return FALSE;
}


void
on_stopbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_STOPSONG, 0, 0, 0);
}


void
on_playbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PLAYSONG, 0, 0, 0);
}


void
on_pausebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PAUSESONG, 0, 0, 0);
}


void
on_prevbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PREVSONG, 0, 0, 0);
}


void
on_nextbtn_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_NEXTSONG, 0, 0, 0);
}


void
on_playrand_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    messagepump_push (M_PLAYRANDOM, 0, 0, 0);
}

