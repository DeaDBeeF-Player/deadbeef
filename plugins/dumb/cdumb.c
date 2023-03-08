/*
    DUMB Plugin for DeaDBeeF Player
    Copyright (C) 2009-2016 Oleksiy Yakovenko <waker@users.sourceforge.net>

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

// based on fb2k dumb plugin from http://kode54.foobar2000.org

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdumb.h"
#include "dumb.h"
#include "internal/it.h"
#include "modloader.h"
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    DUH *duh;
    DUH_SIGRENDERER *renderer;
    int chip_voices;
    int can_loop;
    int rawsignal;
} dumb_info_t;

//#define DUMB_RQ_ALIASING
//#define DUMB_RQ_LINEAR
//#define DUMB_RQ_CUBIC
//#define DUMB_RQ_N_LEVELS
extern int dumb_resampling_quality;
extern int dumb_it_max_to_mix;

static int conf_bps = 16;
static int conf_samplerate = 44100;
static int conf_resampling_quality = 4;
static int conf_ramping_style = 2;
static int conf_global_volume = 64;
static int conf_play_forever = 0;

static int
cdumb_startrenderer (DB_fileinfo_t *_info);

static DB_fileinfo_t *
cdumb_open (uint32_t hints) {
    dumb_info_t *info = calloc (1, sizeof (dumb_info_t));
    info->can_loop = hints & DDB_DECODER_HINT_CAN_LOOP;
    info->rawsignal = hints & DDB_DECODER_HINT_RAW_SIGNAL;
    info->chip_voices = 0xff;
    return &info->info;
}

static int
cdumb_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    trace ("cdumb_init %s\n", deadbeef->pl_find_meta (it, ":URI"));
    dumb_info_t *info = (dumb_info_t *)_info;

    int is_dos, is_it, is_ptcompat;
    deadbeef->pl_lock ();
    const char *uri = strdupa(deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    const char *ext = uri + strlen (uri) - 1;
    while (*ext != '.' && ext > uri) {
        ext--;
    }
    ext++;
    const char *ftype;
    info->duh = g_open_module (uri, &is_it, &is_dos, &is_ptcompat, 0, &ftype);

    dumb_it_do_initial_runthrough (info->duh);

    _info->plugin = &plugin;
    _info->fmt.bps = conf_bps;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = conf_samplerate;
    _info->readpos = 0;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);

    if (cdumb_startrenderer (_info) < 0) {
        return -1;
    }

    trace ("cdumb_init success (ptr=%p)\n", _info);
    return 0;
}

static int
cdumb_startrenderer (DB_fileinfo_t *_info) {
    dumb_info_t *info = (dumb_info_t *)_info;
    // reopen
    if (info->renderer) {
        duh_end_sigrenderer (info->renderer);
        info->renderer = NULL;
    }
    info->renderer = duh_start_sigrenderer (info->duh, 0, 2, 0);
    if (!info->renderer) {
        return -1;
    }

    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer (info->renderer);
    dumb_it_set_loop_callback (itsr, &dumb_it_callback_terminate, NULL);

    int q = conf_resampling_quality;
    if (q < 0) {
        q = 0;
    }
    else if (q >= DUMB_RQ_N_LEVELS) {
        q = DUMB_RQ_N_LEVELS - 1;
    }

    dumb_it_set_resampling_quality (itsr, q);
    dumb_it_set_xm_speed_zero_callback (itsr, &dumb_it_callback_terminate, NULL);
    dumb_it_set_global_volume_zero_callback (itsr, &dumb_it_callback_terminate, NULL);
    
    int rq = conf_ramping_style;
    if (rq < 0) {
        rq = 0;
    }
    else if (rq > 2) {
        rq = 2;
    }
    dumb_it_set_ramp_style(itsr, rq);

    dumb_it_sr_set_global_volume (itsr, conf_global_volume);
    return 0;
}

static void
cdumb_free (DB_fileinfo_t *_info) {
    trace ("cdumb_free %p\n", _info);
    dumb_info_t *info = (dumb_info_t *)_info;
    if (info) {
        if (info->renderer) {
            duh_end_sigrenderer (info->renderer);
            info->renderer = NULL;
        }
        if (info->duh) {
            unload_duh (info->duh);
            info->duh = NULL;
        }
        free (info);
    }
}

static int
cdumb_it_callback_loop_forever(void *unused)
{
    (void)unused;
    return 0;
}

static int
cdumb_read (DB_fileinfo_t *_info, char *bytes, int size) {
    trace ("cdumb_read req %d\n", size);
    dumb_info_t *info = (dumb_info_t *)_info;
    int samplesize = (_info->fmt.bps >> 3) * _info->fmt.channels;
    int length = size / samplesize;
    long ret;

    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer (info->renderer);

    if (!info->rawsignal) {
        int chip_voices = deadbeef->conf_get_int ("chip.voices", 0xff);
        if (chip_voices != info->chip_voices) {
            info->chip_voices = chip_voices;

            for (int i = 0; i < 8; i++) {
                dumb_it_sr_set_channel_muted(itsr, i, (chip_voices&(1<<i)) == 0);
            }
        }
    }

    if (conf_play_forever && info->can_loop)
        dumb_it_set_loop_callback (itsr, &cdumb_it_callback_loop_forever, NULL);
    else
        dumb_it_set_loop_callback (itsr, &dumb_it_callback_terminate, NULL);

    ret = duh_render (info->renderer, _info->fmt.bps, 0, 1, 65536.f / _info->fmt.samplerate, length, bytes);
    _info->readpos += ret / (float)_info->fmt.samplerate;
    trace ("cdumb_read %d\n", ret*samplesize);
    return (int)(ret*samplesize);
}

static int
cdumb_seek (DB_fileinfo_t *_info, float time) {
    trace ("cdumb_read seek %f\n", time);
    dumb_info_t *info = (dumb_info_t *)_info;
    float skiptime = time;
    if (skiptime < _info->readpos) {
        if (cdumb_startrenderer (_info) < 0) {
            return -1;
        }       
    }
    else {
        skiptime -= _info->readpos;
    }
    int pos = skiptime * _info->fmt.samplerate;
    duh_sigrenderer_generate_samples (info->renderer, 0, 65536.0f / _info->fmt.samplerate, pos, NULL);
    _info->readpos = time;
    return 0;
}

enum { MOD_EXT_COUNT = 6 };

static const char * exts[]=
{
    "mod","mdz","stk","m15","fst","oct","nt",
    "s3m","s3z",
    "stm","stz",
    "it","itz",
    "xm","xmz",
    "ptm","ptz",
    "mtm","mtz",
    "669",
    "psm",
    "umx",
    "am","j2b",
    "dsm",
    "amf",
    "okt","okta",
    "mo3",
    NULL
};

int
cdumb_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        conf_bps = deadbeef->conf_get_int ("dumb.8bitoutput", 0) ? 8 : 16;
        conf_samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);
        conf_resampling_quality = deadbeef->conf_get_int ("dumb.resampling_quality", 4);
        conf_ramping_style = deadbeef->conf_get_int ("dumb.volume_ramping", 2);
        conf_global_volume = deadbeef->conf_get_int ("dumb.globalvolume", 64);
        conf_play_forever = deadbeef->streamer_get_repeat () == DDB_REPEAT_SINGLE;
        break;
    }
    return 0;
}

static const char *
convstr (const char* str, int sz, char *out, int out_sz) {
    int i;
    for (i = 0; i < sz; i++) {
        if (str[i] != ' ') {
            break;
        }
    }
    if (i == sz) {
        out[0] = 0;
        return out;
    }

    const char *cs = deadbeef->junk_detect_charset (str);
    if (!cs) {
        return str;
    }
    else {
        if (deadbeef->junk_iconv (str, sz, out, out_sz, cs, "utf-8") >= 0) {
            return out;
        }
    }

    trace ("cdumb: failed to detect charset\n");
    return NULL;
}

static void
read_metadata_internal (DB_playItem_t *it, DUMB_IT_SIGDATA *itsd) {
    char temp[2048];

    if (itsd->name[0])     {
        int tl = sizeof(itsd->name);
        int i;
        for (i = 0; i < tl && itsd->name[i] && itsd->name[i] == ' '; i++);
        if (i == tl || !itsd->name[i]) {
            deadbeef->pl_add_meta (it, "title", NULL);
        }
        else {
            deadbeef->pl_add_meta (it, "title", convstr ((char*)&itsd->name, sizeof(itsd->name), temp, sizeof (temp)));
        }
    }
    else {
        deadbeef->pl_add_meta (it, "title", NULL);
    }
    int i;
    for (i = 0; i < itsd->n_instruments; i++) {
        char key[100];
        snprintf (key, sizeof (key), "INST%03d", i);
        deadbeef->pl_add_meta (it, key, convstr ((char *)&itsd->instrument[i].name, sizeof (itsd->instrument[i].name), temp, sizeof (temp)));
    }
    for (i = 0; i < itsd->n_samples; i++) {
        char key[100];
        snprintf (key, sizeof (key), "SAMP%03d", i);
        deadbeef->pl_add_meta (it, key, convstr ((char *)&itsd->sample[i].name, sizeof (itsd->sample[i].name), temp, sizeof (temp)));
    }

    char s[100];

    snprintf (s, sizeof (s), "%d", itsd->n_orders);
    deadbeef->pl_add_meta (it, ":MOD_ORDERS", s);
    snprintf (s, sizeof (s), "%d", itsd->n_instruments);
    deadbeef->pl_add_meta (it, ":MOD_INSTRUMENTS", s);
    snprintf (s, sizeof (s), "%d", itsd->n_samples);
    deadbeef->pl_add_meta (it, ":MOD_SAMPLES", s);
    snprintf (s, sizeof (s), "%d", itsd->n_patterns);
    deadbeef->pl_add_meta (it, ":MOD_PATTERNS", s);
    snprintf (s, sizeof (s), "%d", itsd->n_pchannels);
    deadbeef->pl_add_meta (it, ":MOD_CHANNELS", s);
}

static int
cdumb_read_metadata (DB_playItem_t *it) {
    DUH* duh = NULL;
    int is_it;
    int is_dos;
    int is_ptcompat;

    deadbeef->pl_lock ();
    const char *fname = strdupa(deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    const char *ftype;
    duh = g_open_module(fname, &is_it, &is_dos, &is_ptcompat, 0, &ftype);

    if (!duh) {
        unload_duh (duh);
        return -1;
    }
    DUMB_IT_SIGDATA * itsd = duh_get_it_sigdata(duh);

    deadbeef->pl_delete_all_meta (it);
    read_metadata_internal (it, itsd);
    unload_duh (duh);
    return 0;
}

static DB_playItem_t *
cdumb_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }
    else {
        ext = "";
    }
    int is_it;
    int is_dos;
    int is_ptcompat;
    const char *ftype = NULL;
    DUH* duh = g_open_module(fname, &is_it, &is_dos, &is_ptcompat, 0, &ftype);
    if (!duh) {
        return NULL;
    }
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    DUMB_IT_SIGDATA * itsd = duh_get_it_sigdata(duh);

    read_metadata_internal (it, itsd);

    dumb_it_do_initial_runthrough (duh);
    deadbeef->plt_set_item_duration (plt, it, duh_get_length (duh)/65536.0f);
    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
//    printf ("duration: %f\n", _info->duration);
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    unload_duh (duh);

    return after;
}

static DUMBFILE_SYSTEM dumb_vfs;

static void *
dumb_vfs_open (const char *fname) {
    ddb_dumb_file_t *f = calloc (1, sizeof (ddb_dumb_file_t));
    f->file = deadbeef->fopen (fname);
    return f;
}

static int
dumb_vfs_skip (void *f, long n) {
    ddb_dumb_file_t *d = f;
    return deadbeef->fseek (d->file, n, SEEK_CUR);
}

static int
dumb_vfs_getc (void *f) {
    ddb_dumb_file_t *d = f;
    uint8_t c;
    if (1 != deadbeef->fread (&c, 1, 1, d->file)) {
        return -1;
    }
    return (int)c;
}

static long
dumb_vfs_getnc (char *ptr, long n, void *f) {
    ddb_dumb_file_t *d = f;
    return deadbeef->fread (ptr, 1, n, d->file);
}

static int
dumb_vfs_seek (void *f, long n) {
    ddb_dumb_file_t *d = f;
    return deadbeef->fseek(d->file, d->startoffs + n, SEEK_SET);
}

static long
dumb_vfs_get_size (void *f) {
    ddb_dumb_file_t *d = f;
    return deadbeef->fgetlength(d->file) - d->startoffs;
}

static void
dumb_vfs_close (void *f) {
    ddb_dumb_file_t *d = f;
    deadbeef->fclose (d->file);
    free (d);
}

static void
dumb_register_db_vfs (void) {
    dumb_vfs.open = dumb_vfs_open;
    dumb_vfs.skip = dumb_vfs_skip;
    dumb_vfs.getc = dumb_vfs_getc;
    dumb_vfs.getnc = dumb_vfs_getnc;
    dumb_vfs.seek = dumb_vfs_seek;
    dumb_vfs.get_size = dumb_vfs_get_size;
    dumb_vfs.close = dumb_vfs_close;
    register_dumbfile_system (&dumb_vfs);
}

int
cdumb_start (void) {
    dumb_register_db_vfs ();
    return 0;
}

int
cdumb_stop (void) {
    dumb_exit ();
    return 0;
}

static const char settings_dlg[] =
    "property \"Resampling quality\" spinbtn[0,5,1] dumb.resampling_quality 4;\n"
    "property \"Internal DUMB volume\" spinbtn[0,128,16] dumb.globalvolume 64;\n"
    "property \"Volume ramping\" select[3] dumb.volume_ramping 0 None \"On/Off Only\" \"Full\";\n"
    "property \"8-bit output\" checkbox dumb.8bitoutput 0;\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stddumb",
    .plugin.name = "DUMB module player",
    .plugin.descr = "module player based on DUMB library",
    .plugin.copyright = 
        "DUMB Plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2016 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses a fork of DUMB (Dynamic Universal Music Bibliotheque), Version 0.9.3\n"
        "Copyright (C) 2001-2005 Ben Davis, Robert J Ohannessian and Julien Cugniere\n"
        "Uses code from kode54's foobar2000 plugin, http://kode54.foobar2000.org/\n"
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
    .plugin.start = cdumb_start,
    .plugin.stop = cdumb_stop,
    .plugin.configdialog = settings_dlg,
    .open = cdumb_open,
    .init = cdumb_init,
    .free = cdumb_free,
    .read = cdumb_read,
    .seek = cdumb_seek,
    .insert = cdumb_insert,
    .read_metadata = cdumb_read_metadata,
    .exts = exts,
    .plugin.message = cdumb_message,
};

DB_plugin_t *
ddb_dumb_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
