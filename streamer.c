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
#include "conf.h"
#include "plugins.h"
#include "optmath.h"
#include "volume.h"
#include "vfs.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static intptr_t streamer_tid;
static int src_quality;
static SRC_STATE *src;
static SRC_DATA srcdata;
static int codecleft;

static int conf_replaygain_mode = 0;
static int conf_replaygain_scale = 1;
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
//#define STREAM_BUFFER_SIZE 0x10000 // slightly more than 3 seconds of 44100 stereo
//#define STREAM_BUFFER_MASK 0xffff

static int streambuffer_fill;
static int streambuffer_pos;
static int bytes_until_next_song = 0;
static char streambuffer[STREAM_BUFFER_SIZE];
static uintptr_t mutex;
static uintptr_t decodemutex;
static int nextsong = -1;
static int nextsong_pstate = -1;
static int badsong = -1;

static float seekpos = -1;

static float playpos = 0; // play position of current song
static int avg_bitrate = -1; // avg bitrate of current song
static int last_bitrate = -1; // last bitrate of current song

static int prevtrack_samplerate = -1;

// copies of current playing and streaming tracks
playItem_t str_playing_song;
playItem_t str_streaming_song;
// remember pointers to original instances of playitems
static playItem_t *orig_playing_song;
static playItem_t *orig_streaming_song;
// current decoder
static DB_fileinfo_t *str_current_decoder;

static int streamer_buffering;

// to allow interruption of stall file requests
static DB_FILE *streamer_file;

playItem_t *
streamer_get_streaming_track (void) {
    return orig_streaming_song;
}

playItem_t *
streamer_get_playing_track (void) {
    return &str_playing_song;
}

// playlist must call that whenever item was removed
void
streamer_song_removed_notify (playItem_t *it) {
    if (!mutex) {
        return; // streamer is not running
    }
    plug_trigger_event (DB_EV_TRACKDELETED, (uintptr_t)it);
    if (it == orig_playing_song) {
        orig_playing_song = NULL;
    }
    if (it == orig_streaming_song) {
        orig_streaming_song = NULL;
        // queue new next song for streaming
        if (bytes_until_next_song > 0) {
            streambuffer_fill = bytes_until_next_song;
            streamer_move_nextsong (0);
        }
    }
}

// that must be called after last sample from str_playing_song was done reading
static int
streamer_set_current (playItem_t *it) {
    int from, to;
    streamer_buffering = 1;
    from = orig_playing_song ? pl_get_idx_of (orig_playing_song) : -1;
    to = it ? pl_get_idx_of (it) : -1;
    if (to != -1) {
        messagepump_push (M_TRACKCHANGED, 0, to, 0);
    }
#if 0
    // this breaks redraw
    if (!orig_playing_song || p_isstopped ()) {
        orig_playing_song = it;
    }
#endif
    trace ("streamer_set_current %p, buns=%d\n", it);
    if(str_current_decoder) {
        str_current_decoder->plugin->free (str_current_decoder);
        str_current_decoder = NULL;
        pl_item_free (&str_streaming_song);
    }
    orig_streaming_song = it;
    if (!it) {
        goto success;
    }
    DB_decoder_t *dec = NULL;
    if (!it->decoder_id && it->filetype && !strcmp (it->filetype, "content")) {
        // try to get content-type
        DB_FILE *fp = streamer_file = vfs_fopen (it->fname);
        const char *plug = NULL;
        if (fp) {
            const char *ct = vfs_get_content_type (fp);
            if (ct) {
                fprintf (stderr, "got content-type: %s\n", ct);
                if (!strcmp (ct, "audio/mpeg")) {
                    plug = "stdmpg";
                }
                else if (!strcmp (ct, "application/ogg")) {
                    plug = "stdogg";
                }
            }
            streamer_file = NULL;
            vfs_fclose (fp);
        }
        if (plug) {
            dec = plug_get_decoder_for_id (plug);
            if (dec) {
                it->decoder_id = plug_get_decoder_id (plug);
                it->filetype = dec->filetypes[0];
            }
        }
    }
    else if (it->decoder_id) {
        dec = plug_get_decoder_for_id (it->decoder_id);
    }
    if (dec) {
        streamer_lock ();
        streamer_unlock ();
        str_current_decoder = dec->init (DB_PLAYITEM (it));
        streamer_lock ();
        streamer_unlock ();
        pl_item_copy (&str_streaming_song, it);
        if (!str_current_decoder) {
            trace ("decoder->init returned NULL\n");
            trace ("orig_playing_song = %p\n", orig_playing_song);
            if (orig_playing_song == it) {
                orig_playing_song = NULL;
                messagepump_push (M_TRACKCHANGED, 0, to, 0);
            }
            return -1;
        }
        else {
            trace ("bps=%d, channels=%d, samplerate=%d\n", str_current_decoder->bps, str_current_decoder->channels, str_current_decoder->samplerate);
        }
        streamer_reset (0); // reset SRC
    }
    else {
        streamer_buffering = 0;
        if (to != -1) {
            messagepump_push (M_TRACKCHANGED, 0, to, 0);
        }
        trace ("no decoder in playitem!\n");
        orig_streaming_song = NULL;
        return -1;
    }
    if (bytes_until_next_song == -1) {
        bytes_until_next_song = 0;
    }
success:
    messagepump_push (M_TRACKCHANGED, 0, to, 0);
    return 0;
}

float
streamer_get_playpos (void) {
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
    trace ("streamer_set_nextsong %d %d\n", song, pstate);
    plug_trigger_event (DB_EV_ABORTREAD, 0);
    nextsong = song;
    nextsong_pstate = pstate;
    if (p_isstopped ()) {
        // no sense to wait until end of previous song, reset buffer
        bytes_until_next_song = 0;
        playpos = 0;
    }
}

void
streamer_set_seek (float pos) {
    seekpos = pos;
}

static int
streamer_read_async (char *bytes, int size);

void
streamer_thread (void *ctx) {
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
            playItem_t *try = pl_get_for_idx (sng);
            if (!try) { // track is not in playlist
                trace ("track #%d is not in playlist; stopping playback\n", sng);
                p_stop ();
                pl_item_free (&str_playing_song);
                pl_item_free (&str_streaming_song);
                orig_playing_song = NULL;
                orig_streaming_song = NULL;
                messagepump_push (M_SONGCHANGED, 0, -1, -1);
                continue;
            }
            int ret = streamer_set_current (try);
            if (ret < 0) {
                trace ("failed to play track %s, skipping...\n", try->fname);
                // remember bad song number in case of looping
                if (badsong == -1) {
                    badsong = sng;
                }
                // try jump to next song
                streamer_move_nextsong (0);
                trace ("streamer_move_nextsong switched to track %d\n", nextsong);
                usleep (50000);
                continue;
            }
            badsong = -1;
            if (pstate == 0) {
                p_stop ();
            }
            else if (pstate == 1) {
                last_bitrate = -1;
                avg_bitrate = -1;
                p_play ();
            }
            else if (pstate == 2) {
                p_pause ();
            }
        }
        else if (nextsong == -2 && (nextsong_pstate==0 || bytes_until_next_song == 0)) {
            int from = orig_playing_song ? pl_get_idx_of (orig_playing_song) : -1;
            bytes_until_next_song = -1;
            trace ("nextsong=-2\n");
            nextsong = -1;
            p_stop ();
            if (str_current_decoder) {
                trace ("sending songfinished to plugins [1]\n");
                plug_trigger_event (DB_EV_SONGFINISHED, 0);
            }
            streamer_set_current (NULL);
            pl_item_free (&str_playing_song);
            orig_playing_song = NULL;
            messagepump_push (M_SONGCHANGED, 0, from, -1);
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
                streamer_set_current (NULL);
                continue;
            }
            trace ("bytes_until_next_song=0, starting playback of new song\n");
            int from = orig_playing_song ? pl_get_idx_of (orig_playing_song) : -1;
            int to = orig_streaming_song ? pl_get_idx_of (orig_streaming_song) : -1;
            trace ("from=%d, to=%d\n", from, to);
            trace ("sending songchanged\n");
            messagepump_push (M_SONGCHANGED, 0, from, to);
            bytes_until_next_song = -1;
            // plugin will get pointer to str_playing_song
            if (str_current_decoder) {
                trace ("sending songfinished to plugins [2]\n");
                plug_trigger_event (DB_EV_SONGFINISHED, 0);
            }
            // free old copy of playing
            pl_item_free (&str_playing_song);
            // copy streaming into playing
            pl_item_copy (&str_playing_song, &str_streaming_song);
            last_bitrate = -1;
            avg_bitrate = -1;
            orig_playing_song = orig_streaming_song;
            if (orig_playing_song) {
                orig_playing_song->played = 1;
                orig_playing_song->started_timestamp = time (NULL);
                str_playing_song.started_timestamp = time (NULL);
            }
            // that is needed for playlist drawing
            // plugin will get pointer to new str_playing_song
            trace ("sending songstarted to plugins\n");
            plug_trigger_event (DB_EV_SONGSTARTED, 0);
            playpos = 0;
            // change samplerate
            if (prevtrack_samplerate != str_current_decoder->samplerate) {
                plug_get_output ()->change_rate (str_current_decoder->samplerate);
                prevtrack_samplerate = str_current_decoder->samplerate;
            }
        }

        if (seekpos >= 0) {
            trace ("seeking to %f\n", seekpos);
            float pos = seekpos;
            seekpos = -1;

            if (orig_playing_song != orig_streaming_song) {
                trace ("streamer already switched to next track\n");
                // restart playing from new position
                if(str_current_decoder) {
                    str_current_decoder->plugin->free (str_current_decoder);
                    str_current_decoder = NULL;
                    pl_item_free (&str_streaming_song);
                }
                orig_streaming_song = orig_playing_song;
                pl_item_copy (&str_streaming_song, orig_streaming_song);
                bytes_until_next_song = -1;
                streamer_buffering = 1;
                int trk = pl_get_idx_of (orig_streaming_song);
                if (trk != -1) {
                    messagepump_push (M_TRACKCHANGED, 0, trk, 0);
                }
                DB_decoder_t *plug = plug_get_decoder_for_id (orig_streaming_song->decoder_id);
                if (plug) {
                    str_current_decoder = plug->init (DB_PLAYITEM (orig_streaming_song));
                }
                if (!plug || !str_current_decoder) {
                    streamer_buffering = 0;
                    if (trk != -1) {
                        messagepump_push (M_TRACKCHANGED, 0, trk, 0);
                    }
                    trace ("failed to restart prev track on seek, trying to jump to next track\n");
                    streamer_move_nextsong (0);
                    trace ("streamer_move_nextsong switched to track %d\n", nextsong);
                    usleep (50000);
                    continue;
                }
            }

            streamer_buffering = 1;
            int trk = pl_get_idx_of (orig_streaming_song);
            if (trk != -1) {
                messagepump_push (M_TRACKCHANGED, 0, trk, 0);
            }
            if (str_current_decoder && str_playing_song._duration > 0) {
                streamer_lock ();
                streambuffer_fill = 0;
                streambuffer_pos = 0;
                codec_lock ();
                codecleft = 0;
                codec_unlock ();
                if (str_current_decoder->plugin->seek (str_current_decoder, pos) >= 0) {
                    playpos = str_current_decoder->readpos;
                }
                last_bitrate = -1;
                avg_bitrate = -1;
                streamer_unlock();
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
            char buf[sz];
            streamer_unlock ();
            int bytesread = streamer_read_async (buf,sz);
            streamer_lock ();
            memcpy (streambuffer+streambuffer_fill, buf, sz);
            streambuffer_fill += bytesread;
        }
        streamer_unlock ();
        struct timeval tm2;
        gettimeofday (&tm2, NULL);

        int ms = (tm2.tv_sec*1000+tm2.tv_usec/1000) - (tm1.tv_sec*1000+tm1.tv_usec/1000);
        alloc_time -= ms;
        if (alloc_time > 0) {
            usleep (alloc_time * 1000);
//            usleep (1000);
        }
//        trace ("fill: %d/%d\n", streambuffer_fill, STREAM_BUFFER_SIZE);
    }

    // stop streaming song
    if(str_current_decoder) {
        str_current_decoder->plugin->free (str_current_decoder);
        str_current_decoder = NULL;
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
    decodemutex = mutex_create ();
    src_quality = conf_get_int ("src_quality", 2);
    src = src_new (src_quality, 2, NULL);
    conf_replaygain_mode = conf_get_int ("replaygain_mode", 0);
    conf_replaygain_scale = conf_get_int ("replaygain_scale", 1);
    if (!src) {
        return -1;
    }
    streamer_tid = thread_start (streamer_thread, NULL);
    return 0;
}

void
streamer_free (void) {
    streaming_terminate = 1;
    thread_join (streamer_tid);
    mutex_free (decodemutex);
    decodemutex = 0;
    mutex_free (mutex);
    mutex = 0;
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
        if (!str_current_decoder) {
            codec_unlock ();
            break;
        }
        DB_decoder_t *decoder = str_current_decoder->plugin;
        if (!decoder) {
            // means there's nothing left to stream, so just do nothing
            codec_unlock ();
            break;
        }
        if (str_current_decoder->samplerate != -1) {
            int nchannels = str_current_decoder->channels;
            int samplerate = str_current_decoder->samplerate;
            if (str_current_decoder->samplerate == p_get_rate ()) {
                // samplerate match
                if (str_current_decoder->channels == 2) {
                    bytesread = decoder->read_int16 (str_current_decoder, bytes, size);
                    apply_replay_gain_int16 (&str_streaming_song, bytes, size);
                    codec_unlock ();
                }
                else {
                    bytesread = decoder->read_int16 (str_current_decoder, g_readbuffer, size>>1);
                    apply_replay_gain_int16 (&str_streaming_song, g_readbuffer, size>>1);
                    mono_int16_to_stereo_int16 ((int16_t*)g_readbuffer, (int16_t*)bytes, size>>2);
                    bytesread *= 2;
                    codec_unlock ();
                }
            }
            else if (src_is_valid_ratio (p_get_rate ()/(double)samplerate)) {
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
                        bytesread = decoder->read_int16 (str_current_decoder, g_readbuffer, nbytes);
                        apply_replay_gain_int16 (&str_streaming_song, g_readbuffer, nbytes);
                    }
                    else {
                        samplesize = 4;
                    }
                }
                codec_unlock ();
                // recalculate nsamples according to how many bytes we've got
                if (nbytes != 0) {
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
                            bytesread = decoder->read_float32 (str_current_decoder, g_readbuffer, nbytes*2);
                            codec_unlock ();
                            apply_replay_gain_float32 (&str_streaming_song, g_readbuffer, nbytes*2);
                            nsamples = bytesread / (samplesize * nchannels) + codecleft;
                            mono_float32_to_stereo_float32 ((float *)g_readbuffer, fbuffer, nsamples-codecleft);
                        }
                        else {
                            codec_lock ();
                            bytesread = decoder->read_float32 (str_current_decoder, (char *)fbuffer, nbytes*2);
                            codec_unlock ();
                            apply_replay_gain_float32 (&str_streaming_song, (char *)fbuffer, nbytes*2);
                            nsamples = bytesread / (samplesize * nchannels) + codecleft;
                        }
                    }
                }
                //codec_lock ();
                // convert samplerate
                mutex_lock (decodemutex);
                srcdata.data_in = g_fbuffer;
                srcdata.data_out = g_srcbuffer;
                srcdata.input_frames = nsamples;
                srcdata.output_frames = size/4;
                srcdata.src_ratio = p_get_rate ()/(double)samplerate;
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
                mutex_unlock (decodemutex);
                // copy spare samples for next update
                memmove (g_fbuffer, &g_fbuffer[srcdata.input_frames_used*2], codecleft * 8);
            }
            else {
                fprintf (stderr, "invalid ratio! %d / %d = %f", p_get_rate (), samplerate, p_get_rate ()/(float)samplerate);
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
                if (conf_get_int ("playlist.stop_after_current", 0)) {
                    conf_set_int ("playlist.stop_after_current", 0);
                    plug_trigger_event (DB_EV_CONFIGCHANGED, 0);
                    streamer_set_nextsong (-2, 1);
                }
                else {
                    streamer_move_nextsong (0);
                }
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
#if 0
    struct timeval tm1;
    gettimeofday (&tm1, NULL);
#endif
    streamer_lock ();
    int sz = min (size, streambuffer_fill);
    if (sz) {
        memcpy (bytes, streambuffer, sz);
        memmove (streambuffer, streambuffer+sz, streambuffer_fill-sz);
        streambuffer_fill -= sz;
        playpos += (float)sz/p_get_rate ()/4.f;
        str_playing_song.playtime += (float)sz/p_get_rate ()/4.f;
        if (orig_playing_song) {
            str_playing_song.filetype = orig_playing_song->filetype;
            orig_playing_song->playtime = str_playing_song.playtime;
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
    return sz;
}

int
streamer_get_fill (void) {
    return streambuffer_fill;
}

int
streamer_ok_to_read (int len) {
    if (len >= 0 && (bytes_until_next_song > 0 || streambuffer_fill >= (len*2))) {
        if (streamer_buffering) {
            streamer_buffering = 0;
            if (orig_streaming_song) {
                int trk = pl_get_idx_of (orig_streaming_song);
                if (trk != -1) {
                    messagepump_push (M_TRACKCHANGED, 0, trk, 0);
                }
            }
        }
        return 1;
    }
    else {
        return 1-streamer_buffering;
    }
    return 0;
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

void
streamer_configchanged (void) {
    conf_replaygain_mode = conf_get_int ("replaygain_mode", 0);
    conf_replaygain_scale = conf_get_int ("replaygain_scale", 1);
    int q = conf_get_int ("src_quality", 2);
    if (q != src_quality && q >= SRC_SINC_BEST_QUALITY && q <= SRC_LINEAR) {
        mutex_lock (decodemutex);
        fprintf (stderr, "changing src_quality from %d to %d\n", src_quality, q);
        src_quality = q;
        if (src) {
            src_delete (src);
            src = NULL;
        }
        memset (&srcdata, 0, sizeof (srcdata));
        src = src_new (src_quality, 2, NULL);
        mutex_unlock (decodemutex);
    }
}

void
streamer_play_current_track (void) {
    playlist_t *plt = plt_get_curr_ptr ();
    if (p_ispaused ()) {
        // unpause currently paused track
        p_unpause ();
        plug_trigger_event_paused (0);
    }
    else if (plt->current_row[PL_MAIN] != -1) {
        // play currently selected track
        p_stop ();
        // get next song in queue
        int idx = -1;
        playItem_t *next = pl_playqueue_getnext ();
        if (next) {
            idx = pl_get_idx_of (next);
            pl_playqueue_pop ();
        }
        else {
            idx = plt->current_row[PL_MAIN];
        }

        streamer_set_nextsong (idx, 1);
    }
    else {
        // restart currently playing track
        p_stop ();
        streamer_set_nextsong (0, 1);
    }
}

int
streamer_move_prevsong (void) {
    playlist_t *plt = plt_get_curr_ptr ();
    pl_playqueue_clear ();
    if (!plt->head[PL_MAIN]) {
        streamer_set_nextsong (-2, 1);
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE) { // shuffle
        if (!orig_playing_song) {
            return streamer_move_nextsong (1);
        }
        else {
            orig_playing_song->played = 0;
            // find already played song with maximum shuffle rating below prev song
            int rating = orig_playing_song->shufflerating;
            playItem_t *pmax = NULL; // played maximum
            playItem_t *amax = NULL; // absolute maximum
            for (playItem_t *i = plt->head[PL_MAIN]; i; i = i->next[PL_MAIN]) {
                if (i != orig_playing_song && i->played && (!amax || i->shufflerating > amax->shufflerating)) {
                    amax = i;
                }
                if (i == orig_playing_song || i->shufflerating > rating || !i->played) {
                    continue;
                }
                if (!pmax || i->shufflerating > pmax->shufflerating) {
                    pmax = i;
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
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (orig_playing_song) {
            it = orig_playing_song->prev[PL_MAIN];
        }
        if (!it) {
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = plt->tail[PL_MAIN];
            }
        }
        if (!it) {
            return -1;
        }
        int r = pl_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        streamer_move_randomsong ();
    }
    return -1;
}

int
streamer_move_nextsong (int reason) {
    playlist_t *plt = plt_get_curr_ptr ();
    playItem_t *it = pl_playqueue_getnext ();
    if (it) {
        pl_playqueue_pop ();
        int r = pl_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        return 0;
    }

    playItem_t *curr = streamer_get_streaming_track ();
    if (!plt->head[PL_MAIN]) {
        streamer_set_nextsong (-2, 1);
        return 0;
    }
    int pl_order = conf_get_int ("playback.order", 0);
    int pl_loop_mode = conf_get_int ("playback.loop", 0);
    if (pl_order == PLAYBACK_ORDER_SHUFFLE) { // shuffle
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
                    pl_reshuffle (&it, NULL);
                }
            }
            if (!it) {
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
        else {
            trace ("pl_next_song: reason=%d, loop=%d\n", reason, pl_loop_mode);
            if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // song finished, loop mode is "loop 1 track"
                int r = pl_get_idx_of (curr);
                streamer_set_nextsong (r, 1);
                return 0;
            }
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
                trace ("all songs played! reshuffle\n");
                // all songs played, reshuffle and try again
                if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) { // loop
                    pl_reshuffle (&it, NULL);
                }
            }
            if (!it) {
                return -1;
            }
            int r = pl_get_idx_of (it);
            streamer_set_nextsong (r, 1);
            return 0;
        }
    }
    else if (pl_order == PLAYBACK_ORDER_LINEAR) { // linear
        playItem_t *it = NULL;
        if (curr) {
            if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE) { // loop same track
                int r = pl_get_idx_of (curr);
                streamer_set_nextsong (r, 1);
                return 0;
            }
            it = curr->next[PL_MAIN];
        }
        if (!it) {
            trace ("streamer_move_nextsong: was last track\n");
            if (pl_loop_mode == PLAYBACK_MODE_LOOP_ALL) {
                it = plt->head[PL_MAIN];
            }
            else {
                streamer_set_nextsong (-2, 1);
                return 0;
            }
        }
        if (!it) {
            return -1;
        }
        int r = pl_get_idx_of (it);
        streamer_set_nextsong (r, 1);
        return 0;
    }
    else if (pl_order == PLAYBACK_ORDER_RANDOM) { // random
        if (reason == 0 && pl_loop_mode == PLAYBACK_MODE_LOOP_SINGLE && curr) {
            int r = pl_get_idx_of (curr);
            streamer_set_nextsong (r, 1);
            return 0;
        }
        return streamer_move_randomsong ();
    }
    return -1;
}

int
streamer_move_randomsong (void) {
    int cnt = pl_getcount (PL_MAIN);
    if (!cnt) {
        return -1;
    }
    int r = rand () / (float)RAND_MAX * cnt;
    streamer_set_nextsong (r, 1);
    return 0;
}

playItem_t *
streamer_get_current (void) {
    return orig_playing_song;
}

struct DB_fileinfo_s *
streamer_get_current_decoder (void) {
    return str_current_decoder;
}
