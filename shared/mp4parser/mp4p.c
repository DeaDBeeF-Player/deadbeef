#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mp4p.h"

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
    return strncasecmp (value1, value2, 4);
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
#if 1
    _dbg_print_indent();
    _dbg_print_fourcc(atom->type);
    printf (" pos=%x size=%x", (int)atom->pos, (int)atom->size);
    printf ("\n");
#endif
}

static int
_load_subatoms (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    _dbg_indent += 4;
    mp4p_atom_t *tail = NULL;
    while (fp->tell (fp) < atom->pos + atom->size) {
        mp4p_atom_t *c = _atom_load (atom, fp);
        if (!c) {
            return -1;
        }
        if (!atom->subatoms) {
            atom->subatoms = tail = c;
        }
        else {
            if (tail) {
                tail->next = c;
            }
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
    *value = (uint32_t)csize[3] | ((uint32_t)csize[2]<<8) | ((uint32_t)csize[1]<<16) | ((uint32_t)csize[0]<<24);
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
#define READ_COMMON_HEADER() {atom_data->ch.version_flags = READ_UINT32(fp);}

#define WRITE_UINT8(x) {if (buffer_size < 1) return 0; *buffer++ = x; buffer_size--; }
#define WRITE_UINT16(x) {if (buffer_size < 2) return 0; *buffer++ = (x>>8); *buffer++ = (x & 0xff); buffer_size -= 2;}
#define WRITE_UINT32(x) {if (buffer_size < 4) return 0; *buffer++ = ((x>>24)); *buffer++ = ((x>>16)&0xff); *buffer++ = ((x>>8)&0xff); *buffer++ = (x & 0xff); buffer_size -=4 ;}
#define WRITE_BUF(buf,size) {if (buffer_size < size) return 0; memcpy (buffer, buf, size); buffer += size; buffer_size -= size; }
#define WRITE_COMMON_HEADER() {WRITE_UINT32(0);}

#define READ_ATOM_BUFFER() uint8_t *atombuf = malloc (atom->size-8); if (fp->read(fp, atombuf, atom->size-8) != atom->size-8) { res = -1; goto error; }
#define FREE_ATOM_BUFFER() free (atombuf);

// Known container atoms, which can contain known sub-atoms.
// Uknown atoms will be loaded as opaque blobs, even if they're technically containers.
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
    mp4p_ilst_meta_t *meta = data;
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
_ilst_meta_write (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size) {
    uint32_t init_size = buffer_size;
    mp4p_ilst_meta_t *meta = atom->data;

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
        WRITE_UINT32(meta->ch.version_flags);
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
    mp4p_ilst_meta_t *atom_data = atom->data;
    uint32_t mean_size = READ_UINT32(fp);
    if (mean_size < 12) {
        return -1;
    }
    mean_size -= 12;

    char mean_type[4];
    READ_BUF(fp, mean_type, 4);
    if (strncasecmp (mean_type, "mean", 4)) {
        return -1;
    }
    READ_COMMON_HEADER();
    char *mean_data = malloc (mean_size + 1);
    READ_BUF(fp, mean_data, mean_size);
    mean_data[mean_size] = 0;
    if (strncasecmp (mean_data, "com.apple.iTunes", 16)) {
        return -1;
    }

    uint32_t name_size = READ_UINT32(fp);
    if (name_size < 12) {
        return -1;
    }
    name_size -= 12;

    char name_type[4];
    READ_BUF(fp, name_type, 4);
    if (strncasecmp (name_type, "name", 4)) {
        return -1;
    }

    READ_COMMON_HEADER();

    atom_data->name = malloc (name_size + 1);
    READ_BUF(fp, atom_data->name, name_size);
    atom_data->name[name_size] = 0;

    uint32_t data_size = READ_UINT32(fp);
    if (data_size < 12) {
        return -1;
    }
    data_size -= 12;
    char data_type[4];
    READ_BUF(fp, data_type, 4);
    if (strncasecmp (data_type, "data", 4)) {
        return -1;
    }

    READ_COMMON_HEADER();

    READ_UINT32(fp);

    atom_data->data_size = data_size;
    atom_data->text = malloc (data_size+1);
    READ_BUF(fp, atom_data->text, data_size);
    atom_data->text[data_size] = 0;

    return 0;
}

static int
_load_metadata_atom (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    mp4p_ilst_meta_t *atom_data = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = atom_data;
    atom->free = _meta_free;

    if (!memcmp (atom->type, "----", 4)) {
        return _load_custom_metadata_atom (atom, fp);
    }

    uint32_t size = READ_UINT32(fp);
    if (size < 12) {
        return -1;
    }
    char data[4];
    READ_BUF(fp, data, 4);
    if (strncasecmp (data, "data", 4)) {
        return -1;
    }
    atom->write = _ilst_meta_write;
    READ_COMMON_HEADER(); // FIXME: version_flags go into wrong atom

    READ_UINT32(fp); // FIXME: ignored value

    atom_data->data_size = size - 16;

    atom_data->data_offset = fp->tell (fp);

    uint32_t flag = atom_data->ch.version_flags & 0xff;

    if (flag == 0) {
        atom_data->values = calloc (atom_data->data_size / 2, sizeof (uint16_t));
        for (int i = 0; i < atom_data->data_size/2; i++) {
            atom_data->values[i] = READ_UINT16(fp);
        }
    }
    else if (flag == 1) {
        if (atom_data->data_size > 255 && strncasecmp (atom->type, COPYRIGHT_SYM "lyr", 4)) {
            return -1;
        }
        atom_data->text = calloc (atom_data->data_size+1, 1);
        READ_BUF(fp, atom_data->text, atom_data->data_size);
        atom_data->text[atom_data->data_size] = 0;

//        printf ("%s\n", meta->text);
    }
    else {
        // opaque metadata
        _meta_free (atom_data);
        atom->data = NULL;
        atom->free = NULL;
        fp->seek(fp, atom->pos + 8, SEEK_SET);
        atom->data = malloc (atom->size - 8);
        READ_BUF(fp, atom->data, atom->size - 8);
        return 0;
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

static void
_mp4p_chpl_free (void *data) {
    mp4p_chpl_t *chpl = data;
    for (int i = 0; i < chpl->nchapters; i++) {
        free (chpl->name[i]);
    }
    free (chpl->name);
    free (chpl->start);
    free (data);
}

static int32_t
_load_chpl_atom(mp4p_atom_t *atom, mp4p_file_callbacks_t *fp)
{
    mp4p_chpl_t *atom_data = calloc (sizeof (mp4p_chpl_t), 1);
    atom->data = atom_data;
    atom->free = _mp4p_chpl_free;

    READ_COMMON_HEADER();

    int i;
    uint32_t i_read = atom->size;

    atom_data->nchapters = READ_UINT8(fp);
    i_read -= 5;

    atom_data->name = calloc (sizeof (char *), atom_data->nchapters);
    atom_data->start = calloc (sizeof (int64_t), atom_data->nchapters);
    for( i = 0; i < atom_data->nchapters; i++ )
    {
        uint64_t i_start;
        uint8_t i_len;
        i_start = READ_UINT64(fp);
        i_read -= 8;
        i_len = READ_UINT8(fp);
        i_read -= 1;

        atom_data->name[i] = malloc( i_len + 1 );

        uint32_t i_copy = i_len < i_read ? i_len : i_read;
        if( i_copy > 0 ) {
            READ_BUF(fp, atom_data->name[i], i_copy)
        }
        atom_data->name[i][i_copy] = '\0';
        atom_data->start[i] = i_start;

        i_read -= i_copy;
    }
    // FIXME: convert to qsort
    /* Bubble sort by increasing start date */
    do
    {
        for( i = 0; i < atom_data->nchapters - 1; i++ )
        {
            if( atom_data->start[i] > atom_data->start[i+1] )
            {
                char *psz = atom_data->name[i+1];
                int64_t i64 = atom_data->start[i+1];

                atom_data->name[i+1] = atom_data->name[i];
                atom_data->start[i+1] = atom_data->start[i];

                atom_data->name[i] = psz;
                atom_data->start[i] = i64;

                i = -1;
                break;
            }
        }
    } while( i == -1 );

    return 0;

error:
    return -1;
}

void
_mp4p_chap_free (void *data) {
    mp4p_chap_t *chap = data;
    free (chap->track_id);
    free (chap);
}

static int
_load_chap_atom (mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    mp4p_chap_t *chap = calloc (sizeof (mp4p_chap_t), 1);
    atom->data = chap;
    atom->free = _mp4p_chap_free;

    chap->track_id = NULL;
    chap->count = (atom->size-8)/ sizeof(uint32_t);
    if (chap->count > 0) {
        chap->track_id = calloc (chap->count, sizeof(uint32_t));
    }
    if (chap->track_id == NULL)
        return -1;

    for (int i = 0; i < chap->count; i++)
    {
        chap->track_id[i] = READ_UINT32(fp);
    }

    return 0;
}

int
mp4p_atom_type_invalid (mp4p_atom_t *atom) {
    for (int i = 0; i < 4; i++) {
        if (atom->type[i] <= 0 && (uint8_t)atom->type[i] != 0xa9) {
            return 1;
        }
    }
    return 0;
}

uint8_t
_adjust_varstring_len (char *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] == 0) {
            return i+1;
        }
    }
    return len;
}

// The function may return -1 on parser failures,
// but this should not be considered a critical failure.
int
mp4p_atom_init (mp4p_atom_t *parent_atom, mp4p_atom_t *atom, mp4p_file_callbacks_t *fp) {
    int res = 0;
    int terminates_with_varstring = 0;

    for (int i = 0; container_atoms[i]; i++) {
        if (!mp4p_atom_type_compare (atom, container_atoms[i])) {
            return _load_subatoms(atom, fp);
        }
    }

    if (mp4p_atom_type_invalid (atom)) {
        return -1;
    }
    else if (!mp4p_atom_type_compare (atom, "ftyp")) {
        mp4p_mtyp_t *mtyp = calloc (atom->size - 8, 1);
        atom->data = mtyp;
        atom->free = free;
        READ_BUF(fp, mtyp, atom->size-8);
    }
    else if (!mp4p_atom_type_compare(atom, "mvhd")) {
        mp4p_mvhd_t *atom_data = calloc (sizeof (mp4p_mvhd_t), 1);
        atom->data = atom_data;
        atom->free = free;
        atom->write = (mp4p_atom_data_writer_t)mp4p_mvhd_atomdata_write;

        READ_ATOM_BUFFER();
        res = mp4p_mvhd_atomdata_read (atom_data, atombuf, atom->size-8);
        FREE_ATOM_BUFFER();
    }
    else if (!mp4p_atom_type_compare(atom, "tkhd")) {
        mp4p_tkhd_t *atom_data = calloc (sizeof (mp4p_tkhd_t), 1);
        atom->data = atom_data;
        atom->free = free;
// FIXME:        atom->to_buffer = _tkhd_to_buffer;

        READ_COMMON_HEADER();

        atom_data->creation_time = READ_UINT32(fp);
        atom_data->modification_time = READ_UINT32(fp);
        atom_data->track_id = READ_UINT32(fp);
        READ_BUF(fp, atom_data->reserved, 4);
        atom_data->duration = READ_UINT32(fp);
        READ_BUF(fp, atom_data->reserved2, 8);
        atom_data->layer = READ_UINT16(fp);
        atom_data->alternate_group = READ_UINT16(fp);
        atom_data->volume = READ_UINT16(fp);
        READ_BUF(fp, atom_data->reserved3, 2);
        READ_BUF(fp, atom_data->matrix_structure, 36);
        atom_data->track_width = READ_UINT32(fp);
        atom_data->track_height = READ_UINT32(fp);
    }
    else if (!mp4p_atom_type_compare(atom, "mdhd")) {
        mp4p_mdhd_t *atom_data = calloc (sizeof (mp4p_mdhd_t), 1);
        atom->data = atom_data;
        atom->free = free;
        // FIXME:        atom->to_buffer = _mdhd_to_buffer;

        READ_COMMON_HEADER();

        atom_data->creation_time = READ_UINT32(fp);
        atom_data->modification_time = READ_UINT32(fp);
        atom_data->time_scale = READ_UINT32(fp);
        atom_data->duration = READ_UINT32(fp);
        atom_data->language = READ_UINT16(fp);
        atom_data->quality = READ_UINT16(fp);
    }
    else if (!mp4p_atom_type_compare(atom, "hdlr")) {
        mp4p_hdlr_t *atom_data = calloc (sizeof (mp4p_hdlr_t), 1);
        atom->data = atom_data;
        atom->free = _hdlr_free;
        terminates_with_varstring = 1;
// FIXME:        atom->to_buffer = _hdlr_to_buffer;

        READ_COMMON_HEADER();

        // NOTE: in the udta/meta/hdlr,
        // type is "\0\0\0\0"
        // the subtype is "mdir"
        // and manufacturer is "appl"
        READ_BUF(fp, atom_data->component_type, 4);
        READ_BUF(fp, atom_data->component_subtype, 4);
        READ_BUF(fp, atom_data->component_manufacturer, 4);

        atom_data->component_flags = READ_UINT32(fp);
        atom_data->component_flags_mask = READ_UINT32(fp);

        atom_data->buf_len = READ_UINT8(fp);
        if (atom_data->buf_len) {
            atom_data->buf = calloc (atom_data->buf_len, 1);
            READ_BUF(fp, atom_data->buf, atom_data->buf_len);
            atom_data->buf_len = _adjust_varstring_len (atom_data->buf, atom_data->buf_len);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "smhd")) {
        mp4p_smhd_t *atom_data = calloc (sizeof (mp4p_smhd_t), 1);
        atom->data = atom_data;
        atom->free = free;
// FIXME:        atom->to_buffer = _smhd_to_buffer;

        READ_COMMON_HEADER();

        atom_data->balance = READ_UINT16(fp);
        atom_data->reserved = READ_UINT16(fp);
    }
    else if (!mp4p_atom_type_compare(atom, "stsd")) {
        mp4p_stsd_t *atom_data = calloc (sizeof (mp4p_stsd_t), 1);
        atom->data = atom_data;
        atom->free = free;
// FIXME:        atom->to_buffer = _stsd_to_buffer;

        READ_COMMON_HEADER();

        atom_data->number_of_entries = READ_UINT32(fp);
        _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "stts")) {
        mp4p_stts_t *atom_data = calloc (sizeof (mp4p_stts_t), 1);
        atom->data = atom_data;
        atom->free = _stts_free;
// FIXME:        atom->to_buffer = _stts_to_buffer;

        READ_COMMON_HEADER();

        atom_data->number_of_entries = READ_UINT32(fp);
        if (atom_data->number_of_entries) {
            atom_data->entries = calloc (sizeof (mp4p_stts_entry_t), atom_data->number_of_entries);
        }
        for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
            atom_data->entries[i].sample_count = READ_UINT32(fp);
            atom_data->entries[i].sample_duration = READ_UINT32(fp);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "stsc")) {
        mp4p_stsc_t *atom_data = calloc (sizeof (mp4p_stsc_t), 1);
        atom->data = atom_data;
        atom->free = _stsc_free;
// FIXME:        atom->to_buffer = _stsc_to_buffer;

        READ_COMMON_HEADER();

        atom_data->number_of_entries = READ_UINT32(fp);
        if (atom_data->number_of_entries) {
            atom_data->entries = calloc (sizeof (mp4p_stsc_entry_t), atom_data->number_of_entries);
        }
        for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
            atom_data->entries[i].first_chunk = READ_UINT32(fp);
            atom_data->entries[i].samples_per_chunk = READ_UINT32(fp);
            atom_data->entries[i].sample_description_id = READ_UINT32(fp);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "stsz")) {
        mp4p_stsz_t *atom_data = calloc (sizeof (mp4p_stsz_t), 1);
        atom->data = atom_data;
        atom->free = _stsz_free;
// FIXME:        atom->to_buffer = _stsz_to_buffer;

        READ_COMMON_HEADER();

        atom_data->sample_size = READ_UINT32(fp);
        atom_data->number_of_entries = READ_UINT32(fp);
        if (atom_data->number_of_entries) {
            atom_data->entries = calloc (sizeof (mp4p_stsz_entry_t), atom_data->number_of_entries);
        }
        for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
            atom_data->entries[i].sample_size = READ_UINT32(fp);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "stco")) {
        mp4p_stco_t *atom_data = calloc (sizeof (mp4p_stco_t), 1);
        atom->data = atom_data;
        atom->free = _stco_free;
// FIXME:        atom->to_buffer = _stco_to_buffer;

        READ_COMMON_HEADER();

        atom_data->number_of_entries = READ_UINT32(fp);
        if (atom_data->number_of_entries) {
            atom_data->entries = calloc (sizeof (mp4p_stco_entry_t), atom_data->number_of_entries);
        }
        for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
            atom_data->entries[i].offset = (uint64_t)READ_UINT32(fp);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "co64")) {
        mp4p_stco_t *atom_data = calloc (sizeof (mp4p_stco_t), 1);
        atom->data = atom_data;
        atom->free = _stco_free;
// FIXME:        atom->to_buffer = _co64_to_buffer;

        READ_COMMON_HEADER();
        atom_data->number_of_entries = READ_UINT32(fp);
        if (atom_data->number_of_entries) {
            atom_data->entries = calloc (sizeof (mp4p_stco_entry_t), atom_data->number_of_entries);
        }

        for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
            atom_data->entries[i].offset = READ_UINT64(fp);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "dref")) {
        mp4p_dref_t *atom_data = calloc (sizeof (mp4p_dref_t), 1);
        atom->data = atom_data;
        atom->free = free;
// FIXME:        atom->to_buffer = _dref_to_buffer;

        READ_COMMON_HEADER();

        atom_data->number_of_entries = READ_UINT32(fp);
        _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "tref")) {
        _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "alac")) {
        mp4p_alac_t *alac = calloc (sizeof (mp4p_alac_t), 1);
        atom->data = alac;
        atom->free = _alac_free;
// FIXME:        atom->to_buffer = _alac_to_buffer;

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
// FIXME:        atom->to_buffer = _mp4a_to_buffer;

        READ_BUF(fp, mp4a->reserved, 6);
        mp4a->data_reference_index = READ_UINT16(fp);

        READ_BUF(fp, mp4a->reserved2, 8);

        // we parse these values, but also read them into the ASC
        mp4a->channel_count = READ_UINT16(fp);
        mp4a->bps = READ_UINT16(fp);
        mp4a->packet_size = READ_UINT16(fp);
        mp4a->sample_rate = READ_UINT32(fp);

        READ_BUF(fp, mp4a->reserved3, 2);

        res = _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "Opus")) {
        mp4p_Opus_t *opus = calloc (sizeof (mp4p_Opus_t), 1);
        atom->data = opus;
// FIXME:        atom->to_buffer = _Opus_to_buffer;

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
        res = _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "dOps")) {
        mp4p_dOps_t *dOps = calloc (sizeof (mp4p_dOps_t), 1);
        atom->data = dOps;
        atom->free = _dOps_free;
// FIXME:        atom->to_buffer = _dOps_to_buffer;

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
        mp4p_esds_t *atom_data = calloc (sizeof (mp4p_esds_t), 1);
        atom->data = atom_data;
        atom->free = _esds_free;
// FIXME:        atom->to_buffer = _esds_to_buffer;

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

        atom_data->dc_audiotype = READ_UINT8(fp);
        atom_data->dc_audiostream = READ_UINT8(fp);
        READ_BUF(fp, atom_data->dc_buffersize_db, 3);

        atom_data->dc_max_bitrate = READ_UINT32(fp);
        atom_data->dc_avg_bitrate = READ_UINT32(fp);

        uint8_t ds_tag = READ_UINT8(fp);
        if (ds_tag != 5) {
            return -1;
        }

        if (_read_esds_tag_size(fp, &atom_data->asc_size)) {
            return -1;
        }
        if (atom_data->asc_size) {
            atom_data->asc = malloc (atom_data->asc_size);
            READ_BUF(fp, atom_data->asc, atom_data->asc_size);
        }
    }
    else if (!mp4p_atom_type_compare(atom, "meta")) {
        mp4p_meta_t *atom_data = calloc (4, 1);
        atom->data = atom_data;
        atom->write_data_before_subatoms = 1;
        READ_COMMON_HEADER();
        res = _load_subatoms(atom, fp);
    }
    else if (!mp4p_atom_type_compare(atom, "ilst")) {
        res = _load_subatoms(atom, fp);
    }
    else if (parent_atom && !mp4p_atom_type_compare(parent_atom, "ilst")) {
// FIXME:        atom->to_buffer = _meta_to_buffer;
        res = _load_metadata_atom (atom, fp);
    }
    else if (!mp4p_atom_type_compare (atom, "chpl")) {
// FIXME:        atom->to_buffer = _chpl_to_buffer;
        res = _load_chpl_atom (atom, fp);
    }
    else if (!mp4p_atom_type_compare (atom, "chap")) {
// FIXME:        atom->to_buffer = _chap_to_buffer;
        res = _load_chap_atom (atom, fp);
    }
    else {
        atom->data = malloc (atom->size - 8);
        READ_BUF(fp, atom->data, atom->size - 8);
    }

    if (!res) {
        // validate position
        off_t offs = fp->tell (fp);
        // NOTE: It would be great to cause a validation error here,
        // but atoms can contain variable-sized strings of wrong size,
        // and overlap subsequent atoms. This is generally not
        // a problem, because the data is 0-terminated anyway,
        // and the tail is ignored, but would be great to use this
        // in testing and troubleshooting.
        // So here we have a special flag indicating that the current
        // atom terminates with a variable-size string,
        // and should be checked for "at least atom size", and not
        // "exact atom size".
        if (terminates_with_varstring) {
            if (offs < atom->pos + atom->size) {
                res = -1;
            }
        }
        else if (offs != atom->pos + atom->size) {
            res = -1;
        }
    }

error:
    return res;
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

    if (mp4p_atom_init (parent_atom, atom, fp) < 0) {
        goto error;
    }

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

mp4p_atom_t *
mp4p_open (mp4p_file_callbacks_t *callbacks) {
    mp4p_atom_t *head = NULL;
    mp4p_atom_t *tail = NULL;

    for (;;) {
        mp4p_atom_t *atom = _atom_load (NULL, callbacks);
        if (!atom) {
            mp4p_atom_free(head);
            return NULL;
        }

        if (!head) {
            head = tail = atom;
        }
        else {
            tail->next = atom;
            tail = atom;
        }

        if (!mp4p_atom_type_compare(atom, "mdat")) {
            break;
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
        if (!strncasecmp (a->type, path, 4)) {
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
mp4p_stts_sample_duration (mp4p_atom_t *stts_atom, uint32_t mp4sample) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    uint32_t n = 0;
    for (uint32_t i = 0; i < stts->number_of_entries; i++) {
        int nsamples = stts->entries[i].sample_count;
        while (nsamples--) {
            if (n >= mp4sample) {
                return stts->entries[i].sample_duration;
            }
            n++;
        }
    }
    return 0;
}

uint32_t
mp4p_stts_mp4sample_containing_sample (mp4p_atom_t *stts_atom, uint64_t sample, uint64_t *mp4sample_startingsample) {
    mp4p_stts_t *stts = stts_atom->data;
    if (!stts) {
        return 0;
    }
    int mp4sample = 0;
    int pos = 0;
    for (int i = 0; i < stts->number_of_entries; i++) {
        int64_t total = stts->entries[i].sample_duration * stts->entries[i].sample_count;
        if (pos + total >= sample) {
            uint32_t idx = (uint32_t)((sample - pos) / stts->entries[i].sample_duration);
            *mp4sample_startingsample = pos + idx * stts->entries[i].sample_duration;
            return mp4sample + idx;
        }
        pos += total;
        mp4sample += stts->entries[i].sample_count;
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

    if (!stco_atom) {
        return 0;
    }

    mp4p_stco_t *stco = stco_atom->data;

    // walk over chunk table, and find the chunk containing the sample
    uint32_t chunk = 0;
    uint32_t subchunk = 0; // repeated chunk index within stsc item

    uint32_t chunk_first_sample = 0;
    uint64_t offs = 0;

    for (;;) {
        if (chunk == stsc->number_of_entries-1) {
            // last chunk entry is repeated infinitely
            break;
        }

        if (chunk_first_sample + stsc->entries[chunk].samples_per_chunk > sample) {
            // sample belongs to "chunk"
            break;
        }

        chunk_first_sample += stsc->entries[chunk].samples_per_chunk;
        subchunk++;
        if (subchunk >= stsc->entries[chunk+1].first_chunk - stsc->entries[chunk].first_chunk) {
            subchunk = 0;
            chunk++;
        }
    }

    // skip N samples in the chunk, until we get to the needed one
    mp4p_atom_t *stsz_atom = mp4p_atom_find(stbl_atom, "stbl/stsz");
    mp4p_stsz_t *stsz = stsz_atom->data;

    offs = stco->entries[stsc->entries[chunk].first_chunk+subchunk-1].offset;
    if (stsz->sample_size) {
        offs += stsz->sample_size * (sample-chunk_first_sample);
    }
    else {
        for (int i = chunk_first_sample; i < sample; i++) {
            offs += stsz->entries[i].sample_size;
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
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->write = _ilst_meta_write;

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
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->write = _ilst_meta_write;

    uint16_t genre_id = mp4p_genre_index_for_name (text);
    if (genre_id) {
        memcpy (atom->type, "gnre", 4);
        atom->size = 24+2;
        meta->ch.version_flags = 0;
        meta->values = malloc (2);
        meta->values[0] = genre_id;
        meta->data_size = 2;
    }
    else {
        memcpy (atom->type, COPYRIGHT_SYM "gen", 4);
        atom->size = 24 + (uint32_t)strlen(text);
        meta->ch.version_flags = 1;
        meta->text = strdup (text);
        meta->data_size = (uint32_t)strlen(text);
    }
    return mp4p_atom_append (ilst_atom, atom);
}

mp4p_atom_t *
mp4p_ilst_append_track_disc (mp4p_atom_t *ilst_atom, const char *type, uint16_t index, uint16_t total) {
    mp4p_atom_t *atom = calloc (sizeof (mp4p_atom_t), 1);
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->write = _ilst_meta_write;
    atom->size = 24+6;

    memcpy (atom->type, type, 4);
    meta->ch.version_flags = 0;
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
    mp4p_ilst_meta_t *meta = calloc (sizeof (mp4p_ilst_meta_t), 1);
    atom->data = meta;
    atom->free = _meta_free;
    atom->write = _ilst_meta_write;
    meta->data_size = (uint32_t)strlen(text);
    atom->size = 24+meta->data_size;

    memcpy (atom->type, type, 4);
    meta->ch.version_flags = 1;
    meta->text = strdup (text);

    return mp4p_atom_append (ilst_atom, atom);
}

void
mp4p_atom_remove_sibling(mp4p_atom_t *atom, mp4p_atom_t *sibling) {
    mp4p_atom_t *prev = NULL;
    mp4p_atom_t *curr = atom;

    while (curr) {
        if (curr == sibling) {
            if (prev) {
                prev->next = sibling->next;
            }
            mp4p_atom_free (sibling);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
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
    dest->write = src->write;

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
mp4p_atom_update_size (mp4p_atom_t *atom) {
    if (atom->data) {
        return;
    }
    atom->size = 8; // type+size = 8 bytes
    for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
        mp4p_atom_update_size(subatom);
        atom->size += subatom->size;
    }
}

void
mp4p_rebuild_positions (mp4p_atom_t *atom, uint64_t init_pos) {
    atom->pos = init_pos;
    if (atom->data) {
        init_pos += atom->size;
    }
    else {
        init_pos += 8;

        for (mp4p_atom_t *subatom = atom->subatoms; subatom; subatom = subatom->next) {
            mp4p_rebuild_positions(subatom, init_pos);
            init_pos += subatom->size;
        }
    }

    for (mp4p_atom_t *next = atom->next; next; next = next->next) {
        mp4p_rebuild_positions(next, init_pos);
        init_pos += next->size;
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
    hdlr_atom->write = _hdlr_write;
    memcpy (hdlr->component_type, type, 4);
    memcpy (hdlr->component_subtype, subtype, 4);
    memcpy (hdlr->component_manufacturer, manufacturer, 4);
}

uint32_t
mp4p_atom_to_buffer (mp4p_atom_t *atom, uint8_t *buffer, uint32_t buffer_size) {
    // calculate the size of all sub-atoms
    if (atom->subatoms) {
        uint32_t size = 8;

        if (atom->write_data_before_subatoms) {
            size += atom->size - 8;
        }

        for (mp4p_atom_t *c = atom->subatoms; c; c = c->next) {
            size += mp4p_atom_to_buffer (c, NULL, 0);
        }

        if (buffer) {
            uint32_t init_size = buffer_size;
            WRITE_UINT32(size);
            WRITE_BUF(atom->type, 4);

            if (atom->write_data_before_subatoms) {
                WRITE_BUF(atom->data, atom->size - 8);
            }

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
                return -1;
            }
            uint32_t init_size = buffer_size;
            WRITE_UINT32(atom->size);
            WRITE_BUF(atom->type, 4);
            if (!atom->write) {
                _dbg_print_fourcc(atom->type);
                WRITE_BUF(atom->data, atom->size - 8);
            }
            else {
                buffer_size -= atom->write (atom, buffer, buffer_size);
                assert (init_size - buffer_size == atom->size);
                return init_size - buffer_size;
            }
        }

        return atom->size;
    }
    return 0;
}

static int
_rewrite_mdat (mp4p_file_callbacks_t *file, off_t mdat_delta, mp4p_atom_t *mdat_src) {
    uint8_t temp[4096];

    off_t size = file->seek (file, 0, SEEK_END);
    if (size < 0) {
        return -1; // couldn't get the size
    }

    // resize the file
    if (file->truncate (file, size + mdat_delta) < 0) {
        return -1;
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
            return -1;
        }

        if (blocksize != file->read (file, temp, blocksize)) {
            return -1;
        }

        off_t pos_dst = pos_src + mdat_delta;

        if (file->seek (file, pos_dst, SEEK_SET) < 0) {
            return -1;
        }

        if (blocksize != file->write (file, temp, blocksize)) {
            return -1;
        }
    } while (pos_src > mdat_src->pos);

    return 0;
}

// FIXME: The mdat offset can ONLY move forward for now, but this can be changed quite easily.
// 1. Rewrite `mdat` at new offset
// 2. Write out all atoms before the mdat
int
mp4p_update_metadata (mp4p_file_callbacks_t *file, mp4p_atom_t *source, mp4p_atom_t *dest) {
    int res = -1;

    mp4p_atom_t *mdat_src = mp4p_atom_find (source, "mdat");
    mp4p_atom_t *mdat_dst = mp4p_atom_find (dest, "mdat");
    off_t mdat_delta = mdat_dst->pos - mdat_src->pos;

    if (mdat_delta < 0) {
        goto error; // truncation unsupported
    }

    // need to move mdat?
    if (mdat_delta > 0) {
        // get file size
        if (_rewrite_mdat (file, mdat_delta, mdat_src) < 0) {
            goto error;
        }
    }

    file->seek (file, 0, SEEK_SET);

    for (mp4p_atom_t *atom = dest; atom; atom = atom->next) {
        if (!mp4p_fourcc_compare(atom->type, "mdat")) {
            break;
        }

        uint32_t atom_size = mp4p_atom_to_buffer (atom, NULL, 0);
        uint8_t *buffer = malloc (atom_size);
        uint32_t written_size = mp4p_atom_to_buffer(atom, buffer, atom_size);
        assert (written_size == atom_size);

        if (file->write (file, buffer, atom_size) != atom_size) {
            free (buffer);
            goto error;
        }
        free (buffer);
    }

    res = 0;

error:

    return res;
}

int
mp4p_trak_playable (mp4p_atom_t *trak_atom) {
    const char *atom_list[] = {
        "trak/mdia/minf/stbl/stts",
        "trak/mdia/mdhd",
        "trak/mdia/minf/stbl",
        "trak/mdia/minf/stbl/stsz",
        NULL
    };

    for (int i = 0; atom_list[i]; i++) {
        if (!mp4p_atom_find(trak_atom, atom_list[i])) {
            return 0;
        }
    }

    return 1;
}

int
mp4p_trak_has_chapters (mp4p_atom_t *trak_atom) {
    const char *atom_list[] = {
        "trak/mdia/minf/stbl/stsd/text",
        "trak/tkhd",
        "trak/mdia/minf/stbl",
        "trak/mdia/minf/stbl/stts",
        "trak/mdia/minf/stbl/stsz",
        "trak/mdia/mdhd",
        NULL
    };

    for (int i = 0; atom_list[i]; i++) {
        if (!mp4p_atom_find(trak_atom, atom_list[i])) {
            return 0;
        }
    }

    return 1;
}

static void mp4p_dbg_dump_subatoms(mp4p_atom_t *atom) {
    _dbg_print_atom(atom);
    _dbg_indent += 4;
    for (mp4p_atom_t *sub = atom->subatoms; sub; sub = sub->next) {
        mp4p_dbg_dump_subatoms(sub);
    }
    _dbg_indent -= 4;
}

void
mp4p_dbg_dump_atom (mp4p_atom_t *atom) {
    for (; atom; atom = atom->next) {
        mp4p_dbg_dump_subatoms(atom);
    }
}
