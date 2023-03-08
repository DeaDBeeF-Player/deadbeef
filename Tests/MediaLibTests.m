//
//  MediaLibTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 11/07/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include <deadbeef/common.h>
#include "medialib.h"
#include "plugins.h"

@interface MediaLibTests : XCTestCase

@property (nonatomic) DB_mediasource_t *plugin;
@property (nonatomic) ddb_medialib_plugin_api_t *medialib;
@property (nonatomic) XCTestExpectation *scanCompletedExpectation;
@property (nonatomic) int waitCount;

@end

@implementation MediaLibTests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
    self.plugin = (DB_mediasource_t *)plug_get_for_id("medialib");
    self.medialib = (ddb_medialib_plugin_api_t *)self.plugin->get_extended_api();
    self.scanCompletedExpectation = [[XCTestExpectation alloc] initWithDescription:@"Scan completed"];
    self.waitCount = 0;
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

static void
_listener(ddb_mediasource_event_type_t event, void *user_data) {
    // The DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE should trigger twice:
    // on the initial load, and on scan completion.
    // Wait for the 2nd event.
    if (event == DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE) {
        MediaLibTests *self = (__bridge MediaLibTests *)(user_data);
        self.waitCount += 1;
        if (self.waitCount == 2) {
            [self.scanCompletedExpectation fulfill];
        }
    }
}

- (void)test_Scan_MultiArtistSingleTrack_1TrackInLibrary {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/MediaLibrary/MultiArtist", dbplugindir);
    const char *folders[] = { path };

    ddb_mediasource_source_t *source;
    source = self.plugin->create_source("IntegrationTest");
    self.medialib->enable_file_operations(source, 0);
    self.plugin->add_listener(source, _listener, (__bridge void *)(self));
    self.medialib->set_folders(source, folders, 1);
    self.plugin->refresh(source);

    [self waitForExpectations:@[self.scanCompletedExpectation] timeout:5];

    ddb_medialib_item_t *tree = self.plugin->create_item_tree(source, (ddb_mediasource_source_t)2, NULL); // FIXME: hardcoded selector

    int count = 0;
    const ddb_medialib_item_t *children = self.plugin->tree_item_get_children(tree);
    for (const ddb_medialib_item_t *child = children; child; child = self.plugin->tree_item_get_next(child), count += 1);
    self.plugin->free_item_tree(source, tree);

    self.plugin->free_source(source);

    XCTAssertEqual(count, 1);
}

@end
