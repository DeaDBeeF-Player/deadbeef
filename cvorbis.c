/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "codec.h"
#include "cvorbis.h"
#include "playlist.h"

static FILE *file;
static OggVorbis_File vorbis_file;
static vorbis_info *vi;
static int cur_bit_stream;
static float timestart;
static float timeend;

void
cvorbis_free (void);

int
cvorbis_seek (float time);

int
cvorbis_init (struct playItem_s *it) {
    file = NULL;
    vi = NULL;
    cur_bit_stream = -1;

    file = fopen (it->fname, "rb");
    if (!file) {
        return -1;
    }

    memset (&cvorbis.info, 0, sizeof (fileinfo_t));
    ov_open (file, &vorbis_file, NULL, 0);
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        cvorbis_free ();
        return -1;
    }
    cvorbis.info.bitsPerSample = 16;
    //cvorbis.info.dataSize = ov_pcm_total (&vorbis_file, -1) * vi->channels * 2;
    cvorbis.info.channels = vi->channels;
    cvorbis.info.samplesPerSecond = vi->rate;
    cvorbis.info.readposition = 0;
    if (timeend > 0) {
        timestart = it->timestart;
        timeend = it->timeend;
        cvorbis_seek (0);
    }
    else {
        timestart = 0;
        timeend = it->duration;
    }
//    printf ("vorbis info: bps: %d, size: %d, chan: %d, rate: %d, dur: %f\n", cvorbis.info.bitsPerSample, cvorbis.info.dataSize, cvorbis.info.channels, cvorbis.info.samplesPerSecond, cvorbis.info.duration);
    return 0;
}

void
cvorbis_free (void) {
    if (file) {
        ov_clear (&vorbis_file);
        //fclose (file); //-- ov_clear closes it
        file = NULL;
        vi = NULL;
    }
}

int
cvorbis_read (char *bytes, int size) {
    if (cvorbis.info.readposition >= (timeend - timestart)) {
        return 0;
    }
    int initsize = size;
    for (;;)
    {
        // read ogg
        long ret=ov_read (&vorbis_file, bytes, size, 0, 2, 1, &cur_bit_stream);
        if (ret < 0)
        {
            break;
        }
        else if (ret == 0) {
            break;
        }
        else if (ret < size)
        {
            size -= ret;
            bytes += ret;
        }
        else {
            size = 0;
            break;
        }
    }
    cvorbis.info.readposition = ov_time_tell(&vorbis_file) - timestart;
    if (cvorbis.info.readposition >= (timeend - timestart)) {
        return 0;
    }

    return initsize - size;
}

int
cvorbis_seek (float time) {
    time += timestart;
    if (!file) {
        return -1;
    }
    int res = ov_time_seek (&vorbis_file, time);
    if (res != 0 && res != OV_ENOSEEK)
        return -1;
    cvorbis.info.readposition = ov_time_tell(&vorbis_file) - timestart;
    return 0;
}

playItem_t *
cvorbis_insert (playItem_t *after, const char *fname) {
    // check for validity
    FILE *fp = fopen (fname, "rb");
    if (!fp) {
        return NULL;
    }
    OggVorbis_File vorbis_file;
    vorbis_info *vi;
    ov_open (fp, &vorbis_file, NULL, 0);
    vi = ov_info (&vorbis_file, -1);
    if (!vi) { // not a vorbis stream
        return NULL;
    }
    playItem_t *it = malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &cvorbis;
    it->fname = strdup (fname);
    it->filetype = "OggVorbis";
    it->duration = ov_seekable (&vorbis_file) ? ov_time_total (&vorbis_file, -1) : -1;

    playItem_t *cue_after = pl_insert_cue (after, fname, &cvorbis, it->filetype);
    if (cue_after) {
        cue_after->timeend = it->duration;
        cue_after->duration = cue_after->timeend - cue_after->timestart;
        pl_item_free (it);
        ov_clear (&vorbis_file);
        return cue_after;
    }

    // metainfo
    int title_added = 0;
    vorbis_comment *vc = ov_comment (&vorbis_file, -1);
    if (vc) {
        pl_add_meta (it, "vendor", vc->vendor);
        for (int i = 0; i < vc->comments; i++) {
            if (!strncmp (vc->user_comments[i], "artist=", 7)) {
                pl_add_meta (it, "artist", vc->user_comments[i] + 7);
            }
            else if (!strncmp (vc->user_comments[i], "title=", 6)) {
                pl_add_meta (it, "title", vc->user_comments[i] + 6);
                title_added = 1;
            }
            else if (!strncmp (vc->user_comments[i], "date=", 5)) {
                pl_add_meta (it, "date", vc->user_comments[i] + 5);
            }
        }
    }
    if (!title_added) {
        pl_add_meta (it, "title", NULL);
    }
    ov_clear (&vorbis_file);
    after = pl_insert_item (after, it);
    return after;
}

static const char * exts[]=
{
	"ogg",NULL
};

const char **cvorbis_getexts (void) {
    return exts;
}

codec_t cvorbis = {
    .init = cvorbis_init,
    .free = cvorbis_free,
    .read = cvorbis_read,
    .seek = cvorbis_seek,
    .insert = cvorbis_insert,
    .getexts = cvorbis_getexts,
    .id = "stdogg",
    .filetypes = { "OggVorbis", NULL }
};

