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
#include <samplerate.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/time.h>
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

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

//#define WRITE_DUMP 1

#if WRITE_DUMP
FILE *out;
#endif


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

static int conf_replaygain_mode = 0;
static int conf_replaygain_scale = 1;

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
static playItem_t *streaming_track;
static playItem_t *playlist_track;

static ddb_waveformat_t output_format;
static int formatchanged;

static DB_fileinfo_t *fileinfo;

static int streamer_buffering;

// to allow interruption of stall file requests
static DB_FILE *streamer_file;

void
streamer_lock (void) {
    mutex_lock (mutex);
}

void
streamer_unlock (void) {
    mutex_unlock (mutex);
}

static void
streamer_abort_files (void) {
    if (fileinfo && fileinfo->file) {
        trace ("\033[0;31maborting current song: %s (fileinfo %p, file %p)\033[37;0m\n", streaming_track ? streaming_track->fname : NULL, fileinfo, fileinfo ? fileinfo->file : NULL);
        deadbeef->fabort (fileinfo->file);
        trace ("\033[0;31maborting current song done\033[37;0m\n");
    }
    mutex_lock (decodemutex);
    if (streamer_file) {
        trace ("\033[0;31maborting streamer_file\033[37;0m\n");
        deadbeef->fabort (streamer_file);
    }
    mutex_unlock (decodemutex);
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
    playlist_track = it;
    // assign new
    playing_track = it;
    if (playing_track) {
        pl_item_ref (playing_track);
        playing_track->played = 1;
        playing_track->started_timestamp = time (NULL);
        trace ("sending songstarted to plugins [2] current playtrack: %s\n", playing_track->fname);
        plug_trigger_event (DB_EV_SONGSTARTED, 0);
        trace ("from=%p (%s), to=%p (%s) [2]\n", from, from ? from->fname : "null", it, it ? it->fname : "null");
        plug_trigger_event_trackchange (from, it);
    }
    if (from) {
        pl_item_unref (from);
    }
    if (it) {
        pl_item_unref (it);
    }
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
    plt_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr_ptr ();
    }
    playItem_t *c = streamer_playlist->head[PL_MAIN];
    int idx = 0;
    while (c && c != it) {
        c = c->next[PL_MAIN];
        idx++;
    }
    if (!c) {
        plt_unlock ();
        return -1;
    }
    plt_unlock ();
    return idx;
}

playItem_t *
str_get_for_idx (int idx) {
    plt_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr_ptr ();
    }
    playItem_t *it = streamer_playlist->head[PL_MAIN];
    while (idx--) {
        if (!it) {
            plt_unlock ();
            return NULL;
        }
        it = it->next[PL_MAIN];
    }
    if (it) {
        pl_item_ref (it);
    }
    plt_unlock ();
    return it;
}


int
streamer_move_to_nextsong (int reason) {
    trace ("streamer_move_to_nextsong (%d)\n", reason);
    pl_global_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr_ptr ();
    }
    while (pl_playqueue_getcount ()) {
        trace ("pl_playqueue_getnext\n");
        playItem_t *it = pl_playqueue_getnext ();
        if (it) {
            pl_playqueue_pop ();
            int r = str_get_idx_of (it);
            if (r >= 0) {
                pl_item_unref (it);
                streamer_set_nextsong (r, 1);
                pl_global_unlock ();
                return 0;
            }
            else {
                trace ("%s not found in current streaming playlist\n", it->fname);
                // find playlist
                playlist_t *old = streamer_playlist;
                streamer_playlist = plt_get_list ();
                int i = 0;
                while (streamer_playlist) {
                    trace ("searching for %s in playlist %d\n", it->fname, i);
                    int r = str_get_idx_of (it);
                    if (r >= 0) {
                        trace ("%s found in playlist %d\n", it->fname, i);
                        pl_item_unref (it);
                        streamer_set_nextsong (r, 3);
                        pl_global_unlock ();
                        return 0;
                    }
                    i++;
                    streamer_playlist = streamer_playlist->next;
                }
                trace ("%s not found in any playlists\n", it->fname);
                streamer_playlist = old;
                pl_item_unref (it);
            }
        }
    }
    playItem_t *curr = playlist_track;
    if (reason == 1) {
        streamer_playlist = plt_get_curr_ptr ();
        // check if prev song is in this playlist
        if (-1 == str_get_idx_of (curr)) {
            curr = NULL;
        }
    }

    playlist_t *plt = streamer_playlist;
    if (!plt->head[PL_MAIN]) {
        streamer_set_nextsong (-2, 1);
        pl_global_unlock ();
        return 0;
    }
    int pl_order = pl_get_order ();

    int pl_loop_mode = conf_get_int ("playback.loop", 0);

    if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
        int r = str_get_idx_of (playing_track);
        if (r == -1) {
            streamer_set_nextsong (-2, 1);
        }
        else {
            streamer_set_nextsong (r, 1);
        }
        pl_global_unlock ();
        return 0;
    }

    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        if (!curr) {
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
                pl_global_unlock ();
                return -1;
            }
            int r = str_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            pl_global_unlock ();
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
                playItem_t *temp;
                plt_reshuffle (streamer_playlist, &temp, NULL);
                streamer_set_nextsong (-2, 1);
                pl_global_unlock ();
                return -1;
            }
            int r = str_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            pl_global_unlock ();
            return 0;
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
                streamer_set_nextsong (-2, 1);
                pl_global_unlock ();
                return 0;
            }
        }
        if (!it) {
            pl_global_unlock ();
            return -1;
        }
        int r = str_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        pl_global_unlock ();
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        int res = streamer_move_to_randomsong ();
        if (res == -1) {
            trace ("streamer_move_to_randomsong error\n");
            pl_global_unlock ();
            streamer_set_nextsong (-2, 1);
            return -1;
        }
    }
    pl_global_unlock ();
    return -1;
}

int
streamer_move_to_prevsong (void) {
    plt_lock ();
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr_ptr ();
    }
    streamer_playlist = plt_get_curr_ptr ();
    // check if prev song is in this playlist
    if (-1 == str_get_idx_of (playlist_track)) {
        playlist_track = NULL;
    }

    playlist_t *plt = streamer_playlist;
    pl_playqueue_clear ();
    if (!plt->head[PL_MAIN]) {
        streamer_set_nextsong (-2, 1);
        plt_unlock ();
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE_TRACKS || pl_order == PLAYBACK_ORDER_SHUFFLE_ALBUMS) { // shuffle
        if (!playlist_track) {
            plt_unlock ();
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
                        pl_reshuffle (NULL, &amax);
                    }
                    it = amax;
                }
            }

            if (!it) {
                plt_unlock ();
                return -1;
            }
            int r = str_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            plt_unlock ();
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
            plt_unlock ();
            return -1;
        }
        int r = str_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        plt_unlock ();
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        int res = streamer_move_to_randomsong ();
        if (res == -1) {
            plt_unlock ();
            streamer_set_nextsong (-2, 1);
            trace ("streamer_move_to_randomsong error\n");
            return -1;
        }
    }
    plt_unlock ();
    return -1;
}

int
streamer_move_to_randomsong (void) {
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr_ptr ();
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
    streamer_set_nextsong (r, 1);
    return 0;
}

// playlist must call that whenever item was removed
void
streamer_song_removed_notify (playItem_t *it) {
    if (!mutex) {
        return; // streamer is not running
    }
    streamer_lock ();
    if (it == playlist_track) {
        playlist_track = playlist_track->next[PL_MAIN];
        // queue new next song for streaming
        if (bytes_until_next_song > 0) {
            streamer_ringbuf.remaining = bytes_until_next_song;
            streamer_move_to_nextsong (0);
        }
    }
    streamer_unlock ();
}

// that must be called after last sample from str_playing_song was done reading
static int
streamer_set_current (playItem_t *it) {
    DB_output_t *output = plug_get_output ();
    int err = 0;
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
        trace ("buffering = on\n");
        streamer_buffering = 1;
        trace ("\033[0;35mstreamer_start_playback[1] from %p to %p\033[37;0m\n", from, it);
        streamer_start_playback (from, it);
        bytes_until_next_song = -1;
    }

// code below breaks seekbar drawing during transition between tracks
    trace ("streamer_set_current %p, buns=%d\n", it, bytes_until_next_song);
    mutex_lock (decodemutex);
    if (fileinfo) {
        fileinfo->plugin->free (fileinfo);
        fileinfo = NULL;
        if (streaming_track) {
            pl_item_unref (streaming_track);
            streaming_track = NULL;
        }
    }
    mutex_unlock (decodemutex);
    if (!it) {
        goto success;
    }
    if (to) {
        trace ("draw before init: %p->%p, playing_track=%p, playlist_track=%p\n", from, to, playing_track, playlist_track);
        plug_trigger_event_trackinfochanged (to);
    }
    if (from) {
        plug_trigger_event_trackinfochanged (from);
    }
    if (!it->decoder_id && it->filetype && !strcmp (it->filetype, "content")) {
        // try to get content-type
        mutex_lock (decodemutex);
        trace ("\033[0;34mopening file %s\033[37;0m\n", it->fname);
        DB_FILE *fp = streamer_file = vfs_fopen (it->fname);
        mutex_unlock (decodemutex);
        const char *plug = NULL;
        if (fp) {
            const char *ct = vfs_get_content_type (fp);
            if (ct) {
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
            }
            mutex_lock (decodemutex);
            streamer_file = NULL;
            vfs_fclose (fp);
            mutex_unlock (decodemutex);
        }
        if (plug) {
            DB_decoder_t **decoders = plug_get_decoder_list ();
            // match by decoder
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, plug)) {
                    it->decoder_id = decoders[i]->plugin.id;
                    it->filetype = decoders[i]->filetypes[0];
                    trace ("\033[0;34mfound plugin %s\033[37;0m\n", plug);
                }
            }
        }
        else {
            trace ("\033[0;34mclosed file %s (bad or interrupted)\033[37;0m\n", it->fname);
        }
    }
    if (it->decoder_id) {
        DB_decoder_t *dec = NULL;
        dec = plug_get_decoder_for_id (it->decoder_id);
        if (dec) {
            trace ("\033[0;33minit decoder for %s\033[37;0m\n", it->fname);
            fileinfo = dec->open (0);
            if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (it)) != 0) {
                trace ("\033[0;31mfailed to init decoder\033[37;0m\n")
                dec->free (fileinfo);
                fileinfo = NULL;
            }
        }

        if (!dec || !fileinfo) {
            it->played = 1;
            trace ("decoder->init returned %p\n", fileinfo);
            streamer_buffering = 0;
            if (playlist_track == it) {
                trace ("redraw track %d; playing_track=%p; playlist_track=%p\n", to, playing_track, playlist_track);
                plug_trigger_event_trackinfochanged (to);
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
            pl_item_ref (streaming_track);
            mutex_unlock (decodemutex);
            trace ("bps=%d, channels=%d, samplerate=%d\n", fileinfo->fmt.bps, fileinfo->fmt.channels, fileinfo->fmt.samplerate);
        }
    }
    else {
        trace ("no decoder in playitem!\n");
        it->played = 1;
        streamer_buffering = 0;
        if (playlist_track == it) {
            plug_trigger_event_trackinfochanged (to);
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
    plug_trigger_event_trackinfochanged (to);

    trace ("\033[0;32mstr: %p (%s), ply: %p (%s)\033[37;0m\n", streaming_track, streaming_track ? streaming_track->fname : "null", playing_track, playing_track ? playing_track->fname : "null");

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
    streamer_lock ();
    streamer_abort_files ();
    nextsong = song;
    nextsong_pstate = pstate;
    if (output->state () == OUTPUT_STATE_STOPPED) {
        if (pstate == 1) { // means user initiated this
            streamer_playlist = plt_get_curr_ptr ();
        }
        // no sense to wait until end of previous song, reset buffer
        bytes_until_next_song = 0;
        playpos = 0;
        seekpos = -1;
    }
    streamer_unlock ();
}

void
streamer_set_seek (float pos) {
    seekpos = pos;
}

static void
streamer_start_new_song (void) {
    trace ("nextsong=%d\n", nextsong);
    streamer_lock ();
    DB_output_t *output = plug_get_output ();
    int sng = nextsong;
    int initsng = nextsong;
    int pstate = nextsong_pstate;
    nextsong = -1;
    streamer_unlock ();
    if (badsong == sng) {
        trace ("looped to bad file. stopping...\n");
        streamer_set_nextsong (-2, 1);
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

        plug_trigger_event_trackchange (NULL, NULL);
        return;
    }
    int ret = streamer_set_current (try);

    if (ret < 0) {
        trace ("\033[0;31mfailed to play track %s, skipping (current=%p/%p)...\033[37;0m\n", try->fname, streaming_track, playlist_track);
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
            if (!dsp_on && fileinfo && memcmp (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                fprintf (stderr, "streamer_set_output_format %dbit %s %dch %dHz channelmask=%X\n", output_format.bps, output_format.is_float ? "float" : "int", output_format.channels, output_format.samplerate, output_format.channelmask);
                streamer_set_output_format ();
            }
            else if (output->state () != OUTPUT_STATE_PLAYING) {
                // might have failed last time, set default params
                output_format.bps = 16;
                output_format.is_float = 0;
                output_format.channels = 2;
                output_format.samplerate = 44100;
                output_format.channelmask = 3;
                streamer_set_output_format ();
            }
            if (output->state () != OUTPUT_STATE_PLAYING) {
                if (0 != output->play ()) {
                    memset (&output_format, 0, sizeof (output_format));
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
            if (fileinfo && memcmp (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                formatchanged = 1;
            }
            // we need to start playback before we can pause it
            if (0 != output->play ()) {
                memset (&output_format, 0, sizeof (output_format));
                fprintf (stderr, "streamer: failed to start playback (start track)\n");
                streamer_set_nextsong (-2, 0);
            }
        }
        output->pause ();
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
                plug_trigger_event (DB_EV_SONGFINISHED, 0);
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
                plug_trigger_event (DB_EV_SONGFINISHED, 0);
            }
            if (from) {
                pl_item_ref (from);
            }
            streamer_set_current (NULL);
            if (playing_track) {
                pl_item_unref (playing_track);
                playing_track = NULL;
            }
            plug_trigger_event_trackchange (from, NULL);
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
                plug_trigger_event (DB_EV_SONGFINISHED, 0);
            }
            // copy streaming into playing
            trace ("\033[0;35mstreamer_start_playback[2] from %p to %p\033[37;0m\n", playing_track, streaming_track);
            streamer_start_playback (playing_track, streaming_track);
            last_bitrate = -1;
            avg_bitrate = -1;
            playlist_track = playing_track;
            playpos = 0;
            seekpos = -1;

            // don't switch if unchanged
            ddb_waveformat_t prevfmt;
            memcpy (&prevfmt, &output->fmt, sizeof (ddb_waveformat_t));
            if (memcmp (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
                memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
                output->setformat (&fileinfo->fmt);
                // check if the format actually changed
                if (memcmp (&output->fmt, &prevfmt, sizeof (ddb_waveformat_t))) {
                    // restart streaming of current track
                    trace ("streamer: output samplerate changed from %d to %d; restarting track\n", prevfmt.samplerate, output->fmt.samplerate);
                    mutex_lock (decodemutex);
                    fileinfo->plugin->free (fileinfo);
                    fileinfo = NULL;
                    DB_decoder_t *dec = NULL;
                    dec = plug_get_decoder_for_id (streaming_track->decoder_id);
                    if (dec) {
                        fileinfo = dec->open (0);
                        if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (streaming_track)) < 0) {
                            dec->free (fileinfo);
                            fileinfo = NULL;
                        }
                    }
                    if (!dec || !fileinfo) {
                        // FIXME: handle error
                    }
                    mutex_unlock (decodemutex);
                    bytes_until_next_song = -1;
                    streamer_buffering = 1;
                    streamer_reset (1);
                    if (output->state () != OUTPUT_STATE_PLAYING) {
                        if (0 != output->play ()) {
                            memset (&output_format, 0, sizeof (output_format));
                            fprintf (stderr, "streamer: failed to start playback (track transition format change)\n");
                            streamer_set_nextsong (-2, 0);
                        }
                    }
                }
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
                    pl_item_unref (streaming_track);
                    streaming_track = NULL;
                }
                streaming_track = playing_track;
                if (streaming_track) {
                    pl_item_ref (streaming_track);
                }
                mutex_unlock (decodemutex);

                bytes_until_next_song = -1;
                streamer_buffering = 1;
                if (streaming_track) {
                    plug_trigger_event_trackinfochanged (streaming_track);
                }

                mutex_lock (decodemutex);
                DB_decoder_t *dec = NULL;
                dec = plug_get_decoder_for_id (streaming_track->decoder_id);
                if (dec) {
                    fileinfo = dec->open (0);
                    if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (streaming_track)) != 0) {
                        dec->free (fileinfo);
                        fileinfo = NULL;
                    }
                }
                mutex_unlock (decodemutex);

                if (!dec || !fileinfo) {
                    if (streaming_track) {
                        plug_trigger_event_trackinfochanged (streaming_track);
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
                plug_trigger_event_trackinfochanged (streaming_track);
            }
            if (fileinfo && playing_track && playing_track->_duration > 0) {
                streamer_lock ();
                streamer_reset (1);
                if (fileinfo->plugin->seek (fileinfo, pos) >= 0) {
                    playpos = fileinfo->readpos;
                }
                last_bitrate = -1;
                avg_bitrate = -1;
                streamer_unlock();
            }
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

        streamer_lock ();
        if (streamer_ringbuf.remaining < (STREAM_BUFFER_SIZE-blocksize * MAX_DSP_RATIO)) {
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
                int nb = streamer_read_async (readbuffer+bytesread,sz-bytesread);
                struct timeval tm2;
                gettimeofday (&tm2, NULL);
                int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
                if (ms >= alloc_time) {
                    break;
                }
                bytesread += nb;
            } while (bytesread < sz-100);
            streamer_lock ();

            if (bytesread > 0) {
                ringbuf_write (&streamer_ringbuf, readbuffer, bytesread);
            }

            //fprintf (stderr, "fill: %d, read: %d, size=%d, blocksize=%d\n", streamer_ringbuf.remaining, bytesread, STREAM_BUFFER_SIZE, blocksize);
        }
        streamer_unlock ();
        if ((streamer_ringbuf.remaining > 128000 && streamer_buffering) || !streaming_track) {
            streamer_buffering = 0;
            if (streaming_track) {
                plug_trigger_event_trackinfochanged (streaming_track);
            }
        }
        struct timeval tm2;
        gettimeofday (&tm2, NULL);

        int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
        //fprintf (stderr, "slept %dms (alloc=%dms, bytespersec=%d, chan=%d, blocksize=%d), fill: %d/%d (cursor=%d)\n", alloc_time-ms, alloc_time, bytes_in_one_second, output->fmt.channels, blocksize, streamer_ringbuf.remaining, STREAM_BUFFER_SIZE, streamer_ringbuf.cursor);
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
        if (memcmp (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t))) {
            memcpy (&output_format, &fileinfo->fmt, sizeof (ddb_waveformat_t));
            formatchanged = 1;
        }
    }
    else if (ctx) {
        dsp_on = 1;
        // set some very generic format, this will allow playback of weird
        // formats after fixing them with dsp plugins
        output_format.bps = 16;
        output_format.is_float = 0;
        output_format.channels = 2;
        output_format.samplerate = 44100;
        output_format.channelmask = 3;
        streamer_set_output_format ();
    }
    else if (!ctx) {
        dsp_on = 0;
    }
}

void
streamer_dsp_init (void) {
    // load dsp chain from file
    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    dsp_chain = streamer_dsp_chain_load (fname);

    eqplug = (DB_dsp_t *)plug_get_for_id ("supereq");
    streamer_dsp_postinit ();

    // load legacy eq settings from pre-0.5
    if (conf_find ("eq.", NULL)) {
        eq->enabled = deadbeef->conf_get_int ("eq.enable", 0);
        eqplug->set_param (eq, 0, conf_get_str ("eq.preamp", "0"));
        for (int i = 0; i < 18; i++) {
            char key[100];
            snprintf (key, sizeof (key), "eq.band%d", i);
            eqplug->set_param (eq, 1+i, conf_get_str (key, "0"));
        }
        // delete obsolete settings
        conf_remove_items ("eq.");
    }
}

int
streamer_init (void) {
#if WRITE_DUMP
    out = fopen ("out.raw", "w+b");
#endif
    mutex = mutex_create ();
    decodemutex = mutex_create ();

    ringbuf_init (&streamer_ringbuf, streambuffer, STREAM_BUFFER_SIZE);

    pl_set_order (conf_get_int ("playback.order", 0));

    streamer_dsp_init ();
    
    conf_replaygain_mode = conf_get_int ("replaygain_mode", 0);
    conf_replaygain_scale = conf_get_int ("replaygain_scale", 1);
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

int replaygain = 1;
int replaygain_scale = 1;

static void
apply_replay_gain_int16 (playItem_t *it, char *bytes, int size) {
    if (!replaygain || !conf_replaygain_mode) {
        return;
    }
    int vol = 1000;
    if (conf_replaygain_mode == 1) {
        if (it->replaygain_track_gain == 0) {
            return;
        }
        vol = db_to_amp (streaming_track->replaygain_track_gain) * 1000;
        if (conf_replaygain_scale && replaygain_scale) {
            if (vol * streaming_track->replaygain_track_peak > 1000) {
                vol = 1000 / streaming_track->replaygain_track_peak;
            }
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (it->replaygain_album_gain == 0) {
            return;
        }
        vol = db_to_amp (streaming_track->replaygain_album_gain) * 1000;
        if (conf_replaygain_scale && replaygain_scale) {
            if (vol * streaming_track->replaygain_album_peak > 1000) {
                vol = 1000 / streaming_track->replaygain_album_peak;
            }
        }
    }
    int16_t *s = (int16_t*)bytes;
    for (int j = 0; j < size/2; j++) {
        int32_t sample = ((int32_t)(*s)) * vol / 1000;
        if (sample > 0x7fff) {
            sample = 0x7fff;
        }
        else if (sample < -0x8000) {
            sample = -0x8000;
        }
        *s = (int16_t)sample;
        s++;
    }
}

static void
apply_replay_gain_float32 (playItem_t *it, char *bytes, int size) {
    if (!replaygain || !conf_replaygain_mode) {
        return;
    }
    float vol = 1.f;
    if (conf_replaygain_mode == 1) {
        if (it->replaygain_track_gain == 0) {
            return;
        }
        vol = db_to_amp (it->replaygain_track_gain);
        if (conf_replaygain_scale && replaygain_scale) {
            if (vol * it->replaygain_track_peak > 1.f) {
                vol = 1.f / it->replaygain_track_peak;
            }
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (it->replaygain_album_gain == 0) {
            return;
        }
        vol = db_to_amp (it->replaygain_album_gain);
        if (conf_replaygain_scale && replaygain_scale) {
            if (vol * it->replaygain_album_peak > 1.f) {
                vol = 1.f / it->replaygain_album_peak;
            }
        }
    }
    float *s = (float*)bytes;
    for (int j = 0; j < size/4; j++) {
        float sample = ((float)*s) * vol;
        if (sample > 1.f) {
            sample = 1.f;
        }
        else if (sample < -1.f) {
            sample = -1.f;
        }
        *s = sample;
        s++;
    }
}

static int
streamer_set_output_format (void) {
    DB_output_t *output = plug_get_output ();
    int playing = (output->state () == OUTPUT_STATE_PLAYING);

    fprintf (stderr, "streamer_set_output_format %dbit %s %dch %dHz channelmask=%X\n", output_format.bps, output_format.is_float ? "float" : "int", output_format.channels, output_format.samplerate, output_format.channelmask);
    output->setformat (&output_format);
    if (playing && output->state () != OUTPUT_STATE_PLAYING) {
        if (0 != output->play ()) {
            memset (&output_format, 0, sizeof (output_format));
            fprintf (stderr, "streamer: failed to start playback (streamer_read format change)\n");
            streamer_set_nextsong (-2, 0);
            return -1;
        }
    }
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

        if (!memcmp (&fileinfo->fmt, &output->fmt, sizeof (ddb_waveformat_t)) && !dsp_on) {
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

            ddb_waveformat_t dspfmt;
            memcpy (&dspfmt, &fileinfo->fmt, sizeof (ddb_waveformat_t));
            dspfmt.bps = 32;
            dspfmt.is_float = 1;

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
                if (memcmp (&output_format, &outfmt, sizeof (ddb_waveformat_t)) && bytes_until_next_song <= 0) {
                    memcpy (&output_format, &outfmt, sizeof (ddb_waveformat_t));
                    streamer_set_output_format ();
                }

                //printf ("convert to %dbit %s %dch %dHz channelmask=%X\n", output->fmt.bps, output->fmt.is_float ? "float" : "int", output->fmt.channels, output->fmt.samplerate, output->fmt.channelmask);
                int n = pcm_convert (&dspfmt, tempbuf, &output->fmt, bytes, nframes * dspsamplesize);

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

        // FIXME: separate replaygain DSP plugin?
        if (output->fmt.bps == 16) {
            apply_replay_gain_int16 (streaming_track, bytes, bytesread);
        }
        else if (output->fmt.bps == 32 && output->fmt.is_float) {
            apply_replay_gain_float32 (streaming_track, bytes, bytesread);
        }
    }
    mutex_unlock (decodemutex);
    if (!is_eof) {
        return bytesread;
    }
    else  {
        // that means EOF
        trace ("streamer: EOF! buns: %d, bytesread: %d, buffering: %d, bufferfill: %d\n", bytes_until_next_song, bytesread, streamer_buffering, streamer_ringbuf.remaining);

        // in case of decoder error, or EOF while buffering - switch to next song instantly
        if (bytesread < 0 || (bytes_until_next_song >= 0 && streamer_buffering && bytesread == 0) || bytes_until_next_song < 0) {
            trace ("finished streaming song, queueing next\n");
            bytes_until_next_song = streamer_ringbuf.remaining;
            if (conf_get_int ("playlist.stop_after_current", 0)) {
                streamer_set_nextsong (-2, 1);
            }
            else {
                streamer_move_to_nextsong (0);
            }
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
        formatchanged = 0;
        streamer_set_output_format ();
    }
    streamer_lock ();
    int sz = min (size, streamer_ringbuf.remaining);
    if (sz) {
        ringbuf_read (&streamer_ringbuf, bytes, sz);
        playpos += (float)sz/output->fmt.samplerate/((output->fmt.bps>>3)*output->fmt.channels) * dsp_ratio;
        playing_track->playtime += (float)sz/output->fmt.samplerate/((output->fmt.bps>>3)*output->fmt.channels);
        if (playlist_track) {
            playing_track->filetype = playlist_track->filetype;
        }
        if (playlist_track) {
            playlist_track->playtime = playing_track->playtime;
        }
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
    conf_replaygain_mode = conf_get_int ("replaygain_mode", 0);
    conf_replaygain_scale = conf_get_int ("replaygain_scale", 1);
    pl_set_order (conf_get_int ("playback.order", 0));
}

void
streamer_play_current_track (void) {
    playlist_t *plt = plt_get_curr_ptr ();
    DB_output_t *output = plug_get_output ();
    if (output->state () == OUTPUT_STATE_PAUSED && playing_track) {
        // unpause currently paused track
        output->unpause ();
        plug_trigger_event_paused (0);
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
        streamer_playlist = plt;
    }
    else {
        // restart currently playing track
        output->stop ();
        streamer_set_nextsong (0, 1);
    }
}

struct DB_fileinfo_s *
streamer_get_current_fileinfo (void) {
    return fileinfo;
}

void
streamer_set_current_playlist (int plt) {
    streamer_playlist = plt_get (plt);
}

int
streamer_get_current_playlist (void) {
    if (!streamer_playlist) {
        streamer_playlist = plt_get_curr_ptr ();
    }
    return plt_get_idx_of (streamer_playlist);
}

void
streamer_notify_playlist_deleted (playlist_t *plt) {
    if (plt == streamer_playlist) {
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

    char fname[PATH_MAX];
    snprintf (fname, sizeof (fname), "%s/dspconfig", plug_get_config_dir ());
    streamer_dsp_chain_save (fname, dsp_chain);
    streamer_reset (1);

    mutex_unlock (decodemutex);
    DB_output_t *output = plug_get_output ();
    if (playing_track && output->state () != OUTPUT_STATE_STOPPED) {
        deadbeef->streamer_seek (playpos);
    }
}
