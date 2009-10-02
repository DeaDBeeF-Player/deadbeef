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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <samplerate.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include "threading.h"
#include "codec.h"
#include "playlist.h"
#include "common.h"
#include "streamer.h"
#include "playback.h"
#include "messagepump.h"
#include "messages.h"
#include "conf.h"
#include "plugins.h"
#include "optmath.h"
#include "volume.h"

static intptr_t streamer_tid;
static SRC_STATE *src;
static SRC_DATA srcdata;
static int codecleft;

// that's buffer for resampling.
// our worst case is 192KHz downsampling to 22050Hz with 2048 sample output buffer
#define INPUT_BUFFER_SIZE (2048*192000/22050*8)
static char g_readbuffer[INPUT_BUFFER_SIZE];
// fbuffer contains readbuffer converted to floating point, to pass to SRC
static float g_fbuffer[INPUT_BUFFER_SIZE];
// output SRC buffer - can't really exceed INPUT_BUFFER_SIZE
#define SRC_BUFFER_SIZE (INPUT_BUFFER_SIZE*2)
static float g_srcbuffer[SRC_BUFFER_SIZE];
static int streaming_terminate;

// buffer up to 3 seconds at 44100Hz stereo
#define STREAM_BUFFER_SIZE 0x80000 // slightly more than 3 seconds of 44100 stereo
#define STREAM_BUFFER_MASK 0x7ffff

static int streambuffer_fill;
static int streambuffer_pos;
static int bytes_until_next_song = 0;
static char streambuffer[STREAM_BUFFER_SIZE];
static uintptr_t mutex;
static int nextsong = -1;
static int nextsong_pstate = -1;
static int badsong = -1;

static float seekpos = -1;

static float playpos = 0; // play position of current song

playItem_t str_playing_song;
playItem_t str_streaming_song;
// remember pointers to original instances of playitems
static playItem_t *orig_playing_song;
static playItem_t *orig_streaming_song;

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

// playlist must call that whenever item was removed
void
streamer_song_removed_notify (playItem_t *it) {
    if (it == orig_playing_song) {
        orig_playing_song = NULL;
    }
    if (it == orig_streaming_song) {
        orig_streaming_song = NULL;
        // queue new next song for streaming
        if (bytes_until_next_song > 0) {
            streambuffer_fill = bytes_until_next_song;
            pl_nextsong (0);
        }
    }
}

// that must be called after last sample from str_playing_song was done reading
static int
streamer_set_current (playItem_t *it) {
    trace ("streamer_set_current %p, buns=%d\n", it);
//    if (str_streaming_song.decoder) {
//        trace ("sending songfinished to plugins [1]\n");
//        plug_trigger_event (DB_EV_SONGFINISHED);
//        str_streaming_song.decoder->free ();
//    }
    if(str_streaming_song.decoder) {
        str_streaming_song.decoder->free ();
        pl_item_free (&str_streaming_song);
    }
    orig_streaming_song = it;
    if (!it) {
        return 0;
    }
    if (it->decoder) {
        int ret = it->decoder->init (DB_PLAYITEM (it));
        trace ("input samplerate: %d\n", it->decoder->info.samplerate);
        pl_item_copy (&str_streaming_song, it);
        if (ret < 0) {
            trace ("decoder->init returned %d\n", ret);
            return ret;
        }
        else {
            trace ("bps=%d, channels=%d, samplerate=%d\n", it->decoder->info.bps, it->decoder->info.channels, it->decoder->info.samplerate);
        }
        streamer_reset (0); // reset SRC
    }
    else {
        trace ("no decoder in playitem!\n");
        orig_streaming_song = NULL;
        return -1;
    }
    if (bytes_until_next_song == -1) {
        bytes_until_next_song = 0;
    }
    return 0;
}

#if 0
static int
str_set_current (playItem_t *it) {
    int ret = 0;
    int from = pl_get_idx_of (playlist_current_ptr);
    int to = it ? pl_get_idx_of (it) : -1;
    if (str_playing_song.decoder) {
        plug_trigger_event (DB_EV_SONGFINISHED);
    }
    codec_lock ();
    if (str_playing_song.decoder) {
        str_playing_song.decoder->free ();
    }
    pl_item_free (&str_playing_song);
    playlist_current_ptr = it;
    if (it && it->decoder) {
        // don't do anything on fail, streamer will take care
        ret = it->decoder->init (DB_PLAYITEM (it));
//        if (ret < 0) {
//        }
    }
    if (playlist_current_ptr) {
        streamer_reset (0);
    }
    if (it) {
        it->played = 1;
        it->started_timestamp = time (NULL);
        pl_item_copy (&str_playing_song, it);
    }
    codec_unlock ();
    if (it) {
        plug_trigger_event (DB_EV_SONGSTARTED);
    }
    messagepump_push (M_SONGCHANGED, 0, from, to);
    return ret;
}
#endif

float
streamer_get_playpos (void) {
    return playpos;
}

void
streamer_set_nextsong (int song, int pstate) {
    trace ("streamer_set_nextsong %d %d\n", song, pstate);
    nextsong = song;
    nextsong_pstate = pstate;
    if (p_isstopped ()) {
        // no sense to wait until end of previous song, reset buffer
        bytes_until_next_song = 0;
    }
}

void
streamer_set_seek (float pos) {
    seekpos = pos;
}

static int
streamer_read_async (char *bytes, int size);

void
streamer_thread (uintptr_t ctx) {
    prctl (PR_SET_NAME, "deadbeef-stream", 0, 0, 0, 0);
    codecleft = 0;

    while (!streaming_terminate) {
        struct timeval tm1;
        gettimeofday (&tm1, NULL);
        if (nextsong >= 0) { // start streaming next song
            trace ("nextsong=%d\n", nextsong);
            int sng = nextsong;
            int pstate = nextsong_pstate;
            nextsong = -1;
            codec_lock ();
            codecleft = 0;
            codec_unlock ();
            if (badsong == sng) {
                trace ("looped to bad file. stopping...\n");
                streamer_set_nextsong (-2, 1);
                badsong = -1;
                continue;
            }
            int ret = streamer_set_current (pl_get_for_idx (sng));
            if (ret < 0) {
                trace ("bad file in playlist, skipping...\n");
                // remember bad song number in case of looping
                if (badsong == -1) {
                    badsong = sng;
                }
                // try jump to next song
                pl_nextsong (0);
                usleep (50000);
                continue;
            }
            badsong = -1;
            if (pstate == 0) {
                p_stop ();
            }
            else if (pstate == 1) {
                p_play ();
            }
            else if (pstate == 2) {
                p_pause ();
            }
        }
        else if (nextsong == -2) {
            trace ("nextsong=-2\n");
            nextsong = -1;
            p_stop ();
            if (str_playing_song.decoder) {
                trace ("sending songfinished to plugins [1]\n");
                plug_trigger_event (DB_EV_SONGFINISHED);
            }
            messagepump_push (M_SONGCHANGED, 0, pl_get_idx_of (orig_playing_song), -1);
            streamer_set_current (NULL);
            pl_item_free (&str_playing_song);
            orig_playing_song = NULL;
            continue;
        }
        else if (p_isstopped ()) {
            usleep (50000);
            continue;
        }

        if (bytes_until_next_song == 0) {
            if (!str_streaming_song.fname) {
                // means last song was deleted during final drain
                nextsong = -1;
                p_stop ();
                messagepump_push (M_SONGCHANGED, 0, pl_get_idx_of (playlist_current_ptr), -1);
                streamer_set_current (NULL);
                continue;
            }
            trace ("bytes_until_next_song=0, starting playback of new song\n");
            bytes_until_next_song = -1;
            // plugin will get pointer to str_playing_song
            if (str_playing_song.decoder) {
                trace ("sending songfinished to plugins [2]\n");
                plug_trigger_event (DB_EV_SONGFINISHED);
            }
            // free old copy of playing
            pl_item_free (&str_playing_song);
            // copy streaming into playing
            pl_item_copy (&str_playing_song, &str_streaming_song);
            int from = orig_playing_song ? pl_get_idx_of (orig_playing_song) : -1;
            int to = orig_streaming_song ? pl_get_idx_of (orig_streaming_song) : -1;
            trace ("from=%d, to=%d\n", from, to);
            orig_playing_song = orig_streaming_song;
            if (orig_playing_song) {
                orig_playing_song->played = 1;
                orig_playing_song->started_timestamp = time (NULL);
                str_playing_song.started_timestamp = time (NULL);
            }
            playlist_current_ptr = orig_playing_song;
            // that is needed for playlist drawing
            trace ("sending songchanged\n");
            messagepump_push (M_SONGCHANGED, 0, from, to);
            // plugin will get pointer to new str_playing_song
            trace ("sending songstarted to plugins\n");
            plug_trigger_event (DB_EV_SONGSTARTED);
            playpos = 0;
        }

        if (seekpos >= 0) {
            trace ("seeking to %f\n", seekpos);
            float pos = seekpos;
            seekpos = -1;

            if (orig_playing_song != orig_streaming_song) {
                // restart playing from new position
                if(str_streaming_song.decoder) {
                    str_streaming_song.decoder->free ();
                    pl_item_free (&str_streaming_song);
                }
                orig_streaming_song = orig_playing_song;
                pl_item_copy (&str_streaming_song, orig_streaming_song);
                bytes_until_next_song = -1;
            }

            if (str_playing_song.decoder && str_playing_song.decoder->seek (pos) >= 0) {
                streamer_lock ();
                playpos = str_playing_song.decoder->info.readpos;
                streambuffer_fill = 0;
                streambuffer_pos = 0;
                streamer_unlock ();
                codec_lock ();
                codecleft = 0;
                codec_unlock ();
            }
        }

        // read ahead at 384K per second
        // that means 10ms per 4k block, or 40ms per 16k block
        int alloc_time = 1000 / (96000 * 4 / 4096);

        streamer_lock ();
        if (streambuffer_fill < (STREAM_BUFFER_SIZE-4096)/* && bytes_until_next_song == 0*/) {
            int sz = STREAM_BUFFER_SIZE - streambuffer_fill;
            int minsize = 4096;
            if (streambuffer_fill < 16384) {
                minsize = 16384;
                alloc_time *= 4;
            }
            sz = min (minsize, sz);
            assert ((sz&3) == 0);

            int writepos = (streambuffer_pos + streambuffer_fill) & STREAM_BUFFER_MASK;
            int part1 = STREAM_BUFFER_SIZE-writepos;
            part1 = min (part1, sz);
            if (part1 > 0) {
                streamer_unlock ();
                int bytesread = streamer_read_async (streambuffer + writepos, part1);
                streamer_lock ();
                streambuffer_fill += bytesread;
            }
            sz -= part1;
            if (sz > 0) {
                streamer_unlock ();
                int bytesread = streamer_read_async (streambuffer, sz);
                streamer_lock ();
                streambuffer_fill += bytesread;
            }
        }
        streamer_unlock ();
        struct timeval tm2;
        gettimeofday (&tm2, NULL);

        int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
        alloc_time -= ms;
        if (alloc_time > 0) {
            usleep (alloc_time * 1000);
        }
//        trace ("fill: %d/%d\n", streambuffer_fill, STREAM_BUFFER_SIZE);
    }

    // stop streaming song
    if(str_streaming_song.decoder) {
        str_streaming_song.decoder->free ();
    }
    pl_item_free (&str_streaming_song);
    pl_item_free (&str_playing_song);
    if (src) {
        src_delete (src);
        src = NULL;
    }
}

int
streamer_init (void) {
    mutex = mutex_create ();
//    src = src_new (SRC_SINC_BEST_QUALITY, 2, NULL);
//    src = src_new (SRC_LINEAR, 2, NULL);
    src = src_new (conf_src_quality, 2, NULL);
    if (!src) {
        return -1;
    }
    streamer_tid = thread_start (streamer_thread, 0);
    return 0;
}

void
streamer_free (void) {
    streaming_terminate = 1;
    thread_join (streamer_tid);
    mutex_free (mutex);
}

void
streamer_reset (int full) { // must be called when current song changes by external reasons
    codecleft = 0;
    if (full) {
        streambuffer_pos = 0;
        streambuffer_fill = 0;
    }
    src_reset (src);
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
        if (conf_replaygain_scale && replaygain_scale) {
            vol = db_to_amp (str_streaming_song.replaygain_track_gain)/str_streaming_song.replaygain_track_peak * 1000;
        }
        else {
            vol = db_to_amp (str_streaming_song.replaygain_track_gain) * 1000;
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (it->replaygain_album_gain == 0) {
            return;
        }
        if (conf_replaygain_scale && replaygain_scale) {
            vol = db_to_amp (str_streaming_song.replaygain_album_gain)/str_streaming_song.replaygain_album_peak * 1000;
        }
        else {
            vol = db_to_amp (str_streaming_song.replaygain_album_gain) * 1000;
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
        if (conf_replaygain_scale && replaygain_scale) {
            vol = db_to_amp (it->replaygain_track_gain)/it->replaygain_track_peak;
        }
        else {
            vol = db_to_amp (it->replaygain_track_gain);
        }
    }
    else if (conf_replaygain_mode == 2) {
        if (it->replaygain_album_gain == 0) {
            return;
        }
        if (conf_replaygain_scale && replaygain_scale) {
            vol = db_to_amp (it->replaygain_album_gain)/it->replaygain_album_peak;
        }
        else {
            vol = db_to_amp (it->replaygain_album_gain);
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

static void
mono_int16_to_stereo_int16 (int16_t *in, int16_t *out, int nsamples) {
    while (nsamples > 0) {
        int16_t sample = *in++;
        *out++ = sample;
        *out++ = sample;
        nsamples--;
    }
}

static void
int16_to_float32 (int16_t *in, float *out, int nsamples) {
    while (nsamples > 0) {
        *out++ = (*in++)/(float)0x7fff;
        nsamples--;
    }
}

static void
mono_int16_to_stereo_float32 (int16_t *in, float *out, int nsamples) {
    while (nsamples > 0) {
        float sample = (*in++)/(float)0x7fff;
        *out++ = sample;
        *out++ = sample;
        nsamples--;
    }
}

static void
mono_float32_to_stereo_float32 (float *in, float *out, int nsamples) {
    while (nsamples > 0) {
        float sample = *in++;
        *out++ = sample;
        *out++ = sample;
        nsamples--;
    }
}

static void
float32_to_int16 (float *in, int16_t *out, int nsamples) {
    fpu_control ctl;
    fpu_setround (&ctl);
    while (nsamples > 0) {
        float sample = *in++;
        if (sample > 1) {
            sample = 1;
        }
        else if (sample < -1) {
            sample = -1;
        }
        *out++ = (int16_t)ftoi (sample*0x7fff);
        nsamples--;
    }
    fpu_restore (ctl);
}

// returns number of bytes been read
static int
streamer_read_async (char *bytes, int size) {
    int initsize = size;
    for (;;) {
        int bytesread = 0;
        codec_lock ();
        DB_decoder_t *decoder = str_streaming_song.decoder;
        if (!decoder) {
            // means there's nothing left to stream, so just do nothing
            codec_unlock ();
            break;
        }
        if (decoder->info.samplerate != -1) {
            int nchannels = decoder->info.channels;
            int samplerate = decoder->info.samplerate;
            if (decoder->info.samplerate == p_get_rate ()) {
                // samplerate match
                int i;
                if (decoder->info.channels == 2) {
                    bytesread = decoder->read_int16 (bytes, size);
                    apply_replay_gain_int16 (&str_streaming_song, bytes, size);
                    codec_unlock ();
                }
                else {
                    bytesread = decoder->read_int16 (g_readbuffer, size>>1);
                    apply_replay_gain_int16 (&str_streaming_song, g_readbuffer, size>>1);
                    mono_int16_to_stereo_int16 ((int16_t*)g_readbuffer, (int16_t*)bytes, size>>2);
                    bytesread *= 2;
                    codec_unlock ();
                }
            }
            else if (src_is_valid_ratio ((double)p_get_rate ()/samplerate)) {
                // read and do SRC
                int nsamples = size/4;
                nsamples = nsamples * samplerate / p_get_rate () * 2;
                // read data at source samplerate (with some room for SRC)
                int nbytes = (nsamples - codecleft) * 2 * nchannels;
                int samplesize = 2;
                if (nbytes <= 0) {
                    nbytes = 0;
                }
                else {
                    if (!decoder->read_float32) {
                        if (nbytes > INPUT_BUFFER_SIZE) {
                            trace ("input buffer overflow\n");
                            nbytes = INPUT_BUFFER_SIZE;
                        }
                        bytesread = decoder->read_int16 (g_readbuffer, nbytes);
                        apply_replay_gain_int16 (&str_streaming_song, g_readbuffer, nbytes);
                    }
                    else {
                        samplesize = 4;
                    }
                }
                codec_unlock ();
                // recalculate nsamples according to how many bytes we've got
                if (nbytes != 0) {
                    int i;
                    if (!decoder->read_float32) {
                        nsamples = bytesread / (samplesize * nchannels) + codecleft;
                        // convert to float
                        float *fbuffer = g_fbuffer + codecleft*2;
                        int n = nsamples - codecleft;
                        if (nchannels == 2) {
                            n <<= 1;
                            int16_to_float32 ((int16_t*)g_readbuffer, fbuffer, n);
                        }
                        else if (nchannels == 1) { // convert mono to stereo
                            mono_int16_to_stereo_float32 ((int16_t*)g_readbuffer, fbuffer, n);
                        }
                    }
                    else {
                        float *fbuffer = g_fbuffer + codecleft*2;
                        if (nchannels == 1) {
                            codec_lock ();
                            bytesread = decoder->read_float32 (g_readbuffer, nbytes*2);
                            codec_unlock ();
                            apply_replay_gain_float32 (&str_streaming_song, g_readbuffer, nbytes*2);
                            nsamples = bytesread / (samplesize * nchannels) + codecleft;
                            mono_float32_to_stereo_float32 ((float *)g_readbuffer, fbuffer, nsamples-codecleft);
                        }
                        else {
                            codec_lock ();
                            bytesread = decoder->read_float32 ((char *)fbuffer, nbytes*2);
                            codec_unlock ();
                            apply_replay_gain_float32 (&str_streaming_song, (char *)fbuffer, nbytes*2);
                            nsamples = bytesread / (samplesize * nchannels) + codecleft;
                        }
                    }
                }
                //codec_lock ();
                // convert samplerate
                srcdata.data_in = g_fbuffer;
                srcdata.data_out = g_srcbuffer;
                srcdata.input_frames = nsamples;
                srcdata.output_frames = size/4;
                srcdata.src_ratio = (double)p_get_rate ()/samplerate;
                srcdata.end_of_input = 0;
    //            src_set_ratio (src, srcdata.src_ratio);
                src_process (src, &srcdata);
                //codec_unlock ();
                // convert back to s16 format
                nbytes = size;
                int genbytes = srcdata.output_frames_gen * 4;
                bytesread = min(size, genbytes);
                float32_to_int16 ((float*)g_srcbuffer, (int16_t*)bytes, bytesread>>1);
                // calculate how many unused input samples left
                codecleft = nsamples - srcdata.input_frames_used;
                // copy spare samples for next update
                memmove (g_fbuffer, &g_fbuffer[srcdata.input_frames_used*2], codecleft * 8);
            }
            else {
                fprintf (stderr, "invalid ratio! %d / %d = %f", p_get_rate (), samplerate, (float)p_get_rate ()/samplerate);
            }
        }
        else {
            codec_unlock ();
        }
        bytes += bytesread;
        size -= bytesread;
        if (size == 0) {
            return initsize;
        }
        else  {
            // that means EOF
            if (bytes_until_next_song < 0) {
                trace ("finished streaming song, queueing next\n");
                bytes_until_next_song = streambuffer_fill;
                pl_nextsong (0);
            }
            break;
        }
    }
    return initsize - size;
}

void
streamer_lock (void) {
    mutex_lock (mutex);
}

void
streamer_unlock (void) {
    mutex_unlock (mutex);
}

int
streamer_read (char *bytes, int size) {
    streamer_lock ();
    int sz = min (size, streambuffer_fill);
    if (sz) {
        int cp = sz;
        int readpos = streambuffer_pos & STREAM_BUFFER_MASK;
        int part1 = STREAM_BUFFER_SIZE-readpos;
        part1 = min (part1, cp);
        if (part1 > 0) {
            memcpy (bytes, streambuffer+readpos, part1);
            streambuffer_pos += part1;
            streambuffer_fill -= part1;
            cp -= part1;
            bytes += part1;
        }
        if (cp > 0) {
            memcpy (bytes, streambuffer, cp);
            streambuffer_pos += cp;
            streambuffer_fill -= cp;
            bytes += cp;
        }
        streambuffer_pos &= STREAM_BUFFER_MASK;
        playpos += (float)sz/p_get_rate ()/4.f;
        str_playing_song.playtime += (float)sz/p_get_rate ()/4.f;
        if (playlist_current_ptr) {
            playlist_current_ptr->playtime = str_playing_song.playtime;
        }
        if (bytes_until_next_song > 0) {
            bytes_until_next_song -= sz;
            if (bytes_until_next_song < 0) {
                bytes_until_next_song = 0;
            }
//            trace ("buns: %d\n", bytes_until_next_song);
        }
    }
    streamer_unlock ();
    return sz;
}

int
streamer_get_fill (void) {
    return streambuffer_fill;
}

int
streamer_ok_to_read (int len) {
    if (bytes_until_next_song > 0) {
        return 1;
    }
    return streambuffer_fill >= (len*2);
}

int
streamer_is_buffering (void) {
    if (streambuffer_fill < 16384) {
        return 1;
    }
    else {
        return 0;
    }
}
