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
#include "deadbeef.h"
#include "src.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

DB_functions_t *deadbeef;

#define SRC_BUFFER 16000
#define SRC_MAX_CHANNELS 8

ddb_dsp_src_t plugin;

typedef struct {
    DB_dsp_instance_t inst;

    int quality;
    SRC_STATE *src;
    SRC_DATA srcdata;
    int remaining; // number of input samples in SRC buffer
    __attribute__((__aligned__(16))) char in_fbuffer[sizeof(float)*SRC_BUFFER*SRC_MAX_CHANNELS];
//    __attribute__((__aligned__(16))) float out_fbuffer[SRC_BUFFER*SRC_MAX_CHANNELS];
    uintptr_t mutex;
} ddb_src_libsamplerate_t;

DB_dsp_instance_t* ddb_src_open (const char *id) {
    ddb_src_libsamplerate_t *src = malloc (sizeof (ddb_src_libsamplerate_t));
    DDB_INIT_DSP_INSTANCE (src,ddb_src_libsamplerate_t);

    src->mutex = deadbeef->mutex_create ();
    char var[20];
    snprintf (var, sizeof (var), "%s.quality");
    src->quality = deadbeef->conf_get_int (var, 2);
    src->src = src_new (src->quality, 2, NULL);
    if (!src->src) {
        plugin.dsp.close ((DB_dsp_instance_t *)src);
        return NULL;
    }
    return (DB_dsp_instance_t *)src;
}

void
ddb_src_close (DB_dsp_instance_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    if (src->mutex) {
        deadbeef->mutex_free (src->mutex);
        src->mutex = 0;
    }
    if (src->src) {
        src_delete (src->src);
        src->src = NULL;
    }
    free (src);
}

void
ddb_src_lock (DB_dsp_instance_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    deadbeef->mutex_lock (src->mutex);
}

void
ddb_src_unlock (DB_dsp_instance_t *_src) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    deadbeef->mutex_unlock (src->mutex);
}

void
ddb_src_reset (DB_dsp_instance_t *_src, int full) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    src->remaining = 0;
    if (full) {
        char var[20];
        snprintf (var, sizeof (var), "%s.quality");
        int q = deadbeef->conf_get_int (var, 2);
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
        else {
            src_reset (src->src);
        }
    }
}

void
ddb_src_set_ratio (DB_dsp_instance_t *_src, float ratio) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;
    src->srcdata.src_ratio = ratio;
    src_set_ratio (src->src, ratio);
}

int
//ddb_src_process (DB_dsp_instance_t *_src, const char * restrict input, int nframes, char * restrict output, int buffersize, float ratio, int nchannels) {
ddb_src_process (DB_dsp_instance_t *_src, float *samples, int nframes, int nchannels) {
    ddb_src_libsamplerate_t *src = (ddb_src_libsamplerate_t*)_src;

    int numoutframes = nframes * src->srcdata.src_ratio;
    float outbuf[numoutframes*nchannels];
    int buffersize = sizeof (outbuf);
    char *output = outbuf;
    float *input = samples;
    int inputsize = numoutframes;

    int samplesize = nchannels * sizeof (float);

    do {
        // add more frames to input SRC buffer
        int n = nframes;
        if (n >= SRC_BUFFER - src->remaining) {
            n = SRC_BUFFER - src->remaining;
        }

        if (n > 0) {
            memcpy (&src->in_fbuffer[src->remaining*samplesize], samples, n * samplesize);

            src->remaining += n;
            samples += n * nchannels;
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
        src->srcdata.output_frames = inputsize;
        src->srcdata.end_of_input = 0;
        ddb_src_lock (_src);
        trace ("src input: %d, ratio %f, buffersize: %d\n", src->srcdata.input_frames, src->srcdata.src_ratio, buffersize);
        int src_err = src_process (src->src, &src->srcdata);
        trace ("src output: %d, used: %d\n", src->srcdata.output_frames_gen, src->srcdata.input_frames_used);

        ddb_src_unlock (_src);
        if (src_err) {
            const char *err = src_strerror (src_err) ;
            fprintf (stderr, "src_process error %s\n"
                    "srcdata.data_in=%p, srcdata.data_out=%p, srcdata.input_frames=%d, srcdata.output_frames=%d, srcdata.src_ratio=%f", err, src->srcdata.data_in, src->srcdata.data_out, (int)src->srcdata.input_frames, (int)src->srcdata.output_frames, src->srcdata.src_ratio);
            exit (-1);
        }

        inputsize -= src->srcdata.output_frames_gen;
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

    memcpy (input, outbuf, sizeof (outbuf));
    //static FILE *out = NULL;
    //if (!out) {
    //    out = fopen ("out.raw", "w+b");
    //}
    //fwrite (input, 1,  numoutframes*sizeof(float)*nchannels, out);

    return numoutframes;
}

ddb_dsp_src_t plugin = {
    .dsp.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .dsp.plugin.api_vminor = DB_API_VERSION_MINOR,
    .dsp.open = ddb_src_open,
    .dsp.close = ddb_src_close,
    .dsp.process = ddb_src_process,
    .dsp.plugin.version_major = 0,
    .dsp.plugin.version_minor = 1,
    .dsp.plugin.id = "dsp_src",
    .dsp.plugin.name = "dsp_src",
    .dsp.plugin.descr = "Samplerate converter using libsamplerate",
    .dsp.plugin.author = "Alexey Yakovenko",
    .dsp.plugin.email = "waker@users.sf.net",
    .dsp.plugin.website = "http://deadbeef.sf.net",
    .reset = ddb_src_reset,
    .set_ratio = ddb_src_set_ratio,
};

DB_plugin_t *
dsp_libsrc_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.dsp.plugin;
}
