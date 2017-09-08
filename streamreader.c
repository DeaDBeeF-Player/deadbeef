/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Alexey Yakovenko and other contributors

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
*/

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "streamreader.h"
#include "replaygain.h"

// read ahead about 5 sec at 44100/16/2
#define BLOCK_SIZE 16384
#define BLOCK_COUNT 48

static streamblock_t *blocks; // list of all blocks

static streamblock_t *block_data; // first available block with data (can be NULL)

static streamblock_t *block_next; // next block available to be read into / queued

static int numblocks_ready;

static int curr_block_bitrate;

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
void
streamreader_init (void) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        streamblock_t *b = calloc (1, sizeof (streamblock_t));
        b->pos = -1;
        b->buf = malloc (BLOCK_SIZE);
        b->next = blocks;
        blocks = b;
    }
    block_next = blocks;
    numblocks_ready = 0;
}

void
streamreader_free (void) {
    streamreader_reset ();
    while (blocks) {
        streamblock_t *next = blocks->next;
        free (blocks->buf);
        free (blocks);
        blocks = next;
    }
    block_next = block_data = NULL;
    numblocks_ready = 0;
}

streamblock_t *
streamreader_get_next_block (void) {
    if (block_next->pos >= 0) {
        return NULL; // all buffers full
    }
    return block_next;
}

int
streamreader_read_block (streamblock_t *block, playItem_t *track, DB_fileinfo_t *fileinfo) {
    // clip size to max possible, with current sample format
    int size = BLOCK_SIZE;
    int samplesize = fileinfo->fmt.channels * (fileinfo->fmt.bps>>3);
    int mod = size % samplesize;
    if (mod) {
        size -= mod;
    }

    // replaygain settings

    ddb_replaygain_settings_t rg_settings;
    rg_settings._size = sizeof (rg_settings);
    replaygain_init_settings (&rg_settings, track);
    replaygain_set_current (&rg_settings);

    // NOTE: streamer_set_bitrate may be called during decoder->read, and set immediated bitrate of the block
    curr_block_bitrate = -1;
    int rb = fileinfo->plugin->read (fileinfo, block->buf, size);
    trace("decoder sent %d bytes\n",rb);
    if (rb < 0) {
        return -1;
    }

    block->bitrate = curr_block_bitrate;

    block->pos = 0;
    block->size = rb;
    memcpy (&block->fmt, &fileinfo->fmt, sizeof (ddb_waveformat_t));
    pl_item_ref (track);
    block->track = track;

    int input_does_rg = fileinfo->plugin->plugin.flags & DDB_PLUGIN_FLAG_REPLAYGAIN;
    if (!input_does_rg) {
        replaygain_apply (&fileinfo->fmt, block->buf, block->size);
    }

    if (rb != size) {
        block->last = 1;
    }
    else {
        block->last = 0;
    }

    return 0;
}

void
streamreader_enqueue_block (streamblock_t *block) {
    // block is passed just for sanity checking
    assert (block->track);
    if (!block_data) {
        block_data = block_next;
    }

    block_next = block_next->next;
    if (!block_next) {
        block_next = blocks;
    }

    block->queued = 1;
    numblocks_ready++;
}

void
streamer_set_bitrate (int bitrate) {
    curr_block_bitrate = bitrate;
}

streamblock_t *
streamreader_get_curr_block (void) {
    return block_data;
}

void
streamreader_next_block (void) {
    if (block_data) {
        block_data->pos = -1;
        pl_item_unref (block_data->track);
        block_data->track = NULL;
        block_data->queued = 0;

        block_data = block_data->next;
        if (!block_data) {
            block_data = blocks;
        }

        numblocks_ready--;
        if (numblocks_ready < 0) {
            numblocks_ready = 0;
        }
    }

    if (block_data && !block_data->queued) {
        block_data = NULL; // no available blocks with data
    }
}

void
streamreader_reset (void) {
    streamblock_t *b = blocks;
    while (b) {
        b->pos = -1;
        if (b->track) {
            pl_item_unref (b->track);
            b->track = NULL;
        }
        b->queued = 0;
        b = b->next;
    }
    block_next = blocks;
    block_data = NULL;
    numblocks_ready = 0;
}

int
streamreader_num_blocks_ready (void) {
    return numblocks_ready;
}
