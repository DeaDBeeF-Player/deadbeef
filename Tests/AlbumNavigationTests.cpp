//
//  AlbumNavigationTests.m
//  Tests
//
//  Created by Robin Ekman on 05/10/23.
//  Copyright © 2023 Robin Ekman. All rights reserved.
//

#include "conf.h"
#include "logger.h"
#include <deadbeef/common.h>
#include <deadbeef/deadbeef.h>
#include "conf.h"
#include "fakeout.h"
#include "messagepump.h"
#include "playlist.h"
#include "playmodes.h"
#include "plmeta.h"
#include "plugins.h"
#include "streamer.h"
#include "threading.h"
#include <stdio.h>
#include <gtest/gtest.h>

extern "C" DB_plugin_t *
fakeout_load (DB_functions_t *api);
// super oversimplified mainloop
static void
mainloop (void *ctx) {
    for (;;) {
        uint32_t msg;
        uintptr_t ctx;
        uint32_t p1;
        uint32_t p2;
        int term = 0;
        while (messagepump_pop (&msg, &ctx, &p1, &p2) != -1) {
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
                        streamer_set_nextsong (p1, 0);
                        break;
                    case DB_EV_STOP:
                        streamer_set_nextsong (-1, 0);
                        break;
                    case DB_EV_NEXT:
                        streamer_move_to_nextsong (1);
                        break;
                    case DB_EV_PREV:
                        streamer_move_to_prevsong (1);
                        break;
                    case DB_EV_PAUSE:
                        if (output->state () != DDB_PLAYBACK_STATE_PAUSED) {
                            output->pause ();
                            messagepump_push (DB_EV_PAUSED, 0, 1, 0);
                        }
                        break;
                    case DB_EV_TOGGLE_PAUSE:
                        if (output->state () != DDB_PLAYBACK_STATE_PLAYING) {
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
                    case DB_EV_SEEK: {
                        int32_t pos = (int32_t)p1;
                        if (pos < 0) {
                            pos = 0;
                        }
                        streamer_set_seek (p1 / 1000.f);
                    } break;
                    case DB_EV_SONGSTARTED:
                        break;
                    case DB_EV_TRACKINFOCHANGED:
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

static size_t
read_field (const char *line, char *buf) {
    // read a tab-separated field from line into buf
    // return the number of characters read including the tab
    const char *q = strchr (line, '\t');
    if (!q) {
        // this was the end of the line
        size_t n = strlen (line);
        // we don't want the terminating newline
        strncpy (buf, line, n - 1);
        buf[n - 1] = '\0';
        return n;
    }
    else {
        memcpy (buf, line, q - line);
        buf[q - line] = '\0';
        return q - line + 1;
    }
}

void
read_metadata (FILE *f, playItem_t *it) {
    // read three tab-separated pieces of metadata from f into it
    int p = 0;
    char linebuf[1024];
    char metabuf[1024];

    fgets (linebuf, sizeof (linebuf), f);
    p += read_field (linebuf, metabuf);
    pl_add_meta (it, "artist", metabuf);

    p += read_field (linebuf + p, metabuf);
    pl_add_meta (it, "album", metabuf);

    p += read_field (linebuf + p, metabuf);
    pl_add_meta (it, "title", metabuf);
}

void
advance_to (playItem_t *it) {
    pl_set_played (it, 1);
    streamer_set_last_played (it);
    pl_item_unref (it);
}

class AlbumNavigationTests : public ::testing::Test {
protected:
    void SetUp () override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);

        messagepump_init ();

        plug_init_plugin (fakeout_load, NULL);
        _fakeout = (DB_output_t *)fakeout_load (plug_get_api ());
        plug_register_out ((DB_plugin_t *)_fakeout);

        plug_set_output (_fakeout);

        streamer_init ();
        streamer_set_repeat (DDB_REPEAT_OFF);
        streamer_set_shuffle (DDB_SHUFFLE_OFF);
        plt = plt_alloc ("testplt");

        // read mock playlist from tab-separated file
        char path[PATH_MAX];
        snprintf (path, sizeof (path), "%s/TestData/AlbumNavigation.csv", dbplugindir);
        FILE *f = fopen (path, "r");
        int k = 0;
        playItem_t *prev = NULL;
        playItem_t *it;
        // the first field is the shufflerating of the track
        // this lets us know which track/album should be next in the shuffle
        // so we can write deterministic tests
        while (fscanf (f, "%d\t", srs + k) != EOF) {
            it = pl_item_alloc ();
            it->shufflerating = srs[k++];
            read_metadata (f, it);

            plt_insert_item (plt, prev, it);
            prev = it;
            pl_item_unref (it);
        }
        n_tracks = k;

        plt_set_curr (plt);
        streamer_set_streamer_playlist (plt);

        _mainloop_tid = thread_start (mainloop, NULL);
    }

    void reset_shuffle_ratings () {
        int n = 0;
        for (playItem_t *it = plt->head[PL_MAIN]; it && n < n_tracks; it = it->next[PL_MAIN]) {
            pl_set_played (it, 0);
            it->shufflerating = srs[n++];
        }
    }

    playItem_t *skip_tracks (unsigned int n, ddb_shuffle_t shuffle, ddb_repeat_t repeat, unsigned int ret) {
        // skip n tracks forward
        // if ret is truthy, return the new current track; otherwise return NULL
        playItem_t *it = NULL;
        for (int i = 0; i < n + 1; i++) {
            if (it) {
                pl_item_unref (it);
            }
            it = streamer_get_next_track_with_direction (1, shuffle, repeat);
            pl_set_played (it, 1);
            streamer_set_last_played (it);
        }
        if (ret) {
            return it;
        }
        else {
            pl_item_unref (it);
            return NULL;
        }
    }

    void TearDown () override {
        plt_set_curr (NULL);
        _fakeout->stop ();
        streamer_free ();
        deadbeef->sendmessage (DB_EV_TERMINATE, 0, 0, 0);
        thread_join (_mainloop_tid);
        messagepump_free ();
        conf_free ();
        ddb_logger_free ();
    }
    playlist_t *plt;
    int srs[128];
    int n_tracks;
    DB_output_t *_fakeout;
    uintptr_t _mainloop_tid;
};

// from_same_album tests

TEST_F (AlbumNavigationTests, test_TracksFromSameAlbum) {
    playlist_t *plt = plt_get_curr ();

    playItem_t *head = plt->head[PL_MAIN];
    playItem_t *next = head->next[PL_MAIN];
    EXPECT_TRUE (pl_items_from_same_album (head, next));

    plt_unref (plt);
}

TEST_F (AlbumNavigationTests, test_TracksNotFromSameAlbum) {
    playlist_t *plt = plt_get_curr ();

    playItem_t *head = plt->head[PL_MAIN];
    playItem_t *tail = plt->tail[PL_MAIN];
    EXPECT_FALSE (pl_items_from_same_album (head, tail));

    plt_unref (plt);
}

// SHUFFLE_ALBUMS tests

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsNextTrackStaysOnSameAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    playItem_t *it = skip_tracks (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 1);
    ASSERT_TRUE (it != NULL);
    printf ("%s\n", pl_find_meta_raw (it, "title"));

    EXPECT_TRUE (pl_items_from_same_album (plt->head[PL_MAIN], it));

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsPrevTrackStaysOnSameAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    playItem_t *it = skip_tracks (7, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 1);
    playItem_t *prev = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);

    EXPECT_TRUE (pl_items_from_same_album (it, prev));

    plt_unref (plt);
    if (it) {
        pl_item_unref (it);
    }
    if (prev) {
        pl_item_unref (prev);
    }
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsPrevTrackGoesToLastOnAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    playItem_t *it = skip_tracks (8, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 1);
    ASSERT_TRUE (it != NULL);
    playItem_t *prev = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (prev != NULL);
    const char *title = pl_find_meta_raw (prev, "title");

    EXPECT_FALSE (pl_items_from_same_album (it, prev));
    EXPECT_TRUE (pl_items_from_same_album (prev, plt->head[PL_MAIN]));
    EXPECT_STREQ (title, "Damage, Inc.");

    plt_unref (plt);
    pl_item_unref (it);
    pl_item_unref (prev);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumNextAlbumGoesToFirstTrack) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");

    EXPECT_STREQ (title, "Red");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsPrevAlbumGoesToFirstTrackPrevAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Red");
    streamer_set_last_played (it);

    playItem_t *prev = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (prev != NULL);

    title = pl_find_meta_raw (prev, "title");
    EXPECT_STREQ (title, "Battery");

    plt_unref (plt);
    pl_item_unref (it);
    pl_item_unref (prev);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsPrevAlbumGoesToFirstTrackCurrentAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (5, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *prev = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (prev != NULL);
    const char *title = pl_find_meta_raw (prev, "title");

    EXPECT_STREQ (title, "Battery");

    plt_unref (plt);
    if (prev) {
        pl_item_unref (prev);
    }
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsNextAlbumMarksPlayed) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    pl_item_unref (it);

    it = plt->head[PL_MAIN];
    while (it && pl_items_from_same_album (it, plt->head[PL_MAIN])) {
        EXPECT_TRUE (pl_get_played (it));
        it = it->next[PL_MAIN];
    }

    plt_unref (plt);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsPrevAlbumMarksNotPlayed) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    advance_to (it);

    it = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    pl_item_unref (it);

    it = plt->head[PL_MAIN];
    while (it && pl_items_from_same_album (it, plt->head[PL_MAIN])) {
        EXPECT_FALSE (pl_get_played (it));
        it = it->next[PL_MAIN];
    }

    plt_unref (plt);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsPrevTrackGoesToLastTrackPrevAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Red");
    streamer_set_last_played (it);

    playItem_t *prev = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (prev != NULL);

    title = pl_find_meta_raw (prev, "title");
    EXPECT_STREQ (title, "Damage, Inc.");

    plt_unref (plt);
    pl_item_unref (it);
    pl_item_unref (prev);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsNextAlbumPrevAlbumNextAlbum) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();

    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Red");

    advance_to (it);

    it = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Battery");

    advance_to (it);

    it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Red");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsRepeatOffNextAlbumStopsAfterAllAlbums) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();
    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF, 0);

    playItem_t *it = NULL;
    for (int i = 0; i < 3; i++) {
        it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_OFF);
        if (it) {
            advance_to (it);
        }
    }
    ASSERT_TRUE (it == NULL);

    plt_unref (plt);
}

TEST_F (AlbumNavigationTests, test_ShuffleAlbumsRepeatAllNextAlbumContinuesAfterAllAlbums) {
    playlist_t *plt = plt_get_curr ();
    reset_shuffle_ratings ();
    skip_tracks (0, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_ALL, 0);

    playItem_t *it = NULL;
    for (int i = 0; i < 3; i++) {
        it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_ALBUMS, DDB_REPEAT_ALL);
        if (it) {
            advance_to (it);
        }
    }
    ASSERT_TRUE (it != NULL);

    plt_unref (plt);
    pl_item_unref (it);
}

// SHUFFLE_OFF tests

TEST_F (AlbumNavigationTests, test_ShuffleOffNextAlbumGoesToFirstTrack) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffNextAlbumGoesToFirstTrackNoCurrentPlaying) {
    playlist_t *plt = plt_get_curr ();

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Battery");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffNextAlbumMarksPlayed) {
    playlist_t *plt = plt_get_curr ();

    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    playItem_t *prev = it->prev[PL_MAIN];
    pl_item_unref (it);
    while (prev != NULL) {
        ASSERT_TRUE (pl_get_played (prev));
        prev = prev->prev[PL_MAIN];
    }

    plt_unref (plt);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffPrevTrackGoesToLastTrackPrevAlbum) {
    playlist_t *plt = plt_get_curr ();

    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");
    streamer_set_last_played (it);

    playItem_t *prev = streamer_get_next_track_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (prev != NULL);

    title = pl_find_meta_raw (prev, "title");
    EXPECT_STREQ (title, "Damage, Inc.");

    plt_unref (plt);
    pl_item_unref (it);
    pl_item_unref (prev);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffPrevAlbumGoesToFirstTrackCurrentAlbum) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    advance_to (it);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");

    it = streamer_get_next_track_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    advance_to (it);
    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "2 Minutes to Midnight");

    it = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);

    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffPrevAlbumOnFirstTrackGoesToFirstTrackPrevAlbum) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    advance_to (it);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");

    it = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Battery");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffPrevAlbumWrapsAround) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Red");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffNextAlbumPrevAlbumNextAlbum) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    advance_to (it);
    const char *title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");

    it = streamer_get_next_album_with_direction (-1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    advance_to (it);
    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Battery");

    it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
    ASSERT_TRUE (it != NULL);
    title = pl_find_meta_raw (it, "title");
    EXPECT_STREQ (title, "Aces High");

    plt_unref (plt);
    pl_item_unref (it);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffRepeatOffNextAlbumStopsAtEndOfPlaylist) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF, 0);

    playItem_t *it = NULL;
    for (int i = 0; i < 3; i++) {
        it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_OFF);
        if (it) {
            advance_to (it);
        }
    }
    ASSERT_TRUE (it == NULL);

    plt_unref (plt);
}

TEST_F (AlbumNavigationTests, test_ShuffleOffRepeatAllNextAlbumWrapsAround) {
    playlist_t *plt = plt_get_curr ();
    skip_tracks (0, DDB_SHUFFLE_OFF, DDB_REPEAT_ALL, 0);

    playItem_t *it = NULL;
    for (int i = 0; i < 3; i++) {
        it = streamer_get_next_album_with_direction (1, DDB_SHUFFLE_OFF, DDB_REPEAT_ALL);
        if (it) {
            advance_to (it);
        }
    }
    ASSERT_TRUE (it != NULL);
    EXPECT_STREQ (pl_find_meta_raw (it, "title"), "Battery");

    plt_unref (plt);
    pl_item_unref (it);
}
