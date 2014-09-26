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

#define AAO_URL "http://www.albumart.org/index.php?searchk=%s+%s&itempage=1&newsearch=1&searchindex=Music"
int fetch_from_albumart_org (const char *artist, const char *album, const char *dest)
{
    char *artist_url = uri_escape (artist ? artist : "", 0);
    char *album_url = uri_escape (album ? album : "", 0);
    char *url = malloc(sizeof(AAO_URL) + strlen(artist_url) + strlen(album_url) + 1);
    if (url) {
        sprintf (url, AAO_URL, artist_url, album_url);
    }
    free (artist_url);
    free (album_url);
    if (!url) {
        return -1;
    }

    trace("fetch_from_albumart_org: %s\n", url);
    DB_FILE *fp = deadbeef->fopen (url);
    free(url);
    if (!fp) {
        trace ("fetch_from_albumart_org: failed to open %s\n", url);
        return -1;
    }
    current_file = fp;
    char buffer[10000];
    const int size = deadbeef->fread(buffer, 1, sizeof (buffer), fp);
    char *img = NULL;
    if (size > 0) {
        buffer[size] = '\0';
        img = strstr (buffer, "http://ecx.images-amazon.com/images/I/");
    }
    current_file = NULL;
    deadbeef->fclose (fp);

    if (!img) {
        trace ("fetch_from_albumart_org: image url not found in response from (%d bytes)\n", size);
        return -1;
    }

    char *end = strstr (img, "._SL160_.jpg");
    if (!end || end == img)
    {
        trace ("fetch_from_albumart_org: bad xml\n");
        return -1;
    }

    strcpy (end, ".jpg");
    return copy_file(img, dest);
}
