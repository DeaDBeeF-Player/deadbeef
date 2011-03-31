#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <fnmatch.h>
#include <inttypes.h>
#include <Imlib2.h>
#include "../../deadbeef.h"
#include "artwork.h"
#include "lastfm.h"
#include "albumartorg.h"

#define min(x,y) ((x)<(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static char default_cover[PATH_MAX];
#define DEFAULT_FILEMASK "*cover*.jpg;*front*.jpg"

static DB_artwork_plugin_t plugin;
DB_functions_t *deadbeef;

DB_FILE *current_file;

typedef struct cover_query_s {
    char *fname;
    char *artist;
    char *album;
    int size;
    artwork_callback callback;
    void *user_data;
    struct cover_query_s *next;
} cover_query_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static uintptr_t mutex;
static uintptr_t imlib_mutex;
static uintptr_t cond;
static volatile int terminate;
static volatile int clear_queue;
static intptr_t tid;

static int artwork_enable_embedded;
static int artwork_enable_local;
static int artwork_enable_lfm;
static int artwork_enable_aao;
static time_t artwork_reset_time;
static char artwork_filemask[200];

static const char *get_default_cover (void) {
    return default_cover;
}

int
make_cache_dir_path (char *path, int size, const char *artist, int img_size) {
    const char *cache = getenv ("XDG_CACHE_HOME");

    int sz;
    
    if (img_size == -1) {
        sz = snprintf (path, size, cache ? "%s/deadbeef/covers/" : "%s/.cache/deadbeef/covers/", cache ? cache : getenv ("HOME"));
    }
    else {
        sz = snprintf (path, size, cache ? "%s/deadbeef/covers-%d/" : "%s/.cache/deadbeef/covers-%d/", cache ? cache : getenv ("HOME"), img_size);
    }
    path += sz;

    sz += snprintf (path, size-sz, "%s", artist);
    for (char *p = path; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
    return sz;
}

void
make_cache_path (char *path, int size, const char *album, const char *artist, int img_size) {
    char *p = path;
    int sz = make_cache_dir_path (path, size, artist, img_size);
    size -= sz;
    path += sz;
    sz = snprintf (path, size, "/%s.jpg", album);
    for (char *p = path+1; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
}

void
queue_add (const char *fname, const char *artist, const char *album, int img_size, artwork_callback callback, void *user_data) {
    if (!artist) {
        artist = "";
    }
    if (!album) {
        album = "";
    }
    deadbeef->mutex_lock (mutex);

    for (cover_query_t *q = queue; q; q = q->next) {
        if (!strcasecmp (artist, q->artist) || !strcasecmp (album, q->album)) {
            deadbeef->mutex_unlock (mutex);
            return; // already in queue
        }
    }

    cover_query_t *q = malloc (sizeof (cover_query_t));
    memset (q, 0, sizeof (cover_query_t));
    q->fname = strdup (fname);
    q->artist = strdup (artist);
    q->album = strdup (album);
    q->size = img_size;
    q->callback = callback;
    q->user_data = user_data;
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

void
queue_pop (void) {
    deadbeef->mutex_lock (mutex);
    cover_query_t *next = queue ? queue->next : NULL;
    free (queue->fname);
    free (queue->artist);
    free (queue->album);
    free (queue);
    queue = next;
    if (!queue) {
        queue_tail = NULL;
    }
    deadbeef->mutex_unlock (mutex);
}

static int
check_dir (const char *dir, mode_t mode)
{
    char *tmp = strdup (dir);
    char *slash = tmp;
    struct stat stat_buf;
    do
    {
        slash = strstr (slash+1, "/");
        if (slash)
            *slash = 0;
        if (-1 == stat (tmp, &stat_buf))
        {
            trace ("creating dir %s\n", tmp);
            if (0 != mkdir (tmp, mode))
            {
                trace ("Failed to create %s (%d)\n", tmp, errno);
                free (tmp);
                return 0;
            }
        }
        if (slash)
            *slash = '/';
    }
    while (slash);
    free (tmp);
    return 1;
}

#define BUFFER_SIZE 4096

static int
copy_file (const char *in, const char *out, int img_size) {
    trace ("copying %s to %s\n", in, out);

    if (img_size != -1) {
        deadbeef->mutex_lock (imlib_mutex);
        // need to scale, use imlib2
        Imlib_Image img = imlib_load_image_immediately (in);
        if (!img) {
            trace ("file %s not found, or imlib2 can't load it\n", in);
            deadbeef->mutex_unlock (imlib_mutex);
            return -1;
        }
        imlib_context_set_image(img);
        int w = imlib_image_get_width ();
        int h = imlib_image_get_height ();
        int sw, sh;
        if (deadbeef->conf_get_int ("artwork.scale_towards_longer", 1)) {
            if (w > h) {
                sh = img_size;
                sw = img_size * w / h;
            }
            else {
                sw = img_size;
                sh = img_size * h / w;
            }
        }
        else {
            if (w < h) {
                sh = img_size;
                sw = img_size * w / h;
            }
            else {
                sw = img_size;
                sh = img_size * h / w;
            }
        }
        Imlib_Image scaled = imlib_create_image (sw, sh);
        imlib_context_set_image (scaled);
        imlib_blend_image_onto_image (img, 1, 0, 0, w, h, 0, 0, sw, sh);
        Imlib_Load_Error err = 0;
        imlib_image_set_format ("jpg");
        imlib_save_image_with_error_return (out, &err);
        if (err != 0) {
            trace ("imlib save %s returned %d\n", out, err);
            imlib_free_image ();
            imlib_context_set_image(img);
            imlib_free_image ();
            deadbeef->mutex_unlock (imlib_mutex);
            return -1;
        }
        imlib_free_image ();
        imlib_context_set_image(img);
        imlib_free_image ();
        deadbeef->mutex_unlock (imlib_mutex);

        return 0;
    }

    FILE *fin = fopen (in, "rb");
    if (!fin) {
        trace ("artwork: failed to open file %s for reading\n", in);
        return -1;
    }
    FILE *fout = fopen (out, "w+b");
    if (!fout) {
        fclose (fin);
        trace ("artwork: failed to open file %s for writing\n", out);
        return -1;
    }
    char *buf = malloc (BUFFER_SIZE);
    if (!buf) {
        trace ("artwork: failed to alloc %d bytes\n", BUFFER_SIZE);
        fclose (fin);
        fclose (fout);
        return -1;
    }

    fseek (fin, 0, SEEK_END);
    size_t sz = ftell (fin);
    rewind (fin);

    while (sz > 0) {
        int rs = min (sz, BUFFER_SIZE);
        if (fread (buf, rs, 1, fin) != 1) {
            trace ("artwork: failed to read file %s\n", in);
            break;
        }
        if (fwrite (buf, rs, 1, fout) != 1) {
            trace ("artwork: failed to write file %s\n", out);
            break;
        }
        sz -= rs;
    }
    free (buf);
    fclose (fin);
    fclose (fout);
    if (sz > 0) {
        unlink (out);
    }
    return 0;
}

static int
filter_custom (const struct dirent *f)
{
    char mask[200] = "";
    char *p = artwork_filemask;
    while (p) {
        *mask = 0;
        char *e = strchr (p, ';');
        if (e) {
            strncpy (mask, p, e-p);
            mask[e-p] = 0;
            e++;
        }
        else {
            strcpy (mask, p);
        }
        if (*mask) {
            if (!fnmatch (mask, f->d_name, FNM_CASEFOLD)) {
                return 1;
            }
        }
        p = e;
    }
    return 0;
}

static int
filter_jpg (const struct dirent *f)
{
    const char *ext = strrchr (f->d_name, '.');
    if (!ext)
        return 0;
    if (!strcasecmp (ext, ".jpg") || !strcasecmp (ext, ".jpeg")) {
        return 1;
    }

    return 0;
}

static uint8_t *
id3v2_skip_str (int enc, uint8_t *ptr, uint8_t *end) {
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


static void
fetcher_thread (void *none)
{
    for (;;) {
        trace ("artwork: waiting for signal\n");
        deadbeef->cond_wait (cond, mutex);
        trace ("artwork: cond signalled\n");
        deadbeef->mutex_unlock (mutex);
        while (!terminate && queue && !clear_queue) {
            cover_query_t *param = queue;
            char path [PATH_MAX];
            struct dirent **files;
            int files_count;

            make_cache_dir_path (path, sizeof (path), param->artist, -1);
            trace ("cache folder: %s\n", path);
            if (!check_dir (path, 0755)) {
                queue_pop ();
                trace ("failed to create folder for %s %s\n", param->album, param->artist);
                continue;
            }
            if (param->size != -1) {
                make_cache_dir_path (path, sizeof (path), param->artist, param->size);
                trace ("cache folder: %s\n", path);
                if (!check_dir (path, 0755)) {
                    queue_pop ();
                    trace ("failed to create folder for %s %s\n", param->album, param->artist);
                    continue;
                }
            }

            trace ("fetching cover for %s %s\n", param->album, param->artist);
            char cache_path[1024];
            make_cache_path (cache_path, sizeof (cache_path), param->album, param->artist, -1);
            int got_pic = 0;

            // try to load embedded from id3v2
            if (deadbeef->is_local_file (param->fname)) {
                if (artwork_enable_embedded) {
                    trace ("trying to load artwork from id3v2 tag for %s\n", param->fname);
                    DB_id3v2_tag_t tag;
                    memset (&tag, 0, sizeof (tag));
                    DB_FILE *fp = deadbeef->fopen (param->fname);
                    current_file = fp;
                    if (fp) {
                        int res = deadbeef->junk_id3v2_read_full (NULL, &tag, fp);
                        if (!res) {
                            for (DB_id3v2_frame_t *f = tag.frames; f; f = f->next) {
                                if (!strcmp (f->id, "APIC")) {

                                    if (f->size < 20) {
                                        trace ("artwork: id3v2 APIC frame is too small\n");
                                        continue;
                                    }
                                    uint8_t *data = f->data;
                                    uint8_t *end = f->data + f->size;
                                    int enc = *data;
                                    data++; // enc
                                    // mime-type must always be ASCII - hence enc is 0 here
                                    uint8_t *mime_end = id3v2_skip_str (0, data, end);
                                    if (!mime_end) {
                                        trace ("artwork: corrupted id3v2 APIC frame\n");
                                        continue;
                                    }
                                    if (strcasecmp (data, "image/jpeg") && strcasecmp (data, "image/png")) {
                                        trace ("artwork: unsupported mime type: %s\n", data);
                                        continue;
                                    }
                                    if (*mime_end != 3) {
                                        trace ("artwork: picture type=%d, skipped: %s\n", *mime_end);
                                        continue;
                                    }
                                    trace ("artwork: mime-type=%s, picture type: %d\n", data, *mime_end);
                                    data = mime_end;
                                    data++; // picture type
                                    data = id3v2_skip_str (enc, data, end); // description
                                    if (!data) {
                                        trace ("artwork: corrupted id3v2 APIC frame\n");
                                        continue;
                                    }
                                    int sz = f->size - (data - f->data);

                                    char tmp_path[1024];
                                    trace ("will write id3v2 APIC into %s\n", cache_path);
                                    snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                                    FILE *out = fopen (tmp_path, "w+b");
                                    if (!out) {
                                        trace ("artwork: failed to open %s for writing\n", tmp_path);
                                        break;
                                    }
                                    if (fwrite (data, 1, sz, out) != sz) {
                                        trace ("artwork: failed to write id3v2 picture into %s\n", tmp_path);
                                        fclose (out);
                                        unlink (tmp_path);
                                        break;
                                    }
                                    fclose (out);
                                    int err = rename (tmp_path, cache_path);
                                    if (err != 0) {
                                        trace ("Failed not move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                                        unlink (tmp_path);
                                        break;
                                    }
                                    unlink (tmp_path);
                                    got_pic = 1;
                                    break;
                                }
                            }
                        }

                        deadbeef->junk_id3v2_free (&tag);
                        current_file = NULL;
                        deadbeef->fclose (fp);
                    }

                    // try to load embedded from apev2
                    {
                        trace ("trying to load artwork from apev2 tag for %s\n", param->fname);
                        DB_apev2_tag_t tag;
                        memset (&tag, 0, sizeof (tag));
                        DB_FILE *fp = deadbeef->fopen (param->fname);
                        current_file = fp;
                        if (fp) {
                            int res = deadbeef->junk_apev2_read_full (NULL, &tag, fp);
                            if (!res) {
                                for (DB_apev2_frame_t *f = tag.frames; f; f = f->next) {
                                    if (!strcasecmp (f->key, "cover art (front)")) {
                                        uint8_t *name = f->data, *ext = f->data, *data = f->data;
                                        uint8_t *end = f->data + f->size;
                                        while (data < end && *data)
                                            data++;
                                        if (data == end) {
                                            trace ("artwork: apev2 cover art frame has no name\n");
                                            continue;
                                        }
                                        int sz = end - ++data;
                                        if (sz < 20) {
                                            trace ("artwork: apev2 cover art frame is too small\n");
                                            continue;
                                        }
                                        ext = strrchr (name, '.');
                                        if (!ext || !*++ext) {
                                            trace ("artwork: apev2 cover art name has no extension\n");
                                            continue;
                                        }
                                        if (strcasecmp (ext, "jpeg") && strcasecmp (ext, "jpg") && strcasecmp (ext, "png")) {
                                            trace ("artwork: unsupported file type: %s\n", ext);
                                            continue;
                                        }
                                        trace ("found apev2 cover art of %d bytes (%s)\n", sz, ext);
                                        char tmp_path[1024];
                                        char cache_path[1024];
                                        make_cache_path (cache_path, sizeof (cache_path), param->album, param->artist, -1);
                                        trace ("will write apev2 cover art into %s\n", cache_path);
                                        snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                                        FILE *out = fopen (tmp_path, "w+b");
                                        if (!out) {
                                            trace ("artwork: failed to open %s for writing\n", tmp_path);
                                            break;
                                        }
                                        if (fwrite (data, 1, sz, out) != sz) {
                                            trace ("artwork: failed to write apev2 picture into %s\n", tmp_path);
                                            fclose (out);
                                            unlink (tmp_path);
                                            break;
                                        }
                                        fclose (out);
                                        int err = rename (tmp_path, cache_path);
                                        if (err != 0) {
                                            trace ("Failed not move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                                            unlink (tmp_path);
                                            break;
                                        }
                                        unlink (tmp_path);
                                        got_pic = 1;
                                        break;
                                    }
                                }
                            }

                            deadbeef->junk_apev2_free (&tag);
                            current_file = NULL;
                            deadbeef->fclose (fp);
                        }
                    }
                }

                if (!got_pic && artwork_enable_local) {
                    /* Searching in track directory */
                    strncpy (path, param->fname, sizeof (path));
                    char *slash = strrchr (path, '/');
                    if (slash) {
                        *slash = 0; // assuming at least one slash exist
                    }
                    trace ("scanning directory: %s\n", path);
                    files_count = scandir (path, &files, filter_custom, alphasort);
                    if (files_count == 0) {
                        files_count = scandir (path, &files, filter_jpg, alphasort);
                    }

                    if (files_count > 0) {
                        trace ("found cover for %s - %s in local folder\n", param->artist, param->album);
                        if (check_dir (path, 0755)) {
                            strcat (path, "/");
                            strcat (path, files[0]->d_name);
                            char cache_path[1024];
                            char tmp_path[1024];
                            make_cache_path (cache_path, sizeof (cache_path), param->album, param->artist, -1);
                            snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                            copy_file (path, tmp_path, -1);
                            int err = rename (tmp_path, cache_path);
                            if (err != 0) {
                                trace ("Failed to move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                                unlink (tmp_path);
                            }
                            int i;
                            for (i = 0; i < files_count; i++) {
                                free (files [i]);
                            }
                            got_pic = 1;
                        }
                    }
                }
            }

            if (!got_pic) {
                if (artwork_enable_lfm && !fetch_from_lastfm (param->artist, param->album, cache_path)) {
                    got_pic = 1;
                }
                else if (artwork_enable_aao && !fetch_from_albumart_org (param->artist, param->album, cache_path)) {
                    got_pic = 1;
                }
            }

            if (got_pic) {
                trace ("downloaded art for %s %s\n", param->album, param->artist);
                if (param->size != -1) {
                    make_cache_dir_path (path, sizeof (path), param->artist, param->size);
                    trace ("cache folder: %s\n", path);
                    if (!check_dir (path, 0755)) {
                        trace ("failed to create folder %s\n", path);
                        queue_pop ();
                        continue;
                    }
                    char scaled_path[1024];
                    make_cache_path (scaled_path, sizeof (scaled_path), param->album, param->artist, param->size);
                    copy_file (cache_path, scaled_path, param->size);
                }
                if (param->callback) {
                    param->callback (param->fname, param->artist, param->album, param->user_data);
                }
            }
            queue_pop ();
        }
        if (clear_queue) {
            trace ("artwork: received queue clear request\n");
            while (queue) {
                queue_pop ();
            }
            clear_queue = 0;
            trace ("artwork: queue clear done\n");
            continue;
        }
        if (terminate) {
            break;
        }
    }
}

static char *
find_image (const char *path) {
    struct stat stat_buf;
    if (0 == stat (path, &stat_buf)) {
        int cache_period = deadbeef->conf_get_int ("artwork.cache.period", 48);
        time_t tm = time (NULL);
        // invalidate cache every 2 days
        if ((cache_period > 0 && (tm - stat_buf.st_mtime > cache_period * 60 * 60))
                || artwork_reset_time > stat_buf.st_mtime) {
            trace ("reloading cached file %s\n", path);
            unlink (path);
            return NULL;
        }

        return strdup (path);
    }
    return NULL;
}

char*
get_album_art (const char *fname, const char *artist, const char *album, int size, artwork_callback callback, void *user_data)
{
//    trace ("get_album_art: %s (%s - %s)\n", fname, artist, album);
    char path [1024];

    if (!album) {
        album = "";
    }
    if (!artist) {
        artist = "";
    }

    if (!*artist || !*album)
    {
        //give up
        return size == -1 ? strdup (get_default_cover ()) : NULL;
    }

    if (!deadbeef->is_local_file (fname)) {
        return size == -1 ? strdup (get_default_cover ()) : NULL;
    }

    make_cache_path (path, sizeof (path), album, artist, size);
    char *p = find_image (path);
    if (p) {
        return p;
    }

    if (size != -1) {
        // check if we have unscaled image
        char unscaled_path[1024];
        make_cache_path (unscaled_path, sizeof (unscaled_path), album, artist, -1);
        p = find_image (unscaled_path);
        if (p) {
            free (p);
            char dir[1024];
            make_cache_dir_path (dir, sizeof (dir), artist, size);
            if (!check_dir (dir, 0755)) {
                trace ("failed to create folder for %s\n", dir);
            }
            else {
                int res = copy_file (unscaled_path, path, size);
                if (!res) {
                    return strdup (path);
                }
            }
        }
    }

    queue_add (fname, artist, album, size, callback, user_data);
    return size == -1 ? strdup (get_default_cover ()) : NULL;
}

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

void
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

static int
artwork_on_configchanged (DB_event_t *ev, uintptr_t data) {
    int new_artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    int new_artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
    int new_artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    int new_artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);

    char new_artwork_filemask[200];
    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, new_artwork_filemask, sizeof (new_artwork_filemask));

    if (new_artwork_enable_embedded != artwork_enable_embedded
            || new_artwork_enable_local != artwork_enable_local
            || new_artwork_enable_lfm != artwork_enable_lfm
            || new_artwork_enable_aao != artwork_enable_aao
            || strcmp (new_artwork_filemask, artwork_filemask)) {
        trace ("artwork config changed, invalidating cache...\n");
        artwork_enable_embedded = new_artwork_enable_embedded;
        artwork_enable_local = new_artwork_enable_local;
        artwork_enable_lfm = new_artwork_enable_lfm;
        artwork_enable_aao = new_artwork_enable_aao;
        artwork_reset_time = time (NULL);
        strcpy (artwork_filemask, new_artwork_filemask);
        deadbeef->conf_set_int64 ("artwork.cache_reset_time", artwork_reset_time);
        artwork_reset (0);
        deadbeef->sendmessage (M_PLAYLIST_REFRESH, 0, 0, 0);
    }

    return 0;
}

static int
artwork_plugin_start (void)
{
    deadbeef->conf_lock ();

    const char *def_art = deadbeef->conf_get_str_fast ("gtkui.nocover_pixmap", NULL);
    if (!def_art) {
        snprintf (default_cover, sizeof (default_cover), "%s/noartwork.jpg", deadbeef->get_pixmap_dir ());
    }
    else {
        strcpy (default_cover, def_art);
    }
    terminate = 0;

    artwork_enable_embedded = deadbeef->conf_get_int ("artwork.enable_embedded", 1);
    artwork_enable_local = deadbeef->conf_get_int ("artwork.enable_localfolder", 1);
    artwork_enable_lfm = deadbeef->conf_get_int ("artwork.enable_lastfm", 0);
    artwork_enable_aao = deadbeef->conf_get_int ("artwork.enable_albumartorg", 0);
    artwork_reset_time = deadbeef->conf_get_int64 ("artwork.cache_reset_time", 0);

    deadbeef->conf_get_str ("artwork.filemask", DEFAULT_FILEMASK, artwork_filemask, sizeof (artwork_filemask));

    deadbeef->conf_unlock ();

    artwork_filemask[sizeof(artwork_filemask)-1] = 0;

    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (artwork_on_configchanged), 0);

    mutex = deadbeef->mutex_create_nonrecursive ();
    imlib_mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority (fetcher_thread, NULL);

    return 0;
}

static int
artwork_plugin_stop (void)
{
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (artwork_on_configchanged), 0);
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
    if (imlib_mutex) {
        deadbeef->mutex_free (imlib_mutex);
        imlib_mutex = 0;
    }
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
    "property \"Fetch from last.fm\" checkbox artwork.enable_lastfm 0;\n"
    "property \"Fetch from albumart.org\" checkbox artwork.enable_albumartorg 0;\n"
    "property \"Scale artwork towards longer side\" checkbox artwork.scale_towards_longer 1;\n"
;

// define plugin interface
static DB_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.version_major = 1,
    .plugin.plugin.version_minor = 0,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "artwork",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork either from local directories or from internet",
    .plugin.plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>\n"
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
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.start = artwork_plugin_start,
    .plugin.plugin.stop = artwork_plugin_stop,
    .plugin.plugin.configdialog = settings_dlg,
    .get_album_art = get_album_art,
    .reset = artwork_reset,
    .get_default_cover = get_default_cover,
};
