/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "interface.h"
#include "support.h"
#include "playlist.h"
#include "playback.h"
#include "unistd.h"
#include "threading.h"
#include "messagepump.h"
#include "messages.h"
#include "gtkplaylist.h"
#include "codec.h"
#include "streamer.h"

GtkWidget *mainwin;
gtkplaylist_t main_playlist;
gtkplaylist_t search_playlist;

int psdl_terminate = 0;

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;

void
update_songinfo (void) {
    if (!mainwin) {
        return;
    }
    char sbtext_new[512] = "-";
    float songpos = last_songpos;
    if (p_ispaused ()) {
        strcpy (sbtext_new, "Paused");
    }
    else if (p_isstopped ()) {
        strcpy (sbtext_new, "Stopped");
    }
    else if (playlist_current.codec) {
        codec_lock ();
        codec_t *c = playlist_current.codec;
        int minpos = c->info.position / 60;
        int secpos = c->info.position - minpos * 60;
        int mindur = playlist_current.duration / 60;
        int secdur = playlist_current.duration - mindur * 60;
        const char *mode = c->info.channels == 1 ? "Mono" : "Stereo";
        int samplerate = c->info.samplesPerSecond;
        int bitspersample = c->info.bitsPerSample;
        songpos = c->info.position;
        codec_unlock ();

        snprintf (sbtext_new, 512, "[%s] %dHz | %d bit | %s | %d:%02d / %d:%02d | %d songs total | streambuffer: %d%%", playlist_current.filetype ? playlist_current.filetype:"-", samplerate, bitspersample, mode, minpos, secpos, mindur, secdur, pl_getcount (), streamer_get_fill_level ());
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
        if (mainwin) {
            GDK_THREADS_ENTER();
            seekbar_draw (lookup_widget (mainwin, "seekbar"));
            GDK_THREADS_LEAVE();
            last_songpos = songpos;
        }
    }
}

void
psdl_thread (uintptr_t ctx) {
    p_play ();
    while (!psdl_terminate) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            switch (msg) {
            case M_SONGCHANGED:
                GDK_THREADS_ENTER();
                // update window title
                int from = p1;
                int to = p2;
                if (from >= 0 || to >= 0) {
                    if (to >= 0) {
                        playItem_t *it = pl_get_for_idx (to);
                        char str[600];
                        char dname[512];
                        pl_format_item_display_name (it, dname, 512);
                        snprintf (str, 600, "DeaDBeeF - %s", dname);
                        gtk_window_set_title (GTK_WINDOW (mainwin), str);
                    }
                    else {
                        gtk_window_set_title (GTK_WINDOW (mainwin), "DeaDBeeF");
                    }
                }
                // update playlist view
                gtkpl_songchanged (&main_playlist, p1, p2);
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYSONG:
                GDK_THREADS_ENTER();
                gtkpl_playsong (&main_playlist);
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYSONGNUM:
                GDK_THREADS_ENTER();
                gtkpl_playsongnum (p1);
                GDK_THREADS_LEAVE();
                break;
            case M_STOPSONG:
                p_stop ();
                GDK_THREADS_ENTER();
                gtkpl_redraw_pl_row (&main_playlist, main_playlist.row);
                GDK_THREADS_LEAVE();
                break;
            case M_NEXTSONG:
                GDK_THREADS_ENTER();
                p_stop ();
                pl_nextsong (1);
                GDK_THREADS_LEAVE();
                break;
            case M_PREVSONG:
                GDK_THREADS_ENTER();
                p_stop ();
                pl_prevsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_PAUSESONG:
                if (p_ispaused ()) {
                    p_unpause ();
                }
                else {
                    p_pause ();
                }

                GDK_THREADS_ENTER();
                gtkpl_redraw_pl_row (&main_playlist, main_playlist.row);
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYRANDOM:
                GDK_THREADS_ENTER();
                gtkpl_randomsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_SONGSEEK:
                if (playlist_current.codec) {
                    p_pause ();
                    codec_lock ();
                    playlist_current.codec->seek (p1 / 1000.f);
                    codec_unlock ();
                    p_unpause ();
                }
                break;
            case M_ADDDIR:
                // long time processing
                gtkpl_add_dir (&main_playlist, (char *)ctx);
                break;
            case M_ADDFILES:
                gtkpl_add_files (&main_playlist, (GSList *)ctx);
                break;
            case M_FMDRAGDROP:
                gtkpl_add_fm_dropped_files (&main_playlist, (char *)ctx, p1, p2);
                break;
            }
        }
        usleep(10000);
        update_songinfo ();
    }
}

int
main (int argc, char *argv[]) {
    char *homedir = getenv ("HOME");
    if (!homedir) {
        fprintf (stderr, "unable to find home directory. stopping.\n");
        return -1;
    }
    char defpl[1024];
    snprintf (defpl, 1024, "%s/.config/deadbeef/default.dbpl", homedir);
    char confdir[1024];
    snprintf (confdir, 1024, "%s/.config", homedir);
    mkdir (confdir, 0755);
    char dbconfdir[1024];
    snprintf (dbconfdir, 1024, "%s/.config/deadbeef", homedir);
    mkdir (dbconfdir, 0755);

    gtkpl_init ();

    messagepump_init ();
    codec_init_locking ();
    streamer_init ();
    p_init ();
    thread_start (psdl_thread, 0);

    g_thread_init (NULL);
    add_pixmap_directory ("/usr/share/deadbeef/images");
    gdk_threads_init ();
    gdk_threads_enter ();
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    pl_load (defpl);
    mainwin = create_mainwin ();
    gtk_widget_show (mainwin);
    gtk_main ();
    mainwin = NULL;
    gdk_threads_leave ();
    messagepump_free ();
    psdl_terminate = 1;
    p_free ();
    streamer_free ();
    codec_free_locking ();

    pl_save (defpl);
    pl_free ();
    return 0;
}
