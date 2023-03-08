/*
    ZIP VFS plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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

#include <string.h>
#include <zip.h>
#include <stdlib.h>
#include <assert.h>
#include <deadbeef/deadbeef.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define ENABLE_CACHE 1

#define min(x,y) ((x)<(y)?(x):(y))

static DB_functions_t *deadbeef;
static DB_vfs_t plugin;

#if ENABLE_CACHE
#define ZIP_BUFFER_SIZE 8192
#endif

typedef struct {
    DB_FILE file;
    struct zip* z;
    struct zip_file *zf;
    int64_t offset;
    zip_uint64_t index;
    int64_t size;

#if ENABLE_CACHE
    uint8_t buffer[ZIP_BUFFER_SIZE];
    zip_int64_t buffer_remaining;
    int buffer_pos;
#endif
} ddb_zip_file_t;

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

    struct zip *z = NULL;
    struct zip_stat st;

    const char *colon = fname;

    for (;;) {
        colon = strchr (colon, ':');
        if (!colon) {
            break;
        }

        char zipname[colon-fname+1];
        memcpy (zipname, fname, colon-fname);
        zipname[colon-fname] = 0;

        colon = colon+1;

        z = zip_open (zipname, 0, NULL);
        if (!z) {
            continue;
        }
        memset (&st, 0, sizeof (st));

        while (*colon == '/') {
            colon++;
        }
        int res = zip_stat(z, colon, 0, &st);
        if (res != 0) {
            zip_close (z);
            return NULL;
        }

        break;
    }

    if (!z) {
        return NULL;
    }

    struct zip_file *zf = zip_fopen_index (z, st.index, 0);
    if (!zf) {
        zip_close (z);
        return NULL;
    }

    ddb_zip_file_t *f = malloc (sizeof (ddb_zip_file_t));
    memset (f, 0, sizeof (ddb_zip_file_t));
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
    ddb_zip_file_t *zf = (ddb_zip_file_t *)f;
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
    ddb_zip_file_t *zf = (ddb_zip_file_t *)f;
//    printf ("read: %d\n", size*nmemb);

    size_t sz = size * nmemb;
#if ENABLE_CACHE
    while (sz) {
        if (zf->buffer_remaining == 0) {
            zf->buffer_pos = 0;
            zip_int64_t rb = zip_fread (zf->zf, zf->buffer, ZIP_BUFFER_SIZE);
            if (rb <= 0) {
                break;
            }
            zf->buffer_remaining = rb;
        }
        size_t from_buf = min (sz, zf->buffer_remaining);
        memcpy (ptr, zf->buffer+zf->buffer_pos, from_buf);
        zf->buffer_remaining -= from_buf;
        zf->buffer_pos += from_buf;
        zf->offset += from_buf;
        sz -= from_buf;
        ptr += from_buf;
    }
#else
    rb = zip_fread (zf->zf, ptr, sz);
    sz -= rb;
    zf->offset += rb;
#endif

    return (size * nmemb - sz) / size;
}

int
vfs_zip_seek (DB_FILE *f, int64_t offset, int whence) {
    ddb_zip_file_t *zf = (ddb_zip_file_t *)f;
//    printf ("seek: %lld (%d)\n", offset, whence);

    if (whence == SEEK_CUR) {
        offset = zf->offset + offset;
    }
    else if (whence == SEEK_END) {
        offset = zf->size + offset;
    }

#if ENABLE_CACHE
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
            assert (zf->buffer_pos < ZIP_BUFFER_SIZE);
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

    zf->offset += zf->buffer_remaining;
#endif
    if (offset < zf->offset) {
        // reopen
        zip_fclose (zf->zf);
        zf->zf = zip_fopen_index (zf->z, zf->index, 0);
        if (!zf->zf) {
            return -1;
        }
        zf->offset = 0;
    }
#if ENABLE_CACHE
    zf->buffer_pos = 0;
    zf->buffer_remaining = 0;
#endif
    char buf[4096];
    int64_t n = offset - zf->offset;
    while (n > 0) {
        int64_t sz = min (n, sizeof (buf));
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
    ddb_zip_file_t *zf = (ddb_zip_file_t *)f;
    return zf->offset;
}

void
vfs_zip_rewind (DB_FILE *f) {
    ddb_zip_file_t *zf = (ddb_zip_file_t *)f;
    zip_fclose (zf->zf);
    zf->zf = zip_fopen_index (zf->z, zf->index, 0);
    assert (zf->zf); // FIXME: better error handling?
    zf->offset = 0;
#if ENABLE_CACHE
    zf->buffer_remaining = 0;
#endif
}

int64_t
vfs_zip_getlength (DB_FILE *f) {
    ddb_zip_file_t *zf = (ddb_zip_file_t *)f;
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

    int num_files = 0;
    const int n = zip_get_num_files(z);
    *namelist = malloc(sizeof(void *) * n);
    for (int i = 0; i < n; i++) {
        const char *nm = zip_get_name(z, i, 0);
        struct dirent entry;
        strncpy(entry.d_name, nm, sizeof(entry.d_name)-1);
        entry.d_name[sizeof(entry.d_name)-1] = '\0';
        if (!selector || selector(&entry)) {
            (*namelist)[num_files] = calloc(1, sizeof(struct dirent));
            strcpy((*namelist)[num_files]->d_name, entry.d_name);
            num_files++;
            trace("vfs_zip: %s\n", nm);
        }
    }

    zip_close (z);
    trace ("vfs_zip: scandir done\n");
    return num_files;
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
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.id = "vfs_zip",
    .plugin.name = "ZIP vfs",
    .plugin.descr = "play files directly from zip files",
    .plugin.copyright = 
        "ZIP VFS plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
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
