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
ogg_artwork_comment_is_picture (const char *comment, int comment_length) {
    const size_t key_length = sizeof (key) - 1;
    if (comment_length < 0 || (size_t)comment_length <= key_length || strncasecmp (comment, key, key_length)) {
        return 0;
    }

    // Decode only the beginning of the picture block, which has the picture type, and the mime type:
    // 4 bytes picture type, 4 bytes mime type size, followed by the mime type string.
    // The base64 text is cut at the multiple of 4 characters, so that it can be decoded on its own.
    char prefix[257];
    size_t prefix_length = (size_t)comment_length - key_length;
    if (prefix_length > sizeof (prefix) - 1) {
        prefix_length = sizeof (prefix) - 1;
    }
    prefix_length &= ~(size_t)3;
    if (prefix_length < 16) {
        return 0;
    }
    memcpy (prefix, comment + key_length, prefix_length);
    prefix[prefix_length] = 0;

    uint8_t decoded[sizeof (prefix)];
    const int decoded_size = Base64decode ((char *)decoded, prefix);
    if (decoded_size < 8) {
        return 0;
    }

    const uint32_t mime_size = (uint32_t)decoded[4] << 24 | (uint32_t)decoded[5] << 16 | (uint32_t)decoded[6] << 8 | decoded[7];
    if (mime_size >= 16 || 8 + mime_size > (uint32_t)decoded_size) {
        return 0;
    }

    char mime_type[16];
    memcpy (mime_type, decoded + 8, mime_size);
    mime_type[mime_size] = 0;

    return !strcasecmp (mime_type, "image/") || !strcasecmp (mime_type, "image/png")
        || !strcasecmp (mime_type, "image/jpeg");
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

    int32_t picture_type = READ_UINT32 ();
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
    if (picture_data_len > (uint32_t)buffer_size) {
        goto error; // truncated picture data
    }

    cover->priv->blob_size = picture_data_len;
    cover->priv->blob_image_size = picture_data_len;
    cover->priv->blob = (char *)decoded_blob;
    cover->priv->blob_image_offset = buffer - decoded_blob;
    cover->cover_type = artwork_cover_type_for_picture_type (picture_type);
    return 0;

error:
    free (decoded_blob);
    free (mime_type);
    free (descr);
    return -1;
}
