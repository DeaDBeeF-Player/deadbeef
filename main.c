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
#include <stddef.h>
#include <time.h>
#include <locale.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#ifndef __linux__
#define _POSIX_C_SOURCE
#endif
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <signal.h>
#ifdef __linux__
#include <execinfo.h>
#endif
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <unistd.h>
#include "gettext.h"
#include "playlist.h"
#include "playback.h"
#include "threading.h"
#include "messagepump.h"
#include "streamer.h"
#include "conf.h"
#include "volume.h"
#include "plugins.h"
#include "common.h"

#ifndef PREFIX
#error PREFIX must be defined
#endif

#ifdef __linux__
#define USE_ABSTRACT_NAME 0
#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)


// some common global variables
char sys_install_path[PATH_MAX]; // see deadbeef->get_prefix
char confdir[PATH_MAX]; // $HOME/.config
char dbconfdir[PATH_MAX]; // $HOME/.config/deadbeef
char dbinstalldir[PATH_MAX]; // see deadbeef->get_prefix
char dbdocdir[PATH_MAX]; // see deadbeef->get_doc_dir
char dbplugindir[PATH_MAX]; // see deadbeef->get_plugin_dir
char dbpixmapdir[PATH_MAX]; // see deadbeef->get_pixmap_dir

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
            fprintf (stdout, _("Usage: deadbeef [options] [file(s)]\n"));
            fprintf (stdout, _("Options:\n"));
            fprintf (stdout, _("   --help  or  -h     Print help (this message) and exit\n"));
            fprintf (stdout, _("   --quit             Quit player\n"));
            fprintf (stdout, _("   --version          Print version info and exit\n"));
            fprintf (stdout, _("   --play             Start playback\n"));
            fprintf (stdout, _("   --stop             Stop playback\n"));
            fprintf (stdout, _("   --pause            Pause playback\n"));
            fprintf (stdout, _("   --next             Next song in playlist\n"));
            fprintf (stdout, _("   --prev             Previous song in playlist\n"));
            fprintf (stdout, _("   --random           Random song in playlist\n"));
            fprintf (stdout, _("   --queue            Append file(s) to existing playlist\n"));
            fprintf (stdout, _("   --nowplaying FMT   Print formatted track name to stdout\n"));
            fprintf (stdout, _("                      FMT %%-syntax: [a]rtist, [t]itle, al[b]um,\n"
                             "                      [l]ength, track[n]umber, [y]ear, [c]omment,\n"
                             "                      copy[r]ight, [e]lapsed\n"));
            fprintf (stdout, _("                      e.g.: --nowplaying \"%%a - %%t\" should print \"artist - title\"\n"));
            return 1;
        }
        else if (!strcmp (parg, "--version")) {
            fprintf (stdout, "DeaDBeeF " VERSION " Copyright © 2009-2010 Alexey Yakovenko\n");
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
                DB_fileinfo_t *dec = streamer_get_current_fileinfo ();
                if (curr && dec) {
                    const char np[] = "nowplaying ";
                    memcpy (sendback, np, sizeof (np)-1);
                    pl_format_title (curr, -1, sendback+sizeof(np)-1, sbsize-sizeof(np)+1, -1, parg);
                }
                else {
                    strcpy (sendback, "nowplaying nothing");
                }
                if (curr) {
                    pl_item_unref (curr);
                }
            }
            else {
                char out[2048];
                playItem_t *curr = streamer_get_playing_track ();
                DB_fileinfo_t *dec = streamer_get_current_fileinfo();
                if (curr && dec) {
                    pl_format_title (curr, -1, out, sizeof (out), -1, parg);
                }
                else {
                    strcpy (out, "nothing");
                }
                if (curr) {
                    pl_item_unref (curr);
                }
                fwrite (out, 1, strlen (out), stdout);
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
        if (conf_get_int ("cli_add_to_specific_playlist", 1)) {
            const char *str = conf_get_str ("cli_add_playlist_name", "Default");
            int idx = plt_find (str);
            if (idx < 0) {
                idx = plt_add (plt_get_count (), str);
            }
            if (idx >= 0) {
                plt_set_curr (idx);
            }
        }
        // add files
        if (!queue && plt_get_curr () != -1) {
            pl_clear ();
            pl_reset_cursor ();
        }
        if (parg < pend) {
            deadbeef->pl_add_files_begin ();
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
            if (deadbeef->pl_add_dir (pname, NULL, NULL) < 0) {
                if (deadbeef->pl_add_file (pname, NULL, NULL) < 0) {
                    fprintf (stderr, "failed to add file or folder %s\n", pname);
                }
            }
            parg += strlen (parg);
            parg++;
        }
        deadbeef->pl_add_files_end ();
        messagepump_push (M_PLAYLISTREFRESH, 0, 0, 0);
        if (!queue) {
            messagepump_push (M_PLAYSONG, 0, 1, 0);
            return 2; // don't reload playlist at startup
        }
    }
    return 0;
}

static struct sockaddr_un srv_local;
static struct sockaddr_un srv_remote;
static unsigned srv_socket;

#if USE_ABSTRACT_NAME
static char server_id[] = "\0deadbeefplayer";
#endif

int
server_start (void) {
    fprintf (stderr, "server_start\n");
    srv_socket = socket (AF_UNIX, SOCK_STREAM, 0);
    int flags;
    flags = fcntl (srv_socket, F_GETFL,0);
    if (flags == -1) {
        perror ("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(srv_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror ("fcntl F_SETFL");
        return -1;
    }
    memset (&srv_local, 0, sizeof (srv_local));
    srv_local.sun_family = AF_UNIX;

#if USE_ABSTRACT_NAME
    memcpy (srv_local.sun_path, server_id, sizeof (server_id));
    int len = offsetof(struct sockaddr_un, sun_path) + sizeof (server_id)-1;
#else
    char *socketdirenv = getenv ("DDB_SOCKET_DIR");
    snprintf (srv_local.sun_path, sizeof (srv_local.sun_path), "%s/socket", socketdirenv ? socketdirenv : dbconfdir);
    if (unlink(srv_local.sun_path) < 0) {
        perror ("INFO: unlink socket");
    }
    int len = offsetof(struct sockaddr_un, sun_path) + strlen (srv_local.sun_path);
#endif

    if (bind(srv_socket, (struct sockaddr *)&srv_local, len) < 0) {
        perror ("bind");
        return -1;
    }

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
        char sendback[1024] = "";
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
                conf_save ();
                break;
            case M_TERMINATE:
                return;
            case M_PLAYSONG:
                if (p1) {
                    p_stop ();
                    pl_playqueue_clear ();
                    streamer_set_nextsong (0, 1);
                }
                else {
                    streamer_play_current_track ();
                }
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
                streamer_move_to_nextsong (1);
                break;
            case M_PREVSONG:
                p_stop ();
                streamer_move_to_prevsong ();
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
                streamer_move_to_randomsong ();
                break;
            case M_PLAYLISTREFRESH:
                pl_save_current ();
                plug_trigger_event_playlistchanged ();
                break;
            case M_CONFIGCHANGED:
                conf_save ();
                streamer_configchanged ();
                plug_trigger_event (DB_EV_CONFIGCHANGED, 0);
                break;
            }
        }
        usleep(50000);
        plug_trigger_event (DB_EV_FRAMEUPDATE, 0);
    }
}

static int sigterm_handled = 0;

void
atexit_handler (void) {
    fprintf (stderr, "atexit_handler\n");
    if (!sigterm_handled) {
        fprintf (stderr, "handling atexit.\n");
        pl_save_all ();
        conf_save ();
    }
}

void
sigterm_handler (int sig) {
    fprintf (stderr, "got sigterm.\n");
    atexit_handler ();
    sigterm_handled = 1;
    fprintf (stderr, "bye.\n");
    exit (0);
}

#ifdef __linux__
void
sigsegv_handler (int sig) {
    fprintf (stderr, "Segmentation Fault\n");
    int j, nptrs;
#define SIZE 100
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, SIZE);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call
     * backtrace_symbols_fd(buffer,
     * nptrs,
     * STDOUT_FILENO)
     would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);

    free(strings);
    exit (0);
}
#endif

void
save_resume_state (void) {
    playItem_t *trk = streamer_get_playing_track ();
    float playpos = -1;
    int playtrack = -1;
    int playlist = streamer_get_current_playlist ();
    int paused = (p_get_state () == OUTPUT_STATE_PAUSED);
    if (trk && playlist >= 0) {
        playtrack = str_get_idx_of (trk);
        playpos = streamer_get_playpos ();
        pl_item_unref (trk);
    }

    conf_set_float ("resume.position", playpos);
    conf_set_int ("resume.track", playtrack);
    conf_set_int ("resume.playlist", playlist);
    conf_set_int ("resume.paused", paused);
}

void
restore_resume_state (void) {
    if (conf_get_int ("resume_last_session", 0) && p_isstopped ()) {
        int plt = conf_get_int ("resume.playlist", -1);
        int track = conf_get_int ("resume.track", -1);
        float pos = conf_get_float ("resume.position", -1);
        int paused = conf_get_int ("resume.paused", 0);
        trace ("resume: track %d pos %f playlist %d\n", track, pos, plt);
        if (plt >= 0 && track >= 0 && pos >= 0) {
            streamer_lock (); // need to hold streamer thread to make the resume operation atomic
            streamer_set_current_playlist (plt);
            streamer_set_nextsong (track, paused ? 2 : 3);
            streamer_set_seek (pos);
            streamer_unlock ();
        }
    }
}

int
main (int argc, char *argv[]) {
#ifdef __linux__
    signal (SIGSEGV, sigsegv_handler);
#endif
    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C");
#ifdef ENABLE_NLS
//    fprintf (stderr, "enabling gettext support: package=" PACKAGE ", dir=" LOCALEDIR "...\n");
	bindtextdomain (PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
#endif					
    fprintf (stderr, "starting deadbeef " VERSION "%s\n", PORTABLE ? " [portable build]" : "");
    srand (time (NULL));
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-main", 0, 0, 0, 0);
#endif

#if PORTABLE
    strcpy (dbinstalldir, argv[0]);
    char *e = dbinstalldir + strlen (dbinstalldir);
    while (e >= dbinstalldir && *e != '/') {
        e--;
    }
    *e = 0;
    if (snprintf (confdir, sizeof (confdir), "%s/config", dbinstalldir) > sizeof (confdir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }

    strcpy (dbconfdir, confdir);

    if (snprintf (dbdocdir, sizeof (dbdocdir), "%s/doc", dbinstalldir) > sizeof (dbdocdir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }
    if (snprintf (dbplugindir, sizeof (dbplugindir), "%s/plugins", dbinstalldir) > sizeof (dbplugindir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }
    if (snprintf (dbpixmapdir, sizeof (dbpixmapdir), "%s/pixmaps", dbinstalldir) > sizeof (dbpixmapdir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }
    trace ("installdir: %s\n", dbinstalldir);
    trace ("confdir: %s\n", confdir);
    trace ("docdir: %s\n", dbdocdir);
    trace ("plugindir: %s\n", dbplugindir);
    mkdir (dbplugindir, 0755);
    trace ("pixmapdir: %s\n", dbpixmapdir);
#else
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
    if (snprintf (dbconfdir, sizeof (dbconfdir), "%s/deadbeef", confdir) > sizeof (dbconfdir)) {
        fprintf (stderr, "fatal: out of memory while configuring\n");
        return -1;
    }
    mkdir (confdir, 0755);
    if (snprintf (dbdocdir, sizeof (dbdocdir), "%s", DOCDIR) > sizeof (dbdocdir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }
    if (snprintf (dbplugindir, sizeof (dbplugindir), "%s/deadbeef", LIBDIR) > sizeof (dbplugindir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }
    if (snprintf (dbpixmapdir, sizeof (dbpixmapdir), "%s/share/deadbeef/pixmaps", PREFIX) > sizeof (dbpixmapdir)) {
        fprintf (stderr, "fatal: too long install path %s\n", dbinstalldir);
        return -1;
    }
#endif

    mkdir (dbconfdir, 0755);

    char cmdline[2048];
    cmdline[0] = 0;
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

    memset (&remote, 0, sizeof (remote));
    remote.sun_family = AF_UNIX;
#if USE_ABSTRACT_NAME
    memcpy (remote.sun_path, server_id, sizeof (server_id));
    len = offsetof(struct sockaddr_un, sun_path) + sizeof (server_id)-1;
#else
    char *socketdirenv = getenv ("DDB_SOCKET_DIR");
    snprintf (remote.sun_path, sizeof (remote.sun_path), "%s/socket", socketdirenv ? socketdirenv : dbconfdir);
    len = offsetof(struct sockaddr_un, sun_path) + strlen (remote.sun_path);
#endif
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
        char out[2048] = "";
        ssize_t sz = recv(s, out, sizeof (out), 0);
        if (sz == -1) {
            fprintf (stderr, "failed to pass args to remote!\n");
            exit (-1);
        }
        else {
            // check if that's nowplaying response
            const char np[] = "nowplaying ";
            const char err[] = "error ";
            if (!strncmp (out, np, sizeof (np)-1)) {
                const char *prn = &out[sizeof (np)-1];
                fwrite (prn, 1, strlen (prn), stdout);
            }
            else if (!strncmp (out, err, sizeof (err)-1)) {
                const char *prn = &out[sizeof (err)-1];
                fwrite (prn, 1, strlen (prn), stderr);
            }
            else if (sz > 0 && out[0]) {
                fprintf (stderr, "got unknown response:\nlength=%d\n%s\n", (int)sz, out);
            }
        }
        close (s);
        exit (0);
    }
//    else {
//        perror ("INFO: failed to connect to existing session:");
//    }
    close(s);

    // hack: report nowplaying
    if (!strcmp (cmdline, "--nowplaying")) {
        char nothing[] = "nothing";
        fwrite (nothing, 1, sizeof (nothing)-1, stdout);
        return 0;
    }


    pl_init ();
    conf_load (); // required by some plugins at startup
    volume_set_db (conf_get_float ("playback.volume", 0)); // volume need to be initialized before plugins start
    messagepump_init (); // required to push messages while handling commandline
    if (plug_load_all ()) { // required to add files to playlist from commandline
        exit (-1);
    }
    pl_load_all ();
    plt_set_curr (conf_get_int ("playlist.current", 0));

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
    if (server_start () < 0) {
        exit (-1);
    }
    signal (SIGTERM, sigterm_handler);
    atexit (atexit_handler); // helps to save in simple cases

    // start all subsystems
    plug_trigger_event_playlistchanged ();

    streamer_init ();

    if (!noloadpl) {
        restore_resume_state ();
    }

    // this runs in main thread (blocks right here)
    player_mainloop ();

    save_resume_state ();

    // save config
    pl_save_all ();
    conf_save ();

    // delete legacy session file
    {
        char sessfile[1024]; // $HOME/.config/deadbeef/session
        if (snprintf (sessfile, sizeof (sessfile), "%s/deadbeef/session", confdir) < sizeof (sessfile)) {
            unlink (sessfile);
        }
    }

    // stop receiving messages from outside
    server_close ();

    // stop streaming and playback before unloading plugins
    p_stop ();
    streamer_free ();
    p_free ();

    // plugins might still hood references to playitems,
    // and query configuration in background
    // so unload everything 1st before final cleanup
    plug_unload_all ();

    // at this point we can simply do exit(0), but let's clean up for debugging
    plt_free (); // plt_free may access conf_*
    pl_free ();
    conf_free ();
    messagepump_free ();
    plug_cleanup ();
    sigterm_handled = 1;
    fprintf (stderr, "hej-hej!\n");
    return 0;
}

