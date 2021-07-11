//
//  MediaLibTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 11/07/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "../../common.h"
#include "medialib.h"
#include "plugins.h"

@interface MediaLibTests : XCTestCase

@property (nonatomic) ddb_medialib_plugin_t *plugin;
@property (nonatomic) XCTestExpectation *scanCompletedExpectation;

@end

@implementation MediaLibTests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.
    self.plugin = (ddb_medialib_plugin_t *)plug_get_for_id("medialib");
    self.scanCompletedExpectation = [[XCTestExpectation alloc] initWithDescription:@"Scan completed"];
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

static void
_listener(ddb_mediasource_event_type_t event, void *user_data) {
    if (event == DDB_MEDIASOURCE_EVENT_SCAN_DID_COMPLETE) {
        MediaLibTests *self = (__bridge MediaLibTests *)(user_data);
        [self.scanCompletedExpectation fulfill];
    }
}

- (void)test_Scan_MultiArtistSingleTrack_1TrackInLibrary {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/MediaLibrary/MultiArtist", dbplugindir);
    const char *folders[] = { path };

    ddb_mediasource_source_t *source;
    source = self.plugin->plugin.create_source("IntegrationTest");
    self.plugin->enable_file_operations(source, 0);
    self.plugin->plugin.add_listener(source, _listener, (__bridge void *)(self));
    self.plugin->set_folders(source, folders, 1);
    self.plugin->plugin.refresh(source);

    [self waitForExpectations:@[self.scanCompletedExpectation] timeout:5];

    ddb_medialib_item_t *tree = self.plugin->plugin.create_item_tree(source, (ddb_mediasource_source_t)2, NULL); // FIXME: hardcoded selector

    int count = 0;
    for (ddb_medialib_item_t *child = tree->children; child; child = child->next, count += 1);
    self.plugin->plugin.free_item_tree(source, tree);

    self.plugin->plugin.free_source(source);

    XCTAssertEqual(count, 1);
}

@end
