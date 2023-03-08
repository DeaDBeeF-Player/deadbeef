/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifdef HAVE_CONFIG_H
    #include "../../config.h"
#endif
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <deadbeef/deadbeef.h>
#include "artwork_internal.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_functions_t *deadbeef;
extern dispatch_queue_t sync_queue;

#define MAX_REQUESTS FETCH_CONCURRENT_LIMIT
static DB_FILE *_requests[MAX_REQUESTS]; // all current open requests

static DB_FILE *
new_http_request (const char *url) {
    errno = 0;

    DB_FILE *http_request = deadbeef->fopen (url);

    __block int registered = 0;

    dispatch_sync(sync_queue, ^{
        for (int i = 0; i < MAX_REQUESTS; i++) {
            if (_requests[i] == NULL) {
                _requests[i] = http_request;
                registered = 1;
                break;
            }
        }
    });

    if (!registered) {
        deadbeef->fclose (http_request);
        trace ("Too many concurrent HTTP requests");
        return NULL;
    }

    return http_request;
}

// Make sure to call on sync queue
void
artwork_abort_all_http_requests (void) {
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (_requests[i] != NULL) {
            deadbeef->fabort (_requests[i]);
        }
    }
}

static void
close_http_request (DB_FILE *request) {
    dispatch_sync(sync_queue, ^{
        for (int i = 0; i < MAX_REQUESTS; i++) {
            if (_requests[i] == request) {
                _requests[i] = NULL;
                break;
            }
        }
    });
    deadbeef->fclose (request);
}

size_t
artwork_http_request (const char *url, char *buffer, const size_t buffer_size) {
    DB_FILE *request = new_http_request (url);
    if (!request) {
        return 0;
    }

    const size_t size = deadbeef->fread (buffer, 1, buffer_size-1, request);
    buffer[size] = '\0';

    close_http_request (request);

    return size;
}

static int
check_dir (const char *path) {
    struct stat stat_struct;
    if (!stat (path, &stat_struct)) {
        return S_ISDIR (stat_struct.st_mode);
    }
    if (errno != ENOENT) {
        return 0;
    }

    char *dir = strdup (path);
    char *dname = strdup (dirname (dir));
    int good_dir = check_dir (dname);
    free (dir);
    free (dname);
    return good_dir && !mkdir (path, 0755);
}

// check if directory of a supplied file exists,
// attempt to create one if it doesn't,
// return 1 on success.
int
ensure_dir (const char *path) {
    char *dir = strdup (path);
    char *dname = strdup (dirname (dir));
    trace ("artwork: ensure folder %s exists\n", dname);
    int res = check_dir (dname);
    free (dir);
    free (dname);
    return res;
}

#define BUFFER_SIZE 4096
int
copy_file (const char *in, const char *out) {
    trace ("copying %s to %s\n", in, out);

    if (!ensure_dir (out)) {
        return -1;
    }

    char tmp_out[PATH_MAX];
    snprintf (tmp_out, PATH_MAX, "%s.part", out);

    struct stat stat_struct;
    if (!stat (tmp_out, &stat_struct) && S_ISREG (stat_struct.st_mode) && stat_struct.st_size > 0) {
        return 0;
    }

    FILE *fout = fopen (tmp_out, "w+b");
    if (!fout) {
        trace ("artwork: failed to open file %s for writing\n", tmp_out);
        return -1;
    }

    DB_FILE *request = new_http_request (in);
    if (!request) {
        fclose (fout);
        trace ("artwork: failed to open file %s for reading\n", in);
        return -1;
    }

    int err = 0;
    int64_t bytes_read;
    size_t file_bytes = 0;
    do {
        char buffer[BUFFER_SIZE];
        bytes_read = deadbeef->fread (buffer, 1, BUFFER_SIZE, request);
        if (bytes_read > 0 && fwrite (buffer, bytes_read, 1, fout) != 1) {
            trace ("artwork: failed to write file %s: %s\n", tmp_out, strerror (errno));
            err = -1;
        }
        file_bytes += bytes_read;
    } while (!err && bytes_read == BUFFER_SIZE);

    close_http_request (request);
    fclose (fout);

    if (file_bytes > 0 && !err) {
        err = rename (tmp_out, out);
        if (err) {
            trace ("artwork: failed to move %s to %s: %s\n", tmp_out, out, strerror (errno));
        }
    }

    unlink (tmp_out);
    if (!err && file_bytes == 0) {
        return -1;
    }
    return err;
}

int
write_file (const char *out, const char *data, const size_t data_length) {
    if (!ensure_dir (out)) {
        return -1;
    }

    char tmp_path[PATH_MAX];
    snprintf (tmp_path, sizeof (tmp_path), "%s.part", out);
    FILE *fp = fopen (tmp_path, "w+b");
    if (!fp) {
        trace ("artwork: failed to open %s for writing\n", tmp_path);
        return -1;
    }

    int err = 0;
    if (fwrite (data, 1, data_length, fp) != data_length) {
        trace ("artwork: failed to write picture into %s\n", tmp_path);
        err = -1;
    }

    fclose (fp);

    if (!err) {
        err = rename (tmp_path, out);
        if (err) {
            trace ("Failed to move %s to %s: %s\n", tmp_path, out, strerror (errno));
        }
    }

    unlink (tmp_path);
    return err;
}
