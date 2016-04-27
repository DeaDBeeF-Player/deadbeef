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
#include "../../common.h"

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

    int cnt = 0;
    char combined[100] = "";

    for (ddb_metaValue_t *data = meta->values; data; data = data->next, cnt++) {
        strcat (combined, data->value);
        strcat (combined, "/");
    }

    XCTAssert(cnt == 3, @"Got count: %d", cnt);
    XCTAssert(!strcmp (combined, "Value1/Value2/Value3/"), @"Got value: %s", meta->value);
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

    int cnt = 0;
    char combined[100] = "";

    for (ddb_metaValue_t *data = meta->values; data; data = data->next, cnt++) {
        strcat (combined, data->value);
        strcat (combined, "/");
    }

    XCTAssert(cnt == 3, @"Got count: %d", cnt);
    XCTAssert(!strcmp (combined, "Value1/Value2/Value3/"), @"Got value: %s", meta->value);
}

- (void)test_ReadID3v2MultiLineTPE1_ReadsAs1Value {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/tpe1_multiline.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_id3v2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Line1\nLine2\nLine3"), @"Actual value: %s", meta->value);
    XCTAssert(!meta->values->next, @"Pass");
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

    int cnt = 0;
    char combined[100] = "";

    for (ddb_metaValue_t *data = meta->values; data; data = data->next, cnt++) {
        strcat (combined, data->value);
        strcat (combined, "/");
    }

    XCTAssert(cnt == 3, @"Got count: %d", cnt);
    XCTAssert(!strcmp (combined, "Value1/Value2/Value3/"), @"Got value: %s", meta->value);
}

- (void)test_ReadAPEv2MultiLineArtist_ReadsAs1Value {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/artist_multiline_apev2.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    junk_apev2_read (it, fp);
    vfs_fclose (fp);

    DB_metaInfo_t *meta = pl_meta_for_key (it, "artist");
    XCTAssert(meta, @"Pass");
    XCTAssert(!strcmp (meta->value, "Line1\nLine2\nLine3"), @"Actual value: %s", meta->value);
    XCTAssert(!meta->values->next, @"Pass");
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


@end
