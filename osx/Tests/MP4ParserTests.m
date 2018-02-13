//
//  MP4Parser.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 27/12/2016.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#include "mp4parser.h"
#include "playlist.h"
#include "mp4tagutil.h"
#include "plugins.h"
#include "conf.h"
#include "../../common.h"
#include "logger.h"

@interface MP4Parser : XCTestCase

@end

@implementation MP4Parser

- (void)setUp {
    [super setUp];
    NSString *resPath = [[NSBundle bundleForClass:[self class]] resourcePath];
    const char *str = [resPath UTF8String];
    strcpy (dbplugindir, str);

    ddb_logger_init ();
    conf_init ();
    conf_enable_saving (0);

    pl_init ();
    if (plug_load_all ()) { // required to add files to playlist from commandline
        exit (-1);
    }
}

- (void)tearDown {
    plug_disconnect_all ();
    plug_unload_all ();
    pl_free ();
    conf_free ();
    ddb_logger_free ();

    [super tearDown];
}

- (void)test_findSample20PosInSimpleStreamWith1000ByteChunks_GivesPosition20000 {
    mp4p_stsc_entry_t entry = {
        1,1,1
    };
    mp4p_stsc_entry_t entries[1] = { entry };

    mp4p_stsc_t stsc = {
        .number_of_entries = 1,
        .entries = entries
    };

    mp4p_atom_t stsc_atom = {
        .type = "stsc",
        .data = &stsc
    };

    mp4p_stco_entry_t stco_entries[1] = {
        {.offset = 0}
    };

    mp4p_stco_t stco = {
        .number_of_entries = 1,
        .entries = stco_entries
    };

    mp4p_atom_t stco_atom = {
        .type = "stco",
        .data = &stco,
        .next = &stsc_atom
    };

    mp4p_stsz_t stsz = {
        .sample_size = 1000,
        .number_of_entries = 0,
        .entries = NULL
    };

    mp4p_atom_t stsz_atom = {
        .type = "stsz",
        .data = &stsz,
        .next = &stco_atom
    };

    mp4p_atom_t stbl_atom = {
        .type = "stbl",
        .subatoms = &stsz_atom
    };

    // this should find sample 0 in chunk 20
    uint64_t offs = mp4p_sample_offset(&stbl_atom, 20);

    XCTAssert(offs == 20000, @"Got %lld instead of expected 20000", offs);
}

- (void)test_findSample20PosInComplesStreamVariableChunks_GivesExpectedPosition {
    mp4p_stsc_entry_t entries[3] = {
        { 1, 5, 1 },
        { 3, 2, 2 },
        { 10, 3, 3 },
    };

    mp4p_stco_entry_t stco_entries[3] = {
        {.offset = 0},
        {.offset = 10000},
        {.offset = 24000},
    };

    mp4p_stsc_t stsc = {
        .number_of_entries = 3,
        .entries = entries
    };

    mp4p_atom_t stsc_atom = {
        .type = "stsc",
        .data = &stsc
    };

    mp4p_stco_t stco = {
        .number_of_entries = 1,
        .entries = stco_entries
    };

    mp4p_atom_t stco_atom = {
        .type = "stco",
        .data = &stco,
        .next = &stsc_atom
    };

    mp4p_stsz_t stsz = {
        .sample_size = 1000,
        .number_of_entries = 0,
        .entries = NULL
    };
    
    mp4p_atom_t stsz_atom = {
        .type = "stsz",
        .data = &stsz,
        .next = &stco_atom
    };
    
    mp4p_atom_t stbl_atom = {
        .type = "stbl",
        .subatoms = &stsz_atom
    };
    
    // this should find sample 0 in chunk 20
    uint64_t offs = mp4p_sample_offset(&stbl_atom, 20);
    
    XCTAssert(offs == 20000, @"Got %lld instead of expected 20000", offs);
}

- (void)test_WriteMoovWithMetadataToBuffer_WrittenSizeMatchingCalculatedSize {
    mp4p_atom_t *mp4file = mp4p_atom_new("moov");
    mp4file->next = mp4p_atom_new("mdat");

    playItem_t *it = pl_item_alloc();

    pl_append_meta(it, "title", "Title");
    pl_append_meta(it, "artist", "Artist");
    pl_append_meta(it, "album", "Album");
    pl_append_meta(it, "my custom tag 1", "custom tag value 1");
    pl_append_meta(it, "genre", "Folk");
    pl_append_meta(it, "track", "5");
    pl_append_meta(it, "numtracks", "15");
    pl_append_meta(it, "disc", "3");
    pl_append_meta(it, "numdiscs", "5");
    pl_append_meta(it, "my custom tag 2", "custom tag value 2");

    mp4p_atom_t *mp4file_tagged = mp4tagutil_modify_meta(mp4file, (DB_playItem_t *)it);

    pl_item_unref (it);
    mp4p_atom_dump (mp4file);

    uint32_t size = mp4p_atom_to_buffer (mp4file_tagged, NULL, 0);

    XCTAssert (size != 0);

    char *buffer = malloc (size);
    uint32_t written_size = mp4p_atom_to_buffer(mp4file_tagged, buffer, size);

    mp4p_atom_free_list (mp4file_tagged);
    mp4p_atom_free_list (mp4file);

    XCTAssert (written_size == size);
}

- (void)test_AddTagsToAFileWithoutPadding_AddsPaddingOf1024Bytes {
    mp4p_atom_t *mp4file = mp4p_atom_new("moov");
    mp4p_atom_t *mdat = mp4file->next = mp4p_atom_new("mdat");

    mdat->pos = 32; // arbitrary small position

    playItem_t *it = pl_item_alloc();
    pl_append_meta(it, "title", "Title");
    mp4p_atom_t *mp4file_tagged = mp4tagutil_modify_meta(mp4file, (DB_playItem_t *)it);
    pl_item_unref (it);

    XCTAssert(!mp4p_atom_type_compare(mp4file_tagged->next, "free"));
    XCTAssert(mp4file_tagged->next->size == 1024);

    mp4p_atom_free_list (mp4file_tagged);
    mp4p_atom_free_list (mp4file);
}

@end
