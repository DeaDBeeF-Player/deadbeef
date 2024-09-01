//
//  GrowableBufferTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 3/31/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#include "growableBuffer.h"
#include <gtest/gtest.h>

TEST (GrowableBufferTests, test_printf_JustEnoughSpace_Success) {
    growableBuffer_t buf;
    growableBufferInitWithSize (&buf, 5);
    growableBufferPrintf (&buf, "%s", "abcd");
    EXPECT_EQ (buf.size, 5);
    EXPECT_EQ (buf.avail, 1);
    EXPECT_TRUE (!strcmp (buf.buffer, "abcd"));
    growableBufferDealloc (&buf);
}

TEST (GrowableBufferTests, test_printf_NotEnoughSpaceFor0_Grow) {
    growableBuffer_t buf;
    growableBufferInitWithSize (&buf, 5);
    growableBufferPrintf (&buf, "%s", "abcde");
    EXPECT_EQ (buf.size, 1005);
    EXPECT_EQ (buf.avail, 1000);
    EXPECT_TRUE (!strcmp (buf.buffer, "abcde"));
    growableBufferDealloc (&buf);
}

TEST (GrowableBufferTests, test_printf_NotEnoughSpaceForString_Grow) {
    growableBuffer_t buf;
    growableBufferInitWithSize (&buf, 5);
    growableBufferPrintf (&buf, "%s", "abcdeEFGHI");
    EXPECT_EQ (buf.size, 1005);
    EXPECT_EQ (buf.avail, 995);
    EXPECT_TRUE (!strcmp (buf.buffer, "abcdeEFGHI"));
    growableBufferDealloc (&buf);
}

TEST (GrowableBufferTests, test_printf_EmptyString_NothingHappens) {
    growableBuffer_t *buf = growableBufferInitWithSize (growableBufferAlloc (), 1000);
    growableBufferInitWithSize (buf, 5);
    growableBufferPrintf (buf, "");
    EXPECT_EQ (buf->size, 5);
    EXPECT_EQ (buf->avail, 5);
    EXPECT_TRUE (!strcmp (buf->buffer, ""));
    growableBufferFree (buf);
}
