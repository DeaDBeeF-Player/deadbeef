/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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
#    include "../../config.h"
#endif
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <dispatch/dispatch.h>
#include <errno.h>
#include <fnmatch.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if HAVE_SYS_CDEFS_H
#    include <sys/cdefs.h>
#endif
#ifdef __linux__
#    include <sys/prctl.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#ifdef _WIN32
#    include <sys/utime.h>
#endif
#include <unistd.h>

#include <deadbeef/deadbeef.h>
#include "artwork_flac.h"
#ifdef USE_OGG
#    include "artwork_ogg.h"
#endif
#ifdef USE_OPUS
#    include "artwork_opus.h"
#endif
#include "albumartorg.h"
#include "artwork.h"
#include "artwork_internal.h"
#include "cache.h"
#include "coverinfo.h"
#include "lastfm.h"
#include "musicbrainz.h"
#include "mp4tagutil.h"
#include <deadbeef/strdupa.h>
#include "wos.h"

//#define DEBUG_COUNTER 1
//#define MEASURE_COVER_GET 1

#ifndef DISPATCH_QUEUE_CONCURRENT
#    define DISPATCH_QUEUE_CONCURRENT NULL
#endif

#define ENABLE_MUSICBRAINZ 0
#define ENABLE_ALBUMART_ORG 0

#define trace(...) \
    { deadbeef->log_detailed (&plugin.plugin.plugin, 0, __VA_ARGS__); }

DB_functions_t *deadbeef;
ddb_artwork_plugin_t plugin;

#define MAX_LISTENERS 100
static ddb_artwork_listener_t listeners[MAX_LISTENERS];
static void *listeners_userdata[MAX_LISTENERS];

dispatch_queue_t sync_queue; // used in artwork_internal, therefore not static
static dispatch_queue_t process_queue;
static dispatch_queue_t fetch_queue;
static dispatch_semaphore_t fetch_semaphore;

// Grouping queries by source id
static int64_t next_source_id;
typedef struct query_group_s {
    ddb_cover_query_t *query;
    struct query_group_s *next;
} query_group_item_t;

static query_group_item_t **query_groups; // unique groups
static int query_groups_count;
static int query_groups_reserved;

// Squashing queries
// To handle concurrency correctly, we need to be able to squash the queries
#define MAX_SQUASHED_QUERIES 50
typedef struct artwork_query_s {
    ddb_cover_query_t *queries[MAX_SQUASHED_QUERIES];
    ddb_cover_callback_t callbacks[MAX_SQUASHED_QUERIES];
    int query_count;
    struct artwork_query_s *next;
} artwork_query_t;

static artwork_query_t *query_head;
static artwork_query_t *query_tail;

// -

static int64_t last_job_idx;
static int64_t cancellation_idx;

#define MAX_COVERS_IN_CACHE 20
static ddb_cover_info_t *cover_cache[MAX_COVERS_IN_CACHE];

#define DEFAULT_SAVE_TO_MUSIC_FOLDERS_FILENAME "cover.jpg"

#ifdef ANDROID
#    define DEFAULT_SAVE_TO_MUSIC_FOLDERS 1
static int artwork_save_to_music_folders = DEFAULT_SAVE_TO_MUSIC_FOLDERS;
#else
#    define DEFAULT_SAVE_TO_MUSIC_FOLDERS 0
int artwork_save_to_music_folders = DEFAULT_SAVE_TO_MUSIC_FOLDERS;
#endif

static char *save_to_music_folders_filename;

static int artwork_enable_embedded;
static int artwork_enable_local;
#ifdef USE_VFS_CURL
static int artwork_enable_lfm;
#    if ENABLE_MUSICBRAINZ
static int artwork_enable_mb;
#    endif
#    if ENABLE_ALBUMART_ORG
static int artwork_enable_aao;
#    endif
static int artwork_enable_wos;
#endif
static int missing_artwork;
static char *nocover_path;
static int artwork_image_size;
static int simplified_cache;

static time_t cache_reset_time;

#define DEFAULT_FILEMASK \
    "front.png;front.jpg;front.jpeg;folder.png;folder.jpg;folder.jpeg;cover.png;cover.jpg;cover.jpeg;f.png;f.jpg;f.jpeg;*front*.png;*front*.jpg;*front*.jpeg;*cover*.png;*cover*.jpg;*cover.jpeg;*folder*.png;*folder*.jpg;*folder*.jpeg;*.png;*.jpg;*.jpeg"
#define DEFAULT_FOLDERS "art;scans;covers;artwork;artworks"

static char *artwork_filemask;
static char *artwork_folders;

static char *album_tf;
static char *artist_tf;
static char *title_tf;
static char *albumartist_tf;
static char *query_compare_tf;
static char *track_cache_filename_tf;
static char *album_cache_filename_tf;
static char *simplified_album_cache_filename_tf;

// Replace illegal file path characters with "-"
static char
_fix_illegal_char (char c) {
#ifdef _WIN32
    static const char chars[] = "<>:\"/\\|?*";
#else
    static const char chars[] = ":/";
#endif

    if (strchr (chars, c)) {
        return '-';
    }
    return c;
}

static int
make_track_cache_filename (ddb_playItem_t *it, char *outpath, size_t outsize) {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = it,
        .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
    };

    return deadbeef->tf_eval (&ctx, track_cache_filename_tf, outpath, (int)outsize);
}

static int
make_album_cache_filename (ddb_playItem_t *it, char *outpath, size_t outsize) {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = it,
        .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
    };

    return deadbeef->tf_eval (
        &ctx,
        simplified_cache ? simplified_album_cache_filename_tf : album_cache_filename_tf,
        outpath,
        (int)outsize);
}

static int
make_track_cache_path (ddb_playItem_t *it, char *outpath, size_t outsize) {
    outpath[0] = '\0';

    char root_path[PATH_MAX];
    if (make_cache_root_path (root_path, sizeof (root_path)) < 0) {
        return -1;
    }

    char cache_fname[PATH_MAX];
    if (make_track_cache_filename (it, cache_fname, sizeof (cache_fname)) < 0) {
        return -1;
    }

    snprintf (outpath, outsize, "%s/%s.jpg", root_path, cache_fname);

    // sanitize
    char *p = outpath + strlen (root_path) + 1;
    while (*p) {
        *p = _fix_illegal_char (*p);
        p++;
    }

    return 0;
}

static int
make_album_cache_path (ddb_playItem_t *it, char *outpath, int outsize) {
    outpath[0] = '\0';

    char root_path[PATH_MAX];
    if (make_cache_root_path (root_path, sizeof (root_path)) < 0) {
        return -1;
    }

    char cache_fname[PATH_MAX];
    if (make_album_cache_filename (it, cache_fname, sizeof (cache_fname)) < 0) {
        return -1;
    }

    snprintf (outpath, outsize, "%s/%s.jpg", root_path, cache_fname);

    // sanitize
    char *p = outpath + strlen (root_path) + 1;
    while (*p) {
        *p = _fix_illegal_char (*p);
        p++;
    }

    return 0;
}

static int
strings_equal (const char *s1, const char *s2) {
    return (s1 == s2) || (s1 && s2 && !strcasecmp (s1, s2));
}

#ifndef FNM_CASEFOLD
#    define FNM_CASEFOLD FNM_IGNORECASE
#endif

static int
vfs_scan_results (struct dirent *entry, const char *container_uri, ddb_cover_info_t *cover, const char *mask) {
    /* VFS container, double check the match in case scandir didn't implement filtering */
    if (!fnmatch (mask, entry->d_name, FNM_CASEFOLD)) {
        trace ("found cover %s in %s\n", entry->d_name, container_uri);
        size_t len = strlen (container_uri) + strlen (entry->d_name) + 2;
        cover->image_filename = malloc (len);
        snprintf (cover->image_filename, len, "%s:%s", container_uri, entry->d_name);
        return 0;
    }

    return -1;
}

static int
dir_scan_results (struct dirent *entry, const char *container, ddb_cover_info_t *cover) {
    /* Local file in a directory */
    trace ("found cover %s in local folder\n", entry->d_name);
    size_t len = strlen (container) + strlen (entry->d_name) + 2;
    cover->image_filename = malloc (len);
    snprintf (cover->image_filename, len, "%s/%s", container, entry->d_name);
    struct stat stat_struct;
    if (!stat (cover->image_filename, &stat_struct) && S_ISREG (stat_struct.st_mode) && stat_struct.st_size > 0) {
        return 0;
    }
    else {
        free (cover->image_filename);
        cover->image_filename = NULL;
    }

    return -1;
}

static int _dirent_alpha_cmp_func(const void *a, const void *b) {
    struct dirent * const *fa = a;
    struct dirent * const *fb = b;

    return strcmp((*fa)->d_name, (*fb)->d_name);
}

static int
scan_local_path (const char *local_path, const char *uri, DB_vfs_t *vfsplug, ddb_cover_info_t *cover) {
    struct dirent **files = NULL;
    int (*custom_scandir) (
        const char *,
        struct dirent ***,
        int (*) (const struct dirent *),
        int (*) (const struct dirent **, const struct dirent **));
    custom_scandir = vfsplug ? vfsplug->scandir : scandir;
    int files_count = custom_scandir (local_path, &files, NULL, NULL);

    __block char *filemask = NULL;

    dispatch_sync (sync_queue, ^{
        filemask = strdup (artwork_filemask);
    });

    int err = -1;

    if (files != NULL && files_count > 0) {

        // sort resulting files alphabetically, to ensure that numbered covers are prioritized correctly
        struct dirent **sorted_files = calloc(files_count, sizeof (struct dirent *));
        memcpy (sorted_files, files, files_count * sizeof (struct dirent *));
        qsort(sorted_files, files_count, sizeof (struct dirent *), _dirent_alpha_cmp_func);

        const char *filemask_end = filemask + strlen (filemask);
        char *p;
        while ((p = strrchr (filemask, ';'))) {
            *p = '\0';
        }

        for (char *mask = filemask; mask < filemask_end; mask += strlen (mask) + 1) {
            for (int i = 0; i < files_count; i++) {
                if (!fnmatch (mask, sorted_files[i]->d_name, FNM_CASEFOLD)) {
                    if (uri) {
                        err = vfs_scan_results (sorted_files[i], uri, cover, mask);
                    }
                    else {
                        err = dir_scan_results (sorted_files[i], local_path, cover);
                    }
                }
                if (!err) {
                    break;
                }
            }
            if (!err) {
                break;
            }
        }
        free (sorted_files);
        for (size_t i = 0; i < files_count; i++) {
            free (files[i]);
        }
        free (files);
    }

    free (filemask);
    return err;
}

// FIXME: this returns only one path that matches subfolder. Usually that's enough, but can be improved.
static char *
get_case_insensitive_path (const char *local_path, const char *subfolder, DB_vfs_t *vfsplug) {
    struct dirent **files = NULL;
    int (*custom_scandir) (
        const char *,
        struct dirent ***,
        int (*) (const struct dirent *),
        int (*) (const struct dirent **, const struct dirent **));
    custom_scandir = vfsplug ? vfsplug->scandir : scandir;
    int files_count = custom_scandir (local_path, &files, NULL, NULL);
    char *ret = NULL;
    if (files != NULL) {
        for (int i = 0; i < files_count; i++) {
            if (!strcasecmp (subfolder, files[i]->d_name)) {
                size_t l = strlen (local_path) + strlen (files[i]->d_name) + 2;
                ret = malloc (l);
                snprintf (ret, l, "%s/%s", local_path, files[i]->d_name);
                break;
            }
        }
        for (size_t i = 0; i < files_count; i++) {
            free (files[i]);
        }
        free (files);
    }
    return ret;
}

static int
local_image_file (const char *local_path, const char *uri, DB_vfs_t *vfsplug, ddb_cover_info_t *cover) {
    if (!artwork_filemask) {
        return -1;
    }

    char *p;

    char *folders = strdup (artwork_folders);
    const char *folders_end = folders + strlen (folders);
    while ((p = strrchr (folders, ';'))) {
        *p = '\0';
    }

    int root = 1;
    char *folder = folders;
    while (folder < folders_end) {
        char *path;
        if (root) {
            root = 0;
            path = strdup (local_path);
        }
        else {
            path = get_case_insensitive_path (local_path, folder, vfsplug);
            folder += strlen (folder) + 1;
        }
        trace ("scanning %s for artwork\n", path);
        if (path && !scan_local_path (path, uri, vfsplug, cover)) {
            free (folders);
            free (path);
            return 0;
        }
        free (path);
    }

    trace ("No cover art files in local folder\n");
    free (folders);
    return -1;
}

static const uint8_t *
id3v2_skip_str (int enc, const uint8_t *ptr, const uint8_t *end) {
    if (enc == 0 || enc == 3) {
        while (ptr < end && *ptr) {
            ptr++;
        }
        ptr++;
        return ptr < end ? ptr : NULL;
    }
    else {
        while (ptr < end - 1 && (ptr[0] || ptr[1])) {
            ptr += 2;
        }
        ptr += 2;
        return ptr < end ? ptr : NULL;
    }
}

static const uint8_t *
id3v2_artwork (const DB_id3v2_frame_t *f, int minor_version, int type) {
    if ((minor_version > 2 && strcmp (f->id, "APIC")) || (minor_version == 2 && strcmp (f->id, "PIC"))) {
        return NULL;
    }

    if (f->size < 20) {
        trace ("artwork: id3v2 APIC frame is too small\n");
        return NULL;
    }

    const uint8_t *data = f->data;

    if (minor_version == 4 && (f->flags[1] & 1)) {
        data += 4;
    }
    const uint8_t *end = f->data + f->size;
    int enc = *data;
    data++;

    if (minor_version > 2) {
        // mime type is ascii always, the enc above is for the picture type
        const uint8_t *mime_end = id3v2_skip_str (0, data, end);
        if (!mime_end) {
            trace ("artwork: corrupted id3v2 APIC frame\n");
            return NULL;
        }
        if (type == -1 || *mime_end == type) {
            trace ("artwork: picture type=%d\n", *mime_end);
            return NULL;
        }
        trace ("artwork: mime-type=%s, picture type: %d\n", data, *mime_end);
        data = mime_end;
    }
    else {
        data += 3; // image format
    }
    data++; // picture type
    data = id3v2_skip_str (enc, data, end); // description
    if (!data) {
        trace ("artwork: corrupted id3v2 APIC frame\n");
        return NULL;
    }

    return data;
}

static const uint8_t *
apev2_artwork (const DB_apev2_frame_t *f) {
    if (strcasecmp (f->key, "cover art (front)")) {
        return NULL;
    }

    const uint8_t *data = f->data;
    const uint8_t *end = f->data + f->size;
    while (data < end && *data)
        data++;
    if (data == end) {
        trace ("artwork: apev2 cover art frame has no name\n");
        return NULL;
    }

    size_t sz = end - ++data;
    if (sz < 20) {
        trace ("artwork: apev2 cover art frame is too small\n");
        return NULL;
    }

    return data;
}

static int
id3_extract_art (ddb_cover_info_t *cover) {
    int err = -1;

    DB_id3v2_tag_t id3v2_tag;
    memset (&id3v2_tag, 0, sizeof (id3v2_tag));
    DB_FILE *id3v2_fp = deadbeef->fopen (cover->priv->filepath);
    if (id3v2_fp && !deadbeef->junk_id3v2_read_full (NULL, &id3v2_tag, id3v2_fp)) {
        int minor_version = id3v2_tag.version[0];
        DB_id3v2_frame_t *fprev = NULL;
        for (DB_id3v2_frame_t *f = id3v2_tag.frames; f; f = f->next) {
            const uint8_t *image_data = id3v2_artwork (f, minor_version, 3);
            if (!image_data) {
                // try any cover if front is not available
                image_data = id3v2_artwork (f, minor_version, 0);
            }
            if (image_data) {
                const size_t sz = f->size - (image_data - f->data);
                if (sz <= 0) {
                    continue;
                }
                // steal the frame memory from DB_id3v2_tag_t
                if (fprev) {
                    fprev->next = f->next;
                }
                else {
                    id3v2_tag.frames = f->next;
                }

                cover->priv->blob = (char *)f;
                cover->priv->blob_size = f->size;
                cover->priv->blob_image_offset = (uint64_t)((char *)image_data - (char *)cover->priv->blob);
                cover->priv->blob_image_size = sz;
                err = 0;
                break;
            }
            fprev = f;
        }
    }
    deadbeef->junk_id3v2_free (&id3v2_tag);
    if (id3v2_fp) {
        deadbeef->fclose (id3v2_fp);
    }
    return err;
}

static int
apev2_extract_art (ddb_cover_info_t *cover) {
    int err = -1;
    DB_apev2_tag_t apev2_tag;
    memset (&apev2_tag, 0, sizeof (apev2_tag));
    DB_FILE *apev2_fp = deadbeef->fopen (cover->priv->filepath);
    if (apev2_fp && !deadbeef->junk_apev2_read_full (NULL, &apev2_tag, apev2_fp)) {
        DB_apev2_frame_t *fprev = NULL;
        for (DB_apev2_frame_t *f = apev2_tag.frames; f; f = f->next) {
            const uint8_t *image_data = apev2_artwork (f);
            if (image_data) {
                const size_t sz = f->size - (image_data - f->data);
                if (sz <= 0) {
                    continue;
                }
                // steal the frame memory from DB_id3v2_tag_t
                if (fprev) {
                    fprev->next = f->next;
                }
                else {
                    apev2_tag.frames = f->next;
                }

                cover->priv->blob = (char *)f;
                cover->priv->blob_size = f->size;
                cover->priv->blob_image_offset = (uint64_t)((char *)image_data - (char *)cover->priv->blob);
                cover->priv->blob_image_size = sz;
                err = 0;
                break;
            }
            fprev = f;
        }
    }
    deadbeef->junk_apev2_free (&apev2_tag);
    if (apev2_fp) {
        deadbeef->fclose (apev2_fp);
    }
    return err;
}

static int
mp4_extract_art (ddb_cover_info_t *cover) {
    int ret = -1;
    mp4p_atom_t *mp4file = NULL;
    DB_FILE *fp = NULL;
    uint8_t *image_blob = NULL;

    if (!strcasestr (cover->priv->filepath, ".mp4") && !strcasestr (cover->priv->filepath, ".m4a") &&
        !strcasestr (cover->priv->filepath, ".m4b")) {
        return -1;
    }

    fp = deadbeef->fopen (cover->priv->filepath);
    if (!fp) {
        goto error;
    }

    mp4p_file_callbacks_t callbacks = { 0 };
    callbacks.ptrhandle = fp;
    mp4_init_ddb_file_callbacks (&callbacks);
    mp4file = mp4p_open (&callbacks);
    if (!mp4file) {
        goto error;
    }

    mp4p_atom_t *covr = mp4_get_cover_atom (mp4file);
    if (!covr) {
        goto error;
    }

    mp4p_ilst_meta_t *data = covr->data;

    uint32_t sz = data->data_size;
    image_blob = malloc (sz);
    if (data->blob) {
        image_blob = memcpy (image_blob, data->blob, sz);
    }
    else if (data->values) {
        uint16_t *v = data->values;
        for (size_t i = 0; i < sz / 2; i++, v++) {
            image_blob[i * 2 + 0] = (*v) >> 8;
            image_blob[i * 2 + 1] = (*v) & 0xff;
        }
    }
    else {
        goto error;
    }

    cover->priv->blob = (char *)image_blob;
    image_blob = NULL;
    cover->priv->blob_size = data->data_size;
    cover->priv->blob_image_size = data->data_size;
    ret = 0;

error:
    if (image_blob) {
        free (image_blob);
        image_blob = NULL;
    }
    if (mp4file) {
        mp4p_atom_free_list (mp4file);
        mp4file = NULL;
    }
    if (fp) {
        deadbeef->fclose (fp);
        fp = NULL;
    }
    return ret;
}

static int
web_lookups (const char *cache_path, ddb_cover_info_t *cover) {
    if (!cache_path) {
        return -1;
    }
#ifdef USE_VFS_CURL
    if (artwork_enable_lfm) {
        if (!fetch_from_lastfm (cover->priv->artist, cover->priv->album, cache_path)) {
            cover->image_filename = strdup (cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return -1;
        }
    }
#    if ENABLE_MUSICBRAINZ
    // Albumart.org and Musicbrainz have either changed their APIs, or are broken in general -- therefore disabled.
    if (artwork_enable_mb) {
        if (!fetch_from_musicbrainz (cover->artist, cover->album, cache_path)) {
            cover->image_filename = strdup (cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return -1;
        }
    }
#    endif

#    if ENABLE_ALBUMART_ORG
    if (artwork_enable_aao) {
        if (!fetch_from_albumart_org (cover->artist, cover->album, cache_path)) {
            cover->image_filename = strdup (cache_path);
            return 1;
        }
        if (errno == ECONNABORTED) {
            return -1;
        }
    }
#    endif
#endif

    return -1;
}

static char *
vfs_path (const char *fname) {
    if (fname[0] == '/' || strstr (fname, "file://") == fname) {
        return NULL;
    }

    char *p = strstr (fname, "://");
    if (p) {
        p += 3;
        char *q = strrchr (p, ':');
        if (q) {
            *q = '\0';
        }
    }
    return p;
}

static DB_vfs_t *
scandir_plug (const char *vfs_fname) {
    DB_vfs_t **vfsplugs = deadbeef->plug_get_vfs_list ();
    for (size_t i = 0; vfsplugs[i]; i++) {
        if (vfsplugs[i]->is_container && vfsplugs[i]->is_container (vfs_fname) && vfsplugs[i]->scandir) {
            return vfsplugs[i];
        }
    }

    return NULL;
}

static int
_is_path_newer_than_time (const char *fname, const time_t time) {
    struct stat stat_buf;
    return !stat (fname, &stat_buf) && stat_buf.st_mtime > time;
}

// returns 1 if enough time passed since the last attemp to find the requested cover
static int
recheck_missing_artwork (const char *input_fname, const time_t cache_mtime) {
    int res = 0;
    char *fname = strdup (input_fname);
    /* Check if local files could have new associated artwork */
    if (deadbeef->is_local_file (fname)) {
        char *vfs_fname = vfs_path (fname);
        const char *real_fname = vfs_fname ? vfs_fname : fname;

        /* Recheck artwork if file (track or VFS container) was modified since the last check */
        if (_is_path_newer_than_time (real_fname, cache_mtime)) {
            return 1;
        }

        /* Recheck local artwork if the directory contents have changed */
        char *dname = strdup (dirname (fname));
        res = artwork_enable_local && _is_path_newer_than_time (dname, cache_mtime);
        free (dname);
    }

    free (fname);
    return res;
}

static void
_touch (const char *path) {
    struct stat stat_struct;
    if (0 != stat (path, &stat_struct)) {
        FILE *fp = fopen (path, "w+b");
        if (fp != NULL) {
            (void)fclose (fp);
        }
    }
    else {
#ifdef _WIN32
        (void)_utime (path, NULL);
#else
        (void)utimes (path, NULL);
#endif
    }
}

static void
_free_blob (ddb_cover_info_t *cover) {
    free (cover->priv->blob);
    cover->priv->blob = NULL;
    cover->priv->blob_size = 0;
    cover->priv->blob_image_offset = 0;
    cover->priv->blob_image_size = 0;
}

static void
_consume_blob (ddb_cover_info_t *cover, const char *cache_path) {
    if (cover->image_filename != NULL) {
        _free_blob (cover);
        return;
    }
    if (cover->priv->blob != NULL) {
        write_file (cache_path, cover->priv->blob + cover->priv->blob_image_offset, cover->priv->blob_image_size);
        cover->image_filename = strdup (cache_path);
        _free_blob (cover);
    }
}

// Behavior:
// Found in cache: сache path is returned
// Local cover: save to cache & return path
// Embedded cover: save to cache & return blob
// Web cover: save_to_local ? save_to_local&return_path : save_to_cache&return_path
static void
process_query (ddb_cover_info_t *cover) {
    // If all sources are off: just return / do nothing.
    if (!artwork_enable_local
        && !artwork_enable_embedded
        && !artwork_enable_lfm
        && !artwork_enable_wos
#    if ENABLE_MUSICBRAINZ
        && !artwork_enable_mb
#    endif
#   if ENABLE_ALBUMART_ORG
        && !artwork_enable_aao
#   endif
        ) {
        return;
    }

    int islocal = deadbeef->is_local_file (cover->priv->filepath);

    struct stat cache_stat;

    int res = -1;
    if (!simplified_cache && cover->priv->track_cache_path[0]) {
        res = stat (cover->priv->track_cache_path, &cache_stat);
        if (res == 0 && cache_stat.st_size != 0 && cache_stat.st_mtime >= cache_reset_time) {
            cover->image_filename = strdup (cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }
    }

    if (cover->priv->album_cache_path[0]) {
        res = stat (cover->priv->album_cache_path, &cache_stat);
        if (res == 0 && cache_stat.st_size != 0 && cache_stat.st_mtime >= cache_reset_time) {
            cover->image_filename = strdup (cover->priv->album_cache_path);
            cover->cover_found = 1;
            return;
        }
    }
    else {
        trace ("artwork: undefined album cache path\n");
        return;
    }

    // Flood control, don't retry missing artwork for an hour unless something changes
    if (res == 0 && cache_stat.st_mtime >= cache_reset_time && cache_stat.st_mtime + 60 * 60 > time (NULL)) {
        int recheck = cache_stat.st_size == 0 || recheck_missing_artwork (cover->priv->filepath, cache_stat.st_mtime);
        if (!recheck) {
            return;
        }
    }

    if (artwork_enable_local && islocal) {
        char *fname_copy = strdup (cover->priv->filepath);
        if (fname_copy) {
            char *vfs_fname = vfs_path (fname_copy);
            if (vfs_fname) {
                /* Search inside scannable VFS containers */
                DB_vfs_t *plugin = scandir_plug (vfs_fname);
                if (plugin && !local_image_file (vfs_fname, fname_copy, plugin, cover)) {
                    free (fname_copy);
                    copy_file (cover->image_filename, cover->priv->album_cache_path);
                    cover->cover_found = 1;
                    return;
                }
            }

            /* Search in file directory */
            if (!local_image_file (dirname (vfs_fname ? vfs_fname : fname_copy), NULL, NULL, cover)) {
                free (fname_copy);
                copy_file (cover->image_filename, cover->priv->album_cache_path);
                cover->cover_found = 1;
                return;
            }

            free (fname_copy);
        }
    }

    if (artwork_enable_embedded && islocal) {
#ifdef USE_METAFLAC
        // try to load embedded from flac metadata
        trace ("trying to load artwork from Flac tag for %s\n", cover->priv->filepath);
        if (!flac_extract_art (cover)) {
            _consume_blob (cover, simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }
#endif

        // try to load embedded from id3v2
        trace ("trying to load artwork from id3v2 tag for %s\n", cover->priv->filepath);
        if (!id3_extract_art (cover)) {
            _consume_blob (cover, simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }

        // try to load embedded from apev2
        trace ("trying to load artwork from apev2 tag for %s\n", cover->priv->filepath);
        if (!apev2_extract_art (cover)) {
            _consume_blob (cover, simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }

        // try to load embedded from mp4
        trace ("trying to load artwork from mp4 tag for %s\n", cover->priv->filepath);
        if (!mp4_extract_art (cover)) {
            _consume_blob (cover, simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }

#ifdef USE_OGG
        trace ("trying to load artwork from ogg tag for %s\n", cover->priv->filepath);
        if (!ogg_extract_art (cover)) {
            _consume_blob (cover, simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }
#endif

#ifdef USE_OPUS
        trace ("trying to load artwork from opus tag for %s\n", cover->priv->filepath);
        if (!opus_extract_art (cover)) {
            _consume_blob (cover, simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            cover->cover_found = 1;
            return;
        }
#endif
    }

#ifdef USE_VFS_CURL
    // Don't do anything if all web lookups are off
    if (!artwork_enable_wos && !artwork_enable_lfm
#    if ENABLE_MUSICBRAINZ
        && !artwork_enable_mb
#    endif
#    if ENABLE_ALBUMART_ORG
        && !artwork_enable_aao
#    endif
    ) {
        unlink (cover->priv->track_cache_path);
        unlink (cover->priv->album_cache_path);
        _touch (cover->priv->album_cache_path);
        return;
    }

    /* Web lookups */
    res = -1;

    // don't attempt to load AY covers from regular music services
    if (artwork_enable_wos && strlen (cover->priv->filepath) > 3 &&
        !strcasecmp (cover->priv->filepath + strlen (cover->priv->filepath) - 3, ".ay")) {
        if (!fetch_from_wos (
                cover->priv->title,
                simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path)) {
            cover->image_filename =
                strdup (simplified_cache ? cover->priv->album_cache_path : cover->priv->track_cache_path);
            res = 1;
        }
        if (errno == ECONNABORTED) {
            res = -1;
        }
    }
    else {
        res = web_lookups (cover->priv->album_cache_path, cover);
    }
    if (res < 0) {
        /* Try stripping parenthesised text off the end of the album name */
        char *p = strpbrk (cover->priv->album, "([");
        if (p) {
            *p = '\0';
            res = web_lookups (cover->priv->album_cache_path, cover);
            *p = '(';
        }
    }
    if (res >= 0) {
        cover->cover_found = res;

        if (res && artwork_save_to_music_folders && cover->image_filename && !cover->priv->is_compilation) {
            // save to the music folder (only if not present)
            char *slash = strrchr (cover->priv->filepath, '/');

            if (!slash) {
                return;
            }
            size_t len = slash - cover->priv->filepath + 1;

            __block char *covername = NULL;
            dispatch_sync (sync_queue, ^{
                if (save_to_music_folders_filename != NULL) {
                    covername = strdup (save_to_music_folders_filename);
                }
            });
            if (covername != NULL) {
                size_t covername_len = strlen (covername) + 1;
                size_t coverpath_len = len + covername_len;
                char *coverpath = malloc (coverpath_len);
                memcpy (coverpath, cover->priv->filepath, len);
                memcpy (coverpath + len, covername, covername_len);
                free (covername);
                covername = NULL;

                struct stat stat_struct;
                if (stat (coverpath, &stat_struct)) {
                    copy_file (cover->image_filename, coverpath);
                }
                free (coverpath);
                coverpath = NULL;
            }
        }

        return;
    }
    else {
        unlink (cover->priv->track_cache_path);
        unlink (cover->priv->album_cache_path);
        _touch (cover->priv->album_cache_path);
    }
#endif
}

#pragma mark - In memory cache

static void
cover_update_cache (ddb_cover_info_t *cover) {
    // any empty items?
    time_t minTimestamp = INT64_MAX;
    int minTimestampIdx = -1;
    int emptyIdx = -1;
    for (int i = 0; i < MAX_COVERS_IN_CACHE; i++) {
        if (!cover_cache[i]) {
            emptyIdx = i;
            break;
        }

        if (cover_cache[i]->priv->timestamp < minTimestamp || emptyIdx == -1) {
            minTimestamp = cover_cache[i]->priv->timestamp;
            minTimestampIdx = i;
        }
    }

    if (emptyIdx < 0) {
        cover_info_release (cover_cache[minTimestampIdx]);
        cover_cache[minTimestampIdx] = NULL;
        emptyIdx = minTimestampIdx;
    }
    cover_cache[emptyIdx] = cover;
    cover->priv->timestamp = time (NULL);
    cover_info_ref (cover);
}

static void
cover_cache_free (void) {
    for (int i = 0; i < MAX_COVERS_IN_CACHE; i++) {
        if (!cover_cache[i]) {
            continue;
        }
        cover_info_release (cover_cache[i]);
        cover_cache[i] = NULL;
    }
}

static ddb_cover_info_t *
cover_cache_find (ddb_cover_info_t *cover) {
    for (int i = 0; i < MAX_COVERS_IN_CACHE; i++) {
        if (!cover_cache[i]) {
            continue;
        }

        ddb_cover_info_t *cached_cover = cover_cache[i];

        if (!strcmp (cover->priv->filepath, cached_cover->priv->filepath)) {
            return cached_cover;
        }
    }

    return NULL;
}

static void
cover_cache_remove (ddb_cover_info_t *cover) {
    for (int i = 0; i < MAX_COVERS_IN_CACHE; i++) {
        if (!cover_cache[i]) {
            continue;
        }

        ddb_cover_info_t *cached_cover = cover_cache[i];

        if (!strcmp (cover->priv->filepath, cached_cover->priv->filepath)) {
            cover_info_release (cached_cover);
            cover_cache[i] = NULL;
            break;
        }
    }
}

#pragma mark - Utility

static void
_setup_tf_once (void) {
    dispatch_sync (sync_queue, ^{
        if (!album_tf) {
            album_tf = deadbeef->tf_compile ("%album%");
        }
        if (!artist_tf) {
            artist_tf = deadbeef->tf_compile ("$itematindex(0,%artist%)");
        }
        if (!title_tf) {
            title_tf = deadbeef->tf_compile ("%title%");
        }
        if (!albumartist_tf) {
            albumartist_tf = deadbeef->tf_compile ("%album artist%");
        }
        if (!query_compare_tf) {
            query_compare_tf = deadbeef->tf_compile (
                "$if($and(%title%,%artist%,%album%),%track number% - %title% - %artist% - %album%)");
        }
        if (!track_cache_filename_tf) {
            track_cache_filename_tf = deadbeef->tf_compile (
                "$if($and(%album%,%artist%,%title%),%album% - %artist% - %title%,[$directory(%path%,2)-][$directory(%path%)-]%filename%)");
        }
        if (!album_cache_filename_tf) {
            album_cache_filename_tf = deadbeef->tf_compile (
                "$if($and(%album%,%artist%),[$directory(%path%,2)-][$directory(%path%)-]%album% - %artist%,[$directory(%path%,2)-][$directory(%path%)-]%filename%)");
        }
        if (!simplified_album_cache_filename_tf) {
            // This is to stay compatible with simple (but ambiguous) cache filename scheme
            simplified_album_cache_filename_tf = deadbeef->tf_compile (
                "$if($and(%album%,%artist%),%album% - %artist%,[$directory(%path%,2)-][$directory(%path%)-]%filename%)");
        }
    });
}

static void
_notify_listeners (ddb_artwork_listener_event_t event, DB_playItem_t *it) {
    __block ddb_artwork_listener_t *callbacks = calloc (MAX_LISTENERS, sizeof (ddb_artwork_listener_t));
    __block void **userdatas = calloc (MAX_LISTENERS, sizeof (void *));
    __block int count = 0;
    dispatch_sync (sync_queue, ^{
        for (int i = 0; i < MAX_LISTENERS; i++) {
            if (listeners[i] != NULL) {
                callbacks[count] = listeners[i];
                userdatas[count] = listeners_userdata[i];
                count += 1;
            }
        }
    });

    for (int i = 0; i < count; i++) {
        callbacks[i](event, userdatas[i], (intptr_t)it, 0);
    }
    free (callbacks);
    free (userdatas);
}

static ddb_cover_info_t *
sync_cover_info_alloc (void) {
    __block ddb_cover_info_t *cover = NULL;
    dispatch_sync (sync_queue, ^{
        cover = cover_info_alloc ();
    });
    return cover;
}

static void
sync_cover_info_ref (ddb_cover_info_t *cover) {
    dispatch_sync (sync_queue, ^{
        cover_info_ref (cover);
    });
}

static void
sync_cover_info_release (ddb_cover_info_t *cover) {
    dispatch_sync (sync_queue, ^{
        cover_info_release (cover);
    });
}

static void
queue_clear (void) {
    dispatch_sync (sync_queue, ^{
        artwork_abort_all_http_requests ();
        cancellation_idx = last_job_idx++;
    });
}

#pragma mark - Grouping queries by source id

static void
_groups_register_query (ddb_cover_query_t *query) {
    // find existing group
    int group_index = -1;
    for (int i = 0; i < query_groups_count; i++) {
        if (query_groups[i] == NULL) {
            group_index = i;
        }
        else if (query_groups[i]->query->source_id == query->source_id) {
            group_index = i;
            break;
        }
    }

    if (group_index < 0) {
        group_index = query_groups_count;
        query_groups_count += 1;
        if (query_groups_count > query_groups_reserved) {
            int prev_size = query_groups_reserved;
            if (query_groups_reserved == 0) {
                query_groups_reserved = 10;
            }
            else {
                query_groups_reserved *= 2;
            }
            query_groups = realloc (query_groups, query_groups_reserved * sizeof (query_group_item_t *));
            memset (query_groups + prev_size, 0, (query_groups_reserved - prev_size) * sizeof (query_group_item_t *));
        }
    }

    query_group_item_t *item = calloc (1, sizeof (query_group_item_t));
    item->query = query;
    item->next = query_groups[group_index];
    query_groups[group_index] = item;
}

static void
_groups_unregister_query (ddb_cover_query_t *query) {
    int group_index = -1;
    for (int i = 0; i < query_groups_count; i++) {
        if (query_groups[i] != NULL && query_groups[i]->query->source_id == query->source_id) {
            group_index = i;
            break;
        }
    }

    if (group_index == -1) {
        trace ("_groups_unregister_query: query not registered\n");
        return;
    }

    int done = 0;
    query_group_item_t *prev = NULL;
    query_group_item_t *group = query_groups[group_index];
    for (; group; prev = group, group = group->next) {
        if (group->query == query) {
            if (prev != NULL) {
                prev->next = group->next;
            }
            else {
                query_groups[group_index] = group->next;
            }
            free (group);
            done = 1;
            break;
        }
    }

    assert (done);
}

#pragma mark - Ending queries / cleanup / executing callbacks

#if DEBUG_COUNTER
static int _queued_jobs_counter = 0;
#endif

static void
_end_query (ddb_cover_query_t *query, ddb_cover_callback_t callback, int error, ddb_cover_info_t *cover) {
    assert (query);
    dispatch_sync (sync_queue, ^{
        _groups_unregister_query (query);
    });
    callback (error, query, cover);
#if DEBUG_COUNTER
    _queued_jobs_counter -= 1;
    printf ("*** count %d\n", _queued_jobs_counter);
#endif
}

static void
_execute_callback (ddb_cover_callback_t callback, ddb_cover_info_t *cover, ddb_cover_query_t *query) {
    int cover_found = cover->cover_found;

    // Make all the callbacks (and free the chain), with data if a file was written
    if (cover_found) {
        trace ("artwork fetcher: cover art file found: %s\n", cover->image_filename);
        sync_cover_info_ref (cover);
        _end_query (query, callback, 0, cover);
    }
    else {
        trace ("artwork fetcher: no cover art found\n");
        _end_query (query, callback, -1, NULL);
    }
}

#pragma mark - Squashing multiple similar queries into one

static int
queries_squashable (ddb_cover_query_t *query1, ddb_cover_query_t *query2) {
    if (query1->flags != query2->flags) {
        return 0;
    }

    if (query1->track == query2->track) {
        return 1;
    }

    const char *uri1 = deadbeef->pl_find_meta (query1->track, ":URI");
    const char *uri2 = deadbeef->pl_find_meta (query2->track, ":URI");

    if (uri1 == uri2) {
        return 1;
    }

    // if all metadata is defined -- compare tracknr, title, album, artist
    ddb_tf_context_t ctx = { 0 };
    ctx._size = sizeof (ddb_tf_context_t);

    char query1title[1000];
    char query2title[1000];
    ctx.it = query1->track;
    deadbeef->tf_eval (&ctx, query_compare_tf, query1title, sizeof (query1title));
    ctx.it = query2->track;
    deadbeef->tf_eval (&ctx, query_compare_tf, query2title, sizeof (query2title));

    if (query1title[0] && query2title[0] && !strcmp (query1title, query2title)) {
        return 1;
    }

    return 0;
}

static int
squash_query (ddb_cover_callback_t callback, ddb_cover_query_t *query) {
    __block int squashed = 0;
    dispatch_sync (sync_queue, ^{
        artwork_query_t *q = query_head;
        for (; q; q = q->next) {
            if (queries_squashable (query, q->queries[0]) && q->query_count < MAX_SQUASHED_QUERIES - 1) {
                squashed = 1;
                break;
            }
        }

        if (!q) {
            // create new
            q = calloc (1, sizeof (artwork_query_t));
            if (query_tail) {
                query_tail->next = q;
                query_tail = q;
            }
            else {
                query_head = query_tail = q;
            }
            squashed = 0;
        }
        q->queries[q->query_count] = query;
        q->callbacks[q->query_count] = callback;
        q->query_count += 1;
    });

    return squashed;
}

static void
callback_and_free_squashed (ddb_cover_info_t *cover, ddb_cover_query_t *query) {
    __block artwork_query_t *squashed_queries = NULL;
    dispatch_sync (sync_queue, ^{
        cover_update_cache (cover);
        // find & remove from the queries list
        artwork_query_t *q = query_head;
        artwork_query_t *prev = NULL;
        for (; q; q = q->next) {
            if (q->queries[0] == query) {
                break;
            }
            prev = q;
        }

        if (q) {
            if (prev) {
                prev->next = q->next;
            }
            else {
                query_head = q->next;
            }
            if (q == query_tail) {
                query_tail = prev;
            }
            squashed_queries = q;
        }
    });

    // send the result
    if (squashed_queries != NULL) {
        for (int i = 0; i < squashed_queries->query_count; i++) {
            _execute_callback (squashed_queries->callbacks[i], cover, squashed_queries->queries[i]);
        }
        free (squashed_queries);
    }
    sync_cover_info_release (cover);
}

static void
_init_cover_metadata (ddb_cover_info_t *cover, ddb_playItem_t *track) {
    _setup_tf_once ();

    deadbeef->pl_lock ();
    strncat (
        cover->priv->filepath,
        deadbeef->pl_find_meta (track, ":URI"),
        sizeof (cover->priv->filepath) - strlen (cover->priv->filepath) - 1);
    deadbeef->pl_unlock ();

    ddb_tf_context_t ctx = { 0 };
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = track;

#ifdef USE_VFS_CURL
    if (artwork_enable_wos && strlen (cover->priv->filepath) > 3 &&
        !strcasecmp (cover->priv->filepath + strlen (cover->priv->filepath) - 3, ".ay")) {
        strcpy (cover->priv->artist, "AY Music");
        deadbeef->tf_eval (&ctx, title_tf, cover->priv->album, sizeof (cover->priv->album));
        for (char *p = cover->priv->album; *p; p++) {
            if (p[0] == ' ' && p[1] == '-') {
                *p = 0;
                break;
            }
        }
        strcpy (cover->priv->title, cover->priv->album);
    }
    else
#endif
    {
        deadbeef->tf_eval (&ctx, album_tf, cover->priv->album, sizeof (cover->priv->album));
        deadbeef->tf_eval (&ctx, artist_tf, cover->priv->artist, sizeof (cover->priv->artist));
        deadbeef->tf_eval (&ctx, title_tf, cover->priv->title, sizeof (cover->priv->title));

        char albumartist[100];
        deadbeef->tf_eval (&ctx, albumartist_tf, albumartist, sizeof (albumartist));
        if (!strcasecmp (albumartist, "Various Artists")) {
            cover->priv->is_compilation = 1;
        }
    }

    if (!simplified_cache) {
        make_album_cache_path (track, cover->priv->album_cache_path, sizeof (cover->priv->album_cache_path));
        make_track_cache_path (track, cover->priv->track_cache_path, sizeof (cover->priv->track_cache_path));
    }
    else {
        make_album_cache_path (track, cover->priv->album_cache_path, sizeof (cover->priv->album_cache_path));
    }
}

#pragma mark - API entry points

static void
cover_get (ddb_cover_query_t *query, ddb_cover_callback_t callback) {
#if DEBUG_COUNTER
    _queued_jobs_counter += 1;
    printf ("*** count %d\n", _queued_jobs_counter);
#endif
    __block int64_t job_idx = 0;
    dispatch_sync (sync_queue, ^{
        job_idx = last_job_idx++;
        _groups_register_query (query);
    });

#if MEASURE_COVER_GET
    struct timeval tm1;
    gettimeofday (&tm1, NULL);
#endif

    dispatch_async (process_queue, ^{
        if (!query->track) {
            _end_query (query, callback, -1, NULL);
            return;
        }

        /* Process this query, hopefully writing a file into cache */
        __block ddb_cover_info_t *cover = sync_cover_info_alloc ();

        _init_cover_metadata (cover, query->track);

        __block int cancel_job = 0;
        dispatch_sync (sync_queue, ^{
            if (job_idx < cancellation_idx || (query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
                cancel_job = 1;
            }
        });

        if (cancel_job) {
            _end_query (query, callback, -1, NULL);
            return;
        }

        // check the cache
        __block int found_in_cache = 0;
        dispatch_sync (sync_queue, ^{
            ddb_cover_info_t *cached_cover = cover_cache_find (cover);
            if (cached_cover) {
                found_in_cache = 1;
                cached_cover->priv->timestamp = time (NULL);
                cover_info_release (cover);
                cover = cached_cover;
            }
        });

        if (found_in_cache) {
            _execute_callback (callback, cover, query);
        }
        else {
            // Check if another query for the same thing is already present in the queue, and squash.
            if (squash_query (callback, query)) {
                return;
            }

            // fetch on concurrent fetch queue
            dispatch_semaphore_wait (fetch_semaphore, DISPATCH_TIME_FOREVER);
            __block int cancel_job = 0;
            dispatch_sync (sync_queue, ^{
                if (job_idx < cancellation_idx) {
                    cancel_job = 1;
                }
            });

            if (cancel_job) {
                callback_and_free_squashed (cover, query);
                dispatch_semaphore_signal (fetch_semaphore);
                return;
            }

            dispatch_async (fetch_queue, ^{
                process_query (cover);

#if MEASURE_COVER_GET
                struct timeval tm2;
                gettimeofday (&tm2, NULL);
                long ms = (tm2.tv_sec * 1000 + tm2.tv_usec / 1000) - (tm1.tv_sec * 1000 + tm1.tv_usec / 1000);
                const char *uri = deadbeef->pl_find_meta (query->track, ":URI");
                printf ("cover_get %s took %d ms\n", uri, (int)ms);
#endif

                // update queue, and notity the caller
                callback_and_free_squashed (cover, query);
                dispatch_semaphore_signal (fetch_semaphore);
            });
        }
    });
}

static void
artwork_reset (void) {
    trace ("artwork: reset queue\n");
    queue_clear ();
}

static void
artwork_add_listener (ddb_artwork_listener_t listener, void *user_data) {
    dispatch_sync (sync_queue, ^{
        for (int i = 0; i < MAX_LISTENERS; i++) {
            if (listeners[i] == NULL) {
                listeners[i] = listener;
                listeners_userdata[i] = user_data;
                break;
            }
        }
    });
}

void
artwork_remove_listener (ddb_artwork_listener_t listener, void *user_data) {
    dispatch_sync (sync_queue, ^{
        for (int i = 0; i < MAX_LISTENERS; i++) {
            if (listeners[i] == listener) {
                listeners[i] = NULL;
                listeners_userdata[i] = NULL;
                break;
            }
        }
    });
}

static void
artwork_default_image_path (char *path, size_t size) {
    *path = 0;
    dispatch_sync (sync_queue, ^{
        if (nocover_path != NULL) {
            strncat (path, nocover_path, size - 1);
        }
    });
}

static int64_t
artwork_allocate_source_id (void) {
    __block int64_t retval;
    dispatch_sync (sync_queue, ^{
        retval = next_source_id;
        next_source_id += 1;
        if (next_source_id < 0) {
            next_source_id = 1;
        }
    });
    return retval;
}

/// Cancel all queries with the specified source_id
static void
artwork_cancel_queries_with_source_id (int64_t source_id) {
    dispatch_sync (sync_queue, ^{
        for (int i = 0; i < query_groups_count; i++) {
            query_group_item_t *group = query_groups[i];
            if (group != NULL && group->query->source_id == source_id) {
                for (query_group_item_t *item = query_groups[i]; item; item = item->next) {
                    item->query->flags |= DDB_ARTWORK_FLAG_CANCELLED;
                }
                break;
            }
        }
    });
}

static void
_get_fetcher_preferences (void) {
    deadbeef->conf_lock ();
    artwork_save_to_music_folders =
        deadbeef->conf_get_int ("artwork.save_to_music_folders", DEFAULT_SAVE_TO_MUSIC_FOLDERS);

    const char *save_filename = deadbeef->conf_get_str_fast (
        "artwork.save_to_music_folders_relative_path",
        DEFAULT_SAVE_TO_MUSIC_FOLDERS_FILENAME);

    free (save_to_music_folders_filename);
    save_to_music_folders_filename = strdup (save_filename);

    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
    const char *new_artwork_filemask = deadbeef->conf_get_str_fast ("artwork.filemask", NULL);
    if (!new_artwork_filemask || !new_artwork_filemask[0]) {
        new_artwork_filemask = DEFAULT_FILEMASK;
    }
    if (!strings_equal (artwork_filemask, new_artwork_filemask)) {
        char *old_artwork_filemask = artwork_filemask;
        artwork_filemask = strdup (new_artwork_filemask);
        if (old_artwork_filemask) {
            free (old_artwork_filemask);
        }
    }

    const char *new_artwork_folders = deadbeef->conf_get_str_fast ("artwork.folders", NULL);
    if (!new_artwork_folders || !new_artwork_folders[0]) {
        new_artwork_folders = DEFAULT_FOLDERS;
    }
    if (!strings_equal (artwork_folders, new_artwork_folders)) {
        char *old_artwork_folders = artwork_folders;
        artwork_folders = strdup (new_artwork_folders);
        if (old_artwork_folders) {
            free (old_artwork_folders);
        }
    }
    deadbeef->conf_unlock ();
#ifdef USE_VFS_CURL
    artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
#    if ENABLE_MUSICBRAINZ
    artwork_enable_mb = deadbeef->conf_get_int ("artwork.enable_musicbrainz", 0);
#    endif
#    if ENABLE_ALBUMART_ORG
    artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
#    endif
    artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif
    missing_artwork = deadbeef->conf_get_int ("artwork.missing_artwork", 1);
    artwork_image_size = deadbeef->conf_get_int ("artwork.image_size", 256);
    if (artwork_image_size < 64) {
        artwork_image_size = 64;
    }
    if (artwork_image_size > 2048) {
        artwork_image_size = 2048;
    }

    simplified_cache = deadbeef->conf_get_int ("artwork.cache.simplified", 0);

    deadbeef->conf_lock ();
    if (missing_artwork == 0) {
        free (nocover_path);
        nocover_path = NULL;
    }
    else if (missing_artwork == 1) {
        const char *res_dir = deadbeef->get_system_dir (DDB_SYS_DIR_PIXMAP);
        char path[PATH_MAX];
        snprintf (path, sizeof (path), "%s/noartwork.png", res_dir);
        if (nocover_path == NULL || strcmp (path, nocover_path)) {
            free (nocover_path);
            nocover_path = strdup (path);
        }
    }
    else if (missing_artwork == 2) {
        const char *new_nocover_path = deadbeef->conf_get_str_fast ("artwork.nocover_path", NULL);
        if (nocover_path == NULL || !strings_equal (new_nocover_path, nocover_path)) {
            char *old_nocover_path = nocover_path;
            nocover_path = new_nocover_path ? strdup (new_nocover_path) : NULL;
            if (old_nocover_path) {
                free (old_nocover_path);
            }
        }
    }
    deadbeef->conf_unlock ();
}

static void
artwork_configchanged (void) {
    __block int need_clear_queue = 0;
    cache_configchanged ();
    dispatch_sync (sync_queue, ^{
        int old_artwork_enable_embedded = artwork_enable_embedded;
        int old_artwork_enable_local = artwork_enable_local;
        char *old_artwork_filemask = strdup (artwork_filemask ? artwork_filemask : "");
        char *old_artwork_folders = strdup (artwork_folders ? artwork_folders : "");
#ifdef USE_VFS_CURL
        int old_artwork_enable_lfm = artwork_enable_lfm;
#    if ENABLE_MUSICBRAINZ
        int old_artwork_enable_mb = artwork_enable_mb;
#    endif
#    if ENABLE_ALBUMART_ORG
        int old_artwork_enable_aao = artwork_enable_aao;
#    endif
        int old_artwork_enable_wos = artwork_enable_wos;
#endif
        int old_missing_artwork = missing_artwork;
        const char *old_nocover_path = nocover_path;

        int old_image_size = artwork_image_size;
        int old_simplified_cache = simplified_cache;

        _get_fetcher_preferences ();

        int cache_did_reset = 0;
        if (old_missing_artwork != missing_artwork || old_nocover_path != nocover_path) {
            trace ("artwork config changed, invalidating default artwork...\n");
            cache_did_reset = 1;
        }

        if (old_artwork_enable_embedded != artwork_enable_embedded || old_artwork_enable_local != artwork_enable_local
#ifdef USE_VFS_CURL
            || old_artwork_enable_lfm != artwork_enable_lfm
#    if ENABLE_MUSICBRAINZ
            || old_artwork_enable_mb != artwork_enable_mb
#    endif
#    if ENABLE_ALBUMART_ORG
            || old_artwork_enable_aao != artwork_enable_aao
#    endif
            || old_artwork_enable_wos != artwork_enable_wos
#endif
            || strcmp (old_artwork_filemask, artwork_filemask) || strcmp (old_artwork_folders, artwork_folders) ||
            cache_did_reset || old_image_size != artwork_image_size || old_simplified_cache != simplified_cache) {

            cache_reset_time = time (NULL);

            /* All artwork is now (including this second) obsolete */
            deadbeef->conf_set_int64 ("artwork.cache_reset_time", cache_reset_time);

            /* Wait for a new second to start before proceeding */
            while (time (NULL) == cache_reset_time) {
                usleep (100000);
            }

            cover_cache_free ();
            need_clear_queue = 1;
        }
        free (old_artwork_filemask);
        free (old_artwork_folders);
    });
    if (need_clear_queue) {
        queue_clear ();
        _notify_listeners (DDB_ARTWORK_SETTINGS_DID_CHANGE, NULL);
    }
}

static int
artwork_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        artwork_configchanged ();
        break;
    }
    return 0;
}

static int
invalidate_playitem_cache (DB_plugin_action_t *action, ddb_action_context_t ctx) {
    ddb_playlist_t *plt = deadbeef->action_get_playlist ();
    if (!plt)
        return -1;

    dispatch_async (process_queue, ^{
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                ddb_cover_info_t *cover = sync_cover_info_alloc ();
                _init_cover_metadata (cover, it);
                ddb_cover_info_t *cached_cover = cover_cache_find (cover);
                if (cached_cover) {
                    cover_cache_remove (cover);
                }

                if (cover->priv->album_cache_path[0]) {
                    remove_cache_item (cover->priv->album_cache_path);
                    (void)unlink (cover->priv->album_cache_path);
                }
                if (cover->priv->track_cache_path[0]) {
                    remove_cache_item (cover->priv->track_cache_path);
                    (void)unlink (cover->priv->track_cache_path);
                }

                cover_info_release (cover);

                _notify_listeners (DDB_ARTWORK_SETTINGS_DID_CHANGE, it);
            }
            deadbeef->pl_item_unref (it);
            it = deadbeef->pl_get_next (it, PL_MAIN);
        }
        deadbeef->plt_unref (plt);
    });

    return 0;
}

static DB_plugin_action_t *
artwork_get_actions (DB_playItem_t *it) {
    if (!it) {
        return NULL; // Only currently show for the playitem context menu
    }

    static DB_plugin_action_t context_action = { .title = "Refresh Cover Art",
                                                 .name = "invalidate_playitem_cache",
                                                 .callback2 = invalidate_playitem_cache,
                                                 .flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS |
                                                          DB_ACTION_ADD_MENU,
                                                 .next = NULL };

    return &context_action;
}

static void
artwork_plugin_stop (void (*completion_callback) (DB_plugin_t *plugin)) {
    queue_clear ();
    stop_cache_cleaner ();

    // lock semaphore
    for (int i = 0; i < FETCH_CONCURRENT_LIMIT; i++) {
        dispatch_semaphore_wait (fetch_semaphore, DISPATCH_TIME_FOREVER);
    }
    dispatch_async (process_queue, ^{
        dispatch_release (fetch_queue);
        fetch_queue = NULL;
        dispatch_release (process_queue);
        process_queue = NULL;

        // unlock semaphore
        for (int i = 0; i < FETCH_CONCURRENT_LIMIT; i++) {
            dispatch_semaphore_signal (fetch_semaphore);
        }
        dispatch_release (fetch_semaphore);
        fetch_semaphore = NULL;

        cover_cache_free ();

        cover_info_cleanup ();

        free (save_to_music_folders_filename);
        save_to_music_folders_filename = NULL;
        free (artwork_filemask);
        artwork_filemask = NULL;
        free (artwork_folders);
        artwork_folders = NULL;

        if (album_tf) {
            deadbeef->tf_free (album_tf);
            album_tf = NULL;
        }

        if (artist_tf) {
            deadbeef->tf_free (artist_tf);
            artist_tf = NULL;
        }

        if (title_tf) {
            deadbeef->tf_free (title_tf);
            title_tf = NULL;
        }

        if (albumartist_tf) {
            deadbeef->tf_free (albumartist_tf);
            albumartist_tf = NULL;
        }

        if (query_compare_tf) {
            deadbeef->tf_free (query_compare_tf);
            query_compare_tf = NULL;
        }

        if (track_cache_filename_tf) {
            deadbeef->tf_free (track_cache_filename_tf);
            track_cache_filename_tf = NULL;
        }

        if (album_cache_filename_tf) {
            deadbeef->tf_free (album_cache_filename_tf);
            album_cache_filename_tf = NULL;
        }

        if (simplified_album_cache_filename_tf) {
            deadbeef->tf_free (simplified_album_cache_filename_tf);
            simplified_album_cache_filename_tf = NULL;
        }

        dispatch_release (sync_queue);
        sync_queue = NULL;
        completion_callback (&plugin.plugin.plugin);
    });
}

static int
artwork_plugin_start (void) {
    deadbeef->plug_register_for_async_deinit(&plugin.plugin.plugin, artwork_plugin_stop);

    _get_fetcher_preferences ();
    cache_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);

    next_source_id = 1;

    sync_queue = dispatch_queue_create ("ArtworkSyncQueue", NULL);
    process_queue = dispatch_queue_create ("ArtworkProcessQueue", NULL);
    fetch_queue = dispatch_queue_create ("ArtworkFetchQueue", DISPATCH_QUEUE_CONCURRENT);
    fetch_semaphore = dispatch_semaphore_create (FETCH_CONCURRENT_LIMIT);

    start_cache_cleaner ();

    return 0;
}

static const char settings_dlg[] =
    "property \"Fetch from embedded tags\" checkbox artwork.enable_embedded 1;\n"
    "property \"Fetch from local folder\" checkbox artwork.enable_localfolder 1;\n"
#ifdef USE_VFS_CURL
    "property \"Fetch from Last.fm\" checkbox artwork.enable_lastfm 0;\n"
#    if ENABLE_MUSICBRAINZ
    "property \"Fetch from MusicBrainz\" checkbox artwork.enable_musicbrainz 0;\n"
#    endif
#    if ENABLE_ALBUMART_ORG
    "property \"Fetch from Albumart.org\" checkbox artwork.enable_albumartorg 0;\n"
#    endif
    "property \"Fetch from World of Spectrum (AY files)\" checkbox artwork.enable_wos 0;\n"
    "property \"Save downloaded files to music folders\" checkbox artwork.save_to_music_folders 0;\n"
#endif
// android doesn't display any image when cover is not found, and positioning algorithm is really different,
// therefore the below options are useless
#ifndef ANDROID
    "property \"When no artwork is found\" select[3] artwork.missing_artwork 1 \"leave blank\" \"use DeaDBeeF default cover\" \"display custom image\";"
    "property \"Custom image path\" file artwork.nocover_path \"\";\n"
    "property \"Save to file name\" entry artwork.save_to_music_folders_relative_path \"cover.jpg\";\n"
#endif
    "property \"Search masks (; separated)\" entry artwork.filemask \"" DEFAULT_FILEMASK "\";\n"
    "property \"Search folders (; separated)\" entry artwork.folders \"" DEFAULT_FOLDERS "\";\n"
// on android, cache is always off and music is saved to music folders by default
#ifndef ANDROID
    "property \"Cache refresh (hrs)\" spinbtn[0,1000,1] artwork.cache.expiration_time 0;\n"
#endif
    "property \"Simplified cache file names\" checkbox artwork.cache.simplified 0;\n"
    "property \"Image size\" spinbtn[64,2048,1] artwork.image_size 256;\n";

// define plugin interface
ddb_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = DDB_ARTWORK_MAJOR_VERSION,
    .plugin.plugin.version_minor = DDB_ARTWORK_MINOR_VERSION,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "artwork2",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork from embedded tags, local directories, or internet services",
    .plugin.plugin.copyright = "DeaDBeeF -- the music player\n"
                               "Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors\n"
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
                               "3. This notice may not be removed or altered from any source distribution.\n",
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.start = artwork_plugin_start,
    .plugin.plugin.configdialog = settings_dlg,
    .plugin.plugin.message = artwork_message,
    .plugin.plugin.get_actions = artwork_get_actions,
    .cover_get = cover_get,
    .reset = artwork_reset,
    .cover_info_release = sync_cover_info_release,
    .add_listener = artwork_add_listener,
    .remove_listener = artwork_remove_listener,
    .default_image_path = artwork_default_image_path,
    .allocate_source_id = artwork_allocate_source_id,
    .cancel_queries_with_source_id = artwork_cancel_queries_with_source_id,
};

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
