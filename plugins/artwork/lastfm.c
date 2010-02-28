#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>

#include "artwork.h"

#define BASE_URL "http://ws.audioscrobbler.com/2.0/"
#define API_KEY "b25b959554ed76058ac220b7b2e0a026"

int
fetch_from_lastfm (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = curl_easy_escape (NULL, artist, 0);
    char *album_url = curl_easy_escape (NULL, album, 0);
    snprintf (url, sizeof (url), BASE_URL "?method=album.getinfo&api_key=" API_KEY "&artist=%s&album=%s", artist_url, album_url );
    curl_free (artist_url);
    curl_free (album_url);

    char *response = fetch (url);
//    printf ("%s\n", response);
    char *img = strstr (response, "<image size=\"extralarge\">");
    if (!img)
    {
        free (response);
        return 0;
    }
    img += 25;
    char *end = strstr (img, "</image>");
    if (!end || (end == img))
    {
        free (response);
        return 0;
    }
    *end = 0;
    int res = fetch_to_file (img, dest);
    free (response);
    return res;
}
