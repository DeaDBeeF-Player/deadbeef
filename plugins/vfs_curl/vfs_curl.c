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
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "../../deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static DB_functions_t *deadbeef;

#define BUFFER_SIZE (0x10000/2)
#define BUFFER_MASK 0xffff

#define STATUS_INITIAL  0
#define STATUS_READING  1
#define STATUS_FINISHED 2
#define STATUS_ABORTED  3
#define STATUS_REWIND   4

typedef struct {
    DB_vfs_t *vfs;
    // buffer must be 2x size of BUFFER_SIZE, to be able to seek back a bit
    uint8_t buffer[BUFFER_SIZE*2];
    int pos; // position in stream; use "& BUFFER_MASK" to make it index into ringbuffer
    int remaining; // remaining bytes in buffer read from stream
    int status;
    char *url;
    intptr_t tid; // thread id which does http requests
    intptr_t mutex;
} HTTP_FILE;

static DB_vfs_t plugin;

static char http_err[CURL_ERROR_SIZE];

static size_t
lastfm_curl_write (void *ptr, size_t size, size_t nmemb, void *stream) {
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    int avail = size * nmemb;
    for (;;) {
        if (fp->status == STATUS_REWIND) {
            return 0;
        }
        if (fp->status == STATUS_ABORTED) {
            break;
        }
        deadbeef->mutex_lock (fp->mutex);
        int sz = BUFFER_SIZE - fp->remaining;
        int cp = min (avail, 10000);
        if (sz >= cp) {
            cp = min (avail, sz);
            int writepos = (fp->pos + fp->remaining) & BUFFER_MASK;
            // copy data
            int part1 = BUFFER_SIZE * 2 - writepos;
            part1 = min (part1, cp);
            int part2 = sz - part1;
            memcpy (fp->buffer+writepos, ptr, part1);
            ptr += part1;
            avail -= part1;
            fp->remaining += part1;
            if (part2 > 0) {
                memcpy (fp->buffer, ptr, part2);
                ptr += part2;
                avail -= part2;
                fp->remaining += part2;
            }
        }
        deadbeef->mutex_unlock (fp->mutex);
        if (avail >= size*nmemb) {
            break;
        }
        usleep (3000);
    }

    return nmemb * size - avail;
}

static void
http_thread_func (uintptr_t ctx) {
    HTTP_FILE *fp = (HTTP_FILE *)ctx;
    CURL *curl;
    curl = curl_easy_init ();
    for (;;) {
        fp->pos = 0;
        fp->remaining = 0;
        curl_easy_setopt (curl, CURLOPT_URL, fp->url);
        curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, lastfm_curl_write);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, ctx);
        curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, http_err);
        curl_easy_setopt (curl, CURLOPT_BUFFERSIZE, BUFFER_SIZE - fp->remaining);
        curl_easy_setopt (curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        fp->status = STATUS_READING;
        int status = curl_easy_perform (curl);
        trace ("curl: curl_easy_perform status=%d\n", status);
        if (fp->status != STATUS_REWIND) {
            break;
        }
    }
    curl_easy_cleanup (curl);
    fp->status = STATUS_FINISHED;
}

static DB_FILE *
http_open (const char *fname) {
    HTTP_FILE *fp = malloc (sizeof (HTTP_FILE));
    memset (fp, 0, sizeof (HTTP_FILE));
    fp->vfs = &plugin;
    fp->url = strdup (fname);
    return (DB_FILE*)fp;
}

static void
http_close (DB_FILE *stream) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->tid) {
        fp->status = STATUS_ABORTED;
        deadbeef->thread_join (fp->tid);
        deadbeef->mutex_free (fp->mutex);
    }
    free (fp->url);
    free (stream);
}

static size_t
http_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    assert (size * nmemb <= BUFFER_SIZE);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (!fp->tid) {
        fp->mutex = deadbeef->mutex_create ();
        fp->tid = deadbeef->thread_start (http_thread_func, (uintptr_t)fp);
    }
    // wait until data is available
    while (fp->remaining < size * nmemb && fp->status != STATUS_FINISHED) {
        sleep (3000);
    }
    deadbeef->mutex_lock (fp->mutex);
    int sz = size * nmemb;
    int readpos = fp->pos & BUFFER_MASK;
    int part1 = BUFFER_SIZE*2-(fp->pos&BUFFER_MASK);
    part1 = min (part1, sz);
    part1 = min (part1, fp->remaining);
    memcpy (ptr, fp->buffer+readpos, part1);
    fp->remaining -= part1;
    fp->pos += part1;
    sz -= part1;
    if (fp->remaining > 0) {
        int part2 = min (fp->remaining, sz);
        memcpy (((char *)ptr) + part1, fp->buffer, part2);
        fp->remaining -= part2;
        fp->pos += part2;
        sz -= part2;
    }
    deadbeef->mutex_unlock (fp->mutex);
    return size * nmemb - sz;
}

static int
http_seek (DB_FILE *stream, long offset, int whence) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (!fp->tid) {
        if (offset == 0 && (whence == SEEK_SET || whence == SEEK_CUR)) {
            return 0;
        }
        else {
            trace ("vfs_curl: cannot do seek(%d,%d)\n", offset, whence);
            return -1;
        }
    }
    // FIXME: can do some limited seeking
    trace ("vfs_curl: seeking not implemented\n");
    return -1;
}

static long
http_tell (DB_FILE *stream) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    return fp->pos;
}

static void
http_rewind (DB_FILE *stream) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->tid) {
        deadbeef->mutex_lock (fp->mutex);
        fp->status = STATUS_REWIND;
        deadbeef->mutex_unlock (fp->mutex);
    }
}

static const char *scheme_names[] = { "http://", NULL };

// standard stdio vfs
static DB_vfs_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.name = "CURL VFS (streaming over http)",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = http_open,
    .close = http_close,
    .read = http_read,
    .seek = http_seek,
    .tell = http_tell,
    .rewind = http_rewind,
    .scheme_names = scheme_names,
    .streaming = 1
};

DB_plugin_t *
vfs_curl_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
