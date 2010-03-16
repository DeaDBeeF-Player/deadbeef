#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include "../../deadbeef.h"
#include "artwork.h"
#include "lastfm.h"
#include "albumartorg.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(...)

#define DEFAULT_COVER_PATH (PREFIX "/share/deadbeef/pixmaps/noartwork.jpg")

static DB_artwork_plugin_t plugin;
DB_functions_t *deadbeef;


typedef struct cover_query_s {
    char *fname;
    char *artist;
    char *album;
    artwork_callback callback;
    void *user_data;
    struct cover_query_s *next;
} cover_query_t;

static cover_query_t *queue;
static cover_query_t *queue_tail;
static uintptr_t mutex;
static uintptr_t cond;
static volatile int terminate;
static volatile int clear_queue;
static intptr_t tid;

void
make_cache_dir_path (char *path, int size, const char *album, const char *artist) {
    int sz = snprintf (path, size, "%s/artcache/", deadbeef->get_config_dir ());
    size -= sz;
    path += sz;

    sz = snprintf (path, size, "%s", artist);
    for (char *p = path; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
}

void
make_cache_path (char *path, int size, const char *album, const char *artist) {
    int sz = snprintf (path, size, "%s/artcache/", deadbeef->get_config_dir ());
    size -= sz;
    path += sz;

    sz = snprintf (path, size, "%s", artist);
    for (char *p = path; *p; p++) {
        if (*p == '/') {
            *p = '_';
        }
    }
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
queue_add (const char *fname, const char *artist, const char *album, artwork_callback callback, void *user_data) {
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

int
fetch_to_stream (const char *url, FILE *stream)
{
    CURL *curl = curl_easy_init();
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, stream);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1);
    CURLcode ret = curl_easy_perform (curl);
    curl_easy_cleanup (curl);
    return ret;
}

int
fetch_to_file (const char *url, const char *filename)
{
    /**
     * Downloading files directly to its locations can cause
     * cachehits of semi-downloaded files. That's why I use
     * temporary files
     */
    char temp [1024];
    int ret;
    snprintf (temp, sizeof (temp), "%s.part", filename);

    FILE *stream = fopen (temp, "wb");
    if (!stream)
    {
        trace ("Could not open %s for writing\n", temp);
        return 0;
    }
    ret = fetch_to_stream (url, stream);
    if (ret != 0) {
        trace ("Failed to fetch %s\n", url);
    }
    fclose (stream);
    if (0 == ret)
    {
        ret = (0 == rename (temp, filename));
        if (!ret) {
            trace ("Could not move %s to %s: %d\n", temp, filename, errno);
            unlink (temp);
        }
    }
    return ret;
}

char*
fetch (const char *url)
{
    char *data;
    size_t size;
    FILE *stream = open_memstream (&data, &size);
    fetch_to_stream (url, stream);
    fclose (stream);
    return data;
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
copy_file (const char *in, const char *out) {
    trace ("copying %s to %s\n", in, out);
    char *buf = malloc (BUFFER_SIZE);
    if (!buf) {
        trace ("artwork: failed to alloc %d bytes\n", BUFFER_SIZE);
        return -1;
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
filter_jpg (const struct dirent *f)
{
    const char *ext = strrchr (f->d_name, '.');
    if (!ext)
        return 0;
    if (0 == strcasecmp (ext, ".jpg") ||
        0 == strcasecmp (ext, ".jpeg"))
        return 1;
    return 0;
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
            char path [1024];
            struct dirent **files;
            int files_count;

            make_cache_dir_path (path, sizeof (path), param->album, param->artist);
            trace ("cache folder: %s\n", path);
            if (!check_dir (path, 0755)) {
                queue_pop ();
                trace ("failed to create folder for %s %s\n", param->album, param->artist);
                continue;
            }

            trace ("fetching cover for %s %s\n", param->album, param->artist);
            /* Searching in track directory */
            strncpy (path, param->fname, sizeof (path));
            char *slash = strrchr (path, '/');
            if (slash) {
                *slash = 0; // assuming at least one slash exist
            }
            trace ("scanning directory: %s\n", path);
            files_count = scandir (path, &files, filter_jpg, alphasort);

            if (files_count > 0) {
                trace ("found cover for %s - %s in local folder\n", param->artist, param->album);
                if (check_dir (path, 0755)) {
                    strcat (path, "/");
                    strcat (path, files[0]->d_name);
                    char cache_path[1024];
                    char tmp_path[1024];
                    make_cache_path (cache_path, sizeof (cache_path), param->album, param->artist);
                    snprintf (tmp_path, sizeof (tmp_path), "%s.part", cache_path);
                    copy_file (path, tmp_path);
                    int err = rename (tmp_path, cache_path);
                    if (err != 0) {
                        trace ("Failed not move %s to %s: %s\n", tmp_path, cache_path, strerror (err));
                        unlink (tmp_path);
                    }
                    int i;
                    for (i = 0; i < files_count; i++) {
                        free (files [i]);
                    }
                    if (param->callback) {
                        param->callback (param->fname, param->artist, param->album, param->user_data);
                    }
                    queue_pop ();
                    continue;
                }
            }

            make_cache_path (path, sizeof (path), param->album, param->artist);

            if (fetch_from_lastfm (param->artist, param->album, path)) {
                trace ("art found on last.fm for %s %s\n", param->album, param->artist);
            }
            else if (fetch_from_albumart_org (param->artist, param->album, path)) {
                trace ("art found on albumart.org for %s %s\n", param->album, param->artist);
            }
            else {
                trace ("art not found for %s %s\n", param->album, param->artist);
                if (!copy_file (DEFAULT_COVER_PATH, path)) {
                    if (param->callback) {
                        param->callback (param->fname, param->artist, param->album, param->user_data);
                    }
                }
                queue_pop ();
                continue;
            }

            trace ("downloaded art for %s %s\n", param->album, param->artist);
            if (param->callback) {
                param->callback (param->fname, param->artist, param->album, param->user_data);
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

char*
get_album_art (const char *fname, const char *artist, const char *album, artwork_callback callback, void *user_data)
{
    char path [1024];

    if (!album) {
        album = "";
    }
    if (!artist) {
        artist = "";
    }
//    trace ("looking for %s - %s\n", artist, album);

    /* Searching in cache */
    if (!*artist || !*album)
    {
        //give up
        return strdup (DEFAULT_COVER_PATH);
    }

    make_cache_path (path, sizeof (path), album, artist);
    struct stat stat_buf;
    if (0 == stat (path, &stat_buf)) {
        int cache_period = deadbeef->conf_get_int ("artwork.cache.period", 48);
        // invalidate cache every 2 days
        if (cache_period > 0) {
            time_t tm = time (NULL);
            if (tm - stat_buf.st_mtime > cache_period * 60 * 60) {
                trace ("reloading cached file %s\n", path);
                unlink (path);
                queue_add (fname, artist, album, callback, user_data);
                return strdup (DEFAULT_COVER_PATH);
            }
        }

//        trace ("found %s in cache\n", path);
        return strdup (path);
    }

    queue_add (fname, artist, album, callback, user_data);
    return strdup (DEFAULT_COVER_PATH);
}

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

void
artwork_reset (int fast) {
    if (fast) {
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
artwork_plugin_start (void)
{
    terminate = 0;
    mutex = deadbeef->mutex_create ();
    mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority (fetcher_thread, NULL);
}

static int
artwork_plugin_stop (void)
{
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
    if (cond) {
        deadbeef->cond_free (cond);
    }
}

static const char settings_dlg[] =
    "property \"Cache update period (hr)\" entry artwork.cache.period 48;\n"
;
// define plugin interface
static DB_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "cover_loader",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork either from local directories or from internet",
    .plugin.plugin.author = "Viktor Semykin, Alexey Yakovenko",
    .plugin.plugin.email = "thesame.ml@gmail.com, waker@users.sourceforge.net",
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.start = artwork_plugin_start,
    .plugin.plugin.stop = artwork_plugin_stop,
    .plugin.plugin.configdialog = settings_dlg,
    .get_album_art = get_album_art,
    .reset = artwork_reset,
};
