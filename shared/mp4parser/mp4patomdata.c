//
//  mp4patomdata.c
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 4/7/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include "mp4p.h"
#include "mp4patomdata.h"

#define READ_UINT8() ({if (buffer_size < 1) return -1; uint8_t _temp8 = *buffer; buffer++; buffer_size--; _temp8;})
#define READ_UINT16() ({if (buffer_size < 2) return -1; uint16_t _temp16 = (buffer[1]) | (buffer[0]<<8); buffer+=2, buffer_size -= 2; _temp16;})
#define READ_INT16() ({if (buffer_size < 2) return -1; int16_t _temp16 = (((int8_t *)buffer)[1]) | (buffer[0]<<8); buffer+=2; buffer_size-=2;  _temp16;})
#define READ_UINT32() ({if (buffer_size < 4) return -1;  uint32_t _temp32 = (uint32_t)buffer[3] | ((uint32_t)buffer[2]<<8) | ((uint32_t)buffer[1]<<16) | ((uint32_t)buffer[0]<<24); buffer+=4; buffer_size-=4; _temp32;})
#define READ_INT32() ({if (buffer_size < 4) return -1;  int32_t _temp32 = (int32_t)buffer[3] | ((uint32_t)buffer[2]<<8) | ((uint32_t)buffer[1]<<16) | ((uint32_t)buffer[0]<<24); buffer+=4; buffer_size-=4; _temp32;})
#define READ_UINT64() ({if (buffer_size < 8) return -1;  uint64_t _temp64 = (uint64_t)buffer[7] | ((uint64_t)buffer[6]<<8) | ((uint64_t)buffer[5]<<16) | ((uint64_t)buffer[4]<<24) | ((uint64_t)buffer[3]<<32) | ((uint64_t)buffer[2]<<40) | ((uint64_t)buffer[1] << 48) | ((uint64_t)buffer[0] << 56); buffer+=8; buffer_size-=8; _temp64;})
#define READ_INT64() ({if (buffer_size < 8) return -1;  uint64_t _temp64 = (int64_t)csize[7] | ((uint64_t)csize[6]<<8) | ((uint64_t)csize[5]<<16) | ((uint64_t)csize[4]<<24) | ((uint64_t)csize[3]<<32) | ((uint64_t)csize[2]<<40) | ((uint64_t)csize[1] << 48) | ((uint64_t)csize[0] << 56); buffer+=8; buffer_size-=8; _temp64;})
#define READ_BUF(buf,size) {if (buffer_size < size) return -1; memcpy (buf, buffer, size); buffer += size; buffer_size -= size; }

// read/skip uint8 version and uint24 flags
#define READ_COMMON_HEADER() {atom_data->ch.version_flags = READ_UINT32();}

#define WRITE_UINT8(x) {if (buffer_size < 1) return 0; *buffer++ = x; buffer_size--; }
#define WRITE_UINT16(x) {if (buffer_size < 2) return 0; *buffer++ = (x>>8); *buffer++ = (x & 0xff); buffer_size -= 2;}
#define WRITE_UINT32(x) {if (buffer_size < 4) return 0; *buffer++ = ((x>>24)); *buffer++ = ((x>>16)&0xff); *buffer++ = ((x>>8)&0xff); *buffer++ = (x & 0xff); buffer_size -=4 ;}
#define WRITE_UINT64(x) {if (buffer_size < 8) return 0; *buffer++ = ((x>>56)); *buffer++ = ((x>>48)&0xff); *buffer++ = ((x>>40)); *buffer++ = ((x>>32)&0xff); *buffer++ = ((x>>24)); *buffer++ = ((x>>16)&0xff); *buffer++ = ((x>>8)&0xff); *buffer++ = (x & 0xff); buffer_size -= 8 ;}
#define WRITE_BUF(buf,size) {if (buffer_size < size) return 0; memcpy (buffer, buf, size); buffer += size; buffer_size -= size; }
#define WRITE_COMMON_HEADER() {WRITE_UINT32(atom_data->ch.version_flags);}

#pragma mark mvhd

int
mp4p_mvhd_atomdata_read (mp4p_mvhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
     READ_COMMON_HEADER();

     atom_data->creation_time = READ_UINT32();
     atom_data->modification_time = READ_UINT32();
     atom_data->time_scale = READ_UINT32();
     atom_data->duration = READ_UINT32();
     atom_data->preferred_rate = READ_UINT32();
     atom_data->preferred_volume = READ_UINT16();
     READ_BUF(atom_data->reserved, 10);
     READ_BUF(atom_data->matrix_structure, 36);
     atom_data->preview_time = READ_UINT32();
     atom_data->preview_duration = READ_UINT32();
     atom_data->poster_time = READ_UINT32();
     atom_data->selection_time = READ_UINT32();
     atom_data->selection_duration = READ_UINT32();
     atom_data->current_time = READ_UINT32();
     atom_data->next_track_id = READ_UINT32();
     return 0;
 }

size_t
mp4p_mvhd_atomdata_write (mp4p_mvhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 100;
    }
    uint8_t *origin = buffer;
    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->creation_time);
    WRITE_UINT32(atom_data->modification_time);
    WRITE_UINT32(atom_data->time_scale);
    WRITE_UINT32(atom_data->duration);
    WRITE_UINT32(atom_data->preferred_rate);
    WRITE_UINT16(atom_data->preferred_volume);
    WRITE_BUF(atom_data->reserved, 10);
    WRITE_BUF(atom_data->matrix_structure, 36);
    WRITE_UINT32(atom_data->preview_time);
    WRITE_UINT32(atom_data->preview_duration);
    WRITE_UINT32(atom_data->poster_time);
    WRITE_UINT32(atom_data->selection_time);
    WRITE_UINT32(atom_data->selection_duration);
    WRITE_UINT32(atom_data->current_time);
    WRITE_UINT32(atom_data->next_track_id);

    return buffer - origin;
}

void
mp4p_mvhd_atomdata_free (void *atom_data) {
    free (atom_data);
}

#pragma mark tkhd

int
mp4p_tkhd_atomdata_read (mp4p_tkhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->creation_time = READ_UINT32();
    atom_data->modification_time = READ_UINT32();
    atom_data->track_id = READ_UINT32();
    READ_BUF(atom_data->reserved, 4);
    atom_data->duration = READ_UINT32();
    READ_BUF(atom_data->reserved2, 8);
    atom_data->layer = READ_UINT16();
    atom_data->alternate_group = READ_UINT16();
    atom_data->volume = READ_UINT16();
    READ_BUF(atom_data->reserved3, 2);
    READ_BUF(atom_data->matrix_structure, 36);
    atom_data->track_width = READ_UINT32();
    atom_data->track_height = READ_UINT32();

    return 0;
}

size_t
mp4p_tkhd_atomdata_write (mp4p_tkhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 84;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->creation_time);
    WRITE_UINT32(atom_data->modification_time);
    WRITE_UINT32(atom_data->track_id);
    WRITE_BUF(atom_data->reserved, 4);
    WRITE_UINT32(atom_data->duration);
    WRITE_BUF(atom_data->reserved2, 8);
    WRITE_UINT16(atom_data->layer);
    WRITE_UINT16(atom_data->alternate_group);
    WRITE_UINT16(atom_data->volume);
    WRITE_BUF(atom_data->reserved3, 2);
    WRITE_BUF(atom_data->matrix_structure, 36);
    WRITE_UINT32(atom_data->track_width);
    WRITE_UINT32(atom_data->track_height);

    return buffer - origin;
}

void
mp4p_tkhd_atomdata_free (void *atom_data) {
    free (atom_data);
}

#pragma mark mdhd

int
mp4p_mdhd_atomdata_read (mp4p_mdhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->creation_time = READ_UINT32();
    atom_data->modification_time = READ_UINT32();
    atom_data->time_scale = READ_UINT32();
    atom_data->duration = READ_UINT32();
    atom_data->language = READ_UINT16();
    atom_data->quality = READ_UINT16();

    return 0;
}

size_t
mp4p_mdhd_atomdata_write (mp4p_mdhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 24;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->creation_time);
    WRITE_UINT32(atom_data->modification_time);
    WRITE_UINT32(atom_data->time_scale);
    WRITE_UINT32(atom_data->duration);
    WRITE_UINT16(atom_data->language);
    WRITE_UINT16(atom_data->quality);

    return buffer - origin;
}

void
mp4p_mdhd_atomdata_free (void *atom_data) {
    free (atom_data);
}

#pragma mark hdlr

int
mp4p_hdlr_atomdata_read (mp4p_hdlr_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    // NOTE: in the udta/meta/hdlr,
    // type is "\0\0\0\0"
    // the subtype is "mdir"
    // and manufacturer is "appl"
    READ_BUF(atom_data->component_type, 4);
    READ_BUF(atom_data->component_subtype, 4);
    READ_BUF(atom_data->component_manufacturer, 4);

    atom_data->component_flags = READ_UINT32();
    atom_data->component_flags_mask = READ_UINT32();

    atom_data->buf_len = READ_UINT8();
    if (atom_data->buf_len) {
        if (atom_data->buf_len > buffer_size) {
            atom_data->buf_len = buffer_size;
        }

        atom_data->buf = malloc (atom_data->buf_len);
        READ_BUF(atom_data->buf, atom_data->buf_len);
    }

    return 0;
}

size_t
mp4p_hdlr_atomdata_write (mp4p_hdlr_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 25 + atom_data->buf_len;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_BUF(atom_data->component_type, 4);
    WRITE_BUF(atom_data->component_subtype, 4);
    WRITE_BUF(atom_data->component_manufacturer, 4);

    WRITE_UINT32(atom_data->component_flags);
    WRITE_UINT32(atom_data->component_flags_mask);

    WRITE_UINT8(atom_data->buf_len);
    if (atom_data->buf_len) {
        WRITE_BUF(atom_data->buf, atom_data->buf_len);
    }

    return buffer - origin;
}

void
mp4p_hdlr_atomdata_free (void *atom_data) {
    mp4p_hdlr_t *hdlr = atom_data;
    if (hdlr->buf) {
        free (hdlr->buf);
    }
    free (hdlr);
}

void
mp4p_hdlr_init (mp4p_atom_t *hdlr_atom, const char *type, const char *subtype, const char *manufacturer) {
    mp4p_hdlr_t *hdlr = calloc(sizeof (mp4p_hdlr_t), 1);
    hdlr_atom->size = 33;
    hdlr_atom->data = hdlr;
    hdlr_atom->free = mp4p_hdlr_atomdata_free;
    hdlr_atom->write = (mp4p_atom_data_write_func_t)mp4p_hdlr_atomdata_write;
    memcpy (hdlr->component_type, type, 4);
    memcpy (hdlr->component_subtype, subtype, 4);
    memcpy (hdlr->component_manufacturer, manufacturer, 4);
}

#pragma mark smhd

int
mp4p_smhd_atomdata_read (mp4p_smhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->balance = READ_UINT16();
    atom_data->reserved = READ_UINT16();

    return 0;
}

size_t
mp4p_smhd_atomdata_write (mp4p_smhd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_UINT16(atom_data->balance);
    WRITE_UINT16(atom_data->reserved);


    return buffer - origin;
}

void
mp4p_smhd_atomdata_free (void *atom_data) {
    free (atom_data);
}

#pragma mark stsd

int
mp4p_stsd_atomdata_read (mp4p_stsd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->number_of_entries = READ_UINT32();

    return 0;
}

size_t
mp4p_stsd_atomdata_write (mp4p_stsd_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->number_of_entries);

    return buffer - origin;
}

void
mp4p_stsd_atomdata_free (void *atom_data) {
    free (atom_data);
}

#pragma mark stts

int
mp4p_stts_atomdata_read (mp4p_stts_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->number_of_entries = READ_UINT32();
    if (atom_data->number_of_entries) {
        atom_data->entries = calloc (sizeof (mp4p_stts_entry_t), atom_data->number_of_entries);
    }
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        atom_data->entries[i].sample_count = READ_UINT32();
        atom_data->entries[i].sample_duration = READ_UINT32();
    }

    return 0;
}

size_t
mp4p_stts_atomdata_write (mp4p_stts_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8 + atom_data->number_of_entries * 8;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->number_of_entries);
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        WRITE_UINT32(atom_data->entries[i].sample_count);
        WRITE_UINT32(atom_data->entries[i].sample_duration);
    }

    return buffer - origin;
}

void
mp4p_stts_atomdata_free (void *data) {
    mp4p_stts_t *stts = data;
    if (stts->entries) {
        free (stts->entries);
    }
    free (stts);
}

#pragma mark stsc

int
mp4p_stsc_atomdata_read (mp4p_stsc_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->number_of_entries = READ_UINT32();
    if (atom_data->number_of_entries) {
        atom_data->entries = calloc (sizeof (mp4p_stsc_entry_t), atom_data->number_of_entries);
    }
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        atom_data->entries[i].first_chunk = READ_UINT32();
        atom_data->entries[i].samples_per_chunk = READ_UINT32();
        atom_data->entries[i].sample_description_id = READ_UINT32();
    }

    return 0;
}

size_t
mp4p_stsc_atomdata_write (mp4p_stsc_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8 + atom_data->number_of_entries * 12;
    }
    uint8_t *origin = buffer;
    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->number_of_entries);
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        WRITE_UINT32(atom_data->entries[i].first_chunk);
        WRITE_UINT32(atom_data->entries[i].samples_per_chunk);
        WRITE_UINT32(atom_data->entries[i].sample_description_id);
    }

    return buffer - origin;
}

void
mp4p_stsc_atomdata_free (void *data) {
    mp4p_stsc_t *stsc = data;
    if (stsc->entries) {
        free (stsc->entries);
    }
    free (stsc);
}

#pragma mark stsz

int
mp4p_stsz_atomdata_read (mp4p_stsz_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->sample_size = READ_UINT32();
    atom_data->number_of_entries = READ_UINT32();
    if (atom_data->number_of_entries) {
        atom_data->entries = calloc (sizeof (mp4p_stsz_entry_t), atom_data->number_of_entries);
    }
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        atom_data->entries[i].sample_size = READ_UINT32();
    }

    return 0;
}

size_t
mp4p_stsz_atomdata_write (mp4p_stsz_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 12 + atom_data->number_of_entries * 4;
    }
    uint8_t *origin = buffer;
    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->sample_size);
    WRITE_UINT32(atom_data->number_of_entries);
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        WRITE_UINT32(atom_data->entries[i].sample_size);
    }

    return buffer - origin;
}

void
mp4p_stsz_atomdata_free (void *data) {
    mp4p_stsz_t *stsz = data;
    if (stsz->entries) {
        free (stsz->entries);
    }
    free (stsz);
}

#pragma mark stco

int
mp4p_stco_atomdata_read (mp4p_stco_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->number_of_entries = READ_UINT32();
    if (atom_data->number_of_entries) {
        atom_data->entries = calloc (sizeof (mp4p_stco_entry_t), atom_data->number_of_entries);
    }
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        atom_data->entries[i].offset = (uint64_t)READ_UINT32();
    }

    return 0;
}

size_t
mp4p_stco_atomdata_write (mp4p_stco_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8 + atom_data->number_of_entries * 4;
    }
    uint8_t *origin = buffer;
    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->number_of_entries);
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        WRITE_UINT32((uint32_t)atom_data->entries[i].offset);
    }

    return buffer - origin;
}

void
mp4p_stco_atomdata_free (void *data) {
    mp4p_stco_t *stco = data;
    if (stco->entries) {
        free (stco->entries);
    }
    free (stco);
}

#pragma mark co64

int
mp4p_co64_atomdata_read (mp4p_co64_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->number_of_entries = READ_UINT32();
    if (atom_data->number_of_entries) {
        atom_data->entries = calloc (sizeof (mp4p_stco_entry_t), atom_data->number_of_entries);
    }
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        atom_data->entries[i].offset = READ_UINT64();
    }

    return 0;
}

size_t
mp4p_co64_atomdata_write (mp4p_co64_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8 + atom_data->number_of_entries * 8;
    }
    uint8_t *origin = buffer;
    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->number_of_entries);
    for (uint32_t i = 0; i < atom_data->number_of_entries; i++) {
        WRITE_UINT64(atom_data->entries[i].offset);
    }

    return buffer - origin;
}

void
mp4p_co64_atomdata_free (void *data) {
    mp4p_co64_t *co64 = data;
    if (co64->entries) {
        free (co64->entries);
    }
    free (co64);
}

#pragma mark dref

int
mp4p_dref_atomdata_read (mp4p_dref_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->number_of_entries = READ_UINT32();

    return 0;
}

size_t
mp4p_dref_atomdata_write (mp4p_dref_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 8;
    }
    uint8_t *origin = buffer;
    WRITE_COMMON_HEADER();

    WRITE_UINT32(atom_data->number_of_entries);

    return buffer - origin;
}

void
mp4p_dref_atomdata_free (void *data) {
    mp4p_dref_t *dref = data;
    free (dref);
}

#pragma mark alac

int
mp4p_alac_atomdata_read (mp4p_alac_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    size_t atomdata_size = buffer_size;

    if (atomdata_size < 32) {
        return -1;
    }

    READ_BUF(atom_data->reserved, 6);
    atom_data->data_reference_index = READ_UINT16();

    READ_BUF(atom_data->reserved2, 8);

    atom_data->asc_size = (uint32_t)(atomdata_size - 16);
    if (atom_data->asc_size > 64) {
        atom_data->asc_size = 64;
    }
    atom_data->asc = calloc (atom_data->asc_size, 1);
    READ_BUF(atom_data->asc, atom_data->asc_size);

    // These values are parsed from the ASC blob
    buffer = atom_data->asc;
    buffer_size = atom_data->asc_size;
    atom_data->channel_count = READ_UINT16();
    atom_data->bps = READ_UINT16();
    atom_data->packet_size = READ_UINT16();
    atom_data->sample_rate = READ_UINT32();

    return 0;
}

size_t
mp4p_alac_atomdata_write (mp4p_alac_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (atom_data->asc_size < 24) {
        return -1;
    }
    if (!buffer) {
        return 16+atom_data->asc_size;
    }
    uint8_t *origin = buffer;

    WRITE_BUF(atom_data->reserved, 6);
    WRITE_UINT16(atom_data->data_reference_index);

    WRITE_BUF(atom_data->reserved2, 8);
    WRITE_BUF(atom_data->asc, atom_data->asc_size);

    return buffer - origin;
}

void
mp4p_alac_atomdata_free (void *data) {
    mp4p_alac_t *alac = data;
    free (alac->asc);
    free (alac);
}
