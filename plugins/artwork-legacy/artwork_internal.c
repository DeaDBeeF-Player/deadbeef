/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include "../../deadbeef.h"
#include "artwork_internal.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(...)

extern DB_functions_t *deadbeef;

static DB_FILE *http_request;
static uintptr_t http_mutex;

static DB_FILE *
new_http_request (const char *url)
{
    errno = 0;

    if (!http_mutex) {
        http_mutex = deadbeef->mutex_create_nonrecursive ();
        if (!http_mutex) {
            return NULL;
        }
    }

    deadbeef->mutex_lock (http_mutex);
    http_request = deadbeef->fopen (url);
    deadbeef->mutex_unlock (http_mutex);
    return http_request;
}

static void
close_http_request (DB_FILE *request)
{
    deadbeef->mutex_lock (http_mutex);
    deadbeef->fclose (request);
    http_request = NULL;
    deadbeef->mutex_unlock (http_mutex);
}

size_t artwork_http_request (const char *url, char *buffer, const size_t buffer_size)
{
    DB_FILE *request = new_http_request (url);
    if (!request) {
        return 0;
    }

    const size_t size = deadbeef->fread (buffer, 1, buffer_size-1, request);
    buffer[size] = '\0';

    close_http_request (request);

    return size;
}

void artwork_abort_http_request (void)
{
    if (http_mutex) {
        deadbeef->mutex_lock (http_mutex);
        if (http_request) {
            deadbeef->fabort (http_request);
        }
        http_request = NULL;
        deadbeef->mutex_unlock (http_mutex);
    }
}

static int
check_dir (const char *path)
{
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

    if (file_bytes == 0) { // curl can fail silently
        trace ("artwork: failed to read %s (errno = %d)\n", in, errno);
        err = -1;
    }
    else if (!err) {
        err = rename (tmp_out, out);
        if (err) {
            trace ("artwork: failed to move %s to %s: %s\n", tmp_out, out, strerror (errno));
        }
    }

    unlink (tmp_out);
    return err;
}

int write_file (const char *out, const char *data, const size_t data_length)
{
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
