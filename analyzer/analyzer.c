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
#include "analyzer.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define OCTAVES 11
#define STEPS 24
#define ROOT24 1.0293022366 // pow(2, 1.0 / STEPS)
#define C0 16.3515978313 // 440 * pow(ROOT24, -114);

#pragma mark - Forward declarations

static void
_generate_freqency_bars (ddb_analyzer_t *analyzer);

static void
_tempered_scale_bands_precalc (ddb_analyzer_t *analyzer);

static float _bin_for_freq_floor (ddb_analyzer_t *analyzer, float freq);

static float _bin_for_freq_round (ddb_analyzer_t *analyzer, float freq);

static float _freq_for_bin (ddb_analyzer_t *analyzer, int bin);

static float
_interpolate_bin_with_ratio (ddb_analyzer_t *analyzer, int bin, float ratio);

static float
_get_bar_height (ddb_analyzer_t *analyzer, float normalized_height, int view_height);

#pragma mark - Public

ddb_analyzer_t *
ddb_analyzer_alloc (void) {
    return calloc(1, sizeof (ddb_analyzer_t));
}

ddb_analyzer_t *
ddb_analyzer_init (ddb_analyzer_t *analyzer) {
    analyzer->mode = DDB_ANALYZER_MODE_FREQUENCIES;
    analyzer->min_freq = 50;
    analyzer->max_freq = 22000;
    analyzer->view_width = 1000;
    analyzer->peak_hold = 10;
    analyzer->db_lower_bound = -80;
    return analyzer;
}

void
ddb_analyzer_dealloc (ddb_analyzer_t *analyzer) {
    free (analyzer->tempered_scale_bands);
    free (analyzer->fft_data);
    memset (analyzer, 0, sizeof (ddb_analyzer_t));
}

void
ddb_analyzer_free (ddb_analyzer_t *analyzer) {
    free (analyzer);
}

void
ddb_analyzer_process (ddb_analyzer_t *analyzer, int samplerate, int channels, const float *fft_data, int fft_size) {
    if (channels != analyzer->channels || fft_size != analyzer->fft_size) {
        analyzer->channels = channels;
        analyzer->fft_size = fft_size;
        free (analyzer->fft_data);
        analyzer->fft_data = malloc (fft_size * channels * sizeof (float));
    }

    memcpy (analyzer->fft_data, fft_data, fft_size * channels * sizeof (float));
    analyzer->samplerate = samplerate;

//    _tempered_scale_bands_precalc (analyzer);

    switch (analyzer->mode) {
    case DDB_ANALYZER_MODE_FREQUENCIES:
        _generate_freqency_bars (analyzer);
        break;
    default:
        break;
    }
}

/// Update bars and peaks for the next frame
void
ddb_analyzer_tick (ddb_analyzer_t *analyzer) {
    // frequency lines
    ddb_analyzer_bar_t *bar = analyzer->bars;
    for (int i = 0; i < analyzer->bar_count; i++, bar++) {
        float l = _interpolate_bin_with_ratio (analyzer, bar->bl, bar->rl);
        float h = _interpolate_bin_with_ratio (analyzer, bar->bh, bar->rh);
        float norm_h = l > h ? l : h;
        bar->height = norm_h;
    }

    // peaks
    bar = analyzer->bars;
    for (int i = 0; i < analyzer->bar_count; i++, bar++) {
        if (bar->peak < bar->height) {
            bar->peak = bar->height;
            bar->peak_speed = analyzer->peak_hold;
        }

        if (bar->peak_speed-- < 0) {
            bar->peak += bar->peak_speed / 1000.f;
            if (bar->peak < bar->height) {
                bar->peak = bar->height;
            }
        }
    }
}

void
ddb_analyzer_get_draw_data (ddb_analyzer_t *analyzer, int view_width, int view_height, ddb_analyzer_draw_data_t *draw_data) {
    if (draw_data->bar_count != analyzer->bar_count) {
        free (draw_data->bars);
        draw_data->bars = calloc (analyzer->bar_count, sizeof (ddb_analuzer_draw_bar_t));
        draw_data->bar_count = analyzer->bar_count;
    }

    draw_data->bar_width = 1;

    // frequency lines
    ddb_analyzer_bar_t *bar = analyzer->bars;
    ddb_analuzer_draw_bar_t *draw_bar = draw_data->bars;
    for (int i = 0; i < analyzer->bar_count; i++, bar++, draw_bar++) {
        float l = _interpolate_bin_with_ratio(analyzer, bar->bl, bar->rl);
        float h = _interpolate_bin_with_ratio(analyzer, bar->bh, bar->rh);
        float norm_h = l > h ? l : h;

        // FIXME: channel
        for ( int j = bar->bl + 1; j < bar->bh; j++ ) {
            if ( analyzer->fft_data[j] > norm_h )
                norm_h = analyzer->fft_data[j];
        }

        draw_bar->bar_height = _get_bar_height (analyzer, norm_h, view_height);
        draw_bar->xpos = bar->xpos * view_width;
        draw_bar->peak_ypos = _get_bar_height (analyzer, bar->peak, view_height);
    }
}

void
ddb_analyzer_draw_data_dealloc (ddb_analyzer_draw_data_t *draw_data) {
    free (draw_data->bars);
    memset (draw_data, 0, sizeof(ddb_analyzer_draw_data_t));
}

#pragma mark - Private

static float
_get_bar_height (ddb_analyzer_t *analyzer, float normalized_height, int view_height) {
    float bound = -analyzer->db_lower_bound;
    float height = (20*log10(normalized_height) + bound)/bound;

    if (height < 0) {
        height = 0;
    }
    else if (height > 1) {
        height = 1;
    }
    height *= view_height;
    return height;
}

static void
_generate_freqency_bars (ddb_analyzer_t *analyzer) {
    float min_freq_log = log10 (analyzer->min_freq);
    float max_freq_log = log10 (analyzer->max_freq);
    float view_width = analyzer->view_width;
    float width_log = view_width / (max_freq_log - min_freq_log);

    float minIndex = _bin_for_freq_floor (analyzer, analyzer->min_freq);
    float maxIndex = _bin_for_freq_round (analyzer, analyzer->max_freq);

    int prev = -1;

    analyzer->bar_count = 0;

    if (analyzer->bars == NULL) {
        analyzer->bars = calloc (sizeof (ddb_analyzer_bar_t), view_width);
    }

    for (int i = minIndex; i <= maxIndex; i++) {
        float freq = _freq_for_bin (analyzer, i);
        int pos = width_log * (log10(freq) - min_freq_log);

        if (pos > prev && pos >= 0) {
            // start accumulating frequencies for the new band
            ddb_analyzer_bar_t *bar = analyzer->bars + analyzer->bar_count;

            bar->xpos = pos / view_width; // normalized position
            bar->bl = i;
            bar->bh = i;
            bar->fl = freq;
            bar->fh = freq;
            bar->rl = 0;
            bar->rh = 0;
            analyzer->bar_count += 1;

            prev = pos;
        }
        else if (analyzer->bar_count > 0) {
            // add to the current band
            analyzer->bars[analyzer->bar_count-1].bh = i;
            analyzer->bars[analyzer->bar_count-1].fh = freq;
        }
    }
}

static float _bin_for_freq_floor (ddb_analyzer_t *analyzer, float freq) {
    float max = analyzer->fft_size - 1;
    float bin = floor (freq * analyzer->fft_size / analyzer->samplerate);
    return bin < max ? bin : max;
}

static float _bin_for_freq_round (ddb_analyzer_t *analyzer, float freq) {
    float max = analyzer->fft_size - 1;
    float bin = round (freq * analyzer->fft_size / analyzer->samplerate);
    return bin < max ? bin : max;
}

static float _freq_for_bin (ddb_analyzer_t *analyzer, int bin) {
    return (int64_t)bin * analyzer->samplerate / analyzer->fft_size;
}

// Precalculate data for tempered scale
static void
_tempered_scale_bands_precalc (ddb_analyzer_t *analyzer) {
    if (analyzer->tempered_scale_bands != NULL) {
        return;
    }

    analyzer->tempered_scale_bands = calloc (OCTAVES * STEPS, sizeof (ddb_analyzer_band_t));

    for (int i = 0; i < OCTAVES * STEPS; i++) {
        float f = C0 * pow(ROOT24, i);
        float bin = _bin_for_freq_floor (analyzer, f);
        float binf  = _freq_for_bin (analyzer, bin);
        float fn = _freq_for_bin (analyzer, bin+1);
        float ratio = (f - binf) / (fn - binf);

        analyzer->tempered_scale_bands[i].bin = bin;
        analyzer->tempered_scale_bands[i].freq = f;
        analyzer->tempered_scale_bands[i].ratio = ratio;
    }
}

static float
_interpolate_bin_with_ratio (ddb_analyzer_t *analyzer, int bin, float ratio) {
    // FIXME: channel
    return analyzer->fft_data[bin] + (analyzer->fft_data[bin + 1] - analyzer->fft_data[bin]) * ratio;
}
