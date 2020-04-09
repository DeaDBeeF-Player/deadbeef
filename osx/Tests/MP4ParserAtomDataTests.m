//
//  MP4ParserAtomDataTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 4/7/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "mp4patomdata.h"

@interface MP4ParserAtomDataTests : XCTestCase

@end

@implementation MP4ParserAtomDataTests

- (void)test_mvhdWriteRead_EqualOutput {
    mp4p_mvhd_t data = {
        .ch.version_flags = 0xaabbccdd,
        .creation_time = 0xddccbbaa,
        .modification_time = 0x11223344,
        .time_scale = 0x44332211,
        .duration = 0xeebbaadd,
        .preferred_rate = 0xddaabbee,
        .preferred_volume = 0x2345,
        .reserved = "10b_filler",
        .matrix_structure = "36 bytes filler1234567890abcdefghijk",
        .preview_time = 0x98765432,
        .preview_duration = 0x23456789,
        .poster_time = 0x56789abc,
        .selection_time = 0xcba98765,
        .selection_duration = 0xabcdef01,
        .current_time = 0x10fedcba,
        .next_track_id = 0x33337777,
    };


    size_t bufsize = mp4p_mvhd_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_mvhd_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_mvhd_t dataread;
    int res = mp4p_mvhd_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof (data)));
}

- (void)test_tkhdWriteRead_EqualOutput {
    mp4p_tkhd_t data = {
        .ch.version_flags = 0xaabbccdd,
        .creation_time = 0xddccbbaa,
        .modification_time = 0x11223344,
        .track_id = 0x44332211,
        .reserved = "3bf",
        .duration = 0xeebbaadd,
        .reserved2 = "abcghj7",
        .layer = 0x2345,
        .alternate_group = 0x1199,
        .volume = 0x5678,
        .reserved3 = "f",
        .matrix_structure = "36 bytes filler1234567890abcdefghij",
        .track_width = 0x74651029,
        .track_height = 0x56473829,
    };

    size_t bufsize = mp4p_tkhd_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_tkhd_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_tkhd_t dataread;
    int res = mp4p_tkhd_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof (data)));
}

- (void)test_mdhdWriteRead_EqualOutput {
    mp4p_mdhd_t data = {
        .ch.version_flags = 0xaabbccdd,
        .creation_time = 0xddccbbaa,
        .modification_time = 0x11223344,
        .time_scale = 0x44332211,
        .duration = 0xeebbaadd,
        .language = 0x3948,
        .quality = 0x8374,
    };

    size_t bufsize = mp4p_mdhd_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_mdhd_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_mdhd_t dataread;
    int res = mp4p_mdhd_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof (data)));
}

- (void)test_hdlrWriteRead_EqualOutput {
    mp4p_hdlr_t data = {
        .ch.version_flags = 0xaabbccdd,
        .component_type = "typ",
        .component_subtype = "sub",
        .component_manufacturer = "man",
        .component_flags = 0x16253498,
        .component_flags_mask = 0x47382910,
        .buf_len = 10,
        .buf = "10b_filler",
    };


    size_t bufsize = mp4p_hdlr_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_hdlr_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_hdlr_t *dataread = malloc (sizeof(mp4p_hdlr_t));
    int res = mp4p_hdlr_atomdata_read(dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (dataread, &data, 25));
    XCTAssert(!memcmp (dataread->buf, data.buf, data.buf_len));

    mp4p_hdlr_atomdata_free(dataread);
}

- (void)test_smhdWriteRead_EqualOutput {
    mp4p_smhd_t data = {
        .ch.version_flags = 0xaabbccdd,
        .balance = 0x1928,
        .reserved = 0x7465,
    };

    size_t bufsize = mp4p_smhd_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_smhd_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_smhd_t dataread;
    int res = mp4p_smhd_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof(data)));
}

- (void)test_stsdWriteRead_EqualOutput {
    mp4p_stsd_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 0x25364758,
    };

    size_t bufsize = mp4p_stsd_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_stsd_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_stsd_t dataread;
    int res = mp4p_stsd_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof(data)));
}

- (void)test_sttsWriteRead_EqualOutput {
    mp4p_stts_entry_t entries[3] = {
        { .sample_count=0x19283746, .sample_duration=0x56473829 },
        { .sample_count=0x13243546, .sample_duration=0x24354657 },
        { .sample_count=0x08978675, .sample_duration=0x75645342 },
    };
    mp4p_stts_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_stts_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_stts_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_stts_t *dataread = malloc(sizeof (mp4p_stts_t));
    int res = mp4p_stts_atomdata_read(dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (dataread, &data, 8));
    XCTAssert(!memcmp (dataread->entries, data.entries, 3*8));

    mp4p_stts_atomdata_free (dataread);
}

- (void)test_stscWriteRead_EqualOutput {
    mp4p_stsc_entry_t entries[3] = {
        { .first_chunk = 0x01928374, .samples_per_chunk=0x19283746, .sample_description_id=0x56473829 },
        { .first_chunk = 0x56473829, .samples_per_chunk=0x13243546, .sample_description_id=0x24354657 },
        { .first_chunk = 0x25364758, .samples_per_chunk=0x08978675, .sample_description_id=0x75645342 },
    };
    mp4p_stsc_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_stsc_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_stsc_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_stsc_t *dataread = malloc(sizeof (mp4p_stsc_t));
    int res = mp4p_stsc_atomdata_read(dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (dataread, &data, 8));
    XCTAssert(!memcmp (dataread->entries, data.entries, 3*8));

    mp4p_stsc_atomdata_free (dataread);
}

- (void)test_stszWriteRead_EqualOutput {
    mp4p_stsz_entry_t entries[3] = {
        { .sample_size = 0x01928374 },
        { .sample_size = 0x56473829 },
        { .sample_size = 0x25364758 },
    };
    mp4p_stsz_t data = {
        .ch.version_flags = 0xaabbccdd,
        .sample_size = 0x13243546,
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_stsz_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_stsz_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_stsz_t *dataread = malloc(sizeof (mp4p_stsz_t));
    int res = mp4p_stsz_atomdata_read(dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (dataread, &data, 8));
    XCTAssert(!memcmp (dataread->entries, data.entries, 3*8));

    mp4p_stsz_atomdata_free (dataread);
}

- (void)test_stcoWriteRead_EqualOutput {
    mp4p_stco_entry_t entries[3] = {
        { .offset = 0x01928374 },
        { .offset = 0x56473829 },
        { .offset = 0x25364758 },
    };
    mp4p_stco_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_stco_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_stco_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_stco_t *dataread = malloc(sizeof (mp4p_stco_t));
    int res = mp4p_stco_atomdata_read(dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (dataread, &data, 8));
    XCTAssert(!memcmp (dataread->entries, data.entries, 3*8));

    mp4p_stco_atomdata_free (dataread);
}

- (void)test_co64WriteRead_EqualOutput {
    mp4p_stco_entry_t entries[3] = {
        { .offset = 0x0192837456473829 },
        { .offset = 0x5647382925364758 },
        { .offset = 0x2536475801928374 },
    };
    mp4p_co64_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_co64_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_co64_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_co64_t *dataread = malloc(sizeof (mp4p_co64_t));
    int res = mp4p_co64_atomdata_read(dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (dataread, &data, 8));
    XCTAssert(!memcmp (dataread->entries, data.entries, 3*8));

    mp4p_co64_atomdata_free (dataread);
}

- (void)test_drefWriteRead_EqualOutput {
    mp4p_dref_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 0x19287564,
    };

    size_t bufsize = mp4p_dref_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_dref_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_dref_t dataread;
    int res = mp4p_dref_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof(data)));
}


- (void)test_alacWriteRead_EqualOutput {
    mp4p_alac_t data = {
        .reserved = "fillf",
        .data_reference_index = 0x1029,
        .reserved2 = "5647382",
        .reserved3 = "6",
        .asc_size = 24,
        .asc = "24bytefillabcdefghijklm"
    };

    size_t bufsize = mp4p_alac_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_alac_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_alac_t dataread;
    int res = mp4p_alac_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, 16));
    XCTAssert(!memcmp (dataread.asc, data.asc, data.asc_size));
}

- (void)test_mp4aWriteRead_EqualOutput {
    mp4p_mp4a_t data = {
        .reserved = "fillf",
        .data_reference_index = 0x7463,
        .reserved2 = "5647382",
        .channel_count = 0x2435,
        .bps = 0x7564,
        .packet_size = 0x3847,
        .sample_rate = 0x10293847,
        .reserved3 = "f"
    };

    size_t bufsize = mp4p_mp4a_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_mp4a_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_mp4a_t dataread;
    int res = mp4p_mp4a_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof(data)));
}

@end
