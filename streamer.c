/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  streamer implementation

  Copyright (C) 2009-2013 Alexey Yakovenko

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
#include "ringbuf.h"
#include "replaygain.h"
#include "fft.h"
#include "handler.h"
#include "plugins/libparser/parser.h"
#include "strdupa.h"
#include "playqueue.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//#define WRITE_DUMP 1
//#define DETECT_PL_LOCK_RC 1

#if WRITE_DUMP
FILE *out;
#endif

#define MAX_PLAYLIST_DOWNLOAD_SIZE 25000
#define STREAMER_HINTS (DDB_DECODER_HINT_NEED_BITRATE|DDB_DECODER_HINT_CAN_LOOP)

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

static char *dsp_input_buffer;
static int dsp_input_buffer_size;

static char *dsp_temp_buffer;
static int dsp_temp_buffer_size;

static int autoconv_8_to_16 = 1;

static int autoconv_16_to_24 = 0;

static int trace_bufferfill = 0;

static int stop_after_current = 0;
static int stop_after_album = 0;

static int conf_streamer_nosleep = 0;

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
static uintptr_t currtrack_mutex;
static uintptr_t wdl_mutex; // wavedata listener

static int nextsong = -1;
static int nextsong_pstate = -1;
static int badsong = -1;

static float last_seekpos = -1;

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
static DB_FILE *fileinfo_file;
static DB_fileinfo_t *new_fileinfo;
static DB_FILE *new_fileinfo_file;

static int streamer_buffering;

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
streamer_set_nextsong_real (int song, int pstate);

static int
streamer_move_to_nextsong_real (int r);

static int
streamer_move_to_prevsong_real (int r);

static int
streamer_move_to_randomsong_real (int r);

static void
streamer_play_current_track_real (void);

static void
streamer_set_current_playlist_real (int plt);


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

static void
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

static int
stop_after_album_check (playItem_t *cur, playItem_t *next) {
    if (!stop_after_album) {
        return 0;
    }

    if (!cur) {
        return 0;
    }

    if (!next) {
        streamer_buffering = 0;
        streamer_set_nextsong_real (-2, -2);
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

    streamer_buffering = 0;
    streamer_set_nextsong_real (-2, -2);
    if (conf_get_int ("playlist.stop_after_album_reset", 0)) {
        conf_set_int ("playlist.stop_after_album", 0);
        stop_after_album = 0;
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }

    return 1;
}

static int
streamer_move_to_nextsong_real (int reason) {
    if (reason) {
        plug_get_output ()->stop ();
    }
    trace ("streamer_move_to_nextsong (%d)\n", reason);
    pl_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr ();
    }

    playItem_t *curr = playlist_track;

    while (playqueue_getcount ()) {
        trace ("playqueue_getnext\n");
        playItem_t *it = playqueue_getnext ();
        if (it) {
            if (stop_after_album_check(curr, it)) {
                pl_unlock ();
                return -1;
            }

            playqueue_pop ();
            int r = str_get_idx_of (it);
            if (r >= 0) {
                pl_item_unref (it);
                pl_unlock ();
                streamer_set_nextsong_real (r, 1);
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
                        streamer_set_nextsong_real (r, 3);
                        return 0;
                    }
                }
                trace ("%s not found in any playlists\n", pl_find_meta (it, ":URI"));
                pl_item_unref (it);
            }
        }
    }

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
        streamer_set_nextsong_real (-2, 1);
        return 0;
    }
    int pl_order = pl_get_order ();

    int pl_loop_mode = conf_get_int ("playback.loop", 0);

    if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
        int r = str_get_idx_of (playing_track);
        pl_unlock ();
        if (r == -1) {
            streamer_set_nextsong_real (-2, 1);
        }
        else {
            streamer_set_nextsong_real (r, 1);
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
            // although it is possible that, although it == NULL, reshuffling the playlist
            // will result in the next track belonging to the same album as this one, this
            // is most likely not what the user wants.
            if (stop_after_album_check(curr, it)) {
                pl_unlock ();
                return -1;
            }
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
                streamer_set_nextsong_real (-2, -2);
                return -1;
            }
            int r = str_get_idx_of (it);
            pl_unlock ();
            streamer_set_nextsong_real (r, 1);
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
            if (stop_after_album_check(curr, it)) {
                pl_unlock ();
                return -1;
            }
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
                streamer_set_nextsong_real (-2, -2);
                return -1;
            }
            int r = str_get_idx_of (it);
            pl_unlock ();
            streamer_set_nextsong_real (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        DB_output_t *output = plug_get_output ();
        playItem_t *it = NULL;
        if (!curr && output->state () == OUTPUT_STATE_STOPPED) {
            int cur = plt_get_cursor (streamer_playlist, PL_MAIN);
            if (cur != -1) {
                curr = plt_get_item_for_idx (streamer_playlist, cur, PL_MAIN);
                pl_item_unref (curr);
            }
        }
        if (curr) {
            it = curr->next[PL_MAIN];
        }
        else {
            it = streamer_playlist->head[PL_MAIN];
        }
        if (stop_after_album_check(curr, it)) {
            pl_unlock ();
            return -1;
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
                streamer_set_nextsong_real (-2, -2);
                return 0;
            }
        }
        if (!it) {
            pl_unlock ();
            return -1;
        }
        int r = str_get_idx_of (it);
        pl_unlock ();
        streamer_set_nextsong_real (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_unlock ();
        int res = streamer_move_to_randomsong_real (0);
        if (res == -1) {
            trace ("streamer_move_to_randomsong error\n");
            streamer_set_nextsong_real (-2, 1);
            return -1;
        }
        return 0;
    }
    pl_unlock ();
    return -1;
}

static int
streamer_move_to_prevsong_real (int r) {
    if (r) {
        plug_get_output ()->stop ();
    }
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
    playqueue_clear ();
    if (!plt->head[PL_MAIN]) {
        pl_unlock ();
        streamer_set_nextsong_real (-2, 1);
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        if (!playlist_track) {
            pl_unlock ();
            return streamer_move_to_nextsong_real (0);
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
                streamer_set_nextsong_real (-2, 1);
                return -1;
            }
            int r = str_get_idx_of (it);
            pl_unlock ();
            streamer_set_nextsong_real (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        DB_output_t *output = plug_get_output ();
        playItem_t *it = NULL;
        if (!playlist_track && output->state () == OUTPUT_STATE_STOPPED) {
            int cur = plt_get_cursor (streamer_playlist, PL_MAIN);
            if (cur != -1) {
                playlist_track = plt_get_item_for_idx (streamer_playlist, cur, PL_MAIN);
                pl_item_unref (playlist_track);
            }
        }
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
            streamer_set_nextsong_real (-2, 1);
            return -1;
        }
        int r = str_get_idx_of (it);
        pl_unlock ();
        streamer_set_nextsong_real (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        pl_unlock ();
        int res = streamer_move_to_randomsong_real (0);
        if (res == -1) {
            streamer_set_nextsong_real (-2, 1);
            trace ("streamer_move_to_randomsong error\n");
            return -1;
        }
        return 0;
    }
    pl_unlock ();
    return -1;
}

static int
streamer_move_to_randomsong_real (int reason) {
    if (reason) {
        plug_get_output ()->stop ();
    }
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

    streamer_set_nextsong_real (r, 1);
    return 0;
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
    if (it == playlist_track) {
        playlist_track = playlist_track->prev[PL_MAIN];
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
    char ct[MAX_TOKEN];
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
    trace ("\033[0;35moutput state: %d\033[37;0m\n", output->state ());
    if (!playing_track || output->state () == OUTPUT_STATE_STOPPED) {
        streamer_buffering = 1;
        trace ("\033[0;35mstreamer_start_playback[1] from %p to %p\033[37;0m\n", from, it);
        do_songstarted = 1;
        streamer_start_playback (from, it);
        bytes_until_next_song = -1;
    }

    trace ("streamer_set_current %p, buns=%d\n", it, bytes_until_next_song);
    mutex_lock (currtrack_mutex);
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }

    mutex_unlock (currtrack_mutex);

    int paused_stream = 0;
    if (it && nextsong_pstate == 2) {
        paused_stream = is_remote_stream (it);
    }

    if (!it || paused_stream) {
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

            int size = vfs_fgetlength (fp);
            if (size <= 0) {
                size = MAX_PLAYLIST_DOWNLOAD_SIZE;
            }
            buf = malloc (size);
            if (!buf) {
                trace ("failed to alloc %d bytes for playlist buffer\n", size);
                goto m3u_error;
            }
            trace ("reading %d bytes\n", size);
            int rd = vfs_fread (buf, 1, size, fp);
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
            int rw = fwrite (buf, 1, rd, out);
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
                res = streamer_set_current (it);
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
    playlist_track = it;

    int plug_idx = 0;
    for (;;) {
        if (!decoder_id[0] && plugs[0] && !plugs[plug_idx]) {
            it->played = 1;
            trace ("decoder->init returned %p\n", new_fileinfo);
            streamer_buffering = 0;
            if (playlist_track == it) {
                trace ("redraw track %p; playing_track=%p; playlist_track=%p\n", to, playing_track, playlist_track);
                send_trackinfochanged (to);
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
                streamer_set_replaygain (streaming_track);
            }

            trace ("bps=%d, channels=%d, samplerate=%d\n", new_fileinfo->fmt.bps, new_fileinfo->fmt.channels, new_fileinfo->fmt.samplerate);
            break;
        }
    }
success:
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
        fileinfo_file = NULL;
    }
    if (new_fileinfo) {
        fileinfo = new_fileinfo;
        new_fileinfo = NULL;
        new_fileinfo_file = NULL;
    }
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
    float seek = last_seekpos;
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
//    pthread_t tid = pthread_self ();
//    assert (tid != streamer_tid);
    if (pstate == 0) {
        // this is a stop query -- clear the queue
        handler_reset (handler);
    }
    streamer_abort_files ();
    handler_push (handler, STR_EV_PLAY_TRACK_IDX, 0, song, pstate);
}

static void
streamer_set_nextsong_real (int song, int pstate) {
    DB_output_t *output = plug_get_output ();
    if (pstate != 4) {
        int n = 0;
    }
    trace ("\033[0;35mstreamer_set_nextsong %d %d\033[37;0m\n", song, pstate);
    if (pstate == 4) {
        pstate = 1;
        output->stop ();
    }
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
        last_seekpos = -1;
    }
    if (pl_get_order () == PLAYBACK_ORDER_SHUFFLE_ALBUMS) {
        plt_init_shuffle_albums (streamer_playlist, song);
    }
    streamer_unlock ();
}

static void
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
    last_seekpos = pos;
    handler_push (handler, STR_EV_SEEK, 0, *((uint32_t *)&pos), 0);
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
        streamer_set_nextsong_real (-2, -2);
        badsong = -1;
        return;
    }
    playItem_t *try = str_get_for_idx (sng);
    if (!try) { // track is not in playlist
        trace ("track #%d is not in playlist; stopping playback\n", sng);
        output->stop ();

        mutex_lock (currtrack_mutex);
        if (playing_track) {
            pl_item_unref (playing_track);
            playing_track = NULL;
        }
        if (streaming_track) {
            pl_item_unref (streaming_track);
            streaming_track = NULL;
        }
        mutex_unlock (currtrack_mutex);

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
            trace ("streamer_move_to_nextsong after skip\n");
            streamer_move_to_nextsong_real (1);
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
//                fprintf (stderr, "streamer_set_output_format %dbit %s %dch %dHz channelmask=%X\n", output_format.bps, output_format.is_float ? "float" : "int", output_format.channels, output_format.samplerate, output_format.channelmask);
                streamer_set_output_format ();
            }
            if (0 != output->play ()) {
                // give a chance to DSP plugins to convert format to something
                // supported
                streamer_set_generic_output_format ();
                if (0 != output->play ()) {
                    memset (&orig_output_format, 0, sizeof (orig_output_format));
                    fprintf (stderr, "streamer: failed to start playback (start track)\n");
                    streamer_set_nextsong_real (-2, 0);
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
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                streamer_set_output_format ();
            }
            // we need to start playback before we can pause it
            if (0 != output->play ()) {
                memset (&orig_output_format, 0, sizeof (orig_output_format));
                fprintf (stderr, "streamer: failed to start playback (start track)\n");
                streamer_set_nextsong_real (-2, 0);
            }
        }
        output->pause ();
    }
}

static void
streamer_next (int bytesread) {
    streamer_lock ();
    bytes_until_next_song = streamer_ringbuf.remaining + bytesread;
    streamer_unlock ();
    if (stop_after_current) {
        streamer_buffering = 0;
        streamer_set_nextsong_real (-2, -2);
        if (conf_get_int ("playlist.stop_after_current_reset", 0)) {
            conf_set_int ("playlist.stop_after_current", 0);
            stop_after_current = 0;
            deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
        }
    }
    else {
        trace ("streamer_move_to_nextsong (0) called from streamer_next\n");
        streamer_move_to_nextsong_real (0);
    }
}

static void
streamer_dsp_postinit (void);

static void
streamer_set_dsp_chain_real (ddb_dsp_context_t *chain);

static void
streamer_notify_order_changed_real (int prev_order, int new_order);

static void
free_dsp_buffers (void);

void
streamer_thread (void *ctx) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-stream", 0, 0, 0, 0);
#endif

    while (!streaming_terminate) {
        float seekpos = -1;

        struct timeval tm1;
        DB_output_t *output = plug_get_output ();
        gettimeofday (&tm1, NULL);

        uint32_t id;
        uintptr_t ctx;
        uint32_t p1, p2;
        if (!handler_pop (handler, &id, &ctx, &p1, &p2)) {
            switch (id) {
            case STR_EV_PLAY_TRACK_IDX:
                streamer_set_nextsong_real (p1, p2);
                break;
            case STR_EV_PLAY_CURR:
                streamer_play_current_track_real ();
                break;
            case STR_EV_NEXT:
                streamer_move_to_nextsong_real (p1);
                break;
            case STR_EV_PREV:
                streamer_move_to_prevsong_real (p1);
                break;
            case STR_EV_RAND:
                streamer_move_to_randomsong_real (p1);
                break;
            case STR_EV_SEEK:
                seekpos = *((float *)&p1);
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

        if (nextsong >= 0) { // start streaming next song
            trace ("\033[0;34mnextsong=%d\033[37;0m\n", nextsong);
            streamer_start_new_song ();
            if (nextsong_pstate == 2) {
                nextsong_pstate = -1;
            }
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
            output->stop ();
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
            last_seekpos = -1;
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

        if (formatchanged && bytes_until_next_song <= 0) {
            streamer_set_output_format ();
            formatchanged = 0;
        }

        float seek = seekpos;
        if (seek >= 0 && pl_get_item_duration (playing_track) > 0) {
            playpos = seek;
            trace ("seeking to %f\n", seek);
            float pos = seek;

            if (playing_track != streaming_track) {
                trace ("streamer already switched to next track\n");

                // restart playing from new position

                mutex_lock (currtrack_mutex);
                if(fileinfo) {
                    fileinfo->plugin->free (fileinfo);
                    fileinfo = NULL;
                    fileinfo_file = NULL;
                    pl_item_unref (streaming_track);
                    streaming_track = NULL;
                }
                streaming_track = playing_track;
                if (streaming_track) {
                    pl_item_ref (streaming_track);
                    streamer_set_replaygain (streaming_track);
                }
                mutex_unlock (currtrack_mutex);

                bytes_until_next_song = -1;
                streamer_buffering = 1;
                if (streaming_track) {
                    send_trackinfochanged (streaming_track);
                }

                DB_decoder_t *dec = NULL;
                pl_lock ();
                const char *decoder_id = pl_find_meta (streaming_track, ":DECODER");
                if (decoder_id) {
                    dec = plug_get_decoder_for_id (decoder_id);
                }
                pl_unlock ();
                if (dec) {
                    fileinfo = dec_open (dec, STREAMER_HINTS, streaming_track);
                    if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (streaming_track)) != 0) {
                        dec->free (fileinfo);
                        fileinfo = NULL;
                        fileinfo_file = NULL;
                    }
                }
                else {
                    if (fileinfo) {
                        fileinfo_file = fileinfo->file;
                    }
                }

                if (!dec || !fileinfo) {
                    if (streaming_track) {
                        send_trackinfochanged (streaming_track);
                    }
                    trace ("failed to restart prev track on seek, trying to jump to next track\n");
                    trace ("streamer_move_to_nextsong from seek\n");
                    streamer_move_to_nextsong (0);
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
        last_seekpos = -1;

        // read ahead at 2x speed of output samplerate, in 4k blocks
        int rate = output->fmt.samplerate;
        if (!rate) {
            trace ("str: got 0 output samplerate\n");
            usleep(20000);
            continue;
        }
        int channels = output->fmt.channels;
        int bytes_in_one_second = rate * (output->fmt.bps>>3) * channels;
        int blocksize = bytes_in_one_second / 120;

        if (blocksize < MIN_BLOCK_SIZE) {
            blocksize = MIN_BLOCK_SIZE;
        }
        else if (blocksize > MAX_BLOCK_SIZE) {
            blocksize = MAX_BLOCK_SIZE;
        }

        blocksize &= ~3; // 4byte alignment is required

        if (bytes_in_one_second < blocksize) {
            bytes_in_one_second = blocksize;
        }

        int alloc_time = 1000 / (bytes_in_one_second / blocksize);

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
                if (nb <= 0) {
                    break;
                }
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

            if (trace_bufferfill >= 1) {
                fprintf (stderr, "fill: %d, read: %d, size=%d, blocksize=%d\n", (int)streamer_ringbuf.remaining, (int)bytesread, (int)STREAM_BUFFER_SIZE, (int)blocksize);
            }
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
        if (trace_bufferfill >= 2) {
            fprintf (stderr, "slept %dms (alloc=%dms, bytespersec=%d, chan=%d, blocksize=%d), fill: %d/%d (cursor=%d)\n", (int)(alloc_time-ms), (int)alloc_time, (int)bytes_in_one_second, output->fmt.channels, blocksize, (int)streamer_ringbuf.remaining, STREAM_BUFFER_SIZE, (int)streamer_ringbuf.cursor);
        }

        // add 1ms here to compensate the rounding error
        // and another 1ms to buffer slightly faster then playing
        alloc_time -= ms+2;
        if (streamer_buffering) {
            alloc_time = 0;
        }
        else if (streamer_ringbuf.remaining < STREAM_BUFFER_SIZE / 2) {
            alloc_time >>= 2; // speed-up loading a little
        }

        //printf ("sleep: %d, buffering: %d, buffer_starving: %d (%d/%d)\n", alloc_time, streamer_buffering, streamer_ringbuf.remaining < STREAM_BUFFER_SIZE / 2, streamer_ringbuf.remaining, STREAM_BUFFER_SIZE / 2);

        if (alloc_time > 0 && !conf_streamer_nosleep) {
            usleep (alloc_time * 1000);
        }
        else if (bytes_until_next_song > 0) {
            usleep (20000);
        }
    }

    // stop streaming song
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
        fileinfo_file = NULL;
    }
    mutex_lock (currtrack_mutex);
    if (streaming_track) {
        pl_item_unref (streaming_track);
        streaming_track = NULL;
    }
    if (playing_track) {
        pl_item_unref (playing_track);
        playing_track = NULL;
    }
    mutex_unlock (currtrack_mutex);
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
        int err = fscanf (fp, "%99s %d {\n", temp, &enabled);
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
streamer_dsp_chain_save_internal (const char *fname, ddb_dsp_context_t *chain) {
    char tempfile[PATH_MAX];
    snprintf (tempfile, sizeof (tempfile), "%s.tmp", fname);
    FILE *fp = fopen (tempfile, "w+t");
    if (!fp) {
        return -1;
    }

    ddb_dsp_context_t *ctx = chain;
    while (ctx) {
        if (fprintf (fp, "%s %d {\n", ctx->plugin->plugin.id, (int)ctx->enabled) < 0) {
            fprintf (stderr, "write to %s failed (%s)\n", tempfile, strerror (errno));
            goto error;
        }
        if (ctx->plugin->num_params) {
            int n = ctx->plugin->num_params ();
            int i;
            for (i = 0; i < n; i++) {
                char v[1000];
                ctx->plugin->get_param (ctx, i, v, sizeof (v));
                if (fprintf (fp, "\t%s\n", v) < 0) {
                    fprintf (stderr, "write to %s failed (%s)\n", tempfile, strerror (errno));
                    goto error;
                }
            }
        }
        if (fprintf (fp, "}\n") < 0) {
            fprintf (stderr, "write to %s failed (%s)\n", tempfile, strerror (errno));
            goto error;
        }
        ctx = ctx->next;
    }

    fclose (fp);
    if (rename (tempfile, fname) != 0) {
        fprintf (stderr, "dspconfig rename %s -> %s failed: %s\n", tempfile, fname, strerror (errno));
        return -1;
    }
    return 0;
error:
    fclose (fp);
    return -1;
}

int
streamer_dsp_chain_save (void) {
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    return streamer_dsp_chain_save_internal (fname, dsp_chain);
}

static void
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
    handler_push (handler, STR_EV_DSP_RELOAD, 0, 0, 0);
}

static void
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
    handler = handler_alloc (100);
#if WRITE_DUMP
    out = fopen ("out.raw", "w+b");
#endif
    mutex = mutex_create ();
    currtrack_mutex = mutex_create ();
    wdl_mutex = mutex_create ();

    ringbuf_init (&streamer_ringbuf, streambuffer, STREAM_BUFFER_SIZE);

    pl_set_order (conf_get_int ("playback.order", 0));

    streamer_dsp_init ();

    replaygain_set (conf_get_int ("replaygain_mode", 0), conf_get_int ("replaygain_scale", 1), conf_get_float ("replaygain_preamp", 0), conf_get_float ("global_preamp", 0));

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

    ctmap_free ();
    ctmap_free_mutex ();

    mutex_free (currtrack_mutex);
    currtrack_mutex = 0;
    mutex_free (mutex);
    mutex = 0;
    mutex_free (wdl_mutex);
    wdl_mutex = 0;

    streamer_dsp_chain_save();

    streamer_dsp_chain_free (dsp_chain);
    dsp_chain = NULL;

    free_dsp_buffers ();

    eqplug = NULL;
    eq = NULL;

    if (handler) {
        handler_free (handler);
        handler = NULL;
    }
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

    trace ("streamer_set_output_format %dbit %s %dch %dHz channelmask=%X, bufferfill: %d\n", output_format.bps, output_format.is_float ? "float" : "int", output_format.channels, output_format.samplerate, output_format.channelmask, streamer_ringbuf.remaining);
    ddb_waveformat_t fmt;
    memcpy (&fmt, &output_format, sizeof (ddb_waveformat_t));
    if (autoconv_8_to_16) {
        if (fmt.bps == 8) {
            fmt.bps = 16;
        }
    }
    if (autoconv_16_to_24) {
        if (fmt.bps == 16) {
            fmt.bps = 24;
        }
    }
    output->setformat (&fmt);
    streamer_buffering = 1;
    if (playing && output->state () != OUTPUT_STATE_PLAYING) {
        if (0 != output->play ()) {
            memset (&output_format, 0, sizeof (output_format));
            fprintf (stderr, "streamer: failed to start playback (streamer_read format change)\n");
            streamer_set_nextsong_real (-2, 0);
            return -1;
        }
    }
    return 0;
}

static char *
ensure_dsp_input_buffer (int size) {
    if (!size) {
        if (dsp_input_buffer) {
            free (dsp_input_buffer);
            dsp_input_buffer = NULL;
        }
        return 0;
    }
    if (size != dsp_input_buffer_size) {
        dsp_input_buffer = realloc (dsp_input_buffer, size);
        dsp_input_buffer_size = size;
    }
    return dsp_input_buffer;
}


static char *
ensure_dsp_temp_buffer (int size) {
    if (!size) {
        if (dsp_temp_buffer) {
            free (dsp_temp_buffer);
            dsp_temp_buffer = NULL;
        }
        return NULL;
    }
    if (size != dsp_temp_buffer_size) {
        dsp_temp_buffer = realloc (dsp_temp_buffer, size);
        dsp_temp_buffer_size = size;
    }
    return dsp_temp_buffer;
}

static void
free_dsp_buffers (void) {
    ensure_dsp_input_buffer (0);
    ensure_dsp_temp_buffer (0);
}

// decodes data and converts to current output format
// returns number of bytes been read
static int
streamer_read_async (char *bytes, int size) {
    DB_output_t *output = plug_get_output ();
    int initsize = size;
    int bytesread = 0;
    if (!fileinfo) {
        // means there's nothing left to stream, so just do nothing
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

            int inputsize = dsp_num_frames * inputsamplesize;
            char *input = ensure_dsp_input_buffer (inputsize);

            // decode pcm
            int nb = fileinfo->plugin->read (fileinfo, input, inputsize);
            if (nb != inputsize) {
                is_eof = 1;
            }
            inputsize = nb;

            if (inputsize > 0) {
                // make *MAX_DSP_RATIO sized buffer for float data
                int tempbuf_size = inputsize/inputsamplesize * dspsamplesize * MAX_DSP_RATIO;
                char *tempbuf = ensure_dsp_temp_buffer (tempbuf_size);

                // convert to float
                int tempsize = pcm_convert (&fileinfo->fmt, input, &dspfmt, tempbuf, inputsize);
                int nframes = inputsize / inputsamplesize;
                ddb_dsp_context_t *dsp = dsp_chain;
                float ratio = 1.f;
                int maxframes = tempbuf_size / dspsamplesize;
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
                if (bytes_until_next_song <= 0 && memcmp (&output_format, &outfmt, sizeof (ddb_waveformat_t))) {
                    memcpy (&output_format, &outfmt, sizeof (ddb_waveformat_t));
                    streamer_set_output_format ();
                }

                //printf ("convert from %dbit %s %dch %dHz channelmask=%X to %dbit %s %dch %dHz channelmask=%X\n", dspfmt.bps, dspfmt.is_float ? "float" : "int", dspfmt.channels, dspfmt.samplerate, dspfmt.channelmask, output->fmt.bps, output->fmt.is_float ? "float" : "int", output->fmt.channels, output->fmt.samplerate, output->fmt.channelmask);

                int n = pcm_convert (&dspfmt, tempbuf, &output->fmt, bytes, nframes * dspfmt.channels * sizeof (float));

                bytesread = n;
            }
        }
        else {
#ifdef ANDROID
            // if we not compensate here, the streamer loop will go crazy
            if (fileinfo->fmt.samplerate != output->fmt.samplerate) {
                if ((fileinfo->fmt.samplerate / output->fmt.samplerate) == 2 && (fileinfo->fmt.samplerate % output->fmt.samplerate) == 0) {
                    size <<= 1;
                }
                else if ((fileinfo->fmt.samplerate / output->fmt.samplerate) == 4 && (fileinfo->fmt.samplerate % output->fmt.samplerate) == 0) {
                    size <<= 2;
                }
            }
#endif
            // convert from input fmt to output fmt
            int inputsize = size/outputsamplesize*inputsamplesize;
            char input[inputsize];
            int nb = fileinfo->plugin->read (fileinfo, input, inputsize);
            if (nb != inputsize) {
                bytesread = nb;
                is_eof = 1;
            }
            inputsize = nb;
//            trace ("convert %d|%d|%d|%d|%d|%d to %d|%d|%d|%d|%d|%d\n"
//                , fileinfo->fmt.bps, fileinfo->fmt.channels, fileinfo->fmt.samplerate, fileinfo->fmt.channelmask, fileinfo->fmt.is_float, fileinfo->fmt.is_bigendian
//                , output->fmt.bps, output->fmt.channels, output->fmt.samplerate, output->fmt.channelmask, output->fmt.is_float, output->fmt.is_bigendian);
            bytesread = pcm_convert (&fileinfo->fmt, input, &output->fmt, bytes, inputsize);

#ifdef ANDROID
            // downsample
            if (fileinfo->fmt.samplerate > output->fmt.samplerate) {
                if ((fileinfo->fmt.samplerate / output->fmt.samplerate) == 2 && (fileinfo->fmt.samplerate % output->fmt.samplerate) == 0) {
                    // clip to multiple of 2 samples
                    int outsamplesize = output->fmt.channels * (output->fmt.bps>>3) * 2;
                    if ((bytesread % outsamplesize) != 0) {
                        bytesread -= (bytesread % outsamplesize);
                    }

                    // 2x downsample
                    int nframes = bytesread / (output->fmt.bps >> 3) / output->fmt.channels;
                    int16_t *in = (int16_t *)bytes;
                    int16_t *out = in;
                    for (int f = 0; f < nframes/2; f++) {
                        for (int c = 0; c < output->fmt.channels; c++) {
                            out[f*output->fmt.channels+c] = (in[f*2*output->fmt.channels+c] + in[(f*2+1)*output->fmt.channels+c]) >> 1;
                        }
                    }
                    bytesread >>= 1;
                }
                else if ((fileinfo->fmt.samplerate / output->fmt.samplerate) == 4 && (fileinfo->fmt.samplerate % output->fmt.samplerate) == 0) {
                    // clip to multiple of 4 samples
                    int outsamplesize = output->fmt.channels * (output->fmt.bps>>3) * 4;
                    if ((bytesread % outsamplesize) != 0) {
                        bytesread -= (bytesread % outsamplesize);
                    }


                    // 4x downsample
                    int nframes = bytesread / (output->fmt.bps >> 3) / output->fmt.channels;
                    assert (bytesread % ((output->fmt.bps >> 3) * output->fmt.channels) == 0);
                    int16_t *in = (int16_t *)bytes;
                    for (int f = 0; f < nframes/4; f++) {
                        for (int c = 0; c < output->fmt.channels; c++) {
                            in[f*output->fmt.channels+c] = (in[f*4*output->fmt.channels+c]
                                    + in[(f*4+1)*output->fmt.channels+c]
                                    + in[(f*4+2)*output->fmt.channels+c]
                                    + in[(f*4+3)*output->fmt.channels+c]) >> 2;
                        }
                    }
                    bytesread >>= 2;
                }
            }
            assert ((bytesread%2) == 0);
#endif

        }
#if WRITE_DUMP
        if (bytesread) {
            fwrite (bytes, 1, bytesread, out);
        }
#endif

        replaygain_apply (&output->fmt, streaming_track, bytes, bytesread);
    }
    if (!is_eof) {
        return bytesread;
    }
    else  {
        // that means EOF
        // trace ("streamer: EOF! buns: %d, bytesread: %d, buffering: %d, bufferfill: %d\n", bytes_until_next_song, bytesread, streamer_buffering, streamer_ringbuf.remaining);

        // EOF or error while buffering -- stop buffering
        if (bytesread <= 0 && bytes_until_next_song >= 0 && streamer_buffering) {
            streamer_buffering = 0;
            return bytesread;
        }

        // if track finished playing -- go to next
        if (bytes_until_next_song < 0) {
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
                    int32_t newsample = (int64_t)sample * ivolume / 1000;
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
                    int32_t newsample = (int64_t)sample * ivolume / 1000;
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

static int
streamer_get_fill (void) {
    return streamer_ringbuf.remaining;
}

int
streamer_ok_to_read (int len) {
    DB_output_t *output = plug_get_output ();
    if (formatchanged && bytes_until_next_song <= 0 && len >= 0) {
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
streamer_play_current_track_real (void) {
    playlist_t *plt = plt_get_curr ();
    DB_output_t *output = plug_get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED && playing_track) {
        if (is_remote_stream (playing_track) && pl_get_item_duration (playing_track) < 0) {
            streamer_reset (1);
            streamer_set_current (NULL);
            streamer_set_current (playing_track);
            if (fileinfo && memcmp (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                streamer_set_output_format ();
            }
        }
        // unpause currently paused track
        output->unpause ();
        messagepump_push (DB_EV_PAUSED, 0, 0, 0);
    }
    else if (plt->current_row[PL_MAIN] != -1) {
        // play currently selected track in current playlist
        output->stop ();
        // get next song in queue
        int idx = -1;
        playItem_t *next = playqueue_getnext ();
        if (next) {
            idx = str_get_idx_of (next);
            playqueue_pop ();
            pl_item_unref (next);
        }
        else {
            idx = plt->current_row[PL_MAIN];
        }

        streamer_set_nextsong_real (idx, 1);
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

static void
streamer_set_dsp_chain_real (ddb_dsp_context_t *chain) {
    streamer_dsp_chain_free (dsp_chain);
    dsp_chain = chain;
    eq = NULL;
    streamer_dsp_postinit ();
    if (fileinfo) {
        memcpy (&orig_output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
        memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
        formatchanged = 1;
    }

    streamer_dsp_chain_save();
    streamer_reset (1);

    DB_output_t *output = plug_get_output ();
    if (playing_track && output->state () != OUTPUT_STATE_STOPPED) {
        streamer_set_seek (playpos);
    }
    messagepump_push (DB_EV_DSPCHAINCHANGED, 0, 0, 0);
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

void
streamer_get_output_format (ddb_waveformat_t *fmt) {
    memcpy (fmt, &output_format, sizeof (ddb_waveformat_t));
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
    if (streamer_playlist) {
        plt_unref (streamer_playlist);
    }
    streamer_playlist = plt;
    if (streamer_playlist) {
        plt_ref (streamer_playlist);
    }
}

struct handler_s *
streamer_get_handler (void) {
    return handler;
}

void
streamer_set_playing_track (playItem_t *it) {
    if (playing_track) {
        pl_item_unref (playing_track);
    }
    playing_track = it;
    if (playing_track) {
        pl_item_ref (playing_track);
    }
}
