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

- (void)runMp3TwoPieceTestWithBackend:(int)backend {
    deadbeef->conf_set_int ("mp3.backend", backend);

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

    dec->seek_sample (fi, 44100/2);
    res = dec->read (fi, buffer2+size/2, (int)size);
    XCTAssertEqual(res, size/2);

#if 0
    FILE *fp1 = fopen ("buffer1.raw", "w+b");
    FILE *fp2 = fopen ("buffer2.raw", "w+b");
    fwrite (buffer, size, 1, fp1);
    fwrite (buffer2, size, 1, fp2);
    fclose (fp1);
    fclose (fp2);
#endif

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

- (void)test_DecodeMP3As2PiecesMPG123_SameAs1Piece {
    [self runMp3TwoPieceTestWithBackend:0]; // mpg123
}

- (void)test_DecodeMP3As2PiecesLibMAD_SameAs1Piece {
    [self runMp3TwoPieceTestWithBackend:1]; // libmad
}

- (void)test_FiniteVBRNetworkStream_DecodesFullAmountOfSamples {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/vbr_rhytm_30sec.mp3", dbplugindir);

    deadbeef->conf_set_int ("mp3.backend", 0);

    playlist_t *plt = plt_alloc ("testplt");

    DB_playItem_t *it = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, path, NULL, NULL, NULL);

    DB_decoder_t *dec = (DB_decoder_t *)deadbeef->plug_get_for_id ("stdmpg");

    DB_fileinfo_t *fi = dec->open (1<<31); // indicate with a flag to decode in streaming mode (internal flag)
    dec->init (fi, it);

    size_t size = 1024751 * 4;
    char *buffer = malloc (size*2);

    int res = dec->read (fi, buffer, (int)size*2);

    free (buffer);

    dec->free (fi);

    deadbeef->plt_unref ((ddb_playlist_t *)plt);

    XCTAssertEqual(res/4, size/4);
}


@end
