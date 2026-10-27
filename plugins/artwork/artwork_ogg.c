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

#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>
#include <deadbeef/deadbeef.h>
#include "artwork_ogg.h"
#include "artwork_internal.h"
#include "base64.h"
#include "ogg_shared.h"

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;

static size_t
cvorbis_fread (void *ptr, size_t size, size_t nmemb, void *datasource) {
    size_t ret = deadbeef->fread (ptr, size, nmemb, datasource);
    //    trace ("cvorbis_fread %d %d %d\n", size, nmemb, ret);
    return ret;
}

static int
cvorbis_fseek (void *datasource, ogg_int64_t offset, int whence) {
    DB_FILE *f = (DB_FILE *)datasource;
    return deadbeef->fseek (f, offset, whence);
}

static int
cvorbis_fclose (void *datasource) {
    deadbeef->fclose (datasource);
    return 0;
}

static long
cvorbis_ftell (void *datasource) {
    return deadbeef->ftell (datasource);
}

// If @c cover is NULL: returns the number of pictures, or -1 on failure.
// Otherwise: loads the picture at @c index into @c cover, and returns 0 on success, or -1 on failure.
static int
ogg_scan (const char *path, ddb_cover_info_t *cover, int index) {
    int result = -1;
    if (!strcasestr (path, ".ogg")
        && !strcasestr (path, ".oga")) {
        return -1;
    }

    DB_FILE *fp = NULL;
    OggVorbis_File vorbis_file;

    fp = deadbeef->fopen (path);
    if (!fp) {
        trace ("ogg_extract_art: failed to fopen %s\n", path);
        return -1;
    }
    if (fp->vfs->is_streaming ()) {
        trace ("ogg_extract_art: failed to fopen %s\n", path);
        deadbeef->fclose(fp);
        return -1;
    }
    ov_callbacks ovcb = {
        .read_func = cvorbis_fread,
        .seek_func = cvorbis_fseek,
        .close_func = cvorbis_fclose,
        .tell_func = cvorbis_ftell
    };
    int res = ov_open_callbacks (fp, &vorbis_file, NULL, 0, ovcb);
    if (res != 0) {
        trace ("ogg_extract_art: ov_open_callbacks returned %d\n", res);
        deadbeef->fclose(fp);
        return -1;
    }

    const vorbis_comment *vc = ov_comment(&vorbis_file, 0);
    if (!vc) {
        trace("ogg_extract_art: ov_comment failed\n");
        goto error;
    }

    int count = 0;
    for (int i = 0; i < vc->comments; i++) {
        const int load = cover != NULL && count == index;
        if (0 == ogg_parse_artwork_comment (vc->user_comments[i], vc->comment_lengths[i], load ? cover : NULL)) {
            if (load) {
                result = 0;
                break;
            }
            count++;
        }
    }
    if (cover == NULL) {
        result = count;
    }

error:
    ov_clear (&vorbis_file);
    return result;
}

int
ogg_extract_art (ddb_cover_info_t *cover) {
    return ogg_scan (cover->priv->filepath, cover, 0);
}

int
ogg_extract_art_at_index (ddb_cover_info_t *cover, int index) {
    return ogg_scan (cover->priv->filepath, cover, index);
}

int
ogg_cover_count (const char *path) {
    int count = ogg_scan (path, NULL, 0);
    return count < 0 ? 0 : count;
}
