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

#import <Foundation/Foundation.h>
#include "conf.h"
#include "logger.h"
#include "plmeta.h"
#include "junklib.h"
#include "vfs.h"
#include <deadbeef/common.h>
#include "tf.h"
#include <gtest/gtest.h>

#define EXPECT_EQ_WITH_ACCURACY(a,b,accuracy) EXPECT_LT(std::abs(a-b), accuracy)

#define TESTFILE "/tmp/ddb_test.mp3"

static void
copy_file(const char *from, const char *to) {
    // FIXME: make portable
    [NSFileManager.defaultManager copyItemAtPath:@(from) toPath:@(to) error:nil];
}

class TaggingTests: public ::testing::Test {
protected:
    void SetUp() override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
        it = pl_item_alloc_init (TESTFILE, "stdmpg");
    }
    void TearDown() override {
        pl_item_unref (it);
        conf_free();
        ddb_logger_free();
    }
    playItem_t *it;
};

TEST_F(TaggingTests, test_loadTestfileTags_DoesntCrash) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);
}

TEST_F(TaggingTests, test_ReadID3v24MultiValueTPE1_ReadsAs3Values) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multivalue_id3v2.4.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Value1"));

    const char refdata[] = "Value1\0Value2\0Value3\0";
    EXPECT_TRUE(sizeof (refdata)-1 == meta->valuesize && !memcmp (meta->value, refdata, meta->valuesize));
}

TEST_F(TaggingTests, test_ReadID3v23MultiValueTPE1_ReadsAs3Values) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multivalue_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Value1"));

    const char refdata[] = "Value1\0Value2\0Value3\0";
    EXPECT_TRUE(sizeof (refdata)-1 == meta->valuesize && !memcmp (meta->value, refdata, meta->valuesize));
}

TEST_F(TaggingTests, test_ReadID3v2MultiLineTPE1_ReadsAs1Value) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multiline.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Line1\r\nLine2\r\nLine3"));
    EXPECT_TRUE(strlen (meta->value) + 1 == meta->valuesize);
}

TEST_F(TaggingTests, test_ReadAPEv2MultiValueArtist_ReadsAs3Values) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/artist_multivalue_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_apev2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Value1"));

    const char refdata[] = "Value1\0Value2\0Value3\0";
    EXPECT_TRUE(sizeof (refdata)-1 == meta->valuesize && !memcmp (meta->value, refdata, meta->valuesize));
}

TEST_F(TaggingTests, test_ReadAPEv2MultiLineArtist_ReadsAs1Value) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/artist_multiline_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_apev2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Line1\r\nLine2\r\nLine3"));
    EXPECT_TRUE(strlen (meta->value) + 1 == meta->valuesize);
}

TEST_F(TaggingTests, test_WriteID3v2MultiLineArtist_MatchingBinaryReference) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Line1\nLine2\nLine3");
    copy_file(path, TESTFILE);
    junk_rewrite_tags(it, JUNK_WRITE_ID3V2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (it, &id3v2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_id3v2_frame_t *tpe1 = id3v2.frames;

    EXPECT_TRUE(!strcmp (tpe1->id, "TPE1"));

    const char refdata[] = "\0Line1\nLine2\nLine3";
    EXPECT_TRUE(!memcmp (tpe1->data, refdata, sizeof (refdata)-1));

    junk_id3v2_free (&id3v2);
}

TEST_F(TaggingTests, test_WriteID3v23MultiValueArtist_MatchingBinaryReference) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Value1");
    pl_append_meta (it, "artist", "Value2");
    pl_append_meta (it, "artist", "Value3");

    copy_file(path, TESTFILE);
    junk_rewrite_tags(it, JUNK_WRITE_ID3V2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (it, &id3v2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_id3v2_frame_t *tpe1 = id3v2.frames;

    EXPECT_TRUE(!strcmp (tpe1->id, "TPE1"));

    const char refdata[] = "\0Value1 / Value2 / Value3";
    EXPECT_TRUE(!memcmp (tpe1->data, refdata, sizeof (refdata)-1));

    junk_id3v2_free (&id3v2);
}

TEST_F(TaggingTests, test_WriteID3v24MultiValueArtist_MatchingBinaryReference) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Value1");
    pl_append_meta (it, "artist", "Value2");
    pl_append_meta (it, "artist", "Value3");

    copy_file(path, TESTFILE);
    junk_rewrite_tags(it, JUNK_WRITE_ID3V2, 4, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (it, &id3v2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_id3v2_frame_t *tpe1 = id3v2.frames;

    EXPECT_TRUE(!strcmp (tpe1->id, "TPE1"));

    const char refdata[] = "\x03Value1\0Value2\0Value3";
    EXPECT_TRUE(!memcmp (tpe1->data, refdata, sizeof (refdata)-1));

    junk_id3v2_free (&id3v2);
}

TEST_F(TaggingTests, test_WriteAPEv2MultiLineArtist_MatchingBinaryReference) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Line1\nLine2\nLine3");
    copy_file(path, TESTFILE);
    junk_rewrite_tags(it, JUNK_WRITE_APEV2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_apev2_tag_t apev2;
    memset (&apev2, 0, sizeof (apev2));
    junk_apev2_read_full (it, &apev2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_apev2_frame_t *artist = apev2.frames;

    EXPECT_TRUE(!strcasecmp (artist->key, "artist"));

    const char refdata[] = "Line1\nLine2\nLine3";
    EXPECT_TRUE(!memcmp (artist->data, refdata, sizeof (refdata)-1));

    junk_apev2_free (&apev2);
}

TEST_F(TaggingTests, test_WriteAPEv2MultiValueArtist_MatchingBinaryReference) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Value1");
    pl_append_meta (it, "artist", "Value2");
    pl_append_meta (it, "artist", "Value3");

    copy_file(path, TESTFILE);
    junk_rewrite_tags(it, JUNK_WRITE_APEV2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_apev2_tag_t apev2;
    memset (&apev2, 0, sizeof (apev2));
    junk_apev2_read_full (it, &apev2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_apev2_frame_t *artist = apev2.frames;

    EXPECT_TRUE(!strcasecmp (artist->key, "artist"));

    const char refdata[] = "Value1\0Value2\0Value3";
    EXPECT_TRUE(!memcmp (artist->data, refdata, sizeof (refdata)-1));

    junk_apev2_free (&apev2);
}

TEST_F(TaggingTests, test_ReadID3v2WithNonprintableChars_MatchingBinaryReference) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_nonprintable_id3v2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (NULL, &id3v2, fp);
    vfs_fclose (fp);

    const unsigned char refdata[] = {
        0x01, 0xff, 0xfe, 0x4c,
        0x00, 0x69, 0x00, 0x6e, 0x00, 0x65, 0x00, 0x31, 0x00, 0x01, 0x00, 0x02,
        0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08,
        0x00, 0x09, 0x00, 0x0a, 0x00, 0x0b, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x0e,
        0x00, 0x0f, 0x00, 0x10, 0x00, 0x11, 0x00, 0x12, 0x00, 0x13, 0x00, 0x14,
        0x00, 0x15, 0x00, 0x16, 0x00, 0x17, 0x00, 0x18, 0x00, 0x19, 0x00, 0x1a,
        0x00, 0x1b, 0x00, 0x1c, 0x00, 0x1d, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x0a,
        0x00, 0x4c, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x65, 0x00, 0x32, 0x00, 0x0d,
        0x00, 0x0a, 0x00, 0x4c, 0x00, 0x69, 0x00, 0x6e, 0x00, 0x65, 0x00, 0x33
    };

    DB_id3v2_frame_t *tpe1 = id3v2.frames;
    EXPECT_TRUE(!strcmp (tpe1->id, "TPE1"));
    EXPECT_TRUE(!memcmp (tpe1->data, refdata, sizeof (refdata)-1));

    junk_id3v2_free (&id3v2);
}

TEST_F(TaggingTests, test_ReadID3v2WithNonprintableChars_TFReplacesNonprintableWithUnderscores) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_nonprintable_id3v2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    char *bc = tf_compile("%artist%");
    char buffer[1000];

    ddb_tf_context_t ctx;
    memset (&ctx, 0, sizeof (ctx));
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = (DB_playItem_t *)it;
    ctx.plt = NULL;

    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);

    EXPECT_TRUE(!strcmp (buffer, "Line1________________________________Line2__Line3"));
}

TEST_F(TaggingTests, test_ReadID3v2COMM_ObtainsTheData) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/comm_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "comment");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."));
    EXPECT_TRUE(strlen (meta->value) + 1 == meta->valuesize);
}

TEST_F(TaggingTests, test_ReadID3v2TXXX_ObtainsTheData) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/txxx_album_artist_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "album artist");
    EXPECT_TRUE(meta);
    EXPECT_TRUE(!strcmp (meta->value, "Artist From ID3v2.3 Test File txxx_album_artist_id3v2.3"));
    EXPECT_TRUE(strlen (meta->value) + 1 == meta->valuesize);
}

TEST_F(TaggingTests, test_ReadID3v23TRCK_ReadsAsTrackNumAndTrackTotal) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/trck_num_with_total_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    ddb_tf_context_t ctx;
    char buffer[1000];
    memset (&ctx, 0, sizeof (ctx));
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = (DB_playItem_t *)it;
    ctx.plt = NULL;

    char *bc = tf_compile("track:%track% total:%numtracks%");
    tf_eval (&ctx, bc, buffer, sizeof (buffer));
    tf_free (bc);

    EXPECT_TRUE(!strcmp (buffer, "track:10 total:11"));
}

TEST_F(TaggingTests, test_ShortMP3WithId3v1_TailIs128Bytes) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tone1sec_id3v1.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    uint32_t head, tail;
    junk_get_tag_offsets(fp, &head, &tail);
    vfs_fclose (fp);
    EXPECT_TRUE(head == 0);
    EXPECT_TRUE(tail == 128);
}

TEST_F(TaggingTests, test_ShortMP3WithApev2_TailIs52Bytes) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tone1sec_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    uint32_t head, tail;
    junk_get_tag_offsets(fp, &head, &tail);
    vfs_fclose (fp);
    EXPECT_TRUE(head == 0);
    EXPECT_TRUE(tail == 52);
}

TEST_F(TaggingTests, test_ShortMP3WithApev2AndId3v1_TailIs186Bytes) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tone1sec_id3v1_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    uint32_t head, tail;
    junk_get_tag_offsets(fp, &head, &tail);
    vfs_fclose (fp);
    EXPECT_TRUE(head == 0);
    EXPECT_TRUE(tail == 186);
}

TEST_F(TaggingTests, test_ShortMP3WithId3v1_ScansCorrectSize) {
    playlist_t *plt = plt_alloc("test");

    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tone1sec_id3v1.mp3", dbplugindir);

    playItem_t *it = plt_insert_file2(0, plt, NULL, path, NULL, NULL, NULL);

    EXPECT_EQ_WITH_ACCURACY(it->_duration, 1.04489791f, 0.0001f);
    plt_unref (plt);
}

TEST_F(TaggingTests, test_ShortMP3WithApev2_ScansCorrectSize) {
    playlist_t *plt = plt_alloc("test");

    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tone1sec_apev2.mp3", dbplugindir);

    playItem_t *it = plt_insert_file2(0, plt, NULL, path, NULL, NULL, NULL);

    EXPECT_EQ_WITH_ACCURACY(it->_duration, 1.04489791f, 0.0001f);
    plt_unref (plt);
}

TEST_F(TaggingTests, test_ShortMP3WithId3v1AndApev2_ScansCorrectSize) {
    playlist_t *plt = plt_alloc("test");

    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tone1sec_id3v1_apev2.mp3", dbplugindir);

    playItem_t *it = plt_insert_file2(0, plt, NULL, path, NULL, NULL, NULL);

    EXPECT_EQ_WITH_ACCURACY(it->_duration, 1.04489791f, 0.0001f);
    plt_unref (plt);
}
