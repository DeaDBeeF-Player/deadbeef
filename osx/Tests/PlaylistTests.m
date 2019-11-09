//
//  PlaylistTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 24/10/2018.
//  Copyright © 2018 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "deadbeef.h"
#include "../../common.h"
#include "playlist.h"
#include "streamer.h"
#include "plugins.h"
#include "conf.h"

@interface PlaylistTests : XCTestCase

@end

@implementation PlaylistTests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)test_SearchForValueInSingleValueItems_FindsTheItem {
    playlist_t *plt = plt_alloc("test");
    playItem_t *it = pl_item_alloc();

    plt_insert_item(plt, NULL, it);

    pl_add_meta(it, "title", "value");

    plt_search_process(plt, "value");

    XCTAssertTrue(plt->head[PL_SEARCH] != NULL);
    XCTAssertTrue(plt->head[PL_MAIN]->selected);

    plt_unref (plt);
}


- (void)test_SearchFor2ndValueInMultiValueItems_FindsTheItem {
    playlist_t *plt = plt_alloc("test");
    playItem_t *it = pl_item_alloc();

    plt_insert_item(plt, NULL, it);

    const char values[] = "value1\0value2\0";
    pl_add_meta_full(it, "title", values, sizeof(values));

    plt_search_process(plt, "value2");

    XCTAssertTrue(plt->head[PL_SEARCH] != NULL);
    XCTAssertTrue(plt->head[PL_MAIN]->selected);

    plt_unref (plt);
}

#pragma mark - IsRelativePathPosix

- (void)test_IsRelativePathPosix_AbsolutePath_False {
    int res = is_relative_path_posix ("/path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathPosix_AbsolutePathWithUriScheme_False {
    int res = is_relative_path_posix ("file:///path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathPosix_RelativePathWithUriScheme_True {
    int res = is_relative_path_posix ("file://path");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePathPosix_VFSPath_False {
    int res = is_relative_path_posix ("zip://path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathPosix_HTTPPath_False {
    int res = is_relative_path_posix ("http://path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathPosix_RelativePath_True {
    int res = is_relative_path_posix ("path");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePathPosix_RelativeWithFoldersPath_True {
    int res = is_relative_path_posix ("path/filename");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePathPosix_WeirdPath_True {
    int res = is_relative_path_posix ("something:something");
    XCTAssertTrue(res);
}

#pragma mark - IsRelativePathWin32

- (void)test_IsRelativePathWin32_AbsolutePath_False {
    int res = is_relative_path_win32 ("z:\\path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathWin32_AbsolutePathWithUriScheme_False {
    int res = is_relative_path_win32 ("file://z:\\path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathWin32_AbsolutePathForwardSlash_False {
    int res = is_relative_path_win32 ("z:/path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathWin32_AbsolutePathForwardSlashWithUriScheme_False {
    int res = is_relative_path_win32 ("file://z:/path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathWin32_RelativePathWithUriScheme_True {
    int res = is_relative_path_win32 ("file://path");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePathWin32_VFSPath_False {
    int res = is_relative_path_win32 ("zip://path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathWin32_HTTPPath_False {
    int res = is_relative_path_win32 ("http://path");
    XCTAssertFalse(res);
}

- (void)test_IsRelativePathWin32_RelativePath_True {
    int res = is_relative_path_win32 ("path");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePathWin32_RelativeWithFoldersPath_True {
    int res = is_relative_path_win32 ("path/filename");
    XCTAssertTrue(res);
}

- (void)test_IsRelativePathWin32_WeirdPath_True {
    int res = is_relative_path_win32 ("something:something");
    XCTAssertTrue(res);
}


- (playlist_t *)setupGetNextTrackTest {
    conf_set_int ("playback.loop", PLAYBACK_MODE_NOLOOP);
    pl_set_order (PLAYBACK_ORDER_LINEAR);
    playlist_t *plt = plt_alloc ("testplt");

    playItem_t *it1 = pl_item_alloc();
    playItem_t *it2 = pl_item_alloc();
    playItem_t *it3 = pl_item_alloc();

    plt_insert_item (plt, NULL, it1);
    plt_insert_item (plt, it1, it2);
    plt_insert_item (plt, it2, it3);

    plt_set_curr (plt);
    streamer_set_streamer_playlist (plt);

    pl_item_unref (it1);
    pl_item_unref (it2);
    pl_item_unref (it3);

    return plt;
}

- (void)teardownGetNextTrackTest {
    plt_set_curr(NULL);
}

- (void)test_GetNextTrackWithDirectionBackwards_RepeatOffShuffleOffNoCurrent_Last {
    [self setupGetNextTrackTest];

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_next_track_with_direction (-1);

    playlist_t *plt = plt_get_curr();

    XCTAssertEqual(it, plt->tail[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }

    [self teardownGetNextTrackTest];
}

- (void)test_GetNextTrackWithDirectionBackwards_RepeatOffShuffleOffCurrentFirst_Null {
    [self setupGetNextTrackTest];

    playlist_t *plt = plt_get_curr();

    streamer_set_last_played (plt->head[PL_MAIN]);

    playItem_t *it = streamer_get_next_track_with_direction (-1);

    XCTAssertEqual(it, NULL);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }

    [self teardownGetNextTrackTest];
}

- (void)test_GetNextTrackWithDirectionForward_RepeatOffShuffleOffCurrentNull_First {
    [self setupGetNextTrackTest];

    playlist_t *plt = plt_get_curr();

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_next_track_with_direction (1);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }

    [self teardownGetNextTrackTest];
}

- (void)test_GetCurrentTrackToPlay_NoCursor_First {
    [self setupGetNextTrackTest];

    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = -1;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }

    [self teardownGetNextTrackTest];
}

- (void)test_GetCurrentTrackToPlay_Cursor0_First {
    [self setupGetNextTrackTest];

    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = 0;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }

    [self teardownGetNextTrackTest];
}

- (void)test_GetCurrentTrackToPlay_Cursor1_Second {
    [self setupGetNextTrackTest];

    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = 1;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    XCTAssertEqual(it, plt->head[PL_MAIN]->next[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }

    [self teardownGetNextTrackTest];
}


@end
