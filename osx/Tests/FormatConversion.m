//
//  FormatConversion.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 22/01/2018.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

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
