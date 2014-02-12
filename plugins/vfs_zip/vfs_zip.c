/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

static DB_functions_t *deadbeef;
static DB_vfs_t plugin;

#define ZIP_BUFFER_SIZE 8192

typedef struct {
    DB_FILE file;
    struct zip* z;
    struct zip_file *zf;
    int64_t offset;
    int index;
    int64_t size;

    uint8_t buffer[ZIP_BUFFER_SIZE];
    int buffer_remaining;
    int buffer_pos;
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
    trace ("vfs_zip: open %s\n", fname);
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
    trace ("vfs_zip: end open %s\n", fname);
    return (DB_FILE*)f;
}

void
vfs_zip_close (DB_FILE *f) {
    trace ("vfs_zip: close\n");
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
//    printf ("read: %d\n", size*nmemb);

    int sz = size * nmemb;
    while (sz) {
        if (zf->buffer_remaining == 0) {
            zf->buffer_pos = 0;
            int rb = zip_fread (zf->zf, zf->buffer, ZIP_BUFFER_SIZE);
            if (rb <= 0) {
                break;
            }
            zf->buffer_remaining = rb;
        }
        int from_buf = min (sz, zf->buffer_remaining);
        memcpy (ptr, zf->buffer+zf->buffer_pos, from_buf);
        zf->buffer_remaining -= from_buf;
        zf->buffer_pos += from_buf;
        zf->offset += from_buf;
        sz -= from_buf;
        ptr += from_buf;
    }

    return (size * nmemb - sz) / size;
}

int
vfs_zip_seek (DB_FILE *f, int64_t offset, int whence) {
    zip_file_t *zf = (zip_file_t *)f;
//    printf ("seek: %lld (%d)\n", offset, whence);

    if (whence == SEEK_CUR) {
        offset = zf->offset + offset;
    }
    else if (whence == SEEK_END) {
        offset = zf->size + offset;
    }

    int64_t offs = offset - zf->offset;
    if ((offs < 0 && -offs <= zf->buffer_pos) || (offs >= 0 && offs < zf->buffer_remaining)) {
        if (offs != 0) {
            //printf ("cache success\n");

            //printf ("[before] absoffs: %lld, offs: %lld, rem: %d, pos: %d\n", offset, offs, zf->buffer_remaining, zf->buffer_pos);

            // test cases:
            // fail: offs = -3, pos = 0, rem = 100
            // fail: offs = 10, pos = 95, rem = 5
            // succ: offs = -3, pos = 3, rem = 97 ----> pos = 0, rem=100
            // succ: offs = 10, pos = 0, rem = 100 ---> pos = 10, rem = 90

            zf->buffer_pos += offs;
            zf->buffer_remaining -= offs;
            //printf ("[after] offs: %lld, rem: %d, pos: %d\n", offs, zf->buffer_remaining, zf->buffer_pos);
            zf->offset = offset;
            return 0;
        }
        else {
//            printf ("cache double success\n");
            return 0;
        }
    }
//    else {
//        printf ("cache miss: abs_offs: %lld, offs: %lld, rem: %d, pos: %d\n", offset, offs, zf->buffer_remaining, zf->buffer_pos);
//    }

    if (offset < zf->offset) {
        // reopen
        zip_fclose (zf->zf);
        zf->zf = zip_fopen_index (zf->z, zf->index, 0);
        if (!zf->zf) {
            return -1;
        }
        zf->offset = 0;
    }
    zf->buffer_pos = 0;
    zf->buffer_remaining = 0;
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
    zf->buffer_remaining = 0;
}

int64_t
vfs_zip_getlength (DB_FILE *f) {
    zip_file_t *zf = (zip_file_t *)f;
    return zf->size;
}

int
vfs_zip_scandir (const char *dir, struct dirent ***namelist, int (*selector) (const struct dirent *), int (*cmp) (const struct dirent **, const struct dirent **)) {
    trace ("vfs_zip_scandir: %s\n", dir);
    int error;
    struct zip *z = zip_open (dir, 0, &error);
    if (!z) {
        trace ("zip_open failed (code: %d)\n", error);
        return -1;
    }

    int n = zip_get_num_files (z);
    *namelist = malloc (sizeof (void *) * n);
    for (int i = 0; i < n; i++) {
        (*namelist)[i] = malloc (sizeof (struct dirent));
        memset ((*namelist)[i], 0, sizeof (struct dirent));
        const char *nm = zip_get_name (z, i, 0);
        trace ("vfs_zip: %s\n", nm);
        snprintf ((*namelist)[i]->d_name, sizeof ((*namelist)[i]->d_name), "%s", nm);
    }

    zip_close (z);
    trace ("vfs_zip: scandir done\n");
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
const char *
vfs_zip_get_scheme_for_name (const char *fname) {
    return scheme_names[0];
}

static DB_vfs_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 6,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.id = "vfs_zip",
    .plugin.name = "ZIP vfs",
    .plugin.descr = "play files directly from zip files",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .get_scheme_for_name = vfs_zip_get_scheme_for_name,
};

DB_plugin_t *
vfs_zip_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
