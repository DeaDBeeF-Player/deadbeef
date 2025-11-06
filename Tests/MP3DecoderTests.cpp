//
//  MP3Decoder.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 03/07/2018.
//  Copyright © 2018 Oleksiy Yakovenko. All rights reserved.
//

#include "conf.h"
#include "logger.h"
#include <deadbeef/deadbeef.h>
#include <deadbeef/common.h>
#include "playlist.h"
#include <gtest/gtest.h>

extern DB_functions_t *deadbeef;

class MP3DecoderTests: public ::testing::Test {
protected:
    void SetUp() override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
    }
    void TearDown() override {
        conf_free();
        ddb_logger_free();
    }
};

static void runMp3TwoPieceTestWithBackend(int backend) {
    deadbeef->conf_set_int ("mp3.backend", backend);

    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    playlist_t *plt = plt_alloc ("testplt");

    DB_playItem_t *it = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, path, NULL, NULL, NULL);


    DB_decoder_t *dec = (DB_decoder_t *)deadbeef->plug_get_for_id ("stdmpg");

    DB_fileinfo_t *fi = dec->open (0);
    dec->init (fi, it);

    const int64_t size = 44100*2*sizeof(uint16_t);
    unsigned char *buffer1 = (unsigned char *)calloc (1, size);
    unsigned char *buffer2 = (unsigned char *)calloc (1, size);

    // Read whole 1 second
    int res1 = dec->read (fi, (char *)buffer1, (int)size);
    EXPECT_EQ(res1, size);

    // read first part
    dec->seek_sample (fi, 0);

    int res2 = dec->read (fi, (char *)buffer2, (int)size/2);
    EXPECT_EQ(res2, size/2);

    dec->seek_sample (fi, 44100/2);
    int res3 = dec->read (fi, (char *)(buffer2 + size/2), (int)size/2);
    EXPECT_EQ(res3, size/2);

#if 0
    FILE *fp1 = fopen ("/Users/waker/buffer1.raw", "w+b");
    FILE *fp2 = fopen ("/Users/waker/buffer2.raw", "w+b");
    fwrite (buffer1, size, 1, fp1);
    fwrite (buffer2, size, 1, fp2);
    fclose (fp1);
    fclose (fp2);
#endif

    int cmp_half1 = memcmp (buffer1, buffer2, size/2);
    EXPECT_EQ(cmp_half1, 0);

    int cmp2_half2 = memcmp (buffer1+size/2, buffer2+size/2, size/2);
    EXPECT_EQ(cmp2_half2, 0);

    int cmp_whole = memcmp (buffer1, buffer2, size);
    EXPECT_EQ(cmp_whole, 0);

    free (buffer1);
    free (buffer2);

    dec->free (fi);

    deadbeef->plt_unref ((ddb_playlist_t *)plt);

}

#if !__aarch64__
// #3107: This test is failing on arm64.
// Proper investigation has been conducted,
// mpg123 doesn't produce the same data on ARM64 when reading in 2 pieces vs 1 piece,
// neither with OPT_NEON64, nor OPT_GENERIC modes.
// Also the whole signal output differs between OPT_NEON64 and OPT_AVX modes.
// Even comparing weakly as a signal with large tolerance doesn't give meaningful result,
// so the test has to be disabled.
TEST_F(MP3DecoderTests, test_DecodeMP3As2PiecesMPG123_SameAs1Piece) {
    runMp3TwoPieceTestWithBackend(0); // mpg123
}
#endif

TEST_F(MP3DecoderTests, test_DecodeMP3As2PiecesLibMAD_SameAs1Piece) {
    runMp3TwoPieceTestWithBackend(1); // libmad
}

TEST_F(MP3DecoderTests, test_FiniteVBRNetworkStream_DecodesFullAmountOfSamples) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/vbr_rhytm_30sec.mp3", dbplugindir);

    deadbeef->conf_set_int ("mp3.backend", 0);

    playlist_t *plt = plt_alloc ("testplt");

    DB_playItem_t *it = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, path, NULL, NULL, NULL);

    DB_decoder_t *dec = (DB_decoder_t *)deadbeef->plug_get_for_id ("stdmpg");

    DB_fileinfo_t *fi = dec->open (1<<31); // indicate with a flag to decode in streaming mode (internal flag)
    dec->init (fi, it);

    size_t size = 1024751 * 4;
    char *buffer = (char *)malloc (size*2);

    int res = dec->read (fi, buffer, (int)size*2);

    free (buffer);

    dec->free (fi);

    deadbeef->plt_unref ((ddb_playlist_t *)plt);

    EXPECT_EQ(res/4, size/4);
}
