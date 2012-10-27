/*
    world of spectrum game cover downloader for the Deadbeef Artwork plugin
    Copyright (C) 2009-2012 Alexey Yakovenko and other contributors

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
#include <ctype.h>

#include "artwork.h"
#include "escape.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_functions_t *deadbeef;

#define BASE_URL "http://www.worldofspectrum.org/showscreen.cgi?screen=screens/load"
#define min(x,y) ((x)<(y)?(x):(y))

void
strcopy_escape (char *dst, int d_len, const char *src, int n) {
    char *e = dst + d_len - 1;
    const char *se = src + n;
    while (dst < e && *src && src < se) {
        if (*src != ' ' && *src != '!') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = 0;
}

int
fetch_from_wos (const char *title, const char *dest)
{
    // extract game title from title
    char t[100];
    char *dash = strstr (title, " -");
    if (!dash) {
        strcopy_escape (t, sizeof (t), title, strlen (title));
    }
    else {
        strcopy_escape(t, sizeof (t), title, dash-title);
    }
    char *sp;
    while (sp = strchr(t, ' ')) {
        *sp = '_';
    }
    char *title_url = uri_escape (t, 0);
    char url [1024];
    snprintf (url, sizeof (url), BASE_URL "/%c/gif/%s.gif", tolower (title_url[0]), title_url);
    free (title_url);
    trace ("WOS request: %s\n", url);

    DB_FILE *fp = NULL;

    fp = deadbeef->fopen (url);
    if (!fp) {
        trace ("fetch_from_wos: failed to open %s\n", url);
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
