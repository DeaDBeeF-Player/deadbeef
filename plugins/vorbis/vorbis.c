/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../../deadbeef.h"
#include "vcedit.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    OggVorbis_File vorbis_file;
    vorbis_info *vi;
    int cur_bit_stream;
    int startsample;
    int endsample;
    int currentsample;
    int last_comment_update;
    DB_playItem_t *ptrack;
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

static const char *metainfo[] = {
    "ARTIST", "artist",
    "TITLE", "title",
    "ALBUM", "album",
    "TRACKNUMBER", "track",
    "DATE", "year",
    "GENRE", "genre",
    "COMMENT", "comment",
    "PERFORMER", "performer",
//    "ENSEMBLE", "band",
    "COMPOSER", "composer",
    "ENCODER", "vendor",
    "DISCNUMBER", "disc",
    "COPYRIGHT", "copyright",
    "TRACKTOTAL", "numtracks",
    "ALBUMARTIST", "band",
    NULL
};

static void
update_vorbis_comments (DB_playItem_t *it, vorbis_comment *vc) {
    if (vc) {
        deadbeef->pl_delete_all_meta (it);
        for (int i = 0; i < vc->comments; i++) {
            char *s = vc->user_comments[i];
            int m;
            for (m = 0; metainfo[m]; m += 2) {
                int l = strlen (metainfo[m]);
                if (vc->comment_lengths[i] > l && !strncasecmp (metainfo[m], s, l) && s[l] == '=') {
                    trace ("ogg adding %s\n", s);
                    deadbeef->pl_append_meta (it, metainfo[m+1], s + l + 1);
                }
            }
            if (!metainfo[m]) {
                if (!strncasecmp (s, "cuesheet=", 9)) {
                    deadbeef->pl_add_meta (it, "cuesheet", s + 9);
                }
                else if (!strncasecmp (s, "replaygain_album_gain=", 22)) {
                    it->replaygain_album_gain = atof (s + 22);
                }
                else if (!strncasecmp (s, "replaygain_album_peak=", 22)) {
                    it->replaygain_album_peak = atof (s + 22);
                }
                else if (!strncasecmp (s, "replaygain_track_gain=", 22)) {
                    it->replaygain_track_gain = atof (s + 22);
                }
                else if (!strncasecmp (s, "replaygain_track_peak=", 22)) {
                    it->replaygain_track_peak = atof (s + 22);
                }
            }
        }
    }
    deadbeef->pl_add_meta (it, "title", NULL);
    uint32_t f = deadbeef->pl_get_item_flags (it);
    f &= ~DDB_TAG_MASK;
    f |= DDB_TAG_VORBISCOMMENTS;
    deadbeef->pl_set_item_flags (it, f);
}

static DB_fileinfo_t *
cvorbis_open (void) {
    DB_fileinfo_t *_info = malloc (sizeof (ogg_info_t));
    ogg_info_t *info = (ogg_info_t *)_info;
    memset (info, 0, sizeof (ogg_info_t));
    return _info;
}

static int
cvorbis_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    ogg_info_t *info = (ogg_info_t *)_info;
    info->info.file = NULL;
    info->vi = NULL;
    if (it->endsample > 0) {
        info->cur_bit_stream = -1;
    }
    else {
        info->cur_bit_stream = it->tracknum;
    }
    info->ptrack = it;
    deadbeef->pl_item_ref (it);

    info->info.file = deadbeef->fopen (it->fname);
    if (!info->info.file) {
        trace ("ogg: failed to open file %s\n", it->fname);
        return -1;
    }
    int ln = deadbeef->fgetlength (info->info.file);
    if (info->info.file->vfs->streaming && ln == -1) {
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
        deadbeef->pl_set_item_duration (it, -1);
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
//        deadbeef->pl_set_item_duration (it, ov_time_total (&vorbis_file, -1));
    }
    info->vi = ov_info (&info->vorbis_file, info->cur_bit_stream);
    if (!info->vi) { // not a vorbis stream
        trace ("not a vorbis stream\n");
        return -1;
    }
    if (info->vi->rate <= 0) {
        trace ("vorbis: bad samplerate\n");
        return -1;
    }
    _info->plugin = &plugin;
    _info->bps = 16;
    //_info->dataSize = ov_pcm_total (&vorbis_file, -1) * vi->channels * 2;
    _info->channels = info->vi->channels;
    _info->samplerate = info->vi->rate;
    _info->readpos = 0;
    info->currentsample = 0;
    if (!info->info.file->vfs->streaming) {
        if (it->endsample > 0) {
            info->startsample = it->startsample;
            info->endsample = it->endsample;
            plugin.seek_sample (_info, 0);
        }
        else {
            info->startsample = 0;
            info->endsample = ov_pcm_total (&info->vorbis_file, -1)-1;
        }
    }
    else {
        info->startsample = 0;
        if (deadbeef->pl_get_item_duration (it) < 0) {
            info->endsample = -1;
        }
        else {
            info->endsample = ov_pcm_total (&info->vorbis_file, -1)-1;
        }
        vorbis_comment *vc = ov_comment (&info->vorbis_file, -1);
        update_vorbis_comments (it, vc);
    }
    return 0;
}

static void
cvorbis_free (DB_fileinfo_t *_info) {
    ogg_info_t *info = (ogg_info_t *)_info;
    if (info) {
        if (info->ptrack) {
            deadbeef->pl_item_unref (info->ptrack);
        }
        if (info->info.file) {
            ov_clear (&info->vorbis_file);
            //fclose (file); //-- ov_clear closes it
        }
        free (info);
    }
}

static int
cvorbis_read (DB_fileinfo_t *_info, char *bytes, int size) {
    ogg_info_t *info = (ogg_info_t *)_info;
//    trace ("cvorbis_read %d bytes\n", size);
    int out_ch = min (_info->channels, 2);
    if (!info->info.file->vfs->streaming) {
        if (info->currentsample + size / (2 * out_ch) > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * 2 * out_ch;
            trace ("size truncated to %d bytes, cursample=%d, info->endsample=%d, totalsamples=%d\n", size, info->currentsample, info->endsample, ov_pcm_total (&info->vorbis_file, -1));
            if (size <= 0) {
                return 0;
            }
        }
    }
    else {
        if (info->ptrack && info->currentsample - info->last_comment_update > 5 * _info->samplerate) {
            if (info->ptrack) {
                info->last_comment_update = info->currentsample;
                vorbis_comment *vc = ov_comment (&info->vorbis_file, -1);
                update_vorbis_comments (info->ptrack, vc);
                deadbeef->plug_trigger_event_trackinfochanged (info->ptrack);
            }
            else {
                info->ptrack = NULL;
            }
        }
    }
//    trace ("cvorbis_read %d bytes[2]\n", size);
    int initsize = size;
    long ret;
    for (;;)
    {
        // read ogg
        int endianess = 0;
#if WORDS_BIGENDIAN
        endianess = 1;
#endif

        if (out_ch != _info->channels) {
            int nframes = size / out_ch / 2;
            int16_t buf[nframes * _info->channels];
            ret=ov_read (&info->vorbis_file, (char*)buf, sizeof(buf), endianess, 2, 1, &info->cur_bit_stream);
            if (ret > 0) {
                int n = ret / _info->channels / 2;
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < out_ch; j++) {
                        ((int16_t *)bytes)[i * out_ch + j] = buf[i * _info->channels + j];
                    }
                }
                ret = n * out_ch * 2;
            }
        }
        else {
            ret=ov_read (&info->vorbis_file, bytes, size, endianess, 2, 1, &info->cur_bit_stream);
        }
        if (ret <= 0)
        {
            if (ret < 0) {
                trace ("ov_read returned %d\n", ret);
                switch (ret) {
                case OV_HOLE:
                    trace ("OV_HOLE\n");
                    break;
                case OV_EBADLINK:
                    trace ("OV_EBADLINK\n");
                    break;
                case OV_EINVAL:
                    trace ("OV_EINVAL\n");
                    break;
                }
            }
            if (ret == OV_HOLE) {
                continue;
            }
            // error or eof
            break;
        }
        else if (ret < size)
        {
            info->currentsample += ret / (out_ch * 2);
            size -= ret;
            bytes += ret;
        }
        else {
            info->currentsample += ret / (out_ch * 2);
            size = 0;
            break;
        }
    }
    _info->readpos = (float)(ov_pcm_tell(&info->vorbis_file)-info->startsample)/info->vi->rate;
    //trace ("cvorbis_read got %d bytes, readpos %f, info->currentsample %d, ret %d\n", initsize-size, _info->readpos, info->currentsample, ret);
    deadbeef->streamer_set_bitrate (ov_bitrate_instant (&info->vorbis_file)/1000);
    return initsize - size;
}

static int
cvorbis_seek_sample (DB_fileinfo_t *_info, int sample) {
    ogg_info_t *info = (ogg_info_t *)_info;
    if (sample < 0) {
        trace ("vorbis: negative seek sample - ignored, but it is a bug!\n");
        return -1;
    }
    if (!info->info.file) {
        trace ("vorbis: file is NULL on seek\n");
        return -1;
    }
    trace ("vorbis: seek to sample %d\n");
    sample += info->startsample;
    int res = ov_pcm_seek (&info->vorbis_file, sample);
    if (res != 0 && res != OV_ENOSEEK) {
        trace ("vorbis: error %x seeking to sample %d\n", sample);
        return -1;
    }
    int tell = ov_pcm_tell (&info->vorbis_file);
    if (tell != sample) {
        trace ("oggvorbis: failed to do sample-accurate seek (%d->%d)\n", sample, tell);
    }
    trace ("vorbis: seek successful\n")
    info->currentsample = sample;
    _info->readpos = (float)(ov_pcm_tell(&info->vorbis_file) - info->startsample)/info->vi->rate;
    return 0;
}

static int
cvorbis_seek (DB_fileinfo_t *_info, float time) {
    ogg_info_t *info = (ogg_info_t *)_info;
    return cvorbis_seek_sample (_info, time * info->vi->rate);
}

static DB_playItem_t *
cvorbis_insert (DB_playItem_t *after, const char *fname) {
    // check for validity
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("vorbis: failed to fopen %s\n", fname);
        return NULL;
    }
    if (fp->vfs->streaming) {
        DB_playItem_t *it = deadbeef->pl_item_alloc ();
        it->fname = strdup (fname);
        it->filetype = "OggVorbis";
        deadbeef->pl_set_item_duration (it, -1);
        deadbeef->pl_add_meta (it, "title", NULL);
        after = deadbeef->pl_insert_item (after, it);
        deadbeef->pl_item_unref (it);
        return after;
    }
    ov_callbacks ovcb = {
        .read_func = cvorbis_fread,
        .seek_func = cvorbis_fseek,
        .close_func = cvorbis_fclose,
        .tell_func = cvorbis_ftell
    };
    OggVorbis_File vorbis_file;
    vorbis_info *vi;
    int err = ov_open_callbacks (fp, &vorbis_file, NULL, 0, ovcb);
    if (err != 0) {
        trace ("ov_open_callbacks returned %d\n", err);
        return NULL;
    }

    long nstreams = ov_streams (&vorbis_file);
    int currentsample = 0;
    for (int stream = 0; stream < nstreams; stream++) {
        vi = ov_info (&vorbis_file, stream);
        if (!vi) { // not a vorbis stream
            trace ("vorbis: ov_info failed for file %s stream %d\n", fname, stream);
            continue;
        }
        float duration = ov_time_total (&vorbis_file, stream);
        int totalsamples = ov_pcm_total (&vorbis_file, stream);

        DB_playItem_t *it = deadbeef->pl_item_alloc ();
        it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
        it->fname = strdup (fname);
        it->filetype = "OggVorbis";
        it->tracknum = stream;
        deadbeef->pl_set_item_duration (it, duration);
        if (nstreams > 1) {
            it->startsample = currentsample;
            it->endsample = currentsample + totalsamples;
            deadbeef->pl_set_item_flags (it, DDB_IS_SUBTRACK);
        }

        // metainfo
        vorbis_comment *vc = ov_comment (&vorbis_file, stream);
        update_vorbis_comments (it, vc);
        int samplerate = vi->rate;

        if (nstreams == 1) {
            DB_playItem_t *cue = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
            if (cue) {
                deadbeef->pl_item_unref (it);
                deadbeef->pl_item_unref (cue);
                ov_clear (&vorbis_file);
                return cue;
            }

            // embedded cue
            const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
            if (cuesheet) {
                cue = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
                if (cue) {
                    deadbeef->pl_item_unref (it);
                    deadbeef->pl_item_unref (cue);
                    ov_clear (&vorbis_file);
                    return cue;
                }
            }
        }
        else {
            currentsample += totalsamples;
        }

        after = deadbeef->pl_insert_item (after, it);
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
    int err = -1;
    DB_FILE *fp = NULL;
    OggVorbis_File vorbis_file;
    vorbis_info *vi = NULL;
    
    fp = deadbeef->fopen (it->fname);
    if (!fp) {
        trace ("cvorbis_read_metadata: failed to fopen %s\n", it->fname);
        return -1;
    }
    if (fp->vfs->streaming) {
        trace ("cvorbis_read_metadata: failed to fopen %s\n", it->fname);
        goto error;
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
        goto error;
    }
    vi = ov_info (&vorbis_file, it->tracknum);
    if (!vi) { // not a vorbis stream
        trace ("cvorbis_read_metadata: failed to ov_open %s\n", it->fname);
        goto error;
    }

    // metainfo
    vorbis_comment *vc = ov_comment (&vorbis_file, it->tracknum);
    if (vc) {
        update_vorbis_comments (it, vc);
    }

    err = 0;
error:
    if (fp) {
        ov_clear (&vorbis_file);
    }
    return err;

}

int
cvorbis_write_metadata (DB_playItem_t *it) {
    vcedit_state *state = NULL;
    vorbis_comment *vc = NULL;
    FILE *fp = NULL;
    FILE *out = NULL;
    int err = -1;
    char outname[PATH_MAX] = "";

    struct field {
        struct field *next;
        int size;
        uint8_t data[0];
    };

    struct field *preserved_fields = NULL;

    state = vcedit_new_state ();
    if (!state) {
        trace ("cvorbis_write_metadata: vcedit_new_state failed\n");
        return -1;
    }
    fp = fopen (it->fname, "rb");
    if (!fp) {
        trace ("cvorbis_write_metadata: failed to read metadata from %s\n", it->fname);
        goto error;
    }
    if (vcedit_open (state, fp) != 0) {
        trace ("cvorbis_write_metadata: vcedit_open failed, error: %s\n", vcedit_error (state));
        goto error;
    }

    vc = vcedit_comments (state);
    if (!vc) {
        trace ("cvorbis_write_metadata: vcedit_comments failed, error: %s\n", vcedit_error (state));
        goto error;
    }

    // copy all unknown fields to separate buffer
    for (int i = 0; i < vc->comments; i++) {
        int m;
        for (m = 0; metainfo[m]; m += 2) {
            int l = strlen (metainfo[m]);
            if (vc->comment_lengths[i] > l && !strncasecmp (vc->user_comments[i], metainfo[m], l) && vc->user_comments[i][l] == '=') {
                break;
            }
        }
        if (!metainfo[m]) {
            trace ("preserved field: %s\n", vc->user_comments[i]);
            // unknown field
            struct field *f = malloc (sizeof (struct field) + vc->comment_lengths[i]);
            memset (f, 0, sizeof (struct field));
            memcpy (f->data, vc->user_comments[i], vc->comment_lengths[i]);
            f->size = vc->comment_lengths[i];
            f->next = preserved_fields;
            preserved_fields = f;
        }
    }

    vorbis_comment_clear(vc);
    vorbis_comment_init(vc);

    // add known fields
    for (int m = 0; metainfo[m]; m += 2) {
        const char *val = deadbeef->pl_find_meta (it, metainfo[m+1]);
        if (val && *val) {
            while (val) {
                const char *next = strchr (val, '\n');
                int l;
                if (next) {
                    l = next - val;
                    next++;
                }
                else {
                    l = strlen (val);
                }
                if (l > 0) {
                    char s[100+l+1];
                    int n = snprintf (s, sizeof (s), "%s=", metainfo[m]);
                    strncpy (s+n, val, l);
                    *(s+n+l) = 0;
                    vorbis_comment_add (vc, s);
                }
                val = next;
            }
        }
    }

    // add preserved fields
    for (struct field *f = preserved_fields; f; f = f->next) {
        vorbis_comment_add (vc, f->data);
    }

    snprintf (outname, sizeof (outname), "%s.temp.ogg", it->fname);

    out = fopen (outname, "w+b");
    if (!out) {
        trace ("cvorbis_write_metadata: failed to open %s for writing\n", outname);
        goto error;
    }

    if (vcedit_write (state, out) < 0) {
        trace ("cvorbis_write_metadata: failed to write tags to %s, error: %s\n", it->fname, vcedit_error (state));
        goto error;
    }

    err = 0;
error:
    if (fp) {
        fclose (fp);
    }
    if (out) {
        fclose (out);
    }
    if (state) {
        vcedit_clear (state);
    }
    while (preserved_fields) {
        struct field *next = preserved_fields->next;
        free (preserved_fields);
        preserved_fields = next;
    }

    if (!err) {
        rename (outname, it->fname);
    }
    else if (out) {
        unlink (outname);
    }

    return err;
}


static const char * exts[] = { "ogg", "ogx", NULL };
static const char *filetypes[] = { "OggVorbis", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdogg",
    .plugin.name = "OggVorbis decoder",
    .plugin.descr = "OggVorbis decoder using standard xiph.org libraries",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = vorbis_start,
    .plugin.stop = vorbis_stop,
    .open = cvorbis_open,
    .init = cvorbis_init,
    .free = cvorbis_free,
    .read_int16 = cvorbis_read,
    // vorbisfile can't output float32
//    .read_float32 = cvorbis_read_float32,
    .seek = cvorbis_seek,
    .seek_sample = cvorbis_seek_sample,
    .insert = cvorbis_insert,
    .read_metadata = cvorbis_read_metadata,
    .write_metadata = cvorbis_write_metadata,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
vorbis_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
