//
//  VfsCurlTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 11/10/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#include "../plugins/vfs_curl/vfs_curl.h"
#include "messagepump.h"
#include "plmeta.h"
#include <gtest/gtest.h>

extern "C" DB_functions_t *deadbeef;
extern "C" DB_plugin_t *
vfs_curl_load (DB_functions_t *api);

class VfsCurlTests : public ::testing::Test {
protected:
    void SetUp () override {
        messagepump_init ();
        vfs_curl_load (deadbeef);

        _file = (HTTP_FILE *)calloc (1, sizeof (HTTP_FILE));
        _file->track = (DB_playItem_t *)pl_item_alloc ();
    }
    void TearDown () override {
        vfs_curl_free_file (_file);
        // drain
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        while (messagepump_pop (&msg, &ctx, &p1, &p2) != -1) {
            if (msg >= DB_EV_FIRST && ctx) {
                messagepump_event_free ((ddb_event_t *)ctx);
            }
        }
        messagepump_free ();
    }
    HTTP_FILE *_file;
};

#pragma mark - In-stream headers

TEST_F (VfsCurlTests, test_HandleIcyHeaders_IcyHeaderData_ConsumedUnterminated) {
    const char *data = "ICY 200 OK";
    size_t consumed = vfs_curl_handle_icy_headers (strlen (data), _file, data);
    EXPECT_EQ (consumed, strlen (data));
    EXPECT_EQ (_file->icyheader, 1);
    EXPECT_EQ (_file->gotheader, 0);
}

TEST_F (VfsCurlTests, test_HandleIcyHeaders_IcyHeaderData_IcyHeaderFieldTrueTerminated) {
    const char *data = "ICY 200 OK\r\n\r\n";
    size_t consumed = vfs_curl_handle_icy_headers (strlen (data), _file, data);
    EXPECT_EQ (consumed, strlen (data));
    EXPECT_EQ (_file->icyheader, 1);
    EXPECT_EQ (_file->gotheader, 1);
}

TEST_F (VfsCurlTests, test_HandleIcyHeaders_NonIcyHeaderData_NotConsumed) {
    const char *data = "Garbage";
    size_t consumed = vfs_curl_handle_icy_headers (strlen (data), _file, data);
    EXPECT_EQ (consumed, 0);
    EXPECT_EQ (_file->icyheader, 0);
    EXPECT_EQ (_file->gotheader, 1);
}

TEST_F (VfsCurlTests, test_HandleIcyHeaders_IcyHeaderDataFollowedByOtherData_OnlyHeaderIsConsumed) {
    const char *data = "ICY 200 OK\r\n\r\nData";
    size_t consumed = vfs_curl_handle_icy_headers (strlen (data), _file, data);
    EXPECT_EQ (consumed, strlen (data) - 4);
    EXPECT_EQ (_file->icyheader, 1);
    EXPECT_EQ (_file->gotheader, 1);
}

TEST_F (VfsCurlTests, test_HandleIcyHeaders_IcyHeaderTitleMeta_GotTitleMeta) {
    const char *data = "ICY 200 OK\r\nicy-name:Title\r\n\r\n";
    size_t consumed = vfs_curl_handle_icy_headers (strlen (data), _file, data);
    EXPECT_EQ (consumed, strlen (data));
    EXPECT_EQ (_file->icyheader, 1);
    EXPECT_EQ (_file->gotheader, 1);

    const char *title = pl_find_meta ((playItem_t *)_file->track, "title");
    EXPECT_EQ (strcmp (title, "Title"), 0);
}

TEST_F (VfsCurlTests, test_HandleIcyHeaders_IcyHeaderAndTitleMetaSeparatePackets_GotTitleMeta) {
    const char *header = "ICY 200 OK\r\n";
    const char *payload = "icy-name:Title\r\n\r\n";
    size_t consumed = vfs_curl_handle_icy_headers (strlen (header), _file, header);
    EXPECT_EQ (consumed, strlen (header));
    consumed = vfs_curl_handle_icy_headers (strlen (payload), _file, payload);
    EXPECT_EQ (consumed, strlen (payload));
    EXPECT_EQ (_file->icyheader, 1);
    EXPECT_EQ (_file->gotheader, 1);

    const char *title = pl_find_meta ((playItem_t *)_file->track, "title");
    EXPECT_NE (title, nullptr);
    EXPECT_EQ (strcmp (title, "Title"), 0);
}
