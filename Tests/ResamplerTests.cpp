/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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

#include "dsp.h"
#include <gtest/gtest.h>

TEST (ResamplerTest, testSimpleDownsamplerBufSizeFrom192To48) {
    char input[4 * 2 * 2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (192000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == sizeof (input));
    EXPECT_TRUE (out_size == sizeof (input) / 4);
}

TEST (ResamplerTest, testSimpleDownsamplerBufSizeFrom96To48) {
    char input[4 * 2 * 2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (96000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == sizeof (input));
    EXPECT_TRUE (out_size == sizeof (input) / 2);
}

TEST (ResamplerTest, testSimpleDownsamplerInvalid32To48) {
    char input[4 * 2 * 2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (32000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == sizeof (input));
    EXPECT_TRUE (out_size == sizeof (input));
}

TEST (ResamplerTest, testSimpleDownsamplerInvalid50To48) {
    char input[4 * 2 * 2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (50000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == sizeof (input));
    EXPECT_TRUE (out_size == sizeof (input));
}

TEST (ResamplerTest, testSimpleDownsampler96To48NonMultiple) {
    const int samples = 5;
    char input[samples * 2 * 2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (96000, 2, input, (int)sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == 4 * 2 * 2);
    EXPECT_TRUE (out_size == 4 * 2);
}

TEST (ResamplerTest, testSimpleDownsampler192To48NonMultiple) {
    const int samples = 5;
    char input[samples * 2 * 2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (192000, 2, input, (int)sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == 4 * 2 * 2);
    EXPECT_TRUE (out_size == 2 * 2);
}

TEST (ResamplerTest, testSimpleDownsamplerOutputBufSizeFrom192To48) {
    short input[4] = { 0, 0x7fff, 0, 0x7fff };
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (192000, 1, (char *)input, sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == 8);
    EXPECT_TRUE (out_size == 2);
    EXPECT_TRUE (((short *)out_buf)[0] == 0x3fff);
}

TEST (ResamplerTest, testSimpleDownsamplerOutputBufSizeFrom96To48) {
    short input[2] = { 0, 0x7fff };
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (96000, 1, (char *)input, sizeof (input), 48000, &out_buf, &out_size);

    EXPECT_TRUE (res == 4);
    EXPECT_TRUE (out_size == 2);
    EXPECT_TRUE (((short *)out_buf)[0] == 0x3fff);
}
