#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#define FRAME_RATE 30
#define HISTORY_FRAMES 100000

static ddb_waveformat_t _current_fmt;
static float freq_data[DDB_FREQ_BANDS * DDB_FREQ_MAX_CHANNELS];
static float audio_data[HISTORY_FRAMES * DDB_FREQ_MAX_CHANNELS];
static int audio_data_fill = 0; // the fill of the history, and also the position of the last sample sent to the output
static int audio_data_channels = 0;
static int read_pos = 0;
static int terminate = 0;
static uintptr_t tid = 0;

static void _viz_consumer_thread(void *ctx);

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
    tid = thread_start(_viz_consumer_thread, NULL);
}

void
viz_free (void) {
    terminate = 1;
    thread_join (tid);
    tid = 0;
    mutex_free (wdl_mutex);
    wdl_mutex = 0;
    free (_temp_audio_buffer);
    _temp_audio_buffer = NULL;
    _temp_audio_buffer_size = 0;
}

static void
_viz_consumer_thread(void *ctx) {
    for (;;) {
        if (terminate) {
            break;
        }

        mutex_lock(wdl_mutex);

        // calc fft
        for (int c = 0; c < audio_data_channels; c++) {
            calc_freq (&audio_data[HISTORY_FRAMES * c + read_pos], &freq_data[DDB_FREQ_BANDS * c]);
        }
        ddb_audio_data_t spectrum_data = {
            .fmt = &_current_fmt,
            .data = freq_data,
            .nframes = DDB_FREQ_BANDS
        };
        for (wavedata_listener_t *l = spectrum_listeners; l; l = l->next) {
            l->callback (l->ctx, &spectrum_data);
        }

        read_pos += _current_fmt.samplerate / FRAME_RATE;
        if (read_pos > audio_data_fill - DDB_FREQ_BANDS) {
            read_pos = audio_data_fill - DDB_FREQ_BANDS;
        }
        if (read_pos < 0) {
            read_pos = 0;
        }

        mutex_unlock(wdl_mutex);

        // run at 30 fps
        usleep (1000000/(FRAME_RATE+10));
    }
}

void
viz_waveform_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data)) {
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
viz_waveform_unlisten (void *ctx) {
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
viz_spectrum_listen (void *ctx, void (*callback)(void *ctx, ddb_audio_data_t *data)) {
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
viz_spectrum_unlisten (void *ctx) {
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
        memcpy (&_current_fmt, &out_fmt, sizeof (out_fmt));

        for (wavedata_listener_t *l = waveform_listeners; l; l = l->next) {
            l->callback (l->ctx, &waveform_data);
        }

        if (out_fmt.channels != audio_data_channels || !spectrum_listeners) {
            // reset
            audio_data_fill = HISTORY_FRAMES;
            read_pos = HISTORY_FRAMES - in_frames;
            audio_data_channels = out_fmt.channels;
        }

        if (spectrum_listeners) {
            // append after audio_data_fill

            int avail = HISTORY_FRAMES - audio_data_fill;
            if (avail < in_frames) {
                // shift buffer
                int shift_samples = in_frames - avail;
                for (int c = 0; c < audio_data_channels; c++) {
                    float *channel = &audio_data[HISTORY_FRAMES * c];
                    memmove (channel, channel + shift_samples * sizeof (float), (HISTORY_FRAMES - shift_samples) * sizeof (float));
                }
                audio_data_fill -= shift_samples;
                read_pos -= shift_samples;
                if (read_pos < 0) {
                    read_pos = 0;
                }
            }

            // append new samples at audio_data_fill
            for (int c = 0; c < audio_data_channels; c++) {
                float *channel = &audio_data[HISTORY_FRAMES * c + audio_data_fill];
                for (int s = 0; s < in_frames; s++) {
                    channel[s] = temp_audio_data[s * audio_data_channels + c];
                }
            }
            audio_data_fill += in_frames;
        }
        mutex_unlock (wdl_mutex);
    }
}

