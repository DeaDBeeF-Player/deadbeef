#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <errno.h>
#include <dirent.h>
#include "../../deadbeef.h"
#include "artwork.h"
#include "lastfm.h"
#include "albumartorg.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

static DB_artwork_plugin_t plugin;
DB_functions_t *deadbeef;

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
        fprintf (stderr, "Could not open %s for writing\n", temp);
        return 0;
    }
    ret = fetch_to_stream (url, stream);
    if (ret != 0)
        printf ("Failed to fetch %s\n", url);
    fclose (stream);
    if (0 == ret)
    {
        ret = (0 == rename (temp, filename));
        if (!ret)
            trace ("Could not move %s to %s: %d\n", temp, filename, errno);
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
//        trace ("Checking %s\n", tmp);
        if (-1 == stat (tmp, &stat_buf))
        {
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

typedef struct
{
    const char *artist;
    const char *album;
    artwork_callback callback;
} fetcher_thread_param_t;

static void
fetcher_thread (fetcher_thread_param_t *param)
{
    char path [1024];

    snprintf (path, sizeof (path), "%s/artcache/%s", deadbeef->get_config_dir (), param->artist);
    if (!check_dir (path, 0755))
        goto finalize;

    snprintf (path, sizeof (path), "%s/artcache/%s/%s.jpg", deadbeef->get_config_dir (), param->artist, param->album);

    if (!fetch_from_lastfm (param->artist, param->album, path))
        if (!fetch_from_albumart_org (param->artist, param->album, path))
            goto finalize;

    if (param->callback)
        param->callback (param->artist, param->album);

finalize:
    free (param);
    return;
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

char*
get_album_art (DB_playItem_t *track, artwork_callback callback)
{
    char path [1024];
    struct dirent **files;
    int files_count;

    const char *album = deadbeef->pl_find_meta (track, "album");
    const char *artist = deadbeef->pl_find_meta (track, "artist");

    /* Searching in track diectory */
    strncpy (path, track->fname, sizeof (path));
    *(strrchr (path, '/')) = 0; //Supposing at least one slash exist
//    trace ("Scanning: %s\n", path);
    files_count = scandir (path, &files, filter_jpg, alphasort);

/*    if (files_count < 0)
        trace ("Failed to scan dir");
    if (files_count == 0)
        trace ("No jpg's found in directoy");*/
    if (files_count > 0)
    {
        strcat (path, "/");
        strcat (path, files[0]->d_name);
        char *res = strdup (path);
        trace ("Found: %s\n", res);
        int i;
        for (i = 0; i < files_count; i++)
            free (files [i]);
        return res;
    }

    /* Searching in cache */
    if (!artist || !*artist || !album || !*album)
    {
        //give up
        return PREFIX "/share/deadbeef/pixmaps/blank_cd.jpg";
    }

    snprintf (path, sizeof (path), "%s/artcache/%s/%s.jpg", deadbeef->get_config_dir (), artist, album);

    struct stat stat_buf;
    if (0 == stat (path, &stat_buf))
    {
        char *res = strdup (path);
//        printf ("Found in cache: %s\n", res);
        return res;
    }

    /* Downloading from internet */
    trace ("Downloading: %s %s\n", artist, album);
    fetcher_thread_param_t *param = malloc (sizeof (fetcher_thread_param_t));
    param->artist = artist;
    param->album = album;
    param->callback = callback;
    deadbeef->thread_start ((void (*)(void *))fetcher_thread, param);
    return PREFIX "/share/deadbeef/pixmaps/blank_cd.jpg";
}

DB_plugin_t *
artwork_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static int
artwork_plugin_start (void)
{
}

static int
artwork_plugin_stop (void)
{
}

// define plugin interface
static DB_artwork_plugin_t plugin = {
    .plugin.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.plugin.type = DB_PLUGIN_MISC,
    .plugin.plugin.id = "cover_loader",
    .plugin.plugin.name = "Album Artwork",
    .plugin.plugin.descr = "Loads album artwork either from local directories or from internet",
    .plugin.plugin.author = "Viktor Semykin",
    .plugin.plugin.email = "thesame.ml@gmail.com",
    .plugin.plugin.website = "http://deadbeef.sf.net",
    .plugin.plugin.start = artwork_plugin_start,
    .plugin.plugin.stop = artwork_plugin_stop,
    .get_album_art = get_album_art,
};
