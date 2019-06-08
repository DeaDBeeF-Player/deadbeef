/*
 * Common bit i/o utils
 * Copyright (c) 2000, 2001 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 * Copyright (c) 2010 Loren Merritt
 *
 * alternative bitstream reader & writer by Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * bitstream api.
 */

//#include "avcodec.h"
#include "ffmpeg_get_bits.h"
#include "ffmpeg_intreadwrite.h"

#define av_log(...)

#ifdef ROCKBOX
#undef DEBUGF
#define DEBUGF(...)
#endif
#define trace(...) { fprintf (stderr, __VA_ARGS__); }
//#define trace(fmt,...)
#define DEBUGF trace

const uint8_t ff_log2_run[32]={
 0, 0, 0, 0, 1, 1, 1, 1,
 2, 2, 2, 2, 3, 3, 3, 3,
 4, 4, 5, 5, 6, 6, 7, 7,
 8, 9,10,11,12,13,14,15
};

#if 0 // unused in rockbox
void align_put_bits(PutBitContext *s)
{
#ifdef ALT_BITSTREAM_WRITER
    put_bits(s,(  - s->index) & 7,0);
#else
    put_bits(s,s->bit_left & 7,0);
#endif
}

void ff_put_string(PutBitContext *pb, const char *string, int terminate_string)
{
    while(*string){
        put_bits(pb, 8, *string);
        string++;
    }
    if(terminate_string)
        put_bits(pb, 8, 0);
}
#endif


/* VLC decoding */

//#define DEBUG_VLC

#define GET_DATA(v, table, i, wrap, size) \
{\
    const uint8_t *ptr = (const uint8_t *)table + i * wrap;\
    switch(size) {\
    case 1:\
        v = *(const uint8_t *)ptr;\
        break;\
    case 2:\
        v = *(const uint16_t *)ptr;\
        break;\
    default:\
        v = *(const uint32_t *)ptr;\
        break;\
    }\
}


static int alloc_table(VLC *vlc, int size, int use_static)
{
    int index;
    index = vlc->table_size;
    vlc->table_size += size;
    if (vlc->table_size > vlc->table_allocated) {
        if(use_static)
        {
            DEBUGF("init_vlc() used with too little memory : table_size > allocated_memory\n");
            return -1;
        }
//            abort(); //cant do anything, init_vlc() is used with too little memory
//        vlc->table_allocated += (1 << vlc->bits);
//        vlc->table = av_realloc(vlc->table,
//                                sizeof(VLC_TYPE) * 2 * vlc->table_allocated);
        if (!vlc->table)
            return -1;
    }
    return index;
}

/* 
static av_always_inline uint32_t bitswap_32(uint32_t x) {
    return av_reverse[x&0xFF]<<24
         | av_reverse[(x>>8)&0xFF]<<16
         | av_reverse[(x>>16)&0xFF]<<8
         | av_reverse[x>>24];
}
*/

typedef struct {
    uint8_t bits;
    uint16_t symbol;
    /** codeword, with the first bit-to-be-read in the msb
     * (even if intended for a little-endian bitstream reader) */
    uint32_t code;
} __attribute__((__packed__)) VLCcode; /* packed to save space */

static int compare_vlcspec(const void *a, const void *b)
{
    const VLCcode *sa=a, *sb=b;
    return (sa->code >> 1) - (sb->code >> 1);
}

/**
 * Build VLC decoding tables suitable for use with get_vlc().
 *
 * @param vlc            the context to be initted
 *
 * @param table_nb_bits  max length of vlc codes to store directly in this table
 *                       (Longer codes are delegated to subtables.)
 *
 * @param nb_codes       number of elements in codes[]
 *
 * @param codes          descriptions of the vlc codes
 *                       These must be ordered such that codes going into the same subtable are contiguous.
 *                       Sorting by VLCcode.code is sufficient, though not necessary.
 */
static int build_table(VLC *vlc, int table_nb_bits, int nb_codes,
                       VLCcode *codes, int flags)
{
    int table_size, table_index, index, symbol, subtable_bits;
    int i, j, k, n, nb, inc;
    uint32_t code, code_prefix;
    VLC_TYPE (*table)[2];

    table_size = 1 << table_nb_bits;
    table_index = alloc_table(vlc, table_size, flags & INIT_VLC_USE_NEW_STATIC);
#ifdef DEBUG_VLC
    av_log(NULL,AV_LOG_DEBUG,"new table index=%d size=%d\n",
           table_index, table_size);
#endif
    if (table_index < 0)
        return -1;
    table = &vlc->table[table_index];

    for (i = 0; i < table_size; i++) {
        table[i][1] = 0; //bits
        table[i][0] = -1; //codes
    }

    /* first pass: map codes and compute auxillary table sizes */
    for (i = 0; i < nb_codes; i++) {
        n = codes[i].bits;
        code = codes[i].code;
        symbol = codes[i].symbol;
#if defined(DEBUG_VLC) && 0
        av_log(NULL,AV_LOG_DEBUG,"i=%d n=%d code=0x%x\n", i, n, code);
#endif
        if (n <= table_nb_bits) {
            /* no need to add another table */
            j = code >> (32 - table_nb_bits);
            nb = 1 << (table_nb_bits - n);
            inc = 1;
/*            if (flags & INIT_VLC_LE) {
                j = bitswap_32(code);
                inc = 1 << n;
            } */
            for (k = 0; k < nb; k++) {
#ifdef DEBUG_VLC
                av_log(NULL, AV_LOG_DEBUG, "%4x: code=%d n=%d\n",
                       j, i, n);
#endif
                if (!table[j] || table[j][1] /*bits*/ != 0) {
                    av_log(NULL, AV_LOG_ERROR, "incorrect codes\n");
                    return -1;
                }
                table[j][1] = n; //bits
                table[j][0] = symbol;
                j += inc;
            }
        } else {
            /* fill auxiliary table recursively */
            n -= table_nb_bits;
            code_prefix = code >> (32 - table_nb_bits);
            subtable_bits = n;
            codes[i].bits = n;
            codes[i].code = code << table_nb_bits;
            for (k = i+1; k < nb_codes; k++) {
                n = codes[k].bits - table_nb_bits;
                if (n <= 0)
                    break;
                code = codes[k].code;
                if (code >> (32 - table_nb_bits) != code_prefix)
                    break;
                codes[k].bits = n;
                codes[k].code = code << table_nb_bits;
                subtable_bits = FFMAX(subtable_bits, n);
            }
            subtable_bits = FFMIN(subtable_bits, table_nb_bits);
            j = /*(flags & INIT_VLC_LE) ? bitswap_32(code_prefix) >> (32 - table_nb_bits) :*/ code_prefix;
            table[j][1] = -subtable_bits;
#ifdef DEBUG_VLC
            av_log(NULL,AV_LOG_DEBUG,"%4x: n=%d (subtable)\n",
                   j, codes[i].bits + table_nb_bits);
#endif
            index = build_table(vlc, subtable_bits, k-i, codes+i, flags);
            if (index < 0)
                return -1;
            /* note: realloc has been done, so reload tables */
            table = &vlc->table[table_index];
            table[j][0] = index; //code
            i = k-1;
        }
    }
    return table_index;
}


/* Build VLC decoding tables suitable for use with get_vlc().

   'nb_bits' set thee decoding table size (2^nb_bits) entries. The
   bigger it is, the faster is the decoding. But it should not be too
   big to save memory and L1 cache. '9' is a good compromise.

   'nb_codes' : number of vlcs codes

   'bits' : table which gives the size (in bits) of each vlc code.

   'codes' : table which gives the bit pattern of of each vlc code.

   'symbols' : table which gives the values to be returned from get_vlc().

   'xxx_wrap' : give the number of bytes between each entry of the
   'bits' or 'codes' tables.

   'xxx_size' : gives the number of bytes of each entry of the 'bits'
   or 'codes' tables.

   'wrap' and 'size' allows to use any memory configuration and types
   (byte/word/long) to store the 'bits', 'codes', and 'symbols' tables.

   'use_static' should be set to 1 for tables, which should be freed
   with av_free_static(), 0 if free_vlc() will be used.
*/

/* Rockbox: support for INIT_VLC_LE is currently disabled since none of our
   codecs use it, there's a LUT based bit reverse function for this commented
   out above (bitswap_32) and an inline asm version in libtremor/codebook.c
   if we ever want this */
   
static VLCcode buf[1336+1]; /* worst case is wma, which has one table with 1336 entries */

int init_vlc_sparse(VLC *vlc, int nb_bits, int nb_codes,
             const void *bits, int bits_wrap, int bits_size,
             const void *codes, int codes_wrap, int codes_size,
             const void *symbols, int symbols_wrap, int symbols_size,
             int flags)
{
    if (nb_codes+1 > (int)(sizeof (buf)/ sizeof (VLCcode)))
    {
        DEBUGF("Table is larger than temp buffer!\n");
        return -1;
    }

    int i, j, ret;

    vlc->bits = nb_bits;
    if(flags & INIT_VLC_USE_NEW_STATIC){
        if(vlc->table_size && vlc->table_size == vlc->table_allocated){
            return 0;
        }else if(vlc->table_size){
            DEBUGF("fatal error, we are called on a partially initialized table\n");
            return -1;
//            abort(); // fatal error, we are called on a partially initialized table
        }
    }else {
        vlc->table = NULL;
        vlc->table_allocated = 0;
        vlc->table_size = 0;
    }

#ifdef DEBUG_VLC
    av_log(NULL,AV_LOG_DEBUG,"build table nb_codes=%d\n", nb_codes);
#endif

//    buf = av_malloc((nb_codes+1)*sizeof(VLCcode));

//    assert(symbols_size <= 2 || !symbols);
    j = 0;
#define COPY(condition)\
    for (i = 0; i < nb_codes; i++) {\
        GET_DATA(buf[j].bits, bits, i, bits_wrap, bits_size);\
        if (!(condition))\
            continue;\
        GET_DATA(buf[j].code, codes, i, codes_wrap, codes_size);\
/*        if (flags & INIT_VLC_LE)*/\
/*            buf[j].code = bitswap_32(buf[j].code);*/\
/*        else*/\
            buf[j].code <<= 32 - buf[j].bits;\
        if (symbols)\
            GET_DATA(buf[j].symbol, symbols, i, symbols_wrap, symbols_size)\
        else\
            buf[j].symbol = i;\
        j++;\
    }
    COPY(buf[j].bits > nb_bits);
    // qsort is the slowest part of init_vlc, and could probably be improved or avoided
    qsort(buf, j, sizeof(VLCcode), compare_vlcspec);
    COPY(buf[j].bits && buf[j].bits <= nb_bits);
    nb_codes = j;

    ret = build_table(vlc, nb_bits, nb_codes, buf, flags);

//    av_free(buf);
    if (ret < 0) {
//        av_freep(&vlc->table);
        return -1;
    }
    if((flags & INIT_VLC_USE_NEW_STATIC) && vlc->table_size != vlc->table_allocated) {
        av_log(NULL, AV_LOG_ERROR, "needed %d had %d\n", vlc->table_size, vlc->table_allocated);
    }
    return 0;
}

/* not used in rockbox
void free_vlc(VLC *vlc)
{
    av_freep(&vlc->table);
}
*/

