//
//  mp4patomdata.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/7/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef mp4patomdata_h
#define mp4patomdata_h

struct mp4p_atom_s;

typedef struct {
    uint32_t version_flags; // uint8 version and uint24 flags
} mp4p_common_header_t;

typedef struct {
    char major_brand[4];
    char version[4];
    char compat_brand_1[4];
    char compat_brand_2[4];
} mp4p_mtyp_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t time_scale;
    uint32_t duration;
    uint32_t preferred_rate;
    uint16_t preferred_volume;
    uint8_t reserved[10];
    uint8_t matrix_structure[36];
    uint32_t preview_time;
    uint32_t preview_duration;
    uint32_t poster_time;
    uint32_t selection_time;
    uint32_t selection_duration;
    uint32_t current_time;
    uint32_t next_track_id;
} mp4p_mvhd_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t track_id;
    uint8_t reserved[4];
    uint32_t duration;
    uint8_t reserved2[8];
    uint16_t layer;
    uint16_t alternate_group;
    uint16_t volume;
    uint8_t reserved3[2];
    uint8_t matrix_structure[36];
    uint32_t track_width;
    uint32_t track_height;
} mp4p_tkhd_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t time_scale;
    uint32_t duration;
    uint16_t language;
    uint16_t quality;
} mp4p_mdhd_t;

typedef struct {
    mp4p_common_header_t ch;
    char component_type[4];
    char component_subtype[4];
    char component_manufacturer[4];
    uint32_t component_flags;
    uint32_t component_flags_mask;
    uint16_t buf_len;
    char *buf;
} mp4p_hdlr_t;

typedef struct {
    mp4p_common_header_t ch;
    uint16_t balance;
    uint16_t reserved;
} mp4p_smhd_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
} mp4p_stsd_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t sample_count;
    uint32_t sample_duration;
} mp4p_stts_entry_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
    mp4p_stts_entry_t *entries;
} mp4p_stts_t;

typedef struct {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t sample_description_id;
} mp4p_stsc_entry_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
    mp4p_stsc_entry_t *entries;
} mp4p_stsc_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t sample_size;
} mp4p_stsz_entry_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t sample_size;
    uint32_t number_of_entries;
    mp4p_stsz_entry_t *entries;
} mp4p_stsz_t;

typedef struct {
    uint64_t offset;
} mp4p_stco_entry_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
    mp4p_stco_entry_t *entries;
} mp4p_stco_t;

typedef mp4p_stco_t mp4p_co64_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
} mp4p_dref_t;

typedef struct {
    uint8_t reserved[6];
    uint16_t data_reference_index;
    uint8_t reserved2[8];
    uint8_t *asc;
    // end of actual atom data, begin of parsed values

    uint32_t asc_size;

    uint16_t channel_count;
    uint16_t bps;
    uint16_t packet_size;
    uint32_t sample_rate;
    uint8_t reserved3[2];
} mp4p_alac_t;

typedef struct {
    uint8_t reserved[6];
    uint16_t data_reference_index;
    uint8_t reserved2[8];
    uint16_t channel_count;
    uint16_t bps;
    uint16_t packet_size;
    uint32_t sample_rate;
    uint8_t reserved3[2];
} mp4p_mp4a_t;


// opus encapsulated in mp4 https://vfrmaniac.fushizen.eu/contents/opus_in_isobmff.html
typedef struct {
    uint8_t reserved[6];
    uint16_t data_reference_index;
    uint8_t reserved2[8];
    uint16_t channel_count;
    uint16_t bps;
    uint16_t packet_size;
    uint32_t sample_rate;
    uint8_t reserved3[2];
    // followed by dOps
} mp4p_Opus_t;

typedef struct {
    uint8_t stream_count;
    uint8_t coupled_count;
    uint8_t *channel_mapping; // [output_channel_count]
} mp4p_opus_channel_mapping_table_t;

typedef struct {
    uint8_t version; // 0
    uint8_t output_channel_count;
    uint16_t pre_skip;
    uint32_t input_sample_rate;
    int16_t output_gain;
    uint8_t channel_mapping_family;
    mp4p_opus_channel_mapping_table_t *channel_mapping_table; // [output_channel_count] if channel_mapping_family!=0
} mp4p_dOps_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t es_tag;
    uint32_t es_tag_size;
    uint8_t ignored1;
    uint8_t ignored2;
    uint8_t ignored3;
    uint8_t dc_tag;
    uint32_t dc_tag_size;
    uint8_t dc_audiotype;
    uint8_t dc_audiostream;
    uint8_t dc_buffersize_db[3];
    uint32_t dc_max_bitrate;
    uint32_t dc_avg_bitrate;
    uint32_t ds_tag;

    uint32_t asc_size;
    char *asc;
} mp4p_esds_t;

typedef struct {
    mp4p_common_header_t ch;
} mp4p_meta_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t data_size;
    uint64_t data_offset;
    char *name;
    char *text;
    uint16_t *values;
} mp4p_ilst_meta_t;

typedef struct {
    mp4p_common_header_t ch;
    uint8_t nchapters;
    int64_t *start;
    uint8_t *name_len;
    char **name;
} mp4p_chpl_t;

typedef struct {
    uint32_t count;
    uint32_t *track_id;
} mp4p_chap_t;


// mvhd
int
mp4p_mvhd_atomdata_read (mp4p_mvhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_mvhd_atomdata_write (mp4p_mvhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_mvhd_atomdata_free (void *atom_data);

// tkhd
int
mp4p_tkhd_atomdata_read (mp4p_tkhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_tkhd_atomdata_write (mp4p_tkhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_tkhd_atomdata_free (void *atom_data);

// mdhd
int
mp4p_mdhd_atomdata_read (mp4p_mdhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_mdhd_atomdata_write (mp4p_mdhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_mdhd_atomdata_free (void *atom_data);

// hdlr
int
mp4p_hdlr_atomdata_read (mp4p_hdlr_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_hdlr_atomdata_write (mp4p_hdlr_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_hdlr_atomdata_free (void *atom_data);
void
mp4p_hdlr_init (struct mp4p_atom_s *hdlr_atom, const char *type, const char *subtype, const char *manufacturer);

// smhd
int
mp4p_smhd_atomdata_read (mp4p_smhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_smhd_atomdata_write (mp4p_smhd_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_smhd_atomdata_free (void *atom_data);

// stsd
int
mp4p_stsd_atomdata_read (mp4p_stsd_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_stsd_atomdata_write (mp4p_stsd_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_stsd_atomdata_free (void *atom_data);

// stts
int
mp4p_stts_atomdata_read (mp4p_stts_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_stts_atomdata_write (mp4p_stts_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_stts_atomdata_free (void *data);

// stsc
int
mp4p_stsc_atomdata_read (mp4p_stsc_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_stsc_atomdata_write (mp4p_stsc_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_stsc_atomdata_free (void *data);

// stsz
int
mp4p_stsz_atomdata_read (mp4p_stsz_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_stsz_atomdata_write (mp4p_stsz_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_stsz_atomdata_free (void *data);

// stco
int
mp4p_stco_atomdata_read (mp4p_stco_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_stco_atomdata_write (mp4p_stco_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_stco_atomdata_free (void *data);

// co64
int
mp4p_co64_atomdata_read (mp4p_co64_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_co64_atomdata_write (mp4p_co64_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_co64_atomdata_free (void *data);

// dref
int
mp4p_dref_atomdata_read (mp4p_dref_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_dref_atomdata_write (mp4p_dref_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_dref_atomdata_free (void *data);

// alac
int
mp4p_alac_atomdata_read (mp4p_alac_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_alac_atomdata_write (mp4p_alac_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_alac_atomdata_free (void *data);

// mp4a
int
mp4p_mp4a_atomdata_read (mp4p_mp4a_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_mp4a_atomdata_write (mp4p_mp4a_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_mp4a_atomdata_free (void *data);

// Opus
int
mp4p_Opus_atomdata_read (mp4p_Opus_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_Opus_atomdata_write (mp4p_Opus_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_Opus_atomdata_free (void *data);

// dOps
int
mp4p_dOps_atomdata_read (mp4p_dOps_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_dOps_atomdata_write (mp4p_dOps_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_dOps_atomdata_free (void *data);

// esds
int
mp4p_esds_atomdata_read (mp4p_esds_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_esds_atomdata_write (mp4p_esds_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_esds_atomdata_free (void *data);

// chpl
int
mp4p_chpl_atomdata_read (mp4p_chpl_t *atom_data, uint8_t *buffer, size_t buffer_size);
size_t
mp4p_chpl_atomdata_write (mp4p_chpl_t *atom_data, uint8_t *buffer, size_t buffer_size);
void
mp4p_chpl_atomdata_free (void *data);

#endif /* mp4patomdata_h */
