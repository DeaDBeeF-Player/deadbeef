#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include <cdio/cdio.h>
#include <cddb/cddb.h>

#include "../../deadbeef.h"

#define SECTORSIZE CDIO_CD_FRAMESIZE_RAW //2352
#define SAMPLESIZE 4 //bytes
#define BUFSIZE (CDIO_CD_FRAMESIZE_RAW * 2)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static CdIo_t* cdio = NULL;
static lsn_t first_sector;
static unsigned int sector_count;
static uint8_t tail [SECTORSIZE];
static unsigned int tail_len;
static int current_sector;

static int use_cddb = 1;
static const char *server;
static int port;
static const char *proxy = NULL;
static int proxy_port;
static int proto_cddb = 1;

static unsigned int current_sample = 0;

inline int min(int a, int b) { return a<b ? a : b; }

struct cddb_thread_params
{
    DB_playItem_t **items;
    CdIo_t *cdio;
};

static char*
trim(char* s)
{
    char *h, *t;
    
    for ( h = s; *h == ' ' || *h == '\t'; h++ );
    for ( t = s + strlen(s); *t == ' ' || *t == '\t'; t-- );
    *(t+1) = 0;
    return h;
}

static int
read_config ()
{
    char param[ 256 ];
    char config[1024];
    char *key, *value;
    snprintf (config, 1024, "%s/cdaudio", deadbeef->get_config_dir());

    FILE *cfg_file = fopen (config, "rt");
    if (!cfg_file) {
        fprintf (stderr, "cdaudio: failed open %s\n", config);
        return -1;
    }

    while ( fgets( param, sizeof(param), cfg_file ) )
    {
        param[ strlen( param )-1 ] = 0; //terminating \n
        if (param[0] == '#' || param[0] == 0)
            continue;

        char *colon = strchr (param, ':');
        if (!colon)
        {
            fprintf (stderr, "cdaudio: malformed config string: %s\n", param);
            continue;
        }
        *(colon++) = 0;
        key = trim (param);
        value = trim (colon + 1);

        if (0 == strcmp (key, "cddb"))
        {
            if (0 == strcmp (value, "on"))
                use_cddb = 1;
            else if (0 == strcmp (value, "off"))
                use_cddb = 0;
            else
            {
                use_cddb = 0;
                fprintf (stderr, "cdaudio: warning, wrong value %s\n", value);
            }
        }
        else if (0 == strcmp (key, "cddb_server"))
        {
            server = strdup (value);
        }
        else if (0 == strcmp (key, "cddb_port"))
        {
            port = atoi (value);
        }
        else if (0 == strcmp (key, "cddb_proxy"))
        {
            proxy = strdup (value);
        }
        else if (0 == strcmp (key, "cddb_proxy_port"))
        {
            proxy_port = atoi (value);
        }
        else if (0 == strcmp (key, "proto"))
        {
            if (0 == strcmp (value, "cddb"))
                proto_cddb = 1;
            else if (0 == strcmp (value, "http"))
                proto_cddb = 0;
            else
                fprintf (stderr, "cdaudio: unknown protocol \"%s\"\n", value);
        }
        else
            fprintf (stderr, "cdaudio: warning, unknown option %s\n", key);
    }
    fclose (cfg_file);
}

static int
cda_init (DB_playItem_t *it) {
    printf ("CDA: initing %s\n", it->fname);

    char *location = strdup (it->fname);

    char *nr = strchr (location, '#');
    *nr = 0; nr++;
    int track_nr = atoi (nr);
    char *fname = (*location) ? location : NULL; //NULL if empty string; means pysical CD drive

    cdio = cdio_open (fname, DRIVER_UNKNOWN);
    if  (!cdio)
    {
        fprintf (stderr, "Could not open CD\n");
        return -1;
    }

    if (TRACK_FORMAT_AUDIO != cdio_get_track_format (cdio, track_nr))
    {
        fprintf (stderr, "Not an audio track (%d)\n", track_nr);
        return -1;
    }

    plugin.info.bps = 16,
    plugin.info.channels = 2,
    plugin.info.samplerate = 44100,
    plugin.info.readpos = 0;

    first_sector = cdio_get_track_lsn (cdio, track_nr);
    sector_count = cdio_get_track_sec_count (cdio, track_nr);
    current_sector = first_sector;
    tail_len = 0;
    current_sample = 0;
}

int
cda_read_int16 (char *bytes, int size) {
    int initsize = size;
    int extrasize = 0;
    
    if (tail_len > 0)
    {
        if (tail_len >= size)
        {
            printf ("Easy case\n");
            memcpy (bytes, tail, size);
            tail_len -= size;
            memmove (tail, tail+size, tail_len);
            return size;
        }
        printf ("Prepending with tail of %d bytes\n", tail_len);
        extrasize = tail_len;
        memcpy (bytes, tail, tail_len);
        bytes += tail_len;
        size -= tail_len;
        tail_len = 0;
    }

    int sectors_to_read = size / SECTORSIZE + 1;
    int end = 0;
    
    if (current_sector + sectors_to_read > first_sector + sector_count) //we reached end of track
    {
        end = 1;
        sectors_to_read = first_sector + sector_count - current_sector;
        printf ("We reached end of track\n");
    }

    int bufsize = sectors_to_read * SECTORSIZE;

    tail_len = end ? 0 : bufsize - size;

    char *buf = alloca (bufsize);

    driver_return_code_t ret = cdio_read_audio_sectors (cdio, buf, current_sector, sectors_to_read);
    if (ret != DRIVER_OP_SUCCESS)
        return 0;
    current_sector += sectors_to_read;

    int retsize = end ? bufsize : size;

    memcpy (bytes, buf, retsize);
    if (!end)
        memcpy (tail, buf+retsize, tail_len);

    retsize += extrasize;
    printf ("requested: %d; tail_len: %d; size: %d; sectors_to_read: %d; return: %d\n", initsize, tail_len, size, sectors_to_read, retsize);
    current_sample += retsize / SAMPLESIZE;
    plugin.info.readpos = current_sample / 44100;
    return retsize;
}

static void
cda_free ()
{
    if (cdio)
    {
        cdio_destroy (cdio);
        cdio = NULL;
    }
}

static int
cda_seek_sample (int sample)
{
    int sector = sample / (SECTORSIZE / SAMPLESIZE) + first_sector;
    int offset = (sample % (SECTORSIZE / SAMPLESIZE)) * SAMPLESIZE; //in bytes
    char buf [SECTORSIZE];

    driver_return_code_t ret = cdio_read_audio_sector (cdio, buf, sector);
    if (ret != DRIVER_OP_SUCCESS)
        return -1;
    memcpy (tail, buf + offset, SECTORSIZE - offset );
    current_sector = sector;
    current_sample = sample;
    plugin.info.readpos = current_sample / 44100;
    return 0;
}

static int
cda_seek (float sec)
{
    return cda_seek_sample (sec * 44100);
}

cddb_disc_t*
resolve_disc (CdIo_t *cdio)
{
    track_t first_track = cdio_get_first_track_num (cdio);
    track_t tracks = cdio_get_num_tracks (cdio);
    track_t i;
    cddb_track_t *track;

    cddb_disc_t *disc = cddb_disc_new();

    cddb_disc_set_length (disc, cdio_get_track_lba (cdio, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC);

    for (i = 0; i < tracks; i++)
    {
        lsn_t offset = cdio_get_track_lba (cdio, i+first_track);
        track = cddb_track_new();
        cddb_track_set_frame_offset (track, offset);
        cddb_disc_add_track (disc, track);
    }
    cdio_destroy (cdio);

    cddb_conn_t *conn = NULL;

    conn = cddb_new();

    cddb_set_server_name (conn, server);
    cddb_set_server_port (conn, port);

    if (!proto_cddb)
    {
        cddb_http_enable (conn);
        if (proxy)
        {
            cddb_set_server_port(conn, proxy_port);
            cddb_set_server_name(conn, proxy);
        }
    }

    int matches = cddb_query (conn, disc);
    if (matches == -1)
    {
        cddb_disc_destroy (disc);
        cddb_destroy (conn);
        return NULL;
    }
    cddb_read (conn, disc);
    cddb_destroy (conn);
    return disc;
}

static DB_playItem_t *
insert_single_track (CdIo_t* cdio, DB_playItem_t *after, const char* file, int track_nr)
{
    char tmp[512]; //FIXME: how much?
    if (file)
        snprintf (tmp, sizeof (tmp), "%s#%d.cda", file, track_nr);
    else
        snprintf (tmp, sizeof (tmp), "#%d.cda", track_nr);

    if (TRACK_FORMAT_AUDIO != cdio_get_track_format (cdio, track_nr))
    {
        fprintf (stderr, "Not an audio track (%d)\n", track_nr);
        return NULL;
    }

    sector_count = cdio_get_track_sec_count (cdio, track_nr);

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (tmp);
    it->filetype = "cda";
    it->duration = (float)sector_count / 75.0;

    snprintf (tmp, sizeof (tmp), "CD Track %d", track_nr);
    deadbeef->pl_add_meta (it, "title", tmp);

    after = deadbeef->pl_insert_item (after, it);

    return after;
}

static void
cddb_thread (uintptr_t items_i)
{
    struct cddb_thread_params *params = (struct cddb_thread_params*)items_i;
    DB_playItem_t **items = params->items;
    DB_playItem_t *item;

    cddb_disc_t* disc = resolve_disc (params->cdio);
    if (!disc)
    {
        printf ("disc not resolved\n");
        return;
    }

    const char *disc_title = cddb_disc_get_title (disc);
    const char *artist = cddb_disc_get_artist (disc);
    cddb_track_t *track;
    int i;
    
    for (i = 0, track = cddb_disc_get_track_first (disc); items[i]; ++i, track = cddb_disc_get_track_next (disc))
    {
        int idx = deadbeef->pl_get_idx_of (items[i]);
        if (idx == -1)
            continue;

        deadbeef->pl_delete_all_meta (items[i]);
        deadbeef->pl_add_meta (items[i], "artist", artist);
        deadbeef->pl_add_meta (items[i], "album", disc_title);
        deadbeef->pl_add_meta (items[i], "title", cddb_track_get_title (track));
        deadbeef->sendmessage (M_TRACKCHANGED, 0, idx, 0);
    }
    cddb_disc_destroy (disc);
}

static DB_playItem_t *
cda_insert (DB_playItem_t *after, const char *fname) {
    printf ("CDA insert: %s\n", fname);

    int all = 0;
    int track_nr;
    DB_playItem_t *res;
    CdIo_t *cdio; //we need its local inst

    static DB_playItem_t *items[100];
    
    char* shortname = strdup (strrchr (fname, '/') + 1);
    const char *ext = strrchr (shortname, '.') + 1;
    int is_image = (0 == strcmp (ext, "nrg"));

    if (0 == strcmp (ext, "cda"))
        cdio = cdio_open (NULL, DRIVER_UNKNOWN);

    else if (is_image)
        cdio = cdio_open (fname, DRIVER_NRG);

    if (!cdio)
        return NULL;

    if (0 == strcasecmp (shortname, "all.cda") || is_image)
    {
        track_t first_track = cdio_get_first_track_num (cdio);
        track_t tracks = cdio_get_num_tracks (cdio);
        track_t i;
        res = after;
        for (i = 0; i < tracks; i++)
        {
            res = insert_single_track (cdio, res, is_image ? fname : NULL, i+first_track);
            items[i] = res;
        }
        items[tracks] = NULL;
        static struct cddb_thread_params p;
        p.items = items;
        p.cdio = cdio;
        deadbeef->thread_start (cddb_thread, (uintptr_t)&p); //will destroy cdio
    }
    else
    {
        track_nr = atoi (shortname);
        res = insert_single_track (cdio, after, NULL, track_nr);
        cdio_destroy (cdio);
    }
    return res;
}

static const char * exts[] = { "cda", "nrg", NULL };
static const char *filetypes[] = { "cda", "Nero CD image", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "Audio CD Player",
    .plugin.author = "Viktor Semykin",
    .plugin.email = "thesame.ml@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .init = cda_init,
    .free = cda_free,
    .read_int16 = cda_read_int16,
    .seek = cda_seek,
    .seek_sample = cda_seek_sample,
    .insert = cda_insert,
    .exts = exts,
    .id = "cda",
    .filetypes = filetypes,
};

DB_plugin_t *
cda_load (DB_functions_t *api) {
    deadbeef = api;
    read_config();
    return DB_PLUGIN (&plugin);
}

