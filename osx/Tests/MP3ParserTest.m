//
//  MP3ParserTest.m
//  Tests
//
//  Created by Alexey Yakovenko on 6/14/18.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "vfs.h"
#include "plugins.h"
#include "conf.h"
#include "playlist.h"
#include "../../common.h"
#include "logger.h"
#include "mp3parser.h"

@interface MP3ParserTest : XCTestCase

@end

@implementation MP3ParserTest

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

}

- (void)tearDown {
    plug_disconnect_all ();
    plug_unload_all ();
    pl_free ();
    conf_free ();
    ddb_logger_free ();

    [super tearDown];
}

- (void)test_2secSquareWithLameHeader_XingDetectedWith88200Samples {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 1);
    XCTAssertEqual(info.ref_packet.samplerate, 44100);
    XCTAssertEqual(info.pcmsample-info.delay-info.padding, 88200);
    XCTAssertEqual(info.delay, 576);
    XCTAssertEqual(info.padding, 1080);
    XCTAssertEqual(info.lame_musiclength, 16508);
}

- (void)test_2secSquareWithLameHeader_FullScanGet88200Samples {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, MP3_PARSE_FULLSCAN, fp, fsize, 0, 0, -1);
    XCTAssert (!res);
    XCTAssertEqual(info.have_xing_header, 1);
    XCTAssertEqual(info.ref_packet.samplerate, 44100);
    XCTAssertEqual(info.pcmsample, 88200+576+1080);
    XCTAssertEqual(info.delay, 0);
    XCTAssertEqual(info.padding, 0);
    XCTAssertEqual(info.lame_musiclength, 16508);
}

- (void)test_2secSquareSeekTo0_GivesPacketAfterXingPos208 {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 0);
    XCTAssert (!res);
    XCTAssertEqual(info.packet_offs, 208);
}

- (void)test_2secSquareSeekToDelay_GivesPacketAfterXingPos208 {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576);
    XCTAssert (!res);
    XCTAssertEqual(info.packet_offs, 0);
}

- (void)test_2secSquareSeekTo1sec_SeeksTo1SecMinus10Packets {
    mp3info_t info;
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/mp3parser/2sec-square-lamehdr.mp3", dbplugindir);
    DB_FILE *fp = vfs_fopen (path);
    int64_t fsize = vfs_fgetlength(fp);
    int res = mp3_parse_file (&info, 0, fp, fsize, 0, 0, 576+44100);
    XCTAssert (!res);
    XCTAssertEqual(info.packet_offs, 5850);
    XCTAssertEqual(info.pcmsample, 32256);
}

@end
