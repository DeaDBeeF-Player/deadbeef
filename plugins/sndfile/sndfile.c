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
    SNDFILE *ctx;
    DB_FILE *file;
    int startsample;
    int endsample;
    int currentsample;
    int bitrate;
} sndfilectx_t;

static sndfilectx_t sfctx;


// vfs wrapper for sf
static sf_count_t
sf_vfs_get_filelen (void *user_data) {
    sndfilectx_t *ctx = user_data;
    return deadbeef->fgetlength (ctx->file);
}

static sf_count_t
sf_vfs_read (void *ptr, sf_count_t count, void *user_data) {
    sndfilectx_t *ctx = user_data;
    return deadbeef->fread (ptr, 1, count, ctx->file);
}

static sf_count_t
sf_vfs_write (const void *ptr, sf_count_t count, void *user_data) {
    return -1;
}

static sf_count_t
sf_vfs_seek (sf_count_t offset, int whence, void *user_data) {
    sndfilectx_t *ctx = user_data;
    int ret = deadbeef->fseek (ctx->file, offset, whence);
    if (!ret) {
        return offset;
    }
    return -1;
}

static sf_count_t
sf_vfs_tell (void *user_data) {
    sndfilectx_t *ctx = user_data;
    return deadbeef->ftell (ctx->file);
}

static SF_VIRTUAL_IO vfs = {
    .get_filelen = sf_vfs_get_filelen,
    .seek = sf_vfs_seek,
    .read = sf_vfs_read,
    .write = sf_vfs_write,
    .tell = sf_vfs_tell
};

static int
sndfile_init (DB_playItem_t *it) {
    memset (&sfctx, 0, sizeof (sfctx));
    SF_INFO inf;
    DB_FILE *fp = deadbeef->fopen (it->fname);
    if (!fp) {
        trace ("sndfile: failed to open %s\n", it->fname);
        return -1;
    }
    int fsize = deadbeef->fgetlength (fp);

    sfctx.file = fp;
    sfctx.ctx = sf_open_virtual (&vfs, SFM_READ, &inf, &sfctx);
    if (!sfctx.ctx) {
        return -1;
    }
    plugin.info.bps = 16;
    plugin.info.channels = inf.channels;
    plugin.info.samplerate = inf.samplerate;
    plugin.info.readpos = 0;
    if (it->endsample > 0) {
        sfctx.startsample = it->startsample;
        sfctx.endsample = it->endsample;
        if (plugin.seek_sample (0) < 0) {
            plugin.free ();
            return -1;
        }
    }
    else {
        sfctx.startsample = 0;
        sfctx.endsample = inf.frames-1;
    }
    // hack bitrate
    float sec = (float)(sfctx.endsample-sfctx.startsample) / inf.samplerate;
    if (sec > 0) {
        sfctx.bitrate = fsize / sec * 8 / 1000;
    }
    else {
        sfctx.bitrate = -1;
    }

    return 0;
}

static void
sndfile_free (void) {
    if (sfctx.ctx) {
        sf_close (sfctx.ctx);
    }
    if (sfctx.file) {
        deadbeef->fclose (sfctx.file);
    }
    memset (&sfctx, 0, sizeof (sfctx));
}

static int
sndfile_read_int16 (char *bytes, int size) {
    if (size / (2 * plugin.info.channels) + sfctx.currentsample > sfctx.endsample) {
        size = (sfctx.endsample - sfctx.currentsample + 1) * 2 * plugin.info.channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, sfctx.currentsample, sfctx.endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n = sf_readf_short (sfctx.ctx, (short *)bytes, size/(2*plugin.info.channels));
    sfctx.currentsample += n;
    size = n * 2 * plugin.info.channels;
    plugin.info.readpos = (float)(sfctx.currentsample-sfctx.startsample)/plugin.info.samplerate;
    if (sfctx.bitrate > 0) {
        deadbeef->streamer_set_bitrate (sfctx.bitrate);
    }
    return size;
}

static int
sndfile_read_float32 (char *bytes, int size) {
    if (size / (4 * plugin.info.channels) + sfctx.currentsample > sfctx.endsample) {
        size = (sfctx.endsample - sfctx.currentsample + 1) * 4 * plugin.info.channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, sfctx.currentsample, sfctx.endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int n = sf_readf_float (sfctx.ctx, (float *)bytes, size/(4*plugin.info.channels));
    sfctx.currentsample += n;
    size = n * 4 * plugin.info.channels;
    plugin.info.readpos = (float)(sfctx.currentsample-sfctx.startsample)/plugin.info.samplerate;
    if (sfctx.bitrate > 0) {
        deadbeef->streamer_set_bitrate (sfctx.bitrate);
    }
    return size;
}

static int
sndfile_seek_sample (int sample) {
    int ret = sf_seek (sfctx.ctx, sample + sfctx.startsample, SEEK_SET);
    if (ret < 0) {
        return -1;
    }
    sfctx.currentsample = ret;
    plugin.info.readpos = (float)(sfctx.currentsample - sfctx.startsample) / plugin.info.samplerate;
    return 0;
}

static int
sndfile_seek (float sec) {
    return sndfile_seek_sample (sec * plugin.info.samplerate);
}

static DB_playItem_t *
sndfile_insert (DB_playItem_t *after, const char *fname) {
    SF_INFO inf;
    sndfilectx_t sfctx;
    sfctx.file = deadbeef->fopen (fname);
    if (!sfctx.file) {
        trace ("sndfile: failed to open %s\n", fname);
        return NULL;
    }
    sfctx.ctx = sf_open_virtual (&vfs, SFM_READ, &inf, &sfctx);
    if (!sfctx.ctx) {
        trace ("sndfile: sf_open failed");
        return NULL;
    }
    int totalsamples = inf.frames;
    int samplerate = inf.samplerate;
    sf_close (sfctx.ctx);
    deadbeef->fclose (sfctx.file);

    float duration = (float)totalsamples / samplerate;
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
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
    .id = "sndfile",
    .filetypes = filetypes
};

DB_plugin_t *
sndfile_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
