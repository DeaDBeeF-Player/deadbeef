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
#define WRITE_INT16(x) {if (buffer_size < 2) return 0; *buffer++ = (x>>8); *buffer++ = (uint8_t)(x & 0xff); buffer_size -= 2;}
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
    if (buffer_size < atom_data->number_of_entries * 4) {
        atom_data->number_of_entries = (uint32_t)(buffer_size / 4);
    }
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

#pragma mark mp4a

int
mp4p_mp4a_atomdata_read (mp4p_mp4a_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_BUF(atom_data->reserved, 6);
    atom_data->data_reference_index = READ_UINT16();

    READ_BUF(atom_data->reserved2, 8);

    atom_data->channel_count = READ_UINT16();
    atom_data->bps = READ_UINT16();
    atom_data->packet_size = READ_UINT16();
    atom_data->sample_rate = READ_UINT32();

    READ_BUF(atom_data->reserved3, 2);

    return 0;
}

size_t
mp4p_mp4a_atomdata_write (mp4p_mp4a_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 28;
    }
    uint8_t *origin = buffer;

    WRITE_BUF(atom_data->reserved, 6);
    WRITE_UINT16(atom_data->data_reference_index);

    WRITE_BUF(atom_data->reserved2, 8);

    WRITE_UINT16(atom_data->channel_count);
    WRITE_UINT16(atom_data->bps);
    WRITE_UINT16(atom_data->packet_size);
    WRITE_UINT32(atom_data->sample_rate);

    WRITE_BUF(atom_data->reserved3, 2);

    return buffer - origin;
}

void
mp4p_mp4a_atomdata_free (void *data) {
    mp4p_mp4a_t *mp4a = data;
    free (mp4a);
}

#pragma mark Opus

int
mp4p_Opus_atomdata_read (mp4p_Opus_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_BUF(atom_data->reserved, 6);
    atom_data->data_reference_index = READ_UINT16();

    READ_BUF(atom_data->reserved2, 8);

    atom_data->channel_count = READ_UINT16();
    atom_data->bps = READ_UINT16();
    if (atom_data->bps != 16) {
        return -1;
    }
    atom_data->packet_size = READ_UINT16();
    atom_data->sample_rate = READ_UINT32();
    if (atom_data->sample_rate != 48000) {
        return -1;
    }
    READ_BUF(atom_data->reserved3, 2);

    return 0;
}

size_t
mp4p_Opus_atomdata_write (mp4p_Opus_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 28;
    }
    uint8_t *origin = buffer;

    WRITE_BUF(atom_data->reserved, 6);
    WRITE_UINT16(atom_data->data_reference_index);

    WRITE_BUF(atom_data->reserved2, 8);

    WRITE_UINT16(atom_data->channel_count);
    WRITE_UINT16(atom_data->bps);
    WRITE_UINT16(atom_data->packet_size);
    WRITE_UINT32(atom_data->sample_rate);
    WRITE_BUF(atom_data->reserved3, 2);

    return buffer - origin;
}

void
mp4p_Opus_atomdata_free (void *data) {
    mp4p_Opus_t *Opus = data;
    free (Opus);
}

#pragma mark dOps

int
mp4p_dOps_atomdata_read (mp4p_dOps_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    atom_data->version = READ_UINT8();
    if (atom_data->version != 0) {
        return -1;
    }
    atom_data->output_channel_count = READ_UINT8();
    atom_data->pre_skip = READ_UINT16();
    atom_data->input_sample_rate = READ_UINT32();
    atom_data->output_gain = READ_INT16();
    atom_data->channel_mapping_family = READ_UINT8();
    if (atom_data->channel_mapping_family != 0) {
        atom_data->channel_mapping_table = calloc (sizeof (mp4p_opus_channel_mapping_table_t), atom_data->output_channel_count);
        for (int i = 0; i < atom_data->output_channel_count; i++) {
            atom_data->channel_mapping_table[i].channel_mapping = calloc(1, atom_data->output_channel_count);
            atom_data->channel_mapping_table[i].stream_count = READ_UINT8();
            atom_data->channel_mapping_table[i].coupled_count = READ_UINT8();
            for (int j = 0; j < atom_data->output_channel_count; j++) {
                atom_data->channel_mapping_table[i].channel_mapping[j] = READ_UINT8();
            }
        }
    }

    return 0;
}

size_t
mp4p_dOps_atomdata_write (mp4p_dOps_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 11 + (2+atom_data->output_channel_count) * atom_data->output_channel_count;
    }
    uint8_t *origin = buffer;

    WRITE_UINT8(atom_data->version);
    WRITE_UINT8(atom_data->output_channel_count);
    WRITE_UINT16(atom_data->pre_skip);
    WRITE_UINT32(atom_data->input_sample_rate);
    WRITE_INT16(atom_data->output_gain);
    WRITE_UINT8(atom_data->channel_mapping_family);
    for (int i = 0; i < atom_data->output_channel_count; i++) {
        WRITE_UINT8(atom_data->channel_mapping_table[i].stream_count);
        WRITE_UINT8(atom_data->channel_mapping_table[i].coupled_count);
        for (int j = 0; j < atom_data->output_channel_count; j++) {
            WRITE_UINT8(atom_data->channel_mapping_table[i].channel_mapping[j]);
        }
    }

    return buffer - origin;
}

void
mp4p_dOps_atomdata_free (void *data) {
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

#pragma mark esds

// read tag size, encoded in a 1-4 byte sequence, terminated when high bit is 0
int
read_esds_tag_size (uint8_t *buffer, size_t buffer_size, uint32_t *retval) {
    size_t initial_size = buffer_size;
    uint32_t num = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t val = READ_UINT8();
        num <<= 7;
        num |= (val & 0x7f);
        if (!(val & 0x80)) {
            break;
        }
    }
    *retval = num;
    return (int)(initial_size - buffer_size);
}

int
mp4p_esds_atomdata_read (mp4p_esds_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    atom_data->es_tag = READ_UINT8();
    if (atom_data->es_tag == 3)
    {
        int res = read_esds_tag_size (buffer, buffer_size, &atom_data->es_tag_size);
        if (res < 0) {
            return -1;
        }
        if (atom_data->es_tag_size < 20) {
            return -1;
        }

        buffer += res;
        buffer_size -= res;

        atom_data->ignored1 = READ_UINT8();
    }

    // FIXME: validate against spec -- could be that this is es_tag_size
    atom_data->ignored2 = READ_UINT8();
    atom_data->ignored3 = READ_UINT8();

    atom_data->dc_tag = READ_UINT8();
    if (atom_data->dc_tag != 4) {
        return -1;
    }

    int res = read_esds_tag_size (buffer, buffer_size, &atom_data->dc_tag_size);
    if (res < 0) {
        return -1;
    }
    if (atom_data->dc_tag_size < 13) {
        return -1;
    }
    buffer += res;
    buffer_size -= res;

    atom_data->dc_audiotype = READ_UINT8();
    atom_data->dc_audiostream = READ_UINT8();
    READ_BUF(atom_data->dc_buffersize_db, 3);

    atom_data->dc_max_bitrate = READ_UINT32();
    atom_data->dc_avg_bitrate = READ_UINT32();

    atom_data->ds_tag = READ_UINT8();
    if (atom_data->ds_tag != 5) {
        return -1;
    }

    res = read_esds_tag_size(buffer, buffer_size, &atom_data->asc_size);
    if (res < 0) {
        return -1;
    }
    buffer += res;
    buffer_size -= res;

    // FIXME: validate asc_size
    if (atom_data->asc_size) {
        atom_data->asc = malloc (atom_data->asc_size);
        READ_BUF(atom_data->asc, atom_data->asc_size);
    }

    return 0;
}

int
write_esds_tag_size (uint8_t *buffer, size_t buffer_size, uint32_t num) {
    uint8_t data[4] = {0};
    int count = 0;
    size_t initial_size = buffer_size;
    while (num) {
        data[count++] = num & 0x7f;
        num >>= 7;
    }
    if (count == 0) {
        count = 1;
    }

    for (int i = count-1; i >= 0; i--) {
        uint8_t val = data[i];
        if (i != 0) {
            val |= 0x80;
        }
        WRITE_UINT8(val);
    }
    return (int)(initial_size - buffer_size);
}

size_t
_esds_tag_written_size (uint32_t tag) {
    uint8_t buffer[4];
    size_t buffer_size = 4;
    return (size_t)write_esds_tag_size(buffer, buffer_size, tag);
}

size_t
mp4p_esds_atomdata_write (mp4p_esds_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        size_t size = 5;
        if (atom_data->es_tag == 3) {
            size += _esds_tag_written_size (atom_data->es_tag_size);
            size++;
        }

        size += 3;
        size += _esds_tag_written_size (atom_data->dc_tag_size);

        size += 14;

        size += _esds_tag_written_size (atom_data->asc_size);
        size += atom_data->asc_size;
        return size;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    WRITE_UINT8(atom_data->es_tag);
    if (atom_data->es_tag == 3)
    {
        int res = write_esds_tag_size (buffer, buffer_size, atom_data->es_tag_size);
        if (res < 0) {
            return 0;
        }

        buffer += res;
        buffer_size -= res;

        WRITE_UINT8(atom_data->ignored1);
    }

    WRITE_UINT8(atom_data->ignored2);
    WRITE_UINT8(atom_data->ignored3);

    WRITE_UINT8(atom_data->dc_tag);
    if (atom_data->dc_tag != 4) {
        return 0;
    }

    int res = write_esds_tag_size (buffer, buffer_size, atom_data->dc_tag_size);
    if (res < 0) {
        return 0;
    }

    buffer += res;
    buffer_size -= res;

    WRITE_UINT8(atom_data->dc_audiotype);
    WRITE_UINT8(atom_data->dc_audiostream);
    WRITE_BUF(atom_data->dc_buffersize_db, 3);

    WRITE_UINT32(atom_data->dc_max_bitrate);
    WRITE_UINT32(atom_data->dc_avg_bitrate);

    WRITE_UINT8(atom_data->ds_tag);

    res = write_esds_tag_size(buffer, buffer_size, atom_data->asc_size);
    if (res < 0) {
        return 0;
    }
    buffer += res;
    buffer_size -= res;

    if (atom_data->asc_size) {
        WRITE_BUF(atom_data->asc, atom_data->asc_size);
    }

    return buffer - origin;
}

void
mp4p_esds_atomdata_free (void *data) {
    mp4p_esds_t *esds = data;
    free (esds->asc);
    free (esds);
}

#pragma mark chpl

int
mp4p_chpl_atomdata_read (mp4p_chpl_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    READ_COMMON_HEADER();

    int i;

    atom_data->number_of_entries = READ_UINT8();

    if (atom_data->number_of_entries) {
        atom_data->entries = calloc (sizeof (mp4p_chpl_entry_t), atom_data->number_of_entries);
    }
    for(i = 0; i < atom_data->number_of_entries; i++)
    {
        atom_data->entries[i].start_time = READ_UINT64();
        uint8_t name_len = READ_UINT8();
        if (name_len > buffer_size) {
            name_len = buffer_size;
        }
        atom_data->entries[i].name_len = name_len;
        if (name_len) {
            atom_data->entries[i].name = malloc(name_len+1);
            READ_BUF(atom_data->entries[i].name, name_len);
        }
        atom_data->entries[i].name[name_len] = 0;
    }
    // FIXME: convert to qsort
    /* Bubble sort by increasing start date */
    do {
        for (i = 0; i < atom_data->number_of_entries - 1; i++) {
            if (atom_data->entries[i].start_time > atom_data->entries[i+1].start_time ) {

                mp4p_chpl_entry_t temp;

                memcpy (&temp, atom_data->entries+i+1, sizeof (mp4p_chpl_entry_t));

                memcpy (atom_data->entries+i+1, atom_data->entries+i, sizeof (mp4p_chpl_entry_t));

                memcpy (atom_data->entries+i, &temp, sizeof (mp4p_chpl_entry_t));

                i = -1;
                break;
            }
        }
    } while( i == -1 );

    return 0;
}

size_t
mp4p_chpl_atomdata_write (mp4p_chpl_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        size_t size = 5;
        for (int i = 0; i < atom_data->number_of_entries; i++) {
            size += 8 + 1 + atom_data->entries[i].name_len;
        }
        return size;
    }
    uint8_t *origin = buffer;

    WRITE_COMMON_HEADER();

    int i;

    WRITE_UINT8(atom_data->number_of_entries);

    for(i = 0; i < atom_data->number_of_entries; i++)
    {
        WRITE_UINT64(atom_data->entries[i].start_time);
        WRITE_UINT8(atom_data->entries[i].name_len);
        if (atom_data->entries[i].name_len) {
            WRITE_BUF(atom_data->entries[i].name, atom_data->entries[i].name_len);
        }
    }

    return buffer - origin;
}

void
mp4p_chpl_atomdata_free (void *data) {
    mp4p_chpl_t *chpl = data;
    for (int i = 0; i < chpl->number_of_entries; i++) {
        free (chpl->entries[i].name);
    }
    free (chpl->entries);
    free (data);
}

#pragma mark chap

int
mp4p_chap_atomdata_read (mp4p_chap_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    atom_data->number_of_entries = (uint32_t)(buffer_size / sizeof(uint32_t));
    if (atom_data->number_of_entries > 0) {
        atom_data->entries = calloc (atom_data->number_of_entries, sizeof(uint32_t));
    }
    else {
        return -1;
    }

    for (int i = 0; i < atom_data->number_of_entries; i++) {
        atom_data->entries[i] = READ_UINT32();
    }

    return 0;
}

size_t
mp4p_chap_atomdata_write (mp4p_chap_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return atom_data->number_of_entries * 4;
    }
    uint8_t *origin = buffer;

    for (int i = 0; i < atom_data->number_of_entries; i++) {
        WRITE_UINT32(atom_data->entries[i]);
    }

    return buffer - origin;
}

void
mp4p_chap_atomdata_free (void *atom_data) {
    mp4p_chap_t *chap = atom_data;
    free (chap->entries);
    free (atom_data);
}

#pragma mark ilst_custom

int
mp4p_ilst_meta_atomdata_read (mp4p_ilst_meta_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (atom_data->custom) {
        // mean subatom
        uint32_t mean_size = READ_UINT32();
        if (mean_size < 12) {
            return -1;
        }
        mean_size -= 12;

        char mean_type[4];
        READ_BUF(mean_type, 4);
        if (strncasecmp (mean_type, "mean", 4)) {
            return -1;
        }
        READ_UINT32(); // 0
        char *mean_data = malloc (mean_size + 1);
        READ_BUF(mean_data, mean_size);
        mean_data[mean_size] = 0;
        if (strncasecmp (mean_data, "com.apple.iTunes", 16)) {
            return -1;
        }

        // name subatom
        uint32_t name_size = READ_UINT32();
        if (name_size < 12) {
            return -1;
        }
        name_size -= 12;

        char name_type[4];
        READ_BUF(name_type, 4);
        if (strncasecmp (name_type, "name", 4)) {
            return -1;
        }

        READ_UINT32(); // 0

        atom_data->name = malloc (name_size + 1);
        READ_BUF(atom_data->name, name_size);
        atom_data->name[name_size] = 0;
    }

    // data subatom
    uint32_t data_size = READ_UINT32();
    if (data_size < 16) {
        return -1;
    }
    data_size -= 16;
    atom_data->data_size = data_size;

    char data_type[4];
    READ_BUF(data_type, 4);
    if (strncasecmp (data_type, "data", 4)) {
        return -1;
    }

    atom_data->data_version_flags = READ_UINT32();
    uint32_t flag = atom_data->data_version_flags & 0xff;

    READ_UINT32();

    if (flag == 0) {
        // array of numbers
        atom_data->values = calloc (atom_data->data_size / 2, sizeof (uint16_t));
        for (int i = 0; i < atom_data->data_size/2; i++) {
            atom_data->values[i] = READ_UINT16();
        }
    }
    else if (flag == 1) {
        // text
        // FIXME: why large texts were skipped?
        //        if (atom_data->data_size > 255 && strncasecmp (atom->type, COPYRIGHT_SYM "lyr", 4)) {
        //            return -1;
        //        }
        atom_data->text = calloc (atom_data->data_size+1, 1);
        READ_BUF(atom_data->text, atom_data->data_size);
        atom_data->text[atom_data->data_size] = 0;

        //        printf ("%s\n", meta->text);
    }
    else {
        // blob
        // FIXME: make it optional, i.e. skip and don't load to memory
        atom_data->blob = calloc (1, atom_data->data_size);
        READ_BUF(atom_data->blob, atom_data->data_size);
    }
    return 0;
}

size_t
mp4p_ilst_meta_atomdata_write (mp4p_ilst_meta_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        size_t size = 0;
        if (atom_data->custom) {
            size += 28; // mean
            size += 12 + strlen (atom_data->name); // name
        }
        size += 16 + atom_data->data_size;
        return size;
    }
    uint8_t *origin = buffer;

    // mean + name
    if (atom_data->name) {
        uint32_t mean_size = 28;
        WRITE_UINT32(mean_size);
        WRITE_BUF("mean", 4);
        WRITE_UINT32(0);
        WRITE_BUF("com.apple.iTunes", 16);

        uint32_t name_size = 12 + (uint32_t)strlen(atom_data->name);
        WRITE_UINT32(name_size);
        WRITE_BUF("name", 4);
        WRITE_UINT32(0);
        WRITE_BUF(atom_data->name, (uint32_t)strlen(atom_data->name));
    }
    // data atom
    if (atom_data->text || atom_data->values) {
        uint32_t data_atom_size = atom_data->data_size+16;
        WRITE_UINT32(data_atom_size);
        WRITE_BUF("data", 4);
    }
    WRITE_UINT32(atom_data->data_version_flags);
    WRITE_UINT32(0); // FIXME: what is this?

    if (atom_data->data_version_flags == 0) {
        for (int i = 0; i < atom_data->data_size/2; i++) {
            WRITE_UINT16(atom_data->values[i]);
        }
    }
    else if (atom_data->data_version_flags == 1) {
        WRITE_BUF(atom_data->text,atom_data->data_size);
    }
    else if (atom_data->data_version_flags == 2) {
        WRITE_BUF(atom_data->blob,atom_data->data_size);
    }
    else {
        return 0;
    }

    return buffer - origin;
}

void
mp4p_ilst_meta_atomdata_free (void *atom_data) {
    mp4p_ilst_meta_t *meta = atom_data;
    free (meta->name);
    free (meta->values);
    free (meta->text);
    free (meta->blob);
    free (atom_data);
}

#if 0
#pragma mark tmpl

int
mp4p_tmpl_atomdata_read (mp4p_tmpl_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    return 0;
}

size_t
mp4p_tmpl_atomdata_write (mp4p_tmpl_t *atom_data, uint8_t *buffer, size_t buffer_size) {
    if (!buffer) {
        return 100;
    }
    uint8_t *origin = buffer;


    return buffer - origin;
}

void
mp4p_tmpl_atomdata_free (void *atom_data) {
    free (atom_data);
}

#endif
