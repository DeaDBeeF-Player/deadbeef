/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003-2005 M. Bakker, Nero AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** The "appropriate copyright message" mentioned in section 2c of the GPLv2
** must read: "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Nero AG through Mpeg4AAClicense@nero.com.
**
** $Id: mp4atom.c,v 1.29 2009/01/19 23:56:30 menno Exp $
**/

#include <stdlib.h>
#ifndef _WIN32
#include "config.h"
#else
#include <tchar.h>
#include <windows.h>
#endif
#ifdef HAVE_GETPWUID
#    include <pwd.h>
#endif
#ifdef HAVE_STRING_H
#    include <string.h>
#endif
#include "mp4ffint.h"
#include <stdio.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)
#define min(x,y) ((x)<(y)?(x):(y))

#define       COPYRIGHT_SYMBOL        ((int8_t)0xA9)

/* parse atom header size */
static int32_t mp4ff_atom_get_size(const int8_t *data)
{
    uint32_t result;
    uint32_t a, b, c, d;

    a = (uint8_t)data[0];
    b = (uint8_t)data[1];
    c = (uint8_t)data[2];
    d = (uint8_t)data[3];

    result = (a<<24) | (b<<16) | (c<<8) | d;
    //if (result > 0 && result < 8) result = 8;

    return (int32_t)result;
}

/* comnapre 2 atom names, returns 1 for equal, 0 for unequal */
static int32_t mp4ff_atom_compare(const int8_t a1, const int8_t b1, const int8_t c1, const int8_t d1,
                                  const int8_t a2, const int8_t b2, const int8_t c2, const int8_t d2)
{
    if (a1 == a2 && b1 == b2 && c1 == c2 && d1 == d2)
        return 1;
    else
        return 0;
}

static uint8_t mp4ff_atom_name_to_type(const int8_t a, const int8_t b,
                                       const int8_t c, const int8_t d)
{
    if (a == 'm')
    {
        if (mp4ff_atom_compare(a,b,c,d, 'm','o','o','v'))
            return ATOM_MOOV;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','i','n','f'))
            return ATOM_MINF;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','d','i','a'))
            return ATOM_MDIA;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','d','a','t'))
            return ATOM_MDAT;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','d','h','d'))
            return ATOM_MDHD;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','v','h','d'))
            return ATOM_MVHD;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','p','4','a'))
            return ATOM_MP4A;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','p','4','v'))
            return ATOM_MP4V;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','p','4','s'))
            return ATOM_MP4S;
        else if (mp4ff_atom_compare(a,b,c,d, 'm','e','t','a'))
            return ATOM_META;
    } else if (a == 't') {
        if (mp4ff_atom_compare(a,b,c,d, 't','r','a','k'))
            return ATOM_TRAK;
        else if (mp4ff_atom_compare(a,b,c,d, 't','k','h','d'))
            return ATOM_TKHD;
        else if (mp4ff_atom_compare(a,b,c,d, 't','r','e','f'))
            return ATOM_TREF;
        else if (mp4ff_atom_compare(a,b,c,d, 't','r','k','n'))
            return ATOM_TRACK;
        else if (mp4ff_atom_compare(a,b,c,d, 't','m','p','o'))
            return ATOM_TEMPO;
        else if (mp4ff_atom_compare(a,b,c,d, 't','v','n','n'))
            return ATOM_NETWORK;
        else if (mp4ff_atom_compare(a,b,c,d, 't','v','s','h'))
            return ATOM_SHOW;
        else if (mp4ff_atom_compare(a,b,c,d, 't','v','e','n'))
            return ATOM_EPISODENAME;
        else if (mp4ff_atom_compare(a,b,c,d, 't','v','s','n'))
            return ATOM_SEASON;
        else if (mp4ff_atom_compare(a,b,c,d, 't','v','e','s'))
            return ATOM_EPISODE;
    } else if (a == 's') {
        if (mp4ff_atom_compare(a,b,c,d, 's','t','b','l'))
            return ATOM_STBL;
        else if (mp4ff_atom_compare(a,b,c,d, 's','m','h','d'))
            return ATOM_SMHD;
        else if (mp4ff_atom_compare(a,b,c,d, 's','t','s','d'))
            return ATOM_STSD;
        else if (mp4ff_atom_compare(a,b,c,d, 's','t','t','s'))
            return ATOM_STTS;
        else if (mp4ff_atom_compare(a,b,c,d, 's','t','c','o'))
            return ATOM_STCO;
        else if (mp4ff_atom_compare(a,b,c,d, 'c','o','6','4'))
            return ATOM_STCO;
        else if (mp4ff_atom_compare(a,b,c,d, 's','t','s','c'))
            return ATOM_STSC;
        else if (mp4ff_atom_compare(a,b,c,d, 's','t','s','z'))
            return ATOM_STSZ;
        else if (mp4ff_atom_compare(a,b,c,d, 's','t','z','2'))
            return ATOM_STZ2;
        else if (mp4ff_atom_compare(a,b,c,d, 's','k','i','p'))
            return ATOM_SKIP;
        else if (mp4ff_atom_compare(a,b,c,d, 's','i','n','f'))
            return ATOM_SINF;
        else if (mp4ff_atom_compare(a,b,c,d, 's','c','h','i'))
            return ATOM_SCHI;
        else if (mp4ff_atom_compare(a,b,c,d, 's','o','n','m'))
            return ATOM_SORTTITLE;
        else if (mp4ff_atom_compare(a,b,c,d, 's','o','a','l'))
            return ATOM_SORTALBUM;
        else if (mp4ff_atom_compare(a,b,c,d, 's','o','a','r'))
            return ATOM_SORTARTIST;
        else if (mp4ff_atom_compare(a,b,c,d, 's','o','a','a'))
            return ATOM_SORTALBUMARTIST;
        else if (mp4ff_atom_compare(a,b,c,d, 's','o','c','o'))
            return ATOM_SORTWRITER;
        else if (mp4ff_atom_compare(a,b,c,d, 's','o','s','n'))
            return ATOM_SORTSHOW;
    } else if (a == COPYRIGHT_SYMBOL) {
        if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'n','a','m'))
            return ATOM_TITLE;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'A','R','T'))
            return ATOM_ARTIST;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'w','r','t'))
            return ATOM_WRITER;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'a','l','b'))
            return ATOM_ALBUM;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'d','a','y'))
            return ATOM_DATE;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'t','o','o'))
            return ATOM_TOOL;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'c','m','t'))
            return ATOM_COMMENT;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'g','e','n'))
            return ATOM_GENRE1;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'g','r','p'))
            return ATOM_CONTENTGROUP;
        else if (mp4ff_atom_compare(a,b,c,d, COPYRIGHT_SYMBOL,'l','y','r'))
            return ATOM_LYRICS;
    }

    if (mp4ff_atom_compare(a,b,c,d, 'e','d','t','s'))
        return ATOM_EDTS;
    else if (mp4ff_atom_compare(a,b,c,d, 'e','s','d','s'))
        return ATOM_ESDS;
    else if (mp4ff_atom_compare(a,b,c,d, 'f','t','y','p'))
        return ATOM_FTYP;
    else if (mp4ff_atom_compare(a,b,c,d, 'f','r','e','e'))
        return ATOM_FREE;
    else if (mp4ff_atom_compare(a,b,c,d, 'h','m','h','d'))
        return ATOM_HMHD;
    else if (mp4ff_atom_compare(a,b,c,d, 'v','m','h','d'))
        return ATOM_VMHD;
    else if (mp4ff_atom_compare(a,b,c,d, 'u','d','t','a'))
        return ATOM_UDTA;
    else if (mp4ff_atom_compare(a,b,c,d, 'i','l','s','t'))
        return ATOM_ILST;
    else if (mp4ff_atom_compare(a,b,c,d, 'n','a','m','e'))
        return ATOM_NAME;
    else if (mp4ff_atom_compare(a,b,c,d, 'd','a','t','a'))
        return ATOM_DATA;
    else if (mp4ff_atom_compare(a,b,c,d, 'd','i','s','k'))
        return ATOM_DISC;
    else if (mp4ff_atom_compare(a,b,c,d, 'g','n','r','e'))
        return ATOM_GENRE2;
    else if (mp4ff_atom_compare(a,b,c,d, 'c','o','v','r'))
        return ATOM_COVER;
    else if (mp4ff_atom_compare(a,b,c,d, 'c','p','i','l'))
        return ATOM_COMPILATION;
    else if (mp4ff_atom_compare(a,b,c,d, 'c','t','t','s'))
        return ATOM_CTTS;
    else if (mp4ff_atom_compare(a,b,c,d, 'd','r','m','s'))
        return ATOM_DRMS;
    else if (mp4ff_atom_compare(a,b,c,d, 'f','r','m','a'))
        return ATOM_FRMA;
    else if (mp4ff_atom_compare(a,b,c,d, 'p','r','i','v'))
        return ATOM_PRIV;
    else if (mp4ff_atom_compare(a,b,c,d, 'i','v','i','v'))
        return ATOM_IVIV;
    else if (mp4ff_atom_compare(a,b,c,d, 'u','s','e','r'))
        return ATOM_USER;
    else if (mp4ff_atom_compare(a,b,c,d, 'k','e','y',' '))
        return ATOM_KEY;
    else if (mp4ff_atom_compare(a,b,c,d, 'a','A','R','T'))
        return ATOM_ALBUM_ARTIST;
    else if (mp4ff_atom_compare(a,b,c,d, 'd','e','s','c'))
        return ATOM_DESCRIPTION;
    else if (mp4ff_atom_compare(a,b,c,d, 'p','c','s','t'))
        return ATOM_PODCAST;
    else if (mp4ff_atom_compare(a,b,c,d, '-','-','-','-'))
        return ATOM_CUSTOM;
    else if (mp4ff_atom_compare(a,b,c,d, 'c','h','p','l'))
        return ATOM_CHPL;
    else if (mp4ff_atom_compare(a,b,c,d, 'c','h','a','p'))
        return ATOM_CHAP;
    else if (mp4ff_atom_compare(a,b,c,d, 't','e','x','t'))
        return ATOM_TEXT;
    else if (mp4ff_atom_compare(a,b,c,d, 's','u','b','p'))
        return ATOM_TEXT;
    else if (mp4ff_atom_compare(a,b,c,d, 't','x','3','g'))
        return ATOM_TEXT;
    else if (mp4ff_atom_compare(a,b,c,d, 's','b','t','l'))
        return ATOM_TEXT;
    else if (mp4ff_atom_compare(a,b,c,d, 'e','l','s','t'))
        return ATOM_ELST;
    else
        return ATOM_UNKNOWN;
}

/* read atom header, return atom size, atom size is with header included */
uint64_t mp4ff_atom_read_header(mp4ff_t *f, uint8_t *atom_type, uint8_t *header_size)
{
    uint64_t size;
    int32_t ret;
    int8_t atom_header[8];

    ret = mp4ff_read_data(f, atom_header, 8);
    if (ret != 8)
        return 0;

    size = mp4ff_atom_get_size(atom_header);
    *header_size = 8;

    /* check for 64 bit atom size */
    if (size == 1)
    {
        *header_size = 16;
        size = mp4ff_read_int64(f);
    }

//    printf("%c%c%c%c\n", atom_header[4], atom_header[5], atom_header[6], atom_header[7]);

    *atom_type = mp4ff_atom_name_to_type(atom_header[4], atom_header[5], atom_header[6], atom_header[7]);

    return size;
}

static int32_t mp4ff_read_stsz(mp4ff_t *f)
{
    trace ("mp4ff_read_stsz\n");
    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */
    f->track[f->total_tracks - 1]->stsz_sample_size = mp4ff_read_int32(f);
    f->track[f->total_tracks - 1]->stsz_sample_count = mp4ff_read_int32(f);

    if (f->track[f->total_tracks - 1]->stsz_sample_size == 0)
    {
        int32_t i;
        f->track[f->total_tracks - 1]->stsz_table = (int32_t*)malloc(f->track[f->total_tracks - 1]->stsz_sample_count*sizeof(int32_t));

        for (i = 0; i < f->track[f->total_tracks - 1]->stsz_sample_count; i++)
        {
            f->track[f->total_tracks - 1]->stsz_table[i] = mp4ff_read_int32(f);
        }
    }

    return 0;
}

static int32_t mp4ff_read_esds(mp4ff_t *f)
{
    uint8_t tag;
    uint32_t temp;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */

    /* get and verify ES_DescrTag */
    tag = mp4ff_read_char(f);
    if (tag == 0x03)
    {
        /* read length */
        if (mp4ff_read_mp4_descr_length(f) < 5 + 15)
        {
            return 1;
        }
        /* skip 3 bytes */
        mp4ff_read_int24(f);
    } else {
        /* skip 2 bytes */
        mp4ff_read_int16(f);
    }

    /* get and verify DecoderConfigDescrTab */
    if (mp4ff_read_char(f) != 0x04)
    {
        return 1;
    }

    /* read length */
    temp = mp4ff_read_mp4_descr_length(f);
    if (temp < 13) return 1;

    f->track[f->total_tracks - 1]->audioType = mp4ff_read_char(f);
    mp4ff_read_int32(f);//0x15000414 ????
    f->track[f->total_tracks - 1]->maxBitrate = mp4ff_read_int32(f);
    f->track[f->total_tracks - 1]->avgBitrate = mp4ff_read_int32(f);

    /* get and verify DecSpecificInfoTag */
    if (mp4ff_read_char(f) != 0x05)
    {
        return 1;
    }

    /* read length */
    f->track[f->total_tracks - 1]->decoderConfigLen = mp4ff_read_mp4_descr_length(f);

    if (f->track[f->total_tracks - 1]->decoderConfig)
        free(f->track[f->total_tracks - 1]->decoderConfig);
    f->track[f->total_tracks - 1]->decoderConfig = malloc(f->track[f->total_tracks - 1]->decoderConfigLen);
    if (f->track[f->total_tracks - 1]->decoderConfig)
    {
        mp4ff_read_data(f, f->track[f->total_tracks - 1]->decoderConfig, f->track[f->total_tracks - 1]->decoderConfigLen);
    } else {
        f->track[f->total_tracks - 1]->decoderConfigLen = 0;
    }

    /* will skip the remainder of the atom */
    return 0;
}

static int32_t mp4ff_read_mp4a(mp4ff_t *f)
{
    uint64_t size;
    int32_t i;
    uint8_t atom_type = 0;
    uint8_t header_size = 0;

    for (i = 0; i < 6; i++)
    {
        mp4ff_read_char(f); /* reserved */
    }
    /* data_reference_index */ mp4ff_read_int16(f);

    mp4ff_read_int32(f); /* reserved */
    mp4ff_read_int32(f); /* reserved */

    f->track[f->total_tracks - 1]->channelCount = mp4ff_read_int16(f);
    f->track[f->total_tracks - 1]->sampleSize = mp4ff_read_int16(f);

    mp4ff_read_int16(f);
    mp4ff_read_int16(f);

    f->track[f->total_tracks - 1]->sampleRate = mp4ff_read_int16(f);

    mp4ff_read_int16(f);

    size = mp4ff_atom_read_header(f, &atom_type, &header_size);
    if (atom_type == ATOM_ESDS)
    {
        mp4ff_read_esds(f);
    }

    return 0;
}

static int32_t mp4ff_read_stsd(mp4ff_t *f)
{
    int32_t i;
    uint8_t header_size = 0;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */

    f->track[f->total_tracks - 1]->stsd_entry_count = mp4ff_read_int32(f);

    for (i = 0; i < f->track[f->total_tracks - 1]->stsd_entry_count; i++)
    {
        uint64_t skip = mp4ff_position(f);
        uint64_t size;
        uint8_t atom_type = 0;
        size = mp4ff_atom_read_header(f, &atom_type, &header_size);
        skip += size;

        if (atom_type == ATOM_MP4A)
        {
            f->track[f->total_tracks - 1]->type = TRACK_AUDIO;
            mp4ff_read_mp4a(f);
        } else if (atom_type == ATOM_MP4V) {
            f->track[f->total_tracks - 1]->type = TRACK_VIDEO;
        } else if (atom_type == ATOM_MP4S) {
            f->track[f->total_tracks - 1]->type = TRACK_SYSTEM;
        } else if (atom_type == ATOM_TEXT) {
            f->track[f->total_tracks - 1]->type = TRACK_TEXT;
        } else {
            f->track[f->total_tracks - 1]->type = TRACK_UNKNOWN;
        }

        mp4ff_set_position(f, skip);
    }

    return 0;
}

static int32_t mp4ff_read_stsc(mp4ff_t *f)
{
    trace ("mp4ff_read_stsc\n");
    int32_t i;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */
    f->track[f->total_tracks - 1]->stsc_entry_count = mp4ff_read_int32(f);

    f->track[f->total_tracks - 1]->stsc_first_chunk =
        (int32_t*)malloc(f->track[f->total_tracks - 1]->stsc_entry_count*sizeof(int32_t));
    f->track[f->total_tracks - 1]->stsc_samples_per_chunk =
        (int32_t*)malloc(f->track[f->total_tracks - 1]->stsc_entry_count*sizeof(int32_t));
    f->track[f->total_tracks - 1]->stsc_sample_desc_index =
        (int32_t*)malloc(f->track[f->total_tracks - 1]->stsc_entry_count*sizeof(int32_t));

    for (i = 0; i < f->track[f->total_tracks - 1]->stsc_entry_count; i++)
    {
        f->track[f->total_tracks - 1]->stsc_first_chunk[i] = mp4ff_read_int32(f);
        f->track[f->total_tracks - 1]->stsc_samples_per_chunk[i] = mp4ff_read_int32(f);
        f->track[f->total_tracks - 1]->stsc_sample_desc_index[i] = mp4ff_read_int32(f);
    }

    return 0;
}

static int32_t mp4ff_read_stco(mp4ff_t *f)
{
    int32_t i;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */
    f->track[f->total_tracks - 1]->stco_entry_count = mp4ff_read_int32(f);

    f->track[f->total_tracks - 1]->stco_chunk_offset =
        (int32_t*)malloc(f->track[f->total_tracks - 1]->stco_entry_count*sizeof(int32_t));

    for (i = 0; i < f->track[f->total_tracks - 1]->stco_entry_count; i++)
    {
        f->track[f->total_tracks - 1]->stco_chunk_offset[i] = mp4ff_read_int32(f);
    }

    return 0;
}

static int32_t mp4ff_read_ctts(mp4ff_t *f)
{
    int32_t i;
    mp4ff_track_t * p_track = f->track[f->total_tracks - 1];

    if (p_track->ctts_entry_count) return 0;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */
    p_track->ctts_entry_count = mp4ff_read_int32(f);

    p_track->ctts_sample_count = (int32_t*)malloc(p_track->ctts_entry_count * sizeof(int32_t));
    p_track->ctts_sample_offset = (int32_t*)malloc(p_track->ctts_entry_count * sizeof(int32_t));

    if (p_track->ctts_sample_count == 0 || p_track->ctts_sample_offset == 0)
    {
        if (p_track->ctts_sample_count) {free(p_track->ctts_sample_count);p_track->ctts_sample_count=0;}
        if (p_track->ctts_sample_offset) {free(p_track->ctts_sample_offset);p_track->ctts_sample_offset=0;}
        p_track->ctts_entry_count = 0;
        return 0;
    }
    else
    {
        for (i = 0; i < f->track[f->total_tracks - 1]->ctts_entry_count; i++)
        {
            p_track->ctts_sample_count[i] = mp4ff_read_int32(f);
            p_track->ctts_sample_offset[i] = mp4ff_read_int32(f);
        }
        return 1;
    }
}

static int32_t mp4ff_read_stts(mp4ff_t *f)
{
    trace ("mp4ff_read_stts\n");
    int32_t i;
    mp4ff_track_t * p_track = f->track[f->total_tracks - 1];

    if (p_track->stts_entry_count) return 0;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */
    p_track->stts_entry_count = mp4ff_read_int32(f);

    p_track->stts_sample_count = (int32_t*)malloc(p_track->stts_entry_count * sizeof(int32_t));
    p_track->stts_sample_delta = (int32_t*)malloc(p_track->stts_entry_count * sizeof(int32_t));

    if (p_track->stts_sample_count == 0 || p_track->stts_sample_delta == 0)
    {
        if (p_track->stts_sample_count) {free(p_track->stts_sample_count);p_track->stts_sample_count=0;}
        if (p_track->stts_sample_delta) {free(p_track->stts_sample_delta);p_track->stts_sample_delta=0;}
        p_track->stts_entry_count = 0;
        return 0;
    }
    else
    {
        for (i = 0; i < f->track[f->total_tracks - 1]->stts_entry_count; i++)
        {
            p_track->stts_sample_count[i] = mp4ff_read_int32(f);
            p_track->stts_sample_delta[i] = mp4ff_read_int32(f);
        }
        return 1;
    }
}

static int32_t mp4ff_read_mvhd(mp4ff_t *f)
{
    int32_t i;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */
    /* creation_time */ mp4ff_read_int32(f);
    /* modification_time */ mp4ff_read_int32(f);
    f->time_scale = mp4ff_read_int32(f);
    f->duration = mp4ff_read_int32(f);
    /* preferred_rate */ mp4ff_read_int32(f); /*mp4ff_read_fixed32(f);*/
    /* preferred_volume */ mp4ff_read_int16(f); /*mp4ff_read_fixed16(f);*/
    for (i = 0; i < 10; i++)
    {
        /* reserved */ mp4ff_read_char(f);
    }
    for (i = 0; i < 9; i++)
    {
        mp4ff_read_int32(f); /* matrix */
    }
    /* preview_time */ mp4ff_read_int32(f);
    /* preview_duration */ mp4ff_read_int32(f);
    /* poster_time */ mp4ff_read_int32(f);
    /* selection_time */ mp4ff_read_int32(f);
    /* selection_duration */ mp4ff_read_int32(f);
    /* current_time */ mp4ff_read_int32(f);
    /* next_track_id */ mp4ff_read_int32(f);

    return 0;
}

static int32_t mp4ff_read_tkhd(mp4ff_t *f)
{
    uint8_t version;
    uint32_t flags;
    version = mp4ff_read_char(f); /* version */
    flags = mp4ff_read_int24(f); /* flags */
    if (version==1)
    {
        mp4ff_read_int64(f);//creation-time
        mp4ff_read_int64(f);//modification-time
        f->track[f->total_tracks - 1]->id = mp4ff_read_int32(f);//track-id
        mp4ff_read_int32(f);//reserved
//        f->track[f->total_tracks - 1]->duration = mp4ff_read_int64(f);//duration
    }
    else //version == 0
    {
        mp4ff_read_int32(f);//creation-time
        mp4ff_read_int32(f);//modification-time
        f->track[f->total_tracks - 1]->id = mp4ff_read_int32(f);//track-id
        mp4ff_read_int32(f);//reserved
//        f->track[f->total_tracks - 1]->duration = mp4ff_read_int32(f);//duration
//        if (f->track[f->total_tracks - 1]->duration == 0xFFFFFFFF)
//            f->track[f->total_tracks - 1]->duration = 0xFFFFFFFFFFFFFFFF;

    }
#if 0
    mp4ff_read_int32(f);//reserved
    mp4ff_read_int32(f);//reserved
    mp4ff_read_int16(f);//layer
    mp4ff_read_int16(f);//pre-defined
    mp4ff_read_int16(f);//volume
    mp4ff_read_int16(f);//reserved

    //matrix
    mp4ff_read_int32(f); mp4ff_read_int32(f); mp4ff_read_int32(f);
    mp4ff_read_int32(f); mp4ff_read_int32(f); mp4ff_read_int32(f);
    mp4ff_read_int32(f); mp4ff_read_int32(f); mp4ff_read_int32(f);
    mp4ff_read_int32(f);//width
    mp4ff_read_int32(f);//height
#endif
    return 1;
}

static int32_t mp4ff_read_mdhd(mp4ff_t *f)
{
    uint32_t version;

    version = mp4ff_read_int32(f);
    if (version==1)
    {
        mp4ff_read_int64(f);//creation-time
        mp4ff_read_int64(f);//modification-time
        f->track[f->total_tracks - 1]->timeScale = mp4ff_read_int32(f);//timescale
        f->track[f->total_tracks - 1]->duration = mp4ff_read_int64(f);//duration
    }
    else //version == 0
    {
        uint32_t temp;

        mp4ff_read_int32(f);//creation-time
        mp4ff_read_int32(f);//modification-time
        f->track[f->total_tracks - 1]->timeScale = mp4ff_read_int32(f);//timescale
        temp = mp4ff_read_int32(f);
        f->track[f->total_tracks - 1]->duration = (temp == (uint32_t)(-1)) ? (uint64_t)(-1) : (uint64_t)(temp);
    }
    mp4ff_read_int16(f);
    mp4ff_read_int16(f);
    return 1;
}
#ifdef USE_TAGGING
static int32_t mp4ff_read_meta(mp4ff_t *f, const uint64_t size)
{
    uint64_t subsize, sumsize = 0;
    uint8_t atom_type;
    uint8_t header_size = 0;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */

    while (sumsize < (size-(header_size+4)))
    {
        subsize = mp4ff_atom_read_header(f, &atom_type, &header_size);
        if (subsize <= header_size+4)
            return 1;
        if (atom_type == ATOM_ILST)
        {
            mp4ff_parse_metadata(f, (uint32_t)(subsize-(header_size+4)));
        } else {
            mp4ff_set_position(f, mp4ff_position(f)+subsize-header_size);
        }
        sumsize += subsize;
    }

    return 0;
}
#endif

static int32_t mp4ff_read_chpl(mp4ff_t *f, const uint64_t size)
{
    int i;
    int i_read = size;

    mp4ff_read_char(f); /* version */
    mp4ff_read_int24(f); /* flags */

    mp4ff_chapterdata_t *p_chpl = &f->chapters;

    p_chpl->i_chapter = mp4ff_read_char (f);
    i_read -= 5;

    for( i = 0; i < p_chpl->i_chapter; i++ )
    {
        uint64_t i_start;
        uint8_t i_len;
        int i_copy;
        i_start = mp4ff_read_int64 (f);
        i_read -= 8;
        i_len = mp4ff_read_char (f);
        i_read -= 1;

        p_chpl->chapter[i].psz_name = malloc( i_len + 1 );
        if( !p_chpl->chapter[i].psz_name )
            goto error;

        i_copy = i_len < i_read ? i_len : i_read;
        if( i_copy > 0 )
            mp4ff_read_data (f, p_chpl->chapter[i].psz_name, i_copy);
        p_chpl->chapter[i].psz_name[i_copy] = '\0';
        p_chpl->chapter[i].i_start = i_start;

        i_read -= i_copy;
    }
    /* Bubble sort by increasing start date */
    do
    {
        for( i = 0; i < p_chpl->i_chapter - 1; i++ )
        {
            if( p_chpl->chapter[i].i_start > p_chpl->chapter[i+1].i_start )
            {
                char *psz = p_chpl->chapter[i+1].psz_name;
                int64_t i64 = p_chpl->chapter[i+1].i_start;

                p_chpl->chapter[i+1].psz_name = p_chpl->chapter[i].psz_name;
                p_chpl->chapter[i+1].i_start = p_chpl->chapter[i].i_start;

                p_chpl->chapter[i].psz_name = psz;
                p_chpl->chapter[i].i_start = i64;

                i = -1;
                break;
            }
        }
    } while( i == -1 );

    return 0;

error:
    return -1;
}

int32_t mp4ff_chapters_get_num_items (mp4ff_t *f)
{
    return f->chapters.i_chapter;
}

const char *mp4ff_chapters_get_item (mp4ff_t *f, int i)
{
    return f->chapters.chapter[i].psz_name;
}

void mp4ff_chapters_free (mp4ff_t *f)
{
    int i;
    for (i = 0; i < f->chapters.i_chapter; i++)
    {
        free (f->chapters.chapter[i].psz_name);
        f->chapters.chapter[i].psz_name = NULL;
    }
}

static int32_t mp4ff_read_tref(mp4ff_t *f, const uint64_t size)
{
    int i;

    mp4ff_trefdata_t *p_tref = &f->tref;

    p_tref->i_track_ID = NULL;
    p_tref->i_entry_count = (size-8)/ sizeof(uint32_t);
    if( p_tref->i_entry_count > 0 ) {
        p_tref->i_track_ID = calloc( p_tref->i_entry_count, sizeof(uint32_t) );
    }
    if( p_tref->i_track_ID == NULL )
        return -1;

    for( i = 0; i < p_tref->i_entry_count; i++ )
    {
        p_tref->i_track_ID[i] = mp4ff_read_int32 (f);
    }

    return 0;
}

void mp4ff_tref_free (mp4ff_t *f)
{
    if (f->tref.i_track_ID) {
        free (f->tref.i_track_ID);
        f->tref.i_track_ID = NULL;
    }
}

int32_t mp4ff_chap_get_num_tracks (mp4ff_t *f)
{
    return f->tref.i_entry_count;
}

int32_t mp4ff_chap_get_track_id (mp4ff_t *f, int t)
{
    return f->tref.i_track_ID[t];
}

int32_t mp4ff_atom_read(mp4ff_t *f, const int32_t size, const uint8_t atom_type)
{
    uint64_t dest_position = mp4ff_position(f)+size-8;
    if (atom_type == ATOM_STSZ)
    {
        /* sample size box */
        mp4ff_read_stsz(f);
    } else if (atom_type == ATOM_STTS) {
        /* time to sample box */
        mp4ff_read_stts(f);
    } else if (atom_type == ATOM_CTTS) {
        /* composition offset box */
        mp4ff_read_ctts(f);
    } else if (atom_type == ATOM_STSC) {
        /* sample to chunk box */
        mp4ff_read_stsc(f);
    } else if (atom_type == ATOM_STCO) {
        /* chunk offset box */
        mp4ff_read_stco(f);
    } else if (atom_type == ATOM_STSD) {
        /* sample description box */
        mp4ff_read_stsd(f);
    } else if (atom_type == ATOM_MVHD) {
        /* movie header box */
        mp4ff_read_mvhd(f);
    } else if (atom_type == ATOM_MDHD) {
        /* track header */
        mp4ff_read_mdhd(f);
#ifdef USE_TAGGING
    } else if (atom_type == ATOM_META) {
        /* iTunes Metadata box */
        mp4ff_read_meta(f, size);
#endif
    } else if (atom_type == ATOM_CHPL) {
        mp4ff_read_chpl(f, size);
    } else if (atom_type == ATOM_CHAP) {
        mp4ff_read_tref(f, size);
    } else if (atom_type == ATOM_TKHD) {
        mp4ff_read_tkhd(f);
    }

    mp4ff_set_position(f, dest_position);


    return 0;
}

#if 0
int mp4ff_track_create_chunks_index(mp4ff_t *f, mp4ff_track_t *trk)
{
    unsigned int i_chunk;
    unsigned int i_index, i_last;

    // handle case when ( (!stco && !co64) || !stsc )
    // NOTE: stco and co64 have the same data

    if (!trk->stco_entry_count || !trk->stsc_entry_count)
    {
        trace ("no chunks\n");
        return -1;
    }

    trk->i_chunk_count = trk->stco_entry_count;

    trace ("\033[0;31mchunk count %d\033[37;0m\n", trk->i_chunk_count);
    trk->chunk_sample_first = malloc (sizeof (int32_t) * trk->i_chunk_count);
    trk->chunk_first_dts = malloc (sizeof (int32_t) * trk->i_chunk_count);
    trk->chunk_last_dts = malloc (sizeof (int32_t) * trk->i_chunk_count);
    trk->p_sample_count_dts = malloc (sizeof (int32_t *) * trk->i_chunk_count);
    trk->p_sample_delta_dts = malloc (sizeof (int32_t *) * trk->i_chunk_count);
    trk->p_sample_count_pts = malloc (sizeof (int32_t *) * trk->i_chunk_count);
    trk->p_sample_offset_pts = malloc (sizeof (int32_t *) * trk->i_chunk_count);

    /* first we read chunk offset */
    for( i_chunk = 0; i_chunk < trk->i_chunk_count; i_chunk++ )
    {
        // chunk.i_offset = stco_chunk_offset[i_chunk]
        trk->chunk_first_dts[i_chunk] = 0;
        trk->p_sample_count_dts[i_chunk] = NULL;
        trk->p_sample_delta_dts[i_chunk] = NULL;
        trk->p_sample_count_pts[i_chunk] = NULL;
        trk->p_sample_offset_pts[i_chunk] = NULL;
    }

    /* now we read index for SampleEntry( soun vide mp4a mp4v ...)
        to be used for the sample XXX begin to 1
        We construct it begining at the end */
    i_last = trk->i_chunk_count; /* last chunk proceded */
    i_index = trk->stsc_entry_count;
    if( !i_index )
    {
        trace ("cannot read chunk table or table empty\n");
        return -1;
    }
    trk->chunk_sample_first[0] = 0;
    trace ("\033[0;35mchunk_sample_first[%d]=%d\033[37;0m\n", 0, trk->chunk_sample_first[0]);
    for (int i = 1; i < trk->i_chunk_count; i++)
    {
        trk->chunk_sample_first[i] = 
            trk->chunk_sample_first[i-1] +
            trk->stsc_samples_per_chunk[i-1];
        trace ("\033[0;35mchunk_sample_first[%d]=%d\033[37;0m\n", i, trk->chunk_sample_first[i]);
    }

    trace ("track[Id 0x%x] read %d chunks\n", trk->id, trk->i_chunk_count);
    return 0;
}

int mp4ff_track_create_samples_index (mp4ff_t *f, mp4ff_track_t *trk)
{
    /* TODO use also stss and stsh table for seeking */
    /* FIXME use edit table */
    int64_t i_sample;
    int64_t i_chunk;

    int64_t i_index;
    int64_t i_index_sample_used;

    int64_t i_next_dts;

    if (trk->stsz_sample_count == 0) {
        trace ("stsz not found\n");
        return -1;
    }
    if (trk->stts_entry_count == 0) {
        trace ("stts not found\n");
        return -1;
    }

    // we already have the table, don't construct anything
    // sample_size = stsz_sample_size == 0 ? stsz_table[i_sample] : stsz_sample_size

    /* Use stts table to create a sample number -> dts table.
     * XXX: if we don't want to waste too much memory, we can't expand
     *  the box! so each chunk will contain an "extract" of this table
     *  for fast research (problem with raw stream where a sample is sometime
     *  just channels*bits_per_sample/8 */

    i_next_dts = 0;
    i_index = 0; i_index_sample_used = 0;
    for( i_chunk = 0; i_chunk < trk->i_chunk_count; i_chunk++ )
    {
        trace ("calculating first/last dts for chunk %d\n", i_chunk);
        int64_t i_entry, i_sample_count, i;

        /* save first dts */
        trk->chunk_first_dts[i_chunk] = i_next_dts;
        trk->chunk_last_dts[i_chunk]  = i_next_dts;
        trace ("init %lld\n", i_next_dts);

        /* count how many entries are needed for this chunk
         * for p_sample_delta_dts and p_sample_count_dts */
        i_sample_count = trk->stsc_samples_per_chunk[i_chunk];
        trace ("init i_sample_count %d\n", i_sample_count);

        i_entry = 0;
        while( i_sample_count > 0 )
        {
            i_sample_count -= trk->stts_sample_count[i_index+i_entry];
            trace ("- i_sample_count %d\n", i_sample_count);
            /* don't count already used sample in this entry */
            if( i_entry == 0 ) {
                i_sample_count += i_index_sample_used;
                trace ("+ i_sample_count %d\n", i_sample_count);
            }

            i_entry++;
        }
        trace ("+ i_entry %d\n", i_entry);
        /* allocate them */
        trace ("alloc mem for chunk %d (%d entries, %d samples-per-chunk)\n", i_chunk, i_entry, trk->stsc_samples_per_chunk[i_chunk]);
        trk->p_sample_count_dts[i_chunk] = calloc( i_entry, sizeof( uint32_t ) );
        trk->p_sample_delta_dts[i_chunk] = calloc( i_entry, sizeof( uint32_t ) );

        if( !trk->p_sample_count_dts[i_chunk] || !trk->p_sample_delta_dts[i_chunk] ) {
            trace ("out of memory allocating p_sample_count_dts or p_sample_delta_dts\n");
            return -1; // oom
        }

        /* now copy */
        i_sample_count = trk->stsc_samples_per_chunk[i_chunk];
        for( i = 0; i < i_entry; i++ )
        {
            int64_t i_used;
            int64_t i_rest;

            i_rest = trk->stts_sample_count[i_index] - i_index_sample_used;

            i_used = min( i_rest, i_sample_count );

            i_index_sample_used += i_used;
            i_sample_count -= i_used;
            i_next_dts += i_used * trk->stts_sample_delta[i_index];

            trk->p_sample_count_dts[i_chunk][i] = i_used;
            trk->p_sample_delta_dts[i_chunk][i] = trk->stts_sample_delta[i_index];
            if( i_used > 0 )
                trk->chunk_last_dts[i_chunk] = i_next_dts - trk->p_sample_delta_dts[i_chunk][i];

            if( i_index_sample_used >= trk->stts_sample_count[i_index] )
            {
                i_index++;
                i_index_sample_used = 0;
            }
        }
    }

    /* Find ctts
     *  Gives the delta between decoding time (dts) and composition table (pts)
     */
    if (trk->ctts_entry_count)
    {
        /* Create pts-dts table per chunk */
        i_index = 0; i_index_sample_used = 0;
        for( i_chunk = 0; i_chunk < trk->i_chunk_count; i_chunk++ )
        {
            int64_t i_entry, i_sample_count, i;

            /* count how many entries are needed for this chunk
             * for p_sample_delta_dts and p_sample_count_dts */
            i_sample_count = trk->stsc_samples_per_chunk[i_chunk];

            i_entry = 0;
            while( i_sample_count > 0 )
            {
                i_sample_count -= trk->ctts_sample_count[i_index+i_entry];

                /* don't count already used sample in this entry */
                if( i_entry == 0 )
                    i_sample_count += i_index_sample_used;

                i_entry++;
            }
            if (i_entry == 0) {
                continue;
            }

            /* allocate them */
            trk->p_sample_count_pts[i_chunk] = calloc( i_entry, sizeof( uint32_t ) );
            trk->p_sample_offset_pts[i_chunk] = calloc( i_entry, sizeof( int32_t ) );
            if( !trk->p_sample_count_pts[i_chunk] || !trk->p_sample_offset_pts[i_chunk] ) {
                trace ("out of memory allocating p_sample_count_pts or p_sample_offset_pts\n");
                return -1; // oom
            }

            /* now copy */
            i_sample_count = trk->stsc_samples_per_chunk[i_chunk];
            for( i = 0; i < i_entry; i++ )
            {
                int64_t i_used;
                int64_t i_rest;

                i_rest = trk->ctts_sample_count[i_index] -
                    i_index_sample_used;

                i_used = min( i_rest, i_sample_count );

                i_index_sample_used += i_used;
                i_sample_count -= i_used;

                trk->p_sample_count_pts[i_chunk][i] = i_used;
                trk->p_sample_offset_pts[i_chunk][i] = trk->ctts_sample_offset[i_index];

                if( i_index_sample_used >= trk->ctts_sample_count[i_index] )
                {
                    i_index++;
                    i_index_sample_used = 0;
                }
            }
        }
    }

    trace ("track[Id 0x%x] read %d samples length:%llds\n",
             trk->id, trk->stsz_sample_count,
             i_next_dts / trk->timeScale );

    return 0;
}

int64_t mp4ff_get_track_dts (mp4ff_t *f, int t, int s)
{
    mp4ff_track_t *p_track = f->track[t];
    // find chunk for the sample
    int i_chunk = 0;
    for (i_chunk = 0; i_chunk < p_track->i_chunk_count-1; i_chunk++) {
        if (p_track->chunk_sample_first[i_chunk+1] > s) {
            break;
        }
    }
//    trace ("i_chunk for sample %d: %d (out of %d)\n", s, i_chunk, p_track->i_chunk_count);

    unsigned int i_index = 0;
    unsigned int i_sample = s - p_track->chunk_sample_first[i_chunk];
    int64_t i_dts = p_track->chunk_first_dts[i_chunk];
    trace ("start dts: %lld (i_sample: %d)\n", i_dts, i_sample);

    while( i_sample > 0 )
    {
        if( i_sample > p_track->p_sample_count_dts[i_chunk][i_index] )
        {
            i_dts += p_track->p_sample_count_dts[i_chunk][i_index] *
                p_track->p_sample_delta_dts[i_chunk][i_index];
            i_sample -= p_track->p_sample_count_dts[i_chunk][i_index];
            i_index++;
        }
        else
        {
            i_dts += i_sample * p_track->p_sample_delta_dts[i_chunk][i_index];
            break;
        }
    }


#if 0
    // we don't need elst
    /* now handle elst */
    if (p_track->elst_entry_count)
    {
        /* convert to offset */
        if( ( p_track->elst_media_rate_integer[p_track->i_elst] > 0 ||
              p_track->elst_media_rate_fraction[p_track->i_elst] > 0 ) &&
            p_track->elst_media_time[p_track->i_elst] > 0 )
        {
            i_dts -= p_track->elst_media_time[p_track->i_elst];
        }

        /* add i_elst_time */
        i_dts += p_track->i_elst_time * p_track->timeScale /
            f->time_scale;

        if( i_dts < 0 ) i_dts = 0;
    }
#endif

    return (int64_t)1000000 * i_dts / p_track->timeScale;
}

int64_t mp4ff_get_track_pts_delta(mp4ff_t *f, int t, int i_sample)
{
    mp4ff_track_t *p_track = f->track[t];
    // find chunk for the sample
    int i_chunk = 0;
    for (i_chunk = 0; i_chunk < p_track->i_chunk_count-1; i_chunk++) {
        if (p_track->chunk_sample_first[i_chunk+1] > i_sample) {
            break;
        }
    }
//    trace ("i_chunk for sample %d: %d (out of %d)\n", i_sample, i_chunk, p_track->i_chunk_count);
    unsigned int i_index = 0;
    i_sample = i_sample - p_track->chunk_sample_first[i_chunk];

    if( p_track->p_sample_count_pts[i_chunk] == NULL || p_track->p_sample_offset_pts[i_chunk] == NULL ) {
        trace ("pts info not found :(\n");
        return -1;
    }

    for( i_index = 0;; i_index++ )
    {
        if( i_sample < p_track->p_sample_count_pts[i_chunk][i_index] )
            return p_track->p_sample_offset_pts[i_chunk][i_index] * (int64_t)1000000 /
                   (int64_t)p_track->timeScale;

        i_sample -= p_track->p_sample_count_pts[i_chunk][i_index];
    }
    return 0;
}

int mp4ff_get_track_sample_size(mp4ff_t *f, int t, int s)
{
    mp4ff_track_t *p_track = f->track[t];
    int i_size;

    if( p_track->stsz_sample_size == 0 )
    {
        /* most simple case */
        return p_track->stsz_table[s];
    }
    if(p_track->type != TRACK_AUDIO)
    {
        return p_track->stsz_sample_size;
    }

// that's all we need for chapters, sound length is calculated differently for now
    return 0;
#if 0
    p_soun = p_track->p_sample->data.p_sample_soun;

    if( p_soun->i_qt_version == 1 )
    {
        int i_samples = p_track->chunk[p_track->i_chunk].i_sample_count;
        if( p_track->fmt.audio.i_blockalign > 1 )
            i_samples = p_soun->i_sample_per_packet;

        i_size = i_samples / p_soun->i_sample_per_packet * p_soun->i_bytes_per_frame;
    }
    else if( p_track->i_sample_size > 256 )
    {
        /* We do that so we don't read too much data
         * (in this case we are likely dealing with compressed data) */
        i_size = p_track->i_sample_size;
    }
    else
    {
        /* Read a bunch of samples at once */
        int i_samples = p_track->chunk[p_track->i_chunk].i_sample_count -
            ( p_track->i_sample -
              p_track->chunk[p_track->i_chunk].i_sample_first );

        i_samples = __MIN( QT_V0_MAX_SAMPLES, i_samples );
        i_size = i_samples * p_track->i_sample_size;
    }

    //fprintf( stderr, "size=%d\n", i_size );
    return i_size;
#endif
}
#endif
