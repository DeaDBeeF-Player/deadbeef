/*
  apedec - Monkey's Audio Decoder plugin for DeaDBeeF player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009 Alexey Yakovenko

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Note: DeaDBeeF player itself uses different license
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../deadbeef.h"
#include "apewrapper.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static void *ape_dec;
static int ape_blk_size;

#define READBUF_SIZE 0x20000

static char ape_readbuf[READBUF_SIZE];
static int ape_blocks_left;
static int ape_total_blocks;
static float timestart;
static float timeend;
static int samplesdecoded;

static int
ape_seek (float seconds);

static int
ape_init (DB_playItem_t *it) {
    ape_dec = ape_decompress_create (it->fname);
    if (!ape_dec) {
        printf ("ape_decompress_create failed for file %s\n", it->fname);
        return -1;
    }
    WAVEFORMATEX wfe;
    ape_decompress_get_info_data (ape_dec, APE_INFO_WAVEFORMATEX, &wfe);
    int size = ape_decompress_get_info_int (ape_dec, APE_INFO_WAV_HEADER_BYTES);
    char buf[size];
    ape_decompress_get_info_data_sized (ape_dec, APE_INFO_WAV_HEADER_DATA, buf, size);
    ape_blk_size = ape_decompress_get_info_int (ape_dec, APE_INFO_BLOCK_ALIGN);
    ape_total_blocks = ape_blocks_left = ape_decompress_get_info_int (ape_dec, APE_DECOMPRESS_TOTAL_BLOCKS);
    samplesdecoded = 0;
    plugin.info.bps = wfe.wBitsPerSample;
    plugin.info.samplerate = wfe.nSamplesPerSec;
    plugin.info.channels = wfe.nChannels;
    plugin.info.readpos = 0;
    if (it->timeend > 0) {
        timestart = it->timestart;
        timeend = it->timeend;
        ape_seek (0);
    }
    else {
        timestart = 0;
        timeend = it->duration;
    }

    return 0;
}

static void
ape_free (void) {
    if (ape_dec) {
        ape_decompress_destroy (ape_dec);
        ape_dec = NULL;
    }
}

static int
ape_read (char *buffer, int size) {
    int initsize = size;
    int nblocks = size / plugin.info.channels / 2;
    nblocks = min (nblocks, ape_blocks_left);
    nblocks = ape_decompress_getdata (ape_dec, buffer, nblocks);
    ape_blocks_left -= nblocks;
    samplesdecoded += nblocks;
    plugin.info.readpos = samplesdecoded / (float)plugin.info.samplerate - timestart;
    if (plugin.info.readpos >= timeend) {
        return 0;
    }
    return nblocks * 2 * plugin.info.channels;
}

static int
ape_seek (float seconds) {
    seconds += timestart;
    int nblock = seconds * plugin.info.samplerate;
    ape_decompress_seek (ape_dec, nblock);
    samplesdecoded = nblock;
    ape_blocks_left = ape_total_blocks - nblock;
    plugin.info.readpos = samplesdecoded / (float)plugin.info.samplerate - timestart;
}

static DB_playItem_t *
ape_insert (DB_playItem_t *after, const char *fname) {
    void *dec = ape_decompress_create (fname);
    if (!dec) {
        ape_decompress_destroy (dec);
        return NULL;
    }
    WAVEFORMATEX wfe;
    ape_decompress_get_info_data (dec, APE_INFO_WAVEFORMATEX, &wfe);

    float duration = ape_decompress_get_info_int (dec, APE_DECOMPRESS_TOTAL_BLOCKS) / (float)wfe.nSamplesPerSec;
    ape_decompress_destroy (dec);
    DB_playItem_t *it = deadbeef->pl_insert_cue (after, fname, &plugin, "APE", duration);
    if (it) {
        return it;
    }

    it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "APE";
    it->duration = duration;

    // try to read tags
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        deadbeef->pl_item_free (it);
        return NULL;
    }

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
    fclose (fp);
 
    after = deadbeef->pl_insert_item (after, it);

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

