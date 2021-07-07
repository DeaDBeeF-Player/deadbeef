/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "artwork_internal.h"
#include "escape.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

#define LFM_URL "http://ws.audioscrobbler.com/2.0/?method=album.getinfo&api_key=%s&artist=%s&album=%s"
#define API_KEY "6b33c8ae4d598a9aff8fe63e334e6e86"
#define MEGA_IMAGE_TAG "<image size=\"mega\">"
#define XL_IMAGE_TAG "<image size=\"extralarge\">"
#define IMAGE_END_TAG "</image>"
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
    char *url = malloc(strlen(artist_url) + strlen(album_url) + sizeof(LFM_URL API_KEY));
    if (url) {
        sprintf(url, LFM_URL, API_KEY, artist_url, album_url);
    }
    free(artist_url);
    free(album_url);
    if (!url) {
        return -1;
    }

    trace("fetch_from_lastfm: query: %s\n", url);
    char buffer[1000];
    /*const size_t size = */artwork_http_request(url, buffer, sizeof(buffer));
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
