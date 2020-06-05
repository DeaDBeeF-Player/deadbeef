//
//  MP4ParserAtomDataTests.m
//  Tests
//
//  Created by Alexey Yakovenko on 4/7/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "mp4p.h"

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

- (void)test_OpusWriteRead_EqualOutput {
    mp4p_Opus_t data = {
        .reserved = "fillf",
        .data_reference_index = 0x7463,
        .reserved2 = "5647382",
        .channel_count = 0x2435,
        .bps = 16,
        .packet_size = 0x3847,
        .sample_rate = 48000,
        .reserved3 = "f"
    };

    size_t bufsize = mp4p_Opus_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_Opus_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_Opus_t dataread;
    int res = mp4p_Opus_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, sizeof(data)));
}

- (void)test_dOpsWriteRead_EqualOutput {
    uint8_t channel_mapping[2] = {0x47,0x83};
    mp4p_opus_channel_mapping_table_t channel_mapping_table[2] = {
        {
            .stream_count = 0x45,
            .coupled_count = 0x87,
            .channel_mapping = channel_mapping,
        },
        {
            .stream_count = 0x45,
            .coupled_count = 0x87,
            .channel_mapping = channel_mapping,
        }
    };
    mp4p_dOps_t data = {
        .version = 0,
        .output_channel_count = 2,
        .pre_skip = 0x1928,
        .input_sample_rate = 0x46576879,
        .output_gain = 0x4857,
        .channel_mapping_family = 0x37,
        .channel_mapping_table = channel_mapping_table,
    };

    size_t bufsize = mp4p_dOps_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_dOps_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_dOps_t dataread;
    int res = mp4p_dOps_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, 11));

    for (int i = 0; i < data.output_channel_count; i++) {
        XCTAssert(!memcmp (&dataread.channel_mapping_table[i], &data.channel_mapping_table[i], 2));
        XCTAssert(!memcmp (dataread.channel_mapping_table[i].channel_mapping, data.channel_mapping_table[i].channel_mapping, data.output_channel_count));
    }
}

- (void)test_esdsWriteRead_EqualOutput {
    mp4p_esds_t data = {
        .ch.version_flags = 0xaabbccdd,
        .es_tag = 3,
        .es_tag_size = 0x0384756,
        .ignored1 = 0x91,
        .ignored2 = 0x72,
        .dc_tag = 4,
        .dc_tag_size = 0x06473829,
        .dc_audiotype = 0x78,
        .dc_audiostream = 0x91,
        .dc_buffersize_db = "fi",
        .dc_max_bitrate = 0x15263748,
        .dc_avg_bitrate = 0x96857463,
        .ds_tag = 5,
        .asc_size = 24,
        .asc = "24bytefillabcdefghijklm",
        .remainder_size = 6,
        .remainder = "ghjklm"
    };

    size_t bufsize = mp4p_esds_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_esds_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_esds_t dataread = {0};
    int res = mp4p_esds_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    size_t offs = offsetof(mp4p_esds_t, asc);
    XCTAssert(!memcmp (&dataread, &data, offs));
    XCTAssert(!memcmp (dataread.asc, data.asc, data.asc_size));
}

- (void)test_writeReadEsdsTagSize_EqualOutput {
    uint32_t input = 0x00bbccdd;
    uint8_t buffer[4];

    int
    write_esds_tag_size (uint8_t *buffer, size_t buffer_size, uint32_t num);
    int
    read_esds_tag_size (uint8_t *buffer, size_t buffer_size, uint32_t *retval);

    int res = write_esds_tag_size(buffer, 4, input);
    XCTAssertEqual(res,4);

    uint32 output = 0;
    res = read_esds_tag_size(buffer, 4, &output);
    XCTAssertEqual(res,4);

    XCTAssertEqual(input,output);
}

- (void)test_chplWriteRead_EqualOutput {
    mp4p_chpl_entry_t entries[3] = {
        {
            .start_time = 0,
            .name_len = 9,
            .name = "Chapter 1"
        },
        {
            .start_time = 1,
            .name_len = 9,
            .name = "Chapter 2"
        },
        {
            .start_time = 2,
            .name_len = 9,
            .name = "Chapter 3"
        }
    };

    mp4p_chpl_t data = {
        .ch.version_flags = 0xaabbccdd,
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_chpl_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_chpl_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_chpl_t dataread = {0};
    int res = mp4p_chpl_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!memcmp (&dataread, &data, 5));

    for (int i = 0; i < 3; i++) {
        XCTAssertEqual(dataread.entries[i].start_time, data.entries[i].start_time);
        XCTAssertEqual(dataread.entries[i].name_len, data.entries[i].name_len);
        XCTAssert(!memcmp (dataread.entries[i].name, data.entries[i].name, data.entries[i].name_len));
    }
}

- (void)test_chapWriteRead_EqualOutput {
    uint32_t entries[3] = { 0x19283746, 0x56473829, 0x15263748 };
    mp4p_chap_t data = {
        .number_of_entries = 3,
        .entries = entries,
    };

    size_t bufsize = mp4p_chap_atomdata_write(&data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_chap_atomdata_write(&data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_chap_t dataread;
    int res = mp4p_chap_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssertEqual(dataread.number_of_entries, data.number_of_entries);
    XCTAssert(!memcmp (dataread.entries, data.entries, 12));
}

- (void)test_textMetaWriteRead_EqualOutput {
    mp4p_atom_t *meta_atom = mp4p_ilst_create_text ("Type", "Value");
    mp4p_ilst_meta_t *data = meta_atom->data;

    size_t bufsize = mp4p_ilst_meta_atomdata_write(data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_ilst_meta_atomdata_write(data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_ilst_meta_t dataread = {0};
    int res = mp4p_ilst_meta_atomdata_read(&dataread, buffer, bufsize);
    XCTAssertEqual(res, 0);

    XCTAssert(!dataread.custom);
    XCTAssertEqual(dataread.data_size, 5);
    XCTAssertEqual(dataread.data_version_flags, 1);
    XCTAssert(!strcmp(dataread.text, "Value"));
}

- (void)test_trknMetaReadWrite_EqualOutput {
    mp4p_atom_t *meta_atom = mp4p_ilst_create_track_disc ("trkn", 5, 8);
    mp4p_ilst_meta_t *data = meta_atom->data;

    size_t bufsize = mp4p_ilst_meta_atomdata_write(data, NULL, 0);
    uint8_t *buffer = malloc (bufsize);
    size_t writtensize = mp4p_ilst_meta_atomdata_write(data, buffer, bufsize);
    XCTAssertEqual (bufsize, writtensize);

    mp4p_ilst_meta_t dataread = {0};
    int res = mp4p_ilst_meta_atomdata_read(&dataread, buffer, bufsize);
    XCTAssert(!res);

    XCTAssert(!dataread.custom);
    XCTAssertEqual(dataread.data_size, 8);
    XCTAssertEqual(dataread.data_version_flags, 0);

    uint16_t *values = data->values;

    XCTAssertEqual(values[0], 0);
    XCTAssertEqual(values[1], 5);
    XCTAssertEqual(values[2], 8);
    XCTAssertEqual(values[3], 0);
}


@end
