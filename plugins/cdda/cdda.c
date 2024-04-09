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

/* screwed/maintained by Oleksiy Yakovenko <waker@users.sourceforge.net> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if USE_PARANOIA_10_2
    #include <cdio/paranoia/cdda.h>
    #include <cdio/paranoia/paranoia.h>
#elif USE_PARANOIA
#if USE_CDDA_SUBDIR
    #include <cdda/cdda_interface.h>
    #include <cdda/cdda_paranoia.h>
#else
    #include <cdda_interface.h>
    #include <cdda_paranoia.h>
#endif
#endif
#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cdio/cdtext.h>
#include <cddb/cddb.h>

#include <deadbeef/deadbeef.h>

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define CDDA_ALL_TRACKS "all.cda"

#define DEFAULT_SERVER "gnudb.gnudb.org"
#define DEFAULT_PORT 8880
#define DEFAULT_USE_CDDB 1
#define DEFAULT_PROTOCOL 1
#define DEFAULT_PREFER_CDTEXT 1

#define SECTORSIZE CDIO_CD_FRAMESIZE_RAW //2352
#define SAMPLESIZE 4 //bytes

#define CDDB_CATEGORY_SIZE 12
#define CDDB_DISCID_SIZE 10
#define MAX_CDDB_DISCS 10
#define MAX_CDDB_MENU 80

#define CDDB_DISCID_TAG ":CDDB_DISCID"
#define CDDB_IDS_TAG ":CDDB IDs"

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct
{
    DB_fileinfo_t info;
    uint32_t hints;
    CdIo_t *cdio;
#if USE_PARANOIA
    cdrom_paranoia *paranoia;
    cdrom_drive *cdrom;
#endif
    lsn_t first_sector;
    lsn_t current_sector;
    lsn_t last_sector;
    uint8_t buffer[SECTORSIZE];
    uint8_t *tail;
    size_t tail_length;
} cdda_info_t;

struct cddb_thread_params
{
    DB_playItem_t **items;
    cddb_disc_t *disc;
    int got_cdtext;
    int prefer_cdtext;
};

static cddb_disc_t *
create_disc(CdIo_t *cdio)
{
    cddb_disc_t *disc = cddb_disc_new();
    if (disc) {
        const lba_t leadout_lba = cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK);
        cddb_disc_set_length(disc, leadout_lba / CDIO_CD_FRAMES_PER_SEC);
        const track_t first_track = cdio_get_first_track_num(cdio);
        const track_t num_tracks = cdio_get_num_tracks(cdio);
        if (leadout_lba == CDIO_INVALID_LBA || first_track == CDIO_INVALID_TRACK || num_tracks == CDIO_INVALID_TRACK) {
            trace("cda: create_disc failed, invalid CD disc format\n");
            cddb_disc_destroy(disc);
            return NULL;
        }

        const track_t last_track = first_track + num_tracks;
        for (track_t i = first_track; i < last_track; i++) {
            cddb_track_t *track = cddb_track_new();
            if (!track) {
                cddb_disc_destroy(disc);
                return NULL;
            }
            const lba_t offset = cdio_get_track_lba(cdio, i);
            cddb_track_set_frame_offset(track, offset);
            cddb_disc_add_track(disc, track);
        }
        cddb_disc_calc_discid(disc);
    }

    return disc;
}

static DB_fileinfo_t *
cda_open (uint32_t hints)
{
    cdda_info_t *info = calloc(1, sizeof (cdda_info_t));
    if (info) {
        info->hints = hints;
        info->info.plugin = &plugin;
        info->info.fmt.bps = 16;
        info->info.fmt.channels = 2;
        info->info.fmt.samplerate = 44100;
        info->info.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT;
    }
    return (DB_fileinfo_t *)info;
}

static int
cda_init (DB_fileinfo_t *_info, DB_playItem_t *it)
{
    deadbeef->pl_lock();
    const char *uri = deadbeef->pl_find_meta(it, ":URI");
    const char *nr = uri ? strchr(uri, '#') : NULL;
    if (!nr || nr == uri) {
        deadbeef->pl_unlock();
        trace ("cdda: bad name: %s\n", uri);
        return -1;
    }

    trace ("cdda: init %s\n", uri);
    const int track_nr = atoi(nr+1);
    const size_t device_length = nr - uri;
    char device[device_length+1];
    strncpy(device, uri, device_length);
    device[device_length] = '\0';
    deadbeef->pl_unlock();

    cdda_info_t *info = (cdda_info_t *)_info;
    info->cdio = cdio_open(device, DRIVER_UNKNOWN);
    if  (!info->cdio) {
        trace ("cdda: Could not open CD device\n");
        return -1;
    }

    const int need_bitrate = info->hints & DDB_DECODER_HINT_NEED_BITRATE;
    const int drive_speed = deadbeef->conf_get_int("cdda.drive_speed", 2);
    cdio_set_speed(info->cdio, need_bitrate && drive_speed < 5 ? 1<<drive_speed : -1);

    cddb_disc_t *disc = create_disc(info->cdio);
    if (!disc) {
        return -1;
    }

    const unsigned long discid = cddb_disc_get_discid(disc);
    cddb_disc_destroy(disc);

    deadbeef->pl_lock();
    const char *discid_hex = deadbeef->pl_find_meta(it, CDDB_DISCID_TAG);
    const unsigned long trk_discid = discid_hex ? strtoul(discid_hex, NULL, 16) : 0;
    deadbeef->pl_unlock();
    if (trk_discid != discid) {
        trace ("cdda: the track belongs to another disc (%x vs %x), skipped\n", trk_discid, discid);
        return -1;
    }

    if (cdio_get_track_format(info->cdio, track_nr) != TRACK_FORMAT_AUDIO) {
        trace ("cdda: Not an audio track (%d)\n", track_nr);
        return -1;
    }

    info->first_sector = cdio_get_track_lsn(info->cdio, track_nr);
    info->current_sector = info->first_sector;
    info->last_sector = info->first_sector + cdio_get_track_sec_count(info->cdio, track_nr) - 1;
    trace("cdio nchannels (should always be 2 for an audio track): %d\n", cdio_get_track_channels (info->cdio, track_nr));
    if (info->first_sector == CDIO_INVALID_LSN || info->last_sector <= info->first_sector) {
        trace ("cdda: invalid track\n");
        return -1;
    }

#if USE_PARANOIA
    if (cdio_get_driver_id(info->cdio) != DRIVER_NRG) {
        info->cdrom = cdda_identify(device, CDDA_MESSAGE_FORGETIT, NULL);
        if (info->cdrom) {
            cdda_open(info->cdrom);
            info->paranoia = paranoia_init(info->cdrom);
        }
        if (!info->paranoia) {
            trace ("cdda: cannot re-open %s for paranoia\n", device);
            return -1;
        }
        const int no_paranoia = need_bitrate || !deadbeef->conf_get_int("cdda.paranoia", 0);
        if (no_paranoia) {
            paranoia_cachemodel_size(info->paranoia, 100);
        }
        paranoia_modeset(info->paranoia, no_paranoia ? PARANOIA_MODE_DISABLE : PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP);
        paranoia_seek(info->paranoia, info->first_sector, SEEK_SET);
    }
#endif
    return 0;
}

#if USE_PARANOIA
static void
paranoia_callback(long bytes, int mode)
{
    if (mode > 1 && mode != 9)
        fprintf(stderr, "%ld %d\n", bytes, mode);
}
#endif

static const char *
read_sector(cdda_info_t *info)
{
#if USE_PARANOIA
    if (info->paranoia) {
        const int16_t *p_readbuf = paranoia_read(info->paranoia, NULL);
        if (p_readbuf) {
            info->current_sector++;
            return (char *)p_readbuf;
        }
    }
    else
#endif
    if (!cdio_read_audio_sector(info->cdio, info->buffer, info->current_sector)) {
        info->current_sector++;
        return info->buffer;
    }

    return NULL;
}

static int
cda_read (DB_fileinfo_t *_info, char *bytes, int size)
{
    cdda_info_t *info = (cdda_info_t *)_info;
    char *fill = bytes;
    const char *high_water = bytes + size;

    if (info->tail_length >= size) {
        memcpy(fill, info->tail, size);
        info->tail += size;
        fill += size;
        info->tail_length -= size;
    }
    else if (info->tail_length > 0) {
        memcpy(fill, info->tail, info->tail_length);
        fill += info->tail_length;
        info->tail_length = 0;
    }

    while (fill < high_water && info->current_sector <= info->last_sector) {
        const char *p_readbuf = read_sector(info);
        if (!p_readbuf) {
            trace("cda_read: read_sector failed\n");
            return -1;
        }

        if (fill+SECTORSIZE <= high_water) {
            memcpy(fill, p_readbuf, SECTORSIZE);
            fill += SECTORSIZE;
        }
        else {
            const size_t bytes_left = high_water - fill;
            memcpy(fill, p_readbuf, bytes_left);
            fill += bytes_left;
            info->tail_length = SECTORSIZE - bytes_left;
            info->tail = (char *)p_readbuf + bytes_left;
        }
    }

//    trace ("requested: %d, return: %d\n", size, fill-bytes);
    _info->readpos = (float)(info->current_sector-info->first_sector) * SECTORSIZE / SAMPLESIZE / _info->fmt.samplerate;
    return fill - bytes;
}

static void
cda_free (DB_fileinfo_t *_info)
{
    if (_info) {
        cdda_info_t *info = (cdda_info_t *)_info;
        if (info->cdio) {
            cdio_destroy (info->cdio);
        }
#if USE_PARANOIA
        if (info->paranoia) {
            paranoia_free(info->paranoia);
        }
        if (info->cdrom) {
            cdda_close(info->cdrom);
        }
#endif
        free (info);
    }
}

static int
cda_seek_sample (DB_fileinfo_t *_info, int sample)
{
    trace("cda_seek_sample %d\n", sample);
    cdda_info_t *info = (cdda_info_t *)_info;
    const int sector = sample * SAMPLESIZE / SECTORSIZE + info->first_sector;
    const int offset = sample * SAMPLESIZE % SECTORSIZE;

#if USE_PARANOIA
    if (info->paranoia) {
        paranoia_seek(info->paranoia, sector, SEEK_SET);
    }
#endif
    info->current_sector = sector;
    const char *p_readbuf = read_sector(info);
    if (!p_readbuf) {
        return -1;
    }

    info->tail = (char *)p_readbuf + offset;
    info->tail_length = SECTORSIZE - offset;
    _info->readpos = (float)sample / _info->fmt.samplerate;

    return 0;
}

static int
cda_seek (DB_fileinfo_t *_info, float sec)
{
    return cda_seek_sample (_info, sec * _info->fmt.samplerate);
}

static cddb_conn_t *
new_cddb_connection(void)
{
    cddb_conn_t *conn = cddb_new();
    if (conn) {
        deadbeef->conf_lock ();
        cddb_set_server_name (conn, deadbeef->conf_get_str_fast ("cdda.freedb.host", DEFAULT_SERVER));
        cddb_set_server_port (conn, deadbeef->conf_get_int ("cdda.freedb.port", DEFAULT_PORT));
        if (!deadbeef->conf_get_int ("cdda.protocol", DEFAULT_PROTOCOL)) {
            cddb_http_enable (conn);
            if (deadbeef->conf_get_int ("network.proxy", 0)) {
                cddb_set_server_port(conn, deadbeef->conf_get_int ("network.proxy.port", 8080));
                cddb_set_server_name(conn, deadbeef->conf_get_str_fast ("network.proxy.address", ""));
            }
        }
        deadbeef->conf_unlock ();
    }

    return conn;
}

static int
resolve_disc (cddb_disc_t* disc, char *disc_list)
{
    trace("cda: resolve_disc\n");

    cddb_conn_t *conn = new_cddb_connection();
    if (!conn) {
        return 0;
    }

    cddb_disc_t *temp_disc = cddb_disc_clone(disc);
    cddb_disc_t *query_disc = disc;

    cddb_cache_disable(conn);
    const int matches = cddb_query(conn, query_disc);

    cddb_cache_enable(conn);
    size_t discs_read = 0;
    disc_list[0] = '\0';
    for (int i = 0; i < matches; i++) {
        if (cddb_read(conn, query_disc) && discs_read < MAX_CDDB_DISCS) {
            discs_read++;
            char temp_string[CDDB_CATEGORY_SIZE + CDDB_DISCID_SIZE + 1];
            sprintf(temp_string, ",%s/%08x", cddb_disc_get_category_str(query_disc), cddb_disc_get_discid(query_disc));
            strcat(disc_list, temp_string);
            query_disc = temp_disc;
        }
        cddb_query_next(conn, query_disc);
    }

    cddb_disc_destroy(temp_disc);
    cddb_destroy(conn);

    return discs_read;
}

static void
cleanup_thread_params (struct cddb_thread_params *params)
{
    trace("cleanup_thread_params\n");
    if (params->items) {
        for (size_t i = 0; params->items[i]; i++) {
            deadbeef->pl_item_unref(params->items[i]);
        }
        free(params->items);
    }
    if (params->disc) {
        cddb_disc_destroy(params->disc);
    }
    free(params);
}

static void
replace_meta(struct cddb_thread_params *params, DB_playItem_t *it, const char *key, const char *value)
{
    if (params && params->got_cdtext && deadbeef->pl_find_meta (it, key) && params->prefer_cdtext) {
        // already got the preferred value from cdtext
        return;
    }

    if (value) {
        deadbeef->pl_replace_meta(it, key, value);
    }
    else {
        deadbeef->pl_delete_meta(it, key);
    }
}

static void
write_metadata(struct cddb_thread_params *params, DB_playItem_t *it, const cddb_disc_t *disc, const char *num_tracks)
{
    const int track_nr = deadbeef->pl_find_meta_int(it, "track", 0);
    cddb_track_t *track = cddb_disc_get_track(disc, track_nr-1);

    replace_meta(params, it, "artist", cddb_disc_get_artist(disc));
    replace_meta(params, it, "title", cddb_track_get_title(track));
    replace_meta(params, it, "album", cddb_disc_get_title(disc));
    replace_meta(params, it, "genre", cddb_disc_get_genre(disc));
    const unsigned int year = cddb_disc_get_year(disc);
    year ? deadbeef->pl_set_meta_int(it, "year", year) : deadbeef->pl_delete_meta(it, "year");
    replace_meta(params, it, "numtracks", num_tracks);

    // if the title is still empty, fill with autogenerated title
    if (!deadbeef->pl_find_meta (it, "title")) {
        char title[50];
        snprintf(title, sizeof (title), "CD Track %02d", track_nr);
        deadbeef->pl_add_meta(it, "title", title);
    }

    ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_TRACKINFOCHANGED);
    ev->track = it;
    if (ev->track) {
        deadbeef->pl_item_ref(ev->track);
    }
    deadbeef->event_send((ddb_event_t *)ev, 0, 0);
}

static void
cddb_thread (void *params_void)
{
    struct cddb_thread_params *params = (struct cddb_thread_params *)params_void;

    char disc_list[(CDDB_CATEGORY_SIZE + CDDB_DISCID_SIZE + 1) * MAX_CDDB_DISCS];
    const int num_discs = resolve_disc(params->disc, disc_list);
    if (num_discs <= 0) {
        trace("disc not resolved\n");
        cleanup_thread_params(params);
        return;
    }
    trace("disc resolved\n");

    char num_tracks[4];
    int track_count = cddb_disc_get_track_count(params->disc);
    snprintf(num_tracks, sizeof(num_tracks), "%02d", track_count);
    for (size_t i = 0; params->items[i]; i++) {
        deadbeef->pl_add_meta(params->items[i], CDDB_IDS_TAG, disc_list);
        write_metadata(params, params->items[i], params->disc, num_tracks);
    }

    cleanup_thread_params(params);

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

static void
read_track_cdtext (CdIo_t *cdio, int track_nr, DB_playItem_t *item)
{
#if CDIO_API_VERSION >= 6
    cdtext_t *cdtext = cdio_get_cdtext (cdio);
#else
    cdtext_t *cdtext = cdio_get_cdtext (cdio, 0);
#endif
    if (!cdtext) {
        trace ("No cdtext\n");
        return;
    }
    const char *artist = NULL;
    const char *album = NULL;
    int field_type;
    for (field_type = 0; field_type < MAX_CDTEXT_FIELDS; field_type++) {
#if CDIO_API_VERSION >= 6
        const char *text = cdtext_get_const (cdtext, field_type, 0);
        if (!text) {
            text = cdtext_get_const (cdtext, field_type, track_nr);
        }
#else
        const char *text = cdtext_get_const (field_type, cdtext);
#endif
        if (text) {
            switch (field_type) {
#if CDIO_API_VERSION >= 6
                case CDTEXT_FIELD_TITLE: album = text; break;
                case CDTEXT_FIELD_PERFORMER: artist = text; break;
#else
                case CDTEXT_TITLE: album = text; break;
                case CDTEXT_PERFORMER: artist = text; break;
#endif
            }
        }
    }

    // some versions of cdio does not convert strings to UTF-8 as documented
    if (album) {
        const char *album_charset = deadbeef->junk_detect_charset (album);
        char *album_converted = NULL;
        if (album_charset) {
            trace ( "cdda: album field using %s charset, converting\n",album_charset);
            char *album_converted = malloc (strlen(album) * 4);
            if (album_converted) {
                deadbeef->junk_iconv (album, strlen(album), album_converted, strlen(album)*4, album_charset, "UTF-8");
                album = album_converted;
            }
        }
        replace_meta(NULL, item, "album", album);
        if (album_converted) {
            free (album_converted);
            album = album_converted = NULL;
        }
    }

    if (artist) {
        const char *artist_charset = deadbeef->junk_detect_charset (artist);
        char *artist_converted = NULL;
        if (artist_charset != NULL) {
            trace ( "cdda: artist field using %s charset, converting\n",artist_charset);
            char *artist_converted = malloc (strlen(artist) * 4);
            if (artist_converted) {
                deadbeef->junk_iconv (artist, strlen(artist), artist_converted, strlen(artist)*4, artist_charset, "UTF-8");
                artist = artist_converted;
            }
        }
        replace_meta(NULL, item, "artist", artist);

        if (artist_charset) {
            free (artist_converted);
            artist = artist_converted = NULL;
        }
    }

#if CDIO_API_VERSION >= 6
    cdtext = cdio_get_cdtext (cdio);
#else
    cdtext = cdio_get_cdtext (cdio, track_nr);
#endif
    if (!cdtext) {
        return;
    }

    for (field_type = 0; field_type < MAX_CDTEXT_FIELDS; field_type++) {
#if CDIO_API_VERSION >= 6
        const char *text = cdtext_get_const (cdtext, field_type, track_nr);
#else
        const char *text = cdtext_get_const (field_type, cdtext);
#endif
        if (!text) {
            continue;
        }

        const char *field = NULL;
        switch (field_type) {
#if CDIO_API_VERSION >= 6
            case CDTEXT_FIELD_TITLE:      field = "title";    break;
            case CDTEXT_FIELD_PERFORMER:  field = "artist";   break;
            case CDTEXT_FIELD_COMPOSER:   field = "composer"; break;
            case CDTEXT_FIELD_GENRE:      field = "genre";    break;
            case CDTEXT_FIELD_SONGWRITER: field = "songwriter";   break;
            case CDTEXT_FIELD_MESSAGE:    field = "comment";  break;
#else
            case CDTEXT_TITLE:      field = "title";    break;
            case CDTEXT_PERFORMER:  field = "artist";   break;
            case CDTEXT_COMPOSER:   field = "composer"; break;
            case CDTEXT_GENRE:      field = "genre";    break;
            case CDTEXT_SONGWRITER: field = "songwriter";   break;
            case CDTEXT_MESSAGE:    field = "comment";  break;
#endif
            default: field = NULL;
        }
        if (field) {
            // convert to UTF-8 if not UTF-8 already
            const char *text_charset = deadbeef->junk_detect_charset (text);
            char *text_converted = NULL;
            if (text_charset != NULL) {
                trace ("cdda: text field using %s charset, converting\n",text_charset);
                text_converted = malloc (strlen(text) * 4);
                if (text_converted) {
                    deadbeef->junk_iconv (field, strlen(text), text_converted, strlen(text)*4, text_charset, "UTF-8");
                    text = text_converted;
                }
            }
            trace("%s: %s\n", field, text);
            replace_meta(NULL, item, field, text);
            if (text_converted && text_charset != NULL) {
                free (text_converted);
                text = text_converted = NULL;
            }
        }
    }
}

static int
read_disc_cdtext (CdIo_t *cdio, DB_playItem_t **items, const track_t tracks)
{
#if CDIO_API_VERSION >= 6
    cdtext_t *cdtext = cdio_get_cdtext(cdio);
#else
    cdtext_t *cdtext = cdio_get_cdtext(cdio, 0);
#endif
    if (!cdtext) {
        return 0;
    }

    for (track_t i = 0; i < tracks; i++) {
        read_track_cdtext(cdio, deadbeef->pl_find_meta_int(items[i], "track", 0), items[i]);
    }

    return 1;
}

static DB_playItem_t *
insert_track (ddb_playlist_t *plt, DB_playItem_t *after, const char* path, const track_t track_nr, CdIo_t *cdio, const int discid)
{
    char fname[strlen(path) + 10];
    sprintf(fname, "%s#%d.cda", path, track_nr);
    DB_playItem_t *it = deadbeef->pl_item_alloc_init(fname, plugin.plugin.id);
    if (it) {
        deadbeef->pl_add_meta(it, ":FILETYPE", "cdda");

        const float sector_count = cdio_get_track_sec_count(cdio, track_nr);
        deadbeef->plt_set_item_duration(plt, it, sector_count / 75);

        char track[4];
        snprintf(track, sizeof (track), "%02d", track_nr);
        deadbeef->pl_add_meta(it, "track", track);

        char discid_string[10];
        snprintf(discid_string, sizeof (discid_string), "%08x", discid);
        deadbeef->pl_add_meta(it, CDDB_DISCID_TAG, discid_string);

        it = deadbeef->plt_insert_item(plt, after, it);
    }

    return it;
}

static DB_playItem_t *
insert_disc (ddb_playlist_t *plt, DB_playItem_t *after, const char *path, const track_t single_track, CdIo_t* cdio)
{
    struct cddb_thread_params *p = calloc(1, sizeof(struct cddb_thread_params));
    if (!p) {
        return NULL;
    }

    p->disc = create_disc(cdio);
    if (!p->disc) {
        cleanup_thread_params(p);
        return NULL;
    }

    const track_t tracks = single_track ? 1 : cddb_disc_get_track_count(p->disc);
    p->items = calloc(tracks+1, sizeof(*p->items));
    if (!p->items) {
        cleanup_thread_params(p);
        return NULL;
    }

    const unsigned long discid = cddb_disc_get_discid(p->disc);
    DB_playItem_t *inserted = NULL;
    const track_t first_track = single_track ? single_track : cdio_get_first_track_num(cdio);
    track_t item_count = 0;
    for (track_t i = 0; i < tracks; i++) {
        if (cdio_get_track_format(cdio, first_track+i) == TRACK_FORMAT_AUDIO) {
            trace("inserting track %d from %s\n", first_track+i, path);
            inserted = insert_track(plt, after, path, first_track+i, cdio, discid);
            p->items[item_count++] = inserted;
            after = inserted;
        }
    }

    intptr_t tid = 0;
    if (item_count) {
        const int got_cdtext = read_disc_cdtext(cdio, p->items, tracks);
        const int prefer_cdtext = deadbeef->conf_get_int("cdda.prefer_cdtext", DEFAULT_PREFER_CDTEXT);
        const int enable_cddb = deadbeef->conf_get_int("cdda.freedb.enable", DEFAULT_USE_CDDB);
        p->got_cdtext = got_cdtext;
        p->prefer_cdtext = prefer_cdtext;
        if (enable_cddb) {
            tid = deadbeef->thread_start(cddb_thread, p);
            if (tid) {
                deadbeef->thread_detach(tid);
            }
        }
        else {
            // if cddb is off, fill empty titles with autogenerated ones
            for (int i = 0; i < item_count; i++) {
                DB_playItem_t *it = p->items[i];
                if (!deadbeef->pl_find_meta (it, "title")) {
                    const int track_nr = deadbeef->pl_find_meta_int(it, "track", 0);
                    char title[50];
                    snprintf(title, sizeof (title), "CD Track %02d", track_nr);
                    deadbeef->pl_add_meta(it, "title", title);
                }
            }
        }
    }

    if (!tid) {
        cleanup_thread_params(p);
    }

    return inserted;
}

static DB_playItem_t *
cda_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *path)
{
    trace("CDA insert: %s\n", path);
    cdio_close_tray(NULL, NULL);

    /* Deal with any NRG files and get them out of the way */
    const char *ext = strrchr(path, '.');
    if (ext && !strcasecmp(ext, ".nrg")) {
        if (!deadbeef->conf_get_int("cdda.enable_nrg", 0)) {
            trace("cda: NRG found but disabled in preferences\n");
            return NULL;
        }
        CdIo_t* cdio = cdio_open(path, DRIVER_NRG);
        if (!cdio) {
            trace("not an NRG image, or file not found (%s)\n", path);
            return NULL;
        }
        DB_playItem_t *inserted = insert_disc(plt, after, path, 0, cdio);
        cdio_destroy(cdio);
        return inserted;
    }

    /* Get a list of all devices containing CD audio */
    driver_id_t driver_id;
    char **device_list = cdio_get_devices_with_cap_ret(NULL, CDIO_FS_AUDIO, false, &driver_id);
    if (!device_list) {
        trace("cda: no audio drives found\n");
        return NULL;
    }

    /* Match the device name for the requested insert (invalid devices may crash cdio) */
    const char *sep = strrchr(path, '/');
    char *drive_device = NULL;
    if (sep) {
        char *real_path = realpath(path, NULL);
        if (!real_path) {
            const size_t device_length = sep - path;
            char device_path[device_length+1];
            strncpy(device_path, path, device_length);
            device_path[device_length] = '\0';
            real_path = realpath(device_path, NULL);
        }
        if (real_path) {
            for (size_t i = 0; device_list[i] && !drive_device; i++) {
                char *real_device = realpath(device_list[i], NULL);
                if (real_device) {
                    if (!strcmp(real_device, real_path)) {
                        drive_device = device_list[i];
                    }
                    free(real_device);
                }
            }
            free(real_path);
        }
    }
    else {
        drive_device = device_list[0];
    }

    /* Open the device and insert the requested track(s) */
    DB_playItem_t *inserted = NULL;
    if (drive_device) {
        trace("cda: try to open device %s\n", drive_device);
        CdIo_t* cdio = cdio_open(drive_device, driver_id);
        if (cdio) {
            char *track_end;
            const unsigned long track_nr = strtoul(sep ? sep + 1 : path, &track_end, 10);
            const track_t single_track = strcmp(track_end, ".cda") || track_nr > CDIO_CD_MAX_TRACKS ? 0 : track_nr;
            inserted = insert_disc(plt, after, drive_device, single_track, cdio);
            cdio_destroy(cdio);
        }
    }
    cdio_free_device_list(device_list);
    return inserted;
}

static int dialog_combo_index;

static void
set_param (const char *key, const char *value)
{
    dialog_combo_index = atoi(value);
    if (dialog_combo_index < 0) {
        dialog_combo_index = 0;
    }
    return;
}

static void
get_param (const char *key, char *value, int len, const char *def)
{
    strcpy(value, "0");
    return;
}

#define DRIVE_COMBO_SCRIPT \
"property box vbox[1] hmg expand fill border=10 height=250;"\
"property box hbox[1] hmg height=-1;"\
"property \"CD drive to load\" select[%u] cdda.drive_device 0"

static int
cda_action_add_cd (DB_plugin_action_t *act, ddb_action_context_t ctx)
{
    /* Get all devices containg CD audio media */
    cdio_close_tray(NULL, NULL);
    char **device_list = cdio_get_devices_with_cap(NULL, CDIO_FS_AUDIO, false);
    if (!device_list) {
        return 0;
    }

    char *drive_device = NULL;
    if (device_list[0] && device_list[1]) {
        /* Multiple devices, ask the user to pick one */
        size_t device_count;
        size_t device_combo_length = sizeof(DRIVE_COMBO_SCRIPT);
        for (device_count = 0; device_list[device_count]; device_count++) {
            device_combo_length += strlen(device_list[device_count]) + 1;
        }

        char *layout = malloc(device_combo_length);
        if (layout) {
            snprintf(layout, device_combo_length, DRIVE_COMBO_SCRIPT, (unsigned)device_count);
            for (char **device = device_list; *device; device++) {
                strcat(layout, " ");
                strcat(layout, *device);
            }
            strcat(layout, ";");

            ddb_dialog_t conf = {
                .title = "Audio CD Drive",
                .layout = layout,
                .set_param = set_param,
                .get_param = get_param,
                .parent = NULL
            };

            struct DB_plugin_s **plugin_list;
            for (plugin_list = deadbeef->plug_get_list(); *plugin_list && (*plugin_list)->type != DB_PLUGIN_GUI; plugin_list++);
            if (*plugin_list) {
                DB_gui_t *gui_plugin = (DB_gui_t *)*plugin_list;
                if (gui_plugin->run_dialog(&conf, 1<<ddb_button_ok|1<<ddb_button_cancel, NULL, NULL) == ddb_button_ok) {
                    drive_device = device_list[dialog_combo_index];
                }
            }
            free(layout);
        }
    }
    else if (device_list[0]) {
        drive_device = device_list[0];
    }

    /* If the user picked a device or there is only one, then add all the tracks */
    if (drive_device) {
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            char path[strlen(drive_device) + sizeof(CDDA_ALL_TRACKS) + 1];
            sprintf(path, "%s/%s", drive_device, CDDA_ALL_TRACKS);
            deadbeef->plt_add_files_begin(plt, 0);
            deadbeef->plt_add_file2(0, plt, path, NULL, NULL);
            deadbeef->plt_add_files_end(plt, 0);
            deadbeef->plt_modified(plt);
            deadbeef->plt_unref(plt);
        }
        deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }

    cdio_free_device_list(device_list);

    return 0;
}

static void
set_disc_id(const char *disc_id, cddb_disc_t *disc)
{
    char category[CDDB_CATEGORY_SIZE];
    unsigned long discid;
    sscanf(disc_id, ",%[^/]/%8lx", category, &discid);
    cddb_disc_set_category_str(disc, category);
    cddb_disc_set_discid(disc, discid);
}

static int
load_cddb_data (ddb_playlist_t *plt, cddb_disc_t *disc, const size_t disc_num)
{
    /* Find the first selected playitem */
    DB_playItem_t *it = deadbeef->plt_get_first(plt, PL_MAIN);
    while (it && !deadbeef->pl_is_selected(it)) {
        deadbeef->pl_item_unref(it);
        it = deadbeef->pl_get_next(it, PL_MAIN);
    }

    /* Find the category/discid for the chosen disc */
    deadbeef->pl_lock();
    const char *disc_list = deadbeef->pl_find_meta(it, CDDB_IDS_TAG);
    char *p = (char *)disc_list;
    size_t i = 0;
    while (p && i++ < disc_num) {
        p = strchr(++p, ',');
    }
    if (p) {
        set_disc_id(p, disc);
    }
    deadbeef->pl_unlock();
    trace("cda: load metadata from discid %s/%u\n", cddb_disc_get_category_str(disc), cddb_disc_get_discid(disc));

    /* Read the cddb data for this disc */
    cddb_conn_t *conn = new_cddb_connection();
    if (!conn) {
        return -1;
    }
    const int read = cddb_read(conn, disc);
    cddb_destroy(conn);
    if (!read) {
        trace("cda: discid read failed\n");
        return -1;
    }

    /* Apply the data to each selected playitem */
    int track_count = cddb_disc_get_track_count(disc);
    char num_tracks[4];
    snprintf(num_tracks, sizeof(num_tracks), "%02d", track_count);
    do {
        if (deadbeef->pl_is_selected(it)) {
            // FIXME: Reloading from cddb can't combine cdtext with cddb data
            write_metadata(NULL, it, disc, num_tracks);
        }
        deadbeef->pl_item_unref(it);
        it = deadbeef->pl_get_next(it, PL_MAIN);
    } while (it);

    deadbeef->plt_modified(plt);
    deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

    return 0;
}

static int
action_disc_n (DB_plugin_action_t *act, ddb_action_context_t ctx)
{
    const int disc_num = atoi(act->name+11);
    int res = -1;
    ddb_playlist_t *plt = deadbeef->plt_get_curr();
    if (plt) {
        cddb_disc_t *disc = cddb_disc_new();
        if (disc) {
            res = load_cddb_data(plt, disc, disc_num);
            cddb_disc_destroy(disc);
        }
        deadbeef->plt_unref(plt);
    }

    return res;
}

static DB_plugin_action_t disc_actions[MAX_CDDB_DISCS];
static char disc_action_titles[MAX_CDDB_DISCS][MAX_CDDB_MENU];

static DB_plugin_action_t add_cd_action =
{
    .name = "cd_add",
    .title = "File/Add Audio CD",
    .flags = DB_ACTION_COMMON | DB_ACTION_ADD_MENU,
    .callback2 = cda_action_add_cd,
    .next = NULL
};

static DB_plugin_action_t *
cda_get_actions (DB_playItem_t *it)
{
    /* Main menu, File->Add Audio CD */
    if (!it) {
        return &add_cd_action;
    }

    /* Quick sanity check that the current playitem is a CD track with a CDDB IDs tag */
    char disc_list[(CDDB_CATEGORY_SIZE + CDDB_DISCID_SIZE) * MAX_CDDB_DISCS + 1] = "";
    deadbeef->pl_get_meta(it, CDDB_IDS_TAG, disc_list, sizeof(disc_list));
    if (!*disc_list) {
        return NULL;
    }

    /* Make sure all selected playitems are from the same CD disc */
    ddb_playlist_t *plt = deadbeef->plt_get_curr();
    if (!plt) {
        return NULL;
    }
    DB_playItem_t *test_it = deadbeef->plt_get_first(plt, PL_MAIN);
    while (test_it) {
        if (deadbeef->pl_is_selected(test_it)) {
            deadbeef->pl_lock();
            const char *it_disc_list = deadbeef->pl_find_meta(test_it, CDDB_IDS_TAG);
            if (!it_disc_list || strcmp(disc_list, it_disc_list)) {
                deadbeef->pl_item_unref(test_it);
                deadbeef->plt_unref(plt);
                deadbeef->pl_unlock();
                return NULL;
            }
            deadbeef->pl_unlock();
        }

        deadbeef->pl_item_unref(test_it);
        test_it = deadbeef->pl_get_next(test_it, PL_MAIN);
    }
    deadbeef->plt_unref(plt);

    /* Make sure all the static menu items are initialised */
    if (!disc_actions[0].name) {
        disc_actions[0].name = "disc_action0";
        disc_actions[1].name = "disc_action1";
        disc_actions[2].name = "disc_action2";
        disc_actions[3].name = "disc_action3";
        disc_actions[4].name = "disc_action4";
        disc_actions[5].name = "disc_action5";
        disc_actions[6].name = "disc_action6";
        disc_actions[7].name = "disc_action7";
        disc_actions[8].name = "disc_action8";
        disc_actions[9].name = "disc_action9";
        for (size_t i = 0; i < MAX_CDDB_DISCS; i++) {
            disc_actions[i].title = disc_action_titles[i];
            disc_actions[i].callback2 = action_disc_n;
        }
    }

    /* Only query against the cache, everything has been read before and we need to be fast */
    cddb_conn_t *conn = new_cddb_connection();
    if (!conn) {
        return NULL;
    }
    cddb_cache_only(conn);
    cddb_disc_t *disc = cddb_disc_new();
    if (!disc) {
        cddb_destroy(conn);
        return NULL;
    }

    /* Build a menu item for each possible cddb disc for this CD */
    size_t i = 0;
    char *p = disc_list;
    do {
        set_disc_id(p, disc);
        if (cddb_read(conn, disc)) {
            const char *title = cddb_disc_get_title(disc);
            const unsigned int year_int = cddb_disc_get_year(disc);
            char year[8] = "";
            if (year_int && year_int <= 9999) {
                sprintf(year, "[%u] ", year_int);
            }
            snprintf(disc_action_titles[i], MAX_CDDB_MENU, "Load CDDB metadata/%s%s", year, title);
            disc_actions[i].flags = DB_ACTION_SINGLE_TRACK | DB_ACTION_MULTIPLE_TRACKS | DB_ACTION_ADD_MENU;
            disc_actions[i].next = &disc_actions[i+1];
            i++;
        }
        p = strchr(p+1, ',');
    } while (p);
    disc_actions[i-1].next = NULL;

    cddb_disc_destroy(disc);
    cddb_destroy(conn);

    return disc_actions;
}

static const char settings_dlg[] =
    "property \"Use CDDB/GnuDb\" checkbox cdda.freedb.enable 1;\n"
    "property box hbox[2] height=-1;"
    "property box hbox[0] border=5 height=-1;"
    "property box vbox[4] expand fill height=-1;"
    "property \"Prefer CD-Text over CDDB\" checkbox cdda.prefer_cdtext 1;\n"
    "property \"CDDB url (e.g. 'gnudb.gnudb.org')\" entry cdda.freedb.host gnudb.gnudb.org;\n"
    "property box hbox[1] height=-1;"
    "property \"CDDB port number (e.g. '8880')\" entry cdda.freedb.port 8880;\n"
    "property \"Use CDDB protocol\" checkbox cdda.protocol 1;\n"
    "property \"Enable NRG image support\" checkbox cdda.enable_nrg 0;"
    "property box hbox[1] height=-1;"
    "property \"Drive speed for normal playback\" select[6] cdda.drive_speed 2 1x 2x 4x 8x 16x Max;"
#if USE_PARANOIA
    "property \"Use cdparanoia error correction when ripping\" checkbox cdda.paranoia 0;"
#endif
;

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "cda",
    .plugin.name = "Audio CD player",
    .plugin.descr = "Audio CD plugin using libcdio and libcddb",
    .plugin.copyright =
        "Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
    .exts = (char const *[]){"cda", "nrg", NULL}
};

DB_plugin_t *
cdda_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

