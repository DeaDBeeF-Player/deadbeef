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

#ifndef streamreader_h
#define streamreader_h

#include <deadbeef/deadbeef.h>
#include "playlist.h"

typedef struct streamblock_s {
    struct streamblock_s *next;
    char *buf;
    int size; // how much bytes total in the buffer, up to BLOCK_SIZE, but can be less
    int pos; // read position in the buffer
    int first; // set to 1 for the first buffer of the stream, following the block with last=1
    int last; // set to 1 for last buffer of the stream
    int bitrate;
    int is_silent_header; // set to 1 if the block represents the added silence

    playItem_t *track;
    ddb_waveformat_t fmt;

    int queued;
} streamblock_t;

void
streamreader_init (void);

void
streamreader_free (void);

// returns next available (free) block, or NULL.
streamblock_t *
streamreader_get_next_block (void);

// Reads data from stream to the specified block.
// The mutex must NOT be locked when this function is called.
// It will get locked if successful.
// Returns negative value on error.
int
streamreader_read_block (streamblock_t *block, playItem_t *track, DB_fileinfo_t *fileinfo, uint64_t mutex);

int
streamreader_silence_block (streamblock_t *block, playItem_t *track, DB_fileinfo_t *fileinfo, uint64_t mutex);

// Appends (enqueues) the block to the list of blocks containing data.
// The passed block pointer must be the same as returned by `streamreader_get_next_block`.
void
streamreader_enqueue_block (streamblock_t *block);

// Get the first (current) block with data from the queue.
// Can return NULL.
streamblock_t *
streamreader_get_curr_block (void);

// Release (unqueue) the current data block, move to next one
void
streamreader_next_block (void);

// Resets the queue
void
streamreader_reset (void);

// Number of blocks in the queue
int
streamreader_num_blocks_ready (void);

// Notify streamreader that some configuration has changed
void
streamreader_configchanged (void);

// Remove any blocks after the ones referencing `it`
void
streamreader_flush_after (playItem_t *it);

#endif /* streamreader_h */
