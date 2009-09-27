/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    SNDFILE *ctx;
    int startsample;
    int endsample;
    int currentsample;
} sndfilectx_t;

static sndfilectx_t sfctx;

static int
sndfile_init (DB_playItem_t *it) {
    memset (&sfctx, 0, sizeof (sfctx));
    SF_INFO inf;
    sfctx.ctx = sf_open (it->fname, SFM_READ, &inf);
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
    return 0;
}

static void
sndfile_free (void) {
    if (sfctx.ctx) {
        sf_close (sfctx.ctx);
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
    return size;
}

static int
sndfile_seek_sample (int sample) {
    sfctx.currentsample = sf_seek (sfctx.ctx, sample + sfctx.startsample, SEEK_SET);
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
    SNDFILE *sf = sf_open (fname, SFM_READ, &inf);
    if (!sf) {
        trace ("sndfile: sf_open failed");
        return NULL;
    }
    int totalsamples = inf.frames;
    int samplerate = inf.samplerate;
    sf_close (sf);
    float duration = (float)totalsamples / samplerate;
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, fname, &plugin, "sndfile", totalsamples, samplerate);
    if (cue_after) {
        return cue_after;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "wav";
    it->duration = duration;

    trace ("sndfile: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);

    return after;
}

static const char * exts[] = { "wav", NULL };
static const char *filetypes[] = { "wav", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "SNDFILE decoder",
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
