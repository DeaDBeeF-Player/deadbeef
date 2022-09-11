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
#ifndef decodedblock_h
#define decodedblock_h

#include "playlist.h"

// Each decoded block directly corresponds to encoded block.
// The decoded blocks don't hold the data, which is stored in the _output_buffer.
// As the data is consumed, playpos/playtime should advance, and the blocks should be recycled.
typedef struct decoded_block_s {
    int is_silent_header; // set to 1 if the block represents the added silence
    int last;
    int first;
    int remaining_bytes;
    int total_bytes;
    int queued;
    float playback_time;
    playItem_t *track;
    struct decoded_block_s *next;
} decoded_block_t;

void
decoded_blocks_init (void);

void
decoded_blocks_free (void);

// Recycle all blocks / empty queue.
// Should be called from streamer_reset and similar situations.
void
decoded_blocks_reset (void);

decoded_block_t *
decoded_blocks_current (void);

void
decoded_blocks_next (void);

decoded_block_t *
decoded_blocks_append (void);

int
decoded_blocks_have_free (void);

float
decoded_blocks_playback_time_total (void);

#endif /* decodedblock_h */
