/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Alexey Yakovenko and other contributors

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

#import <XCTest/XCTest.h>
#include "deadbeef.h"
#include "premix.h"

@interface FormatConversion : XCTestCase

@end

@implementation FormatConversion

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testConvertFromStereoToBackLeftBackRight_AllSamplesDiscarded {
    int16_t samples[4] = { 0x1000, 0x2000, 0x3000, 0x4000 };
    int16_t outsamples[4] = { 0, 0, 0, 0 };

    ddb_waveformat_t inputfmt = {
        .bps = 16,
        .channels = 2,
        .samplerate = 44100,
        .channelmask = DDB_SPEAKER_FRONT_LEFT|DDB_SPEAKER_FRONT_RIGHT
    };

    ddb_waveformat_t outputfmt = {
        .bps = 16,
        .channels = 2,
        .samplerate = 44100,
        .channelmask = DDB_SPEAKER_BACK_LEFT|DDB_SPEAKER_BACK_RIGHT
    };

    int res = pcm_convert (&inputfmt, samples, &outputfmt, outsamples, sizeof (samples));
    XCTAssert(res == 8, @"The result is %d", res);
    XCTAssert(outsamples[0] == 0, @"sample0 is %d", outsamples[0]);
    XCTAssert(outsamples[1] == 0, @"sample1 is %d", outsamples[1]);
    XCTAssert(outsamples[2] == 0, @"sample2 is %d", outsamples[2]);
    XCTAssert(outsamples[3] == 0, @"sample3 is %d", outsamples[3]);
}

- (void)testConvertFromStereoToBackLeftFrontRight_LeftChannelDiscarded {
    int16_t samples[4] = { 0x1000, 0x2000, 0x3000, 0x4000 };
    int16_t outsamples[4] = { 0, 0, 0, 0 };

    ddb_waveformat_t inputfmt = {
        .bps = 16,
        .channels = 2,
        .samplerate = 44100,
        .channelmask = DDB_SPEAKER_FRONT_LEFT|DDB_SPEAKER_FRONT_RIGHT
    };

    ddb_waveformat_t outputfmt = {
        .bps = 16,
        .channels = 2,
        .samplerate = 44100,
        .channelmask = DDB_SPEAKER_BACK_LEFT|DDB_SPEAKER_FRONT_RIGHT
    };

    int res = pcm_convert (&inputfmt, samples, &outputfmt, outsamples, sizeof (samples));
    XCTAssert(res == 8, @"The result is %d", res);
    XCTAssert(outsamples[0] == 0, @"sample0 is %d", outsamples[0]);
    XCTAssert(outsamples[1] == 0x2000, @"sample1 is %d", outsamples[1]);
    XCTAssert(outsamples[2] == 0, @"sample2 is %d", outsamples[2]);
    XCTAssert(outsamples[3] == 0x4000, @"sample3 is %d", outsamples[3]);
}

@end
