/*
  DeaDBeeF -- the music player
  Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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

/* Opus artwork decoder, based on `artwork_ogg.c'. Made in 2025 by
   Emil Kosz. */

#include <stdlib.h>
#include <string.h>
#include <opusfile.h>
#include <deadbeef/deadbeef.h>

#include "artwork_opus.h"
#include "ogg_shared.h"

#include "artwork_internal.h"

#define trace(...)                                                      \
    {                                                                   \
        deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); \
    }

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;
extern int artwork_disable_cache;

static int
copus_fread (void *stream, unsigned char *ptr, int nbytes) {
    size_t ret = deadbeef->fread (ptr, 1, nbytes, stream);

    if (ret != nbytes) {
        return -1;
    }

    return (int)ret;
}

static int
copus_fseek (void *stream, opus_int64 offset, int whence) {
    DB_FILE *f = (DB_FILE *)stream;
    return deadbeef->fseek (f, offset, whence);
}

static int
copus_fclose (void *stream) {
    deadbeef->fclose (stream);
    return 0;
}

static opus_int64
copus_ftell (void *stream) {
    return deadbeef->ftell (stream);
}

int
opus_extract_art (ddb_cover_info_t *cover) {
    int err = -1;
    if (!strcasestr (cover->priv->filepath, ".ogg")
        && !strcasestr (cover->priv->filepath, ".oga")
        && !strcasestr (cover->priv->filepath, ".opus")) {
        return -1;
    }

    DB_FILE *fp = NULL;

    fp = deadbeef->fopen (cover->priv->filepath);
    if (!fp) {
        trace ("opus_extract_art: failed to fopen %s\n", cover->priv->filepath);
        return -1;
    }
    if (fp->vfs->is_streaming ()) {
        trace ("opus_extract_art: failed to fopen %s\n", cover->priv->filepath);
        deadbeef->fclose (fp);
        return -1;
    }
    OpusFileCallbacks ofc = { .read = copus_fread, .seek = copus_fseek, .tell = copus_ftell, .close = copus_fclose };
    int res = -1;
    OggOpusFile *opus_file = op_open_callbacks (fp, &ofc, NULL, 0, &res);
    if (res != 0) {
        trace ("opus_extract_art: op_open_callbacks returned: %d\n", res);
        deadbeef->fclose (fp);
        return -1;
    }

    // Now that we correctly open the Opus file, it's time to extract
    // the comments. They are the same as in Ogg Vorbis, and in fact can
    // be cast freely to `vorbis_comment' structs.
    const OpusTags *ot = op_tags (opus_file, 0);
    if (!ot) {
        trace ("opus_extract_art: op_tags failed\n");
        goto error;
    }

    for (int i = 0; i < ot->comments; i++) {
        if (0 == ogg_parse_artwork_comment (ot->user_comments[i], ot->comment_lengths[i], cover)) {
            err = 0;
            break;
        }
    }

error:
    op_free (opus_file);
    return err;
}
