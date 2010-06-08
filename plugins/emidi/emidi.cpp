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

#include <stdlib.h>
#include <string.h>
#include "../../deadbeef.h"
#include "emidi.h"
#include "CSMFPlay.hpp"

extern DB_decoder_t emidi_plugin;

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_functions_t *deadbeef;

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef struct {
    DB_fileinfo_t info;
    dsa::CSMFPlay *smfplay;
} emidi_info_t;

DB_fileinfo_t *
emidi_open (void) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)malloc (sizeof (emidi_info_t));
    memset (_info, 0, sizeof (emidi_info_t));
    return _info;
}

int
emidi_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    emidi_info_t *info = (emidi_info_t *)_info;
    info->smfplay = new dsa::CSMFPlay(44100, 8);
    if (!info->smfplay->Open (it->fname)) {
        return -1;
    }
    _info->plugin = &emidi_plugin;
    _info->channels = 2;
    _info->bps = 16;
    _info->samplerate = 44100;
    _info->readpos = 0;

    info->smfplay->Start ();

    return 0;
}

void
emidi_free (DB_fileinfo_t *_info) {
    emidi_info_t *info = (emidi_info_t *)_info;
    if (info) {
        if (info->smfplay) {
            delete info->smfplay;
        }
        free (info);
    }
}

int
emidi_read (DB_fileinfo_t *_info, char *bytes, int size) {
    emidi_info_t *info = (emidi_info_t *)_info;
    if (!info->smfplay->Render ((short *)bytes, size/4)) {
        return 0;
    }
    return size;
}

int
emidi_seek (DB_fileinfo_t *_info, float time) {
    return 0;
}

DB_playItem_t *
emidi_insert (DB_playItem_t *after, const char *fname) {
    DB_playItem_t *it = NULL;
    dsa::CSMFPlay *smfplay = NULL;

    smfplay = new dsa::CSMFPlay(44100, 8);
    if (!smfplay->Open (fname)) {
        fprintf (stderr, "emidi: failed to open %f\n");
        delete smfplay;
        return NULL;
    }

    it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (emidi_plugin.plugin.id);
    it->fname = strdup (fname);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->pl_set_item_duration (it, -1);
    it->filetype = "MID";
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);

    delete smfplay;
    return after;
}

int
emidi_start (void) {
    return 0;
}

int
emidi_stop (void) {
    return 0;
}

extern "C" DB_plugin_t *
emidi_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&emidi_plugin);
}

