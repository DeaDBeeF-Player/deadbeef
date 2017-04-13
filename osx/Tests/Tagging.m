//
//  Tagging.m
//  
//
//  Created by waker on 26/04/16.
//
//

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include "playlist.h"
#include "junklib.h"
#include "vfs.h"
#include "plugins.h"
#include "conf.h"
#include "tf.h"
#include "../../common.h"
#include "logger.h"

#define TESTFILE "/tmp/ddb_test.mp3"

@interface Tagging : XCTestCase {
    playItem_t *it;
}
@end

@implementation Tagging

- (void)setUp {
    [super setUp];

    NSString *resPath = [[NSBundle bundleForClass:[self class]] resourcePath];
    const char *str = [resPath UTF8String];
    strcpy (dbplugindir, str);

    ddb_logger_init ();
    conf_init ();
    conf_enable_saving (0);

    pl_init ();
    if (plug_load_all ()) { // required to add files to playlist from commandline
        exit (-1);
    }

    it = pl_item_alloc_init (TESTFILE, "stdmpg");
}

- (void)tearDown {
    pl_item_unref (it);

    plug_disconnect_all ();
    plug_unload_all ();
    pl_free ();
    conf_free ();
    ddb_logger_free ();

    [super tearDown];
}

- (void)test_loadTestfileTags_DoesntCrash {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);
}

- (void)test_ReadID3v24MultiValueTPE1_ReadsAs3Values {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multivalue_id3v2.4.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Value1"), @"Got value: %s", meta->value);

    const char refdata[] = "Value1\0Value2\0Value3\0";
    XCTAssert(sizeof (refdata)-1 == meta->valuesize && !memcmp (meta->value, refdata, meta->valuesize), @"Got value: %s", meta->value);
}

- (void)test_ReadID3v23MultiValueTPE1_ReadsAs3Values {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multivalue_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Value1"), @"Got value: %s", meta->value);

    const char refdata[] = "Value1\0Value2\0Value3\0";
    XCTAssert(sizeof (refdata)-1 == meta->valuesize && !memcmp (meta->value, refdata, meta->valuesize), @"Got value: %s", meta->value);
}

- (void)test_ReadID3v2MultiLineTPE1_ReadsAs1Value {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multiline.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Line1\r\nLine2\r\nLine3"), @"Actual value: %s", meta->value);
    XCTAssert(strlen (meta->value) + 1 == meta->valuesize, @"Pass");
}

- (void)test_ReadAPEv2MultiValueArtist_ReadsAs3Values {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/artist_multivalue_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_apev2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Value1"), @"Got value: %s", meta->value);

    const char refdata[] = "Value1\0Value2\0Value3\0";
    XCTAssert(sizeof (refdata)-1 == meta->valuesize && !memcmp (meta->value, refdata, meta->valuesize), @"Got value: %s", meta->value);
}

- (void)test_ReadAPEv2MultiLineArtist_ReadsAs1Value {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/artist_multiline_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_apev2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Line1\r\nLine2\r\nLine3"), @"Actual value: %s", meta->value);
    XCTAssert(strlen (meta->value) + 1 == meta->valuesize, @"Pass");
}

- (void)test_WriteID3v2MultiLineArtist_MatchingBinaryReference {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Line1\nLine2\nLine3");
    [[NSFileManager defaultManager] copyItemAtPath:[NSString stringWithUTF8String:path] toPath:@TESTFILE error:nil];
    junk_rewrite_tags(it, JUNK_WRITE_ID3V2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (it, &id3v2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_id3v2_frame_t *tpe1 = id3v2.frames;

    XCTAssert (!strcmp (tpe1->id, "TPE1"), @"Unexpected frame: %s", tpe1->id);

    const char refdata[] = "\0Line1\nLine2\nLine3";
    XCTAssert (!memcmp (tpe1->data, refdata, sizeof (refdata)-1), @"TPE1 frame contents don't match reference");

    junk_id3v2_free (&id3v2);
}

- (void)test_WriteID3v23MultiValueArtist_MatchingBinaryReference {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Value1");
    pl_append_meta (it, "artist", "Value2");
    pl_append_meta (it, "artist", "Value3");

    [[NSFileManager defaultManager] copyItemAtPath:[NSString stringWithUTF8String:path] toPath:@TESTFILE error:nil];
    junk_rewrite_tags(it, JUNK_WRITE_ID3V2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (it, &id3v2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_id3v2_frame_t *tpe1 = id3v2.frames;

    XCTAssert (!strcmp (tpe1->id, "TPE1"), @"Unexpected frame: %s", tpe1->id);

    const char refdata[] = "\0Value1 / Value2 / Value3";
    XCTAssert (!memcmp (tpe1->data, refdata, sizeof (refdata)-1), @"TPE1 frame contents don't match reference");

    junk_id3v2_free (&id3v2);
}

- (void)test_WriteID3v24MultiValueArtist_MatchingBinaryReference {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Value1");
    pl_append_meta (it, "artist", "Value2");
    pl_append_meta (it, "artist", "Value3");

    [[NSFileManager defaultManager] copyItemAtPath:[NSString stringWithUTF8String:path] toPath:@TESTFILE error:nil];
    junk_rewrite_tags(it, JUNK_WRITE_ID3V2, 4, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (it, &id3v2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_id3v2_frame_t *tpe1 = id3v2.frames;

    XCTAssert (!strcmp (tpe1->id, "TPE1"), @"Unexpected frame: %s", tpe1->id);

    const char refdata[] = "\x03Value1\0Value2\0Value3";
    XCTAssert (!memcmp (tpe1->data, refdata, sizeof (refdata)-1), @"TPE1 frame contents don't match reference");

    junk_id3v2_free (&id3v2);
}

- (void)test_WriteAPEv2MultiLineArtist_MatchingBinaryReference {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Line1\nLine2\nLine3");
    [[NSFileManager defaultManager] copyItemAtPath:[NSString stringWithUTF8String:path] toPath:@TESTFILE error:nil];
    junk_rewrite_tags(it, JUNK_WRITE_APEV2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_apev2_tag_t apev2;
    memset (&apev2, 0, sizeof (apev2));
    junk_apev2_read_full (it, &apev2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_apev2_frame_t *artist = apev2.frames;

    XCTAssert (!strcasecmp (artist->key, "artist"), @"Unexpected frame: %s", artist->key);

    const char refdata[] = "Line1\nLine2\nLine3";
    XCTAssert (!memcmp (artist->data, refdata, sizeof (refdata)-1), @"ARTIST frame contents don't match reference");

    junk_apev2_free (&apev2);
}

- (void)test_WriteAPEv2MultiValueArtist_MatchingBinaryReference {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/empty.mp3", dbplugindir);

    pl_append_meta (it, "artist", "Value1");
    pl_append_meta (it, "artist", "Value2");
    pl_append_meta (it, "artist", "Value3");

    [[NSFileManager defaultManager] copyItemAtPath:[NSString stringWithUTF8String:path] toPath:@TESTFILE error:nil];
    junk_rewrite_tags(it, JUNK_WRITE_APEV2, 3, NULL);

    DB_FILE *fp = vfs_fopen (TESTFILE);
    DB_apev2_tag_t apev2;
    memset (&apev2, 0, sizeof (apev2));
    junk_apev2_read_full (it, &apev2, fp);
    vfs_fclose (fp);
    unlink (TESTFILE);

    DB_apev2_frame_t *artist = apev2.frames;

    XCTAssert (!strcasecmp (artist->key, "artist"), @"Unexpected frame: %s", artist->key);

    const char refdata[] = "Value1\0Value2\0Value3";
    XCTAssert (!memcmp (artist->data, refdata, sizeof (refdata)-1), @"ARTIST frame contents don't match reference");

    junk_apev2_free (&apev2);
}

- (void)test_ReadID3v2WithNonprintableChars_MatchingBinaryReference {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_nonprintable_id3v2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    DB_id3v2_tag_t id3v2;
    memset (&id3v2, 0, sizeof (id3v2));
    junk_id3v2_read_full (NULL, &id3v2, fp);
    vfs_fclose (fp);

    const char refdata[] = {
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
    XCTAssert (!strcmp (tpe1->id, "TPE1"), @"Unexpected frame: %s", tpe1->id);
    XCTAssert (!memcmp (tpe1->data, refdata, sizeof (refdata)-1), @"TPE1 frame contents don't match reference");

    junk_id3v2_free (&id3v2);
}

- (void)test_ReadID3v2WithNonprintableChars_TFReplacesNonprintableWithUnderscores {
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

    XCTAssert (!strcmp (buffer, "Line1________________________________Line2__Line3"), @"Unexpected data: %s", buffer);
}

- (void)test_ReadID3v2COMM_ObtainsTheData {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/comm_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "comment");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."), @"Actual value: %s", meta->value);
    XCTAssert(strlen (meta->value) + 1 == meta->valuesize, @"Pass");
}

- (void)test_ReadID3v2TXXX_ObtainsTheData {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/txxx_album_artist_id3v2.3.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "album artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Artist From ID3v2.3 Test File txxx_album_artist_id3v2.3"), @"Actual value: %s", meta->value);
    XCTAssert(strlen (meta->value) + 1 == meta->valuesize, @"Pass");
}

- (void)test_ReadID3v23TRCK_ReadsAsTrackNumAndTrackTotal {
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

    XCTAssert(!strcmp (buffer, "track:10 total:11"), @"Got value: %s", buffer);
}

@end
