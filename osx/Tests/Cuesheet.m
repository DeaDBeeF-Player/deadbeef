#import <XCTest/XCTest.h>
#include "playlist.h"
#include "logger.h"

@interface Cuesheet : XCTestCase

@end

@implementation Cuesheet

- (void)setUp {
    [super setUp];

    ddb_logger_init ();
    pl_init ();
}

- (void)tearDown {
    pl_free ();
    ddb_logger_free ();

    [super tearDown];
}

- (void)testCueWithoutTitles {
    const char cue[] =
        "FILE \"file.wav\" WAVE\n"
        "TRACK 01 AUDIO\n"
        "INDEX 01 00:00:00\n"
        "TRACK 02 AUDIO\n"
        "INDEX 01 05:50:65\n"
        "TRACK 03 AUDIO\n"
        "INDEX 01 09:47:50\n";

    playlist_t *plt = plt_alloc("test");

    playItem_t *it = pl_item_alloc_init ("testfile.flac", "stdflac");
    pl_add_meta (it, "cuesheet", cue);
    plt_process_cue(plt, NULL, it, 60*10*44100, 44100);

    int cnt = plt_get_item_count(plt, PL_MAIN);

    XCTAssert(cnt == 3, @"The actual output is: %d", cnt);

    plt_free (plt);
}

@end
