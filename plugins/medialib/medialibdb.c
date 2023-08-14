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

#include <stdlib.h>
#include <string.h>
#include "medialibdb.h"

static DB_functions_t *deadbeef;

uint32_t
ml_collection_hash_for_ptr (void *ptr) {
    // scrambling multiplier from http://vigna.di.unimi.it/ftp/papers/xorshift.pdf
    uint64_t scrambled = 1181783497276652981ULL * (uintptr_t)ptr;
    return (uint32_t)(scrambled & (ML_HASH_SIZE-1));
}


#pragma mark -

void
ml_db_free (ml_db_t *db) {
    fprintf (stderr, "clearing index...\n");

    for (int i = 0; i < ML_HASH_SIZE; i++) {
        ml_filename_hash_item_t *en = db->filename_hash[i];
        while (en) {
            ml_filename_hash_item_t *next = en->bucket_next;
            if (en->file) {
                deadbeef->metacache_remove_string (en->file);
            }
            free (en);
            en = next;
        }
        db->filename_hash[i] = NULL;
    }

    memset (db, 0, sizeof (ml_db_t));
}

void
ml_db_init (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
}
