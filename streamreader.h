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

#ifndef streamreader_h
#define streamreader_h

#include "deadbeef.h"
#include "playlist.h"

typedef struct streamblock_s {
    struct streamblock_s *next;
    char *buf;
    int size; // how much bytes total in the buffer, up to BLOCK_SIZE, but can be less
    int pos; // read position in the buffer
    int last; // set to 1 for last buffer of the stream
    int bitrate;

    playItem_t *track;
    ddb_waveformat_t fmt;
} streamblock_t;

void
streamreader_init (void);

void
streamreader_free (void);

// returns:
// negative value: error
// 0: no buffers available
// positive value: number of blocks read (1)
int
streamreader_read_next_block (playItem_t *track, DB_fileinfo_t *fileinfo, streamblock_t **block);

// get current block with data
streamblock_t *
streamreader_get_curr_block (void);

// release the current data block, move to next one
void
streamreader_next_block (void);

void
streamreader_reset (void);

int
streamreader_num_blocks_ready (void);

#endif /* streamreader_h */
