/*
    world of spectrum game cover downloader for the Deadbeef Artwork plugin
    Copyright (C) 2009-2013 Alexey Yakovenko and other contributors

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
#include <ctype.h>
#include "artwork_internal.h"
#include "escape.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

void
strcopy_escape (char *dst, int d_len, const char *src, size_t n) {
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

#define WOS_URL "http://www.worldofspectrum.org/showscreen.cgi?screen=screens/load/%c/gif/%s.gif"
int fetch_from_wos (const char *title, const char *dest)
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

    char *title_url = uri_escape(t, 0);
    char url[sizeof(WOS_URL) + strlen(title_url)];
    sprintf(url, WOS_URL, tolower(title_url[0]), title_url);
    free(title_url);
    trace("WOS request: %s\n", url);
    return copy_file(url, dest);
}
