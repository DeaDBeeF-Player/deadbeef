//
//  M3UTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 07/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "m3u.h"
#include "../../common.h"

extern DB_functions_t *deadbeef;

@interface M3UTests : XCTestCase

@end

@implementation M3UTests

- (void)setUp {
    m3u_load(deadbeef);
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)test_loadM3UFromBuffer_SimplePlaylist_Loads2Items {
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

    XCTAssertTrue(after);
    XCTAssertEqual(deadbeef->plt_get_item_count(plt, PL_MAIN), 2);

    deadbeef->plt_unref (plt);
}
- (void)test_loadM3UFromBuffer_UnicodeCharactersNoDash_LoadsItemWithCorrectArtistTitle {
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

    XCTAssertTrue(after);
    XCTAssertEqual(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    XCTAssertTrue(!strcmp (title, "АБВГД"));
    XCTAssertTrue(artist == NULL);

    deadbeef->plt_unref (plt);
}

- (void)test_loadM3UFromBuffer_TrailingExtinf_LoadsItemWithCorrectArtistTitle {
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

    XCTAssertTrue(after);
    XCTAssertEqual(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    XCTAssertTrue(!strcmp (title, "АБВГД"));
    XCTAssertTrue(artist == NULL);

    deadbeef->plt_unref (plt);
}


- (void)test_loadM3UFromBuffer_UnicodeCharactersWithDash_LoadsItemWithCorrectArtistTitle {
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

    XCTAssertTrue(after);
    XCTAssertEqual(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    XCTAssertTrue(!strcmp (title, "Sample title"));
    XCTAssertTrue(!strcmp (artist, "АБВГД"));

    deadbeef->plt_unref (plt);
}

- (void)test_loadM3UFromBuffer_UnicodeBom_LoadsCorrectly {
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

    XCTAssertTrue(after);
    XCTAssertEqual(deadbeef->plt_get_item_count(plt, PL_MAIN), 1);

    const char *title = deadbeef->pl_find_meta(after, "title");
    const char *artist = deadbeef->pl_find_meta(after, "artist");

    XCTAssertTrue(!strcmp (title, "Sample title"));
    XCTAssertTrue(!strcmp (artist, "АБВГД"));

    deadbeef->plt_unref (plt);
}

@end
