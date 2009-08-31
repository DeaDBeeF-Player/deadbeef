/*

libdemac - A Monkey's Audio decoder

$Id: parser.h 19552 2008-12-21 23:49:02Z amiconn $

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#ifndef _APE_PARSER_H
#define _APE_PARSER_H

#include <inttypes.h>
#include "demac_config.h"

/* The earliest and latest file formats supported by this library */
#define APE_MIN_VERSION 3970
#define APE_MAX_VERSION 3990

#define MAC_FORMAT_FLAG_8_BIT                 1    // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2    // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4    // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8    // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16    // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32    // create the wave header on decompression (not stored)


/* Special frame codes:

   MONO_SILENCE - All PCM samples in frame are zero (mono streams only)
   LEFT_SILENCE - All PCM samples for left channel in frame are zero (stereo streams)
   RIGHT_SILENCE - All PCM samples for left channel in frame are zero (stereo streams)
   PSEUDO_STEREO - Left and Right channels are identical

*/

#define APE_FRAMECODE_MONO_SILENCE    1
#define APE_FRAMECODE_LEFT_SILENCE    1 /* same as mono */
#define APE_FRAMECODE_RIGHT_SILENCE   2
#define APE_FRAMECODE_STEREO_SILENCE  3 /* combined */
#define APE_FRAMECODE_PSEUDO_STEREO   4

#define PREDICTOR_ORDER 8
/* Total size of all predictor histories - 50 * sizeof(int32_t) */
#define PREDICTOR_SIZE 50


/* NOTE: This struct is used in predictor-arm.S - any updates need to
   be reflected there. */

struct predictor_t
{
    /* Filter histories */
    int32_t* buf;

    int32_t YlastA;
    int32_t XlastA;

    /* NOTE: The order of the next four fields is important for
       predictor-arm.S */
    int32_t YfilterB;
    int32_t XfilterA;
    int32_t XfilterB;
    int32_t YfilterA;

    /* Adaption co-efficients */
    int32_t YcoeffsA[4];
    int32_t XcoeffsA[4];
    int32_t YcoeffsB[5];
    int32_t XcoeffsB[5];
    int32_t historybuffer[PREDICTOR_HISTORY_SIZE + PREDICTOR_SIZE];
};

struct ape_ctx_t
{
    /* Derived fields */
    uint32_t      junklength;
    uint32_t      firstframe;
    uint32_t      totalsamples;

    /* Info from Descriptor Block */
    char          magic[4];
    int16_t       fileversion;
    int16_t       padding1;
    uint32_t      descriptorlength;
    uint32_t      headerlength;
    uint32_t      seektablelength;
    uint32_t      wavheaderlength;
    uint32_t      audiodatalength;
    uint32_t      audiodatalength_high;
    uint32_t      wavtaillength;
    uint8_t       md5[16];

    /* Info from Header Block */
    uint16_t      compressiontype;
    uint16_t      formatflags;
    uint32_t      blocksperframe;
    uint32_t      finalframeblocks;
    uint32_t      totalframes;
    uint16_t      bps;
    uint16_t      channels;
    uint32_t      samplerate;

    /* Seektable */
    uint32_t*     seektable;        /* Seektable buffer */
    uint32_t      maxseekpoints;    /* Max seekpoints we can store (size of seektable buffer) */
    uint32_t      numseekpoints;    /* Number of seekpoints */
    int           seektablefilepos; /* Location in .ape file of seektable */

    /* Decoder state */
    uint32_t      CRC;
    int           frameflags;
    int           currentframeblocks;
    int           blocksdecoded;
    struct predictor_t predictor;
};

int ape_parseheader(int fd, struct ape_ctx_t* ape_ctx);
int ape_parseheaderbuf(unsigned char* buf, struct ape_ctx_t* ape_ctx);
void ape_dumpinfo(struct ape_ctx_t* ape_ctx);

#endif
