//
//  TrackSwitchingTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 11/9/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#include "conf.h"
#include "logger.h"
#include <deadbeef/common.h>
#include <deadbeef/deadbeef.h>
#include "conf.h"
#include "fakeout.h"
#include "messagepump.h"
#include "playlist.h"
#include "playmodes.h"
#include "plmeta.h"
#include "plugins.h"
#include "streamer.h"
#include "threading.h"
#include <gtest/gtest.h>

extern "C" DB_plugin_t * fakeout_load (DB_functions_t *api);

// super oversimplified mainloop
static void
mainloop (void *ctx) {
    for (;;) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        int term = 0;
        while (messagepump_pop(&msg, &ctx, &p1, &p2) != -1) {
            if (!term) {
                DB_output_t *output = plug_get_output ();
                switch (msg) {
                case DB_EV_TERMINATE:
                    term = 1;
                    break;
                case DB_EV_PLAY_CURRENT:
                    streamer_play_current_track ();
                    break;
                case DB_EV_PLAY_NUM:
                    streamer_set_nextsong (p1, 0);
                    break;
                case DB_EV_STOP:
                    streamer_set_nextsong (-1, 0);
                    break;
                case DB_EV_NEXT:
                    streamer_move_to_nextsong (1);
                    break;
                case DB_EV_PREV:
                    streamer_move_to_prevsong (1);
                    break;
                case DB_EV_PAUSE:
                    if (output->state () != DDB_PLAYBACK_STATE_PAUSED) {
                        output->pause ();
                        messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                    }
                    break;
                case DB_EV_TOGGLE_PAUSE:
                    if (output->state () != DDB_PLAYBACK_STATE_PLAYING) {
                        streamer_play_current_track ();
                    }
                    else {
                        output->pause ();
                        messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                    }
                    break;
                case DB_EV_PLAY_RANDOM:
                    streamer_move_to_randomsong (1);
                    break;
                case DB_EV_SEEK:
                    {
                        int32_t pos = (int32_t)p1;
                        if (pos < 0) {
                            pos = 0;
                        }
                        streamer_set_seek (p1 / 1000.f);
                    }
                    break;
                case DB_EV_SONGSTARTED:
                    break;
                case DB_EV_TRACKINFOCHANGED:
                    break;
                }
            }
            if (msg >= DB_EV_FIRST && ctx) {
                messagepump_event_free ((ddb_event_t *)ctx);
            }
        }
        if (term) {
            return;
        }
        messagepump_wait ();
    }
}

class TrackSwitchingTests: public ::testing::Test {
protected:
    void SetUp() override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);

        messagepump_init();

        plug_init_plugin (fakeout_load, NULL);
        _fakeout = (DB_output_t *)fakeout_load (plug_get_api ());
        plug_register_out ((DB_plugin_t *)_fakeout);

        plug_set_output (_fakeout);

        streamer_init();
        streamer_set_repeat (DDB_REPEAT_OFF);
        streamer_set_shuffle (DDB_SHUFFLE_OFF);
        playlist_t *plt = plt_alloc ("testplt");

        playItem_t *it1 = pl_item_alloc();
        pl_replace_meta(it1, ":URI", "track1");
        playItem_t *it2 = pl_item_alloc();
        pl_replace_meta(it2, ":URI", "track2");
        playItem_t *it3 = pl_item_alloc();
        pl_replace_meta(it3, ":URI", "track3");

        plt_insert_item (plt, NULL, it1);
        plt_insert_item (plt, it1, it2);
        plt_insert_item (plt, it2, it3);

        plt_set_curr (plt);
        streamer_set_streamer_playlist (plt);

        pl_item_unref (it1);
        pl_item_unref (it2);
        pl_item_unref (it3);

        _mainloop_tid = thread_start (mainloop, NULL);
    }
    void TearDown() override {
        plt_set_curr(NULL);
        _fakeout->stop ();
        streamer_free();
        deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
        thread_join (_mainloop_tid);
        messagepump_free();
        conf_free();
        ddb_logger_free();
    }
    DB_output_t *_fakeout;
    uintptr_t _mainloop_tid;
};

#pragma mark - Get Next Track

TEST_F(TrackSwitchingTests, test_GetNextTrackWithDirectionBackwards_RepeatOffShuffleOffNoCurrent_Last) {
    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    playlist_t *plt = plt_get_curr();

    EXPECT_EQ(it, plt->tail[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

TEST_F(TrackSwitchingTests, test_GetNextTrackWithDirectionBackwards_RepeatOffShuffleOffCurrentFirst_Null) {
    playlist_t *plt = plt_get_curr();

    streamer_set_last_played (plt->head[PL_MAIN]);

    playItem_t *it = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    EXPECT_EQ(it, nullptr);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

TEST_F(TrackSwitchingTests, test_GetNextTrackWithDirectionForward_RepeatOffShuffleOffCurrentNull_First) {
    playlist_t *plt = plt_get_curr();

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_next_track_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    EXPECT_EQ(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

#pragma mark - Get Current Track

TEST_F(TrackSwitchingTests, test_GetCurrentTrackToPlay_NoCursor_First) {
    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = -1;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    EXPECT_EQ(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

TEST_F(TrackSwitchingTests, test_GetCurrentTrackToPlay_Cursor0_First) {
    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = 0;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    EXPECT_EQ(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

TEST_F(TrackSwitchingTests, test_GetCurrentTrackToPlay_Cursor1_Second) {
    playlist_t *plt = plt_get_curr();
    plt->current_row[PL_MAIN] = 1;

    streamer_set_last_played (NULL);

    playItem_t *it = streamer_get_current_track_to_play(plt);

    EXPECT_EQ(it, plt->head[PL_MAIN]->next[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
}

TEST_F(TrackSwitchingTests, test_GetNextTrack_CurrentIsNotInPlaylist_First) {
    playlist_t *plt = plt_get_curr();

    playItem_t *playing = pl_item_alloc();

    streamer_set_playing_track(playing);
    streamer_set_last_played (playing);

    playItem_t *it = streamer_get_next_track_with_direction(1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);

    EXPECT_EQ(it, plt->head[PL_MAIN]);

    plt_unref (plt);

    if (it) {
        pl_item_unref (it);
    }
    pl_item_unref (playing);
}
