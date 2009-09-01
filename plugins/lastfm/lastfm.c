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

#define LFM_TESTMODE 0
#define LFM_IGNORE_RULES 0
#define LFM_NOSEND 0

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

#define LFM_CLIENTID "ddb"
#define SCROBBLER_URL_V1 "http://post.audioscrobbler.com"
#define SCROBBLER_URL_V2 "http://ws.audioscrobbler.com/2.0"
#define LFM_API_KEY "6b33c8ae4d598a9aff8fe63e334e6e86"
#define LFM_API_SECRET "a9f5e17e358377d96e96477d870b2b18"

static char lfm_user[100];
static char lfm_pass[100];

static char lfm_sess[33];
static char lfm_nowplaying_url[256];
static char lfm_submission_url[256];

static uintptr_t lfm_mutex;
static uintptr_t lfm_cond;
static int lfm_stopthread;
static intptr_t lfm_tid;

DB_plugin_t *
lastfm_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

#define MAX_REPLY 4096
static char lfm_reply[MAX_REPLY];
static char lfm_reply_sz;
static char lfm_err[CURL_ERROR_SIZE];

static char lfm_nowplaying[2048]; // packet for nowplaying, or ""
#define LFM_SUBMISSION_QUEUE_SIZE 50
//static char lfm_subm_queue[LFM_SUBMISSION_QUEUE_SIZE][2048];
static DB_playItem_t *lfm_subm_queue[LFM_SUBMISSION_QUEUE_SIZE];

static size_t
lastfm_curl_res (void *ptr, size_t size, size_t nmemb, void *stream)
{
    int len = size * nmemb;
    if (lfm_reply_sz + len >= MAX_REPLY) {
        fprintf (stderr, "reply is too large. stopping.\n");
        return 0;
    }
    memcpy (lfm_reply + lfm_reply_sz, ptr, len);
    lfm_reply_sz += len;
//    char s[size*nmemb+1];
//    memcpy (s, ptr, size*nmemb);
//    s[size*nmemb] = 0;
//    fprintf (stderr, "got from net: %s\n", s);
    return len;
}

static int
curl_req_send (const char *req, const char *post) {
    fprintf (stderr, "sending request: %s\n", req);
    CURL *curl;
    curl = curl_easy_init ();
    if (!curl) {
        fprintf (stderr, "lastfm: failed to init curl\n");
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(curl, CURLOPT_URL, req);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, lastfm_curl_res);
    memset(lfm_err, 0, sizeof(lfm_err));
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, lfm_err);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    if (post) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post));
    }
    int status = curl_easy_perform(curl);
    curl_easy_cleanup (curl);
    if (!status) {
        lfm_reply[lfm_reply_sz] = 0;
    }
    if (status != 0) {
        fprintf (stderr, "curl request failed, err:\n%s\n", lfm_err);
    }
    return status;
}

static void
curl_req_cleanup (void) {
    lfm_reply_sz = 0;
}

static int
auth (void) {
    if (lfm_sess[0]) {
        return 0;
    }
    char req[4096];
    time_t timestamp = time(0);
    uint8_t sig[16];
    char passmd5[33];
    char token[100];
    deadbeef->md5 (sig, lfm_pass, strlen (lfm_pass));
    deadbeef->md5_to_str (passmd5, sig);
    snprintf (token, sizeof (token), "%s%d", passmd5, timestamp);
    deadbeef->md5 (sig, token, strlen (token));
    deadbeef->md5_to_str (token, sig);

#if LFM_TESTMODE
    snprintf (req, sizeof (req), "%s/?hs=true&p=1.2.1&c=tst&v=1.0&u=%s&t=%d&a=%s", SCROBBLER_URL_V1, lfm_user, timestamp, token);
#else
    snprintf (req, sizeof (req), "%s/?hs=true&p=1.2.1&c=%s&v=%d.%d&u=%s&t=%d&a=%s", SCROBBLER_URL_V1, LFM_CLIENTID, plugin.plugin.version_major, plugin.plugin.version_minor, lfm_user, timestamp, token);
#endif
    // handshake
    int status = curl_req_send (req, NULL);
    if (!status) {
        // check status and extract session id, nowplaying url, submission url
        if (strncmp (lfm_reply, "OK", 2)) {
            uint8_t *p = lfm_reply;
            while (*p && *p >= 0x20) {
                p++;
            }
            *p = 0;
            fprintf (stderr, "scrobbler auth failed, response: %s\n", lfm_reply);
            goto fail;
        }
        uint8_t *p = lfm_reply + 2;
        // skip whitespace
        while (*p && *p < 0x20) {
            p++;
        }
        // get session
        if (!*p) {
            fprintf (stderr, "unrecognized scrobbler reply:\n%s\n", lfm_reply);
            goto fail;
        }
        uint8_t *end = p+1;
        while (*end && *end >= 0x20) {
            end++;
        }
        if (end-p > 32) {
            fprintf (stderr, "scrobbler session id is invalid length. probably plugin needs fixing.\n");
            goto fail;
        }
        strncpy (lfm_sess, p, 32);
        lfm_sess[32] = 0;
        fprintf (stderr, "obtained scrobbler session: %s\n", lfm_sess);
        p = end;
        // skip whitespace
        while (*p && *p < 0x20) {
            p++;
        }
        // get nowplaying url
        if (!*p) {
            fprintf (stderr, "unrecognized scrobbler reply:\n%s\n", lfm_reply);
            goto fail;
        }
        end = p+1;
        while (*end && *end >= 0x20) {
            end++;
        }
        if (end - p > sizeof (lfm_nowplaying_url)-1) {
            fprintf (stderr, "scrobbler nowplaying url is too long:\n", lfm_reply);
            goto fail;
        }
        strncpy (lfm_nowplaying_url, p, end-p);
        lfm_nowplaying_url[end-p] = 0;
        fprintf (stderr, "obtained scrobbler nowplaying url: %s\n", lfm_nowplaying_url);
        p = end;
        // skip whitespace
        while (*p && *p < 0x20) {
            p++;
        }
        // get submission url
        if (!*p) {
            fprintf (stderr, "unrecognized scrobbler reply:\n%s\n", lfm_reply);
            goto fail;
        }
        end = p+1;
        while (*end && *end >= 0x20) {
            end++;
        }
        if (end - p > sizeof (lfm_submission_url)-1) {
            fprintf (stderr, "scrobbler submission url is too long:\n", lfm_reply);
            goto fail;
        }
        strncpy (lfm_submission_url, p, end-p);
        lfm_submission_url[end-p] = 0;
        fprintf (stderr, "obtained scrobbler submission url: %s\n", lfm_submission_url);
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
lfm_fetch_song_info (DB_playItem_t *song, const char **a, const char **t, const char **b, float *l, const char **n, const char **m) {
    *a = deadbeef->pl_find_meta (song, "artist");
    if (!*a) {
        return -1;
    }
    *t = deadbeef->pl_find_meta (song, "title");
    if (!*t) {
        return -1;
    }
    *b = deadbeef->pl_find_meta (song, "album");
    if (!*b) {
        *b = "";
    }
    *l = song->duration;
    *n = deadbeef->pl_find_meta (song, "track");
    if (!*n) {
        *n = "";
    }
    *m = deadbeef->pl_find_meta (song, "mbid");
    if (!*m) {
        *m = "";
    }
    return 0;
}

// returns number of encoded chars on success, or -1 in case of error
static int
lfm_uri_encode (char *out, int outl, const char *str) {
    int l = outl;
    static const char echars[] = " ;/?:@=#&";
    //fprintf (stderr, "lfm_uri_encode %p %d %s\n", out, outl, str);
    while (*str) {
        if (outl <= 1) {
            //fprintf (stderr, "no space left for 1 byte in buffer\n");
            return -1;
        }
        if (strchr (echars, *str)) {
            if (outl <= 3) {
                //fprintf (stderr, "no space left for 3 bytes in the buffer\n");
                return -1;
            }
            //fprintf (stderr, "adding escaped value for %c\n", *str);
            snprintf (out, outl, "%%%02x", (int)*str);
            outl -= 3;
            str++;
            out += 3;
        }
        else {
            *out = *str;
            out++;
            str++;
            outl--;
        }
    }
    *out = 0;
    return l - outl;
}

// returns number of encoded chars on success
// or -1 on error
static int
lfm_add_keyvalue_uri_encoded (char **out, int *outl, const char *key, const char *value) {
    int ll = *outl;
    int keyl = strlen (key);
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
lfm_format_uri (int subm, DB_playItem_t *song, char *out, int outl) {
    if (subm > 50) {
        fprintf (stderr, "lastfm: it's only allowed to send up to 50 submissions at once (got idx=%d)\n", subm);
        return -1;
    }
    int sz = outl;
    const char *a; // artist
    const char *t; // title
    const char *b; // album
    float l; // duration
    const char *n; // tracknum
    const char *m; // muzicbrainz id

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

    if (lfm_fetch_song_info (song, &a, &t, &b, &l, &n, &m) == 0) {
        fprintf (stderr, "playtime: %f\nartist: %s\ntitle: %s\nalbum: %s\nduration: %f\ntracknum: %s\n---\n", song->playtime, a, t, b, l, n);
    }
    else {
        fprintf (stderr, "file %s doesn't have enough tags to submit to last.fm\n", song->fname);
        return -1;
    }

    if (lfm_add_keyvalue_uri_encoded (&out, &outl, ka, a) < 0) {
//        fprintf (stderr, "failed to add %s=%s\n", ka, a);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, kt, t) < 0) {
//        fprintf (stderr, "failed to add %s=%s\n", kt, t);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, kb, b) < 0) {
//        fprintf (stderr, "failed to add %s=%s\n", kb, b);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, kn, n) < 0) {
//        fprintf (stderr, "failed to add %s=%s\n", kn, n);
        return -1;
    }
    if (lfm_add_keyvalue_uri_encoded (&out, &outl, km, m) < 0) {
//        fprintf (stderr, "failed to add %s=%s\n", km, m);
        return -1;
    }
    int processed;
    processed = snprintf (out, outl, "%s=%d&", kl, (int)l);
    if (processed > outl) {
//        fprintf (stderr, "failed to add %s=%d\n", kl, (int)l);
        return -1;
    }
    out += processed;
    outl -= processed;
    if (subm >= 0) {
        processed = snprintf (out, outl, "i[%d]=%d&o[%d]=P&r[%d]=&", subm, (int)song->started_timestamp, subm, subm);
        if (processed > outl) {
//            fprintf (stderr, "failed to add i[%d]=%d&o[%d]=P&r[%d]=&\n", subm, (int)song->started_timestamp, subm, subm);
            return -1;
        }
        out += processed;
        outl -= processed;
    }

    return sz - outl;
}

static int
lastfm_songstarted (DB_event_song_t *ev, uintptr_t data) {
    deadbeef->mutex_lock (lfm_mutex);
    if (lfm_format_uri (-1, ev->song, lfm_nowplaying, sizeof (lfm_nowplaying)) < 0) {
        lfm_nowplaying[0] = 0;
    }
//    fprintf (stderr, "%s\n", lfm_nowplaying);
    deadbeef->mutex_unlock (lfm_mutex);
    if (lfm_nowplaying[0]) {
        deadbeef->cond_signal (lfm_cond);
    }

    return 0;
}

static int
lastfm_songfinished (DB_event_song_t *ev, uintptr_t data) {
#if !LFM_IGNORE_RULES
    // check submission rules
    // must be played for >=240sec of half the total time
    if (ev->song->playtime < 240 && ev->song->playtime < ev->song->duration/2) {
        return 0;
    }

    // duration must be >= 30 sec
    if (ev->song->duration < 30) {
        return 0;
    }
#endif

    if (!deadbeef->pl_find_meta (ev->song, "artist")
            || !deadbeef->pl_find_meta (ev->song, "title")
//            || !deadbeef->pl_find_meta (ev->song, "album")
       ) {
        return 0;
    }
    deadbeef->mutex_lock (lfm_mutex);
    // find free place in queue
    for (int i = 0; i < LFM_SUBMISSION_QUEUE_SIZE; i++) {
        if (!lfm_subm_queue[i]) {
            lfm_subm_queue[i] = deadbeef->pl_item_alloc ();
            deadbeef->pl_item_copy (lfm_subm_queue[i], ev->song);
            break;
        }
    }
    deadbeef->mutex_unlock (lfm_mutex);
    deadbeef->cond_signal (lfm_cond);

    return 0;
}

static void
lfm_send_nowplaying (void) {
    if (auth () < 0) {
        fprintf (stderr, "auth failed! nowplaying cancelled.\n");
        lfm_nowplaying[0] = 0;
        return;
    }
    fprintf (stderr, "auth successful! setting nowplaying\n");
    char s[100];
    snprintf (s, sizeof (s), "s=%s&", lfm_sess);
    strcat (lfm_nowplaying, s);
#if !LFM_NOSEND
    int status = curl_req_send (lfm_nowplaying_url, lfm_nowplaying);
    if (!status) {
        if (strncmp (lfm_reply, "OK", 2)) {
            fprintf (stderr, "nowplaying failed, response:\n%s\n", lfm_reply);
            if (!strncmp (lfm_reply, "BADSESSION", 7)) {
                lfm_sess[0] = 0;
            }
        }
        else {
            fprintf (stderr, "nowplaying success! response:\n%s\n", lfm_reply);
        }
    }
    curl_req_cleanup ();
#endif
    lfm_nowplaying[0] = 0;
}

static void
lfm_send_submissions (void) {
    int i;
    char req[1024*50];
    int idx = 0;
    char *r = req;
    int len = sizeof (req);
    int res;
    deadbeef->mutex_lock (lfm_mutex);
    for (i = 0; i < LFM_SUBMISSION_QUEUE_SIZE; i++) {
        if (lfm_subm_queue[i]) {
            res = lfm_format_uri (idx, lfm_subm_queue[i], r, len);
            if (res < 0) {
                return;
            }
            len -= res;
            r += res;
            idx++;
        }
    }
    deadbeef->mutex_unlock (lfm_mutex);
    if (!idx) {
        return;
    }
    if (auth () < 0) {
        return;
    }
    res = snprintf (r, len, "s=%s&", lfm_sess);
    if (res > len) {
        return;
    }
    fprintf (stderr, "submission req string:\n%s\n", req);
#if !LFM_NOSEND
    int status = curl_req_send (lfm_submission_url, req);
    if (!status) {
        if (strncmp (lfm_reply, "OK", 2)) {
            fprintf (stderr, "submission failed, response:\n%s\n", lfm_reply);
            if (!strncmp (lfm_reply, "BADSESSION", 7)) {
                lfm_sess[0] = 0;
            }
        }
        else {
            fprintf (stderr, "submission successful, response:\n%s\n", lfm_reply);
            deadbeef->mutex_lock (lfm_mutex);
            for (i = 0; i < LFM_SUBMISSION_QUEUE_SIZE; i++) {
                if (lfm_subm_queue[i]) {
                    deadbeef->pl_item_free (lfm_subm_queue[i]);
                    lfm_subm_queue[i] = NULL;
                }
            }
            deadbeef->mutex_unlock (lfm_mutex);
        }
    }
    curl_req_cleanup ();
#endif
}

static void
lfm_thread (uintptr_t ctx) {
    //fprintf (stderr, "lfm_thread started\n");
    for (;;) {
        deadbeef->cond_wait (lfm_cond, lfm_mutex);
        fprintf (stderr, "cond signalled!\n");
        if (lfm_stopthread) {
            deadbeef->mutex_unlock (lfm_mutex);
            deadbeef->cond_signal (lfm_cond);
            fprintf (stderr, "lfm_thread end\n");
            return;
        }
        deadbeef->mutex_unlock (lfm_mutex);

        // try to send nowplaying
        if (lfm_nowplaying[0]) {
            lfm_send_nowplaying ();
        }

        lfm_send_submissions ();
    }
}


// {{{ lastfm v2 get session
#if 0
int
auth_v2 (void) {
    if (lfm_sess[0]) {
        return 0;
    }
    char msg[4096];
    char sigstr[4096];
    uint8_t sig[16];
    snprintf (sigstr, sizeof (sigstr), "api_key%smethodauth.getToken%s", LASTFM_API_KEY, LASTFM_API_SECRET);
    deadbeef->md5 (sig, sigstr, strlen (sigstr));
    deadbeef->md5_to_str (sigstr, sig);
    snprintf (msg, sizeof (msg), "%s/?api_key=%s&method=auth.getToken&api_sig=%s", SCROBBLER_URL, LASTFM_API_KEY, sigstr);
    // get token
    char lfm_token[33] = "";
    int status = curl_req_send (msg, NULL);
    if (!status) {
        // parse output
        if (strstr (lfm_reply, "<lfm status=\"ok\">")) {
            char *token = strstr (lfm_reply, "<token>");
            if (token) {
                token += 7;
                char *end = strstr (token, "</token>");
                if (end) {
                    *end = 0;
                    snprintf (msg, sizeof (msg), "http://www.last.fm/api/auth/?api_key=%s&token=%s", LASTFM_API_KEY, token);
                    fprintf (stderr, "Dear user. Please visit this URL and authenticate deadbeef. Thanks.\n");

                    fprintf (stderr, "%s\n", msg);
                    strncpy (lfm_token, token, 32);
                    lfm_token[32] = 0;
                }
            }
        }
    }
    curl_req_cleanup ();
    if (!lfm_token[0]) {
        // total fail, give up
        return -1;
    }
    // get session
    snprintf (sigstr, sizeof (sigstr), "api_key%smethodauth.getSessiontoken%s%s", LASTFM_API_KEY, lfm_token, LASTFM_API_SECRET);
    deadbeef->md5 (sig, sigstr, strlen (sigstr));
    deadbeef->md5_to_str (sigstr, sig);
    snprintf (msg, sizeof (msg), "method=auth.getSession&token=%s&api_key=%s&api_sig=%s", lfm_token, LASTFM_API_KEY, sigstr);
    for (;;) {
        status = curl_req_send (SCROBBLER_URL, msg);
        if (!status) {
            char *sess = strstr (lfm_reply, "<key>");
            if (sess) {
                sess += 5;
                char *end = strstr (sess, "</key>");
                if (end) {
                    *end = 0;
                    char config[1024];
                    snprintf (config, sizeof (config), "%s/.config/deadbeef/lastfmv2", getenv ("HOME"));
                    fprintf (stderr, "got session key %s\n", sess);
                    FILE *fp = fopen (config, "w+b");
                    if (!fp) {
                        fprintf (stderr, "lastfm: failed to write config file %s\n", config);
                        curl_req_cleanup ();
                        return -1;
                    }
                    if (fwrite (sess, 1, 32, fp) != 32) {
                        fclose (fp);
                        fprintf (stderr, "lastfm: failed to write config file %s\n", config);
                        curl_req_cleanup ();
                        return -1;
                    }
                    fclose (fp);
                    strcpy (lfm_sess, sess);
                }
            }
//            fprintf (stderr, "reply: %s\n", lfm_reply);
        }
        curl_req_cleanup ();
        if (lfm_sess[0]) {
            break;
        }
        sleep (5);
    }
    return 0;
}
#endif
// }}}


static int
lastfm_start (void) {
    lfm_stopthread = 0;
    lfm_mutex = deadbeef->mutex_create ();
    lfm_cond = deadbeef->cond_create ();
    lfm_tid = deadbeef->thread_start (lfm_thread, 0);
    // subscribe to frameupdate event
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGSTARTED, DB_CALLBACK (lastfm_songstarted), 0);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_SONGFINISHED, DB_CALLBACK (lastfm_songfinished), 0);
    // load login/pass
    char config[1024];

// {{{ lastfm v2 auth
#if 0
    snprintf (config, 1024, "%s/.config/deadbeef/lastfmv2", getenv ("HOME"));
    FILE *fp = fopen (config, "rt");
    if (fp) {
        if (fread (lfm_sess, 1, 32, fp) != 32) {
            lfm_sess[0] = 0;
        }
        else {
            lfm_sess[32] = 0;
        }
        fclose (fp);
    }
    auth_v2 ();
#endif
// }}}

    snprintf (config, 1024, "%s/lastfm", deadbeef->get_config_dir ());
    FILE *fp = fopen (config, "rt");
    if (!fp) {
        fprintf (stderr, "lastfm: failed open %s\n", config);
        return -1;
    }
    if (!fgets (lfm_user, 50, fp)) {
        fprintf (stderr, "lastfm: failed to read login from %s\n", config);
        fclose (fp);
        return -1;
    }
    if (!fgets (lfm_pass, 50, fp)) {
        fprintf (stderr, "lastfm: failed to read pass from %s\n", config);
        fclose (fp);
        return -1;
    }
    fclose (fp);
    // remove trailing garbage
    int l;
    char *p;
    l = strlen (lfm_user);
    p = lfm_user+l-1;
    while (p >= lfm_user && *p < 0x20) {
        p--;
    }
    p++;
    *p = 0;
    l = strlen (lfm_pass);
    p = lfm_pass+l-1;
    while (p >= lfm_pass && *p < 0x20) {
        p--;
    }
    p++;
    *p = 0;

    return 0;
}

static int
lastfm_stop (void) {
    //fprintf (stderr, "lastfm_stop\n");
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGSTARTED, DB_CALLBACK (lastfm_songstarted), 0);
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_SONGFINISHED, DB_CALLBACK (lastfm_songfinished), 0);
    lfm_stopthread = 1;
    deadbeef->cond_signal (lfm_cond);
    deadbeef->thread_join (lfm_tid);
    lfm_tid = -1;
    deadbeef->cond_free (lfm_cond);
    deadbeef->mutex_free (lfm_mutex);
    return 0;
}

// define plugin interface
static DB_misc_t plugin = {
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_MISC,
    .plugin.name = "last.fm scrobbler",
    .plugin.descr = "sends played songs information to your last.fm account",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = lastfm_start,
    .plugin.stop = lastfm_stop
};
