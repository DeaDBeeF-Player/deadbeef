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
#include <stdlib.h>
#include <string.h>
#include "../../deadbeef.h"
#include "ayemu.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf (stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static const char * exts[] = { "vtx", NULL };
static const char *filetypes[] = { "VTX", NULL };

static int
vtx_init (DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure
    return 0;
}

static void
vtx_free (void) {
    // free everything allocated in _init
}

static int
vtx_read_int16 (char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF
    return 0;
}

static int
vtx_seek_sample (int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
}

static int
vtx_seek (float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    // e.g. return vtx_seek_sample (time * samplerate);
    return 0;
}

static DB_playItem_t *
vtx_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

    trace ("vtx_insert %s\n");
    // load header
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("vtx: failed to open file\n");
        return NULL;
    }
    char buf[0x4000];
    size_t sz;
    sz = deadbeef->fread (buf, 1, sizeof (buf), fp);
    deadbeef->fclose (fp);
    if (sz <= 0) {
        trace ("vtx: failed to read header data from file\n");
        return NULL;
    }
    ayemu_vtx_t *hdr = ayemu_vtx_header (buf, sz);
    if (!hdr) {
        trace ("vtx: failed to read header\n");
        return NULL;
    }

    DB_playItem_t *it = deadbeef->pl_item_alloc ();

    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = filetypes[0];

    int totalsamples = 44100*60*5;
    deadbeef->pl_set_item_duration (it, (float)totalsamples/hdr->playerFreq);

    // add metadata
    deadbeef->pl_add_meta (it, "title", hdr->title);
    deadbeef->pl_add_meta (it, "artist", hdr->author);
    deadbeef->pl_add_meta (it, "album", hdr->from);

    ayemu_vtx_free (hdr);
    after = deadbeef->pl_insert_item (after, it);
    return after;
}

static int
vtx_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    return 0;
}
static int
vtx_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    return 0;
}
// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "VTX decoder",
    .plugin.descr = "AY8910/12 chip emulator and vtx file player",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = vtx_start,
    .plugin.stop = vtx_stop,
    .init = vtx_init,
    .free = vtx_free,
    .read_int16 = vtx_read_int16,
//    .read_float32 = vtx_read_float32,
    .seek = vtx_seek,
    .seek_sample = vtx_seek_sample,
    .insert = vtx_insert,
    .exts = exts,
    .id = "vtx",
    .filetypes = filetypes
};

DB_plugin_t *
vtx_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
