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

#include <stdio.h>
#include <string.h>
#include "../../deadbeef.h"
#include "adplug.h"
#include "emuopl.h"
#include "silentopl.h"

#define trace(...) { fprintf (stderr, __VA_ARGS__); }
//#define trace(fmt,...)

extern "C" {

extern DB_decoder_t adplug_plugin;
static DB_functions_t *deadbeef;


const char *adplug_exts[] = { "A2M", "ADL", "AMD", "BAM", "CFF", "CMF", "D00", "DFM", "DMO", "DRO", "DTM", "HSC", "HSP", "IMF", "KSM", "LAA", "LDS", "M", "MAD", "MID", "MKJ", "MSC", "MTK", "RAD", "RAW", "RIX", "ROL", "S3M", "SA2", "SAT", "SCI", "SNG", "SNG", "SNG", "XAD", "XMS", "XSM", NULL };

const char *adplug_filetypes[] = { "A2M", "ADL", "AMD", "BAM", "CFF", "CMF", "D00", "DFM", "DMO", "DRO", "DTM", "HSC", "HSP", "IMF", "KSM", "LAA", "LDS", "M", "MAD", "MID", "MKJ", "MSC", "MTK", "RAD", "RAW", "RIX", "ROL", "S3M", "SA2", "SAT", "SCI", "SNG", "SNG", "SNG", "XAD", "XMS", "XSM", NULL };

int
adplug_init (DB_playItem_t *it) {
    // prepare to decode the track
    // return -1 on failure

    // fill in mandatory plugin fields
    //adplug_plugin.info.bps = deadbeef->get_output ()->bitspersample ();
    //adplug_plugin.info.channels = deadbeef->get_output ()->channels ();
    //adplug_plugin.info.samplerate = decoder->samplerate;
    //adplug_plugin.info.readpos = 0;

    return 0;
}

void
adplug_free (void) {
    // free everything allocated in _init
}

int
adplug_read_int16 (char *bytes, int size) {
    // try decode `size' bytes
    // return number of decoded bytes
    // return 0 on EOF

    //adplug_plugin.info.readpos = (float)currentsample / adplug_plugin.info.samplerate;
    return size;
}

int
adplug_seek_sample (int sample) {
    // seek to specified sample (frame)
    // return 0 on success
    // return -1 on failure
    
    // update readpos
    adplug_plugin.info.readpos = (float)sample / adplug_plugin.info.samplerate;
    return 0;
}

int
adplug_seek (float time) {
    // seek to specified time in seconds
    // return 0 on success
    // return -1 on failure
    return adplug_seek_sample (time * adplug_plugin.info.samplerate);
    return 0;
}

static const char *
adplug_get_extension (const char *fname) {
    const char *e = fname + strlen (fname);
    while (*e != '.' && e != fname) {
        e--;
    }
    if (*e == '.') {
        e++;
        // now find ext in list
    }
    return "adplug-unknown";
}

DB_playItem_t *
adplug_insert (DB_playItem_t *after, const char *fname) {
    // read information from the track
    // load/process cuesheet if exists
    // insert track into playlist
    // return track pointer on success
    // return NULL on failure

    CSilentopl opl;
    CPlayers::const_iterator i;
    CPlayer *p = CAdPlug::factory (fname, &opl, CAdPlug::players);
    if (!p) {
        trace ("adplug: failed to open %s\n", fname);
        return NULL;
    }

    int subsongs = p->getsubsongs ();
    for (int i = 0; i < subsongs; i++) {
        // prepare track for addition
        DB_playItem_t *it = deadbeef->pl_item_alloc ();
        it->decoder = &adplug_plugin;
        it->fname = strdup (fname);
        it->filetype = adplug_get_extension (fname);
        deadbeef->pl_set_item_duration (it, p->songlength (i)/1000.f);
        // add metainfo
        if (!p->gettitle().empty()) {
            deadbeef->pl_add_meta (it, "title", p->gettitle().c_str());
        }
        else if (!p->getdesc().empty()) {
            deadbeef->pl_add_meta (it, "title", p->getdesc().c_str());
        }
        else {
            deadbeef->pl_add_meta (it, "title", NULL);
        }
        deadbeef->pl_add_meta (it, "artist", p->getauthor().c_str());
        // insert
        after = deadbeef->pl_insert_item (after, it);
    }

    // free decoder
    delete p;

    // now the track is ready, insert into playlist
    return after;
}

int
adplug_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    return 0;
}

int
adplug_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    return 0;
}

DB_plugin_t *
adplug_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&adplug_plugin);
}

}
