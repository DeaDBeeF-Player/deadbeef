/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include "artwork.h"

int
fetch_from_albumart_org (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = curl_easy_escape (NULL, artist, 0);
    char *album_url = curl_easy_escape (NULL, album, 0);
    snprintf (url, sizeof (url), "http://www.albumart.org/index.php?srchkey=%s+%s&itempage=1&newsearch=1&searchindex=Music", artist_url, album_url);
    curl_free (artist_url);
    curl_free (album_url);

    char *response = fetch (url);
//    printf ("%s\n", response);
    char *img = strstr (response, "http://ecx.images-amazon.com/images/I/");
    if (!img)
    {
        free (response);
        return 0;
    }
    char *end = strstr (img, "._SL160_");
    if (!end)
    {
        free (response);
        return 0;
    }
    strcpy (end, ".jpg");

    int res = fetch_to_file (img, dest);
    free (response);
    return res;
}

