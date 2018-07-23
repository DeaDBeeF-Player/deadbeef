//
//  MP3Decoder.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 03/07/2018.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "deadbeef.h"
#include "../../common.h"
#include "playlist.h"

extern DB_functions_t *deadbeef;

@interface MP3Decoder : XCTestCase

@end

@implementation MP3Decoder

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)test_DecodeMP3As2Pieces_SameAs1Piece {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    playlist_t *plt = plt_alloc ("testplt");

    DB_playItem_t *it = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, path, NULL, NULL, NULL);


    DB_decoder_t *dec = (DB_decoder_t *)deadbeef->plug_get_for_id ("stdmpg");

    DB_fileinfo_t *fi = dec->open (0);
    dec->init (fi, it);

    int64_t size = 176400;
    char *buffer = malloc (size*2);

    // request twice as much to make sure we don't overshoot
    int res = dec->read (fi, buffer, (int)size*2);

    XCTAssertEqual(res, size);

    char *buffer2 = malloc (size*2); // buffer is over-allocated to check for trailing data

    // read first part
    dec->seek_sample (fi, 0);

    res = dec->read (fi, buffer2, (int)size/2);
    XCTAssertEqual(res, size/2);

    printf ("-------------\n");
    dec->seek_sample (fi, 44100/2);
    res = dec->read (fi, buffer2+size/2, (int)size);
    XCTAssertEqual(res, size/2);


    FILE *fp1 = fopen ("/Users/oleksiy/buffer1.raw", "w+b");
    FILE *fp2 = fopen ("/Users/oleksiy/buffer2.raw", "w+b");
    fwrite (buffer, size, 1, fp1);
    fwrite (buffer2, size, 1, fp2);
    fclose (fp1);
    fclose (fp2);


    int cmp1 = memcmp (buffer, buffer2, size/2);
    XCTAssertTrue(cmp1==0);

    int cmp2 = memcmp (buffer+size/2, buffer2+size/2, size/2);
    XCTAssertTrue(cmp2==0);

    int cmp = memcmp (buffer, buffer2, size);

    free (buffer);
    free (buffer2);

    dec->free (fi);

    deadbeef->plt_unref ((ddb_playlist_t *)plt);

    XCTAssertTrue(cmp==0);
}


@end
