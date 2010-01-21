/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __linux__
#define _LARGEFILE64_SOURCE
#endif
#include <string.h>
#include <sndfile.h>
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    SNDFILE *ctx;
    DB_FILE *file;
    int startsample;
    int endsample;
    int currentsample;
    int bitrate;
} sndfile_info_t;

// vfs wrapper for sf
static sf_count_t
sf_vfs_get_filelen (void *user_data) {
    sndfile_info_t *ctx = user_data;
    return deadbeef->fgetlength (ctx->file);
}

static sf_count_t
sf_vfs_read (void *ptr, sf_count_t count, void *user_data) {
    sndfile_info_t *ctx = user_data;
    return deadbeef->fread (ptr, 1, count, ctx->file);
}

static sf_count_t
sf_vfs_write (const void *ptr, sf_count_t count, void *user_data) {
    return -1;
}

static sf_count_t
sf_vfs_seek (sf_count_t offset, int whence, void *user_data) {
    sndfile_info_t *ctx = user_data;
    int ret = deadbeef->fseek (ctx->file, offset, whence);
    if (!ret) {
        return offset;
    }
    return -1;
}

static sf_count_t
sf_vfs_tell (void *user_data) {
    sndfile_info_t *ctx = user_data;
    return deadbeef->ftell (ctx->file);
}

static SF_VIRTUAL_IO vfs = {
    .get_filelen = sf_vfs_get_filelen,
    .seek = sf_vfs_seek,
    .read = sf_vfs_read,
    .write = sf_vfs_write,
    .tell = sf_vfs_tell
};

static DB_fileinfo_t *
sndfile_init (DB_playItem_t *it) {
    DB_fileinfo_t *_info = malloc (sizeof (sndfile_info_t));
    sndfile_info_t *info = (sndfile_info_t*)_info;
    memset (info, 0, sizeof (sndfile_info_t));

    SF_INFO inf;
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        trace ("sndfile: failed to open %s\n", it->fname);
        plugin.free (_info);
        return NULL;
    }
    int fsize = deadbeef->fgetlength (fp);

    info->file = fp;
    info->ctx = sf_open_virtual (&vfs, SFM_READ, &inf, info);
    if (!info->ctx) {
        trace ("sndfile: %s: unsupported file format\n");
        plugin.free (_info);
        return NULL;
    }
    _info->plugin = &plugin;
    _info->bps = 16;
    _info->channels = inf.channels;
    _info->samplerate = inf.samplerate;
    _info->readpos = 0;
    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        if (plugin.seek_sample (_info, 0) < 0) {
            plugin.free (_info);
            return NULL;
        }
    }
    else {
        info->startsample = 0;
        info->endsample = inf.frames-1;
    }
    // hack bitrate
    float sec = (float)(info->endsample-info->startsample) / inf.samplerate;
    if (sec > 0) {
        info->bitrate = fsize / sec * 8 / 1000;
    }
    else {
        info->bitrate = -1;
    }

    return _info;
}

static void
sndfile_free (DB_fileinfo_t *_info) {
    sndfile_info_t *info = (sndfile_info_t*)_info;
    if (info->ctx) {
        sf_close (info->ctx);
    }
    if (info->file) {
        deadbeef->fclose (info->file);
    }
    memset (&info, 0, sizeof (info));
}

static int
sndfile_read_int16 (DB_fileinfo_t *_info, char *bytes, int size) {
    sndfile_info_t *info = (sndfile_info_t*)_info;
    if (size / (2 * _info->channels) + info->currentsample > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 2 * _info->channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n = sf_readf_short (info->ctx, (short *)bytes, size/(2*_info->channels));
    info->currentsample += n;
    size = n * 2 * _info->channels;
    _info->readpos = (float)(info->currentsample-info->startsample)/_info->samplerate;
    if (info->bitrate > 0) {
        deadbeef->streamer_set_bitrate (info->bitrate);
    }
    return size;
}

static int
sndfile_read_float32 (DB_fileinfo_t *_info, char *bytes, int size) {
    sndfile_info_t *info = (sndfile_info_t*)_info;
    if (size / (4 * _info->channels) + info->currentsample > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * 4 * _info->channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, info->currentsample, info->endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n = sf_readf_float (info->ctx, (float *)bytes, size/(4*_info->channels));
    info->currentsample += n;
    size = n * 4 * _info->channels;
    _info->readpos = (float)(info->currentsample-info->startsample)/_info->samplerate;
    if (info->bitrate > 0) {
        deadbeef->streamer_set_bitrate (info->bitrate);
    }
    return size;
}

static int
sndfile_seek_sample (DB_fileinfo_t *_info, int sample) {
    sndfile_info_t *info = (sndfile_info_t*)_info;
    int ret = sf_seek (info->ctx, sample + info->startsample, SEEK_SET);
    if (ret < 0) {
        return -1;
    }
    info->currentsample = ret;
    _info->readpos = (float)(info->currentsample - info->startsample) / _info->samplerate;
    return 0;
}

static int
sndfile_seek (DB_fileinfo_t *_info, float sec) {
    return sndfile_seek_sample (_info, sec * _info->samplerate);
}

static DB_playItem_t *
sndfile_insert (DB_playItem_t *after, const char *fname) {
    SF_INFO inf;
    sndfile_info_t info;
    memset (&info, 0, sizeof (info));
    info.file = deadbeef->fopen (fname);
    if (!info.file) {
        trace ("sndfile: failed to open %s\n", fname);
        return NULL;
    }
    info.ctx = sf_open_virtual (&vfs, SFM_READ, &inf, &info);
    if (!info.ctx) {
        trace ("sndfile: sf_open failed");
        return NULL;
    }
    int totalsamples = inf.frames;
    int samplerate = inf.samplerate;
    sf_close (info.ctx);
    deadbeef->fclose (info.file);

    float duration = (float)totalsamples / samplerate;
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
    it->fname = strdup (fname);
    it->filetype = "wav";
    deadbeef->pl_set_item_duration (it, duration);

    trace ("sndfile: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
    if (cue_after) {
        return cue_after;
    }

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);

    return after;
}

static const char * exts[] = { "wav", "aif", "aiff", "snd", "au", "paf", "svx", "nist", "voc", "ircam", "w64", "mat4", "mat5", "pvf", "xi", "htk", "sds", "avr", "wavex", "sd2", "caf", "wve", NULL };
static const char *filetypes[] = { "wav", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "sndfile",
    .plugin.name = "pcm player",
    .plugin.descr = "wav/aiff player using libsndfile",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = sndfile_init,
    .free = sndfile_free,
    .read_int16 = sndfile_read_int16,
    .read_float32 = sndfile_read_float32,
    .seek = sndfile_seek,
    .seek_sample = sndfile_seek_sample,
    .insert = sndfile_insert,
    .exts = exts,
    .filetypes = filetypes
};

DB_plugin_t *
sndfile_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
