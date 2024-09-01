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

#include "ConvertUTF.h"
#include "junklib.h"
#include <gtest/gtest.h>

extern "C" {
ConversionResult
ConvertUTF16BEtoUTF8 (
    const UTF16 **sourceStart,
    const UTF16 *sourceEnd,
    UTF8 **targetStart,
    UTF8 *targetEnd,
    ConversionFlags flags);

ConversionResult
ConvertUTF8toUTF16BE (
    const UTF8 **sourceStart,
    const UTF8 *sourceEnd,
    UTF16 **targetStart,
    UTF16 *targetEnd,
    ConversionFlags flags);

void
_split_multivalue (char *text, size_t text_size);

int
junk_utf8_to_cp1252 (const uint8_t *in, int inlen, uint8_t *out, int outlen);
}

TEST (JunklibTests, testConvertUTF16BEtoUTF8) {
    const char input[] = { 0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04,
                           0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64 };
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16BEtoUTF8 (
        (const UTF16 **)&pInput,
        (const UTF16 *)(input + inlen),
        (UTF8 **)&pOut,
        (UTF8 *)(output + outlen),
        strictConversion);

    *pOut = 0;

    EXPECT_TRUE (result == conversionOK && !strcmp (output, "АБВГДabcd"));
}

TEST (JunklibTests, testConvertUTF16toUTF8) {
    const char input[] = { 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14,
                           0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00 };
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16toUTF8 (
        (const UTF16 **)&pInput,
        (const UTF16 *)(input + inlen),
        (UTF8 **)&pOut,
        (UTF8 *)(output + outlen),
        strictConversion);

    *pOut = 0;

    EXPECT_TRUE (result == conversionOK && !strcmp (output, "АБВГДabcd"));
}

TEST (JunklibTests, testConvertUTF8toUTF16) {
    const char input[] = "АБВГДabcd";
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF8toUTF16 (
        (const UTF8 **)&pInput,
        (const UTF8 *)(input + inlen),
        (UTF16 **)&pOut,
        (UTF16 *)(output + outlen),
        strictConversion);

    *pOut = 0;

    const char utf16be_reference[] = { 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14,
                                       0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00 };
    EXPECT_TRUE (result == conversionOK && !memcmp (output, utf16be_reference, sizeof (utf16be_reference)));
}

TEST (JunklibTests, testConvertUTF8toUTF16BE) {

    const char input[] = "АБВГДabcd";
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF8toUTF16BE (
        (const UTF8 **)&pInput,
        (const UTF8 *)(input + inlen),
        (UTF16 **)&pOut,
        (UTF16 *)(output + outlen),
        strictConversion);

    *pOut = 0;

    const char utf16be_reference[] = { 0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04,
                                       0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64 };
    EXPECT_TRUE (result == conversionOK && !memcmp (output, utf16be_reference, sizeof (utf16be_reference)));
}

TEST (JunklibTests, testConvertUTF8toCP1252) {
    const char input[] = "€ù‰•abcd";
    size_t inlen = sizeof (input) - 1;
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);

    int res = junk_utf8_to_cp1252 ((const uint8_t *)input, (int)inlen, (uint8_t *)output, (int)outlen);

    const unsigned char cp1252_reference[] = { 0x80, 0xf9, 0x89, 0x95, 0x61, 0x62, 0x63, 0x64 };
    EXPECT_TRUE (res == sizeof (cp1252_reference) && !memcmp (output, cp1252_reference, sizeof (cp1252_reference)));
}

TEST (JunklibTests, testSplitMultivalue_IgnoresUnspacedSlash) {
    char input[] = "Test/Value/With/Shashes";
    _split_multivalue (input, sizeof (input) - 1);

    EXPECT_TRUE (!strcmp (input, "Test/Value/With/Shashes"));
}

TEST (JunklibTests, testSplitMultivalue_SplitsOnSpacedSlash) {
    char input[] = "Test / Value / With / Shashes";
    _split_multivalue (input, sizeof (input) - 1);

    EXPECT_TRUE (!strcmp (input, "Test\0\0\0Value\0\0\0With\0\0\0Shashes"));
}

TEST (JunklibTests, testSplitMultivalue_IgnoreSpacedSlashBegin) {
    char input[] = "/ Test";
    _split_multivalue (input, sizeof (input) - 1);

    EXPECT_TRUE (!strcmp (input, "/ Test"));
}

TEST (JunklibTests, testSplitMultivalue_IgnoreSpacedSlashEnd) {
    char input[] = "Test /";
    _split_multivalue (input, sizeof (input) - 1);

    EXPECT_TRUE (!strcmp (input, "Test /"));
}

TEST (JunklibTests, test_starsFromPopmRating_0_0) {
    unsigned stars = junk_stars_from_popm_rating (0);
    EXPECT_EQ (stars, 0);
}

TEST (JunklibTests, test_starsFromPopmRating_1_1) {
    unsigned stars = junk_stars_from_popm_rating (1);
    EXPECT_EQ (stars, 1);
}

TEST (JunklibTests, test_starsFromPopmRating_63_1) {
    unsigned stars = junk_stars_from_popm_rating (63);
    EXPECT_EQ (stars, 1);
}

TEST (JunklibTests, test_starsFromPopmRating_64_2) {
    unsigned stars = junk_stars_from_popm_rating (64);
    EXPECT_EQ (stars, 2);
}

TEST (JunklibTests, test_starsFromPopmRating_127_2) {
    unsigned stars = junk_stars_from_popm_rating (127);
    EXPECT_EQ (stars, 2);
}

TEST (JunklibTests, test_starsFromPopmRating_128_3) {
    unsigned stars = junk_stars_from_popm_rating (128);
    EXPECT_EQ (stars, 3);
}

TEST (JunklibTests, test_starsFromPopmRating_195_3) {
    unsigned stars = junk_stars_from_popm_rating (195);
    EXPECT_EQ (stars, 3);
}

TEST (JunklibTests, test_starsFromPopmRating_196_4) {
    unsigned stars = junk_stars_from_popm_rating (196);
    EXPECT_EQ (stars, 4);
}

TEST (JunklibTests, test_starsFromPopmRating_254_4) {
    unsigned stars = junk_stars_from_popm_rating (254);
    EXPECT_EQ (stars, 4);
}

TEST (JunklibTests, test_starsFromPopmRating_255_5) {
    unsigned stars = junk_stars_from_popm_rating (255);
    EXPECT_EQ (stars, 5);
}

TEST (JunklibTests, test_popmRatingFromStars_0_0) {
    uint8_t rating = junk_popm_rating_from_stars (0);
    EXPECT_EQ (rating, 0);
}

TEST (JunklibTests, test_popmRatingFromStars_1_63) {
    uint8_t rating = junk_popm_rating_from_stars (1);
    EXPECT_EQ (rating, 63);
}

TEST (JunklibTests, test_popmRatingFromStars_2_127) {
    uint8_t rating = junk_popm_rating_from_stars (2);
    EXPECT_EQ (rating, 127);
}

TEST (JunklibTests, test_popmRatingFromStars_3_195) {
    uint8_t rating = junk_popm_rating_from_stars (3);
    EXPECT_EQ (rating, 195);
}

TEST (JunklibTests, test_popmRatingFromStars_4_254) {
    uint8_t rating = junk_popm_rating_from_stars (4);
    EXPECT_EQ (rating, 254);
}

TEST (JunklibTests, test_junkMakeTdrcString_fullInput_validOutput) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, 16, 55);

    EXPECT_TRUE (!strcmp (buffer, "2022-11-07-T16:55"));
}

TEST (JunklibTests, test_junkMakeTdrcString_invalidHours_noTimeInOutput) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, -1, 55);

    EXPECT_TRUE (!strcmp (buffer, "2022-11-07"));
}

TEST (JunklibTests, test_junkMakeTdrcString_invalidMinutes_noTimeInOutput) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, 16, -1);

    EXPECT_TRUE (!strcmp (buffer, "2022-11-07"));
}

TEST (JunklibTests, test_junkMakeTdrcString_zeroMinutes_validOutputWithTime) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, 16, 00);

    EXPECT_TRUE (!strcmp (buffer, "2022-11-07-T16:00"));
}

TEST (JunklibTests, test_junkMakeTdrcString_zeroHours_validOutputWithTime) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, 00, 55);

    EXPECT_TRUE (!strcmp (buffer, "2022-11-07-T00:55"));
}

TEST (JunklibTests, test_junkMakeTdrcString_invalidDay_emptyOutput) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, -1, 16, 55);

    EXPECT_STREQ (buffer, "");
}

TEST (JunklibTests, test_junkMakeTdrcString_invalidMonth_emptyOutput) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 0, 7, 16, 55);

    EXPECT_STREQ (buffer, "");
}

TEST (JunklibTests, test_junkMakeTdrcString_invalidYear_emptyOutput) {
    char buffer[100];
    junk_make_tdrc_string (buffer, sizeof (buffer), 0, 11, 7, 16, 55);

    EXPECT_STREQ (buffer, "");
}

TEST (JunklibTests, test_junkMakeTdrcString_bufferTooSmallForTime_outputClippedAtDateBoundary) {
    char buffer[12];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, 16, 55);

    EXPECT_TRUE (!strcmp (buffer, "2022-11-07"));
}

TEST (JunklibTests, test_junkMakeTdrcString_bufferTooSmallForDate_outputEmpty) {
    char buffer[5];
    junk_make_tdrc_string (buffer, sizeof (buffer), 2022, 11, 7, 16, 55);

    EXPECT_TRUE (!strcmp (buffer, ""));
}
