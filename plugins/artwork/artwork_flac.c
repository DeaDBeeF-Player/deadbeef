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

#include <string.h>
#include <limits.h>
#include <deadbeef/deadbeef.h>
#include "artwork_flac.h"
#include "artwork_internal.h"
#ifdef USE_METAFLAC
#include <FLAC/metadata.h>
#endif

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;
extern int artwork_disable_cache;

#ifdef USE_METAFLAC
static size_t
flac_io_read (void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle) {
    return deadbeef->fread (ptr, size, nmemb, (DB_FILE *)handle);
}

static int
flac_io_seek (FLAC__IOHandle handle, FLAC__int64 offset, int whence) {
    return deadbeef->fseek ((DB_FILE *)handle, offset, whence);
}

static FLAC__int64
flac_io_tell (FLAC__IOHandle handle) {
    return deadbeef->ftell ((DB_FILE *)handle);
}

static FLAC__IOCallbacks flac_iocb = {
    .read = flac_io_read,
    .write = NULL,
    .seek = flac_io_seek,
    .tell = flac_io_tell,
    .eof = NULL,
    .close = NULL
};

int
flac_extract_art (ddb_cover_info_t *cover) {
    if (!strcasestr (cover->priv->filepath, ".flac") && !strcasestr (cover->priv->filepath, ".oga")) {
        return -1;
    }
    int err = -1;
    DB_FILE *file = NULL;
    FLAC__Metadata_Iterator *iterator = NULL;

    FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new ();
    if (!chain) {
        return -1;
    }

    file = deadbeef->fopen (cover->priv->filepath);
    if (!file) {
        trace ("artwork: failed to open %s\n", cover->priv->filepath);
        goto error;
    }

    int res = FLAC__metadata_chain_read_with_callbacks (chain, (FLAC__IOHandle)file, flac_iocb);
#if USE_OGG
    if (!res && FLAC__metadata_chain_status (chain) == FLAC__METADATA_CHAIN_STATUS_NOT_A_FLAC_FILE) {
        res = FLAC__metadata_chain_read_ogg_with_callbacks (chain, (FLAC__IOHandle)file, flac_iocb);
    }
#endif
    deadbeef->fclose (file);
    if (!res) {
        trace ("artwork: failed to read metadata from flac: %s\n", cover->priv->filepath);
        goto error;
    }

    FLAC__StreamMetadata *picture = 0;
    iterator = FLAC__metadata_iterator_new ();
    if (!iterator) {
        goto error;
    }
    FLAC__metadata_iterator_init (iterator, chain);
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block (iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            picture = block;
        }
    } while (FLAC__metadata_iterator_next (iterator) && 0 == picture);

    if (!picture) {
        trace ("%s doesn't have an embedded cover\n", cover->priv->filepath);
        goto error;
    }

    FLAC__StreamMetadata_Picture *pic = &picture->data.picture;
    if (pic && pic->data_length > 0) {
        trace ("found flac cover art of %d bytes (%s)\n", pic->data_length, pic->description);
        cover->priv->blob = malloc (pic->data_length);
        memcpy (cover->priv->blob, pic->data, pic->data_length);
        cover->priv->blob_size = pic->data_length;
        cover->priv->blob_image_size = pic->data_length;
        err = 0;
    }
error:
    if (chain) {
        FLAC__metadata_chain_delete (chain);
    }
    if (iterator) {
        FLAC__metadata_iterator_delete (iterator);
    }
    return err;
}
#endif
