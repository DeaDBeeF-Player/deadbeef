#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp4parser.h"

static mp4p_atom_t *
_atom_load (mp4p_file_callbacks_t *fp);

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
_dbg_print_indented (const char *msg) {
    for (int i = 0; i < _dbg_indent; i++) {
        printf (" ");
    }
    printf ("%s\n", msg);
}

static void
_dbg_print_atom (mp4p_atom_t *atom) {
	char type[5];
	memcpy (type, atom->type, 4);
	type[4] = 0;
    _dbg_print_indented(type);
}

int
_load_subatoms (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
	_dbg_indent += 4;
	mp4p_atom_t *tail = NULL;
	while (fp->ftell (fp->data) < atom->pos + atom->size) {
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
_load_fourcc_atom (mp4p_atom_t *atom, const char *expected, mp4p_file_callbacks_t *fp) {
	char fourcc[4];
	if (4 != fp->fread (fourcc, 1, 4, fp->data)) {
		return -1;
	}
	if (_str_type_compare(fourcc, expected)) {
		return -1;
	}
	return _load_subatoms (atom, fp);
}
#endif

static int
_read_uint16 (mp4p_file_callbacks_t *fp, uint16_t *value) {
	uint8_t csize[2];
	if (2 != fp->fread (csize, 1, 2, fp->data)) {
		return -1;
	}
	*value = (csize[1]) | (csize[0]<<8);
	return 0;
}

static int
_read_uint32 (mp4p_file_callbacks_t *fp, uint32_t *value) {
	uint8_t csize[4];
	if (4 != fp->fread (csize, 1, 4, fp->data)) {
		return -1;
	}
	*value = csize[3] | (csize[2]<<8) | (csize[1]<<16) | (csize[0]<<24);
	return 0;
}

static int
_read_uint64 (mp4p_file_callbacks_t *fp, uint64_t *value) {
	uint8_t csize[8];
	if (8 != fp->fread (csize, 1, 8, fp->data)) {
		return -1;
	}
	*value = (uint64_t)csize[7] | ((uint64_t)csize[6]<<8) | ((uint64_t)csize[5]<<16) | ((uint64_t)csize[4]<<24) | ((uint64_t)csize[3]<<32) | ((uint64_t)csize[2]<<40) | ((uint64_t)csize[1] << 48) | ((uint64_t)csize[0] << 56);
	return 0;
}

#define READ_UINT8(fp) ({uint8_t _temp8; if (1 != fp->fread (&_temp8, 1, 1, fp->data)) return -1; _temp8;})
#define READ_UINT16(fp) ({uint16_t _temp16; if (_read_uint16 (fp, &_temp16) < 0) return -1; _temp16;})
#define READ_UINT32(fp) ({ uint32_t _temp32; if (_read_uint32 (fp, &_temp32) < 0) return -1; _temp32;})
#define READ_UINT64(fp) ({ uint64_t _temp64; if (_read_uint64 (fp, &_temp64) < 0) return -1; _temp64;})
#define READ_BUF(fp,buf,size) {if (size != fp->fread(buf, 1, size, fp->data)) return -1;}

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
_stts_free (void *data) {
	mp4p_stts_t *stts = data;
	if (stts->entries) {
		free (stts->entries);
	}
	free (stts);
}

static void
_stsc_free (void *data) {
	mp4p_stsc_t *stsc = data;
	if (stsc->entries) {
		free (stsc->entries);
	}
	free (stsc);
}

static void
_stsz_free (void *data) {
	mp4p_stsz_t *stsz = data;
	if (stsz->entries) {
		free (stsz->entries);
	}
	free (stsz);
}

static void
_stco_free (void *data) {
	mp4p_stco_t *stco = data;
	if (stco->entries) {
		free (stco->entries);
	}
	free (stco);
}

static void
_alac_free (void *data) {
    mp4p_alac_t *alac = data;
    if (alac->asc) {
        free (alac->asc);
    }
    free (alac);
}

// The function may return -1 on parser failures,
// but this should not be considered a critical failure.
int
mp4p_atom_init (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    _dbg_print_atom (atom);
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
		atom->free = free;
		READ_COMMON_HEADER();

		stsd->number_of_entries = READ_UINT32(fp);
        _load_subatoms(atom, fp);
#if 0
// NOTE: the stsd entries can be loaded as atoms, but then we would need to parse each atom individually
// Instead, we load their fixed headers, + blob with decoder-specific info
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
#endif
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
		mp4p_stsc_t *stsc = calloc (sizeof (mp4p_stsc_t), 1);
		atom->data = stsc;
		atom->free = _stsc_free;

		READ_COMMON_HEADER();

		stsc->number_of_entries = READ_UINT32(fp);
		if (stsc->number_of_entries) {
			stsc->entries = calloc (sizeof (mp4p_stsc_entry_t), stsc->number_of_entries);
		}
		for (uint32_t i = 0; i < stsc->number_of_entries; i++) {
			stsc->entries[i].first_chunk = READ_UINT32(fp);
			stsc->entries[i].samples_per_chunk = READ_UINT32(fp);
			stsc->entries[i].sample_description_id = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stsz")) {
		mp4p_stsz_t *stsz = calloc (sizeof (mp4p_stsz_t), 1);
		atom->data = stsz;
		atom->free = _stsz_free;

		READ_COMMON_HEADER();

		stsz->sample_size = READ_UINT32(fp);
		stsz->number_of_entries = READ_UINT32(fp);
		if (stsz->number_of_entries) {
			stsz->entries = calloc (sizeof (mp4p_stsz_entry_t), stsz->number_of_entries);
		}
		for (uint32_t i = 0; i < stsz->number_of_entries; i++) {
			stsz->entries[i].sample_size = READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "stco")) {
		mp4p_stco_t *stco = calloc (sizeof (mp4p_stco_t), 1);
		atom->data = stco;
		atom->free = _stco_free;

		READ_COMMON_HEADER();

		stco->number_of_entries = READ_UINT32(fp);
		if (stco->number_of_entries) {
			stco->entries = calloc (sizeof (mp4p_stco_entry_t), stco->number_of_entries);
		}
		for (uint32_t i = 0; i < stco->number_of_entries; i++) {
			stco->entries[i].offset = (uint64_t)READ_UINT32(fp);
		}
	}
	else if (!_atom_type_compare(atom, "co64")) {
		mp4p_stco_t *stco = calloc (sizeof (mp4p_stco_t), 1);
		atom->data = stco;
		atom->free = _stco_free;

		READ_COMMON_HEADER();
		stco->number_of_entries = READ_UINT32(fp);
		if (stco->number_of_entries) {
			stco->entries = calloc (sizeof (mp4p_stco_entry_t), stco->number_of_entries);
		}

		for (uint32_t i = 0; i < stco->number_of_entries; i++) {
			stco->entries[i].offset = READ_UINT64(fp);
		}
	}
	else if (!_atom_type_compare(atom, "dref")) {
		mp4p_stco_t *dref = calloc (sizeof (mp4p_dref_t), 1);
		atom->data = dref;
		atom->free = free;

		READ_COMMON_HEADER();

		dref->number_of_entries = READ_UINT32(fp);
		_load_subatoms(atom, fp);
	}
	else if (!_atom_type_compare(atom, "tref")) {
		_load_subatoms(atom, fp);
	}
    else if (!_atom_type_compare(atom, "alac")) {
        mp4p_alac_t *alac = calloc (sizeof (mp4p_alac_t), 1);
        atom->data = alac;
        atom->free = _alac_free;

        READ_BUF(fp, alac->reserved, 6);
        alac->data_reference_index = READ_UINT16(fp);

        READ_BUF(fp, alac->reserved2, 8);

        // we parse these values, but also read them into the ASC
        alac->channel_count = READ_UINT16(fp);
        alac->bps = READ_UINT16(fp);
        alac->packet_size = READ_UINT16(fp);
        alac->sample_rate = READ_UINT32(fp);

        alac->asc_size = atom->size - 24;
        if (alac->asc_size > 64) {
            alac->asc_size = 64;
        }
        fp->fseek (fp->data, -10, SEEK_CUR);
        alac->asc = calloc (alac->asc_size, 1);
        READ_BUF(fp, alac->asc, alac->asc_size);
    }

    else {
        _dbg_print_indented ("unknown");
    }

	return 0;
}

static mp4p_atom_t *
_atom_load (mp4p_file_callbacks_t *fp) {
	size_t fpos = fp->ftell (fp->data);

	mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);

	atom->pos = fpos;

	// big-endian
	if (_read_uint32 (fp, &atom->size) < 0) {
		goto error;
	}

	if (4 != fp->fread (&atom->type, 1, 4, fp->data)) {
		goto error;
	}

	mp4p_atom_init (atom, fp);

	fp->fseek (fp->data, fpos + atom->size, SEEK_SET);

	goto success;
error:
	if (atom) {
		mp4p_atom_free (atom);
		atom = NULL;
	}
success:
	return atom;
}

static size_t
_stdio_fread (void *ptr, size_t size, size_t nmemb, void *stream) {
	return fread (ptr, size, nmemb, (FILE *)stream);
}

static int
_stdio_fseek (void *stream, int64_t offset, int whence) {
	return fseek ((FILE *)stream, offset, whence);
}

static int64_t
_stdio_ftell (void *stream) {
	return ftell ((FILE *)stream);
}

mp4p_atom_t *
mp4p_open (const char *fname, mp4p_file_callbacks_t *callbacks) {
	mp4p_file_callbacks_t stdio_callbacks;
	FILE *fp = NULL;
	if (!callbacks) {
		fp = fopen (fname, "rb");
		stdio_callbacks.data = fp;
		stdio_callbacks.fread = _stdio_fread;
		stdio_callbacks.fseek = _stdio_fseek;
		stdio_callbacks.ftell = _stdio_ftell;
		callbacks = &stdio_callbacks;
	}

	mp4p_atom_t *head = NULL;
	mp4p_atom_t *tail = NULL;

	for (;;) {
		mp4p_atom_t *atom = _atom_load(callbacks);
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

mp4p_atom_t *
mp4p_atom_find (mp4p_atom_t *root, const char *path) {
	mp4p_atom_t *a = root;
	while (a) {
		if (!memcmp (a->type, path, 4)) {
			printf ("found: ");
			_dbg_print_atom (a);
			break;
		}
		a = a->next;
	}
    if (a && !path[4]) {
        return a;
    }
	if (a && path[4]) {
		return mp4p_atom_find (a->subatoms, path+5);
	}
	return NULL;
}

uint64_t
mp4p_stts_total_num_samples (mp4p_atom_t *stts_atom) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint64_t total = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        total += stts->entries[i].sample_count;
    }
    return total;
}

uint64_t
mp4p_stts_total_sample_duration (mp4p_atom_t *stts_atom) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint64_t total = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        total += stts->entries[i].sample_duration * stts->entries[i].sample_count;
    }
    return total;
}

uint32_t
mp4p_sample_size (mp4p_atom_t *stsz_atom, uint32_t sample)
{
    mp4p_stsz_t *stsz = stsz_atom->data;
    if (stsz->sample_size) {
        return stsz->sample_size;
    }
    else if (sample < stsz->number_of_entries) {
        return stsz->entries[sample].sample_size;
    }
    return 0;
}

uint64_t
mp4p_sample_offset (mp4p_atom_t *stbl_atom, uint32_t sample) {
    // get chunk idx from sample (stsc table)
    mp4p_atom_t *stsc_atom = mp4p_atom_find(stbl_atom, "stbl/stsc");
    mp4p_stsc_t *stsc = stsc_atom->data;

	if (!stsc->number_of_entries) {
		return 0;
	}

	// get chunk offset (stco/co64 table)
	mp4p_atom_t *stco_atom = mp4p_atom_find(stbl_atom, "stbl/co64");
	if (!stco_atom) {
		stco_atom = mp4p_atom_find(stbl_atom, "stbl/stco");
	}

	mp4p_stco_t *stco = stco_atom->data;

	// walk over chunk table, and find the chunk containing the sample
	uint32_t chunk = 0;
	uint32_t nsample = 0;
	uint64_t offs = 0;

	for (;;) {
		if (chunk == stsc->number_of_entries-1) {
			// last chunk entry is repeated infinitely
			break;
		}

		uint32_t repeat_chunks = stsc->entries[chunk+1].first_chunk - stsc->entries[chunk].first_chunk;
		if (nsample + repeat_chunks * stsc->entries[chunk].samples_per_chunk >= sample) {
			break;
		}

		nsample += repeat_chunks * stsc->entries[chunk].samples_per_chunk;
		chunk++;
	}

    // skip N samples in the chunk, until we get to the needed one
    mp4p_atom_t *stsz_atom = mp4p_atom_find(stbl_atom, "stbl/stsz");
    mp4p_stsz_t *stsz = stsz_atom->data;

	offs = stco->entries[chunk].offset;
	if (stsz->sample_size) {
		offs += stsz->sample_size * (sample-nsample);
	}
	else {
		while (nsample < sample) {
			offs += stsz->entries[nsample].sample_size;
			nsample++;
		}
	}

    return offs;
}
