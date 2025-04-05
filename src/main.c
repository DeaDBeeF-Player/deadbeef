/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  application launcher, compatible with GNU/Linux and most other POSIX systems

  Copyright (C) 2009-2017 Oleksiy Yakovenko

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif
#include <assert.h>
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <utime.h>
#include <locale.h>
#include <sys/time.h>
#ifdef __linux__
#    include <sys/prctl.h>
#endif
#if !defined(__linux__) && !defined(_POSIX_C_SOURCE)
#    define _POSIX_C_SOURCE 1
#endif
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

// #define USE_INET_SOCKET
#define DEFAULT_LISTENING_PORT 48879

#ifdef __MINGW32__
#    define USE_INET_SOCKET
#    include <winsock2.h>
#else
#    include <sys/socket.h>
#    include <sys/select.h>
#    include <sys/un.h>
#    include <sys/errno.h>
#    ifdef USE_INET_SOCKET
#        include <netinet/in.h>
#        include <arpa/inet.h>
#    endif
#endif

#include <sys/fcntl.h>
#include <signal.h>
#ifdef __GLIBC__
#    include <execinfo.h>
#endif
#include <unistd.h>
#include "gettext.h"
#include "playlist.h"
#include "threading.h"
#include "messagepump.h"
#include "streamer.h"
#include "playmodes.h"
#include "conf.h"
#include "volume.h"
#include "plugins.h"
#include <deadbeef/common.h>
#include "junklib.h"
#ifdef OSX_APPBUNDLE
#    include "cocoautil.h"
#endif
#include "playqueue.h"
#include "tf.h"
#include "logger.h"
#include "metacache.h"

#ifdef OSX_APPBUNDLE
#    include "scriptable/scriptable.h"
#    include "scriptable/scriptable_dsp.h"
#    include "scriptable/scriptable_encoder.h"
//#include "scriptable/scriptable_tfquery.h"
#endif

#include "undo/undomanager.h"

#ifndef PREFIX
#    error PREFIX must be defined
#endif

#ifdef OSX_APPBUNDLE
#    define SYS_CONFIG_DIR "Library/Preferences"
#elif defined(__MINGW32__)
#    define SYS_CONFIG_DIR "AppData/Roaming"
#else
#    define SYS_CONFIG_DIR ".config"
#endif

#ifdef __MINGW32__
#    include "../shared/windows/utils.h"
#endif

// some common global variables
char sys_install_path[PATH_MAX]; // see deadbeef->get_prefix
char confdir[PATH_MAX]; // $HOME/.config
char dbconfdir[PATH_MAX]; // $HOME/.config/deadbeef
char dbinstalldir[PATH_MAX]; // see deadbeef->get_prefix
char dbdocdir[PATH_MAX]; // see deadbeef->get_doc_dir
char dbplugindir[PATH_MAX]; // see deadbeef->get_plugin_dir
char dbpixmapdir[PATH_MAX]; // see deadbeef->get_pixmap_dir
char dbcachedir[PATH_MAX];
char dbruntimedir[PATH_MAX]; // /run/user/<uid>/deadbeef
char dbresourcedir[PATH_MAX];
char dbstatedir[PATH_MAX];

char use_gui_plugin[100];

static int _previous_session_did_crash;

static void
print_help (void) {
#ifdef ENABLE_NLS
    bind_textdomain_codeset (PACKAGE, "");
#endif
    fprintf (stdout, _ ("Usage: deadbeef [options] [--] [file(s)]\n"));
    fprintf (stdout, _ ("Options:\n"));
    fprintf (stdout, _ ("   --help  or  -h     Print help (this message) and exit\n"));
    fprintf (stdout, _ ("   --quit             Quit player\n"));
    fprintf (stdout, _ ("   --version          Print version info and exit\n"));
    fprintf (stdout, _ ("   --play             Start playback\n"));
    fprintf (stdout, _ ("   --stop             Stop playback\n"));
    fprintf (stdout, _ ("   --pause            Pause playback\n"));
    fprintf (stdout, _ ("   --toggle-pause     Toggle pause\n"));
    fprintf (stdout, _ ("   --play-pause       Start playback if stopped, toggle pause otherwise\n"));
    fprintf (stdout, _ ("   --next             Next song in playlist\n"));
    fprintf (stdout, _ ("   --prev             Previous song in playlist\n"));
    fprintf (stdout, _ ("   --random           Random song in playlist\n"));
    fprintf (stdout, _ ("   --queue            Append file(s) to existing playlist\n"));
    fprintf (stdout, _ ("   --gui PLUGIN       Tells which GUI plugin to use, default is \"GTK2\"\n"));
    fprintf (stdout, _ ("   --nowplaying FMT   Print formatted track name to stdout\n"));
    fprintf (
        stdout,
        _ ("                      FMT %%-syntax: [a]rtist, [t]itle, al[b]um,\n"
           "                      [l]ength, track[n]umber, [y]ear, [c]omment,\n"
           "                      copy[r]ight, [e]lapsed\n"));
    fprintf (stdout, _ ("                      example: --nowplaying \"%%a - %%t\" should print \"artist - title\"\n"));
    fprintf (
        stdout,
        _ ("                      for more info, see %s\n"),
        "http://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting");
    fprintf (stdout, _ ("                      NOTE: --nowplaying is deprecated.\n"));
    fprintf (
        stdout,
        _ ("   --nowplaying-tf FMT  Print formatted track name to stdout, using the new title formatting\n"));
    fprintf (
        stdout,
        _ ("                      FMT syntax: http://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0\n"));
    fprintf (
        stdout,
        _ ("                      example: --nowplaying-tf \"%%artist%% - %%title%%\" should print \"artist - title\"\n"));
    fprintf (stdout, _ ("   --volume [NUM]     Print or set deadbeef volume level.\n"));
    fprintf (
        stdout,
        _ ("                      The NUM parameter can be specified in percents (absolute value or increment/decrement)\n"));
    fprintf (stdout, _ ("                      or in dB [-50, 0] (if with suffix).\n"));
    fprintf (stdout, _ ("                      Examples: --volume 80, --volume +10, --volume -5 or --volume -20dB\n"));
    fprintf (
        stdout,
        _ ("   --plugin=[PLUG]    Send commands to a specific plugin. Use PLUG=main to send commands to deadbeef itself.\n"));
    fprintf (stdout, _ ("                      To get plugin specific commands use --plugin=[PLUG] --help\n"));
    fprintf (
        stdout,
        _ ("   --plugin-list      List all available plugins including indication for plugins that support commands.\n"));
#ifdef ENABLE_NLS
    bind_textdomain_codeset (PACKAGE, "UTF-8");
#endif
}

// Parse command line an return a single buffer with all
// parameters concatenated (separated by \0).  File names
// are resolved.
char *
prepare_command_line (int argc, char *argv[], int *size) {
    int seen_ddash = 0;

    // initial buffer limit, will expand if needed
    size_t limit = 4096;
    char *buf = (char *)malloc (limit);

    if (argc <= 1) {
        buf[0] = 0;
        *size = 1;
        return buf;
    }

    int p = 0;
    for (int i = 1; i < argc; i++) {
        // if argument is a filename, try to resolve it
        char resolved[PATH_MAX];
        char *arg;
        if ((!strncmp ("--", argv[i], 2) && !seen_ddash) || !realpath (argv[i], resolved)) {
            arg = argv[i];
        }
        else {
            arg = resolved;
        }

        // make sure that there is enough space in the buffer;
        // re-allocate, if needed
        size_t arglen = strlen (arg) + 1;
        while (p + arglen >= limit) {
            char *newbuf = (char *)malloc (limit * 2);
            memcpy (newbuf, buf, p);
            free (buf);
            limit *= 2;
            buf = newbuf;
        }

        memcpy (buf + p, arg, arglen);
        p += arglen;

        if (!strcmp ("--", argv[i])) {
            seen_ddash = 1;
        }
    }

    *size = p;
    return buf;
}

typedef struct {
    ddb_response_t base;
    char *buffer;
    size_t buffer_size;
    size_t buffer_capacity;
} ddb_response_impl_t;

static int
_append_response (ddb_response_t *response, char *buffer, size_t size) {
    ddb_response_impl_t *impl = (ddb_response_impl_t *)response;
    size_t new_size = impl->buffer_size + size;
    if (new_size > impl->buffer_capacity) {
        impl->buffer_capacity = new_size + 1000;
        impl->buffer = realloc (impl->buffer, impl->buffer_capacity);
    }

    memcpy (impl->buffer + impl->buffer_size, buffer, size);
    impl->buffer_size = new_size;
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
    const char *parg = cmdline;
    const char *pend = cmdline + len;
    int queue = 0;
    while (parg < pend) {
        if (strlen (parg) >= 2 && parg[0] == '-' && parg[1] != '-') {
            parg += strlen (parg);
            parg++;
            return 0; // running under osx debugger?
        }
        else if (!strcmp (parg, "--nowplaying")) {
            parg += strlen (parg);
            parg++;
            if (parg >= pend) {
                const char *errtext = "--nowplaying expects format argument";
                if (sendback) {
                    snprintf (sendback, sbsize, "\2%s\n", errtext);
                    return 0;
                }
                else {
                    trace_err ("%s\n", errtext);
                    return -1;
                }
            }
            char out[2048];
            playItem_t *curr = streamer_get_playing_track ();
            if (curr) {
                pl_format_title (curr, -1, out, sizeof (out), -1, parg);
                pl_item_unref (curr);
            }
            else {
                strcpy (out, "nothing");
            }
            if (sendback) {
                snprintf (sendback, sbsize, "\1%s", out);
            }
            else {
                fwrite (out, 1, strlen (out), stdout);
                return 1; // exit
            }
        }
        else if (!strcmp (parg, "--nowplaying-tf")) {
            parg += strlen (parg);
            parg++;
            if (parg >= pend) {
                const char *errtext = "--nowplaying-tf expects format argument";
                if (sendback) {
                    snprintf (sendback, sbsize, "\2%s\n", errtext);
                    return 0;
                }
                else {
                    trace_err ("%s\n", errtext);
                    return -1;
                }
            }
            char out[2048];
            playItem_t *curr = streamer_get_playing_track ();
            char *script = tf_compile (parg);
            if (script) {
                ddb_tf_context_t ctx = {
                    ._size = sizeof (ddb_tf_context_t),
                    .it = (DB_playItem_t *)curr,
                };
                tf_eval (&ctx, script, out, sizeof (out));
                tf_free (script);
            }
            else {
                *out = 0;
            }
            if (curr) {
                pl_item_unref (curr);
            }
            if (sendback) {
                snprintf (sendback, sbsize, "\1%s", out);
            }
            else {
                fwrite (out, 1, strlen (out), stdout);
                return 1; // exit
            }
        }
        else if (!strcmp (parg, "--next")) {
            messagepump_push (DB_EV_NEXT, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--prev")) {
            messagepump_push (DB_EV_PREV, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--play")) {
            messagepump_push (DB_EV_PLAY_CURRENT, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--stop")) {
            messagepump_push (DB_EV_STOP, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--pause")) {
            messagepump_push (DB_EV_PAUSE, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--toggle-pause")) {
            messagepump_push (DB_EV_TOGGLE_PAUSE, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--play-pause")) {
            ddb_playback_state_t state = deadbeef->get_output ()->state ();
            if (state == DDB_PLAYBACK_STATE_PLAYING) {
                deadbeef->sendmessage (DB_EV_PAUSE, 0, 0, 0);
            }
            else {
                deadbeef->sendmessage (DB_EV_PLAY_CURRENT, 0, 0, 0);
            }
            return 0;
        }
        else if (!strcmp (parg, "--random")) {
            messagepump_push (DB_EV_PLAY_RANDOM, 0, 0, 0);
            return 0;
        }
        else if (!strcmp (parg, "--queue")) {
            queue = 1;
        }
        else if (!strcmp (parg, "--quit")) {
            messagepump_push (DB_EV_TERMINATE, 0, 0, 0);
        }
        else if (!strcmp (parg, "--gui")) {
            // need to skip --gui here, it is handled in the client cmdline
            parg += strlen (parg);
            parg++;
            if (parg >= pend) {
                break;
            }
            parg += strlen (parg);
            parg++;
            continue;
        }
        else if (!strcmp (parg, "--volume")) {
            parg += strlen (parg);
            parg++;

            if (parg < pend) {
                char *end;
                const int pct = (int)strtol (parg, &end, 10);
                if (!strcasecmp (end, "db")) {
                    deadbeef->volume_set_db (pct);
                }
                else {
                    const char is_increment = (parg[0] == '-' || parg[0] == '+');
                    const float new_volume = is_increment ? deadbeef->volume_get_db () * 2 + 100 + pct : pct;
                    deadbeef->volume_set_db ((new_volume / 100.0 * 50) - 50);
                }
            }
            if (sendback) {
                snprintf (
                    sendback,
                    sbsize,
                    "\1%.0f%% (%.2f dB)",
                    deadbeef->volume_get_db () * 2 + 100,
                    deadbeef->volume_get_db ());
            }
            return 0;
        }
        else if (!strncmp (parg, "--plugin=", strlen ("--plugin="))) {
            if (!strcmp (parg + strlen ("--plugin="), "main")) {
                parg += strlen (parg) + 1;
                continue;
            }

            // get length until pend or next "--plugin" occurrence
            int parg_len = 0;
            while ((parg + parg_len) < pend) {
                if (parg_len && !strncmp (parg + parg_len, "--plugin=", strlen ("--plugin="))) {
                    break;
                }
                parg_len += strlen (parg + parg_len) + 1;
            }
            parg_len--;

            const char *plugid = parg + strlen ("--plugin=");
            DB_plugin_t *p = plug_get_for_id (plugid);
            if (p && p->api_vmajor == 1 && p->api_vminor >= 15 && p->exec_cmdline != NULL) {

                ddb_response_impl_t response = {
                    .base._size = sizeof (ddb_response_t),
                    .base.append = _append_response,
                };

                size_t plugarg_len = parg_len - strlen (parg) - 1;
                int ret = p->exec_cmdline (parg + strlen (parg) + 1, (int)plugarg_len, &response.base);

                off_t out_size = response.buffer_size;
                if (out_size > sbsize - 2) {
                    out_size = sbsize - 2;
                }
                // copy plugin output to sendback (if any output produced)
                if (out_size > 0 && sendback) {
                    sendback[0] = '\1';

                    memcpy (sendback + 1, response.buffer, out_size);
                    sendback[out_size + 1] = 0;
                }
                if (ret && sendback) {
                    // TODO have specific error codes sent to client?
                    sendback[0] = '\2';
                }

                free (response.buffer);
            }
            parg += parg_len + 1;
            continue;
        }
        else if (!strcmp (parg, "--plugin-list")) {
            char out[2048];
            int out_pos = 0;
            int i = 0;
            DB_plugin_t **plugins = plug_get_list ();
            while (plugins[i] && (2048 - out_pos) >= 0) {
                const char *format = plugins[i]->exec_cmdline ? "\033[32m%s\033[0m, " : "%s, ";
                out_pos += snprintf (out + out_pos, 2048 - out_pos, format, plugins[i]->id);
                i++;
            }
            out[out_pos - 1] = '\0';
            out[out_pos - 2] = '\n';
            if (sendback) {
                snprintf (sendback, sbsize, "\1%s", out);
            }
        }
        else if (parg[0] != '-') {
            break; // unknown option is filename
        }
        parg += strlen (parg);
        parg++;
    }
    if (parg < pend) {
        if (add_paths (parg, (int)(pend - parg), queue, sendback, sbsize) > 0) {
            return 0; // files not loaded, but continue normally
        }
        if (!queue) {
            return 2; // don't reload playlist at startup
        }
    }
    return 0;
}

// parses a list of paths and adds them to deadbeef
// 0 - no error, files loaded
// 1 - no error, but files not loaded
int
add_paths (const char *paths, int len, int queue, char *sendback, int sbsize) {
    const char *parg = paths;
    const char *pend = paths + len;

    if (conf_get_int ("cli_add_to_specific_playlist", 1)) {
        char str[200];
        conf_get_str ("cli_add_playlist_name", "Default", str, sizeof (str));
        int idx = plt_find (str);
        if (idx < 0) {
            idx = plt_add (plt_get_count (), str);
        }
        if (idx >= 0) {
            plt_set_curr_idx (idx);
        }
    }
    playlist_t *curr_plt = plt_get_curr ();
    if (plt_add_files_begin (curr_plt, 0) != 0) {
        plt_unref (curr_plt);
        snprintf (
            sendback,
            sbsize,
            "it's not allowed to add files to playlist right now, because another file adding operation is in progress. please try again later.");
        return 1;
    }
    // add files
    if (!queue) {
        plt_clear (curr_plt);
        messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        plt_reset_cursor (curr_plt);
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
        if (deadbeef->plt_add_dir2 (0, (ddb_playlist_t *)curr_plt, pname, NULL, NULL) < 0) {
            if (deadbeef->plt_add_file2 (0, (ddb_playlist_t *)curr_plt, pname, NULL, NULL) < 0) {
                int ab = 0;
                playItem_t *after = plt_get_last (curr_plt, PL_MAIN);
                playItem_t *it = plt_load2 (0, curr_plt, after, pname, &ab, NULL, NULL);
                if (after) {
                    pl_item_unref (after);
                }
                if (!it) {
                    trace_err ("failed to add file or folder %s\n", pname);
                }
            }
        }
        parg += strlen (parg);
        parg++;
    }
    pl_save_current ();
    messagepump_push (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    plt_add_files_end (curr_plt, 0);
    plt_unref (curr_plt);
    if (!queue) {
        messagepump_push (DB_EV_PLAY_NUM, 0, 0, 0);
    }
    return 0;
}

#if USE_ABSTRACT_SOCKET_NAME
static char server_id[] = "\0deadbeefplayer";
#endif

static unsigned srv_socket;

#ifdef USE_INET_SOCKET
static struct sockaddr_in srv_local;
static struct sockaddr_in srv_remote;

int
db_socket_init_inet () {
#    ifdef __MINGW32__
    // initiate winsock
    WSADATA wsaData;
    if (WSAStartup (MAKEWORD (2, 2), &wsaData) != 0) {
        trace_err ("Error with WSAStartup(), WinSock startup failed.\n");
        return -1;
    }
#    endif

    return 0;
}

int
db_socket_set_inet (struct sockaddr_in *remote, int *len) {
    int s;
    if ((s = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
        perror ("socket");
        exit (1);
    }

    memset (remote, 0, sizeof (*remote));

    remote->sin_family = AF_INET;
    remote->sin_addr.s_addr = inet_addr ("127.0.0.1");
    remote->sin_port = htons (DEFAULT_LISTENING_PORT);

    *len = sizeof (*remote);

    return s;
}

#else
static struct sockaddr_un srv_local;
static struct sockaddr_un srv_remote;

int
db_socket_set_unix (struct sockaddr_un *remote, int *len) {
    int s;
    if ((s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror ("socket");
        exit (1);
    }

    memset (remote, 0, sizeof (struct sockaddr_un));
    remote->sun_family = AF_UNIX;

#    if USE_ABSTRACT_SOCKET_NAME
    memcpy (remote->sun_path, server_id, sizeof (server_id));
    *len = offsetof (struct sockaddr_un, sun_path) + sizeof (server_id) - 1;
#    else
    char *socketdirenv = getenv ("DDB_SOCKET_DIR");
    snprintf (remote->sun_path, sizeof (remote->sun_path), "%s/socket", socketdirenv ? socketdirenv : dbruntimedir);
    *len = offsetof (struct sockaddr_un, sun_path) + (int)strlen (remote->sun_path);
#    endif
    return s;
}
#endif

void
db_socket_close (int s) {
#ifdef __MINGW32__
    closesocket (s);
#else
    close (s);
#endif
}

int
server_start (void) {
    int len;

    trace ("server_start\n");

#ifdef USE_INET_SOCKET
    srv_socket = db_socket_set_inet (&srv_local, &len);

#    ifdef __MINGW32__
    unsigned long flags = 1;
    if (ioctlsocket (srv_socket, FIONBIO, &flags) == SOCKET_ERROR) {
        perror ("ioctlsocket FIONBIO");
        return -1;
    }
#    endif
#else
    srv_socket = db_socket_set_unix (&srv_local, &len);
#    ifndef USE_ABSTRACT_SOCKET_NAME
    if (unlink (srv_local.sun_path) < 0) {
        perror ("INFO: unlink socket");
    }
    len = offsetof (struct sockaddr_un, sun_path) + (int)strlen (srv_local.sun_path);
#    endif

    int flags;
    flags = fcntl (srv_socket, F_GETFL, 0);
    if (flags == -1) {
        perror ("fcntl F_GETFL");
        return -1;
    }
    if (fcntl (srv_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror ("fcntl F_SETFL");
        return -1;
    }
#endif

    if (len == -1) {
        return -1;
    }

    if (bind (srv_socket, (struct sockaddr *)&srv_local, len) < 0) {
        perror ("bind");
        return -1;
    }

    if (listen (srv_socket, 5) == -1) {
        perror ("listen");
        return -1;
    }
    return 0;
}

void
server_close (void) {
    if (srv_socket) {
        db_socket_close (srv_socket);
        srv_socket = 0;
    }
}

// Read the whole message till end-of-stream
char *
read_entire_message (int sockfd, int *size) {
    int bufsize = 4096; // initial buffer size, will expand if
        // the actual package turns out to be bigger
    char *buf = (char *)malloc (bufsize);
    int rdp = 0;

    for (;;) {
        if (rdp >= bufsize) {
            int newsize = bufsize * 2;
            char *newbuf = (char *)malloc (newsize);
            memcpy (newbuf, buf, rdp);
            free (buf);
            buf = newbuf;
            bufsize = newsize;
        }

        ssize_t rd = recv (sockfd, buf + rdp, bufsize - rdp, 0);
        if (rd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep (50000);
                continue;
            }
            free (buf);
            return NULL;
        }
        if (rd == 0) {
            break;
        }
        rdp += rd;
    }

    *size = rdp;
    return buf;
}

int
server_update (void) {
    // handle remote stuff
    int t = sizeof (srv_remote);
    int s2;
    s2 = accept (srv_socket, (struct sockaddr *)&srv_remote, &t);
    if (s2 == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror ("accept");
        return -1;
    }
    else if (s2 != -1) {
        int size = -1;
        char *buf = read_entire_message (s2, &size);
        char sendback[1024] = "";
        if (size > 0) {
            if (size == 1 && buf[0] == 0) {
                // FIXME: that should be called right after activation of gui plugin
                messagepump_push (DB_EV_ACTIVATED, 0, 0, 0);
            }
            else {
                server_exec_command_line (buf, size, sendback, sizeof (sendback));
            }
        }
        if (sendback[0]) {
            // send nowplaying back to client
            send (s2, sendback, strlen (sendback) + 1, 0);
        }
        else {
            send (s2, "", 1, 0);
        }
        db_socket_close (s2);

        if (buf) {
            free (buf);
        }
    }
    return 0;
}

static uintptr_t server_tid;
static int server_terminate;

void
server_loop (void *ctx) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-server", 0, 0, 0, 0);
#endif
    fd_set rds;
    int ret;
    struct timeval timeout = { 0, 0 };

    FD_ZERO (&rds);
    while (!server_terminate) {
        FD_SET (srv_socket, &rds);
        timeout.tv_usec = 500000;
        if ((ret = select (srv_socket + 1, &rds, NULL, NULL, &timeout)) < 0 && errno != EINTR) {
            perror ("select");
            exit (-1);
        }
        if (ret > 0) {
            if (server_update () < 0) {
                messagepump_push (DB_EV_TERMINATE, 0, 0, 0);
            }
        }
    }
}

void
save_resume_state (void) {
    playItem_t *trk = streamer_get_playing_track ();
    DB_output_t *output = plug_get_output ();
    float playpos = -1;
    int playtrack = -1;
    int playlist = -1;
    playlist_t *plt = pl_get_playlist (trk);
    int paused = (output->state () == DDB_PLAYBACK_STATE_PAUSED);
    if (trk && plt) {
        playlist = plt_get_idx_of (plt);
        playtrack = plt_get_item_idx (plt, trk, PL_MAIN);
        playpos = streamer_get_playpos ();
        pl_item_unref (trk);
    }

    if (plt) {
        plt_unref (plt);
        plt = NULL;
    }

    conf_set_float ("resume.position", playpos);
    conf_set_int ("resume.track", playtrack);
    conf_set_int ("resume.playlist", playlist);
    conf_set_int ("resume.paused", paused);
}

void
player_mainloop (void) {
    for (;;) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        int term = 0;
        while (messagepump_pop (&msg, &ctx, &p1, &p2) != -1) {
            // broadcast to all plugins
            DB_plugin_t **plugs = plug_get_list ();
            for (int n = 0; plugs[n]; n++) {
                if (plugs[n]->message) {
                    plugs[n]->message (msg, ctx, p1, p2);
                }
            }
            if (!term) {
                DB_output_t *output = plug_get_output ();
                switch (msg) {
                case DB_EV_REINIT_SOUND:
                    plug_reinit_sound ();
                    conf_save ();
                    break;
                case DB_EV_TERMINATE: {
                    save_resume_state ();

                    playqueue_clear ();

                    term = 1;
                } break;
                case DB_EV_PLAY_CURRENT:
                    streamer_play_current_track ();
                    break;
                case DB_EV_PLAY_NUM:
                    playqueue_clear ();
                    streamer_set_nextsong (p1, 0);
                    break;
                case DB_EV_STOP:
                    streamer_set_nextsong (-1, 0);
                    break;
                case DB_EV_NEXT:
                    streamer_move_to_nextsong (1);
                    break;
                case DB_EV_PREV:
                    streamer_move_to_prevsong (1);
                    break;
                case DB_EV_PLAY_NEXT_ALBUM:
                    streamer_move_to_nextalbum (1);
                    break;
                case DB_EV_PLAY_PREV_ALBUM:
                    streamer_move_to_prevalbum (1);
                    break;
                case DB_EV_PAUSE:
                    if (output->state () != DDB_PLAYBACK_STATE_PAUSED) {
                        output->pause ();
                        messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                    }
                    break;
                case DB_EV_TOGGLE_PAUSE:
                    if (output->state () == DDB_PLAYBACK_STATE_PAUSED) {
                        output->unpause ();
                        messagepump_push (DB_EV_PAUSED, 0, 0, 0);
                    }
                    else if (output->state () == DDB_PLAYBACK_STATE_PLAYING) {
                        output->pause ();
                        messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                    }
                    break;
                case DB_EV_PLAY_RANDOM:
                    streamer_move_to_randomsong (1);
                    break;
                case DB_EV_PLAY_RANDOM_ALBUM:
                    streamer_move_to_randomalbum (1);
                    break;
                case DB_EV_CONFIGCHANGED:
                    conf_save ();
                    streamer_configchanged ();
                    pl_configchanged ();
                    junk_configchanged ();
                    break;
                case DB_EV_SEEK: {
                    int32_t pos = (int32_t)p1;
                    if (pos < 0) {
                        pos = 0;
                    }
                    streamer_set_seek (p1 / 1000.f);
                } break;
                case DB_EV_PLAYLISTCHANGED:
                    switch (p1) {
                    case DDB_PLAYLIST_CHANGE_CONTENT:
                    case DDB_PLAYLIST_CHANGE_CREATED:
                    case DDB_PLAYLIST_CHANGE_DELETED:
                        streamer_notify_track_deleted ();
                        break;
                    }
                    // fallthrough
                case DB_EV_PAUSED:
                case DB_EV_SONGFINISHED:
                    save_resume_state ();
                    conf_save ();
                    break;
                }
            }
            if (msg >= DB_EV_FIRST && ctx) {
                messagepump_event_free ((ddb_event_t *)ctx);
            }
        }
        if (term) {
            break;
        }
        messagepump_wait ();
    }
}

#ifdef __GLIBC__
void
sigsegv_handler (int sig) {
    trace_err ("Segmentation Fault\n");
    int j, nptrs;
#    define SIZE 100
    void *buffer[100];
    char **strings;

    nptrs = backtrace (buffer, SIZE);
    printf ("backtrace() returned %d addresses\n", nptrs);

    /* The call
     * backtrace_symbols_fd(buffer,
     * nptrs,
     * STDOUT_FILENO)
     would produce similar output to the following: */

    strings = backtrace_symbols (buffer, nptrs);
    if (strings == NULL) {
        perror ("backtrace_symbols");
        exit (EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        printf ("%s\n", strings[j]);

    free (strings);
    exit (0);
}
#endif

void
restore_resume_state (void) {
    DB_output_t *output = plug_get_output ();
    if (conf_get_int ("resume_last_session", 1) && output->state () == DDB_PLAYBACK_STATE_STOPPED) {
        int plt = conf_get_int ("resume.playlist", -1);
        int track = conf_get_int ("resume.track", -1);
        float pos = conf_get_float ("resume.position", -1);
        int paused = conf_get_int ("resume.paused", 0);
        trace ("resume: track %d pos %f playlist %d\n", track, pos, plt);
        if (plt >= 0 && track >= 0 && pos >= 0) {
            plt_set_curr_idx (plt);
            streamer_set_current_playlist (plt);
            if (!_previous_session_did_crash) {
                streamer_yield ();
                streamer_set_nextsong (track, paused);
                streamer_yield ();
                streamer_set_seek (pos);
            }
        }
    }
}

uintptr_t mainloop_tid;

DB_plugin_t *
plug_get_gui (void) {
    struct DB_plugin_s **plugs = plug_get_list ();
    for (int i = 0; plugs[i]; i++) {
        if (plugs[i]->type == DB_PLUGIN_GUI) {
            return plugs[i];
        }
    }
    return NULL;
}

#if !_DEBUG
static void
_touch (const char *path) {
    struct stat stat_struct;
    if (0 != stat (path, &stat_struct)) {
        FILE *fp = fopen (path, "w+b");
        if (fp != NULL) {
            (void)fclose (fp);
        }
    }
    else {
        (void)utime (path, NULL);
    }
}
#endif

void
main_cleanup_and_quit (void) {
    // stop streaming and playback before unloading plugins
    DB_output_t *output = plug_get_output ();
    output->stop ();
    streamer_free ();

    // drain main message queue
    uint32_t msg;
    uintptr_t ctx;
    uint32_t p1;
    uint32_t p2;
    while (messagepump_pop (&msg, &ctx, &p1, &p2) != -1) {
        if (msg >= DB_EV_FIRST && ctx) {
            messagepump_event_free ((ddb_event_t *)ctx);
        }
    }

    output->free ();

    // terminate server and wait for completion
    if (server_tid) {
        server_terminate = 1;
        thread_join (server_tid);
        server_tid = 0;
    }

    // save config
    pl_save_all ();
    conf_save ();

    // delete legacy session file
    {
        char sessfile[1024]; // $HOME/.config/deadbeef/session
        if (snprintf (sessfile, sizeof (sessfile), "%s/deadbeef/session", confdir) < (int)sizeof (sessfile)) {
            unlink (sessfile);
        }
    }

    // stop receiving messages from outside
    server_close ();

    // plugins might still hold references to playitems,
    // and query configuration in background
    // so unload everything 1st before final cleanup
    plug_disconnect_all ();
    plug_unload_all (^{
    // at this point we can simply do exit(0), but let's clean up for debugging
#ifdef OSX_APPBUNDLE
        scriptableDeinitShared ();
#endif

        pl_free (); // may access conf_*
        ddb_undomanager_free(ddb_undomanager_shared());

        conf_free ();

        tf_deinit ();

        trace ("messagepump_free\n");
        messagepump_free ();
        trace ("plug_cleanup\n");
        plug_cleanup ();
        trace ("logger_free\n");

        metacache_deinit ();

        trace ("💛💙\n");
        ddb_logger_free ();

        char crash_marker[PATH_MAX];
        snprintf (crash_marker, sizeof (crash_marker), "%s/running", dbconfdir);
        unlink (crash_marker);

        exit (0);
    });

}

static void
mainloop_thread (void *ctx) {
    // this runs until DB_EV_TERMINATE is sent (blocks right here)
    player_mainloop ();

    // tell the gui thread to finish
    DB_plugin_t *gui = plug_get_gui ();
    if (gui) {
        gui->stop ();
    }

    return;
}

static int
mkdir_recursive(const char *path) {
    char *tmp = strdup(path);
    if (tmp == NULL) {
        return -1;
    }

    char *p = tmp;

    do {
        while (*p == '/') {
            p++;
        }

        p = strchr(p, '/');

        if (p != NULL) {
            *p = '\0';
        }

        if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
            free(tmp);
            return -1;
        }

        if (p != NULL) {
            *p = '/';
        }

    } while (p != NULL);

    free(tmp);
    return 0;
}

int
main (int argc, char *argv[]) {
#if __MINGW32__
    windows_arg_fix (&argc, argv);
#endif
    ddb_logger_init ();
    int portable = 0;
    int staticlink = 0;
    int portable_full = 0;
#if STATICLINK
    staticlink = 1;
#endif
#if PORTABLE
    portable = 1;
#endif
#if PORTABLE_FULL
    portable_full = 1;
#endif

    int exe_path_found = 0;
#ifndef _WIN32
    char *exe_path = calloc (PATH_MAX, 1);
    if (-1 != readlink ("/proc/self/exe", exe_path, PATH_MAX)) {
        char *e = strrchr (exe_path, '/');
        if (e != NULL) {
            *e = 0;

            // check for plugins folder
            char *plugins_path = calloc (PATH_MAX, 1);
            snprintf (plugins_path, PATH_MAX, "%s/plugins", exe_path);

            struct stat st;
            if (0 == stat (plugins_path, &st) && S_ISDIR (st.st_mode)) {
                exe_path_found = 1;
                strcpy (dbinstalldir, exe_path);
            }
            free (plugins_path);
        }
    }
    free (exe_path);
#endif
    if (!exe_path_found) {
        if (!realpath (argv[0], dbinstalldir)) {
            strcpy (dbinstalldir, argv[0]);
        }
        char *e = strrchr (dbinstalldir, '/');
        if (e) {
            *e = 0;
        }
        else {
            strcpy (dbinstalldir, PREFIX);
        }
    }

    // detect portable version by looking for plugins/ and deadbeef.png and portable_full by config/
    while (!portable || !portable_full) {
        struct stat st;
        char checkpath[PATH_MAX];
        if (!portable) {
            snprintf (checkpath, sizeof (checkpath), "%s/plugins", dbinstalldir);
            if (stat (checkpath, &st) || !S_ISDIR (st.st_mode)) {
                break;
            }
            snprintf (checkpath, sizeof (checkpath), "%s/deadbeef.png", dbinstalldir);
            if (stat (checkpath, &st) || !S_ISREG (st.st_mode)) {
                break;
            }
            portable = 1;
        }
        if (!portable_full) {
            snprintf (checkpath, sizeof (checkpath), "%s/config", dbinstalldir);
            if (stat (checkpath, &st) || !S_ISDIR (st.st_mode)) {
                break;
            }
            portable_full = 1;
        }
        break;
    }
    if (!portable) {
        strcpy (dbinstalldir, PREFIX);
    }

#ifdef __GLIBC__
    signal (SIGSEGV, sigsegv_handler);
#endif
    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C");
#ifdef ENABLE_NLS
    //    trace ("enabling gettext support: package=" PACKAGE ", dir=" LOCALEDIR "...\n");
    if (portable) {
        char localedir[PATH_MAX];
        snprintf (localedir, sizeof (localedir), "%s/locale", dbinstalldir);
        bindtextdomain (PACKAGE, localedir);
    }
    else {
        bindtextdomain (PACKAGE, LOCALEDIR);
    }
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    textdomain (PACKAGE);
#endif

#ifndef VERSION
#    define VERSION "devel"
#endif

    trace ("starting deadbeef " VERSION "%s%s\n", staticlink ? " [static]" : "", portable ? " [portable]" : "");
    srand ((unsigned int)time (NULL));
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-main", 0, 0, 0, 0);
#endif

    char *homedir = getenv (HOMEDIR);
    if (!homedir) {
        trace_err ("unable to find home directory. stopping.\n");
        return -1;
    }

    // Get config directory
    if (portable_full) {
        if (snprintf (confdir, sizeof (confdir), "%s/config", dbinstalldir) > (int)sizeof (confdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
        strcpy (dbconfdir, confdir);
    }
    else {
        char *xdg_conf_dir = getenv (CONFIGDIR);
        if (xdg_conf_dir) {
            if (snprintf (confdir, sizeof (confdir), "%s", xdg_conf_dir) > (int)sizeof (confdir)) {
                trace_err ("fatal: XDG_CONFIG_HOME value is too long: %s\n", xdg_conf_dir);
                return -1;
            }
        }
        else {
            if (snprintf (confdir, sizeof (confdir), "%s/" SYS_CONFIG_DIR, homedir) > (int)sizeof (confdir)) {
                trace_err ("fatal: HOME value is too long: %s\n", homedir);
                return -1;
            }
        }

        if (snprintf (dbconfdir, sizeof (dbconfdir), "%s/deadbeef", confdir) > (int)sizeof (dbconfdir)) {
            trace_err ("fatal: config path is too long: %s\n", dbconfdir);
            return -1;
        }
    }
    (void)mkdir_recursive (confdir);

#if __APPLE__
    char appcachepath[PATH_MAX];
    cocoautil_get_cache_path (appcachepath, sizeof (appcachepath));
    if (snprintf (dbcachedir, sizeof (dbcachedir), "%s/Deadbeef", appcachepath) > (int)sizeof (dbcachedir)) {
        trace_err ("fatal: cache path is too long: %s\n", dbcachedir);
        return -1;
    }
#else
    // Get cache directory
    if (portable_full) {
        if (snprintf (dbcachedir, sizeof (dbcachedir), "%s/cache", dbconfdir) > (int)sizeof (dbcachedir)) {
            trace_err ("fatal: cache path is too long: %s\n", dbcachedir);
            return -1;
        }
    }
    else {
        const char *xdg_cache = getenv (CACHEDIR);
        if (xdg_cache) {
            if (snprintf (dbcachedir, sizeof (dbcachedir), "%s/deadbeef/", xdg_cache) > (int)sizeof (dbcachedir)) {
                trace_err ("fatal: cache path is too long: %s\n", dbcachedir);
                return -1;
            }
        }
        else {
            if (snprintf (dbcachedir, sizeof (dbcachedir), "%s/.cache/deadbeef", homedir) > (int)sizeof (dbcachedir)) {
                trace_err ("fatal: cache path is too long: %s\n", dbcachedir);
                return -1;
            }
        }
    }
#endif

    // Get runtime directory
    const char *xdg_runtime = getenv (RUNTIMEDIR);
    if (xdg_runtime) {
        if (snprintf (dbruntimedir, sizeof (dbruntimedir), "%s/deadbeef", xdg_runtime) >= (int)sizeof (dbruntimedir)) {
            trace_err ("fatal: cache path is too long: %s\n", dbruntimedir);
            return -1;
        }
        (void)mkdir_recursive (dbruntimedir);
    }
    else {
        strcpy (dbruntimedir, dbconfdir);
    }

    // Get plugins dir from environment variable, portable directory or library dir
    char *env_plugin_dir = getenv ("DEADBEEF_PLUGIN_DIR");
    if (env_plugin_dir) {
        strncpy (dbplugindir, env_plugin_dir, sizeof (dbplugindir));
        if (dbplugindir[sizeof (dbplugindir) - 1] != 0) {
            trace_err ("fatal: plugin path is too long: %s\n", env_plugin_dir);
            return -1;
        }
    }
    else if (portable) {
#ifdef OSX_APPBUNDLE
        cocoautil_get_plugins_path (dbplugindir, sizeof (dbplugindir));
#else
        if (snprintf (dbplugindir, sizeof (dbplugindir), "%s/plugins", dbinstalldir) > sizeof (dbplugindir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
#endif
        (void)mkdir_recursive (dbplugindir);
    }
    else {
        if (snprintf (dbplugindir, sizeof (dbplugindir), "%s/deadbeef", LIBDIR) > (int)sizeof (dbplugindir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
    }

    if (portable) {
#ifdef OSX_APPBUNDLE
        cocoautil_get_resources_path (dbresourcedir, sizeof (dbresourcedir));
#else
        if (snprintf (dbresourcedir, sizeof (dbresourcedir), "%s/plugins", dbinstalldir) > sizeof (dbresourcedir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
#endif
        (void)mkdir_recursive (dbresourcedir);
    }
    else {
        strcpy (dbresourcedir, dbplugindir);
    }

    // Get doc and pixmaps dirs
    if (portable) {
#ifdef OSX_APPBUNDLE
        if (snprintf (dbdocdir, sizeof (dbdocdir), "%s/doc", dbresourcedir) > (int)sizeof (dbdocdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbresourcedir);
            return -1;
        }
        if (snprintf (dbpixmapdir, sizeof (dbpixmapdir), "%s/pixmaps", dbresourcedir) > (int)sizeof (dbpixmapdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbresourcedir);
            return -1;
        }
#else
        if (snprintf (dbdocdir, sizeof (dbdocdir), "%s/doc", dbinstalldir) > sizeof (dbdocdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
        if (snprintf (dbpixmapdir, sizeof (dbpixmapdir), "%s/pixmaps", dbinstalldir) > sizeof (dbpixmapdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
#endif
    }
    else {
        if (snprintf (dbdocdir, sizeof (dbdocdir), "%s", DOCDIR) > (int)sizeof (dbdocdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
        if (snprintf (dbpixmapdir, sizeof (dbpixmapdir), "%s/share/deadbeef/pixmaps", PREFIX) >
            (int)sizeof (dbpixmapdir)) {
            trace_err ("fatal: install path is too long: %s\n", dbinstalldir);
            return -1;
        }
    }

    // State directory
#if __APPLE__
    char statedir[PATH_MAX];
    cocoautil_get_application_support_path (statedir, sizeof (statedir));
    if (snprintf (dbstatedir, sizeof (dbstatedir), "%s/Deadbeef/State", statedir) > (int)sizeof (dbstatedir)) {
        trace_err ("fatal: state path is too long: %s\n", dbstatedir);
        return -1;
    }
#else
    const char *xdg_state = getenv (STATEDIR);
    if (xdg_state != NULL) {
        if (snprintf (dbstatedir, sizeof (dbstatedir), "%s/deadbeef", xdg_state) > (int)sizeof (dbstatedir)) {
            trace_err ("fatal: state path is too long: %s\n", dbstatedir);
            return -1;
        }
    }
    else {
        if (snprintf (dbstatedir, sizeof (dbstatedir), "%s/.local/state/deadbeef", homedir) > (int)sizeof (dbstatedir)) {
            trace_err ("fatal: state path is too long: %s\n", dbstatedir);
            return -1;
        }
    }
#endif
    (void)mkdir_recursive (dbstatedir);

    const char *plugname = "main";
    for (int i = 1; i < argc; i++) {
        if (!strncmp (argv[i], "--plugin=", strlen ("--plugin="))) {
            plugname = argv[i] + strlen ("--plugin=");
        }
        // help, version and nowplaying are executed with any filter
        if (!strcmp (argv[i], "--help") || !strcmp (argv[i], "-h")) {
            if (!strcmp (plugname, "main")) {
                print_help ();
                return 0;
            }
        }
        else if (!strcmp (argv[i], "--version")) {
            printf ("DeaDBeeF " VERSION " Copyright © 2009-2021 Oleksiy Yakovenko\n");
            return 0;
        }
        else if (!strcmp (argv[i], "--gui")) {
            if (i == argc - 1) {
                break;
            }
            i++;
            strncpy (use_gui_plugin, argv[i], sizeof (use_gui_plugin) - 1);
            use_gui_plugin[sizeof (use_gui_plugin) - 1] = 0;
        }
    }

    //    trace ("installdir: %s\n", dbinstalldir);
    //    trace ("confdir: %s\n", confdir);
    //    trace ("docdir: %s\n", dbdocdir);
    //    trace ("plugindir: %s\n", dbplugindir);
    //    trace ("pixmapdir: %s\n", dbpixmapdir);

#ifdef __MINGW32__
    char *directories[] = {
        dbconfdir, dbinstalldir, dbdocdir, dbplugindir, dbpixmapdir, dbcachedir, dbresourcedir, NULL
    };
    for (int i = 0; directories[i] != NULL; i++) {
        // replace backslashes with normal slashes
        if (strchr (directories[i], '\\')) {
            char *slash_p = directories[i];
            while (slash_p = strchr (slash_p, '\\')) {
                *slash_p = '/';
                slash_p++;
            }
        }
    }
#endif

    (void)mkdir_recursive (dbconfdir);

    int size = 0;
    char *cmdline = prepare_command_line (argc, argv, &size);

    // try to connect to remote player
    int s;
    unsigned int len;
#ifdef USE_INET_SOCKET
    struct sockaddr_in remote;
    if (db_socket_init_inet () < 0) {
        exit (-1);
    }
    s = db_socket_set_inet (&remote, &len);
#else
    struct sockaddr_un remote;
    s = db_socket_set_unix (&remote, &len);
#endif
    if (connect (s, (struct sockaddr *)&remote, len) == 0) {
        // pass args to remote and exit
        if (send (s, cmdline, size, 0) == -1) {
            perror ("send");
            exit (-1);
        }
        // end of message
        shutdown (s, SHUT_WR);

        int sz = -1;
        char *out = read_entire_message (s, &sz);
        if (sz == -1) {
            trace_err ("failed to pass args to remote!\n");
            exit (-1);
        }
        else {
            // check if that's nowplaying response
            if (*out == '\1') {
                const char *prn = out + 1;
                fwrite (prn, 1, strlen (prn), stdout);
            }
            else if (*out == '\2') {
                const char *prn = out + 1;
                trace_err ("%s", prn);
            }
            else if (sz > 0 && out[0]) {
                trace_err ("%s\n", out);
            }
        }
        int exit_code = out[0] ? out[0] - 1 : 0;
        if (out) {
            free (out);
        }
        db_socket_close (s);
        exit (exit_code);
    }
    //    else {
    //        perror ("INFO: failed to connect to existing session:");
    //    }
    db_socket_close (s);

    // become a server
    if (server_start () < 0) {
        exit (-1);
    }

    // hack: report nowplaying
    if (!strcmp (cmdline, "--nowplaying")) {
        char nothing[] = "nothing";
        fwrite (nothing, 1, sizeof (nothing) - 1, stdout);
        free (cmdline);
        cmdline = NULL;
        return 0;
    }

    char crash_marker[PATH_MAX];
    snprintf (crash_marker, sizeof (crash_marker), "%s/running", dbconfdir);

#if !_DEBUG
    struct stat crash_marker_stat = { 0 };
    if (!stat (crash_marker, &crash_marker_stat)) {
        trace_err ("We had a crash. Will not resume the saved session to avoid a crash cycle.\n");
        _previous_session_did_crash = 1;
    }

    _touch (crash_marker);
#endif

    metacache_init ();
    tf_init ();
    pl_init ();
    conf_init ();
    conf_load (); // required by some plugins at startup

    if (use_gui_plugin[0]) {
        conf_set_str ("gui_plugin", use_gui_plugin);
    }

    conf_set_str ("deadbeef_version", VERSION);

    // Volume needs to be initialized before the plugins start

    int volume_needs_migration = conf_get_str_fast ("playback.volume.normalized", NULL) == NULL;
    if (volume_needs_migration) {
        // migrate the volume from dB to normalized / amplitude
        float vol_db = conf_get_float ("playback.volume", 0);
        volume_set_db (vol_db);
    }
    else {
        volume_set_amp (conf_get_float ("playback.volume.normalized", 1));
    }

    messagepump_init (); // required to push messages while handling commandline

#ifdef OSX_APPBUNDLE
    scriptableInitShared ();
#endif
    if (plug_load_all ()) { // required to add files to playlist from commandline
        exit (-1);
    }

    streamer_playmodes_init ();

    ddb_undomanager_shared_init (NULL);

    pl_load_all ();

    // execute server commands in local context
    int noloadpl = 0;
    if (argc > 1) {
        int res = server_exec_command_line (cmdline, size, NULL, 0);
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

    free (cmdline);

#ifdef OSX_APPBUNDLE
    scriptableDspLoadPresets (scriptableRootShared ());
    scriptableEncoderLoadPresets (scriptableRootShared ());
#endif

    streamer_init ();

    plug_connect_all ();
    messagepump_push (DB_EV_PLUGINSLOADED, 0, 0, 0);

    if (!noloadpl) {
        restore_resume_state ();
        plt_set_curr_idx (conf_get_int ("playlist.current", 0));
    }

    server_tid = thread_start (server_loop, NULL);

    mainloop_tid = thread_start (mainloop_thread, NULL);

    messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);

    DB_plugin_t *gui = plug_get_gui ();
    if (gui) {
        gui->start ();
    }

    ddb_logger_stop_buffering ();

    // NOTE: It's not guaranteed that the code after this line will be called.
    // On some platforms (cocoa), main_cleanup_and_quit is called directly before quit.

    trace ("gui plugin has quit; waiting for mainloop thread to finish\n");
    thread_join (mainloop_tid);

    main_cleanup_and_quit ();

    // main_cleanup_and_quit will call "exit" after async jobs finish, which may occur on another thread.
    // Therefore infinite wait here.
    for (;;) {
        usleep (10000000);
    }
    return 0;
}
