#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <unistd.h>

#include "artwork.h"

#define BASE_URL "http://ws.audioscrobbler.com/2.0/"
#define API_KEY "b25b959554ed76058ac220b7b2e0a026"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_functions_t *deadbeef;

int
fetch_from_lastfm (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = curl_easy_escape (NULL, artist, 0);
    char *album_url = curl_easy_escape (NULL, album, 0);
    snprintf (url, sizeof (url), BASE_URL "?method=album.getinfo&api_key=" API_KEY "&artist=%s&album=%s", artist_url, album_url);
    curl_free (artist_url);
    curl_free (album_url);

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
    int size = deadbeef->fread (buffer, 1, sizeof (buffer), fp);
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

    FILE *out = fopen (dest, "w+b");
    if (!out) {
        trace ("fetch_from_lastfm: failed to open %s for writing\n", dest);
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
        unlink (dest);
        return -1;
    }
    return 0;
}
