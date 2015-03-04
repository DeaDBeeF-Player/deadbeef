/*
    sc86 (atari st & amiga) input plugin for deadbeef
    Copyright (C) 2015 Alexey Yakovenko

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

// TODO: the sc68 emulator lib is not reentrant, needs fixing, otherwise playing + adding tracks at the same time would crash

#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "../../deadbeef.h"
#include "api68/api68.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;
static api68_init_t init68;
static api68_t *sc68;

typedef struct {
    DB_fileinfo_t info;
    int currentsample;
} in_sc68_info_t;

static const char * exts[] = { "sndh", NULL };

static void *
intmalloc (unsigned int size) {
    return malloc (size);
}

// allocate codec control structure
static DB_fileinfo_t *
in_sc68_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (in_sc68_info_t));
    in_sc68_info_t *info = (in_sc68_info_t *)_info;
    memset (info, 0, sizeof (in_sc68_info_t));
    return _info;
}

// prepare to decode the track, fill in mandatory plugin fields
// return -1 on failure
static int
in_sc68_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    in_sc68_info_t *info = (in_sc68_info_t *)_info;

    // Load an sc68 file.
    deadbeef->pl_lock ();
    const char *fname = deadbeef->pl_find_meta (it, ":URI");
    int res = api68_load_file(sc68, fname);
    deadbeef->pl_unlock ();

    if (res) {
        return -1;
    }

    int tr = deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0);
    api68_music_info_t ti;
    res = api68_music_info (sc68, &ti, tr+1, 0);
    if (res < 0) {
        return -1;
    }

    int samplerate = deadbeef->conf_get_int ("c68.samplerate", 44100);

    _info->plugin = &plugin;
    _info->fmt.bps = 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = samplerate;
    _info->fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT;
    _info->readpos = 0;

    api68_play(sc68, tr+1);
    return 0;
}

// free everything allocated in _init
static void
in_sc68_free (DB_fileinfo_t *_info) {
    in_sc68_info_t *info = (in_sc68_info_t *)_info;
    if (info) {
        if (sc68) {
            api68_stop (sc68);
        }
        free (info);
    }
}


// try decode `size' bytes
// return number of decoded bytes
// or 0 on EOF/error
static int
in_sc68_read (DB_fileinfo_t *_info, char *bytes, int size) {
    in_sc68_info_t *info = (in_sc68_info_t *)_info;
    info->currentsample += size / (_info->fmt.channels * _info->fmt.bps/8);
    int res = api68_process(sc68, bytes, size>>2);
    if (res & API68_END) {
        return 0;
    }
    return size;
}

// seek to specified sample (frame)
// return 0 on success
// return -1 on failure
static int
in_sc68_seek_sample (DB_fileinfo_t *_info, int sample) {
    in_sc68_info_t *info = (in_sc68_info_t *)_info;

    info->currentsample = sample;
    _info->readpos = (float)sample / _info->fmt.samplerate;
    return 0;
}

// seek to specified time in seconds
// return 0 on success
// return -1 on failure
static int
in_sc68_seek (DB_fileinfo_t *_info, float time) {
    return in_sc68_seek_sample (_info, time * _info->fmt.samplerate);
}

// read information from the track
// load/process cuesheet if exists
// insert track into playlist
// return track pointer on success
// return NULL on failure

static DB_playItem_t *
in_sc68_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
#if 0
    // open file
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("in_sc68: failed to fopen %s\n", fname);
        return NULL;
    }
#endif

    // Load an sc68 file.
    if (api68_load_file(sc68, fname)) {
        return NULL;
    }

    // replace "in_sc68" with your file type (e.g. MP3, WAV, etc)
    const char *ft = "sc68";

    api68_music_info_t di;
    int err = api68_music_info (sc68, &di, 0, 0);
    if (err < 0) {
        return NULL;
    }

    int samplerate = deadbeef->conf_get_int ("c68.samplerate", 44100);
    for (int tr = 0; tr < di.tracks; tr++) {
        api68_music_info_t ti;
        int err = api68_music_info (sc68, &ti, tr+1, 0);
        if (err < 0) {
            continue;
        }

        int totalsamples;
        if (ti.time_ms > 0) {
            totalsamples = ti.time_ms * samplerate / 1000;
        }
        else {
            totalsamples = deadbeef->conf_get_float ("c68.songlength", 2) * 60 * samplerate;
        }

        DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);

        deadbeef->pl_replace_meta (it, ":FILETYPE", ft);
        deadbeef->plt_set_item_duration (plt, it, (float)totalsamples/samplerate);

        // add metainfo
        if (!ti.title || !ti.title[0]) {
            // title is empty, this call will set track title to filename without extension
            deadbeef->pl_add_meta (it, "title", NULL);
        }
        else {
            deadbeef->pl_add_meta (it, "title", ti.title);
        }

        if (ti.author && ti.author[0]) {
            deadbeef->pl_add_meta (it, "artist", ti.author);
        }
        if (ti.composer && ti.composer[0]) {
            deadbeef->pl_add_meta (it, "composer", ti.composer);
        }

        if (di.tracks > 0) {
            deadbeef->pl_set_item_flags (it, deadbeef->pl_get_item_flags (it) | DDB_IS_SUBTRACK);
        }
        deadbeef->pl_set_meta_int (it, ":TRACKNUM", tr);

        // now the track is ready, insert into playlist
        after = deadbeef->plt_insert_item (plt, after, it);
        deadbeef->pl_item_unref (it);
    }

    return after;
}

static int
in_sc68_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    // Clean up init structure (required).

    char datadir[PATH_MAX];
    snprintf (datadir, sizeof (datadir), "%s/sc68data", deadbeef->get_plugin_dir ());
    setenv ("SC68_DATA", datadir, 1);

    // Clean up init structure (required).
    memset(&init68, 0, sizeof(init68));
    // Set dynamic handler (required).
    init68.alloc = intmalloc;
    init68.free = free;
    // Set debug message handler (optionnal).
    init68.debug = (debugmsg68_t)vfprintf;
    init68.debug_cookie = stderr;
    sc68 = api68_init(&init68);
    if (!sc68) {
        api68_shutdown (sc68);
        return -1;
    }

    return 0;
}

static int
in_sc68_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    if (sc68) {
        api68_shutdown (sc68);
    }
    return 0;
}

static const char settings_dlg[] =
"property \"Default song length (in minutes)\" entry c68.songlength 2;\n" // 0..1440
"property \"Samplerate\" entry c68.samplerate 44100;\n" // 6000-50000
"property \"Skip when shorter than (sec)\" entry c68.skip_time 4;\n" // 4..86400
;

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "C68 player (Atari ST and Amiga)",
    .plugin.id = "in_sc68",
    .plugin.descr = "C68 player (Atari ST and Amiga)",
    .plugin.copyright = "copyright message - author(s), license, etc",
    .plugin.start = in_sc68_start,
    .plugin.stop = in_sc68_stop,
    .plugin.configdialog = settings_dlg,
    .open = in_sc68_open,
    .init = in_sc68_init,
    .free = in_sc68_free,
    .read = in_sc68_read,
    .seek = in_sc68_seek,
    .seek_sample = in_sc68_seek_sample,
    .insert = in_sc68_insert,
    .exts = exts,
};

DB_plugin_t *
in_sc68_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

