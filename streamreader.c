/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors

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
#include "threading.h"

// read ahead about 5 sec at 44100/16/2
#define BLOCK_SIZE 16384
#define BLOCK_COUNT 48

static streamblock_t *blocks; // list of all blocks

static streamblock_t *block_data; // first available block with data (can be NULL)

static streamblock_t *block_next; // next block available to be read into / queued

static int numblocks_ready;

static int curr_block_bitrate;

static playItem_t *_prev_rg_track;
static int _rg_settingschanged = 1;
static int _firstblock = 0;

void
streamreader_init (void) {
    _prev_rg_track = NULL;
    _rg_settingschanged = 1;
    for (int i = 0; i < BLOCK_COUNT; i++) {
        streamblock_t *b = calloc (1, sizeof (streamblock_t));
        b->pos = -1;
        b->buf = malloc (BLOCK_SIZE);
        b->next = blocks;
        blocks = b;
    }
    block_next = blocks;
    numblocks_ready = 0;
    _firstblock = 0;
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
    _prev_rg_track = NULL;
    _rg_settingschanged = 1;
    _firstblock = 0;
}

streamblock_t *
streamreader_get_next_block (void) {
    if (block_next->pos >= 0) {
        return NULL; // all buffers full
    }

    // FIXME: initialize
    block_next->is_silent_header = 0;
    return block_next;
}

void
streamreader_configchanged (void) {
    _rg_settingschanged = 1;
}

int
streamreader_read_block (streamblock_t *block, playItem_t *track, DB_fileinfo_t *fileinfo, uint64_t mutex) {
    int size = BLOCK_SIZE;
    int samplesize = fileinfo->fmt.channels * (fileinfo->fmt.bps>>3);

    // NOTE: samplesize has to be checked to protect against faulty input plugins

    if (!fileinfo->plugin || samplesize <= 0) {
        // return dummy block for a failed track
        _firstblock = 1;
        size = 0;
    }
    else {
        // clip size to max possible, with current sample format
        int mod = size % samplesize;
        if (mod) {
            size -= mod;
        }
    }

    // replaygain settings
    mutex_lock (mutex);
    if (_rg_settingschanged || _prev_rg_track != track) {
        _prev_rg_track = track;
        _rg_settingschanged = 0;
        ddb_replaygain_settings_t rg_settings;
        rg_settings._size = sizeof (rg_settings);
        replaygain_init_settings (&rg_settings, track);
        replaygain_set_current (&rg_settings);
    }
    mutex_unlock (mutex);

    // NOTE: streamer_set_bitrate may be called during decoder->read, and set immediated bitrate of the block
    curr_block_bitrate = -1;
    int rb;
    if (size > 0) {
        rb = fileinfo->plugin->read (fileinfo, block->buf, size);
    }
    else {
        rb = -1;
    }

    mutex_lock (mutex);

    block->bitrate = curr_block_bitrate;

    block->pos = 0;
    if (rb >= 0) {
        block->size = rb;
    }
    else {
        // this handles both a failed dummy track,
        // and an interrupted corrupt stream.
        block->size = 0;
    }
    memcpy (&block->fmt, &fileinfo->fmt, sizeof (ddb_waveformat_t));
    block->track = track;
    if (block->track != NULL) {
        pl_item_ref(block->track);
    }

    if (size > 0) {
        int input_does_rg = fileinfo->plugin->plugin.flags & DDB_PLUGIN_FLAG_REPLAYGAIN;
        if (!input_does_rg) {
            replaygain_apply (&fileinfo->fmt, block->buf, block->size);
        }
    }

    if (_firstblock) {
        block->first = 1;
        _firstblock = 0;
    }
    else {
        block->first = 0;
    }

    if (rb != size) {
        block->last = 1;
        _firstblock = 1;
    }
    else {
        block->last = 0;
    }

    return 0;
}

int
streamreader_silence_block (streamblock_t *block, playItem_t *track, DB_fileinfo_t *fileinfo, uint64_t mutex) {
    curr_block_bitrate = -1;
    mutex_lock (mutex);
    block->bitrate = -1;
    block->pos = 0;
    memset (block->buf, 0, BLOCK_SIZE);
    block->size = BLOCK_SIZE;

    memcpy (&block->fmt, &fileinfo->fmt, sizeof (ddb_waveformat_t));
    block->track = track;
    if (block->track != NULL) {
        pl_item_ref(block->track);
    }
    block->is_silent_header = 1;

    if (_firstblock) {
        block->first = 1;
        _firstblock = 0;
    }
    else {
        block->first = 0;
    }

    block->last = 0;

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

static void
_streamreader_release_block (streamblock_t *block) {
    block->pos = -1;
    if (block->track != NULL) {
        pl_item_unref(block->track);
    }
    block->track = NULL;
    block->queued = 0;

    numblocks_ready--;
    if (numblocks_ready < 0) {
        numblocks_ready = 0;
    }
}

void
streamreader_next_block (void) {
    if (block_data) {
        _streamreader_release_block (block_data);

        block_data = block_data->next;
        if (!block_data) {
            block_data = blocks;
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
        if (b->track != NULL) {
            pl_item_unref(b->track);
            b->track = NULL;
        }
        b->queued = 0;
        b = b->next;
    }
    block_next = blocks;
    block_data = NULL;
    numblocks_ready = 0;
    _firstblock = 0;
}

int
streamreader_num_blocks_ready (void) {
    return numblocks_ready;
}

void
streamreader_flush_after (playItem_t *it) {
    if (!block_data) {
        return;
    }

    streamblock_t *b = block_data;

    int n = numblocks_ready;
    while (b->track == it && n > 0) {
        b = b->next;
        if (!b) {
            b = blocks;
        }
        n--;
    }

    block_next = b;

    while (n > 0) {
        _streamreader_release_block (b);
        _firstblock = 1;
        b = b->next;
        if (!b) {
            b = blocks;
        }
        n--;
    }
}
