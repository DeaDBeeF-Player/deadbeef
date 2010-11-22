/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gme/gme.h"
#include <zlib.h>
#include "../../deadbeef.h"

int _Unwind_Resume_or_Rethrow;
int _Unwind_RaiseException;
int _Unwind_GetLanguageSpecificData;
int _Unwind_Resume;
int _Unwind_DeleteException;
int _Unwind_GetTextRelBase;
int _Unwind_SetIP;
int _Unwind_GetDataRelBase;
int _Unwind_GetRegionStart;
int _Unwind_SetGR;
int _Unwind_GetIPInfo;

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    Music_Emu *emu;
    int reallength;
    uint32_t cgme_voicemask;
    float duration; // of current song
} gme_fileinfo_t;

static DB_fileinfo_t *
cgme_open (uint32_t hint) {
    DB_fileinfo_t *_info = malloc (sizeof (gme_fileinfo_t));
    memset (_info, 0, sizeof (gme_fileinfo_t));
    return _info;
}

static int
read_gzfile (const char *fname, char **buffer, int *size) {
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        trace ("failed to fopen %s\n", fname);
        return -1;
    }
    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);
    fclose (fp);

    sz *= 2;
    int readsize = sz;
    *buffer = malloc (sz);
    if (!(*buffer)) {
        return -1;
    }

    gzFile gz = gzopen (fname, "rb");
    if (!gz) {
        trace ("failed to gzopen %s\n", fname);
        return -1;
    }
    *size = 0;
    int nb;
    int pos = 0;
    do {
        printf ("gzread %d pos %d\n", readsize, pos);
        nb = gzread (gz, *buffer + pos, readsize);
        printf ("got %d\n", nb);
        if (nb < 0) {
            free (*buffer);
            trace ("failed to gzread from %s\n", fname);
            return -1;
        }
        if (nb > 0) {
            pos += nb;
            *size += nb;
            printf ("size %d\n", *size);
        }
        if (nb != readsize) {
            printf ("done!\n", *size);
            break;
        }
        else {
            readsize = sz;
            sz *= 2;
            *buffer = realloc (*buffer, sz);
        }
    } while (nb > 0);
    gzclose (gz);
    trace ("got %d bytes from %s\n", *size, fname);

    return 0;
}

static int
cgme_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    gme_fileinfo_t *info = (gme_fileinfo_t*)_info;
    int samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);

    gme_err_t res;
    const char *ext = strrchr (it->fname, '.');
    if (ext && !strcasecmp (ext, ".vgz")) {
        trace ("opening gzipped vgm...\n");
        char *buffer;
        int sz;
        if (!read_gzfile (it->fname, &buffer, &sz)) {
            res = gme_open_data (buffer, sz, &info->emu, samplerate);
            free (buffer);
        }
    }
    else {
        res = gme_open_file (it->fname, &info->emu, samplerate);
    }

    if (res) {
        trace ("failed with error %d\n", res);
        return -1;
    }
    gme_mute_voices (info->emu, info->cgme_voicemask);
    gme_start_track (info->emu, it->tracknum);

#ifdef GME_VERSION_055
    gme_info_t *inf;
    gme_track_info (info->emu, &inf, it->tracknum);
#else
    track_info_t _inf;
    gme_track_info (info->emu, &inf, it->tracknum);
    track_info_t *inf = &_inf;
#endif

    _info->plugin = &plugin;
    _info->fmt.bps = 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = samplerate;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    info->duration = deadbeef->pl_get_item_duration (it);
    info->reallength = inf->length; 
    _info->readpos = 0;
    return 0;
}

static void
cgme_free (DB_fileinfo_t *_info) {
    gme_fileinfo_t *info = (gme_fileinfo_t*)_info;
    if (info->emu) {
        gme_delete (info->emu);
    }
    free (info);
}

static int
cgme_read (DB_fileinfo_t *_info, char *bytes, int size) {
    gme_fileinfo_t *info = (gme_fileinfo_t*)_info;
    float t = (size/4) / (float)_info->fmt.samplerate;
    if (_info->readpos + t >= info->duration) {
        t = info->duration - _info->readpos;
        if (t <= 0) {
            return 0;
        }
        // DON'T ajust size, buffer must always be po2
        //size = t * (float)info->samplerate * 4;
    }
    if (gme_play (info->emu, size/2, (short*)bytes)) {
        return 0;
    }
    _info->readpos += t;
    if (info->reallength == -1) {
        if (gme_track_ended (info->emu)) {
            return 0;
        }
    }
    return size;
}

static int
cgme_seek (DB_fileinfo_t *_info, float time) {
    gme_fileinfo_t *info = (gme_fileinfo_t*)_info;
    if (gme_seek (info->emu, (long)(time * 1000))) {
        return -1;
    }
    _info->readpos = time;
    return 0;
}

static DB_playItem_t *
cgme_insert (DB_playItem_t *after, const char *fname) {
    Music_Emu *emu;
    trace ("gme_open_file %s\n", fname);

    gme_err_t res;

    const char *ext = strrchr (fname, '.');
    if (ext && !strcasecmp (ext, ".vgz")) {
        trace ("opening gzipped vgm...\n");
        char *buffer;
        int sz;
        if (!read_gzfile (fname, &buffer, &sz)) {
            res = gme_open_data (buffer, sz, &emu, gme_info_only);
            free (buffer);
        }
    }
    else {
        res = gme_open_file (fname, &emu, gme_info_only);
    }


    if (!res) {
        int cnt = gme_track_count (emu);
        trace ("track cnt %d\n", cnt);
        for (int i = 0; i < cnt; i++) {
#ifdef GME_VERSION_055
            gme_info_t *inf;
            const char *ret = gme_track_info (emu, &inf, i);
#else
            track_info_t _inf;
            const char *ret = gme_track_info (emu, &inf, i);
            track_info_t *inf = &_inf;
#endif
            if (!ret) {
                DB_playItem_t *it = deadbeef->pl_item_alloc ();
                it->decoder_id = deadbeef->plug_get_decoder_id (plugin.plugin.id);
                it->fname = strdup (fname);
                char str[1024];
                if (inf->song[0]) {
                    snprintf (str, 1024, "%d %s - %s", i, inf->game, inf->song);
                }
                else {
                    snprintf (str, 1024, "%d %s - ?", i, inf->game);
                }
                trace ("track subtune %d %s, length=%d\n", i, str, inf->length);
                it->tracknum = i;

                // add metadata
                deadbeef->pl_add_meta (it, "system", inf->system);
                deadbeef->pl_add_meta (it, "album", inf->game);
                int tl = sizeof (inf->song);
                int n;
                for (n = 0; i < tl && inf->song[n] && inf->song[n] == ' '; n++);
                if (n == tl || !inf->song[n]) {
                    deadbeef->pl_add_meta (it, "title", NULL);
                }
                else {
                    deadbeef->pl_add_meta (it, "title", inf->song);
                }
                deadbeef->pl_add_meta (it, "artist", inf->author);
                deadbeef->pl_add_meta (it, "copyright", inf->copyright);
                deadbeef->pl_add_meta (it, "comment", inf->comment);
                deadbeef->pl_add_meta (it, "dumper", inf->dumper);
                char trk[10];
                snprintf (trk, 10, "%d", i+1);
                deadbeef->pl_add_meta (it, "track", trk);
                if (inf->length == -1) {
                    float songlength = deadbeef->conf_get_float ("gme.songlength", 3);
                    deadbeef->pl_set_item_duration (it, songlength * 60.f);
                }
                else {
                    deadbeef->pl_set_item_duration (it, (float)inf->length/1000.f);
                }
                const char *ext = fname + strlen (fname) - 1;
                while (ext >= fname && *ext != '.') {
                    ext--;
                }
                it->filetype = NULL;
                if (*ext == '.') {
                    ext++;
                    for (int i = 0; plugin.exts[i]; i++) {
                        if (!strcasecmp (ext, plugin.exts[i])) {
                            it->filetype = plugin.exts[i];
                        }
                    }
                }
                after = deadbeef->pl_insert_item (after, it);
                deadbeef->pl_item_unref (it);
            }
            else {
                printf ("gme error: %s\n", ret);
            }
        }
        if (emu) {
            gme_delete (emu);
        }
    }
    else {
        printf ("gme_open_file/data failed\n");
    }
    return after;
}

static const char * exts[]=
{
	"ay","gbs","gym","hes","kss","nsf","nsfe","sap","spc","vgm","vgz",NULL
};

#if 0
static int
cgme_numvoices (void) {
    if (!emu) {
        return 0;
    }
    return gme_voice_count (emu);
}

static void
cgme_mutevoice (int voice, int mute) {
    cgme_voicemask &= ~ (1<<voice);
    cgme_voicemask |= ((mute ? 1 : 0) << voice);
    if (emu) {
        gme_mute_voices (emu, cgme_voicemask);
    }
}
#endif

static int
cgme_start (void) {
    return 0;
}

static int
cgme_stop (void) {
    return 0;
}

static const char settings_dlg[] =
    "property \"Max song length (in minutes)\" entry gme.songlength 3;\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdgme",
    .plugin.name = "Game_Music_Emu decoder",
    .plugin.descr = "chiptune music player based on GME",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = cgme_start,
    .plugin.stop = cgme_stop,
    .plugin.configdialog = settings_dlg,
    .open = cgme_open,
    .init = cgme_init,
    .free = cgme_free,
    .read = cgme_read,
    .seek = cgme_seek,
    .insert = cgme_insert,
    .exts = exts,
    .filetypes = exts
};

DB_plugin_t *
gme_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
