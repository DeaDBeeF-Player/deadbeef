/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <math.h>
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
static int conf_fadeout = 10;
static int conf_loopcount = 2;

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
        nb = gzread (gz, *buffer + pos, readsize);
        if (nb < 0) {
            free (*buffer);
            trace ("failed to gzread from %s\n", fname);
            return -1;
        }
        if (nb > 0) {
            pos += nb;
            *size += nb;
        }
        if (nb != readsize) {
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

    gme_err_t res = "gme uninitialized";
    const char *ext = strrchr (deadbeef->pl_find_meta (it, ":URI"), '.');
    char *buffer;
    int sz;
    if (!read_gzfile (deadbeef->pl_find_meta (it, ":URI"), &buffer, &sz)) {
        res = gme_open_data (deadbeef->pl_find_meta (it, ":URI"), buffer, sz, &info->emu, samplerate);
        free (buffer);
    }
    if (res) {
        DB_FILE *f = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
        int64_t sz = deadbeef->fgetlength (f);
        if (sz <= 0) {
            deadbeef->fclose (f);
            return -1;
        }
        char *buf = malloc (sz);
        if (!buf) {
            deadbeef->fclose (f);
            return -1;
        }
        int64_t rb = deadbeef->fread (buf, 1, sz, f);
        deadbeef->fclose(f);
        if (rb != sz) {
            free (buf);
            return -1;
        }

        res = gme_open_data (deadbeef->pl_find_meta (it, ":URI"), buf, sz, &info->emu, samplerate);
        free (buf);
    }

    if (res) {
        trace ("failed with error %d\n", res);
        return -1;
    }
    gme_mute_voices (info->emu, info->cgme_voicemask);
    gme_start_track (info->emu, deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0));

#ifdef GME_VERSION_055
    gme_info_t *inf;
    gme_track_info (info->emu, &inf, deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0));
#else
    track_info_t _inf;
    gme_track_info (info->emu, &inf, deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0));
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
    if (conf_fadeout > 0 && info->duration >= conf_fadeout && info->reallength <= 0 && _info->readpos >= info->duration - conf_fadeout) {
        float fade_amnt =  (info->duration - _info->readpos) / (float)conf_fadeout;
        int nsamples = size/2;
        float fade_incr = 1.f / (_info->fmt.samplerate * conf_fadeout) * 256;
        const float ln10=2.3025850929940002f;
        float fade = exp(ln10*(-(1.f-fade_amnt) * 3));

        for (int i = 0; i < nsamples; i++) {
            ((short*)bytes)[i] *= fade;
            if (!(i & 0xff)) {
                fade_amnt += fade_incr;
                fade = exp(ln10*(-(1.f-fade_amnt) * 3));
            }
        }
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

static void
cgme_add_meta (DB_playItem_t *it, const char *key, const char *value) {
    if (!value) {
        return;
    }
    char len = strlen (value);
    char out[1024];
    // check for utf8 (hack)
    if (deadbeef->junk_iconv (value, len, out, sizeof (out), "utf-8", "utf-8") >= 0) {
        deadbeef->pl_add_meta (it, key, out);
        return;
    }

    if (deadbeef->junk_iconv (value, len, out, sizeof (out), "iso8859-1", "utf-8") >= 0) {
        deadbeef->pl_add_meta (it, key, out);
        return;
    }

    if (deadbeef->junk_iconv (value, len, out, sizeof (out), "SHIFT-JIS", "utf-8") >= 0) {
        deadbeef->pl_add_meta (it, key, out);
        return;
    }

    // FIXME: try other encodings?

    return;
}

static DB_playItem_t *
cgme_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    Music_Emu *emu;
    trace ("gme_open_file %s\n", fname);

    gme_err_t res = "gme uninitialized";

    const char *ext = strrchr (fname, '.');
    char *buffer;
    int sz;
    if (!read_gzfile (fname, &buffer, &sz)) {
        res = gme_open_data (fname, buffer, sz, &emu, gme_info_only);
        free (buffer);
    }
    if (res) {
        DB_FILE *f = deadbeef->fopen (fname);
        if (!f) {
            return NULL;
        }
        int64_t sz = deadbeef->fgetlength (f);
        if (sz <= 0) {
            deadbeef->fclose (f);
            return NULL;
        }
        char *buf = malloc (sz);
        if (!buf) {
            deadbeef->fclose (f);
            return NULL;
        }
        int64_t rb = deadbeef->fread (buf, 1, sz, f);
        deadbeef->fclose(f);
        if (rb != sz) {
            free (buf);
            return NULL;
        }

        res = gme_open_data (fname, buf, sz, &emu, gme_info_only);
        free (buf);
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
                DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
                char str[1024];
                if (inf->song[0]) {
                    snprintf (str, sizeof(str), "%d %s - %s", i, inf->game, inf->song);
                }
                else {
                    snprintf (str, sizeof(str), "%d %s - ?", i, inf->game);
                }
                trace ("track subtune %d %s, length=%d\n", i, str, inf->length);
                deadbeef->pl_set_meta_int (it, ":TRACKNUM", i);

                // add metadata
                cgme_add_meta (it, "system", inf->system);
                cgme_add_meta (it, "album", inf->game);
                int tl = sizeof (inf->song);
                int n;
                for (n = 0; i < tl && inf->song[n] && inf->song[n] == ' '; n++);
                if (n == tl || !inf->song[n]) {
                    deadbeef->pl_add_meta (it, "title", NULL);
                }
                else {
                    cgme_add_meta (it, "title", inf->song);
                }
                cgme_add_meta (it, "artist", inf->author);
                cgme_add_meta (it, "copyright", inf->copyright);
                cgme_add_meta (it, "comment", inf->comment);
                cgme_add_meta (it, "dumper", inf->dumper);
                char trk[10];
                snprintf (trk, 10, "%d", i+1);
                cgme_add_meta (it, "track", trk);
                snprintf (str, sizeof(str), "%d", inf->length);
                deadbeef->pl_add_meta (it, ":GME_LENGTH", str);
                snprintf (str, sizeof(str), "%d", inf->intro_length);
                deadbeef->pl_add_meta (it, ":GME_INTRO_LENGTH", str);
                snprintf (str, sizeof(str), "%d", inf->loop_length);
                deadbeef->pl_add_meta (it, ":GME_LOOP_LENGTH", str);
                if (inf->length == -1 || inf->length == 0) {
                    float songlength;
                    
                    if (inf->loop_length > 0 && conf_loopcount > 0) {
                        songlength = inf->intro_length / 1000.f;
                        if (songlength < 0) {
                            songlength = 0;
                        }
                        songlength += (inf->loop_length * conf_loopcount) / 1000.f;
                    }
                    else {
                        songlength = deadbeef->conf_get_float ("gme.songlength", 3) * 60.f;
                    }
                    deadbeef->plt_set_item_duration (plt, it, songlength);
                }
                else {
                    deadbeef->plt_set_item_duration (plt, it, (float)inf->length/1000.f);
                }
                const char *ext = fname + strlen (fname) - 1;
                while (ext >= fname && *ext != '.') {
                    ext--;
                }
                if (*ext == '.') {
                    ext++;
                    for (int i = 0; plugin.exts[i]; i++) {
                        if (!strcasecmp (ext, plugin.exts[i])) {
                            deadbeef->pl_add_meta (it, ":FILETYPE", plugin.exts[i]);
                            break;
                        }
                    }
                }
                after = deadbeef->plt_insert_item (plt, after, it);
                deadbeef->pl_item_unref (it);
            }
            else {
                trace ("gme error: %s\n", ret);
            }
        }
        if (emu) {
            gme_delete (emu);
        }
    }
    else {
        trace ("gme_open_file/data failed with error %s\n", res);
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

int
cgme_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        conf_fadeout = deadbeef->conf_get_int ("gme.fadeout", 10);
        conf_loopcount = deadbeef->conf_get_int ("gme.loopcount", 2);
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Max song length (in minutes)\" entry gme.songlength 3;\n"
    "property \"Fadeout length (seconds)\" entry gme.fadeout 10;\n"
    "property \"Play loops nr. of times (if available)\" entry gme.loopcount 2;\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdgme",
    .plugin.name = "Game-Music-Emu player",
    .plugin.descr = "chiptune/game music player based on GME library",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses Game-Music-Emu by Shay Green <gblargg@gmail.com>, http://code.google.com/p/game-music-emu/\n"
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
};

DB_plugin_t *
gme_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
