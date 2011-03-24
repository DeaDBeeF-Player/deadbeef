/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include "../../gettext.h"

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
wmidi_open (uint32_t hints) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)malloc (sizeof (wmidi_info_t));
    memset (_info, 0, sizeof (wmidi_info_t));
    return _info;
}

int
wmidi_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    wmidi_info_t *info = (wmidi_info_t *)_info;

    info->m = WildMidi_Open (deadbeef->pl_find_meta (it, ":URI"));
    if (!info->m) {
        trace ("wmidi: failed to open %s\n", deadbeef->pl_find_meta (it, ":URI"));
        return -1;
    }

    _info->plugin = &wmidi_plugin;
    _info->fmt.channels = 2;
    _info->fmt.bps = 16;
    _info->fmt.samplerate = 44100;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
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
        trace ("WildMidi_GetOutput returned %d\n", bufferused);
        return 0;
    }

    return bufferused;
}

int
wmidi_seek_sample (DB_fileinfo_t *_info, int sample) {
    wmidi_info_t *info = (wmidi_info_t *)_info;
    unsigned long int s = sample;
    WildMidi_SampledSeek (info->m, &s);
    _info->readpos = s/(float)_info->fmt.samplerate;
    return 0;
}

int
wmidi_seek (DB_fileinfo_t *_info, float time) {
    return wmidi_seek_sample (_info, time * _info->fmt.samplerate);
}

DB_playItem_t *
wmidi_insert (DB_playItem_t *after, const char *fname) {
    DB_playItem_t *it = NULL;

    midi *m = WildMidi_Open (fname);
    if (!m) {
        trace ("wmidi: failed to open %s\n", fname);
        return NULL;
    }

    struct _WM_Info *inf = WildMidi_GetInfo (m);
    it = deadbeef->pl_item_alloc_init (fname, wmidi_plugin.plugin.id);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->pl_set_item_duration (it, inf->approx_total_samples / 44100.f);
    deadbeef->pl_add_meta (it, ":FILETYPE", "MID");
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    WildMidi_Close (m);
    return after;
}

#define DEFAULT_TIMIDITY_CONFIG "/etc/timidity++/timidity-freepats.cfg:/etc/timidity/freepats.cfg:/etc/timidity/freepats/freepats.cfg"

int
wmidi_start (void) {
    const char *config_files = deadbeef->conf_get_str ("wildmidi.config", DEFAULT_TIMIDITY_CONFIG);
    char config[1024] = "";
    const char *p = config_files;
    while (p) {
        *config = 0;
        char *e = strchr (p, ':');
        if (e) {
            strncpy (config, p, e-p);
            config[e-p] = 0;
            e++;
        }
        else {
            strcpy (config, p);
        }
        if (*config) {
            FILE *f = fopen (config, "rb");
            if (f) {
                fclose (f);
                break;
            }
        }
        p = e;
    }
    if (*config) {
        WildMidi_Init (config, 44100, 0);
    }
    else {
        fprintf (stderr, _("wildmidi: freepats config file not found. Please install timidity-freepats package, or specify path to freepats.cfg in the plugin settings."));
    }
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
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.name = "WildMidi player",
    .plugin.descr = "MIDI player based on WildMidi library",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses WildMidi v0.2.2, (C) 2001-2004 Chris Ison"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = wmidi_start,
    .plugin.stop = wmidi_stop,
    .plugin.id = "wmidi",
    .plugin.configdialog = settings_dlg,
    .open = wmidi_open,
    .init = wmidi_init,
    .free = wmidi_free,
    .read = wmidi_read,
    .seek = wmidi_seek,
    .seek_sample = wmidi_seek_sample,
    .insert = wmidi_insert,
    .exts = exts,
    .filetypes = filetypes,
};
