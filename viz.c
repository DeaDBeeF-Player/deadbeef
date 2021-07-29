/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

static float freq_data[DDB_FREQ_BANDS * DDB_FREQ_MAX_CHANNELS];
static float audio_data[DDB_FREQ_BANDS * 2 * DDB_FREQ_MAX_CHANNELS];
static int _need_reset = 0;
static int audio_data_channels = 0;

void
viz_init (void) {
    sync_queue = dispatch_queue_create("Viz Sync Queue", NULL);
    process_queue = dispatch_queue_create("Viz Process Queue", NULL);
}

void
viz_free (void) {
    dispatch_release(process_queue);
    dispatch_release(sync_queue);
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
viz_process (char * restrict _bytes, int _bytes_size, DB_output_t *output) {
    dispatch_sync(sync_queue, ^{
        if (!waveform_listeners && !spectrum_listeners) {
            return;
        }

        char * bytes = _bytes;
        int bytes_size = _bytes_size;

        int in_frame_size = (output->fmt.bps >> 3) * output->fmt.channels;
        int in_frames = bytes_size / in_frame_size;

        // convert to float
        ddb_waveformat_t out_fmt = {
            .bps = 32,
            .channels = output->fmt.channels,
            .samplerate = output->fmt.samplerate,
            .channelmask = output->fmt.channelmask,
            .is_float = 1,
            .is_bigendian = 0
        };

        int process_samples = in_frames;
        // if the input is smaller than the buffer, pad with zeroes
        if (process_samples != DDB_FREQ_BANDS * 2) {
            process_samples = DDB_FREQ_BANDS * 2;
            bytes_size = process_samples * output->fmt.channels * output->fmt.bps/8;
        }
        size_t data_size = process_samples * out_fmt.channels * sizeof (float);
        __block float *data = calloc(1, data_size);

        pcm_convert (&output->fmt, bytes, &out_fmt, (char *)data, bytes_size);

        ddb_audio_data_t waveform_data = {
            .fmt = &out_fmt,
            .data = data,
            .nframes = process_samples
        };

        // Pass to async queue for processing and callbacks
        dispatch_async (process_queue, ^{
            for (wavedata_listener_t *l = waveform_listeners; l; l = l->next) {
                l->callback (l->ctx, &waveform_data);
            }

            if (_need_reset || out_fmt.channels != audio_data_channels || !spectrum_listeners) {
                // reset
                audio_data_channels = out_fmt.channels;
                memset (freq_data, 0, sizeof (freq_data));
                memset (audio_data, 0, sizeof (audio_data));
                _need_reset = 0;
            }

            if (spectrum_listeners) {
                // convert samples in planar layout
                assert (process_samples == DDB_FREQ_BANDS * 2);
                for (int c = 0; c < audio_data_channels; c++) {
                    float *channel = &audio_data[DDB_FREQ_BANDS * 2 * c];
                    for (int s = 0; s < process_samples; s++) {
                        channel[s] = data[s * audio_data_channels + c];
                    }
                    // pad zeroes
                    for (int s = process_samples; s < DDB_FREQ_BANDS * 2; s++) {
                        channel[s] = 0;
                    }
                }

                // calc fft
                for (int c = 0; c < audio_data_channels; c++) {
                    fft_calculate (&audio_data[DDB_FREQ_BANDS * 2 * c], &freq_data[DDB_FREQ_BANDS * c]);
                }
                ddb_audio_data_t spectrum_data = {
                    .fmt = &out_fmt,
                    .data = freq_data,
                    .nframes = DDB_FREQ_BANDS
                };
                for (wavedata_listener_t *l = spectrum_listeners; l; l = l->next) {
                    l->callback (l->ctx, &spectrum_data);
                }
            }

            free (data);
        });
    });
}

