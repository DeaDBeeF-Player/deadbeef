/*

libdemac - A Monkey's Audio decoder

$Id: parser.c 13601 2007-06-09 00:58:15Z dave $

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

#include <inttypes.h>
#include <string.h>
#ifndef ROCKBOX
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "parser.h"

#ifdef APE_MAX
#undef APE_MAX
#endif
#define APE_MAX(a,b) ((a)>(b)?(a):(b))


static inline int16_t get_int16(unsigned char* buf)
{
    return(buf[0] | (buf[1] << 8));
}

static inline uint16_t get_uint16(unsigned char* buf)
{
    return(buf[0] | (buf[1] << 8));
}

static inline uint32_t get_uint32(unsigned char* buf)
{
    return(buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24));
}


int ape_parseheaderbuf(unsigned char* buf, struct ape_ctx_t* ape_ctx)
{
    unsigned char* header;

    memset(ape_ctx,0,sizeof(struct ape_ctx_t));
    /* TODO: Skip any leading junk such as id3v2 tags */
    ape_ctx->junklength = 0;

    memcpy(ape_ctx->magic, buf, 4);
    if (memcmp(ape_ctx->magic,"MAC ",4)!=0)
    {
        return -1;
    }

    ape_ctx->fileversion = get_int16(buf + 4);

    if (ape_ctx->fileversion >= 3980)
    {
        ape_ctx->padding1 = get_int16(buf + 6);
        ape_ctx->descriptorlength = get_uint32(buf + 8);
        ape_ctx->headerlength = get_uint32(buf + 12);
        ape_ctx->seektablelength = get_uint32(buf + 16);
        ape_ctx->wavheaderlength = get_uint32(buf + 20);
        ape_ctx->audiodatalength = get_uint32(buf + 24);
        ape_ctx->audiodatalength_high = get_uint32(buf + 28);
        ape_ctx->wavtaillength = get_uint32(buf + 32);
        memcpy(ape_ctx->md5, buf + 36, 16);

        header = buf + ape_ctx->descriptorlength;

        /* Read header data */
        ape_ctx->compressiontype = get_uint16(header + 0);
        ape_ctx->formatflags = get_uint16(header + 2);
        ape_ctx->blocksperframe = get_uint32(header + 4);
        ape_ctx->finalframeblocks = get_uint32(header + 8);
        ape_ctx->totalframes = get_uint32(header + 12);
        ape_ctx->bps = get_uint16(header + 16);
        ape_ctx->channels = get_uint16(header + 18);
        ape_ctx->samplerate = get_uint32(header + 20);

        ape_ctx->seektablefilepos = ape_ctx->junklength + 
                                    ape_ctx->descriptorlength +
                                    ape_ctx->headerlength;

        ape_ctx->firstframe = ape_ctx->junklength + ape_ctx->descriptorlength +
                              ape_ctx->headerlength + ape_ctx->seektablelength +
                              ape_ctx->wavheaderlength;
    } else {
        ape_ctx->headerlength = 32;
        ape_ctx->compressiontype = get_uint16(buf + 6);
        ape_ctx->formatflags = get_uint16(buf + 8);
        ape_ctx->channels = get_uint16(buf + 10);
        ape_ctx->samplerate = get_uint32(buf + 12);
        ape_ctx->wavheaderlength = get_uint32(buf + 16);
        ape_ctx->totalframes = get_uint32(buf + 24);
        ape_ctx->finalframeblocks = get_uint32(buf + 28);

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
        {
            ape_ctx->headerlength += 4;
        }

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
        {
            ape_ctx->seektablelength = get_uint32(buf + ape_ctx->headerlength);
            ape_ctx->seektablelength *= sizeof(int32_t);
            ape_ctx->headerlength += 4;
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

        ape_ctx->seektablefilepos = ape_ctx->junklength + ape_ctx->headerlength +
                                    ape_ctx->wavheaderlength;

        ape_ctx->firstframe = ape_ctx->junklength + ape_ctx->headerlength +
                              ape_ctx->wavheaderlength + ape_ctx->seektablelength;
    }

    ape_ctx->totalsamples = ape_ctx->finalframeblocks;
    if (ape_ctx->totalframes > 1)
        ape_ctx->totalsamples += ape_ctx->blocksperframe * (ape_ctx->totalframes-1);

    ape_ctx->numseekpoints = APE_MAX(ape_ctx->maxseekpoints,
                                     ape_ctx->seektablelength / sizeof(int32_t));

    return 0;
}


#ifndef ROCKBOX
/* Helper functions */

static int read_uint16(int fd, uint16_t* x)
{
    unsigned char tmp[2];
    int n;

    n = read(fd,tmp,2);

    if (n != 2)
        return -1;

    *x = tmp[0] | (tmp[1] << 8);

    return 0;
}

static int read_int16(int fd, int16_t* x)
{
    return read_uint16(fd, (uint16_t*)x);
}

static int read_uint32(int fd, uint32_t* x)
{
    unsigned char tmp[4];
    int n;

    n = read(fd,tmp,4);

    if (n != 4)
        return -1;

    *x = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24);

    return 0;
}

int ape_parseheader(int fd, struct ape_ctx_t* ape_ctx)
{
    int i,n;

    /* TODO: Skip any leading junk such as id3v2 tags */
    ape_ctx->junklength = 0;

    lseek(fd,ape_ctx->junklength,SEEK_SET);

    n = read(fd,&ape_ctx->magic,4);
    if (n != 4) return -1;

    if (memcmp(ape_ctx->magic,"MAC ",4)!=0)
    {
        return -1;
    }

    if (read_int16(fd,&ape_ctx->fileversion) < 0)
        return -1;

    if (ape_ctx->fileversion >= 3980)
    {
        if (read_int16(fd,&ape_ctx->padding1) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->descriptorlength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->headerlength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->seektablelength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->wavheaderlength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->audiodatalength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->audiodatalength_high) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->wavtaillength) < 0)
            return -1;
        if (read(fd,&ape_ctx->md5,16) != 16)
            return -1;

        /* Skip any unknown bytes at the end of the descriptor.  This is for future
           compatibility */
        if (ape_ctx->descriptorlength > 52)
            lseek(fd,ape_ctx->descriptorlength - 52, SEEK_CUR);

        /* Read header data */
        if (read_uint16(fd,&ape_ctx->compressiontype) < 0)
            return -1;
        if (read_uint16(fd,&ape_ctx->formatflags) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->blocksperframe) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->finalframeblocks) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->totalframes) < 0)
            return -1;
        if (read_uint16(fd,&ape_ctx->bps) < 0)
            return -1;
        if (read_uint16(fd,&ape_ctx->channels) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->samplerate) < 0)
            return -1;
    } else {
        ape_ctx->descriptorlength = 0;
        ape_ctx->headerlength = 32;

        if (read_uint16(fd,&ape_ctx->compressiontype) < 0)
            return -1;
        if (read_uint16(fd,&ape_ctx->formatflags) < 0)
            return -1;
        if (read_uint16(fd,&ape_ctx->channels) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->samplerate) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->wavheaderlength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->wavtaillength) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->totalframes) < 0)
            return -1;
        if (read_uint32(fd,&ape_ctx->finalframeblocks) < 0)
            return -1;

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
        {
            lseek(fd, 4, SEEK_CUR);   /* Skip the peak level */
            ape_ctx->headerlength += 4;
        }

        if (ape_ctx->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
        {
            if (read_uint32(fd,&ape_ctx->seektablelength) < 0)
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
            lseek(fd, ape_ctx->wavheaderlength, SEEK_CUR);
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
            if (read_uint32(fd,&ape_ctx->seektable[i]) < 0)
            {
                 free(ape_ctx->seektable);
                 return -1;
            }
        }
    }

    ape_ctx->firstframe = ape_ctx->junklength + ape_ctx->descriptorlength +
                           ape_ctx->headerlength + ape_ctx->seektablelength +
                           ape_ctx->wavheaderlength;

    return 0;
}

void ape_dumpinfo(struct ape_ctx_t* ape_ctx)
{
  int i;

    printf("Descriptor Block:\n\n");
    printf("magic                = \"%c%c%c%c\"\n",
            ape_ctx->magic[0],ape_ctx->magic[1],
            ape_ctx->magic[2],ape_ctx->magic[3]);
    printf("fileversion          = %d\n",ape_ctx->fileversion);
    printf("descriptorlength     = %d\n",ape_ctx->descriptorlength);
    printf("headerlength         = %d\n",ape_ctx->headerlength);
    printf("seektablelength      = %d\n",ape_ctx->seektablelength);
    printf("wavheaderlength      = %d\n",ape_ctx->wavheaderlength);
    printf("audiodatalength      = %d\n",ape_ctx->audiodatalength);
    printf("audiodatalength_high = %d\n",ape_ctx->audiodatalength_high);
    printf("wavtaillength        = %d\n",ape_ctx->wavtaillength);
    printf("md5                  = ");
    for (i = 0; i < 16; i++)
        printf("%02x",ape_ctx->md5[i]);
    printf("\n");

    printf("\nHeader Block:\n\n");

    printf("compressiontype      = %d\n",ape_ctx->compressiontype);
    printf("formatflags          = %d\n",ape_ctx->formatflags);
    printf("blocksperframe       = %d\n",ape_ctx->blocksperframe);
    printf("finalframeblocks     = %d\n",ape_ctx->finalframeblocks);
    printf("totalframes          = %d\n",ape_ctx->totalframes);
    printf("bps                  = %d\n",ape_ctx->bps);
    printf("channels             = %d\n",ape_ctx->channels);
    printf("samplerate           = %d\n",ape_ctx->samplerate);

    printf("\nSeektable\n\n");
    if ((ape_ctx->seektablelength / sizeof(uint32_t)) != ape_ctx->totalframes)
    {
        printf("No seektable\n");
    }
    else
    {
        for ( i = 0; i < ape_ctx->seektablelength / sizeof(uint32_t) ; i++)
        {
            if (i < ape_ctx->totalframes-1) {
                printf("%8d   %d (%d bytes)\n",i,ape_ctx->seektable[i],ape_ctx->seektable[i+1]-ape_ctx->seektable[i]);
            } else {
                printf("%8d   %d\n",i,ape_ctx->seektable[i]);
            }
        }
    }
    printf("\nCalculated information:\n\n");
    printf("junklength           = %d\n",ape_ctx->junklength);
    printf("firstframe           = %d\n",ape_ctx->firstframe);
    printf("totalsamples         = %d\n",ape_ctx->totalsamples);
}

#endif /* !ROCKBOX */
