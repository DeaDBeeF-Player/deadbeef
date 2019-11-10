//
//  VfsCurlTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 11/10/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "vfs_curl.h"
#include "playlist.h"

extern DB_functions_t *deadbeef;

@interface VfsCurlTests : XCTestCase {
    HTTP_FILE *_file;
}

@end

@implementation VfsCurlTests

- (void)setUp {
    extern DB_plugin_t *vfs_curl_load (DB_functions_t *api);
    vfs_curl_load (deadbeef);

    _file = calloc (sizeof (HTTP_FILE), 1);
    _file->track = (DB_playItem_t *)pl_item_alloc ();
}

- (void)tearDown {
    vfs_curl_free_file(_file);
}

#pragma mark - In-stream headers

- (void)test_HandleIcyHeaders_IcyHeaderData_ConsumedUnterminated {
    char *data = "ICY 200 OK";
    size_t consumed = vfs_curl_handle_icy_headers (strlen(data), _file, data);
    XCTAssertEqual (consumed, strlen (data));
    XCTAssertEqual (_file->icyheader, 1);
    XCTAssertEqual (_file->gotheader, 0);
}

- (void)test_HandleIcyHeaders_IcyHeaderData_IcyHeaderFieldTrueTerminated {
    char *data = "ICY 200 OK\r\n\r\n";
    size_t consumed = vfs_curl_handle_icy_headers (strlen(data), _file, data);
    XCTAssertEqual (consumed, strlen (data));
    XCTAssertEqual (_file->icyheader, 1);
    XCTAssertEqual (_file->gotheader, 1);
}

- (void)test_HandleIcyHeaders_NonIcyHeaderData_NotConsumed {
    char *data = "Garbage";
    size_t consumed = vfs_curl_handle_icy_headers (strlen(data), _file, data);
    XCTAssertEqual (consumed, 0);
    XCTAssertEqual (_file->icyheader, 0);
    XCTAssertEqual (_file->gotheader, 1);
}

- (void)test_HandleIcyHeaders_IcyHeaderDataFollowedByOtherData_OnlyHeaderIsConsumed {
    char *data = "ICY 200 OK\r\n\r\nData";
    size_t consumed = vfs_curl_handle_icy_headers (strlen(data), _file, data);
    XCTAssertEqual (consumed, strlen(data)-4);
    XCTAssertEqual (_file->icyheader, 1);
    XCTAssertEqual (_file->gotheader, 1);
}

- (void)test_HandleIcyHeaders_IcyHeaderTitleMeta_GotTitleMeta {
    char *data = "ICY 200 OK\r\nicy-name:Title\r\n\r\n";
    size_t consumed = vfs_curl_handle_icy_headers (strlen(data), _file, data);
    XCTAssertEqual (consumed, strlen (data));
    XCTAssertEqual (_file->icyheader, 1);
    XCTAssertEqual (_file->gotheader, 1);

    const char *title = pl_find_meta((playItem_t *)_file->track, "title");
    XCTAssertEqual (strcmp (title, "Title"), 0);
}

- (void)test_HandleIcyHeaders_IcyHeaderAndTitleMetaSeparatePackets_GotTitleMeta {
    char *header = "ICY 200 OK\r\n";
    char *payload = "icy-name:Title\r\n\r\n";
    size_t consumed = vfs_curl_handle_icy_headers (strlen(header), _file, header);
    XCTAssertEqual (consumed, strlen (header));
    consumed = vfs_curl_handle_icy_headers (strlen(payload), _file, payload);
    XCTAssertEqual (consumed, strlen (payload));
    XCTAssertEqual (_file->icyheader, 1);
    XCTAssertEqual (_file->gotheader, 1);

    const char *title = pl_find_meta((playItem_t *)_file->track, "title");
    XCTAssertNotEqual (title, NULL);
    XCTAssertEqual (strcmp (title, "Title"), 0);
}

@end
