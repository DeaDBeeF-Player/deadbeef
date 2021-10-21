//
//  scope.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

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
    if (channels != scope->channels || sample_count != scope->sample_count || samplerate != scope->samplerate) {
        scope->channels = channels;
        scope->sample_count = sample_count;
        scope->samplerate = samplerate;
        free (scope->samples);
        scope->samples = malloc (sample_count * channels * sizeof (float));
    }

    memcpy (scope->samples, samples, sample_count * channels * sizeof (float));
}

void
ddb_scope_tick (ddb_scope_t * restrict scope) {
}

void
ddb_scope_get_draw_data (ddb_scope_t * restrict scope, int view_width, int view_height, ddb_scope_draw_data_t * restrict draw_data) {
    if (draw_data->point_count != view_width) {
        free (draw_data->points);
        draw_data->points = calloc (view_width, sizeof (ddb_scope_point_t));
        draw_data->point_count = view_width;
    }
    float pixel_amplitude = view_height/2;
    float incr = (float)view_width / scope->sample_count;
    float xpos = 0;
    float ypos_min = 1;
    float ypos_max = -1;
    float n = 0;
    int point_index = 0;
    for (int i = 0; i < scope->sample_count && point_index < draw_data->point_count; i++) {
        float new_xpos = xpos + incr;
        float pixel_xpos = floor(xpos);
        if (floor(new_xpos) > pixel_xpos) {
            if (n > 0) {
                draw_data->points[point_index].x = pixel_xpos;
                draw_data->points[point_index].ymin = ypos_min * pixel_amplitude + pixel_amplitude;
                draw_data->points[point_index].ymax = ypos_max * pixel_amplitude + pixel_amplitude;
                point_index += 1;
            }
            n = 0;
            ypos_min = 1;
            ypos_max = -1;
        }
        xpos = new_xpos;
        float sample = 0;
        int ch = scope->channels;
        for (int c = 0; c < ch; c++) {
            sample = scope->samples[i * scope->channels + c];
        }
        sample /= ch;
        if (sample > ypos_max) {
            ypos_max = sample;
        }
        if (sample < ypos_min) {
            ypos_min = sample;
        }
        n += 1;
    }
    if (n > 0 && point_index < draw_data->point_count) {
        draw_data->points[point_index].x = view_width - 1;
        draw_data->points[point_index].ymin = ypos_min * pixel_amplitude + pixel_amplitude;
        draw_data->points[point_index].ymax = ypos_max * pixel_amplitude + pixel_amplitude;
        point_index += 1;
    }
}

void
ddb_scope_draw_data_dealloc (ddb_scope_draw_data_t *draw_data) {
    free (draw_data->points);
    memset (draw_data, 0, sizeof(ddb_scope_draw_data_t));
}
