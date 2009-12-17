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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/prctl.h>
#include <signal.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "playlist.h"
#include "playback.h"
#include "unistd.h"
#include "threading.h"
#include "messagepump.h"
#include "codec.h"
#include "streamer.h"
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
                printf ("   --random           Random song in playlist\n");
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
            pl_reset_cursor ();
        }
        while (parg < pend) {
            char resolved[PATH_MAX];
            const char *pname;
            if (realpath (parg, resolved)) {
                pname = resolved;
            }
            else {
                pname = parg;
            }
            if (pl_add_file (pname, NULL, NULL) >= 0) {
                if (queue) {
                    exitcode = 3;
                }
                else {
                    exitcode = 2;
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
                // FIXME: that should be called right after activation of gui plugin
                plug_trigger_event (DB_EV_ACTIVATE, 0);
            }
            else {
                int res = exec_command_line (str, size, 0);
                if (res == 2) {
                    streamer_play_current_track ();
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
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            switch (msg) {
            case M_REINIT_SOUND:
                plug_reinit_sound ();
                break;
            case M_TERMINATE:
                return;
            case M_SONGCHANGED:
                plug_trigger_event_trackchange (p1, p2);
                break;
            case M_PLAYSONG:
                streamer_play_current_track ();
                break;
            case M_TRACKCHANGED:
                plug_trigger_event_trackinfochanged (p1);
                break;
            case M_PLAYSONGNUM:
                p_stop ();
                streamer_set_nextsong (p1, 1);
                break;
            case M_STOPSONG:
                streamer_set_nextsong (-2, 0);
                break;
            case M_NEXTSONG:
                p_stop ();
                pl_nextsong (1);
                break;
            case M_PREVSONG:
                p_stop ();
                pl_prevsong ();
                break;
            case M_PAUSESONG:
                if (p_get_state () == OUTPUT_STATE_PAUSED) {
                    p_unpause ();
                    plug_trigger_event_paused (0);
                }
                else {
                    p_pause ();
                    plug_trigger_event_paused (1);
                }
                break;
            case M_PLAYRANDOM:
                p_stop ();
                pl_randomsong ();
                break;
            case M_PLAYLISTREFRESH:
                plug_trigger_event_playlistchanged ();
                break;
            case M_CONFIGCHANGED:
                //plug_get_output ()->configchanged ();
                streamer_configchanged ();
                plug_trigger_event (DB_EV_CONFIGCHANGED, 0);
                break;
            }
        }
        usleep(50000);
        plug_trigger_event (DB_EV_FRAMEUPDATE, 0);
    }
}

void
sigterm_handler (int sig) {
    fprintf (stderr, "got sigterm, saving...\n");
    pl_save (defpl);
    conf_save ();
    fprintf (stderr, "bye.\n");
    exit (0);
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
            char resolved[PATH_MAX];
            // need to resolve path here, because remote doesn't know current
            // path of this process
            if (argv[i][0] != '-' && realpath (argv[i], resolved)) {
                len = strlen (resolved);
                if (len >= size) {
                    break;
                }
                memcpy (p, resolved, len+1);
            }
            else {
                memcpy (p, argv[i], len+1);
            }
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

    signal (SIGTERM, sigterm_handler);
    // become a server
    server_start ();

    conf_load ();
    volume_set_db (conf_get_float ("playback.volume", 0));
    plug_load_all ();
    pl_load (defpl);
    plug_trigger_event_playlistchanged ();
    session_load (sessfile);
    messagepump_init ();
    codec_init_locking ();
    streamer_init ();
//    p_init ();
//    thread_start (player_thread, 0);

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

    player_thread (0);

    // save config
    pl_save (defpl);
    conf_save ();
    // stop receiving messages from outside
    server_close ();
    // at this point we can simply do exit(0), but let's clean up for debugging
    messagepump_free ();
    p_free ();
    streamer_free ();
    codec_free_locking ();
    session_save (sessfile);
    pl_free ();
    conf_free ();
    plug_unload_all ();
    fprintf (stderr, "hej-hej!\n");
    return 0;
}
