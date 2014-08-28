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
#include "../../deadbeef.h"
#include "artwork_internal.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static int
check_dir(const char *path)
{
    struct stat stat_struct;
    if (!stat(path, &stat_struct)) {
        return S_ISDIR(stat_struct.st_mode);
    }
    if (errno != ENOENT) {
        return 0;
    }

    char* dir = strdup(path);
    if (!dir) {
        return 0;
    }

    const int good_dir = check_dir(dirname(dir));
    free(dir);
    return good_dir && !mkdir(path, 0755);
}

int ensure_dir(const char *path)
{
    char dir[PATH_MAX];
    strcpy(dir, path);
    dirname(dir);
    trace("artwork: ensure folder %s exists\n", dir);
    return check_dir(dir);
}

#define BUFFER_SIZE 4096
int copy_file (const char *in, const char *out)
{
    trace ("copying %s to %s\n", in, out);

    if (!ensure_dir(out)) {
        return -1;
    }

    char tmp_out[PATH_MAX];
    snprintf(tmp_out, PATH_MAX, "%s.part", out);
    FILE *fout = fopen(tmp_out, "w+b");
    if (!fout) {
        trace("artwork: failed to open file %s for writing\n", tmp_out);
        return -1;
    }

    DB_FILE *fin = deadbeef->fopen(in);
    if (!fin) {
        fclose(fout);
        trace("artwork: failed to open file %s for reading\n", in);
        return -1;
    }
    current_file = fin;

    errno = 0;
    int err = 0;
    int bytes_read;
    do {
        char buffer[BUFFER_SIZE];
        bytes_read = deadbeef->fread(buffer, 1, BUFFER_SIZE, fin);
        if (bytes_read < 0 || errno) {
            trace("artwork: failed to read file %s: %s\n", tmp_out, strerror(errno));
            err = -1;
        }
        else if (bytes_read > 0 && fwrite(buffer, bytes_read, 1, fout) != 1) {
            trace("artwork: failed to write file %s: %s\n", tmp_out, strerror(errno));
            err = -1;
        }
    } while (!err && bytes_read == BUFFER_SIZE);

    current_file = NULL;
    deadbeef->fclose(fin);
    fclose(fout);

    if (!err) {
        err = rename(tmp_out, out);
        if (err) {
            trace("artwork: failed to move %s to %s: %s\n", tmp_out, out, strerror(errno));
        }
    }

    unlink(tmp_out);
    return err;
}

int write_file(const char *out, const char *data, const size_t data_length)
{
    if (!ensure_dir(out)) {
        return -1;
    }

    char tmp_path[PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s.part", out);
    FILE *fp = fopen(tmp_path, "w+b");
    if (!fp) {
        trace ("artwork: failed to open %s for writing\n", tmp_path);
        return -1;
    }

    int err = 0;
    if (fwrite(data, 1, data_length, fp) != data_length) {
        trace ("artwork: failed to write picture into %s\n", tmp_path);
        err = -1;
    }

    fclose(fp);

    if (!err) {
        err = rename(tmp_path, out);
        if (err) {
            trace ("Failed to move %s to %s: %s\n", tmp_path, out, strerror(errno));
        }
    }

    unlink(tmp_path);
    return err;
}
