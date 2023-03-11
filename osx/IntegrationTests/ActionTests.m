/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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
#import <deadbeef/common.h>
#import "conf.h"
#import "plugins.h"
#import "playlist.h"
#import "streamer.h"

@interface ActionTests : XCTestCase

// path to store temp files
@property (nonatomic) NSString *musicFilePath;
@property (nonatomic) XCTestExpectation *completionExpectation;

@end

@implementation ActionTests

- (void)setUp {
    conf_enable_saving(0);
    conf_remove_items("");

    self.musicFilePath = [NSString stringWithFormat:@"%@/ddbIntegrationTestTemp", NSTemporaryDirectory()];

    // change the configuration folder path
    snprintf (confdir, sizeof(confdir), "%s/config", self.musicFilePath.UTF8String);
    snprintf (dbconfdir, sizeof(dbconfdir), "%s/config/deadbeef", self.musicFilePath.UTF8String);

    // refresh UI
    [NSRunLoop.mainRunLoop runUntilDate:[NSDate.date dateByAddingTimeInterval:0.1]];
}


- (void)tearDown {
}

- (void)setupAction {
    // stop playback
    streamer_set_nextsong (-1, 0);
    streamer_yield();

    // remove all playlists except default
    int cnt = plt_get_count();
    while (cnt > 0) {
        plt_remove(cnt-1);
        cnt -= 1;
    }

    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [NSFileManager.defaultManager removeItemAtPath:self.musicFilePath error:nil];

    [NSFileManager.defaultManager createDirectoryAtPath:self.musicFilePath withIntermediateDirectories:YES attributes:nil error:nil];

    // create temp files
    NSString *testFile = [NSString stringWithFormat:@"%@/TestData/chirp-1sec.mp3", [NSBundle bundleForClass:self.class].resourcePath];

    // add items
    playlist_t *plt = plt_get_curr();
    for (int i = 0; i < 10; i++) {
        NSString *destPath = [NSString stringWithFormat:@"%@/%d.mp3", self.musicFilePath, i];
        [NSFileManager.defaultManager copyItemAtPath:testFile toPath:destPath error:nil];
        plt_add_file(plt, destPath.UTF8String, NULL, NULL);
    }
    plt_unref (plt);
}

- (void)test_executeAllActions_singlePlaylistWithMultipleItems_Completes {
    self.completionExpectation = [[XCTestExpectation alloc] initWithDescription:@"completion"];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        DB_plugin_t **plugins = plug_get_list();

        for (int i = 0; plugins[i]; i++) {
            DB_plugin_t *p = plugins[i];
            if (p->get_actions == NULL) {
                continue;
            }
            DB_plugin_action_t *actions = p->get_actions (NULL);
            dispatch_sync(dispatch_get_main_queue(), ^{
                [self setupAction];
            });

            [NSRunLoop.mainRunLoop runUntilDate:[NSDate.date dateByAddingTimeInterval:0.1]];

            for (DB_plugin_action_t *act = actions; act != NULL; act = act->next) {
                if (act->callback2 != NULL) {
                    act->callback2 (act, DDB_ACTION_CTX_MAIN);
                    act->callback2 (act, DDB_ACTION_CTX_SELECTION);
                    act->callback2 (act, DDB_ACTION_CTX_PLAYLIST);
                    act->callback2 (act, DDB_ACTION_CTX_NOWPLAYING);
                    printf ("action %s success\n", act->name);
                }
            }
        }
        [self.completionExpectation fulfill];
    });

    [self waitForExpectations:@[self.completionExpectation] timeout:5];
}

@end
