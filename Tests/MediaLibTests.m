//
//  MediaLibTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 11/07/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include <deadbeef/common.h>
#include "conf.h"
#include "logger.h"
#include "medialib.h"
#include "plugins.h"
#include "scriptable/scriptable.h"
#include "scriptable_tfquery.h"

@interface MediaLibTests : XCTestCase

@property (nonatomic) DB_mediasource_t *plugin;
@property (nonatomic) ddb_medialib_plugin_api_t *medialib;
@property (nonatomic) XCTestExpectation *scanCompletedExpectation;
@property (nonatomic) int waitCount;
@property (nonatomic) ddb_mediasource_source_t *source;

@end

@implementation MediaLibTests

- (void)setUp {
    ddb_logger_init ();
    conf_init ();
    conf_enable_saving (0);
    self.plugin = (DB_mediasource_t *)plug_get_for_id("medialib");
    self.medialib = (ddb_medialib_plugin_api_t *)self.plugin->get_extended_api();
}

- (void)tearDown {
    self.plugin->free_source(self.source);

    conf_free();
    ddb_logger_free();
}

static void
_listener(ddb_mediasource_event_type_t event, void *user_data) {
    // The DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE should trigger multiple times:
    // 1. initial setup (enable source)
    // 2. playlist load
    // 3. scan completion
    // We need to wait for the scan completion, so countint to 3.
    if (event == DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE) {
        MediaLibTests *self = (__bridge MediaLibTests *)(user_data);
        self.waitCount += 1;
        if (self.waitCount == 3) {
            [self.scanCompletedExpectation fulfill];
        }
    }
}

- (void)test_Scan_MultiArtistSingleTrack_1TrackInLibrary {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/MediaLibrary/MultiArtist", dbplugindir);
    const char *folders[] = { path };

    self.scanCompletedExpectation = [[XCTestExpectation alloc] initWithDescription:@"Scan completed"];
    self.waitCount = 0;

    conf_set_int("medialib.IntegrationTest.enabled", 0);
    self.source = self.plugin->create_source("IntegrationTest");
    self.plugin->add_listener(self.source, _listener, (__bridge void *)(self));
    self.medialib->enable_file_operations(self.source, 0);
    self.plugin->set_source_enabled(self.source, 1);

    self.medialib->set_folders(self.source, folders, 1);
    self.plugin->refresh(self.source);

    [self waitForExpectations:@[self.scanCompletedExpectation] timeout:5];

    scriptableItem_t *root = scriptableTFQueryRootCreate ();
    ml_scriptable_init(deadbeef, self.plugin, root);

    ddb_medialib_item_t *tree = self.plugin->create_item_tree(self.source, scriptableItemSubItemForName(root, "Genres"), NULL);

    int count = 0;
    const ddb_medialib_item_t *children = self.plugin->tree_item_get_children(tree);
    for (const ddb_medialib_item_t *child = children; child; child = self.plugin->tree_item_get_next(child), count += 1);
    self.plugin->free_item_tree(self.source, tree);

    scriptableItemFree(root);

    XCTAssertEqual(count, 1);
}

@end
