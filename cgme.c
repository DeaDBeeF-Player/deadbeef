/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include "codec.h"
#include "cgme.h"
#include "playlist.h"
#include "playback.h"

static Music_Emu *emu;
static int reallength;
static int nzerosamples;
static uint32_t cgme_voicemask = 0;

int
cgme_init (playItem_t *it) {
    if (gme_open_file (it->fname, &emu, p_get_rate ())) {
        return -1;
    }
    gme_mute_voices (emu, cgme_voicemask);
    gme_start_track (emu, it->tracknum);
    track_info_t inf;
    gme_track_info (emu, &inf, it->tracknum);
    cgme.info.bitsPerSample = 16;
    cgme.info.channels = 2;
    cgme.info.samplesPerSecond = p_get_rate ();
    reallength = inf.length; 
    nzerosamples = 0;
    cgme.info.readposition = 0;
    return 0;
}

void
cgme_free (void) {
    if (emu) {
        gme_delete (emu);
        emu = NULL;
    }
}

int
cgme_read (char *bytes, int size) {
    float t = (size/4) / (float)cgme.info.samplesPerSecond;
    if (cgme.info.readposition + t >= playlist_current.duration) {
        t = playlist_current.duration - cgme.info.readposition;
        if (t <= 0) {
            return 0;
        }
        // DON'T ajust size, buffer must always be po2
        //size = t * (float)cgme.info.samplesPerSecond * 4;
    }
    if (gme_play (emu, size/2, (short*)bytes)) {
        return 0;
    }
    cgme.info.readposition += t;
    if (reallength == -1) {
        // check if whole buffer is zeroes
        int i;
        for (i = 0; i < size; i++) {
            if (bytes[i]) {
                break;
            }
        }
        if (i == size) {
            nzerosamples += size / 4;
            if (nzerosamples > cgme.info.samplesPerSecond * 4) {
                return 0;
            }
        }
        else {
            nzerosamples = 0;
        }
    }
    return size;
}

int
cgme_seek (float time) {
    if (gme_seek (emu, (long)(time * 1000))) {
        return -1;
    }
    cgme.info.readposition = time;
    return 0;
}

playItem_t *
cgme_insert (playItem_t *after, const char *fname) {
//    printf ("adding %s chiptune\n", fname);
    Music_Emu *emu;
    if (!gme_open_file (fname, &emu, gme_info_only)) {
        int cnt = gme_track_count (emu);
        for (int i = 0; i < cnt; i++) {
            track_info_t inf;
            const char *ret = gme_track_info (emu, &inf, i);
            if (!ret) {
                playItem_t *it = malloc (sizeof (playItem_t));
                memset (it, 0, sizeof (playItem_t));
                it->codec = &cgme;
                it->fname = strdup (fname);
                char str[1024];
                if (inf.song[0]) {
                    snprintf (str, 1024, "%d %s - %s", i, inf.game, inf.song);
                }
                else {
                    snprintf (str, 1024, "%d %s - ?", i, inf.game);
                }
                it->tracknum = i;

                // add metadata
                pl_add_meta (it, "system", inf.system);
                pl_add_meta (it, "album", inf.game);
                pl_add_meta (it, "title", inf.song);
                pl_add_meta (it, "artist", inf.author);
                pl_add_meta (it, "copyright", inf.copyright);
                pl_add_meta (it, "comment", inf.comment);
                pl_add_meta (it, "dumper", inf.dumper);
                char trk[10];
                snprintf (trk, 10, "%d", i+1);
                pl_add_meta (it, "track", trk);
                if (inf.length == -1) {
                    it->duration = 300;
                }
                else {
                    it->duration = (float)inf.length/1000.f;
                }
                it->filetype = "gme";
                after = pl_insert_item (after, it);
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
        printf ("error adding %s\n", fname);
    }
    return after;
}

static const char * exts[]=
{
	"ay","gbs","gym","hes","kss","nsf","nsfe","sap","spc","vgm","vgz",NULL
};

const char **cgme_getexts (void) {
    return exts;
}

int
cgme_numvoices (void) {
    if (!emu) {
        return 0;
    }
    return gme_voice_count (emu);
}

void
cgme_mutevoice (int voice, int mute) {
    cgme_voicemask &= ~ (1<<voice);
    cgme_voicemask |= ((mute ? 1 : 0) << voice);
    if (emu) {
        gme_mute_voices (emu, cgme_voicemask);
    }
}

codec_t cgme = {
    .init = cgme_init,
    .free = cgme_free,
    .read = cgme_read,
    .seek = cgme_seek,
    .insert = cgme_insert,
    .getexts = cgme_getexts,
    .numvoices = cgme_numvoices,
    .mutevoice = cgme_mutevoice,
    .id = "stdgme",
    .filetypes = { "gme", NULL }
};

