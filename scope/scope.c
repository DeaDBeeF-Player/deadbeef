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

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "scope.h"
#include "../fastftoi.h"

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
        scope->mode_did_change = 1;
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
    int average_channels;
    float faverage_channels_inv;
    switch (scope->mode) {
    case DDB_SCOPE_MONO:
        output_channels = 1;
        average_channels = scope->channels;
        break;
    case DDB_SCOPE_MULTICHANNEL:
        output_channels = scope->channels;
        average_channels = 1;
        break;
    }

    faverage_channels_inv = 1.f / average_channels;

    float channel_height = view_height / output_channels;
    float pixel_amplitude = channel_height / 2;

    fpu_control fpu;
    (void)fpu;
    fpu_setround(&fpu);

    int left_a = 0;
    int left_b = 0;
    float leftfrac = 0;

    float fpoint_count = (float)draw_data->point_count;
    float fsample_count = (float)scope->sample_count;

    for (int i = 0; i < draw_data->point_count; i++) {
        float right = (float)(i+1) / fpoint_count * fsample_count;
        if (right > scope->sample_count-1) {
            right = scope->sample_count-1;
        }

        int right_a = ftoi(floorf(right));
        float fright_b = ceilf(right);
        int right_b = ftoi(fright_b);

        float rightfrac = fright_b - right;

        for (int c = 0; c < output_channels; c++) {
            draw_data->points[draw_data->point_count * c + i].ymin = 1;
            draw_data->points[draw_data->point_count * c + i].ymax = -1;
        }

        for (int c = 0; c < output_channels; c++) {
            int output_channel = c;

            ddb_scope_point_t * restrict minmax = &draw_data->points[draw_data->point_count * output_channel + i];
            float minmax_ymin = minmax->ymin;
            float minmax_ymax = minmax->ymax;

            // Interpolated leftmost and rightmost samples
            float leftsample = 0;
            float rightsample = 0;

            for (int ac = 0; ac < average_channels; ac++) {
                float leftsample_a = scope->samples[left_a * scope->channels + c + ac];
                float leftsample_b = scope->samples[left_b * scope->channels + c + ac];
                leftsample += leftsample_a + (leftsample_b - leftsample_a) * leftfrac;

                float rightsample_a = scope->samples[right_a * scope->channels + c + ac];
                float rightsample_b = scope->samples[right_b * scope->channels + c + ac];
                rightsample += rightsample_a + (rightsample_b - rightsample_a) * rightfrac;
            }

            leftsample *= faverage_channels_inv;
            rightsample *= faverage_channels_inv;

            if (leftsample > minmax_ymax) {
                minmax_ymax = leftsample;
            }
            if (leftsample < minmax_ymin) {
                minmax_ymin = leftsample;
            }

            if (rightsample > minmax_ymax) {
                minmax_ymax = rightsample;
            }
            if (rightsample < minmax_ymin) {
                minmax_ymin = rightsample;
            }

            // The rest of the "whole" samples
            for (int n = left_b; n <= right_a; n++) {
                float sample = 0;
                for (int ac = 0; ac < average_channels; ac++) {
                    sample += scope->samples[n * scope->channels + c + ac];
                }
                sample *= faverage_channels_inv;

                if (sample > minmax_ymax) {
                    minmax_ymax = sample;
                }
                if (sample < minmax_ymin) {
                    minmax_ymin = sample;
                }
            }

            int offs;
            float ymin;
            float ymax;
            if (y_axis_flip) {
                offs = output_channel * channel_height;
                ymin = -minmax_ymax;
                ymax = -minmax_ymin;
            }
            else {
                offs = (output_channels - output_channel - 1) * channel_height;
                ymin = minmax_ymin;
                ymax = minmax_ymax;
            }
            minmax->ymin = ymin * pixel_amplitude + pixel_amplitude + offs;
            minmax->ymax = ymax * pixel_amplitude + pixel_amplitude + offs;
        }
        left_a = right_a;
        left_b = right_b;
        leftfrac = rightfrac;
    }

    fpu_restore(fpu);

    draw_data->mode = scope->mode;
    draw_data->channels = scope->channels;
}

void
ddb_scope_draw_data_dealloc (ddb_scope_draw_data_t *draw_data) {
    free (draw_data->points);
    memset (draw_data, 0, sizeof(ddb_scope_draw_data_t));
}
