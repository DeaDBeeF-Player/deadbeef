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
#include "conf.h"
#include "junklib.h"
#include "playlist.h"
#include "plmeta.h"
#include "vfs.h"
#include <deadbeef/common.h>
#include <fstream>
#include <unistd.h>
#include <gtest/gtest.h>

extern "C" {
    ConversionResult
    ConvertUTF16BEtoUTF8 (const UTF16** sourceStart, const UTF16* sourceEnd, UTF8** targetStart, UTF8* targetEnd, ConversionFlags flags);

    ConversionResult
    ConvertUTF8toUTF16BE (const UTF8** sourceStart, const UTF8* sourceEnd, UTF16** targetStart, UTF16* targetEnd, ConversionFlags flags);

    void
    _split_multivalue (char *text, size_t text_size);

    int
    junk_utf8_to_cp1252(const uint8_t *in, int inlen, uint8_t *out, int outlen);
}

TEST(JunklibTests, testConvertUTF16BEtoUTF8) {
    const char input[] = {0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64};
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16BEtoUTF8 ((const UTF16**)&pInput, (const UTF16*)(input + inlen), (UTF8**)&pOut, (UTF8*)(output + outlen), strictConversion);

    *pOut = 0;

    EXPECT_TRUE(result == conversionOK && !strcmp (output, "АБВГДabcd"));
}

TEST(JunklibTests, testConvertUTF16toUTF8) {
    const char input[] = {0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00};
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF16toUTF8 ((const UTF16**)&pInput, (const UTF16*)(input + inlen), (UTF8**)&pOut, (UTF8*)(output + outlen), strictConversion);

    *pOut = 0;

    EXPECT_TRUE(result == conversionOK && !strcmp (output, "АБВГДabcd"));
}

TEST(JunklibTests, testConvertUTF8toUTF16) {
    const char input[] = "АБВГДabcd";
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF8toUTF16 ((const UTF8**)&pInput, (const UTF8*)(input + inlen), (UTF16**)&pOut, (UTF16*)(output + outlen), strictConversion);

    *pOut = 0;

    const char utf16be_reference[] = {0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x04, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64, 0x00};
    EXPECT_TRUE(result == conversionOK && !memcmp (output, utf16be_reference, sizeof (utf16be_reference)));
}


TEST(JunklibTests, testConvertUTF8toUTF16BE) {

    const char input[] = "АБВГДabcd";
    const char *pInput = input;
    size_t inlen = sizeof (input);
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);
    char *pOut = output;
    ConversionResult result = ConvertUTF8toUTF16BE ((const UTF8**)&pInput, (const UTF8*)(input + inlen), (UTF16**)&pOut, (UTF16*)(output + outlen), strictConversion);

    *pOut = 0;

    const char utf16be_reference[] = {0x04, 0x10, 0x04, 0x11, 0x04, 0x12, 0x04, 0x13, 0x04, 0x14, 0x00, 0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0x64};
    EXPECT_TRUE(result == conversionOK && !memcmp (output, utf16be_reference, sizeof (utf16be_reference)));
}

TEST(JunklibTests, testConvertUTF8toCP1252) {
    const char input[] = "€ù‰•abcd";
    size_t inlen = sizeof (input)-1;
    char output[1024];
    size_t outlen = sizeof (output);
    memset (output, 0, outlen);

    int res = junk_utf8_to_cp1252((const uint8_t *)input, (int)inlen, (uint8_t *)output, (int)outlen);

    const unsigned char cp1252_reference[] = { 0x80, 0xf9, 0x89, 0x95, 0x61, 0x62, 0x63, 0x64 };
    EXPECT_TRUE(res == sizeof (cp1252_reference) && !memcmp (output, cp1252_reference, sizeof (cp1252_reference)));
}

TEST(JunklibTests, testSplitMultivalue_IgnoresUnspacedSlash) {
    char input[] = "Test/Value/With/Shashes";
    _split_multivalue(input, sizeof (input)-1);

    EXPECT_TRUE(!strcmp (input, "Test/Value/With/Shashes"));
}

TEST(JunklibTests, testSplitMultivalue_SplitsOnSpacedSlash) {
    char input[] = "Test / Value / With / Shashes";
    _split_multivalue(input, sizeof (input)-1);

    EXPECT_TRUE(!strcmp (input, "Test\0\0\0Value\0\0\0With\0\0\0Shashes"));
}

TEST(JunklibTests, testSplitMultivalue_IgnoreSpacedSlashBegin) {
    char input[] = "/ Test";
    _split_multivalue(input, sizeof (input)-1);

    EXPECT_TRUE(!strcmp (input, "/ Test"));
}

TEST(JunklibTests, testSplitMultivalue_IgnoreSpacedSlashEnd) {
    char input[] = "Test /";
    _split_multivalue(input, sizeof (input)-1);

    EXPECT_TRUE(!strcmp (input, "Test /"));
}

TEST(JunklibTests, test_starsFromPopmRating_0_0) {
    unsigned stars = junk_stars_from_popm_rating (0);
    EXPECT_EQ(stars, 0);
}

TEST(JunklibTests, test_starsFromPopmRating_1_1) {
    unsigned stars = junk_stars_from_popm_rating (1);
    EXPECT_EQ(stars, 1);
}

TEST(JunklibTests, test_starsFromPopmRating_63_1) {
    unsigned stars = junk_stars_from_popm_rating (63);
    EXPECT_EQ(stars, 1);
}

TEST(JunklibTests, test_starsFromPopmRating_64_2) {
    unsigned stars = junk_stars_from_popm_rating (64);
    EXPECT_EQ(stars, 2);
}

TEST(JunklibTests, test_starsFromPopmRating_127_2) {
    unsigned stars = junk_stars_from_popm_rating (127);
    EXPECT_EQ(stars, 2);
}

TEST(JunklibTests, test_starsFromPopmRating_128_3) {
    unsigned stars = junk_stars_from_popm_rating (128);
    EXPECT_EQ(stars, 3);
}

TEST(JunklibTests, test_starsFromPopmRating_195_3) {
    unsigned stars = junk_stars_from_popm_rating (195);
    EXPECT_EQ(stars, 3);
}

TEST(JunklibTests, test_starsFromPopmRating_196_4) {
    unsigned stars = junk_stars_from_popm_rating (196);
    EXPECT_EQ(stars, 4);
}

TEST(JunklibTests, test_starsFromPopmRating_254_4) {
    unsigned stars = junk_stars_from_popm_rating (254);
    EXPECT_EQ(stars, 4);
}

TEST(JunklibTests, test_starsFromPopmRating_255_5) {
    unsigned stars = junk_stars_from_popm_rating (255);
    EXPECT_EQ(stars, 5);
}

TEST(JunklibTests, test_popmRatingFromStars_0_0) {
    uint8_t rating = junk_popm_rating_from_stars(0);
    EXPECT_EQ(rating, 0);
}

TEST(JunklibTests, test_popmRatingFromStars_1_63) {
    uint8_t rating = junk_popm_rating_from_stars(1);
    EXPECT_EQ(rating, 63);
}

TEST(JunklibTests, test_popmRatingFromStars_2_127) {
    uint8_t rating = junk_popm_rating_from_stars(2);
    EXPECT_EQ(rating, 127);
}

TEST(JunklibTests, test_popmRatingFromStars_3_195) {
    uint8_t rating = junk_popm_rating_from_stars(3);
    EXPECT_EQ(rating, 195);
}

TEST(JunklibTests, test_popmRatingFromStars_4_254) {
    uint8_t rating = junk_popm_rating_from_stars(4);
    EXPECT_EQ(rating, 254);
}

TEST(JunklibTests, test_junkMakeTdrcString_fullInput_validOutput) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, 16, 55);

    EXPECT_TRUE(!strcmp(buffer, "2022-11-07-T16:55"));
}

TEST(JunklibTests, test_junkMakeTdrcString_invalidHours_noTimeInOutput) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, -1, 55);

    EXPECT_TRUE(!strcmp(buffer, "2022-11-07"));
}

TEST(JunklibTests, test_junkMakeTdrcString_invalidMinutes_noTimeInOutput) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, 16, -1);

    EXPECT_TRUE(!strcmp(buffer, "2022-11-07"));
}

TEST(JunklibTests, test_junkMakeTdrcString_zeroMinutes_validOutputWithTime) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, 16, 00);

    EXPECT_TRUE(!strcmp(buffer, "2022-11-07-T16:00"));
}

TEST(JunklibTests, test_junkMakeTdrcString_zeroHours_validOutputWithTime) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, 00, 55);

    EXPECT_TRUE(!strcmp(buffer, "2022-11-07-T00:55"));
}

TEST(JunklibTests, test_junkMakeTdrcString_invalidDay_emptyOutput) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, -1, 16, 55);

    EXPECT_STREQ(buffer, "");
}

TEST(JunklibTests, test_junkMakeTdrcString_invalidMonth_emptyOutput) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 0, 7, 16, 55);

    EXPECT_STREQ(buffer, "");
}

TEST(JunklibTests, test_junkMakeTdrcString_invalidYear_emptyOutput) {
    char buffer[100];
    junk_make_tdrc_string(buffer, sizeof(buffer), 0, 11, 7, 16, 55);

    EXPECT_STREQ(buffer, "");
}

TEST(JunklibTests, test_junkMakeTdrcString_bufferTooSmallForTime_outputClippedAtDateBoundary) {
    char buffer[12];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, 16, 55);

    EXPECT_TRUE(!strcmp(buffer, "2022-11-07"));
}

TEST(JunklibTests, test_junkMakeTdrcString_bufferTooSmallForDate_outputEmpty) {
    char buffer[5];
    junk_make_tdrc_string(buffer, sizeof(buffer), 2022, 11, 7, 16, 55);

    EXPECT_TRUE(!strcmp(buffer, ""));
}

#define TPE2_TESTFILE "/tmp/ddb_tpe2_test.mp3"

class JunklibTPE2Tests: public ::testing::Test {
protected:
    void SetUp() override {
        conf_init ();
        conf_enable_saving (0);

        char path[PATH_MAX];
        snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);
        std::ifstream src (path, std::ios::binary);
        std::ofstream dst (TPE2_TESTFILE, std::ios::binary | std::ios::trunc);
        dst << src.rdbuf ();
        dst.close ();

        it = pl_item_alloc_init (TPE2_TESTFILE, "stdmpg");
    }

    void TearDown() override {
        pl_item_unref (it);
        unlink (TPE2_TESTFILE);
        conf_remove_items ("junk.id3v2_tpe2_album_artist");
        junk_configchanged ();
        conf_free ();
    }

    void setTPE2IsAlbumArtist (int enable) {
        conf_set_int ("junk.id3v2_tpe2_album_artist", enable);
        junk_configchanged ();
    }

    void writeTags (int id3v2_version) {
        junk_rewrite_tags (it, JUNK_WRITE_ID3V2, id3v2_version, NULL);
    }

    // Reads the test file into a new track, which must be unref'd by the caller
    playItem_t *readTags () {
        playItem_t *res = pl_item_alloc_init (TPE2_TESTFILE, "stdmpg");
        DB_FILE *fp = vfs_fopen (TPE2_TESTFILE);
        junk_id3v2_read (res, fp);
        vfs_fclose (fp);
        return res;
    }

    // Returns the value of the TPE2 frame, or of the TXXX frame with the given description, as an ASCII string
    std::string frameValue (const char *frame_id, const char *txxx_desc = nullptr) {
        std::string res;
        DB_id3v2_tag_t id3v2;
        memset (&id3v2, 0, sizeof (id3v2));
        DB_FILE *fp = vfs_fopen (TPE2_TESTFILE);
        junk_id3v2_read_full (NULL, &id3v2, fp);
        vfs_fclose (fp);
        for (DB_id3v2_frame_t *f = id3v2.frames; f; f = f->next) {
            if (strcmp (f->id, frame_id)) {
                continue;
            }
            const char *text = (const char *)f->data + 1;
            const char *end = (const char *)f->data + f->size;
            if (txxx_desc) {
                if (strcmp (text, txxx_desc)) {
                    continue;
                }
                text += strlen (text) + 1;
            }
            res.assign (text, strnlen (text, end - text));
            break;
        }
        junk_id3v2_free (&id3v2);
        return res;
    }

    playItem_t *it;
};

TEST_F(JunklibTPE2Tests, test_writeAlbumArtistAndBand_tpe2IsAlbumArtist_writesAlbumArtistToTPE2AndBandToTXXX) {
    setTPE2IsAlbumArtist (1);
    pl_add_meta (it, "album artist", "Album Artist Value");
    pl_add_meta (it, "band", "Band Value");
    writeTags (4);

    EXPECT_EQ(frameValue ("TPE2"), "Album Artist Value");
    EXPECT_EQ(frameValue ("TXXX", "BAND"), "Band Value");
    EXPECT_EQ(frameValue ("TXXX", "album artist"), "");
}

TEST_F(JunklibTPE2Tests, test_writeAlbumArtistAndBand_tpe2IsBand_writesBandToTPE2AndAlbumArtistToTXXX) {
    setTPE2IsAlbumArtist (0);
    pl_add_meta (it, "album artist", "Album Artist Value");
    pl_add_meta (it, "band", "Band Value");
    writeTags (4);

    EXPECT_EQ(frameValue ("TPE2"), "Band Value");
    EXPECT_EQ(frameValue ("TXXX", "album artist"), "Album Artist Value");
    EXPECT_EQ(frameValue ("TXXX", "BAND"), "");
}

TEST_F(JunklibTPE2Tests, test_readTPE2_tpe2IsAlbumArtist_readsAsAlbumArtist) {
    setTPE2IsAlbumArtist (0);
    pl_add_meta (it, "band", "Value");
    writeTags (3);

    setTPE2IsAlbumArtist (1);
    playItem_t *read = readTags ();
    EXPECT_STREQ(pl_find_meta (read, "album artist"), "Value");
    EXPECT_EQ(pl_find_meta (read, "band"), nullptr);
    pl_item_unref (read);
}

TEST_F(JunklibTPE2Tests, test_readTPE2_tpe2IsBand_readsAsBand) {
    setTPE2IsAlbumArtist (1);
    pl_add_meta (it, "album artist", "Value");
    writeTags (4);

    setTPE2IsAlbumArtist (0);
    playItem_t *read = readTags ();
    EXPECT_STREQ(pl_find_meta (read, "band"), "Value");
    EXPECT_EQ(pl_find_meta (read, "album artist"), nullptr);
    pl_item_unref (read);
}

TEST_F(JunklibTPE2Tests, test_readTXXXBand_tpe2IsAlbumArtist_readsAsBand) {
    setTPE2IsAlbumArtist (1);
    pl_add_meta (it, "album artist", "Album Artist Value");
    pl_add_meta (it, "band", "Band Value");
    writeTags (3);

    playItem_t *read = readTags ();
    EXPECT_STREQ(pl_find_meta (read, "album artist"), "Album Artist Value");
    EXPECT_STREQ(pl_find_meta (read, "band"), "Band Value");
    pl_item_unref (read);
}

TEST_F(JunklibTPE2Tests, test_readOldLayoutWithDifferentTXXXAlbumArtist_tpe2IsAlbumArtist_readsTPE2AsBand) {
    setTPE2IsAlbumArtist (0);
    pl_add_meta (it, "album artist", "Album Artist Value");
    pl_add_meta (it, "band", "Band Value");
    writeTags (4);

    setTPE2IsAlbumArtist (1);
    playItem_t *read = readTags ();
    DB_metaInfo_t *meta = pl_meta_for_key (read, "album artist");
    ASSERT_NE(meta, nullptr);
    EXPECT_STREQ(meta->value, "Album Artist Value");
    EXPECT_EQ(meta->valuesize, (int)strlen ("Album Artist Value") + 1);
    EXPECT_STREQ(pl_find_meta (read, "band"), "Band Value");
    pl_item_unref (read);
}

TEST_F(JunklibTPE2Tests, test_readOldLayoutWithSameTXXXAlbumArtist_tpe2IsAlbumArtist_readsSingleAlbumArtist) {
    setTPE2IsAlbumArtist (0);
    pl_add_meta (it, "album artist", "Value");
    pl_add_meta (it, "band", "Value");
    writeTags (4);

    setTPE2IsAlbumArtist (1);
    playItem_t *read = readTags ();
    DB_metaInfo_t *meta = pl_meta_for_key (read, "album artist");
    ASSERT_NE(meta, nullptr);
    EXPECT_STREQ(meta->value, "Value");
    EXPECT_EQ(meta->valuesize, (int)strlen ("Value") + 1);
    EXPECT_EQ(pl_find_meta (read, "band"), nullptr);
    pl_item_unref (read);
}

TEST_F(JunklibTPE2Tests, test_rewriteOldLayout_tpe2IsAlbumArtist_convertsToNewLayout) {
    setTPE2IsAlbumArtist (0);
    pl_add_meta (it, "album artist", "Album Artist Value");
    pl_add_meta (it, "band", "Band Value");
    writeTags (4);

    setTPE2IsAlbumArtist (1);
    pl_item_unref (it);
    it = readTags ();
    writeTags (4);

    EXPECT_EQ(frameValue ("TPE2"), "Album Artist Value");
    EXPECT_EQ(frameValue ("TXXX", "BAND"), "Band Value");
    EXPECT_EQ(frameValue ("TXXX", "album artist"), "");
}
