/*
    CD audio plugin for DeaDBeeF
    Copyright (C) 2009 Viktor Semykin <thesame.ml@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* screwed/maintained by Alexey Yakovenko <waker@users.sourceforge.net> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <cddb/cddb.h>

#include "../../deadbeef.h"

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define DEFAULT_SERVER "freedb.org"
#define DEFAULT_PORT 888
#define DEFAULT_USE_CDDB 1
#define DEFAULT_PROTOCOL 1
#define DEFAULT_PREFER_CDTEXT 1

#define SECTORSIZE CDIO_CD_FRAMESIZE_RAW //2352
#define SAMPLESIZE 4 //bytes
#define BUFSIZE (CDIO_CD_FRAMESIZE_RAW * 2)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    CdIo_t* cdio;
    lsn_t first_sector;
    unsigned int sector_count;
    uint8_t tail [SECTORSIZE];
    unsigned int tail_len;
    int current_sector;
    unsigned int current_sample;
} cdda_info_t;

struct cddb_thread_params
{
    DB_playItem_t *items[100];
    CdIo_t *cdio;
};

static inline int
min (int a, int b) {
    return a < b ? a : b;
}

static DB_fileinfo_t *
cda_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (cdda_info_t));
    memset (_info, 0, sizeof (cdda_info_t));
    return _info;
}

static int
cda_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    cdda_info_t *info = (cdda_info_t *)_info;

    trace ("cdda: init %s\n", deadbeef->pl_find_meta (it, ":URI"));

    size_t l = strlen (deadbeef->pl_find_meta (it, ":URI"));
    char location[l+1];
    memcpy (location, deadbeef->pl_find_meta (it, ":URI"), l+1);

    char *nr = strchr (location, '#');
    if (nr) {
        *nr = 0; nr++;
    }
    else {
        trace ("cdda: bad name: %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }
    int track_nr = atoi (nr);
    char *fname = (*location) ? location : NULL; //NULL if empty string; means pysical CD drive

    info->cdio = cdio_open (fname, DRIVER_UNKNOWN);
    if  (!info->cdio)
    {
        trace ("cdda: Could not open CD\n");
        return -1;
    }

    track_t first_track = cdio_get_first_track_num (info->cdio);
    if (first_track == 0xff) {
        trace ("cdda: no medium found\n");
        return -1;
    }
    track_t tracks = cdio_get_num_tracks (info->cdio);
    track_t i;
    cddb_track_t *track;

    cddb_disc_t *disc = cddb_disc_new();

    cddb_disc_set_length (disc, cdio_get_track_lba (info->cdio, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC);

    for (i = 0; i < tracks; i++)
    {
        lsn_t offset = cdio_get_track_lba (info->cdio, i+first_track);
        track = cddb_track_new();
        cddb_track_set_frame_offset (track, offset);
        cddb_disc_add_track (disc, track);
    }
    cddb_disc_calc_discid (disc);
    int discid = cddb_disc_get_discid (disc);

    int trk_discid = deadbeef->pl_find_meta_int (it, ":CDIO_DISCID", 0);
    if (trk_discid != discid) {
        cddb_disc_destroy (disc);
        trace ("cdda: the track belongs to another disc, skipped\n");
        return -1;
    }
    cddb_disc_destroy (disc);


    if (TRACK_FORMAT_AUDIO != cdio_get_track_format (info->cdio, track_nr))
    {
        trace ("cdda: Not an audio track (%d)\n", track_nr);
        return -1;
    }

    int channels = cdio_get_track_channels (info->cdio, track_nr);
    trace ("cdio nchannels: %d\n", channels);

    _info->plugin = &plugin;
    _info->fmt.bps = 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = 44100;
    _info->fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT;
    _info->readpos = 0;

    info->first_sector = cdio_get_track_lsn (info->cdio, track_nr);
    info->sector_count = cdio_get_track_sec_count (info->cdio, track_nr);
    info->current_sector = info->first_sector;
    info->tail_len = 0;
    info->current_sample = 0;
    return 0;
}

int
cda_read (DB_fileinfo_t *_info, char *bytes, int size) {
    cdda_info_t *info = (cdda_info_t *)_info;
    int extrasize = 0;
    
    if (info->tail_len > 0)
    {
        if (info->tail_len >= size)
        {
//            trace ("Easy case\n");
            memcpy (bytes, info->tail, size);
            info->tail_len -= size;
            memmove (info->tail, info->tail+size, info->tail_len);
            return size;
        }
//        trace ("Prepending with tail of %d bytes\n", tail_len);
        extrasize = info->tail_len;
        memcpy (bytes, info->tail, info->tail_len);
        bytes += info->tail_len;
        size -= info->tail_len;
        info->tail_len = 0;
    }

    int sectors_to_read = size / SECTORSIZE + 1;
    int end = 0;
    
    if (info->current_sector + sectors_to_read > info->first_sector + info->sector_count) // reached end of track
    {
        end = 1;
        sectors_to_read = info->first_sector + info->sector_count - info->current_sector;
//        trace ("cdda: reached end of track\n");
    }

    int bufsize = sectors_to_read * SECTORSIZE;

    info->tail_len = end ? 0 : bufsize - size;

    char *buf = alloca (bufsize);

    driver_return_code_t ret = cdio_read_audio_sectors (info->cdio, buf, info->current_sector, sectors_to_read);
    if (ret != DRIVER_OP_SUCCESS)
        return 0;
    info->current_sector += sectors_to_read;

    int retsize = end ? bufsize : size;

    memcpy (bytes, buf, retsize);
    if (!end)
        memcpy (info->tail, buf+retsize, info->tail_len);

    retsize += extrasize;
//    trace ("requested: %d; tail_len: %d; size: %d; sectors_to_read: %d; return: %d\n", initsize, tail_len, size, sectors_to_read, retsize);
    info->current_sample += retsize / SAMPLESIZE;
    _info->readpos = (float)info->current_sample / _info->fmt.samplerate;
    return retsize;
}

static void
cda_free (DB_fileinfo_t *_info)
{
    if (_info) {
        cdda_info_t *info = (cdda_info_t *)_info;
        if (info->cdio) {
            cdio_destroy (info->cdio);
        }
        free (_info);
    }
}

static int
cda_seek_sample (DB_fileinfo_t *_info, int sample)
{
    cdda_info_t *info = (cdda_info_t *)_info;
    int sector = sample / (SECTORSIZE / SAMPLESIZE) + info->first_sector;
    int offset = (sample % (SECTORSIZE / SAMPLESIZE)) * SAMPLESIZE; //in bytes
    char buf [SECTORSIZE];

    driver_return_code_t ret = cdio_read_audio_sector (info->cdio, buf, sector);
    if (ret != DRIVER_OP_SUCCESS)
        return -1;
    memcpy (info->tail, buf + offset, SECTORSIZE - offset);
    info->current_sector = sector;
    info->current_sample = sample;
    _info->readpos = (float)info->current_sample / _info->fmt.samplerate;
    return 0;
}

static int
cda_seek (DB_fileinfo_t *_info, float sec)
{
    return cda_seek_sample (_info, sec * _info->fmt.samplerate);
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
    cddb_conn_t *conn = NULL;

    conn = cddb_new();

    deadbeef->conf_lock ();
    cddb_set_server_name (conn, deadbeef->conf_get_str_fast ("cdda.freedb.host", DEFAULT_SERVER));
    cddb_set_server_port (conn, deadbeef->conf_get_int ("cdda.freedb.port", DEFAULT_PORT));

    if (!deadbeef->conf_get_int ("cdda.protocol", DEFAULT_PROTOCOL))
    {
        cddb_http_enable (conn);
        if (deadbeef->conf_get_int ("network.proxy", 0))
        {
            cddb_set_server_port(conn, deadbeef->conf_get_int ("network.proxy.port", 8080));
            cddb_set_server_name(conn, deadbeef->conf_get_str_fast ("network.proxy.address", ""));
        }
    }
    deadbeef->conf_unlock ();

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
insert_single_track (CdIo_t* cdio, ddb_playlist_t *plt, DB_playItem_t *after, const char* file, int track_nr, int discid)
{
    char tmp[file ? strlen (file) + 20 : 20];
    if (file)
        snprintf (tmp, sizeof (tmp), "%s#%d.cda", file, track_nr);
    else
        snprintf (tmp, sizeof (tmp), "#%d.cda", track_nr);

    if (TRACK_FORMAT_AUDIO != cdio_get_track_format (cdio, track_nr))
    {
        trace ("Not an audio track (%d)\n", track_nr);
        return NULL;
    }

    int sector_count = cdio_get_track_sec_count (cdio, track_nr);

    DB_playItem_t *it = deadbeef->pl_item_alloc_init (tmp, plugin.plugin.id);
    deadbeef->pl_add_meta (it, ":FILETYPE", "cdda");
    deadbeef->plt_set_item_duration (plt, it, (float)sector_count / 75.0);

    snprintf (tmp, sizeof (tmp), "CD Track %02d", track_nr);
    deadbeef->pl_add_meta (it, "title", tmp);
    snprintf (tmp, sizeof (tmp), "%02d", track_nr);
    deadbeef->pl_add_meta (it, "track", tmp);

    deadbeef->pl_set_meta_int (it, ":CDIO_DISCID", discid);

    after = deadbeef->plt_insert_item (plt, after, it);

    return after;
}

static void
cleanup_thread_params (struct cddb_thread_params *params)
{
    int i;
    for (i = 0; params->items[i]; i++)
        deadbeef->pl_item_unref (params->items[i]);
    cdio_destroy (params->cdio);
    free (params);
}


static void
cddb_thread (void *items_i)
{
    struct cddb_thread_params *params = (struct cddb_thread_params*)items_i;
    DB_playItem_t **items = params->items;

    trace ("calling resolve_disc\n");
    cddb_disc_t* disc = resolve_disc (params->cdio);
    if (!disc)
    {
        trace ("disc not resolved\n");
        if (params->cdio) {
            cdio_destroy (params->cdio);
        }
        free (params);
        return;
    }
    trace ("disc resolved\n");

    const char *disc_title = cddb_disc_get_title (disc);
    const char *artist = cddb_disc_get_artist (disc);
    trace ("disc_title=%s, disk_artist=%s\n", disc_title, artist);
    cddb_track_t *track;
    int i;
    
    // FIXME: playlist must be locked before doing that
    int trk = 1;
    for (i = 0, track = cddb_disc_get_track_first (disc); items[i]; trk++, ++i, track = cddb_disc_get_track_next (disc))
    {
        // FIXME: problem will happen here if item(s) were deleted from playlist, and new items were added in their places
        // possible solutions: catch EV_TRACKDELETED and mark item(s) in every thread as NULL
        trace ("track %d, artist=%s, album=%s, title=%s\n", i, artist, disc_title, cddb_track_get_title (track));

        deadbeef->pl_delete_all_meta (items[i]);
        deadbeef->pl_add_meta (items[i], "artist", artist);
        deadbeef->pl_add_meta (items[i], "album", disc_title);
        deadbeef->pl_add_meta (items[i], "title", cddb_track_get_title (track));
        char tmp[5];
        snprintf (tmp, sizeof (tmp), "%02d", trk);
        deadbeef->pl_add_meta (items[i], "track", tmp);
        ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
        ev->track = items[i];
        if (ev->track) {
            deadbeef->pl_item_ref (ev->track);
        }
        deadbeef->event_send ((ddb_event_t *)ev, 0, 0);
    }
    cddb_disc_destroy (disc);
    cleanup_thread_params (params);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}

static void
read_track_cdtext (CdIo_t *cdio, int track_nr, DB_playItem_t *item)
{
    cdtext_t *cdtext = cdio_get_cdtext (cdio, 0);
    if (!cdtext)
    {
        trace ("No cdtext\n");
        return;
    }
    const char *artist = NULL;
    const char *album = NULL;
    int field_type;
    for (field_type = 0; field_type < MAX_CDTEXT_FIELDS; field_type++)
    {
        const char *text = cdtext_get_const (field_type, cdtext);
        const char *field = NULL;
        if (text)
        {
            switch (field_type)
            {
                case CDTEXT_TITLE: album = text; break;
                case CDTEXT_PERFORMER: artist = text; break;
            }
        }
    }

    trace ("artist: %s; album: %s\n", artist, album);
    if (artist) {
        deadbeef->pl_replace_meta (item, "artist", artist);
    }
    if (album) {
        deadbeef->pl_replace_meta (item, "album", album);
    }

    cdtext = cdio_get_cdtext (cdio, track_nr);
    if (!cdtext)
        return;

    for (field_type = 0; field_type < MAX_CDTEXT_FIELDS; field_type++)
    {
        const char *text = cdtext_get_const (field_type, cdtext);
        const char *field = NULL;
        if (!text)
            continue;
        switch (field_type)
        {
            case CDTEXT_TITLE:      field = "title";    break;
            case CDTEXT_PERFORMER:  field = "artist";   break;
            case CDTEXT_COMPOSER:   field = "composer"; break;
            case CDTEXT_GENRE:      field = "genre";    break;
            case CDTEXT_SONGWRITER: field = "songwriter";   break;
            case CDTEXT_MESSAGE:    field = "comment";  break;
            default: field = NULL;
        }
        if (field && text)
        {
            trace ("%s: %s\n", field, text);
            deadbeef->pl_replace_meta (item, field, text);
        }
    }
}

static int
read_disc_cdtext (struct cddb_thread_params *params)
{
    DB_playItem_t **items = params->items;
    cdtext_t *cdtext = cdio_get_cdtext (params->cdio, 0);
    if (!cdtext)
        return 0;

    track_t first_track = cdio_get_first_track_num (params->cdio);
    track_t tracks = cdio_get_num_tracks (params->cdio);
    track_t i;
    for (i = 0; i < tracks; i++)
        read_track_cdtext (params->cdio, i + first_track, params->items[i]);

    return 1;
}

static DB_playItem_t *
cda_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("CDA insert: %s\n", fname);
    CdIo_t* cdio = NULL;
    int track_nr;
    DB_playItem_t *res;

    const char* shortname = strrchr (fname, '/');
    if (shortname) {
        shortname++;
    }
    else {
        shortname = fname;
    }
    const char *ext = strrchr (shortname, '.') + 1;
    int is_image = ext && (0 == strcmp (ext, "nrg"));
    if (is_image && !deadbeef->conf_get_int ("cdda.enable_nrg", 0)) {
        return NULL;
    }

    if (0 == strcmp (ext, "cda")) {
        cdio = cdio_open (NULL, DRIVER_UNKNOWN);
    }
    else if (is_image) {
        cdio = cdio_open (fname, DRIVER_NRG);
    }

    if (!cdio) {
        trace ("not an audio disc/image, or file not found (%s)\n", fname);
        return NULL;
    }


    // calculate discid
    track_t first_track = cdio_get_first_track_num (cdio);
    if (first_track == 0xff) {
        trace ("cdda: no medium found\n");
        cdio_destroy (cdio);
        return NULL;
    }
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
    cddb_disc_calc_discid (disc);
    int discid = cddb_disc_get_discid (disc);

    if (0 == strcasecmp (shortname, "all.cda") || is_image)
    {
        track_t i;
        res = after;
        struct cddb_thread_params *p = malloc (sizeof (struct cddb_thread_params));
        memset (p, 0, sizeof (struct cddb_thread_params));
        p->cdio = cdio;

        int enable_cddb = deadbeef->conf_get_int ("cdda.freedb.enable", DEFAULT_USE_CDDB);

        for (i = 0; i < tracks; i++)
        {
            trace ("inserting track %d\n", i);
            res = insert_single_track (cdio, plt, res, is_image ? fname : NULL, i+first_track, discid);
            if (res) {
                p->items[i] = res;
            }
        }

        int got_cdtext = read_disc_cdtext (p);
        int prefer_cdtext = deadbeef->conf_get_int ("cdda.prefer_cdtext", DEFAULT_PREFER_CDTEXT);

        if ((!got_cdtext || !prefer_cdtext) && enable_cddb)
        {
            trace ("cdda: querying freedb...\n");
            intptr_t tid = deadbeef->thread_start (cddb_thread, p); //will destroy cdio
            deadbeef->thread_detach (tid);
        }
        else
            cleanup_thread_params (p);
    }
    else
    {
        track_nr = atoi (shortname);
        res = insert_single_track (cdio, plt, after, NULL, track_nr, discid);
        if (res) {
            read_track_cdtext (cdio, track_nr, res);
            deadbeef->pl_item_unref (res);
        }
        cdio_destroy (cdio);
    }
    cddb_disc_destroy (disc);
    return res;
}

static int
cda_action_add_cd (DB_plugin_action_t *act, DB_playItem_t *it)
{
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->pl_add_files_begin (plt);
        deadbeef->plt_add_file (plt, "all.cda", NULL, NULL);
        deadbeef->pl_add_files_end ();
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}

static DB_plugin_action_t add_cd_action = {
    .title = "File/Add Audio CD",
    .flags = DB_ACTION_COMMON,
    .callback = DDB_ACTION_CALLBACK(cda_action_add_cd),
    .next = NULL
};

static DB_plugin_action_t *
cda_get_actions (DB_playItem_t *unused)
{
    return &add_cd_action;
}

static const char *exts[] = { "cda", "nrg", NULL };
static const char settings_dlg[] =
    "property \"Use CDDB/FreeDB\" checkbox cdda.freedb.enable 1;\n"
    "property \"Prefer CD-Text over CDDB\" checkbox cdda.prefer_cdtext 1;\n"
    "property \"CDDB url (e.g. 'freedb.org')\" entry cdda.freedb.host freedb.org;\n"
    "property \"CDDB port number (e.g. '888')\" entry cdda.freedb.port 888;\n"
    "property \"Prefer CDDB protocol over HTTP\" checkbox cdda.protocol 1;\n"
    "property \"Enable NRG image support\" checkbox cdda.enable_nrg 0;"
;

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "cda",
    .plugin.name = "Audio CD player",
    .plugin.descr = "Audio CD plugin using libcdio and libcddb",
    .plugin.copyright = 
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
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.configdialog = settings_dlg,
    .plugin.get_actions = cda_get_actions,
    .open = cda_open,
    .init = cda_init,
    .free = cda_free,
    .read = cda_read,
    .seek = cda_seek,
    .seek_sample = cda_seek_sample,
    .insert = cda_insert,
    .exts = exts,
};

DB_plugin_t *
cdda_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

