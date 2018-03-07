/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Alexey Yakovenko and other contributors

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
