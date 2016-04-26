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
#include "dummy_mp3.c"

#define TESTFILE "/tmp/testfile.mp3"

static int
write_file_from_data(const char *filename, const char *data, size_t size) {
    FILE *fp = fopen (filename, "w+b");
    if (!fp) {
        return -1;
    }

    int res = (size == fwrite (data, 1, size, fp)) ? 0 : -1;

    fclose (fp);
    return res;
}


@interface Tagging : XCTestCase {
    playItem_t *it;
}

@end

@implementation Tagging

- (void)setUp {
    [super setUp];

    write_file_from_data(TESTFILE, mp3_file_data, sizeof (mp3_file_data));

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

    unlink (TESTFILE);

    [super tearDown];
}

- (void)test_loadTestfileTags_DoesntCrash {
    DB_FILE *fp = vfs_fopen (TESTFILE);
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

- (void)test_ReadMultiLineTPE1_ReadsAs1Value {
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

@end
