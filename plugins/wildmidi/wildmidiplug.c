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
#include "wildmidi_lib.h"

extern DB_decoder_t wmidi_plugin;

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_functions_t *deadbeef;

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef struct {
    DB_fileinfo_t info;
    midi *m;
} wmidi_info_t;

DB_fileinfo_t *
wmidi_open (void) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)malloc (sizeof (wmidi_info_t));
    memset (_info, 0, sizeof (wmidi_info_t));
    return _info;
}

int
wmidi_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    wmidi_info_t *info = (wmidi_info_t *)_info;

    info->m = WildMidi_Open (it->fname);
    if (!info->m) {
        fprintf (stderr, "wmidi: failed to open %s\n", it->fname);
        return -1;
    }

    _info->plugin = &wmidi_plugin;
    _info->channels = 2;
    _info->bps = 16;
    _info->samplerate = 44100;
    _info->readpos = 0;

    return 0;
}

void
wmidi_free (DB_fileinfo_t *_info) {
    wmidi_info_t *info = (wmidi_info_t *)_info;
    if (info) {
        if (info->m) {
            WildMidi_Close (info->m);
            info->m = NULL;
        }
        free (info);
    }
}

int
wmidi_read (DB_fileinfo_t *_info, char *bytes, int size) {
    wmidi_info_t *info = (wmidi_info_t *)_info;
    int bufferused = WildMidi_GetOutput (info->m, (char *)bytes, size);
    if (bufferused < 0) {
        fprintf (stderr, "WildMidi_GetOutput returned %d\n", bufferused);
        return 0;
    }

    return bufferused;
}

int
wmidi_seek_sample (DB_fileinfo_t *_info, int sample) {
    wmidi_info_t *info = (wmidi_info_t *)_info;
    unsigned long int s = sample;
    WildMidi_SampledSeek (info->m, &s);
    _info->readpos = s/44100.0f;
    return 0;
}

int
wmidi_seek (DB_fileinfo_t *_info, float time) {
    return wmidi_seek_sample (_info, time * 44100);
}

DB_playItem_t *
wmidi_insert (DB_playItem_t *after, const char *fname) {
    DB_playItem_t *it = NULL;

    midi *m = WildMidi_Open (fname);
    if (!m) {
        fprintf (stderr, "wmidi: failed to open %s\n", fname);
        return NULL;
    }

    struct _WM_Info *inf = WildMidi_GetInfo (m);
    it = deadbeef->pl_item_alloc ();
    it->decoder_id = deadbeef->plug_get_decoder_id (wmidi_plugin.plugin.id);
    it->fname = strdup (fname);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->pl_set_item_duration (it, inf->approx_total_samples / 44100.f);
    it->filetype = "MID";
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    WildMidi_Close (m);
    return after;
}

#define DEFAULT_TIMIDITY_CONFIG "/etc/timidity++/timidity-freepats.cfg"

int
wmidi_start (void) {
    const char *config_file = deadbeef->conf_get_str ("wildmidi.config", DEFAULT_TIMIDITY_CONFIG);
    WildMidi_Init (config_file, 44100, 0);
    return 0;
}

int
wmidi_stop (void) {
    WildMidi_Shutdown ();
    return 0;
}

DB_plugin_t *
wildmidi_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&wmidi_plugin);
}

static const char *exts[] = { "mid",NULL };
const char *filetypes[] = { "MID", NULL };

static const char settings_dlg[] =
    "property \"Timidity++ bank configuration file\" file wildmidi.config \"" DEFAULT_TIMIDITY_CONFIG "\";\n"
;
// define plugin interface
DB_decoder_t wmidi_plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.name = "WildMidi player",
    .plugin.descr = "MIDI player based on WildMidi library",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = wmidi_start,
    .plugin.stop = wmidi_stop,
    .plugin.id = "wmidi",
    .plugin.configdialog = settings_dlg,
    .open = wmidi_open,
    .init = wmidi_init,
    .free = wmidi_free,
    .read_int16 = wmidi_read,
    .seek = wmidi_seek,
    .seek_sample = wmidi_seek_sample,
    .insert = wmidi_insert,
    .exts = exts,
    .filetypes = filetypes,
};
