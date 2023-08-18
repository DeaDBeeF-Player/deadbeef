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

#ifndef medialibdb_h
#define medialibdb_h

#include <stdint.h>
#include <deadbeef/deadbeef.h>
#include "medialibstate.h"

#define ML_HASH_SIZE 4096
typedef struct ml_entry_s {
    const char *file;
    ddb_playItem_t **tracks;
    size_t track_count;
    struct ml_entry_s *bucket_next;
} ml_filename_hash_item_t;

typedef struct {
    // A hash formed by filename pointer.
    // This hash purpose is to quickly check whether the filename is in the library already.
    // Doesn't contain subtracks.
    ml_filename_hash_item_t *filename_hash[ML_HASH_SIZE];

    /// Selected / expanded state.
    /// State is associated with IDs, therefore it survives updates/scans, as long as IDs are reused correctly.
    ml_collection_state_t state;
} ml_db_t;

uint32_t
ml_collection_hash_for_ptr (void *ptr);

void
ml_db_free (ml_db_t *db);

void
ml_db_init (DB_functions_t *_deadbeef);

#endif /* medialibdb_h */

