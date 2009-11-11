/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "../../deadbeef.h"
#include <gtk/gtk.h>
#include "gtkplaylist.h"
#include "search.h"
#include "progress.h"
#include "interface.h"
#include "callbacks.h"
#include "support.h"

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

// playlist configuration structures
gtkplaylist_t main_playlist;
gtkplaylist_t search_playlist;

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;

static void
update_songinfo (void) {
    char sbtext_new[512] = "-";
    float songpos = last_songpos;

    int daystotal = (int)pl_totaltime / (3600*24);
    int hourtotal = ((int)pl_totaltime / 3600) % 24;
    int mintotal = ((int)pl_totaltime/60) % 60;
    int sectotal = ((int)pl_totaltime) % 60;

    char totaltime_str[512] = "";
    if (daystotal == 0)
        snprintf (totaltime_str, sizeof (totaltime_str), "%d:%02d:%02d", hourtotal, mintotal, sectotal);

    else if (daystotal == 1)
        snprintf (totaltime_str, sizeof (totaltime_str), "1 day %d:%02d:%02d", hourtotal, mintotal, sectotal);

    else
        snprintf (totaltime_str, sizeof (totaltime_str), "%d days %d:%02d:%02d", daystotal, hourtotal, mintotal, sectotal);

    if (p_isstopped ()) {
        snprintf (sbtext_new, sizeof (sbtext_new), "Stopped | %s total playtime", totaltime_str);
        songpos = 0;
    }
    else if (str_playing_song.decoder) {
//        codec_lock ();
        DB_decoder_t *c = str_playing_song.decoder;
        float playpos = streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = str_playing_song._duration / 60;
        int secdur = str_playing_song._duration - mindur * 60;

        const char *mode = c->info.channels == 1 ? "Mono" : "Stereo";
        int samplerate = c->info.samplerate;
        int bitspersample = c->info.bps;
        songpos = playpos;
//        codec_unlock ();

        char t[100];
        if (str_playing_song._duration >= 0) {
            snprintf (t, sizeof (t), "%d:%02d", mindur, secdur);
        }
        else {
            strcpy (t, "-:--");
        }

        char sbitrate[20] = "";
#if 0 // NOTE: do not enable that for stable branch yet
        int bitrate = streamer_get_bitrate ();
        if (bitrate > 0) {
            snprintf (sbitrate, sizeof (sbitrate), "%d kbps ", bitrate);
        }
#endif
        const char *spaused = p_ispaused () ? "Paused | " : "";
        snprintf (sbtext_new, sizeof (sbtext_new), "%s%s %s| %dHz | %d bit | %s | %d:%02d / %s | %d songs | %s total playtime", spaused, str_playing_song.filetype ? str_playing_song.filetype:"-", sbitrate, samplerate, bitspersample, mode, minpos, secpos, t, pl_getcount (), totaltime_str);
    }

    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);

        // form statusline
        GDK_THREADS_ENTER();
        // FIXME: don't update if window is not visible
        GtkStatusbar *sb = GTK_STATUSBAR (lookup_widget (mainwin, "statusbar"));
        if (sb_context_id == -1) {
            sb_context_id = gtk_statusbar_get_context_id (sb, "msg");
        }

        gtk_statusbar_pop (sb, sb_context_id);
        gtk_statusbar_push (sb, sb_context_id, sb_text);

        GDK_THREADS_LEAVE();
    }

    if (songpos != last_songpos) {
        void seekbar_draw (GtkWidget *widget);
        void seekbar_expose (GtkWidget *widget, int x, int y, int w, int h);
        if (mainwin) {
            GtkWidget *widget = lookup_widget (mainwin, "seekbar");
            // translate volume to seekbar pixels
            songpos /= str_playing_song._duration;
            songpos *= widget->allocation.width;
            if ((int)(songpos*2) != (int)(last_songpos*2)) {
                GDK_THREADS_ENTER();
                seekbar_draw (widget);
                seekbar_expose (widget, 0, 0, widget->allocation.width, widget->allocation.height);
                GDK_THREADS_LEAVE();
                last_songpos = songpos;
            }
        }
    }
}

gboolean
on_trayicon_scroll_event               (GtkWidget       *widget,
                                        GdkEventScroll  *event,
                                        gpointer         user_data)
{
    float vol = volume_get_db ();
    if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_RIGHT) {
        vol += 1;
    }
    else if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_LEFT) {
        vol -= 1;
    }
    if (vol > 0) {
        vol = 0;
    }
    else if (vol < -60) {
        vol = -60;
    }
    volume_set_db (vol);
    GtkWidget *volumebar = lookup_widget (mainwin, "volumebar");
    volumebar_draw (volumebar);
    volumebar_expose (volumebar, 0, 0, volumebar->allocation.width, volumebar->allocation.height);
    return FALSE;
}

#if GTK_MINOR_VERSION<=14
gboolean
on_trayicon_activate (GtkWidget       *widget,
                                        GdkEvent  *event,
                                        gpointer         user_data)
{
    if (GTK_WIDGET_VISIBLE (mainwin)) {
        gtk_widget_hide (mainwin);
    }
    else {
        gtk_widget_show (mainwin);
        gtk_window_present (GTK_WINDOW (mainwin));
    }
    return FALSE;
}
#endif

gboolean
on_trayicon_button_press_event (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 1) {
        if (GTK_WIDGET_VISIBLE (mainwin)) {
            gtk_widget_hide (mainwin);
        }
        else {
            gtk_widget_show (mainwin);
            gtk_window_present (GTK_WINDOW (mainwin));
        }
    }
    return FALSE;
}

gboolean
on_trayicon_popup_menu (GtkWidget       *widget,
        guint button,
        guint time,
                                        gpointer         user_data)
{
    gtk_menu_popup (GTK_MENU (traymenu), NULL, NULL, gtk_status_icon_position_menu, trayicon, button, time);
    return FALSE;
}

void
guiplug_showwindow (void) {
    GDK_THREADS_ENTER();
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    GDK_THREADS_LEAVE();
}

void
guiplug_play_current_song (void) {
    GDK_THREADS_ENTER();
    gtkpl_playsong (&main_playlist);
    GDK_THREADS_LEAVE();
}

void
guiplug_shutdown (void) {
    GDK_THREADS_ENTER();
    gtk_widget_hide (mainwin);
    gtk_main_quit ();
    GDK_THREADS_LEAVE();
}

void
guiplug_start_current_track (void) {
    gtkpl_playsong (&main_playlist);
    if (playlist_current_ptr) {
        GDK_THREADS_ENTER();
        gtkpl_redraw_pl_row (&main_playlist, pl_get_idx_of (playlist_current_ptr), playlist_current_ptr);
        GDK_THREADS_LEAVE();
    }
}
void
guiplug_track_changed (int idx) {
    playItem_t *it = pl_get_for_idx (idx);
    if (it) {
        GDK_THREADS_ENTER();
        gtkpl_redraw_pl_row (&main_playlist, idx, it);
        if (it == playlist_current_ptr) {
            gtkpl_current_track_changed (it);
        }
        GDK_THREADS_LEAVE();
    }
}

void
guiplug_start_track (int idx) {
    GDK_THREADS_ENTER();
    gtkpl_playsongnum (idx);
    GDK_THREADS_LEAVE();
}

void
guiplug_track_paused (int idx) {
    GDK_THREADS_ENTER();
    gtkpl_redraw_pl_row (&main_playlist, idx, pl_get_for_idx (idx));
    GDK_THREADS_LEAVE();
}

void
guiplug_start_random (void) {
    GDK_THREADS_ENTER();
    gtkpl_randomsong ();
    GDK_THREADS_LEAVE();
}

void
guiplug_add_dir (const char *dir) {
    // long time processing
    //                float t1 = (float)clock () / CLOCKS_PER_SEC;
    gtkpl_add_dir (&main_playlist, dir);
    //                float t2 = (float)clock () / CLOCKS_PER_SEC;
    //                printf ("time: %f\n", t2-t1);
}

void
guiplug_add_dirs (GSList *dirs) {
    // long time processing
    //                float t1 = (float)clock () / CLOCKS_PER_SEC;
    gtkpl_add_dirs (&main_playlist, dirs);
    //                float t2 = (float)clock () / CLOCKS_PER_SEC;
    //                printf ("time: %f\n", t2-t1);
}

void
guiplug_add_files (GSList *files) {
    gtkpl_add_files (&main_playlist, files);
}

void
guiplug_open_files (GSList *files) {
    gtkpl_add_files (&main_playlist, files);
    gtkpl_playsong (&main_playlist);
}

void
guiplug_refresh_playlist (void) {
    GDK_THREADS_ENTER();
    playlist_refresh ();
    search_refresh ();
    GDK_THREADS_LEAVE();
}

void
guiplug_add_fm_dropped_files (char *files, int p1, int p2) {
    gtkpl_add_fm_dropped_files (&main_playlist, (char *)ctx, p1, p2);
}

void
guiplug_frameupdate (void) {
    update_songinfo ();
}

static int
gtkui_on_activate (DB_event_t *ev, uintptr_t data) {
    GDK_THREADS_ENTER();
    gtk_widget_show (mainwin);
    gtk_window_present (GTK_WINDOW (mainwin));
    GDK_THREADS_LEAVE();
}

static int
gtkui_on_songchanged (DB_event_song_t *ev, uintptr_t data) {
    gtkpl_songchanged_wrapper (from, to);
}

static int
gtkui_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    return 0;
}

static int
gtkui_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_ACTIVATE, DB_CALLBACK (gtkui_on_activate), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, DB_CALLBACK (gtkui_on_songchanged), 0);
    return 0;
}

// define plugin interface
static DB_gui_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "Standard GTK2 user interface",
    .plugin.descr = "",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = gtkui_start,
    .plugin.stop = gtkui_stop
};
