/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "artwork.h"
#include "escape.h"

extern DB_functions_t *deadbeef;

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

int
fetch_from_albumart_org (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = uri_escape (artist, 0);
    char *album_url = uri_escape (album, 0);
    snprintf (url, sizeof (url), "http://www.albumart.org/index.php?srchkey=%s+%s&itempage=1&newsearch=1&searchindex=Music", artist_url, album_url);
    free (artist_url);
    free (album_url);

    DB_FILE *fp = deadbeef->fopen (url);
    if (!fp) {
        trace ("fetch_from_albumart_org: failed to open %s\n", url);
        return -1;
    }
    current_file = fp;
    const char searchstr[] = "http://ecx.images-amazon.com/images/I/";
    char buffer[10000];
    memset (buffer, 0, sizeof (buffer));
    char *img = NULL;
    int size = deadbeef->fread (buffer, 1, sizeof (buffer), fp);
    if (size > 0) {
        img = strstr (buffer, searchstr);
    }
    current_file = NULL;
    deadbeef->fclose (fp);

    if (!img) {
        trace ("fetch_from_albumart_org: image url not found in response from %s (%d bytes)\n", url, size);
        return -1;
    }

    char *end = strstr (img, "._SL160_");
    if (!end || end == img)
    {
        trace ("fetch_from_albumart_org: bad xml from %s\n", url);
        return -1;
    }
    strcpy (end, ".jpg");

    fp = deadbeef->fopen (img);
    if (!fp) {
        trace ("fetch_from_albumart_org: failed to open %s\n", img);
        return -1;
    }
    current_file = fp;

    char temp[PATH_MAX];
    snprintf (temp, sizeof (temp), "%s.part", dest);
    FILE *out = fopen (temp, "w+b");
    if (!out) {
        trace ("fetch_from_albumart_org: failed to open %s for writing\n", temp);
        current_file = NULL;
        deadbeef->fclose (fp);
        return -1;
    }

    char *writebuffer[4096];
    int len;
    int error = 0;
    while ((len = deadbeef->fread (writebuffer, 1, sizeof (writebuffer), fp)) > 0) {
        if (fwrite (writebuffer, 1, len, out) != len) {
            trace ("fetch_from_albumart_org: failed to write to %s\n", dest);
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

