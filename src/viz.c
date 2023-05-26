/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
*/
#include <assert.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fft.h"
#include "premix.h"
#include "threading.h"
#include "viz.h"

static dispatch_queue_t sync_queue;
static dispatch_queue_t process_queue;

// Listeners
typedef struct wavedata_listener_s {
    void *ctx;
    void (*callback)(void *ctx, const ddb_audio_data_t *data);
    struct wavedata_listener_s *next;
} wavedata_listener_t;

static wavedata_listener_t *waveform_listeners;
static wavedata_listener_t *spectrum_listeners;

//#define HISTORY_FRAMES 100000

static int _fft_size = 0;
static float *_freq_data;
static float *_audio_data;
static int _need_reset = 0;
static int audio_data_channels = 0;

static void
_free_buffers (void) {
    free (_freq_data);
    free (_audio_data);
    _freq_data = NULL;
    _audio_data = NULL;
}

static void
_init_buffers (int fft_size) {
    if (fft_size != _fft_size) {
        _free_buffers ();
        if (fft_size != 0) {
            _freq_data = calloc(fft_size * DDB_FREQ_MAX_CHANNELS, sizeof (float));
            _audio_data = calloc(fft_size * 2 * DDB_FREQ_MAX_CHANNELS, sizeof (float));
        }
        _fft_size = fft_size;
    }
}

void
viz_init (void) {
    sync_queue = dispatch_queue_create("Viz Sync Queue", NULL);
    process_queue = dispatch_queue_create("Viz Process Queue", NULL);
}

void
viz_free (void) {
    dispatch_release(process_queue);
    dispatch_release(sync_queue);
    _free_buffers();
}

void
viz_waveform_listen (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data)) {
    dispatch_sync(sync_queue, ^{
        wavedata_listener_t *l = malloc (sizeof (wavedata_listener_t));
        memset (l, 0, sizeof (wavedata_listener_t));
        l->ctx = ctx;
        l->callback = callback;
        l->next = waveform_listeners;
        waveform_listeners = l;
    });
}

void
viz_waveform_unlisten (void *ctx) {
    dispatch_sync(sync_queue, ^{
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
    });
}

void
viz_spectrum_listen (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data)) {
    dispatch_sync(sync_queue, ^{
        wavedata_listener_t *l = malloc (sizeof (wavedata_listener_t));
        memset (l, 0, sizeof (wavedata_listener_t));
        l->ctx = ctx;
        l->callback = callback;
        l->next = spectrum_listeners;
        spectrum_listeners = l;
    });
}

void
viz_spectrum_unlisten (void *ctx) {
    dispatch_sync(sync_queue, ^{
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
    });
}

void
viz_reset (void) {
    dispatch_sync(sync_queue, ^{
        _need_reset = 1;
    });
}

void
viz_process (char * restrict _bytes, int _bytes_size, DB_output_t *output, int fft_size, int wave_size) {
    dispatch_sync(sync_queue, ^{
        _init_buffers(fft_size);
        if (!waveform_listeners && !spectrum_listeners) {
            return;
        }

        char *bytes = _bytes;
        if (output->fmt.is_dop) {
            bytes = NULL;
        }
        
        // convert to float
        ddb_waveformat_t *out_fmt = calloc (1, sizeof (ddb_waveformat_t));
        out_fmt->bps = 32;
        out_fmt->channels = output->fmt.channels;
        out_fmt->samplerate = output->fmt.samplerate;
        out_fmt->channelmask = output->fmt.channelmask;
        out_fmt->is_float = 1;
        out_fmt->is_bigendian = 0;

        const int fft_nframes = fft_size * 2;

        // calculate the size which can fit either the FFT input, or the wave data.
        const int output_nframes = fft_nframes > wave_size ? fft_nframes : wave_size;

        const int final_output_size = output_nframes * out_fmt->channels * sizeof (float);
        const int final_input_size = output_nframes * output->fmt.channels * (output->fmt.bps/8);
        float *data = calloc(1, final_output_size);

        if (bytes != NULL) {
            // take only as much bytes as we have available.
            const int convert_size = _bytes_size < final_input_size ? _bytes_size : final_input_size;

            // After this runs, we'll have a buffer with enough samples for FFT, padded with 0s if needed.
            pcm_convert (&output->fmt, bytes, out_fmt, (char *)data, convert_size);
        }

        ddb_audio_data_t *waveform_data = calloc(1, sizeof(ddb_audio_data_t));
        waveform_data->fmt = out_fmt;
        waveform_data->data = data;
        waveform_data->nframes = wave_size;

        if (_need_reset || out_fmt->channels != audio_data_channels || !spectrum_listeners) {
            // reset
            audio_data_channels = out_fmt->channels;
            memset (_freq_data, 0, sizeof (float) * _fft_size * DDB_FREQ_MAX_CHANNELS);
            memset (_audio_data, 0, sizeof (float) * _fft_size * 2 * DDB_FREQ_MAX_CHANNELS);
            _need_reset = 0;
        }

        if (spectrum_listeners) {
            // convert samples in planar layout
            assert (fft_nframes == _fft_size * 2);
            for (int c = 0; c < audio_data_channels; c++) {
                float *channel = &_audio_data[_fft_size * 2 * c];
                for (int s = 0; s < fft_nframes; s++) {
                    channel[s] = data[s * audio_data_channels + c];
                }
            }

            // calc fft
            if (_audio_data != NULL) {
                for (int c = 0; c < audio_data_channels; c++) {
                    fft_calculate (&_audio_data[_fft_size * 2 * c], &_freq_data[_fft_size * c], _fft_size);
                }
            }
            ddb_audio_data_t spectrum_data = {
                .fmt = out_fmt,
                .data = _freq_data,
                .nframes = _fft_size
            };
            for (wavedata_listener_t *l = spectrum_listeners; l; l = l->next) {
                l->callback (l->ctx, &spectrum_data);
            }
        }

        // Pass to async queue for processing and callbacks
        dispatch_async (process_queue, ^{
            for (wavedata_listener_t *l = waveform_listeners; l; l = l->next) {
                l->callback (l->ctx, waveform_data);
            }

            free (data);
            free (out_fmt);
            free (waveform_data);
        });

    });
}

