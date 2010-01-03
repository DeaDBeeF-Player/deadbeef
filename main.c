/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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

// client-side commandline support
// -1 error, program must exit with error code -1
//  0 proceed normally as nothing happened
//  1 no error, but program must exit with error code 0
int
client_exec_command_line (const char *cmdline, int len) {
    const uint8_t *parg = (const uint8_t *)cmdline;
    const uint8_t *pend = cmdline + len;
    while (parg < pend) {
        //        if (filter == 1) {
        // help, version and nowplaying are executed with any filter
        if (!strcmp (parg, "--help") || !strcmp (parg, "-h")) {
            fprintf (stderr, "Usage: deadbeef [options] [file(s)]\n");
            fprintf (stderr, "Options:\n");
            fprintf (stderr, "   --help  or  -h     Print help (this message) and exit\n");
            fprintf (stderr, "   --quit             Quit player\n");
            fprintf (stderr, "   --version          Print version info and exit\n");
            fprintf (stderr, "   --play             Start playback\n");
            fprintf (stderr, "   --stop             Stop playback\n");
            fprintf (stderr, "   --pause            Pause playback\n");
            fprintf (stderr, "   --next             Next song in playlist\n");
            fprintf (stderr, "   --prev             Previous song in playlist\n");
            fprintf (stderr, "   --random           Random song in playlist\n");
            fprintf (stderr, "   --queue            Append file(s) to existing playlist\n");
            fprintf (stderr, "   --nowplaying FMT   Print formatted track name to stdout\n");
            fprintf (stderr, "                      FMT %%-syntax: [a]rtist, [t]itle, al[b]um,\n"
                             "                      [l]ength, track[n]umber, [y]ear, [c]omment, copy[r]ight\n");
            fprintf (stderr, "                      e.g.: --nowplaying \"%%a - %%t\" should print \"artist - title\"\n");
            return 1;
        }
        else if (!strcmp (parg, "--version")) {
            fprintf (stderr, "DeaDBeeF %s Copyright (C) 2009 Alexey Yakovenko\n", VERSION);
            return 1;
        }
        parg += strlen (parg);
        parg++;
    }
    return 0;
}

// this function executes server-side commands only
// must be called only from within server
// -1 error, program must exit with error code -1
//  0 proceed normally as nothing happened
//  1 no error, but program must exit with error code 0
//  2 don't load playlist on startup
//  when executed in remote server -- error code will be ignored
int
server_exec_command_line (const char *cmdline, int len, char *sendback, int sbsize) {
    if (sendback) {
        sendback[0] = 0;
    }
    const uint8_t *parg = (const uint8_t *)cmdline;
    const uint8_t *pend = cmdline + len;
    int queue = 0;
    while (parg < pend) {
        if (!strcmp (parg, "--nowplaying")) {
            parg += strlen (parg);
            parg++;
            if (parg >= pend) {
                if (sendback) {
                    snprintf (sendback, sbsize, "error --nowplaying expects format argument\n");
                    return 0;
                }
                else {
                    fprintf (stderr, "--nowplaying expects format argument\n");
                    return -1;
                }
            }
            if (sendback) {
                playItem_t *curr = streamer_get_playing_track ();
                if (curr && curr->decoder) {
                    const char np[] = "nowplaying ";
                    memcpy (sendback, np, sizeof (np)-1);
                    pl_format_title (curr, sendback+sizeof(np)-1, sbsize-sizeof(np)+1, -1, parg);
                }
                else {
                    strcpy (sendback, "nowplaying nothing");
                }
            }
            else {
                char out[2048];
                playItem_t *curr = streamer_get_playing_track ();
                if (curr && curr->decoder) {
                    pl_format_title (curr, out, sizeof (out), -1, parg);
                }
                else {
                    strcpy (out, "nothing");
                }
                printf (out);
                return 1; // exit
            }
        }
        else if (!strcmp (parg, "--next")) {
            messagepump_push (M_NEXTSONG, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--prev")) {
            messagepump_push (M_PREVSONG, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--play")) {
            messagepump_push (M_PLAYSONG, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--stop")) {
            messagepump_push (M_STOPSONG, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--pause")) {
            messagepump_push (M_PAUSESONG, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--random")) {
            messagepump_push (M_PLAYRANDOM, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--queue")) {
            queue = 1;
        }
        else if (!strcmp (parg, "--quit")) {
            messagepump_push (M_TERMINATE, 0, 0, 0);
        }
        else if (parg[0] != '-') {
            break; // unknown option is filename
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
            if (pl_add_file (pname, NULL, NULL) < 0) {
                fprintf (stderr, "failed to add file %s\n", pname);
            }
            parg += strlen (parg);
            parg++;
        }
        messagepump_push (M_PLAYLISTREFRESH, 0, 0, 0);
        if (!queue) {
            messagepump_push (M_PLAYSONG, 0, 0, 0);
            return 2; // don't reload playlist at startup
        }
    }
    return 0;
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
        char sendback[1024];
        int size;
        if ((size = recv (s2, str, 2048, 0)) >= 0) {
            if (size == 1 && str[0] == 0) {
                // FIXME: that should be called right after activation of gui plugin
                plug_trigger_event (DB_EV_ACTIVATE, 0);
            }
            else {
                server_exec_command_line (str, size, sendback, sizeof (sendback));
            }
        }
        if (sendback[0]) {
            // send nowplaying back to client
            send (s2, sendback, strlen (sendback)+1, 0);
        }
        else {
            send (s2, "", 1, 0);
        }
        close(s2);
    }
    return 0;
}

void
player_mainloop (void) {
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
                pl_playqueue_clear ();
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
    }
    int res;
    res = client_exec_command_line (cmdline, size);
    if (res == 1) {
        return 0;
    }
    else if (res < 0) {
        return res;
    }

    // try to connect to remote player
    int s, len;
    struct sockaddr_un remote;

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
        char out[2048];
        if (recv(s, out, sizeof (out), 0) == -1) {
            fprintf (stderr, "failed to pass args to remote!\n");
            exit (-1);
        }
        else {
            // check if that's nowplaying response
            const char np[] = "nowplaying ";
            const char err[] = "error ";
            if (!strncmp (out, np, sizeof (np)-1)) {
                const char *prn = &out[sizeof (np)-1];
                printf (prn);
            }
            else if (!strncmp (out, err, sizeof (err)-1)) {
                const char *prn = &out[sizeof (err)-1];
                fprintf (stderr, prn);
            }
            else if (out[0]) {
                fprintf (stderr, "got unkown response:\n%s\n", out);
            }
        }
        close (s);
        exit (0);
    }
    close(s);

    signal (SIGTERM, sigterm_handler);

    conf_load (); // required by some plugin at startup
    messagepump_init (); // required to push messages while handling commandline
    plug_load_all (); // required to add files to playlist from commandline

    // execute server commands in local context
    int noloadpl = 0;
    if (argc > 1) {
        res = server_exec_command_line (cmdline, size, NULL, 0);
        // some of the server commands ran on 1st instance should terminate it
        if (res == 2) {
            noloadpl = 1;
        }
        else if (res > 0) {
            exit (0);
        }
        else if (res < 0) {
            exit (-1);
        }
    }

    // become a server
    server_start ();

    // start all subsystems
    volume_set_db (conf_get_float ("playback.volume", 0));
    if (!noloadpl) {
        pl_load (defpl);
    }
    plug_trigger_event_playlistchanged ();
    session_load (sessfile);
    codec_init_locking ();
    streamer_init ();

    // this runs in main thread (blocks right here)
    player_mainloop ();

    // save config
    pl_save (defpl);
    conf_save ();

    // stop receiving messages from outside
    server_close ();

    // stop streaming and playback before unloading plugins
    p_stop ();
    p_free ();
    streamer_free ();

    // plugins might still hood references to playitems,
    // and query configuration in background
    // so unload everything 1st before final cleanup
    plug_unload_all ();

    // at this point we can simply do exit(0), but let's clean up for debugging
    codec_free_locking ();
    session_save (sessfile);
    pl_free ();
    conf_free ();
    messagepump_free ();
    fprintf (stderr, "hej-hej!\n");
    return 0;
}
