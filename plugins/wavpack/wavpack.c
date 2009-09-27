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
#include <wavpack/wavpack.h>
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    WavpackContext *ctx;
    int startsample;
    int endsample;
} wvctx_t;

static wvctx_t wvctx;

static int
wv_init (DB_playItem_t *it) {
    memset (&wvctx, 0, sizeof (wvctx));
    wvctx.ctx = WavpackOpenFileInput (it->fname, NULL, 0, 0);
    if (!wvctx.ctx) {
        return -1;
    }
    plugin.info.bps = WavpackGetBitsPerSample (wvctx.ctx);
    plugin.info.channels = WavpackGetNumChannels (wvctx.ctx);
    plugin.info.samplerate = WavpackGetSampleRate (wvctx.ctx);
    plugin.info.readpos = 0;
    if (it->endsample > 0) {
        wvctx.startsample = it->startsample;
        wvctx.endsample = it->endsample;
        if (plugin.seek_sample (0) < 0) {
            plugin.free ();
            return -1;
        }
    }
    else {
        wvctx.startsample = 0;
        wvctx.endsample = WavpackGetNumSamples (wvctx.ctx)-1;
    }
    return 0;
}

static void
wv_free (void) {
    if (wvctx.ctx) {
        WavpackCloseFile (wvctx.ctx);
    }
    memset (&wvctx, 0, sizeof (wvctx));
}


static int
wv_read_int16 (char *bytes, int size) {
    int currentsample = WavpackGetSampleIndex (wvctx.ctx);
    if (size / (2 * plugin.info.channels) + currentsample > wvctx.endsample) {
        size = (wvctx.endsample - currentsample + 1) * 2 * plugin.info.channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, currentsample, wvctx.endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int32_t buffer[size/2];
    int nchannels = WavpackGetNumChannels (wvctx.ctx);
    int n = WavpackUnpackSamples (wvctx.ctx, buffer, size/(2*nchannels));
    size = n * 2 * nchannels;
    // convert to int16
    int32_t *p = buffer;
    n *= nchannels;
    while (n > 0) {
        *((int16_t *)bytes) = (int16_t)(*p);
        bytes += sizeof (int16_t);
        p++;
        n--;
    }
    plugin.info.readpos = (float)(WavpackGetSampleIndex (wvctx.ctx)-wvctx.startsample)/WavpackGetSampleRate (wvctx.ctx);
    return size;
}

static int
wv_seek_sample (int sample) {
    WavpackSeekSample (wvctx.ctx, sample + wvctx.startsample);
    plugin.info.readpos = (float)(WavpackGetSampleIndex (wvctx.ctx) - wvctx.startsample) / WavpackGetSampleRate (wvctx.ctx);
    return 0;
}

static int
wv_seek (float sec) {
    return wv_seek_sample (sec * WavpackGetSampleRate (wvctx.ctx));
}

static DB_playItem_t *
wv_insert (DB_playItem_t *after, const char *fname) {
    WavpackContext *ctx = WavpackOpenFileInput (fname, NULL, 0, 0);
    if (!ctx) {
        trace ("WavpackOpenFileInput failed");
        return NULL;
    }
    int totalsamples = WavpackGetNumSamples (ctx);
    int samplerate = WavpackGetSampleRate (ctx);
    float duration = (float)totalsamples / samplerate;
    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, fname, &plugin, "wv", totalsamples, samplerate);
    if (cue_after) {
        WavpackCloseFile (ctx);
        return cue_after;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "wv";
    it->duration = duration;
    WavpackCloseFile (ctx);

    trace ("wv: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

    FILE *fp = fopen (fname, "rb");
    if (fp) {
        int apeerr = deadbeef->junk_read_ape (it, fp);
        if (!apeerr) {
            trace ("wv: ape tag found\n");
        }
        int v1err = deadbeef->junk_read_id3v1 (it, fp);
        if (!v1err) {
            trace ("wv: id3v1 tag found\n");
        }
        fclose (fp);
    }
    deadbeef->pl_add_meta (it, "title", NULL);
    after = deadbeef->pl_insert_item (after, it);

    return after;
}

static const char * exts[] = { "wv", NULL };
static const char *filetypes[] = { "wv", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "WavPack decoder",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = wv_init,
    .free = wv_free,
    .read_int16 = wv_read_int16,
//    .read_float32 = wv_read_float32,
    .seek = wv_seek,
    .seek_sample = wv_seek_sample,
    .insert = wv_insert,
    .exts = exts,
    .id = "wv",
    .filetypes = filetypes
};

DB_plugin_t *
wavpack_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
