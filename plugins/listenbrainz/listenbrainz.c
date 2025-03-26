/*
    ListenBrainz plugin for DeaDBeeF Player
    Copyright (C) Kartik Ohri

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
#include <math.h>
#include <unistd.h>
#include <jansson.h>
#include <dispatch/dispatch.h>
#include <deadbeef/deadbeef.h>

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

static DB_misc_t plugin;
static DB_functions_t *deadbeef;

#define LISTENBRAINZ_SUBMISSION_URL "https://api.listenbrainz.org"

static char listenbrainz_pass[100];

static int terminate;
static dispatch_queue_t sync_queue;
static dispatch_queue_t request_queue;

#define META_FIELD_SIZE 200

typedef struct {
    char artist[META_FIELD_SIZE];
    char title[META_FIELD_SIZE];
    char album[META_FIELD_SIZE];
    float duration;
    char tracknum[META_FIELD_SIZE];
    char recording_mbid[META_FIELD_SIZE];
    char track_mbid[META_FIELD_SIZE];
} track_metadata_t;

DB_plugin_t *
listenbrainz_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

#define MAX_REPLY 4096
static char listenbrainz_reply[MAX_REPLY];
static int listenbrainz_reply_sz;
static char listenbrainz_err[CURL_ERROR_SIZE];

#define LISTENBRAINZ_SUBMISSION_QUEUE_SIZE 50

typedef struct {
    DB_playItem_t *it;
    time_t started_timestamp;
    float playtime;
} subm_item_t;

static subm_item_t listenbrainz_subm_queue[LISTENBRAINZ_SUBMISSION_QUEUE_SIZE];

static void
listenbrainz_update_auth (void) {
    deadbeef->conf_lock ();
    const char *pass = deadbeef->conf_get_str_fast ("listenbrainz.usertoken", "");
    if (strcmp (pass, listenbrainz_pass))
        strncpy (listenbrainz_pass, pass, sizeof(listenbrainz_pass) - 1);
        listenbrainz_pass[sizeof(listenbrainz_pass) - 1] = 0;
    deadbeef->conf_unlock ();
}

static size_t
listenbrainz_curl_res (void *ptr, size_t size, size_t nmemb, void *stream)
{
    __block int need_cancel = 0;
    dispatch_sync(sync_queue, ^{
        need_cancel = terminate;
    });
    
    if (need_cancel) {
        trace ("listenbrainz: listenbrainz_curl_res: aborting current request\n");
        return 0;
    }
    
    int len = size * nmemb;
    if (listenbrainz_reply_sz + len >= MAX_REPLY) {
        trace ("reply is too large. stopping.\n");
        return 0;
    }
    memcpy (listenbrainz_reply + listenbrainz_reply_sz, ptr, len);
    listenbrainz_reply_sz += len;
    return len;
}

static int
listenbrainz_curl_control (void *stream, double dltotal, double dlnow, double ultotal, double ulnow) {
    __block int need_cancel = 0;
    dispatch_sync(sync_queue, ^{
        need_cancel = terminate;
    });
    
    if (need_cancel) {
        trace ("listenbrainz: aborting current request\n");
        return -1;
    }
    return 0;
}

static int
listenbrainz_fetch_song_info(DB_playItem_t *song, float playtime, track_metadata_t *meta) {
    if (!meta) {
        return -1;
    }
    
    if (!deadbeef->pl_get_meta(song, "artist", meta->artist, META_FIELD_SIZE)) {
        if (!deadbeef->pl_get_meta(song, "band", meta->artist, META_FIELD_SIZE)) {
            if (!deadbeef->pl_get_meta(song, "album artist", meta->artist, META_FIELD_SIZE)) {
                if (!deadbeef->pl_get_meta(song, "albumartist", meta->artist, META_FIELD_SIZE)) {
                    return -1;
                }
            }
        }
    }
    
    if (!deadbeef->pl_get_meta(song, "title", meta->title, META_FIELD_SIZE)) {
        return -1;
    }
    
    if (!deadbeef->pl_get_meta(song, "album", meta->album, META_FIELD_SIZE)) {
        meta->album[0] = 0;
    }
    
    meta->duration = deadbeef->pl_get_item_duration(song);
    if (meta->duration <= 0) {
        meta->duration = playtime;
    }
    
    if (!deadbeef->pl_get_meta(song, "track", meta->tracknum, META_FIELD_SIZE)) {
        meta->tracknum[0] = 0;
    }
    
    if (!deadbeef->pl_get_meta(song, "musicbrainz_trackid", meta->track_mbid, META_FIELD_SIZE)) {
        meta->track_mbid[0] = 0;
    }
    
    if (!deadbeef->pl_get_meta(song, "musicbrainz_recordingid", meta->recording_mbid, META_FIELD_SIZE)) {
        meta->recording_mbid[0] = 0;
    }
    
    return 0;
}

static int
curl_req_send (const char *url, const char *json_payload, const char *auth_token) {
    trace ("sending request: %s\n", url);
    CURL *curl;
    curl = curl_easy_init ();
    if (!curl) {
        trace ("listenbrainz: failed to init curl\n");
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, listenbrainz_curl_res);
    listenbrainz_reply_sz = 0;
    memset(listenbrainz_err, 0, sizeof(listenbrainz_err));
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, listenbrainz_err);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, listenbrainz_curl_control);
#ifdef __MINGW32__
    curl_easy_setopt (curl, CURLOPT_CAINFO, getenv ("CURL_CA_BUNDLE"));
#endif    
    char ua[100];
    deadbeef->conf_get_str("network.http_user_agent", "deadbeef", ua, sizeof(ua));
    curl_easy_setopt(curl, CURLOPT_USERAGENT, ua);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    
    struct curl_slist *headers = NULL;
    
    if (json_payload) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(json_payload));
    }
    
    if (auth_token && *auth_token) {
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Token %s", auth_token);
        headers = curl_slist_append(headers, auth_header);
    }
    
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    if (deadbeef->conf_get_int("network.proxy", 0)) {
        deadbeef->conf_lock();
        curl_easy_setopt(curl, CURLOPT_PROXY, deadbeef->conf_get_str_fast("network.proxy.address", ""));
        curl_easy_setopt(curl, CURLOPT_PROXYPORT, deadbeef->conf_get_int("network.proxy.port", 8080));
        const char *type = deadbeef->conf_get_str_fast("network.proxy.type", "HTTP");
        int curlproxytype = CURLPROXY_HTTP;
        if (!strcasecmp(type, "HTTP")) {
            curlproxytype = CURLPROXY_HTTP;
        }
#if LIBCURL_VERSION_MINOR >= 19 && LIBCURL_VERSION_PATCH >= 4
        else if (!strcasecmp(type, "HTTP_1_0")) {
            curlproxytype = CURLPROXY_HTTP_1_0;
        }
#endif
#if LIBCURL_VERSION_MINOR >= 15 && LIBCURL_VERSION_PATCH >= 2
        else if (!strcasecmp(type, "SOCKS4")) {
            curlproxytype = CURLPROXY_SOCKS4;
        }
#endif
        else if (!strcasecmp(type, "SOCKS5")) {
            curlproxytype = CURLPROXY_SOCKS5;
        }
#if LIBCURL_VERSION_MINOR >= 18 && LIBCURL_VERSION_PATCH >= 0
        else if (!strcasecmp(type, "SOCKS4A")) {
            curlproxytype = CURLPROXY_SOCKS4A;
        }
        else if (!strcasecmp(type, "SOCKS5_HOSTNAME")) {
            curlproxytype = CURLPROXY_SOCKS5_HOSTNAME;
        }
#endif
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, curlproxytype);

        const char *proxyuser = deadbeef->conf_get_str_fast("network.proxy.username", "");
        const char *proxypass = deadbeef->conf_get_str_fast("network.proxy.password", "");
        if (*proxyuser || *proxypass) {
#if LIBCURL_VERSION_MINOR >= 19 && LIBCURL_VERSION_PATCH >= 1
            curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxyuser);
            curl_easy_setopt(curl, CURLOPT_PROXYUSERNAME, proxypass);
#else
            char pwd[200];
            snprintf(pwd, sizeof(pwd), "%s:%s", proxyuser, proxypass);
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, pwd);
#endif
        }
        deadbeef->conf_unlock();
    }
    
    int status = curl_easy_perform(curl);

    if (!status) {
        listenbrainz_reply[listenbrainz_reply_sz] = 0;
        
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code == 200 || http_code == 201) {
            trace("HTTP request successful, response:\n%s\n", listenbrainz_reply);
            status = 0;
        } else {
            trace("HTTP request failed with HTTP code %ld, response:\n%s\n", http_code, listenbrainz_reply);
            status = http_code;
        }
    } else {
        trace("curl request failed, err:\n%s\n", listenbrainz_err);
    }
    
    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    
    return status;
}

static int
listenbrainz_validate_token(void) {
    listenbrainz_update_auth();

    if (!listenbrainz_pass[0]) {
        trace("listenbrainz: no user token configured\n");
        return -1;
    }
    
    const char *listenbrainz_submission_url = deadbeef->conf_get_str_fast(
        "listenbrainz.submission_url", 
        LISTENBRAINZ_SUBMISSION_URL
    );
    char validate_url[1024];
    snprintf(validate_url, sizeof(validate_url), "%s/1/validate-token", listenbrainz_submission_url);
    
    int status = curl_req_send(validate_url, NULL, listenbrainz_pass);
    
    if (status == 0) {
        trace("listenbrainz: token validation successful\n");
        return 0;
    } else {
        trace("listenbrainz: token validation failed\n");
        return -1;
    }
}

static json_t *
listenbrainz_create_listen_object(const track_metadata_t *meta, time_t timestamp) {
    if (!meta || !meta->artist[0] || !meta->title[0]) {
        trace("listenbrainz: artist or title missing\n");
        return NULL;
    }
    
    json_t *listen = json_object();
    if (!listen) {
        trace("listenbrainz: failed to create listen object\n");
        return NULL;
    }
    
    if (timestamp > 0) {
        json_object_set_new(listen, "listened_at", json_integer((json_int_t)timestamp));
    }
    
    json_t *track_metadata = json_object();
    if (!track_metadata) {
        json_decref(listen);
        trace("listenbrainz: failed to create track_metadata object\n");
        return NULL;
    }
    
    json_object_set_new(track_metadata, "artist_name", json_string(meta->artist));
    json_object_set_new(track_metadata, "track_name", json_string(meta->title));
    
    if (meta->album[0]) {
        json_object_set_new(track_metadata, "release_name", json_string(meta->album));
    }
    
    json_t *additional_info = json_object();
    if (!additional_info) {
        json_decref(track_metadata);
        json_decref(listen);
        trace("listenbrainz: failed to create additional_info object\n");
        return NULL;
    }
    
    json_object_set_new(additional_info, "submission_client", json_string("DeaDBeeF-ListenBrainz-Plugin"));
    json_object_set_new(additional_info, "submission_client_version", json_string("1.0"));
    
    if (meta->tracknum[0]) {
        json_object_set_new(additional_info, "track_number", json_string(meta->tracknum));
    }
    
    if (meta->recording_mbid[0]) {
        json_object_set_new(additional_info, "recording_mbid", json_string(meta->recording_mbid));
    }
    
    if (meta->track_mbid[0]) {
        json_object_set_new(additional_info, "track_mbid", json_string(meta->track_mbid));
    }

    if (meta->duration > 0) {
        json_object_set_new(additional_info, "duration_ms", 
                          json_integer((json_int_t)(meta->duration * 1000)));
    }
    
    json_object_set_new(track_metadata, "additional_info", additional_info);
    json_object_set_new(listen, "track_metadata", track_metadata);
    
    return listen;
}

static int
listenbrainz_format_json_payload(char *out, int outl, const char *listen_type, 
                                DB_playItem_t *single_track, int queue_items) {
    json_t *root = json_object();
    if (!root) {
        trace("listenbrainz: failed to create root JSON object\n");
        return -1;
    }
    
    json_object_set_new(root, "listen_type", json_string(listen_type));
    
    json_t *payload = json_array();
    if (!payload) {
        json_decref(root);
        trace("listenbrainz: failed to create payload array\n");
        return -1;
    }
    json_object_set_new(root, "payload", payload);
    
    int added_listens = 0;
    
    if (single_track) {
        track_metadata_t meta;
        
        if (listenbrainz_fetch_song_info(single_track, 0, &meta) < 0) {
            json_decref(root);
            trace("listenbrainz: failed to fetch song info\n");
            return -1;
        }
        
        json_t *listen = listenbrainz_create_listen_object(&meta, 0);
        
        if (!listen) {
            json_decref(root);
            return -1;
        }
        
        json_array_append_new(payload, listen);
        added_listens = 1;
    }
    else {
        for (int i = 0; i < LISTENBRAINZ_SUBMISSION_QUEUE_SIZE && added_listens < queue_items; i++) {
            if (!listenbrainz_subm_queue[i].it) continue;
            
            track_metadata_t meta;
            
            if (listenbrainz_fetch_song_info(
                    listenbrainz_subm_queue[i].it, 
                    listenbrainz_subm_queue[i].playtime,
                    &meta) < 0) {
                continue;
            }
            
            json_t *listen = listenbrainz_create_listen_object(
                &meta, listenbrainz_subm_queue[i].started_timestamp);
            
            if (!listen) {
                continue;
            }
            
            json_array_append_new(payload, listen);
            added_listens++;
        }
    }
    
    if (added_listens == 0) {
        json_decref(root);
        trace("listenbrainz: no valid tracks to submit\n");
        return -1;
    }
    
    char *json_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    
    if (!json_str) {
        trace("listenbrainz: failed to convert JSON to string\n");
        return -1;
    }
    
    if (strlen(json_str) >= outl) {
        free(json_str);
        trace("listenbrainz: JSON string too large for output buffer\n");
        return -1;
    }
    
    strcpy(out, json_str);
    int length = strlen(json_str);
    free(json_str);
    
    return length;
}

static int
listenbrainz_send_request(const char *listen_type, DB_playItem_t *single_track, int queue_items) {
    __block int need_cancel = 0;
    dispatch_sync(sync_queue, ^{
        need_cancel = terminate;
    });
    
    if (need_cancel) {
        return -1;
    }
    
    listenbrainz_update_auth();
    
    if (!listenbrainz_pass[0]) {
        trace("listenbrainz: no user token configured\n");
        return -1;
    }

    char json_payload[2048 * (single_track ? 10 : 50)];
    int res = listenbrainz_format_json_payload(
        json_payload, sizeof(json_payload), 
        listen_type, 
        single_track, 
        queue_items);
    
    if (res < 0) {
        trace("listenbrainz: failed to format JSON payload for %s\n", listen_type);
        return -1;
    }
    
    trace("%s JSON payload:\n%s\n", 
          strcmp(listen_type, "playing_now") == 0 ? "now playing" : "submission", 
          json_payload);
    
    const char *listenbrainz_submission_url = deadbeef->conf_get_str_fast(
        "listenbrainz.submission_url", 
        LISTENBRAINZ_SUBMISSION_URL);
    
    char submit_url[1024];
    snprintf(submit_url, sizeof(submit_url), "%s/1/submit-listens", listenbrainz_submission_url);
    
    for (int attempts = 2; attempts > 0; attempts--) {
        int status = curl_req_send(submit_url, json_payload, listenbrainz_pass);
        
        if (status == 0) {
            trace("listenbrainz: %s successful\n", 
                  strcmp(listen_type, "playing_now") == 0 ? "now playing submission" : "listen submission");
            
            if (single_track == NULL && queue_items > 0) {
                for (int i = 0; i < LISTENBRAINZ_SUBMISSION_QUEUE_SIZE; i++) {
                    if (listenbrainz_subm_queue[i].it) {
                        deadbeef->pl_item_unref(listenbrainz_subm_queue[i].it);
                        listenbrainz_subm_queue[i].it = NULL;
                        listenbrainz_subm_queue[i].started_timestamp = 0;
                    }
                }
            }
            
            return 0;
        }
        
        dispatch_sync(sync_queue, ^{
            need_cancel = terminate;
        });
        
        if (need_cancel || attempts <= 1) {
            break;
        }
        
        sleep(1);
    }
    
    return -1;
}

static void
listenbrainz_send_submissions(void) {
    trace("listenbrainz_send_submissions\n");
    
    int queued_tracks = 0;
    for (int i = 0; i < LISTENBRAINZ_SUBMISSION_QUEUE_SIZE; i++) {
        if (listenbrainz_subm_queue[i].it) {
            queued_tracks++;
        }
    }
    
    if (!queued_tracks) {
        trace("listenbrainz: no tracks in submission queue\n");
        return;
    }
    
    listenbrainz_send_request("import", NULL, queued_tracks);
}

static void
listenbrainz_send_now_playing(DB_playItem_t *track) {
    trace("listenbrainz_send_now_playing\n");
    
    if (!track) {
        trace("listenbrainz: no track to send now playing\n");
        return;
    }
    
    listenbrainz_send_request("playing_now", track, 0);             
}

static int
listenbrainz_songstarted(ddb_event_track_t *ev, uintptr_t data) {
    trace("listenbrainz songstarted %p\n", ev->track);
    if (!deadbeef->conf_get_int("listenbrainz.enable", 0)) {
        return 0;
    }
    
    if (!ev->track || 
        !deadbeef->pl_meta_exists(ev->track, "artist") || 
        !deadbeef->pl_meta_exists(ev->track, "title")) {
        trace("listenbrainz: not enough metadata for now playing\n");
        return 0;
    }
    
    static time_t last_submitted_timestamp = 0;
    if (last_submitted_timestamp == ev->started_timestamp) {
        trace ("listenbrainz: skipping duplicate track with same timestamp\n");
        return 0;
    }
    last_submitted_timestamp = ev->started_timestamp;

    DB_playItem_t *track = ev->track;
    deadbeef->pl_item_ref(track);
    
    if (!deadbeef->conf_get_int("listenbrainz.disable_np", 0)) {
        dispatch_async(request_queue, ^{
            listenbrainz_send_now_playing(track);
            deadbeef->pl_item_unref(track);
        });
    } else {
        deadbeef->pl_item_unref(track);
    }
    
    return 0;
}

static int
listenbrainz_songchanged (ddb_event_trackchange_t *ev, uintptr_t data) {
    if (!deadbeef->conf_get_int ("listenbrainz.enable", 0)) {
        return 0;
    }
    
    if (!ev->from) {
        return 0;
    }
    trace ("listenbrainz songfinished %s\n", deadbeef->pl_find_meta (ev->from, ":URI"));

    static time_t last_submitted_timestamp = 0;
    if (last_submitted_timestamp == ev->started_timestamp) {
        trace ("listenbrainz: skipping duplicate track with same timestamp\n");
        return 0;
    }
    
    if (!deadbeef->pl_meta_exists (ev->from, "artist")
        || !deadbeef->pl_meta_exists (ev->from, "title")) {
        trace ("listenbrainz: not enough metadata for submission, artist=%s, title=%s, album=%s\n",
               deadbeef->pl_find_meta (ev->from, "artist"), 
               deadbeef->pl_find_meta (ev->from, "title"), 
               deadbeef->pl_find_meta (ev->from, "album"));
        return 0;
    }
        
    ddb_playItem_t *it = ev->from;
    deadbeef->pl_item_ref (it);
    time_t started_timestamp = ev->started_timestamp;
    float playtime = ev->playtime;
    
    last_submitted_timestamp = started_timestamp;

    dispatch_async(request_queue, ^{
        __block int need_cancel = 0;
        dispatch_sync(sync_queue, ^{
            need_cancel = terminate;
        });
        
        if (need_cancel) {
            deadbeef->pl_item_unref (it);
            return;
        }
        
        int added = 0;
        
        for (int i = 0; i < LISTENBRAINZ_SUBMISSION_QUEUE_SIZE && !added; i++) {
            if (!listenbrainz_subm_queue[i].it) {
                trace ("listenbrainz: song is now in queue for submission\n");
                listenbrainz_subm_queue[i].it = it;
                listenbrainz_subm_queue[i].started_timestamp = started_timestamp;
                listenbrainz_subm_queue[i].playtime = playtime;
                added = 1;
            }
        }
        
        if (!added) {
            deadbeef->pl_item_unref (it);
            trace ("listenbrainz: submission queue is full, skipping track\n");
            return;
        }
        
        listenbrainz_send_submissions();
    });
    
    return 0;
}

static int
listenbrainz_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    trace ("listenbrainz_message received: %d\n", id);
    switch (id) {
        case DB_EV_SONGSTARTED:
            listenbrainz_songstarted ((ddb_event_track_t *)ctx, 0);
            break;
        case DB_EV_SONGCHANGED:
            listenbrainz_songchanged ((ddb_event_trackchange_t *)ctx, 0);
            break;
        case DB_EV_CONFIGCHANGED:
            if (deadbeef->conf_get_int ("listenbrainz.trace", 0)) {
                plugin.plugin.flags |= DDB_PLUGIN_FLAG_LOGGING;
            }
            else {
                plugin.plugin.flags &= ~DDB_PLUGIN_FLAG_LOGGING;
            }
            break;
    }
    return 0;
}

static int
listenbrainz_start (void) {
    trace ("listenbrainz_start called\n");
    
    terminate = 0;
    request_queue = dispatch_queue_create("ListenBrainzRequestQueue", NULL);
    sync_queue = dispatch_queue_create("ListenBrainzSyncQueue", NULL);
    
    for (int i = 0; i < LISTENBRAINZ_SUBMISSION_QUEUE_SIZE; i++) {
        listenbrainz_subm_queue[i].it = NULL;
        listenbrainz_subm_queue[i].started_timestamp = 0;
        listenbrainz_subm_queue[i].playtime = 0;
    }
    
    listenbrainz_validate_token();
    
    return 0;
}

static int
listenbrainz_stop (void) {
    trace ("listenbrainz_stop\n");
    
    dispatch_sync(sync_queue, ^{
        terminate = 1;
    });
    
    dispatch_sync(request_queue, ^{
        // Wait for all pending requests to complete
    });
    
    dispatch_release(request_queue);
    dispatch_release(sync_queue);
    
    for (int i = 0; i < LISTENBRAINZ_SUBMISSION_QUEUE_SIZE; i++) {
        if (listenbrainz_subm_queue[i].it) {
            deadbeef->pl_item_unref(listenbrainz_subm_queue[i].it);
            listenbrainz_subm_queue[i].it = NULL;
        }
    }
    
    return 0;
}

static const char settings_dlg[] =
        "property \"Enable submission\" checkbox listenbrainz.enable 0;"
        "property \"Disable now playing\" checkbox listenbrainz.disable_np 0;"
        "property \"User token\" entry listenbrainz.usertoken \"\";"
        "property \"Submission URL\" entry listenbrainz.submission_url \""LISTENBRAINZ_SUBMISSION_URL"\";"
        "property \"Enable logging\" checkbox listenbrainz.trace 0;\n";

static DB_misc_t plugin = {
        DDB_PLUGIN_SET_API_VERSION
        .plugin.version_major = 1,
        .plugin.version_minor = 0,
        .plugin.type = DB_PLUGIN_MISC,
        .plugin.name = "ListenBrainz",
        .plugin.descr = "Sends your listens history to your ListenBrainz account",
        .plugin.copyright =
        "ListenBrainz plugin for DeaDBeeF Player\n"
        "Copyright (C) Kartik Ohri\n"
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
        .plugin.website = "https://deadbeef.sourceforge.io/",
        .plugin.start = listenbrainz_start,
        .plugin.stop = listenbrainz_stop,
        .plugin.configdialog = settings_dlg,
        .plugin.message = listenbrainz_message,
};
