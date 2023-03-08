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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <deadbeef/deadbeef.h>
#include "artwork.h"
#include "artwork_internal.h"
#include "escape.h"

extern DB_functions_t *deadbeef;
extern ddb_artwork_plugin_t plugin;

#define trace(...) { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }
#define trace_err(...) { deadbeef->log_detailed (&plugin.plugin.plugin, DDB_LOG_LAYER_DEFAULT, __VA_ARGS__); }

static const char LFM_URL[] = "http://ws.audioscrobbler.com/2.0/?method=album.getinfo&api_key=%s&artist=%s&album=%s";
static const char API_KEY[] = "6b33c8ae4d598a9aff8fe63e334e6e86";
static const char MEGA_IMAGE_TAG[] = "<image size=\"mega\">";
static const char XL_IMAGE_TAG[] = "<image size=\"extralarge\">";
static const char IMAGE_END_TAG[] = "</image>";

int fetch_from_lastfm (const char *artist, const char *album, const char *dest) {
    struct stat stat_struct;
    int stat_err = stat (dest, &stat_struct);
    int is_reg = S_ISREG (stat_struct.st_mode);
    off_t size = stat_struct.st_size;

    // Is the item in the disk cache?
    if (!stat_err && is_reg && size > 0) {
        return 0;
    }
    if (!artist || !album) {
        return -1;
    }

    if (!*artist || !*album) {
        return -1;
    }

    char *artist_url = uri_escape(artist, 0);
    char *album_url = uri_escape(album, 0);
    size_t url_size = strlen(artist_url) + strlen(album_url) + strlen(LFM_URL) + strlen(API_KEY) + 1;
    char *url = malloc(url_size);

    snprintf(url, url_size, LFM_URL, API_KEY, artist_url, album_url);
    free(artist_url);
    free(album_url);
    artist_url = NULL;
    album_url = NULL;

    trace("fetch_from_lastfm: query: %s\n", url);
    size_t buffer_size = 1000;
    char *buffer = malloc(buffer_size);
    /*const size_t size = */artwork_http_request(url, buffer, buffer_size);

    free (url);
    url = NULL;

    char *img = strstr(buffer, MEGA_IMAGE_TAG);
    if (img) {
        img += sizeof(MEGA_IMAGE_TAG)-1;
    }
    else {
        img = strstr(buffer, XL_IMAGE_TAG);
        if (img) {
            img += sizeof(XL_IMAGE_TAG)-1;
        }
    }
//    trace("fetch_from_lastfm: scrobbler response:\n%s\n", buffer);

    if (!img) {
        trace("fetch_from_lastfm: image tag not found in response (album not found?)\n");
        return -1;
    }

    char *end = strstr(img, IMAGE_END_TAG);
    if (!end) {
        trace("fetch_from_lastfm: XML not well formed, image end tag missing\n");
        return -1;
    }

    if (end == img) {
        trace("fetch_from_lastfm: no image found\n");
        return -1;
    }

    *end = '\0';
    return copy_file(img, dest);
}
