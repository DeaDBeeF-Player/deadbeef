/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  streamer implementation

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#ifdef __linux__
#    include <sys/prctl.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include "threading.h"
#include "playlist.h"
#include "plmeta.h"
#include <deadbeef/common.h>
#include "shared/ctmap.h"
#include "streamer.h"
#include "messagepump.h"
#include "conf.h"
#include "plugins.h"
#include "volume.h"
#include "vfs.h"
#include "premix.h"
#include "handler.h"
#include "shared/parser.h"
#include "resizable_buffer.h"
#include "ringbuf.h"
#include <deadbeef/strdupa.h>
#include "playqueue.h"
#include "streamreader.h"
#include "decodedblock.h"
#include "dsp.h"
#include "playmodes.h"
#include "tf.h"
#include "viz.h"
#include "fft.h"
#ifdef __APPLE__
#    include "coreaudio.h"
#endif

#ifdef trace
#    undef trace
#    define trace(...)
#endif

//#define WRITE_DUMP 1
//#define DETECT_PL_LOCK_RC 1

#if WRITE_DUMP
FILE *out;
#endif

#define MAX_PLAYLIST_DOWNLOAD_SIZE 25000
#define STREAMER_HINTS (DDB_DECODER_HINT_NEED_BITRATE | DDB_DECODER_HINT_CAN_LOOP)

static intptr_t streamer_tid;

static int autoconv_8_to_16 = 1;

static int autoconv_16_to_24 = 0;

static int bit_override = 0;

static int conf_streamer_override_samplerate = 0;
static int conf_streamer_use_dependent_samplerate = 0;
static int conf_streamer_samplerate = 44100;
static int conf_streamer_samplerate_mult_48 = 48000;
static int conf_streamer_samplerate_mult_44 = 44100;
static float conf_format_silence = -1.f;
static float conf_playback_buffer_size = 0.3f;

static int trace_bufferfill = 0;

static int stop_after_current = 0;
static int stop_after_album = 0;

static int streaming_terminate;

static uintptr_t mutex;

static float last_seekpos = -1;

static float playpos = 0; // play position of current song
static int avg_bitrate = -1; // avg bitrate of current song

static int streamer_is_buffering;

static playlist_t *streamer_playlist;
static playItem_t *playing_track;

// If streaming_track has been removed from playlist, fallback to one of those tracks to pick next/prev.
static playItem_t *next_track_to_play;
static playItem_t *prev_track_to_play;

static playItem_t *buffering_track;
static float playtime; // total playtime of playing track
static time_t started_timestamp; // result of calling time(NULL)
static playItem_t *streaming_track;
static playItem_t *last_played; // this is the last track that was played, should avoid setting this to NULL

static ddb_waveformat_t prev_output_format; // last format that was sent to output via streamer_set_output_format
static ddb_waveformat_t last_block_fmt; // input file format corresponding to the current output

static char *_artist_tf;
static char *_album_tf;

static DB_fileinfo_t *fileinfo_curr;
static uint64_t fileinfo_file_identifier;
static DB_vfs_t *fileinfo_file_vfs;
static DB_fileinfo_t *new_fileinfo;
static uint64_t new_fileinfo_file_identifier;
static DB_vfs_t *new_fileinfo_file_vfs;

// This counter is incremented by one for each streamer_read call, which returns -1,
// which means audio should stop, but we need to wait a bit until buffered data has finished playing,
// so we wait AUDIO_STALL_WAIT periods
#define AUDIO_STALL_WAIT 20
static int _audio_stall_count;

// to allow interruption of stall file requests
static uint64_t streamer_file_identifier;
static DB_vfs_t *streamer_file_vfs;

static char *_int_output_buffer;
static ringbuf_t _output_ringbuf;

static resizable_buffer_t _dsp_process_buffer;
static resizable_buffer_t _viz_read_buffer;

#if defined(HAVE_XGUI) || defined(ANDROID)
#    include "equalizer.h"
#endif

// message queue
static struct handler_s *handler;

#if DETECT_PL_LOCK_RC
volatile pthread_t streamer_lock_tid = 0;
#endif
void
streamer_lock (void) {
#if DETECT_PL_LOCK_RC
    extern pthread_t pl_lock_tid;
    assert (pl_lock_tid != pthread_self ()); // not permitted to lock streamer from inside of pl_lock
#endif
    mutex_lock (mutex);
#if DETECT_PL_LOCK_RC
    streamer_lock_tid = pthread_self ();
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
play_next (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat);

static void
play_next_album (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat);

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
    streamer_lock ();
    DB_vfs_t *file_vfs = fileinfo_file_vfs;
    uint64_t file_identifier = fileinfo_file_identifier;

    DB_vfs_t *newfile_vfs = new_fileinfo_file_vfs;
    uint64_t newfile_identifier = new_fileinfo_file_identifier;

    DB_vfs_t *strfile_vfs = streamer_file_vfs;
    uint64_t strfile_identifier = streamer_file_identifier;
    streamer_unlock ();

    trace ("\033[0;33mstreamer_abort_files\033[37;0m\n");
    trace ("%lld %lld %lld\n", file_identifier, newfile_identifier, strfile_identifier);

    if (file_vfs && file_identifier) {
        vfs_abort_with_identifier (file_vfs, file_identifier);
    }
    if (newfile_vfs && newfile_identifier) {
        vfs_abort_with_identifier (newfile_vfs, newfile_identifier);
    }
    if (strfile_vfs && strfile_identifier) {
        vfs_abort_with_identifier (strfile_vfs, strfile_identifier);
    }
}

static void
send_songstarted (playItem_t *trk) {
    ddb_event_track_t *pev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_SONGSTARTED);
    pev->track = DB_PLAYITEM (trk);
    pl_item_ref (trk);
    pev->playtime = 0;
    pev->started_timestamp = time (NULL);
    messagepump_push_event ((ddb_event_t *)pev, 0, 0);
}

static void
send_songfinished (playItem_t *trk) {
    ddb_event_track_t *pev = (ddb_event_track_t *)messagepump_event_alloc (DB_EV_SONGFINISHED);
    pev->track = DB_PLAYITEM (trk);
    pl_item_ref (trk);
    pev->playtime = playtime;
    pev->started_timestamp = started_timestamp;
    messagepump_push_event ((ddb_event_t *)pev, 0, 0);
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
streamer_set_last_played (playItem_t *track) {
    streamer_lock ();
    if (last_played) {
        pl_item_unref (last_played);
    }
    last_played = track;
    if (last_played) {
        pl_item_ref (last_played);
    }
    streamer_unlock ();
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
        streamer_set_last_played (playing_track);

        playItem_t *qnext = playqueue_getnext ();
        if (qnext == playing_track) {
            playqueue_pop ();
        }
        if (qnext) {
            // playlist change?
            int idx = plt_get_item_idx (streamer_playlist, qnext, PL_MAIN);
            if (idx == -1) {
                playlist_t *plt = pl_get_playlist (qnext);
                if (plt != NULL) {
                    streamer_set_streamer_playlist (plt);
                    plt_unref (plt);
                }
            }
            pl_item_unref (qnext);
        }

        trace (
            "from=%p (%s), to=%p (%s) [2]\n",
            from,
            from ? pl_find_meta (from, ":URI") : "null",
            it,
            it ? pl_find_meta (it, ":URI") : "null");
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

static void
streamer_set_streaming_track (playItem_t *it) {
    streamer_lock ();
    if (streaming_track) {
        pl_item_unref (streaming_track);
    }
    streaming_track = it;
    if (streaming_track) {
        pl_item_ref (streaming_track);
    }
    streamer_unlock ();
}

playItem_t *
streamer_get_streaming_track (void) {
    if (streaming_track) {
        pl_item_ref (streaming_track);
    }
    return streaming_track;
}

playItem_t *
streamer_get_playing_track_unsafe (void) {
    playItem_t *it = (buffering_track && !playing_track) ? buffering_track : playing_track;
    if (it) {
        pl_item_ref (it);
    }
    return it;
}

playItem_t *
streamer_get_playing_track (void) {
    // some plugins may call this from plugin.start, before streamer is initialized
    if (mutex == 0) {
        return NULL;
    }
    streamer_lock ();
    playItem_t *it = streamer_get_playing_track_unsafe ();
    streamer_unlock ();
    return it;
}

void
streamer_set_playing_track (playItem_t *it) {
    if (it == playing_track) {
        return;
    }

    streamer_lock ();
    playItem_t *prev = playing_track;
    playing_track = it;
    streamer_unlock ();

    if (it) {
        pl_item_ref (it);
    }

    if (prev) {
        send_trackinfochanged (prev);
        pl_item_unref (prev);
    }

    if (it) {
        send_trackinfochanged (it);
    }
}

playItem_t *
streamer_get_buffering_track (void) {
    playItem_t *it = buffering_track;
    if (it) {
        pl_item_ref (it);
    }
    return it;
}

void
streamer_set_buffering_track (playItem_t *it) {
    streamer_lock ();
    if (it == buffering_track) {
        streamer_unlock ();
        return;
    }

    playItem_t *prev = buffering_track;

    buffering_track = it;
    if (buffering_track) {
        pl_item_ref (buffering_track);
    }
    streamer_unlock ();

    send_trackinfochanged (prev);

    if (it) {
        send_trackinfochanged (it);
    }

    if (prev) {
        pl_item_unref (prev);
    }
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
            messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
        }
        return 1;
    }

    if (pl_items_from_same_album (cur, next)) {
        return 0;
    }

    stream_track (NULL, 0);
    if (conf_get_int ("playlist.stop_after_album_reset", 0)) {
        conf_set_int ("playlist.stop_after_album", 0);
        stop_after_album = 0;
        messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
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
    int r = (int)(rand () / (double)RAND_MAX * cnt);
    if (r == curr) {
        r++;
        if (r >= cnt) {
            r = 0;
        }
    }

    return plt_get_item_for_idx (plt, r, PL_MAIN);
}

static playItem_t *
get_random_album (void) {
    if (!streamer_playlist) {
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
    }
    playlist_t *plt = streamer_playlist;
    int cnt = plt->count[PL_MAIN];
    if (!cnt) {
        return NULL;
    }
    playItem_t *prev = NULL;

    int album_cnt = 0;
    int album_buf_size = 1024;
    playItem_t **album_buf = malloc (sizeof (playItem_t *) * album_buf_size);
    playItem_t **album_buf2 = NULL;

    for (playItem_t *it = plt->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
        if (!prev || !pl_items_from_same_album (prev, it)) {
            album_buf[album_cnt] = it;
            album_cnt++;
            if (album_cnt == album_buf_size) {
                album_buf2 = malloc (sizeof (playItem_t *) * album_buf_size * 2);
                memcpy (album_buf2, album_buf, album_buf_size * sizeof (playItem_t *));
                album_buf_size *= 2;
                free (album_buf);
                album_buf = album_buf2;
            }
        }
        prev = it;
    }
    int r = (int)(rand () / (double)RAND_MAX * album_cnt);
    playItem_t *ret = album_buf[r];
    free (album_buf);
    pl_item_ref (ret);
    return ret;
}

static playItem_t *
_streamer_find_minimal_notplayed_imp (playlist_t *plt, unsigned int check_floor, int floor) {
    // if check_floor is truthy, such that shufflerating > floor
    playItem_t *pmin = NULL;
    for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
        if (!pl_get_played (i) && (!pmin || pl_get_shufflerating (i) < pl_get_shufflerating (pmin)) &&
            (!check_floor || floor < pl_get_shufflerating (i))) {
            pmin = i;
        }
    }
    return pmin;
}

static playItem_t *
_streamer_find_minimal_notplayed (playlist_t *plt) {
    return _streamer_find_minimal_notplayed_imp (plt, 0, 0);
}

static playItem_t *
_streamer_find_minimal_notplayed_with_floor (playlist_t *plt, int floor) {
    return _streamer_find_minimal_notplayed_imp (plt, 1, floor);
}

static playItem_t *
_streamer_find_maximal_played_imp (playlist_t *plt, unsigned int check_ceil, int ceil) {
    // if check_ceil is truthy, such that shufflerating < ceil
    playItem_t *pmax = NULL;
    for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
        if (pl_get_played (i) && (!pmax || pl_get_shufflerating (i) > pl_get_shufflerating (pmax)) &&
            (!check_ceil || pl_get_shufflerating (i) < ceil)) {
            pmax = i;
        }
    }
    return pmax;
}

static playItem_t *
_streamer_find_maximal_played (playlist_t *plt) {
    return _streamer_find_maximal_played_imp (plt, 0, 0);
}

static playItem_t *
_streamer_find_maximal_played_with_ceil (playlist_t *plt, int ceil) {
    return _streamer_find_maximal_played_imp (plt, 1, ceil);
}

static playItem_t *
get_next_track (playItem_t *curr, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    pl_lock ();

    if (next_track_to_play != NULL) {
        pl_item_ref (next_track_to_play);
        pl_unlock ();
        return next_track_to_play;
    }

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

    if (plt_get_item_idx (streamer_playlist, curr, PL_MAIN) == -1) {
        playlist_t *item_plt = pl_get_playlist (curr);
        if (!item_plt) {
            curr = NULL;
        }
        if (item_plt) {
            plt_unref (item_plt);
        }
    }

    if (shuffle == DDB_SHUFFLE_TRACKS || shuffle == DDB_SHUFFLE_ALBUMS) { // shuffle
        playItem_t *it = NULL;
        if (!curr || shuffle == DDB_SHUFFLE_TRACKS) {
            // find minimal notplayed
            it = _streamer_find_minimal_notplayed (plt);
            if (!it) {
                // all songs played, reshuffle and try again
                if (repeat == DDB_REPEAT_ALL) { // loop
                    plt_reshuffle (streamer_playlist, &it, NULL);
                }
            }
            if (!it) { // nothing found after reshuffle
                pl_unlock ();
                return NULL;
            }
        }
        else {
            int rating = pl_get_shufflerating (curr);
            // find minimal notplayed above *or equal to* (hence the -1) current
            it = _streamer_find_minimal_notplayed_with_floor (plt, rating - 1);
            if (!it) {
                // all songs played, reshuffle and try again
                if (repeat == DDB_REPEAT_ALL) { // loop
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
    else if (shuffle == DDB_SHUFFLE_OFF) { // linear
        playItem_t *it = NULL;
        if (curr) {
            it = curr->next[PL_MAIN];
        }
        else {
            it = plt->head[PL_MAIN];
        }
        if (!it) {
            trace ("streamer_move_nextsong: was last track\n");
            if (repeat == DDB_REPEAT_ALL) {
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
    else if (shuffle == DDB_SHUFFLE_RANDOM) { // random
        pl_unlock ();
        return get_random_track ();
    }
    pl_unlock ();
    return NULL;
}

static playItem_t *
get_prev_track (playItem_t *curr, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    pl_lock ();

    if (prev_track_to_play != NULL) {
        pl_item_ref (prev_track_to_play);
        pl_unlock ();
        return prev_track_to_play;
    }

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
    if (shuffle == DDB_SHUFFLE_TRACKS || shuffle == DDB_SHUFFLE_ALBUMS) { // shuffle
        if (!curr) {
            playItem_t *it = plt->head[PL_MAIN];
            pl_item_ref (it);
            pl_unlock ();
            return it;
        }
        else {
            pl_set_played (curr, 0);
            // find already played song with maximum shuffle rating below prev song
            int rating = pl_get_shufflerating (curr);
            playItem_t *pmax = NULL; // played maximum
            playItem_t *amax = NULL; // absolute maximum
            for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                int played = pl_get_played (i);
                if (i != curr && played && (!amax || pl_get_shufflerating (i) > pl_get_shufflerating (amax))) {
                    amax = i;
                }
                if (i == curr || pl_get_shufflerating (i) > rating || !played) {
                    continue;
                }
                if (!pmax || pl_get_shufflerating (i) > pl_get_shufflerating (pmax)) {
                    pmax = i;
                }
            }

            if (pmax && shuffle == DDB_SHUFFLE_ALBUMS) {
                while (pmax && pmax->next[PL_MAIN] && pl_get_played (pmax->next[PL_MAIN]) &&
                       pl_get_shufflerating (pmax) == pl_get_shufflerating (pmax->next[PL_MAIN])) {
                    pmax = pmax->next[PL_MAIN];
                }
            }

            playItem_t *it = pmax;
            if (!it) {
                // that means 1st in playlist, take amax
                if (repeat == DDB_REPEAT_ALL) {
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
    else if (shuffle == DDB_SHUFFLE_OFF) { // linear
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
        pl_item_ref (it);
        pl_unlock ();
        return it;
    }
    else if (shuffle == DDB_SHUFFLE_RANDOM) { // random
        pl_unlock ();
        return get_random_track ();
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

static void
streamer_set_next_track_to_play (playItem_t *next) {
    if (next_track_to_play != NULL) {
        pl_item_unref (next_track_to_play);
        next_track_to_play = NULL;
    }
    next_track_to_play = next;
    if (next_track_to_play) {
        pl_item_ref (next_track_to_play);
    }
}

static void
streamer_set_prev_track_to_play (playItem_t *prev) {
    if (prev_track_to_play != NULL) {
        pl_item_unref (prev_track_to_play);
        prev_track_to_play = NULL;
    }
    prev_track_to_play = prev;
    if (prev_track_to_play) {
        pl_item_ref (prev_track_to_play);
    }
}

int
streamer_move_to_nextalbum (int r) {
    if (r) {
        streamer_abort_files ();
    }
    handler_push (handler, STR_EV_NEXT_ALBUM, 0, r, 0);
    return 0;
}

int
streamer_move_to_prevalbum (int r) {
    if (r) {
        streamer_abort_files ();
    }
    handler_push (handler, STR_EV_PREV_ALBUM, 0, r, 0);
    return 0;
}

int
streamer_move_to_randomalbum (int r) {
    if (r) {
        streamer_abort_files ();
    }
    handler_push (handler, STR_EV_RAND_ALBUM, 0, r, 0);
    return 0;
}

// playlist must call that whenever item was removed
void
streamer_song_removed_notify (playItem_t *it) {
    if (!mutex) {
        return; // streamer is not running
    }
    streamer_lock ();

    playItem_t *next = NULL;
    playItem_t *prev = NULL;

    if (streaming_track == it || next_track_to_play == it) {
        ddb_shuffle_t shuffle = streamer_get_shuffle ();
        ddb_repeat_t repeat = streamer_get_repeat ();
        streamer_set_next_track_to_play (NULL);
        next = get_next_track (it, shuffle, repeat);
        if (next == it) {
            pl_item_unref(next);
            next = NULL;
        }
        prev = get_prev_track (it, shuffle, repeat);
        if (prev == it) {
            pl_item_unref(prev);
            prev = NULL;
        }
        streamer_set_next_track_to_play (next);
        streamer_set_prev_track_to_play (prev);
    }

    if (next != NULL) {
        pl_item_unref (next);
    }
    if (prev != NULL) {
        pl_item_unref (prev);
    }

    streamer_unlock ();
}

static ddb_ctmap_t *streamer_ctmap;
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

static DB_fileinfo_t *
dec_open (DB_decoder_t *dec, uint32_t hints, playItem_t *it) {
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
    streamer_lock ();
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
        fileinfo_curr = calloc (1, sizeof (DB_fileinfo_t));

        fileinfo_file_vfs = NULL;
        fileinfo_file_identifier = 0;

        new_fileinfo = NULL;
        new_fileinfo_file_vfs = NULL;
        new_fileinfo_file_identifier = 0;

        streamer_set_streaming_track (failed_track);
    }
    streamer_unlock ();
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
    streamer_lock ();
    if (fileinfo_curr) {
        fileinfo_free (fileinfo_curr);
        fileinfo_curr = NULL;
        fileinfo_file_vfs = NULL;
        fileinfo_file_identifier = 0;
    }
    streamer_unlock ();
    trace ("stream_track %s\n", playing_track ? pl_find_meta (playing_track, ":URI") : "null");
    int err = 0;
    playItem_t *from = NULL;
    playItem_t *to = NULL;

    if (first_failed_track && first_failed_track == it) {
        // looped to the first failed track
        _handle_playback_stopped ();
        goto error;
    }

    // need to add refs here, because streamer_start_playback can destroy items
    from = playing_track;
    to = it;
    if (from) {
        pl_item_ref (from);
    }
    if (to) {
        pl_set_played (to, 1);
        pl_item_ref (to);
    }

    streamer_set_streaming_track (NULL);

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
    const char *dec_id = pl_find_meta (it, ":DECODER");
    if (dec_id) {
        strncpy (decoder_id, dec_id, sizeof (decoder_id));
    }

    if (!decoder_id[0]) {
        // some decoders set filetype override,
        // but the override is invalid when decoder is not set.
        // reset to default here, so that tracks become playable after failures
        pl_delete_meta (it, "!FILETYPE");
    }

    const char *ft = pl_find_meta (it, ":FILETYPE");
    if (ft) {
        strncpy (filetype, ft, sizeof (filetype));
    }
    pl_unlock ();
    char *cct = "undefined";
    char *plugs[DDB_CTMAP_MAX_PLUGINS] = { NULL };
    if (!decoder_id[0] && (!strcmp (filetype, "content") || !filetype[0])) {
        // try to get content-type
        trace ("\033[0;34mopening file %s\033[37;0m\n", pl_find_meta (it, ":URI"));
        pl_lock ();
        char *uri = strdupa (pl_find_meta (it, ":URI"));
        pl_unlock ();
        DB_FILE *fp = vfs_fopen (uri);
        trace ("\033[0;34mgetting content-type\033[37;0m\n");
        if (!fp) {
            err = -1;
            goto error;
        }
        streamer_file_vfs = fp->vfs;
        streamer_file_identifier = vfs_get_identifier (fp);

        const char *ct = vfs_get_content_type (fp);
        if (!ct) {
            vfs_fclose (fp);
            fp = NULL;

            streamer_file_vfs = NULL;
            streamer_file_identifier = 0;

            if (!startpaused) {
                streamer_play_failed (it);
            }
            else {
                err = -1;
            }

            pl_lock ();
            trace_err ("Failed to play track: %s\n", pl_find_meta (it, ":URI"));
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
        ddb_ctmap_t *ctmap = streamer_ctmap;
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

        if (!plugs[0] && (!strcmp (cct, "audio/x-mpegurl") || !strncmp (cct, "text/html", 9) ||
                          !strncmp (cct, "audio/x-scpls", 13) || !strncmp (cct, "application/octet-stream", 9))) {
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
            usleep (conf_get_int ("streamer.wait_ms_after_m3u_link", 400000));

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
            if (buf) {
                free (buf);
            }
            if (out) {
                fclose (out);
            }
            else if (fd != -1) {
                close (fd);
            }

            if (!startpaused) {
                streamer_play_failed (it);
            }
            else {
                err = -1;
            }

            pl_lock ();
            trace_err ("Failed to play track: %s\n", pl_find_meta (it, ":URI"));
            pl_unlock ();

            goto error;
        }

        streamer_file_vfs = NULL;
        streamer_file_identifier = 0;

        vfs_fclose (fp);
    }

    int plug_idx = 0;
    for (;;) {
        if (!decoder_id[0] && plugs[0] && !plugs[plug_idx]) {
            pl_set_played (it, 1);
            trace_err (
                "No suitable decoder found for stream %s of content-type %s\n",
                pl_find_meta (playing_track, ":URI"),
                cct);

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
                                    fprintf (
                                        stderr,
                                        "streamer: %s : changed decoder plugin to %s\n",
                                        fname,
                                        decs[i]->plugin.id);
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

            if (!startpaused) {
                streamer_play_failed (it);
            }
            else {
                err = -1;
            }

            pl_lock ();
            trace_err ("Failed to play track: %s\n", pl_find_meta (it, ":URI"));
            pl_unlock ();

            goto error;
        }

        trace ("\033[0;33minit decoder for %s (%s)\033[37;0m\n", pl_find_meta (it, ":URI"), dec->plugin.id);
        DB_fileinfo_t *temp_fileinfo = dec_open (dec, STREAMER_HINTS, it);
        streamer_lock ();
        new_fileinfo = temp_fileinfo;
        if (new_fileinfo && new_fileinfo->file) {
            new_fileinfo_file_vfs = new_fileinfo->file->vfs;
            new_fileinfo_file_identifier = vfs_get_identifier (new_fileinfo->file);
        }
        else {
            new_fileinfo_file_vfs = NULL;
            new_fileinfo_file_identifier = 0;
        }
        int have_new_fileinfo = new_fileinfo != NULL;
        streamer_unlock ();
        if (have_new_fileinfo && dec->init (new_fileinfo, DB_PLAYITEM (it)) != 0) {
            trace ("\033[0;31mfailed to init decoder\033[37;0m\n");
            pl_delete_meta (it, "!DECODER");
            streamer_lock ();
            dec->free (new_fileinfo);
            new_fileinfo = NULL;
            new_fileinfo_file_vfs = NULL;
            new_fileinfo_file_identifier = 0;
            have_new_fileinfo = 0;
            streamer_unlock ();
        }

        if (!have_new_fileinfo) {
            trace ("decoder %s failed\n", dec->plugin.id);
            continue;
        }
        else {
            streamer_lock ();
            if (new_fileinfo->file) {
                new_fileinfo_file_vfs = new_fileinfo->file->vfs;
                new_fileinfo_file_identifier = vfs_get_identifier (new_fileinfo->file);
            }
            else {
                new_fileinfo_file_vfs = NULL;
                new_fileinfo_file_identifier = 0;
            }

            streamer_set_streaming_track (it);
            streamer_unlock ();

            trace (
                "bps=%d, channels=%d, samplerate=%d\n",
                new_fileinfo->fmt.bps,
                new_fileinfo->fmt.channels,
                new_fileinfo->fmt.samplerate);
            break;
        }
    }
success:
    streamer_play_failed (NULL);
    streamer_lock ();
    if (new_fileinfo) {
        fileinfo_curr = new_fileinfo;
        new_fileinfo = NULL;
        new_fileinfo_file_vfs = NULL;
        new_fileinfo_file_identifier = 0;
    }
    streamer_unlock ();

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
    streamer_lock ();
    float seek = last_seekpos;
    if (seek >= 0) {
        streamer_unlock ();
        return seek;
    }
    float ret = playpos;
    streamer_unlock ();
    return ret;
}

int
streamer_get_apx_bitrate (void) {
    streamer_lock ();
    int res = avg_bitrate;
    streamer_unlock ();
    return res;
}

void
streamer_set_nextsong (int song, int startpaused) {
    streamer_abort_files ();
    if (song == -1) {
        // this is a stop query -- clear the queue
        handler_reset (handler);
    }
    handler_push (handler, STR_EV_PLAY_TRACK_IDX, 0, song, startpaused);
}

void
streamer_set_seek (float pos) {
    streamer_lock ();
    last_seekpos = pos;
    streamer_unlock ();
    handler_push (handler, STR_EV_SEEK, 0, *((uint32_t *)&pos), 0);
}

static void
update_stop_after_current (void) {
    if (conf_get_int ("playlist.stop_after_current_reset", 0)) {
        conf_set_int ("playlist.stop_after_current", 0);
        stop_after_current = 0;
        messagepump_push (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }
}

static void
streamer_next (ddb_shuffle_t shuffle, ddb_repeat_t repeat, playItem_t *next) {
    if (playing_track) {
        if (repeat == DDB_REPEAT_SINGLE) { // song finished, loop mode is "loop 1 track"
            next = playing_track;
        }
    }
    if (!next) {
        next = get_next_track (streaming_track, shuffle, repeat);
    }
    else {
        pl_item_ref (next);
    }
    stream_track (next, 0);
    if (next) {
        pl_item_unref (next);
    }
}

static void
streamer_shuffle_changed (ddb_shuffle_t prev, ddb_shuffle_t shuffle);

static void
streamer_seek_real (float seekpos) {
    // Some fileinfos can exist without plugin bound to them,
    // for example when a track failed to play.
    // Don't attempt seeking in them.
    if (!fileinfo_curr || !fileinfo_curr->plugin) {
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

        streamer_lock ();
        playpos = seek;
        streamer_unlock ();
        trace ("seeking to %f\n", seek);

        if (track == playing_track && track != streaming_track) {
            // restart streaming the playing track
            if (stream_track (playing_track, 0) < 0) {
                streamer_move_to_nextsong (0);
                return;
            }
        }

        if (fileinfo_curr && track && dur > 0) {
            streamer_lock ();
            float seek_to_playpos = playpos;
            streamer_unlock ();

            if (fileinfo_curr->plugin->seek (fileinfo_curr, seek_to_playpos) >= 0) {
                streamer_reset (1);
            }
            streamer_lock ();
            playpos = fileinfo_curr->readpos;
            avg_bitrate = -1;
            streamer_unlock ();
        }
        ddb_event_playpos_t *ev = (ddb_event_playpos_t *)messagepump_event_alloc (DB_EV_SEEKED);
        ev->track = DB_PLAYITEM (track);
        if (track) {
            pl_item_ref (track);
        }
        ev->playpos = playpos;
        messagepump_push_event ((ddb_event_t *)ev, 0, 0);
    }
    streamer_lock ();
    last_seekpos = -1;
    streamer_unlock ();
}

static void
_update_buffering_state (void) {
    streamer_lock ();
    int blocks_ready = streamreader_num_blocks_ready ();
    streamer_unlock ();
    int buffering = (blocks_ready < 4) && streaming_track;

    if (buffering != streamer_is_buffering) {
        streamer_lock ();
        streamer_is_buffering = buffering;
        streamer_unlock ();

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
        streamer_lock ();
        playpos = 0;
        streamer_unlock ();
    }

    streamer_start_playback (from, track);

    // only reset playpos/bitrate if track changing to another,
    // otherwise the track is the first one, and playpos is pre-set
    playtime = 0;
    avg_bitrate = -1;
    streamer_lock ();
    last_seekpos = -1;
    streamer_unlock ();
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
    if (bit_override == 1) {
        out_fmt->bps = 8;
        out_fmt->is_float = 0;
    }
    else if (bit_override == 2) {
        out_fmt->bps = 16;
        out_fmt->is_float = 0;
    }
    else if (bit_override == 3) {
        out_fmt->bps = 24;
        out_fmt->is_float = 0;
    }
    else if (bit_override == 4) {
        out_fmt->bps = 32;
        out_fmt->is_float = 0;
    }
    else if (bit_override == 5) {
        out_fmt->bps = 32;
        out_fmt->is_float = 1;
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

static void
_streamer_requeue_after_current (ddb_repeat_t repeat, ddb_shuffle_t shuffle) {
    if (!playing_track) {
        return;
    }
    streamer_lock ();
    streamreader_flush_after (playing_track);

    if (playing_track == streaming_track) {
        streamer_unlock ();
        return;
    }

    streamer_set_streaming_track (playing_track);
    streamer_unlock ();
    streamer_next (shuffle, repeat, NULL);
}

static void
_streamer_track_deleted (ddb_repeat_t repeat, ddb_shuffle_t shuffle) {
    // cancel buffering of next track, if it's not in playlist anymore

    if (!streaming_track) {
        return;
    }

    if (playing_track && streaming_track == playing_track) {
        return;
    }

    playlist_t *plt = pl_get_playlist (streaming_track);
    if (!plt) {
        _streamer_requeue_after_current (repeat, shuffle);
    }
    else {
        plt_unref (plt);
    }
}

void
streamer_thread (void *unused) {
#if defined(__linux__) && !defined(ANDROID)
    prctl (PR_SET_NAME, "deadbeef-stream", 0, 0, 0, 0);
#endif

    ddb_shuffle_t shuffle = (ddb_shuffle_t)-1;
    ddb_repeat_t repeat = (ddb_repeat_t)-1;

    uint32_t id;
    uintptr_t ctx;
    uint32_t p1, p2;

    ddb_waveformat_t prev_block_fmt = { 0 };
    double _add_format_silence = .0;

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
                play_next (1, shuffle, repeat);
                break;
            case STR_EV_PREV:
                play_next (-1, shuffle, repeat);
                break;
            case STR_EV_RAND:
                play_next (0, shuffle, repeat);
                break;
            case STR_EV_RAND_ALBUM:
                play_next_album (0, shuffle, repeat);
                break;
            case STR_EV_NEXT_ALBUM:
                play_next_album (1, shuffle, repeat);
                break;
            case STR_EV_PREV_ALBUM:
                play_next_album (-1, shuffle, repeat);
                break;
            case STR_EV_SEEK:
                streamer_seek_real (*((float *)&p1));
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
            case STR_EV_TRACK_DELETED:
                _streamer_track_deleted (repeat, shuffle);
                break;
            }
        }

        // each event can modify shuffle/repeat, so update them here, so that subsequent event handlers use new values
        ddb_shuffle_t new_shuffle = streamer_get_shuffle ();
        ddb_repeat_t new_repeat = streamer_get_repeat ();

        int need_requeue = 0;
        if (new_shuffle != shuffle || new_repeat != repeat) {
            need_requeue = 1;
        }

        if (new_shuffle != shuffle) {
            pl_reshuffle_all ();
            streamer_shuffle_changed (shuffle, new_shuffle);
            shuffle = new_shuffle;
        }

        repeat = new_repeat;

        if (need_requeue) {
            _streamer_requeue_after_current (repeat, shuffle);
        }

        if (output->state () == DDB_PLAYBACK_STATE_STOPPED) {
            if (!handler_hasmessages (handler)) {
                usleep (50000);
            }
            continue;
        }

        _update_buffering_state ();

        if (!fileinfo_curr) {
            // HACK: This is to overcome the output plugin API limitation.
            // We count the number of times the output plugin has starved,
            // and stop playback after counter reaches the limit.
            // The correct way to solve this is to add a `drain` API.
            if (_audio_stall_count >= AUDIO_STALL_WAIT) {
                output->stop ();
                streamer_lock ();
                _handle_playback_stopped ();
                _audio_stall_count = 0;
                streamer_unlock ();
                continue;
            }
            usleep (50000); // nothing is streaming -- about to stop
            continue;
        }

        streamer_lock ();
        streamblock_t *block = streamreader_get_next_block ();
        streamer_unlock ();

        if (!block) {
            usleep (50000); // all blocks are full
            continue;
        }

        int res = 0;

        // insert silence at the format change
        if (memcmp (&fileinfo_curr->fmt, &prev_block_fmt, sizeof (ddb_waveformat_t)) && conf_format_silence > 0) {
            memcpy (&prev_block_fmt, &fileinfo_curr->fmt, sizeof (ddb_waveformat_t));

            _add_format_silence = conf_format_silence;
        }
        if (_add_format_silence > 0) {
            res = streamreader_silence_block (block, streaming_track, fileinfo_curr, mutex);
            int bytes_per_sec =
                fileinfo_curr->fmt.samplerate * fileinfo_curr->fmt.channels * (fileinfo_curr->fmt.bps / 8);
            _add_format_silence -= block->size / (double)bytes_per_sec;
        }
        else {
            res = streamreader_read_block (block, streaming_track, fileinfo_curr, mutex);
        }

        // streamreader has locked the mutex on success
        int last = 0;

        if (res >= 0) {
            streamreader_enqueue_block (block);
            last = block->last;
            streamer_unlock ();
        }

        if (res < 0 || last) {
            // error or eof

            playItem_t *next = NULL;

            // handle stop after current
            int stop = 0;
            if (block->last) {
                if (stop_after_current) {
                    stop = 1;
                }
                else {
                    next = get_next_track (streaming_track, shuffle, repeat);

                    if (stop_after_album_check (streaming_track, next)) {
                        stop = 1;
                    }

                    if (next) {
                        pl_item_unref (next);
                    }
                }
            }

            if (stop) {
                stream_track (NULL, 0);
            }
            else {
                streamer_next (shuffle, repeat, next);
            }
        }
    }

    // drain event queue
    while (!handler_pop (handler, &id, &ctx, &p1, &p2))
        ;

    // stop streaming song
    streamer_lock ();
    if (fileinfo_curr) {
        fileinfo_free (fileinfo_curr);
        fileinfo_curr = NULL;
        fileinfo_file_vfs = NULL;
        fileinfo_file_identifier = 0;
    }
    streamer_set_streaming_track (NULL);
    streamer_set_playing_track (NULL);
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

    viz_init ();

    streamreader_init ();
    decoded_blocks_init ();

    streamer_dsp_init ();

    ctmap_init_mutex ();
    conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, conf_network_ctmapping, sizeof (conf_network_ctmapping));
    ddb_ctmap_free (streamer_ctmap);
    streamer_ctmap = NULL;
    streamer_ctmap = ddb_ctmap_init_from_string (conf_network_ctmapping);

    _artist_tf = tf_compile ("%album artist%");
    _album_tf = tf_compile ("%album%");

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

    tf_free (_album_tf);
    _album_tf = NULL;
    tf_free (_artist_tf);
    _artist_tf = NULL;

    streamreader_free ();
    decoded_blocks_free ();

    if (first_failed_track) {
        pl_item_unref (first_failed_track);
        first_failed_track = NULL;
    }
    streamer_set_streaming_track (NULL);
    streamer_set_next_track_to_play (NULL);
    streamer_set_prev_track_to_play (NULL);
    streamer_set_playing_track (NULL);
    streamer_set_buffering_track (NULL);
    streamer_set_last_played (NULL);
    streamer_set_streamer_playlist (NULL);

    ddb_ctmap_free (streamer_ctmap);
    streamer_ctmap = NULL;
    ctmap_free_mutex ();

    mutex_free (mutex);
    mutex = 0;
    viz_free ();
    fft_free ();

    streamer_dsp_chain_save ();

    dsp_free ();

    if (handler) {
        handler_free (handler);
        handler = NULL;
    }

    playpos = 0;
    playtime = 0;

    ringbuf_deinit (&_output_ringbuf);
    free (_int_output_buffer);
    _int_output_buffer = NULL;

    resizable_buffer_deinit (&_dsp_process_buffer);
    resizable_buffer_deinit (&_viz_read_buffer);
}

void
streamer_reset (int full) { // must be called when current song changes by external reasons
    if (!mutex) {
        fprintf (stderr, "ERROR: someone called streamer_reset after exit\n");
        return; // failsafe, in case someone calls streamer reset after deinit
    }

    streamer_lock ();
    streamreader_reset ();
    decoded_blocks_reset ();
    dsp_reset ();
    ringbuf_flush (&_output_ringbuf);
    streamer_unlock ();
    viz_reset ();
}

static int
process_output_block (streamblock_t *block, char *bytes, int bytes_available_size) {
    DB_output_t *output = plug_get_output ();

    if (block->pos < 0) {
        return 0;
    }

    // A block with 0 size is a valid block, and needs to be processed as usual (code above this line).
    // But here we do early exit, because there's no data to process in it.
    if (!block->size) {
        decoded_block_t *decoded_block = decoded_blocks_append ();
        if (decoded_block != NULL) {
            decoded_block->track = block->track;
            if (decoded_block->track != NULL) {
                pl_item_ref (decoded_block->track);
            }
            decoded_block->last = block->last;
            decoded_block->first = block->first;
        }

        streamreader_next_block ();
        _update_buffering_state ();
        return 0;
    }

    decoded_block_t *decoded_block = decoded_blocks_append ();
    if (decoded_block == NULL) {
        return 0; // queue is full!
    }

    assert (block->size > block->pos);
    int sz = block->size - block->pos;

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
    if (block->fmt.bps != 16) {
        temp_audio_data = alloca (tempsize);
        ddb_waveformat_t out_fmt = {
            .bps = 16,
            .channels = block->fmt.channels,
            .samplerate = block->fmt.samplerate,
            .channelmask = block->fmt.channelmask,
            .is_float = 0,
        };

        pcm_convert (&block->fmt, (char *)input, &out_fmt, (char *)temp_audio_data, sz);
        input = (char *)temp_audio_data;
        memcpy (&datafmt, &out_fmt, sizeof (ddb_waveformat_t));
        sz = tempsize;
    }

    extern void android_eq_apply (char *dspbytes, int dspsize);
    android_eq_apply (input, sz);

    dsp_apply_simple_downsampler (
        datafmt.samplerate,
        datafmt.channels,
        input,
        sz,
        output->fmt.samplerate,
        &dspbytes,
        &dspsize);
    datafmt.samplerate = output->fmt.samplerate;
    sz = dspsize;
#else
    int dsp_res = dsp_apply (&block->fmt, block->buf + block->pos, sz, &datafmt, &dspbytes, &dspsize, &dspratio);
    if (dsp_res) {
        sz = dspsize;
    }
    else {
        memcpy (&datafmt, &block->fmt, sizeof (ddb_waveformat_t));
        dspbytes = block->buf + block->pos;
    }
#endif

    int need_convert = memcmp (&output->fmt, &datafmt, sizeof (ddb_waveformat_t));
    int required_size = 0;
    if (need_convert) {
        int input_ss = datafmt.channels * datafmt.bps / 8;
        int output_ss = output->fmt.channels * output->fmt.bps / 8;
        required_size = sz / input_ss * output_ss;
    }
    else {
        required_size = sz;
    }

    // Crash here to catch the buffer issues early, instead of corrupting sound.
    assert (bytes_available_size >= required_size);

    if (need_convert) {
        sz = pcm_convert (&datafmt, dspbytes, &output->fmt, bytes, sz);
    }
    else {
        memcpy (bytes, dspbytes, sz);
    }

    streamer_lock ();
    decoded_block->track = block->track;
    if (decoded_block->track != NULL) {
        pl_item_ref (decoded_block->track);
    }
    decoded_block->last = block->last;
    decoded_block->first = block->first;
    decoded_block->total_bytes = decoded_block->remaining_bytes = sz;
    decoded_block->is_silent_header = block->is_silent_header;
    decoded_block->playback_time =
        (float)sz / output->fmt.samplerate / ((output->fmt.bps >> 3) * output->fmt.channels) * dspratio;

    block->pos = block->size;
    streamreader_next_block ();
    streamer_unlock ();

    _update_buffering_state ();

    return sz;
}

static float (*streamer_volume_modifier) (float delta_time);

// used in android branch, do not delete
void
streamer_set_volume_modifier (float (*modifier) (float delta_time)) {
    streamer_volume_modifier = modifier;
}

static void
streamer_apply_soft_volume (char *bytes, int sz) {
    if (audio_is_mute ()) {
        memset (bytes, 0, sz);
        return;
    }
    DB_output_t *output = plug_get_output ();
    if (output->has_volume) {
        return;
    }

    if (output->fmt.flags & DDB_WAVEFORMAT_FLAG_IS_DOP) {
        return;
    }

    float mod = 1.f;

    if (streamer_volume_modifier) {
        float dt = sz * (output->fmt.bps >> 3) / output->fmt.channels / (float)output->fmt.samplerate;
        mod = streamer_volume_modifier (dt);
    }

    float vol = volume_get_amp () * mod;

    char *stream = bytes;
    int bytesread = sz;
    if (output->fmt.bps == 16) {
        int16_t ivolume = (int16_t)(vol * 1000);
        if (ivolume != 1000) {
            int half = bytesread / 2;
            for (int i = 0; i < half; i++) {
                int16_t sample = *((int16_t *)stream);
                *((int16_t *)stream) = (int16_t)(((int32_t)sample) * ivolume / 1000);
                stream += 2;
            }
        }
    }
    else if (output->fmt.bps == 8) {
        int16_t ivolume = (int16_t)(vol * 255);
        if (ivolume != 255) {
            for (int i = 0; i < bytesread; i++) {
                *stream = (int8_t)(((int32_t)(*stream)) * ivolume / 255);
                stream++;
            }
        }
    }
    else if (output->fmt.bps == 24) {
        int16_t ivolume = (int16_t)(vol * 1000);
        if (ivolume != 1000) {
            int third = bytesread / 3;
            for (int i = 0; i < third; i++) {
                int32_t sample =
                    ((unsigned char)stream[0]) | ((unsigned char)stream[1] << 8) | ((signed char)stream[2] << 16);
                int32_t newsample = (int32_t)((int64_t)sample * ivolume / 1000);
                stream[0] = (newsample & 0x0000ff);
                stream[1] = (newsample & 0x00ff00) >> 8;
                stream[2] = (newsample & 0xff0000) >> 16;
                stream += 3;
            }
        }
    }
    else if (output->fmt.bps == 32 && !output->fmt.is_float) {
        int16_t ivolume = (int16_t)(vol * 1000);
        if (ivolume != 1000) {
            for (int i = 0; i < bytesread / 4; i++) {
                int32_t sample = *((int32_t *)stream);
                int32_t newsample = (int32_t)((int64_t)sample * ivolume / 1000);
                *((int32_t *)stream) = newsample;
                stream += 4;
            }
        }
    }
    else if (output->fmt.bps == 32 && output->fmt.is_float) {
        if (vol != 1.f) {
            for (int i = 0; i < bytesread / 4; i++) {
                *((float *)stream) = (*((float *)stream)) * vol;
                stream += 4;
            }
        }
    }
}

static int
_streamer_get_bytes (char *bytes, int size) {
    DB_output_t *output = plug_get_output ();

    // consume decoded data
    int sz = size;
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

    int rb = sz;
    char *writeptr = bytes;

    // streamer_reset may clear all blocks, therefore need a lock
    streamer_lock ();
    while (rb > 0) {
        decoded_block_t *decoded_block = decoded_blocks_current ();
        if (decoded_block == NULL) {
            break;
        }

        // handle change of track
        if (decoded_block->first) {
            handle_track_change (playing_track, decoded_block->track);
        }

        if (decoded_block->remaining_bytes != 0) {
            size_t got_bytes = min (rb, decoded_block->remaining_bytes);
            got_bytes = ringbuf_read (&_output_ringbuf, writeptr, got_bytes);
            writeptr += got_bytes;
            rb -= got_bytes;

            decoded_block->remaining_bytes -= got_bytes;
        }

        if (decoded_block->remaining_bytes == 0) {
            if (decoded_block->last) {
                update_stop_after_current ();
            }

            if (!decoded_block->is_silent_header) {
                playpos += decoded_block->playback_time;
                playtime += decoded_block->playback_time;
            }

            decoded_blocks_next ();
        }
    }

    sz -= rb; // how many bytes we actually got

    streamer_unlock ();

    streamer_apply_soft_volume (bytes, sz);

    return sz;
}

// @return latency buffer size
static size_t
_output_ringbuf_setup (const ddb_waveformat_t *fmt) {
    // Need to be able to upsample from 8000 mono to the current format.
    // Add some padding to allow multiple blocks to be decoded.
    // FIXME: this could be improved by walking the current dsp chain, and calculating the real ratio.
    size_t size = (size_t)(16384 * 1.5 * MAX_DSP_RATIO);
    size_t latency = 0;
#ifdef __APPLE__
    // add 3 seconds of history for airplay latency / visualization compensation
    latency = 3 * fmt->channels * fmt->samplerate * fmt->bps / 8;
#endif
    size += latency;

    if (size != _output_ringbuf.size) {
        free (_int_output_buffer);
        _int_output_buffer = malloc (size);
        ringbuf_init (&_output_ringbuf, _int_output_buffer, size);
        decoded_blocks_reset ();
    }

    return latency;
}

// Decode enough blocks to fill the _output_buffer, and update avg_bitrate
static void
_streamer_fill_playback_buffer (void) {
    streamer_lock ();
    streamblock_t *block = streamreader_get_curr_block ();
    if (!block) {
        // NULL streaming_track means playback stopped,
        // otherwise just a buffer starvation (e.g. after seeking)
        if (!streaming_track) {
            update_stop_after_current ();
            _handle_playback_stopped ();
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
        streamer_unlock ();

        if (streaming_track) {
            return;
        }
        _audio_stall_count++;
        return;
    }

    _audio_stall_count = 0;

    int block_bitrate = -1;

    // only decode until the next format change
    // decode enough blocks to fill the output buffer
    resizable_buffer_ensure_size (&_dsp_process_buffer, block->size * MAX_DSP_RATIO);

    size_t latency = _output_ringbuf_setup (&plug_get_output ()->fmt);

    while (block != NULL && decoded_blocks_have_free () &&
           decoded_blocks_playback_time_total () < conf_playback_buffer_size &&
           (_output_ringbuf.size - _output_ringbuf.remaining - latency) >= block->size * MAX_DSP_RATIO &&
           !memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
        int rb = process_output_block (block, _dsp_process_buffer.buffer, block->size * MAX_DSP_RATIO);
        if (rb <= 0) {
            break;
        }
        ringbuf_write (&_output_ringbuf, _dsp_process_buffer.buffer, rb);

        block_bitrate = block->bitrate;
        block = streamreader_get_curr_block ();
    }
    // empty buffer and the next block format differs? request format change!

    if (_output_ringbuf.remaining == 0 && block && memcmp (&block->fmt, &last_block_fmt, sizeof (ddb_waveformat_t))) {
        streamer_set_output_format (&block->fmt);
        memcpy (&last_block_fmt, &block->fmt, sizeof (ddb_waveformat_t));

        streamer_unlock ();
        return;
    }

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
    streamer_unlock ();
}

int
streamer_read (char *bytes, int size) {
    DB_output_t *output = plug_get_output ();

    // Ensure that the buffer has enough data for analysis
    int ss = output->fmt.channels * output->fmt.bps / 8;
    int max_bytes = output->fmt.samplerate * ss;

    // Read into the output buffer
    _streamer_fill_playback_buffer ();

    // Process
#ifndef ANDROID
    // Read extra bytes from output buffer
    size_t viz_bytes = min (_output_ringbuf.size, max_bytes);
    int wave_size = size / ss;
    resizable_buffer_ensure_size (&_viz_read_buffer, max_bytes);
    size_t offset = 0;

#    ifdef __APPLE__
    const int AIRPLAY_LATENCY = 2;
    if (output->plugin.flags & DDB_COREAUDIO_FLAG_AIRPLAY) {
        offset = AIRPLAY_LATENCY * output->fmt.samplerate * output->fmt.channels * output->fmt.bps / 8;
    }
#    endif
    ringbuf_read_keep_offset (&_output_ringbuf, _viz_read_buffer.buffer, viz_bytes, -offset);
    viz_process (
        _viz_read_buffer.buffer,
        (int)viz_bytes,
        output,
        4096,
        wave_size); // FIXME: fft size needs to be configurable
#endif

    // Play
    return _streamer_get_bytes (bytes, size);
}

int
streamer_ok_to_read (int len) {
    streamer_lock ();
    int res = !streamer_is_buffering;
    streamer_unlock ();
    return res;
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

    ddb_shuffle_t shuffle = conf_get_int ("playback.order", DDB_SHUFFLE_OFF);
    ddb_repeat_t repeat = conf_get_int ("playback.loop", DDB_REPEAT_OFF);

    // legacy handling, in case repeat/shuffle values change via config directly
    streamer_set_shuffle (shuffle);
    streamer_set_repeat (repeat);

    if (playing_track) {
        pl_set_played (playing_track, 1);
    }

    int formatchanged = 0;

    int conf_autoconv_8_to_16 = conf_get_int ("streamer.8_to_16", 1);
    if (conf_autoconv_8_to_16 != autoconv_8_to_16) {
        autoconv_8_to_16 = conf_autoconv_8_to_16;
        formatchanged = 1;
    }
    int conf_autoconv_16_to_24 = conf_get_int ("streamer.16_to_24", 0);
    if (conf_autoconv_16_to_24 != autoconv_16_to_24) {
        autoconv_16_to_24 = conf_autoconv_16_to_24;
        formatchanged = 1;
    }
    int conf_bit_override = conf_get_int ("streamer.bit_override", 0);
    if (conf_bit_override != bit_override) {
        bit_override = conf_bit_override;
        formatchanged = 1;
    }

    trace_bufferfill = conf_get_int ("streamer.trace_buffer_fill", 0);

    stop_after_current = conf_get_int ("playlist.stop_after_current", 0);
    stop_after_album = conf_get_int ("playlist.stop_after_album", 0);

    char mapstr[2048];
    conf_get_str ("network.ctmapping", DDB_DEFAULT_CTMAPPING, mapstr, sizeof (mapstr));
    if (strcmp (mapstr, conf_network_ctmapping)) {
        strcpy (conf_network_ctmapping, mapstr);
        streamer_ctmap = ddb_ctmap_init_from_string (conf_network_ctmapping);
    }

    int new_conf_streamer_override_samplerate = conf_get_int ("streamer.override_samplerate", 0);
    int new_conf_streamer_use_dependent_samplerate = conf_get_int ("streamer.use_dependent_samplerate", 0);
    int new_conf_streamer_samplerate = clamp_samplerate (conf_get_int ("streamer.samplerate", 44100));
    int new_conf_streamer_samplerate_mult_48 = clamp_samplerate (conf_get_int ("streamer.samplerate_mult_48", 48000));
    int new_conf_streamer_samplerate_mult_44 = clamp_samplerate (conf_get_int ("streamer.samplerate_mult_44", 44100));

    if (conf_streamer_override_samplerate != new_conf_streamer_override_samplerate ||
        conf_streamer_use_dependent_samplerate != new_conf_streamer_use_dependent_samplerate ||
        conf_streamer_samplerate != new_conf_streamer_samplerate ||
        conf_streamer_samplerate_mult_48 != new_conf_streamer_samplerate_mult_48 ||
        conf_streamer_samplerate_mult_44 != new_conf_streamer_samplerate_mult_44 || formatchanged) {
        memset (&last_block_fmt, 0, sizeof (last_block_fmt));
    }

    conf_streamer_override_samplerate = new_conf_streamer_override_samplerate;
    conf_streamer_use_dependent_samplerate = new_conf_streamer_use_dependent_samplerate;
    conf_streamer_samplerate = new_conf_streamer_samplerate;
    conf_streamer_samplerate_mult_48 = new_conf_streamer_samplerate_mult_48;
    conf_streamer_samplerate_mult_44 = new_conf_streamer_samplerate_mult_44;

    conf_format_silence = conf_get_float ("streamer.format_change_silence", -1.f);

    int playback_buffer_size = conf_get_int ("streamer.playback_buffer_size", 300);
    if (playback_buffer_size < 100) {
        playback_buffer_size = 100;
    }
    else if (playback_buffer_size > 2000) {
        playback_buffer_size = 2000;
    }
    conf_playback_buffer_size = playback_buffer_size / 1000.f;

    streamreader_configchanged ();

    streamer_unlock ();
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
    DB_output_t *output = plug_get_output ();
    output->stop ();
    streamer_lock ();
    streamer_reset (1);
    streamer_is_buffering = 1;
    streamer_unlock ();

    playItem_t *prev = playing_track;
    if (prev) {
        pl_item_ref (prev);
    }

    streamer_set_playing_track (NULL);
    streamer_set_next_track_to_play (NULL);
    streamer_set_prev_track_to_play (NULL);
    streamer_set_buffering_track (it);
    handle_track_change (prev, it);

    if (prev) {
        pl_item_unref (prev);
        prev = NULL;
    }

    if (!stream_track (it, startpaused)) {
        playpos = 0;
        playtime = 0;
        if (startpaused) {
            output->pause ();
            messagepump_push (DB_EV_PAUSED, 0, 1, 0);
            streamer_start_playback (NULL, it);
            send_songstarted (playing_track);
        }
        else {
            int st = output->state ();
            output->play ();
            if (st == DDB_PLAYBACK_STATE_PAUSED) {
                messagepump_push (DB_EV_PAUSED, 0, 0, 0);
            }
        }
    }
    else {
        streamer_set_buffering_track (NULL);
    }
}

static void
_rebuild_shuffle_albums_after_manual_trigger (playlist_t *plt, playItem_t *it) {
    plt_reshuffle (plt, NULL, NULL);

    ddb_shuffle_t shuffle = streamer_get_shuffle ();
    if (shuffle == DDB_SHUFFLE_ALBUMS) {
        pl_lock ();
        _streamer_mark_album_played_up_to (it);
        pl_unlock ();
    }
    else {
        // This ensures that the manually triggered item becomes first in shuffle queue.
        // It works because shufflerating is generated using rand(), which gives only numbers in the [0..RAND_MAX] range.
        pl_set_shufflerating (it, -1);
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

    plt = plt_get_curr ();

    streamer_set_streamer_playlist (plt);

    if (idx < 0) {
        plt_reshuffle (plt, NULL, NULL);
        streamer_set_last_played (NULL);
        goto error;
    }

    it = plt_get_item_for_idx (plt, idx, PL_MAIN);
    if (!it) {
        goto error;
    }

    // rebuild shuffle order
    _rebuild_shuffle_albums_after_manual_trigger (plt, it);

    _play_track (it, startpaused);

    pl_item_unref (it);
    plt_unref (plt);
    return;

error:
    output->stop ();
    streamer_reset (1);
    viz_reset ();
    viz_process (NULL, 0, output, 0, 0);

    streamer_lock ();
    _handle_playback_stopped ();
    stream_track (NULL, 0);
    if (plt) {
        plt_unref (plt);
    }
    streamer_unlock ();
}

playItem_t *
streamer_get_current_track_to_play (playlist_t *plt) {
    int idx = plt->current_row[PL_MAIN];
    if (plt->current_row[PL_MAIN] == -1 && plt->count[PL_MAIN]) {
        idx = 0;
    }

    playItem_t *it = NULL;

    if (idx >= 0) {
        // currently selected track in current playlist
        it = plt_get_item_for_idx (plt, idx, PL_MAIN);
    }

    return it;
}

// if a track is playing: restart
// if a track is paused: unpause
// if no track is playing: do what comes first:
//     play next in queue
//     play track under cursor
//     stop playback
static void
play_current (void) {
    DB_output_t *output = plug_get_output ();
    if (output->state () == DDB_PLAYBACK_STATE_PAUSED && playing_track) {
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

    playlist_t *plt = plt_get_curr ();
    playItem_t *it = streamer_get_current_track_to_play (plt);

    streamer_reset (1);
    if (it) {
        pl_lock ();
        if (plt != streamer_playlist) {
            streamer_set_streamer_playlist (plt);
        }
        pl_unlock ();
        _play_track (it, 0);
        pl_item_unref (it);
    }

    if (plt) {
        plt_unref (plt);
    }
}

playItem_t *
streamer_get_next_track_with_direction (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    playItem_t *origin = NULL;
    if (buffering_track) {
        origin = buffering_track;
    }
    else {
        origin = last_played;
    }

    playItem_t *next = NULL;
    if (dir > 0) {
        next = get_next_track (origin, shuffle, repeat);
    }
    else if (dir < 0) {
        next = get_prev_track (origin, shuffle, repeat);
    }
    else {
        next = get_random_track ();
    }

    // possibly need a reshuffle
    if (!next && streamer_playlist->count[PL_MAIN] != 0) {
        if (repeat == DDB_REPEAT_OFF) {
            if (shuffle == DDB_SHUFFLE_ALBUMS || shuffle == DDB_SHUFFLE_TRACKS) {
                plt_reshuffle (streamer_playlist, dir > 0 ? &next : NULL, dir < 0 ? &next : NULL);
                if (next && dir < 0) {
                    // mark all songs as played except the current one
                    playItem_t *it = streamer_playlist->head[PL_MAIN];
                    while (it) {
                        if (it != next) {
                            pl_set_played (it, 1);
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

void
set_album_played (playItem_t *curr, int played) {
    for (playItem_t *it = curr; it && pl_items_from_same_album (curr, it); it = it->next[PL_MAIN]) {
        pl_set_played (it, played);
    }
}

playItem_t *
get_next_album (playItem_t *curr, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    // next album is only distinct from next track if shuffle is ALBUMS or OFF
    if (shuffle == DDB_SHUFFLE_TRACKS || shuffle == DDB_SHUFFLE_RANDOM) {
        return get_next_track (curr, shuffle, repeat);
    }

    pl_lock ();

    if (next_track_to_play != NULL && !pl_items_from_same_album (next_track_to_play, curr)) {
        pl_item_ref (next_track_to_play);
        pl_unlock ();
        return next_track_to_play;
    }

    if (!streamer_playlist) {
        playlist_t *plt = plt_get_curr ();
        streamer_set_streamer_playlist (plt);
        plt_unref (plt);
    }

    while (playqueue_getcount ()) {
        trace ("playqueue_getnext\n");
        playItem_t *it = playqueue_getnext ();
        if (it && !pl_items_from_same_album (it, curr)) {
            pl_unlock ();
            return it; // from playqueue
        }
    }

    playlist_t *plt = streamer_playlist;
    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        return NULL; // empty playlist
    }

    if (plt_get_item_idx (streamer_playlist, curr, PL_MAIN) == -1) {
        playlist_t *item_plt = pl_get_playlist (curr);
        if (!item_plt) {
            curr = NULL;
        }
        else {
            plt_unref (item_plt);
        }
    }

    playItem_t *it = NULL;
    if (shuffle == DDB_SHUFFLE_OFF) {
        it = curr;
        if (curr) {
            do {
                pl_set_played (it, 1);
                it = it->next[PL_MAIN];
            } while (it != NULL && pl_items_from_same_album (curr, it));
        }
        else {
            it = plt->head[PL_MAIN];
        }
        if (!it) {
            trace ("streamer_move_nextalbum: reached end of playlist\n");
            if (repeat == DDB_REPEAT_ALL) {
                it = plt->head[PL_MAIN];
            }
        }
    }
    else if (shuffle == DDB_SHUFFLE_ALBUMS) {
        // find the first not played playlist item with minimal shufflerating > curr's shufflerating
        // since tracks from the same album have the same shufflerating strict inequality guarantees this is a different album
        if (!curr) {
            it = _streamer_find_minimal_notplayed (plt);
        }
        else {
            set_album_played (curr, 1);
            it = _streamer_find_minimal_notplayed_with_floor (plt, pl_get_shufflerating (curr));
        }
        if (!it) {
            // all songs played, reshuffle
            if (repeat == DDB_REPEAT_ALL) {
                plt_reshuffle (streamer_playlist, &it, NULL);
            }
        }
    }

    if (it) {
        pl_item_ref (it);
    }
    pl_unlock ();
    return it;
}

playItem_t *
get_prev_album (playItem_t *curr, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    // prev album is only distinct from prev track if shuffle is ALBUMS or OFF
    if (shuffle == DDB_SHUFFLE_TRACKS || shuffle == DDB_SHUFFLE_RANDOM) {
        return get_prev_track (curr, shuffle, repeat);
    }

    pl_lock ();

    playlist_t *plt = streamer_playlist;
    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        return NULL; // empty playlist
    }

    if (plt_get_item_idx (streamer_playlist, curr, PL_MAIN) == -1) {
        playlist_t *item_plt = pl_get_playlist (curr);
        if (!item_plt) {
            curr = NULL;
        }
        else {
            plt_unref (item_plt);
        }
    }

    playItem_t *it = NULL;
    if (shuffle == DDB_SHUFFLE_OFF) {
        it = curr->prev[PL_MAIN];
        if (!it) {
            it = plt->tail[PL_MAIN];
        }
        while (it->prev[PL_MAIN] && pl_items_from_same_album (it, it->prev[PL_MAIN])) {
            it = it->prev[PL_MAIN];
        }
    }
    else if (shuffle == DDB_SHUFFLE_ALBUMS) {
        if (!curr) {
            it = _streamer_find_maximal_played (plt);
        }
        else if (curr->prev[PL_MAIN] && pl_items_from_same_album (curr, curr->prev[PL_MAIN])) {
            it = curr;
            while (it->prev[PL_MAIN] && pl_items_from_same_album (it, it->prev[PL_MAIN])) {
                it = it->prev[PL_MAIN];
            }
        }
        else {
            it = _streamer_find_maximal_played_with_ceil (plt, pl_get_shufflerating (curr));
        }
    }
    if (curr) {
        set_album_played (curr, 0);
    }
    if (it) {
        set_album_played (it, 0);
        pl_item_ref (it);
    }

    pl_unlock ();
    return it;
}

playItem_t *
streamer_get_next_album_with_direction (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    playItem_t *origin = NULL;
    playItem_t *next = NULL;
    if (dir == 0) {
        return get_random_album ();
    }
    else if (buffering_track) {
        origin = buffering_track;
    }
    else {
        origin = last_played;
    }
    if (!origin) {
        return streamer_get_next_track_with_direction (dir, shuffle, repeat);
    }
    if (dir > 0) {
        next = get_next_album (origin, shuffle, repeat);
    }
    else {
        next = get_prev_album (origin, shuffle, repeat);
    }

    return next;
}

static void
play_next (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    DB_output_t *output = plug_get_output ();

    playItem_t *next = streamer_get_next_track_with_direction (dir, shuffle, repeat);

    if (!next) {
        streamer_set_last_played (NULL);
        output->stop ();
        streamer_reset (1);
        _handle_playback_stopped ();
        return;
    }

    if (dir == 0) {
        // rebuild shuffle order
        _rebuild_shuffle_albums_after_manual_trigger (streamer_playlist, next);
    }
    _play_track (next, 0);
    pl_item_unref (next);
}

static void
play_next_album (int dir, ddb_shuffle_t shuffle, ddb_repeat_t repeat) {
    DB_output_t *output = plug_get_output ();

    playItem_t *next = streamer_get_next_album_with_direction (dir, shuffle, repeat);

    if (!next) {
        streamer_set_last_played (NULL);
        output->stop ();
        streamer_reset (1);
        _handle_playback_stopped ();
        return;
    }

    if (dir == 0) {
        // rebuild shuffle order
        _rebuild_shuffle_albums_after_manual_trigger (streamer_playlist, next);
    }
    streamer_lock ();
    _play_track (next, 0);
    streamer_unlock ();
    pl_item_unref (next);
}

void
streamer_play_current_track (void) {
    handler_push (handler, STR_EV_PLAY_CURR, 0, 0, 0);
}

struct DB_fileinfo_s *
streamer_get_current_fileinfo (void) {
    return fileinfo_curr;
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

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .flags = DDB_TF_CONTEXT_NO_MUTEX_LOCK | DDB_TF_CONTEXT_NO_DYNAMIC,
    };

    char album[100];
    char artist[100];
    char other_album[100];
    char other_artist[100];

    ctx.it = (ddb_playItem_t *)item;
    tf_eval (&ctx, _album_tf, album, sizeof (album));
    tf_eval (&ctx, _artist_tf, artist, sizeof (artist));

    pl_set_played (item, 1);
    playItem_t *next = item->prev[PL_MAIN];
    while (next) {
        ctx.it = (ddb_playItem_t *)next;
        tf_eval (&ctx, _album_tf, other_album, sizeof (other_album));
        tf_eval (&ctx, _artist_tf, other_artist, sizeof (artist));

        int same_album = 1;

        if (*album == 0 || strcmp (album, other_album)) {
            same_album = 0;
        }
        else if (*artist == 0 || strcmp (artist, other_artist)) {
            same_album = 0;
        }

        if (same_album) {
            pl_set_played (next, 1);
            next = next->prev[PL_MAIN];
        }
        else {
            break;
        }
    }
    pl_unlock ();
}

static void
streamer_shuffle_changed (ddb_shuffle_t prev, ddb_shuffle_t shuffle) {
    if (prev != DDB_SHUFFLE_ALBUMS && shuffle == DDB_SHUFFLE_ALBUMS) {

        streamer_lock ();

        playItem_t *curr = playing_track;
        if (curr) {
            _streamer_mark_album_played_up_to (curr);
        }

        streamer_unlock ();
    }
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
streamer_yield (void) {
    while (handler_hasmessages (handler)) {
        usleep (50000);
    }
}

void
streamer_set_output (DB_output_t *output) {
    DB_output_t *prev = plug_get_output ();
    ddb_playback_state_t state = DDB_PLAYBACK_STATE_STOPPED;
    ddb_waveformat_t fmt = { 0 };
    if (prev) {
        state = prev->state ();
        memcpy (&fmt, &prev->fmt, sizeof (ddb_waveformat_t));
        prev->free ();
    }

    if (mutex) {
        streamer_lock ();
    }
    plug_set_output (output);

    if (fmt.channels) {
        output->setformat (&fmt);
    }

    int res = 0;
    if (state == DDB_PLAYBACK_STATE_PLAYING) {
        res = output->play ();
    }
    else if (state == DDB_PLAYBACK_STATE_PAUSED) {
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

void
streamer_notify_track_deleted (void) {
    handler_push (handler, STR_EV_TRACK_DELETED, 0, 0, 0);
}
