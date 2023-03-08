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

#include <deadbeef/deadbeef.h>
#include "artwork.h"
#include "artwork_internal.h"
#include "coverinfo.h"
#include <assert.h>
#include <stdlib.h>

static ddb_cover_info_t *cover_info_list;

ddb_cover_info_t *
cover_info_alloc (void) {
    ddb_cover_info_t *info = calloc(1, sizeof (ddb_cover_info_t));

    info->priv = calloc(1, sizeof (ddb_cover_info_priv_t));

    info->_size = sizeof (ddb_cover_info_t);
    info->priv->refc = 1;
    info->priv->timestamp = time(NULL);

    info->priv->prev = NULL;

    if (cover_info_list != NULL) {
        cover_info_list->priv->prev = info;
    }

    info->priv->next = cover_info_list;
    cover_info_list = info;

    return info;
}

void
cover_info_ref (ddb_cover_info_t *cover) {
    cover->priv->refc++;
}

void
cover_info_release (ddb_cover_info_t *cover) {
    assert (cover->priv->refc > 0);
    cover->priv->refc -= 1;
    if (cover->priv->refc != 0) {
        return;
    }
    free (cover->image_filename);
    free (cover->priv->blob);

    // remove from list
    if (cover->priv->prev) {
        cover->priv->prev->priv->next = cover->priv->next;
    }
    else {
        cover_info_list = cover->priv->next;
    }
    if (cover->priv->next) {
        cover->priv->next->priv->prev = cover->priv->prev;
    }
    free (cover->priv);

    free (cover);
}

void
cover_info_cleanup (void) {
    while (cover_info_list) {
        cover_info_release(cover_info_list);
    }
}
