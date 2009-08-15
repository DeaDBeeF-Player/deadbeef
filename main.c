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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
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
GtkWidget *searchwin;
gtkplaylist_t main_playlist;
gtkplaylist_t search_playlist;

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;

// some common global variables
char confdir[1024]; // $HOME/.config
char dbconfdir[1024]; // $HOME/.config/deadbeef
char defpl[1024]; // $HOME/.config/deadbeef/default.dbpl

void
update_songinfo (void) {
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
        void seekbar_expose (GtkWidget *widget, int x, int y, int w, int h);
        if (mainwin) {
            GtkWidget *widget = lookup_widget (mainwin, "seekbar");
            // translate volume to seekbar pixels
            songpos /= playlist_current.duration;
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


// -1 error, program must exit with error code -1
//  0 proceed normally as nothing happened
//  1 no error, but program must exit with error code 0
//  2 no error, don't load default playlist, start playback immediately after startup
int
exec_command_line (const char *cmdline, int len, int filter) {
    const uint8_t *parg = (const uint8_t *)cmdline;
    const uint8_t *pend = cmdline + len;
    int exitcode = 0;
    while (parg < pend) {
        if (filter == 1) {
            if (!strcmp (parg, "--help") || !strcmp (parg, "-h")) {
                printf ("DeaDBeeF %s Copyright (C) 2009 Alexey Yakovenko\n", VERSION);
                printf ("Usage: deadbeef [options] [files]\n");
                printf ("Options:\n");
                printf ("   --help  or  -h     Print help (this message) and exit\n");
                printf ("   --play             Start playback\n");
                printf ("   --stop             Stop playback\n");
                printf ("   --pause            Pause playback\n");
                printf ("   --next             Next song in playlist\n");
                printf ("   --prev             Previous song in playlist\n");
                printf ("   --random           Previous song in playlist\n");
                return 1;
            }
        }
        else if (filter == 0) {
            if (!strcmp (parg, "--next")) {
                messagepump_push (M_NEXTSONG, 0, 0, 0);
            }
            else if (!strcmp (parg, "--prev")) {
                messagepump_push (M_PREVSONG, 0, 0, 0);
            }
            else if (!strcmp (parg, "--play")) {
                messagepump_push (M_PLAYSONG, 0, 0, 0);
            }
            else if (!strcmp (parg, "--stop")) {
                messagepump_push (M_STOPSONG, 0, 0, 0);
            }
            else if (!strcmp (parg, "--pause")) {
                messagepump_push (M_PAUSESONG, 0, 0, 0);
            }
            else if (!strcmp (parg, "--random")) {
                messagepump_push (M_PLAYRANDOM, 0, 0, 0);
            }
            else {
                if (pl_add_file (parg, NULL, NULL) >= 0) {
                    exitcode = 2;
                }
            }
        }
        parg += strlen (parg);
        parg++;
    }
    return exitcode;
}

void
player_thread (uintptr_t ctx) {
    // become a server
    struct sockaddr_un local, remote;
    unsigned s = socket(AF_UNIX, SOCK_STREAM, 0);
    int flags;
    flags = fcntl(s,F_GETFL,0);
    assert(flags != -1);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
    local.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
    snprintf (local.sun_path, 108, "%s/socket", dbconfdir);
    unlink(local.sun_path);
    int len = strlen(local.sun_path) + sizeof(local.sun_family);
    bind(s, (struct sockaddr *)&local, len);

    if (listen(s, 5) == -1) {
        perror("listen");
        exit(1);
    }
    for (;;) {
        // handle remote stuff
        int t = sizeof(remote);
        unsigned s2;
        s2 = accept(s, (struct sockaddr *)&remote, &t);
        if (s2 == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
            exit(1);
        }
        else if (s2 != -1) {
            char str[2048];
            int size;
            if (size = recv (s2, str, 2048, 0) >= 0) {
                if (size > 0) {
                    printf ("received: %s\n", str);
                }
                int res = exec_command_line (str, size, 0);
                if (res == 2) {
                    GDK_THREADS_ENTER();
                    gtkpl_playsong (&main_playlist);
                    GDK_THREADS_LEAVE();
                }
            }
            send (s2, "", 1, 0);
            close(s2);
        }

        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            switch (msg) {
            case M_TERMINATE:
                GDK_THREADS_ENTER();
                gtk_widget_hide (mainwin);
                gtk_main_quit ();
                GDK_THREADS_LEAVE();
                return;
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
        usleep(1000);
        update_songinfo ();
    }
    close (s);
}

int
main (int argc, char *argv[]) {
    char *homedir = getenv ("HOME");
    if (!homedir) {
        fprintf (stderr, "unable to find home directory. stopping.\n");
        return -1;
    }
    snprintf (defpl, 1024, "%s/.config/deadbeef/default.dbpl", homedir);
    snprintf (confdir, 1024, "%s/.config", homedir);
    mkdir (confdir, 0755);
    snprintf (dbconfdir, 1024, "%s/.config/deadbeef", homedir);
    mkdir (dbconfdir, 0755);

    // join command line into single string
    char cmdline[2048];
    char *p = cmdline;
    int size = 2048;
    cmdline[0] = 0;
    for (int i = 1; i < argc; i++) {
        if (size < 2) {
            break;
        }
        if (i > 1) {
            size--;
            p++;
        }
        int len = strlen (argv[i]);
        if (len >= size) {
            break;
        }
        memcpy (p, argv[i], len+1);
        p += len;
        size -= len;
    }
    size = 2048-size;
    if (exec_command_line (cmdline, size, 1) == 1) {
        return 0; // if it was help request
    }
    // try to connect to remote player
    {
        int s, t, len;
        struct sockaddr_un remote;
        char str[100];

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

//        printf("Trying to connect...\n");

        remote.sun_family = AF_UNIX;
        snprintf (remote.sun_path, 108, "%s/socket", dbconfdir);
        len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(s, (struct sockaddr *)&remote, len) == 0) {

            // pass args to remote and exit
            if (send(s, cmdline, size+1, 0) == -1) {
                perror ("send");
                exit (-1);
            }
            char out[1];
            if (recv(s, out, 1, 0) == -1) {
                printf ("failed to pass args to remote!\n");
                exit (-1);
            }
            exit (0);
        }
        close(s);
    }

    int res = exec_command_line (cmdline, size, 1);
    if (res == -1) {
        return -1;
    }
    else if (res == 0) {
        pl_load (defpl);
    }

    messagepump_init ();
    codec_init_locking ();
    streamer_init ();
    p_init ();
    thread_start (player_thread, 0);

    g_thread_init (NULL);
    add_pixmap_directory ("/usr/share/deadbeef/images");
    gdk_threads_init ();
    gdk_threads_enter ();
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    gtkpl_init ();

    mainwin = create_mainwin ();
    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));
    extern void main_playlist_init (GtkWidget *widget);
    main_playlist_init (lookup_widget (mainwin, "playlist"));
    extern void search_playlist_init (GtkWidget *widget);
    search_playlist_init (lookup_widget (searchwin, "searchlist"));

    if (res == 2) {
        messagepump_push (M_PLAYSONG, 0, 0, 0);
    }

    gtk_widget_show (mainwin);
    gtk_main ();
    gdk_threads_leave ();
    messagepump_free ();
    p_free ();
    streamer_free ();
    codec_free_locking ();

    pl_save (defpl);
    pl_free ();
    return 0;
}
