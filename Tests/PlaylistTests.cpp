//
//  PlaylistTests.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 24/10/2018.
//  Copyright © 2018 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#include <deadbeef/common.h>
#include "conf.h"
#include "logger.h"
#include "messagepump.h"
#include "plmeta.h"
#include "pltmeta.h"
#include "plugins.h"
#include "sort.h"
#include <string.h>
#include <string>
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
    EXPECT_STREQ("A", pl_find_meta_raw(head, "artist"));
    pl_item_unref(head);

    tail = plt_get_tail_item(plt, PL_MAIN);
    EXPECT_STREQ("C", pl_find_meta_raw(tail, "artist"));

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
    EXPECT_STREQ("A", pl_find_meta_raw(head, "artist"));
    pl_item_unref(head);

    tail = plt_get_tail_item(plt, PL_MAIN);
    EXPECT_STREQ("C", pl_find_meta_raw(tail, "artist"));

    pl_item_unref(tail);

    pl_item_unref(it_A);
    pl_item_unref(it_B);
    pl_item_unref(it_C);

    plt_unref(plt);
}

TEST(PlaylistTests, test_AutosortNewFiles) {
    messagepump_init();
    // -1 leaves the setting absent, exercising its default.
    const struct { int enabled; int exclude_added; int ascending; const char *expected; } cases[] = {
        {-1, -1, -1, "DBCA"}, // Autosort stays off by default.
        { 0,  0, -1, "DBCA"}, // Including additions cannot enable Autosort.
        { 0,  1, -1, "DBCA"},
        { 1, -1, -1, "DBAC"}, // Autosort includes new additions by default.
        { 1,  0, -1, "DBAC"},
        { 1,  1, -1, "DBCA"}, // Excluding additions leaves them in insertion order.
        { 1,  0,  0, "DBCA"}, // Honor the saved direction.
    };
    for (const auto &test : cases) {
        playlist_t *plt = plt_alloc("test");
        playlist_t *incoming = plt_alloc("incoming");
        if (test.enabled != -1) {
            plt_set_meta_int(plt, "autosort_enabled", test.enabled);
        }
        if (test.exclude_added != -1) {
            plt_set_meta_int(plt, "autosort_exclude_added", test.exclude_added);
        }
        plt_add_meta(plt, "autosort_mode", "tf");
        plt_add_meta(plt, "autosort_tf", "%path%");
        plt_set_meta_int(plt, "autosort_ascending", test.ascending == -1 ? 1 : test.ascending);
        playItem_t *items[5];
        const char *paths[] = {"/music/D", "/music/B", "/music/C", "/music/A", "/music/0"};
        for (int i = 0; i < 5; i++) {
            items[i] = pl_item_alloc();
            pl_add_meta(items[i], ":URI", paths[i]);
        }
        auto order = [&]() {
            std::string result;
            for (playItem_t *it = plt->head[PL_MAIN]; it; it = it->next[PL_MAIN]) {
                result += strrchr(pl_find_meta_raw(it, ":URI"), '/')[1];
            }
            return result;
        };
        plt_insert_item(plt, NULL, items[0]);
        plt_insert_item(plt, items[0], items[1]);
        EXPECT_EQ(0, plt_add_files_begin(plt, 0));
        // Re-inserting an existing track in place must not make it part of the new batch.
        uint32_t indices[] = {0};
        plt_move_items(plt, PL_MAIN, plt, items[1], indices, 1);
        plt_insert_item(incoming, NULL, items[2]);
        plt_insert_item(incoming, items[2], items[3]);
        plt_move_all_items(plt, incoming, items[1]);
        if (test.enabled == 1 && test.exclude_added == 1) {
            plt_autosort(plt); // The setting also applies to direct calls during an addition.
            EXPECT_EQ("DBCA", order());
        }
        plt_add_files_end(plt, 0);
        EXPECT_EQ(test.expected, order());
        if (test.enabled == 1 && test.exclude_added == 1) {
            plt_autosort(plt);
            EXPECT_EQ("ABCD", order()); // Exclusion affects additions, not the main Autosort API.
        }
        if (test.enabled == 1 && test.exclude_added == -1) {
            plt_move_items(plt, PL_MAIN, plt, items[1], indices, 1);
            EXPECT_EQ("DBAC", order()); // A same-playlist move does not trigger Autosort.
            // Later additions must leave the earlier batch alone.
            EXPECT_EQ(0, plt_add_files_begin(plt, 0));
            plt_insert_item(plt, plt->tail[PL_MAIN], items[4]);
            plt_add_files_end(plt, 0);
            EXPECT_EQ("DBAC0", order());
            // The main Autosort API still sorts the whole playlist outside an addition.
            plt_autosort(plt);
            EXPECT_EQ("0ABCD", order());
        }
        plt_unref(incoming);
        plt_unref(plt);
        for (auto item : items) {
            pl_item_unref(item);
        }
    }
}

TEST(PlaylistTests, test_MoveSelectedItemsToAnotherPlaylist_SelectionPlaybackTimeIsPreserved) {
    messagepump_init();

    playlist_t *from = plt_alloc("from");
    playlist_t *to = plt_alloc("to");

    playItem_t *it_A = pl_item_alloc();
    playItem_t *it_B = pl_item_alloc();

    playItem_t *tail = plt_insert_item(from, NULL, it_A);
    plt_set_item_duration(from, it_A, 10);
    tail = plt_insert_item(from, tail, it_B);
    plt_set_item_duration(from, it_B, 20);

    plt_select_all(from);
    EXPECT_EQ(30, plt_get_selection_playback_time(from));
    EXPECT_EQ(0, plt_get_selection_playback_time(to));

    uint32_t indexes[] = {0, 1};
    plt_move_items(to, PL_MAIN, from, NULL, indexes, 2);

    EXPECT_EQ(0, plt_get_selection_playback_time(from));
    EXPECT_EQ(30, plt_get_selection_playback_time(to));

    pl_item_unref(it_A);
    pl_item_unref(it_B);

    plt_unref(from);
    plt_unref(to);
}

class PlaylistAddFilesTests: public ::testing::Test {
protected:
    void SetUp() override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
    }

    void TearDown() override {
        conf_free ();
        ddb_logger_free ();
    }
};

TEST_F(PlaylistAddFilesTests, test_InsertStreamUrlWhileAddingFiles_GoesToTargetPlaylistNotAddFilesPlaylist) {
    messagepump_init();

    playlist_t *curr = plt_alloc("curr");
    playlist_t *scratch = plt_alloc("scratch");

    EXPECT_EQ(0, plt_add_files_begin(curr, 0));

    int abort = 0;
    playItem_t *it = plt_insert_file2(0, scratch, NULL, "http://example.com/stream", &abort, NULL, NULL);
    EXPECT_NE(nullptr, it);

    plt_add_files_end(curr, 0);

    EXPECT_EQ(1, plt_get_item_count(scratch, PL_MAIN));
    EXPECT_EQ(0, plt_get_item_count(curr, PL_MAIN));

    plt_unref(scratch);
    plt_unref(curr);
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

class PlaylistAddTests: public ::testing::Test {
protected:
    void SetUp() override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
        messagepump_init ();
        // plt_remove never removes the last remaining playlist, make sure there's one to keep
        if (plt_get_count () == 0) {
            plt_add2 (0, "Baseline");
        }
        _initialCount = plt_get_count ();
    }

    void TearDown() override {
        // remove all playlists added by the test
        while (plt_get_count () > _initialCount) {
            int count = plt_get_count ();
            plt_remove (count - 1);
            if (plt_get_count () == count) {
                break;
            }
        }
        conf_free ();
        ddb_logger_free ();
    }

    int _initialCount = 0;

    static std::string
    TitleAt (int idx) {
        playlist_t *plt = plt_get_for_idx (idx);
        char title[100];
        plt_get_title (plt, title, sizeof (title));
        plt_unref (plt);
        return title;
    }
};

TEST_F(PlaylistAddTests, test_AddWithTitle_UsesTitleAsIs) {
    int idx = plt_add2 (plt_get_count (), "My Playlist");
    EXPECT_EQ ("My Playlist", TitleAt (idx));

    int idx2 = plt_add2 (plt_get_count (), "My Playlist");
    EXPECT_EQ ("My Playlist", TitleAt (idx2));
}

TEST_F(PlaylistAddTests, test_AddWithNullTitle_GeneratesUniqueTitles) {
    int idx1 = plt_add2 (plt_get_count (), NULL);
    int idx2 = plt_add2 (plt_get_count (), NULL);
    int idx3 = plt_add2 (plt_get_count (), NULL);

    EXPECT_EQ ("New Playlist", TitleAt (idx1));
    EXPECT_EQ ("New Playlist (1)", TitleAt (idx2));
    EXPECT_EQ ("New Playlist (2)", TitleAt (idx3));
}

TEST_F(PlaylistAddTests, test_AddWithNullTitle_ExistingTitleDiffersByCase_GeneratesUniqueTitle) {
    plt_add2 (plt_get_count (), "new playlist");
    int idx = plt_add2 (plt_get_count (), NULL);

    EXPECT_EQ ("New Playlist (1)", TitleAt (idx));
}

TEST_F(PlaylistAddTests, test_AddWithNullTitle_FreedTitleIsReused) {
    plt_add2 (plt_get_count (), NULL);
    int idx2 = plt_add2 (plt_get_count (), NULL);
    plt_remove (idx2);

    int idx3 = plt_add2 (plt_get_count (), NULL);
    EXPECT_EQ ("New Playlist (1)", TitleAt (idx3));
}
