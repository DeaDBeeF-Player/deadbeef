//
//  PlaylistTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 24/10/2018.
//  Copyright © 2018 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#include <deadbeef/common.h>
#include "messagepump.h"
#include "plmeta.h"
#include "pltmeta.h"
#include "plugins.h"
#include "sort.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(PlaylistTests, test_SearchForValueInSingleValueItems_FindsTheItem) {
    playlist_t *plt = plt_alloc("test");
    playItem_t *it = pl_item_alloc();

    plt_insert_item(plt, NULL, it);

    pl_add_meta(it, "title", "value");

    plt_search_process(plt, "value");

    EXPECT_TRUE(plt->head[PL_SEARCH] != NULL);
    EXPECT_TRUE(plt->head[PL_MAIN]->selected);

    pl_item_unref(it);
    plt_unref (plt);
}


TEST(PlaylistTests, test_SearchFor2ndValueInMultiValueItems_FindsTheItem) {
    playlist_t *plt = plt_alloc("test");
    playItem_t *it = pl_item_alloc();

    plt_insert_item(plt, NULL, it);

    const char values[] = "value1\0value2\0";
    pl_add_meta_full(it, "title", values, sizeof(values));

    plt_search_process(plt, "value2");

    EXPECT_TRUE(plt->head[PL_SEARCH] != NULL);
    EXPECT_TRUE(plt->head[PL_MAIN]->selected);

    pl_item_unref(it);
    plt_unref (plt);
}

TEST(PlaylistTests, test_PlaylistSort) {
    messagepump_init();

    const char sort_tf[] = "%artist%";

    playlist_t *plt = plt_alloc("test");

    playItem_t *it_A = pl_item_alloc();
    playItem_t *it_B = pl_item_alloc();
    playItem_t *it_C = pl_item_alloc();

    pl_add_meta(it_A, "artist", "A");
    pl_add_meta(it_B, "artist", "B");
    pl_add_meta(it_C, "artist", "C");

    playItem_t *tail = plt_insert_item(plt, NULL, it_A);
    tail = plt_insert_item(plt, tail, it_C);
    tail = plt_insert_item(plt, tail, it_B);

    plt_sort_v2(plt, PL_MAIN, -1, sort_tf, DDB_SORT_ASCENDING);

    EXPECT_STREQ(plt_find_meta(plt, "autosort_tf"), sort_tf);

    playItem_t *head = plt_get_head_item(plt, PL_MAIN);
    ASSERT_STREQ("A", pl_find_meta_raw(head, "artist"));
    pl_item_unref(head);

    tail = plt_get_tail_item(plt, PL_MAIN);
    ASSERT_STREQ("C", pl_find_meta_raw(tail, "artist"));

    pl_item_unref(tail);

    pl_item_unref(it_A);
    pl_item_unref(it_B);
    pl_item_unref(it_C);

    plt_unref(plt);
}

TEST(PlaylistTests, test_PlaylistAutosort) {
    messagepump_init();

    const char sort_tf[] = "%artist%";

    playlist_t *plt = plt_alloc("test");
    plt_add_meta(plt, "autosort_enabled", "1");
    plt_add_meta(plt, "autosort_mode", "tf");
    plt_add_meta(plt, "autosort_tf", "%artist%");
    plt_add_meta(plt, "autosort_ascending", "1");

    playItem_t *it_A = pl_item_alloc();
    playItem_t *it_B = pl_item_alloc();
    playItem_t *it_C = pl_item_alloc();

    pl_add_meta(it_A, "artist", "A");
    pl_add_meta(it_B, "artist", "B");
    pl_add_meta(it_C, "artist", "C");

    playItem_t *tail = plt_insert_item(plt, NULL, it_A);
    tail = plt_insert_item(plt, tail, it_C);
    tail = plt_insert_item(plt, tail, it_B);

    plt_autosort(plt);

    EXPECT_STREQ(plt_find_meta(plt, "autosort_tf"), sort_tf);

    playItem_t *head = plt_get_head_item(plt, PL_MAIN);
    ASSERT_STREQ("A", pl_find_meta_raw(head, "artist"));
    pl_item_unref(head);

    tail = plt_get_tail_item(plt, PL_MAIN);
    ASSERT_STREQ("C", pl_find_meta_raw(tail, "artist"));

    pl_item_unref(tail);

    pl_item_unref(it_A);
    pl_item_unref(it_B);
    pl_item_unref(it_C);

    plt_unref(plt);
}

TEST (PlaylistTests, test_LoadDBPLWithRelativepaths) {
    using ::testing::StartsWith;
    ddb_playlist_t *plt = deadbeef->plt_alloc ("test");

    char dname[PATH_MAX];
    snprintf (dname, sizeof (dname), "%s/TestData", dbplugindir);
    char fname[] = "RelativePaths.dbpl";
    char plt_path[PATH_MAX];
    snprintf (plt_path, sizeof (plt_path), "%s/%s", dname, fname);

    ddb_playItem_t *after = deadbeef->plt_load2 (-1, plt, NULL, plt_path, NULL, NULL, NULL);

    ASSERT_TRUE (after);
    EXPECT_EQ (deadbeef->plt_get_item_count (plt, PL_MAIN), 8);

    ddb_playItem_t **its;
    size_t n_its = deadbeef->plt_get_items (plt, &its);
    for (size_t k = 0; k < n_its; k++) {
        EXPECT_THAT (deadbeef->pl_find_meta (its[k], ":URI"), StartsWith (dname));
        deadbeef->pl_item_unref (its[k]);
    }
    free (its);

    deadbeef->plt_unref (plt);
}

#pragma mark - IsRelativePathPosix

TEST(PlaylistTests, test_IsRelativePathPosix_AbsolutePath_False) {
    int res = is_relative_path_posix ("/path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_AbsolutePathWithUriScheme_False) {
    int res = is_relative_path_posix ("file:///path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_RelativePathWithUriScheme_True) {
    int res = is_relative_path_posix ("file://path");
    EXPECT_TRUE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_VFSPath_False) {
    int res = is_relative_path_posix ("zip://path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_HTTPPath_False) {
    int res = is_relative_path_posix ("http://path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_RelativePath_True) {
    int res = is_relative_path_posix ("path");
    EXPECT_TRUE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_RelativeWithFoldersPath_True) {
    int res = is_relative_path_posix ("path/filename");
    EXPECT_TRUE(res);
}

TEST(PlaylistTests, test_IsRelativePathPosix_WeirdPath_True) {
    int res = is_relative_path_posix ("something:something");
    EXPECT_TRUE(res);
}

#pragma mark - IsRelativePathWin32

TEST(PlaylistTests, test_IsRelativePathWin32_AbsolutePath_False) {
    int res = is_relative_path_win32 ("z:\\path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_AbsolutePathWithUriScheme_False) {
    int res = is_relative_path_win32 ("file://z:\\path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_AbsolutePathForwardSlash_False) {
    int res = is_relative_path_win32 ("z:/path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_AbsolutePathForwardSlashWithUriScheme_False) {
    int res = is_relative_path_win32 ("file://z:/path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_RelativePathWithUriScheme_True) {
    int res = is_relative_path_win32 ("file://path");
    EXPECT_TRUE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_VFSPath_False) {
    int res = is_relative_path_win32 ("zip://path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_HTTPPath_False) {
    int res = is_relative_path_win32 ("http://path");
    EXPECT_FALSE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_RelativePath_True) {
    int res = is_relative_path_win32 ("path");
    EXPECT_TRUE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_RelativeWithFoldersPath_True) {
    int res = is_relative_path_win32 ("path/filename");
    EXPECT_TRUE(res);
}

TEST(PlaylistTests, test_IsRelativePathWin32_WeirdPath_True) {
    int res = is_relative_path_win32 ("something:something");
    EXPECT_TRUE(res);
}
