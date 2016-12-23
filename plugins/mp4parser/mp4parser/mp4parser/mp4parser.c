#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp4parser.h"

static mp4p_atom_t *
_atom_load (FILE *fp);

void
mp4p_atom_free (mp4p_atom_t *atom) {
	mp4p_atom_t *c = atom->subatoms;
	while (c) {
		mp4p_atom_t *next = c->next;
		mp4p_atom_free (c);
		c = next;
	}

	if (atom->free) {
		atom->free (atom);
	}
	free (atom);
}

static int _atom_type_compare (mp4p_atom_t *atom, const char *type) {
	return memcmp (atom->type, type, 4);
}

static int _dbg_indent = 0;
static void
_dbg_print_atom (mp4p_atom_t *atom) {
	for (int i = 0; i < _dbg_indent; i++) {
		printf (" ");
	}
	char type[5];
	memcpy (type, atom->type, 4);
	type[4] = 0;
	printf ("%s\n", type);
}

int
_load_subatoms (mp4p_atom_t *atom, FILE *fp) {
	_dbg_indent += 4;
	mp4p_atom_t *tail = NULL;
	while (ftell (fp) < atom->pos + atom->size) {
		mp4p_atom_t *c = _atom_load (fp);
		if (!c) {
			break;
		}
		if (!atom->subatoms) {
			atom->subatoms = tail = c;
		}
		else {
			tail->next = c;
			tail = c;
		}
	}
	_dbg_indent -= 4;
	return 0;
}

#if 0
static int
_load_fourcc_atom (mp4p_atom_t *atom, const char *expected, FILE *fp) {
	char fourcc[4];
	if (4 != fread (fourcc, 1, 4, fp)) {
		return -1;
	}
	if (_str_type_compare(fourcc, expected)) {
		return -1;
	}
	return _load_subatoms (atom, fp);
}
#endif

static int
_read_uint16 (FILE *fp, uint16_t *value) {
	uint8_t csize[2];
	if (2 != fread (csize, 1, 2, fp)) {
		return -1;
	}
	*value = (csize[1]) | (csize[0]<<8);
	return 0;
}

static int
_read_uint32 (FILE *fp, uint32_t *value) {
	uint8_t csize[4];
	if (4 != fread (csize, 1, 4, fp)) {
		return -1;
	}
	*value = csize[3] | (csize[2]<<8) | (csize[1]<<16) | (csize[0]<<24);
	return 0;
}

static int
_read_uint64 (FILE *fp, uint64_t *value) {
	uint8_t csize[8];
	if (8 != fread (csize, 1, 8, fp)) {
		return -1;
	}
	*value = (uint64_t)csize[7] | ((uint64_t)csize[6]<<8) | ((uint64_t)csize[5]<<16) | ((uint64_t)csize[4]<<24) | ((uint64_t)csize[3]<<32) | ((uint64_t)csize[2]<<40) | ((uint64_t)csize[1] << 48) | ((uint64_t)csize[0] << 56);
	return 0;
}

#define READ_UINT8(fp) ({uint8_t _temp8; if (1 != fread (&_temp8, 1, 1, fp)) return -1; _temp8;})
#define READ_UINT16(fp) ({uint16_t _temp16; if (_read_uint16 (fp, &_temp16) < 0) return -1; _temp16;})
#define READ_UINT32(fp) ({ uint32_t _temp32; if (_read_uint32 (fp, &_temp32) < 0) return -1; _temp32;})
#define READ_UINT64(fp) ({ uint64_t _temp64; if (_read_uint64 (fp, &_temp64) < 0) return -1; _temp64;})
#define READ_BUF(fp,buf,size) {if (size != fread(buf, 1, size, fp)) return -1;}

static const char *container_atoms[] = {
	"moov",
	"trak",
	"mdia",
	"minf",
	"dinf",
	"stbl",
	NULL
};

// The function may return -1 on parser failures,
// but this should not be considered a critical failure.
int
mp4p_atom_init (mp4p_atom_t *atom, FILE *fp) {
	for (int i = 0; container_atoms[i]; i++) {
		if (!_atom_type_compare (atom, container_atoms[i])) {
			return _load_subatoms(atom, fp);
		}
	}

	if (!_atom_type_compare (atom, "ftyp")) {
		char major_brand[4];
		char version[4];
		char compat_brand_1[4];
		char compat_brand_2[4];
		READ_BUF(fp,major_brand,4);
		READ_BUF(fp,version,4);
		READ_BUF(fp,compat_brand_1,4);
		READ_BUF(fp,compat_brand_2,4);

		// can have more than 4 values, which can be extracted if needed
#if 0
		char more[4];
		int n = atom->size / 4 - 4;
		for (int i = 0; i < n; i++) {
			READ_BUF(fp,more,4);
		}
#endif
	}
	else if (!_atom_type_compare(atom, "mvhd")) {
		uint8_t version;
		uint8_t flags[3];
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

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		creation_time = READ_UINT32(fp);
		modification_time = READ_UINT32(fp);
		time_scale = READ_UINT32(fp);
		duration = READ_UINT32(fp);
		preferred_rate = READ_UINT32(fp);
		preferred_volume = READ_UINT16(fp);
		READ_BUF(fp, reserved, 10);
		READ_BUF(fp, matrix_structure, 36);
		preview_time = READ_UINT32(fp);
		preview_duration = READ_UINT32(fp);
		poster_time = READ_UINT32(fp);
		selection_time = READ_UINT32(fp);
		selection_duration = READ_UINT32(fp);
		current_time = READ_UINT32(fp);
		next_track_id = READ_UINT32(fp);
	}
	else if (!_atom_type_compare(atom, "tkhd")) {
		uint8_t version;
		uint8_t flags[3];
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

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		creation_time = READ_UINT32(fp);
		modification_time = READ_UINT32(fp);
		track_id = READ_UINT32(fp);
		READ_BUF(fp, reserved, 4);
		duration = READ_UINT32(fp);
		READ_BUF(fp, reserved2, 8);
		layer = READ_UINT16(fp);
		alternate_group = READ_UINT16(fp);
		volume = READ_UINT16(fp);
		READ_BUF(fp, reserved3, 2);
		READ_BUF(fp, matrix_structure, 36);
		track_width = READ_UINT32(fp);
		track_height = READ_UINT32(fp);
	}
	else if (!_atom_type_compare(atom, "mdhd")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t creation_time;
		uint32_t modification_time;
		uint32_t time_scale;
		uint32_t duration;
		uint16_t language;
		uint16_t quality;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		creation_time = READ_UINT32(fp);
		modification_time = READ_UINT32(fp);
		time_scale = READ_UINT32(fp);
		duration = READ_UINT32(fp);
		language = READ_UINT16(fp);
		quality = READ_UINT16(fp);
	}
	else if (!_atom_type_compare(atom, "hdlr")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t component_type;
		uint32_t component_subtype;
		uint32_t component_manufacturer;
		uint32_t component_flags;
		uint32_t component_flags_mask;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		component_type = READ_UINT32(fp);
		component_subtype = READ_UINT32(fp);
		component_manufacturer = READ_UINT32(fp);
		component_flags = READ_UINT32(fp);
		component_flags_mask = READ_UINT32(fp);

		uint16_t len = READ_UINT16(fp);
		if (len) {
			char *buf = malloc (len);
			READ_BUF(fp, buf, len);
		}
	}
	else if (!_atom_type_compare(atom, "smhd")) {
		uint8_t version;
		uint8_t flags[3];
		uint16_t balance;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		balance = READ_UINT16(fp);
	}
	else if (!_atom_type_compare(atom, "stsd")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		number_of_entries = READ_UINT32(fp);

		for (int i = 0; i < number_of_entries; i++) {
			uint32_t sample_description_size;
			char data_format[4];
			uint8_t reserved[6];
			uint16_t data_reference_index;

			sample_description_size = READ_UINT32(fp);
			READ_BUF(fp, data_format, 4);
			READ_BUF(fp, reserved, 6);
			data_reference_index = READ_UINT16(fp);

			if (sample_description_size) {
				char decoder_info[sample_description_size];
				READ_BUF(fp, decoder_info, sizeof (decoder_info));
			}
		}
	}
	else if (!_atom_type_compare(atom, "stts")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint32_t sample_count;
			uint32_t sample_duration;

			sample_count = READ_UINT32(fp);
			sample_duration = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stsc")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint32_t first_chunk;
			uint32_t samples_per_chunk;
			uint32_t sample_description_id;

			first_chunk = READ_UINT32(fp);
			samples_per_chunk = READ_UINT32(fp);
			sample_description_id = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stsz")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t sample_size;
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		sample_size = READ_UINT32(fp);
		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint32_t sample_size;
			sample_size = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stco")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint32_t offset;
			offset = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "co64")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint64_t offset;
			offset = READ_UINT64(fp);
		}
	}
	else if (!_atom_type_compare(atom, "dref")) {
		uint8_t version;
		uint8_t flags[3];
		uint32_t number_of_entries;

		version = READ_UINT8(fp);
		READ_BUF(fp, flags, 3);
		number_of_entries = READ_UINT32(fp);
		_load_subatoms(atom, fp);
	}
	else if (!_atom_type_compare(atom, "tref")) {
		_load_subatoms(atom, fp);
	}

	return 0;
}

static mp4p_atom_t *
_atom_load (FILE *fp) {
	size_t fpos = ftell (fp);

	mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);

	atom->pos = fpos;

	// big-endian
	if (_read_uint32 (fp, &atom->size) < 0) {
		goto error;
	}

	if (4 != fread (&atom->type, 1, 4, fp)) {
		goto error;
	}

	_dbg_print_atom (atom);
	mp4p_atom_init (atom, fp);

	fseek (fp, fpos + atom->size, SEEK_SET);

	goto success;
error:
	if (atom) {
		mp4p_atom_free (atom);
		atom = NULL;
	}
success:
	return atom;
}

mp4p_atom_t *
mp4p_open (const char *fname) {
	FILE *fp = fopen (fname, "rb");

	mp4p_atom_t *head = NULL;
	mp4p_atom_t *tail = NULL;

	for (;;) {
		mp4p_atom_t *atom = _atom_load(fp);
		if (!atom) {
			break;
		}
		if (!head) {
			head = tail = atom;
		}
		else {
			tail->next = atom;
			tail = atom;
		}
	}

	if (fp) {
		fclose (fp);
	}
	return head;
}

