/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include "threading.h"
#include "playlist.h"
#include "common.h"
#include "streamer.h"
#include "messagepump.h"
#include "conf.h"
#include "plugins.h"
#include "optmath.h"
#include "volume.h"
#include "vfs.h"
#include "premix.h"
#include "ringbuf.h"
#include "replaygain.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//#define WRITE_DUMP 1
//#define DETECT_PL_LOCK_RC 1

#if WRITE_DUMP
FILE *out;
#endif

#define MAX_PLAYLIST_DOWNLOAD_SIZE 25000

static int
streamer_read_async (char *bytes, int size);

static int
streamer_set_output_format (void);

static intptr_t streamer_tid;
static ddb_dsp_context_t *dsp_chain;
static float dsp_ratio = 1;

static DB_dsp_t *eqplug;
static ddb_dsp_context_t *eq;

static int dsp_on = 0;

static int autoconv_8_to_16 = 1;

static int streaming_terminate;

// buffer up to 3 seconds at 44100Hz stereo
#define STREAM_BUFFER_SIZE 0x80000 // slightly more than 3 seconds of 44100 stereo

// how much bigger should read-buffer be to allow upsampling.
// e.g. 8000Hz -> 192000Hz upsampling requires 24x buffer size,
// so if we originally request 4096 bytes blocks -
// that will require 24x buffer size, which is 98304 bytes buffer
#define MAX_DSP_RATIO 24

#define MIN_BLOCK_SIZE 4096
#define MAX_BLOCK_SIZE 16384
#define READBUFFER_SIZE (MAX_BLOCK_SIZE * MAX_DSP_RATIO)
static char readbuffer[READBUFFER_SIZE];

static ringbuf_t streamer_ringbuf;
static char streambuffer[STREAM_BUFFER_SIZE];

static int bytes_until_next_song = 0;
static uintptr_t mutex;
static uintptr_t decodemutex;
static int nextsong = -1;
static int nextsong_pstate = -1;
static int badsong = -1;

static float seekpos = -1;

static float playpos = 0; // play position of current song
static int avg_bitrate = -1; // avg bitrate of current song
static int last_bitrate = -1; // last bitrate of current song

static playlist_t *streamer_playlist;
static playItem_t *playing_track;
static float playtime; // total playtime of playing track
static time_t started_timestamp; // result of calling time(NULL)
static playItem_t *streaming_track;
static playItem_t *playlist_track;

static ddb_waveformat_t output_format; // format that was requested after DSP
static ddb_waveformat_t orig_output_format; // format that was requested before DSP
static int formatchanged;

static DB_fileinfo_t *fileinfo;
static DB_fileinfo_t *new_fileinfo;

static int streamer_buffering;

// to allow interruption of stall file requests
static DB_FILE *streamer_file;

#if DETECT_PL_LOCK_RC
volatile pthread_t streamer_lock_tid = 0;
#endif
void
streamer_lock (void) {
#if DETECT_PL_LOCK_RC
    extern pthread_t pl_lock_tid;
    assert (pl_lock_tid != pthread_self()); // not permitted to lock streamer from inside of pl_lock
#endif
    mutex_lock (mutex);
#if DETECT_PL_LOCK_RC
    streamer_lock_tid = pthread_self();
#endif
}

void
streamer_unlock (void) {
#if DETECT_PL_LOCK_RC
    streamer_lock_tid = 0;
#endif
    mutex_unlock (mutex);
}

static void
streamer_abort_files (void) {
    trace ("\033[0;33mstreamer_abort_files\033[37;0m\n");
    mutex_lock (decodemutex);
    if (fileinfo && fileinfo->file) {
        deadbeef->fabort (fileinfo->file);
    }
    if (new_fileinfo && new_fileinfo->file) {
        deadbeef->fabort (new_fileinfo->file);
    }
    if (streamer_file) {
        deadbeef->fabort (streamer_file);
    }
    mutex_unlock (decodemutex);
}


void
streamer_set_replaygain (playItem_t *it) {
    // setup replaygain
    pl_lock ();
    const char *gain;
    gain = pl_find_meta (it, ":REPLAYGAIN_ALBUMGAIN");
    float albumgain = gain ? atof (gain) : 1000;
    float albumpeak = pl_get_item_replaygain (it, DDB_REPLAYGAIN_ALBUMPEAK);

    gain = pl_find_meta (it, ":REPLAYGAIN_TRACKGAIN");
    float trackgain = gain ? atof (gain) : 1000;
    float trackpeak = pl_get_item_replaygain (it, DDB_REPLAYGAIN_TRACKPEAK);
    pl_unlock ();
    replaygain_set_values (albumgain, albumpeak, trackgain, trackpeak);
}

static void
send_songstarted (playItem_t *trk) {
    ddb_event_track_t *pev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_SONGSTARTED);
    pev->track = DB_PLAYITEM (trk);
    pl_item_ref (trk);
    pev->playtime = 0;
    pev->started_timestamp = time(NULL);
    messagepump_push_event ((ddb_event_t*)pev, 0, 0);
}

static void
send_songfinished (playItem_t *trk) {
    ddb_event_track_t *pev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_SONGFINISHED);
    pev->track = DB_PLAYITEM (trk);
    pl_item_ref (trk);
    pev->playtime = playtime;
    pev->started_timestamp = started_timestamp;
    messagepump_push_event ((ddb_event_t*)pev, 0, 0);
}

static void
send_trackchanged (playItem_t *from, playItem_t *to) {
    ddb_event_trackchange_t *event = (ddb_event_trackchange_t *)messagepump_event_alloc (DB_EV_SONGCHANGED);
    event->playtime = playtime;
    event->started_timestamp = started_timestamp;
    if (from) {
        pl_item_ref (from);
    }
    if (to) {
        pl_item_ref (to);
    }
    event->from = (DB_playItem_t *)from;
    event->to = (DB_playItem_t *)to;
    messagepump_push_event ((ddb_event_t *)event, 0, 0);
}

void
streamer_start_playback (playItem_t *from, playItem_t *it) {
    if (from) {
        pl_item_ref (from);
    }
    if (it) {
        pl_item_ref (it);
    }
    // free old copy of playing
    if (playing_track) {
        pl_item_unref (playing_track);
        playing_track = NULL;
    }
    pl_lock ();
    playlist_track = it;
    pl_unlock ();
    // assign new
    playing_track = it;
    if (playing_track) {
        pl_item_ref (playing_track);

        playing_track->played = 1;
        trace ("from=%p (%s), to=%p (%s) [2]\n", from, from ? pl_find_meta (from, ":URI") : "null", it, it ? pl_find_meta (it, ":URI") : "null");
        send_trackchanged (from, it);
        started_timestamp = time (NULL);
    }
    if (from) {
        pl_item_unref (from);
    }
    if (it) {
        pl_item_unref (it);
    }
    trace ("streamer_start_playback %s\n", playing_track ? pl_find_meta (playing_track, ":URI") : "null");
}

playItem_t *
streamer_get_streaming_track (void) {
    if (streaming_track) {
        pl_item_ref (streaming_track);
    }
    return streaming_track;
}

playItem_t *
streamer_get_playing_track (void) {
    playItem_t *it = playing_track;// ? playing_track : playlist_track;
    if (it) {
        pl_item_ref (it);
    }
    return it;
}

int
str_get_idx_of (playItem_t *it) {
    pl_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr ();
    }
    playItem_t *c = streamer_playlist->head[PL_MAIN];
    int idx = 0;
    while (c && c != it) {
        c = c->next[PL_MAIN];
        idx++;
    }
    if (!c) {
        pl_unlock ();
        return -1;
    }
    pl_unlock ();
    return idx;
}

playItem_t *
str_get_for_idx (int idx) {
    pl_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr ();
    }
    playItem_t *it = streamer_playlist->head[PL_MAIN];
    while (idx--) {
        if (!it) {
            pl_unlock ();
            return NULL;
        }
        it = it->next[PL_MAIN];
    }
    if (it) {
        pl_item_ref (it);
    }
    pl_unlock ();
    return it;
}

static void
send_trackinfochanged (playItem_t *track) {
    ddb_event_track_t *ev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_TRACKINFOCHANGED);
    ev->track = DB_PLAYITEM (track);
    if (track) {
        pl_item_ref (track);
    }
    messagepump_push_event ((ddb_event_t*)ev, 0, 0);
}

int
streamer_move_to_nextsong (int reason) {
    trace ("streamer_move_to_nextsong (%d)\n", reason);
    pl_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr ();
    }
    while (pl_playqueue_getcount ()) {
        trace ("pl_playqueue_getnext\n");
        playItem_t *it = pl_playqueue_getnext ();
        if (it) {
            pl_playqueue_pop ();
            int r = str_get_idx_of (it);
            if (r >= 0) {
                pl_item_unref (it);
                pl_unlock ();
                streamer_set_nextsong (r, 1);
                return 0;
            }
            else {
                trace ("%s not found in current streaming playlist\n", pl_find_meta (it, ":URI"));

                playlist_t *p = pl_get_playlist (it);
                if (p) {
                    if (streamer_playlist) {
                        plt_unref (streamer_playlist);
                    }
                    streamer_playlist = p;
                    int r = str_get_idx_of (it);
                    if (r >= 0) {
                        pl_item_unref (it);
                        pl_unlock ();
                        streamer_set_nextsong (r, 3);
                        return 0;
                    }
                }
                trace ("%s not found in any playlists\n", pl_find_meta (it, ":URI"));
                pl_item_unref (it);
            }
        }
    }

    playItem_t *curr = playlist_track;
    if (reason == 1) {
        if (streamer_playlist) {
            plt_unref (streamer_playlist);
        }
        streamer_playlist = plt_get_curr ();
        // check if prev song is in this playlist
        if (-1 == str_get_idx_of (curr)) {
            curr = NULL;
        }
    }

    playlist_t *plt = streamer_playlist;
    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        streamer_set_nextsong (-2, 1);
        return 0;
    }
    int pl_order = pl_get_order ();

    int pl_loop_mode = conf_get_int ("playback.loop", 0);

    if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
        int r = str_get_idx_of (playing_track);
        pl_unlock ();
        if (r == -1) {
            streamer_set_nextsong (-2, 1);
        }
        else {
            streamer_set_nextsong (r, 1);
        }
        return 0;
    }

    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        if (!curr || pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS) {
            // find minimal notplayed
            playItem_t *pmin = NULL; // notplayed minimum
            for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i->played) {
                    continue;
                }
                if (!pmin || i->shufflerating < pmin->shufflerating) {
                    pmin = i;
                }
            }
            playItem_t *it = pmin;
            if (!it) {
                // all songs played, reshuffle and try again
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) { // loop
                    plt_reshuffle (streamer_playlist, &it, NULL);
                }
            }
            if (!it) {
                streamer_buffering = 0;
                send_trackinfochanged (streaming_track);
                playItem_t *temp;
                plt_reshuffle (streamer_playlist, &temp, NULL);
                pl_unlock ();
                streamer_set_nextsong (-2, -2);
                return -1;
            }
            int r = str_get_idx_of (it);
            pl_unlock ();
            streamer_set_nextsong (r, 1);
            return 0;
        }
        else {
            trace ("pl_next_song: reason=%d, loop=%d\n", reason, pl_loop_mode);
            // find minimal notplayed above current
            int rating = curr->shufflerating;
            playItem_t *pmin = NULL; // notplayed minimum
            for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i->played || i->shufflerating < rating) {
                    continue;
                }
                if (!pmin || i->shufflerating < pmin->shufflerating) {
                    pmin = i;
                }
            }
            playItem_t *it = pmin;
            if (!it) {
                // all songs played, reshuffle and try again
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL || reason == 1) { // loop
                    trace ("all songs played! reshuffle\n");
                    plt_reshuffle (streamer_playlist, &it, NULL);
                }
            }
            if (!it) {
                streamer_buffering = 0;
                send_trackinfochanged (streaming_track);
                playItem_t *temp;
                plt_reshuffle (streamer_playlist, &temp, NULL);
                pl_unlock ();
                streamer_set_nextsong (-2, -2);
                return -1;
            }
            int r = str_get_idx_of (it);
            pl_unlock ();
            streamer_set_nextsong (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (curr) {
            it = curr->next[PL_MAIN];
        }
        else {
            it = streamer_playlist->head[PL_MAIN];
        }
        if (!it) {
            trace ("streamer_move_nextsong: was last track\n");
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = plt->head[PL_MAIN];
            }
            else {
                streamer_buffering = 0;
                send_trackinfochanged (streaming_track);
                badsong = -1;
                pl_unlock ();
                streamer_set_nextsong (-2, -2);
                return 0;
            }
        }
        if (!it) {
            pl_unlock ();
            return -1;
        }
        int r = str_get_idx_of (it);
        pl_unlock ();
        streamer_set_nextsong (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_unlock ();
        int res = streamer_move_to_randomsong ();
        if (res == -1) {
            trace ("streamer_move_to_randomsong error\n");
            streamer_set_nextsong (-2, 1);
            return -1;
        }
        return 0;
    }
    pl_unlock ();
    return -1;
}

int
streamer_move_to_prevsong (void) {
    pl_lock ();
    if (streamer_playlist) {
        plt_unref (streamer_playlist);
    }
    streamer_playlist = plt_get_curr ();
    // check if prev song is in this playlist
    if (-1 == str_get_idx_of (playlist_track)) {
        playlist_track = NULL;
    }

    playlist_t *plt = streamer_playlist;
    pl_playqueue_clear ();
    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        streamer_set_nextsong (-2, 1);
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        if (!playlist_track) {
            pl_unlock ();
            return streamer_move_to_nextsong (1);
        }
        else {
            playlist_track->played = 0;
            // find already played song with maximum shuffle rating below prev song
            int rating = playlist_track->shufflerating;
            playItem_t *pmax = NULL; // played maximum
            playItem_t *amax = NULL; // absolute maximum
            for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i != playlist_track && i->played && (!amax || i->shufflerating > amax->shufflerating)) {
                    amax = i;
                }
                if (i == playlist_track || i->shufflerating > rating || !i->played) {
                    continue;
                }
                if (!pmax || i->shufflerating > pmax->shufflerating) {
                    pmax = i;
                }
            }

            if (pmax && pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
                while (pmax && pmax->next[PL_MAIN] && pmax->next[PL_MAIN]->played && pmax->shufflerating == pmax->next[PL_MAIN]->shufflerating) {
                    pmax = pmax->next[PL_MAIN];
                }
            }

            playItem_t *it = pmax;
            if (!it) {
                // that means 1st in playlist, take amax
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                    if (!amax) {
                        plt_reshuffle (streamer_playlist, NULL, &amax);
                    }
                    it = amax;
                }
            }

            if (!it) {
                pl_unlock ();
                streamer_set_nextsong (-2, 1);
                return -1;
            }
            int r = str_get_idx_of (it);
            pl_unlock ();
            streamer_set_nextsong (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (playlist_track) {
            it = playlist_track->prev[PL_MAIN];
        }
        if (!it) {
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = plt->tail[PL_MAIN];
            }
        }
        if (!it) {
            pl_unlock ();
            streamer_set_nextsong (-2, 1);
            return -1;
        }
        int r = str_get_idx_of (it);
        pl_unlock ();
        streamer_set_nextsong (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_unlock ();
        int res = streamer_move_to_randomsong ();
        if (res == -1) {
            streamer_set_nextsong (-2, 1);
            trace ("streamer_move_to_randomsong error\n");
            return -1;
        }
        return 0;
    }
    pl_unlock ();
    return -1;
}

int
streamer_move_to_randomsong (void) {
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr ();
    }
    playlist_t *plt = streamer_playlist;
    int cnt = plt->count[PL_MAIN];
    if (!cnt) {
        trace ("empty playlist\n");
        return -1;
    }
    int curr = str_get_idx_of (playing_track);
    int r = rand () / (float)RAND_MAX * cnt;
    if (r == curr) {
        r++;
        if (r >= cnt) {
            r = 0;
        }
    }

    if (pl_get_order () == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
        plt_init_shuffle_albums (plt, r);
    }

    streamer_set_nextsong (r, 1);
    return 0;
}

// playlist must call that whenever item was removed
void
streamer_song_removed_notify (playItem_t *it) {
    if (!mutex) {
        return; // streamer is not running
    }
    if (it == playlist_track) {
        playlist_track = playlist_track->prev[PL_MAIN];
    }
}

// that must be called after last sample from str_playing_song was done reading
static int
streamer_set_current (playItem_t *it) {
    trace ("streamer_set_current %s\n", playing_track ? pl_find_meta (playing_track, ":URI") : "null");
    DB_output_t *output = plug_get_output ();
    int err = 0;
    int do_songstarted = 0;
    playItem_t *from, *to;
    // need to add refs here, because streamer_start_playback can destroy items
    from = playing_track;
    to = it;
    if (from) {
        pl_item_ref (from);
    }
    if (to) {
        pl_item_ref (to);
    }
    trace ("\033[0;35mstreamer_set_current from %p to %p\033[37;0m\n", from, it);
    if (!playing_track || output->state () == OUTPUT_STATE_STOPPED) {
        streamer_buffering = 1;
        trace ("\033[0;35mstreamer_start_playback[1] from %p to %p\033[37;0m\n", from, it);
        do_songstarted = 1;
        streamer_start_playback (from, it);
        bytes_until_next_song = -1;
    }

    trace ("streamer_set_current %p, buns=%d\n", it, bytes_until_next_song);
    mutex_lock (decodemutex);
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    mutex_unlock (decodemutex);
    if (!it) {
        goto success;
    }
    if (to) {
        trace ("draw before init: %p->%p, playing_track=%p, playlist_track=%p\n", from, to, playing_track, playlist_track);
        send_trackinfochanged (to);
    }
    if (from) {
        send_trackinfochanged (from);
    }
    char decoder_id[100] = "";
    char filetype[100] = "";
    pl_lock ();
    const char *dec = pl_find_meta (it, ":DECODER");
    if (dec) {
        strncpy (decoder_id, dec, sizeof (decoder_id));
    }
    const char *ft = pl_find_meta (it, ":FILETYPE");
    if (ft) {
        strncpy (filetype, ft, sizeof (filetype));
    }
    pl_unlock ();
    if (!decoder_id[0] && (!strcmp (filetype, "content") || !filetype[0])) {
        // try to get content-type
        mutex_lock (decodemutex);
        trace ("\033[0;34mopening file %s\033[37;0m\n", pl_find_meta (it, ":URI"));
        DB_FILE *fp = streamer_file = vfs_fopen (pl_find_meta (it, ":URI"));
        mutex_unlock (decodemutex);
        const char *plug = NULL;
        trace ("\033[0;34mgetting content-type\033[37;0m\n");
        if (!fp) {
            err = -1;
            goto error;
        }
        const char *ct = vfs_get_content_type (fp);
        if (!ct) {
            vfs_fclose (fp);
            fp = NULL;
            streamer_file = NULL;
            err = -1;
            goto error;
        }
        trace ("got content-type: %s\n", ct);
        if (!strcmp (ct, "audio/mpeg")) {
            plug = "stdmpg";
        }
        else if (!strcmp (ct, "application/ogg")) {
            plug = "stdogg";
        }
        else if (!strcmp (ct, "audio/aacp")) {
            plug = "aac";
        }
        else if (!strcmp (ct, "audio/aac")) {
            plug = "aac";
        }
        else if (!strcmp (ct, "audio/wma")) {
            plug = "ffmpeg";
        }
        else if (!strcmp (ct, "audio/x-mpegurl") || !strncmp (ct, "text/html", 9)) {
            // download playlist into temp file
            char *buf = NULL;
            int fd = -1;
            FILE *out = NULL;

            int size = vfs_fgetlength (fp);
            if (size <= 0) {
                size = MAX_PLAYLIST_DOWNLOAD_SIZE;
            }
            buf = malloc (size);
            if (!buf) {
                trace ("failed to alloc %d bytes for playlist buffer\n");
                goto m3u_error;
            }
            int rd = vfs_fread (buf, 1, size, fp);
            if (rd != size) {
                trace ("failed to download %d bytes (got %d bytes)\n", size, rd);
                goto m3u_error;
            }
            char tempfile[1000];
            const char *tmpdir = getenv ("TMPDIR");
            if (!tmpdir) {
                tmpdir = "/tmp";
            }
            snprintf (tempfile, sizeof (tempfile), "%s/ddbm3uXXXXXX", tmpdir);

            fd = mkstemp (tempfile);
            if (fd == -1) {
                trace ("failed to open temp file %s\n", tempfile);
                goto m3u_error;
            }
            out = fdopen (fd, "w+b");
            if (!out) {
                trace ("fdopen failed for %s\n", tempfile);
                goto m3u_error;
            }
            int rw = fwrite (buf, 1, size, out);
            if (rw != size) {
                trace ("failed to write %d bytes into file %s\n", size, tempfile);
                goto m3u_error;
            }
            fclose (out);
            fd = -1;
            out = NULL;

            // load playlist
            playlist_t *plt = plt_alloc ("temp");
            DB_playlist_t **plug = plug_get_playlist_list ();
            int p, e;
            DB_playItem_t *m3u = NULL;
            for (p = 0; plug[p]; p++) {
                if (plug[p]->load) {
                    m3u = plug[p]->load ((ddb_playlist_t *)plt, NULL, tempfile, NULL, NULL, NULL);
                    if (m3u) {
                        break;
                    }
                }
            }
            if (!m3u) {
                trace ("failed to load playlist from %s using any of the installed playlist plugins\n", tempfile);
                plt_free (plt);
                goto m3u_error;
            }

            // for every playlist uri: override stream uri with the one from playlist, and try to play it
            playItem_t *i = (playItem_t *)m3u;
            pl_item_ref (i);
            int res = -1;
            while (i) {
                pl_replace_meta (it, "!URI", pl_find_meta_raw (i, ":URI"));
                res = streamer_set_current (it);
                if (!res) {
                    pl_item_unref (i);
                    break;
                }
                playItem_t *next = pl_get_next (i, PL_MAIN);
                pl_item_unref (i);
                i = next;
            }
            pl_item_unref ((playItem_t*)m3u);
            plt_free (plt);
            if (res == 0) {
                // succeeded -- playing now
                if (from) {
                    pl_item_unref (from);
                }
                if (to) {
                    pl_item_unref (to);
                }
                return res;
            }
            unlink (tempfile);

m3u_error:
            if (buf) {
                free (buf);
            }
            if (out) {
                fclose (out);
            }
            else if (fd != -1) {
                close (fd);
            }
            goto error;
        }
        mutex_lock (decodemutex);
        streamer_file = NULL;
        vfs_fclose (fp);
        mutex_unlock (decodemutex);
        if (plug) {
            DB_decoder_t **decoders = plug_get_decoder_list ();
            // match by decoder
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, plug)) {
                    pl_replace_meta (it, "!DECODER", decoders[i]->plugin.id);
                    strncpy (decoder_id, decoders[i]->plugin.id, sizeof (decoder_id));
                    trace ("\033[0;34mfound plugin %s\033[37;0m\n", plug);
                    break;
                }
            }
        }
        else {
            trace ("\033[0;34mclosed file %s (bad or interrupted)\033[37;0m\n", pl_find_meta (it, ":URI"));
        }
    }
    playlist_track = it;
    if (decoder_id[0]) {
        DB_decoder_t *dec = NULL;
        dec = plug_get_decoder_for_id (decoder_id);
        if (!dec) {
            // find new decoder by file extension
            const char *fname = pl_find_meta (it, ":URI");
            const char *ext = strrchr (fname, '.');
            if (ext) {
                ext++;
                DB_decoder_t **decs = plug_get_decoder_list ();
                for (int i = 0; decs[i]; i++) {
                    const char **exts = decs[i]->exts;
                    if (exts) {
                        for (int j = 0; exts[j]; j++) {
                            if (!strcasecmp (exts[j], ext)) {
                                fprintf (stderr, "streamer: %s : changed decoder plugin to %s\n", fname, decs[i]->plugin.id);
                                pl_replace_meta (it, "!DECODER", decs[i]->plugin.id);
                                pl_replace_meta (it, "!FILETYPE", ext);
                                dec = decs[i];
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (dec) {
            trace ("\033[0;33minit decoder for %s (%s)\033[37;0m\n", pl_find_meta (it, ":URI"), decoder_id);
            mutex_lock (decodemutex);
            new_fileinfo = dec->open (0);
            mutex_unlock (decodemutex);
            if (new_fileinfo && dec->init (new_fileinfo, DB_PLAYITEM (it)) != 0) {
                trace ("\033[0;31mfailed to init decoder\033[37;0m\n");
                mutex_lock (decodemutex);
                dec->free (new_fileinfo);
                new_fileinfo = NULL;
                mutex_unlock (decodemutex);
//                goto error;
            }
        }

        if (!dec || !new_fileinfo) {
            it->played = 1;
            trace ("decoder->init returned %p\n", new_fileinfo);
            streamer_buffering = 0;
            if (playlist_track == it) {
                trace ("redraw track %d; playing_track=%p; playlist_track=%p\n", to, playing_track, playlist_track);
                send_trackinfochanged (to);
            }
            err = -1;
            goto error;
        }
        else {
            mutex_lock (decodemutex);
            if (streaming_track) {
                pl_item_unref (streaming_track);
            }
            streaming_track = it;
            if (streaming_track) {
                pl_item_ref (streaming_track);
                streamer_set_replaygain (streaming_track);
            }

            mutex_unlock (decodemutex);
            trace ("bps=%d, channels=%d, samplerate=%d\n", new_fileinfo->fmt.bps, new_fileinfo->fmt.channels, new_fileinfo->fmt.samplerate);
        }
    }
    else {
        trace ("no decoder in playitem!\n");
        it->played = 1;
        streamer_buffering = 0;
        if (playlist_track == it) {
            send_trackinfochanged (to);
        }
        if (from) {
            pl_item_unref (from);
        }
        if (to) {
            pl_item_unref (to);
        }
        return -1;
    }
success:
    mutex_lock (decodemutex);
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
    }
    if (new_fileinfo) {
        fileinfo = new_fileinfo;
        new_fileinfo = NULL;
    }
    mutex_unlock (decodemutex);
    if (do_songstarted && playing_track) {
        trace ("songstarted %s\n", playing_track ? pl_find_meta (playing_track, ":URI") : "null");
        playtime = 0;
        send_songstarted (playing_track);
    }
    send_trackinfochanged (to);

    trace ("\033[0;32mstr: %p (%s), ply: %p (%s)\033[37;0m\n", streaming_track, streaming_track ? pl_find_meta (streaming_track, ":URI") : "null", playing_track, playing_track ? pl_find_meta (playing_track, ":URI") : "null");

error:
    if (from) {
        pl_item_unref (from);
    }
    if (to) {
        pl_item_unref (to);
    }

    return err;
}

float
streamer_get_playpos (void) {
    float seek = seekpos;
    if (seek >= 0) {
        return seek;
    }
    return playpos;
}

void
streamer_set_bitrate (int bitrate) {
    if (bytes_until_next_song <= 0) { // prevent next track from resetting current playback bitrate
        last_bitrate = bitrate;
    }
}

int
streamer_get_apx_bitrate (void) {
    return avg_bitrate;
}

void
streamer_set_nextsong (int song, int pstate) {
    DB_output_t *output = plug_get_output ();
    trace ("streamer_set_nextsong %d %d\n", song, pstate);
    streamer_abort_files ();
    streamer_lock ();
    nextsong = song;
    nextsong_pstate = pstate;
    if (output->state () == OUTPUT_STATE_STOPPED) {
        if (pstate == 1) { // means user initiated this
            pl_lock ();
            if (streamer_playlist) {
                plt_unref (streamer_playlist);
            }
            streamer_playlist = plt_get_curr ();
            pl_unlock ();
        }
        // no sense to wait until end of previous song, reset buffer
        bytes_until_next_song = 0;
        playpos = 0;
        seekpos = -1;
    }
    streamer_unlock ();
}

void
streamer_set_generic_output_format (void) {
    output_format.bps = 16;
    output_format.is_float = 0;
    output_format.channels = 2;
    output_format.samplerate = 44100;
    output_format.channelmask = 3;
    streamer_set_output_format ();
}

void
streamer_set_seek (float pos) {
    seekpos = pos;
}

static void
streamer_start_new_song (void) {
    trace ("nextsong=%d (badsong=%d)\n", nextsong, badsong);
    streamer_lock ();
    DB_output_t *output = plug_get_output ();
    int sng = nextsong;
    int initsng = nextsong;
    int pstate = nextsong_pstate;
    nextsong = -1;
    streamer_unlock ();
    if (badsong == sng) {
        trace ("looped to bad file. stopping...\n");
        streamer_set_nextsong (-2, -2);
        badsong = -1;
        return;
    }
    playItem_t *try = str_get_for_idx (sng);
    if (!try) { // track is not in playlist
        trace ("track #%d is not in playlist; stopping playback\n", sng);
        output->stop ();

        mutex_lock (decodemutex);
        if (playing_track) {
            pl_item_unref (playing_track);
            playing_track = NULL;
        }
        if (streaming_track) {
            pl_item_unref (streaming_track);
            streaming_track = NULL;
        }
        mutex_unlock (decodemutex);

        send_trackchanged (NULL, NULL);
        return;
    }
    int ret = streamer_set_current (try);

    if (ret < 0) {
        trace ("\033[0;31mfailed to play track %s, skipping (current=%p/%p)...\033[37;0m\n", pl_find_meta (try, ":URI"), streaming_track, playlist_track);
        pl_item_unref (try);
        try = NULL;
        // remember bad song number in case of looping
        if (badsong == -1) {
            badsong = sng;
        }
        trace ("\033[0;34mbadsong=%d\033[37;0m\n", badsong);
        // try jump to next song
        if (nextsong == -1) {
            streamer_move_to_nextsong (0);
            trace ("streamer_move_to_nextsong switched to track %d\n", nextsong);
            usleep (50000);
        }
        else {
            trace ("nextsong changed from %d to %d by another thread, reinit\n", initsng, nextsong);
            badsong = -1;
        }
        return;
    }
    pl_item_unref (try);
    try = NULL;
    badsong = -1;
    trace ("pstate = %d\n", pstate);
    trace ("playback state = %d\n", output->state ());
    if (pstate == 0) {
        output->stop ();
    }
    else if (pstate == 1 || pstate == 3) {
        last_bitrate = -1;
        avg_bitrate = -1;
        if (output->state () != OUTPUT_STATE_PLAYING) {
            streamer_reset (1);
            if (fileinfo && memcmp (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                fprintf (stderr, "streamer_set_output_format %dbit %s %dch %dHz channelmask=%X\n", output_format.bps, output_format.is_float ? "float" : "int", output_format.channels, output_format.samplerate, output_format.channelmask);
                streamer_set_output_format ();
            }
            if (0 != output->play ()) {
                // give a chance to DSP plugins to convert format to something
                // supported
                streamer_set_generic_output_format ();
                if (0 != output->play ()) {
                    memset (&orig_output_format, 0, sizeof (orig_output_format));
                    fprintf (stderr, "streamer: failed to start playback (start track)\n");
                    streamer_set_nextsong (-2, 0);
                }
            }
        }
    }
    else if (pstate == 2) {
        if (output->state () == OUTPUT_STATE_STOPPED) {
            last_bitrate = -1;
            avg_bitrate = -1;
            streamer_reset (1);
            if (fileinfo && memcmp (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                formatchanged = 1;
            }
            // we need to start playback before we can pause it
            if (0 != output->play ()) {
                memset (&orig_output_format, 0, sizeof (orig_output_format));
                fprintf (stderr, "streamer: failed to start playback (start track)\n");
                streamer_set_nextsong (-2, 0);
            }
        }
        output->pause ();
    }
}

void
streamer_next (int bytesread) {
    streamer_lock ();
    bytes_until_next_song = streamer_ringbuf.remaining + bytesread;
    streamer_unlock ();
    if (conf_get_int ("playlist.stop_after_current", 0)) {
        streamer_buffering = 0;
        streamer_set_nextsong (-2, -2);
    }
    else {
        streamer_move_to_nextsong (0);
    }
}

void
streamer_thread (void *ctx) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-stream", 0, 0, 0, 0);
#endif

    while (!streaming_terminate) {
        struct timeval tm1;
        DB_output_t *output = plug_get_output ();
        gettimeofday (&tm1, NULL);
        if (nextsong >= 0) { // start streaming next song
            trace ("\033[0;34mnextsong=%d\033[37;0m\n", nextsong);
            if (playing_track) {
                trace ("sending songfinished to plugins [3]\n");
                send_songfinished (playing_track);
            }
            streamer_start_new_song ();
            // it's totally possible that song was switched
            // while streamer_set_current was running,
            // so we need to restart here
            continue;
        }
        else if (nextsong == -2 && (nextsong_pstate==0 || bytes_until_next_song == 0)) {
            streamer_lock ();
            playItem_t *from = playing_track;
            bytes_until_next_song = -1;
            trace ("nextsong=-2\n");
            nextsong = -1;
            output->stop ();
            if (playing_track) {
                trace ("sending songfinished to plugins [1]\n");
                send_songfinished (playing_track);
            }
            if (from) {
                pl_item_ref (from);
            }
            streamer_set_current (NULL);
            if (playing_track) {
                pl_item_unref (playing_track);
                playing_track = NULL;
            }
            send_trackchanged (from, NULL);
            if (from) {
                pl_item_unref (from);
            }
            streamer_unlock ();
            continue;
        }
        else if (output->state () == OUTPUT_STATE_STOPPED) {
            usleep (50000);
            continue;
        }

        if (bytes_until_next_song == 0) {
            streamer_lock ();
            if (!streaming_track) {
                // means last song was deleted during final drain
                nextsong = -1;
                output->stop ();
                streamer_set_current (NULL);
                streamer_unlock ();
                continue;
            }
            trace ("bytes_until_next_song=0, starting playback of new song\n");
            //playItem_t *from = playing_track;
            //playItem_t *to = streaming_track;
            trace ("sending songchanged\n");
            bytes_until_next_song = -1;
            // plugin will get pointer to str_playing_song
            if (playing_track) {
                trace ("sending songfinished to plugins [2]\n");
                send_songfinished (playing_track);
            }
            // copy streaming into playing
            trace ("\033[0;35mstreamer_start_playback[2] from %p to %p\033[37;0m\n", playing_track, streaming_track);
            streamer_start_playback (playing_track, streaming_track);
            trace ("songstarted %s\n", playing_track ? pl_find_meta (playing_track, ":URI") : "null");
            playtime = 0;
            send_songstarted (playing_track);
            last_bitrate = -1;
            avg_bitrate = -1;
            playlist_track = playing_track;
            playpos = 0;
            seekpos = -1;

            // don't switch if unchanged
            ddb_waveformat_t prevfmt;
            memcpy (&prevfmt, &output->fmt, sizeof (ddb_waveformat_t));
            if (memcmp (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                formatchanged = 1;
            }
            streamer_unlock ();
        }

        int seek = seekpos;
        if (seek >= 0) {
            playpos = seek;
            seekpos = -1;
            trace ("seeking to %f\n", seek);
            float pos = seek;

            if (playing_track != streaming_track) {
                trace ("streamer already switched to next track\n");
                
                // restart playing from new position
                
                mutex_lock (decodemutex);
                if(fileinfo) {
                    fileinfo->plugin->free (fileinfo);
                    fileinfo = NULL;
                    pl_item_unref (streaming_track);
                    streaming_track = NULL;
                }
                streaming_track = playing_track;
                if (streaming_track) {
                    pl_item_ref (streaming_track);
                    streamer_set_replaygain (streaming_track);
                }
                mutex_unlock (decodemutex);

                bytes_until_next_song = -1;
                streamer_buffering = 1;
                if (streaming_track) {
                    send_trackinfochanged (streaming_track);
                }

                mutex_lock (decodemutex);
                DB_decoder_t *dec = NULL;
                pl_lock ();
                const char *decoder_id = pl_find_meta (streaming_track, ":DECODER");
                dec = plug_get_decoder_for_id (decoder_id);
                pl_unlock ();
                if (dec) {
                    fileinfo = dec->open (0);
                    mutex_unlock (decodemutex);
                    if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (streaming_track)) != 0) {
                        mutex_lock (decodemutex);
                        dec->free (fileinfo);
                        fileinfo = NULL;
                        mutex_unlock (decodemutex);
                    }
                }
                else {
                    mutex_unlock (decodemutex);
                }

                if (!dec || !fileinfo) {
                    if (streaming_track) {
                        send_trackinfochanged (streaming_track);
                    }
                    trace ("failed to restart prev track on seek, trying to jump to next track\n");
                    streamer_move_to_nextsong (0);
                    trace ("streamer_move_to_nextsong switched to track %d\n", nextsong);
                    usleep (50000);
                    continue;
                }
            }

            bytes_until_next_song = -1;
            streamer_buffering = 1;
            if (streaming_track) {
                send_trackinfochanged (streaming_track);
            }
            float dur = pl_get_item_duration (playing_track);
            if (fileinfo && playing_track && dur > 0) {
                if (pos >= dur) {
                    output->stop ();
                    streamer_move_to_nextsong (1);
                    continue;
                }
                streamer_lock ();
                streamer_reset (1);
                if (fileinfo->plugin->seek (fileinfo, pos) >= 0) {
                    playpos = fileinfo->readpos;
                }
                last_bitrate = -1;
                avg_bitrate = -1;
                streamer_unlock();
            }
            ddb_event_playpos_t *ev = (ddb_event_playpos_t *)messagepump_event_alloc (DB_EV_SEEKED);
            ev->track = DB_PLAYITEM (playing_track);
            if (playing_track) {
                pl_item_ref (playing_track);
            }
            ev->playpos = playpos;
            messagepump_push_event ((ddb_event_t*)ev, 0, 0);
        }

        // read ahead at 2x speed of output samplerate, in 4k blocks
        int rate = output->fmt.samplerate;
        if (!rate) {
            trace ("str: got 0 output samplerate\n");
            usleep(20000);
            continue;
        }
        int channels = output->fmt.channels;
        int bytes_in_one_second = rate * (output->fmt.bps>>3) * channels;
        const int blocksize = MIN_BLOCK_SIZE;
        int alloc_time = 1000 / (bytes_in_one_second / blocksize);
        alloc_time /= 1.2;

        int skip = 0;
        if (bytes_until_next_song >= 0) {
            // check if streaming format differs from output
            if (memcmp(&fileinfo->fmt, &orig_output_format, sizeof (ddb_waveformat_t))) {
                skip = 1;
                streamer_buffering = 0;
            }
        }
        streamer_lock ();

        if (!formatchanged && !skip && streamer_ringbuf.remaining < (STREAM_BUFFER_SIZE-blocksize * MAX_DSP_RATIO)) {
            int sz = STREAM_BUFFER_SIZE - streamer_ringbuf.remaining;
            int minsize = blocksize;

            // speed up buffering when empty
            if (streamer_ringbuf.remaining < MAX_BLOCK_SIZE) {
                minsize *= 4;
                alloc_time *= 4;
            }
            sz = min (minsize, sz);
            assert ((sz&3) == 0);
            // buffer must be larger enough to accomodate resamplers/pitchers/...
            // FIXME: bounds checking
            streamer_unlock ();

            // ensure that size is possible with current format
            int samplesize = output->fmt.channels * (output->fmt.bps>>3);
            if (sz % samplesize) {
                sz -= (sz % samplesize);
            }

            int bytesread = 0;
            do {
                int prev_buns = bytes_until_next_song;
                int nb = streamer_read_async (readbuffer+bytesread,sz-bytesread);
                bytesread += nb;
                struct timeval tm2;
                gettimeofday (&tm2, NULL);
                int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
                if (ms >= alloc_time) {
                    break;
                }
                if (prev_buns != bytes_until_next_song) {
                    break;
                }
            } while (bytesread < sz-100);
            streamer_lock ();

            if (bytesread > 0) {
                ringbuf_write (&streamer_ringbuf, readbuffer, bytesread);
            }

            //trace ("fill: %d, read: %d, size=%d, blocksize=%d\n", streamer_ringbuf.remaining, bytesread, STREAM_BUFFER_SIZE, blocksize);
        }
        streamer_unlock ();
        if ((streamer_ringbuf.remaining > 128000 && streamer_buffering) || !streaming_track) {
            streamer_buffering = 0;
            if (streaming_track) {
                send_trackinfochanged (streaming_track);
            }
        }
        struct timeval tm2;
        gettimeofday (&tm2, NULL);

        int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
        //trace ("slept %dms (alloc=%dms, bytespersec=%d, chan=%d, blocksize=%d), fill: %d/%d (cursor=%d)\n", alloc_time-ms, alloc_time, bytes_in_one_second, output->fmt.channels, blocksize, streamer_ringbuf.remaining, STREAM_BUFFER_SIZE, streamer_ringbuf.cursor);
        alloc_time -= ms;
        if (!streamer_buffering && alloc_time > 0) {
            usleep (alloc_time * 1000);
        }
    }

    // stop streaming song
    mutex_lock (decodemutex);
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
    }
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    if (playing_track) {
        pl_item_unref (playing_track);
        playing_track = NULL;
    }
    mutex_unlock (decodemutex);
}

void
streamer_dsp_chain_free (ddb_dsp_context_t *dsp_chain) {
    while (dsp_chain) {
        ddb_dsp_context_t *next = dsp_chain->next;
        dsp_chain->plugin->close (dsp_chain);
        dsp_chain = next;
    }
}

ddb_dsp_context_t *
streamer_dsp_chain_load (const char *fname) {
    int err = 1;
    FILE *fp = fopen (fname, "rt");
    if (!fp) {
        return NULL;
    }

    char temp[100];
    ddb_dsp_context_t *chain = NULL;
    ddb_dsp_context_t *tail = NULL;
    for (;;) {
        // plugin enabled {
        int enabled = 0;
        int err = fscanf (fp, "%100s %d {\n", temp, &enabled);
        if (err == EOF) {
            break;
        }
        else if (2 != err) {
            fprintf (stderr, "error plugin name\n");
            goto error;
        }

        DB_dsp_t *plug = (DB_dsp_t *)deadbeef->plug_get_for_id (temp);
        if (!plug) {
            fprintf (stderr, "streamer_dsp_chain_load: plugin %s not found. preset will not be loaded\n", temp);
            goto error;
        }
        ddb_dsp_context_t *ctx = plug->open ();
        if (!ctx) {
            fprintf (stderr, "streamer_dsp_chain_load: failed to open ctxance of plugin %s\n", temp);
            goto error;
        }

        if (tail) {
            tail->next = ctx;
            tail = ctx;
        }
        else {
            tail = chain = ctx;
        }

        int n = 0;
        for (;;) {
            char value[1000];
            if (!fgets (temp, sizeof (temp), fp)) {
                fprintf (stderr, "streamer_dsp_chain_load: unexpected eof while reading plugin params\n");
                goto error;
            }
            if (!strcmp (temp, "}\n")) {
                break;
            }
            else if (1 != sscanf (temp, "\t%1000[^\n]\n", value)) {
                fprintf (stderr, "streamer_dsp_chain_load: error loading param %d\n", n);
                goto error;
            }
            if (plug->num_params) {
                plug->set_param (ctx, n, value);
            }
            n++;
        }
        ctx->enabled = enabled;
    }

    err = 0;
error:
    if (err) {
        fprintf (stderr, "streamer_dsp_chain_load: error loading %s\n", fname);
    }
    if (fp) {
        fclose (fp);
    }
    if (err && chain) {
        streamer_dsp_chain_free (chain);
        chain = NULL;
    }
    return chain;
}

int
streamer_dsp_chain_save (const char *fname, ddb_dsp_context_t *chain) {
    FILE *fp = fopen (fname, "w+t");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *ctx = chain;
    while (ctx) {
        fprintf (fp, "%s %d {\n", ctx->plugin->plugin.id, (int)ctx->enabled);
        if (ctx->plugin->num_params) {
            int n = ctx->plugin->num_params ();
            int i;
            for (i = 0; i < n; i++) {
                char v[1000];
                ctx->plugin->get_param (ctx, i, v, sizeof (v));
                fprintf (fp, "\t%s\n", v);
            }
        }
        fprintf (fp, "}\n");
        ctx = ctx->next;
    }

    fclose (fp);
    return 0;
}

void
streamer_dsp_postinit (void) {
    // note about EQ hack:
    // we 1st check if there's an EQ in dsp chain, and just use it
    // if not -- we add our own

    // eq plug
    if (eqplug) {
        ddb_dsp_context_t *p;

        for (p = dsp_chain; p; p = p->next) {
            if (!strcmp (p->plugin->plugin.id, "supereq")) {
                break;
            }
        }
        if (p) {
            eq = p;
        }
        else {
            eq = eqplug->open ();
            eq->enabled = 0;
            eq->next = dsp_chain;
            dsp_chain = eq;
        }

    }
    ddb_dsp_context_t *ctx = dsp_chain;
    while (ctx) {
        if (ctx->enabled) {
            break;
        }
        ctx = ctx->next;
    }
    if (!ctx && fileinfo) {
        if (memcmp (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
            memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
            memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
            formatchanged = 1;
        }
        dsp_on = 0;
    }
    else if (ctx) {
        dsp_on = 1;
        // set some very generic format, this will allow playback of weird
        // formats after fixing them with dsp plugins
        streamer_set_generic_output_format ();
    }
    else if (!ctx) {
        dsp_on = 0;
    }
}

void
streamer_dsp_refresh (void) {
    mutex_lock (decodemutex);
    streamer_dsp_postinit ();
    mutex_unlock (decodemutex);
}

void
streamer_dsp_init (void) {
    // load dsp chain from file
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    dsp_chain = streamer_dsp_chain_load (fname);
    if (!dsp_chain) {
        // first run, let's add resampler
        DB_dsp_t *src = (DB_dsp_t *)plug_get_for_id ("SRC");
        if (src) {
            ddb_dsp_context_t *inst = src->open ();
            inst->enabled = 1;
            src->set_param (inst, 0, "48000"); // samplerate
            src->set_param (inst, 1, "2"); // quality=SINC_FASTEST
            src->set_param (inst, 2, "1"); // auto
            inst->next = dsp_chain;
            dsp_chain = inst;
        }
    }

    eqplug = (DB_dsp_t *)plug_get_for_id ("supereq");
    streamer_dsp_postinit ();

    // load legacy eq settings from pre-0.5
    if (eq && eqplug && conf_find ("eq.", NULL)) {
        eq->enabled = deadbeef->conf_get_int ("eq.enable", 0);
        char s[50];

        // 0.4.4 was writing buggy settings, need to multiply by 2 to compensate
        conf_get_str ("eq.preamp", "0", s, sizeof (s));
        snprintf (s, sizeof (s), "%f", atof(s)*2);
        eqplug->set_param (eq, 0, s);
        for (int i = 0; i < 18; i++) {
            char key[100];
            snprintf (key, sizeof (key), "eq.band%d", i);
            conf_get_str (key, "0", s, sizeof (s));
            snprintf (s, sizeof (s), "%f", atof(s)*2);
            eqplug->set_param (eq, 1+i, s);
        }
        // delete obsolete settings
        conf_remove_items ("eq.");
    }
}

int
streamer_init (void) {
    streaming_terminate = 0;
#if WRITE_DUMP
    out = fopen ("out.raw", "w+b");
#endif
    mutex = mutex_create ();
    decodemutex = mutex_create ();

    ringbuf_init (&streamer_ringbuf, streambuffer, STREAM_BUFFER_SIZE);

    pl_set_order (conf_get_int ("playback.order", 0));

    streamer_dsp_init ();
    
    replaygain_set (conf_get_int ("replaygain_mode", 0), conf_get_int ("replaygain_scale", 1), conf_get_float ("replaygain_preamp", 0), conf_get_float ("global_preamp", 0));
    streamer_tid = thread_start (streamer_thread, NULL);
    return 0;
}

void
streamer_free (void) {
#if WRITE_DUMP
    fclose (out);
#endif
    streamer_abort_files ();
    streaming_terminate = 1;
    thread_join (streamer_tid);
    mutex_free (decodemutex);

    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    if (playing_track) {
        pl_item_unref (playing_track);
        playing_track = NULL;
    }
    if (playlist_track) {
        playlist_track = NULL;
    }
    if (streamer_playlist) {
        plt_unref (streamer_playlist);
        streamer_playlist = NULL;
    }

    decodemutex = 0;
    mutex_free (mutex);
    mutex = 0;

    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    streamer_dsp_chain_save (fname, dsp_chain);

    streamer_dsp_chain_free (dsp_chain);
    dsp_chain = NULL;

    eqplug = NULL;
    eq = NULL;
}

void
streamer_reset (int full) { // must be called when current song changes by external reasons
    if (!mutex) {
        fprintf (stderr, "ERROR: someone called streamer_reset after exit\n");
        return; // failsafe, in case someone calls streamer reset after deinit
    }
    if (full) {
        streamer_lock ();
        streamer_ringbuf.remaining = 0;
        streamer_unlock ();
    }

    // reset dsp
    ddb_dsp_context_t *dsp = dsp_chain;
    while (dsp) {
        if (dsp->plugin->reset) {
            dsp->plugin->reset (dsp);
        }
        dsp = dsp->next;
    }
}

static int
streamer_set_output_format (void) {
    DB_output_t *output = plug_get_output ();
    int playing = (output->state () == OUTPUT_STATE_PLAYING);

    fprintf (stderr, "streamer_set_output_format %dbit %s %dch %dHz channelmask=%X, bufferfill: %d\n", output_format.bps, output_format.is_float ? "float" : "int", output_format.channels, output_format.samplerate, output_format.channelmask, streamer_ringbuf.remaining);
    ddb_waveformat_t fmt;
    memcpy (&fmt, &output_format, sizeof (ddb_waveformat_t));
    if (autoconv_8_to_16) {
        if (fmt.bps == 8) {
            fmt.bps = 16;
        }
    }
    output->setformat (&fmt);
    streamer_buffering = 1;
    if (playing && output->state () != OUTPUT_STATE_PLAYING) {
        if (0 != output->play ()) {
            memset (&output_format, 0, sizeof (output_format));
            fprintf (stderr, "streamer: failed to start playback (streamer_read format change)\n");
            streamer_set_nextsong (-2, 0);
            return -1;
        }
    }
    return 0;
}

// decodes data and converts to current output format
// returns number of bytes been read
static int
streamer_read_async (char *bytes, int size) {
    DB_output_t *output = plug_get_output ();
    int initsize = size;
    int bytesread = 0;
    mutex_lock (decodemutex);
    if (!fileinfo) {
        // means there's nothing left to stream, so just do nothing
        mutex_unlock (decodemutex);
        return 0;
    }
    int is_eof = 0;

    if (fileinfo->fmt.samplerate != -1) {
        int outputsamplesize = output->fmt.channels * output->fmt.bps / 8;
        int inputsamplesize = fileinfo->fmt.channels * fileinfo->fmt.bps / 8;

        ddb_waveformat_t dspfmt;
        memcpy (&dspfmt, &fileinfo->fmt, sizeof (ddb_waveformat_t));
        dspfmt.bps = 32;
        dspfmt.is_float = 1;
        int can_bypass = 0;
        if (dsp_on) {
            // check if DSP can be passed through
            ddb_dsp_context_t *dsp = dsp_chain;
            while (dsp) {
                if (dsp->enabled) {
                    if (dsp->plugin->plugin.api_vminor >= 1) {
                        if (dsp->plugin->can_bypass && !dsp->plugin->can_bypass (dsp, &dspfmt)) {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }
                dsp = dsp->next;
            }
            if (!dsp) {
                can_bypass = 1;
            }
        }

        if (!memcmp (&fileinfo->fmt, &output->fmt, sizeof (ddb_waveformat_t)) && (!dsp_on || can_bypass)) {
            // pass through from input to output
            bytesread = fileinfo->plugin->read (fileinfo, bytes, size);

            if (bytesread != size) {
                is_eof = 1;
            }
        }
        else if (dsp_on) {
            // convert to float, pass through streamer DSP chain
            int dspsamplesize = fileinfo->fmt.channels * sizeof (float);
            int dsp_num_frames = size / (output->fmt.channels * output->fmt.bps / 8);

            char outbuf[dsp_num_frames * dspsamplesize];

            int inputsize = dsp_num_frames * inputsamplesize;
            char input[inputsize];

            // decode pcm
            int nb = fileinfo->plugin->read (fileinfo, input, inputsize);
            if (nb != inputsize) {
                is_eof = 1;
            }
            inputsize = nb;

            if (inputsize > 0) {
                // make *MAX_DSP_RATIO sized buffer for float data
                char tempbuf[inputsize/inputsamplesize * dspsamplesize * MAX_DSP_RATIO];

                // convert to float
                int tempsize = pcm_convert (&fileinfo->fmt, input, &dspfmt, tempbuf, inputsize);
                int nframes = inputsize / inputsamplesize;
                ddb_dsp_context_t *dsp = dsp_chain;
                float ratio = 1.f;
                int maxframes = sizeof (tempbuf) / dspsamplesize;
                while (dsp) {
                    if (dsp->enabled) {
                        float r = 1;
                        nframes = dsp->plugin->process (dsp, (float *)tempbuf, nframes, maxframes, &dspfmt, &r);
                        ratio *= r;
                    }
                    dsp = dsp->next;
                }
                dsp_ratio = ratio;

                ddb_waveformat_t outfmt;
                // preserve sampleformat, but take channels, samplerate
                outfmt.bps = fileinfo->fmt.bps;
                outfmt.is_float = fileinfo->fmt.is_float;
                // channelmask from dsp chain
                outfmt.channels = dspfmt.channels;
                outfmt.samplerate = dspfmt.samplerate;
                outfmt.channelmask = dspfmt.channelmask;
                outfmt.is_bigendian = fileinfo->fmt.is_bigendian;
                if (memcmp (&output_format, &outfmt, sizeof (ddb_waveformat_t)) && bytes_until_next_song <= 0) {
                    memcpy (&output_format, &outfmt, sizeof (ddb_waveformat_t));
                    streamer_set_output_format ();
                }

                //printf ("convert from %dbit %s %dch %dHz channelmask=%X to %dbit %s %dch %dHz channelmask=%X\n", dspfmt.bps, dspfmt.is_float ? "float" : "int", dspfmt.channels, dspfmt.samplerate, dspfmt.channelmask, output->fmt.bps, output->fmt.is_float ? "float" : "int", output->fmt.channels, output->fmt.samplerate, output->fmt.channelmask);

                int n = pcm_convert (&dspfmt, tempbuf, &output->fmt, bytes, nframes * dspfmt.channels * sizeof (float));

                bytesread = n;
            }
        }
        else {
            // convert from input fmt to output fmt
            int inputsize = size/outputsamplesize*inputsamplesize;
            char input[inputsize];
            int nb = fileinfo->plugin->read (fileinfo, input, inputsize);
            if (nb != inputsize) {
                bytesread = nb;
                is_eof = 1;
            }
            inputsize = nb;
            bytesread = pcm_convert (&fileinfo->fmt, input, &output->fmt, bytes, inputsize);
        }
#if WRITE_DUMP
        if (bytesread) {
            fwrite (bytes, 1, bytesread, out);
        }
#endif

        replaygain_apply (&output->fmt, streaming_track, bytes, bytesread);
    }
    mutex_unlock (decodemutex);
    if (!is_eof) {
        return bytesread;
    }
    else  {
        // that means EOF
        // trace ("streamer: EOF! buns: %d, bytesread: %d, buffering: %d, bufferfill: %d\n", bytes_until_next_song, bytesread, streamer_buffering, streamer_ringbuf.remaining);

        // in case of decoder error, or EOF while buffering - switch to next song instantly
        if (bytesread < 0 || (bytes_until_next_song >= 0 && streamer_buffering && bytesread == 0) || bytes_until_next_song < 0) {
            trace ("finished streaming song, queueing next (%d %d %d %d)\n", bytesread, bytes_until_next_song, streamer_buffering, nextsong_pstate);
            streamer_next (bytesread);
        }
    }
    return bytesread;
}

int
streamer_read (char *bytes, int size) {
#if 0
    struct timeval tm1;
    gettimeofday (&tm1, NULL);
#endif
    if (!playing_track) {
        return -1;
    }
    DB_output_t *output = plug_get_output ();
    if (formatchanged && bytes_until_next_song <= 0) {
        streamer_set_output_format ();
        formatchanged = 0;
    }
    streamer_lock ();
    int sz = min (size, streamer_ringbuf.remaining);
    if (sz) {
        ringbuf_read (&streamer_ringbuf, bytes, sz);
        playpos += (float)sz/output->fmt.samplerate/((output->fmt.bps>>3)*output->fmt.channels) * dsp_ratio;
        playtime += (float)sz/output->fmt.samplerate/((output->fmt.bps>>3)*output->fmt.channels);
        if (bytes_until_next_song > 0) {
            bytes_until_next_song -= sz;
            if (bytes_until_next_song < 0) {
                bytes_until_next_song = 0;
            }
        }
    }
    streamer_unlock ();

    // approximate bitrate
    if (last_bitrate != -1) {
        if (avg_bitrate == -1) {
            avg_bitrate = last_bitrate;
        }
        else {
            if (avg_bitrate < last_bitrate) {
                avg_bitrate += 5;
                if (avg_bitrate > last_bitrate) {
                    avg_bitrate = last_bitrate;
                }
            }
            else if (avg_bitrate > last_bitrate) {
                avg_bitrate -= 5;
                if (avg_bitrate < last_bitrate) {
                    avg_bitrate = last_bitrate;
                }
            }
        }
//        printf ("apx bitrate: %d (last %d)\n", avg_bitrate, last_bitrate);
    }
    else {
        avg_bitrate = -1;
    }

#if 0
    struct timeval tm2;
    gettimeofday (&tm2, NULL);

    int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    printf ("streamer_read took %d ms\n", ms);
#endif

    if (!output->has_volume) {
        char *stream = bytes;
        int bytesread = sz;
        if (output->fmt.bps == 16) {
            int16_t ivolume = volume_get_amp () * 1000;
            for (int i = 0; i < bytesread/2; i++) {
                int16_t sample = *((int16_t*)stream);
                *((int16_t*)stream) = (int16_t)(((int32_t)sample) * ivolume / 1000);
                stream += 2;
            }
        }
        else if (output->fmt.bps == 8) {
            int16_t ivolume = volume_get_amp () * 255;
            for (int i = 0; i < bytesread; i++) {
                *stream = (int8_t)(((int32_t)(*stream)) * ivolume / 1000);
                stream++;
            }
        }
        else if (output->fmt.bps == 24) {
            int16_t ivolume = volume_get_amp () * 1000;
            for (int i = 0; i < bytesread/3; i++) {
                int32_t sample = ((unsigned char)stream[0]) | ((unsigned char)stream[1]<<8) | (stream[2]<<16);
                int32_t newsample = (int64_t)sample * ivolume / 1000;
                stream[0] = (newsample&0x0000ff);
                stream[1] = (newsample&0x00ff00)>>8;
                stream[2] = (newsample&0xff0000)>>16;
                stream += 3;
            }
        }
        else if (output->fmt.bps == 32 && !output->fmt.is_float) {
            int16_t ivolume = volume_get_amp () * 1000;
            for (int i = 0; i < bytesread/4; i++) {
                int32_t sample = *((int32_t*)stream);
                int32_t newsample = (int64_t)sample * ivolume / 1000;
                *((int32_t*)stream) = newsample;
                stream += 4;
            }
        }
        else if (output->fmt.bps == 32 && output->fmt.is_float) {
            float fvolume = volume_get_amp ();
            for (int i = 0; i < bytesread/4; i++) {
                *((float*)stream) = (*((float*)stream)) * fvolume;
                stream += 4;
            }
        }
    }

    return sz;
}

int
streamer_get_fill (void) {
    return streamer_ringbuf.remaining;
}

int
streamer_ok_to_read (int len) {
    DB_output_t *output = plug_get_output ();
    if (formatchanged && bytes_until_next_song <= 0) {
        streamer_set_output_format ();
        formatchanged = 0;
    }
    if (len >= 0 && (bytes_until_next_song > 0 || streamer_ringbuf.remaining >= (len*2))) {
        return 1;
    }
    else {
        return 1-streamer_buffering;
    }
    return 0;
}

void
streamer_configchanged (void) {
    replaygain_set (conf_get_int ("replaygain_mode", 0), conf_get_int ("replaygain_scale", 1), conf_get_float ("replaygain_preamp", 0), conf_get_float ("global_preamp", 0));
    pl_set_order (conf_get_int ("playback.order", 0));
    if (playing_track) {
        playing_track->played = 1;
    }
    int conf_autoconv_8_to_16 = conf_get_int ("streamer.8_to_16", 1);
    if (conf_autoconv_8_to_16 != autoconv_8_to_16) {
        autoconv_8_to_16 = conf_autoconv_8_to_16;
        formatchanged = 1;
        streamer_reset (1);
    }
}

void
streamer_play_current_track (void) {
    playlist_t *plt = plt_get_curr ();
    DB_output_t *output = plug_get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED && playing_track) {
        // unpause currently paused track
        output->unpause ();
        messagepump_push (DB_EV_PAUSED, 0, 0, 0);
    }
    else if (plt->current_row[PL_MAIN] != -1) {
        // play currently selected track in current playlist
        output->stop ();
        // get next song in queue
        int idx = -1;
        playItem_t *next = pl_playqueue_getnext ();
        if (next) {
            idx = str_get_idx_of (next);
            pl_playqueue_pop ();
            pl_item_unref (next);
        }
        else {
            idx = plt->current_row[PL_MAIN];
        }

        streamer_set_nextsong (idx, 1);
        pl_lock ();
        if (streamer_playlist) {
            plt_unref (streamer_playlist);
        }
        streamer_playlist = plt;
        pl_unlock ();
        return;
    }
    else {
        output->stop ();
        streamer_move_to_nextsong (1);
    }
    if (plt) {
        plt_unref (plt);
    }
}

struct DB_fileinfo_s *
streamer_get_current_fileinfo (void) {
    return fileinfo;
}

void
streamer_set_current_playlist (int plt) {
    pl_lock ();
    if (streamer_playlist) {
        plt_unref (streamer_playlist);
    }
    streamer_playlist = plt_get_for_idx (plt);
    pl_unlock ();
}

int
streamer_get_current_playlist (void) {
    pl_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr ();
    }
    int idx = plt_get_idx_of (streamer_playlist);
    pl_unlock ();
    return idx;
}

void
streamer_notify_playlist_deleted (playlist_t *plt) {
    // this is only called from playlist code, no lock required
    if (plt == streamer_playlist) {
        plt_unref (streamer_playlist);
        streamer_playlist = NULL;
    }
}

ddb_dsp_context_t *
streamer_get_dsp_chain (void) {
    return dsp_chain;
}

static ddb_dsp_context_t *
dsp_clone (ddb_dsp_context_t *from) {
    ddb_dsp_context_t *dsp = from->plugin->open ();
    char param[2000];
    if (from->plugin->num_params) {
        int n = from->plugin->num_params ();
        for (int i = 0; i < n; i++) {
            from->plugin->get_param (from, i, param, sizeof (param));
            dsp->plugin->set_param (dsp, i, param);
        }
    }
    dsp->enabled = from->enabled;
    return dsp;
}

void
streamer_set_dsp_chain (ddb_dsp_context_t *chain) {
    mutex_lock (decodemutex);
    streamer_dsp_chain_free (dsp_chain);

    dsp_chain = NULL;
    eq = NULL;

    ddb_dsp_context_t *tail = NULL;
    while (chain) {
        ddb_dsp_context_t *new = dsp_clone (chain);
        if (tail) {
            tail->next = new;
            tail = new;
        }
        else {
            dsp_chain = tail = new;
        }
        chain = chain->next;
    }

    streamer_dsp_postinit ();
    if (fileinfo) {
        memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
        memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
        formatchanged = 1;
    }

    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    streamer_dsp_chain_save (fname, dsp_chain);
    streamer_reset (1);

    mutex_unlock (decodemutex);
    DB_output_t *output = plug_get_output ();
    if (playing_track && output->state () != OUTPUT_STATE_STOPPED) {
        streamer_set_seek (playpos);
    }
}

void
streamer_get_output_format (ddb_waveformat_t *fmt) {
    memcpy (fmt, &output_format, sizeof (ddb_waveformat_t));
}

void
streamer_notify_order_changed (int prev_order, int new_order) {
    if (prev_order != PLAYBACK_ORDER_SHUFFLE_ALBUMS && new_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
        streamer_lock ();
        playItem_t *curr = playing_track;
        if (curr) {

            const char *alb = pl_find_meta_raw (curr, "album");
            const char *art = pl_find_meta_raw (curr, "artist");
            playItem_t *next = curr->prev[PL_MAIN];
            while (next) {
                if (alb == pl_find_meta_raw (next, "album") && art == pl_find_meta_raw (next, "artist")) {
                    next->played = 1;
                    next = next->prev[PL_MAIN];
                }
                else {
                    break;
                }
            }
        }
        streamer_unlock ();
    }
}
