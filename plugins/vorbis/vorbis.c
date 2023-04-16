/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and other contributors

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
    #include <config.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <deadbeef/deadbeef.h>
#include "../liboggedit/oggedit.h"
#if TREMOR
    #include <tremor/ivorbisfile.h>
    #define FIXED_POINT 1
#else
    #include <vorbis/vorbisfile.h>
#endif
#include <deadbeef/strdupa.h>

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

// #define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

#if WORDS_BIGENDIAN
#define CVORBIS_ENDIANNESS 1
#else
#define CVORBIS_ENDIANNESS 0
#endif

static const char *tag_rg_names[] = {
    "REPLAYGAIN_ALBUM_GAIN",
    "REPLAYGAIN_ALBUM_PEAK",
    "REPLAYGAIN_TRACK_GAIN",
    "REPLAYGAIN_TRACK_PEAK",
    NULL
};

// replaygain key names in deadbeef internal metadata
static const char *ddb_internal_rg_keys[] = {
    ":REPLAYGAIN_ALBUMGAIN",
    ":REPLAYGAIN_ALBUMPEAK",
    ":REPLAYGAIN_TRACKGAIN",
    ":REPLAYGAIN_TRACKPEAK",
    NULL
};

static ddb_decoder2_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    OggVorbis_File vorbis_file;
    int cur_bit_stream;
    float next_update;
    DB_playItem_t *it;
    int set_bitrate;
    const DB_playItem_t *new_track;
    uint8_t *channel_map;
    float prev_playpos;
    time_t started_timestamp;
} ogg_info_t;

static size_t
cvorbis_fread (void *ptr, size_t size, size_t nmemb, void *datasource) {
    size_t ret = deadbeef->fread (ptr, size, nmemb, datasource);
//    trace ("cvorbis_fread %d %d %d\n", size, nmemb, ret);
    return ret;
}

static int
cvorbis_fseek (void *datasource, ogg_int64_t offset, int whence) {
    DB_FILE *f = (DB_FILE *)datasource;
    return deadbeef->fseek (f, offset, whence);
}

static int
cvorbis_fclose (void *datasource) {
    deadbeef->fclose (datasource);
    return 0;
}

static long
cvorbis_ftell (void *datasource) {
    return deadbeef->ftell (datasource);
}

static bool
replaygain_tag(DB_playItem_t *it, const int tag_enum, const char *tag, const char *value)
{
    if (strcasecmp(tag_rg_names[tag_enum], tag))
        return false;

    deadbeef->pl_set_item_replaygain (it, tag_enum, atof(value));
    return true;
}

static int
update_vorbis_comments (DB_playItem_t *it, OggVorbis_File *vorbis_file, const int tracknum) {
    const vorbis_comment *vc = ov_comment(vorbis_file, tracknum);
    if (!vc) {
        trace("update_vorbis_comments: ov_comment failed\n");
        return -1;
    }

    deadbeef->pl_delete_all_meta (it);
    for (int i = 0; i < vc->comments; i++) {
        char *tag = strdup(vc->user_comments[i]);
        char *value;
        if (tag && (value = strchr(tag, '='))
#ifdef ANDROID
            && strlen (value) < 4000
#endif
            ) {
            *value++ = '\0';
            if (!replaygain_tag(it, DDB_REPLAYGAIN_ALBUMGAIN, tag, value) &&
                !replaygain_tag(it, DDB_REPLAYGAIN_ALBUMPEAK, tag, value) &&
                !replaygain_tag(it, DDB_REPLAYGAIN_TRACKGAIN, tag, value) &&
                !replaygain_tag(it, DDB_REPLAYGAIN_TRACKPEAK, tag, value)
                && strcasecmp (tag, "METADATA_BLOCK_PICTURE")
                && *value) {
                    deadbeef->pl_append_meta(it, oggedit_map_tag(tag, "tag2meta"), value);
            }
        }
        if (tag) {
            free(tag);
        }
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    uint32_t f = deadbeef->pl_get_item_flags (it);
    f &= ~DDB_TAG_MASK;
    f |= DDB_TAG_VORBISCOMMENTS;
    deadbeef->pl_set_item_flags (it, f);
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

    return 0;
}

static void
set_meta_ll(DB_playItem_t *it, const char *key, const int64_t value)
{
    char string[11];
    snprintf(string, sizeof(string), "%lld", (long long)value);
    deadbeef->pl_replace_meta(it, key, string);
}

static int
cvorbis_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    ogg_info_t *info = (ogg_info_t *)_info;
    if (sample < 0) {
        trace ("vorbis: negative seek sample - ignored, but it is a bug!\n");
        return -1;
    }
    if (!info->info.file) {
        trace ("vorbis: file is NULL on seek\n");
        return -1;
    }
    if (sample == 0) {
        deadbeef->pl_lock ();
        const char *filetype = deadbeef->pl_find_meta_raw(info->it, ":FILETYPE");
        if (filetype && strncmp(filetype, "Ogg Vorbis", 10)) {
            sample = 1; // workaround libvorbis bug #1486 (ddb issue #1116)
        }
        deadbeef->pl_unlock ();
    }
    int64_t startsample = deadbeef->pl_item_get_startsample (info->it);
    sample += startsample;
    trace ("vorbis: seek to sample %d\n", sample);
    int res = ov_pcm_seek (&info->vorbis_file, sample);
    if (res != 0 && res != OV_ENOSEEK) {
        trace ("vorbis: error %x seeking to sample %d\n", res, sample);
        return -1;
    }
    int64_t tell = ov_pcm_tell (&info->vorbis_file);
    if (tell != sample) {
        trace ("vorbis: failed to do sample-accurate seek (%d->%d)\n", sample, tell);
    }
    trace ("vorbis: seek successful\n")
    _info->readpos = (int64_t)((double)(sample - startsample)/_info->fmt.samplerate);
    info->next_update = -2;
    return 0;
}

static int
cvorbis_seek_sample (DB_fileinfo_t *_info, int sample) {
    return cvorbis_seek_sample64(_info, sample);
}

static ogg_info_t *
cvorbis_open_int (uint32_t hints) {
    ogg_info_t *info = calloc(1, sizeof(ogg_info_t));
    if (info) {
        info->info.plugin = &plugin.decoder;
#if FIXED_POINT
        info->info.fmt.bps = 16;
#else
        info->info.fmt.is_float = 1;
        info->info.fmt.bps = 32;
#endif
        info->set_bitrate = hints & DDB_DECODER_HINT_NEED_BITRATE;
    }
    return info;
}

static DB_fileinfo_t *
cvorbis_open (uint32_t hints) {
    return (DB_fileinfo_t *)cvorbis_open_int(hints);
}

static DB_fileinfo_t *
cvorbis_open2 (uint32_t hints, DB_playItem_t *it) {
    ogg_info_t *info = cvorbis_open_int(hints);
    if (!info) {
        return NULL;
    }

    deadbeef->pl_lock();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock();
    info->info.file = deadbeef->fopen(uri);
    if (!info->info.file) {
        trace("cvorbis_open2 failed to open file %s\n", uri);
    }

    return &info->info;
}

static int
cvorbis_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    trace("cvorbis_init\n");
    ogg_info_t *info = (ogg_info_t *)_info;
    info->new_track = info->it = it;
    deadbeef->pl_item_ref (it);

    if (!info->info.file) {
        deadbeef->pl_lock ();
        const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
        deadbeef->pl_unlock ();
        info->info.file = deadbeef->fopen (uri);
        if (!info->info.file) {
            trace ("cvorbis_init failed to open file %s\n", uri);
            return -1;
        }
    }

    if (info->info.file->vfs->is_streaming () && deadbeef->fgetlength (info->info.file) == -1) {
        ov_callbacks ovcb = {
            .read_func = cvorbis_fread,
            .seek_func = NULL,
            .close_func = cvorbis_fclose,
            .tell_func = NULL
        };

        trace ("calling ov_open_callbacks\n");
        int err = ov_open_callbacks (info->info.file, &info->vorbis_file, NULL, 0, ovcb);
        if (err != 0) {
            trace ("ov_open_callbacks returned %d\n", err);
            return -1;
        }
        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        deadbeef->plt_set_item_duration (plt, it, -1);
        if (plt) {
            deadbeef->plt_unref (plt);
        }
    }
    else
    {
        ov_callbacks ovcb = {
            .read_func = cvorbis_fread,
            .seek_func = cvorbis_fseek,
            .close_func = cvorbis_fclose,
            .tell_func = cvorbis_ftell
        };

        trace ("calling ov_open_callbacks\n");
        int err = ov_open_callbacks (info->info.file, &info->vorbis_file, NULL, 0, ovcb);
        if (err != 0) {
            trace ("ov_open_callbacks returned %d\n", err);
            return -1;
        }
    }
//    _info->readpos = 0;
    if (info->info.file->vfs->is_streaming ()) {
        deadbeef->pl_item_set_startsample (info->it, 0);
        if (deadbeef->pl_get_item_duration (it) < 0) {
            deadbeef->pl_item_set_endsample (info->it, -1);
        }
        else {
            deadbeef->pl_item_set_endsample (it, ov_pcm_total (&info->vorbis_file, -1)-1);
        }
        if (update_vorbis_comments (it, &info->vorbis_file, -1))
            return -1;
        deadbeef->pl_set_meta_int(info->it, ":TRACKNUM", 0);
    }
    else {
        cvorbis_seek_sample (_info, 0);
    }

    vorbis_info *vi = ov_info (&info->vorbis_file, -1);
    if (!vi) {
        trace ("not a vorbis stream\n");
        return -1;
    }
    if (vi->rate <= 0) {
        trace ("vorbis: bad samplerate\n");
        return -1;
    }
    _info->fmt.samplerate = (int)vi->rate;
    _info->fmt.channels = vi->channels;
    info->channel_map = oggedit_vorbis_channel_map(vi->channels);
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }

    deadbeef->pl_set_meta_int (it, ":TRACKNUM", 0);
    deadbeef->pl_set_meta_int (it, ":CHANNELS", vi->channels);
    deadbeef->pl_set_meta_int (it, ":SAMPLERATE", (int)vi->rate);
    deadbeef->pl_replace_meta (it, "!FILETYPE", "Ogg Vorbis");
    info->cur_bit_stream = -1;

    info->started_timestamp = time(NULL);

    return 0;
}

static void
cvorbis_free (DB_fileinfo_t *_info) {
    trace("cvorbis_free\n");
    ogg_info_t *info = (ogg_info_t *)_info;
    if (info) {
        if (info->it) {
            deadbeef->pl_item_unref (info->it);
        }
        free(info->channel_map);
        info->channel_map = NULL;
        if (info->info.file) {
            if (info->vorbis_file.datasource) {
                ov_clear (&info->vorbis_file);
            }
            else {
                deadbeef->fclose (info->info.file);
            }
        }
        free (info);
    }
}

static void send_event(DB_playItem_t *it, const int event_enum)
{
    ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(event_enum);
    if ((event->track = it))
        deadbeef->pl_item_ref(event->track);
    deadbeef->event_send((ddb_event_t *)event, 0, 0);
}

static bool
new_streaming_link(ogg_info_t *info, const int new_link)
{
    // Typically the files with multiple links are split into multiple tracks,
    // and we don't want the file's properties to change while it's playing.
    // However, we want this for network streams.
    if (!info->info.file->vfs->is_streaming () || new_link < 0) {
        return false;
    }

    DB_playItem_t *from = deadbeef->pl_item_alloc ();
    deadbeef->pl_items_copy_junk (info->it, from, from);

    trace ("Streaming link changed from %d to %d\n", info->cur_bit_stream, new_link);
    update_vorbis_comments (info->it, &info->vorbis_file, new_link);

    ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)deadbeef->event_alloc (DB_EV_SONGCHANGED);
    float playpos = deadbeef->streamer_get_playpos ();
    ev->from = from;
    ev->to = info->it;
    ev->playtime = playpos - info->prev_playpos;
    ev->started_timestamp = info->started_timestamp;
    deadbeef->pl_item_ref (ev->from);
    deadbeef->pl_item_ref (ev->to);
    deadbeef->event_send ((ddb_event_t *)ev, 0, 0);
    deadbeef->pl_item_unref (from);
    from = NULL;

    info->started_timestamp = time(NULL);
    info->prev_playpos = playpos;

    send_event (info->it, DB_EV_SONGSTARTED);
    send_event (info->it, DB_EV_TRACKINFOCHANGED);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    info->cur_bit_stream = new_link;

    vorbis_info *vi = ov_info (&info->vorbis_file, new_link);
    if (vi && (info->info.fmt.channels != vi->channels || info->info.fmt.samplerate != vi->rate)) {
        info->info.fmt.samplerate = (int)vi->rate;
        info->info.fmt.channels = vi->channels;
        deadbeef->pl_set_meta_int (info->it, ":CHANNELS", vi->channels);
        deadbeef->pl_set_meta_int (info->it, ":SAMPLERATE", (int)vi->rate);
        return true;
    }

    return false;
}

#if FIXED_POINT
static void
map_channels(int16_t *dest, const int16_t *src, const int16_t *src_end, const uint8_t *map, const int channel_count)
{
    while (src < src_end) {
        for (uint8_t *map_ptr = (uint8_t *)map; map_ptr < map + channel_count; map_ptr++, src++)
            dest[*map_ptr] = *src;
        dest += channel_count;
    }
}
#endif

static bool
is_playing_track(const DB_playItem_t *it)
{
    DB_playItem_t *track = deadbeef->streamer_get_playing_track_safe();
    if (track)
        deadbeef->pl_item_unref(track);
    return track == it;
}

static int
cvorbis_read (DB_fileinfo_t *_info, char *buffer, int bytes_to_read) {
    ogg_info_t *info = (ogg_info_t *)_info;

    /* Work round some streamer limitations and infobar issue #22 */
    if (info->new_track && is_playing_track(info->new_track)) {
        info->new_track = NULL;
        send_event(info->it, DB_EV_TRACKINFOCHANGED);
        info->next_update = -2;
    }

    /* Don't read past the end of a sub-track */
    int samples_to_read = bytes_to_read / sizeof(float) / _info->fmt.channels;
    int64_t endsample = deadbeef->pl_item_get_endsample (info->it);
    if (endsample > 0) {
        const ogg_int64_t samples_left = endsample - ov_pcm_tell(&info->vorbis_file);
        if (samples_left < samples_to_read) {
            samples_to_read = (int)samples_left;
            bytes_to_read = samples_to_read * sizeof(float) * _info->fmt.channels;
        }
    }

    /* Read until we have enough bytes to satisfy streamer, or there are none left */
    int bytes_read = 0;
    int ret = OV_HOLE;

#if FIXED_POINT
    char map_buffer[info->channel_map ? bytes_to_read : 0];
    char *ptr = info->channel_map ? map_buffer : buffer;
    while ((ret > 0 || ret == OV_HOLE) && bytes_read < bytes_to_read)
    {
        int new_link = -1;
#if TREMOR
        ret=ov_read(&info->vorbis_file, ptr+bytes_read, bytes_to_read-bytes_read, &new_link);
#else
        ret=ov_read(&info->vorbis_file, ptr+bytes_read, bytes_to_read-bytes_read, CVORBIS_ENDIANNESS, 2, 1, &new_link);
#endif
        if (ret < 0) {
            trace("cvorbis_read: ov_read returned %d\n", ret);
        }
        else if (new_link != info->cur_bit_stream && new_streaming_link(info, new_link)) {
            bytes_read = bytes_to_read;
            break;
        }
        else {
            bytes_read += ret;
        }

//        trace("cvorbis_read got %d bytes towards %d bytes (%d bytes still required)\n", ret, bytes_to_read, bytes_to_read-bytes_read);
    }

    if (info->channel_map)
        map_channels((int16_t *)buffer, (int16_t *)map_buffer, (int16_t *)ptr, info->channel_map, _info->fmt.channels);
#else
    int samples_read = 0;
    while (samples_read < samples_to_read && (ret > 0 || ret == OV_HOLE))
    {
        float **pcm = NULL;
        int new_link = -1;
        ret = (int)ov_read_float(&info->vorbis_file, &pcm, samples_to_read-samples_read, &new_link);

        if (ret < 0) {
            trace("cvorbis_read: ov_read returned %d\n", ret);
        }
        else if (new_link != info->cur_bit_stream && new_streaming_link(info, new_link)) {
            samples_read = samples_to_read;
            break;
        }
        else if (ret > 0) {
            float *ptr = (float *)buffer + samples_read*_info->fmt.channels;
            for (int channel = 0; channel < _info->fmt.channels; channel++, ptr++) {
                const float *pcm_channel = pcm[info->channel_map ? info->channel_map[channel] : channel];
                for (int sample = 0; sample < ret; sample++) {
                    ptr[sample*_info->fmt.channels] = pcm_channel[sample];
                }
            }
            samples_read += ret;
        }

//        trace("cvorbis_read got %d samples towards %d bytes (%d samples still required)\n", ret, bytes_to_read, samples_to_read-samples_read);
    }
    bytes_read = samples_read * sizeof(float) * _info->fmt.channels;
#endif

    int64_t startsample = deadbeef->pl_item_get_startsample (info->it);
    _info->readpos = (float)(ov_pcm_tell(&info->vorbis_file) - startsample) / _info->fmt.samplerate;
    if (info->set_bitrate && _info->readpos > info->next_update) {
        const int rate = (int)ov_bitrate_instant(&info->vorbis_file) / 1000;
        if (rate > 0) {
            deadbeef->streamer_set_bitrate(rate);
            info->next_update = info->next_update <= 0 ? info->next_update + 1 : _info->readpos + 5;
        }
    }

//    trace("cvorbis_read returning %d bytes out of %d\n", bytes_read, bytes_to_read);
    return bytes_read;
}

static int
cvorbis_seek (DB_fileinfo_t *_info, float time) {
    ogg_info_t *info = (ogg_info_t *)_info;
    return cvorbis_seek_sample64 (_info, (int64_t)((double)time * (int64_t)info->info.fmt.samplerate));
}

static off_t sample_offset(OggVorbis_File *vorbis_file, const ogg_int64_t sample)
{
    if (sample <= 0 || sample == ov_pcm_total(vorbis_file, -1))
        return 0;

    if (ov_pcm_seek(vorbis_file, sample)) {
        trace("ov_pcm_seek failed to find sample %lld: %d\n", sample, ov_pcm_seek(vorbis_file, sample));
        return -1;
    }

    return ov_raw_tell(vorbis_file);
}

static DB_playItem_t *
cvorbis_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("vorbis: failed to fopen %s\n", fname);
        return NULL;
    }
    int64_t fsize = deadbeef->fgetlength (fp);
    if (fp->vfs->is_streaming ()) {
        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
        deadbeef->plt_set_item_duration (plt, it, -1);
        deadbeef->pl_add_meta (it, "title", NULL);
        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
        deadbeef->fclose (fp);
        return after;
    }
    ov_callbacks ovcb = {
        .read_func = cvorbis_fread,
        .seek_func = cvorbis_fseek,
        .close_func = cvorbis_fclose,
        .tell_func = cvorbis_ftell
    };
    OggVorbis_File vorbis_file;
    int err = ov_open_callbacks (fp, &vorbis_file, NULL, 0, ovcb);
    if (err != 0) {
        trace ("ov_open_callbacks returned %d\n", err);
        ov_clear (&vorbis_file);
        deadbeef->fclose (fp);
        return NULL;
    }

    long nstreams = ov_streams (&vorbis_file);
    int64_t currentsample = 0;
    for (int stream = 0; stream < nstreams; stream++) {
        const vorbis_info *vi = ov_info (&vorbis_file, stream);
        if (!vi) {
            trace ("vorbis: ov_info failed for file %s stream %d\n", fname, stream);
            continue;
        }
#if TREMOR
        const float duration = ov_time_total(&vorbis_file, stream) / 1000.0;
#else
        const float duration = ov_time_total(&vorbis_file, stream);
#endif
        int64_t totalsamples = ov_pcm_total (&vorbis_file, stream);

        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
        deadbeef->pl_set_meta_int (it, ":TRACKNUM", stream);
        deadbeef->plt_set_item_duration (plt, it, duration);
        if (nstreams > 1) {
            deadbeef->pl_item_set_startsample (it, currentsample);
            deadbeef->pl_item_set_endsample (it, currentsample + totalsamples - 1);
            deadbeef->pl_set_item_flags (it, DDB_IS_SUBTRACK);
        }

        if (update_vorbis_comments (it, &vorbis_file, stream))
            continue;
        int samplerate = (int)vi->rate;

        int64_t startsample = deadbeef->pl_item_get_startsample (it);
        int64_t endsample = deadbeef->pl_item_get_endsample (it);

        const off_t start_offset = sample_offset(&vorbis_file, startsample-1);
        const off_t end_offset = sample_offset(&vorbis_file, endsample);
        char *filetype = NULL;
        const off_t stream_size = oggedit_vorbis_stream_info(deadbeef->fopen(fname), start_offset, end_offset, &filetype);
        if (filetype) {
            deadbeef->pl_replace_meta(it, ":FILETYPE", filetype);
            free(filetype);
        }
        if (stream_size > 0) {
            set_meta_ll(it, ":STREAM SIZE", stream_size);
            deadbeef->pl_set_meta_int(it, ":BITRATE", 8.f * samplerate * stream_size / totalsamples / 1000);
        }
        set_meta_ll (it, ":FILE_SIZE", fsize);
//        deadbeef->pl_add_meta (it, ":BPS", "32");
        deadbeef->pl_set_meta_int (it, ":CHANNELS", vi->channels);
        deadbeef->pl_set_meta_int (it, ":SAMPLERATE", samplerate);
//        deadbeef->pl_set_meta_int (it, ":BITRATE", ov_bitrate (&vorbis_file, stream)/1000);

        if (nstreams == 1) {
            DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it,  totalsamples, samplerate);
            if (cue) {
                deadbeef->pl_item_unref (it);
                ov_clear (&vorbis_file);
                return cue;
            }
        }
        else {
            currentsample += totalsamples;
        }

        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
    }
    ov_clear (&vorbis_file);
    return after;
}

static int
vorbis_start (void) {
    return 0;
}

static int
vorbis_stop (void) {
    return 0;
}
int
cvorbis_read_metadata (DB_playItem_t *it) {
    DB_FILE *fp = NULL;
    OggVorbis_File vorbis_file;
    vorbis_info *vi = NULL;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    fp = deadbeef->fopen (uri);
    if (!fp) {
        trace ("cvorbis_read_metadata: failed to fopen %s\n", uri);
        return -1;
    }
    if (fp->vfs->is_streaming ()) {
        trace ("cvorbis_read_metadata: failed to fopen %s\n", uri);
        deadbeef->fclose(fp);
        return -1;
    }
    ov_callbacks ovcb = {
        .read_func = cvorbis_fread,
        .seek_func = cvorbis_fseek,
        .close_func = cvorbis_fclose,
        .tell_func = cvorbis_ftell
    };
    int res = ov_open_callbacks (fp, &vorbis_file, NULL, 0, ovcb);
    if (res != 0) {
        trace ("cvorbis_read_metadata: ov_open_callbacks returned %d\n", res);
        deadbeef->fclose(fp);
        return -1;
    }
    int tracknum = deadbeef->pl_find_meta_int (it, ":TRACKNUM", -1);
    vi = ov_info (&vorbis_file, tracknum);
    if (!vi) {
        trace ("cvorbis_read_metadata: failed to ov_open %s\n", uri);
        ov_clear (&vorbis_file);
        return -1;
    }

    if (update_vorbis_comments (it, &vorbis_file, tracknum)) {
        ov_clear (&vorbis_file);
        return -1;
    }

    ov_clear (&vorbis_file);
    return 0;
}

static void
split_tag(vorbis_comment *tags, const char *key, const char *value, int valuesize)
{
    while (valuesize > 0) {
        vorbis_comment_add_tag(tags, key, value);
        size_t l = strlen (value) + 1;
        value += l;
        valuesize -= l;
    }
}

static vorbis_comment *
tags_list(DB_playItem_t *it, OggVorbis_File *vorbis_file)
{
    vorbis_comment *tags = calloc (1, sizeof (vorbis_comment));
    if (!tags)
        return NULL;

    deadbeef->pl_lock ();
    for (DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (it); m; m = m->next) {
        if (strchr (":!_", m->key[0])) {
            break;
        }
        char *key = strdupa (m->key);
        split_tag (tags, oggedit_map_tag (key, "meta2tag"), m->value, m->valuesize);
    }
    deadbeef->pl_unlock ();

    // add replaygain values
    for (int n = 0; ddb_internal_rg_keys[n]; n++) {
        if (deadbeef->pl_find_meta (it, ddb_internal_rg_keys[n])) {
            float value = deadbeef->pl_get_item_replaygain (it, n);
            char s[100];
            // https://wiki.hydrogenaud.io/index.php?title=ReplayGain_2.0_specification#Metadata_format
            switch (n) {
            case DDB_REPLAYGAIN_ALBUMGAIN:
            case DDB_REPLAYGAIN_TRACKGAIN:
                snprintf (s, sizeof (s), "%.2f dB", value);
                break;
            case DDB_REPLAYGAIN_ALBUMPEAK:
            case DDB_REPLAYGAIN_TRACKPEAK:
                snprintf (s, sizeof (s), "%.6f", value);
                break;
            }
            split_tag (tags, tag_rg_names[n], s, (int)strlen (s)+1);
        }
    }

    return tags;
}

static int
cvorbis_write_metadata (DB_playItem_t *it) {
    char fname[PATH_MAX];
    deadbeef->pl_get_meta (it, ":URI", fname, sizeof (fname));

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("cvorbis_write_metadata: failed to fopen %s\n", fname);
        return -1;
    }
    ov_callbacks ovcb = {
        .read_func = cvorbis_fread,
        .seek_func = cvorbis_fseek,
        .close_func = cvorbis_fclose,
        .tell_func = cvorbis_ftell
    };
    OggVorbis_File vorbis_file;
    int err = ov_test_callbacks (fp, &vorbis_file, NULL, 0, ovcb);
    if (err != 0) {
        trace ("ov_test_callbacks returned %d\n", err);
        deadbeef->fclose (fp);
        return -1;
    }

    vorbis_comment *tags = tags_list(it, &vorbis_file);
    ov_clear(&vorbis_file);
    if (!tags) {
        trace("cvorbis_write_metadata: tags list allocation failed\n");
        return -1;
    }

    deadbeef->pl_lock();
    const char *stream_size_string = deadbeef->pl_find_meta(it, ":STREAM SIZE");
    const size_t stream_size = stream_size_string ? (off_t)atoll(stream_size_string) : 0;
    deadbeef->pl_unlock();
    const off_t file_size = oggedit_write_vorbis_metadata (deadbeef->fopen(fname), fname, 0, stream_size, tags->comments, tags->user_comments);
    vorbis_comment_clear(tags);
    free(tags);

    int res = 0;

    if (file_size <= 0) {
        trace ("cvorbis_write_metadata: failed to write tags to %s, code %d\n", fname, file_size);
        res = -1;
    }

    ov_clear (&vorbis_file);

    if (!res) {
        set_meta_ll(it, ":FILE_SIZE", (int64_t)file_size);
        res = cvorbis_read_metadata(it);
    }

    return res;
}


static const char * exts[] = { "ogg", "ogx", "oga", NULL };

// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "stdogg",
    .decoder.plugin.name = "Ogg Vorbis decoder",
    .decoder.plugin.descr = "Ogg Vorbis decoder using standard xiph.org libraries",
    .decoder.plugin.copyright =
    "Ogg Vorbis plugin for DeaDBeeF\n"
    "Copyright (C) 2009-2014 Oleksiy Yakovenko et al.\n"
    "\n"
    "vcedit.c\n"
    "Ogg Vorbis plugin Ogg edit functions\n"
    "\n"
    "Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>\n"
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
    "\n"
    "\n"
    "\n"
    "Uses libogg,libvorbis Copyright (c) 2002, Xiph.org Foundation\n"
    "\n"
    "Redistribution and use in source and binary forms, with or without\n"
    "modification, are permitted provided that the following conditions\n"
    "are met:\n"
    "\n"
    "- Redistributions of source code must retain the above copyright\n"
    "notice, this list of conditions and the following disclaimer.\n"
    "\n"
    "- Redistributions in binary form must reproduce the above copyright\n"
    "notice, this list of conditions and the following disclaimer in the\n"
    "documentation and/or other materials provided with the distribution.\n"
    "\n"
    "- Neither the name of the DeaDBeeF Player nor the names of its\n"
    "contributors may be used to endorse or promote products derived from\n"
    "this software without specific prior written permission.\n"
    "\n"
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
    "``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
    "LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
    "A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR\n"
    "CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n"
    "EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n"
    "PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n"
    "PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n"
    "LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n"
    "NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n"
    "SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
    ,
    .decoder.plugin.website = "http://deadbeef.sf.net",
    .decoder.plugin.start = vorbis_start,
    .decoder.plugin.stop = vorbis_stop,
    .decoder.open = cvorbis_open,
    .decoder.open2 = cvorbis_open2,
    .decoder.init = cvorbis_init,
    .decoder.free = cvorbis_free,
    .decoder.read = cvorbis_read,
    .decoder.seek = cvorbis_seek,
    .decoder.seek_sample = cvorbis_seek_sample,
    .decoder.insert = cvorbis_insert,
    .decoder.read_metadata = cvorbis_read_metadata,
    .decoder.write_metadata = cvorbis_write_metadata,
    .decoder.exts = exts,
    .seek_sample64 = cvorbis_seek_sample64,
};

DB_plugin_t *
vorbis_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
