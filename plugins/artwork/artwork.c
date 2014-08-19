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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#ifdef __linux__
    #include <sys/prctl.h>
#endif
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <inttypes.h>
#if HAVE_SYS_CDEFS_H
    #include <sys/cdefs.h>
#endif
#if HAVE_SYS_SYSLIMITS_H
    #include <sys/syslimits.h>
#endif
#include "../../deadbeef.h"
#include "artwork.h"
#include "file_utils.h"
#include "escape.h"
#ifdef USE_METAFLAC
    #include <FLAC/metadata.h>
#endif

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static DB_artwork_plugin_t plugin;

DB_FILE *current_file;

#define MAX_CALLBACKS 200

typedef struct cover_callback_s {
    artwork_callback cb;
    void *ud;
} cover_callback_t;

typedef struct cover_query_s {
    char *fname;
    char *artist;
    char *album;
    int size;
    cover_callback_t callbacks[MAX_CALLBACKS];
    int numcb;
    struct cover_query_s *next;
} cover_query_t;

typedef struct mutex_cond_s {
    uintptr_t mutex;
    uintptr_t cond;
} mutex_cond_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static uintptr_t mutex;
static uintptr_t cond;
static volatile int terminate;
static volatile int clear_queue;
static intptr_t tid;
static uintptr_t culler_mutex;

static int artwork_enable_embedded;
static int artwork_enable_local;
#ifdef USE_VFS_CURL
    static int artwork_enable_lfm;
    static int artwork_enable_aao;
    static int artwork_enable_wos;
#endif
static int scale_towards_longer;
static time_t artwork_reset_time;
static time_t artwork_scaled_reset_time;

#define DEFAULT_FILEMASK "*cover*.jpg;*front*.jpg;*folder*.jpg;*cover*.png;*front*.png;*folder*.png"
#define FALLBACK_FILEMASK ";*.jpg;*.jpeg"
#define MAX_FILEMASK_CONFIG_LENGTH 200
#define MAX_FILEMASK_LENGTH 215
static char artwork_filemask[MAX_FILEMASK_LENGTH];

static char default_cover[PATH_MAX];

static const char *get_default_cover (void) {
    return default_cover;
}

static int
make_cache_root_path(char *path, const size_t size)
{
    const char *xdg_cache = getenv("XDG_CACHE_HOME");
    const char *cache_root = xdg_cache ? xdg_cache : getenv("HOME");
    return snprintf(path, size, xdg_cache ? "%s/deadbeef/" : "%s/.cache/deadbeef/", cache_root);
}

static int
filter_scaled_dirs (const struct dirent *f)
{
    return !strncasecmp(f->d_name, "covers-", 7);
}

static void
culler_thread (void *none)
{
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-artwork-gc", 0, 0, 0, 0);
#endif

    sleep(5);

    /* Find where it all happens */
    char cache_root_path[PATH_MAX];
    make_cache_root_path(cache_root_path, PATH_MAX-7);
    char covers_path[PATH_MAX];
    strcpy(covers_path, cache_root_path);
    strcat(covers_path, "covers/");

    while (true) {
        /* Loop through all the artist directories */
        DIR *covers_dir = opendir(covers_path);
        struct dirent covers_subdir;
        struct dirent *covers_result;
        while (covers_dir && !readdir_r(covers_dir, &covers_subdir, &covers_result) && covers_result) {
            const int32_t cache_secs = deadbeef->conf_get_int("artwork.cache.period", 48) * 60 * 60;
            if (cache_secs > 0 && strcmp(covers_subdir.d_name, ".") && strcmp(covers_subdir.d_name, "..")) {
                const time_t cache_expiry = time(NULL) - cache_secs;

                /* Loop through all the image files in this artist directory */
                char subdir_path[PATH_MAX];
                snprintf(subdir_path, PATH_MAX, "%s%s", covers_path, covers_subdir.d_name);
                DIR *subdir = opendir(subdir_path);
                struct dirent entry;
                struct dirent *entry_result;
                while (subdir && !readdir_r(subdir, &entry, &entry_result) && entry_result) {
                    if (strcmp(entry.d_name, ".") && strcmp(entry.d_name, "..")) {
                        char entry_path[PATH_MAX];
                        snprintf(entry_path, PATH_MAX, "%s/%s", subdir_path, entry.d_name);

                        /* Test against the time the cache expired (cache invalidation resets are not handled here) */
                        struct stat stat_buf;
                        if (!stat(entry_path, &stat_buf) && stat_buf.st_mtime < cache_expiry) {
                            deadbeef->mutex_lock(culler_mutex);

                            /* Unlink the expired file, and the artist directory if it is empty */
                            unlink(entry_path);
                            rmdir(subdir_path);
                            fprintf(stderr, "%s expired from cache\n", entry_path);

                            /* Remove any scaled copies of this file, plus directories that are now empty */
                            struct dirent **scaled_dirs = NULL;
                            const int scaled_dirs_count = scandir(cache_root_path, &scaled_dirs, filter_scaled_dirs, NULL);
                            for (size_t i = 0; i < scaled_dirs_count; i++) {
                                char scaled_entry_path[PATH_MAX];
                                snprintf(scaled_entry_path, PATH_MAX, "%s%s/%s/%s", cache_root_path, scaled_dirs[i]->d_name, covers_subdir.d_name, entry.d_name);
                                unlink(scaled_entry_path);
                                rmdir(dirname(scaled_entry_path));
                                rmdir(dirname(scaled_entry_path));
                                free(scaled_dirs[i]);
                            }
                            free(scaled_dirs);

                            deadbeef->mutex_unlock(culler_mutex);
                        }
                    }
                }

                closedir(subdir);
            }

            sleep(1);
        }

        closedir(covers_dir);
        sleep(60);
        covers_dir = opendir(covers_path);
    }
}

static char
esc_char (char c) {
    if (c < 1
        || (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == ' '
        || c == '_'
        || c == '-') {
        return c;
    }
    return '_';
}

static int
make_cache_dir_path (char *path, int size, const char *artist, int img_size) {
    char esc_artist[PATH_MAX];
    int i;

    if (artist) {
        for (i = 0; artist[i] && i < PATH_MAX-1; i++) {
            esc_artist[i] = esc_char (artist[i]);
        }
        esc_artist[i] = '\0';
    }
    else {
        strcpy (esc_artist, "Unknown artist");
    }

    int sz = make_cache_root_path(path, size);
    path += sz;
    sz += snprintf (path, size-sz, img_size == -1 ? "covers/%2$s" : "covers-%1$d/%2$s", img_size, esc_artist);

    for (char *p = path+sz-strlen(esc_artist); *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
    return sz;
}

static int
make_cache_path2 (char *path, int size, const char *fname, const char *album, const char *artist, int img_size) {
    *path = 0;

    int unk = 0;
    int unk_artist = 0;

    if (!album || !(*album)) {
        album = "Unknown album";
        unk = 1;
    }
    if (!artist || !(*artist)) {
        artist = "Unknown artist";
        unk_artist = 1;
    }

    if (unk)
    {
        if (fname) {
            album = fname;
        }
        else if (!unk_artist) {
            album = artist;
        }
        else {
            trace ("not possible to get any unique album name\n");
            return -1;
        }
    }

    char *p = path;
    char esc_album[PATH_MAX];
    const char *palbum = album;
    size_t l = strlen (album);
    if (l > 200) {
        palbum = album + l - 200;
    }
    int i;
    for (i = 0; palbum[i]; i++) {
        esc_album[i] = esc_char (palbum[i]);
    }
    esc_album[i] = 0;

    int sz = make_cache_dir_path (path, size, artist, img_size);
    size -= sz;
    path += sz;
    sz = snprintf (path, size, "/%s.jpg", esc_album);
    for (char *p = path+1; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
}

static void
make_cache_path (char *path, int size, const char *album, const char *artist, int img_size) {
    make_cache_path2 (path, size, NULL, album, artist, img_size);
}

static void
queue_add (const char *fname, const char *artist, const char *album, int img_size, artwork_callback callback, void *user_data) {
    if (!artist) {
        artist = "";
    }
    if (!album) {
        album = "";
    }
    deadbeef->mutex_lock (mutex);

    for (cover_query_t *q = queue; q; q = q->next) {
        if (!strcasecmp (artist, q->artist) && !strcasecmp (album, q->album) && (q->size == -1 || img_size == -1)) {
            if (q->size == -1 && img_size != -1) {
                trace("Already in queue - set size to %d\n", img_size);
                q->size = img_size;
            }
            if (q->numcb < MAX_CALLBACKS && callback) {
                trace("Already in queue - add to callbacks\n");
                q->callbacks[q->numcb].cb = callback;
                q->callbacks[q->numcb].ud = user_data;
                q->numcb++;
            }
            deadbeef->mutex_unlock (mutex);
            return;
        }
    }

    trace("artwork:queue_add %s %s %s %d\n", fname, artist, album, img_size);
    cover_query_t *q = malloc (sizeof (cover_query_t));
    memset (q, 0, sizeof (cover_query_t));
    q->fname = strdup (fname);
    q->artist = strdup (artist);
    q->album = strdup (album);
    q->size = img_size;
    q->callbacks[q->numcb].cb = callback;
    q->callbacks[q->numcb].ud = user_data;
    q->numcb++;
    if (queue_tail) {
        queue_tail->next = q;
        queue_tail = q;
    }
    else {
        queue = queue_tail = q;
    }
    deadbeef->mutex_unlock (mutex);
    deadbeef->cond_signal (cond);
}

static void
queue_pop (void) {
    deadbeef->mutex_lock (mutex);
    cover_query_t *next = queue ? queue->next : NULL;
    if (queue) {
        if (queue->fname) {
            free (queue->fname);
        }
        if (queue->artist) {
            free (queue->artist);
        }
        if (queue->album) {
            free (queue->album);
        }
        for (int i = 0; i < queue->numcb; i++) {
            if (queue->callbacks[i].cb) {
                queue->callbacks[i].cb (NULL, NULL, NULL, queue->callbacks[i].ud);
            }
        }
        free (queue);
    }
    queue = next;
    if (!queue) {
        queue_tail = NULL;
    }
    deadbeef->mutex_unlock (mutex);
}

static char *filter_custom_mask = NULL;

static int
filter_custom (const struct dirent *f)
{
// FNM_CASEFOLD is not defined on solaris. On other platforms it is.
// It should be safe to define it as FNM_INGORECASE if it isn't defined.
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD FNM_IGNORECASE
#endif
    if (!fnmatch (filter_custom_mask, f->d_name, FNM_CASEFOLD)) {
        return 1;
    }
    return 0;
}

static char *local_image_file(const char *filename)
{
    char filename_dir[PATH_MAX];
    strncpy(filename_dir, filename, sizeof(filename_dir));
    filename_dir[PATH_MAX-1] = '\0';
    dirname(filename_dir);
    trace("scanning %s for artwork\n", filename_dir);

    struct dirent **files;
    int files_count = 0;
    char *p = artwork_filemask;
    while (files_count == 0 && p) {
        char mask[MAX_FILEMASK_LENGTH] = "";
        char *e = strchr(p, ';');
        if (e) {
            strncpy(mask, p, e-p);
            mask[e-p] = '\0';
            e++;
        }
        else {
            strcpy(mask, p);
        }
        if (mask[0]) {
            filter_custom_mask = mask;
            files_count = scandir(filename_dir, &files, filter_custom, NULL);
        }
        p = e;
    }

    if (files_count == 0) {
        trace("No cover art files in local folder\n");
        return NULL;
    }

    char *artwork_path = malloc(strlen(filename_dir) + 1 + strlen(files[0]->d_name) + 1);
    if (artwork_path) {
        trace("found cover %s in local folder\n", files[0]->d_name);
        strcpy(artwork_path, filename_dir);
        strcat(artwork_path, "/");
        strcat(artwork_path, files[0]->d_name);
    }

    for (int i = 0; i < files_count; i++) {
        free(files[i]);
    }
    free(files);

    return artwork_path;
}

static const uint8_t *
id3v2_skip_str (const int enc, const uint8_t *ptr, const uint8_t *end) {
    if (enc == 0 || enc == 3) {
        while (ptr < end && *ptr) {
            ptr++;
        }
        ptr++;
        if (ptr >= end) {
            return NULL;
        }
        return ptr;
    }
    else {
        while (ptr < end-1 && (ptr[0] || ptr[1])) {
            ptr += 2;
        }
        ptr += 2;
        if (ptr >= end) {
            return NULL;
        }
        return ptr;
    }
    return NULL;
}

static const uint8_t *id3v2_artwork(const DB_id3v2_frame_t *f, const int minor_version)
{
    if (strcmp (f->id, "APIC")) {
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
#if 0
    printf ("version: %d, flags: %d %d\n", minor_version, (int)f->flags[0], (int)f->flags[1]);
    for (int i = 0; i < 20; i++) {
        printf ("%c", data[i] < 0x20 ? '?' : data[i]);
    }
    printf ("\n");
    for (int i = 0; i < 20; i++) {
        printf ("%02x ", data[i]);
    }
    printf ("\n");
#endif
    const uint8_t *end = f->data + f->size;
    int enc = *data;
    data++; // enc
    // mime-type must always be ASCII - hence enc is 0 here
    const uint8_t *mime_end = id3v2_skip_str (enc, data, end);
    if (!mime_end) {
        trace ("artwork: corrupted id3v2 APIC frame\n");
        return NULL;
    }
    if (strcasecmp(data, "image/jpeg") &&
#ifdef USE_IMLIB2
        strcasecmp(data, "image/gif") &&
        strcasecmp(data, "image/tiff") &&
#endif
        strcasecmp(data, "image/png")) {
        trace ("artwork: unsupported mime type: %s\n", data);
        return NULL;
    }
    if (*mime_end != 3) {
        trace ("artwork: picture type=%d\n", *mime_end);
        return NULL;
    }
    trace ("artwork: mime-type=%s, picture type: %d\n", data, *mime_end);
    data = mime_end;
    data++; // picture type
    data = id3v2_skip_str (enc, data, end); // description
    if (!data) {
        trace ("artwork: corrupted id3v2 APIC frame\n");
        return NULL;
    }

    return data;
}

static const uint8_t *apev2_artwork(const DB_apev2_frame_t *f)
{
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

    const int sz = end - ++data;
    if (sz < 20) {
        trace ("artwork: apev2 cover art frame is too small\n");
        return NULL;
    }

    uint8_t *ext = strrchr (f->data, '.');
    if (!ext || !*++ext) {
        trace ("artwork: apev2 cover art name has no extension\n");
        return NULL;
    }

    if (strcasecmp(ext, "jpeg") &&
        strcasecmp(ext, "jpg") &&
#ifdef USE_IMLIB2
        strcasecmp(ext, "gif") &&
        strcasecmp(ext, "tif") &&
        strcasecmp(ext, "tiff") &&
#endif
        strcasecmp(ext, "png")) {
        trace ("artwork: unsupported file type: %s\n", ext);
        return NULL;
    }

    return data;
}

#ifdef USE_METAFLAC
static size_t
flac_io_read (void *ptr, size_t size, size_t nmemb, FLAC__IOHandle handle) {
    return deadbeef->fread (ptr, size, nmemb, (DB_FILE *)handle);
}

static int
flac_io_seek (FLAC__IOHandle handle, FLAC__int64 offset, int whence) {
    return deadbeef->fseek ((DB_FILE *)handle, offset, whence);
}

static FLAC__int64
flac_io_tell (FLAC__IOHandle handle) {
    return deadbeef->ftell ((DB_FILE *)handle);
}

static int
flac_io_eof (FLAC__IOHandle handle) {
    int64_t pos = deadbeef->ftell ((DB_FILE *)handle);
    return pos == deadbeef->fgetlength ((DB_FILE *)handle);
}

static int
flac_io_close (FLAC__IOHandle handle) {
    deadbeef->fclose ((DB_FILE *)handle);
    return 0;
}

static FLAC__StreamMetadata_Picture *flac_extract_art(FLAC__Metadata_Chain *chain, const char *filename)
{
    DB_FILE *file = deadbeef->fopen (filename);
    if (!file) {
        trace ("artwork: failed to open %s\n", filename);
        return NULL;
    }

    FLAC__IOCallbacks iocb = {
        .read = flac_io_read,
        .write = NULL,
        .seek = flac_io_seek,
        .tell = flac_io_tell,
        .eof = NULL,
        .close = NULL
    };
    int res = FLAC__metadata_chain_read_with_callbacks(chain, (FLAC__IOHandle)file, iocb);
#if USE_OGG
    if (!res && FLAC__metadata_chain_status(chain) == FLAC__METADATA_SIMPLE_ITERATOR_STATUS_NOT_A_FLAC_FILE) {
        res = FLAC__metadata_chain_read_ogg_with_callbacks(chain, (FLAC__IOHandle)file, iocb);
    }
#endif
    deadbeef->fclose (file);
    if (!res) {
        trace ("artwork: failed to read metadata from flac (not flac file?): %s\n", filename);
        return NULL;
    }

    FLAC__StreamMetadata *picture = 0;
    FLAC__Metadata_Iterator *iterator = FLAC__metadata_iterator_new();
    if (!iterator) {
        return NULL;
    }
    FLAC__metadata_iterator_init(iterator, chain);
    do {
        FLAC__StreamMetadata *block = FLAC__metadata_iterator_get_block(iterator);
        if (block->type == FLAC__METADATA_TYPE_PICTURE) {
            picture = block;
        }
    } while(FLAC__metadata_iterator_next(iterator) && 0 == picture);
    FLAC__metadata_iterator_delete(iterator);
    if (!picture) {
        trace ("%s doesn't have an embedded cover\n", filename);
        return NULL;
    }

    return &picture->data.picture;
}
#endif

#ifdef USE_VFS_CURL
#define BASE_URL "http://ws.audioscrobbler.com/2.0/"
#define API_KEY "6b33c8ae4d598a9aff8fe63e334e6e86"
static int
fetch_from_lastfm (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = uri_escape(artist, 0);
    char *album_url = uri_escape(album, 0);
    snprintf (url, sizeof (url), BASE_URL "?method=album.getinfo&api_key=" API_KEY "&artist=%s&album=%s", artist_url, album_url);
    free (artist_url);
    free (album_url);

    DB_FILE *fp = deadbeef->fopen (url);
    if (!fp) {
        trace ("fetch_from_lastfm: failed to open %s\n", url);
        return -1;
    }
    current_file = fp;

    const char searchstr[] = "<image size=\"extralarge\">";
    char buffer[1000];
    memset (buffer, 0, sizeof (buffer));
    char *img = NULL;
    int size = deadbeef->fread (buffer, 1, sizeof (buffer)-1, fp);
    if (size > 0) {
        img = strstr (buffer, searchstr);
    }
    current_file = NULL;
    deadbeef->fclose (fp);

    if (!img) {
        trace ("fetch_from_lastfm: image url not found in response from %s\n", url);
        return -1;
    }

    img += sizeof (searchstr)-1;

    char *end = strstr (img, "</image>");
    if (!end || end == img) {
        char *fixed_alb = strdup(album);
        char *openp = strchr (fixed_alb, '(');
        if (openp && openp != fixed_alb) {
            *openp = '\0';
            return fetch_from_lastfm (artist, fixed_alb, dest);
        }

        if (artist != album) {
            return fetch_from_lastfm (album, album, dest);
        }

        trace ("fetch_from_lastfm: bad xml (or image not found) from %s\n", url);
        return -1;
    }


    *end = 0;
    return copy_file(img, dest);
}

static int
fetch_from_albumart_org (const char *artist, const char *album, const char *dest)
{
    char url [1024];
    char *artist_url = uri_escape (artist, 0);
    char *album_url = uri_escape (album, 0);
    snprintf (url, sizeof (url), "http://www.albumart.org/index.php?searchkey=%s+%s&itempage=1&newsearch=1&searchindex=Music", artist_url, album_url);
    free (artist_url);
    free (album_url);

    DB_FILE *fp = deadbeef->fopen (url);
    if (!fp) {
        trace ("fetch_from_albumart_org: failed to open %s\n", url);
        return -1;
    }
    current_file = fp;
    char buffer[10000];
    memset (buffer, 0, sizeof (buffer));
    char *img = NULL;
    int size = deadbeef->fread (buffer, 1, sizeof (buffer), fp);
    if (size > 0) {
        img = strstr (buffer, "http://ecx.images-amazon.com/images/I/");
    }
    current_file = NULL;
    deadbeef->fclose (fp);

    if (!img) {
        trace ("fetch_from_albumart_org: image url not found in response from %s (%d bytes)\n", url, size);
        return -1;
    }

    char *end = strstr (img, "._SL160_");
    if (!end || end == img)
    {
        trace ("fetch_from_albumart_org: bad xml from %s\n", url);
        return -1;
    }

    strcpy (end, ".jpg");
    return copy_file(img, dest);
}

static void
strcopy_escape (char *dst, int d_len, const char *src, int n) {
    char *e = dst + d_len - 1;
    const char *se = src + n;
    while (dst < e && *src && src < se) {
        if (*src != ' ' && *src != '!') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = 0;
}

static int
fetch_from_wos (const char *title, const char *dest)
{
    // extract game title from title
    char t[100];
    char *dash = strstr (title, " -");
    if (!dash) {
        strcopy_escape (t, sizeof (t), title, strlen (title));
    }
    else {
        strcopy_escape(t, sizeof (t), title, dash-title);
    }
    char *sp;
    while (sp = strchr(t, ' ')) {
        *sp = '_';
    }
    char *title_url = uri_escape (t, 0);
    char url [1024];
    snprintf (url, sizeof (url), "http://www.worldofspectrum.org/showscreen.cgi?screen=screens/load" "/%c/gif/%s.gif", tolower (title_url[0]), title_url);
    free (title_url);
    trace ("WOS request: %s\n", url);
    return copy_file(url, dest);
}
#endif

static void
fetcher_thread (void *none)
{
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-artwork", 0, 0, 0, 0);
#endif
    while (!terminate) {
        trace ("artwork: waiting for signal\n");
        deadbeef->cond_wait (cond, mutex);
        trace ("artwork: cond signalled\n");
        deadbeef->mutex_unlock (mutex);
        while (!terminate && queue && !clear_queue) {
            cover_query_t *param = queue;

            char cache_path[1024];
            make_cache_path2 (cache_path, sizeof (cache_path), param->fname, param->album, param->artist, -1);
            trace ("fetching cover for %s %s to %s\n", param->album, param->artist, cache_path);
struct timeval timeval;
gettimeofday(&timeval, NULL);
int usecs = timeval.tv_sec*1000000 + timeval.tv_usec;

            int got_pic = 0;
            struct stat placeholder_stat;
            const int placeholder_exists = !stat(cache_path, &placeholder_stat);

            if (deadbeef->is_local_file(param->fname) && !(placeholder_exists && placeholder_stat.st_mtime + 10 > time(NULL))) {
                if (artwork_enable_embedded) {
                    // try to load embedded from id3v2
                    trace("trying to load artwork from id3v2 tag for %s\n", param->fname);
                    DB_id3v2_tag_t tag;
                    memset (&tag, 0, sizeof (tag));
                    DB_FILE *fp = deadbeef->fopen (param->fname);
                    if (fp && !deadbeef->junk_id3v2_read_full (NULL, &tag, fp)) {
                        const int minor_version = tag.version[0];
                        for (DB_id3v2_frame_t *f = tag.frames; f && !got_pic; f = f->next) {
                            const uint8_t *image_data = id3v2_artwork(f, minor_version);
                            if (image_data) {
                                const size_t sz = f->size - (image_data - f->data);
                                trace("found id3v2 cover art of %d bytes (%s)\n", sz);
                                trace("will write id3v2 APIC into %s\n", cache_path);
                                if (!write_file(cache_path, image_data, sz) == 0) {
                                    got_pic = 1;
                                }
                            }
                        }

                        deadbeef->junk_id3v2_free (&tag);
                        deadbeef->fclose (fp);
                    }

                    if (!got_pic) {
                        // try to load embedded from apev2
                        trace("trying to load artwork from apev2 tag for %s\n", param->fname);
                        DB_apev2_tag_t tag;
                        memset (&tag, 0, sizeof (tag));
                        DB_FILE *fp = deadbeef->fopen (param->fname);
                        if (fp && !deadbeef->junk_apev2_read_full (NULL, &tag, fp)) {
                            for (DB_apev2_frame_t *f = tag.frames; f; f = f->next) {
                                const uint8_t *image_data = apev2_artwork(f);
                                if (image_data) {
                                    const size_t sz = f->size - (image_data - f->data);
                                    trace("found apev2 cover art of %d bytes (%s)\n", sz);
                                    trace("will write apev2 cover art into %s\n", cache_path);
                                    if (!write_file(cache_path, image_data, sz) == 0) {
                                        got_pic = 1;
                                    }
                                    break;
                                }
                            }

                            deadbeef->junk_apev2_free (&tag);
                            deadbeef->fclose (fp);
                        }
                    }

#ifdef USE_METAFLAC
                    if (!got_pic) {
                        // try to load embedded from flac metadata
                        trace("trying to load artwork from Flac tag for %s\n", param->fname);
                        FLAC__Metadata_Chain *chain = FLAC__metadata_chain_new();
                        if (chain) {
                            FLAC__StreamMetadata_Picture *pic = flac_extract_art(chain, param->fname);
                            if (pic) {
                                trace("found flac cover art of %d bytes (%s)\n", pic->data_length, pic->description);
                                trace("will write flac cover art into %s\n", cache_path);
                                if (!write_file(cache_path, pic->data, pic->data_length)) {
                                    got_pic = 1;
                                }
                            }
                            FLAC__metadata_chain_delete(chain);
                        }
                    }
#endif
                }

                if (!got_pic && artwork_enable_local) {
                    /* Searching in track directory */
                    char *artwork = local_image_file(param->fname);
                    if (artwork) {
                        copy_file(artwork, cache_path);
                        free(artwork);
                        got_pic = 1;
                    }
                }

                if (!got_pic) {
                    write_file(cache_path, NULL, 0);
                }
            }

#ifdef USE_VFS_CURL
            if (!got_pic && !(placeholder_exists && placeholder_stat.st_mtime + 3600 > time(NULL))) {
                if (artwork_enable_wos) {
                    char *dot = strrchr (param->fname, '.');
                    if (dot && !strcasecmp (dot, ".ay") && !fetch_from_wos (param->album, cache_path)) {
                        got_pic = 1;
                    }
                }

                if (!got_pic && artwork_enable_lfm && !fetch_from_lastfm (param->artist, param->album, cache_path)) {
                    got_pic = 1;
                }

                if (!got_pic && artwork_enable_aao && !fetch_from_albumart_org (param->artist, param->album, cache_path)) {
                    got_pic = 1;
                }

                if (!got_pic) {
                    write_file(cache_path, NULL, 0);
                }
            }
#endif
            if (got_pic) {
                trace("Cached art for %s %s\n", param->album, param->artist);
                if (param->size != -1) {
                    got_pic = 0;
                    char scaled_path[1024];
                    make_cache_path2(scaled_path, sizeof(scaled_path), param->fname, param->album, param->artist, param->size);
                    trace ("scaled cache: %s\n", scaled_path);
                    if (ensure_dir(scaled_path) && !scale_file(cache_path, scaled_path, param->size)) {
                        got_pic = 1;
                    }
                }
            }

            if (got_pic) {
                for (int i = 0; i < param->numcb; i++) {
                    if (param->callbacks[i].cb) {
                        param->callbacks[i].cb (param->fname, param->artist, param->album, param->callbacks[i].ud);
                        param->callbacks[i].cb = NULL;
                    }
                }
            }

            queue_pop ();
gettimeofday(&timeval, NULL);
usecs = timeval.tv_sec*1000000 + timeval.tv_usec - usecs;
fprintf(stderr, "%d micro-seconds\n", usecs);
        }

        if (clear_queue) {
            trace ("artwork: received queue clear request\n");
            while (queue) {
                queue_pop ();
            }
            clear_queue = 0;
            trace ("artwork: queue clear done\n");
        }
    }
}

static char *
find_image (const char *path, const int scaled) {
    struct stat stat_buf;
    if (!stat(path, &stat_buf)) {
        const time_t reset_time = scaled ? artwork_scaled_reset_time : artwork_reset_time;
        if (stat_buf.st_mtime < reset_time) {
            trace("deleting cached file %s\n", path);
            unlink(path);
            return NULL;
        }

        return stat_buf.st_size == 0 ? NULL : strdup(path);
    }

    return NULL;
}

static char*
get_album_art (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data)
{
    char path [1024];

    make_cache_path2 (path, sizeof (path), fname, album, artist, size);
    char *p = find_image (path, size == -1 ? 0 : 1);
    if (p && *p) {
        if (callback) {
            callback (NULL, NULL, NULL, user_data);
        }
        return p;
    }

    if (size != -1) {
        // check if we have unscaled image
        char unscaled_path[1024];
        make_cache_path2 (unscaled_path, sizeof (unscaled_path), fname, album, artist, -1);
        p = find_image (unscaled_path, 0);
        if (p && *p) {
            free (p);
            if (ensure_dir (unscaled_path) && !scale_file (unscaled_path, path, size)) {
                if (callback) {
                    callback (NULL, NULL, NULL, user_data);
                }
                return strdup (path);
            }
        }
    }

    queue_add (fname, artist, album, size, callback, user_data);
    return NULL;
}

static void
sync_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    mutex_cond_t *mc = (mutex_cond_t *)user_data;
    deadbeef->mutex_lock (mc->mutex);
    deadbeef->cond_signal (mc->cond);
    deadbeef->mutex_unlock (mc->mutex);
}

static char*
get_album_art_sync (const char *fname, const char *artist, const char *album, int size) {
    mutex_cond_t mc;
    mc.mutex = deadbeef->mutex_create ();
    mc.cond = deadbeef->cond_create ();
    deadbeef->mutex_lock (mc.mutex);
    char *image_fname = get_album_art (fname, artist, album, size, sync_callback, &mc);
    while (!image_fname) {
        deadbeef->cond_wait (mc.cond, mc.mutex);
        image_fname = get_album_art (fname, artist, album, size, sync_callback, &mc);
    }
    deadbeef->mutex_unlock (mc.mutex);
    deadbeef->mutex_free (mc.mutex);
    deadbeef->cond_free (mc.cond);
    return image_fname;
}

static void
artwork_reset (int fast) {
    if (fast) {
//        if (current_file) {
//            deadbeef->fabort (current_file);
//        }
        deadbeef->mutex_lock (mutex);
        while (queue && queue->next) {
            cover_query_t *next = queue->next->next;
            free (queue->next->fname);
            free (queue->next->artist);
            free (queue->next->album);
            for (int i = 0; i < queue->next->numcb; i++) {
                if (queue->next->callbacks[i].cb == sync_callback) {
                    sync_callback (NULL, NULL, NULL, queue->next->callbacks[i].ud);
                }
            }
            queue->next = next;
            if (next == NULL) {
                queue_tail = queue;
            }
        }
        deadbeef->mutex_unlock (mutex);
    }
    else {
        trace ("artwork: reset\n");
        clear_queue = 1;
        deadbeef->cond_signal (cond);
        trace ("artwork: waiting for clear to complete\n");
        while (clear_queue) {
            usleep (100000);
        }
    }
}

static void
artwork_configchanged (void) {
    int new_artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    int new_artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
#ifdef USE_VFS_CURL
    int new_artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    int new_artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    int new_artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif

    char new_artwork_filemask[MAX_FILEMASK_LENGTH];
    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, new_artwork_filemask, MAX_FILEMASK_CONFIG_LENGTH);
    strcat(new_artwork_filemask, FALLBACK_FILEMASK);

    if (new_artwork_enable_embedded != artwork_enable_embedded
            || new_artwork_enable_local != artwork_enable_local
#ifdef USE_VFS_CURL
            || new_artwork_enable_lfm != artwork_enable_lfm
            || new_artwork_enable_aao != artwork_enable_aao
            || new_artwork_enable_wos != artwork_enable_wos
#endif
            || strcmp (new_artwork_filemask, artwork_filemask)) {
        trace ("artwork config changed, invalidating cache...\n");
        artwork_enable_embedded = new_artwork_enable_embedded;
        artwork_enable_local = new_artwork_enable_local;
#ifdef USE_VFS_CURL
        artwork_enable_lfm = new_artwork_enable_lfm;
        artwork_enable_aao = new_artwork_enable_aao;
        artwork_enable_wos = new_artwork_enable_wos;
#endif
        artwork_reset_time = time (NULL);
        deadbeef->conf_set_int64 ("artwork.cache_reset_time", artwork_reset_time);
        artwork_scaled_reset_time = time (NULL);
        deadbeef->conf_set_int64 ("artwork.scaled.cache_reset_time", artwork_scaled_reset_time);
        strcpy (artwork_filemask, new_artwork_filemask);
        artwork_reset (0);
        deadbeef->sendmessage (DB_EV_PLAYLIST_REFRESH, 0, 0, 0);
    }

    int new_scale_towards_longer = deadbeef->conf_get_int ("artwork.scale_towards_longer", 1);
    if (new_scale_towards_longer != scale_towards_longer) {
        trace ("artwork config changed, invalidating scaled cache...\n");
        artwork_scaled_reset_time = time (NULL);
        deadbeef->conf_set_int64 ("artwork.scaled.cache_reset_time", artwork_scaled_reset_time);
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
artwork_plugin_start (void)
{
    deadbeef->conf_lock ();

    const char *def_art = deadbeef->conf_get_str_fast ("gtkui.nocover_pixmap", NULL);
    if (!def_art) {
        snprintf (default_cover, sizeof (default_cover), "%s/noartwork.png", deadbeef->get_pixmap_dir ());
    }
    else {
        strcpy (default_cover, def_art);
    }
    terminate = 0;

    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
#ifdef USE_VFS_CURL
    artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    artwork_enable_wos = deadbeef->conf_get_int ("artwork.enable_wos", 0);
#endif
    artwork_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);
    artwork_scaled_reset_time = deadbeef->conf_get_int64 ("artwork.scaled.cache_reset_time", 0);

    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, artwork_filemask, MAX_FILEMASK_CONFIG_LENGTH);
    strcat(artwork_filemask, FALLBACK_FILEMASK);

    deadbeef->conf_unlock ();

    artwork_filemask[MAX_FILEMASK_LENGTH-1] = 0;

    mutex = deadbeef->mutex_create_nonrecursive ();
    culler_mutex = deadbeef->mutex_create_nonrecursive ();
#ifdef USE_IMLIB2
    imlib_mutex = deadbeef->mutex_create_nonrecursive ();
#endif
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority (fetcher_thread, NULL);
    deadbeef->thread_start_low_priority (culler_thread, NULL);

    return 0;
}

static int
artwork_plugin_stop (void)
{
    if (current_file) {
        deadbeef->fabort (current_file);
    }
    if (tid) {
        terminate = 1;
        deadbeef->cond_signal (cond);
        deadbeef->thread_join (tid);
        tid = 0;
    }
    while (queue) {
        queue_pop ();
    }
    if (mutex) {
        deadbeef->mutex_free (mutex);
        mutex = 0;
    }
    if (culler_mutex) {
        deadbeef->mutex_free (culler_mutex);
        culler_mutex = 0;
    }
#ifdef USE_IMLIB2
    if (imlib_mutex) {
        deadbeef->mutex_free (imlib_mutex);
        imlib_mutex = 0;
    }
#endif
    if (cond) {
        deadbeef->cond_free (cond);
        cond = 0;
    }

    return 0;
}

static const char settings_dlg[] =
    "property \"Cache update period (hr)\" entry artwork.cache.period 48;\n"
    "property \"Fetch from embedded tags\" checkbox artwork.enable_embedded 1;\n"
    "property \"Fetch from local folder\" checkbox artwork.enable_localfolder 1;\n"
    "property \"Local cover file mask\" entry artwork.filemask \"" DEFAULT_FILEMASK "\";\n"
#ifdef USE_VFS_CURL
    "property \"Fetch from last.fm\" checkbox artwork.enable_lastfm 0;\n"
    "property \"Fetch from albumart.org\" checkbox artwork.enable_albumartorg 0;\n"
    "property \"Fetch from worldofspectrum.org (AY only)\" checkbox artwork.enable_wos 0;\n"
#endif
    "property \"Scale artwork towards longer side\" checkbox artwork.scale_towards_longer 1;\n"
;

// define plugin interface
static DB_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = 1,
    .plugin.plugin.api_vminor = 0,
    .plugin.plugin.version_major = 1,
    .plugin.plugin.version_minor = 2,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "artwork",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork either from local directories or from internet",
    .plugin.plugin.copyright =
        "Album Art plugin for DeaDBeeF\n"
        "Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>\n"
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.start = artwork_plugin_start,
    .plugin.plugin.stop = artwork_plugin_stop,
    .plugin.plugin.configdialog = settings_dlg,
    .plugin.plugin.message = artwork_message,
    .get_album_art = get_album_art,
    .reset = artwork_reset,
    .get_default_cover = get_default_cover,
    .get_album_art_sync = get_album_art_sync,
    .make_cache_path = make_cache_path,
    .make_cache_path2 = make_cache_path2,
};

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
