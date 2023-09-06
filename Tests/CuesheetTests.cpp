/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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

#include "conf.h"
#include "logger.h"
#include "plmeta.h"
#include <deadbeef/common.h>
#include <gtest/gtest.h>

class CuesheetTests: public ::testing::Test {
protected:

    void SetUp() override {
        ddb_logger_init ();
        conf_init ();
        conf_enable_saving (0);
    }

    void TearDown() override {
        conf_free();
        ddb_logger_free();
    }
};


TEST_F(CuesheetTests, testCueWithoutTitles) {
    const char cue[] =
        "FILE \"file.wav\" WAVE\n"
        "TRACK 01 AUDIO\n"
        "INDEX 01 00:00:00\n"
        "TRACK 02 AUDIO\n"
        "INDEX 01 05:50:65\n"
        "TRACK 03 AUDIO\n"
        "INDEX 01 09:47:50\n";

    playlist_t *plt = plt_alloc("test");

    playItem_t *it = pl_item_alloc_init ("testfile.flac", "stdflac");
    pl_add_meta (it, "cuesheet", cue);
    plt_process_cue(plt, NULL, it, 60*10*44100, 44100);

    int cnt = plt_get_item_count(plt, PL_MAIN);

    EXPECT_TRUE(cnt == 3);

    plt_free (plt);
}

TEST_F(CuesheetTests, test_BogusEmbeddedImageCueInSingleTrack_ReturnsSingleTrackWithCorrectMeta) {
    playlist_t *plt = plt_alloc("test");

    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/bogus_emb_cue.mp3", dbplugindir);

    playItem_t *it = plt_insert_file2(0, plt, NULL, path, NULL, NULL, NULL);

    EXPECT_NE(it, nullptr);
    EXPECT_EQ(plt_get_item_count(plt, PL_MAIN), 1);
    EXPECT_EQ(strcmp (pl_find_meta (it, "title"), "TrackTitle2"), 0);
    EXPECT_EQ(strcmp (pl_find_meta (it, "track"), "2"), 0);

    plt_free (plt);
}

TEST_F(CuesheetTests, test_ImageAndCue_Adds2TracksWithCorrectTitles) {
    playlist_t *plt = plt_alloc("test");

    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/image+cue", dbplugindir);

    plt_insert_dir2(0, plt, NULL, path, NULL, NULL, NULL);

    EXPECT_EQ(plt_get_item_count(plt, PL_MAIN), 2);
    EXPECT_EQ(strcmp (pl_find_meta (plt->head[PL_MAIN], "title"), "Test Track 01"), 0);
    EXPECT_EQ(strcmp (pl_find_meta (plt->head[PL_MAIN]->next[PL_MAIN], "title"), "Test Track 02"), 0);

    plt_free (plt);
}

TEST_F(CuesheetTests, test_CueWithTrackComposer_SetsCueComposerForOneTrack) {
    const char cue[] =
    "FILE \"file.wav\" WAVE\n"
    "TRACK 01 AUDIO\n"
    "REM COMPOSER \"TEST_COMPOSER\"\n"
    "INDEX 01 00:00:00\n"
    "TRACK 02 AUDIO\n"
    "INDEX 01 00:01:00\n";

    playlist_t *plt = plt_alloc("test");

    playItem_t *it = pl_item_alloc_init ("testfile.flac", "stdflac");
    pl_add_meta (it, "cuesheet", cue);
    plt_process_cue(plt, NULL, it, 60*10*44100, 44100);

    int cnt = plt_get_item_count(plt, PL_MAIN);

    EXPECT_EQ(cnt, 2);

    playItem_t *cueItem = plt_get_first(plt, PL_MAIN);

    const char *composer = pl_find_meta(cueItem, "composer");
    EXPECT_TRUE(composer);
    EXPECT_TRUE(!strcmp(composer, "TEST_COMPOSER"));

    const char *composer2 = pl_find_meta(cueItem->next[PL_MAIN], "composer");
    EXPECT_TRUE(composer2 == NULL);

    pl_item_unref(cueItem);
    plt_free (plt);
}

TEST_F(CuesheetTests, test_CueWithTrackGain_SetsCueTrackGainForOneTrack) {
    const char cue[] =
    "FILE \"file.wav\" WAVE\n"
    "TRACK 01 AUDIO\n"
    "REM COMPOSER \"TEST_COMPOSER\"\n"
    "REM REPLAYGAIN_TRACK_GAIN +1.00 dB\n"
    "INDEX 01 00:00:00\n"
    "TRACK 02 AUDIO\n"
    "INDEX 01 00:01:00\n";

    playlist_t *plt = plt_alloc("test");

    playItem_t *it = pl_item_alloc_init ("testfile.flac", "stdflac");
    pl_add_meta (it, "cuesheet", cue);
    plt_process_cue(plt, NULL, it, 60*10*44100, 44100);

    int cnt = plt_get_item_count(plt, PL_MAIN);

    EXPECT_EQ(cnt, 2);

    playItem_t *cueItem = plt_get_first(plt, PL_MAIN);

    const char *trackgain = pl_find_meta (cueItem, ":REPLAYGAIN_TRACKGAIN");
    EXPECT_TRUE(trackgain!=NULL);
    EXPECT_TRUE(!strcmp(trackgain, "1.00 dB"));

    pl_item_unref(cueItem);
    plt_free (plt);
}
