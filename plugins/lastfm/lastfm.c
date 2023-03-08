/*
    Last.fm scrobbler plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko 

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
#include "../../config.h"
#endif
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <math.h>
#include <deadbeef/deadbeef.h>

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

#define LFM_TESTMODE 0
#define LFM_IGNORE_RULES 0
#define LFM_NOSEND 0

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

#define LFM_CLIENTID "ddb"
#define SCROBBLER_URL_LFM "https://post.audioscrobbler.com"
#define SCROBBLER_URL_LIBRE "https://turtle.libre.fm"

#if defined(__MINGW32__)
#define LOOKUP_URL_FORMAT "cmd /c start https://www.last.fm/music/%s/_/%s"
#elif defined(__APPLE__)
#define LOOKUP_URL_FORMAT "open 'https://www.last.fm/music/%s/_/%s' &"
#else
#define LOOKUP_URL_FORMAT "xdg-open 'https://www.last.fm/music/%s/_/%s' &"
#endif

static char lfm_user[100];
static char lfm_pass[100];

#define SESS_ID_MAX 100

static char lfm_sess[SESS_ID_MAX];
static char lfm_nowplaying_url[256];
static char lfm_submission_url[256];

static int terminate;
static dispatch_queue_t sync_queue;
static dispatch_queue_t request_queue;


#define META_FIELD_SIZE 200

DB_plugin_t *
lastfm_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

#define MAX_REPLY 4096
static uint8_t lfm_reply[MAX_REPLY];
static size_t lfm_reply_sz;
static char lfm_err[CURL_ERROR_SIZE];

static void
lfm_submit (ddb_playItem_t *it, time_t started_timestamp, float playtime);

static void
lfm_send_nowplaying (const char *lfm_nowplaying);

static void
lfm_update_auth (void) {
    deadbeef->conf_lock ();
    const char *user = deadbeef->conf_get_str_fast ("lastfm.login", "");
    const char *pass = deadbeef->conf_get_str_fast ("lastfm.password", "");
    if (strcmp (user, lfm_user) || strcmp (pass, lfm_pass)) {
        strcpy (lfm_user, user);
        strcpy (lfm_pass, pass);
        lfm_sess[0] = 0;
    }
    deadbeef->conf_unlock ();
}

static size_t
lastfm_curl_res (void *ptr, size_t size, size_t nmemb, void *stream)
{
    __block int need_cancel = 0;
    dispatch_sync(sync_queue, ^{
        need_cancel = terminate;
    });
    if (need_cancel) {
        trace ("lfm: lastfm_curl_res: aborting current request\n");
        return 0;
    }
    size_t len = size * nmemb;
    if (lfm_reply_sz + len >= MAX_REPLY) {
        trace ("reply is too large. stopping.\n");
        return 0;
    }
    memcpy (lfm_reply + lfm_reply_sz, ptr, len);
    lfm_reply_sz += len;
    return len;
}

static int
lfm_curl_control (void *stream, double dltotal, double dlnow, double ultotal, double ulnow) {
    __block int need_cancel = 0;
    dispatch_sync(sync_queue, ^{
        need_cancel = terminate;
    });
    if (need_cancel) {
        trace ("lfm: aborting current request\n");
        return -1;
    }
    return 0;
}
static int
curl_req_send (const char *req, const char *post) {
    trace ("sending request: %s\n", req);
    CURL *curl;
    curl = curl_easy_init ();
    if (!curl) {
        trace ("lastfm: failed to init curl\n");
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, req);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, lastfm_curl_res);
    memset(lfm_err, 0, sizeof(lfm_err));
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, lfm_err);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, lfm_curl_control);
    char ua[100];
    deadbeef->conf_get_str ("network.http_user_agent", "deadbeef", ua, sizeof (ua));
    curl_easy_setopt (curl, CURLOPT_USERAGENT, ua);
    curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0);
    if (post) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post));
    }
    if (deadbeef->conf_get_int ("network.proxy", 0)) {
        deadbeef->conf_lock ();
        curl_easy_setopt (curl, CURLOPT_PROXY, deadbeef->conf_get_str_fast ("network.proxy.address", ""));
        curl_easy_setopt (curl, CURLOPT_PROXYPORT, deadbeef->conf_get_int ("network.proxy.port", 8080));
        const char *type = deadbeef->conf_get_str_fast ("network.proxy.type", "HTTP");
        int curlproxytype = CURLPROXY_HTTP;
        if (!strcasecmp (type, "HTTP")) {
            curlproxytype = CURLPROXY_HTTP;
        }
#if LIBCURL_VERSION_MINOR >= 19 && LIBCURL_VERSION_PATCH >= 4
        else if (!strcasecmp (type, "HTTP_1_0")) {
            curlproxytype = CURLPROXY_HTTP_1_0;
        }
#endif
#if LIBCURL_VERSION_MINOR >= 15 && LIBCURL_VERSION_PATCH >= 2
        else if (!strcasecmp (type, "SOCKS4")) {
            curlproxytype = CURLPROXY_SOCKS4;
        }
#endif
        else if (!strcasecmp (type, "SOCKS5")) {
            curlproxytype = CURLPROXY_SOCKS5;
        }
#if LIBCURL_VERSION_MINOR >= 18 && LIBCURL_VERSION_PATCH >= 0
        else if (!strcasecmp (type, "SOCKS4A")) {
            curlproxytype = CURLPROXY_SOCKS4A;
        }
        else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
            curlproxytype = CURLPROXY_SOCKS5_HOSTNAME;
        }
#endif
        curl_easy_setopt (curl, CURLOPT_PROXYTYPE, curlproxytype);

        const char *proxyuser = deadbeef->conf_get_str_fast ("network.proxy.username", "");
        const char *proxypass = deadbeef->conf_get_str_fast ("network.proxy.password", "");
        if (*proxyuser || *proxypass) {
#if LIBCURL_VERSION_MINOR >= 19 && LIBCURL_VERSION_PATCH >= 1
            curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME, proxyuser);
            curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME, proxypass);
#else
            char pwd[200];
            snprintf (pwd, sizeof (pwd), "%s:%s", proxyuser, proxypass);
            curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD, pwd);
#endif
        }
        deadbeef->conf_unlock ();
    }
    int status = curl_easy_perform(curl);
    curl_easy_cleanup (curl);
    if (!status) {
        lfm_reply[lfm_reply_sz] = 0;
    }
    if (status != 0) {
        trace ("curl request failed, err:\n%s\n", lfm_err);
    }
    return status;
}

static void
curl_req_cleanup (void) {
    lfm_reply_sz = 0;
}

static int
auth (void) {
    lfm_update_auth ();
    if (lfm_sess[0]) {
        return 0;
    }
    if (!lfm_user[0] || !lfm_pass[0]) {
        return -1;
    }
    char req[4096];
    time_t timestamp = time(NULL);
    uint8_t sig[16];
    char passmd5[33];
    char token[100];
    deadbeef->md5 (sig, lfm_pass, (int)strlen (lfm_pass));
    deadbeef->md5_to_str (passmd5, sig);
    snprintf (token, sizeof (token), "%s%d", passmd5, (int)timestamp);
    deadbeef->md5 (sig, token, (int)strlen (token));
    deadbeef->md5_to_str (token, sig);

    deadbeef->conf_lock ();
    const char *scrobbler_url = deadbeef->conf_get_str_fast ("lastfm.scrobbler_url", SCROBBLER_URL_LFM);
#if LFM_TESTMODE
    snprintf (req, sizeof (req), "%s/?hs=true&p=1.2.1&c=tst&v=1.0&u=%s&t=%d&a=%s", scrobbler_url, lfm_user, (int)timestamp, token);
#else
    snprintf (req, sizeof (req), "%s/?hs=true&p=1.2.1&c=%s&v=%d.%d&u=%s&t=%d&a=%s", scrobbler_url, LFM_CLIENTID, plugin.plugin.version_major, plugin.plugin.version_minor, lfm_user, (int)timestamp, token);
#endif
    deadbeef->conf_unlock ();
    // handshake
    int status = curl_req_send (req, NULL);
    if (!status) {
        // check status and extract session id, nowplaying url, submission url
        if (strncmp ((char *)lfm_reply, "OK", 2)) {
            uint8_t *p = lfm_reply;
            while (*p && *p >= 0x20) {
                p++;
            }
            *p = 0;
            trace ("scrobbler auth failed, response: %s\n", lfm_reply);
            goto fail;
        }
        uint8_t *p = lfm_reply + 2;
        // skip whitespace
        while (*p && *p < 0x20) {
            p++;
        }
        // get session
        if (!*p) {
            trace ("unrecognized scrobbler reply:\n%s\n", lfm_reply);
            goto fail;
        }
        uint8_t *end = p+1;
        while (*end && *end >= 0x20) {
            end++;
        }
        if (end-p >= SESS_ID_MAX) {
            trace ("scrobbler session id is too large (%d).\n", (int)(end-p));
            goto fail;
        }
        strncpy (lfm_sess, (char *)p, 32);
        lfm_sess[32] = 0;
        trace ("obtained scrobbler session: %s\n", lfm_sess);
        p = end;
        // skip whitespace
        while (*p && *p < 0x20) {
            p++;
        }
        // get nowplaying url
        if (!*p) {
            trace ("unrecognized scrobbler reply:\n%s\n", lfm_reply);
            goto fail;
        }
        end = p+1;
        while (*end && *end >= 0x20) {
            end++;
        }
        if (end - p > sizeof (lfm_nowplaying_url)-1) {
            trace ("scrobbler nowplaying url is too long %d:\n", (int)(end-p));
            goto fail;
        }
        strncpy (lfm_nowplaying_url, (char *)p, end-p);
        lfm_nowplaying_url[end-p] = 0;
        trace ("obtained scrobbler nowplaying url: %s\n", lfm_nowplaying_url);
        p = end;
        // skip whitespace
        while (*p && *p < 0x20) {
            p++;
        }
        // get submission url
        if (!*p) {
            trace ("unrecognized scrobbler reply:\n%s\n", lfm_reply);
            goto fail;
        }
        end = p+1;
        while (*end && *end >= 0x20) {
            end++;
        }
        if (end - p > sizeof (lfm_submission_url)-1) {
            trace ("scrobbler submission url is too long: %d\n", (int)(end-p));
            goto fail;
        }
        strncpy (lfm_submission_url, (char *)p, end-p);
        lfm_submission_url[end-p] = 0;
        trace ("obtained scrobbler submission url: %s\n", lfm_submission_url);
        p = end;
    }
    else {
        // send failed, but that doesn't mean session is invalid
        curl_req_cleanup ();
        return -1;
    }

    curl_req_cleanup ();
    return 0;
fail:
    lfm_sess[0] = 0;
    curl_req_cleanup ();
    return -1;
}

static int
lfm_fetch_song_info (DB_playItem_t *song, float playtime, char *a, char *t, char *b, float *l, char *n, char *m) {
    if (deadbeef->conf_get_int ("lastfm.prefer_album_artist", 0)) {
        if (!deadbeef->pl_get_meta_with_override (song, "band", a, META_FIELD_SIZE)) {
            if (!deadbeef->pl_get_meta_with_override (song, "album artist", a, META_FIELD_SIZE)) {
                if (!deadbeef->pl_get_meta_with_override (song, "albumartist", a, META_FIELD_SIZE)) {
                    if (!deadbeef->pl_get_meta_with_override (song, "artist", a, META_FIELD_SIZE)) {
                        return -1;
                    }
                }
            }
        }
    }
    else {
        if (!deadbeef->pl_get_meta_with_override (song, "artist", a, META_FIELD_SIZE)) {
            if (!deadbeef->pl_get_meta_with_override (song, "band", a, META_FIELD_SIZE)) {
                if (!deadbeef->pl_get_meta_with_override (song, "album artist", a, META_FIELD_SIZE)) {
                    if (!deadbeef->pl_get_meta_with_override (song, "albumartist", a, META_FIELD_SIZE)) {
                        return -1;
                    }
                }
            }
        }
    }
    if (!deadbeef->pl_get_meta_with_override (song, "title", t, META_FIELD_SIZE)) {
        return -1;
    }
    if (!deadbeef->pl_get_meta_with_override (song, "album", b, META_FIELD_SIZE)) {
        *b = 0;
    }
    *l = deadbeef->pl_get_item_duration (song);
    if (*l <= 0) {
        *l = playtime;
    }
    if (*l < 30 && deadbeef->conf_get_int ("lastfm.submit_tiny_tracks", 0)) {
        *l = 30;
    }
    if (!deadbeef->pl_get_meta_with_override (song, "track", n, META_FIELD_SIZE)) {
        *n = 0;
    }
    if (!deadbeef->conf_get_int ("lastfm.mbid", 0) || !deadbeef->pl_get_meta_with_override (song, "musicbrainz_trackid", m, META_FIELD_SIZE)) {
        *m = 0;
    }
    return 0;
}

// returns number of encoded chars on success, or -1 in case of error
static int
lfm_uri_encode (char *out, size_t outl, const char *str) {
    size_t l = outl;
    //trace ("lfm_uri_encode %p %d %s\n", out, outl, str);
    while (*str && *((uint8_t*)str) >= 32) {
        if (outl <= 1) {
            //trace ("no space left for 1 byte in buffer\n");
            return -1;
        }

        if (!(
            (*str >= '0' && *str <= '9') ||
            (*str >= 'a' && *str <= 'z') ||
            (*str >= 'A' && *str <= 'Z') ||
            (*str == ' ')
        ))
        {
            if (outl <= 3) {
                //trace ("no space left for 3 bytes in the buffer\n");
                return -1;
            }
            snprintf (out, outl, "%%%02x", (uint8_t)*str);
            outl -= 3;
            str++;
            out += 3;
        }
        else {
            *out = *str == ' ' ? '+' : *str;
            out++;
            str++;
            outl--;
        }
    }
    *out = 0;
    return (int)(l - outl);
}

// returns number of encoded chars on success
// or -1 on error
static int
lfm_add_keyvalue_uri_encoded (char **out, int *outl, const char *key, const char *value) {
    int ll = *outl;
    size_t keyl = strlen (key);
    if (*outl <= keyl+1) {
        return -1;
    }
    // append key and '=' sign
    memcpy (*out, key, keyl);
    (*out)[keyl] = '=';
    *out += keyl+1;
    *outl -= keyl+1;
    // encode and append value
    int l = lfm_uri_encode (*out, *outl, value);
    if (l < 0) {
        return -1;
    }
    *out += l;
    *outl -= l;
    // append '&'
    if (*outl <= 1) {
        return -1;
    }
    strcpy (*out, "&");
    *out += 1;
    *outl -= 1;
    return ll - *outl;
}

// subm is submission idx, or -1 for nowplaying
// returns number of bytes added, or -1
static int
lfm_format_uri (int subm, DB_playItem_t *song, char *out, int outl, time_t started_timestamp, float playtime) {
    if (subm > 50) {
        trace ("lastfm: it's only allowed to send up to 50 submissions at once (got idx=%d)\n", subm);
        return -1;
    }
    int sz = outl;
    char a[META_FIELD_SIZE]; // artist
    char t[META_FIELD_SIZE]; // title
    char b[META_FIELD_SIZE]; // album
    float l; // duration
    char n[META_FIELD_SIZE]; // tracknum
    char m[META_FIELD_SIZE]; // muzicbrainz id

    char ka[6] = "a";
    char kt[6] = "t";
    char kb[6] = "b";
    char kl[6] = "l";
    char kn[6] = "n";
    char km[6] = "m";

    if (subm >= 0) {
        snprintf (ka+1, 5, "[%d]", subm);
        strcpy (kt+1, ka+1);
        strcpy (kb+1, ka+1);
        strcpy (kl+1, ka+1);
        strcpy (kn+1, ka+1);
        strcpy (km+1, ka+1);
    }

    if (lfm_fetch_song_info (song, playtime, a, t, b, &l, n, m) == 0) {
//        trace ("playtime: %f\nartist: %s\ntitle: %s\nalbum: %s\nduration: %f\ntracknum: %s\n---\n", song->playtime, a, t, b, l, n);
    }
    else {
//        trace ("file %s doesn't have enough tags to submit to last.fm\n", song->fname);
        return -1;
    }

    if (lfm_add_keyvalue_uri_encoded (&out, &outl, ka, a) < 0) {
//        trace ("failed to add %s=%s\n", ka, a);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, kt, t) < 0) {
//        trace ("failed to add %s=%s\n", kt, t);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, kb, b) < 0) {
//        trace ("failed to add %s=%s\n", kb, b);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, kn, n) < 0) {
//        trace ("failed to add %s=%s\n", kn, n);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, km, m) < 0) {
//        trace ("failed to add %s=%s\n", km, m);
        return -1;
    }
    int processed;
    processed = snprintf (out, outl, "%s=%d&", kl, (int)l);
    if (processed > outl) {
//        trace ("failed to add %s=%d\n", kl, (int)l);
        return -1;
    }
    out += processed;
    outl -= processed;
    if (subm >= 0) {
        processed = snprintf (out, outl, "i[%d]=%d&o[%d]=P&r[%d]=&", subm, (int)started_timestamp, subm, subm);
        if (processed > outl) {
//            trace ("failed to add i[%d]=%d&o[%d]=P&r[%d]=&\n", subm, (int)song->started_timestamp, subm, subm);
            return -1;
        }
        out += processed;
        outl -= processed;
    }

    return sz - outl;
}

static int
lastfm_songstarted (ddb_event_track_t *ev, uintptr_t data) {
    trace ("lfm songstarted %p\n", ev->track);
    if (!deadbeef->conf_get_int ("lastfm.enable", 0)) {
        return 0;
    }

    ddb_playItem_t *it = ev->track;
    time_t started_timestamp = ev->started_timestamp;
    deadbeef->pl_item_ref (it);
    dispatch_async(request_queue, ^{
        __block int need_cancel = 0;
        dispatch_sync(sync_queue, ^{
            need_cancel = terminate;
        });
        if (need_cancel) {
            deadbeef->pl_item_unref (it);
            return;
        }
        char lfm_nowplaying[2048];
        if (lfm_format_uri (-1, it, lfm_nowplaying, sizeof (lfm_nowplaying), started_timestamp, 120) > 0) {
            // try to send nowplaying
            if (lfm_nowplaying[0] && !deadbeef->conf_get_int ("lastfm.disable_np", 0)) {
                lfm_send_nowplaying (lfm_nowplaying);
            }
        }
        deadbeef->pl_item_unref (it);
    });
    return 0;
}

static int
lastfm_songchanged (ddb_event_trackchange_t *ev, uintptr_t data) {
    if (!deadbeef->conf_get_int ("lastfm.enable", 0)) {
        return 0;
    }
    // previous track must exist
    if (!ev->from) {
        return 0;
    }
    trace ("lfm songfinished %s\n", deadbeef->pl_find_meta (ev->from, ":URI"));
#if !LFM_IGNORE_RULES
    // check submission rules
    // duration/playtime must be >= 30 sec
    float dur = deadbeef->pl_get_item_duration (ev->from);
    if (dur < 30 && ev->playtime < 30) {
        // the lastfm.send_tiny_tracks option can override this rule
        // only if the track played fully, and has determined duration
        if (!(dur > 0 && fabs (ev->playtime - dur) < 1.f && deadbeef->conf_get_int ("lastfm.submit_tiny_tracks", 0))) {
            trace ("track duration is %f sec, playtime if %f sec. not eligible for submission\n", dur, ev->playtime);
            return 0;
        }
    }
    // must be played for >=240sec or half the total time
    if (ev->playtime < 240 && ev->playtime < dur/2) {
        trace ("track playtime=%f seconds. not eligible for submission\n", ev->playtime);
        return 0;
    }

#endif

    if (!deadbeef->pl_meta_exists_with_override (ev->from, "artist")
            || !deadbeef->pl_meta_exists_with_override (ev->from, "title")
       ) {
        trace ("lfm: not enough metadata for submission, artist=%s, title=%s, album=%s\n", deadbeef->pl_find_meta_with_override (ev->from, "artist"), deadbeef->pl_find_meta_with_override (ev->from, "title"), deadbeef->pl_find_meta_with_override (ev->from, "album"));
        return 0;
    }

    ddb_playItem_t *it = ev->from;
    deadbeef->pl_item_ref (it); // unreffed by lfm_submit
    time_t started_timestamp = ev->started_timestamp;
    float playtime = ev->playtime;

    dispatch_async(request_queue, ^{
        lfm_submit (it, started_timestamp, playtime);
    });

    return 0;
}

static void
lfm_send_nowplaying (const char *nowplaying_uri) {
    if (auth () < 0) {
        trace ("auth failed! nowplaying cancelled.\n");
        return;
    }
    trace ("auth successful! setting nowplaying\n");
    char s[SESS_ID_MAX + 4];
    snprintf (s, sizeof (s), "s=%s&", lfm_sess);

    char lfm_nowplaying[2048] = "";

    snprintf (lfm_nowplaying, sizeof (lfm_nowplaying), "%s%s", nowplaying_uri, s);
    trace ("content:\n%s\n", lfm_nowplaying);
#if !LFM_NOSEND
    for (int attempts = 2; attempts > 0; attempts--) {
        int status = curl_req_send (lfm_nowplaying_url, lfm_nowplaying);
        if (!status) {
            if (strncmp ((char *)lfm_reply, "OK", 2)) {
                trace ("nowplaying failed, response:\n%s\n", lfm_reply);
                if (!strncmp ((char *)lfm_reply, "BADSESSION", 7)) {
                    trace ("got badsession; trying to restore session...\n");
                    lfm_sess[0] = 0;
                    curl_req_cleanup ();
                    if (auth () < 0) {
                        trace ("fail!\n");
                        break; // total fail
                    }
                    trace ("success! retrying send nowplaying...\n");
                    snprintf (s, sizeof (s), "s=%s&", lfm_sess);
                    snprintf (lfm_nowplaying, sizeof (lfm_nowplaying), "%s%s", nowplaying_uri, s);
                    continue; // retry with new session
                }
            }
            else {
                trace ("nowplaying success! response:\n%s\n", lfm_reply);
            }
        }
        curl_req_cleanup ();
        break;
    }
#endif
}

static void
lfm_send_submission (ddb_playItem_t *it, time_t started_timestamp, float playtime) {
    trace ("lfm_send_submission\n");
    char req[1024*50];
    int idx = 0;
    char *r = req;
    int len = sizeof (req);
    int res = lfm_format_uri (idx, it, r, len, started_timestamp, playtime);
    if (res < 0) {
        trace ("lfm: failed to format uri\n");
        deadbeef->pl_item_unref (it);
        return;
    }
    len -= res;
    r += res;
    idx++;
    if (!idx) {
        deadbeef->pl_item_unref (it);
        return;
    }
    if (auth () < 0) {
        deadbeef->pl_item_unref (it);
        return;
    }
    res = snprintf (r, len, "s=%s&", lfm_sess);
    if (res > len) {
        deadbeef->pl_item_unref (it);
        return;
    }
    trace ("submission req string:\n%s\n", req);
#if !LFM_NOSEND
    for (int attempts = 2; attempts > 0; attempts--) {
        int status = curl_req_send (lfm_submission_url, req);
        if (!status) {
            if (strncmp ((char *)lfm_reply, "OK", 2)) {
                trace ("submission failed, response:\n%s\n", lfm_reply);
                if (!strncmp ((char *)lfm_reply, "BADSESSION", 7)) {
                    trace ("got badsession; trying to restore session...\n");
                    lfm_sess[0] = 0;
                    curl_req_cleanup ();
                    if (auth () < 0) {
                        trace ("fail!\n");
                        break; // total fail
                    }
                    trace ("success! retrying send nowplaying...\n");
                    res = snprintf (r, len, "s=%s&", lfm_sess);
                    continue; // retry with new session
                }
            }
        }
        curl_req_cleanup ();
        break;
    }
    deadbeef->pl_item_unref (it);
#else
    trace ("submission successful (NOSEND=1):\n");
    deadbeef->pl_item_unref (it);
#endif
}

static void
lfm_submit (ddb_playItem_t *it, time_t started_timestamp, float playtime) {
    __block int need_cancel = 0;
    dispatch_sync(sync_queue, ^{
        need_cancel = terminate;
    });
    if (need_cancel) {
        deadbeef->pl_item_unref (it);
        return;
    }

    if (!deadbeef->conf_get_int ("lastfm.enable", 0)) {
        deadbeef->pl_item_unref (it);
        return;
    }
    trace ("lfm sending submissions...\n");
    lfm_send_submission (it, started_timestamp, playtime);
}

static int
lfm_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_SONGSTARTED:
        lastfm_songstarted ((ddb_event_track_t *)ctx, 0);
        break;
    case DB_EV_SONGCHANGED:
        lastfm_songchanged ((ddb_event_trackchange_t *)ctx, 0);
        break;
    }
    return 0;
}

static int
lastfm_start (void) {
    terminate = 0;
    request_queue = dispatch_queue_create("LastfmRequestQueue", NULL);
    sync_queue = dispatch_queue_create("LastfmSyncQueue", NULL);

    return 0;
}

static int
lastfm_stop (void) {
    trace ("lastfm_stop\n");
    dispatch_sync(sync_queue, ^{
        terminate = 1; // prevent any new items from being scheduled
    });

    dispatch_sync(request_queue, ^{
    });
    dispatch_release(request_queue);
    dispatch_release(sync_queue);
    return 0;
}

static int
lfm_action_lookup (DB_plugin_action_t *action, ddb_action_context_t ctx) {
    char *command = NULL;
    DB_playItem_t *it = NULL;
    char artist[META_FIELD_SIZE];
    char title[META_FIELD_SIZE];

    ddb_playlist_t *plt = NULL;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        // find first selected
        plt = deadbeef->plt_get_curr ();
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        plt = deadbeef->action_get_playlist();
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        it = deadbeef->streamer_get_playing_track_safe ();
    }

    if (plt) {
        it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
        deadbeef->plt_unref (plt);
    }

    if (!it) {
        goto out;
    }

    if (!deadbeef->pl_get_meta_with_override (it, "artist", artist, sizeof (artist))) {
        goto out;
    }
    if (!deadbeef->pl_get_meta_with_override (it, "title", title, sizeof (title))) {
        goto out;
    }

    size_t la = strlen (artist) * 3 + 1;
    size_t lt = strlen (title) * 3 + 1;
    char *eartist = alloca (la);
    char *etitle = alloca (lt);

    if (-1 == lfm_uri_encode (eartist, la, artist)) {
        goto out;
    }

    if (-1 == lfm_uri_encode (etitle, lt, title)) {
        goto out;
    }

    if (-1 == asprintf (&command, LOOKUP_URL_FORMAT, eartist, etitle)) {
        goto out;
    }

    system (command);
out:
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    if (command) {
        free (command);
    }
    return 0;
}

static DB_plugin_action_t lookup_action = {
    .title = "Lookup on Last.fm",
    .name = "lfm_lookup",
    .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_ADD_MENU,
    .callback2 = lfm_action_lookup,
    .next = NULL
};

static DB_plugin_action_t *
lfm_get_actions (DB_playItem_t *it)
{
    deadbeef->pl_lock ();
    if (!it ||
        !deadbeef->pl_find_meta_with_override (it, "artist") ||
        !deadbeef->pl_find_meta_with_override (it, "title"))
    {
        lookup_action.flags |= DB_ACTION_DISABLED;
    }
    else
    {
        lookup_action.flags &= ~DB_ACTION_DISABLED;
    }
    deadbeef->pl_unlock ();
    return &lookup_action;
}

static const char settings_dlg[] =
    "property \"Enable scrobbler\" checkbox lastfm.enable 0;"
    "property \"Disable nowplaying\" checkbox lastfm.disable_np 0;"
    "property Username entry lastfm.login \"\";\n"
    "property Password password lastfm.password \"\";"
    "property \"Scrobble URL\" entry lastfm.scrobbler_url \""SCROBBLER_URL_LFM"\";"
    "property \"Prefer Album Artist over Artist field\" checkbox lastfm.prefer_album_artist 0;"
    "property \"Send MusicBrainz ID\" checkbox lastfm.mbid 0;"
    "property \"Submit tracks shorter than 30 seconds (not recommended)\" checkbox lastfm.submit_tiny_tracks 0;"
;

// define plugin interface
static DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.name = "last.fm scrobbler",
    .plugin.descr = "Sends played songs information to your last.fm account, or other service that use AudioScrobbler protocol",
    .plugin.copyright =
        "Last.fm scrobbler plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = lastfm_start,
    .plugin.stop = lastfm_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.get_actions = lfm_get_actions,
    .plugin.message = lfm_message,
};
