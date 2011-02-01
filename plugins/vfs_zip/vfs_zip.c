/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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

#include <string.h>
#include <zip.h>
#include <stdlib.h>
#include <assert.h>
#include "../../deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

static DB_functions_t *deadbeef;
static DB_vfs_t plugin;

typedef struct {
    DB_FILE file;
    struct zip* z;
    struct zip_file *zf;
    int64_t offset;
    int index;
    int64_t size;
} zip_file_t;

static const char *scheme_names[] = { "zip://", NULL };

const char **
vfs_zip_get_schemes (void) {
    return scheme_names;
}

int
vfs_zip_is_streaming (void) {
    return 0;
}

// fname must have form of zip://full_filepath.zip:full_filepath_in_zip
DB_FILE*
vfs_zip_open (const char *fname) {
    if (strncasecmp (fname, "zip://", 6)) {
        return NULL;
    }

    fname += 6;
    const char *colon = strchr (fname, ':');
    if (!colon) {
        return NULL;
    }


    char zipname[colon-fname+1];
    memcpy (zipname, fname, colon-fname);
    zipname[colon-fname] = 0;

    fname = colon+1;

    struct zip *z = zip_open (zipname, 0, NULL);
    if (!z) {
        return NULL;
    }
    struct zip_stat st;
    memset (&st, 0, sizeof (st));

    int res = zip_stat(z, fname, 0, &st);
    if (res != 0) {
        zip_close (z);
        return NULL;
    }

    struct zip_file *zf = zip_fopen_index (z, st.index, 0);
    if (!zf) {
        zip_close (z);
        return NULL;
    }

    zip_file_t *f = malloc (sizeof (zip_file_t));
    memset (f, 0, sizeof (zip_file_t));
    f->file.vfs = &plugin;
    f->z = z;
    f->zf = zf;
    f->index = st.index;
    f->size = st.size;
    return (DB_FILE*)f;
}

void
vfs_zip_close (DB_FILE *f) {
    zip_file_t *zf = (zip_file_t *)f;
    if (zf->zf) {
        zip_fclose (zf->zf);
    }
    if (zf->z) {
        zip_close (zf->z);
    }
    free (zf);
}

size_t
vfs_zip_read (void *ptr, size_t size, size_t nmemb, DB_FILE *f) {
    zip_file_t *zf = (zip_file_t *)f;
    ssize_t rb = zip_fread (zf->zf, ptr, size * nmemb);
    zf->offset += rb;
    return rb / size;
}

int
vfs_zip_seek (DB_FILE *f, int64_t offset, int whence) {
    zip_file_t *zf = (zip_file_t *)f;

    if (whence == SEEK_CUR) {
        offset = zf->offset + offset;
    }
    else if (whence == SEEK_END) {
        offset = zf->size + offset;
    }

    if (offset < zf->offset) {
        // reopen
        zip_fclose (zf->zf);
        zf->zf = zip_fopen_index (zf->z, zf->index, 0);
        if (!zf->zf) {
            return -1;
        }
        zf->offset = 0;
    }
    char buf[4096];
    int64_t n = offset - zf->offset;
    while (n > 0) {
        int sz = min (n, sizeof (buf));
        ssize_t rb = zip_fread (zf->zf, buf, sz);
        n -= rb;
        assert (n >= 0);
        zf->offset += rb;
        if (rb != sz) {
            break;
        }
    }
    if (n > 0) {
        return -1;
    }
    return 0;
}

int64_t
vfs_zip_tell (DB_FILE *f) {
    zip_file_t *zf = (zip_file_t *)f;
    return zf->offset;
}

void
vfs_zip_rewind (DB_FILE *f) {
    zip_file_t *zf = (zip_file_t *)f;
    zip_fclose (zf->zf);
    zf->zf = zip_fopen_index (zf->z, zf->index, 0);
    assert (zf->zf); // FIXME: better error handling?
    zf->offset = 0;
}

int64_t
vfs_zip_getlength (DB_FILE *f) {
    zip_file_t *zf = (zip_file_t *)f;
    return zf->size;
}

int
vfs_zip_scandir (const char *dir, struct dirent ***namelist, int (*selector) (const struct dirent *), int (*cmp) (const struct dirent **, const struct dirent **)) {
    struct zip *z = zip_open (dir, ZIP_CHECKCONS, NULL);
    if (!z) {
        return -1;
    }

    int n = zip_get_num_files (z);
    *namelist = malloc (sizeof (void *) * n);
    for (int i = 0; i < n; i++) {
        (*namelist)[i] = malloc (sizeof (struct dirent));
        memset ((*namelist)[i], 0, sizeof (struct dirent));
        const char *nm = zip_get_name (z, i, 0);
        snprintf ((*namelist)[i]->d_name, sizeof ((*namelist)[i]->d_name), "zip://%s:%s", dir, nm);
    }

    zip_close (z);
    return n;
}

int
vfs_zip_is_container (const char *fname) {
    const char *ext = strrchr (fname, '.');
    if (ext && !strcasecmp (ext, ".zip")) {
        return 1;
    }
    return 0;
}

static DB_vfs_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.id = "vfs_curl",
    .plugin.name = "cURL vfs",
    .plugin.descr = "http and ftp streaming module using libcurl, with ICY protocol support",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = vfs_zip_open,
    .close = vfs_zip_close,
    .read = vfs_zip_read,
    .seek = vfs_zip_seek,
    .tell = vfs_zip_tell,
    .rewind = vfs_zip_rewind,
    .getlength = vfs_zip_getlength,
    .get_schemes = vfs_zip_get_schemes,
    .is_streaming = vfs_zip_is_streaming,
    .is_container = vfs_zip_is_container,
    .scandir = vfs_zip_scandir,
};

DB_plugin_t *
vfs_zip_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
