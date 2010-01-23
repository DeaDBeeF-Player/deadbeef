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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf (stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    DB_FILE *file;
    OggVorbis_File vorbis_file;
    vorbis_info *vi;
    int cur_bit_stream;
    int startsample;
    int endsample;
    int currentsample;
    int last_comment_update;
    DB_playItem_t *ptrack; // FIXME: addref that
} ogg_info_t;

static size_t
cvorbis_fread (void *ptr, size_t size, size_t nmemb, void *datasource) {
    size_t ret = deadbeef->fread (ptr, size, nmemb, datasource);
    trace ("cvorbis_fread %d %d %d\n", size, nmemb, ret);
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

static void
update_vorbis_comments (DB_playItem_t *it, vorbis_comment *vc) {
    if (vc) {
        deadbeef->pl_delete_all_meta (it);
        deadbeef->pl_add_meta (it, "vendor", vc->vendor);
        for (int i = 0; i < vc->comments; i++) {
            if (!strncasecmp (vc->user_comments[i], "artist=", 7)) {
                deadbeef->pl_add_meta (it, "artist", vc->user_comments[i] + 7);
            }
            else if (!strncasecmp (vc->user_comments[i], "album=", 6)) {
                deadbeef->pl_add_meta (it, "album", vc->user_comments[i] + 6);
            }
            else if (!strncasecmp (vc->user_comments[i], "title=", 6)) {
                deadbeef->pl_add_meta (it, "title", vc->user_comments[i] + 6);
            }
            else if (!strncasecmp (vc->user_comments[i], "tracknumber=", 12)) {
                deadbeef->pl_add_meta (it, "track", vc->user_comments[i] + 12);
            }
            else if (!strncasecmp (vc->user_comments[i], "date=", 5)) {
                deadbeef->pl_add_meta (it, "year", vc->user_comments[i] + 5);
            }
            else if (!strncasecmp (vc->user_comments[i], "comment=", 8)) {
                deadbeef->pl_add_meta (it, "comment", vc->user_comments[i] + 8);
            }
            else if (!strncasecmp (vc->user_comments[i], "genre=", 6)) {
                deadbeef->pl_add_meta (it, "genre", vc->user_comments[i] + 6);
            }
            else if (!strncasecmp (vc->user_comments[i], "copyright=", 10)) {
                deadbeef->pl_add_meta (it, "copyright", vc->user_comments[i] + 10);
            }
            else if (!strncasecmp (vc->user_comments[i], "cuesheet=", 9)) {
                deadbeef->pl_add_meta (it, "cuesheet", vc->user_comments[i] + 9);
            }
            else if (!strncasecmp (vc->user_comments[i], "replaygain_album_gain=", 22)) {
                it->replaygain_album_gain = atof (vc->user_comments[i] + 22);
            }
            else if (!strncasecmp (vc->user_comments[i], "replaygain_album_peak=", 22)) {
                it->replaygain_album_peak = atof (vc->user_comments[i] + 22);
            }
            else if (!strncasecmp (vc->user_comments[i], "replaygain_track_gain=", 22)) {
                it->replaygain_track_gain = atof (vc->user_comments[i] + 22);
            }
            else if (!strncasecmp (vc->user_comments[i], "replaygain_track_peak=", 22)) {
                it->replaygain_track_peak = atof (vc->user_comments[i] + 22);
            }
        }
    }
    deadbeef->pl_add_meta (it, "title", NULL);
}

static DB_fileinfo_t *
cvorbis_init (DB_playItem_t *it) {
    DB_fileinfo_t *_info = malloc (sizeof (ogg_info_t));
    ogg_info_t *info = (ogg_info_t *)_info;
    memset (info, 0, sizeof (ogg_info_t));
    info->file = NULL;
    info->vi = NULL;
    info->cur_bit_stream = -1;
    info->ptrack = it;
    // FIXME: add reference to that one
    // deadbeef->pl_item_ref (it);

    info->file = deadbeef->fopen (it->fname);
    if (!info->file) {
        trace ("ogg: failed to open file %s\n", it->fname);
        plugin.free (_info);
        return NULL;
    }
    int ln = deadbeef->fgetlength (info->file);
    if (info->file->vfs->streaming && ln == -1) {
        ov_callbacks ovcb = {
            .read_func = cvorbis_fread,
            .seek_func = NULL,
            .close_func = cvorbis_fclose,
            .tell_func = NULL
        };

        trace ("calling ov_open_callbacks\n");
        int err = ov_open_callbacks (info->file, &info->vorbis_file, NULL, 0, ovcb);
        if (err != 0) {
            trace ("ov_open_callbacks returned %d\n", err);
            plugin.free (_info);
            return NULL;
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
        int err = ov_open_callbacks (info->file, &info->vorbis_file, NULL, 0, ovcb);
        if (err != 0) {
            trace ("ov_open_callbacks returned %d\n", err);
            plugin.free (_info);
            return NULL;
        }
//        deadbeef->pl_set_item_duration (it, ov_time_total (&vorbis_file, -1));
    }
    info->vi = ov_info (&info->vorbis_file, -1);
    if (!info->vi) { // not a vorbis stream
        trace ("not a vorbis stream\n");
        plugin.free (_info);
        return NULL;
    }
    _info->plugin = &plugin;
    _info->bps = 16;
    //_info->dataSize = ov_pcm_total (&vorbis_file, -1) * vi->channels * 2;
    _info->channels = info->vi->channels;
    _info->samplerate = info->vi->rate;
    _info->readpos = 0;
    info->currentsample = 0;
    if (!info->file->vfs->streaming) {
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
    return _info;
}

static void
cvorbis_free (DB_fileinfo_t *_info) {
    ogg_info_t *info = (ogg_info_t *)_info;
    if (info) {
        if (info->file) {
            info->ptrack = NULL;
            // FIXME: unref that
            // if (info->ptrack) {
            //     deadbeef->pl_item_unref (info->ptrack);
            // }
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
    if (!info->file->vfs->streaming) {
        if (info->currentsample + size / (2 * _info->channels) > info->endsample) {
            size = (info->endsample - info->currentsample + 1) * 2 * _info->channels;
            trace ("size truncated to %d bytes, cursample=%d, info->endsample=%d, totalsamples=%d\n", size, info->currentsample, info->endsample, ov_pcm_total (&vorbis_file, -1));
            if (size <= 0) {
                return 0;
            }
        }
    }
    else {
        if (info->ptrack && info->currentsample - info->last_comment_update > 5 * _info->samplerate) {
            int idx = deadbeef->pl_get_idx_of (info->ptrack);
            if (idx >= 0) {
                info->last_comment_update = info->currentsample;
                vorbis_comment *vc = ov_comment (&info->vorbis_file, -1);
                update_vorbis_comments (info->ptrack, vc);
                deadbeef->sendmessage (M_TRACKCHANGED, 0, idx, 0);
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
        ret=ov_read (&info->vorbis_file, bytes, size, endianess, 2, 1, &info->cur_bit_stream);
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
            info->currentsample += ret / (info->vi->channels * 2);
            size -= ret;
            bytes += ret;
        }
        else {
            info->currentsample += ret / (info->vi->channels * 2);
            size = 0;
            break;
        }
    }
    _info->readpos = (float)(ov_pcm_tell(&info->vorbis_file)-info->startsample)/info->vi->rate;
    trace ("cvorbis_read got %d bytes, readpos %f, info->currentsample %d, ret %d\n", initsize-size, _info->readpos, info->currentsample, ret);
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
    if (!info->file) {
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
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        trace ("vorbis: failed to ov_open %s\n", fname);
        return NULL;
    }
    float duration = ov_time_total (&vorbis_file, -1);
    int totalsamples = ov_pcm_total (&vorbis_file, -1);

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = "OggVorbis";
    deadbeef->pl_set_item_duration (it, duration);

    // metainfo
    vorbis_comment *vc = ov_comment (&vorbis_file, -1);
    update_vorbis_comments (it, vc);
    int samplerate = vi->rate;
    ov_clear (&vorbis_file);

    DB_playItem_t *cue = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
    if (cue) {
        deadbeef->pl_item_unref (it);
        return cue;
    }

    // embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        cue = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
        if (cue) {
            deadbeef->pl_item_unref (it);
            return cue;
        }
    }

    after = deadbeef->pl_insert_item (after, it);
    return after;
}

// that won't be needed with refcounting

//static int
//vorbis_trackdeleted (DB_event_track_t *ev, uintptr_t data) {
//    if (ev->track == info->ptrack) {
//        info->ptrack = NULL;
//    }
//    return 0;
//}

static int
vorbis_start (void) {
//    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_TRACKDELETED, DB_CALLBACK (vorbis_trackdeleted), 0);
    return 0;
}

static int
vorbis_stop (void) {
//    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_TRACKDELETED, DB_CALLBACK (vorbis_trackdeleted), 0);
    return 0;
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
    .init = cvorbis_init,
    .free = cvorbis_free,
    .read_int16 = cvorbis_read,
    // vorbisfile can't output float32
//    .read_float32 = cvorbis_read_float32,
    .seek = cvorbis_seek,
    .seek_sample = cvorbis_seek_sample,
    .insert = cvorbis_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
vorbis_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
