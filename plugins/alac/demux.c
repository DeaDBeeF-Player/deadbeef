/*
 * ALAC (Apple Lossless Audio Codec) decoder
 * Copyright (c) 2005 David Hammerton
 * All rights reserved.
 *
 * This is the quicktime container demuxer.
 *
 * http://crazney.net/programs/itunes/alac.html
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
	#include "stdint_win.h"
#else
	#include <stdint.h>
#endif

#include "stream.h"
#include "demux.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

typedef struct
{
    stream_t *stream;
    demux_res_t *res;
    long saved_mdat_pos;
} qtmovie_t;


/* chunk handlers */
static void read_chunk_ftyp(qtmovie_t *qtmovie, size_t chunk_len)
{
    fourcc_t type;
    uint32_t minor_ver;
    size_t size_remaining = chunk_len - 8; /* FIXME: can't hardcode 8, size may be 64bit */

    type = stream_read_uint32(qtmovie->stream);
    size_remaining-=4;
    if ((type != MAKEFOURCC('M','4','A',' ')) &&
        (type != MAKEFOURCC('m','4','a',' ')) &&
        (type != MAKEFOURCC('M','4','B',' ')) &&
        (type != MAKEFOURCC('m','p','4','2')) &&
        (type != MAKEFOURCC('3','g','p','6')) &&
        (type != MAKEFOURCC('q','t',' ',' ')) &&
        (type != MAKEFOURCC('i','s','o','m')))
    {
        trace ("not M4A file\n");
        return;
    }
    minor_ver = stream_read_uint32(qtmovie->stream);
    size_remaining-=4;

    /* compatible brands */
    while (size_remaining)
    {
        /* unused */
        /*fourcc_t cbrand =*/ stream_read_uint32(qtmovie->stream);
        size_remaining-=4;
    }
}

static void read_chunk_tkhd(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_edts(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

static void read_chunk_elst(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

static int read_chunk_stsd(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8;

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

//    if (numentries != 1)
//    {
//        trace ("only expecting one entry in sample description atom!\n");
//        return 0;
//    }

    for (i = 0; i < numentries; i++)
    {
        uint32_t entry_size;

        uint32_t entry_remaining;

        entry_size = stream_read_uint32(qtmovie->stream);
        qtmovie->res->format = stream_read_uint32(qtmovie->stream);
        trace ("format: %c%c%c%c\n",SPLITFOURCC(qtmovie->res->format));
        entry_remaining = entry_size;
        entry_remaining -= 8;


        /* sound info: */

        /* reserved + data reference index + sound version + reserved */
        stream_skip(qtmovie->stream, 6 + 2 + 2 + 6);
        entry_remaining -= 6 + 2 + 2 + 6;

        qtmovie->res->num_channels = stream_read_uint16(qtmovie->stream);
        qtmovie->res->sample_size = stream_read_uint16(qtmovie->stream);
        entry_remaining -= 4;

        /* packet size */
        stream_skip(qtmovie->stream, 2);
        qtmovie->res->sample_rate = stream_read_uint32(qtmovie->stream);
        /* reserved size */
        stream_skip(qtmovie->stream, 2);
        entry_remaining -= 8;

        /* remaining is codec data */
        /* 12 = audio format atom, 8 = padding */
        qtmovie->res->codecdata_len = entry_remaining + 12 + 8;
        if (qtmovie->res->codecdata_len > 64)
        {
            trace ("codecdata too large (%d) in stsd\n", 
                    (int)qtmovie->res->codecdata_len);
            return 0;
        }

        memset(qtmovie->res->codecdata, 0, qtmovie->res->codecdata_len);
        /* audio format atom */
#if 0
        /* The ALAC decoder skips these bytes, so there is no need to store them,
           and this code isn't endian/alignment safe */
        ((unsigned int*)qtmovie->res->codecdata)[0] = 0x0c000000;
        ((unsigned int*)qtmovie->res->codecdata)[1] = MAKEFOURCC('a','m','r','f');
        ((unsigned int*)qtmovie->res->codecdata)[2] = MAKEFOURCC('c','a','l','a');
#endif

        stream_read(qtmovie->stream,
                entry_remaining,
                ((char*)qtmovie->res->codecdata) + 12);
        entry_remaining -= entry_remaining;

        if (entry_remaining)
            stream_skip(qtmovie->stream, entry_remaining);


        qtmovie->res->format_read = 1;
        if (qtmovie->res->format != MAKEFOURCC('a','l','a','c'))
        {
            /*trace ("expecting 'alac' data format, got %c%c%c%c\n",
                    SPLITFOURCC(qtmovie->res->format));*/
            return 0;
        }
    }

    return 1;
}

static void read_chunk_stts(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_time_to_samples = numentries;
    qtmovie->res->time_to_sample = malloc(numentries * sizeof(*qtmovie->res->time_to_sample));

    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->time_to_sample[i].sample_count = stream_read_uint32(qtmovie->stream);
        qtmovie->res->time_to_sample[i].sample_duration = stream_read_uint32(qtmovie->stream);
        size_remaining -= 8;
    }

    if (size_remaining)
    {
        trace ("ehm, size remianing?\n");
        stream_skip(qtmovie->stream, size_remaining);
    }
}

static void read_chunk_stsz(qtmovie_t *qtmovie, size_t chunk_len)
{
    unsigned int i;
    uint32_t numentries;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    /* version */
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 1;
    /* flags */
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    stream_read_uint8(qtmovie->stream);
    size_remaining -= 3;

    /* default sample size */
    if (stream_read_uint32(qtmovie->stream) != 0)
    {
        trace ("i was expecting variable samples sizes\n");
        stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
        return;
    }
    size_remaining -= 4;

    numentries = stream_read_uint32(qtmovie->stream);
    size_remaining -= 4;

    qtmovie->res->num_sample_byte_sizes = numentries;
    qtmovie->res->sample_byte_size = malloc(numentries * sizeof(*qtmovie->res->sample_byte_size));

    for (i = 0; i < numentries; i++)
    {
        qtmovie->res->sample_byte_size[i] = stream_read_uint32(qtmovie->stream);
        size_remaining -= 4;
    }

    if (size_remaining)
    {
        trace ("ehm, size remianing?\n");
        stream_skip(qtmovie->stream, size_remaining);
    }
}

static int read_chunk_stbl(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            trace ("strange size for chunk inside stbl (%lu) (remaining: %lu)\n",
                    sub_chunk_len, size_remaining);
            return 0;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('s','t','s','d'):
            if (read_chunk_stsd(qtmovie, sub_chunk_len) == 0)
                return 0;
            break;
        case MAKEFOURCC('s','t','t','s'):
            read_chunk_stts(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('s','t','s','z'):
            read_chunk_stsz(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('s','t','s','c'):
        case MAKEFOURCC('s','t','c','o'):
            /* skip these, no indexing for us! */
            stream_skip(qtmovie->stream, sub_chunk_len - 8);
            break;
        default:
            trace ("(stbl) unknown chunk id: %c%c%c%c\n",
                    SPLITFOURCC(sub_chunk_id));
            return 0;
        }

        size_remaining -= sub_chunk_len;
    }

    return 1;
}

static int read_chunk_minf(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t dinf_size, stbl_size;
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

  /**** SOUND HEADER CHUNK ****/
    if (stream_read_uint32(qtmovie->stream) != 16)
    {
        trace ("unexpected size in media info\n");
        return 0;
    }
    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('s','m','h','d'))
    {
        trace ("not a sound header! can't handle this.\n");
        return 0;
    }
    /* now skip the rest */
    stream_skip(qtmovie->stream, 16 - 8);
    size_remaining -= 16;
  /****/

  /**** DINF CHUNK ****/
    dinf_size = stream_read_uint32(qtmovie->stream);
    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('d','i','n','f'))
    {
        trace ("expected dinf, didn't get it.\n");
        return 0;
    }
    /* skip it */
    stream_skip(qtmovie->stream, dinf_size - 8);
    size_remaining -= dinf_size;
  /****/


  /**** SAMPLE TABLE ****/
    stbl_size = stream_read_uint32(qtmovie->stream);
    if (stream_read_uint32(qtmovie->stream) != MAKEFOURCC('s','t','b','l'))
    {
        trace ("expected stbl, didn't get it.\n");
        return 0;
    }
    if (read_chunk_stbl(qtmovie, stbl_size) == 0)
        return 0;
    size_remaining -= stbl_size;

    if (size_remaining)
    {
        trace ("oops\n");
        stream_skip(qtmovie->stream, size_remaining);
    }

    return 1;
}

static int read_chunk_mdia(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            trace ("strange size for chunk inside mdia\n");
            return 0;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

    switch (sub_chunk_id)
    {
    case MAKEFOURCC('m','i','n','f'):
        if (read_chunk_minf(qtmovie, sub_chunk_len) == 0)
            return 0;
        break;
    default:
        trace ("(mdia) unknown chunk id: %c%c%c%c\n",
                SPLITFOURCC(sub_chunk_id));
        stream_skip(qtmovie->stream, sub_chunk_len - 8);
        break;;
    }

        size_remaining -= sub_chunk_len;
    }

    return 1;
}

/* 'trak' - a movie track - contains other atoms */
static int read_chunk_trak(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            trace ("strange size for chunk inside trak\n");
            return 0;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('t','k','h','d'):
            read_chunk_tkhd(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('m','d','i','a'):
            if (read_chunk_mdia(qtmovie, sub_chunk_len) == 0)
                return 0;
            break;
        case MAKEFOURCC('e','d','t','s'):
            read_chunk_edts(qtmovie, sub_chunk_len);
            break;
        default:
            trace ("(trak) unknown chunk id: %c%c%c%c\n",
                    SPLITFOURCC(sub_chunk_id));
            return 0;
        }

        size_remaining -= sub_chunk_len;
    }

    return 1;
}

/* 'mvhd' movie header atom */
static void read_chunk_mvhd(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

/* 'udta' user data.. contains tag info */
static void read_chunk_udta(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

/* 'iods' */
static void read_chunk_iods(qtmovie_t *qtmovie, size_t chunk_len)
{
    /* don't need anything from here atm, skip */
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    stream_skip(qtmovie->stream, size_remaining);
}

/* 'moov' movie atom - contains other atoms */
static int read_chunk_moov(qtmovie_t *qtmovie, size_t chunk_len)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    while (size_remaining)
    {
        size_t sub_chunk_len;
        fourcc_t sub_chunk_id;

        sub_chunk_len = stream_read_uint32(qtmovie->stream);
        if (sub_chunk_len <= 1 || sub_chunk_len > size_remaining)
        {
            trace ("strange size for chunk inside moov\n");
            return 0;
        }

        sub_chunk_id = stream_read_uint32(qtmovie->stream);

        switch (sub_chunk_id)
        {
        case MAKEFOURCC('m','v','h','d'):
            read_chunk_mvhd(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('t','r','a','k'):
            if (read_chunk_trak(qtmovie, sub_chunk_len) == 0)
                return 0;
            break;
        case MAKEFOURCC('u','d','t','a'):
            read_chunk_udta(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('e','l','s','t'):
            read_chunk_elst(qtmovie, sub_chunk_len);
            break;
        case MAKEFOURCC('i','o','d','s'):
            read_chunk_iods(qtmovie, sub_chunk_len);
            break;
        default:
            stream_skip(qtmovie->stream, sub_chunk_len-8);
            break;
        }

        size_remaining -= sub_chunk_len;
    }

    return 1;
}

static void read_chunk_mdat(qtmovie_t *qtmovie, size_t chunk_len, int skip_mdat)
{
    size_t size_remaining = chunk_len - 8; /* FIXME WRONG */

    if (size_remaining == 0) return;

    qtmovie->res->mdat_len = (uint32_t)size_remaining;
    if (skip_mdat)
    {
        qtmovie->saved_mdat_pos = stream_tell(qtmovie->stream);
        stream_skip(qtmovie->stream, size_remaining);
    }
#if 0
    qtmovie->res->mdat = malloc(size_remaining);

    stream_read(qtmovie->stream, size_remaining, qtmovie->res->mdat);
#endif
}

static int set_saved_mdat(qtmovie_t *qtmovie)
{
    if (qtmovie->saved_mdat_pos == -1)
    {
        trace ("stream contains mdat before moov but is not seekable\n");
        return 0;
    }

    if (stream_setpos(qtmovie->stream, qtmovie->saved_mdat_pos))
    {
        trace ("error while seeking stream to mdat pos\n");
        return 0;
    }

    return 1;
}

int qtmovie_read(stream_t *file, demux_res_t *demux_res)
{
    int found_moov = 0;
    int found_mdat = 0;
    qtmovie_t *qtmovie;

    qtmovie = (qtmovie_t*)malloc(sizeof(qtmovie_t));
    memset (qtmovie, 0, sizeof (qtmovie_t));

    /* construct the stream */
    qtmovie->stream = file;

    qtmovie->res = demux_res;

    memset(demux_res, 0, sizeof(demux_res_t));

    /* read the chunks */
    while (1)
    {
        size_t chunk_len;
        fourcc_t chunk_id;

        chunk_len = stream_read_uint32(qtmovie->stream);
        if (stream_eof(qtmovie->stream))
        {
            free (qtmovie);
            return 0;
        }

        if (chunk_len == 1)
        {
            trace ("need 64bit support\n");
            free (qtmovie);
            return 0;
        }
        chunk_id = stream_read_uint32(qtmovie->stream);

        switch (chunk_id)
        {
        case MAKEFOURCC('f','t','y','p'):
            read_chunk_ftyp(qtmovie, chunk_len);
            break;
        case MAKEFOURCC('m','o','o','v'):
            if (read_chunk_moov(qtmovie, chunk_len) == 0) {
                free (qtmovie);
                return 0; /* failed to read moov, can't do anything */
            }
            if (found_mdat)
            {
                int res = set_saved_mdat(qtmovie);
                free (qtmovie);
                return res;
            }
            found_moov = 1;
            break;
            /* if we hit mdat before we've found moov, record the position
             * and move on. We can then come back to mdat later.
             * This presumes the stream supports seeking backwards.
             */
        case MAKEFOURCC('m','d','a','t'):
            read_chunk_mdat(qtmovie, chunk_len, !found_moov);
            if (found_moov) {
                free (qtmovie);
                return 1;
            }
            found_mdat = 1;
            break;

            /*  these following atoms can be skipped !!!! */
        case MAKEFOURCC('f','r','e','e'):
            stream_skip(qtmovie->stream, chunk_len - 8); /* FIXME not 8 */
            break;
        default:
            trace ("(top) unknown chunk id: %c%c%c%c\n",
                    SPLITFOURCC(chunk_id));
            free (qtmovie);
            return 0;
        }

    }
    free (qtmovie);
    return 0;
}


void qtmovie_free_demux (demux_res_t *demux_res) {
    if (demux_res->time_to_sample) {
        free (demux_res->time_to_sample);
    }
    if (demux_res->sample_byte_size) {
        free (demux_res->sample_byte_size);
    }
    memset (demux_res, 0, sizeof (demux_res_t));
}
