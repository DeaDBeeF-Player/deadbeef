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
	uint32_t sample_description_size;
	char data_format[4];
	uint8_t reserved[6];
	uint16_t data_reference_index;
	char *decoder_info;
} mp4p_stsd_entry_t;

typedef struct {
	uint32_t number_of_entries;
	mp4p_stsd_entry_t *entries;
} mp4p_stsd_t;

void
mp4p_atom_free (mp4p_atom_t *atom);

mp4p_atom_t *
mp4p_open (const char *fname);

#endif /* mp4parser_h */
