/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "../../deadbeef.h"

static DB_misc_t plugin;
static DB_functions_t *deadbeef;
static char lastfm_login[50];
static char lastfm_pass[50];

#define SCROBBLER_URL "http://ws.audioscrobbler.com/2.0"
#define LASTFM_API_KEY "6b33c8ae4d598a9aff8fe63e334e6e86"
#define LASTFM_API_SECRET "a9f5e17e358377d96e96477d870b2b18"

DB_plugin_t *
lastfm_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static char *lastfm_srv_res;
static char lastfm_srv_size;
static char lastfm_curl_err[CURL_ERROR_SIZE];

static size_t
lastfm_curl_res (void *ptr, size_t size, size_t nmemb, void *stream)
{
    int len = size * nmemb;
    lastfm_srv_res = realloc (lastfm_srv_res, lastfm_srv_size + len + 1);
    memcpy (lastfm_srv_res + lastfm_srv_size, ptr, len);
    lastfm_srv_size += len;

    char s[size*nmemb+1];
    memcpy (s, ptr, size*nmemb);
    s[size*nmemb] = 0;
    printf ("%s\n", s);

    return len;
}

int
lastfm_auth (void) {
    // auth
    char msg[4096];
    char sigstr[4096];
    uint8_t sig[16];
    snprintf (sigstr, sizeof (sigstr), "api_key%smethodauth.getToken%s", SCROBBLER_URL, LASTFM_API_KEY, LASTFM_API_SECRET);
    deadbeef->md5 (sig, sigstr, strlen (sigstr));
    deadbeef->md5_to_str (sigstr, sig);
    snprintf (msg, sizeof (msg), "%s/?method=auth.getToken&api_key=%s&api_sig=%s", SCROBBLER_URL, LASTFM_API_KEY, sigstr);
    printf ("sending request: %s\n", msg);
    // init curl
    CURL *curl;
    curl = curl_easy_init ();
    if (!curl) {
        fprintf (stderr, "lastfm: failed to init curl\n");
        return 0;
    }
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_URL, msg);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, lastfm_curl_res);
    memset(lastfm_curl_err, 0, sizeof(lastfm_curl_err));
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, lastfm_curl_err);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    int status = curl_easy_perform(curl);
    curl_easy_cleanup (curl);

    if (!status) {
        // parse output
        if (strstr (lastfm_srv_res, "<lfm status=\"ok\">")) {
            char *token = strstr (lastfm_srv_res, "<token>");
            if (token) {
                token += 7;
                char *end = strstr (token, "</token>");
                if (end) {
                    *end = 0;
                    printf ("token: %s\n", token);
                }
                else {
                    printf ("no </token>\n");
                }
            }
            else {
                printf ("no <token>\n");
            }
        }
        else {
            printf ("not ok\n");
        }
    }

    if (lastfm_srv_res) {
        free (lastfm_srv_res);
        lastfm_srv_res = NULL;
    }
}

static int
lastfm_frameupdate (int ev, uintptr_t data) {
//    printf ("lastfm tick\n");
    return 0;
}

static int
lastfm_songchanged (int ev, uintptr_t data) {
    printf ("song changed\n");
    return 0;
}

static int
lastfm_start (void) {
    // subscribe to frameupdate event
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, lastfm_frameupdate, 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, lastfm_songchanged, 0);
    // load login/pass
    char config[1024];
    snprintf (config, 1024, "%s/.config/deadbeef/lastfm", getenv ("HOME"));
    FILE *fp = fopen (config, "rt");
    if (!fp) {
        fprintf (stderr, "lastfm: failed open %s\n", config);
        return -1;
    }
    if (!fgets (lastfm_login, 50, fp)) {
        fprintf (stderr, "lastfm: failed to read login from %s\n", config);
        fclose (fp);
        return -1;
    }
    if (!fgets (lastfm_pass, 50, fp)) {
        fprintf (stderr, "lastfm: failed to read pass from %s\n", config);
        fclose (fp);
        return -1;
    }
    fclose (fp);
    // remove trailing garbage
    int l;
    char *p;
    l = strlen (lastfm_login);
    p = lastfm_login+l-1;
    while (p >= lastfm_login && *p < 0x20) {
        p--;
    }
    *p = 0;
    l = strlen (lastfm_pass);
    p = lastfm_pass+l-1;
    while (p >= lastfm_login && *p < 0x20) {
        p--;
    }
    *p = 0;
    lastfm_auth ();

    return 0;
}

static int
lastfm_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_FRAMEUPDATE, lastfm_frameupdate, 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGCHANGED, lastfm_songchanged, 0);
    return 0;
}

// define plugin interface
static DB_misc_t plugin = {
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "last.fm scrobbler",
    .plugin.descr = "sends played songs information to your last.fm account",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = lastfm_start,
    .plugin.stop = lastfm_stop
};
