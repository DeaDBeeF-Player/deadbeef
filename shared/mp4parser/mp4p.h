#ifndef mp4parser_h
#define mp4parser_h

#include <stdint.h>
#include "mp4pfile.h"

typedef struct mp4p_atom_s {
    uint64_t pos;
    uint32_t size;
    char type[4];
    void *data;
    struct mp4p_atom_s *subatoms;
    struct mp4p_atom_s *next;

    void (*free) (void *data);

    // if to_buffer is null, data must point to a plain buffer of size-8 bytes, that can be saved directly
    uint32_t (*to_buffer) (struct mp4p_atom_s *atom, uint8_t *buffer, uint32_t buffer_size);

    // Special case for `meta` atom, which has both the common header and subatoms.
    // For this case, the version_flags is stored in data, and is written before subatoms.
    unsigned write_data_before_subatoms : 1;
} mp4p_atom_t;

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
    mp4p_common_header_t ch;
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

// NOTE: reused for stco and co64
typedef struct {
    uint64_t offset;
} mp4p_stco_entry_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
    mp4p_stco_entry_t *entries;
} mp4p_stco_t;

typedef struct {
    mp4p_common_header_t ch;
    uint32_t number_of_entries;
} mp4p_dref_t;

typedef struct {
    uint8_t reserved[6];
    uint16_t data_reference_index;
    uint8_t reserved2[8];
    uint16_t channel_count;
    uint16_t bps;
    uint16_t packet_size;
    uint32_t sample_rate;
    uint8_t reserved3[2];

    uint32_t asc_size;
    uint8_t *asc;
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
    uint8_t dc_audiotype;
    uint8_t dc_audiostream;
    uint8_t dc_buffersize_db[3];
    uint32_t dc_max_bitrate;
    uint32_t dc_avg_bitrate;

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
    char    **name;
    int64_t  *start;
} mp4p_chpl_t;

typedef struct {
    uint32_t count;
    uint32_t *track_id;
} mp4p_chap_t;

void
mp4p_atom_free (mp4p_atom_t *atom);

void
mp4p_atom_free_list (mp4p_atom_t *atom);

// Loading starts from the current position in the stream.
mp4p_atom_t *
mp4p_open (mp4p_file_callbacks_t *callbacks);

mp4p_atom_t *
mp4p_atom_find (mp4p_atom_t *root, const char *path);

uint64_t
mp4p_stts_total_num_samples (mp4p_atom_t *stts_atom);

uint32_t
mp4p_stts_sample_duration (mp4p_atom_t *stts_atom, uint32_t mp4sample);

uint32_t
mp4p_stts_mp4sample_containing_sample (mp4p_atom_t *stts_atom, uint64_t sample, uint64_t *mp4sample_startingsample);

uint64_t
mp4p_stts_total_sample_duration (mp4p_atom_t *stts_atom);

uint32_t
mp4p_sample_size (mp4p_atom_t *stsz_atom, uint32_t sample);

uint64_t
mp4p_sample_offset (mp4p_atom_t *stbl_atom, uint32_t sample);

const char *
mp4p_genre_name_for_index (uint16_t index);

void
mp4p_atom_remove_sibling(mp4p_atom_t *atom, mp4p_atom_t *sibling);

void
mp4p_atom_remove_subatom (mp4p_atom_t *atom, mp4p_atom_t *subatom);

mp4p_atom_t *
mp4p_atom_new (const char *type);

mp4p_atom_t *
mp4p_atom_clone (mp4p_atom_t *src);

void
mp4p_atom_update_size (mp4p_atom_t *atom);

void
mp4p_rebuild_positions (mp4p_atom_t *atom, uint64_t init_pos);

mp4p_atom_t *
mp4p_atom_insert (mp4p_atom_t *parent, mp4p_atom_t *before, mp4p_atom_t *atom);

mp4p_atom_t *
mp4p_atom_append (mp4p_atom_t *parent, mp4p_atom_t *atom);

mp4p_atom_t *
mp4p_ilst_append_genre (mp4p_atom_t *ilst_atom, const char *text);

mp4p_atom_t *
mp4p_ilst_append_track_disc (mp4p_atom_t *ilst_atom, const char *type, uint16_t index, uint16_t total);

mp4p_atom_t *
mp4p_ilst_append_text (mp4p_atom_t *ilst_atom, const char *type, const char *text);

mp4p_atom_t *
mp4p_ilst_append_custom (mp4p_atom_t *ilst_atom, const char *name, const char *text);

void
mp4p_atom_dump (mp4p_atom_t *atom);

void
mp4p_hdlr_init (mp4p_atom_t *hdlr_atom, const char *type, const char *subtype, const char *manufacturer);

int
mp4p_atom_type_compare (mp4p_atom_t *atom, const char *type);

int
mp4p_fourcc_compare (const char *value1, const char *value2);

uint32_t
mp4p_atom_to_buffer (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size);

int
mp4p_update_metadata (mp4p_file_callbacks_t *callbacks, mp4p_atom_t *source, mp4p_atom_t *dest);

int
mp4p_trak_playable (mp4p_atom_t *trak_atom);

int
mp4p_trak_has_chapters (mp4p_atom_t *trak_atom);

void
mp4p_dbg_dump_atom (mp4p_atom_t *atom);

#endif /* mp4parser_h */
