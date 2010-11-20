/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <samplerate.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "conf.h"
#include "threading.h"
#include "src.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define SRC_BUFFER 16000
#define SRC_MAX_CHANNELS 8

typedef struct {
    ddb_src_t ddb_src;

    int quality;
    SRC_STATE *src;
    SRC_DATA srcdata;
    int remaining; // number of input samples in SRC buffer
    __attribute__((__aligned__(16))) char in_fbuffer[sizeof(float)*SRC_BUFFER*SRC_MAX_CHANNELS];
//    __attribute__((__aligned__(16))) float out_fbuffer[SRC_BUFFER*SRC_MAX_CHANNELS];
    uintptr_t mutex;
} ddb_src_libsamplerate_t;

ddb_src_t *
ddb_src_init (void) {
    ddb_src_libsamplerate_t *src = malloc (sizeof (ddb_src_libsamplerate_t));
    memset (src, 0, sizeof (ddb_src_libsamplerate_t));
    src->mutex = mutex_create ();
    src->quality = conf_get_int ("src->quality", 2);
    src->src = src_new (src->quality, 2, NULL);
    if (!src->src) {
        ddb_src_free ((ddb_src_t *)src);
        return NULL;
    }
    return (ddb_src_t *)src;
}

void
ddb_src_free (ddb_src_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    if (src->mutex) {
        mutex_free (src->mutex);
        src->mutex = 0;
    }
    if (src->src) {
        src_delete (src->src);
        src->src = NULL;
    }
    free (src);
}

void
ddb_src_lock (ddb_src_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    mutex_lock (src->mutex);
}

void
ddb_src_unlock (ddb_src_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    mutex_unlock (src->mutex);
}

void
ddb_src_reset (ddb_src_t *_src, int full) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    src->remaining = 0;
    if (full) {
        src_reset (src->src);
    }
}

void
ddb_src_confchanged (ddb_src_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    int q = conf_get_int ("src->quality", 2);
    if (q != src->quality && q >= SRC_SINC_BEST_QUALITY && q <= SRC_LINEAR) {
        ddb_src_lock (_src);
        trace ("changing src->quality from %d to %d\n", src->quality, q);
        src->quality = q;
        if (src) {
            src_delete (src->src);
            src->src = NULL;
        }
        memset (&src->srcdata, 0, sizeof (src->srcdata));
        src->src = src_new (src->quality, 2, NULL);
        ddb_src_unlock (_src);
    }
}

int
ddb_src_process (ddb_src_t *_src, const char * restrict input, int nframes, char * restrict output, int buffersize, float ratio, int nchannels) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;

extern FILE *out;
    int initsize = buffersize;
    int samplesize = nchannels * sizeof (float);

    do {
        // add more frames to input SRC buffer
        int n = nframes;
        if (n >= SRC_BUFFER - src->remaining) {
            n = SRC_BUFFER - src->remaining;
        }

        if (n > 0) {
            memcpy (&src->in_fbuffer[src->remaining*samplesize], input, n * samplesize);

            src->remaining += n;
            input += n * nchannels;
            nframes -= n;
        }
        if (!src->remaining) {
            //trace ("WARNING: SRC input buffer starved\n");
            break;
        }

        // call libsamplerate
        src->srcdata.data_in = (float *)src->in_fbuffer;
        src->srcdata.data_out = (float *)output;
        src->srcdata.input_frames = src->remaining;
        src->srcdata.output_frames = buffersize/samplesize;
        src->srcdata.src_ratio = ratio;
        src->srcdata.end_of_input = 0;
        ddb_src_lock (_src);
        src_set_ratio (src->src, ratio);
        trace ("src input: %d, ratio %f, buffersize: %d\n", src->srcdata.input_frames, ratio, buffersize);
        int src_err = src_process (src->src, &src->srcdata);
        trace ("src output: %d, used: %d\n", src->srcdata.output_frames_gen, src->srcdata.input_frames_used);

        ddb_src_unlock (_src);
        if (src_err) {
            const char *err = src_strerror (src_err) ;
            fprintf (stderr, "src_process error %s\n"
                    "srcdata.data_in=%p, srcdata.data_out=%p, srcdata.input_frames=%d, srcdata.output_frames=%d, srcdata.src_ratio=%f", err, src->srcdata.data_in, src->srcdata.data_out, (int)src->srcdata.input_frames, (int)src->srcdata.output_frames, ratio);
            exit (-1);
        }

        buffersize -= src->srcdata.output_frames_gen * samplesize;
        output += src->srcdata.output_frames_gen * samplesize;

        // calculate how many unused input samples left
        src->remaining -= src->srcdata.input_frames_used;
        // copy spare samples for next update
        if (src->remaining > 0 && src->srcdata.input_frames_used > 0) {
            memmove (src->in_fbuffer, &src->in_fbuffer[src->srcdata.input_frames_used*samplesize], src->remaining * samplesize);
        }
        if (src->srcdata.output_frames_gen == 0) {
            break;
        }
    } while (nframes > 0);

    trace ("src processed: %d bytes\n", initsize-buffersize);
    return initsize - buffersize;
}

