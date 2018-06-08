/*
    CURL VFS plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Alexey Yakovenko

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

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <curl/curlver.h>
#include <time.h>
#include "../../deadbeef.h"

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_functions_t *deadbeef;

#define BUFFER_SIZE (0x10000)
#define BUFFER_MASK 0xffff

#define MAX_METADATA 1024

#define TIMEOUT 10 // in seconds

enum {
    STATUS_INITIAL  = 0,
    STATUS_READING  = 1,
    STATUS_FINISHED = 2,
    STATUS_ABORTED  = 3,
    STATUS_SEEK     = 4,
    STATUS_DESTROY  = 5,
};

typedef struct {
    DB_vfs_t *vfs;
    char *url;
    uint8_t buffer[BUFFER_SIZE];

    DB_playItem_t *track;
    int64_t pos; // position in stream; use "& BUFFER_MASK" to make it index into ringbuffer
    int64_t length;
    int32_t remaining; // remaining bytes in buffer read from stream
    int64_t skipbytes;
    intptr_t tid; // thread id which does http requests
    intptr_t mutex;
    uint8_t nheaderpackets;
    char *content_type;
    CURL *curl;
    struct timeval last_read_time;
    uint8_t status;
    int icy_metaint;
    int wait_meta;

    char metadata[MAX_METADATA];
    int metadata_size; // size of metadata in stream
    int metadata_have_size; // amount which is already in metadata buffer

    char http_err[CURL_ERROR_SIZE];

    float prev_playtime;
    time_t started_timestamp;

    // flags (bitfields to save some space)
    unsigned seektoend : 1; // indicates that next tell must return length
    unsigned gotheader : 1; // tells that all headers (including ICY) were processed (to start reading body)
    unsigned icyheader : 1; // tells that we're currently reading ICY headers
    unsigned gotsomeheader : 1; // tells that we got some headers before body started
} HTTP_FILE;

static DB_vfs_t plugin;

static int allow_new_streams;

static size_t
http_content_header_handler (void *ptr, size_t size, size_t nmemb, void *stream);

static int64_t biglock;

#define MAX_ABORT_FILES 100
static DB_FILE *open_files[MAX_ABORT_FILES];
static int num_open_files = 0;
static DB_FILE *abort_files[MAX_ABORT_FILES];
static int num_abort_files = 0;

static int
http_need_abort (DB_FILE *fp);

static void
http_cancel_abort (DB_FILE *fp);

static void
http_abort (DB_FILE *fp);

static void
http_reg_open_file (DB_FILE *fp);

static void
http_unreg_open_file (DB_FILE *fp);

static size_t
http_curl_write_wrapper (HTTP_FILE *fp, void *ptr, size_t size) {
    size_t avail = size;
    while (avail > 0) {
        deadbeef->mutex_lock (fp->mutex);
        if (fp->status == STATUS_SEEK) {
            trace ("vfs_curl seek request, aborting current request\n");
            deadbeef->mutex_unlock (fp->mutex);
            return 0;
        }
        if (http_need_abort ((DB_FILE*)fp)) {
            fp->status = STATUS_ABORTED;
            trace ("vfs_curl STATUS_ABORTED in the middle of packet\n");
            deadbeef->mutex_unlock (fp->mutex);
            break;
        }
        int sz = BUFFER_SIZE/2 - fp->remaining; // number of bytes free in buffer
                                                // don't allow to fill more than half -- used for seeking backwards

        if (sz > 5000) { // wait until there are at least 5k bytes free
            int cp = min (avail, sz);
            int writepos = (fp->pos + fp->remaining) & BUFFER_MASK;
            // copy 1st portion (before end of buffer
            int part1 = BUFFER_SIZE - writepos;
            // may not be more than total
            part1 = min (part1, cp);
            memcpy (fp->buffer+writepos, ptr, part1);
            ptr += part1;
            avail -= part1;
            fp->remaining += part1;
            cp -= part1;
            if (cp > 0) {
                memcpy (fp->buffer, ptr, cp);
                ptr += cp;
                avail -= cp;
                fp->remaining += cp;
            }
        }
        deadbeef->mutex_unlock (fp->mutex);
        usleep (3000);
    }
    return size - avail;
}

void
vfs_curl_set_meta (DB_playItem_t *it, const char *meta, const char *value) {
    const char *cs = deadbeef->junk_detect_charset (value);
    if (cs) {
        char out[1024];
        deadbeef->junk_recode (value, strlen (value), out, sizeof (out), cs);
        deadbeef->pl_replace_meta (it, meta, out);
    }
    else {
        deadbeef->pl_replace_meta (it, meta, value);
    }
    uint32_t f = deadbeef->pl_get_item_flags (it);
    f |= DDB_TAG_ICY;
    deadbeef->pl_set_item_flags (it, f);
    ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
    ev->track = it;
    if (ev->track) {
        deadbeef->pl_item_ref (ev->track);
    }
    deadbeef->event_send ((ddb_event_t *)ev, 0, 0);
}

int
http_parse_shoutcast_meta (HTTP_FILE *fp, const char *meta, int size) {
//    trace ("reading %d bytes of metadata\n", size);
    trace ("%s\n", meta);
    const char *e = meta + size;
    const char strtitle[] = "StreamTitle='";
    char title[256] = "";
    while (meta < e) {
        if (!memcmp (meta, strtitle, sizeof (strtitle)-1)) {
//            trace ("extracting streamtitle\n");
            meta += sizeof (strtitle)-1;
            const char *substr_end = meta;
            while (substr_end < e-1 && (*substr_end != '\'' || *(substr_end+1) != ';')) {
                substr_end++;
            }
            if (substr_end >= e) {
                return -1; // end of string not found
            }
            int s = substr_end - meta;
            s = min (sizeof (title)-1, s);
            memcpy (title, meta, s);
            title[s] = 0;
            trace ("got stream title: %s\n", title);
            if (fp->track) {
                int songstarted = 0;
                char *tit = strstr (title, " - ");
                deadbeef->pl_lock ();
                int emulate_trackchange = 1;
                // create dummy track with previous meta
                DB_playItem_t *from = NULL;
                if (emulate_trackchange) {
                    from = deadbeef->pl_item_alloc ();
                    deadbeef->pl_items_copy_junk (fp->track, from, from);
                }

                if (tit) {
                    *tit = 0;
                    tit += 3;

                    const char *orig_title = deadbeef->pl_find_meta (fp->track, "title");
                    const char *orig_artist = deadbeef->pl_find_meta (fp->track, "artist");

                    if (!orig_title || strcasecmp (orig_title, tit)) {
                        vfs_curl_set_meta (fp->track, "title", tit);
                        songstarted = 1;
                    }
                    if (!orig_artist || strcasecmp (orig_artist, title)) {
                        vfs_curl_set_meta (fp->track, "artist", title);
                        songstarted = 1;
                    }
                }
                else {
                    const char *orig_title = deadbeef->pl_find_meta (fp->track, "title");
                    if (!orig_title || strcasecmp (orig_title, title)) {
                        deadbeef->pl_delete_meta (fp->track, "artist");
                        vfs_curl_set_meta (fp->track, "title", title);
                        songstarted = 1;
                    }
                }
                deadbeef->pl_unlock ();
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
                    deadbeef->plt_modified (plt);
                    deadbeef->plt_unref (plt);
                }
                deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
                if (songstarted) {

                    float playpos = deadbeef->streamer_get_playpos ();
                    if (emulate_trackchange)
                    {
                        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)deadbeef->event_alloc (DB_EV_SONGCHANGED);

                        ev->from = from;
                        ev->to = fp->track;
                        ev->playtime = playpos - fp->prev_playtime;
                        ev->started_timestamp = fp->started_timestamp;
                        deadbeef->pl_item_ref (ev->from);
                        deadbeef->pl_item_ref (ev->to);
                        deadbeef->event_send ((ddb_event_t *)ev, 0, 0);
                    }

                    ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc (DB_EV_SONGSTARTED);
                    ev->track = fp->track;
                    fp->started_timestamp = time(NULL);
                    ev->started_timestamp = fp->started_timestamp;
                    if (ev->track) {
                        deadbeef->pl_item_ref (ev->track);
                    }
                    deadbeef->event_send ((ddb_event_t *)ev, 0, 0);
                    fp->prev_playtime = playpos;
                }
                if (from) {
                    deadbeef->pl_item_unref (from);
                    from = NULL;
                }
            }
            return 0;
        }
        while (meta < e && *meta != ';') {
            meta++;
        }
        if (meta < e) {
            meta++;
        }
    }
    return -1;
}

static void
http_stream_reset (HTTP_FILE *fp) {
    // FIXME: calling this without reopening the entire stream causes metadata counter unsync.
    fp->gotheader = 0;
    fp->icyheader = 0;
    fp->gotsomeheader = 0;
    fp->remaining = 0;
    fp->metadata_size = 0;
    fp->metadata_have_size = 0;
    fp->skipbytes = 0;
    fp->nheaderpackets = 0;
    fp->icy_metaint = 0;
    fp->wait_meta = 0;
}

static size_t
http_curl_write (void *ptr, size_t size, size_t nmemb, void *stream) {
    int avail = size * nmemb;
    HTTP_FILE *fp = (HTTP_FILE *)stream;

//    trace ("http_curl_write %d bytes, wait_meta=%d\n", size * nmemb, fp->wait_meta);
    gettimeofday (&fp->last_read_time, NULL);
    if (http_need_abort (stream)) {
        fp->status = STATUS_ABORTED;
        trace ("vfs_curl STATUS_ABORTED at start of packet\n");
        return 0;
    }
//    if (fp->gotsomeheader) {
//        fp->gotheader = 1;
//    }
    if (!fp->gotheader) {
        // check if that's ICY
        if (!fp->icyheader && avail >= 10 && !memcmp (ptr, "ICY 200 OK", 10)) {
            trace ("icy headers in the stream\n");
            fp->icyheader = 1;
        }
        if (fp->icyheader) {
            if (fp->nheaderpackets > 10) {
                fprintf (stderr, "vfs_curl: warning: seems like stream has unterminated ICY headers\n");
                fp->gotheader = 1;
            }
            else {
//                trace ("parsing icy headers:\n%s\n", ptr);
                fp->nheaderpackets++;
                avail = http_content_header_handler (ptr, size, nmemb, stream);
                if (avail == size * nmemb) {
                    if (fp->gotheader) {
                        fp->gotheader = 0; // don't reset icy header
                    }
                }
                else {
                    fp->gotheader = 1;
                }
            }
        }
        else {
            fp->gotheader = 1;
        }
        if (!avail) {
            return nmemb*size;
        }
    }

    deadbeef->mutex_lock (fp->mutex);
    if (fp->status == STATUS_INITIAL && fp->gotheader) {
        fp->status = STATUS_READING;
    }
    deadbeef->mutex_unlock (fp->mutex);

    while (fp->icy_metaint > 0) {
//            trace ("wait_meta=%d, avail=%d\n", fp->wait_meta, avail);
        if (fp->metadata_size > 0) {
            if (fp->metadata_size > fp->metadata_have_size) {
                trace ("metadata fetch mode, avail: %d, metadata_size: %d, metadata_have_size: %d)\n", avail, fp->metadata_size, fp->metadata_have_size);
                int sz = (fp->metadata_size - fp->metadata_have_size);
                sz = min (sz, avail);
                int space = MAX_METADATA - fp->metadata_have_size;
                int copysize = min (space, sz);
                if (copysize > 0) {
                    trace ("fetching %d bytes of metadata (out of %d)\n", sz, fp->metadata_size);
                    memcpy (fp->metadata + fp->metadata_have_size, ptr, copysize);
                }
                avail -= sz;
                ptr += sz;
                fp->metadata_have_size += sz;
            }
            if (fp->metadata_size == fp->metadata_have_size) {
                int sz = fp->metadata_size;
                fp->metadata_size = fp->metadata_have_size = 0;
                if (http_parse_shoutcast_meta (fp, fp->metadata, sz) < 0) {
                    trace ("vfs_curl: got invalid icy metadata block\n");
                    http_stream_reset (fp);
                    fp->status = STATUS_SEEK;
                    return 0;
                }
            }
        }
        if (fp->wait_meta < avail) {
            // read bytes remaining until metadata block
            size_t res1 = http_curl_write_wrapper (fp, ptr, fp->wait_meta);
            if (res1 != fp->wait_meta) {
                return 0;
            }
            avail -= res1;
            ptr += res1;
            uint32_t sz = (uint32_t)(*((uint8_t *)ptr)) * 16;
            if (sz > 1024) {
                trace ("metadata size %d is too large\n", sz);
            }
            //assert (sz < MAX_METADATA);
            ptr ++;
            fp->metadata_size = sz;
            fp->metadata_have_size = 0;
            fp->wait_meta = fp->icy_metaint;
            avail--;
            if (sz != 0) {
                trace ("found metadata block at pos %lld, size: %d (avail=%d)\n", fp->pos, sz, avail);
            }
        }
        if ((!fp->metadata_size || !avail) && fp->wait_meta >= avail) {
            break;
        }
        if (avail < 0) {
            trace ("vfs_curl: something bad happened in metadata parser. can't continue streaming.\n");
            return 0;
        }
    }

    if (avail) {
//        trace ("http_curl_write_wrapper [2] %d\n", avail);
        size_t res = http_curl_write_wrapper (fp, ptr, avail);
        avail -= res;
        fp->wait_meta -= res;
    }
    return nmemb * size - avail;
}

static const uint8_t *
parse_header (const uint8_t *p, const uint8_t *e, uint8_t *key, int keysize, uint8_t *value, int valuesize) {
    int sz; // will hold lenght of extracted string
    const uint8_t *v; // pointer to current character
    keysize--;
    valuesize--;
    *key = 0;
    *value = 0;
    v = p;
    // find :
    while (v < e && *v != 0x0d && *v != 0x0a && *v != ':') {
        v++;
    }
    if (*v != ':') {
        // skip linebreaks
        while (v < e && (*v == 0x0d || *v == 0x0a)) {
            v++;
        }
        return v;
    }
    // copy key
    sz = v-p;
    sz = min (keysize, sz);
    memcpy (key, p, sz);
    key[sz] = 0;

    // skip whitespace
    v++;
    while (v < e && (*v == 0x20 || *v == 0x08)) {
        v++;
    }
    if (*v == 0x0d || *v == 0x0a) {
        // skip linebreaks
        while (v < e && (*v == 0x0d || *v == 0x0a)) {
            v++;
        }
        return v;
    }
    p = v;

    // find linebreak
    while (v < e && *v != 0x0d && *v != 0x0a) {
        v++;
    }
    
    // copy value
    sz = v-p;
    sz = min (valuesize, sz);
    memcpy (value, p, sz);
    value[sz] = 0;

    return v;
}

static size_t
http_content_header_handler (void *ptr, size_t size, size_t nmemb, void *stream) {
//    trace ("http_content_header_handler\n");
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    const uint8_t *p = ptr;
    const uint8_t *end = p + size*nmemb;
    uint8_t key[256];
    uint8_t value[256];
    int refresh_playlist = 0;

    if (fp->length == 0) {
        fp->length = -1;
    }

    while (p < end) {
        if (p <= end - 4) {
            if (!memcmp (p, "\r\n\r\n", 4)) {
                p += 4;
                return size * nmemb - (size_t)(p-(const uint8_t *)ptr);
            }
        }
        // skip linebreaks
        while (p < end && (*p == 0x0d || *p == 0x0a)) {
            p++;
        }
        p = parse_header (p, end, key, sizeof (key), value, sizeof (value));
        trace ("%skey=%s value=%s\n", fp->icyheader ? "[icy] " : "", key, value);
        if (!strcasecmp (key, "Content-Type")) {
            if (fp->content_type) {
                free (fp->content_type);
            }
            fp->content_type = strdup (value);
        }
        else if (!strcasecmp (key, "Content-Length")) {
            fp->length = atoi (value);
        }
        else if (!strcasecmp (key, "icy-name")) {
            if (fp->track) {
                vfs_curl_set_meta (fp->track, "album", value);
                refresh_playlist = 1;
            }
        }
        else if (!strcasecmp (key, "icy-genre")) {
            if (fp->track) {
                vfs_curl_set_meta (fp->track, "genre", value);
                refresh_playlist = 1;
            }
        }
        else if (!strcasecmp (key, "icy-metaint")) {
            //printf ("icy-metaint: %d\n", atoi (value));
            fp->icy_metaint = atoi (value);
            fp->wait_meta = fp->icy_metaint; 
        }
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    if (refresh_playlist) {
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }
    if (!fp->icyheader) {
        fp->gotsomeheader = 1;
    }
    return size * nmemb;
}

static int
http_curl_control (void *stream, double dltotal, double dlnow, double ultotal, double ulnow) {
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    deadbeef->mutex_lock (fp->mutex);

    struct timeval tm;
    gettimeofday (&tm, NULL);
    float sec = tm.tv_sec - fp->last_read_time.tv_sec;
    long response;
    CURLcode code = curl_easy_getinfo (fp->curl, CURLINFO_RESPONSE_CODE, &response);
    //trace ("http_curl_control: status = %d, response = %d, interval: %f seconds\n", fp ? fp->status : -1, (int)response, sec);
    if (fp->status == STATUS_READING && sec > TIMEOUT) {
        trace ("http_curl_control: timed out, restarting read\n");
        memcpy (&fp->last_read_time, &tm, sizeof (struct timeval));
        http_stream_reset (fp);
        fp->status = STATUS_SEEK;
    }
    else if (fp->status == STATUS_SEEK) {
        trace ("vfs_curl STATUS_SEEK in progress callback\n");
        deadbeef->mutex_unlock (fp->mutex);
        return -1;
    }
    if (http_need_abort ((DB_FILE *)fp)) {
        fp->status = STATUS_ABORTED;
        trace ("vfs_curl STATUS_ABORTED in progress callback\n");
        deadbeef->mutex_unlock (fp->mutex);
        return -1;
    }
    deadbeef->mutex_unlock (fp->mutex);
    return 0;
}

static void
http_destroy (HTTP_FILE *fp) {
    if (fp->content_type) {
        free (fp->content_type);
    }
    if (fp->track) {
        deadbeef->pl_item_unref (fp->track);
    }
    if (fp->url) {
        free (fp->url);
    }
    if (fp->mutex) {
        deadbeef->mutex_free (fp->mutex);
    }
    free (fp);
}

static void
http_thread_func (void *ctx) {
    HTTP_FILE *fp = (HTTP_FILE *)ctx;
    CURL *curl;
    curl = curl_easy_init ();
    fp->length = -1;
    fp->status = STATUS_INITIAL;
    fp->curl = curl;

    int status;

    trace ("vfs_curl: started loading data %s\n", fp->url);
    for (;;) {
        struct curl_slist *headers = NULL;
        curl_easy_reset (curl);
        curl_easy_setopt (curl, CURLOPT_URL, fp->url);
        char ua[100];
        deadbeef->conf_get_str ("network.http_user_agent", "deadbeef", ua, sizeof (ua));
        curl_easy_setopt (curl, CURLOPT_USERAGENT, ua);
        curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, http_curl_write);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, ctx);
        curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, fp->http_err);
        curl_easy_setopt (curl, CURLOPT_BUFFERSIZE, BUFFER_SIZE/2);
        curl_easy_setopt (curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, http_content_header_handler);
        curl_easy_setopt (curl, CURLOPT_HEADERDATA, ctx);
        curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt (curl, CURLOPT_PROGRESSFUNCTION, http_curl_control);
        curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt (curl, CURLOPT_PROGRESSDATA, ctx);
        // enable up to 10 redirects
        curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 10);
        headers = curl_slist_append (headers, "Icy-Metadata:1");
        curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
        if (fp->pos > 0 && fp->length >= 0) {
            curl_easy_setopt (curl, CURLOPT_RESUME_FROM, (long)fp->pos);
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
                curl_easy_setopt (curl, CURLOPT_PROXYPASSWORD, proxypass);
#else
                char pwd[200];
                snprintf (pwd, sizeof (pwd), "%s:%s", proxyuser, proxypass);
                curl_easy_setopt (curl, CURLOPT_PROXYUSERPWD, pwd);
#endif
            }
            deadbeef->conf_unlock ();
        }
//        fp->status = STATUS_INITIAL;
        trace ("vfs_curl: calling curl_easy_perform (status=%d)...\n", fp->status);
        gettimeofday (&fp->last_read_time, NULL);
        status = curl_easy_perform (curl);
        trace ("vfs_curl: curl_easy_perform retval=%d\n", status);
        if (status != 0) {
            trace ("curl error:\n%s\n", fp->http_err);
        }
        deadbeef->mutex_lock (fp->mutex);
        if (fp->status != STATUS_SEEK) {
            trace ("vfs_curl: break loop\n");
            deadbeef->mutex_unlock (fp->mutex);
            break;
        }
        else {
            trace ("vfs_curl: restart loop\n");
            fp->skipbytes = 0;
            fp->status = STATUS_INITIAL;
            trace ("seeking to %lld\n", fp->pos);
            if (fp->length < 0) {
                // icy -- need full restart
                fp->pos = 0;
                if (fp->content_type) {
                    free (fp->content_type);
                    fp->content_type = NULL;
                }
                fp->seektoend = 0;
                fp->gotheader = 0;
                fp->icyheader = 0;
                fp->gotsomeheader = 0;
                fp->wait_meta = 0;
                fp->icy_metaint = 0;
            }
        }
        deadbeef->mutex_unlock (fp->mutex);
        curl_slist_free_all (headers);
    }
    fp->curl = NULL;
    curl_easy_cleanup (curl);

    deadbeef->mutex_lock (fp->mutex);

    if (fp->status == STATUS_ABORTED) {
        trace ("vfs_curl: thread ended due to abort signal\n");
    }
    else {
        trace ("vfs_curl: thread ended normally\n");
        fp->status = STATUS_FINISHED;
    }
    deadbeef->mutex_unlock (fp->mutex);
}

static void
http_start_streamer (HTTP_FILE *fp) {
    fp->mutex = deadbeef->mutex_create ();
    fp->tid = deadbeef->thread_start (http_thread_func, fp);
//    deadbeef->thread_detach (fp->tid);
}

static DB_FILE *
http_open (const char *fname) {
    if (!allow_new_streams) {
        return NULL;
    }
    trace ("http_open\n");
    HTTP_FILE *fp = malloc (sizeof (HTTP_FILE));
    http_reg_open_file ((DB_FILE *)fp);
    memset (fp, 0, sizeof (HTTP_FILE));
    fp->vfs = &plugin;
    fp->url = strdup (fname);
    return (DB_FILE*)fp;
}

static void
http_set_track (DB_FILE *f, DB_playItem_t *it) {
    HTTP_FILE *fp = (HTTP_FILE *)f;
    fp->track = it;
    if (it) {
        deadbeef->pl_item_ref (it);
    }
}

static void
http_close (DB_FILE *stream) {
    trace ("http_close %p\n", stream);
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;

    http_abort (stream);
    if (fp->tid) {
        deadbeef->thread_join (fp->tid);
    }
    http_cancel_abort ((DB_FILE *)fp);
    http_destroy (fp);
    http_unreg_open_file ((DB_FILE *)fp);
    trace ("http_close done\n");
}

static size_t
http_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
//    trace ("http_read %d (status=%d)\n", size*nmemb, fp->status);
    fp->seektoend = 0;
    if (fp->status == STATUS_ABORTED || (fp->status == STATUS_FINISHED && fp->remaining == 0)) {
        errno = ECONNABORTED;
        return 0;
    }
    if (!fp->tid) {
        http_start_streamer (fp);
    }

    size_t sz = size * nmemb;
    while ((fp->remaining > 0 || fp->status != STATUS_FINISHED && fp->status != STATUS_ABORTED) && sz > 0)
    {
        // wait until data is available
        while ((fp->remaining == 0 || fp->skipbytes > 0) && fp->status != STATUS_FINISHED && fp->status != STATUS_ABORTED) {
//            trace ("vfs_curl: readwait, status: %d..\n", fp->status);
            deadbeef->mutex_lock (fp->mutex);
            if (fp->status == STATUS_READING) {
                struct timeval tm;
                gettimeofday (&tm, NULL);
                float sec = tm.tv_sec - fp->last_read_time.tv_sec;
                if (sec > TIMEOUT) {
                    trace ("http_read: timed out, restarting read\n");
                    memcpy (&fp->last_read_time, &tm, sizeof (struct timeval));
                    http_stream_reset (fp);
                    fp->status = STATUS_SEEK;
                    deadbeef->mutex_unlock (fp->mutex);
                    if (fp->track) { // don't touch streamer if the stream is not assosiated with a track
                        deadbeef->streamer_reset (1);
                        continue;
                    }
                    errno = ETIMEDOUT;
                    return 0;
                }
            }
            int skip = min (fp->remaining, fp->skipbytes);
            if (skip > 0) {
//                trace ("skipping %d bytes\n");
                fp->pos += skip;
                fp->remaining -= skip;
                fp->skipbytes -= skip;
            }
            deadbeef->mutex_unlock (fp->mutex);
            usleep (3000);
        }
    //    trace ("buffer remaining: %d\n", fp->remaining);
        deadbeef->mutex_lock (fp->mutex);
        //trace ("http_read %lld/%lld/%d\n", fp->pos, fp->length, fp->remaining);
        int cp = min (sz, fp->remaining);
        int readpos = fp->pos & BUFFER_MASK;
        int part1 = BUFFER_SIZE-readpos;
        part1 = min (part1, cp);
//        trace ("readpos=%d, remaining=%d, req=%d, cp=%d, part1=%d, part2=%d\n", readpos, fp->remaining, sz, cp, part1, cp-part1);
        memcpy (ptr, fp->buffer+readpos, part1);
        fp->remaining -= part1;
        fp->pos += part1;
        sz -= part1;
        ptr += part1;
        cp -= part1;
        if (cp > 0) {
            memcpy (ptr, fp->buffer, cp);
            fp->remaining -= cp;
            fp->pos += cp;
            sz -= cp;
            ptr += cp;
        }
        deadbeef->mutex_unlock (fp->mutex);
    }
    if (fp->status == STATUS_ABORTED) {
        errno = ECONNABORTED;
        return 0;
    }
//    if (size * nmemb == 1) {
//        trace ("%02x\n", (unsigned int)*((uint8_t*)ptr));
//    }
    return (size * nmemb - sz) / size;
}

static int
http_seek (DB_FILE *stream, int64_t offset, int whence) {
    //trace ("http_seek %lld %d\n", offset, whence);
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    fp->seektoend = 0;
    if (whence == SEEK_END) {
        if (offset == 0) {
            fp->seektoend = 1;
            return 0;
        }
        trace ("vfs_curl: can't seek in curl stream relative to EOF\n");
        return -1;
    }
    if (!fp->tid) {
        if (offset == 0 && (whence == SEEK_SET || whence == SEEK_CUR)) {
            return 0;
        }
        else {
            trace ("vfs_curl: cannot do seek(%lld,%d)\n", offset, whence);
            return -1;
        }
    }
    deadbeef->mutex_lock (fp->mutex);
    if (whence == SEEK_CUR) {
        whence = SEEK_SET;
        offset = fp->pos + offset;
    }
    if (whence == SEEK_SET) {
        if (fp->pos == offset) {
            fp->skipbytes = 0;
            deadbeef->mutex_unlock (fp->mutex);
            return 0;
        }
        else if (fp->pos < offset && fp->pos + BUFFER_SIZE > offset) {
            fp->skipbytes = offset - fp->pos;
            deadbeef->mutex_unlock (fp->mutex);
            return 0;
        }
        else if (fp->pos-offset >= 0 && fp->pos-offset <= BUFFER_SIZE-fp->remaining) {
            fp->skipbytes = 0;
            fp->remaining += fp->pos - offset;
            fp->pos = offset;
            deadbeef->mutex_unlock (fp->mutex);
            return 0;
        }
    }
    // reset stream, and start over
    http_stream_reset (fp);
    fp->pos = offset;
    fp->status = STATUS_SEEK;

    deadbeef->mutex_unlock (fp->mutex);
    return 0;
}

static int64_t
http_tell (DB_FILE *stream) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->seektoend) {
        return fp->length;
    }
    return fp->pos + fp->skipbytes;
}

static void
http_rewind (DB_FILE *stream) {
    trace ("http_rewind\n");
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->tid) {
        deadbeef->mutex_lock (fp->mutex);
        fp->status = STATUS_SEEK;
        http_stream_reset (fp);
        fp->pos = 0;
        deadbeef->mutex_unlock (fp->mutex);
    }
}

static int64_t
http_getlength (DB_FILE *stream) {
    trace ("http_getlength\n");
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->status == STATUS_ABORTED) {
        trace ("length: -1\n");
        return -1;
    }
    if (!fp->tid) {
        http_start_streamer (fp);
    }
    while (fp->status == STATUS_INITIAL) {
        usleep (3000);
    }
    trace ("length: %lld\n", fp->length);
    return fp->length;
}

static const char *
http_get_content_type (DB_FILE *stream) {
    trace ("http_get_content_type\n");
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->status == STATUS_ABORTED) {
        return NULL;
    }
    if (fp->gotheader) {
        return fp->content_type;
    }
    if (!fp->tid) {
        http_start_streamer (fp);
    }
    trace ("http_get_content_type waiting for response...\n");
    while (fp->status != STATUS_FINISHED && fp->status != STATUS_ABORTED && !fp->gotheader) {
        usleep (3000);
    }
    return fp->content_type;
}

static void
http_abort (DB_FILE *fp) {
    trace ("abort file: %p\n", fp);
    deadbeef->mutex_lock (biglock);
    int i;
    for (i = 0; i < num_abort_files; i++) {
        if (abort_files[i] == fp) {
            break;
        }
    }
    if (i == num_abort_files) {
        if (num_abort_files == MAX_ABORT_FILES) {
            trace ("vfs_curl: abort_files list overflow\n");
        }
        else {
            trace ("added file to abort list: %p\n", fp);
            abort_files[num_abort_files++] = fp;
        }
    }
    deadbeef->mutex_unlock (biglock);
}

static int
http_need_abort (DB_FILE *fp) {
    deadbeef->mutex_lock (biglock);
    for (int i = 0; i < num_abort_files; i++) {
        if (abort_files[i] == fp) {
            trace ("need to abort: %p\n", fp);
            deadbeef->mutex_unlock (biglock);
            return 1;
        }
    }
    deadbeef->mutex_unlock (biglock);
    return 0;
}

static void
http_cancel_abort (DB_FILE *fp) {
    deadbeef->mutex_lock (biglock);
    for (int i = 0; i < num_abort_files; i++) {
        if (abort_files[i] == fp) {
            if (i != num_abort_files-1) {
                abort_files[i] = abort_files[num_abort_files-1];
            }
            num_abort_files--;
            break;
        }
    }
    deadbeef->mutex_unlock (biglock);
}

static void
http_reg_open_file (DB_FILE *fp) {
    deadbeef->mutex_lock (biglock);
    for (int i = 0; i < num_open_files; i++) {
        if (open_files[i] == fp) {
            deadbeef->mutex_unlock (biglock);
            return;
        }
    }
    if (num_open_files == MAX_ABORT_FILES) {
        fprintf (stderr, "vfs_curl: open files overflow\n");
        deadbeef->mutex_unlock (biglock);
        return;
    }
    open_files[num_open_files++] = fp;
    deadbeef->mutex_unlock (biglock);
}

static void
http_unreg_open_file (DB_FILE *fp) {
    deadbeef->mutex_lock (biglock);
    int i;
    for (i = 0; i < num_open_files; i++) {
        if (open_files[i] == fp) {
            if (i != num_open_files-1) {
                open_files[i] = open_files[num_open_files-1];
            }
            num_open_files--;
            trace ("remove from open list: %p\n", fp);
            break;
        }
    }

    // gc abort_files
    int j = 0;
    while (j < num_abort_files) {
        for (i = 0; i < num_open_files; i++) {
            if (abort_files[j] == open_files[i]) {
                break;
            }
        }
        if (i == num_open_files) {
            // remove abort
            http_cancel_abort (abort_files[j]);
            continue;
        }
        j++;
    }
    deadbeef->mutex_unlock (biglock);
}

static int
vfs_curl_start (void) {
    allow_new_streams = 1;
    biglock = deadbeef->mutex_create ();
    return 0;
}

static int
vfs_curl_stop (void) {
    allow_new_streams = 0;
    if (biglock) {
        deadbeef->mutex_free (biglock);
        biglock = 0;
    }
    return 0;
}

static const char *scheme_names[] = { "http://", "https://", "ftp://", NULL };

const char **
http_get_schemes (void) {
    return scheme_names;
}

int
http_is_streaming (void) {
    return 1;
}

static DB_vfs_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id = "vfs_curl",
    .plugin.name = "cURL vfs",
    .plugin.descr = "http and ftp streaming module using libcurl, with ICY protocol support",
    .plugin.copyright = 
        "CURL VFS plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Alexey Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = vfs_curl_start,
    .plugin.stop = vfs_curl_stop,
    .open = http_open,
    .set_track = http_set_track,
    .close = http_close,
    .abort = http_abort,
    .read = http_read,
    .seek = http_seek,
    .tell = http_tell,
    .rewind = http_rewind,
    .getlength = http_getlength,
    .get_content_type = http_get_content_type,
    .get_schemes = http_get_schemes,
    .is_streaming = http_is_streaming,
};

DB_plugin_t *
vfs_curl_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
