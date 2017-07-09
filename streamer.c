/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  streamer implementation

  Copyright (C) 2009-2017 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
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
#include "fastftoi.h"
#include "volume.h"
#include "vfs.h"
#include "premix.h"
#include "fft.h"
#include "handler.h"
#include "plugins/libparser/parser.h"
#include "strdupa.h"
#include "playqueue.h"
#include "streamreader.h"
#include "dsp.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#undef trace
#define trace(fmt,...)

//#define WRITE_DUMP 1
//#define DETECT_PL_LOCK_RC 1

#if WRITE_DUMP
FILE *out;
#endif

#define MAX_PLAYLIST_DOWNLOAD_SIZE 25000
#define STREAMER_HINTS (DDB_DECODER_HINT_NEED_BITRATE|DDB_DECODER_HINT_CAN_LOOP)

static intptr_t streamer_tid;

static int autoconv_8_to_16 = 1;

static int autoconv_16_to_24 = 0;

static int trace_bufferfill = 0;

static int stop_after_current = 0;
static int stop_after_album = 0;

static int conf_streamer_nosleep = 0;

static int streaming_terminate;

// buffer up to 3 seconds at 44100Hz stereo
#define STREAM_BUFFER_SIZE 0x80000 // slightly more than 3 seconds of 44100 stereo


static uintptr_t mutex;
static uintptr_t wdl_mutex; // wavedata listener

static int autoplay = 0; // set to 1 if streamer should call output->play on track change (e.g. after manual track change)

static float last_seekpos = -1;

static float playpos = 0; // play position of current song
static int avg_bitrate = -1; // avg bitrate of current song

static int streamer_is_buffering;

static playlist_t *streamer_playlist;
static playItem_t *playing_track;
static playItem_t *buffering_track;
static float playtime; // total playtime of playing track
static time_t started_timestamp; // result of calling time(NULL)
static playItem_t *streaming_track;
static playItem_t *last_played; // this is the last track that was played, should avoid setting this to NULL

static ddb_waveformat_t prev_output_format; // last format that was sent to output via streamer_set_output_format
static ddb_waveformat_t last_block_fmt; // input file format corresponding to the current output

static int formatchanged;

static DB_fileinfo_t *fileinfo;
static DB_FILE *fileinfo_file;
static DB_fileinfo_t *new_fileinfo;
static DB_FILE *new_fileinfo_file;

// to allow interruption of stall file requests
static DB_FILE *streamer_file;

// for vis plugins
static float freq_data[DDB_FREQ_BANDS * DDB_FREQ_MAX_CHANNELS];
static float audio_data[DDB_FREQ_BANDS * 2 * DDB_FREQ_MAX_CHANNELS];
static int audio_data_fill = 0;
static int audio_data_channels = 0;

// message queue
static struct handler_s *handler;

// visualization stuff
typedef struct wavedata_listener_s {
    void *ctx;
    void (*callback)(void *ctx, ddb_audio_data_t *data);
    struct wavedata_listener_s *next;
} wavedata_listener_t;

static wavedata_listener_t *waveform_listeners;
static wavedata_listener_t *spectrum_listeners;

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
play_index (int idx, int startpaused);

static void
play_current (void);

static void
play_next (void);

static void
streamer_set_current_playlist_real (int plt);

static int
stream_track (playItem_t *track, int startpaused);

static void
_handle_playback_stopped (void);

static void
streamer_abort_files (void) {
    DB_FILE *file = fileinfo_file;
    DB_FILE *newfile = new_fileinfo_file;
    DB_FILE *strfile = streamer_file;
    trace ("\033[0;33mstreamer_abort_files\033[37;0m\n");
    trace ("%p %p %p\n", file, newfile, strfile);

    if (file) {
        deadbeef->fabort (file);
    }
    if (newfile) {
        deadbeef->fabort (newfile);
    }
    if (strfile) {
        deadbeef->fabort (strfile);
    }

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

static void
set_last_played (playItem_t *track) {
    if (last_played) {
        pl_item_unref (last_played);
    }
    last_played = track;
    if (last_played) {
        pl_item_ref (last_played);
    }
}

static void
streamer_start_playback (playItem_t *from, playItem_t *it) {
    if (from) {
        pl_item_ref (from);
    }
    if (it) {
        pl_item_ref (it);
    }

    streamer_set_playing_track (it);
    if (playing_track) {
        playing_track->played = 1;

        set_last_played (playing_track);

        playItem_t *qnext = playqueue_getnext();
        if (qnext == playing_track) {
            playqueue_pop ();
        }
        if (qnext) {
            pl_item_unref (qnext);
        }

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
    playItem_t *it = buffering_track ? buffering_track : playing_track;
    if (it) {
        pl_item_ref (it);
    }
    return it;
}

playItem_t *
streamer_get_buffering_track (void) {
    playItem_t *it = buffering_track;
    if (it) {
        pl_item_ref (it);
    }
    return it;
}

int
str_get_idx_of (playItem_t *it) {
    pl_lock ();
    if (!streamer_playlist) {
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
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
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
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

static int
stop_after_album_check (playItem_t *cur, playItem_t *next) {
    if (!stop_after_album) {
        return 0;
    }

    if (!cur) {
        return 0;
    }

    if (!next) {
        stream_track (NULL, 0);
        if (conf_get_int ("playlist.stop_after_album_reset", 0)) {
            conf_set_int ("playlist.stop_after_album", 0);
            stop_after_album = 0;
            deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
        }
        return 1;
    }

    const char *cur_album = pl_find_meta_raw (cur, "album");
    const char *next_album = pl_find_meta_raw (next, "album");

    const char *keys[] = {
        "band",
        "album artist",
        "albumartist",
        "artist",
        NULL
    };

    const char *cur_artist = NULL;
    const char *next_artist = NULL;
    for (int i = 0; keys[i]; i++) {
        if (!cur_artist) {
            cur_artist = pl_find_meta_raw (cur, keys[i]);
        }
        if (!next_artist) {
            next_artist = pl_find_meta_raw (next, keys[i]);
        }
        if (cur_artist && next_artist) {
            break;
        }
    }

    if (cur_artist == next_artist && cur_album == next_album) {
        return 0;
    }

    stream_track (NULL, 0);
    if (conf_get_int ("playlist.stop_after_album_reset", 0)) {
        conf_set_int ("playlist.stop_after_album", 0);
        stop_after_album = 0;
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }

    return 1;
}

static playItem_t *
get_random_track (void) {
    if (!streamer_playlist) {
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
    }
    playlist_t *plt = streamer_playlist;
    int cnt = plt->count[PL_MAIN];
    if (!cnt) {
        trace ("empty playlist\n");
        return NULL;
    }
    int curr = str_get_idx_of (streaming_track);
    int r = rand () / (float)RAND_MAX * cnt;
    if (r == curr) {
        r++;
        if (r >= cnt) {
            r = 0;
        }
    }

    return plt_get_item_for_idx (plt, r, PL_MAIN);
}

static playItem_t *
get_next_track (playItem_t *curr) {
    pl_lock ();
    if (!streamer_playlist) {
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
    }

    while (playqueue_getcount ()) {
        trace ("playqueue_getnext\n");
        playItem_t *it = playqueue_getnext ();
        if (it) {
            pl_unlock ();
            return it; // from playqueue
        }
    }

    playlist_t *plt = streamer_playlist;
    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        return NULL; // empty playlist
    }

    int pl_order = pl_get_order ();

    int pl_loop_mode = conf_get_int ("playback.loop", 0);

    if (pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
        int r = str_get_idx_of (curr);
        pl_unlock ();
        if (r == -1) {
            return NULL; // track is not in current playlist
        }
        else {
            pl_item_ref (curr);
            return curr;
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
            if (!it) { // nothing found after reshuffle
                pl_unlock ();
                return NULL;
            }
            // plt_reshuffle doesn't add ref
            pl_item_ref (it);

            pl_unlock ();
            return it;
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
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) { // loop
                    trace ("all songs played! reshuffle\n");
                    plt_reshuffle (streamer_playlist, &it, NULL);
                }
            }
            if (!it) {
                playItem_t *temp;
                plt_reshuffle (streamer_playlist, &temp, NULL);
                pl_unlock ();
                return NULL;
            }
            // plt_reshuffle doesn't add ref
            pl_item_ref (it);
            pl_unlock ();
            return it;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (curr) {
            it = curr->next[PL_MAIN];
        }
        if (!it) {
            trace ("streamer_move_nextsong: was last track\n");
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = plt->head[PL_MAIN];
            }
            else {
                pl_unlock ();
                return NULL;
            }
        }
        if (!it) {
            pl_unlock ();
            return NULL;
        }
        pl_item_ref (it);
        pl_unlock ();
        return it;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_unlock ();
        return get_random_track ();
    }
    pl_unlock ();
    return NULL;
}

static playItem_t *
get_prev_track (playItem_t *curr) {
    pl_lock ();
    playlist_t *plt = plt_get_curr ();
    streamer_set_streamer_playlist (plt);
    plt_unref (plt);
    // check if prev song is in this playlist
    if (-1 == str_get_idx_of (curr)) {
        curr = NULL;
    }

    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        return NULL;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        if (!curr) {
            playItem_t *it = plt->head[PL_MAIN];
            pl_item_ref(it);
            pl_unlock ();
            return it;
        }
        else {
            curr->played = 0;
            // find already played song with maximum shuffle rating below prev song
            int rating = curr->shufflerating;
            playItem_t *pmax = NULL; // played maximum
            playItem_t *amax = NULL; // absolute maximum
            for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i != curr && i->played && (!amax || i->shufflerating > amax->shufflerating)) {
                    amax = i;
                }
                if (i == curr || i->shufflerating > rating || !i->played) {
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
                return NULL;
            }
            pl_item_ref (it);
            pl_unlock ();
            return it;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (curr) {
            it = curr->prev[PL_MAIN];
        }
        if (!it) {
            pl_unlock ();
            return NULL;
        }
        pl_item_ref(it);
        pl_unlock ();
        return it;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_unlock();
        return get_random_track();
    }
    pl_unlock ();
    return NULL;
}

int
streamer_move_to_nextsong (int r) {
    if (r) {
        streamer_abort_files ();
    }
    handler_push (handler, STR_EV_NEXT, 0, r, 0);
    return 0;
}

int
streamer_move_to_prevsong (int r) {
    if (r) {
        streamer_abort_files ();
    }
    handler_push (handler, STR_EV_PREV, 0, r, 0);
    return 0;
}

int
streamer_move_to_randomsong (int r) {
    if (r) {
        streamer_abort_files ();
    }
    handler_push (handler, STR_EV_RAND, 0, r, 0);
    return 0;
}

// playlist must call that whenever item was removed
void
streamer_song_removed_notify (playItem_t *it) {
    if (!mutex) {
        return; // streamer is not running
    }
    if (it == last_played) {
        set_last_played (last_played->prev[PL_MAIN]);
    }
}

#define CTMAP_MAX_PLUGINS 5

typedef struct ctmap_s {
    char *ct;
    char *plugins[CTMAP_MAX_PLUGINS];
    struct ctmap_s *next;
} ctmap_t;

static ctmap_t *streamer_ctmap;
static char conf_network_ctmapping[2048];
static uintptr_t ctmap_mutex;

static void
ctmap_init_mutex (void) {
    ctmap_mutex = mutex_create ();
}

static void
ctmap_free_mutex (void) {
    if (ctmap_mutex) {
        mutex_free (ctmap_mutex);
        ctmap_mutex = 0;
    }
}

static void
ctmap_lock (void) {
    mutex_lock (ctmap_mutex);
}

static void
ctmap_unlock (void) {
    mutex_unlock (ctmap_mutex);
}

static void
ctmap_free (void) {
    while (streamer_ctmap) {
        ctmap_t *ct = streamer_ctmap;
        free (ct->ct);
        for (int i = 0; ct->plugins[i]; i++) {
            free (ct->plugins[i]);
        }
        streamer_ctmap = ct->next;
        free (ct);
    }
}

static void
ctmap_init (void) {
    ctmap_free ();
    char *mapstr = conf_network_ctmapping;

    const char *p = mapstr;
    char t[MAX_TOKEN];
    char plugins[MAX_TOKEN*5];

    ctmap_t *tail = NULL;

    for (;;) {
        p = gettoken (p, t);

        if (!p) {
            break;
        }

        ctmap_t *ctmap = malloc (sizeof (ctmap_t));
        memset (ctmap, 0, sizeof (ctmap_t));
        ctmap->ct = strdup (t);

        int n = 0;

        p = gettoken (p, t);
        if (!p || strcmp (t, "{")) {
            free (ctmap->ct);
            free (ctmap);
            break;
        }

        plugins[0] = 0;
        for (;;) {
            p = gettoken (p, t);
            if (!p || !strcmp (t, "}") || n >= CTMAP_MAX_PLUGINS-1) {
                break;
            }

            ctmap->plugins[n++] = strdup (t);
        }
        ctmap->plugins[n] = NULL;
        if (tail) {
            tail->next = ctmap;
        }
        tail = ctmap;
        if (!streamer_ctmap) {
            streamer_ctmap = ctmap;
        }
    }
}

static int
is_remote_stream (playItem_t *it) {
    int remote = 0;
    pl_lock ();
    const char *uri = pl_find_meta (it, ":URI");
    if (uri && !plug_is_local_file (uri)) {
        remote = 1;
    }
    pl_unlock ();
    return remote;
}

static DB_fileinfo_t *dec_open (DB_decoder_t *dec, uint32_t hints, playItem_t *it) {
    if (dec->plugin.api_vminor >= 7 && dec->open2) {
        DB_fileinfo_t *fi = dec->open2 (hints, DB_PLAYITEM (it));
        return fi;
    }
    return dec->open (hints);
}

static playItem_t *first_failed_track;

static void
streamer_play_failed (playItem_t *failed_track) {
    streamer_lock();
    if (!first_failed_track) {
        first_failed_track = failed_track;
        if (first_failed_track) {
            pl_item_ref (first_failed_track);
        }
    }
    else if (!failed_track) { // reset fail check
        if (first_failed_track) {
            pl_item_unref (first_failed_track);
            first_failed_track = NULL;
        }
    }

    if (failed_track) {
        pl_lock ();
        trace_err ("Failed to play track: %s\n", pl_find_meta(failed_track, ":URI"));
        pl_unlock ();
        set_last_played (failed_track);
        handler_push (handler, STR_EV_NEXT, 0, 0, 0);
    }
    streamer_unlock();
}

static int
stream_track (playItem_t *it, int startpaused) {
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
        fileinfo_file = NULL;
    }
    trace ("stream_track %s\n", playing_track ? pl_find_meta (playing_track, ":URI") : "null");
    int err = 0;
    playItem_t *from = NULL;
    playItem_t *to = NULL;

    if (first_failed_track && first_failed_track == it) {
        streamer_play_failed (NULL); // looped to the first failed track
        goto error;
    }

    // need to add refs here, because streamer_start_playback can destroy items
    from = playing_track;
    to = it;
    if (from) {
        pl_item_ref (from);
    }
    if (to) {
        pl_item_ref (to);
    }

    streamer_lock ();
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    streamer_unlock ();

    int paused_stream = 0;
    if (it && startpaused) {
        paused_stream = is_remote_stream (it);
    }


    if (!it || paused_stream) {
        goto success;
    }

    char decoder_id[100] = "";
    char filetype[100] = "";
    pl_lock ();
    const char *dec = pl_find_meta (it, ":DECODER");
    if (dec) {
        strncpy (decoder_id, dec, sizeof (decoder_id));
    }

    if (!decoder_id[0]) {
        // some decoders set filetype override,
        // but the override is invalid when decoder is not set.
        // reset to default here, so that tracks become playable after failures
        pl_delete_meta(it, "!FILETYPE");
    }

    const char *ft = pl_find_meta (it, ":FILETYPE");
    if (ft) {
        strncpy (filetype, ft, sizeof (filetype));
    }
    pl_unlock ();
    char *plugs[CTMAP_MAX_PLUGINS] = {NULL};
    if (!decoder_id[0] && (!strcmp (filetype, "content") || !filetype[0])) {
        // try to get content-type
        trace ("\033[0;34mopening file %s\033[37;0m\n", pl_find_meta (it, ":URI"));
        pl_lock ();
        char *uri = strdupa (pl_find_meta (it, ":URI"));
        pl_unlock ();
        DB_FILE *fp = streamer_file = vfs_fopen (uri);
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
        char *cct = strdupa (ct);
        char *sc = strchr (cct, ';');
        if (sc) {
            *sc = 0;
        }

        ctmap_lock ();
        ctmap_t *ctmap = streamer_ctmap;
        while (ctmap) {
            if (!strcmp (cct, ctmap->ct)) {
                break;
            }
            ctmap = ctmap->next;
        }
        if (ctmap) {
            int i;
            for (i = 0; ctmap->plugins[i]; i++) {
                plugs[i] = strdupa (ctmap->plugins[i]);
            }
            plugs[i] = NULL;
        }
        ctmap_unlock ();

        if (!plugs[0] && (!strcmp (cct, "audio/x-mpegurl") || !strncmp (cct, "text/html", 9) || !strncmp (cct, "audio/x-scpls", 13) || !strncmp (cct, "application/octet-stream", 9))) {
            // download playlist into temp file
            trace ("downloading playlist into temp file...\n");
            char *buf = NULL;
            int fd = -1;
            FILE *out = NULL;
            char tempfile[1000] = "";

            int64_t size = vfs_fgetlength (fp);
            if (size <= 0) {
                size = MAX_PLAYLIST_DOWNLOAD_SIZE;
            }
            buf = malloc (size);
            if (!buf) {
                trace ("failed to alloc %d bytes for playlist buffer\n", size);
                goto m3u_error;
            }
            trace ("reading %d bytes\n", size);
            int64_t rd = vfs_fread (buf, 1, size, fp);
            if (rd <= 0) {
                trace ("failed to download %d bytes (got %d bytes)\n", size, rd);
                goto m3u_error;
            }
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
            trace ("writing to %s\n", tempfile);
            out = fdopen (fd, "w+b");
            if (!out) {
                trace ("fdopen failed for %s\n", tempfile);
                goto m3u_error;
            }
            int64_t rw = fwrite (buf, 1, rd, out);
            if (rw != rd) {
                trace ("failed to write %d bytes into file %s\n", size, tempfile);
                goto m3u_error;
            }
            fclose (out);
            fd = -1;
            out = NULL;

            trace ("loading playlist from %s\n", tempfile);
            // load playlist
            playlist_t *plt = plt_alloc ("temp");
            DB_playlist_t **plug = plug_get_playlist_list ();
            int p;
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

            // hack: need to sleep here, some servers like to reject frequent connections
            usleep(conf_get_int ("streamer.wait_ms_after_m3u_link", 400000));

            // for every playlist uri: override stream uri with the one from playlist, and try to play it
            playItem_t *i = (playItem_t *)m3u;
            pl_item_ref (i);
            int res = -1;
            while (i) {
                pl_lock ();
                pl_replace_meta (it, "!URI", pl_find_meta_raw (i, ":URI"));
                pl_unlock ();
                res = stream_track (it, 0);
                if (!res) {
                    pl_item_unref (i);
                    break;
                }
                playItem_t *next = pl_get_next (i, PL_MAIN);
                pl_item_unref (i);
                i = next;
            }
            plt_free (plt);
            if (res == 0) {
                // succeeded -- playing now
                streamer_play_failed (NULL); // reset failed track
                if (from) {
                    pl_item_unref (from);
                }
                if (to) {
                    pl_item_unref (to);
                }
                if (buf) {
                    free (buf);
                }
                unlink (tempfile);
                return res;
            }

m3u_error:
            if (*tempfile) {
                unlink (tempfile);
            }
            err = -1;
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
        streamer_file = NULL;
        vfs_fclose (fp);
    }

    int plug_idx = 0;
    for (;;) {
        if (!decoder_id[0] && plugs[0] && !plugs[plug_idx]) {
            it->played = 1;
            trace ("decoder->init returned %p\n", new_fileinfo);
            if (playing_track == it) {
                send_trackinfochanged (to); // got new metadata, refresh UI
            }
            err = -1;
            goto error;
        }

        DB_decoder_t *dec = NULL;

        if (decoder_id[0]) {
            dec = plug_get_decoder_for_id (decoder_id);
            decoder_id[0] = 0;
            if (!dec) {
                // find new decoder by file extension
                pl_lock ();
                const char *fname = pl_find_meta (it, ":URI");
                const char *ext = strrchr (fname, '.');
                if (ext) {
                    ext++;
                    DB_decoder_t **decs = plug_get_decoder_list ();
                    for (int i = 0; decs[i]; i++) {
                        const char **exts = decs[i]->exts;
                        if (exts) {
                            for (int j = 0; exts[j]; j++) {
                                if (!strcasecmp (exts[j], ext) || !strcmp (exts[j], "*")) {
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
                pl_unlock ();
            }
        }
        else if (plugs[0]) {
            // match by decoder
            dec = plug_get_decoder_for_id (plugs[plug_idx]);
            if (dec) {
                pl_replace_meta (it, "!DECODER", dec->plugin.id);
            }
            plug_idx++;
        }

        if (!dec) {
            trace ("no decoder in playitem!\n");
            it->played = 1;

            streamer_set_playing_track (NULL);

            // failed to play the track, ask for the next one
            streamer_play_failed (it);
            if (from) {
                pl_item_unref (from);
            }
            if (to) {
                pl_item_unref (to);
            }
            return -1;
        }

        trace ("\033[0;33minit decoder for %s (%s)\033[37;0m\n", pl_find_meta (it, ":URI"), dec->plugin.id);
        new_fileinfo = dec_open (dec, STREAMER_HINTS, it);
        if (new_fileinfo->file) {
            new_fileinfo_file = new_fileinfo->file;
        }
        if (new_fileinfo && dec->init (new_fileinfo, DB_PLAYITEM (it)) != 0) {
            trace ("\033[0;31mfailed to init decoder\033[37;0m\n");
            pl_delete_meta (it, "!DECODER");
            dec->free (new_fileinfo);
            new_fileinfo = NULL;
            new_fileinfo_file = NULL;
        }

        if (!new_fileinfo) {
            trace ("decoder %s failed\n", dec->plugin.id);
            continue;
        }
        else {
            new_fileinfo_file = new_fileinfo->file;
            if (streaming_track) {
                pl_item_unref (streaming_track);
            }
            streaming_track = it;
            if (streaming_track) {
                pl_item_ref (streaming_track);
            }

            trace ("bps=%d, channels=%d, samplerate=%d\n", new_fileinfo->fmt.bps, new_fileinfo->fmt.channels, new_fileinfo->fmt.samplerate);
            break;
        }
    }
success:
    streamer_play_failed (NULL);
    if (new_fileinfo) {
        fileinfo = new_fileinfo;
        new_fileinfo = NULL;
        new_fileinfo_file = NULL;
    }

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
    float seek = last_seekpos;
    if (seek >= 0) {
        return seek;
    }
    return playpos;
}

int
streamer_get_apx_bitrate (void) {
    return avg_bitrate;
}

void
streamer_set_nextsong (int song, int startpaused) {
    if (song == -1) {
        // this is a stop query -- clear the queue
        handler_reset (handler);
    }
    streamer_abort_files ();
    handler_push (handler, STR_EV_PLAY_TRACK_IDX, 0, song, startpaused);
}

void
streamer_set_seek (float pos) {
    last_seekpos = pos;
    handler_push (handler, STR_EV_SEEK, 0, *((uint32_t *)&pos), 0);
}

static void
update_stop_after_current (void) {
    if (conf_get_int ("playlist.stop_after_current_reset", 0)) {
        conf_set_int ("playlist.stop_after_current", 0);
        stop_after_current = 0;
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }
}

static void
streamer_next (void) {
    playItem_t *next = get_next_track (streaming_track);
    stream_track (next, 0);
    if (next) {
        pl_item_unref (next);
    }
}

static void
streamer_notify_order_changed_real (int prev_order, int new_order);

static void
streamer_seek_real (float seekpos) {
    float seek = seekpos;
    playItem_t *track = playing_track;
    if (!playing_track) {
        track = streaming_track;
    }
    float dur = track ? pl_get_item_duration (track) : -1;
    if (seek >= 0 && dur > 0) {
        if (seek >= dur) {
            seek = dur - 0.000001f;
        }
        playpos = seek;
        trace ("seeking to %f\n", seek);

        if (track == playing_track && track != streaming_track) {
            // restart streaming the playing track
            if (stream_track (playing_track, 0) < 0) {
                streamer_move_to_nextsong (0);
                return;
            }
        }

        if (fileinfo && track && dur > 0) {
            streamer_lock ();
            if (fileinfo->plugin->seek (fileinfo, playpos) >= 0) {
                streamer_reset (1);
            }
            playpos = fileinfo->readpos;
            avg_bitrate = -1;
            streamer_unlock();
        }
        ddb_event_playpos_t *ev = (ddb_event_playpos_t *)messagepump_event_alloc (DB_EV_SEEKED);
        ev->track = DB_PLAYITEM (track);
        if (track) {
            pl_item_ref (track);
        }
        ev->playpos = playpos;
        messagepump_push_event ((ddb_event_t*)ev, 0, 0);
    }
    last_seekpos = -1;
}

static void
_update_buffering_state () {
    int buffering = (streamreader_num_blocks_ready () < 4) && streaming_track;

    if (buffering != streamer_is_buffering) {
        streamer_is_buffering = buffering;

        // update buffering UI
        if (!buffering) {
            streamer_set_buffering_track (NULL);
        }
        else if (buffering_track) {
            send_trackinfochanged (buffering_track);
        }

        if (playing_track) {
            send_trackinfochanged (playing_track);
        }
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

        uint32_t id;
        uintptr_t ctx;
        uint32_t p1, p2;
        if (!handler_pop (handler, &id, &ctx, &p1, &p2)) {
            switch (id) {
            case STR_EV_PLAY_TRACK_IDX:
                play_index (p1, p2);
                break;
            case STR_EV_PLAY_CURR:
                play_current ();
                break;
            case STR_EV_NEXT:
                play_next ();
                break;
            case STR_EV_PREV:
                {
                    playItem_t *next = get_prev_track(last_played);
                    streamer_reset(1);
                    stream_track(next, 0);
                    if (next) {
                        pl_item_unref(next);
                    }
                    output->play ();
                }
                break;
            case STR_EV_RAND:
                {
                    playItem_t *next = get_random_track();
                    streamer_reset(1);
                    stream_track(next, 0);
                }
                break;
            case STR_EV_SEEK:
                streamer_seek_real(*((float *)&p1));
                break;
            case STR_EV_SET_CURR_PLT:
                streamer_set_current_playlist_real (p1);
                break;
            case STR_EV_DSP_RELOAD:
                streamer_dsp_postinit ();
                break;
            case STR_EV_SET_DSP_CHAIN:
                streamer_set_dsp_chain_real ((ddb_dsp_context_t *)ctx);
                break;
            case STR_EV_ORDER_CHANGED:
                streamer_notify_order_changed_real(p1, p2);
                break;
            }
        }

        if (output->state () == OUTPUT_STATE_STOPPED) {
            usleep (50000);
            continue;
        }

        _update_buffering_state ();

        streamer_lock ();
        if (!fileinfo) {
            streamer_unlock ();
            continue;
        }

        streamblock_t *block = streamreader_get_next_block ();
        streamer_unlock ();

        if (!block) {
            usleep (50000);
            continue;
        }

        int res = streamreader_read_block (block, streaming_track, fileinfo);

        if (res < 0) {
            // error
            streamer_next ();
        }
        else  {
            streamer_lock ();
            streamreader_enqueue_block (block);
            int last = block->last;
            streamer_unlock ();

            if (last) {
                // end of file, next track
                streamer_next ();
            }
        }
    }

    // stop streaming song
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
        fileinfo_file = NULL;
    }
    streamer_lock ();
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    if (playing_track) {
        pl_item_unref (playing_track);
        playing_track = NULL;
    }
    streamer_unlock ();
}

void
streamer_dsp_refresh (void) {
    handler_push (handler, STR_EV_DSP_RELOAD, 0, 0, 0);
}

int
streamer_init (void) {
    streaming_terminate = 0;
    handler = handler_alloc (100);
#if WRITE_DUMP
    out = fopen ("out.raw", "w+b");
#endif
    mutex = mutex_create ();
    wdl_mutex = mutex_create ();

    streamreader_init();
    pl_set_order (conf_get_int ("playback.order", 0));

    streamer_dsp_init ();

    ctmap_init_mutex ();
    deadbeef->conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, conf_network_ctmapping, sizeof (conf_network_ctmapping));
    ctmap_init ();

    streamer_tid = thread_start (streamer_thread, NULL);
    return 0;
}

void
streamer_free (void) {
#if WRITE_DUMP
    fclose (out);
#endif

    if (playing_track) {
        send_trackchanged (playing_track, NULL);
    }
    streamer_abort_files ();
    streaming_terminate = 1;
    thread_join (streamer_tid);

    streamreader_free ();

    if (first_failed_track) {
        pl_item_unref (first_failed_track);
        first_failed_track = NULL;
    }
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    if (playing_track) {
        pl_item_unref (playing_track);
        playing_track = NULL;
    }
    if (buffering_track) {
        pl_item_unref (buffering_track);
        buffering_track = NULL;
    }
    if (last_played) {
        pl_item_unref (last_played);
        last_played = NULL;
    }
    streamer_set_streamer_playlist (NULL);

    ctmap_free ();
    ctmap_free_mutex ();

    mutex_free (mutex);
    mutex = 0;
    mutex_free (wdl_mutex);
    wdl_mutex = 0;

    streamer_dsp_chain_save();

    dsp_free ();

    if (handler) {
        handler_free (handler);
        handler = NULL;
    }
}

// We always decode the entire block, 16384 bytes of input PCM
// after DSP that can become really big.
// Think converting from 8KHz/8 bit to 192KHz/32 bit, thats 96x size increase,
// which gives us the need of 1.5MB buffer.
//
// It's guaranteed that outbuffer contains only samples from the files with same wave format.
//
// FIXME: this BSS allocation is temporary, needs to be on heap, and allocated on demand.
// FIXME: streamer_reset should flush this.
static char outbuffer[512*1024];
static int outbuffer_remaining;

void
streamer_reset (int full) { // must be called when current song changes by external reasons
    if (!mutex) {
        fprintf (stderr, "ERROR: someone called streamer_reset after exit\n");
        return; // failsafe, in case someone calls streamer reset after deinit
    }

    streamer_lock();
    streamreader_reset ();
    dsp_reset ();
    outbuffer_remaining = 0;
    streamer_unlock();
}

// NOTE: this is supposed to be only called from streamer_read
static void
get_desired_output_format (ddb_waveformat_t *in_fmt, ddb_waveformat_t *out_fmt) {
    memcpy (out_fmt, in_fmt, sizeof (ddb_waveformat_t));

    if (autoconv_8_to_16) {
        if (out_fmt->bps == 8) {
            out_fmt->bps = 16;
        }
    }
    if (autoconv_16_to_24) {
        if (out_fmt->bps == 16) {
            out_fmt->bps = 24;
        }
    }
}

static void
streamer_set_output_format (ddb_waveformat_t *fmt) {
    ddb_waveformat_t outfmt;
    get_desired_output_format (fmt, &outfmt);
    if (memcmp (&prev_output_format, &outfmt, sizeof (ddb_waveformat_t))) {
        memcpy (&prev_output_format, &outfmt, sizeof (ddb_waveformat_t));
        DB_output_t *output = plug_get_output ();
        output->setformat (&outfmt);
    }
}

// when firstblock is true -- means it's allowed to change output format
static int
process_output_block (char *bytes, int firstblock) {
    streamblock_t *block = streamreader_get_curr_block();
    if (!block) {
        return -1;
    }
    if (!block->size) {
        streamreader_next_block ();
        _update_buffering_state ();
        return 0;
    }

    DB_output_t *output = plug_get_output ();
    int sz = block->size - block->pos;
    assert (sz);

    // handle stop after current
    int stop = 0;
    if (block->last) {
        if (stop_after_current) {
            stop = 1;
        }
        else {
            if (stop_after_album_check (playing_track, block->track)) {
                stop = 1;
            }
        }
    }

    // handle change of track, or start of a new track
    if (block->last || block->track != playing_track || (playing_track && last_played != playing_track)) {
        // next track started
        update_stop_after_current ();

        if (stop) {
            output->stop ();
            streamer_lock();
            streamer_reset (1);
            _handle_playback_stopped ();
            stream_track (NULL, 0);
            streamer_unlock();
            return 0;
        }

        if (playing_track) {
            send_songfinished (playing_track);
            playpos = 0;
        }

        streamer_start_playback (playing_track, block->track);

        // only reset playpos/bitrate if track changing to another,
        // otherwise the track is the first one, and playpos is pre-set
        playtime = 0;
        avg_bitrate = -1;
        last_seekpos = -1;
        if (playing_track) {
            send_songstarted (playing_track);
        }
    }

    if (firstblock) {
        // Try to set output format to the input format, before running dsp.
        // This is needed so that resampler knows what to resample to.
        // This only needs to be done once per input format change.
        if (memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
            streamer_set_output_format (&block->fmt);
            memcpy (&last_block_fmt, &block->fmt, sizeof (ddb_waveformat_t));
        }
    }

    ddb_waveformat_t datafmt; // comes either from dsp, or from input plugin
    memcpy (&datafmt, &block->fmt, sizeof (ddb_waveformat_t));

    char *dspbytes = NULL;
    int dspsize = 0;
    float dspratio = 1;
    int dsp_res = dsp_apply (&block->fmt, block->buf + block->pos, sz,
                             &datafmt, &dspbytes, &dspsize, &dspratio);
    if (dsp_res) {
        block->pos += sz;
        sz = dspsize;
    }
    else {
        memcpy (&datafmt, &block->fmt, sizeof (ddb_waveformat_t));
        dspbytes = block->buf+block->pos;
        block->pos += sz;
    }

    // Set the final post-dsp output format, if differs.
    // DSP plugins may change output format at any time.
    if (firstblock) {
        streamer_set_output_format (&datafmt);
    }

    if (memcmp (&output->fmt, &datafmt, sizeof (ddb_waveformat_t))) {
        sz = pcm_convert (&datafmt, dspbytes, &output->fmt, bytes, sz);
    }
    else {
        memcpy (bytes, dspbytes, sz);
    }

    playpos += (float)sz/output->fmt.samplerate/((output->fmt.bps>>3)*output->fmt.channels) * dspratio;
    playtime += (float)sz/output->fmt.samplerate/((output->fmt.bps>>3)*output->fmt.channels) * dspratio;

    if (block->pos >= block->size) {
        streamreader_next_block ();
        _update_buffering_state ();
    }

    return sz;
}

int
streamer_read (char *bytes, int size) {
#if 0
    struct timeval tm1;
    gettimeofday (&tm1, NULL);
#endif
    DB_output_t *output = plug_get_output ();

    streamer_lock ();
    streamblock_t *block = streamreader_get_curr_block();
    if (!block) {
        // NULL streaming_track means playback stopped,
        // otherwise just a buffer starvation (e.g. after seeking)
        if (!streaming_track) {
            update_stop_after_current ();
            _handle_playback_stopped();
            playpos = 0;
            playtime = 0;
            avg_bitrate = -1;
            last_seekpos = -1;
        }
        else {
            // this message is printed in more cases, than if output is broken, so disable it
            // but keep for reference -- this is a good place to set breakpoint
//            fprintf (stderr, "streamer: streamer_read has starved. The current output plugin might be broken\n");
        }
        streamer_unlock();

        return streaming_track ? 0 : -1;
    }

    // decode enough blocks to fill the output buffer
    int firstblock = 1;
    while (outbuffer_remaining < size) {
        int rb = process_output_block (outbuffer + outbuffer_remaining, firstblock);
        if (rb <= 0) {
            break;
        }
        outbuffer_remaining += rb;
        firstblock = 0;
    }
    streamer_unlock ();

    // consume decoded data
    int sz = min (size, outbuffer_remaining);
    memcpy (bytes, outbuffer, sz);
    if (sz < outbuffer_remaining) {
        memmove (outbuffer, outbuffer + sz, outbuffer_remaining - sz);
    }
    outbuffer_remaining -= sz;

    // approximate bitrate
    if (block->bitrate != -1) {
        if (avg_bitrate == -1) {
            avg_bitrate = block->bitrate;
        }
        else {
            if (avg_bitrate < block->bitrate) {
                avg_bitrate += 5;
                if (avg_bitrate > block->bitrate) {
                    avg_bitrate = block->bitrate;
                }
            }
            else if (avg_bitrate > block->bitrate) {
                avg_bitrate -= 5;
                if (avg_bitrate < block->bitrate) {
                    avg_bitrate = block->bitrate;
                }
            }
        }
//        printf ("apx bitrate: %d (last %d)\n", avg_bitrate, last_bitrate);
    }

#if 0
    struct timeval tm2;
    gettimeofday (&tm2, NULL);

    int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
    printf ("streamer_read took %d ms\n", ms);
#endif

    if (waveform_listeners || spectrum_listeners) {
        int in_frame_size = (output->fmt.bps >> 3) * output->fmt.channels;
        int in_frames = sz / in_frame_size;
        ddb_waveformat_t out_fmt = {
            .bps = 32,
            .channels = output->fmt.channels,
            .samplerate = output->fmt.samplerate,
            .channelmask = output->fmt.channelmask,
            .is_float = 1,
            .is_bigendian = 0
        };

        float temp_audio_data[in_frames * out_fmt.channels];
        pcm_convert (&output->fmt, bytes, &out_fmt, (char *)temp_audio_data, sz);
        ddb_audio_data_t data;
        data.fmt = &out_fmt;
        data.data = temp_audio_data;
        data.nframes = in_frames;
        mutex_lock (wdl_mutex);
        for (wavedata_listener_t *l = waveform_listeners; l; l = l->next) {
            l->callback (l->ctx, &data);
        }
        mutex_unlock (wdl_mutex);

        if (out_fmt.channels != audio_data_channels || !spectrum_listeners) {
            audio_data_fill = 0;
            audio_data_channels = out_fmt.channels;
        }

        if (spectrum_listeners) {
            int remaining = in_frames;
            do {
                int sz = DDB_FREQ_BANDS * 2 -audio_data_fill;
                sz = min (sz, remaining);
                for (int c = 0; c < audio_data_channels; c++) {
                    for (int s = 0; s < sz; s++) {
                        audio_data[DDB_FREQ_BANDS * 2 * c + audio_data_fill + s] = temp_audio_data[(in_frames-remaining + s) * audio_data_channels + c];
                    }
                }
                //            memcpy (&audio_data[audio_data_fill], &temp_audio_data[in_frames-remaining], sz * sizeof (float));
                audio_data_fill += sz;
                remaining -= sz;
                if (audio_data_fill == DDB_FREQ_BANDS * 2) {
                    for (int c = 0; c < audio_data_channels; c++) {
                        calc_freq (&audio_data[DDB_FREQ_BANDS * 2 * c], &freq_data[DDB_FREQ_BANDS * c]);
                    }
                    ddb_audio_data_t data;
                    data.fmt = &out_fmt;
                    data.data = freq_data;
                    data.nframes = DDB_FREQ_BANDS;
                    mutex_lock (wdl_mutex);
                    for (wavedata_listener_t *l = spectrum_listeners; l; l = l->next) {
                        l->callback (l->ctx, &data);
                    }
                    mutex_unlock (wdl_mutex);
                    audio_data_fill = 0;
                }
            } while (remaining > 0);
        }
    }

    if (!output->has_volume) {
        int mult = 1-audio_is_mute ();
        char *stream = bytes;
        int bytesread = sz;
        if (output->fmt.bps == 16) {
            mult *= 1000;
            int16_t ivolume = volume_get_amp () * mult;
            if (ivolume != 1000) {
                int half = bytesread/2;
                for (int i = 0; i < half; i++) {
                    int16_t sample = *((int16_t*)stream);
                    *((int16_t*)stream) = (int16_t)(((int32_t)sample) * ivolume / 1000);
                    stream += 2;
                }
            }
        }
        else if (output->fmt.bps == 8) {
            mult *= 255;
            int16_t ivolume = volume_get_amp () * mult;
            if (ivolume != 255) {
                for (int i = 0; i < bytesread; i++) {
                    *stream = (int8_t)(((int32_t)(*stream)) * ivolume / 1000);
                    stream++;
                }
            }
        }
        else if (output->fmt.bps == 24) {
            mult *= 1000;
            int16_t ivolume = volume_get_amp () * mult;
            if (ivolume != 1000) {
                int third = bytesread/3;
                for (int i = 0; i < third; i++) {
                    int32_t sample = ((unsigned char)stream[0]) | ((unsigned char)stream[1]<<8) | (stream[2]<<16);
                    int32_t newsample = (int32_t)((int64_t)sample * ivolume / 1000);
                    stream[0] = (newsample&0x0000ff);
                    stream[1] = (newsample&0x00ff00)>>8;
                    stream[2] = (newsample&0xff0000)>>16;
                    stream += 3;
                }
            }
        }
        else if (output->fmt.bps == 32 && !output->fmt.is_float) {
            mult *= 1000;
            int16_t ivolume = volume_get_amp () * mult;
            if (ivolume != 1000) {
                for (int i = 0; i < bytesread/4; i++) {
                    int32_t sample = *((int32_t*)stream);
                    int32_t newsample = (int32_t)((int64_t)sample * ivolume / 1000);
                    *((int32_t*)stream) = newsample;
                    stream += 4;
                }
            }
        }
        else if (output->fmt.bps == 32 && output->fmt.is_float) {
            float fvolume = volume_get_amp () * (1-audio_is_mute ());
            if (fvolume != 1.f) {
                for (int i = 0; i < bytesread/4; i++) {
                    *((float*)stream) = (*((float*)stream)) * fvolume;
                    stream += 4;
                }
            }
        }
    }

    return sz;
}

int
streamer_ok_to_read (int len) {
    return !streamer_is_buffering;
}

void
streamer_configchanged (void) {
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
    int conf_autoconv_16_to_24 = conf_get_int ("streamer.16_to_24",0);
    if (conf_autoconv_16_to_24 != autoconv_16_to_24) {
        autoconv_16_to_24 = conf_autoconv_16_to_24;
        formatchanged = 1;
        streamer_reset (1);
    }

    trace_bufferfill = conf_get_int ("streamer.trace_buffer_fill",0);

    stop_after_current = conf_get_int ("playlist.stop_after_current", 0);
    stop_after_album = conf_get_int ("playlist.stop_after_album", 0);

    char mapstr[2048];
    deadbeef->conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, mapstr, sizeof (mapstr));
    if (strcmp (mapstr, conf_network_ctmapping)) {
        ctmap_init ();
    }

    conf_streamer_nosleep = conf_get_int ("streamer.nosleep", 0);
}

static void
_handle_playback_stopped (void) {
    if (playing_track) {
        playItem_t *trk = playing_track;
        pl_item_ref (trk);
        send_songfinished (trk);
        streamer_is_buffering = 0;
        streamer_start_playback (playing_track, NULL);
        streamer_set_buffering_track (NULL);
        send_trackchanged (trk, NULL);
        pl_item_unref (trk);
    }
}

// play track in current playlist by index;
// negative index will stop playback
static void
play_index (int idx, int startpaused) {
    DB_output_t *output = plug_get_output ();
    playItem_t *it = NULL;
    playlist_t *plt = NULL;

    playqueue_clear ();

    if (idx < 0) {
        goto error;
    }

    plt = plt_get_curr ();
    it = plt_get_item_for_idx (plt, idx, PL_MAIN);
    if (!it) {
        goto error;
    }

    pl_lock ();
    if (plt != streamer_playlist) {
        streamer_set_streamer_playlist (plt);
    }
    pl_unlock();
    streamer_reset(1);
    streamer_is_buffering = 1;
    streamer_set_playing_track(NULL);
    streamer_set_buffering_track (it);
    if (!stream_track(it, startpaused)) {
        playpos = 0;
        playtime = 0;
        if (startpaused) {
            output->pause ();
            streamer_start_playback (NULL, it);
            send_songstarted (playing_track);
        }
        else {
            output->play ();
        }
    }
    else {
        streamer_set_buffering_track (NULL);
    }

    pl_item_unref(it);
    plt_unref (plt);
    return;

error:
    output->stop ();

    streamer_lock();
    streamer_reset (1);

    _handle_playback_stopped ();
    stream_track (NULL, 0);
    if (plt) {
        plt_unref (plt);
    }
    streamer_unlock();
}

// if a track is playing: restart
// if a track is paused: unpause
// if no track is playing: do what comes first:
//     play next in queue
//     play track under cursor
//     stop playback
static void
play_current (void) {
    playlist_t *plt = plt_get_curr ();
    DB_output_t *output = plug_get_output ();
    autoplay = 1;
    if (output->state () == OUTPUT_STATE_PAUSED && playing_track) {
        // restart if network stream
        if (is_remote_stream (playing_track) && pl_get_item_duration (playing_track) < 0) {
            streamer_reset (1);
            stream_track (playing_track, 0);
        }
        // unpause currently paused track
        output->unpause ();
        messagepump_push (DB_EV_PAUSED, 0, 0, 0);
    }
    else if (plt->current_row[PL_MAIN] != -1) {
        // play currently selected track in current playlist
        streamer_reset(1);

        playItem_t *next = NULL;
        int idx = plt->current_row[PL_MAIN];
        if (idx >= 0) {
            next = plt_get_item_for_idx (plt, idx, PL_MAIN);
        }

        if (next) {
            pl_lock ();
            if (plt != streamer_playlist) {
                streamer_set_streamer_playlist (plt);
            }
            pl_unlock ();
            streamer_is_buffering = 1;
            streamer_set_playing_track(NULL);
            streamer_set_buffering_track (next);
            if (!stream_track (next, 0)) {
                playpos = 0;
                playtime = 0;
                output->play ();
            }
            else {
                streamer_set_buffering_track (NULL);
            }
        }
    }
    if (plt) {
        plt_unref (plt);
    }
}

static void
play_next (void) {
    DB_output_t *output = plug_get_output ();
    streamer_reset(1);
    playItem_t *next = get_next_track(last_played);
    streamer_is_buffering = 1;

    if (!next) {
        output->stop ();
        _handle_playback_stopped ();
        return;
    }

    streamer_set_playing_track(NULL);
    streamer_set_buffering_track (next);
    if (!stream_track(next, 0)) {
        playpos = 0;
        playtime = 0;
        output->play ();
    }
    else {
        streamer_set_buffering_track (NULL);
    }
    pl_item_unref(next);
}

void
streamer_play_current_track (void) {
    handler_push (handler, STR_EV_PLAY_CURR, 0, 0, 0);
}

struct DB_fileinfo_s *
streamer_get_current_fileinfo (void) {
    return fileinfo;
}

static void
streamer_set_current_playlist_real (int plt) {
    pl_lock ();
    if (streamer_playlist) {
        plt_unref (streamer_playlist);
    }
    streamer_playlist = plt_get_for_idx (plt);
    pl_unlock ();
}

void
streamer_set_current_playlist (int plt) {
    handler_push (handler, STR_EV_SET_CURR_PLT, 0, plt, 0);
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

void
streamer_set_dsp_chain (ddb_dsp_context_t *chain) {
    ddb_dsp_context_t *new_chain = NULL;
    ddb_dsp_context_t *tail = NULL;
    while (chain) {
        ddb_dsp_context_t *new = dsp_clone (chain);
        if (tail) {
            tail->next = new;
            tail = new;
        }
        else {
            new_chain = tail = new;
        }
        chain = chain->next;
    }

    handler_push (handler, STR_EV_SET_DSP_CHAIN, (uintptr_t)new_chain, 0, 0);
}

static void
streamer_notify_order_changed_real (int prev_order, int new_order) {
    if (prev_order != PLAYBACK_ORDER_SHUFFLE_ALBUMS && new_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
        streamer_lock ();
        playItem_t *curr = playing_track;
        if (curr) {

            pl_lock ();
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
            pl_unlock ();
        }
        streamer_unlock ();
    }
}

void
streamer_notify_order_changed (int prev_order, int new_order) {
    handler_push (handler, STR_EV_ORDER_CHANGED, 0, prev_order, new_order);
}

void
vis_waveform_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data)) {
    mutex_lock (wdl_mutex);
    wavedata_listener_t *l = malloc (sizeof (wavedata_listener_t));
    memset (l, 0, sizeof (wavedata_listener_t));
    l->ctx = ctx;
    l->callback = callback;
    l->next = waveform_listeners;
    waveform_listeners = l;
    mutex_unlock (wdl_mutex);
}

void
vis_waveform_unlisten (void *ctx) {
    mutex_lock (wdl_mutex);
    wavedata_listener_t *l, *prev = NULL;
    for (l = waveform_listeners; l; prev = l, l = l->next) {
        if (l->ctx == ctx) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                waveform_listeners = l->next;
            }
            free (l);
            break;
        }
    }
    mutex_unlock (wdl_mutex);
}

void
vis_spectrum_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data)) {
    mutex_lock (wdl_mutex);
    wavedata_listener_t *l = malloc (sizeof (wavedata_listener_t));
    memset (l, 0, sizeof (wavedata_listener_t));
    l->ctx = ctx;
    l->callback = callback;
    l->next = spectrum_listeners;
    spectrum_listeners = l;
    mutex_unlock (wdl_mutex);
}

void
vis_spectrum_unlisten (void *ctx) {
    mutex_lock (wdl_mutex);
    wavedata_listener_t *l, *prev = NULL;
    for (l = spectrum_listeners; l; prev = l, l = l->next) {
        if (l->ctx == ctx) {
            if (prev) {
                prev->next = l->next;
            }
            else {
                spectrum_listeners = l->next;
            }
            free (l);
            break;
        }
    }
    mutex_unlock (wdl_mutex);
}

void
streamer_set_streamer_playlist (playlist_t *plt) {
    pl_lock ();
    if (streamer_playlist) {
        plt_unref (streamer_playlist);
    }
    streamer_playlist = plt;
    if (streamer_playlist) {
        plt_ref (streamer_playlist);
    }
    pl_unlock ();
}

struct handler_s *
streamer_get_handler (void) {
    return handler;
}

void
streamer_set_playing_track (playItem_t *it) {
    if (it == playing_track) {
        return;
    }

    playItem_t *prev = playing_track;

    playing_track = it;
    if (playing_track) {
        pl_item_ref (playing_track);
    }

    send_trackinfochanged(prev);

    if (playing_track) {
        send_trackinfochanged(playing_track);
    }

    if (prev) {
        pl_item_unref (prev);
    }
}

void
streamer_set_buffering_track (playItem_t *it) {
    if (it == buffering_track) {
        return;
    }

    playItem_t *prev = buffering_track;

    buffering_track = NULL;

    buffering_track = it;
    if (buffering_track) {
        pl_item_ref (buffering_track);
    }

    send_trackinfochanged(prev);

    if (buffering_track) {
        send_trackinfochanged(buffering_track);
    }

    if (prev) {
        pl_item_unref (prev);
    }
}

void
streamer_yield (void) {
    while (handler_hasmessages(handler)) {
        usleep(50000);
    }
}
