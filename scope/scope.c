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
ddb_scope_get_draw_data (ddb_scope_t * restrict scope, int view_width, int view_height, ddb_scope_draw_data_t * restrict draw_data) {
    if (scope->mode_did_change
        || draw_data->point_count != view_width) {
        free (draw_data->points);
        int channels = scope->mode == DDB_SCOPE_MONO ? 1 : scope->channels;
        draw_data->points = calloc (view_width * channels, sizeof (ddb_scope_point_t));
        draw_data->point_count = view_width;
        scope->mode_did_change = 0;
    }
    float incr = (float)view_width / scope->sample_count;
    float ypos_min = 1;
    float ypos_max = -1;
    float n = 0;
    int point_index = 0;

    int output_channels = scope->mode == DDB_SCOPE_MULTICHANNEL ? scope->channels : 1;

    float channel_height = view_height / output_channels;
    float pixel_amplitude = channel_height / 2;

    float ymin_prev = pixel_amplitude;
    float ymax_prev = pixel_amplitude;

    for (int output_channel = 0; output_channel < output_channels; output_channel++) {
        int channel_point_index = 0;
        float xpos = 0;
        for (int i = 0; i < scope->sample_count && channel_point_index < draw_data->point_count; i++) {
            float new_xpos = xpos + incr;
            float pixel_xpos = floor(xpos);
            if (floor(new_xpos) > pixel_xpos) {
                if (n > 0) {
                    float ymin = (ypos_min * pixel_amplitude + pixel_amplitude) + output_channel * channel_height;
                    float ymax = (ypos_max * pixel_amplitude + pixel_amplitude) + output_channel * channel_height;
                    if (ymin > ymax_prev) {
                        ymin = ymax_prev;
                    }
                    if (ymax < ymin_prev) {
                        ymax = ymin_prev;
                    }
                    if (ymax - ymin < 1) {
                        ymin = ymax-1;
                    }

                    draw_data->points[point_index].ymin = ymin;
                    draw_data->points[point_index].ymax = ymax;
                    point_index += 1;
                    channel_point_index += 1;

                    ymin_prev = ymin;
                    ymax_prev = ymax;
                }
                n = 0;
                ypos_min = 1;
                ypos_max = -1;
            }
            xpos = new_xpos;
            float sample = 0;
            if (scope->mode == DDB_SCOPE_MONO) {
                int ch = scope->channels;
                for (int c = 0; c < ch; c++) {
                    sample = scope->samples[i * scope->channels + c];
                }
                sample /= ch;
            }
            else if (scope->mode == DDB_SCOPE_MULTICHANNEL) {
                sample = scope->samples[i * scope->channels + output_channel];
            }
            if (sample > ypos_max) {
                ypos_max = sample;
            }
            if (sample < ypos_min) {
                ypos_min = sample;
            }
            n += 1;
        }
        if (n > 0 && point_index < draw_data->point_count) {
            draw_data->points[point_index].ymin = ypos_min * pixel_amplitude + pixel_amplitude;
            draw_data->points[point_index].ymax = ypos_max * pixel_amplitude + pixel_amplitude;
            point_index += 1;
            channel_point_index += 1;
        }
    }
}

void
ddb_scope_draw_data_dealloc (ddb_scope_draw_data_t *draw_data) {
    free (draw_data->points);
    memset (draw_data, 0, sizeof(ddb_scope_draw_data_t));
}
