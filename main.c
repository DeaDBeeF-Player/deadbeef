/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/prctl.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "interface.h"
#include "callbacks.h"
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
#include "search.h"
#include "progress.h"
#include "conf.h"
#include "volume.h"
#include "session.h"
#include "plugins.h"

#ifndef PREFIX
#error PREFIX must be defined
#endif

// some common global variables
char confdir[1024]; // $HOME/.config
char dbconfdir[1024]; // $HOME/.config/deadbeef
char defpl[1024]; // $HOME/.config/deadbeef/default.dbpl
char sessfile[1024]; // $HOME/.config/deadbeef/session

// main widgets
GtkWidget *mainwin;
GtkWidget *searchwin;
GtkStatusIcon *trayicon;
GtkWidget *traymenu;

void
set_tray_tooltip (const char *text) {
#if (GTK_MINOR_VERSION < 16)
        gtk_status_icon_set_tooltip (trayicon, text);
#else
        gtk_status_icon_set_tooltip_text (trayicon, text);
#endif
}

// playlist configuration structures
gtkplaylist_t main_playlist;
gtkplaylist_t search_playlist;

// update status bar and window title
static int sb_context_id = -1;
static char sb_text[512];
static float last_songpos = -1;

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
    else if (str_playing_song.decoder) {
        codec_lock ();
        DB_decoder_t *c = str_playing_song.decoder;
        float playpos = streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = str_playing_song.duration / 60;
        int secdur = str_playing_song.duration - mindur * 60;
        const char *mode = c->info.channels == 1 ? "Mono" : "Stereo";
        int samplerate = c->info.samplerate;
        int bitspersample = c->info.bps;
        songpos = playpos;
        codec_unlock ();

        snprintf (sbtext_new, 512, "[%s] %dHz | %d bit | %s | %d:%02d / %d:%02d | %d songs total", str_playing_song.filetype ? str_playing_song.filetype:"-", samplerate, bitspersample, mode, minpos, secpos, mindur, secdur, pl_getcount ());
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
            songpos /= str_playing_song.duration;
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
//  2 no error, start playback immediately after startup
//  3 no error, don't start playback immediately after startup
int
exec_command_line (const char *cmdline, int len, int filter) {
    const uint8_t *parg = (const uint8_t *)cmdline;
    const uint8_t *pend = cmdline + len;
    int exitcode = 0;
    int queue = 0;
    while (parg < pend) {
        if (filter == 1) {
            if (!strcmp (parg, "--help") || !strcmp (parg, "-h")) {
                printf ("Usage: deadbeef [options] [file(s)]\n");
                printf ("Options:\n");
                printf ("   --help  or  -h     Print help (this message) and exit\n");
                printf ("   --version          Print version info and exit\n");
                printf ("   --play             Start playback\n");
                printf ("   --stop             Stop playback\n");
                printf ("   --pause            Pause playback\n");
                printf ("   --next             Next song in playlist\n");
                printf ("   --prev             Previous song in playlist\n");
                printf ("   --random           Previous song in playlist\n");
                printf ("   --queue            Append file(s) to existing playlist\n");
                return 1;
            }
            else if (!strcmp (parg, "--version")) {
                printf ("DeaDBeeF %s Copyright (C) 2009 Alexey Yakovenko\n", VERSION);
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
            else if (!strcmp (parg, "--queue")) {
                queue = 1;
            }
            else if (parg[0] != '-') {
                break;
            }
        }
        parg += strlen (parg);
        parg++;
    }
    if (parg < pend) {
        // add files
        if (!queue) {
            pl_free ();
        }
        while (parg < pend) {
            char resolved[PATH_MAX];
            if (!realpath (parg, resolved)) {
                fprintf (stderr, "error: cannot expand filename %s, file will not play\n", parg);
            }
            else {
                if (pl_add_file (resolved, NULL, NULL) >= 0) {
                    if (queue) {
                        exitcode = 3;
                    }
                    else {
                        exitcode = 2;
                    }
                }
            }
            parg += strlen (parg);
            parg++;
        }
    }
    if (exitcode == 2 || exitcode == 3) {
        // added some files, need to redraw
        messagepump_push (M_PLAYLISTREFRESH, 0, 0, 0);
    }
    return exitcode;
}

static struct sockaddr_un srv_local;
static struct sockaddr_un srv_remote;
static unsigned srv_socket;

int
server_start (void) {
    srv_socket = socket (AF_UNIX, SOCK_STREAM, 0);
    int flags;
    flags = fcntl (srv_socket, F_GETFL,0);
    if (flags == -1) {
        fprintf (stderr, "server_start failed, flags == -1\n");
        return -1;
    }
    fcntl(srv_socket, F_SETFL, flags | O_NONBLOCK);
    srv_local.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
    snprintf (srv_local.sun_path, 108, "%s/socket", dbconfdir);
    unlink(srv_local.sun_path);
    int len = strlen(srv_local.sun_path) + sizeof(srv_local.sun_family);
    bind(srv_socket, (struct sockaddr *)&srv_local, len);

    if (listen(srv_socket, 5) == -1) {
        perror("listen");
        return -1;
    }
    return 0;
}

void
server_close (void) {
    if (srv_socket) {
        close (srv_socket);
        srv_socket = 0;
    }
}

int
server_update (void) {
    // handle remote stuff
    int t = sizeof (srv_remote);
    unsigned s2;
    s2 = accept(srv_socket, (struct sockaddr *)&srv_remote, &t);
    if (s2 == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("accept");
        return -1;
    }
    else if (s2 != -1) {
        char str[2048];
        int size;
        if ((size = recv (s2, str, 2048, 0)) >= 0) {
            if (size == 1 && str[0] == 0) {
                GDK_THREADS_ENTER();
                gtk_widget_show (mainwin);
                gtk_window_present (GTK_WINDOW (mainwin));
                GDK_THREADS_LEAVE();
            }
            else {
                int res = exec_command_line (str, size, 0);
                if (res == 2) {
                    GDK_THREADS_ENTER();
                    gtkpl_playsong (&main_playlist);
                    GDK_THREADS_LEAVE();
                }
            }
        }
        send (s2, "", 1, 0);
        close(s2);
    }
    return 0;
}

void
player_thread (uintptr_t ctx) {
    prctl (PR_SET_NAME, "deadbeef-player", 0, 0, 0, 0);
    for (;;) {
        static int srvupd_count = 0;
        if (--srvupd_count <= 0) {
            srvupd_count = 10;
            if (server_update () < 0) {
                messagepump_push (M_TERMINATE, 0, 0, 0);
            }
        }
        plug_trigger_event (DB_EV_FRAMEUPDATE);
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            switch (msg) {
            case M_REINIT_SOUND:
                {
                    int play = 0;
                    if (!palsa_ispaused () && !palsa_isstopped ()) {
                        play = 1;
                    }
                
                    palsa_free ();
                    palsa_init ();
                    if (play) {
                        palsa_play ();
                    }
                }
                break;
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
                        set_tray_tooltip (str);
                    }
                    else {
                        gtk_window_set_title (GTK_WINDOW (mainwin), "DeaDBeeF");
                        set_tray_tooltip ("DeaDBeeF");
                    }
                }
                // update playlist view
                gtkpl_songchanged (&main_playlist, p1, p2);
                GDK_THREADS_LEAVE();
                plug_trigger_event (DB_EV_SONGCHANGED);
                break;
            case M_PLAYSONG:
                gtkpl_playsong (&main_playlist);
                break;
            case M_PLAYSONGNUM:
                GDK_THREADS_ENTER();
                gtkpl_playsongnum (p1);
                GDK_THREADS_LEAVE();
                break;
            case M_STOPSONG:
                //p_stop ();
                streamer_set_nextsong (-2, 0);
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
                if (playlist_current_ptr) {
                    gtkpl_redraw_pl_row (&main_playlist, pl_get_idx_of (playlist_current_ptr), playlist_current_ptr);
                }
                GDK_THREADS_LEAVE();
                break;
            case M_PLAYRANDOM:
                GDK_THREADS_ENTER();
                gtkpl_randomsong ();
                GDK_THREADS_LEAVE();
                break;
            case M_ADDDIR:
                {
                // long time processing
//                float t1 = (float)clock () / CLOCKS_PER_SEC;
                gtkpl_add_dir (&main_playlist, (char *)ctx);
//                float t2 = (float)clock () / CLOCKS_PER_SEC;
//                printf ("time: %f\n", t2-t1);
                }
                break;
            case M_ADDDIRS:
                {
                // long time processing
//                float t1 = (float)clock () / CLOCKS_PER_SEC;
                gtkpl_add_dirs (&main_playlist, (GSList *)ctx);
//                float t2 = (float)clock () / CLOCKS_PER_SEC;
//                printf ("time: %f\n", t2-t1);
                }
                break;
            case M_ADDFILES:
                gtkpl_add_files (&main_playlist, (GSList *)ctx);
                break;
            case M_OPENFILES:
                p_stop ();
                gtkpl_add_files (&main_playlist, (GSList *)ctx);
                gtkpl_playsong (&main_playlist);
                break;
            case M_FMDRAGDROP:
                gtkpl_add_fm_dropped_files (&main_playlist, (char *)ctx, p1, p2);
                break;
            case M_PLAYLISTREFRESH:
                GDK_THREADS_ENTER();
                playlist_refresh ();
                search_refresh ();
                GDK_THREADS_LEAVE();
                break;
            }
        }
        usleep(50000);
        update_songinfo ();
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

int
main (int argc, char *argv[]) {
    srand (time (NULL));
    prctl (PR_SET_NAME, "deadbeef-main", 0, 0, 0, 0);
    char *homedir = getenv ("HOME");
    if (!homedir) {
        fprintf (stderr, "unable to find home directory. stopping.\n");
        return -1;
    }

    char *xdg_conf_dir = getenv ("XDG_CONFIG_HOME");
    if (xdg_conf_dir) {
        if (snprintf (confdir, sizeof (confdir), "%s", xdg_conf_dir) > sizeof (confdir)) {
            fprintf (stderr, "fatal: XDG_CONFIG_HOME value is too long: %s\n", xdg_conf_dir);
            return -1;
        }
    }
    else {
        if (snprintf (confdir, sizeof (confdir), "%s/.config", homedir) > sizeof (confdir)) {
            fprintf (stderr, "fatal: HOME value is too long: %s\n", homedir);
            return -1;
        }
    }
    if (snprintf (defpl, sizeof (defpl), "%s/deadbeef/default.dbpl", confdir) > sizeof (defpl)) {
        fprintf (stderr, "fatal: out of memory while configuring\n");
        return -1;
    }
    if (snprintf (sessfile, sizeof (sessfile), "%s/deadbeef/session", confdir) > sizeof (sessfile)) {
        fprintf (stderr, "fatal: out of memory while configuring\n");
        return -1;
    }
    mkdir (confdir, 0755);
    if (snprintf (dbconfdir, sizeof (dbconfdir), "%s/deadbeef", confdir) > sizeof (dbconfdir)) {
        fprintf (stderr, "fatal: out of memory while configuring\n");
        return -1;
    }
    mkdir (dbconfdir, 0755);

    char cmdline[2048];
    int size = 0;
    if (argc > 1) {
        size = 2048;
        // join command line into single string
        char *p = cmdline;
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
        size = 2048 - size + 1;
        if (exec_command_line (cmdline, size, 1) == 1) {
            return 0; // if it was help request
        }
    }
    // try to connect to remote player
    int s, t, len;
    struct sockaddr_un remote;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    remote.sun_family = AF_UNIX;
    snprintf (remote.sun_path, 108, "%s/socket", dbconfdir);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == 0) {
        if (argc <= 1) {
            cmdline[0] = 0;
            size = 1;
        }

        // pass args to remote and exit
        if (send(s, cmdline, size, 0) == -1) {
            perror ("send");
            exit (-1);
        }
        char out[1];
        if (recv(s, out, 1, 0) == -1) {
            fprintf (stderr, "failed to pass args to remote!\n");
            exit (-1);
        }
        close (s);
        exit (0);
    }
    close(s);

    // become a server
    server_start ();

    conf_load ();
    plug_load_all ();
    pl_load (defpl);
    session_reset ();
    session_load (sessfile);
    messagepump_init ();
    codec_init_locking ();
    streamer_init ();
    p_init ();
    thread_start (player_thread, 0);

    g_thread_init (NULL);
    add_pixmap_directory (PREFIX "/share/deadbeef/pixmaps");
    gdk_threads_init ();
    gdk_threads_enter ();
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    // system tray icon
    traymenu = create_traymenu ();
    GdkPixbuf *trayicon_pixbuf = create_pixbuf ("play_24.png");
    trayicon = gtk_status_icon_new_from_pixbuf (trayicon_pixbuf);
    set_tray_tooltip ("DeaDBeeF");
    //gtk_status_icon_set_title (GTK_STATUS_ICON (trayicon), "DeaDBeeF");
#if GTK_MINOR_VERSION <= 14
    g_signal_connect ((gpointer)trayicon, "activate", G_CALLBACK (on_trayicon_activate), NULL);
#else
    g_signal_connect ((gpointer)trayicon, "scroll_event", G_CALLBACK (on_trayicon_scroll_event), NULL);
    g_signal_connect ((gpointer)trayicon, "button_press_event", G_CALLBACK (on_trayicon_button_press_event), NULL);
#endif
    g_signal_connect ((gpointer)trayicon, "popup_menu", G_CALLBACK (on_trayicon_popup_menu), NULL);

    gtkpl_init ();

    mainwin = create_mainwin ();
    GdkPixbuf *mainwin_icon_pixbuf;
    mainwin_icon_pixbuf = create_pixbuf ("play_24.png");
    if (mainwin_icon_pixbuf)
    {
        gtk_window_set_icon (GTK_WINDOW (mainwin), mainwin_icon_pixbuf);
        gdk_pixbuf_unref (mainwin_icon_pixbuf);
    }
    session_restore_window_attrs ((uintptr_t)mainwin);
    volume_set_db (session_get_volume ());
    // order and looping
    const char *orderwidgets[3] = { "order_linear", "order_shuffle", "order_random" };
    const char *loopingwidgets[3] = { "loop_all", "loop_disable", "loop_single" };
    const char *w;
    w = orderwidgets[session_get_playlist_order ()];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);
    pl_set_order (session_get_playlist_order ());
    w = loopingwidgets[session_get_playlist_looping ()];
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, w)), TRUE);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (lookup_widget (mainwin, "scroll_follows_playback")), session_get_scroll_follows_playback () ? TRUE : FALSE);
    pl_set_loop_mode (session_get_playlist_looping ());

    searchwin = create_searchwin ();
    gtk_window_set_transient_for (GTK_WINDOW (searchwin), GTK_WINDOW (mainwin));
    extern void main_playlist_init (GtkWidget *widget);
    main_playlist_init (lookup_widget (mainwin, "playlist"));
    extern void search_playlist_init (GtkWidget *widget);
    search_playlist_init (lookup_widget (searchwin, "searchlist"));

    progress_init ();

    if (argc > 1) {
        int res = exec_command_line (cmdline, size, 0);
        if (res == -1) {
            server_close ();
            return -1;
        }
        if (res == 2) {
            messagepump_push (M_PLAYSONG, 0, 0, 0);
        }
    }

    gtk_widget_show (mainwin);
    gtk_main ();
    server_close ();
    gdk_threads_leave ();
    messagepump_free ();
    p_free ();
    streamer_free ();
    codec_free_locking ();
    session_save (sessfile);
    pl_save (defpl);
    pl_free ();
    plug_unload_all ();
    return 0;
}
