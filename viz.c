#include <stdlib.h>
#include <string.h>
#include "fft.h"
#include "premix.h"
#include "threading.h"
#include "viz.h"

// A buffer used in streamer_read.
static float *_temp_audio_buffer;
static size_t _temp_audio_buffer_size;

// Listeners
typedef struct wavedata_listener_s {
    void *ctx;
    void (*callback)(void *ctx, ddb_audio_data_t *data);
    struct wavedata_listener_s *next;
} wavedata_listener_t;

static wavedata_listener_t *waveform_listeners;
static wavedata_listener_t *spectrum_listeners;
static uintptr_t wdl_mutex; // wavedata listener

static float freq_data[DDB_FREQ_BANDS * DDB_FREQ_MAX_CHANNELS];
static float audio_data[DDB_FREQ_BANDS * 2 * DDB_FREQ_MAX_CHANNELS];
static int audio_data_fill = 0;
static int audio_data_channels = 0;

static float *
_get_temp_audio_buffer (size_t size) {
    if (_temp_audio_buffer) {
        if (_temp_audio_buffer_size >= size) {
            return _temp_audio_buffer;
        }
        free (_temp_audio_buffer);
    }
    _temp_audio_buffer_size = size;
    _temp_audio_buffer = malloc (_temp_audio_buffer_size);
    return _temp_audio_buffer;
}

void
viz_init (void) {
    wdl_mutex = mutex_create ();
}

void
viz_free (void) {
    mutex_free (wdl_mutex);
    wdl_mutex = 0;
    free (_temp_audio_buffer);
    _temp_audio_buffer = NULL;
    _temp_audio_buffer_size = 0;
}

void
vis_waveform_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data)) {
    mutex_lock (wdl_mutex);
    wavedata_listener_t *l = malloc (sizeof (wavedata_listener_t));
    memset (l, 0, sizeof (wavedata_listener_t));
    l->ctx = ctx;
    l->callback = callback;
    l->next = waveform_listeners;
    waveform_listeners = l;
    mutex_unlock (wdl_mutex);
}

void
vis_waveform_unlisten (void *ctx) {
    mutex_lock (wdl_mutex);
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
    mutex_unlock (wdl_mutex);
}

void
vis_spectrum_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data)) {
    mutex_lock (wdl_mutex);
    wavedata_listener_t *l = malloc (sizeof (wavedata_listener_t));
    memset (l, 0, sizeof (wavedata_listener_t));
    l->ctx = ctx;
    l->callback = callback;
    l->next = spectrum_listeners;
    spectrum_listeners = l;
    mutex_unlock (wdl_mutex);
}

void
vis_spectrum_unlisten (void *ctx) {
    mutex_lock (wdl_mutex);
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
    mutex_unlock (wdl_mutex);
}

void
viz_process (char * restrict bytes, int bytes_size, DB_output_t *output) {
    if (waveform_listeners || spectrum_listeners) {
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

        float *temp_audio_data = _get_temp_audio_buffer (in_frames * out_fmt.channels * sizeof (float));
        pcm_convert (&output->fmt, bytes, &out_fmt, (char *)temp_audio_data, bytes_size);
        ddb_audio_data_t waveform_data = {
            .fmt = &out_fmt,
            .data = temp_audio_data,
            .nframes = in_frames
        };
        mutex_lock (wdl_mutex);
        for (wavedata_listener_t *l = waveform_listeners; l; l = l->next) {
            l->callback (l->ctx, &waveform_data);
        }
        mutex_unlock (wdl_mutex);

        if (out_fmt.channels != audio_data_channels || !spectrum_listeners) {
            audio_data_fill = 0;
            audio_data_channels = out_fmt.channels;
        }

        if (spectrum_listeners) {
            // shift buffer
            int shift_samples = 0;
            if (in_frames < DDB_FREQ_BANDS * 2) {
                shift_samples = DDB_FREQ_BANDS * 2 - in_frames;
                for (int c = 0; c < audio_data_channels; c++) {
                    float *channel = &audio_data[DDB_FREQ_BANDS * 2 * c];
                    memmove (channel, channel + in_frames * sizeof (float), shift_samples * sizeof (float));
                }
            }

            // append new samples
            for (int c = 0; c < audio_data_channels; c++) {
                float *channel = &audio_data[DDB_FREQ_BANDS * 2 * c];
                for (int s = 0; s < in_frames; s++) {
                    channel[s + shift_samples] = temp_audio_data[s * audio_data_channels + c];
                }
            }

            // calc fft
            for (int c = 0; c < audio_data_channels; c++) {
                calc_freq (&audio_data[DDB_FREQ_BANDS * 2 * c], &freq_data[DDB_FREQ_BANDS * c]);
            }
            ddb_audio_data_t spectrum_data = {
                .fmt = &out_fmt,
                .data = freq_data,
                .nframes = DDB_FREQ_BANDS
            };
            mutex_lock (wdl_mutex);
            for (wavedata_listener_t *l = spectrum_listeners; l; l = l->next) {
                l->callback (l->ctx, &spectrum_data);
            }
            mutex_unlock (wdl_mutex);
            audio_data_fill = 0;
        }
    }
}

