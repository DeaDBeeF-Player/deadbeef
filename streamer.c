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
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/time.h>
#include "threading.h"
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

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static intptr_t streamer_tid;
static int src_quality;
static SRC_STATE *src;
static SRC_DATA srcdata;
static int src_remaining; // number of input samples in SRC buffer

static int conf_replaygain_mode = 0;
static int conf_replaygain_scale = 1;

#define SRC_BUFFER 16000
static int src_in_remaining = 0;
//static int16_t g_src_in_buffer[SRC_BUFFER*2];
static float g_src_in_fbuffer[SRC_BUFFER*2];
static float g_src_out_fbuffer[SRC_BUFFER*2];

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

playItem_t str_playing_song;
playItem_t str_streaming_song;
// remember pointers to original instances of playitems
static playItem_t *orig_playing_song;
static playItem_t *orig_streaming_song;

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
            pl_nextsong (0);
        }
    }
}

// that must be called after last sample from str_playing_song was done reading
static int
streamer_set_current (playItem_t *it) {
    int from, to;
    from = orig_playing_song ? pl_get_idx_of (orig_playing_song) : -1;
    to = it ? pl_get_idx_of (it) : -1;
    if (!orig_playing_song || p_isstopped ()) {
        streamer_buffering = 1;
        playlist_current_ptr = it;
        //trace ("from=%d, to=%d\n", from, to);
        //messagepump_push (M_SONGCHANGED, 0, from, to);
    }
    trace ("streamer_set_current %p, buns=%d\n", it);
    mutex_lock (decodemutex);
    if(str_streaming_song.decoder) {
        str_streaming_song.decoder->free ();
        pl_item_free (&str_streaming_song);
    }
    mutex_unlock (decodemutex);
    orig_streaming_song = it;
    if (!it) {
        goto success;
    }
    if (to != -1) {
        messagepump_push (M_TRACKCHANGED, 0, to, 0);
    }
    if (from != -1) {
        messagepump_push (M_TRACKCHANGED, 0, from, 0);
    }
    if (!it->decoder && it->filetype && !strcmp (it->filetype, "content")) {
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
            DB_decoder_t **decoders = plug_get_decoder_list ();
            // match by decoder
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->id, plug)) {
                    it->decoder = decoders[i];
                    it->filetype = decoders[i]->filetypes[0];
                }
            }
        }
    }
    if (it->decoder) {
        mutex_lock (decodemutex);
        int ret = it->decoder->init (DB_PLAYITEM (it));
        pl_item_copy (&str_streaming_song, it);
        mutex_unlock (decodemutex);
        if (ret < 0) {
            it->played = 1;
            trace ("decoder->init returned %d\n", ret);
            trace ("orig_playing_song = %p\n", orig_playing_song);
            streamer_buffering = 0;
            if (playlist_current_ptr == it) {
//                playlist_current_ptr = NULL;
                messagepump_push (M_TRACKCHANGED, 0, to, 0);
            }
            return ret;
        }
        else {
            trace ("bps=%d, channels=%d, samplerate=%d\n", it->decoder->info.bps, it->decoder->info.channels, it->decoder->info.samplerate);
        }
        streamer_reset (0); // reset SRC
    }
    else {
        trace ("no decoder in playitem!\n");
        it->played = 1;
        streamer_buffering = 0;
        if (playlist_current_ptr == it) {
            playlist_current_ptr = NULL;
            messagepump_push (M_TRACKCHANGED, 0, to, 0);
        }
        //orig_streaming_song = NULL;
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
        // try to interrupt file operation
        streamer_lock ();
        streamer_unlock ();
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
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-stream", 0, 0, 0, 0);
#endif
    src_remaining = 0;

    while (!streaming_terminate) {
        struct timeval tm1;
        gettimeofday (&tm1, NULL);
        if (nextsong >= 0) { // start streaming next song
            trace ("nextsong=%d\n", nextsong);
            int sng = nextsong;
            int pstate = nextsong_pstate;
            nextsong = -1;
            src_remaining = 0;
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

                mutex_lock (decodemutex);
                pl_item_free (&str_streaming_song);
                mutex_unlock (decodemutex);

                orig_playing_song = NULL;
                orig_streaming_song = NULL;
                messagepump_push (M_SONGCHANGED, 0, -1, -1);
                continue;
            }
            int ret = streamer_set_current (try);
            if (ret < 0) {
                trace ("\033[0;31mfailed to play track %s, skipping (current=%p)...\033[37;0m\n", try->fname, orig_streaming_song);
                // remember bad song number in case of looping
                if (badsong == -1) {
                    badsong = sng;
                }
                // try jump to next song
                pl_nextsong (0);
                trace ("pl_nextsong switched to track %d\n", nextsong);
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
                if (p_state () != OUTPUT_STATE_PLAYING) {
                    if (p_play () < 0) {
                        fprintf (stderr, "streamer: failed to start playback; output plugin doesn't work\n");
                        streamer_set_nextsong (-2, 0);
                    }
                }
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
            if (str_playing_song.decoder) {
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
            if (str_playing_song.decoder) {
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
            playlist_current_ptr = orig_playing_song;
            // that is needed for playlist drawing
            // plugin will get pointer to new str_playing_song
            trace ("sending songstarted to plugins\ncurrent playtrack: %s\n", str_playing_song.fname);
            plug_trigger_event (DB_EV_SONGSTARTED, 0);
            playpos = 0;

            // try to switch samplerate to the closest supported by output plugin
            if (conf_get_int ("playback.dynsamplerate", 0)) {

                // don't switch if unchanged
                int prevtrack_samplerate = p_get_rate ();
                if (prevtrack_samplerate != str_playing_song.decoder->info.samplerate) {
                    int newrate = plug_get_output ()->change_rate (str_playing_song.decoder->info.samplerate);
                    if (newrate != prevtrack_samplerate) {
                        // restart streaming of current track
                        trace ("streamer: output samplerate changed from %d to %d; restarting track\n", prevtrack_samplerate, newrate);
                        mutex_lock (decodemutex);
                        str_streaming_song.decoder->free ();
                        str_streaming_song.decoder->init (DB_PLAYITEM (orig_streaming_song));
                        mutex_unlock (decodemutex);
                        bytes_until_next_song = -1;
                        streamer_buffering = 1;
                        streamer_reset (1);
                        prevtrack_samplerate = str_playing_song.decoder->info.samplerate;
                    }
                }

                // output plugin may stop playback before switching samplerate
                if (p_state () != OUTPUT_STATE_PLAYING) {
                    if (p_play () < 0) {
                        fprintf (stderr, "streamer: failed to start playback after samplerate change; output plugin doesn't work\n");
                        streamer_set_nextsong (-2, 0);
                        continue;
                    }
                }
            }
        }

        if (seekpos >= 0) {
            trace ("seeking to %f\n", seekpos);
            float pos = seekpos;
            seekpos = -1;

            if (orig_playing_song != orig_streaming_song) {
                trace ("streamer already switched to next track\n");
                
                // restart playing from new position
                
                mutex_lock (decodemutex);
                if(str_streaming_song.decoder) {
                    str_streaming_song.decoder->free ();
                    pl_item_free (&str_streaming_song);
                }
                orig_streaming_song = orig_playing_song;
                pl_item_copy (&str_streaming_song, orig_streaming_song);
                mutex_unlock (decodemutex);

                bytes_until_next_song = -1;
                streamer_buffering = 1;
                int trk = pl_get_idx_of (orig_streaming_song);
                if (trk != -1) {
                    messagepump_push (M_TRACKCHANGED, 0, trk, 0);
                }

                mutex_lock (decodemutex);
                int ret = str_streaming_song.decoder->init (DB_PLAYITEM (orig_streaming_song));
                mutex_unlock (decodemutex);

                if (ret < 0) {
                    if (trk != -1) {
                        messagepump_push (M_TRACKCHANGED, 0, trk, 0);
                    }
                    trace ("failed to restart prev track on seek, trying to jump to next track\n");
                    pl_nextsong (0);
                    trace ("pl_nextsong switched to track %d\n", nextsong);
                    usleep (50000);
                    continue;
                }
            }

            streamer_buffering = 1;
            int trk = pl_get_idx_of (orig_streaming_song);
            if (trk != -1) {
                messagepump_push (M_TRACKCHANGED, 0, trk, 0);
            }
            if (str_playing_song.decoder && str_playing_song._duration > 0) {
                streamer_lock ();
                streambuffer_fill = 0;
                streambuffer_pos = 0;
                src_remaining = 0;
                if (str_playing_song.decoder->seek (pos) >= 0) {
                    playpos = str_playing_song.decoder->info.readpos;
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
        if (streambuffer_fill > 128000 && streamer_buffering || !orig_streaming_song) {
            streamer_buffering = 0;
            if (orig_streaming_song) {
                int trk = pl_get_idx_of (orig_streaming_song);
                if (trk != -1) {
                    messagepump_push (M_TRACKCHANGED, 0, trk, 0);
                }
            }
        }
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
    mutex_lock (decodemutex);
    if(str_streaming_song.decoder) {
        str_streaming_song.decoder->free ();
    }
    pl_item_free (&str_streaming_song);
    mutex_unlock (decodemutex);

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
    src_remaining = 0;
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

/*

   src algorithm

   initsize = size;
   while (size > 0) {
        need_frames = SRC_BUFFER - src_remaining
        read_samples (need_frames)
        src_process (output_frames)
        convert_to_int16 (bytes, size)
        // handle errors
    }
    return initsize-size;

*/

static int
streamer_read_data_for_src (int16_t *buffer, int frames) {
    DB_decoder_t *decoder = str_streaming_song.decoder;
    int channels = decoder->info.channels;
    if (channels > 2) {
        channels = 2;
    }
    int bytesread = decoder->read_int16 ((uint8_t*)buffer, frames * sizeof (int16_t) * channels);
    apply_replay_gain_int16 (&str_streaming_song, (uint8_t*)buffer, bytesread);
    if (channels == 1) {
        // convert to stereo
        int n = frames-1;
        while (n >= 0) {
            buffer[n*2+0] = buffer[n];
            buffer[n*2+1] = buffer[n];
        }
    }
    return bytesread / (sizeof (int16_t) * channels);
}

static int
streamer_read_data_for_src_float (float *buffer, int frames) {
    DB_decoder_t *decoder = str_streaming_song.decoder;
    int channels = decoder->info.channels;
    if (channels > 2) {
        channels = 2;
    }
    if (decoder->read_float32) {
        int bytesread = decoder->read_float32 ((uint8_t*)buffer, frames * sizeof (float) * channels);
        apply_replay_gain_float32 (&str_streaming_song, (uint8_t*)buffer, bytesread);
        if (channels == 1) {
            // convert to stereo
            int n = frames-1;
            while (n >= 0) {
                buffer[n*2+0] = buffer[n];
                buffer[n*2+1] = buffer[n];
            }
            bytesread *= 2;
        }
        return bytesread / (sizeof (float) * channels);
    }

//    trace ("read_float32 not impl\n");
    int16_t intbuffer[frames*2];
    int res = streamer_read_data_for_src (intbuffer, frames);
    for (int i = 0; i < res; i++) {
        buffer[i*2+0] = intbuffer[i*2+0]/(float)0x7fff;
        buffer[i*2+1] = intbuffer[i*2+1]/(float)0x7fff;
    }
    return res;
}

static int
streamer_decode_src_libsamplerate (uint8_t *bytes, int size) {
    int initsize = size;
    int16_t *out = (int16_t *)bytes;
    DB_decoder_t *decoder = str_streaming_song.decoder;
    int samplerate = decoder->info.samplerate;
    float ratio = (float)p_get_rate ()/samplerate;
    while (size > 0) {
        int n_output_frames = size / sizeof (int16_t) / 2;
        int n_input_frames = n_output_frames * samplerate / p_get_rate () + 100;
        // read data from decoder
        if (n_input_frames > SRC_BUFFER - src_remaining ) {
            n_input_frames = SRC_BUFFER - src_remaining;
        }
        int nread = streamer_read_data_for_src_float (&g_src_in_fbuffer[src_remaining*2], n_input_frames);
        src_remaining += nread;
        // resample

        srcdata.data_in = g_src_in_fbuffer;
        srcdata.data_out = g_src_out_fbuffer;
        srcdata.input_frames = src_remaining;
        srcdata.output_frames = size/4;
        srcdata.src_ratio = ratio;
//        trace ("SRC from %d to %d\n", samplerate, p_get_rate ());
        srcdata.end_of_input = (nread == n_input_frames) ? 0 : 1;
        src_process (src, &srcdata);
        // convert back to s16 format
//        trace ("out frames: %d\n", srcdata.output_frames_gen);
        int genbytes = srcdata.output_frames_gen * 4;
        int bytesread = min(size, genbytes);
//        trace ("bytesread: %d\n", bytesread);
        float32_to_int16 ((float*)g_src_out_fbuffer, (int16_t*)bytes, bytesread>>1);
        size -= bytesread;
//        trace ("size: %d\n", size);
        bytes += bytesread;
        // calculate how many unused input samples left
        src_remaining -= srcdata.input_frames_used;

        // copy spare samples for next update
        if (src_remaining > 0) {
            memmove (g_src_in_fbuffer, &g_src_in_fbuffer[srcdata.input_frames_used*2], src_remaining * sizeof (float) * 2);
        }

        if (nread != n_input_frames) {
            trace ("nread=%d, n_input_frames=%d, mismatch!\n", nread, n_input_frames);
            break;
        }
    }
    return initsize-size;
}

#if 0
static int
streamer_decode_src (uint8_t *bytes, int size) {
    int initsize = size;
    int16_t *out = (int16_t *)bytes;
    DB_decoder_t *decoder = str_streaming_song.decoder;
    int samplerate = decoder->info.samplerate;
    float ratio = (float)samplerate / p_get_rate ();
    while (size > 0) {
        int n_output_frames = size / sizeof (int16_t) / 2;
        int n_input_frames = n_output_frames * samplerate / p_get_rate () + 100;
        // read data from decoder
        if (n_input_frames > SRC_BUFFER - src_remaining ) {
            n_input_frames = SRC_BUFFER - src_remaining;
        }
        int nread = streamer_read_data_for_src (&g_src_in_buffer[src_remaining*2], n_input_frames);
        src_remaining += nread;
        // resample
        float idx = 0;
        int i = 0;
        while (i < n_output_frames && idx < src_remaining) {
            int k = (int)idx;
            out[0] = g_src_in_buffer[k*2+0];
            out[1] = g_src_in_buffer[k*2+1];
            i++;
            idx += ratio;
            out += 2;
        }
        size -= i*4;
        src_remaining -= idx;
        if (src_remaining < 0) {
            src_remaining = 0;
        }
        else if (src_remaining > 0) {
            memmove (g_src_in_buffer, &g_src_in_buffer[(SRC_BUFFER-src_remaining)*2], src_remaining*2*sizeof (int16_t));
        }
        if (nread != n_input_frames) {
            break;
        }
    }
    return initsize-size;
}
#endif

// returns number of bytes been read
static int
streamer_read_async (char *bytes, int size) {
    int initsize = size;
    for (;;) {
        int bytesread = 0;
        mutex_lock (decodemutex);
        DB_decoder_t *decoder = str_streaming_song.decoder;
        if (!decoder) {
            // means there's nothing left to stream, so just do nothing
            mutex_unlock (decodemutex);
            break;
        }
        if (decoder->info.samplerate != -1) {
            int nchannels = decoder->info.channels;
            if (nchannels > 2) {
                nchannels = 2;
            }
            int samplerate = decoder->info.samplerate;
            if (decoder->info.samplerate == p_get_rate ()) {
                // samplerate match
                if (nchannels == 2) {
                    bytesread = decoder->read_int16 (bytes, size);
                    apply_replay_gain_int16 (&str_streaming_song, bytes, bytesread);
                }
                else {
                    uint8_t buffer[size>>1];
                    bytesread = decoder->read_int16 (buffer, size>>1);
                    apply_replay_gain_int16 (&str_streaming_song, buffer, bytesread);
                    mono_int16_to_stereo_int16 ((int16_t*)buffer, (int16_t*)bytes, size>>2);
                    bytesread *= 2;
                }
            }
            else if (src_is_valid_ratio (p_get_rate ()/(double)samplerate)) {
                bytesread = streamer_decode_src_libsamplerate (bytes, size);
            }
            else {
                fprintf (stderr, "invalid ratio! %d / %d = %f", p_get_rate (), samplerate, p_get_rate ()/(float)samplerate);
            }
        }
        mutex_unlock (decodemutex);
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
                    pl_nextsong (0);
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
        if (playlist_current_ptr) {
            str_playing_song.filetype = playlist_current_ptr->filetype;
        }
        if (playlist_current_ptr) {
            playlist_current_ptr->playtime = str_playing_song.playtime;
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
    if (p_ispaused () && orig_playing_song) {
        // unpause currently paused track
        p_unpause ();
        plug_trigger_event_paused (0);
    }
    else if (playlist_current_row[PL_MAIN] != -1) {
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
            idx = playlist_current_row[PL_MAIN];
        }

        streamer_set_nextsong (idx, 1);
    }
    else {
        // restart currently playing track
        p_stop ();
        streamer_set_nextsong (0, 1);
    }
}

