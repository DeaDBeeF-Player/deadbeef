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
}

- (void)tearDown {
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

    // subchunk: 7
    // off =

    mp4p_stco_entry_t stco_entries[] = {
        {.offset = 0},
        {.offset = 10000},
        {.offset = 20000},
        {.offset = 30000},
        {.offset = 40000},
        {.offset = 50000},
        {.offset = 60000},
        {.offset = 70000},
        {.offset = 80000},
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
    
    // this should find sample 0 in chunk 7
    uint64_t offs = mp4p_sample_offset(&stbl_atom, 20);
    
    XCTAssertEqual(offs, 70000);
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

- (void)test_AddTagsToAFileWithoutPadding_PaddingIsReduced {
    mp4p_atom_t *mp4file = mp4p_atom_new("moov");
    mp4p_atom_t *padding = mp4file->next = mp4p_atom_new("free");
    padding->size = 1024;
    mp4p_atom_t *mdat = padding->next = mp4p_atom_new("mdat");

    mdat->pos = padding->size;

    playItem_t *it = pl_item_alloc();
    pl_append_meta(it, "title", "Title");
    mp4p_atom_t *mp4file_tagged = mp4tagutil_modify_meta(mp4file, (DB_playItem_t *)it);
    pl_item_unref (it);

    XCTAssert(!mp4p_atom_type_compare(mp4file_tagged->next, "free"));
    XCTAssert(mp4file_tagged->next->size == 1024 - mp4file_tagged->size);

    mp4p_atom_free_list (mp4file_tagged);
    mp4p_atom_free_list (mp4file);
}

mp4p_atom_t *createFakeMP4File (void) {
    mp4p_atom_t *ftyp = mp4p_atom_new("ftyp");
    ftyp->size = 24;

    mp4p_atom_t *moov = ftyp->next = mp4p_atom_new("moov");
    moov->pos = 24;
    moov->size = 8;

    mp4p_atom_t *mdat = moov->next = mp4p_atom_new("mdat");
    mdat->pos = 32;
    mdat->size = 1008;
    return ftyp;
}

typedef struct {
    mp4p_file_callbacks_t cb;
    off_t pos;
    off_t size;
} fake_callbacks_t;

static ssize_t _fake_read (struct mp4p_file_callbacks_s *stream, void *ptr, size_t size) {
    fake_callbacks_t *cb = (fake_callbacks_t *)stream;
    cb->pos += size;
    return size;
}
static ssize_t _fake_write (struct mp4p_file_callbacks_s *stream, void *ptr, size_t size) {
    fake_callbacks_t *cb = (fake_callbacks_t *)stream;
    cb->pos += size;
    return size;
}
static off_t _fake_seek (struct mp4p_file_callbacks_s *stream, off_t offset, int whence) {
    fake_callbacks_t *cb = (fake_callbacks_t *)stream;
    switch (whence) {
        case SEEK_SET:
            cb->pos = offset;
            break;
        case SEEK_CUR:
            cb->pos += offset;
            break;
        case SEEK_END:
            cb->pos = cb->size + offset;
            break;
        default:
            return -1;
    }
    return cb->pos;
}

static off_t _fake_tell (struct mp4p_file_callbacks_s *stream) {
    fake_callbacks_t *cb = (fake_callbacks_t *)stream;
    return cb->pos;
}

static int _fake_truncate (struct mp4p_file_callbacks_s *stream, off_t length) {
    fake_callbacks_t *cb = (fake_callbacks_t *)stream;
    cb->size = length;
    return 0;
}

static fake_callbacks_t _fake_file_cb = {
    .cb.read = _fake_read,
    .cb.write = _fake_write,
    .cb.seek = _fake_seek,
    .cb.tell = _fake_tell,
    .cb.truncate = _fake_truncate,
};

- (void)test_SimulateWriteTags_GivesExpectedFileSize {
    mp4p_atom_t *mp4file = createFakeMP4File();

    fake_callbacks_t cb;
    memcpy (&cb, &_fake_file_cb, sizeof (fake_callbacks_t));

    cb.size = 1040;

    playItem_t *it = pl_item_alloc();

    // ftyp = 24
    //
    // moov = 8
    // udta = 8
    // meta = 8
    // hdlr = 33
    // ilst = 8
    // ©nam = 8
    // data = 16 + valuelength (5 bytes for Hello)
    // ------
    // total = 8+8+8+34+8+8+16+5 = 94
    //
    // + 1024 bytes `free`
    // + 1008 bytes `mdat`

    // we write one metadata atom, which will add
    pl_append_meta(it, "title", "Hello");

    mp4p_atom_t *mp4file_updated = mp4tagutil_modify_meta(mp4file, (DB_playItem_t *)it);
    pl_item_unref (it);

    int res = mp4p_update_metadata (&cb.cb, mp4file, mp4file_updated);

    mp4p_atom_free (mp4file);

    XCTAssert (!res);
    XCTAssertEqual(2150, cb.size);
}

- (void)test_ReadMP4Opus_GivesExpectedFormatData {
    char path[PATH_MAX];
    snprintf (path, sizeof (path), "%s/TestData/opus.mp4", dbplugindir);
    mp4p_file_callbacks_t *cb = mp4p_open_file_read (path);
    mp4p_atom_t *mp4file = mp4p_open (cb);
    mp4p_atom_t *opus = mp4p_atom_find(mp4file, "moov/trak/mdia/minf/stbl/stsd/Opus");
    XCTAssertFalse(opus == NULL);
    if (!opus) {
        mp4p_file_close (cb);
        return;
    }
    mp4p_Opus_t *Opus = opus->data;
    XCTAssertEqual(48000, Opus->sample_rate);
    mp4p_file_close (cb);
}

- (void)test_SttsSampleDuration_1EntryMultipleSamples_ReturnsCorrectDuration {
    mp4p_stts_entry_t entry = {
        .sample_count = 1000,
        .sample_duration = 50000
    };

    mp4p_stts_t stts = {
        .number_of_entries = 1,
        .entries = &entry
    };

    mp4p_atom_t atom = {
        .data = &stts
    };

    uint32_t mp4sample = mp4p_stts_sample_duration (&atom, 20);

    XCTAssertEqual (mp4sample, 50000);
}

- (void)test_SttsMP4SampleContainingSample_1EntryMultipleSamples_ReturnsCorrectMP4Sample {
    mp4p_stts_entry_t entry = {
        .sample_count = 1000,
        .sample_duration = 50000
    };

    mp4p_stts_t stts = {
        .number_of_entries = 1,
        .entries = &entry
    };

    mp4p_atom_t atom = {
        .data = &stts
    };

    int64_t startsample = 0;
    uint32_t mp4sample = mp4p_stts_mp4sample_containing_sample (&atom, 1000000, &startsample);

    XCTAssertEqual (mp4sample, 20);
    XCTAssertEqual (startsample, 1000000);
}

- (void)test_SttsMP4SampleContainingSample_2EntryMultipleSamples_ReturnsCorrectMP4Sample {
    mp4p_stts_entry_t entries[] = {
        {
            .sample_count = 10,
            .sample_duration = 50000
        },
        {
            .sample_count = 10,
            .sample_duration = 50000
        },
    };

    mp4p_stts_t stts = {
        .number_of_entries = 2,
        .entries = entries
    };

    mp4p_atom_t atom = {
        .data = &stts
    };

    int64_t startsample = 0;
    uint32_t mp4sample = mp4p_stts_mp4sample_containing_sample (&atom, 620000, &startsample);

    XCTAssertEqual (mp4sample, 12);
    XCTAssertEqual (startsample, 600000);
}

- (void)test_modifyMeta_none_positionUnchanged {
    mp4p_atom_t *mp4file = NULL;

    mp4p_atom_t *ftyp = mp4file = mp4p_atom_new("ftyp");
    mp4p_atom_t *moov = mp4file->next = mp4p_atom_new("moov");
    mp4p_atom_t *mdat = mp4file->next = mp4p_atom_new("mdat");
    

}

@end
