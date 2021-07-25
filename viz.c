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

#define HISTORY_FRAMES 100000

static float freq_data[DDB_FREQ_BANDS * DDB_FREQ_MAX_CHANNELS];
static float audio_data[HISTORY_FRAMES * DDB_FREQ_MAX_CHANNELS];
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

        // if the input is larger than buffer, use the tail
        if (in_frames > DDB_FREQ_BANDS * 2) {
            bytes += (in_frames - DDB_FREQ_BANDS * 2) * in_frame_size;
            in_frames = DDB_FREQ_BANDS * 2;
            bytes_size = DDB_FREQ_BANDS * 2 * in_frame_size;
        }

        // convert to float
        ddb_waveformat_t out_fmt = {
            .bps = 32,
            .channels = output->fmt.channels,
            .samplerate = output->fmt.samplerate,
            .channelmask = output->fmt.channelmask,
            .is_float = 1,
            .is_bigendian = 0
        };

        size_t data_size = in_frames * out_fmt.channels * sizeof (float);
        __block float *data = malloc(data_size);
        pcm_convert (&output->fmt, bytes, &out_fmt, (char *)data, bytes_size);

        ddb_audio_data_t waveform_data = {
            .fmt = &out_fmt,
            .data = data,
            .nframes = in_frames
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
                // shift buffer
                for (int c = 0; c < audio_data_channels; c++) {
                    float *channel = &audio_data[HISTORY_FRAMES * c];
                    memmove (channel, channel + in_frames, (HISTORY_FRAMES - in_frames) * sizeof (float));
                }
                // append new samples in planar layout
                for (int c = 0; c < audio_data_channels; c++) {
                    float *channel = &audio_data[HISTORY_FRAMES * c + HISTORY_FRAMES - in_frames];
                    for (int s = 0; s < in_frames; s++) {
                        channel[s] = data[s * audio_data_channels + c];
                    }
                }

                // calc fft
                for (int c = 0; c < audio_data_channels; c++) {
                    fft_calculate (&audio_data[HISTORY_FRAMES * c + HISTORY_FRAMES - DDB_FREQ_BANDS*2], &freq_data[DDB_FREQ_BANDS * c]);
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

