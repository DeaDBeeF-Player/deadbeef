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
static char g_readbuffer[20000]; // hack!
static float g_fbuffer[20000]; // hack!
static float g_srcbuffer[20000]; // hack!

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

// returns number of bytes been read
int
streamer_read (char *bytes, int size) {
    codec_t *codec = playlist_current.codec;
    int bytesread = 0;
    if (!codec) {
        return 0;
    }
    // read and do SRC
    if (codec->info.samplesPerSecond == sdl_player_freq) {
        int i;
        if (codec->info.channels == 2) {
            codec_lock ();
            bytesread = codec->read (bytes, size);
            codec_unlock ();
        }
        else {
            codec_lock ();
            bytesread = codec->read (g_readbuffer, size/2);
            for (i = 0; i < size/4; i++) {
                int16_t sample = (int16_t)(((int32_t)(((int16_t*)g_readbuffer)[i])));
                ((int16_t*)bytes)[i*2+0] = sample;
                ((int16_t*)bytes)[i*2+1] = sample;
            }
            bytesread *= 2;
            codec_unlock ();
        }
    }
    else {
        int nsamples = size/4;
        // convert to codec samplerate
        // add 5 extra samples for SRC
        nsamples = nsamples * codec->info.samplesPerSecond / sdl_player_freq + 5;
        // read data at source samplerate (with some room for SRC)
        codec_lock ();
        int nbytes = (nsamples - codecleft) * 2 * codec->info.channels;
        bytesread = codec->read (g_readbuffer, nbytes);
        // recalculate nsamples according to how many bytes we've got
        nsamples -= (nbytes - bytesread) / (2 * codec->info.channels);
        // convert to float, and apply soft volume
        int i;
        float *fbuffer = g_fbuffer + codecleft*2;
        if (codec->info.channels == 2) { // convert mono to stereo
            for (i = 0; i < (nsamples - codecleft) * 2; i++) {
                fbuffer[i] = ((int16_t *)g_readbuffer)[i]/32767.f;
            }
        }
        else if (codec->info.channels == 1) {
            for (i = 0; i < (nsamples - codecleft); i++) {
                fbuffer[i*2+0] = ((int16_t *)g_readbuffer)[i];
                fbuffer[i*2+1] = fbuffer[i*2+0];
            }
        }
        // convert samplerate
        srcdata.data_in = g_fbuffer;
        srcdata.data_out = g_srcbuffer;
        srcdata.input_frames = nsamples;
        srcdata.output_frames = size/4;
        srcdata.src_ratio = (double)sdl_player_freq/codec->info.samplesPerSecond;
        srcdata.end_of_input = 0;
        src_process (src, &srcdata);
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
        memmove (fbuffer, &fbuffer[srcdata.input_frames_used*2], codecleft * 8);
        codec_unlock ();
    }
    return bytesread;
}
