#ifndef mp4parser_h
#define mp4parser_h

#include <stdint.h>

typedef struct mp4p_atom_s {
    uint64_t pos;
    uint32_t size;
    char type[4];
    void *data;
    struct mp4p_atom_s *subatoms;
    struct mp4p_atom_s *next;

    void (*free) (void *data);
} mp4p_atom_t;

typedef struct {
    char major_brand[4];
    char version[4];
    char compat_brand_1[4];
    char compat_brand_2[4];
} mp4p_mtyp_t;

typedef struct {
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
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t time_scale;
    uint32_t duration;
    uint16_t language;
    uint16_t quality;
} mp4p_mdhd_t;

typedef struct {
    uint32_t component_type;
    uint32_t component_subtype;
    uint32_t component_manufacturer;
    uint32_t component_flags;
    uint32_t component_flags_mask;
    char *buf;
} mp4p_hdlr_t;

typedef struct {
    uint16_t balance;
} mp4p_smhd_t;

typedef struct {
    uint32_t number_of_entries;
} mp4p_stsd_t;

typedef struct {
    uint32_t sample_count;
    uint32_t sample_duration;
} mp4p_stts_entry_t;

typedef struct {
    uint32_t number_of_entries;
    mp4p_stts_entry_t *entries;
} mp4p_stts_t;

typedef struct {
    uint32_t first_chunk;
    uint32_t samples_per_chunk;
    uint32_t sample_description_id;
} mp4p_stsc_entry_t;

typedef struct {
    uint32_t number_of_entries;
    mp4p_stsc_entry_t *entries;
} mp4p_stsc_t;

typedef struct {
    uint32_t sample_size;
} mp4p_stsz_entry_t;

typedef struct {
    uint32_t sample_size;
    uint32_t number_of_entries;
    mp4p_stsz_entry_t *entries;
} mp4p_stsz_t;

// NOTE: reused for stco and co64
typedef struct {
    uint64_t offset;
} mp4p_stco_entry_t;

typedef struct {
    uint32_t number_of_entries;
    mp4p_stco_entry_t *entries;
} mp4p_stco_t;

typedef struct {
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
    uint32_t version_flags;
    uint32_t data_size;
    uint64_t data_offset;
    char *name;
    char *text;
    uint16_t *values;
} mp4p_meta_t;

void
mp4p_atom_free (mp4p_atom_t *atom);

typedef struct {
    void *data;
    size_t (*fread) (void *ptr, size_t size, size_t nmemb, void *stream);
    int (*fseek) (void *stream, int64_t offset, int whence);
    int64_t (*ftell) (void *stream);
} mp4p_file_callbacks_t;

// `callbacks` can be NULL, in which case stdio is used.
// Loading starts from the current position in the stream.
mp4p_atom_t *
mp4p_open (const char *fname, mp4p_file_callbacks_t *callbacks);

mp4p_atom_t *
mp4p_atom_find (mp4p_atom_t *root, const char *path);

uint64_t
mp4p_stts_total_num_samples (mp4p_atom_t *stts_atom);

uint32_t
mp4p_stts_sample_duration (mp4p_atom_t *stts_atom, uint32_t sample);

uint64_t
mp4p_stts_total_sample_duration (mp4p_atom_t *stts_atom);

uint32_t
mp4p_sample_size (mp4p_atom_t *stsz_atom, uint32_t sample);

uint64_t
mp4p_sample_offset (mp4p_atom_t *stbl_atom, uint32_t sample);

const char *
mp4p_genre_name_for_index (uint16_t index);

#endif /* mp4parser_h */
