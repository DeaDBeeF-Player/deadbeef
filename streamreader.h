//
//  streamreader.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 09/02/2017.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

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

#endif /* streamreader_h */
