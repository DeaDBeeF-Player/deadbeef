#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <samplerate.h>
#include "codec.h"
#include "playlist.h"
#include "common.h"

extern int sdl_player_freq; // hack!
static SRC_STATE *src;
static SRC_DATA srcdata;
static int codecleft;
static char g_readbuffer[200000]; // hack!
static float g_fbuffer[200000]; // hack!
static float g_srcbuffer[200000]; // hack!

int
streamer_init (void) {
//    src = src_new (SRC_LINEAR, 2, NULL);
    src = src_new (SRC_SINC_BEST_QUALITY, 2, NULL);
    if (!src) {
        return -1;
    }
    codecleft = 0;
    return 0;
}

void
streamer_free (void) {
    if (src) {
        src_delete (src);
        src = NULL;
    }
}

void
streamer_reset (void) { // must be called when current song changes by external reasons
    codecleft = 0;
    src_reset (src);
}

// returns number of bytes been read
int
streamer_read (char *bytes, int size) {
    int initsize = size;
    for (;;) {
        int bytesread = 0;
        codec_lock ();
        codec_t *codec = playlist_current.codec;
        if (!codec) {
            codec_unlock ();
            break;
        }
        int nchannels = codec->info.channels;
        int samplerate = codec->info.samplesPerSecond;
        // read and do SRC
        if (codec->info.samplesPerSecond == sdl_player_freq) {
            int i;
            if (codec->info.channels == 2) {
                bytesread = codec->read (bytes, size);
                codec_unlock ();
            }
            else {
                bytesread = codec->read (g_readbuffer, size/2);
                codec_unlock ();
                for (i = 0; i < size/4; i++) {
                    int16_t sample = (int16_t)(((int32_t)(((int16_t*)g_readbuffer)[i])));
                    ((int16_t*)bytes)[i*2+0] = sample;
                    ((int16_t*)bytes)[i*2+1] = sample;
                }
                bytesread *= 2;
            }
        }
        else {
            int nsamples = size/4;
            // convert to codec samplerate
            nsamples = nsamples * samplerate / sdl_player_freq * 2 ;
            // read data at source samplerate (with some room for SRC)
            int nbytes = (nsamples - codecleft) * 2 * nchannels;
            bytesread = codec->read (g_readbuffer, nbytes);
            codec_unlock ();
            // recalculate nsamples according to how many bytes we've got
            nsamples = bytesread / (2 * nchannels) + codecleft;
            // convert to float
            int i;
            float *fbuffer = g_fbuffer + codecleft*2;
            if (nchannels == 2) {
                for (i = 0; i < (nsamples - codecleft) * 2; i++) {
                    fbuffer[i] = ((int16_t *)g_readbuffer)[i]/32767.f;
                }
            }
            else if (nchannels == 1) { // convert mono to stereo
                for (i = 0; i < (nsamples - codecleft); i++) {
                    fbuffer[i*2+0] = ((int16_t *)g_readbuffer)[i]/32767.f;
                    fbuffer[i*2+1] = fbuffer[i*2+0];
                }
            }
            // convert samplerate
            srcdata.data_in = g_fbuffer;
            srcdata.data_out = g_srcbuffer;
            srcdata.input_frames = nsamples;
            srcdata.output_frames = size/4;
            srcdata.src_ratio = (double)sdl_player_freq/samplerate;
            srcdata.end_of_input = 0;
//            src_set_ratio (src, srcdata.src_ratio);
            src_process (src, &srcdata);
            //printf ("processed %d/%d samples (input=%d)\n", srcdata.output_frames_gen, srcdata.output_frames, srcdata.input_frames);
            // convert back to s16 format
            nbytes = size;
            int genbytes = srcdata.output_frames_gen * 4;
            bytesread = min(size, genbytes);
            for (i = 0; i < bytesread/2; i++) {
                ((int16_t*)bytes)[i] = (int16_t)(g_srcbuffer[i]*32767.f);
            }
            // calculate how many unused input samples left
            codecleft = nsamples - srcdata.input_frames_used;
            // copy spare samples for next update
            memmove (g_fbuffer, &g_fbuffer[srcdata.input_frames_used*2], codecleft * 8);
        }
        bytes += bytesread;
        size -= bytesread;
        if (size == 0) {
            return initsize;
        }
        else {
            //printf ("eof (size=%d)\n", size);
            // that means EOF
            if (ps_nextsong () < 0) {
                break;
            }
        }
    }
    return initsize - size;
}
