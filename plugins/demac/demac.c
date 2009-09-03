/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko
    This plugin is based on code from demac and libdemac (C) Dave Chapman 2007

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "../../deadbeef.h"
#include "libdemac/demac.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

#define CALC_CRC 0

#define BLOCKS_PER_LOOP     4608
#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3

#define INPUT_CHUNKSIZE (32*1024)

/* 4608*4 = 18432 bytes per channel */
static int32_t decoded0[BLOCKS_PER_LOOP];
static int32_t decoded1[BLOCKS_PER_LOOP];

/* We assume that 32KB of compressed data is enough to extract up to
   27648 bytes of decompressed data. */

static unsigned char inbuffer[INPUT_CHUNKSIZE];

static int currentframe;
static uint32_t frame_crc;
static int crc_errors;
static FILE *fp;
static int firstbyte;
static int bytesinbuffer;
static struct ape_ctx_t ape_ctx;
static int bytesconsumed;
static int timestart;
static int timeend;
static int nblocks;
static int samplestoskip;
static int samplesdecoded;

static float ape_duration;

/* 4608*2*3 = 27648 bytes */
// 5x big to fit extra data while streaming
#define WAVBUFFER_SIZE (BLOCKS_PER_LOOP*MAX_CHANNELS*MAX_BYTESPERSAMPLE * 5)
static uint8_t wavbuffer[WAVBUFFER_SIZE];
static int bufferfill;

inline static int
read_uint16(FILE *fp, uint16_t* x)
{
    unsigned char tmp[2];
    int n;

    n = fread(tmp, 1, 2, fp);

    if (n != 2)
        return -1;

    *x = tmp[0] | (tmp[1] << 8);

    return 0;
}


inline static int
read_int16(FILE *fp, int16_t* x)
{
    return read_uint16(fp, (uint16_t*)x);
}

inline static int
read_uint32(FILE *fp, uint32_t* x)
{
    unsigned char tmp[4];
    int n;

    n = fread(tmp, 1, 4, fp);

    if (n != 4)
        return -1;

    *x = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);

    return 0;
}

int
demac_ape_parseheader(FILE *fp, struct ape_ctx_t* ape_ctx)
{
    int i,n;
    memset (ape_ctx, 0, sizeof (struct ape_ctx_t));

    /* TODO: Skip any leading junk such as id3v2 tags */
    ape_ctx->junklength = 0;

    fseek(fp,ape_ctx->junklength,SEEK_SET);

    n = fread (&ape_ctx->magic, 1, 4, fp);
    if (n != 4) {
        return -1;
    }

    if (memcmp(ape_ctx->magic,"MAC ",4)!=0)
    {
        return -1;
    }

    if (read_int16(fp,&ape_ctx->fileversion) < 0) {
        return -1;
    }

    if (ape_ctx->fileversion >= 3980)
    {
        if (read_int16(fp,&ape_ctx->padding1) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->descriptorlength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->headerlength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->seektablelength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->wavheaderlength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->audiodatalength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->audiodatalength_high) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->wavtaillength) < 0)
            return -1;
        if (fread(&ape_ctx->md5, 1, 16, fp) != 16)
            return -1;

        /* Skip any unknown bytes at the end of the descriptor.  This is for future
           compatibility */
        if (ape_ctx->descriptorlength > 52)
            fseek(fp,ape_ctx->descriptorlength - 52, SEEK_CUR);

        /* Read header data */
        if (read_uint16(fp,&ape_ctx->compressiontype) < 0)
            return -1;
        if (read_uint16(fp,&ape_ctx->formatflags) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->blocksperframe) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->finalframeblocks) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->totalframes) < 0)
            return -1;
        if (read_uint16(fp,&ape_ctx->bps) < 0)
            return -1;
        if (read_uint16(fp,&ape_ctx->channels) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->samplerate) < 0)
            return -1;
    } else {
        ape_ctx->descriptorlength = 0;
        ape_ctx->headerlength = 32;

        if (read_uint16(fp,&ape_ctx->compressiontype) < 0)
            return -1;
        if (read_uint16(fp,&ape_ctx->formatflags) < 0)
            return -1;
        if (read_uint16(fp,&ape_ctx->channels) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->samplerate) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->wavheaderlength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->wavtaillength) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->totalframes) < 0)
            return -1;
        if (read_uint32(fp,&ape_ctx->finalframeblocks) < 0)
            return -1;

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
        {
            fseek(fp, 4, SEEK_CUR);   /* Skip the peak level */
            ape_ctx->headerlength += 4;
        }

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
        {
            if (read_uint32(fp,&ape_ctx->seektablelength) < 0)
                return -1;
            ape_ctx->headerlength += 4;
            ape_ctx->seektablelength *= sizeof(int32_t);
        } else {
            ape_ctx->seektablelength = ape_ctx->totalframes * sizeof(int32_t);
        }

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_8_BIT)
            ape_ctx->bps = 8;
        else if (ape_ctx->formatflags & MAC_FORMAT_FLAG_24_BIT)
            ape_ctx->bps = 24;
        else
            ape_ctx->bps = 16;

        if (ape_ctx->fileversion >= 3950)
            ape_ctx->blocksperframe = 73728 * 4;
        else if ((ape_ctx->fileversion >= 3900) || (ape_ctx->fileversion >= 3800 && ape_ctx->compressiontype >= 4000))
            ape_ctx->blocksperframe = 73728;
        else
            ape_ctx->blocksperframe = 9216;

        /* Skip any stored wav header */
        if (!(ape_ctx->formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
        {
            fseek(fp, ape_ctx->wavheaderlength, SEEK_CUR);
        }
    }

    ape_ctx->totalsamples = ape_ctx->finalframeblocks;
    if (ape_ctx->totalframes > 1)
        ape_ctx->totalsamples += ape_ctx->blocksperframe * (ape_ctx->totalframes-1);

    if (ape_ctx->seektablelength > 0)
    {
        ape_ctx->seektable = malloc(ape_ctx->seektablelength);
        if (ape_ctx->seektable == NULL)
            return -1;
        for (i=0; i < ape_ctx->seektablelength / sizeof(uint32_t); i++)
        {
            if (read_uint32(fp,&ape_ctx->seektable[i]) < 0)
            {
                 free(ape_ctx->seektable);
                 ape_ctx->seektable = NULL;
                 return -1;
            }
        }
        ape_ctx->numseekpoints = ape_ctx->seektablelength/sizeof (int32_t);
    }

    ape_ctx->firstframe = ape_ctx->junklength + ape_ctx->descriptorlength +
                           ape_ctx->headerlength + ape_ctx->seektablelength +
                           ape_ctx->wavheaderlength;

    return 0;
}

static int
demac_seek (float seconds);

static int
demac_init (DB_playItem_t *it) {
    crc_errors = 0;
    fp = fopen (it->fname, "rb");
    if (!fp) {
        return -1;
    }
    if (demac_ape_parseheader (fp, &ape_ctx) < 0) {
        fprintf (stderr, "demac: failed to read ape header\n");
        fclose (fp);
        fp = NULL;
        if (ape_ctx.seektable) {
            free (ape_ctx.seektable);
            ape_ctx.seektable = NULL;
        }
        return -1;
    }
    if ((ape_ctx.fileversion < APE_MIN_VERSION) || (ape_ctx.fileversion > APE_MAX_VERSION)) {
        fprintf(stderr, "demac: unsupported file version - %.2f\n", ape_ctx.fileversion/1000.0);
        fclose (fp);
        fp = NULL;
        if (ape_ctx.seektable) {
            free (ape_ctx.seektable);
            ape_ctx.seektable = NULL;
        }
        return -1;
    }
    fprintf(stderr, "demac: decoding file - v%.2f, compression level %d\n", ape_ctx.fileversion/1000.0, ape_ctx.compressiontype);

    currentframe = 0;
    fprintf (stderr, "seek first frame: %d\n", ape_ctx.firstframe);
    fseek (fp, ape_ctx.firstframe, SEEK_SET);
    bytesinbuffer = fread (inbuffer, 1, INPUT_CHUNKSIZE, fp);
    firstbyte = 3;
    bufferfill = 0;
    bytesconsumed = 0;
    nblocks = 0;
    samplestoskip = 0;
    samplesdecoded = 0;

    plugin.info.bps = ape_ctx.bps;
    plugin.info.samplerate = ape_ctx.samplerate;
    plugin.info.channels = ape_ctx.channels;
    plugin.info.readpos = 0;
    ape_duration = it->duration;
    if (it->timeend > 0) {
        timestart = it->timestart;
        timeend = it->timeend;
        demac_seek (0);
    }
    else {
        timestart = 0;
        timeend = it->duration;
    }

    return 0;
}

static void
demac_free (void) {
    if (fp) {
        fclose (fp);
        fp = NULL;
        if (ape_ctx.seektable) {
            free (ape_ctx.seektable);
            ape_ctx.seektable = NULL;
        }
    }
}

static int
demac_read (char *buffer, int size) {
    /* The main decoding loop - we decode the frames a small chunk at a time */
//    fprintf (stderr, "starting read size=%d bufferfill=%d!\n", size, bufferfill);
    while (currentframe < ape_ctx.totalframes && bufferfill < size)
    {
        int n, i;
        assert (bufferfill < WAVBUFFER_SIZE-(size-bufferfill));
        if (nblocks <= 0) {
            /* Calculate how many blocks there are in this frame */
            if (currentframe == (ape_ctx.totalframes - 1))
                nblocks = ape_ctx.finalframeblocks;
            else
                nblocks = ape_ctx.blocksperframe;

            ape_ctx.currentframeblocks = nblocks;

            /* Initialise the frame decoder */
            init_frame_decoder(&ape_ctx, inbuffer, &firstbyte, &bytesconsumed);

            /* Update buffer */
            memmove(inbuffer,inbuffer + bytesconsumed, bytesinbuffer - bytesconsumed);
            bytesinbuffer -= bytesconsumed;

            n = fread(inbuffer + bytesinbuffer, 1, INPUT_CHUNKSIZE - bytesinbuffer, fp);
            bytesinbuffer += n;

#if CALC_CRC
            frame_crc = ape_initcrc();
#endif
        }

        /* Decode the frame a chunk at a time */
        while (nblocks > 0 && bufferfill < size)
        {
            int blockstodecode = min (BLOCKS_PER_LOOP, nblocks);
            int res;
            int16_t sample16;
            int32_t sample32;

            if ((res = decode_chunk(&ape_ctx, inbuffer, &firstbyte,
                                    &bytesconsumed,
                                    decoded0, decoded1,
                                    blockstodecode)) < 0)
            {
                /* Frame decoding error, abort */
                return 0;
            }

            /* Convert the output samples to WAV format and write to output file */
#if CALC_CRC
            uint8_t *pp = wavbuffer + bufferfill;
#endif
            uint8_t *p = wavbuffer + bufferfill;
            int i = 0;
            while (samplestoskip > 0 && i < blockstodecode) {
                i++;
                samplestoskip--;
                p += ape_ctx.channels * 2;
                samplesdecoded++;
            }

            for (; i < blockstodecode ; i++)
            {
                sample16 = decoded0[i];
                *(p++) = sample16 & 0xff;
                *(p++) = (sample16 >> 8) & 0xff;
                bufferfill += 2;

                if (ape_ctx.channels == 2) {
                    sample16 = decoded1[i];
                    *(p++) = sample16 & 0xff;
                    *(p++) = (sample16 >> 8) & 0xff;
                    bufferfill += 2;
                }
                samplesdecoded++;
            }

            assert (bufferfill <= WAVBUFFER_SIZE);

#if CALC_CRC
            frame_crc = ape_updatecrc(pp, p - pp, frame_crc);
#endif
            memmove(inbuffer,inbuffer + bytesconsumed, bytesinbuffer - bytesconsumed);
            bytesinbuffer -= bytesconsumed;

            n = fread(inbuffer + bytesinbuffer, 1, INPUT_CHUNKSIZE - bytesinbuffer, fp);
            bytesinbuffer += n;

            /* Decrement the block count */
            nblocks -= blockstodecode;
        }

        if (nblocks <= 0) {
#if CALC_CRC
            frame_crc = ape_finishcrc(frame_crc);

            if (ape_ctx.CRC != frame_crc)
            {
                fprintf(stderr,"CRC error in frame %d\n",currentframe);
                crc_errors++;
            }
#endif
            currentframe++;
        }
    }

    // copy from wavbuffer to buffer
    int sz = min (bufferfill, size);
    if (sz) {
        memcpy (buffer, wavbuffer, sz);
        if (sz == bufferfill) {
            bufferfill = 0;
        }
        else if (bufferfill > sz) {
            memmove (wavbuffer, wavbuffer+sz, bufferfill-sz);
            bufferfill -= sz;
        }
    }
    plugin.info.readpos = samplesdecoded / (float)plugin.info.samplerate - timestart;
    if (plugin.info.readpos >= (timeend - timestart)) {
        return 0;
    }
    return sz;
}

/* Given an ape_ctx and a sample to seek to, return the file position
   to the frame containing that sample, and the number of samples to
   skip in that frame.
   */

static int ape_calc_seekpos(struct ape_ctx_t* ape_ctx,
        uint32_t new_sample,
        uint32_t* newframe,
        uint32_t* filepos,
        uint32_t* samplestoskip)
{
    uint32_t n;

    n = new_sample / ape_ctx->blocksperframe;
    if (n >= ape_ctx->numseekpoints)
    {
        /* We don't have a seekpoint for that frame */
        return 0;
    }

    *newframe = n;
    *filepos = ape_ctx->seektable[n];
    *samplestoskip = new_sample - (n * ape_ctx->blocksperframe);

    return 1;
}


static int
demac_seek (float seconds) {
    seconds += timestart;
    uint32_t newsample = seconds * plugin.info.samplerate;
    if (newsample > ape_ctx.totalsamples) {
        return -1;
    }
    uint32_t nf, fpos;
    if (ape_calc_seekpos (&ape_ctx, newsample, &nf, &fpos, &samplestoskip)) {
        firstbyte = 3 - (fpos & 3);
        fpos &= ~3;
        fseek (fp, fpos, SEEK_SET);
        bytesinbuffer = fread (inbuffer, 1, INPUT_CHUNKSIZE, fp);
        bufferfill = 0;
        bytesconsumed = 0;
        nblocks = 0;
        samplesdecoded = newsample-samplestoskip;
        plugin.info.readpos = seconds;
    }
    else {
        // no seektable
        if (newsample > samplesdecoded) {
            samplestoskip = newsample - samplesdecoded;
        }
        else {
            fpos = ape_ctx.firstframe;
            samplestoskip = newsample;
            firstbyte = 3 - (fpos & 3);
            fpos &= ~3;
            fseek (fp, fpos, SEEK_SET);
            bytesinbuffer = fread (inbuffer, 1, INPUT_CHUNKSIZE, fp);
            bufferfill = 0;
            bytesconsumed = 0;
            nblocks = 0;
            samplesdecoded = 0;
        }
    }
    plugin.info.readpos = seconds - timestart;
    return 0;
}

static DB_playItem_t *
demac_insert (DB_playItem_t *after, const char *fname) {
    struct ape_ctx_t ape_ctx;
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return NULL;
    }
    if (demac_ape_parseheader (fp, &ape_ctx) < 0) {
        fprintf (stderr, "demac: failed to read ape header\n");
        fclose (fp);
        return NULL;
    }
//    ape_dumpinfo (&ape_ctx);
    if ((ape_ctx.fileversion < APE_MIN_VERSION) || (ape_ctx.fileversion > APE_MAX_VERSION)) {
        fprintf(stderr, "demac: unsupported file version - %.2f\n", ape_ctx.fileversion/1000.0);
        fclose (fp);
        return NULL;
    }

    float duration = ape_ctx.totalsamples / (float)ape_ctx.samplerate;
    DB_playItem_t *it;
    it  = deadbeef->pl_insert_cue (after, fname, &plugin, "APE", duration);
    if (it) {
        fclose (fp);
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
        fseek (fp, -128, SEEK_END);
    }
    else {
        fseek (fp, 0, SEEK_END);
    }
    int apeerr = deadbeef->junk_read_ape (it, fp);
    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);

    fclose (fp);
    return after;
}

static const char *exts[] = { "ape", NULL };
static const char *filetypes[] = { "APE", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "Monkey's Audio decoder",
    .plugin.descr = "Based on libdemac",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = demac_init,
    .free = demac_free,
    .read_int16 = demac_read,
    .seek = demac_seek,
    .insert = demac_insert,
    .exts = exts,
    .id = "stddemac",
    .filetypes = filetypes
};

DB_plugin_t *
demac_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
