//
//  M3UTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 07/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "../plugins/m3u/m3u.h"
#include <deadbeef/common.h>
#include <gtest/gtest.h>

extern DB_functions_t *deadbeef;

class M3UTests: public ::testing::Test {
protected:
    void SetUp() override {
        m3u_load(deadbeef);
    }
};

TEST_F(M3UTests, test_loadM3UFromBuffer_SimplePlaylist_Loads2Items) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    char m3u[1000];
    snprintf (m3u, sizeof (m3u),
              "#EXTM3U\n"
              "%s/TestData/chirp-1sec.mp3\n"
              "%s/TestData/comm_id3v2.3.mp3\n",
              dbplugindir, dbplugindir
              );

    ddb_playlist_t *plt = deadbeef->plt_alloc("plt");

    ddb_playItem_t *after = load_m3u_from_buffer(NULL, m3u, strlen(m3u), NULL, "", NULL, plt, NULL);

    EXPECT_TRUE(after);
    EXPECT_EQ(deadbeef->plt_get_item_count(plt, PL_MAIN), 2);

    deadbeef->plt_unref (plt);
}

TEST_F(M3UTests, test_loadM3UFromBuffer_UnicodeCharactersNoDash_LoadsItemWithCorrectArtistTitle) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    char m3u[1000];
    snprintf (m3u, sizeof (m3u),
              "#EXTM3U\n"
              "#EXTINF:123, АБВГД\n"
              "%s/TestData/chirp-1sec.mp3\n",
              dbplugindir
              );

    ddb_playlist_t *plt = deadbeef->plt_alloc("plt");

    ddb_playItem_t *after = load_m3u_from_buffer(NULL, m3u, strlen(m3u), NULL, "", NULL, plt, NULL);

    EXPECT_TRUE(after);
    EXPECT_EQ(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    EXPECT_TRUE(!strcmp (title, "АБВГД"));
    EXPECT_TRUE(artist == NULL);

    deadbeef->plt_unref (plt);
}

TEST_F(M3UTests, test_loadM3UFromBuffer_TrailingExtinf_LoadsItemWithCorrectArtistTitle) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    char m3u[1000];
    snprintf (m3u, sizeof (m3u),
              "#EXTM3U\n"
              "#EXTINF:123, АБВГД\n"
              "%s/TestData/chirp-1sec.mp3\n"
              "#EXTINF:123, test",
              dbplugindir
              );

    ddb_playlist_t *plt = deadbeef->plt_alloc("plt");

    ddb_playItem_t *after = load_m3u_from_buffer(NULL, m3u, strlen(m3u), NULL, "", NULL, plt, NULL);

    EXPECT_TRUE(after);
    EXPECT_EQ(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    EXPECT_TRUE(!strcmp (title, "АБВГД"));
    EXPECT_TRUE(artist == NULL);

    deadbeef->plt_unref (plt);
}


TEST_F(M3UTests, test_loadM3UFromBuffer_UnicodeCharactersWithDash_LoadsItemWithCorrectArtistTitle) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    char m3u[1000];
    snprintf (m3u, sizeof (m3u),
              "#EXTM3U\n"
              "#EXTINF:123, АБВГД - Sample title\n"
              "%s/TestData/chirp-1sec.mp3\n",
              dbplugindir
              );

    ddb_playlist_t *plt = deadbeef->plt_alloc("plt");

    ddb_playItem_t *after = load_m3u_from_buffer(NULL, m3u, strlen(m3u), NULL, "", NULL, plt, NULL);

    EXPECT_TRUE(after);
    EXPECT_EQ(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    EXPECT_TRUE(!strcmp (title, "Sample title"));
    EXPECT_TRUE(!strcmp (artist, "АБВГД"));

    deadbeef->plt_unref (plt);
}

TEST_F(M3UTests, test_loadM3UFromBuffer_UnicodeBom_LoadsCorrectly) {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/chirp-1sec.mp3", dbplugindir);

    char m3u[1000];
    snprintf (m3u, sizeof (m3u),
              "\xef\xbb\xbf#EXTM3U\n"
              "#EXTINF:123, АБВГД - Sample title\n"
              "%s/TestData/chirp-1sec.mp3\n",
              dbplugindir
              );

    ddb_playlist_t *plt = deadbeef->plt_alloc("plt");

    ddb_playItem_t *after = load_m3u_from_buffer(NULL, m3u, strlen(m3u), NULL, "", NULL, plt, NULL);

    EXPECT_TRUE(after);
    EXPECT_EQ(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    EXPECT_TRUE(!strcmp (title, "Sample title"));
    EXPECT_TRUE(!strcmp (artist, "АБВГД"));

    deadbeef->plt_unref (plt);
}

