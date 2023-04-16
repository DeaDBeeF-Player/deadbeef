/*
    deadbeef-opus
    Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors

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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <opusfile.h>
#include <deadbeef/deadbeef.h>
#include <stdbool.h>
#include "../liboggedit/oggedit.h"
#include <deadbeef/strdupa.h>

#define trace(...) { deadbeef->log_detailed (&plugin.decoder.plugin, 0, __VA_ARGS__); }

static ddb_decoder2_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    int64_t currentsample;

    OggOpusFile *opusfile;
    uint8_t *channelmap;

    int cur_bit_stream;

    int set_bitrate;
    float next_update;
    DB_playItem_t *it;
    DB_playItem_t *new_track;

    float prev_playpos;
    time_t started_timestamp;
} opusdec_info_t;

static const char * exts[] = { "ogg", "opus", "ogv", NULL };

// opus file wrapper
static int
opus_file_read(void *source, unsigned char *ptr, const int bytes) {
    size_t res = ((DB_FILE *)source)->vfs->read(ptr, 1, bytes, source);
    return (int)res;
}

static int
opus_file_seek(void *source, const opus_int64 offset, const int whence) {
    int is_streaming = ((DB_FILE *)source)->vfs->is_streaming();
    if (is_streaming) {
        return -1;
    }

    return ((DB_FILE *)source)->vfs->seek(source, offset, whence);
}

static opus_int64
opus_file_tell (void *source) {
    return (opus_int64)((DB_FILE *)source)->vfs->tell(source);
}

static int
opus_file_close (void *source) {
    return 0;
}

static OpusFileCallbacks opcb = {
    .read = opus_file_read,
    .seek = opus_file_seek,
    .tell = opus_file_tell,
    .close = opus_file_close
};

static OggOpusFile *
opus_file_open(DB_FILE *fp)
{
    int res = 0;
    return op_open_callbacks(fp, &opcb, NULL, 0, &res);
}

static DB_fileinfo_t *
opusdec_open (uint32_t hints) {
    opusdec_info_t *info = calloc (1, sizeof (opusdec_info_t));
    return &info->info;
}

static DB_fileinfo_t *
opusdec_open2 (uint32_t hints, DB_playItem_t *it) {
    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    DB_FILE *fp = deadbeef->fopen (uri);

    if (!fp) {
        return NULL;
    }

    opusdec_info_t *info = calloc (1, sizeof (opusdec_info_t));

    info->info.file = fp;
    info->it = it;
    deadbeef->pl_item_ref (it);

    return &info->info;
}

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



static bool
_is_replaygain_tag(DB_playItem_t *it, const char *tag)
{
    for (int i = 0; i <= DDB_REPLAYGAIN_TRACKPEAK; i++) {
        if (!strcasecmp(tag_rg_names[i], tag)) {
            return 1;
        }
    }
    return 0;
}

static int
update_vorbis_comments (DB_playItem_t *it, OggOpusFile *opusfile, const int tracknum) {
    const OpusTags *vc = op_tags (opusfile, tracknum);
    if (!vc) {
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
            // skip the ignored RG fields, and the picture
            if (_is_replaygain_tag (it, tag) || !strcasecmp (tag, "METADATA_BLOCK_PICTURE")) {
                free (tag);
                continue;
            }
            *value++ = '\0';
            deadbeef->pl_append_meta(it, oggedit_map_tag(tag, "tag2meta"), value);
        }
        if (tag) {
            free(tag);
        }
    }

    const char *r128_trackgain = deadbeef->pl_find_meta (it, "R128_TRACK_GAIN");
    if (r128_trackgain) {
        int trackgain = atoi (r128_trackgain) + op_head (opusfile, tracknum)->output_gain;
        if (trackgain != 0) {
            deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_TRACKGAIN, trackgain / 256.0f + 5.0f);
            deadbeef->pl_delete_meta (it, "R128_TRACK_GAIN");
        }
    }

    int albumgain = op_head (opusfile, tracknum)->output_gain;
    const char *r128_albumgain = deadbeef->pl_find_meta (it, "R128_ALBUM_GAIN");
    if (r128_albumgain) {
        albumgain += atoi (r128_albumgain);
        deadbeef->pl_delete_meta (it, "R128_ALBUM_GAIN");
    }
    if (albumgain != 0) {
        deadbeef->pl_set_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN, albumgain / 256.0f + 5.0f);
    }

    char s[100];
    int output_gain = op_head (opusfile, tracknum)->output_gain;
    snprintf (s, sizeof (s), "%0.2f dB", output_gain / 256.0f + 5.0f);
    deadbeef->pl_replace_meta (it, ":OPUS_HEADER_GAIN", s);

    deadbeef->pl_set_meta_int (it, ":SAMPLERATE_ORIGINAL", op_head (opusfile, tracknum)->input_sample_rate);

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

    return 0;
}

static void
set_meta_ll(DB_playItem_t *it, const char *key, const int64_t value)
{
    char string[11];
    sprintf(string, "%lld", (long long)value);
    deadbeef->pl_replace_meta(it, key, string);
}

static int
opusdec_seek_sample64 (DB_fileinfo_t *_info, int64_t sample) {
    opusdec_info_t *info = (opusdec_info_t *)_info;
    if (sample < 0) {
        return -1;
    }
    if (!info->info.file) {
        return -1;
    }
    int64_t startsample = deadbeef->pl_item_get_startsample (info->it);

    int res = op_pcm_seek (info->opusfile, sample + startsample);
    if (res != 0 && res != OP_ENOSEEK) {
        return -1;
    }
    info->currentsample = sample;
    _info->readpos = (float)sample/_info->fmt.samplerate;
    info->next_update = -2;
    return 0;
}

static int
opusdec_seek_sample (DB_fileinfo_t *_info, int sample) {
    return opusdec_seek_sample64(_info, sample);
}

static int
opusdec_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    opusdec_info_t *info = (opusdec_info_t *)_info;

    if (!info->info.file) {
        deadbeef->pl_lock ();
	const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
        deadbeef->pl_unlock ();
        DB_FILE *fp = deadbeef->fopen (uri);

        if (!fp) {
            return -1;
        }
        info->info.file = fp;
        info->it = it;
        deadbeef->pl_item_ref (it);
    }

    info->opusfile = opus_file_open (info->info.file);
    if (!info->opusfile) {
        return -1;
    }

    const OpusHead *head = op_head (info->opusfile, 0);

    if (head->channel_count > 8) {
        trace ("opus: the track has %d channels, but 8 is max supported.\n");
        return -1;
    }

    // take this parameters from your input file
    // we set constants for clarity sake
    _info->fmt.bps = 32;
    _info->fmt.is_float = 1;
    _info->fmt.channels = head->channel_count;
    _info->fmt.samplerate  = 48000;
    if (head->mapping_family == 1) {
        info->channelmap = oggedit_vorbis_channel_map (head->channel_count);
    }

    for (int i = 0; i < (_info->fmt.channels&0x1f); i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    _info->plugin = &plugin.decoder;

    // set all gain adjustment to 0, because deadbeef is performing that.
    op_set_gain_offset (info->opusfile, OP_ABSOLUTE_GAIN, 0);

    if (info->info.file->vfs->is_streaming ()) {
        deadbeef->pl_item_set_startsample (it, 0);
        if (deadbeef->pl_get_item_duration (it) < 0) {
            deadbeef->pl_item_set_endsample (it, -1);
        }
        else {
            deadbeef->pl_item_set_endsample (it, op_pcm_total (info->opusfile, -1) - 1);
        }

        if (update_vorbis_comments (it, info->opusfile, -1))
            return -1;

        deadbeef->pl_set_meta_int(it, ":TRACKNUM", 0);
    }
    else {
        opusdec_seek_sample64 (_info, 0);
    }

    deadbeef->pl_replace_meta (it, "!FILETYPE", "Ogg Opus");
    deadbeef->pl_set_meta_int (it, ":CHANNELS", head->channel_count);

    info->cur_bit_stream = -1;

    info->started_timestamp = time(NULL);

    return 0;
}

// free everything allocated in _init
static void
opusdec_free (DB_fileinfo_t *_info) {
    opusdec_info_t *info = (opusdec_info_t *)_info;

    if (info->opusfile) {
        op_free(info->opusfile);
        info->opusfile = NULL;
    }
    if (info->info.file) {
        deadbeef->fclose (info->info.file);
        info->info.file = NULL;
    }
    if (info->it) {
        deadbeef->pl_item_unref (info->it);
        info->it = NULL;
    }

    if (info) {
        free (info);
    }
}

static void send_event(DB_playItem_t *it, const int event_enum)
{
    ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(event_enum);
    event->track = it;
    if (event->track) {
        deadbeef->pl_item_ref(event->track);
    }
    deadbeef->event_send((ddb_event_t *)event, 0, 0);
}

static bool
new_streaming_link(opusdec_info_t *info, const int new_link)
{
    if (!info->info.file->vfs->is_streaming () || new_link < 0) {
        return false;
    }

    DB_playItem_t *from = deadbeef->pl_item_alloc ();
    deadbeef->pl_items_copy_junk (info->it, from, from);

    update_vorbis_comments(info->it, info->opusfile, new_link);

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

    send_event(info->it, DB_EV_SONGSTARTED);
    send_event(info->it, DB_EV_TRACKINFOCHANGED);
    deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    info->cur_bit_stream = new_link;

    const OpusHead *head = op_head (info->opusfile, new_link);
    if (head && info->info.fmt.channels != head->channel_count) {
        info->info.fmt.channels = head->channel_count;
        deadbeef->pl_set_meta_int (info->it, ":CHANNELS", head->channel_count);
        return true;
    }

    return false;
}

static bool
is_playing_track(const DB_playItem_t *it)
{
    DB_playItem_t *track = deadbeef->streamer_get_playing_track_safe();
    if (track)
        deadbeef->pl_item_unref(track);
    return track == it;
}

static int
opusdec_read (DB_fileinfo_t *_info, char *bytes, int size) {
    opusdec_info_t *info = (opusdec_info_t *)_info;

    // Work round some streamer limitations and infobar issue #22
    if (info->new_track && is_playing_track(info->new_track)) {
        info->new_track = NULL;
        send_event(info->it, DB_EV_TRACKINFOCHANGED);
        info->next_update = -2;
    }

    // Don't read past the end of a sub-track
    int samples_to_read = size / sizeof(float) / _info->fmt.channels;
    int64_t endsample = deadbeef->pl_item_get_endsample (info->it);
    if (endsample > 0) {
        opus_int64 samples_left = endsample - op_pcm_tell (info->opusfile);
        if (samples_left < samples_to_read) {
            samples_to_read = (int)samples_left;
            size = samples_to_read * sizeof(float) * _info->fmt.channels;
        }
    }

    // Read until we have enough bytes to satisfy streamer, or there are none left
    int bytes_read = 0;
    int ret = OP_HOLE;

    int samples_read = 0;
    while (samples_read < samples_to_read && (ret > 0 || ret == OP_HOLE))
    {
        int nframes = samples_to_read-samples_read;
        float pcm[nframes * _info->fmt.channels];
        int new_link = -1;
        ret = op_read_float(info->opusfile, pcm, nframes * _info->fmt.channels, &new_link);

        if (ret < 0) {
        }
        else if (new_link != info->cur_bit_stream && !op_seekable (info->opusfile) && new_streaming_link(info, new_link)) {
            samples_read = samples_to_read;
            break;
        }
        else if (ret > 0) {
            for (int channel = 0; channel < _info->fmt.channels; channel++) {
                const float *pcm_channel = &pcm[info->channelmap ? info->channelmap[channel] : channel];
                float *ptr = ((float *)bytes + samples_read*_info->fmt.channels) + channel;
                for (int sample = 0; sample < ret; sample ++, pcm_channel += _info->fmt.channels) {
                    *ptr = *pcm_channel;
                    ptr += _info->fmt.channels;
                }
            }
            samples_read += ret;
        }
    }
    bytes_read = samples_read * sizeof(float) * _info->fmt.channels;
    info->currentsample += bytes_read / (sizeof (float) * _info->fmt.channels);


    int64_t startsample = deadbeef->pl_item_get_startsample (info->it);
    _info->readpos = (float)(op_pcm_tell(info->opusfile) - startsample) / _info->fmt.samplerate;
    if (info->set_bitrate && _info->readpos > info->next_update) {
        const int rate = (int)op_bitrate_instant(info->opusfile) / 1000;
        if (rate > 0) {
            deadbeef->streamer_set_bitrate(rate);
            info->next_update = info->next_update <= 0 ? info->next_update + 1 : _info->readpos + 5;
        }
    }

    return bytes_read;
}

static int
opusdec_seek (DB_fileinfo_t *_info, float time) {
    return opusdec_seek_sample64 (_info, (int64_t)((double)time * (int64_t)_info->fmt.samplerate));
}

static off_t
sample_offset(OggOpusFile *opusfile, const opus_int64 sample)
{
    if (sample <= 0 || sample == op_pcm_total(opusfile, -1))
        return 0;

    if (op_pcm_seek(opusfile, sample)) {
        return -1;
    }

    return op_raw_tell(opusfile);
}

static DB_playItem_t *
opusdec_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
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

    OggOpusFile *opusfile = opus_file_open (fp);
    if (!opusfile) {
        deadbeef->fclose (fp);
        return NULL;
    }

    long nstreams = op_link_count (opusfile);
    int64_t currentsample = 0;
    for (int stream = 0; stream < nstreams; stream++) {
        const OpusHead *head = op_head (opusfile, stream);
        if (!head) {
            continue;
        }
        int64_t totalsamples = op_pcm_total (opusfile, stream);
        const float duration = totalsamples / 48000.f;

        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.decoder.plugin.id);
        deadbeef->pl_set_meta_int (it, ":TRACKNUM", stream);
        deadbeef->plt_set_item_duration (plt, it, duration);
        if (nstreams > 1) {
            deadbeef->pl_item_set_startsample (it, currentsample);
            deadbeef->pl_item_set_endsample (it, currentsample + totalsamples - 1);
            deadbeef->pl_set_item_flags (it, DDB_IS_SUBTRACK);
        }

        if (update_vorbis_comments (it, opusfile, stream))
            continue;
        int samplerate = 48000;

        int64_t startsample = deadbeef->pl_item_get_startsample (it);
        int64_t endsample = deadbeef->pl_item_get_endsample (it);

        const off_t start_offset = sample_offset(opusfile, startsample-1);
        const off_t end_offset = sample_offset(opusfile, endsample);
        char *filetype = NULL;
        const off_t stream_size = oggedit_opus_stream_info(deadbeef->fopen(fname), start_offset, end_offset, &filetype);
        if (filetype) {
            deadbeef->pl_replace_meta(it, ":FILETYPE", filetype);
            free(filetype);
        }
        if (stream_size > 0) {
            set_meta_ll(it, ":OPUS_STREAM_SIZE", stream_size);
            deadbeef->pl_set_meta_int(it, ":BITRATE", 8.f * samplerate * stream_size / totalsamples / 1000);
        }
        set_meta_ll (it, ":FILE_SIZE", fsize);
        deadbeef->pl_set_meta_int (it, ":CHANNELS", head->channel_count);
        deadbeef->pl_set_meta_int (it, ":SAMPLERATE", samplerate);

        if (nstreams == 1) {
            DB_playItem_t *cue = deadbeef->plt_process_cue (plt, after, it,  totalsamples, samplerate);
            if (cue) {
                deadbeef->pl_item_unref (it);
                op_free(opusfile);
                deadbeef->fclose (fp);
                return cue;
            }
        }
        else {
            currentsample += totalsamples;
        }

        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
    }
    op_free(opusfile);
    deadbeef->fclose (fp);
    return after;
}

static int
opusdec_read_metadata (DB_playItem_t *it) {
    int res = -1;

    DB_FILE *fp = NULL;
    OggOpusFile *opusfile = NULL;
    const OpusHead *head = NULL;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    fp = deadbeef->fopen (uri);
    if (!fp) {
        goto error;
    }
    if (fp->vfs->is_streaming ()) {
        goto error;
    }

    opusfile = opus_file_open (fp);
    if (!opusfile) {
        goto error;
    }

    int tracknum = deadbeef->pl_find_meta_int (it, ":TRACKNUM", -1);
    head = op_head (opusfile, tracknum);
    if (!head) {
        goto error;
    }

    if (update_vorbis_comments (it, opusfile, tracknum)) {
        goto error;
    }

    res = 0;
error:
    if (opusfile) {
        op_free (opusfile);
    }
    if (fp) {
        deadbeef->fclose (fp);
    }
    return res;
}

static void
split_tag(OpusTags *tags, const char *key, const char *value, int valuesize)
{
    while (valuesize > 0) {
        opus_tags_add(tags, key, value);
        size_t l = strlen (value) + 1;
        value += l;
        valuesize -= l;
    }
}

static OpusTags *
tags_list(DB_playItem_t *it, OggOpusFile *opusfile, int link)
{
    const OpusTags *orig = op_tags (opusfile, link);

    OpusTags *tags = calloc (1, sizeof (OpusTags));
    if (!tags)
        return NULL;

    deadbeef->pl_lock ();
    for (DB_metaInfo_t *m = deadbeef->pl_get_metadata_head (it); m; m = m->next) {
        if (strchr (":!_", m->key[0])) {
            break;
        }
        char *key = strdupa (m->key);
        if (!strcasecmp(key, "R128_TRACK_GAIN")) {
            continue;
        }
        split_tag (tags, oggedit_map_tag (key, "meta2tag"), m->value, m->valuesize);
    }

    deadbeef->pl_unlock ();

    // preserve album art
    int i = 0;
    const char *tag;
    while ((tag = opus_tags_query(orig, ALBUM_ART_KEY, i++))) {
        split_tag (tags, ALBUM_ART_KEY, tag, (int)strlen (tag) + 1);
    }

    return tags;
}

static int
opusdec_write_metadata (DB_playItem_t *it) {
    char fname[PATH_MAX];
    deadbeef->pl_get_meta (it, ":URI", fname, sizeof (fname));

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return -1;
    }

    int is_streaming = fp->vfs->is_streaming();
    const OpusFileCallbacks opcb = {
        .read = opus_file_read,
        .seek = is_streaming ? NULL : opus_file_seek,
        .tell = is_streaming ? NULL : opus_file_tell,
        .close = opus_file_close
    };

    int res = 0;

    OggOpusFile *opusfile = op_test_callbacks(fp, &opcb, NULL, 0, &res);
    if (!opusfile) {
        deadbeef->fclose (fp);
        return -1;
    }

    int link = 0;
    if (deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK) {
        link = deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0);
    }

    OpusTags *tags = tags_list(it, opusfile, link);
    if (!tags) {
        op_free (opusfile);
        deadbeef->fclose (fp);
        return -1;
    }

    deadbeef->pl_lock();

    // RG info
    const char *track_gain_str = deadbeef->pl_find_meta (it, ddb_internal_rg_keys[DDB_REPLAYGAIN_TRACKGAIN]);
    float track_gain = 0;
    if (track_gain_str) {
        track_gain = atof (track_gain_str);
    }

    const char *album_gain_str = deadbeef->pl_find_meta (it, ddb_internal_rg_keys[DDB_REPLAYGAIN_ALBUMGAIN]);
    float album_gain = 0;
    if (album_gain_str) {
        album_gain = atof (album_gain_str);
    }

    if (track_gain_str) {
        char s[100];
        snprintf (s, sizeof (s), "%d", 0);
        split_tag (tags, oggedit_map_tag (strdupa ("R128_TRACK_GAIN"), "meta2tag"), s, (int)strlen (s) + 1);
    }

    float value = deadbeef->pl_get_item_replaygain (it, DDB_REPLAYGAIN_ALBUMGAIN);
    if (value != 0) {
        char s[100];
        snprintf (s, sizeof (s), "%d", (int)(album_gain - track_gain) * 256);
        split_tag (tags, oggedit_map_tag (strdupa ("R128_ALBUM_GAIN"), "meta2tag"), s, (int)strlen (s) + 1);
    }

    int header_gain = 0;
    if (track_gain_str) {
        header_gain = (track_gain - 5.f) * 256;
    }

    const char *stream_size_string = deadbeef->pl_find_meta(it, ":STREAM SIZE");
    const size_t stream_size = stream_size_string ? (off_t)atoll(stream_size_string) : 0;
    deadbeef->pl_unlock();
    const off_t file_size = oggedit_write_opus_metadata (deadbeef->fopen(fname), fname, 0, stream_size, header_gain, tags->comments, tags->user_comments);
    opus_tags_clear(tags);

    res = 0;
    if (file_size <= 0) {
        res = -1;
    }

    op_free (opusfile);
    deadbeef->fclose (fp);

    if (!res) {
        set_meta_ll(it, ":FILE_SIZE", (int64_t)file_size);
        res = opusdec_read_metadata(it);
    }
    return res;
}


// define plugin interface
static ddb_decoder2_t plugin = {
    .decoder.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .decoder.plugin.api_vminor = DB_API_VERSION_MINOR,
    .decoder.plugin.version_major = 1,
    .decoder.plugin.version_minor = 0,
    .decoder.plugin.type = DB_PLUGIN_DECODER,
    .decoder.plugin.flags = DDB_PLUGIN_FLAG_LOGGING | DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2,
    .decoder.plugin.id = "opus",
    .decoder.plugin.name = "Opus player",
    .decoder.plugin.descr = "Opus player based on libogg, libopus and libopusfile.",
    .decoder.plugin.copyright =
        "deadbeef-opus\n"
        "Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors\n"
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
        "\n\n\n"
        "libogg, opus, opusfile\n"
        "Copyright (c) 1994-2013 Xiph.Org Foundation and contributors\n"
        "\n\n\n"
        "liboggedit\n"
        "Copyright (C) 2014 Ian Nartowicz <deadbeef@nartowicz.co.uk>\n",
    .decoder.open = opusdec_open,
    .decoder.open2 = opusdec_open2,
    .decoder.init = opusdec_init,
    .decoder.free = opusdec_free,
    .decoder.read = opusdec_read,
    .decoder.seek = opusdec_seek,
    .decoder.seek_sample = opusdec_seek_sample,
    .decoder.insert = opusdec_insert,
    .decoder.read_metadata = opusdec_read_metadata,
    .decoder.write_metadata = opusdec_write_metadata,
    .decoder.exts = exts,
    .seek_sample64 = opusdec_seek_sample64,
};

DB_plugin_t *
opus_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

