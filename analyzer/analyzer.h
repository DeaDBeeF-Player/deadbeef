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
#ifndef analyzer_h
#define analyzer_h

typedef struct {
    float freq;
    float ratio;
    int bin;
} ddb_analyzer_band_t;

typedef struct {
    // interpolation data
    int bl, bh;
    float fl, fh;
    float rl, rh;

    // normalized position
    float xpos;
    float height;
    float peak;
    float peak_speed;
} ddb_analyzer_bar_t;

typedef struct {
    float xpos;
    float peak_ypos;
    float bar_height;
} ddb_analuzer_draw_bar_t;

typedef struct {
    int bar_count;
    ddb_analuzer_draw_bar_t *bars;
    int bar_width;
} ddb_analyzer_draw_data_t;

typedef enum {
    DDB_ANALYZER_MODE_FREQUENCIES,
    DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS,
}  ddb_analyzer_mode_t;

typedef struct ddb_analyzer_s {
    // Settings

    float min_freq;
    float max_freq;
    ddb_analyzer_mode_t mode;

    /// Can be either a real or virtual view width.
    /// The calculated values are normalized,
    /// and are going to be converted to the real view size at the draw time.
    int view_width;
    float peak_hold; // how many frames to hold the peak in place
    float peak_speed_scale;
    float db_upper_bound; // dB value corresponding to the top of the view
    float db_lower_bound; // dB value corresponding to the bottom of the view

    /// The bars get created / updated by calling @c ddb_analyzer_process.
    /// The same bars get updated on every call to @c ddb_analyzer_tick.
    ddb_analyzer_bar_t *bars;
    int bar_count;
    int bar_count_max;
    int samplerate;
    int channels;
    int fft_size;
    float *fft_data;

    /// Tempered scale data, precalculated from fft pins
    ddb_analyzer_band_t *tempered_scale_bands;

    // settings from previous process

} ddb_analyzer_t;

ddb_analyzer_t *ddb_analyzer_alloc (void);

ddb_analyzer_t *ddb_analyzer_init (ddb_analyzer_t *analyzer);

void ddb_analyzer_dealloc (ddb_analyzer_t *analyzer);

void ddb_analyzer_free (ddb_analyzer_t *analyzer);

/// Process fft data into bars and peaks
void ddb_analyzer_process (ddb_analyzer_t *analyzer, int samplerate, int channels, const float *fft_data, int fft_size);

/// Update bars and peaks for the next frame
void ddb_analyzer_tick (ddb_analyzer_t *analyzer);

void ddb_analyzer_get_draw_data (ddb_analyzer_t *analyzer, int view_width, int view_height, ddb_analyzer_draw_data_t *draw_data);

void ddb_analyzer_draw_data_dealloc (ddb_analyzer_draw_data_t *draw_data);

#endif /* analyzer_h */
