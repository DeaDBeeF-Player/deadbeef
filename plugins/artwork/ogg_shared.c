/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ogg_shared.h"
#include <deadbeef/deadbeef.h>
#include "artwork_internal.h"
#include "artwork.h"
#include "base64.h"

#define trace(...)                                                      \
    {                                                                   \
        deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); \
    }

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;

static const char key[] = "METADATA_BLOCK_PICTURE=";

#define READ_UINT32()                                                                                       \
    ({                                                                                                      \
        if (buffer_size < 4)                                                                                \
            goto error;                                                                   \
        uint32_t _temp32 = (uint32_t)buffer[3] | ((uint32_t)buffer[2] << 8) | ((uint32_t)buffer[1] << 16) | \
                           ((uint32_t)buffer[0] << 24);                                                     \
        buffer += 4;                                                                                        \
        buffer_size -= 4;                                                                                   \
        _temp32;                                                                                            \
    })

#define READ_BUF(buf, size)               \
    {                                     \
        if (buffer_size < size)           \
            goto error; \
        memcpy (buf, buffer, size);       \
        buffer += size;                   \
        buffer_size -= size;              \
    }

int
ogg_parse_artwork_comment (const char *comment, int comment_length, ddb_cover_info_t *cover) {
    uint8_t *decoded_blob = NULL;
    char *mime_type = NULL;
    char *descr = NULL;
    int size = comment_length;
    if (comment_length <= sizeof (key) - 1 || strncasecmp (comment, key, sizeof (key) - 1)) {
        return -1;
    }
    trace ("opus_extract_art: found cover art of %d bytes\n", size);

    // decode base64
    const char *blob = comment + sizeof (key) - 1;
    const int predicted_decoded_size = Base64decode_len (blob);

    if (predicted_decoded_size <= 0) {
        return -1;
    }

    decoded_blob = malloc (predicted_decoded_size);
    if (decoded_blob == NULL) {
        goto error;
    }
    const int decoded_size = Base64decode ((char *)decoded_blob, blob);

    // decode flac picture block
    uint8_t *buffer = decoded_blob;
    int buffer_size = decoded_size;

    /*int32_t picture_type = */ READ_UINT32 ();
    int32_t mime_size = READ_UINT32 ();
    mime_type = calloc (1, mime_size + 1);
    READ_BUF (mime_type, mime_size);
    mime_type[mime_size] = 0;

    // skip non-image data
    if (strcasecmp (mime_type, "image/") && strcasecmp (mime_type, "image/png") &&
        strcasecmp (mime_type, "image/jpeg")) {
        goto error; // unsupported mime type
    }

    free (mime_type);
    mime_type = NULL;
    int32_t descr_size = READ_UINT32 ();
    descr = calloc (1, descr_size + 1);
    READ_BUF (descr, descr_size);
    descr[descr_size] = 0;
    free (descr);
    descr = NULL;
    /*uint32_t width = */ READ_UINT32 ();
    /*uint32_t height = */ READ_UINT32 ();
    /*uint32_t depth = */ READ_UINT32 ();
    /*uint32_t palette_size = */ READ_UINT32 ();
    uint32_t picture_data_len = READ_UINT32 ();

    cover->priv->blob_size = picture_data_len;
    cover->priv->blob_image_size = picture_data_len;
    cover->priv->blob = (char *)decoded_blob;
    cover->priv->blob_image_offset = buffer - decoded_blob;
    return 0;

error:
    free (decoded_blob);
    free (mime_type);
    free (descr);
    return -1;
}
