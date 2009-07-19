#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gme/gme.h"
#include "codec.h"
#include "cgme.h"
#include "playlist.h"

static Music_Emu *emu;
static int reallength;
static int nzerosamples;
extern int sdl_player_freq; // hack!

int
cgme_init (const char *fname, int track, float start, float end) {
    if (gme_open_file (fname, &emu, sdl_player_freq)) {
        return -1;
    }
    gme_start_track (emu, track);
    track_info_t inf;
    gme_track_info (emu, &inf, track);
    cgme.info.bitsPerSample = 16;
    cgme.info.channels = 2;
    cgme.info.samplesPerSecond = sdl_player_freq;
    reallength = inf.length; 
    nzerosamples = 0;
    if (inf.length == -1) {
        cgme.info.duration = 120;
    }
    else {
        cgme.info.duration = (float)inf.length/1000.f;
    }
    cgme.info.position = 0;
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
    if (cgme.info.position + t >= cgme.info.duration) {
        t = cgme.info.duration - cgme.info.position;
        if (t <= 0) {
            return 0;
        }
        // DON'T ajust size, buffer must always be po2
        //size = t * (float)cgme.info.samplesPerSecond * 4;
    }
    if (gme_play (emu, size/2, (short*)bytes)) {
        return 0;
    }
    cgme.info.position += t;
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
    cgme.info.position = time;
    return 0;
}

int
cgme_add (const char *fname) {
//    printf ("adding %s chiptune\n", fname);
    Music_Emu *emu;
    if (!gme_open_file (fname, &emu, 44100)) {
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
                ps_add_meta (it, "system", inf.system);
                ps_add_meta (it, "album", inf.game);
                ps_add_meta (it, "title", inf.song);
                ps_add_meta (it, "artist", inf.author);
                ps_add_meta (it, "copyright", inf.copyright);
                ps_add_meta (it, "comment", inf.comment);
                ps_add_meta (it, "dumper", inf.dumper);
                char trk[10];
                snprintf (trk, 10, "%d", i+1);
                ps_add_meta (it, "track", trk);
                ps_append_item (it);
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
    return 0;
}

static const char * exts[]=
{
	"ay","gbs","gym","hes","kss","nsf","nsfe","sap","spc","vgm","vgz",NULL
};

const char **cgme_getexts (void) {
    return exts;
}


codec_t cgme = {
    .init = cgme_init,
    .free = cgme_free,
    .read = cgme_read,
    .seek = cgme_seek,
    .add = cgme_add,
    .getexts = cgme_getexts
};

