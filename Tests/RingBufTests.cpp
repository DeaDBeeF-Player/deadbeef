/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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
#include "ringbuf.h"

TEST(RingBufTests, read_nowrap_success) {
    char buffer[5];
    ringbuf_t ringbuf;
    ringbuf_init(&ringbuf, buffer, sizeof (buffer));

    ringbuf_write(&ringbuf, (char *)"hello", 5);
    char buf[5];
    size_t sz = ringbuf_read(&ringbuf, buf, 5);
    EXPECT_EQ(sz, 5);
    EXPECT_TRUE(!memcmp(buf, "hello", 5));
}

TEST(RingBufTests, read_wrap_success) {
    char buffer[10];
    ringbuf_t ringbuf;
    ringbuf_init(&ringbuf, buffer, sizeof (buffer));

    char buf[10];

    ringbuf_write(&ringbuf, (char *)"-----", 5);
    ringbuf_write(&ringbuf, (char *)"hello", 5);

    ringbuf_read(&ringbuf, buf, 5);

    ringbuf_write(&ringbuf, (char *)"world", 5);
    size_t sz = ringbuf_read(&ringbuf, buf, 10);

    EXPECT_EQ(sz, 10);
    EXPECT_TRUE(!memcmp(buf, "helloworld", 10));
}

TEST(RingBufTests, read_fromEmpty_returns0) {
    char buffer[10];
    ringbuf_t ringbuf;
    ringbuf_init(&ringbuf, buffer, sizeof (buffer));

    char buf[5];

    size_t sz = ringbuf_read(&ringbuf, buf, 5);

    EXPECT_EQ(sz, 0);
}

TEST(RingBufTests, read_tooMuchNoWrap_returnsHowMuchWasRead) {
    char buffer[10];
    ringbuf_t ringbuf;
    ringbuf_init(&ringbuf, buffer, sizeof (buffer));

    ringbuf_write(&ringbuf, (char *)"hello", 5);

    char buf[10];

    size_t sz = ringbuf_read(&ringbuf, buf, 10);

    EXPECT_EQ(sz, 5);
    EXPECT_TRUE(!memcmp(buf, "hello", 5));
}

TEST(RingBufTests, read_tooMuchWrap_returnsHowMuchWasRead) {
    char buffer[10];
    ringbuf_t ringbuf;
    ringbuf_init(&ringbuf, buffer, sizeof (buffer));

    char buf[15];

    ringbuf_write(&ringbuf, (char *)"-----", 5);
    ringbuf_read(&ringbuf, buf, 5);

    ringbuf_write(&ringbuf, (char *)"helloworld", 10);

    size_t sz = ringbuf_read(&ringbuf, buf, 15);

    EXPECT_EQ(sz, 10);
    EXPECT_TRUE(!memcmp(buf, "helloworld", 5));
}

TEST(RingBufTests, readKeep_twice_success) {
    char buffer[5];
    ringbuf_t ringbuf;
    ringbuf_init(&ringbuf, buffer, sizeof (buffer));

    ringbuf_write(&ringbuf, (char *)"hello", 5);
    char buf[5];
    char buf2[5];
    size_t sz = ringbuf_read_keep(&ringbuf, buf, 5);
    size_t sz2 = ringbuf_read_keep(&ringbuf, buf2, 5);
    EXPECT_EQ(sz, 5);
    EXPECT_EQ(sz2, 5);
    EXPECT_TRUE(!memcmp(buf, "hello", 5));
    EXPECT_TRUE(!memcmp(buf2, "hello", 5));
}
