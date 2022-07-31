//
//  TrackSwitchingTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 11/9/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "deadbeef.h"
#include "../../common.h"
#include "conf.h"
#include "playlist.h"
#include "streamer.h"
#include "playmodes.h"

@interface TrackSwitchingTests : XCTestCase

@end

@implementation TrackSwitchingTests

- (void)setUp {
    streamer_init();
    streamer_set_repeat (DDB_REPEAT_OFF);
    streamer_set_shuffle (DDB_SHUFFLE_OFF);
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
}

- (void)tearDown {
    plt_set_curr(NULL);
    streamer_free();
}

#pragma mark - Get Next Track

- (void)test_GetNextTrackWithDirectionBackwards_RepeatOffShuffleOffNoCurrent_Last {
    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    playlist_t *plt = plt_get_curr();

    XCTAssertEqual(it, plt->tail[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

- (void)test_GetNextTrackWithDirectionBackwards_RepeatOffShuffleOffCurrentFirst_Null {
    playlist_t *plt = plt_get_curr();

    streamer_set_last_played (plt->head[PL_MAIN]);

    playItem_t *it = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    XCTAssertEqual(it, NULL);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

- (void)test_GetNextTrackWithDirectionForward_RepeatOffShuffleOffCurrentNull_First {
    playlist_t *plt = plt_get_curr();

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_next_track_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

#pragma mark - Get Current Track

- (void)test_GetCurrentTrackToPlay_NoCursor_First {
    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = -1;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

- (void)test_GetCurrentTrackToPlay_Cursor0_First {
    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = 0;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

- (void)test_GetCurrentTrackToPlay_Cursor1_Second {
    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = 1;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    XCTAssertEqual(it, plt->head[PL_MAIN]->next[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

- (void)test_GetNextTrack_CurrentIsNotInPlaylist_First {
    playlist_t *plt = plt_get_curr();

    playItem_t *playing = pl_item_alloc();

    streamer_set_playing_track(playing);
    streamer_set_last_played (playing);

    playItem_t *it = streamer_get_next_track_with_direction(1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    XCTAssertEqual(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
    pl_item_unref (playing);
}

@end
