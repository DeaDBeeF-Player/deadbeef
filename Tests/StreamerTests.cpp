/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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

#include "deadbeef.h"
#include "playlist.h"
#include "plugins.h"
#include "conf.h"
#include "../common.h"
#include "streamer.h"
#include "threading.h"
#include "messagepump.h"
#include "fakein.h"
#include "fakeout.h"
#include "playmodes.h"
#include <gtest/gtest.h>

extern "C" DB_plugin_t * fakein_load (DB_functions_t *api);
extern "C" DB_plugin_t * fakeout_load (DB_functions_t *api);



class StreamerTests: public ::testing::Test {
protected:

    void SetUp() override {
        messagepump_init ();

        // register fakein and fakeout plugin
        plug_init_plugin (fakein_load, NULL);
        plug_init_plugin (fakeout_load, NULL);

        _fakein = fakein_load (plug_get_api ());
        _fakeout = (DB_output_t *)fakeout_load (plug_get_api ());
        plug_register_in (_fakein);
        plug_register_out ((DB_plugin_t *)_fakeout);

        plug_set_output (_fakeout);

        streamer_init ();

        streamer_set_repeat(DDB_REPEAT_OFF);
        count_played = 0;

        _mainloop_tid = thread_start (mainloop_wrapper, this);
    }

    void TearDown() override {
        deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
        thread_join (_mainloop_tid);

        _fakeout->stop ();
        streamer_free ();
    }

protected:

    DB_plugin_t *_fakein;
    DB_output_t *_fakeout;
    uintptr_t _mainloop_tid;

    DB_playItem_t *switchtest_tracks[2];
    int switchtest_counts[2];
    int count_played;

    static void switchtest_trackinfochanged_handler (ddb_event_track_t *ev, StreamerTests *self) {
        if (deadbeef->streamer_ok_to_read (-1)) {
            DB_playItem_t *playing = deadbeef->streamer_get_playing_track_safe ();
            if (ev->track == self->switchtest_tracks[0] && playing == ev->track) {
                self->switchtest_counts[0]++;
            }
            else if (ev->track == self->switchtest_tracks[1] && playing == ev->track) {
                self->switchtest_counts[1]++;
            }
            if (playing) {
                deadbeef->pl_item_unref (playing);
            }
        }
    }

    // super oversimplified mainloop
    static void
    mainloop_wrapper (void *ctx) {
        StreamerTests *self = (StreamerTests *)ctx;
        self->mainloop();
    }

    static void (*_trackinfochanged_handler)(ddb_event_track_t *ev, StreamerTests *self);

    void mainloop() {
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
                        count_played++;
                        break;
                    case DB_EV_TRACKINFOCHANGED:
                        if (_trackinfochanged_handler) {
                            _trackinfochanged_handler ((ddb_event_track_t *)ctx, this);
                        }
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

    void
    wait_until_stopped (void) {
        // wait until finished!
        bool finished = false;

        while (!finished) {
            playItem_t *streaming_track = streamer_get_streaming_track();
            playItem_t *playing_track = streamer_get_playing_track();
            if (!streaming_track && !playing_track) {
                finished = true;
            }
            if (streaming_track) {
                pl_item_unref (streaming_track);
            }
            if (playing_track) {
                pl_item_unref(playing_track);
            }
        }
    }
};

void (*StreamerTests::_trackinfochanged_handler)(ddb_event_track_t *ev, StreamerTests *self);

TEST_F(StreamerTests, test_Play2TracksNoLoop_Sends2SongChanged) {
    playlist_t *plt = plt_alloc ("testplt");
    // create two test fake tracks
//    DB_playItem_t *_sinewave = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL);
//    DB_playItem_t *_squarewave = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, _sinewave, "/square.fake", NULL, NULL, NULL);

    plt_set_curr (plt);

    streamer_set_nextsong (0, 0);
    streamer_yield ();

    wait_until_stopped ();

    plt_set_curr (NULL);
    deadbeef->plt_unref ((ddb_playlist_t *)plt);


    EXPECT_TRUE(count_played = 2);
}

// This test is a complicated
// Given two tracks A and B
// Start track A
// Start track B
// Monitor trackinfochanged events, and make sure that track A is never in "playing" state after track B started "buffering"

TEST_F(StreamerTests, test_SwitchBetweenTracks_DoesNotJumpBackToPrevious) {
    // for this test, we want "loop single" mode, to make sure first track is playing when we start the 2nd one.
    streamer_set_repeat(DDB_REPEAT_SINGLE);

    playlist_t *plt = plt_alloc ("testplt");
    // create two test fake tracks
    switchtest_tracks[0] = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL);
    switchtest_tracks[1] = deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, switchtest_tracks[0], "/square.fake", NULL, NULL, NULL);

    plt_set_curr (plt);

    printf ("A:%p B:%p\n", switchtest_tracks[0], switchtest_tracks[1]);

    switchtest_counts[0] = switchtest_counts[1] = 0;

    fakein_set_sleep (100000);
    fakeout_set_manual (1);
    fakeout_set_realtime (1);

    printf ("start track A...\n");
    streamer_set_nextsong (0, 0);
    streamer_yield ();

    printf ("consume 1 sec...\n");
    fakeout_consume (44100 * 4 * 2);

    printf ("start track B...\n");
    streamer_set_nextsong (1, 0);
    streamer_yield ();

    // we're testing that track A is never "playing" after this point
    _trackinfochanged_handler = switchtest_trackinfochanged_handler;

    printf ("consume 1 sec...\n");
    fakeout_consume (44100 * 4 * 2);
    fakeout_set_manual (0);

    _trackinfochanged_handler = NULL;

    plt_set_curr (NULL);
    deadbeef->plt_unref ((ddb_playlist_t *)plt);

    EXPECT_TRUE(switchtest_counts[0] == 0);
    EXPECT_TRUE(count_played = 2);
}

TEST_F(StreamerTests, test_nextTrack_currentWasDeleted_picksNextTrack) {
    streamer_set_repeat(DDB_REPEAT_OFF);
    streamer_set_shuffle(DDB_SHUFFLE_OFF);
    ddb_playlist_t *plt = deadbeef->plt_alloc ("testplt");

    ddb_playItem_t *tracks[5] = {
        deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL),
        deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL),
        deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL),
        deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL),
        deadbeef->plt_insert_file2 (0, (ddb_playlist_t *)plt, NULL, "/sine.fake", NULL, NULL, NULL),
    };

    fakein_set_sleep (0);
    fakeout_set_manual (1);
    fakeout_set_realtime (0);

    deadbeef->plt_set_curr (plt);

    streamer_set_nextsong (2, 0);
    streamer_yield ();
    fakeout_consume (44100 * 4 * 2);

    ddb_playItem_t *curr = deadbeef->streamer_get_streaming_track();
    EXPECT_EQ(curr, tracks[2]);

    deadbeef->plt_remove_item(plt, tracks[2]);

    streamer_move_to_nextsong(0);
    streamer_yield ();
    fakeout_consume (44100 * 4 * 2);

    curr = deadbeef->streamer_get_streaming_track();

    EXPECT_EQ(deadbeef->plt_get_item_count(plt, PL_MAIN), 4);
    EXPECT_EQ(curr, tracks[1]); // the list is reversed

    deadbeef->pl_item_unref(curr);

    plt_set_curr (NULL);
    deadbeef->plt_unref(plt);
}

