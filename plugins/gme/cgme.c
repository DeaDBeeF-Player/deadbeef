/*
    GameMusicEmu plugin for DeaDBeeF
    Copyright (C) 2009-2015 Oleksiy Yakovenko <waker@users.sourceforge.net>

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

#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gme/gme.h"
#include <zlib.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/strdupa.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "gmewrap.h"

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

// how big vgz can be?
#define MAX_REMOTE_GZ_FILE 0x100000

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;
static int conf_fadeout = 10;
static int conf_loopcount = 2;
static int conf_play_forever = 0;
static char *coleco_rom;

typedef struct {
    DB_fileinfo_t info;
    Music_Emu *emu;
    int reallength;
    float duration; // of current song
    int eof;
    int chip_voices;
    int can_loop;
    int rawsignal;
    int fade_set;
} gme_fileinfo_t;

static DB_fileinfo_t *
cgme_open (uint32_t hints) {
    gme_fileinfo_t *info = calloc (1, sizeof (gme_fileinfo_t));
    info->chip_voices = 0xff;
    info->can_loop = hints & DDB_DECODER_HINT_CAN_LOOP;
    info->rawsignal = hints & DDB_DECODER_HINT_RAW_SIGNAL;
    return &info->info;
}

static int
read_gzfile (const char *fname, char **buffer, int *size) {
    int fd = -1;
    int res = -1;
    DB_FILE *fp = deadbeef->fopen (fname);
    char tmpnm[PATH_MAX] = "";
    if (!fp) {
        trace ("gme read_gzfile: failed to fopen %s\n", fname);
        return -1;
    }

    int64_t sz = deadbeef->fgetlength (fp);
    if (fp->vfs && fp->vfs->plugin.id && strcmp (fp->vfs->plugin.id, "vfs_stdio") && sz > 0 && sz <= MAX_REMOTE_GZ_FILE) {
        trace ("gme read_gzfile: reading %s of size %lld and writing to temp file\n", fname, sz);
        char *buffer = malloc(sz);
        if (sz == deadbeef->fread (buffer, 1, sz, fp)) {
            static int idx = 0;
            const char *tmp = getenv ("TMPDIR");
            if (!tmp) {
                tmp = "/tmp";
            }
#if defined(ANDROID)
            // newer androids don't allow writing to /tmp, so write to ${configdir}/tmp/
            const char *confdir = deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG);
            snprintf (tmpnm, sizeof (tmpnm), "%s/tmp", confdir);
            mkdir (tmpnm, 0755);
            snprintf (tmpnm, sizeof (tmpnm), "%s/tmp/ddbgme%03d.vgz", confdir, idx);
            fd = open (tmpnm, O_RDWR|O_CREAT|O_TRUNC);
#elif defined(STATICLINK)
            // mkstemps is unavailable in this case (linking to old glibc),
            // and mkstemp is considered insecure,
            // so just make the name manually.
            // This is as insecure as mkstemp, but (hopefully) won't be bugged by static analyzers
            snprintf (tmpnm, sizeof (tmpnm), "%s/ddbgme%03d.vgz", tmp, idx);
            fd = open (tmpnm, O_RDWR|O_CREAT|O_TRUNC);
#else
            snprintf (tmpnm, sizeof (tmpnm), "%s/ddbgmeXXXXXX.vgz", tmp);
            fd = mkstemps (tmpnm, 4);
#endif
            idx++;
            if (fd == -1 || sz != write (fd, buffer, sz)) {
                trace ("gme read_gzfile: failed to write temp file %s\n", tmpnm);
                if (fd != -1) {
                    close (fd);
                    fd = -1;
                }
            }
            if (fd != -1) {
                lseek (fd, 0, SEEK_SET);
            }
            trace ("%s written successfully\n", tmpnm);
            free (buffer);
        }
    }

    deadbeef->fclose (fp);

    sz *= 2;
    int readsize = (int)sz;
    *buffer = malloc (sz);
    if (!(*buffer)) {
        goto error;
    }

    gzFile gz = fd == -1 ? gzopen (fname, "rb") : gzdopen (fd, "r");
    if (!gz) {
        trace ("failed to gzopen %s\n", fname);
        goto error;
    }
    *size = 0;
    int nb;
    int pos = 0;
    do {
        nb = gzread (gz, *buffer + pos, readsize);
        if (nb < 0) {
            free (*buffer);
            *buffer = NULL;
            trace ("failed to gzread from %s\n", fname);
            gzclose (gz);
            goto error;
        }
        if (nb > 0) {
            pos += nb;
            *size += nb;
        }
        if (nb != readsize) {
            break;
        }
        else {
            readsize = (int)sz;
            sz *= 2;
            *buffer = realloc (*buffer, sz);
        }
    } while (nb > 0);
    gzclose (gz);
    trace ("got %d bytes from %s\n", *size, fname);

    res = 0;
error:
    if (*tmpnm) {
        unlink (tmpnm);
    }

    return res;
}

static int
cgme_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    gme_fileinfo_t *info = (gme_fileinfo_t*)_info;
    int samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);

    gme_err_t res = "gme uninitialized";
    deadbeef->pl_lock ();
    const char *fname = strdupa(deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    char *buffer;
    int sz;
    if (!read_gzfile (fname, &buffer, &sz)) {
        res = gme_open_data (buffer, sz, &info->emu, samplerate);
        free (buffer);
    }
    if (res) {
        DB_FILE *f = deadbeef->fopen (fname);
        if (!f) {
            return -1;
        }
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

        res = gme_open_data (buf, sz, &info->emu, samplerate);
        free (buf);
    }

    if (res) {
        trace ("failed with error %d\n", res);
        return -1;
    }
    gme_start_track (info->emu, deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0));

    gme_info_t *inf;
    gme_track_info (info->emu, &inf, deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0));

    _info->plugin = &plugin;
    _info->fmt.bps = 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = samplerate;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    info->duration = deadbeef->pl_get_item_duration (it);
    info->reallength = inf->length; 
    _info->readpos = 0;
    info->eof = 0;
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
    int playForever = conf_play_forever && info->can_loop;
    float t = (size/4) / (float)_info->fmt.samplerate;
    if (info->eof) {
        return 0;
    }
    if (!playForever && _info->readpos + t >= info->duration) {
        t = info->duration - _info->readpos;
        if (t <= 0) {
            return 0;
        }
    }

    if (!info->rawsignal) {
        int chip_voices = deadbeef->conf_get_int ("chip.voices", 0xff);
        if (chip_voices != info->chip_voices) {
            info->chip_voices = chip_voices;
            gme_mute_voices (info->emu, chip_voices^0xff);
        }
    }

    // FIXME: it makes more sense to call gme_set_fade on init and configchanged
    if (playForever && info->fade_set) {
        gme_set_fade(info->emu, -1, 0);
        info->fade_set = 0;
    }
    else if (!playForever && !info->fade_set && conf_fadeout > 0 && info->duration >= conf_fadeout && _info->readpos >= info->duration - conf_fadeout) {
        gme_set_fade(info->emu, (int)(_info->readpos * 1000), conf_fadeout * 1000);
        info->fade_set = 1;
    }

    if (gme_play (info->emu, size/2, (short*)bytes)) {
        return 0;
    }

    _info->readpos += t;
    if (gme_track_ended (info->emu)) {
        info->eof = 1;
    }
    return size;
}

static int
cgme_seek (DB_fileinfo_t *_info, float time) {
    gme_fileinfo_t *info = (gme_fileinfo_t*)_info;
    if (gme_seek (info->emu, (int)(time * 1000))) {
        return -1;
    }
    _info->readpos = time;
    info->eof = 0;
    return 0;
}

static void
cgme_add_meta (DB_playItem_t *it, const char *key, const char *value) {
    if (!value) {
        return;
    }
    size_t len = strlen (value);
    char out[1024];
    // check for utf8 (hack)
    if (deadbeef->junk_iconv (value, (int)len, out, sizeof (out), "utf-8", "utf-8") >= 0) {
        deadbeef->pl_add_meta (it, key, out);
        return;
    }

    if (deadbeef->junk_iconv (value, (int)len, out, sizeof (out), "cp1252", "utf-8") >= 0) {
        deadbeef->pl_add_meta (it, key, out);
        return;
    }

    if (deadbeef->junk_iconv (value, (int)len, out, sizeof (out), "SHIFT-JIS", "utf-8") >= 0) {
        deadbeef->pl_add_meta (it, key, out);
        return;
    }

    // FIXME: try other encodings?

    return;
}

static DB_playItem_t *
cgme_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    Music_Emu *emu = NULL;
    trace ("gme_open_file %s\n", fname);

    gme_err_t res = "gme uninitialized";

    char *buffer = NULL;
    int sz;
    if (!read_gzfile (fname, &buffer, &sz)) {
        res = gme_open_data (buffer, sz, &emu, gme_info_only);
    }
    free (buffer);
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

        res = gme_open_data (buf, sz, &emu, gme_info_only);
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
            const char *ret = gme_track_info (emu, &_inf, i);
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
                    inf->length += conf_fadeout*1000;
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
                if (cnt > 1) {
                    deadbeef->pl_set_item_flags (it, deadbeef->pl_get_item_flags (it) | DDB_IS_SUBTRACK);
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
	"ay","gbs","gym","hes","kss","nsf","nsfe","sap","sfm","spc","vgm","vgz","sgc",NULL
};

static int
cgme_start (void) {
    return 0;
}

static int
cgme_stop (void) {
    if (coleco_rom) {
        free (coleco_rom);
        coleco_rom = NULL;
    }
    gme_set_sgc_coleco_bios (NULL);
    return 0;
}

static void
init_coleco_bios () {
    if (coleco_rom) {
        free (coleco_rom);
        coleco_rom = NULL;
    }
    gme_set_sgc_coleco_bios (NULL);
    char path[PATH_MAX];
    deadbeef->conf_get_str ("gme.coleco_rom", "", path, sizeof (path));
    if (path[0]) {
        FILE *fp = fopen (path, "rb");
        if (!fp) {
            return;
        }
        fseek (fp, 0, SEEK_END);
        size_t size = ftell (fp);
        rewind (fp);
        if (size != 0x2000) {
            fclose (fp);
            deadbeef->log_detailed (&plugin.plugin, DDB_LOG_LAYER_DEFAULT, "ColecoVision ROM file %s has invalid size (expected 8192 bytes)", path);
            return;
        }

        coleco_rom = malloc (size);
        size_t rb = fread (coleco_rom, 1, size, fp);
        fclose (fp);

        if (rb != 0x2000) {
            free (coleco_rom);
            coleco_rom = NULL;
            deadbeef->log_detailed (&plugin.plugin, DDB_LOG_LAYER_DEFAULT, "Failed to load ColecoVision ROM from file %s, invalid file?", path);
        }

        gme_set_sgc_coleco_bios (coleco_rom);
    }
}

int
cgme_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        conf_fadeout = deadbeef->conf_get_int ("gme.fadeout", 10);
        conf_loopcount = deadbeef->conf_get_int ("gme.loopcount", 2);
        conf_play_forever = deadbeef->streamer_get_repeat () == DDB_REPEAT_SINGLE;
        init_coleco_bios ();
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Max song length (in minutes)\" entry gme.songlength 3;\n"
    "property \"Fadeout length (seconds)\" entry gme.fadeout 10;\n"
    "property \"Play loops nr. of times (if available)\" entry gme.loopcount 2;\n"
    "property \"ColecoVision BIOS (for SGC file format)\" file gme.coleco_rom \"\";\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stdgme",
    .plugin.name = "Game-Music-Emu player",
    .plugin.descr = "chiptune/game music player based on GME library",
    .plugin.copyright = 
        "Game_Music_Emu plugin for DeaDBeeF\n"
        "Copyright (C) 2009-2015 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
        "\n"
        "\n"
        "\n"
        "Game_Music_Emu (modified)\n"
        "Copyright (C) 2003-2009 Shay Green.\n"
        "Foobar2000-related modifications (C) Chris Moeller\n"
        "DeaDBeeF-related modifications (C) Oleksiy Yakovenko.\n"
        "\n"
        "This library is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU Lesser General Public\n"
        "License as published by the Free Software Foundation; either\n"
        "version 2.1 of the License, or (at your option) any later version.\n"
        "\n"
        "This library is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
        "Lesser General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU Lesser General Public\n"
        "License along with this library; if not, write to the Free Software\n"
        "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n"
        "\n"
        "\n"
        "\n"
        "VGMPlay\n"
        "Copyright Nicola Salmoria and the MAME team\n"
        "All rights reserved.\n"
        "\n"
        "Redistribution and use of this code or any derivative works are permitted\n"
        "provided that the following conditions are met:\n"
        "\n"
        "* Redistributions may not be sold, nor may they be used in a commercial\n"
        "product or activity.\n"
        "\n"
        "* Redistributions that are modified from the original source must include the\n"
        "complete source code, including the source code for all components used by a\n"
        "binary built from the modified sources. However, as a special exception, the\n"
        "source code distributed need not include anything that is normally distributed\n"
        "(in either source or binary form) with the major components (compiler, kernel,\n"
        "and so on) of the operating system on which the executable runs, unless that\n"
        "component itself accompanies the executable.\n"
        "\n"
        "* Redistributions must reproduce the above copyright notice, this list of\n"
        "conditions and the following disclaimer in the documentation and/or other\n"
        "materials provided with the distribution.\n"
        "\n"
        "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
        "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
        "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
        "ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE\n"
        "LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"
        "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"
        "SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n"
        "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN\n"
        "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n"
        "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE\n"
        "POSSIBILITY OF SUCH DAMAGE.\n"
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
    .plugin.message = cgme_message,
};

DB_plugin_t *
gme_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
