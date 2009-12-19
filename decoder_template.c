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

// this is a decoder plugin skeleton
// use to create new decoder plugins

#include "../../deadbeef.h"

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static const char * exts[] = { "example", NULL }; // e.g. mp3
static const char *filetypes[] = { "example", NULL }; // e.g. MP3

static int
example_init (DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure

    // fill in mandatory plugin fields
    plugin.info.bps = deadbeef->get_output ()->bitspersample ();
    plugin.info.channels = deadbeef->get_output ()->channels ();
    plugin.info.samplerate = decoder->samplerate;
    plugin.info.readpos = 0;

    return 0;
}

static void
example_free (void) {
    // free everything allocated in _init
}

static int
example_read_int16 (char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF

    plugin.info.readpos = (float)currentsample / plugin.info.samplerate;
    return size;
}

static int
example_seek_sample (int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    
    // update readpos
    plugin.info.readpos = (float)sample / plugin.info.samplerate;
    return 0;
}

static int
example_seek (float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return example_seek_sample (time * plugin.info.samplerate);
    return 0;
}

static DB_playItem_t *
example_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

example:

    // open file
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("example: failed to fopen %s\n", fname);
        return NULL;
    }
    // setup decoder to use vfs
    decoder_callbacks_t cb = {
        open = vfs_open_wrapper,
        .... etc ....
    }
    decoder_info_t *di = decoder_open_callbacks (&cb);
    if (!di) {
        trace ("example: failed to init decoder\n");
        return NULL;
    }
    // read track info/tags
    track_info_t ti;
    if (decoder_read_info (&ti) < 0) {
        trace ("example: failed to read info\n");
        decoder_free (di);
        return NULL;
    }

    // now we should have track duration, and can try loading cuesheet
    // 1st try embedded cuesheet
    if (ti.embeddedcuesheet[0]) {
        DB_playItem_t *cue = deadbeef->pl_insert_cue_from_buffer (after, fname, ti.embeddedcuesheet, strlen (ti.embeddedcuesheet), &plugin, plugin.filetypes[0], ti.total_num_samples, ti.samplerate);
        if (cue) {
            // cuesheet loaded
            decoder_free (di);
            return cue;
        }
    }

    // embedded cuesheet not found, try external one
    DB_playItem_t *cue = deadbeef->pl_insert_cue (after, fname, &plugin, plugin.filetypes[0], ti.total_num_samples, ti.samplerate);
    if (cue) {
        // cuesheet loaded
        decoder_free (di);
        return cue;
    }

    // no cuesheet, prepare track for addition
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = filetypes[0];
    deadbeef->pl_set_item_duration (it, (float)ti.total_num_samples/ti.samplerate);

    // add metainfo
    if (!strlen (ti.title)) {
        // title is empty, this call will set track title to filename without extension
        deadbeef->pl_add_meta ("title", NULL);
    }
    else {
        deadbeef->pl_add_meta ("title", ti.title);
    }
    deadbeef->pl_add_meta ("artist", ti.artist);
    // ... etc ...

    // free decoder
    decoder_free (di);

    // now the track is ready, insert into playlist
    after = deadbeef->pl_insert_item (after, it);
    return after;
}

static int
example_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    return 0;
}
static int
example_stop (void) {
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
    .plugin.name = "short plugin name",
    .plugin.descr = "plugin description",
    .plugin.author = "author name",
    .plugin.email = "author email",
    .plugin.website = "author/plugin website",
    .plugin.start = example_start,
    .plugin.stop = example_stop,
    .init = example_init,
    .free = example_free,
    .read_int16 = example_read_int16,
//    .read_float32 = example_read_float32,
    .seek = example_seek,
    .seek_sample = example_seek_sample,
    .insert = example_insert,
    .exts = exts,
    .id = "example",
    .filetypes = filetypes
};

DB_plugin_t *
example_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

