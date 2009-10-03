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

#define BUFFER_SIZE (0x10000)
#define BUFFER_MASK 0xffff

#define STATUS_INITIAL  0
#define STATUS_STARTING 1
#define STATUS_READING  2
#define STATUS_FINISHED 3
#define STATUS_ABORTED  4
#define STATUS_SEEK     5

typedef struct {
    DB_vfs_t *vfs;
    char *url;
    uint8_t buffer[BUFFER_SIZE];
    int64_t pos; // position in stream; use "& BUFFER_MASK" to make it index into ringbuffer
    int64_t length;
    int32_t remaining; // remaining bytes in buffer read from stream
    int32_t skipbytes;
    intptr_t tid; // thread id which does http requests
    intptr_t mutex;
    uint8_t status;
} HTTP_FILE;

static DB_vfs_t plugin;

static char http_err[CURL_ERROR_SIZE];


static size_t
http_curl_write (void *ptr, size_t size, size_t nmemb, void *stream) {
//    trace ("http_curl_write %d bytes\n", size * nmemb);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    int avail = size * nmemb;
    while (avail > 0) {
        deadbeef->mutex_lock (fp->mutex);
        if (fp->status == STATUS_SEEK) {
            trace ("vfs_curl seek request, aborting current request\n");
            deadbeef->mutex_unlock (fp->mutex);
            return 0;
        }
        if (fp->status == STATUS_ABORTED) {
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
            //trace ("part1=%d\n", part1);
//            trace ("writepos=%d, remaining=%d, avail=%d, free=%d, writing=%d, part1=%d, part2=%d\n", writepos, fp->remaining, avail, sz, cp, part1, cp-part1);
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

//    trace ("returning %d\n", nmemb * size - avail);
    return nmemb * size - avail;
}

static size_t
http_size_request_handler (void *ptr, size_t size, size_t nmemb, void *stream) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    // don't copy/allocate mem, just grep for "Content-Length: "
    const char *cl = strstr (ptr, "Content-Length: ");
    if (cl) {
        cl += 16;
        fp->length = atoi (cl);
        trace ("vfs_curl: file size is %d bytes\n", fp->length);
    }
//    else {
//        trace ("vfs_curl: unable to get file size\n");
//        fp->length = -1; // infinite
//    }
    return size * nmemb;
}

static void
http_thread_func (uintptr_t ctx) {
    HTTP_FILE *fp = (HTTP_FILE *)ctx;
    CURL *curl;
    curl = curl_easy_init ();
    fp->length = -1;
    fp->status = STATUS_INITIAL;

    // get filesize (once)
    int status;
    curl_easy_setopt(curl, CURLOPT_URL, fp->url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, http_size_request_handler);
    curl_easy_setopt (curl, CURLOPT_HEADERDATA, ctx);
    status = curl_easy_perform(curl);
    if (status != 0) {
        trace ("vfs_curl: curl_easy_perform failed while getting filesize, status %d\n", status);
        fp->status = STATUS_FINISHED;
        fp->tid = 0;
        return;
    }

    fp->status = STATUS_STARTING;

    for (;;) {
        curl_easy_reset (curl);
        curl_easy_setopt (curl, CURLOPT_URL, fp->url);
        curl_easy_setopt (curl, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, http_curl_write);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, ctx);
        curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, http_err);
        curl_easy_setopt (curl, CURLOPT_BUFFERSIZE, BUFFER_SIZE/2);
        curl_easy_setopt (curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
        // enable up to 10 redirects
        curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 10);
        if (fp->pos > 0) {
            curl_easy_setopt (curl, CURLOPT_RESUME_FROM, fp->pos);
        }
        deadbeef->mutex_lock (fp->mutex);
        fp->status = STATUS_READING;
        deadbeef->mutex_unlock (fp->mutex);
        status = curl_easy_perform (curl);
        trace ("vfs_curl: curl_easy_perform status=%d\n", status);
        if (status != 0) {
            trace ("curl error:\n%s\n", http_err);
        }
        deadbeef->mutex_lock (fp->mutex);
        if (fp->status != STATUS_SEEK) {
            deadbeef->mutex_unlock (fp->mutex);
            break;
        }
        fp->status = STATUS_INITIAL;
        trace ("seeking to %d\n", fp->pos);
        deadbeef->mutex_unlock (fp->mutex);
    }
    curl_easy_cleanup (curl);

    deadbeef->mutex_lock (fp->mutex);
    fp->status = STATUS_FINISHED;
    deadbeef->mutex_unlock (fp->mutex);
    fp->tid = 0;
}

static void
http_start_streamer (HTTP_FILE *fp) {
    fp->mutex = deadbeef->mutex_create ();
    fp->tid = deadbeef->thread_start (http_thread_func, (uintptr_t)fp);
}

static DB_FILE *
http_open (const char *fname) {
    trace ("http_open\n");
    HTTP_FILE *fp = malloc (sizeof (HTTP_FILE));
    memset (fp, 0, sizeof (HTTP_FILE));
    fp->vfs = &plugin;
    fp->url = strdup (fname);
    return (DB_FILE*)fp;
}

static void
http_close (DB_FILE *stream) {
    trace ("http_close\n");
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (fp->tid) {
        deadbeef->mutex_lock (fp->mutex);
        fp->status = STATUS_ABORTED;
        deadbeef->mutex_unlock (fp->mutex);
        deadbeef->thread_join (fp->tid);
        deadbeef->mutex_free (fp->mutex);
    }
    free (fp->url);
    free (stream);
}

static size_t
http_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
//    trace ("http_read %d\n", size*nmemb);
    assert (stream);
    assert (ptr);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    int sz = size * nmemb;
//    assert (size * nmemb <= BUFFER_SIZE);
//    trace ("readpos=%d, readsize=%d\n", fp->pos & BUFFER_SIZE, sz);
    if (!fp->tid) {
        http_start_streamer (fp);
    }
    while (fp->status != STATUS_FINISHED && sz > 0)
    {
        // wait until data is available
        while ((fp->remaining == 0 || fp->skipbytes > 0) && fp->status != STATUS_FINISHED) {
            deadbeef->mutex_lock (fp->mutex);
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
//    if (size * nmemb == 1) {
//        trace ("%02x\n", (unsigned int)*((uint8_t*)ptr));
//    }
    return size * nmemb - sz;
}

static int
http_seek (DB_FILE *stream, int64_t offset, int whence) {
    trace ("http_seek %x %d\n", offset, whence);
    if (whence == SEEK_END) {
        trace ("vfs_curl: can't seek in curl stream relative to EOF\n");
        return -1;
    }
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
    fp->skipbytes = 0;
    fp->remaining = 0;
    fp->pos = offset;
    fp->status = STATUS_SEEK;

    deadbeef->mutex_unlock (fp->mutex);
    return 0;
}

static int64_t
http_tell (DB_FILE *stream) {
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
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
        fp->remaining = 0;
        fp->pos = 0;
        deadbeef->mutex_unlock (fp->mutex);
    }
}

static int64_t
http_getlength (DB_FILE *stream) {
    trace ("http_getlength\n");
    assert (stream);
    HTTP_FILE *fp = (HTTP_FILE *)stream;
    if (!fp->tid) {
        http_start_streamer (fp);
    }
    while (fp->status == STATUS_INITIAL) {
        sleep (3000);
    }
    return fp->length;
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
    .getlength = http_getlength,
    .scheme_names = scheme_names,
    .streaming = 1
};

DB_plugin_t *
vfs_curl_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
