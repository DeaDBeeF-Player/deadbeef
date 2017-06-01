//
//  StreamerTest.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 5/31/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "deadbeef.h"
#include "playlist.h"
#include "plugins.h"
#include "conf.h"
#include "../../common.h"
#include "logger.h"
#include "streamer.h"
#include "threading.h"
#include "messagepump.h"

static int count_played = 0;

// super oversimplified mainloop
static void
mainloop (void) {
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
                        streamer_set_nextsong (p1);
                        break;
                    case DB_EV_STOP:
                        streamer_set_nextsong (-1);
                        break;
                    case DB_EV_NEXT:
                        streamer_move_to_nextsong (1);
                        break;
                    case DB_EV_PREV:
                        streamer_move_to_prevsong (1);
                        break;
                    case DB_EV_PAUSE:
                        if (output->state () != OUTPUT_STATE_PAUSED) {
                            output->pause ();
                            messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                        }
                        break;
                    case DB_EV_TOGGLE_PAUSE:
                        if (output->state () != OUTPUT_STATE_PLAYING) {
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
    BOOL finished = NO;

    while (!finished) {
        playItem_t *streaming_track = streamer_get_streaming_track();
        playItem_t *playing_track = streamer_get_playing_track();
        if (!streaming_track && !playing_track) {
            finished = YES;
        }
        if (streaming_track) {
            pl_item_unref (streaming_track);
        }
        if (playing_track) {
            pl_item_unref(playing_track);
        }
    }
}

@interface StreamerTest : XCTestCase {
    DB_plugin_t *_fakein;
    DB_output_t *_fakeout;
    uintptr_t _mainloop_tid;
}

@end

@implementation StreamerTest

- (void)setUp {
    [super setUp];

    // init deadbeef core
    NSString *resPath = [[NSBundle bundleForClass:[self class]] resourcePath];
    const char *str = [resPath UTF8String];
    strcpy (dbplugindir, str);

    ddb_logger_init ();
    conf_init ();
    conf_enable_saving (0);

    pl_init ();

    messagepump_init ();

    // register fakein and fakeout plugin
    extern DB_plugin_t * fakein_load (DB_functions_t *api);
    extern DB_plugin_t * fakeout_load (DB_functions_t *api);

    plug_init_plugin (fakein_load, NULL);
    plug_init_plugin (fakeout_load, NULL);

    _fakein = fakein_load (plug_get_api ());
    _fakeout = (DB_output_t *)fakeout_load (plug_get_api ());
    plug_register_in (_fakein);
    plug_register_out ((DB_plugin_t *)_fakeout);

    plug_set_output (_fakeout);

    streamer_init ();

    conf_set_int ("playback.loop", 1);
    count_played = 0;

    _mainloop_tid = thread_start (mainloop, NULL);
}

- (void)tearDown {
    deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
    thread_join (_mainloop_tid);

    _fakeout->stop ();
    streamer_free ();

    plug_disconnect_all ();
    plug_unload_all ();
    pl_free ();
    conf_free ();
    ddb_logger_free ();

    [super tearDown];
}

- (void)test_Play2TracksNoLoop_Sends2SongChanged {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    // create two test fake tracks
    DB_playItem_t *_sinewave = deadbeef->plt_insert_file2 (0, plt, NULL, "sine.fake", NULL, NULL, NULL);
    DB_playItem_t *_squarewave = deadbeef->plt_insert_file2 (0, plt, _sinewave, "square.fake", NULL, NULL, NULL);

    streamer_set_nextsong (0);
    streamer_yield ();

    wait_until_stopped ();

    deadbeef->pl_item_unref (_sinewave);
    deadbeef->pl_item_unref (_squarewave);
    deadbeef->plt_unref (plt);
    XCTAssert (count_played = 2);
}

@end
