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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "artwork.h"
#include "escape.h"

#define BASE_URL "http://ws.audioscrobbler.com/2.0/"
#define API_KEY "6b33c8ae4d598a9aff8fe63e334e6e86"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_functions_t *deadbeef;

int
fetch_from_lastfm (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = uri_escape (artist, 0);
    char *album_url = uri_escape (album, 0);
    snprintf (url, sizeof (url), BASE_URL "?method=album.getinfo&api_key=" API_KEY "&artist=%s&album=%s", artist_url, album_url);
    free (artist_url);
    free (album_url);

    DB_FILE *fp = deadbeef->fopen (url);
    if (!fp) {
        trace ("fetch_from_lastfm: failed to open %s\n", url);
        return -1;
    }
    current_file = fp;

    const char searchstr[] = "<image size=\"extralarge\">";
    char buffer[1000];
    memset (buffer, 0, sizeof (buffer));
    char *img = NULL;
    int size = deadbeef->fread (buffer, 1, sizeof (buffer)-1, fp);
    if (size > 0) {
        img = strstr (buffer, searchstr);
    }
    current_file = NULL;
    deadbeef->fclose (fp);

    if (!img) {
        trace ("fetch_from_lastfm: image url not found in response from %s\n", url);
        return -1;
    }

    img += sizeof (searchstr)-1;

    char *end = strstr (img, "</image>");
    if (!end || end == img) {
        trace ("fetch_from_lastfm: bad xml (or image not found) from %s\n", url);
        return -1;
    }

    *end = 0;

    fp = deadbeef->fopen (img);
    if (!fp) {
        trace ("fetch_from_lastfm: failed to open %s\n", img);
        return -1;
    }
    current_file = fp;

    char temp[PATH_MAX];
    snprintf (temp, sizeof (temp), "%s.part", dest);
    FILE *out = fopen (temp, "w+b");
    if (!out) {
        trace ("fetch_from_lastfm: failed to open %s for writing\n", temp);
        deadbeef->fclose (fp);
        current_file = NULL;
        return -1;
    }

    char *writebuffer[4096];
    int len;
    int error = 0;
    while ((len = deadbeef->fread (writebuffer, 1, sizeof (writebuffer), fp)) > 0) {
        if (fwrite (writebuffer, 1, len, out) != len) {
            trace ("fetch_from_lastfm: failed to write to %s\n", dest);
            error = 1;
            break;
        }
    }

    fclose (out);
    current_file = NULL;
    deadbeef->fclose (fp);

    if (error) {
        unlink (temp);
        return -1;
    }

    if (rename (temp, dest) != 0) {
        unlink (temp);
        unlink (dest);
        return -1;
    }

    return 0;
}
