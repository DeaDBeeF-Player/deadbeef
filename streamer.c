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

#ifdef trace
#undef trace
#define trace(...)
#endif

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


static int conf_streamer_override_samplerate = 0;
static int conf_streamer_use_dependent_samplerate = 0;
static int conf_streamer_samplerate = 44100;
static int conf_streamer_samplerate_mult_48 = 48000;
static int conf_streamer_samplerate_mult_44 = 44100;

static int trace_bufferfill = 0;

static int stop_after_current = 0;
static int stop_after_album = 0;

static int streaming_terminate;

static uintptr_t mutex;
static uintptr_t wdl_mutex; // wavedata listener

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

static int _format_change_wait;
static ddb_waveformat_t prev_output_format; // last format that was sent to output via streamer_set_output_format
static ddb_waveformat_t last_block_fmt; // input file format corresponding to the current output

static DB_fileinfo_t *fileinfo;
static DB_FILE *fileinfo_file;
static DB_fileinfo_t *new_fileinfo;
static DB_FILE *new_fileinfo_file;

// This counter is incremented by one for each streamer_read call, which returns -1,
// which means audio should stop, but we need to wait a bit until buffered data has finished playing,
// so we wait AUDIO_STALL_WAIT periods
#define AUDIO_STALL_WAIT 20
static int _audio_stall_count;

// to allow interruption of stall file requests
static DB_FILE *streamer_file;

#if defined(HAVE_XGUI) || defined(ANDROID)
#include "equalizer.h"
#endif

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
play_next (int dir);

static void
streamer_set_current_playlist_real (int plt);

static int
stream_track (playItem_t *track, int startpaused);

static void
_handle_playback_stopped (void);

static void
_streamer_mark_album_played_up_to (playItem_t *item);

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
    if (from == to) {
        return;
    }
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
streamer_set_last_played (playItem_t *track) {
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

        streamer_set_last_played (playing_track);

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
    playItem_t *it = (buffering_track && !playing_track) ? buffering_track : playing_track;
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

    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        playItem_t *it = NULL;
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
            it = pmin;
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
        }
        else {
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
            it = pmin;
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
        }
        // prevent repeating the same track after reshuffle
        if (it == curr) {
            if (it->next[PL_MAIN]) {
                it = it->next[PL_MAIN];
            }
            else if (plt->head[PL_MAIN] && plt->head[PL_MAIN] != it) {
                it = plt->head[PL_MAIN];
            }
        }

        // plt_reshuffle doesn't add ref
        pl_item_ref (it);
        pl_unlock ();
        return it;
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (curr) {
            it = curr->next[PL_MAIN];
        }
        else {
            it = plt->head[PL_MAIN];
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
    
    // check if prev song is in this playlist
    if (curr && -1 == str_get_idx_of (curr)) {
        curr = NULL;
    }

    if (!streamer_playlist) {
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
    }
    
    playlist_t *plt = streamer_playlist;

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
        else {
            it = plt->tail[PL_MAIN];
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

// handle "first failed track" logic,
// and initialize dummy fileinfo for handling track switching for the tracks which can't be played
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
        streamer_set_last_played (failed_track);
        // the track has failed to be played,
        // but we want to send it down to streamreader for proper track switching etc.
        fileinfo = calloc(sizeof (DB_fileinfo_t), 1);
        fileinfo_file = NULL;
        new_fileinfo = NULL;
        new_fileinfo_file = NULL;
        if (streaming_track) {
            pl_item_unref (streaming_track);
        }
        streaming_track = failed_track;
        if (streaming_track) {
            pl_item_ref (streaming_track);
        }
    }
    streamer_unlock();
}

static void
fileinfo_free (DB_fileinfo_t *fileinfo) {
    if (fileinfo->plugin) {
        fileinfo->plugin->free (fileinfo);
    }
    else {
        free (fileinfo);
    }
}

static int
stream_track (playItem_t *it, int startpaused) {
    if (fileinfo) {
        fileinfo_free (fileinfo);
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
    char *cct = "undefined";
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
            if (!startpaused) {
                streamer_play_failed (it);
            }
            else {
                err = -1;
            }

            pl_lock ();
            trace_err ("Failed to play track: %s\n", pl_find_meta(it, ":URI"));
            pl_unlock ();

            goto error;
        }
        trace ("got content-type: %s\n", ct);
        cct = strdupa (ct);
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
#ifndef ANDROID
            const char *tmpdir = getenv ("TMPDIR");
            if (!tmpdir) {
                tmpdir = "/tmp";
            }
#else
            const char *tmpdir = dbconfdir;
#endif
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
            trace_err ("No suitable decoder found for stream %s of content-type %s\n", pl_find_meta (playing_track, ":URI"), cct);

            if (!startpaused) {
                streamer_play_failed (it);
            }
            else {
                err = -1;
            }
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

            if (!startpaused) {
                streamer_play_failed (it);
            }
            else {
                err = -1;
            }

            pl_lock ();
            trace_err ("Failed to play track: %s\n", pl_find_meta(it, ":URI"));
            pl_unlock ();

            goto error;
        }

        trace ("\033[0;33minit decoder for %s (%s)\033[37;0m\n", pl_find_meta (it, ":URI"), dec->plugin.id);
        new_fileinfo = dec_open (dec, STREAMER_HINTS, it);
        if (new_fileinfo && new_fileinfo->file) {
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
    playItem_t *next = NULL;
    if (playing_track) {
        int pl_loop_mode = conf_get_int ("playback.loop", 0);
        if (pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
            next = playing_track;
            pl_item_ref (next);
        }
    }
    if (!next) {
        next = get_next_track (streaming_track);
    }
    stream_track (next, 0);
    if (next) {
        pl_item_unref (next);
    }
}

static void
streamer_notify_order_changed_real (int prev_order, int new_order);

static void
streamer_seek_real (float seekpos) {
    // Some fileinfos can exist without plugin bound to them,
    // for example when a track failed to play.
    // Don't attempt seeking in them.
    if (!fileinfo || !fileinfo->plugin) {
        return;
    }
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
    int blocks_ready = streamreader_num_blocks_ready ();
    int buffering = (blocks_ready < 4) && streaming_track;

    if (buffering != streamer_is_buffering) {
        streamer_is_buffering = buffering;

        // update buffering UI
        if (!buffering) {
            if (buffering_track) {
                playItem_t *prev = buffering_track;
                pl_item_ref (prev);
                streamer_set_buffering_track (NULL);
                send_trackinfochanged (prev);
                pl_item_unref (prev);
            }
            else if (playing_track) {
                send_trackinfochanged (playing_track);
            }
        }
        else if (buffering_track) {
            send_trackinfochanged (buffering_track);
        }
    }
}

static void
handle_track_change (playItem_t *from, playItem_t *track) {
    // next track started
    if (from) {
        send_songfinished (from);
        playpos = 0;
    }

    streamer_start_playback (from, track);

    // only reset playpos/bitrate if track changing to another,
    // otherwise the track is the first one, and playpos is pre-set
    playtime = 0;
    avg_bitrate = -1;
    last_seekpos = -1;
    if (playing_track) {
        send_songstarted (playing_track);
    }
}

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

#if !defined(ANDROID) && !defined(HAVE_XGUI)
    // samplerate override
    if (conf_streamer_override_samplerate) {
        out_fmt->samplerate = conf_streamer_samplerate;
        if (conf_streamer_use_dependent_samplerate) {
            if (0 == (in_fmt->samplerate % 48000)) {
                out_fmt->samplerate = conf_streamer_samplerate_mult_48;
            }
            else if (0 == (in_fmt->samplerate % 44100)) {
                out_fmt->samplerate = conf_streamer_samplerate_mult_44;
            }
        }
    }
#endif

    // android simulation on PC: force 16 bit and 44100 or 48000 only, to force format conversion
#if !defined(ANDROID) && defined(HAVE_XGUI)
    out_fmt->bps = 16;
    if (!(out_fmt->samplerate % 48000)) {
        out_fmt->samplerate = 48000;
    }
    else if (!(out_fmt->samplerate % 44100)) {
        out_fmt->samplerate = 44100;
    }
#endif
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

void
streamer_thread (void *unused) {
#if defined(__linux__) && !defined(ANDROID)
    prctl (PR_SET_NAME, "deadbeef-stream", 0, 0, 0, 0);
#endif

    uint32_t id;
    uintptr_t ctx;
    uint32_t p1, p2;
    while (!streaming_terminate) {
        struct timeval tm1;
        DB_output_t *output = plug_get_output ();
        gettimeofday (&tm1, NULL);

        while (!handler_pop (handler, &id, &ctx, &p1, &p2)) {
            switch (id) {
            case STR_EV_PLAY_TRACK_IDX:
                play_index (p1, p2);
                break;
            case STR_EV_PLAY_CURR:
                play_current ();
                break;
            case STR_EV_NEXT:
                play_next (1);
                break;
            case STR_EV_PREV:
                play_next (-1);
                break;
            case STR_EV_RAND:
                play_next (0);
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
            if (!handler_hasmessages (handler)) {
                usleep (50000);
            }
            continue;
        }

        if (_format_change_wait) {
            streamer_lock ();
            streamblock_t *block = streamreader_get_curr_block();
            if (block) {
                if (memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
                    streamer_set_output_format (&block->fmt);
                    memcpy (&last_block_fmt, &block->fmt, sizeof (ddb_waveformat_t));
                }
            }
            _format_change_wait = 0;
            streamer_unlock ();
        }

        _update_buffering_state ();

        if (!fileinfo) {
            // HACK: This is to overcome the output plugin API limitation.
            // We count the number of times the output plugin has starved,
            // and stop playback after counter reaches the limit.
            // The correct way to solve this is to add a `drain` API.
            if (_audio_stall_count >= AUDIO_STALL_WAIT) {
                output->stop ();
                streamer_lock ();
                _handle_playback_stopped();
                _audio_stall_count = 0;
                streamer_unlock ();
                continue;
            }
            usleep (50000); // nothing is streaming -- about to stop
            continue;
        }

        streamblock_t *block = streamreader_get_next_block ();

        if (!block) {
            usleep (50000); // all blocks are full
            continue;
        }

        // streamreader_read_block will lock the mutex after success
        int res = streamreader_read_block (block, streaming_track, fileinfo, mutex);
        int last = 0;

        if (res >= 0) {
            streamreader_enqueue_block (block);
            last = block->last;
            streamer_unlock ();
        }

        if (res < 0 || last) {
            // error or eof

            // handle stop after current
            int stop = 0;
            if (block->last) {
                if (stop_after_current) {
                    stop = 1;
                }
                else {
                    playItem_t *next = get_next_track(playing_track);

                    if (stop_after_album_check (playing_track, next)) {
                        stop = 1;
                    }

                    if (next) {
                        pl_item_unref (next);
                    }
                }
            }

            // next track
            if (!stop) {
                streamer_next ();
            }
            else {
                stream_track(NULL, 0);
            }
        }

    }

    // drain event queue
    while (!handler_pop (handler, &id, &ctx, &p1, &p2));

    // stop streaming song
    if (fileinfo) {
        fileinfo_free (fileinfo);
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

    playpos = 0;
    playtime = 0;
}

// We always decode the entire block, 16384 bytes of input PCM
// after DSP that can become really big.
// Think converting from 8KHz/8 bit to 192KHz/32 bit, thats 96x size increase,
// which gives us the need of 1.5MB buffer.
//
// It's guaranteed that outbuffer contains only samples from the files with same wave format.
//
// FIXME: this BSS allocation is temporary, needs to be on heap, and allocated on demand.
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

static int
process_output_block (streamblock_t *block, char *bytes) {
    DB_output_t *output = plug_get_output ();

    // handle change of track
    if (block->last) {
        update_stop_after_current ();
    }
    if (block->first) {
        handle_track_change (playing_track, block->track);
    }

    // A block with 0 size is a valid block, and needs to be processed as usual (code above this line).
    // But here we do early exit, because there's no data to process in it.
    if (!block->size) {
        streamreader_next_block ();
        _update_buffering_state ();
        return 0;
    }

    int sz = block->size - block->pos;
    assert (sz);

    ddb_waveformat_t datafmt; // comes either from dsp, or from input plugin
    memcpy (&datafmt, &block->fmt, sizeof (ddb_waveformat_t));

    char *dspbytes = NULL;
    int dspsize = 0;
    float dspratio = 1;

#if defined(ANDROID) || defined(HAVE_XGUI)
    // android EQ and resampling require 16 bit, so convert here if needed
    int tempsize = sz * 16 / block->fmt.bps;
    int16_t *temp_audio_data = NULL;
    char *input = block->buf + block->pos;
    block->pos += sz;
    if (block->fmt.bps != 16) {
        temp_audio_data = alloca (tempsize);
        ddb_waveformat_t out_fmt = {
            .bps = 16,
            .channels = block->fmt.channels,
            .samplerate = block->fmt.samplerate,
            .channelmask = block->fmt.channelmask,
            .is_float = 0,
            .is_bigendian = 0
        };

        pcm_convert (&block->fmt, (char *)input, &out_fmt, (char *)temp_audio_data, sz);
        input = (char *)temp_audio_data;
        memcpy (&datafmt, &out_fmt, sizeof (ddb_waveformat_t));
        sz = tempsize;
    }

    extern void android_eq_apply (char *dspbytes, int dspsize);
    android_eq_apply (input, sz);

    dsp_apply_simple_downsampler(datafmt.samplerate, datafmt.channels, input, sz, output->fmt.samplerate, &dspbytes, &dspsize);
    datafmt.samplerate = output->fmt.samplerate;
    sz = dspsize;
#else
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
#endif

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


static float (*streamer_volume_modifier) (float delta_time);

void
streamer_set_volume_modifier (float (*modifier) (float delta_time)) {
    streamer_volume_modifier = modifier;
}

static void
streamer_apply_soft_volume (char *bytes, int sz) {
    DB_output_t *output = plug_get_output ();

    float mod = 1.f;

    if (streamer_volume_modifier) {
        float dt = sz * (output->fmt.bps >> 3) / output->fmt.channels / (float)output->fmt.samplerate;
        mod = streamer_volume_modifier (dt);
    }

    float vol = volume_get_amp () * mod;

    if (!output->has_volume) {
        int mult = 1-audio_is_mute ();
        char *stream = bytes;
        int bytesread = sz;
        if (output->fmt.bps == 16) {
            mult *= 1000;
            int16_t ivolume = vol * mult;
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
            int16_t ivolume = vol * mult;
            if (ivolume != 255) {
                for (int i = 0; i < bytesread; i++) {
                    *stream = (int8_t)(((int32_t)(*stream)) * ivolume / 1000);
                    stream++;
                }
            }
        }
        else if (output->fmt.bps == 24) {
            mult *= 1000;
            int16_t ivolume = vol * mult;
            if (ivolume != 1000) {
                int third = bytesread/3;
                for (int i = 0; i < third; i++) {
                    int32_t sample = ((unsigned char)stream[0]) | ((unsigned char)stream[1]<<8) | ((signed char)stream[2]<<16);
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
            int16_t ivolume = vol * mult;
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
            float fvolume = vol * (1-audio_is_mute ());
            if (fvolume != 1.f) {
                for (int i = 0; i < bytesread/4; i++) {
                    *((float*)stream) = (*((float*)stream)) * fvolume;
                    stream += 4;
                }
            }
        }
    }
}

int
streamer_read (char *bytes, int size) {
#if 0
    struct timeval tm1;
    gettimeofday (&tm1, NULL);
#endif
    DB_output_t *output = plug_get_output ();

    if (_format_change_wait) {
        memset (bytes, 0, size);
        return size;
    }

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

        if (streaming_track) {
            memset (bytes, 0, size);
            return size;
        }
        _audio_stall_count++;
        return 0;
    }

    _audio_stall_count = 0;

    int block_bitrate = -1;

    // only decode until the next format change
    if (!memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
        // decode enough blocks to fill the output buffer
        while (block != NULL && outbuffer_remaining < size && !memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
            int rb = process_output_block (block, outbuffer + outbuffer_remaining);
            if (rb <= 0) {
                break;
            }
            outbuffer_remaining += rb;
            block_bitrate = block->bitrate;
            block = streamreader_get_curr_block();
        }
    }
    // empty buffer and the next block format differs? request format change!

    if (!outbuffer_remaining && block && memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
        _format_change_wait = 1;
        streamer_unlock();
        memset (bytes, 0, size);
        return size;
    }
    streamer_unlock ();

    // consume decoded data
    int sz = min (size, outbuffer_remaining);
    if (!sz) {
        // no data available
        memset (bytes, 0, size);
        return size;
    }

    // clip to frame size
    int ss = output->fmt.channels * output->fmt.bps / 8;
    if ((sz % ss) != 0) {
        sz -= (sz % ss);
    }

    memcpy (bytes, outbuffer, sz);
    if (sz < outbuffer_remaining) {
        memmove (outbuffer, outbuffer + sz, outbuffer_remaining - sz);
    }
    outbuffer_remaining -= sz;

    // approximate bitrate
    if (block_bitrate != -1) {
        if (avg_bitrate == -1) {
            avg_bitrate = block_bitrate;
        }
        else {
            if (avg_bitrate < block_bitrate) {
                avg_bitrate += 5;
                if (avg_bitrate > block_bitrate) {
                    avg_bitrate = block_bitrate;
                }
            }
            else if (avg_bitrate > block_bitrate) {
                avg_bitrate -= 5;
                if (avg_bitrate < block_bitrate) {
                    avg_bitrate = block_bitrate;
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

#ifndef ANDROID

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
#endif

    streamer_apply_soft_volume (bytes, sz);

    return sz;
}

int
streamer_ok_to_read (int len) {
    return !streamer_is_buffering;
}

static int
clamp_samplerate (int val) {
    if (val < 8000) {
        return 8000;
    }
    else if (val > 768000) {
        return 768000;
    }
    return val;
}

void
streamer_configchanged (void) {
    streamer_lock ();
    pl_set_order (conf_get_int ("playback.order", 0));
    if (playing_track) {
        playing_track->played = 1;
    }

    int formatchanged = 0;

    int conf_autoconv_8_to_16 = conf_get_int ("streamer.8_to_16", 1);
    if (conf_autoconv_8_to_16 != autoconv_8_to_16) {
        autoconv_8_to_16 = conf_autoconv_8_to_16;
        formatchanged = 1;
    }
    int conf_autoconv_16_to_24 = conf_get_int ("streamer.16_to_24",0);
    if (conf_autoconv_16_to_24 != autoconv_16_to_24) {
        autoconv_16_to_24 = conf_autoconv_16_to_24;
        formatchanged = 1;
    }

    trace_bufferfill = conf_get_int ("streamer.trace_buffer_fill",0);

    stop_after_current = conf_get_int ("playlist.stop_after_current", 0);
    stop_after_album = conf_get_int ("playlist.stop_after_album", 0);

    char mapstr[2048];
    deadbeef->conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, mapstr, sizeof (mapstr));
    if (strcmp (mapstr, conf_network_ctmapping)) {
        ctmap_init ();
    }

    int new_conf_streamer_override_samplerate = conf_get_int ("streamer.override_samplerate", 0);
    int new_conf_streamer_use_dependent_samplerate = conf_get_int ("streamer.use_dependent_samplerate", 0);
    int new_conf_streamer_samplerate = clamp_samplerate (conf_get_int ("streamer.samplerate", 44100));
    int new_conf_streamer_samplerate_mult_48 = clamp_samplerate (conf_get_int ("streamer.samplerate_mult_48", 48000));
    int new_conf_streamer_samplerate_mult_44 = clamp_samplerate (conf_get_int ("streamer.samplerate_mult_44", 44100));

    if (conf_streamer_override_samplerate != new_conf_streamer_override_samplerate
        || conf_streamer_use_dependent_samplerate != new_conf_streamer_use_dependent_samplerate
        || conf_streamer_samplerate != new_conf_streamer_samplerate
        || conf_streamer_samplerate_mult_48 != new_conf_streamer_samplerate_mult_48
        || conf_streamer_samplerate_mult_44 != new_conf_streamer_samplerate_mult_44
        || formatchanged) {
        memset (&last_block_fmt, 0, sizeof (last_block_fmt));
    }

    conf_streamer_override_samplerate = new_conf_streamer_override_samplerate;
    conf_streamer_use_dependent_samplerate = new_conf_streamer_use_dependent_samplerate;
    conf_streamer_samplerate = new_conf_streamer_samplerate;
    conf_streamer_samplerate_mult_48 = new_conf_streamer_samplerate_mult_48;
    conf_streamer_samplerate_mult_44 = new_conf_streamer_samplerate_mult_44;

    streamer_unlock ();

    streamreader_configchanged ();
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
    streamer_play_failed (NULL);
}

static void
_play_track (playItem_t *it, int startpaused) {
    DB_output_t *output = plug_get_output();
    output->stop ();
    streamer_reset(1);
    streamer_is_buffering = 1;

    playItem_t *prev = playing_track;
    if (prev) {
        pl_item_ref (prev);
    }

    streamer_set_playing_track(NULL);
    streamer_set_buffering_track (it);
    handle_track_change (prev, it);

    if (prev) {
        pl_item_unref (prev);
        prev = NULL;
    }

    if (!stream_track(it, startpaused)) {
        playpos = 0;
        playtime = 0;
        if (startpaused) {
            output->pause ();
            messagepump_push(DB_EV_PAUSED, 0, 1, 0);
            streamer_start_playback (NULL, it);
            send_songstarted (playing_track);
        }
        else {
            int st = output->state();
            output->play ();
            if (st == OUTPUT_STATE_PAUSED) {
                messagepump_push(DB_EV_PAUSED, 0, 0, 0);
            }
        }
    }
    else {
        streamer_set_buffering_track (NULL);
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

    _streamer_mark_album_played_up_to (it);
    _play_track(it, startpaused);

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
    if (output->state () == OUTPUT_STATE_PAUSED && playing_track) {
        // restart if network stream
        if (is_remote_stream (playing_track) && pl_get_item_duration (playing_track) < 0) {
            streamer_reset (1);
            stream_track (playing_track, 0);
        }
        // unpause currently paused track
        output->unpause ();
        messagepump_push (DB_EV_PAUSED, 0, 0, 0);
        return;
    }

    int idx = plt->current_row[PL_MAIN];
    if (plt->current_row[PL_MAIN] == -1 && plt->count[PL_MAIN]) {
        idx = 0;
    }


    if (idx >= 0) {
        // play currently selected track in current playlist
        streamer_reset(1);

        playItem_t *next = plt_get_item_for_idx (plt, idx, PL_MAIN);

        if (next) {
            pl_lock ();
            if (plt != streamer_playlist) {
                streamer_set_streamer_playlist (plt);
            }
            pl_unlock ();
            _play_track(next, 0);
            pl_item_unref (next);
        }
    }

    if (plt) {
        plt_unref (plt);
    }
}

playItem_t *
streamer_get_next_track_with_direction (int dir) {
    playItem_t *origin = NULL;
    if (buffering_track) {
        origin = buffering_track;
    }
    else {
        origin = last_played;
    }

    playItem_t *next = NULL;
    if (dir > 0) {
        next = get_next_track (origin);
    }
    else if (dir < 0) {
        next = get_prev_track (origin);
    }
    else {
        next = get_random_track ();
    }

    // possibly need a reshuffle
    if (!next && streamer_playlist->count[PL_MAIN] != 0) {
        int pl_loop_mode = conf_get_int ("playback.loop", 0);
        int pl_order = conf_get_int ("playback.order", 0);

        if (pl_loop_mode == PLAYBACK_MODE_NOLOOP) {
            if (pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS || pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS) {
                plt_reshuffle (streamer_playlist, dir > 0 ? &next : NULL, dir < 0 ? &next : NULL);
                if (next && dir < 0) {
                    // mark all songs as played except the current one
                    playItem_t *it = streamer_playlist->head[PL_MAIN];
                    while (it) {
                        if (it != next) {
                            it->played = 1;
                        }
                        it = it->next[PL_MAIN];
                    }
                }
                if (next) {
                    pl_item_ref (next);
                }
            }
        }
    }
    return next;
}

static void
play_next (int dir) {
    DB_output_t *output = plug_get_output ();
    streamer_reset(1);

    playItem_t *next = streamer_get_next_track_with_direction (dir);

    if (!next) {
        streamer_set_last_played (NULL);
        output->stop ();
        _handle_playback_stopped ();
        return;
    }

    _play_track(next, 0);
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
_streamer_mark_album_played_up_to (playItem_t *item) {
    pl_lock ();
    const char *alb = pl_find_meta_raw (item, "album");
    const char *art = pl_find_meta_raw (item, "artist");
    playItem_t *next = item->prev[PL_MAIN];
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

static void
streamer_notify_order_changed_real (int prev_order, int new_order) {
    if (prev_order != PLAYBACK_ORDER_SHUFFLE_ALBUMS && new_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
        streamer_lock ();

        playItem_t *curr = playing_track;
        if (curr) {
            _streamer_mark_album_played_up_to (curr);
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

void
streamer_set_output (DB_output_t *output) {
    printf ("streamer_set_output\n");
    if (mutex) {
        streamer_lock ();
    }
    DB_output_t *prev = plug_get_output ();
    int state = OUTPUT_STATE_STOPPED;

    ddb_waveformat_t fmt = {0};
    if (prev) {
        state = prev->state ();
        memcpy (&fmt, &prev->fmt, sizeof (ddb_waveformat_t));
        prev->free ();
    }
    plug_set_output (output);

    if (fmt.channels) {
        output->setformat (&fmt);
    }

    int res = 0;
    if (state == OUTPUT_STATE_PLAYING) {
        res = output->play ();
    }
    else if (state == OUTPUT_STATE_PAUSED) {
        res = output->pause ();
    }

    if (res < 0) {
        trace_err ("failed to init sound output\n");
        streamer_set_nextsong (-1, 0);
    }
    if (mutex) {
        streamer_unlock ();
    }
    messagepump_push (DB_EV_OUTPUTCHANGED, 0, 0, 0);
}
