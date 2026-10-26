/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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

#include <gtest/gtest.h>
#include <math.h>
#include <vector>
#include "../shared/analyzer/analyzer.h"

static const int kFFTSize = 8192;

static float
expected_height (const ddb_analyzer_t *analyzer, float magnitude) {
    float bound = -analyzer->db_lower_bound;
    return (20 * log10f (magnitude) + bound) / bound;
}

// With a low sample rate, the high bands clamp to the last FFT bin.
// Interpolating with the "next" bin used to read one float past the end of the FFT data buffer
// (caught by AddressSanitizer), and now must use the last bin's value.
TEST(AnalyzerTests, tick_octave_bands_last_bin_does_not_read_out_of_bounds) {
    ddb_analyzer_t analyzer = {};
    ddb_analyzer_init (&analyzer);
    analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;

    std::vector<float> fft (kFFTSize, 0.1f);
    fft[kFFTSize - 1] = 0.5f;

    ddb_analyzer_process (&analyzer, 8000, 1, fft.data (), kFFTSize);
    ddb_analyzer_tick (&analyzer);

    ASSERT_GT (analyzer.bar_count, 0);
    const ddb_analyzer_bar_t *top = &analyzer.bars[analyzer.bar_count - 1];
    EXPECT_EQ (top->bin, kFFTSize - 1);
    EXPECT_NEAR (top->height, expected_height (&analyzer, 0.5f), 0.001f);

    ddb_analyzer_dealloc (&analyzer);
}

// A bar spanning several FFT bins must take the max over the bins of the channel being processed,
// not always from the first channel.
TEST(AnalyzerTests, tick_stereo_bar_max_uses_second_channel_data) {
    ddb_analyzer_t analyzer = {};
    ddb_analyzer_init (&analyzer);
    analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
    analyzer.max_of_stereo_data = 1;

    const float quiet = 0.001f;
    const float loud = 0.5f;
    std::vector<float> fft (kFFTSize * 2, quiet);

    // first pass to generate the bars
    ddb_analyzer_process (&analyzer, 44100, 2, fft.data (), kFFTSize);
    ASSERT_EQ (analyzer.channels, 2);

    // find a bar which spans more than 2 bins
    int bar_index = -1;
    for (int i = 0; i < analyzer.bar_count; i++) {
        if (analyzer.bars[i].last_bin >= analyzer.bars[i].bin + 2) {
            bar_index = i;
            break;
        }
    }
    ASSERT_NE (bar_index, -1);
    int bin = analyzer.bars[bar_index].bin;

    // put a peak in the second channel, in the span of the bar, but not in the bins used for interpolation
    fft[kFFTSize + bin + 2] = loud;

    ddb_analyzer_process (&analyzer, 44100, 2, fft.data (), kFFTSize);
    ddb_analyzer_tick (&analyzer);

    EXPECT_NEAR (analyzer.bars[bar_index].height, expected_height (&analyzer, loud), 0.001f);

    ddb_analyzer_dealloc (&analyzer);
}
