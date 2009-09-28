/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko
    based on apedec from FFMpeg Copyright (c) 2007 Benjamin Zores <ben@geexbox.org>
    based upon libdemac from Dave Chapman.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
   main changes compared to ffmpeg:
     demuxer and decoder joined into 1 module
     no mallocs/reallocs during decoding
     streaming through fixed ringbuffer (small mem footprint)
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include "../../deadbeef.h"

#define ENABLE_DEBUG 0

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

//static float timestart;
//static float timeend;
static int startsample;
static int endsample;

#define PACKET_BUFFER_SIZE 100000

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static inline unsigned int bytestream_get_buffer(const uint8_t **b, uint8_t *dst, unsigned int size)
{
    memcpy(dst, *b, size);
    (*b) += size;
    return size;
}

static inline void bytestream_put_buffer(uint8_t **b, const uint8_t *src, unsigned int size)
{
    memcpy(*b, src, size);
    (*b) += size;
}

static inline uint8_t bytestream_get_byte (const uint8_t **ptr) {
    uint8_t v = *(*ptr);
    (*ptr)++;
    return v;
}

static inline uint32_t bytestream_get_be32 (const uint8_t **ptr) {
    const uint8_t *tmp = *ptr;
    uint32_t x = tmp[3] | (tmp[2] << 8) | (tmp[1] << 16) | (tmp[0] << 24);
    (*ptr) += 4;
    return x;
}


#define BLOCKS_PER_LOOP     4608
#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3

#define APE_FRAMECODE_MONO_SILENCE    1
#define APE_FRAMECODE_STEREO_SILENCE  3
#define APE_FRAMECODE_PSEUDO_STEREO   4

#define HISTORY_SIZE 512
#define PREDICTOR_ORDER 8
/** Total size of all predictor histories */
#define PREDICTOR_SIZE 50

#define YDELAYA (18 + PREDICTOR_ORDER*4)
#define YDELAYB (18 + PREDICTOR_ORDER*3)
#define XDELAYA (18 + PREDICTOR_ORDER*2)
#define XDELAYB (18 + PREDICTOR_ORDER)

#define YADAPTCOEFFSA 18
#define XADAPTCOEFFSA 14
#define YADAPTCOEFFSB 10
#define XADAPTCOEFFSB 5

/**
 * Possible compression levels
 * @{
 */
enum APECompressionLevel {
    COMPRESSION_LEVEL_FAST       = 1000,
    COMPRESSION_LEVEL_NORMAL     = 2000,
    COMPRESSION_LEVEL_HIGH       = 3000,
    COMPRESSION_LEVEL_EXTRA_HIGH = 4000,
    COMPRESSION_LEVEL_INSANE     = 5000
};
/** @} */

#define APE_FILTER_LEVELS 3

/** Filter orders depending on compression level */
static const uint16_t ape_filter_orders[5][APE_FILTER_LEVELS] = {
    {  0,   0,    0 },
    { 16,   0,    0 },
    { 64,   0,    0 },
    { 32, 256,    0 },
    { 16, 256, 1280 }
};

/** Filter fraction bits depending on compression level */
static const uint8_t ape_filter_fracbits[5][APE_FILTER_LEVELS] = {
    {  0,  0,  0 },
    { 11,  0,  0 },
    { 11,  0,  0 },
    { 10, 13,  0 },
    { 11, 13, 15 }
};


/** Filters applied to the decoded data */
typedef struct APEFilter {
    int16_t *coeffs;        ///< actual coefficients used in filtering
    int16_t *adaptcoeffs;   ///< adaptive filter coefficients used for correcting of actual filter coefficients
    int16_t *historybuffer; ///< filter memory
    int16_t *delay;         ///< filtered values

    int avg;
} APEFilter;

typedef struct APERice {
    uint32_t k;
    uint32_t ksum;
} APERice;

typedef struct APERangecoder {
    uint32_t low;           ///< low end of interval
    uint32_t range;         ///< length of interval
    uint32_t help;          ///< bytes_to_follow resp. intermediate value
    unsigned int buffer;    ///< buffer for input/output
} APERangecoder;

/** Filter histories */
typedef struct APEPredictor {
    int32_t *buf;

    int32_t lastA[2];

    int32_t filterA[2];
    int32_t filterB[2];

    int32_t coeffsA[2][4];  ///< adaption coefficients
    int32_t coeffsB[2][5];  ///< adaption coefficients
    int32_t historybuffer[HISTORY_SIZE + PREDICTOR_SIZE];
} APEPredictor;

/* The earliest and latest file formats supported by this library */
#define APE_MIN_VERSION 3950
#define APE_MAX_VERSION 3990

#define MAC_FORMAT_FLAG_8_BIT                 1 // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2 // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4 // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8 // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16 // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32 // create the wave header on decompression (not stored)

#define MAC_SUBFRAME_SIZE 4608

#define APE_EXTRADATA_SIZE 6

typedef struct {
    int64_t pos;
    int nblocks;
    int size;
    int skip;
} APEFrame;

/** Decoder context */
typedef struct APEContext {
    /* Derived fields */
    uint32_t junklength;
    uint32_t firstframe;
    uint32_t totalsamples;
    int currentframe;
    APEFrame *frames;

    /* Info from Descriptor Block */
    char magic[4];
    int16_t fileversion;
    int16_t padding1;
    uint32_t descriptorlength;
    uint32_t headerlength;
    uint32_t seektablelength;
    uint32_t wavheaderlength;
    uint32_t audiodatalength;
    uint32_t audiodatalength_high;
    uint32_t wavtaillength;
    uint8_t md5[16];

    /* Info from Header Block */
    uint16_t compressiontype;
    uint16_t formatflags;
    uint32_t blocksperframe;
    uint32_t finalframeblocks;
    uint32_t totalframes;
    uint16_t bps;
    uint16_t channels;
    uint32_t samplerate;
    int samples;                             ///< samples left to decode in current frame

    /* Seektable */
    uint32_t *seektable;

    int fset;                                ///< which filter set to use (calculated from compression level)
    int flags;                               ///< global decoder flags

    uint32_t CRC;                            ///< frame CRC
    int frameflags;                          ///< frame flags
    int currentframeblocks;                  ///< samples (per channel) in current frame
    int blocksdecoded;                       ///< count of decoded samples in current frame
    APEPredictor predictor;                  ///< predictor used for final reconstruction

    int32_t decoded0[BLOCKS_PER_LOOP];       ///< decoded data for the first channel
    int32_t decoded1[BLOCKS_PER_LOOP];       ///< decoded data for the second channel

    int16_t* filterbuf[APE_FILTER_LEVELS];   ///< filter memory

    APERangecoder rc;                        ///< rangecoder used to decode actual values
    APERice riceX;                           ///< rice code parameters for the second channel
    APERice riceY;                           ///< rice code parameters for the first channel
    APEFilter filters[APE_FILTER_LEVELS][2]; ///< filters used for reconstruction

    uint8_t *data_end;                       ///< frame data end
    const uint8_t *ptr;                      ///< current position in frame data
    const uint8_t *last_ptr;

    uint8_t packet_data[PACKET_BUFFER_SIZE];
    int packet_remaining; // number of bytes in packet_data
    int packet_sizeleft; // number of bytes left unread for current ape frame
    int samplestoskip;
    int currentsample; // current sample from beginning of file

    uint8_t buffer[BLOCKS_PER_LOOP * 2 * 2 * 2];
    int remaining;

    int error;
} APEContext;

APEContext ape_ctx;

inline static int
read_uint16(DB_FILE *fp, uint16_t* x)
{
    unsigned char tmp[2];
    int n;

    n = deadbeef->fread(tmp, 1, 2, fp);

    if (n != 2)
        return -1;

    *x = tmp[0] | (tmp[1] << 8);

    return 0;
}


inline static int
read_int16(DB_FILE *fp, int16_t* x)
{
    return read_uint16(fp, (uint16_t*)x);
}

inline static int
read_uint32(DB_FILE *fp, uint32_t* x)
{
    unsigned char tmp[4];
    int n;

    n = deadbeef->fread(tmp, 1, 4, fp);

    if (n != 4)
        return -1;

    *x = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);

    return 0;
}

static void ape_dumpinfo(APEContext * ape_ctx)
{
#if ENABLE_DEBUG
    int i;

    fprintf (stderr, "Descriptor Block:\n\n");
    fprintf (stderr, "magic                = \"%c%c%c%c\"\n", ape_ctx->magic[0], ape_ctx->magic[1], ape_ctx->magic[2], ape_ctx->magic[3]);
    fprintf (stderr, "fileversion          = %d\n", ape_ctx->fileversion);
    fprintf (stderr, "descriptorlength     = %d\n", ape_ctx->descriptorlength);
    fprintf (stderr, "headerlength         = %d\n", ape_ctx->headerlength);
    fprintf (stderr, "seektablelength      = %d\n", ape_ctx->seektablelength);
    fprintf (stderr, "wavheaderlength      = %d\n", ape_ctx->wavheaderlength);
    fprintf (stderr, "audiodatalength      = %d\n", ape_ctx->audiodatalength);
    fprintf (stderr, "audiodatalength_high = %d\n", ape_ctx->audiodatalength_high);
    fprintf (stderr, "wavtaillength        = %d\n", ape_ctx->wavtaillength);
    fprintf (stderr, "md5                  = ");
    for (i = 0; i < 16; i++)
         fprintf (stderr, "%02x", ape_ctx->md5[i]);
    fprintf (stderr, "\n");

    fprintf (stderr, "\nHeader Block:\n\n");

    fprintf (stderr, "compressiontype      = %d\n", ape_ctx->compressiontype);
    fprintf (stderr, "formatflags          = %d\n", ape_ctx->formatflags);
    fprintf (stderr, "blocksperframe       = %d\n", ape_ctx->blocksperframe);
    fprintf (stderr, "finalframeblocks     = %d\n", ape_ctx->finalframeblocks);
    fprintf (stderr, "totalframes          = %d\n", ape_ctx->totalframes);
    fprintf (stderr, "bps                  = %d\n", ape_ctx->bps);
    fprintf (stderr, "channels             = %d\n", ape_ctx->channels);
    fprintf (stderr, "samplerate           = %d\n", ape_ctx->samplerate);

    fprintf (stderr, "\nSeektable\n\n");
    if ((ape_ctx->seektablelength / sizeof(uint32_t)) != ape_ctx->totalframes) {
        fprintf (stderr, "No seektable\n");
    } else {
        for (i = 0; i < ape_ctx->seektablelength / sizeof(uint32_t); i++) {
            if (i < ape_ctx->totalframes - 1) {
                fprintf (stderr, "%8d   %d (%d bytes)\n", i, ape_ctx->seektable[i], ape_ctx->seektable[i + 1] - ape_ctx->seektable[i]);
            } else {
                fprintf (stderr, "%8d   %d\n", i, ape_ctx->seektable[i]);
            }
        }
    }

    fprintf (stderr, "\nFrames\n\n");
    for (i = 0; i < ape_ctx->totalframes; i++)
        fprintf (stderr, "%8d   %8lld %8d (%d samples)\n", i, ape_ctx->frames[i].pos, ape_ctx->frames[i].size, ape_ctx->frames[i].nblocks);

    fprintf (stderr, "\nCalculated information:\n\n");
    fprintf (stderr, "junklength           = %d\n", ape_ctx->junklength);
    fprintf (stderr, "firstframe           = %d\n", ape_ctx->firstframe);
    fprintf (stderr, "totalsamples         = %d\n", ape_ctx->totalsamples);
#endif
}

static int
ape_read_header(DB_FILE *fp, APEContext *ape)
{
    int i;
    int total_blocks;

    /* TODO: Skip any leading junk such as id3v2 tags */
    ape->junklength = 0;

    if (deadbeef->fread (ape->magic, 1, 4, fp) != 4) {
        return -1;
    }
    if (memcmp (ape->magic, "MAC ", 4))
        return -1;

    if (read_uint16 (fp, &ape->fileversion) < 0) {
        return -1;
    }

    if (ape->fileversion < APE_MIN_VERSION || ape->fileversion > APE_MAX_VERSION) {
        fprintf (stderr, "Unsupported file version - %d.%02d\n", ape->fileversion / 1000, (ape->fileversion % 1000) / 10);
        return -1;
    }

    if (ape->fileversion >= 3980) {
        if (read_uint16 (fp, &ape->padding1) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->descriptorlength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->headerlength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->seektablelength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->wavheaderlength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->audiodatalength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->audiodatalength_high) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->wavtaillength) < 0) {
            return -1;
        }
        if (deadbeef->fread (ape->md5, 1, 16, fp) != 16) {
            return -1;
        }

        /* Skip any unknown bytes at the end of the descriptor.
           This is for future compatibility */
        if (ape->descriptorlength > 52) {
            deadbeef->fseek (fp, ape->descriptorlength - 52, SEEK_CUR);
        }

        /* Read header data */
        if (read_uint16 (fp, &ape->compressiontype) < 0) {
            return -1;
        }
        if (read_uint16 (fp, &ape->formatflags) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->blocksperframe) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->finalframeblocks) < 0) {
            return -1;
        }
        if (read_uint32 (fp, & ape->totalframes) < 0) {
            return -1;
        }
        if (read_uint16 (fp, &ape->bps) < 0) {
            return -1;
        }
        if (read_uint16 (fp, &ape->channels) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->samplerate) < 0) {
            return -1;
        }
    } else {
        ape->descriptorlength = 0;
        ape->headerlength = 32;

        if (read_uint16 (fp, &ape->compressiontype) < 0) {
            return -1;
        }
        if (read_uint16 (fp, &ape->formatflags) < 0) {
            return -1;
        }
        if (read_uint16 (fp, &ape->channels) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->samplerate) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->wavheaderlength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->wavtaillength) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->totalframes) < 0) {
            return -1;
        }
        if (read_uint32 (fp, &ape->finalframeblocks) < 0) {
            return -1;
        }

        if (ape->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL) {
            deadbeef->fseek(fp, 4, SEEK_CUR); /* Skip the peak level */
            ape->headerlength += 4;
        }

        if (ape->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS) {
            if (read_uint32 (fp, &ape->seektablelength) < 0) {
                return -1;
            };
            ape->headerlength += 4;
            ape->seektablelength *= sizeof(int32_t);
        } else
            ape->seektablelength = ape->totalframes * sizeof(int32_t);

        if (ape->formatflags & MAC_FORMAT_FLAG_8_BIT)
            ape->bps = 8;
        else if (ape->formatflags & MAC_FORMAT_FLAG_24_BIT)
            ape->bps = 24;
        else
            ape->bps = 16;

        if (ape->fileversion >= 3950)
            ape->blocksperframe = 73728 * 4;
        else if (ape->fileversion >= 3900 || (ape->fileversion >= 3800  && ape->compressiontype >= 4000))
            ape->blocksperframe = 73728;
        else
            ape->blocksperframe = 9216;

        /* Skip any stored wav header */
        if (!(ape->formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER)) {
            deadbeef->fseek (fp, ape->wavheaderlength, SEEK_CUR);
        }
    }

    if(ape->totalframes > UINT_MAX / sizeof(APEFrame)){
        fprintf (stderr, "Too many frames: %d\n", ape->totalframes);
        return -1;
    }
    ape->frames       = malloc(ape->totalframes * sizeof(APEFrame));
    if(!ape->frames)
        return -1;
    ape->firstframe   = ape->junklength + ape->descriptorlength + ape->headerlength + ape->seektablelength + ape->wavheaderlength;
    ape->currentframe = 0;


    ape->totalsamples = ape->finalframeblocks;
    if (ape->totalframes > 1)
        ape->totalsamples += ape->blocksperframe * (ape->totalframes - 1);

    if (ape->seektablelength > 0) {
        ape->seektable = malloc(ape->seektablelength);
        for (i = 0; i < ape->seektablelength / sizeof(uint32_t); i++) {
            if (read_uint32 (fp, &ape->seektable[i]) < 0) {
                return -1;
            }
        }
    }

    ape->frames[0].pos     = ape->firstframe;
    ape->frames[0].nblocks = ape->blocksperframe;
    ape->frames[0].skip    = 0;
    for (i = 1; i < ape->totalframes; i++) {
        ape->frames[i].pos      = ape->seektable[i]; //ape->frames[i-1].pos + ape->blocksperframe;
        ape->frames[i].nblocks  = ape->blocksperframe;
        ape->frames[i - 1].size = ape->frames[i].pos - ape->frames[i - 1].pos;
        ape->frames[i].skip     = (ape->frames[i].pos - ape->frames[0].pos) & 3;
    }
    ape->frames[ape->totalframes - 1].size    = ape->finalframeblocks * 4;
    ape->frames[ape->totalframes - 1].nblocks = ape->finalframeblocks;

    for (i = 0; i < ape->totalframes; i++) {
        if(ape->frames[i].skip){
            ape->frames[i].pos  -= ape->frames[i].skip;
            ape->frames[i].size += ape->frames[i].skip;
        }
        ape->frames[i].size = (ape->frames[i].size + 3) & ~3;
    }


    ape_dumpinfo(ape);

#if ENABLE_DEBUG
    fprintf (stderr, "Decoding file - v%d.%02d, compression level %d\n", ape->fileversion / 1000, (ape->fileversion % 1000) / 10, ape->compressiontype);
#endif

    total_blocks = (ape->totalframes == 0) ? 0 : ((ape->totalframes - 1) * ape->blocksperframe) + ape->finalframeblocks;

    return 0;
}

#   define AV_WB32(p, d) do {                   \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)

#define AV_WL32(p, v) AV_WB32(p, bswap_32(v))

static inline const uint32_t bswap_32(uint32_t x)
{
    x= ((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
    x= (x>>16) | (x<<16);
    return x;
}

static int ape_read_packet(DB_FILE *fp, APEContext *ape_ctx)
{
    int ret;
    int nblocks;
    APEContext *ape = ape_ctx;
    uint32_t extra_size = 8;

    if (ape->currentframe > ape->totalframes)
        return -1;
//    fprintf (stderr, "seeking to %d\n", ape->frames[ape->currentframe].pos);
    if (deadbeef->fseek (fp, ape->frames[ape->currentframe].pos, SEEK_SET) != 0) {
        return -1;
    }

    /* Calculate how many blocks there are in this frame */
    if (ape->currentframe == (ape->totalframes - 1))
        nblocks = ape->finalframeblocks;
    else
        nblocks = ape->blocksperframe;

//    if (PACKET_MAX_SIZE < ape->frames[ape->currentframe].size + extra_size) {
//        return -1;
//    }
//    packet_sizeleft = ape->frames[ape->currentframe].size + extra_size;

    AV_WL32(ape->packet_data    , nblocks);
    AV_WL32(ape->packet_data + 4, ape->frames[ape->currentframe].skip);
//    packet_sizeleft -= 8;

    int sz = PACKET_BUFFER_SIZE-8;
    sz = min (sz, ape->frames[ape->currentframe].size);
//    fprintf (stderr, "readsize: %d, packetsize: %d\n", sz, ape->frames[ape->currentframe].size);
    ret = deadbeef->fread (ape->packet_data + extra_size, 1, sz, fp);
    ape->packet_sizeleft = ape->frames[ape->currentframe].size - sz + 8;
    ape->packet_remaining = sz+8;

    ape->currentframe++;

    return 0;
}

static void
ape_free_ctx (APEContext *ape_ctx) {
    int i;
    if (ape_ctx->frames) {
        free (ape_ctx->frames);
        ape_ctx->frames = NULL;
    }
    if (ape_ctx->seektable) {
        free (ape_ctx->seektable);
        ape_ctx->seektable = NULL;
    }
    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (ape_ctx->filterbuf) {
            free (ape_ctx->filterbuf[i]);
            ape_ctx->filterbuf[i] = NULL;
        }
    }
}

static void
ffap_free (void)
{
    ape_free_ctx (&ape_ctx);
}

#if 0
static int ape_read_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags)
{
    AVStream *st = s->streams[stream_index];
    APEContext *ape = s->priv_data;
    int index = av_index_search_timestamp(st, timestamp, flags);

    if (index < 0)
        return -1;

    ape->currentframe = index;
    return 0;
}
#endif

static DB_FILE *fp;

static int
ffap_init(DB_playItem_t *it)
{
    fp = deadbeef->fopen (it->fname);
    if (!fp) {
        return -1;
    }
    memset (&ape_ctx, 0, sizeof (ape_ctx));
    ape_read_header (fp, &ape_ctx);
    int i;

    if (ape_ctx.bps != 16) {
        fprintf (stderr, "Only 16-bit samples are supported\n");
        return -1;
    }
    if (ape_ctx.channels > 2) {
        fprintf (stderr, "Only mono and stereo is supported\n");
        return -1;
    }

#if ENABLE_DEBUG
    fprintf (stderr, "Compression Level: %d - Flags: %d\n", ape_ctx.compressiontype, ape_ctx.formatflags);
#endif
    if (ape_ctx.compressiontype % 1000 || ape_ctx.compressiontype > COMPRESSION_LEVEL_INSANE) {
        fprintf (stderr, "Incorrect compression level %d\n", ape_ctx.compressiontype);
        return -1;
    }
    ape_ctx.fset = ape_ctx.compressiontype / 1000 - 1;
    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (!ape_filter_orders[ape_ctx.fset][i])
            break;
        ape_ctx.filterbuf[i] = malloc((ape_filter_orders[ape_ctx.fset][i] * 3 + HISTORY_SIZE) * 4);
    }

    plugin.info.bps = ape_ctx.bps;
    plugin.info.samplerate = ape_ctx.samplerate;
    plugin.info.channels = ape_ctx.channels;
    plugin.info.readpos = 0;
    if (it->endsample > 0) {
        startsample = it->startsample;
        endsample = it->endsample;
//        timestart = it->timestart;
//        timeend = it->timeend;
        plugin.seek_sample (0);
        //trace ("start: %d/%f, end: %d/%f\n", startsample, timestart, endsample, timeend);
    }
    else {
        //timestart = 0;
        //timeend = it->duration;
        startsample = 0;
        endsample = ape_ctx.totalsamples-1;
    }
    return 0;
}

/**
 * @defgroup rangecoder APE range decoder
 * @{
 */

#define CODE_BITS    32
#define TOP_VALUE    ((unsigned int)1 << (CODE_BITS-1))
#define SHIFT_BITS   (CODE_BITS - 9)
#define EXTRA_BITS   ((CODE_BITS-2) % 8 + 1)
#define BOTTOM_VALUE (TOP_VALUE >> 8)

/** Start the decoder */
static inline void range_start_decoding(APEContext * ctx)
{
    ctx->rc.buffer = bytestream_get_byte(&ctx->ptr);
    ctx->rc.low    = ctx->rc.buffer >> (8 - EXTRA_BITS);
    ctx->rc.range  = (uint32_t) 1 << EXTRA_BITS;
}

/** Perform normalization */
static inline void range_dec_normalize(APEContext * ctx)
{
    while (ctx->rc.range <= BOTTOM_VALUE) {
        ctx->rc.buffer <<= 8;
        if(ctx->ptr < ctx->data_end)
            ctx->rc.buffer += *ctx->ptr;
        ctx->ptr++;
        ctx->rc.low    = (ctx->rc.low << 8)    | ((ctx->rc.buffer >> 1) & 0xFF);
        ctx->rc.range  <<= 8;
    }
}

/**
 * Calculate culmulative frequency for next symbol. Does NO update!
 * @param ctx decoder context
 * @param tot_f is the total frequency or (code_value)1<<shift
 * @return the culmulative frequency
 */
static inline int range_decode_culfreq(APEContext * ctx, int tot_f)
{
    range_dec_normalize(ctx);
    ctx->rc.help = ctx->rc.range / tot_f;
    return ctx->rc.low / ctx->rc.help;
}

/**
 * Decode value with given size in bits
 * @param ctx decoder context
 * @param shift number of bits to decode
 */
static inline int range_decode_culshift(APEContext * ctx, int shift)
{
    range_dec_normalize(ctx);
    ctx->rc.help = ctx->rc.range >> shift;
    return ctx->rc.low / ctx->rc.help;
}


/**
 * Update decoding state
 * @param ctx decoder context
 * @param sy_f the interval length (frequency of the symbol)
 * @param lt_f the lower end (frequency sum of < symbols)
 */
static inline void range_decode_update(APEContext * ctx, int sy_f, int lt_f)
{
    ctx->rc.low  -= ctx->rc.help * lt_f;
    ctx->rc.range = ctx->rc.help * sy_f;
}

/** Decode n bits (n <= 16) without modelling */
static inline int range_decode_bits(APEContext * ctx, int n)
{
    int sym = range_decode_culshift(ctx, n);
    range_decode_update(ctx, 1, sym);
    return sym;
}


#define MODEL_ELEMENTS 64

/**
 * Fixed probabilities for symbols in Monkey Audio version 3.97
 */
static const uint16_t counts_3970[22] = {
        0, 14824, 28224, 39348, 47855, 53994, 58171, 60926,
    62682, 63786, 64463, 64878, 65126, 65276, 65365, 65419,
    65450, 65469, 65480, 65487, 65491, 65493,
};

/**
 * Probability ranges for symbols in Monkey Audio version 3.97
 */
static const uint16_t counts_diff_3970[21] = {
    14824, 13400, 11124, 8507, 6139, 4177, 2755, 1756,
    1104, 677, 415, 248, 150, 89, 54, 31,
    19, 11, 7, 4, 2,
};

/**
 * Fixed probabilities for symbols in Monkey Audio version 3.98
 */
static const uint16_t counts_3980[22] = {
        0, 19578, 36160, 48417, 56323, 60899, 63265, 64435,
    64971, 65232, 65351, 65416, 65447, 65466, 65476, 65482,
    65485, 65488, 65490, 65491, 65492, 65493,
};

/**
 * Probability ranges for symbols in Monkey Audio version 3.98
 */
static const uint16_t counts_diff_3980[21] = {
    19578, 16582, 12257, 7906, 4576, 2366, 1170, 536,
    261, 119, 65, 31, 19, 10, 6, 3,
    3, 2, 1, 1, 1,
};

/**
 * Decode symbol
 * @param ctx decoder context
 * @param counts probability range start position
 * @param counts_diff probability range widths
 */
static inline int range_get_symbol(APEContext * ctx,
                                   const uint16_t counts[],
                                   const uint16_t counts_diff[])
{
    int symbol, cf;

    cf = range_decode_culshift(ctx, 16);

    if(cf > 65492){
        symbol= cf - 65535 + 63;
        range_decode_update(ctx, 1, cf);
        if(cf > 65535)
            ctx->error=1;
        return symbol;
    }
    /* figure out the symbol inefficiently; a binary search would be much better */
    for (symbol = 0; counts[symbol + 1] <= cf; symbol++);

    range_decode_update(ctx, counts_diff[symbol], counts[symbol]);

    return symbol;
}
/** @} */ // group rangecoder

static inline void update_rice(APERice *rice, int x)
{
    int lim = rice->k ? (1 << (rice->k + 4)) : 0;
    rice->ksum += ((x + 1) / 2) - ((rice->ksum + 16) >> 5);

    if (rice->ksum < lim)
        rice->k--;
    else if (rice->ksum >= (1 << (rice->k + 5)))
        rice->k++;
}

static inline int ape_decode_value(APEContext * ctx, APERice *rice)
{
    int x, overflow;

    if (ctx->fileversion < 3990) {
        int tmpk;

        overflow = range_get_symbol(ctx, counts_3970, counts_diff_3970);

        if (overflow == (MODEL_ELEMENTS - 1)) {
            tmpk = range_decode_bits(ctx, 5);
            overflow = 0;
        } else
            tmpk = (rice->k < 1) ? 0 : rice->k - 1;

        if (tmpk <= 16)
            x = range_decode_bits(ctx, tmpk);
        else {
            x = range_decode_bits(ctx, 16);
            x |= (range_decode_bits(ctx, tmpk - 16) << 16);
        }
        x += overflow << tmpk;
    } else {
        int base, pivot;

        pivot = rice->ksum >> 5;
        if (pivot == 0)
            pivot = 1;

        overflow = range_get_symbol(ctx, counts_3980, counts_diff_3980);

        if (overflow == (MODEL_ELEMENTS - 1)) {
            overflow  = range_decode_bits(ctx, 16) << 16;
            overflow |= range_decode_bits(ctx, 16);
        }

        base = range_decode_culfreq(ctx, pivot);
        range_decode_update(ctx, 1, base);

        x = base + overflow * pivot;
    }

    update_rice(rice, x);

    /* Convert to signed */
    if (x & 1)
        return (x >> 1) + 1;
    else
        return -(x >> 1);
}

static void entropy_decode(APEContext * ctx, int blockstodecode, int stereo)
{
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    ctx->blocksdecoded = blockstodecode;

    if (ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        /* We are pure silence, just memset the output buffer. */
        memset(decoded0, 0, blockstodecode * sizeof(int32_t));
        memset(decoded1, 0, blockstodecode * sizeof(int32_t));
    } else {
        while (blockstodecode--) {
            *decoded0++ = ape_decode_value(ctx, &ctx->riceY);
            if (stereo)
                *decoded1++ = ape_decode_value(ctx, &ctx->riceX);
        }
    }

    if (ctx->blocksdecoded == ctx->currentframeblocks)
        range_dec_normalize(ctx);   /* normalize to use up all bytes */
}

static void init_entropy_decoder(APEContext * ctx)
{
    /* Read the CRC */
    ctx->CRC = bytestream_get_be32(&ctx->ptr);

    /* Read the frame flags if they exist */
    ctx->frameflags = 0;
    if ((ctx->fileversion > 3820) && (ctx->CRC & 0x80000000)) {
        ctx->CRC &= ~0x80000000;

        ctx->frameflags = bytestream_get_be32(&ctx->ptr);
    }

    /* Keep a count of the blocks decoded in this frame */
    ctx->blocksdecoded = 0;

    /* Initialize the rice structs */
    ctx->riceX.k = 10;
    ctx->riceX.ksum = (1 << ctx->riceX.k) * 16;
    ctx->riceY.k = 10;
    ctx->riceY.ksum = (1 << ctx->riceY.k) * 16;

    /* The first 8 bits of input are ignored. */
    ctx->ptr++;

    range_start_decoding(ctx);
}

static const int32_t initial_coeffs[4] = {
    360, 317, -109, 98
};

static void init_predictor_decoder(APEContext * ctx)
{
    APEPredictor *p = &ctx->predictor;

    /* Zero the history buffers */
    memset(p->historybuffer, 0, PREDICTOR_SIZE * sizeof(int32_t));
    p->buf = p->historybuffer;

    /* Initialize and zero the coefficients */
    memcpy(p->coeffsA[0], initial_coeffs, sizeof(initial_coeffs));
    memcpy(p->coeffsA[1], initial_coeffs, sizeof(initial_coeffs));
    memset(p->coeffsB, 0, sizeof(p->coeffsB));

    p->filterA[0] = p->filterA[1] = 0;
    p->filterB[0] = p->filterB[1] = 0;
    p->lastA[0]   = p->lastA[1]   = 0;
}

/** Get inverse sign of integer (-1 for positive, 1 for negative and 0 for zero) */
static inline int APESIGN(int32_t x) {
    return (x < 0) - (x > 0);
}

static int predictor_update_filter(APEPredictor *p, const int decoded, const int filter, const int delayA, const int delayB, const int adaptA, const int adaptB)
{
    int32_t predictionA, predictionB;

    p->buf[delayA]     = p->lastA[filter];
    p->buf[adaptA]     = APESIGN(p->buf[delayA]);
    p->buf[delayA - 1] = p->buf[delayA] - p->buf[delayA - 1];
    p->buf[adaptA - 1] = APESIGN(p->buf[delayA - 1]);

    predictionA = p->buf[delayA    ] * p->coeffsA[filter][0] +
                  p->buf[delayA - 1] * p->coeffsA[filter][1] +
                  p->buf[delayA - 2] * p->coeffsA[filter][2] +
                  p->buf[delayA - 3] * p->coeffsA[filter][3];

    /*  Apply a scaled first-order filter compression */
    p->buf[delayB]     = p->filterA[filter ^ 1] - ((p->filterB[filter] * 31) >> 5);
    p->buf[adaptB]     = APESIGN(p->buf[delayB]);
    p->buf[delayB - 1] = p->buf[delayB] - p->buf[delayB - 1];
    p->buf[adaptB - 1] = APESIGN(p->buf[delayB - 1]);
    p->filterB[filter] = p->filterA[filter ^ 1];

    predictionB = p->buf[delayB    ] * p->coeffsB[filter][0] +
                  p->buf[delayB - 1] * p->coeffsB[filter][1] +
                  p->buf[delayB - 2] * p->coeffsB[filter][2] +
                  p->buf[delayB - 3] * p->coeffsB[filter][3] +
                  p->buf[delayB - 4] * p->coeffsB[filter][4];

    p->lastA[filter] = decoded + ((predictionA + (predictionB >> 1)) >> 10);
    p->filterA[filter] = p->lastA[filter] + ((p->filterA[filter] * 31) >> 5);

    if (!decoded) // no need updating filter coefficients
        return p->filterA[filter];

    if (decoded > 0) {
        p->coeffsA[filter][0] -= p->buf[adaptA    ];
        p->coeffsA[filter][1] -= p->buf[adaptA - 1];
        p->coeffsA[filter][2] -= p->buf[adaptA - 2];
        p->coeffsA[filter][3] -= p->buf[adaptA - 3];

        p->coeffsB[filter][0] -= p->buf[adaptB    ];
        p->coeffsB[filter][1] -= p->buf[adaptB - 1];
        p->coeffsB[filter][2] -= p->buf[adaptB - 2];
        p->coeffsB[filter][3] -= p->buf[adaptB - 3];
        p->coeffsB[filter][4] -= p->buf[adaptB - 4];
    } else {
        p->coeffsA[filter][0] += p->buf[adaptA    ];
        p->coeffsA[filter][1] += p->buf[adaptA - 1];
        p->coeffsA[filter][2] += p->buf[adaptA - 2];
        p->coeffsA[filter][3] += p->buf[adaptA - 3];

        p->coeffsB[filter][0] += p->buf[adaptB    ];
        p->coeffsB[filter][1] += p->buf[adaptB - 1];
        p->coeffsB[filter][2] += p->buf[adaptB - 2];
        p->coeffsB[filter][3] += p->buf[adaptB - 3];
        p->coeffsB[filter][4] += p->buf[adaptB - 4];
    }
    return p->filterA[filter];
}

static void predictor_decode_stereo(APEContext * ctx, int count)
{
    int32_t predictionA, predictionB;
    APEPredictor *p = &ctx->predictor;
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    while (count--) {
        /* Predictor Y */
        predictionA = predictor_update_filter(p, *decoded0, 0, YDELAYA, YDELAYB, YADAPTCOEFFSA, YADAPTCOEFFSB);
        predictionB = predictor_update_filter(p, *decoded1, 1, XDELAYA, XDELAYB, XADAPTCOEFFSA, XADAPTCOEFFSB);
        *(decoded0++) = predictionA;
        *(decoded1++) = predictionB;

        /* Combined */
        p->buf++;

        /* Have we filled the history buffer? */
        if (p->buf == p->historybuffer + HISTORY_SIZE) {
            memmove(p->historybuffer, p->buf, PREDICTOR_SIZE * sizeof(int32_t));
            p->buf = p->historybuffer;
        }
    }
}

static void predictor_decode_mono(APEContext * ctx, int count)
{
    APEPredictor *p = &ctx->predictor;
    int32_t *decoded0 = ctx->decoded0;
    int32_t predictionA, currentA, A;

    currentA = p->lastA[0];

    while (count--) {
        A = *decoded0;

        p->buf[YDELAYA] = currentA;
        p->buf[YDELAYA - 1] = p->buf[YDELAYA] - p->buf[YDELAYA - 1];

        predictionA = p->buf[YDELAYA    ] * p->coeffsA[0][0] +
                      p->buf[YDELAYA - 1] * p->coeffsA[0][1] +
                      p->buf[YDELAYA - 2] * p->coeffsA[0][2] +
                      p->buf[YDELAYA - 3] * p->coeffsA[0][3];

        currentA = A + (predictionA >> 10);

        p->buf[YADAPTCOEFFSA]     = APESIGN(p->buf[YDELAYA    ]);
        p->buf[YADAPTCOEFFSA - 1] = APESIGN(p->buf[YDELAYA - 1]);

        if (A > 0) {
            p->coeffsA[0][0] -= p->buf[YADAPTCOEFFSA    ];
            p->coeffsA[0][1] -= p->buf[YADAPTCOEFFSA - 1];
            p->coeffsA[0][2] -= p->buf[YADAPTCOEFFSA - 2];
            p->coeffsA[0][3] -= p->buf[YADAPTCOEFFSA - 3];
        } else if (A < 0) {
            p->coeffsA[0][0] += p->buf[YADAPTCOEFFSA    ];
            p->coeffsA[0][1] += p->buf[YADAPTCOEFFSA - 1];
            p->coeffsA[0][2] += p->buf[YADAPTCOEFFSA - 2];
            p->coeffsA[0][3] += p->buf[YADAPTCOEFFSA - 3];
        }

        p->buf++;

        /* Have we filled the history buffer? */
        if (p->buf == p->historybuffer + HISTORY_SIZE) {
            memmove(p->historybuffer, p->buf, PREDICTOR_SIZE * sizeof(int32_t));
            p->buf = p->historybuffer;
        }

        p->filterA[0] = currentA + ((p->filterA[0] * 31) >> 5);
        *(decoded0++) = p->filterA[0];
    }

    p->lastA[0] = currentA;
}

static void do_init_filter(APEFilter *f, int16_t * buf, int order)
{
    f->coeffs = buf;
    f->historybuffer = buf + order;
    f->delay       = f->historybuffer + order * 2;
    f->adaptcoeffs = f->historybuffer + order;

    memset(f->historybuffer, 0, (order * 2) * sizeof(int16_t));
    memset(f->coeffs, 0, order * sizeof(int16_t));
    f->avg = 0;
}

static void init_filter(APEContext * ctx, APEFilter *f, int16_t * buf, int order)
{
    do_init_filter(&f[0], buf, order);
    do_init_filter(&f[1], buf + order * 3 + HISTORY_SIZE, order);
}

#ifdef HAVE_SSE2

#if ARCH_X86_64
#    define REG_a "rax"
#    define REG_b "rbx"
#    define REG_c "rcx"
#    define REG_d "rdx"
#    define REG_D "rdi"
#    define REG_S "rsi"
#    define PTR_SIZE "8"
#    define REG_SP "rsp"
#    define REG_BP "rbp"
#    define REGBP   rbp
#    define REGa    rax
#    define REGb    rbx
#    define REGc    rcx
#    define REGd    rdx
#    define REGSP   rsp
typedef int64_t x86_reg;
#elif ARCH_X86_32
#    define REG_a "eax"
#    define REG_b "ebx"
#    define REG_c "ecx"
#    define REG_d "edx"
#    define REG_D "edi"
#    define REG_S "esi"
#    define PTR_SIZE "4"
#    define REG_SP "esp"
#    define REG_BP "ebp"
#    define REGBP   ebp
#    define REGa    eax
#    define REGb    ebx
#    define REGc    ecx
#    define REGd    edx
#    define REGSP   esp
typedef int32_t x86_reg;
#else
#warning unknown arch
typedef int x86_reg;
#endif

typedef struct { uint64_t a, b; } xmm_reg;
#define DECLARE_ALIGNED(n,t,v)      t v __attribute__ ((aligned (n)))
#define DECLARE_ALIGNED_16(t, v) DECLARE_ALIGNED(16, t, v)
static int32_t scalarproduct_int16_sse2 (int16_t * v1, int16_t * v2, int order, int shift)
{
    int res = 0;
    DECLARE_ALIGNED_16(xmm_reg, sh);
    x86_reg o = -(order << 1);

    v1 += order;
    v2 += order;
    sh.a = shift;
    __asm__ volatile(
        "pxor      %%xmm7,  %%xmm7        \n\t"
        "1:                               \n\t"
        "movdqu    (%0,%3), %%xmm0        \n\t"
        "movdqu  16(%0,%3), %%xmm1        \n\t"
        "pmaddwd   (%1,%3), %%xmm0        \n\t"
        "pmaddwd 16(%1,%3), %%xmm1        \n\t"
        "paddd     %%xmm0,  %%xmm7        \n\t"
        "paddd     %%xmm1,  %%xmm7        \n\t"
        "add       $32,     %3            \n\t"
        "js        1b                     \n\t"
        "movhlps   %%xmm7,  %%xmm2        \n\t"
        "paddd     %%xmm2,  %%xmm7        \n\t"
        "psrad     %4,      %%xmm7        \n\t"
        "pshuflw   $0x4E,   %%xmm7,%%xmm2 \n\t"
        "paddd     %%xmm2,  %%xmm7        \n\t"
        "movd      %%xmm7,  %2            \n\t"
        : "+r"(v1), "+r"(v2), "=r"(res), "+r"(o)
        : "m"(sh)
    );
    return res;
}
static void add_int16_sse2(int16_t * v1, int16_t * v2, int order)
{
    x86_reg o = -(order << 1);
    v1 += order;
    v2 += order;
    __asm__ volatile(
        "1:                          \n\t"
        "movdqu   (%1,%2),   %%xmm0  \n\t"
        "movdqu 16(%1,%2),   %%xmm1  \n\t"
        "paddw    (%0,%2),   %%xmm0  \n\t"
        "paddw  16(%0,%2),   %%xmm1  \n\t"
        "movdqa   %%xmm0,    (%0,%2) \n\t"
        "movdqa   %%xmm1,  16(%0,%2) \n\t"
        "add      $32,       %2      \n\t"
        "js       1b                 \n\t"
        : "+r"(v1), "+r"(v2), "+r"(o)
    );
}

static void sub_int16_sse2(int16_t * v1, int16_t * v2, int order)
{
    x86_reg o = -(order << 1);
    v1 += order;
    v2 += order;
    __asm__ volatile(
        "1:                           \n\t"
        "movdqa    (%0,%2),   %%xmm0  \n\t"
        "movdqa  16(%0,%2),   %%xmm2  \n\t"
        "movdqu    (%1,%2),   %%xmm1  \n\t"
        "movdqu  16(%1,%2),   %%xmm3  \n\t"
        "psubw     %%xmm1,    %%xmm0  \n\t"
        "psubw     %%xmm3,    %%xmm2  \n\t"
        "movdqa    %%xmm0,    (%0,%2) \n\t"
        "movdqa    %%xmm2,  16(%0,%2) \n\t"
        "add       $32,       %2      \n\t"
        "js        1b                 \n\t"
        : "+r"(v1), "+r"(v2), "+r"(o)
    );
}
#endif

static int32_t
scalarproduct_int16_c(int16_t * v1, int16_t * v2, int order, int shift)
{
    int res = 0;

    while (order--)
        res += (*v1++ * *v2++) >> shift;

    return res;
}

static void
add_int16_c (int16_t *v1/*align 16*/, int16_t *v2, int len) {
    while (len--) {
        *v1++ += *v2++;
    }
}

static void
sub_int16_c (int16_t *v1/*align 16*/, int16_t *v2, int len) {
    while (len--) {
        *v1++ -= *v2++;
    }
}

static int32_t
(*scalarproduct_int16)(int16_t * v1, int16_t * v2, int order, int shift);

static void
(*add_int16) (int16_t *v1/*align 16*/, int16_t *v2, int len);

static void
(*sub_int16) (int16_t *v1/*align 16*/, int16_t *v2, int len);

static inline int16_t clip_int16(int a)
{
    if ((a+32768) & ~65535) return (a>>31) ^ 32767;
        else                    return a;
}

static void bswap_buf(uint32_t *dst, const uint32_t *src, int w){
    int i;

    for(i=0; i+8<=w; i+=8){
        dst[i+0]= bswap_32(src[i+0]);
        dst[i+1]= bswap_32(src[i+1]);
        dst[i+2]= bswap_32(src[i+2]);
        dst[i+3]= bswap_32(src[i+3]);
        dst[i+4]= bswap_32(src[i+4]);
        dst[i+5]= bswap_32(src[i+5]);
        dst[i+6]= bswap_32(src[i+6]);
        dst[i+7]= bswap_32(src[i+7]);
    }
    for(;i<w; i++){
        dst[i+0]= bswap_32(src[i+0]);
    }
}


static inline void do_apply_filter(APEContext * ctx, int version, APEFilter *f, int32_t *data, int count, int order, int fracbits)
{
    int res;
    int absres;

    while (count--) {
        /* round fixedpoint scalar product */
        res = (scalarproduct_int16(f->delay - order, f->coeffs, order, 0) + (1 << (fracbits - 1))) >> fracbits;


        if (*data < 0)
            add_int16(f->coeffs, f->adaptcoeffs - order, order);
        else if (*data > 0)
            sub_int16(f->coeffs, f->adaptcoeffs - order, order);

        res += *data;

        *data++ = res;

        /* Update the output history */
        *f->delay++ = clip_int16(res);

        if (version < 3980) {
            /* Version ??? to < 3.98 files (untested) */
            f->adaptcoeffs[0]  = (res == 0) ? 0 : ((res >> 28) & 8) - 4;
            f->adaptcoeffs[-4] >>= 1;
            f->adaptcoeffs[-8] >>= 1;
        } else {
            /* Version 3.98 and later files */

            /* Update the adaption coefficients */
            absres = (res < 0 ? -res : res);

            if (absres > (f->avg * 3))
                *f->adaptcoeffs = ((res >> 25) & 64) - 32;
            else if (absres > (f->avg * 4) / 3)
                *f->adaptcoeffs = ((res >> 26) & 32) - 16;
            else if (absres > 0)
                *f->adaptcoeffs = ((res >> 27) & 16) - 8;
            else
                *f->adaptcoeffs = 0;

            f->avg += (absres - f->avg) / 16;

            f->adaptcoeffs[-1] >>= 1;
            f->adaptcoeffs[-2] >>= 1;
            f->adaptcoeffs[-8] >>= 1;
        }

        f->adaptcoeffs++;

        /* Have we filled the history buffer? */
        if (f->delay == f->historybuffer + HISTORY_SIZE + (order * 2)) {
            memmove(f->historybuffer, f->delay - (order * 2),
                    (order * 2) * sizeof(int16_t));
            f->delay = f->historybuffer + order * 2;
            f->adaptcoeffs = f->historybuffer + order;
        }
    }
}

static void apply_filter(APEContext * ctx, APEFilter *f,
                         int32_t * data0, int32_t * data1,
                         int count, int order, int fracbits)
{
    do_apply_filter(ctx, ctx->fileversion, &f[0], data0, count, order, fracbits);
    if (data1)
        do_apply_filter(ctx, ctx->fileversion, &f[1], data1, count, order, fracbits);
}

static void ape_apply_filters(APEContext * ctx, int32_t * decoded0,
                              int32_t * decoded1, int count)
{
    int i;

    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (!ape_filter_orders[ctx->fset][i])
            break;
        apply_filter(ctx, ctx->filters[i], decoded0, decoded1, count, ape_filter_orders[ctx->fset][i], ape_filter_fracbits[ctx->fset][i]);
    }
}

static void init_frame_decoder(APEContext * ctx)
{
    int i;
    init_entropy_decoder(ctx);
    init_predictor_decoder(ctx);

    for (i = 0; i < APE_FILTER_LEVELS; i++) {
        if (!ape_filter_orders[ctx->fset][i])
            break;
        init_filter(ctx, ctx->filters[i], ctx->filterbuf[i], ape_filter_orders[ctx->fset][i]);
    }
}

static void ape_unpack_mono(APEContext * ctx, int count)
{
    int32_t left;
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    if (ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        entropy_decode(ctx, count, 0);
        /* We are pure silence, so we're done. */
        fprintf (stderr, "pure silence mono\n");
        return;
    }

    entropy_decode(ctx, count, 0);
    ape_apply_filters(ctx, decoded0, NULL, count);

    /* Now apply the predictor decoding */
    predictor_decode_mono(ctx, count);

    /* Pseudo-stereo - just copy left channel to right channel */
    if (ctx->channels == 2) {
        while (count--) {
            left = *decoded0;
            *(decoded1++) = *(decoded0++) = left;
        }
    }
}

static void ape_unpack_stereo(APEContext * ctx, int count)
{
    int32_t left, right;
    int32_t *decoded0 = ctx->decoded0;
    int32_t *decoded1 = ctx->decoded1;

    if (ctx->frameflags & APE_FRAMECODE_STEREO_SILENCE) {
        /* We are pure silence, so we're done. */
        fprintf (stderr, "pure silence stereo\n");
        return;
    }

    entropy_decode(ctx, count, 1);
    ape_apply_filters(ctx, decoded0, decoded1, count);

    /* Now apply the predictor decoding */
    predictor_decode_stereo(ctx, count);

    /* Decorrelate and scale to output depth */
    while (count--) {
        left = *decoded1 - (*decoded0 / 2);
        right = left + *decoded0;

        *(decoded0++) = left;
        *(decoded1++) = right;
    }
}

static int
ape_decode_frame(APEContext *s, void *data, int *data_size)
{
    int16_t *samples = data;
    int nblocks;
    int i, n;
    int blockstodecode;
    int bytes_used;

    /* should not happen but who knows */
    if (BLOCKS_PER_LOOP * 2 * s->channels > *data_size) {
        fprintf (stderr, "Packet size is too big! (max is %d where you have %d)\n", *data_size, BLOCKS_PER_LOOP * 2 * s->channels);
        return -1;
    }

    if (s->packet_remaining < PACKET_BUFFER_SIZE) {
//        assert (packet_sizeleft >= 0 && packet_remaining >= 0);
//        if (packet_sizeleft == 0 && packet_remaining == 0) {
        if (s->samples == 0) {
            if (s->currentframe == s->totalframes) {
                return -1;
            }
            assert (!s->samples);
//            fprintf (stderr, "start reading packet %d\n", ape_ctx.currentframe);
            assert (s->samples == 0); // all samples from prev packet must have been read
            // start new packet
            if (ape_read_packet (fp, &ape_ctx) < 0) {
                fprintf (stderr, "error reading packet\n");
                return -1;
            }
            bswap_buf((uint32_t*)(s->packet_data), (const uint32_t*)(s->packet_data), s->packet_remaining >> 2);

//            fprintf (stderr, "packet_sizeleft=%d packet_remaining=%d\n", packet_sizeleft, packet_remaining);
            s->ptr = s->last_ptr = s->packet_data;

            nblocks = s->samples = bytestream_get_be32(&s->ptr);
            //fprintf (stderr, "s->samples=%d (1)\n", s->samples);
            n = bytestream_get_be32(&s->ptr);
            if(n < 0 || n > 3){
                fprintf (stderr, "Incorrect offset passed\n");
                return -1;
            }
            s->ptr += n;

            s->currentframeblocks = nblocks;
            //buf += 4;
            if (s->samples <= 0) {
                *data_size = 0;
                bytes_used = s->packet_remaining;
                goto error;
            }

            memset(s->decoded0,  0, sizeof(s->decoded0));
            memset(s->decoded1,  0, sizeof(s->decoded1));

            /* Initialize the frame decoder */
            init_frame_decoder(s);
        }
        else {
            int sz = PACKET_BUFFER_SIZE - s->packet_remaining;
            sz = min (sz, s->packet_sizeleft);
            sz = sz&~3;
            uint8_t *p = s->packet_data + s->packet_remaining;
            int r = deadbeef->fread (p, 1, sz, fp);
            //if (r != s) {
            //    fprintf (stderr, "unexpected eof while reading ape frame\n");
            //    return -1;
            //}
            bswap_buf((uint32_t*)p, (const uint32_t*)p, r >> 2);
            s->packet_sizeleft -= r;
            s->packet_remaining += r;
            //fprintf (stderr, "read more %d bytes for current packet, sizeleft=%d, packet_remaining=%d\n", r, packet_sizeleft, packet_remaining);
        }
    }
    s->data_end = s->packet_data + s->packet_remaining;

    if (s->packet_remaining == 0 && !s->samples) {
        *data_size = 0;
        return 0;
    }
    if (!s->packet_remaining) {
        fprintf (stderr, "packetbuf is empty!!\n");
        *data_size = 0;
        bytes_used = s->packet_remaining;
        goto error;
    }

    nblocks = s->samples;
    blockstodecode = min(BLOCKS_PER_LOOP, nblocks);

    s->error=0;

    if ((s->channels == 1) || (s->frameflags & APE_FRAMECODE_PSEUDO_STEREO))
        ape_unpack_mono(s, blockstodecode);
    else
        ape_unpack_stereo(s, blockstodecode);

    if(s->error || s->ptr >= s->data_end){
        s->samples=0;
        if (s->error) {
            fprintf (stderr, "Error decoding frame, error=%d\n", s->error);
        }
        else {
            fprintf (stderr, "Error decoding frame, ptr > data_end\n");
        }
        return -1;
    }

    int skip = min (s->samplestoskip, blockstodecode);
    i = skip;

    for (; i < blockstodecode; i++) {
        *samples++ = s->decoded0[i];
        if(s->channels == 2) {
            *samples++ = s->decoded1[i];
        }
    }
    
    s->samplestoskip -= skip;
    s->samples -= blockstodecode;

    *data_size = (blockstodecode - skip) * 2 * s->channels;
//    ape_ctx.currentsample += blockstodecode - skip;
    bytes_used = s->samples ? s->ptr - s->last_ptr : s->packet_remaining;

    // shift everything
error:
    if (bytes_used < s->packet_remaining) {
        memmove (s->packet_data, s->packet_data+bytes_used, s->packet_remaining-bytes_used);
    }
    s->packet_remaining -= bytes_used;
    s->ptr -= bytes_used;
    s->last_ptr = s->ptr;

    return bytes_used;
}

static DB_playItem_t *
ffap_insert (DB_playItem_t *after, const char *fname) {
    APEContext ape_ctx;
    memset (&ape_ctx, 0, sizeof (ape_ctx));
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    if (ape_read_header (fp, &ape_ctx) < 0) {
        fprintf (stderr, "failed to read ape header\n");
        deadbeef->fclose (fp);
        ape_free_ctx (&ape_ctx);
        return NULL;
    }
    if ((ape_ctx.fileversion < APE_MIN_VERSION) || (ape_ctx.fileversion > APE_MAX_VERSION)) {
        fprintf(stderr, "unsupported file version - %.2f\n", ape_ctx.fileversion/1000.0);
        deadbeef->fclose (fp);
        ape_free_ctx (&ape_ctx);
        return NULL;
    }

    float duration = ape_ctx.totalsamples / (float)ape_ctx.samplerate;
    DB_playItem_t *it;
    it  = deadbeef->pl_insert_cue (after, fname, &plugin, "APE", ape_ctx.totalsamples, ape_ctx.samplerate);
    if (it) {
        deadbeef->fclose (fp);
        ape_free_ctx (&ape_ctx);
        return it;
    }

    it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "APE";
    it->duration = duration;
 
    int v2err = deadbeef->junk_read_id3v2 (it, fp);
    int v1err = deadbeef->junk_read_id3v1 (it, fp);
    if (v1err >= 0) {
        deadbeef->fseek (fp, -128, SEEK_END);
    }
    else {
        deadbeef->fseek (fp, 0, SEEK_END);
    }
    int apeerr = deadbeef->junk_read_ape (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);

    deadbeef->fclose (fp);
    ape_free_ctx (&ape_ctx);
    return after;
}

static int
ffap_read_int16 (char *buffer, int size) {
    if (ape_ctx.currentsample + size / (2 * ape_ctx.channels) > endsample) {
        size = (endsample - ape_ctx.currentsample + 1) * 2 * ape_ctx.channels;
        trace ("size truncated to %d bytes, cursample=%d, endsample=%d, totalsamples=%d\n", size, ape_ctx.currentsample, endsample, ape_ctx.totalsamples);
        if (size <= 0) {
            return 0;
        }
    }
    int inits = size;
    while (size > 0) {
        if (ape_ctx.remaining > 0) {
            int sz = min (size, ape_ctx.remaining);
            memcpy (buffer, ape_ctx.buffer, sz);
            buffer += sz;
            size -= sz;
            if (ape_ctx.remaining > sz) {
                memmove (ape_ctx.buffer, ape_ctx.buffer + sz, ape_ctx.remaining-sz);
            }
            ape_ctx.remaining -= sz;
            continue;
        }
        int s = BLOCKS_PER_LOOP * 2 * 2 * 2;
        assert (ape_ctx.remaining <= s/2);
        s -= ape_ctx.remaining;
        uint8_t *buf = ape_ctx.buffer + ape_ctx.remaining;
        int n = ape_decode_frame (&ape_ctx, buf, &s);
        if (n == -1) {
            break;
        }
        ape_ctx.remaining += s;

        int sz = min (size, ape_ctx.remaining);
        memcpy (buffer, ape_ctx.buffer, sz);
        buffer += sz;
        size -= sz;
        if (ape_ctx.remaining > sz) {
            memmove (ape_ctx.buffer, ape_ctx.buffer + sz, ape_ctx.remaining-sz);
        }
        ape_ctx.remaining -= sz;
    }
    ape_ctx.currentsample += (inits - size) / (2 * ape_ctx.channels);
    plugin.info.readpos = (ape_ctx.currentsample-startsample) / (float)plugin.info.samplerate;
    return inits - size;
}

static int
ffap_seek_sample (int sample) {
    sample += startsample;
    trace ("seeking to %d/%d\n", sample, ape_ctx.totalsamples);
    uint32_t newsample = sample;
    if (newsample > ape_ctx.totalsamples) {
        trace ("eof\n");
        return -1;
    }
    int nframe = newsample / ape_ctx.blocksperframe;
    if (nframe >= ape_ctx.totalframes) {
        trace ("eof2\n");
        return -1;
    }
    ape_ctx.currentframe = nframe;
    ape_ctx.samplestoskip = newsample - nframe * ape_ctx.blocksperframe;

    // reset decoder
    ape_ctx.packet_remaining = 0;
    ape_ctx.samples = 0;
    ape_ctx.currentsample = newsample;
    plugin.info.readpos = (float)(newsample-startsample)/ape_ctx.samplerate;
    return 0;
}

static int
ffap_seek (float seconds) {
    return ffap_seek_sample (seconds * plugin.info.samplerate);
}

static const char *exts[] = { "ape", NULL };
static const char *filetypes[] = { "APE", NULL };
// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "FFAP Monkey's Audio decoder",
    .plugin.descr = "Based on ffmpeg apedec by Benjamin Zores and rockbox libdemac by Dave Chapman",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = ffap_init,
    .free = ffap_free,
    .read_int16 = ffap_read_int16,
    .seek = ffap_seek,
    .seek_sample = ffap_seek_sample,
    .insert = ffap_insert,
    .exts = exts,
    .id = "ffap",
    .filetypes = filetypes
};

#ifdef HAVE_SSE2

#define FF_MM_MMX      0x0001 ///< standard MMX
#define FF_MM_3DNOW    0x0004 ///< AMD 3DNOW
#define FF_MM_MMX2     0x0002 ///< SSE integer functions or AMD MMX ext
#define FF_MM_SSE      0x0008 ///< SSE functions
#define FF_MM_SSE2     0x0010 ///< PIV SSE2 functions
#define FF_MM_3DNOWEXT 0x0020 ///< AMD 3DNowExt
#define FF_MM_SSE3     0x0040 ///< Prescott SSE3 functions
#define FF_MM_SSSE3    0x0080 ///< Conroe SSSE3 functions
#define FF_MM_SSE4     0x0100 ///< Penryn SSE4.1 functions
#define FF_MM_SSE42    0x0200 ///< Nehalem SSE4.2 functions
#define FF_MM_IWMMXT   0x0100 ///< XScale IWMMXT
#define FF_MM_ALTIVEC  0x0001 ///< standard AltiVec

/* ebx saving is necessary for PIC. gcc seems unable to see it alone */
#define cpuid(index,eax,ebx,ecx,edx)\
    __asm__ volatile\
        ("mov %%"REG_b", %%"REG_S"\n\t"\
         "cpuid\n\t"\
         "xchg %%"REG_b", %%"REG_S\
         : "=a" (eax), "=S" (ebx),\
           "=c" (ecx), "=d" (edx)\
         : "0" (index));

/* Function to test if multimedia instructions are supported...  */
int mm_support(void)
{
    int rval = 0;
    int eax, ebx, ecx, edx;
    int max_std_level, max_ext_level, std_caps=0, ext_caps=0;

#if ARCH_X86_32
    x86_reg a, c;
    __asm__ volatile (
        /* See if CPUID instruction is supported ... */
        /* ... Get copies of EFLAGS into eax and ecx */
        "pushfl\n\t"
        "pop %0\n\t"
        "mov %0, %1\n\t"

        /* ... Toggle the ID bit in one copy and store */
        /*     to the EFLAGS reg */
        "xor $0x200000, %0\n\t"
        "push %0\n\t"
        "popfl\n\t"

        /* ... Get the (hopefully modified) EFLAGS */
        "pushfl\n\t"
        "pop %0\n\t"
        : "=a" (a), "=c" (c)
        :
        : "cc"
        );

    if (a == c)
        return 0; /* CPUID not supported */
#endif

    cpuid(0, max_std_level, ebx, ecx, edx);

    if(max_std_level >= 1){
        cpuid(1, eax, ebx, ecx, std_caps);
        if (std_caps & (1<<23))
            rval |= FF_MM_MMX;
        if (std_caps & (1<<25))
            rval |= FF_MM_MMX2
#ifdef HAVE_SSE
                  | FF_MM_SSE;
        if (std_caps & (1<<26))
            rval |= FF_MM_SSE2;
        if (ecx & 1)
            rval |= FF_MM_SSE3;
        if (ecx & 0x00000200 )
            rval |= FF_MM_SSSE3;
        if (ecx & 0x00080000 )
            rval |= FF_MM_SSE4;
        if (ecx & 0x00100000 )
            rval |= FF_MM_SSE42;
#endif
                  ;
    }

    cpuid(0x80000000, max_ext_level, ebx, ecx, edx);

    if(max_ext_level >= 0x80000001){
        cpuid(0x80000001, eax, ebx, ecx, ext_caps);
        if (ext_caps & (1<<31))
            rval |= FF_MM_3DNOW;
        if (ext_caps & (1<<30))
            rval |= FF_MM_3DNOWEXT;
        if (ext_caps & (1<<23))
            rval |= FF_MM_MMX;
        if (ext_caps & (1<<22))
            rval |= FF_MM_MMX2;
    }

    return rval;
}
#endif

DB_plugin_t *
ffap_load (DB_functions_t *api) {
    // detect sse2
#ifdef HAVE_SSE2
    int mm_flags = mm_support ();
    if (mm_flags & FF_MM_SSE2) {
        scalarproduct_int16 = scalarproduct_int16_sse2;
        add_int16 = add_int16_sse2;
        sub_int16 = sub_int16_sse2;
    }
    else {
        scalarproduct_int16 = scalarproduct_int16_c;
        add_int16 = add_int16_c;
        sub_int16 = sub_int16_c;
    }
#else
    scalarproduct_int16 = scalarproduct_int16_c;
    add_int16 = add_int16_c;
    sub_int16 = sub_int16_c;
#endif
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
