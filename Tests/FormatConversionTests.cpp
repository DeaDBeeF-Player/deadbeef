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

#include <deadbeef/deadbeef.h>
#include "premix.h"
#include <gtest/gtest.h>

TEST (FormatConversionTests, testConvertFromStereoToBackLeftBackRight_AllSamplesDiscarded) {
    int16_t samples[4] = { 0x1000, 0x2000, 0x3000, 0x4000 };
    int16_t outsamples[4] = { 0, 0, 0, 0 };

    ddb_waveformat_t inputfmt = { .bps = 16,
                                  .channels = 2,
                                  .samplerate = 44100,
                                  .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT };

    ddb_waveformat_t outputfmt = { .bps = 16,
                                   .channels = 2,
                                   .samplerate = 44100,
                                   .channelmask = DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT };

    int res = pcm_convert (&inputfmt, (const char *)samples, &outputfmt, (char *)outsamples, sizeof (samples));
    EXPECT_TRUE (res == 8);
    EXPECT_TRUE (outsamples[0] == 0);
    EXPECT_TRUE (outsamples[1] == 0);
    EXPECT_TRUE (outsamples[2] == 0);
    EXPECT_TRUE (outsamples[3] == 0);
}

TEST (FormatConversionTests, testConvertFromStereoToBackLeftFrontRight_LeftChannelDiscarded) {
    int16_t samples[4] = { 0x1000, 0x2000, 0x3000, 0x4000 };
    int16_t outsamples[4] = { 0, 0, 0, 0 };

    ddb_waveformat_t inputfmt = { .bps = 16,
                                  .channels = 2,
                                  .samplerate = 44100,
                                  .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT };

    ddb_waveformat_t outputfmt = { .bps = 16,
                                   .channels = 2,
                                   .samplerate = 44100,
                                   .channelmask = DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_FRONT_RIGHT };

    int res = pcm_convert (&inputfmt, (const char *)samples, &outputfmt, (char *)outsamples, sizeof (samples));
    EXPECT_TRUE (res == 8);
    EXPECT_TRUE (outsamples[0] == 0);
    EXPECT_TRUE (outsamples[1] == 0x2000);
    EXPECT_TRUE (outsamples[2] == 0);
    EXPECT_TRUE (outsamples[3] == 0x4000);
}
