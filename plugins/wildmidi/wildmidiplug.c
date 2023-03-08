/*
    WildMidi plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

    Based on wildmidi library
 	Midi Wavetable Processing library
 	Copyright (C)2001-2004 Chris Ison

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/



#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>
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
    wmidi_info_t *info = calloc (1, sizeof (wmidi_info_t));
    return &info->info;
}

static int
wmidi_init_conf (void);

int
wmidi_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    if (wmidi_init_conf () < 0) {
        return -1;
    }

    wmidi_info_t *info = (wmidi_info_t *)_info;

    deadbeef->pl_lock ();
    const char *uri = strdupa (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    info->m = WildMidi_Open (uri);
    if (!info->m) {
        trace ("wmidi: failed to open %s\n", uri);
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
wmidi_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    if (wmidi_init_conf () < 0) {
        return NULL;
    }

    DB_playItem_t *it = NULL;

    midi *m = WildMidi_Open (fname);
    if (!m) {
        trace ("wmidi: failed to open %s\n", fname);
        return NULL;
    }

    struct _WM_Info *inf = WildMidi_GetInfo (m);
    it = deadbeef->pl_item_alloc_init (fname, wmidi_plugin.plugin.id);
    deadbeef->pl_add_meta (it, "title", NULL);
    deadbeef->plt_set_item_duration (plt, it, inf->approx_total_samples / 44100.f);
    deadbeef->pl_add_meta (it, ":FILETYPE", "MID");
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    WildMidi_Close (m);
    return after;
}

#define DEFAULT_TIMIDITY_CONFIG "/etc/timidity++/timidity-freepats.cfg:/etc/timidity/freepats.cfg:/etc/timidity/freepats/freepats.cfg"

extern int WM_Initialized;

static int
wmidi_init_conf (void) {
    if (WM_Initialized) {
        return 0;
    }
    char config_files[1000];
    deadbeef->conf_get_str ("wildmidi.config", DEFAULT_TIMIDITY_CONFIG, config_files, sizeof (config_files));
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
            *config = 0;
        }
        p = e;
    }
    if (*config) {
        WildMidi_Init (config, 44100, 0);
    }
    else {
        fprintf (stderr, _("wildmidi: freepats config file not found. Please install timidity-freepats package, or specify path to freepats.cfg in the plugin settings."));
        return -1;
    }
    return 0;
}

int
wmidi_start (void) {
    return 0;
}

int
wmidi_stop (void) {
    if (WM_Initialized) {
        WildMidi_Shutdown ();
    }
    return 0;
}

DB_plugin_t *
wildmidi_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&wmidi_plugin);
}

static const char *exts[] = { "mid","midi",NULL };

static const char settings_dlg[] =
    "property \"Timidity++ bank configuration file\" file wildmidi.config \"" DEFAULT_TIMIDITY_CONFIG "\";\n"
;
// define plugin interface
DB_decoder_t wmidi_plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.name = "WildMidi player",
    .plugin.descr = "MIDI player based on WildMidi library\n\nRequires freepats package to be installed\nSee http://freepats.zenvoid.org/\nMake sure to set correct freepats.cfg path in plugin settings.",
    .plugin.copyright = 
        "WildMidi plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "Based on wildmidi library\n"
        "Midi Wavetable Processing library\n"
        "Copyright (C)2001-2004 Chris Ison\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
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
};
