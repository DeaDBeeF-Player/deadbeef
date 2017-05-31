//
//  ResamplerTest.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 13/04/2017.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "dsp.h"

@interface ResamplerTest : XCTestCase

@end

@implementation ResamplerTest

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testSimpleDownsamplerBufSizeFrom192To48 {
    char input[4*2*2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (192000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == sizeof (input), @"The actual output is: %d", res);
    XCTAssert(out_size == sizeof (input) / 4, @"The actual output is: %d", out_size);
}

- (void)testSimpleDownsamplerBufSizeFrom96To48 {
    char input[4*2*2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (96000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == sizeof (input), @"The actual output is: %d", res);
    XCTAssert(out_size == sizeof (input) / 2, @"The actual output is: %d", out_size);
}

- (void)testSimpleDownsamplerInvalid32To48 {
    char input[4*2*2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (32000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == sizeof (input), @"The actual output is: %d", res);
    XCTAssert(out_size == sizeof (input), @"The actual output is: %d", out_size);
}

- (void)testSimpleDownsamplerInvalid50To48 {
    char input[4*2*2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (50000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == sizeof (input), @"The actual output is: %d", res);
    XCTAssert(out_size == sizeof (input), @"The actual output is: %d", out_size);
}

- (void)testSimpleDownsampler96To48NonMultiple {
    const int samples = 5;
    char input[samples*2*2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (96000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == 4*2*2, @"The actual output is: %d", res);
    XCTAssert(out_size == 4*2, @"The actual output is: %d", out_size);
}

- (void)testSimpleDownsampler192To48NonMultiple {
    const int samples = 5;
    char input[samples*2*2];
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (192000, 2, input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == 4*2*2, @"The actual output is: %d", res);
    XCTAssert(out_size == 2*2, @"The actual output is: %d", out_size);
}

- (void)testSimpleDownsamplerOutputBufSizeFrom192To48 {
    short input[4] = {
        0, 0x7fff, 0, 0x7fff
    };
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (192000, 1, (char *)input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == 8);
    XCTAssert(out_size == 2);
    XCTAssert(((short*)out_buf)[0] == 0x3fff, @"The actual output is: %d", (int)((short*)out_buf)[0]);
}

- (void)testSimpleDownsamplerOutputBufSizeFrom96To48 {
    short input[2] = {
        0, 0x7fff
    };
    char *out_buf;
    int out_size;

    int res = dsp_apply_simple_downsampler (96000, 1, (char *)input, sizeof (input), 48000, &out_buf, &out_size);

    XCTAssert(res == 4);
    XCTAssert(out_size == 2);
    XCTAssert(((short*)out_buf)[0] == 0x3fff, @"The actual output is: %d", (int)((short*)out_buf)[0]);
}

@end
