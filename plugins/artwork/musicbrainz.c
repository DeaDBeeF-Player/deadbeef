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
#include <stdlib.h>
#include <string.h>
#include "../../deadbeef.h"
#include "artwork_internal.h"
#include "escape.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

#define MB_ID_URL "http://musicbrainz.org/ws/2/release/?query=artist:%%22%s%%22%%20AND%%20release:%%22%s%%22"
#define MB_ID_STRING "<release id=\""

#define MB_ART_URL "http://coverartarchive.org/release/%s"
#define MB_ART_STRING "\"large\":\""

int fetch_from_musicbrainz (const char *artist, const char *album, const char *dest)
{
    char *artist_url = uri_escape (artist ? artist : "", 0);
    char *album_url = uri_escape (album ? album : "", 0);
    char *id_url = malloc(sizeof(MB_ID_URL) + strlen(artist_url) + strlen(album_url) + 1);
    if (id_url) {
        sprintf (id_url, MB_ID_URL, artist_url, album_url);
    }
    free (artist_url);
    free (album_url);
    if (!id_url) {
        return -1;
    }

    trace("fetch_from_musicbrainz: search %s\n", id_url);
    DB_FILE *fp = deadbeef->fopen(id_url);
    free(id_url);
    if (!fp) {
        trace("fetch_from_musicbrainz: failed to open url\n");
        return -1;
    }

    current_file = fp;
    char buffer[1000];
    const int id_size = deadbeef->fread(buffer, 1, sizeof(buffer), fp);
    current_file = NULL;
    deadbeef->fclose (fp);
    if (id_size <= 0) {
        trace("fetch_from_musicbrainz: failed to read (%d)\n", id_size);
        return -1;
    }

    buffer[id_size] = '\0';
    char *mbid = strstr(buffer, MB_ID_STRING);
    if (!mbid || mbid+strlen(MB_ID_STRING)+36 > buffer+id_size) {
        trace("fetch_from_musicbrainz: release ID not found in response (%d bytes)\n", id_size);
        return -1;
    }

    mbid += strlen(MB_ID_STRING);
    *(mbid + 36) = '\0';
    char *art_url = malloc(sizeof(MB_ART_URL) + strlen(mbid) + 1);
    if (!art_url) {
        return -1;
    }

    sprintf(art_url, MB_ART_URL, mbid);
    trace("fetch_from_musicbrainz: art %s\n", art_url);
    fp = deadbeef->fopen(art_url);
    free(art_url);
    if (!fp) {
        trace("fetch_from_musicbrainz: failed to open url\n");
        return -1;
    }

    current_file = fp;
    const int art_size = deadbeef->fread(buffer, 1, sizeof(buffer), fp);
    current_file = NULL;
    deadbeef->fclose (fp);
    if (art_size <= 0) {
        trace("fetch_from_musicbrainz: failed to read (%d)\n", art_size);
        return -1;
    }

    buffer[art_size] = '\0';
    char *image_url = strstr(buffer, MB_ART_STRING);
    if (!image_url) {
        trace("fetch_from_musicbrainz: large thumb not found in response (%d bytes)\n", art_size);
        return -1;
    }

    image_url += strlen(MB_ART_STRING);
    char *image_url_end = strchr(image_url, '"');
    if (image_url_end) {
        *image_url_end = '\0';
    }

    trace("fetch_from_musicbrainz: image %s\n", image_url);
    return copy_file(image_url, dest);
}
