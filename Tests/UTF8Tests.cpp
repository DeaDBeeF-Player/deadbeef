/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

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
#include "../src/utf8.h"

TEST(UTF8Tests, test_u8_strnbcpy_size_inputWithTrailingGarbage_successful) {
    const unsigned char input[4] = { '0', '1', 0xbe, 0xbe };
    char output[8] = { 0, };
    int len = u8_strnbcpy_size(output, (const char *)input, 2, 2);
    EXPECT_EQ(len, 2);
    int cmp = memcmp(input, output, 2);
    EXPECT_EQ(cmp, 0);
}
