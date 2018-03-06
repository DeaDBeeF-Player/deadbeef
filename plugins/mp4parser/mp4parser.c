#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "mp4parser.h"

#ifndef __linux__
#define O_LARGEFILE 0
#endif

static mp4p_atom_t *
_atom_load (mp4p_atom_t *parent_atom, mp4p_file_callbacks_t *fp);

void
mp4p_atom_free (mp4p_atom_t *atom) {
    if (atom->subatoms) {
        mp4p_atom_free_list (atom->subatoms);
    }

    if (atom->free) {
        atom->free (atom->data);
    }
    free (atom);
}

void
mp4p_atom_free_list (mp4p_atom_t *atom) {
    while (atom) {
        mp4p_atom_t *next = atom->next;
        mp4p_atom_free (atom);
        atom = next;
    }
}

int
mp4p_fourcc_compare (const char *value1, const char *value2) {
    // FIXME: should be case-insensitive
    return memcmp (value1, value2, 4);
}

int
mp4p_atom_type_compare (mp4p_atom_t *atom, const char *type) {
    return mp4p_fourcc_compare(atom->type, type);
}

static int _dbg_indent = 0;

static void
_dbg_print_fourcc (const char *fourcc) {
    printf ("%c%c%c%c", fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
}

static void
_dbg_print_indent (void) {
    for (int i = 0; i < _dbg_indent; i++) {
        printf (" ");
    }
}

static void
_dbg_print_atom (mp4p_atom_t *atom) {
    _dbg_print_indent();
    _dbg_print_fourcc(atom->type);
    printf ("\n");
}

static int
_load_subatoms (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    _dbg_indent += 4;
    mp4p_atom_t *tail = NULL;
    while (fp->tell (fp) < atom->pos + atom->size) {
        mp4p_atom_t *c = _atom_load (atom, fp);
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
    if (2 != fp->read (fp, csize, 2)) {
        return -1;
    }
    *value = (csize[1]) | (csize[0]<<8);
    return 0;
}

static int
_read_int16 (mp4p_file_callbacks_t *fp, int16_t *value) {
    uint8_t csize[2];
    if (2 != fp->read (fp, csize, 2)) {
        return -1;
    }
    *value = (((int8_t *)csize)[1]) | (csize[0]<<8);
    return 0;
}

static int
_read_uint32 (mp4p_file_callbacks_t *fp, uint32_t *value) {
    uint8_t csize[4];
    if (4 != fp->read (fp, csize, 4)) {
        return -1;
    }
    *value = csize[3] | (csize[2]<<8) | (csize[1]<<16) | (csize[0]<<24);
    return 0;
}

static int
_read_uint64 (mp4p_file_callbacks_t *fp, uint64_t *value) {
    uint8_t csize[8];
    if (8 != fp->read (fp, csize, 8)) {
        return -1;
    }
    *value = (uint64_t)csize[7] | ((uint64_t)csize[6]<<8) | ((uint64_t)csize[5]<<16) | ((uint64_t)csize[4]<<24) | ((uint64_t)csize[3]<<32) | ((uint64_t)csize[2]<<40) | ((uint64_t)csize[1] << 48) | ((uint64_t)csize[0] << 56);
    return 0;
}

#define READ_UINT8(fp) ({uint8_t _temp8; if (1 != fp->read (fp, &_temp8, 1)) return -1; _temp8;})
#define READ_UINT16(fp) ({uint16_t _temp16; if (_read_uint16 (fp, &_temp16) < 0) return -1; _temp16;})
#define READ_INT16(fp) ({int16_t _temp16; if (_read_int16 (fp, &_temp16) < 0) return -1; _temp16;})
#define READ_UINT32(fp) ({ uint32_t _temp32; if (_read_uint32 (fp, &_temp32) < 0) return -1; _temp32;})
#define READ_UINT64(fp) ({ uint64_t _temp64; if (_read_uint64 (fp, &_temp64) < 0) return -1; _temp64;})
#define READ_BUF(fp,buf,size) {if (size != fp->read(fp, buf, size)) return -1;}

// read/skip uint8 version and uint24 flags
#define READ_COMMON_HEADER() {READ_UINT32(fp);}

#define WRITE_UINT8(x) {if (buffer_size < 1) return 0; *buffer++ = x; buffer_size--; }
#define WRITE_UINT16(x) {if (buffer_size < 2) return 0; *buffer++ = (x>>8); *buffer++ = (x & 0xff); buffer_size -= 2;}
#define WRITE_UINT32(x) {if (buffer_size < 4) return 0; *buffer++ = ((x>>24)); *buffer++ = ((x>>16)&0xff); *buffer++ = ((x>>8)&0xff); *buffer++ = (x & 0xff); buffer_size -=4 ;}
#define WRITE_BUF(buf,size) {if (buffer_size < size) return 0; memcpy (buffer, buf, size); buffer += size; buffer_size -= size; }
#define WRITE_COMMON_HEADER() {WRITE_UINT32(0);}

static const char *container_atoms[] = {
    "moov",
    "trak",
    "mdia",
    "minf",
    "dinf",
    "stbl",
    "udta",
    NULL
};


static void
_hdlr_free (void *data) {
    mp4p_hdlr_t *hdlr = data;
    if (hdlr->buf) {
        free (hdlr->buf);
    }
    free (hdlr);
}

static uint32_t
_hdlr_write (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size) {
    uint32_t init_size = buffer_size;
    mp4p_hdlr_t *hdlr = atom->data;
    WRITE_COMMON_HEADER();
    WRITE_BUF(hdlr->component_type, 4);
    WRITE_BUF(hdlr->component_subtype, 4);
    WRITE_BUF(hdlr->component_manufacturer, 4);
    WRITE_UINT32(hdlr->component_flags);
    WRITE_UINT32(hdlr->component_flags_mask);
    WRITE_UINT8(hdlr->buf_len);
    if (hdlr->buf_len) {
        WRITE_BUF(hdlr->buf, hdlr->buf_len);
    }
    return init_size - buffer_size;
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

static void
_dOps_free (void *data) {
    mp4p_dOps_t *dOps = data;
    if (dOps->channel_mapping_table) {
        for (int i = 0; i < dOps->output_channel_count; i++) {
            if (dOps->channel_mapping_table[i].channel_mapping) {
                free (dOps->channel_mapping_table[i].channel_mapping);
            }
        }
        free (dOps->channel_mapping_table);
    }
    free (dOps);
}

static void
_meta_free (void *data) {
    mp4p_meta_t *meta = data;
    if (meta->name) {
        free (meta->name);
    }
    if (meta->values) {
        free (meta->values);
    }
    if (meta->text) {
        free (meta->text);
    }
    free (meta);
}

static void
_esds_free (void *data) {
    mp4p_esds_t *esds = data;
    if (esds->asc) {
        free (esds->asc);
    }
    free (esds);
}

static uint32_t
_meta_write (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size) {
    uint32_t init_size = buffer_size;
    mp4p_meta_t *meta = atom->data;

    // mean + name
    if (meta->name) {
        uint32_t mean_size = 28;
        WRITE_UINT32(mean_size);
        WRITE_BUF("mean", 4);
        WRITE_UINT32(0);
        WRITE_BUF("com.apple.iTunes", 16);

        uint32_t name_size = 12 + (uint32_t)strlen(meta->name);
        WRITE_UINT32(name_size);
        WRITE_BUF("name", 4);
        WRITE_UINT32(0);
        WRITE_BUF(meta->name, (uint32_t)strlen(meta->name));
    }
    // data atom
    if (meta->text || meta->values) {
        uint32_t data_atom_size = meta->data_size+16;
        WRITE_UINT32(data_atom_size);
        WRITE_BUF("data", 4);
        WRITE_UINT32(meta->version_flags);
        WRITE_UINT32(0);
    }
    if (meta->text) {
        WRITE_BUF(meta->text,meta->data_size);
    }
    else if (meta->values) {
        for (int i = 0; i < meta->data_size/2; i++) {
            WRITE_UINT16(meta->values[i]);
        }
    }
    return init_size - buffer_size;
}


#define COPYRIGHT_SYM "\xa9"

static int
_load_custom_metadata_atom (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    mp4p_meta_t *meta = atom->data;
    uint32_t mean_size = READ_UINT32(fp);
    char mean_type[4];
    READ_BUF(fp, mean_type, 4);
    if (memcmp (mean_type, "mean", 4)) {
        return -1;
    }
    READ_COMMON_HEADER();
    char *mean_data = malloc (mean_size - 12);
    READ_BUF(fp, mean_data, mean_size - 12);
    if (memcmp (mean_data, "com.apple.iTunes", 16)) {
        return -1;
    }

    uint32_t name_size = READ_UINT32(fp);
    char name_type[4];
    READ_BUF(fp, name_type, 4);
    if (memcmp (name_type, "name", 4)) {
        return -1;
    }

    READ_COMMON_HEADER();

    meta->name = malloc (name_size-12);
    READ_BUF(fp, meta->name, name_size - 12);

    uint32_t data_size = READ_UINT32(fp);
    char data_type[4];
    READ_BUF(fp, data_type, 4);
    if (memcmp (data_type, "data", 4)) {
        return -1;
    }

    READ_COMMON_HEADER();

    READ_UINT32(fp);

    meta->data_size = data_size-12;
    meta->text = malloc (meta->data_size+1);
    READ_BUF(fp, meta->text, meta->data_size);
    meta->text[meta->data_size] = 0;

    return 0;
}

static int
_load_metadata_atom (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    mp4p_meta_t *meta = calloc (sizeof (mp4p_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;

    if (!memcmp (atom->type, "----", 4)) {
        return _load_custom_metadata_atom (atom, fp);
    }

    uint32_t size = READ_UINT32(fp);
    char data[4];
    READ_BUF(fp, data, 4);
    if (memcmp (data, "data", 4)) {
        return -1;
    }
    atom->to_buffer = _meta_write;
    meta->version_flags = READ_UINT32(fp);

    READ_UINT32(fp);

    meta->data_size = size - 12;

    meta->data_offset = fp->tell (fp);

    uint32_t flag = meta->version_flags & 0xff;

    if (flag == 0) {
        meta->values = calloc (meta->data_size / 2, 1);
        for (int i = 0; i < meta->data_size/2; i++) {
            meta->values[i] = READ_UINT16(fp);
        }
    }
    else if (flag == 1) {
        if (meta->data_size > 255 && memcmp (atom->type, COPYRIGHT_SYM "lyr", 4)) {
            return -1;
        }
        meta->text = calloc (meta->data_size+1, 1);
        READ_BUF(fp, meta->text, meta->data_size);
        meta->text[meta->data_size] = 0;

        printf ("%s\n", meta->text);
    }
    else {
        return -1;
    }

    return 0;
}

// read tag size, encoded in a 1-4 byte sequence, terminated when high bit is 0
int
_read_esds_tag_size (mp4p_file_callbacks_t *fp, uint32_t *retval) {
    uint32_t num = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t val = READ_UINT8(fp);
        num <<= 7;
        num |= (val & 0x7f);
        if (!(val & 0x80)) {
            break;
        }
    }
    *retval = num;
    return 0;
}

// The function may return -1 on parser failures,
// but this should not be considered a critical failure.
int
mp4p_atom_init (mp4p_atom_t *parent_atom, mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    _dbg_print_atom (atom);
    for (int i = 0; container_atoms[i]; i++) {
        if (!mp4p_atom_type_compare (atom, container_atoms[i])) {
            return _load_subatoms(atom, fp);
        }
    }

    if (!mp4p_atom_type_compare (atom, "ftyp")) {
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
    else if (!mp4p_atom_type_compare(atom, "mvhd")) {
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
    else if (!mp4p_atom_type_compare(atom, "tkhd")) {
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
    else if (!mp4p_atom_type_compare(atom, "mdhd")) {
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
    else if (!mp4p_atom_type_compare(atom, "hdlr")) {
        mp4p_hdlr_t *hdlr = calloc (sizeof (mp4p_hdlr_t), 1);
        atom->data = hdlr;
        atom->free = _hdlr_free;

        READ_COMMON_HEADER();

        // NOTE: in the udta/meta/hdlr,
        // type is "\0\0\0\0"
        // the subtype is "mdir"
        // and manufacturer is "appl"
        READ_BUF(fp, hdlr->component_type, 4);
        READ_BUF(fp, hdlr->component_subtype, 4);
        READ_BUF(fp, hdlr->component_manufacturer, 4);

        hdlr->component_flags = READ_UINT32(fp);
        hdlr->component_flags_mask = READ_UINT32(fp);

        hdlr->buf_len = READ_UINT8(fp);
        if (hdlr->buf_len) {
            hdlr->buf = calloc (hdlr->buf_len, 1);
            READ_BUF(fp, hdlr->buf, hdlr->buf_len);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "smhd")) {
        mp4p_smhd_t *smhd = calloc (sizeof (mp4p_smhd_t), 1);
        atom->data = smhd;
        atom->free = free;

        READ_COMMON_HEADER();

        smhd->balance = READ_UINT16(fp);
    }
    else if (!mp4p_atom_type_compare(atom, "stsd")) {
        mp4p_stsd_t *stsd = calloc (sizeof (mp4p_stsd_t), 1);
        atom->data = stsd;
        atom->free = free;
        READ_COMMON_HEADER();

        stsd->number_of_entries = READ_UINT32(fp);
        _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "stts")) {
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
    else if (!mp4p_atom_type_compare(atom, "stsc")) {
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
    else if (!mp4p_atom_type_compare(atom, "stsz")) {
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
    else if (!mp4p_atom_type_compare(atom, "stco")) {
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
    else if (!mp4p_atom_type_compare(atom, "co64")) {
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
    else if (!mp4p_atom_type_compare(atom, "dref")) {
        mp4p_stco_t *dref = calloc (sizeof (mp4p_dref_t), 1);
        atom->data = dref;
        atom->free = free;

        READ_COMMON_HEADER();

        dref->number_of_entries = READ_UINT32(fp);
        _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "tref")) {
        _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "alac")) {
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
        fp->seek (fp, -10, SEEK_CUR);
        alac->asc = calloc (alac->asc_size, 1);
        READ_BUF(fp, alac->asc, alac->asc_size);
    }
    // mp4a is the same as alac, but followed with subatoms
    else if (!mp4p_atom_type_compare(atom, "mp4a")) {
        mp4p_mp4a_t *mp4a = calloc (sizeof (mp4p_mp4a_t), 1);
        atom->data = mp4a;

        READ_BUF(fp, mp4a->reserved, 6);
        mp4a->data_reference_index = READ_UINT16(fp);

        READ_BUF(fp, mp4a->reserved2, 8);

        // we parse these values, but also read them into the ASC
        mp4a->channel_count = READ_UINT16(fp);
        mp4a->bps = READ_UINT16(fp);
        mp4a->packet_size = READ_UINT16(fp);
        mp4a->sample_rate = READ_UINT32(fp);

        READ_BUF(fp, mp4a->reserved3, 2);

        return _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "Opus")) {
        mp4p_Opus_t *opus = calloc (sizeof (mp4p_Opus_t), 1);
        atom->data = opus;

        READ_BUF(fp, opus->reserved, 6);
        opus->data_reference_index = READ_UINT16(fp);

        READ_BUF(fp, opus->reserved2, 8);

        // we parse these values, but also read them into the ASC
        opus->channel_count = READ_UINT16(fp);
        opus->bps = READ_UINT16(fp);
        if (opus->bps != 16) {
            return -1;
        }
        opus->packet_size = READ_UINT16(fp);
        opus->sample_rate = READ_UINT32(fp);
        if (opus->sample_rate != 48000) {
            return -1;
        }
        READ_BUF(fp, opus->reserved3, 2);
        return _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "dOps")) {
        mp4p_dOps_t *dOps = calloc (sizeof (mp4p_Opus_t), 1);
        atom->data = dOps;
        atom->free = _dOps_free;

        dOps->version = READ_UINT8(fp);
        if (dOps->version != 0) {
            return -1;
        }
        dOps->output_channel_count = READ_UINT8(fp);
        dOps->pre_skip = READ_UINT16(fp);
        dOps->input_sample_rate = READ_UINT32(fp);
        dOps->output_gain = READ_INT16(fp);
        dOps->channel_mapping_family = READ_UINT8(fp);
        if (dOps->channel_mapping_family != 0) {
            dOps->channel_mapping_table = calloc (sizeof (mp4p_opus_channel_mapping_table_t), dOps->output_channel_count);
            for (int i = 0; i < dOps->output_channel_count; i++) {
                dOps->channel_mapping_table[i].channel_mapping = calloc(1, dOps->output_channel_count);
                dOps->channel_mapping_table[i].stream_count = READ_UINT8(fp);
                dOps->channel_mapping_table[i].coupled_count = READ_UINT8(fp);
                for (int j = 0; j < dOps->output_channel_count; j++) {
                    dOps->channel_mapping_table[i].channel_mapping[j] = READ_UINT8(fp);
                }
            }
        }

    }
    else if (!mp4p_atom_type_compare(atom, "esds")) {
        mp4p_esds_t *esds = calloc (sizeof (mp4p_esds_t), 1);
        atom->data = esds;
        atom->free = _esds_free;

        READ_COMMON_HEADER();

        uint8_t es_tag = READ_UINT8(fp);
        if (es_tag == 3)
        {
            uint32_t es_tag_size;
            if (_read_esds_tag_size (fp, &es_tag_size)) {
                return -1;
            }
            if (es_tag_size < 20) {
                return -1;
            }

            READ_UINT8(fp);
        }

        READ_UINT8(fp);
        READ_UINT8(fp);

        uint8_t dc_tag = READ_UINT8(fp);
        if (dc_tag != 4) {
            return -1;
        }

        uint32_t dc_tag_size;
        if (_read_esds_tag_size (fp, &dc_tag_size)) {
            return -1;
        }
        if (dc_tag_size < 13) {
            return -1;
        }

        esds->dc_audiotype = READ_UINT8(fp);
        esds->dc_audiostream = READ_UINT8(fp);
        READ_BUF(fp, esds->dc_buffersize_db, 3);

        esds->dc_max_bitrate = READ_UINT32(fp);
        esds->dc_avg_bitrate = READ_UINT32(fp);

        uint8_t ds_tag = READ_UINT8(fp);
        if (ds_tag != 5) {
            return -1;
        }

        if (_read_esds_tag_size(fp, &esds->asc_size)) {
            return -1;
        }
        if (!esds->asc_size) {
            return 0;
        }

        esds->asc = malloc (esds->asc_size);
        READ_BUF(fp, esds->asc, esds->asc_size);
    }
    else if (!mp4p_atom_type_compare(atom, "meta")) {
        READ_COMMON_HEADER();
        return _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "ilst")) {
        return _load_subatoms(atom, fp);
    }
    else if (parent_atom && !mp4p_atom_type_compare(parent_atom, "ilst")) {
        return _load_metadata_atom (atom, fp);
    }
    else {
        _dbg_print_indent ();
        printf ("[opaque]\n");
    }

    return 0;
}

static mp4p_atom_t *
_atom_load (mp4p_atom_t *parent_atom, mp4p_file_callbacks_t *fp) {
    size_t fpos = fp->tell (fp);

    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);

    atom->pos = fpos;

    if (_read_uint32 (fp, &atom->size) < 0) {
        goto error;
    }

    if (4 != fp->read (fp, &atom->type, 4)) {
        goto error;
    }

    mp4p_atom_init (parent_atom, atom, fp);

    fp->seek (fp, fpos + atom->size, SEEK_SET);

    goto success;
error:
    if (atom) {
        mp4p_atom_free (atom);
        atom = NULL;
    }
success:
    return atom;
}

static ssize_t
_file_read (mp4p_file_callbacks_t *stream, void *ptr, size_t size) {
    return read (stream->handle, ptr, size);
}

static ssize_t
_file_write (mp4p_file_callbacks_t *stream, void *ptr, size_t size) {
    return write (stream->handle, ptr, size);
}

static off_t
_file_seek (mp4p_file_callbacks_t *stream, off_t offset, int whence) {
    return lseek (stream->handle, offset, whence);
}

static int64_t
_file_tell (mp4p_file_callbacks_t *stream) {
    return lseek(stream->handle, 0, SEEK_CUR);
}

static int
_file_truncate (mp4p_file_callbacks_t *stream, off_t length) {
    return ftruncate (stream->handle, length);
}

static void
_init_file_callbacks (mp4p_file_callbacks_t *file) {
    file->read = _file_read;
    file->write = _file_write;
    file->seek = _file_seek;
    file->tell = _file_tell;
    file->truncate = _file_truncate;
}

mp4p_file_callbacks_t *
mp4p_open_file_read (const char *fname) {
    int fd = open (fname, O_RDONLY|O_LARGEFILE);
    if (fd < 0) {
        return NULL;
    }

    mp4p_file_callbacks_t *file = calloc (1, sizeof (mp4p_file_callbacks_t));
    file->handle = fd;
    _init_file_callbacks(file);
    return file;
}

mp4p_file_callbacks_t *
mp4p_open_file_readwrite (const char *fname) {
    int fd = open (fname, O_RDWR|O_LARGEFILE);
    if (fd < 0) {
        return NULL;
    }

    mp4p_file_callbacks_t *file = calloc (1, sizeof (mp4p_file_callbacks_t));
    file->handle = fd;
    _init_file_callbacks(file);
    return file;
}

int
mp4p_file_close (mp4p_file_callbacks_t *file) {
    int res = close (file->handle);
    free (file);
    return res;
}

mp4p_atom_t *
mp4p_open (mp4p_file_callbacks_t *callbacks) {
    mp4p_atom_t *head = NULL;
    mp4p_atom_t *tail = NULL;

    for (;;) {
        mp4p_atom_t *atom = _atom_load (NULL, callbacks);
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

    return head;
}

mp4p_atom_t *
mp4p_atom_find (mp4p_atom_t *root, const char *path) {
    if (strlen (path) < 4) {
        return NULL;
    }

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
    if (a && path[4] == '/') {
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

uint32_t
mp4p_stts_sample_duration (mp4p_atom_t *stts_atom, uint32_t sample) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint32_t n = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        n += stts->entries[i].sample_count;
        if (n >= sample) {
            return stts->entries[i].sample_duration;
        }
    }
    return 0;
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
mp4p_sample_size (mp4p_atom_t *stbl_atom, uint32_t sample)
{
    mp4p_atom_t *stsz_atom = mp4p_atom_find(stbl_atom, "stbl/stsz");
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

#define _GENRE_COUNT (sizeof(_genretbl) / sizeof (char *) - 1)
static const char *_genretbl[] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychedelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "Acapella",
    "Euro-House",
    "Dance Hall",
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",
    "Christian Gangsta",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary C",
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "JPop",
    "SynthPop",
    "Abstract",
    "Art Rock",
    "Baroque",
    "Bhangra",
    "Big Beat",
    "Breakbeat",
    "Chillout",
    "Downtempo",
    "Dub",
    "EBM",
    "Eclectic",
    "Electro",
    "Electroclash",
    "Emo",
    "Experimental",
    "Garage",
    "Global",
    "IDM",
    "Illbient",
    "Industro-Goth",
    "Jam Band",
    "Krautrock",
    "Leftfield",
    "Lounge",
    "Math Rock",
    "New Romantic",
    "Nu-Breakz",
    "Post-Punk",
    "Post-Rock",
    "Psytrance",
    "Shoegaze",
    "Space Rock",
    "Trop Rock",
    "World Music",
    "Neoclassical",
    "Audiobook",
    "Audio Theatre",
    "Neue Deutsche Welle",
    "Podcast",
    "Indie Rock",
    "G-Funk",
    "Dubstep",
    "Garage Rock",
    "Psybient",
    NULL
};

const char *
mp4p_genre_name_for_index (uint16_t genreid) {
    if (genreid-1 < _GENRE_COUNT) {
        return _genretbl[genreid-1];
    }
    return NULL;
}

uint16_t
mp4p_genre_index_for_name (const char *name) {
    for (uint16_t i = 0; _genretbl[i]; i++) {
        if (!strcasecmp (name, _genretbl[i])) {
            return i+1;
        }
    }
    return 0;
}

mp4p_atom_t *
mp4p_ilst_append_custom (mp4p_atom_t *ilst_atom, const char *name, const char *text) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_meta_t *meta = calloc (sizeof (mp4p_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->to_buffer = _meta_write;

    memcpy (atom->type, "----", 4);
    atom->size = 8;
    atom->size += 28; // mean
    atom->size += 12 + (uint32_t)strlen(name); // name
    atom->size += 16 + (uint32_t)strlen(text); // data
    meta->name = strdup (name);
    meta->text = strdup (text);
    meta->data_size = (uint32_t)strlen(text);
    return mp4p_atom_append (ilst_atom, atom);
}

mp4p_atom_t *
mp4p_ilst_append_genre (mp4p_atom_t *ilst_atom, const char *text) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_meta_t *meta = calloc (sizeof (mp4p_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->to_buffer = _meta_write;

    uint16_t genre_id = mp4p_genre_index_for_name (text);
    if (genre_id) {
        memcpy (atom->type, "gnre", 4);
        atom->size = 24+2;
        meta->version_flags = 0;
        meta->values = malloc (2);
        meta->values[0] = genre_id;
        meta->data_size = 2;
    }
    else {
        memcpy (atom->type, COPYRIGHT_SYM "gen", 4);
        atom->size = 24 + (uint32_t)strlen(text);
        meta->version_flags = 1;
        meta->text = strdup (text);
        meta->data_size = (uint32_t)strlen(text);
    }
    return mp4p_atom_append (ilst_atom, atom);
}

mp4p_atom_t *
mp4p_ilst_append_track_disc (mp4p_atom_t *ilst_atom, const char *type, uint16_t index, uint16_t total) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_meta_t *meta = calloc (sizeof (mp4p_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->to_buffer = _meta_write;
    atom->size = 24+6;

    memcpy (atom->type, type, 4);
    meta->version_flags = 0;
    meta->values = malloc (6);
    meta->data_size = 6;
    meta->values[0] = 0;
    meta->values[1] = index;
    meta->values[2] = total;
    return mp4p_atom_append (ilst_atom, atom);
}

mp4p_atom_t *
mp4p_ilst_append_text (mp4p_atom_t *ilst_atom, const char *type, const char *text) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_meta_t *meta = calloc (sizeof (mp4p_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->to_buffer = _meta_write;
    meta->data_size = (uint32_t)strlen(text);
    atom->size = 24+meta->data_size;

    memcpy (atom->type, type, 4);
    meta->version_flags = 1;
    meta->text = strdup (text);

    return mp4p_atom_append (ilst_atom, atom);
}

void
mp4p_atom_remove_subatom (mp4p_atom_t *atom, mp4p_atom_t *subatom) {
    mp4p_atom_t *c = atom->subatoms;
    mp4p_atom_t *prev = NULL;
    while (c) {
        mp4p_atom_t *next = c->next;
        if (c == subatom) {
            mp4p_atom_free (subatom);
            if (prev) {
                prev->next = next;
            }
            else {
                atom->subatoms = next;
            }
            return;
        }
        prev = c;
        c = next;
    }
}

mp4p_atom_t *
mp4p_atom_new (const char *type) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    memcpy (atom->type, type, 4);
    return atom;
}

// NOTE: the cloned atom's data is a direct pointer to src data.
// The downside is that the source must exist until the dest is deleted.
mp4p_atom_t *
mp4p_atom_clone (mp4p_atom_t *src) {
    mp4p_atom_t *dest = mp4p_atom_new (src->type);
    dest->pos = src->pos;
    dest->size = src->size;
    dest->to_buffer = src->to_buffer;

    if (dest->size > 0) {
        dest->data = src->data;
    }

    mp4p_atom_t *tail = NULL;

    if (src->subatoms) {
        dest->subatoms = mp4p_atom_clone(src->subatoms);
    }

    tail = NULL;
    mp4p_atom_t *next = src->next;
    while (next) {
        mp4p_atom_t *next_copy = mp4p_atom_clone(next);

        if (tail) {
            tail = tail->next = next_copy;
        }
        else {
            tail = dest->next = next_copy;
        }
        next = next->next;
    }

    return dest;
}

void
mp4p_atom_calculate_size (mp4p_atom_t *atom) {
    atom->size = 8; // type+size = 8 bytes
    for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
        if (subatom->subatoms) {
            mp4p_atom_calculate_size(subatom);
        }
        atom->size += subatom->size;
    }
}

void
mp4p_rebuild_positions (mp4p_atom_t *atom, uint64_t init_pos) {
    atom->pos = init_pos;

    uint64_t offs = init_pos;
    for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
        mp4p_rebuild_positions(subatom, offs);
        offs += subatom->size;
    }

    offs = atom->pos + atom->size;
    for (mp4p_atom_t *next = atom->next; next; next = next->next) {
        mp4p_rebuild_positions(next, offs);
        offs += next->size;
    }
}

mp4p_atom_t *
mp4p_atom_insert (mp4p_atom_t *parent, mp4p_atom_t *before, mp4p_atom_t *atom) {
    mp4p_atom_t *prev = NULL;
    mp4p_atom_t *subatom = parent->subatoms;
    while (subatom && subatom != before) {
        prev = subatom;
        subatom = subatom->next;
    }

    if (!subatom) { // `before` not found
        return NULL;
    }

    if (prev) {
        prev->next = atom;
    }
    else {
        parent->subatoms = atom;
    }
    atom->next = before;

    return atom;
}

mp4p_atom_t *
mp4p_atom_append (mp4p_atom_t *parent, mp4p_atom_t *atom) {
    mp4p_atom_t *prev = NULL;
    mp4p_atom_t *c = parent->subatoms;
    while (c) {
        prev = c;
        c = c->next;
    }
    if (prev) {
        prev->next = atom;
    }
    else {
        parent->subatoms = atom;
    }
    return atom;
}

void
mp4p_atom_dump (mp4p_atom_t *atom) {
    _dbg_print_atom(atom);
    _dbg_indent += 4;
    for (mp4p_atom_t *c = atom->subatoms; c; c = c->next) {
        mp4p_atom_dump (c);
    }
    _dbg_indent -= 4;
}

void
mp4p_hdlr_init (mp4p_atom_t *hdlr_atom, const char *type, const char *subtype, const char *manufacturer) {
    mp4p_hdlr_t *hdlr = calloc(sizeof (mp4p_hdlr_t), 1);
    hdlr_atom->size = 33;
    hdlr_atom->data = hdlr;
    hdlr_atom->free = _hdlr_free;
    hdlr_atom->to_buffer = _hdlr_write;
    memcpy (hdlr->component_type, type, 4);
    memcpy (hdlr->component_subtype, subtype, 4);
    memcpy (hdlr->component_manufacturer, manufacturer, 4);
}

uint32_t
mp4p_atom_to_buffer (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size) {
    // calculate the size of all sub-atoms
    if (atom->subatoms) {
        uint32_t size = 8;
        for (mp4p_atom_t *c = atom->subatoms; c; c = c->next) {
            size += mp4p_atom_to_buffer (c, NULL, 0);
        }

        if (buffer) {
            uint32_t init_size = buffer_size;
            WRITE_UINT32(size);
            WRITE_BUF(atom->type, 4);
            for (mp4p_atom_t *c = atom->subatoms; c; c = c->next) {
                buffer_size -= mp4p_atom_to_buffer (c, buffer, buffer_size);
            }

            assert (init_size - buffer_size == size);
            return init_size - buffer_size;
        }

        return size;
    }
    else {
        if (buffer) {
            if (atom->size == 0) {
                _dbg_print_fourcc(atom->type);
                printf (": size=0\n");
                return 0;
            }
            uint32_t init_size = buffer_size;
            WRITE_UINT32(atom->size);
            WRITE_BUF(atom->type, 4);
            if (!atom->to_buffer) {
                _dbg_print_fourcc(atom->type);
                printf (": doesn't have writer\n");
                //WRITE_BUF(atom->data, atom->size - 8);
            }
            else {
                buffer_size -= atom->to_buffer (atom, buffer, buffer_size);
                assert (init_size - buffer_size == atom->size);
                return init_size - buffer_size;
            }
        }

        return atom->size;
    }
    return 0;
}

// FIXME: The mdat offset can ONLY move forward for now, but this can be changed quite easily.
// 1. Move `mdat` forward, if needed
// 2. Rewrite the size value of `moov`
// 3. rewrite the whole `udta` with subatoms (via mp4p_atom_to_buffer)
// 4. Write the `free`
int
mp4p_update_metadata (mp4p_file_callbacks_t *file, mp4p_atom_t *source, mp4p_atom_t *dest) {
    int res = -1;
    uint8_t temp[4096];

    mp4p_atom_t *moov_src = mp4p_atom_find (source, "moov");
    mp4p_atom_t *mdat_src = mp4p_atom_find (source, "mdat");

    mp4p_atom_t *moov_dst = mp4p_atom_find (dest, "moov");
    mp4p_atom_t *free_dst = mp4p_atom_find (dest, "free");
    mp4p_atom_t *mdat_dst = mp4p_atom_find (dest, "mdat");

    mp4p_atom_t *udta_dst = mp4p_atom_find (moov_dst, "moov/udta");

    assert (moov_dst->pos == moov_src->pos);

    int64_t offs = mdat_dst->pos - mdat_src->pos;
    if (offs < 0) {
        goto error; // truncation unsupported
    }

    // need to move mdat?
    if (offs > 0) {
        // get file size
        off_t size = file->seek (file, 0, SEEK_END);
        if (size < 0) {
            goto error; // couldn't get the size
        }

        // resize the file
        if (file->truncate (file, size + offs) < 0) {
            goto error;
        }

        off_t pos_src = size;
        do {
            ssize_t blocksize = sizeof (temp);
            if (pos_src - (off_t)sizeof (temp) < (off_t)mdat_src->pos) {
                blocksize = pos_src - mdat_src->pos;
                pos_src = mdat_src->pos;
            }
            else {
                pos_src -= sizeof (temp);
            }

            if (file->seek (file, pos_src, SEEK_SET) < 0) {
                goto error;
            }

            if (blocksize != file->read (file, temp, blocksize)) {
                goto error;
            }

            off_t pos_dst = pos_src + offs;

            if (file->seek (file, pos_dst, SEEK_SET) < 0) {
                goto error;
            }

            if (blocksize != file->write (file, temp, blocksize)) {
                goto error;
            }
        } while (pos_src > mdat_src->pos);
    }

    // rewrite moov size
    if (file->seek (file, moov_dst->pos, SEEK_SET) < 0) {
        goto error;
    }
    // FIXME: a better uint32_le writer needed
    size_t buffer_size = 4;
    uint8_t value[4];
    uint8_t *buffer = value;
    WRITE_UINT32(moov_dst->size);
    if (file->write (file, value, 4) != 4) {
        goto error;
    }

    // write udta
    if (file->seek (file, udta_dst->pos, SEEK_SET) < 0) {
        goto error;
    }
    uint32_t atom_size = mp4p_atom_to_buffer (udta_dst, NULL, 0);
    buffer = malloc (atom_size);
    uint32_t written_size = mp4p_atom_to_buffer(udta_dst, buffer, atom_size);
    assert (written_size == atom_size);

    if (file->write (file, buffer, atom_size) != atom_size) {
        free (buffer);
        goto error;
    }
    free (buffer);

    // write free
    if (file->seek (file, free_dst->pos, SEEK_SET) < 0) {
        goto error;
    }
    buffer = value;
    WRITE_UINT32(free_dst->size);
    if (file->write (file, value, 4) != 4) {
        goto error;
    }
    if (file->write (file, "free", 4) != 4) {
        goto error;
    }
    memset (temp, 0, sizeof (temp));
    size_t size = free_dst->size;
    while (size > 0) {
        size_t blocksize = sizeof (temp);
        if (blocksize > size) {
            blocksize = size;
        }
        if (file->write (file, temp, blocksize) != blocksize) {
            goto error;
        }
    }

    res = 0;

error:

    return res;
}

