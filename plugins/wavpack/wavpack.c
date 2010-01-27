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
#include <wavpack/wavpack.h>
#include <stdio.h>
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_FILE *file;
    WavpackContext *ctx;
    int startsample;
    int endsample;
} wvctx_t;

static wvctx_t wvctx;

int32_t wv_read_bytes(void *id, void *data, int32_t bcount) {
    trace ("wv_read_bytes\n");
    return deadbeef->fread (data, 1, bcount, id);
}

uint32_t wv_get_pos(void *id) {
    trace ("wv_get_pos\n");
    return deadbeef->ftell (id);
}

int wv_set_pos_abs(void *id, uint32_t pos) {
    trace ("wv_set_pos_abs\n");
    return deadbeef->fseek (id, pos, SEEK_SET);
}
int wv_set_pos_rel(void *id, int32_t delta, int mode) {
    trace ("wv_set_pos_rel\n");
    return deadbeef->fseek (id, delta, SEEK_CUR);
}
int wv_push_back_byte(void *id, int c) {
    trace ("wv_push_back_byte\n");
    deadbeef->fseek (id, -1, SEEK_CUR);
    return deadbeef->ftell (id);
}
uint32_t wv_get_length(void *id) {
    trace ("wv_get_length\n");
    size_t pos = deadbeef->ftell (id);
    deadbeef->fseek (id, 0, SEEK_END);
    size_t sz = deadbeef->ftell (id);
    deadbeef->fseek (id, pos, SEEK_SET);
    return sz;
}
int wv_can_seek(void *id) {
    trace ("wv_can_seek\n");
    return 1;
}

int32_t wv_write_bytes (void *id, void *data, int32_t bcount) {
    return 0;
}

static WavpackStreamReader wsr = {
    .read_bytes = wv_read_bytes,
    .get_pos = wv_get_pos,
    .set_pos_abs = wv_set_pos_abs,
    .set_pos_rel = wv_set_pos_rel,
    .push_back_byte = wv_push_back_byte,
    .get_length = wv_get_length,
    .can_seek = wv_can_seek,
    .write_bytes = wv_write_bytes
};

static int
wv_init (DB_playItem_t *it) {
    memset (&wvctx, 0, sizeof (wvctx));

    wvctx.file = deadbeef->fopen (it->fname);
    if (!wvctx.file) {
        return -1;
    }
    wvctx.ctx = WavpackOpenFileInputEx (&wsr, wvctx.file, NULL, NULL, OPEN_2CH_MAX/*|OPEN_WVC*/, 0);
    if (!wvctx.ctx) {
        plugin.free ();
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
    if (wvctx.file) {
        deadbeef->fclose (wvctx.file);
        wvctx.file = NULL;
    }
    if (wvctx.ctx) {
        WavpackCloseFile (wvctx.ctx);
        wvctx.ctx = NULL;
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
        *((int16_t *)bytes) = (int16_t)((*p) >> (plugin.info.bps-16));
        bytes += sizeof (int16_t);
        p++;
        n--;
    }
    plugin.info.readpos = (float)(WavpackGetSampleIndex (wvctx.ctx)-wvctx.startsample)/WavpackGetSampleRate (wvctx.ctx);

    deadbeef->streamer_set_bitrate (WavpackGetInstantBitrate (wvctx.ctx) / 1000);

    return size;
}

static int
wv_read_float32 (char *bytes, int size) {
    int currentsample = WavpackGetSampleIndex (wvctx.ctx);
    if (size / (4 * plugin.info.channels) + currentsample > wvctx.endsample) {
        size = (wvctx.endsample - currentsample + 1) * 4 * plugin.info.channels;
        trace ("wv: size truncated to %d bytes, cursample=%d, endsample=%d\n", size, currentsample, wvctx.endsample);
        if (size <= 0) {
            return 0;
        }
    }
    int32_t buffer[size/4];
    int nchannels = WavpackGetNumChannels (wvctx.ctx);
    int n = WavpackUnpackSamples (wvctx.ctx, buffer, size/(4*nchannels));
    size = n * 4 * nchannels;
    // convert to int16
    int32_t *p = buffer;
    n *= nchannels;
    float mul = 1.f/ ((1 << (plugin.info.bps-1))-1);
    while (n > 0) {
        *((float *)bytes) = (*p) * mul;
        bytes += sizeof (float);
        p++;
        n--;
    }
    plugin.info.readpos = (float)(WavpackGetSampleIndex (wvctx.ctx)-wvctx.startsample)/WavpackGetSampleRate (wvctx.ctx);
    deadbeef->streamer_set_bitrate (WavpackGetInstantBitrate (wvctx.ctx) / 1000);
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

    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    WavpackContext *ctx = WavpackOpenFileInputEx (&wsr, fp, NULL, NULL, 0, 0);
    if (!ctx) {
        trace ("WavpackOpenFileInput failed");
        deadbeef->fclose (fp);
        return NULL;
    }
    int totalsamples = WavpackGetNumSamples (ctx);
    int samplerate = WavpackGetSampleRate (ctx);
    WavpackCloseFile (ctx);
    float duration = (float)totalsamples / samplerate;

    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = "wv";
    deadbeef->pl_set_item_duration (it, duration);

    trace ("wv: totalsamples=%d, samplerate=%d, duration=%f\n", totalsamples, samplerate, duration);

    int apeerr = deadbeef->junk_read_ape (it, fp);
    if (!apeerr) {
        trace ("wv: ape tag found\n");
    }
    int v1err = deadbeef->junk_read_id3v1 (it, fp);
    if (!v1err) {
        trace ("wv: id3v1 tag found\n");
    }
    deadbeef->fclose (fp);

    DB_playItem_t *cue_after = deadbeef->pl_insert_cue (after, it, totalsamples, samplerate);
    if (cue_after) {
        return cue_after;
    }

    // embedded cue
    const char *cuesheet = deadbeef->pl_find_meta (it, "cuesheet");
    if (cuesheet) {
        DB_playItem_t *last = deadbeef->pl_insert_cue_from_buffer (after, it, cuesheet, strlen (cuesheet), totalsamples, samplerate);
        if (last) {
            deadbeef->pl_item_free (it);
            return last;
        }
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
    .plugin.descr = ".wv player using libwavpack",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = wv_init,
    .free = wv_free,
    .read_int16 = wv_read_int16,
    .read_float32 = wv_read_float32,
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
