/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

static DB_FILE *file;
static OggVorbis_File vorbis_file;
static vorbis_info *vi;
static int cur_bit_stream;
static int startsample;
static int endsample;
static int currentsample;
static int last_comment_update;
static DB_playItem_t *ptrack;

static void
cvorbis_free (void);

static size_t
cvorbis_fread (void *ptr, size_t size, size_t nmemb, void *datasource) {
    size_t ret = deadbeef->fread (ptr, size, nmemb, datasource);
    trace ("cvorbis_fread %d %d %d\n", size, nmemb, ret);
    return ret;
}

static int
cvorbis_fseek (void *datasource, ogg_int64_t offset, int whence) {
    DB_FILE *f = (DB_FILE *)datasource;
    return deadbeef->fseek (datasource, offset, whence);
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
                deadbeef->pl_add_meta (it, "date", vc->user_comments[i] + 5);
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

static int
cvorbis_init (DB_playItem_t *it) {
    file = NULL;
    vi = NULL;
    cur_bit_stream = -1;
    ptrack = it;

    file = plugin.info.file = deadbeef->fopen (it->fname);
    if (!file) {
        return -1;
    }
    memset (&plugin.info, 0, sizeof (plugin.info));

    int ln = deadbeef->fgetlength (file);
    if (file->vfs->streaming && ln == -1) {
        ov_callbacks ovcb = {
            .read_func = cvorbis_fread,
            .seek_func = NULL,
            .close_func = cvorbis_fclose,
            .tell_func = NULL
        };

        int err;
        trace ("calling ov_open_callbacks\n");
        if (err = ov_open_callbacks (file, &vorbis_file, NULL, 0, ovcb) != 0) {
            trace ("ov_open_callbacks returned %d\n", err);
            plugin.free ();
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
        int err;
        if (err = ov_open_callbacks (file, &vorbis_file, NULL, 0, ovcb) != 0) {
            trace ("ov_open_callbacks returned %d\n", err);
            plugin.free ();
            return -1;
        }
//        deadbeef->pl_set_item_duration (it, ov_time_total (&vorbis_file, -1));
    }
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        cvorbis_free ();
        trace ("not a vorbis stream\n");
        return -1;
    }
    plugin.info.bps = 16;
    //plugin.info.dataSize = ov_pcm_total (&vorbis_file, -1) * vi->channels * 2;
    plugin.info.channels = vi->channels;
    plugin.info.samplerate = vi->rate;
    plugin.info.readpos = 0;
    currentsample = 0;
    if (!file->vfs->streaming) {
        if (it->endsample > 0) {
            startsample = it->startsample;
            endsample = it->endsample;
            plugin.seek_sample (0);
        }
        else {
            startsample = 0;
            endsample = ov_pcm_total (&vorbis_file, -1)-1;
        }
    }
    else {
        startsample = 0;
        if (deadbeef->pl_get_item_duration (it) < 0) {
            endsample = -1;
        }
        else {
            endsample = ov_pcm_total (&vorbis_file, -1)-1;
        }
        vorbis_comment *vc = ov_comment (&vorbis_file, -1);
        update_vorbis_comments (it, vc);
    }
    return 0;
}

static void
cvorbis_free (void) {
    plugin.info.file = NULL;
    if (file) {
        ptrack = NULL;
        ov_clear (&vorbis_file);
        //fclose (file); //-- ov_clear closes it
        file = NULL;
        vi = NULL;
    }
}

static int
cvorbis_read (char *bytes, int size) {
//    trace ("cvorbis_read %d bytes\n", size);
    if (!file->vfs->streaming) {
        if (currentsample + size / (2 * plugin.info.channels) > endsample) {
            size = (endsample - currentsample + 1) * 2 * plugin.info.channels;
            trace ("size truncated to %d bytes, cursample=%d, endsample=%d, totalsamples=%d\n", size, currentsample, endsample, ov_pcm_total (&vorbis_file, -1));
            if (size <= 0) {
                return 0;
            }
        }
    }
    else {
        if (ptrack && currentsample - last_comment_update > 5 * plugin.info.samplerate) {
            int idx = deadbeef->pl_get_idx_of (ptrack);
            if (idx >= 0) {
                last_comment_update = currentsample;
                vorbis_comment *vc = ov_comment (&vorbis_file, -1);
                update_vorbis_comments (ptrack, vc);
                deadbeef->sendmessage (M_TRACKCHANGED, 0, idx, 0);
            }
            else {
                ptrack = NULL;
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
        ret=ov_read (&vorbis_file, bytes, size, endianess, 2, 1, &cur_bit_stream);
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
            currentsample += ret / (vi->channels * 2);
            size -= ret;
            bytes += ret;
        }
        else {
            currentsample += ret / (vi->channels * 2);
            size = 0;
            break;
        }
    }
    plugin.info.readpos = (float)(ov_pcm_tell(&vorbis_file)-startsample)/vi->rate;
    trace ("cvorbis_read got %d bytes, readpos %f, currentsample %d, ret %d\n", initsize-size, plugin.info.readpos, currentsample, ret);
    return initsize - size;
}

static int
cvorbis_seek_sample (int sample) {
    if (sample < 0) {
        trace ("vorbis: negative seek sample - ignored, but it is a bug!\n");
        return -1;
    }
    if (!file) {
        trace ("vorbis: file is NULL on seek\n");
        return -1;
    }
    trace ("vorbis: seek to sample %d\n");
    sample += startsample;
    int res = ov_pcm_seek (&vorbis_file, sample);
    if (res != 0 && res != OV_ENOSEEK) {
        trace ("vorbis: error %x seeking to sample %d\n", sample);
        return -1;
    }
    int tell = ov_pcm_tell (&vorbis_file);
    if (tell != sample) {
        trace ("oggvorbis: failed to do sample-accurate seek (%d->%d)\n", sample, tell);
    }
    trace ("vorbis: seek successful\n")
    currentsample = sample;
    plugin.info.readpos = (float)(ov_pcm_tell(&vorbis_file) - startsample)/vi->rate;
    return 0;
}

static int
cvorbis_seek (float time) {
    return cvorbis_seek_sample (time * vi->rate);
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
        it->decoder = &plugin;
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
    ov_open_callbacks (fp, &vorbis_file, NULL, 0, ovcb);
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        trace ("vorbis: failed to ov_open %s\n", fname);
        return NULL;
    }
    float duration = ov_time_total (&vorbis_file, -1);
    int totalsamples = ov_pcm_total (&vorbis_file, -1);
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, fname, &plugin, "OggVorbis", totalsamples, vi->rate);
    if (cue_after) {
        ov_clear (&vorbis_file);
        return cue_after;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "OggVorbis";
    deadbeef->pl_set_item_duration (it, duration);

    // metainfo
    vorbis_comment *vc = ov_comment (&vorbis_file, -1);
    update_vorbis_comments (it, vc);
    ov_clear (&vorbis_file);
    after = deadbeef->pl_insert_item (after, it);
    return after;
}

static int
vorbis_trackdeleted (DB_event_song_t *ev, uintptr_t data) {
    if (ev->song == ptrack) {
        ptrack = NULL;
    }
    return 0;
}

static int
vorbis_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_TRACKDELETED, DB_CALLBACK (vorbis_trackdeleted), 0);
}

static int
vorbis_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_TRACKDELETED, DB_CALLBACK (vorbis_trackdeleted), 0);
}

static const char * exts[] = { "ogg", NULL };
static const char *filetypes[] = { "OggVorbis", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
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
    .id = "stdogg",
    .filetypes = filetypes
};

DB_plugin_t *
vorbis_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
