//
//  GrowableBufferTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 3/31/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "growableBuffer.h"

@interface GrowableBufferTests : XCTestCase

@end

@implementation GrowableBufferTests

- (void)test_printf_JustEnoughSpace_Success {
    growableBuffer_t buf;
    growableBufferInitWithSize(&buf, 5);
    growableBufferPrintf(&buf, "%s", "abcd");
    XCTAssertEqual(buf.size, 5);
    XCTAssertEqual(buf.avail, 1);
    XCTAssertTrue(!strcmp(buf.buffer, "abcd"));
    growableBufferDealloc(&buf);
}

- (void)test_printf_NotEnoughSpaceFor0_Grow {
    growableBuffer_t buf;
    growableBufferInitWithSize(&buf, 5);
    growableBufferPrintf(&buf, "%s", "abcde");
    XCTAssertEqual(buf.size, 1005);
    XCTAssertEqual(buf.avail, 1000);
    XCTAssertTrue(!strcmp(buf.buffer, "abcde"));
    growableBufferDealloc(&buf);
}

- (void)test_printf_NotEnoughSpaceForString_Grow {
    growableBuffer_t buf;
    growableBufferInitWithSize(&buf, 5);
    growableBufferPrintf(&buf, "%s", "abcdeEFGHI");
    XCTAssertEqual(buf.size, 1005);
    XCTAssertEqual(buf.avail, 995);
    XCTAssertTrue(!strcmp(buf.buffer, "abcdeEFGHI"));
    growableBufferDealloc(&buf);
}

- (void)test_printf_EmptyString_NothingHappens {
    growableBuffer_t *buf = growableBufferInitWithSize (growableBufferAlloc(), 1000);
    growableBufferInitWithSize(buf, 5);
    growableBufferPrintf(buf, "");
    XCTAssertEqual(buf->size, 5);
    XCTAssertEqual(buf->avail, 5);
    XCTAssertTrue(!strcmp(buf->buffer, ""));
    growableBufferFree(buf);
}


@end
