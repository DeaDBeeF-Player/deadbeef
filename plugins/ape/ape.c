/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include "../../deadbeef.h"
#include "apewrapper.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static void *ape_dec;
static int ape_blk_size;

#define READBUF_SIZE 0x20000
#define READBUF_MASK 0x1ffff

static char ape_readbuf[READBUF_SIZE];
static int ape_blocks_left;
//static int ape_remaining;
//static int ape_position;

int
ape_init (DB_playItem_t *it) {
    ape_dec = ape_decompress_create (it->fname);
    if (!ape_dec) {
        return -1;
    }
    WAVEFORMATEX wfe;
    ape_decompress_get_info_data (ape_dec, APE_INFO_WAVEFORMATEX, &wfe);
    int size = ape_decompress_get_info_int (ape_dec, APE_INFO_WAV_HEADER_BYTES);
    char buf[size];
    ape_decompress_get_info_data_sized (ape_dec, APE_INFO_WAV_HEADER_DATA, buf, size);
    ape_blk_size = ape_decompress_get_info_int (ape_dec, APE_INFO_BLOCK_ALIGN);
    ape_blocks_left = ape_decompress_get_info_int (ape_dec, APE_DECOMPRESS_TOTAL_BLOCKS);
    plugin.info.bps = wfe.wBitsPerSample;
    plugin.info.samplerate = wfe.nSamplesPerSec;
    plugin.info.channels = wfe.nChannels;
    plugin.info.readpos = 0;

    return 0;
}

void
ape_free (void) {
    if (ape_dec) {
        ape_decompress_destroy (ape_dec);
        ape_dec = NULL;
    }
}

int
ape_read (char *buffer, int size) {
    int initsize = size;
    int nblocks = size / plugin.info.channels / 2;
    nblocks = min (nblocks, ape_blocks_left);
    nblocks = ape_decompress_getdata (ape_dec, buffer, nblocks);
    ape_blocks_left -= nblocks;
    printf ("samples left: %d       \r", ape_blocks_left);
    return nblocks * 2 * plugin.info.channels;
}

int
ape_seek (float seconds) {
    int nblock = seconds * plugin.info.samplerate;
    ape_decompress_seek (ape_dec, nblock);
    ape_blocks_left = ape_decompress_get_info_int (ape_dec, APE_DECOMPRESS_TOTAL_BLOCKS) - nblock;
    plugin.info.readpos = seconds;
}

DB_playItem_t *
ape_insert (DB_playItem_t *after, const char *fname) {
    void *dec = ape_decompress_create (fname);
    if (!dec) {
        return NULL;
    }
    WAVEFORMATEX wfe;
    ape_decompress_get_info_data (dec, APE_INFO_WAVEFORMATEX, &wfe);
    fprintf (stderr, "%s\nWAVEFORMATEX:\nwFormatTag=%04x\nnChannels=%d\nnSamplesPerSec=%d\nnAvgBytesPerSec=%d\nnBlockAlign=%d\nwBitsPerSample=%d\ncbSize=%d\n", fname, wfe.wFormatTag, wfe.nChannels, wfe.nSamplesPerSec, wfe.nAvgBytesPerSec, wfe.nBlockAlign, wfe.wBitsPerSample, wfe.cbSize);

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "APE";
    it->duration = ape_decompress_get_info_int (dec, APE_DECOMPRESS_TOTAL_BLOCKS) / (float)wfe.nSamplesPerSec;
 
    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);

    // print info about ape
    ape_decompress_destroy (dec);
    return after;
}

static const char * exts[] = { "ape", NULL };
static const char *filetypes[] = { "APE", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "Monkey's Audio decoder",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = ape_init,
    .free = ape_free,
    .read_int16 = ape_read,
    .seek = ape_seek,
    .insert = ape_insert,
    .exts = exts,
    .id = "stdape",
    .filetypes = filetypes
};

DB_plugin_t *
ape_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// {{{
#if 0
    for (;;) {
        // fill buffer
        int nblocks = (READBUF_SIZE-ape_remaining)/ape_blksize;
        nblocks = min (nblocks, ape_blocks_left);
        int n = ape_decompress_getdata (ape_dec, ape_readbuf, nblocks);
        if (ape_remaining > 0) {
            int sz = min (size, ape_remaining);
            int nsamples = sz / 4;
            for (int i = 0; i < nsamples; i++) {
                int32_t sample = *((int32_t*)&ape_readbuf[ape_position]);
                *((int16_t *)buffer) = (int16_t)sample;
                ape_position = (ape_position + sizeof (int32_t)) & ape_ring_mask;
                ape_remaining -= sizeof (int32_t);
                buffer += sizeof (int16_t);
                size -= sizeof (int16_t);
            }
        }
        assert (size >= 0);
        if (size == 0) {
            return initsize - size;
        }
    }
#endif
// }}}
