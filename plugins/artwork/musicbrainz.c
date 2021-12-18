/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>
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
#include <stdlib.h>
#include <string.h>
#include "artwork_internal.h"
#include "escape.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

#define MB_ID_LENGTH 36
#define MB_BUFFER_LENGTH 4096

#define MB_ID_URL "http://musicbrainz.org/ws/2/release-group/?query=artist:%%22%s%%22%%20AND%%20release:%%22%s%%22"
#define MB_ID_STRING "<release-group id=\""

#define MB_ART_URL "http://coverartarchive.org/release-group/%s/"
#define MB_ART_STRING "\"large\":\""


int fetch_from_musicbrainz (const char *artist, const char *album, const char *dest) {
    if (!artist || !album) {
        return -1;
    }

    char *artist_url = uri_escape(artist, 0);
    char *album_url = uri_escape(album, 0);
    if (!artist_url || !album_url) {
        return -1;
    }

    char *id_url = malloc(sizeof(MB_ID_URL) + strlen(artist_url) + strlen(album_url));
    if (id_url) {
        sprintf(id_url, MB_ID_URL, artist_url, album_url);
    }
    free(artist_url);
    free(album_url);
    if (!id_url) {
        return -1;
    }

    trace("fetch_from_musicbrainz: search for release group MBID %s\n", id_url);
    char buffer[MB_BUFFER_LENGTH+1];
    const size_t id_size = artwork_http_request(id_url, buffer, sizeof(buffer));
    if (!id_size) {
        trace("fetch_from_musicbrainz: failed to read (%d)\n", id_size);
        return -1;
    }

    char *mbid = strstr(buffer, MB_ID_STRING);
    if (!mbid || mbid+sizeof(MB_ID_STRING)+MB_ID_LENGTH > buffer+id_size) {
        trace("fetch_from_musicbrainz: release-group ID not found in response (%d bytes)\n", id_size);
        return -1;
    }

    mbid += sizeof(MB_ID_STRING)-1;
    *(mbid + MB_ID_LENGTH) = '\0';
    char art_url[sizeof(MB_ART_URL) + MB_ID_LENGTH];
    sprintf(art_url, MB_ART_URL, mbid);

    trace("fetch_from_musicbrainz: get art list for the MBID %s\n", art_url);
    const size_t art_size = artwork_http_request(art_url, buffer, sizeof(buffer));
    if (!art_size) {
        trace("fetch_from_musicbrainz: failed to read (%d)\n", art_size);
        return -1;
    }

    char *image_url = strstr(buffer, MB_ART_STRING);
    if (!image_url) {
        trace("fetch_from_musicbrainz: large thumb not found in response (%d bytes)\n", art_size);
        return -1;
    }

    image_url += sizeof(MB_ART_STRING)-1;
    char *image_url_end = strchr(image_url, '"');
    if (image_url_end) {
        *image_url_end = '\0';
    }

    trace("fetch_from_musicbrainz: download image %s\n", image_url);
    return copy_file(image_url, dest);
}
