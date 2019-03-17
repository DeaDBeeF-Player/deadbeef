//
//  CoreAudioTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 3/17/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>

static int samplerates_reg[2] = {
    44100,
    48000,
};

static int samplerates_high[4] = {
    88200,
    96000,
    176400,
    192000
};

static int samplerates_low[5] = {
    8000,
    11025,
    16000,
    22050,
    48000
};

int
get_best_samplerate (int samplerate, int *avail_samplerates, int count);

@interface CoreAudioTests : XCTestCase

@end

@implementation CoreAudioTests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)test_Input96khz_RegOutput48khz {
    int sr = get_best_samplerate(96000, samplerates_reg, 2);
    XCTAssertEqual(48000, sr);
}

- (void)test_Input882khz_RegOutput441khz {
    int sr = get_best_samplerate(88200, samplerates_reg, 2);
    XCTAssertEqual(44100, sr);
}

- (void)test_Input48khz_RegOutput48khz {
    int sr = get_best_samplerate(48000, samplerates_reg, 2);
    XCTAssertEqual(48000, sr);
}

- (void)test_Input441khz_RegOutput441khz {
    int sr = get_best_samplerate(44100, samplerates_reg, 2);
    XCTAssertEqual(44100, sr);
}

- (void)test_Input8khz_RegOutput48khz {
    int sr = get_best_samplerate(11025, samplerates_reg, 2);
    XCTAssertEqual(44100, sr);
}

- (void)test_Input96khz_HighOutput96khz {
    int sr = get_best_samplerate(96000, samplerates_high, 4);
    XCTAssertEqual(96000, sr);
}

- (void)test_Input882khz_HighOutput882khz {
    int sr = get_best_samplerate(88200, samplerates_high, 4);
    XCTAssertEqual(88200, sr);
}

- (void)test_Input48khz_HighOutput96khz {
    int sr = get_best_samplerate(48000, samplerates_high, 4);
    XCTAssertEqual(96000, sr);
}

- (void)test_Input441khz_HighOutput882khz {
    int sr = get_best_samplerate(44100, samplerates_high, 4);
    XCTAssertEqual(88200, sr);
}

- (void)test_Input1125khz_HighOutput882khz {
    int sr = get_best_samplerate(11025, samplerates_high, 4);
    XCTAssertEqual(88200, sr);
}

- (void)test_Input96khz_LowOutput48000khz {
    int sr = get_best_samplerate(96000, samplerates_low, 5);
    XCTAssertEqual(48000, sr);
}

- (void)test_Input882khz_LowOutput48khz {
    int sr = get_best_samplerate(88200, samplerates_low, 5);
    XCTAssertEqual(48000, sr);
}

- (void)test_Input48khz_LowOutput48khz {
    int sr = get_best_samplerate(48000, samplerates_low, 5);
    XCTAssertEqual(48000, sr);
}

- (void)test_Input441khz_LowOutput48khz {
    int sr = get_best_samplerate(44100, samplerates_low, 5);
    XCTAssertEqual(48000, sr);
}

- (void)test_Input8khz_LowOutput48khz {
    int sr = get_best_samplerate(8000, samplerates_low, 5);
    XCTAssertEqual(8000, sr);
}


@end
