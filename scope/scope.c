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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "scope.h"

#pragma mark - Public

ddb_scope_t *
ddb_scope_alloc (void) {
    return calloc(1, sizeof (ddb_scope_t));
}

ddb_scope_t *
ddb_scope_init (ddb_scope_t *scope) {
    return scope;
}

void
ddb_scope_dealloc (ddb_scope_t *scope) {
    free (scope->samples);
    memset (scope, 0, sizeof (ddb_scope_t));
}

void
ddb_scope_free (ddb_scope_t *scope) {
    free (scope);
}

void
ddb_scope_process (ddb_scope_t * restrict scope, int samplerate, int channels, const float * restrict samples, int sample_count) {

    if (scope->fragment_duration == 0) {
        scope->fragment_duration = 50;
    }

    int fragment_sample_count = (float)scope->fragment_duration / 1000.f * samplerate;
    if (channels != scope->channels
        || samplerate != scope->samplerate
        || fragment_sample_count != scope->sample_count) {
        scope->channels = channels;
        scope->sample_count = fragment_sample_count;
        scope->samplerate = samplerate;
        free (scope->samples);
        scope->samples = calloc (scope->sample_count * channels, sizeof (float));
    }

    // append samples

    // input size larger than the buffer? copy the tail.
    if (sample_count > scope->sample_count) {
        memcpy (scope->samples, samples + (sample_count - scope->sample_count) * channels, scope->sample_count * channels * sizeof (float));
    }
    // otherwise append
    else {
        int move_samples = scope->sample_count - sample_count;
        memmove (scope->samples, scope->samples + (scope->sample_count - move_samples) * channels, move_samples * channels * sizeof(float));
        memcpy (scope->samples + move_samples * channels, samples, sample_count * channels * sizeof (float));
    }
}

void
ddb_scope_tick (ddb_scope_t * restrict scope) {
}

void
ddb_scope_get_draw_data (ddb_scope_t * restrict scope, int view_width, int view_height, int y_axis_flip, ddb_scope_draw_data_t * restrict draw_data) {
    if (scope->mode_did_change
        || draw_data->point_count != view_width) {
        free (draw_data->points);
        int channels = scope->mode == DDB_SCOPE_MONO ? 1 : scope->channels;
        draw_data->points = calloc (view_width * channels, sizeof (ddb_scope_point_t));
        draw_data->point_count = view_width;
        scope->mode_did_change = 0;
    }

    int output_channels;
    switch (scope->mode) {
    case DDB_SCOPE_MONO:
        output_channels = 1;
        break;
    case DDB_SCOPE_MULTICHANNEL:
        output_channels = scope->channels;
        break;
    }

    float channel_height = view_height / output_channels;
    float pixel_amplitude = channel_height / 2;

    float left = 0;
    float left_a = 0;
    float left_b = 0;
    float leftfrac = 0;
    for (int i = 0; i < draw_data->point_count; i++) {
        float right = (float)(i+1) / draw_data->point_count * scope->sample_count;
        if (right > scope->sample_count-1) {
            right = scope->sample_count-1;
        }

        float right_a = floor(right);
        float right_b = ceil(right);

        float rightfrac = right_b-right;

        for (int c = 0; c < output_channels; c++) {
            draw_data->points[draw_data->point_count * c + i].ymin = 1;
            draw_data->points[draw_data->point_count * c + i].ymax = -1;
        }

        for (int c = 0; c < scope->channels; c++) {
            int output_channel;

            switch (scope->mode) {
            case DDB_SCOPE_MONO:
                output_channel = 0;
                break;
            case DDB_SCOPE_MULTICHANNEL:
                output_channel = c;
                break;
            }

            ddb_scope_point_t *minmax = &draw_data->points[draw_data->point_count * output_channel + i];

            float leftsample_a = scope->samples[(int)left_a * scope->channels + c];
            float leftsample_b = scope->samples[(int)left_b * scope->channels + c];
            float leftsample = leftsample_a + (leftsample_b - leftsample_a) * leftfrac;

            if (leftsample > minmax->ymax) {
                minmax->ymax = leftsample;
            }
            if (leftsample < minmax->ymin) {
                minmax->ymin = leftsample;
            }

            float rightsample_a = scope->samples[(int)right_a * scope->channels + c];
            float rightsample_b = scope->samples[(int)right_b * scope->channels + c];
            float rightsample = rightsample_a + (rightsample_b - rightsample_a) * rightfrac;

            if (rightsample > minmax->ymax) {
                minmax->ymax = rightsample;
            }
            if (rightsample < minmax->ymin) {
                minmax->ymin = rightsample;
            }

            for (int n = (int)left_b; n <= (int)right_a; n++) {
                float sample = scope->samples[n * scope->channels + c];

                if (sample > minmax->ymax) {
                    minmax->ymax = sample;
                }
                if (sample < minmax->ymin) {
                    minmax->ymin = sample;
                }
            }

            int rescale = 0;
            switch (scope->mode) {
            case DDB_SCOPE_MONO:
                if (c == scope->channels - 1) {
                    rescale = 1;
                }
                break;
            case DDB_SCOPE_MULTICHANNEL:
                rescale = 1;
                break;
            }

            if (rescale) {
                int offs;
                float ymin;
                float ymax;
                if (y_axis_flip) {
                    offs = output_channel * channel_height;
                    ymin = -minmax->ymax;
                    ymax = -minmax->ymin;
                }
                else {
                    offs = (output_channels - output_channel - 1) * channel_height;
                    ymin = minmax->ymin;
                    ymax = minmax->ymax;
                }
                minmax->ymin = ymin * pixel_amplitude + pixel_amplitude + offs;
                minmax->ymax = ymax * pixel_amplitude + pixel_amplitude + offs;
            }
        }
        left = right;
        left_a = right_a;
        left_b = right_b;
        leftfrac = rightfrac;
    }

    draw_data->mode = scope->mode;
    draw_data->channels = scope->channels;
}

void
ddb_scope_draw_data_dealloc (ddb_scope_draw_data_t *draw_data) {
    free (draw_data->points);
    memset (draw_data, 0, sizeof(ddb_scope_draw_data_t));
}
