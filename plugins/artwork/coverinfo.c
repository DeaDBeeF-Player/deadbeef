/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

#include "artwork.h"
#include "coverinfo.h"
#include <assert.h>
#include <stdlib.h>

typedef struct cover_info_s {
    ddb_cover_info_t info;

    int refc; // Reference count, to allow sending the same cover to multiple callbacks

    // prev/next in the list of all alive cover_info_t objects
    struct cover_info_s *prev;
    struct cover_info_s *next;
} cover_info_t;

static cover_info_t *cover_info_list;

ddb_cover_info_t *
cover_info_alloc (void) {
    cover_info_t *info = calloc(1, sizeof (cover_info_t));

    info->refc = 1;
    info->info.timestamp = time(NULL);

    info->prev = NULL;

    if (cover_info_list != NULL) {
        cover_info_list->prev = info;
    }

    info->next = cover_info_list;
    cover_info_list = info;

    return &info->info;
}

void
cover_info_ref (ddb_cover_info_t *cover) {
    cover_info_t *info = (cover_info_t *)cover;
    info->refc++;
}

void
cover_info_release (ddb_cover_info_t *cover) {
    cover_info_t *info = (cover_info_t *)cover;

    assert (info->refc > 0);
    info->refc -= 1;
    if (info->refc != 0) {
        return;
    }
    if (cover->type) {
        free (cover->type);
    }
    if (cover->image_filename) {
        free (cover->image_filename);
    }
    if (cover->blob) {
        free (cover->blob);
    }

    // remove from list
    if (info->prev) {
        info->prev->next = info->next;
    }
    else {
        cover_info_list = info->next;
    }
    if (info->next) {
        info->next->prev = info->prev;
    }

    free (cover);
}

void
cover_info_cleanup (void) {
    while (cover_info_list) {
        cover_info_release(&cover_info_list->info);
    }
}
