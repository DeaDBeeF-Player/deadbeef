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
		atom->free (atom->data);
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

// read/skip uint8 version and uint24 flags
#define READ_COMMON_HEADER() {READ_UINT32(fp);}

static void
_hdlr_free (void *data) {
	mp4p_hdlr_t *hdlr = data;
	if (hdlr->buf) {
		free (hdlr->buf);
	}
	free (hdlr);
}

static void
_stsd_free (void *data) {
	mp4p_stsd_t *stsd = data;
	if (stsd->entries) {
		for (uint32_t i = 0; i < stsd->number_of_entries; i++) {
			if (stsd->entries[i].decoder_info) {
				free (stsd->entries[i].decoder_info);
			}
		}
		free (stsd->entries);
	}
	free (stsd);
}

static void
_stts_free (void *data) {
	mp4p_stts_t *stts = data;
	if (stts->entries) {
		free (stts->entries);
	}
	free (stts);
}

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
		mp4p_mtyp_t *mtyp = calloc (sizeof (mp4p_mtyp_t), 1);
		atom->data = mtyp;
		atom->free = free;
		READ_BUF(fp,mtyp->major_brand,4);
		READ_BUF(fp,mtyp->version,4);
		READ_BUF(fp,mtyp->compat_brand_1,4);
		READ_BUF(fp,mtyp->compat_brand_2,4);

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
		mp4p_mvhd_t *mvhd = calloc (sizeof (mp4p_mvhd_t), 1);
		atom->data = mvhd;
		atom->free = free;

		READ_COMMON_HEADER();

		mvhd->creation_time = READ_UINT32(fp);
		mvhd->modification_time = READ_UINT32(fp);
		mvhd->time_scale = READ_UINT32(fp);
		mvhd->duration = READ_UINT32(fp);
		mvhd->preferred_rate = READ_UINT32(fp);
		mvhd->preferred_volume = READ_UINT16(fp);
		READ_BUF(fp, mvhd->reserved, 10);
		READ_BUF(fp, mvhd->matrix_structure, 36);
		mvhd->preview_time = READ_UINT32(fp);
		mvhd->preview_duration = READ_UINT32(fp);
		mvhd->poster_time = READ_UINT32(fp);
		mvhd->selection_time = READ_UINT32(fp);
		mvhd->selection_duration = READ_UINT32(fp);
		mvhd->current_time = READ_UINT32(fp);
		mvhd->next_track_id = READ_UINT32(fp);
	}
	else if (!_atom_type_compare(atom, "tkhd")) {
		mp4p_tkhd_t *tkhd = calloc (sizeof (mp4p_tkhd_t), 1);
		atom->data = tkhd;
		atom->free = free;

		READ_COMMON_HEADER();

		tkhd->creation_time = READ_UINT32(fp);
		tkhd->modification_time = READ_UINT32(fp);
		tkhd->track_id = READ_UINT32(fp);
		READ_BUF(fp, tkhd->reserved, 4);
		tkhd->duration = READ_UINT32(fp);
		READ_BUF(fp, tkhd->reserved2, 8);
		tkhd->layer = READ_UINT16(fp);
		tkhd->alternate_group = READ_UINT16(fp);
		tkhd->volume = READ_UINT16(fp);
		READ_BUF(fp, tkhd->reserved3, 2);
		READ_BUF(fp, tkhd->matrix_structure, 36);
		tkhd->track_width = READ_UINT32(fp);
		tkhd->track_height = READ_UINT32(fp);
	}
	else if (!_atom_type_compare(atom, "mdhd")) {
		mp4p_mdhd_t *mdhd = calloc (sizeof (mp4p_mdhd_t), 1);
		atom->data = mdhd;
		atom->free = free;

		READ_COMMON_HEADER();

		mdhd->creation_time = READ_UINT32(fp);
		mdhd->modification_time = READ_UINT32(fp);
		mdhd->time_scale = READ_UINT32(fp);
		mdhd->duration = READ_UINT32(fp);
		mdhd->language = READ_UINT16(fp);
		mdhd->quality = READ_UINT16(fp);
	}
	else if (!_atom_type_compare(atom, "hdlr")) {
		mp4p_hdlr_t *hdlr = calloc (sizeof (mp4p_hdlr_t), 1);
		atom->data = hdlr;
		atom->free = _hdlr_free;

		READ_COMMON_HEADER();

		hdlr->component_type = READ_UINT32(fp);
		hdlr->component_subtype = READ_UINT32(fp);
		hdlr->component_manufacturer = READ_UINT32(fp);
		hdlr->component_flags = READ_UINT32(fp);
		hdlr->component_flags_mask = READ_UINT32(fp);

		uint16_t len = READ_UINT16(fp);
		if (len) {
            hdlr->buf = calloc (len, 1);
			READ_BUF(fp, hdlr->buf, len);
		}
	}
	else if (!_atom_type_compare(atom, "smhd")) {
		mp4p_smhd_t *smhd = calloc (sizeof (mp4p_smhd_t), 1);
		atom->data = smhd;
		atom->free = free;

		READ_COMMON_HEADER();

		smhd->balance = READ_UINT16(fp);
	}
	else if (!_atom_type_compare(atom, "stsd")) {
		mp4p_stsd_t *stsd = calloc (sizeof (mp4p_stsd_t), 1);
		atom->data = stsd;
		atom->free = _stsd_free;
		READ_COMMON_HEADER();

		stsd->number_of_entries = READ_UINT32(fp);
		if (stsd->number_of_entries) {
			stsd->entries = calloc (sizeof (mp4p_stsd_entry_t), stsd->number_of_entries);
		}

		for (int i = 0; i < stsd->number_of_entries; i++) {
			stsd->entries[i].sample_description_size = READ_UINT32(fp);
			READ_BUF(fp, stsd->entries[i].data_format, 4);
			READ_BUF(fp, stsd->entries[i].reserved, 6);
			stsd->entries[i].data_reference_index = READ_UINT16(fp);

			if (stsd->entries[i].sample_description_size) {
				stsd->entries[i].decoder_info = calloc (stsd->entries[i].sample_description_size, 1);
				READ_BUF(fp, stsd->entries[i].decoder_info, stsd->entries[i].sample_description_size);
			}
		}
	}
	else if (!_atom_type_compare(atom, "stts")) {
		mp4p_stts_t *stts = calloc (sizeof (mp4p_stts_t), 1);
		atom->data = stts;
		atom->free = _stts_free;

		READ_COMMON_HEADER();

		stts->number_of_entries = READ_UINT32(fp);
		if (stts->number_of_entries) {
			stts->entries = calloc (sizeof (mp4p_stts_entry_t), stts->number_of_entries);
		}
		for (uint32_t i = 0; i < stts->number_of_entries; i++) {
			stts->entries[i].sample_count = READ_UINT32(fp);
			stts->entries[i].sample_duration = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stsc")) {
		READ_COMMON_HEADER();
		uint32_t number_of_entries;

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
		READ_COMMON_HEADER();
		uint32_t sample_size;
		uint32_t number_of_entries;

		sample_size = READ_UINT32(fp);
		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint32_t sample_size;
			sample_size = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stco")) {
		READ_COMMON_HEADER();
		uint32_t number_of_entries;

		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint32_t offset;
			offset = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "co64")) {
		READ_COMMON_HEADER();
		uint32_t number_of_entries;

		number_of_entries = READ_UINT32(fp);
		for (uint32_t i = 0; i < number_of_entries; i++) {
			uint64_t offset;
			offset = READ_UINT64(fp);
		}
	}
	else if (!_atom_type_compare(atom, "dref")) {
		READ_COMMON_HEADER();
		uint32_t number_of_entries;

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

