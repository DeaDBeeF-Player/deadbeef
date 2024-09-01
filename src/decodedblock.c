/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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
#include "decodedblock.h"
#include <assert.h>
#include <stdlib.h>

#define BLOCK_COUNT 48 // FIXME: must be the same or more than streamreader block count

static decoded_block_t *_decoded_blocks; // list of all _decoded_blocks

static decoded_block_t *_decoded_blocks_queued; // first available block with data (can be NULL)

static decoded_block_t *_decoded_blocks_free; // next block available to be read into / queued

void
decoded_blocks_init (void) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        decoded_block_t *b = calloc (1, sizeof (decoded_block_t));
        b->next = _decoded_blocks;
        _decoded_blocks = b;
    }
    _decoded_blocks_free = _decoded_blocks;
}

void
decoded_blocks_free (void) {
    while (_decoded_blocks) {
        decoded_block_t *next = _decoded_blocks->next;
        if (_decoded_blocks->track != NULL) {
            pl_item_unref (_decoded_blocks->track);
            _decoded_blocks->track = NULL;
        }
        free (_decoded_blocks);
        _decoded_blocks = next;
    }
    _decoded_blocks_free = _decoded_blocks_queued = NULL;
}

void
decoded_blocks_release (decoded_block_t *b) {
    b->is_silent_header = 0;
    b->last = 0;
    b->first = 0;
    b->remaining_bytes = 0;
    b->total_bytes = 0;
    b->queued = 0;
    b->playback_time = 0;
    if (b->track != NULL) {
        pl_item_unref (b->track);
    }
    b->track = NULL;
}

// Recycle all _decoded_blocks / empty queue.
// Should be called from streamer_reset and similar situations.
void
decoded_blocks_reset (void) {
    decoded_block_t *b = _decoded_blocks;
    while (b) {
        decoded_blocks_release (b);
        b = b->next;
    }
    _decoded_blocks_free = _decoded_blocks;
    _decoded_blocks_queued = NULL;
}

decoded_block_t *
decoded_blocks_current (void) {
    return _decoded_blocks_queued;
}

void
decoded_blocks_next (void) {
    if (_decoded_blocks_queued) {
        decoded_blocks_release (_decoded_blocks_queued);

        _decoded_blocks_queued = _decoded_blocks_queued->next;
        if (!_decoded_blocks_queued) {
            _decoded_blocks_queued = _decoded_blocks;
        }
    }

    if (_decoded_blocks_queued && !_decoded_blocks_queued->queued) {
        _decoded_blocks_queued = NULL; // no available _decoded_blocks with data
    }
}

decoded_block_t *
decoded_blocks_append (void) {
    if (_decoded_blocks_free->queued) {
        return NULL; // all buffers full
    }

    if (!_decoded_blocks_queued) {
        _decoded_blocks_queued = _decoded_blocks_free;
    }

    decoded_block_t *queued_block = _decoded_blocks_free;

    queued_block->queued = 1;

    _decoded_blocks_free = _decoded_blocks_free->next;
    if (!_decoded_blocks_free) {
        _decoded_blocks_free = _decoded_blocks;
    }

    return queued_block;
}

int
decoded_blocks_have_free (void) {
    return _decoded_blocks_free != NULL;
}

float
decoded_blocks_playback_time_total (void) {
    decoded_block_t *first = _decoded_blocks_queued;

    if (first == NULL) {
        return 0;
    }

    decoded_block_t *curr = first;

    float time = 0;

    int wrap = 0;
    while (curr->queued && !(wrap && curr == first)) {
        time += curr->playback_time;

        if (curr == first) {
            wrap = 1;
        }

        curr = curr->next;
        if (curr == NULL) {
            curr = _decoded_blocks;
        }
    }

    return time;
}
