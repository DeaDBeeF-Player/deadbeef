/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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

#include <string.h>
#include <stdio.h>
#include <neaacdec.h>
#include <mp4ff.h>
#include <stdlib.h>
#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

static int
aac_init (DB_playItem_t *it) {
    return 0;
}

static void
aac_free (void) {
}

static int
aac_read_int16 (char *bytes, int size) {
    return 0;
}

static int
aac_seek_sample (int sample) {
    return 0;
}

static int
aac_seek (float t) {
    return aac_seek_sample (t * plugin.info.samplerate);
}

static uint32_t
aac_fs_read (void *user_data, void *buffer, uint32_t length) {
    trace ("aac_fs_read\n");
    DB_FILE *fp = (DB_FILE *)user_data;
    return deadbeef->fread (buffer, 1, length, fp);
}

static uint32_t
aac_fs_seek (void *user_data, uint64_t position) {
    trace ("aac_fs_seek\n");
    DB_FILE *fp = (DB_FILE *)user_data;
    return deadbeef->fseek (fp, position, SEEK_SET);
}

/*
 * These routines are derived from MPlayer.
 */

/// \param srate (out) sample rate
/// \param num (out) number of audio frames in this ADTS frame
/// \return size of the ADTS frame in bytes
/// aac_parse_frames needs a buffer at least 8 bytes long
static int
aac_parse_frame(uint8_t *buf, int *srate, int *num)
{
    int i = 0, sr, fl = 0;
    static int srates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0, 0, 0};

    if((buf[i] != 0xFF) || ((buf[i+1] & 0xF6) != 0xF0))
        return 0;

    /* We currently have no use for the id below.
       id = (buf[i+1] >> 3) & 0x01;    //id=1 mpeg2, 0: mpeg4
       */
    sr = (buf[i+2] >> 2)  & 0x0F;
    if(sr > 11)
        return 0;
    *srate = srates[sr];

    fl = ((buf[i+3] & 0x03) << 11) | (buf[i+4] << 3) | ((buf[i+5] >> 5) & 0x07);
    *num = (buf[i+6] & 0x02) + 1;

    return fl;
}

static int
parse_aac_stream(DB_FILE *stream)
{
    int cnt = 0, len, srate, num;
    int8_t c;
    off_t init, probed;
    static uint8_t buf[8];

    init = probed = deadbeef->ftell(stream);
    while(probed-init <= 32768 && cnt < 8)
    {
        c = 0;
        while(probed-init <= 32768 && c != 0xFF)
        {
            if (deadbeef->fread (&c, 1, 1, stream) != 1) {
                return 1;
            }
            if(c < 0)
                return 0;
            probed = deadbeef->ftell(stream);
        }
        buf[0] = 0xFF;
        if(deadbeef->fread(&(buf[1]), 1, 7, stream) < 7)
            return 0;

        len = aac_parse_frame(buf, &srate, &num);
        if(len > 0)
        {
            cnt++;
            deadbeef->fseek(stream, len - 8, SEEK_CUR);
        }
        probed = deadbeef->ftell(stream);
    }

    if(cnt < 8)
        return 0;

    return 1;
}

static DB_playItem_t *
aac_insert (DB_playItem_t *after, const char *fname) {
    return NULL; // to avoid crashes
    trace ("adding %s\n", fname);
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("not found\n");
        return NULL;
    }
    if (fp->vfs->streaming) {
        trace ("no streaming aac yet (%s)\n", fname);
        deadbeef->fclose (fp);
        return NULL;
    }

    // try mp4

    mp4ff_callback_t cb = {
        .read = aac_fs_read,
        .write = NULL,
        .seek = aac_fs_seek,
        .truncate = NULL,
        .user_data = fp
    };

    float duration = -1;
    const char *ftype = NULL;
    mp4ff_t *mp4 = mp4ff_open_read (&cb);
    if (!mp4) {
        trace ("not an mp4 file\n");
        deadbeef->fclose (fp);
        return NULL;
    }
    int ntracks = mp4ff_total_tracks (mp4);
    trace ("ntracks=%d\n", ntracks);
    int i = -1;
    for (i = 0; i < ntracks; i++) {
        unsigned char*  buff = 0;
        unsigned int    buff_size = 0;
        mp4AudioSpecificConfig mp4ASC;
        mp4ff_get_decoder_config(mp4, i, &buff, &buff_size);
        if(buff){
            int rc = AudioSpecificConfig(buff, buff_size, &mp4ASC);
            free(buff);
            if(rc < 0)
                continue;
            break;
        }
    }
    if (i != ntracks) 
    {
        trace ("mp4 track: %d\n", i);
        int samplerate = mp4ff_get_sample_rate (mp4, i);
        duration = mp4ff_get_track_duration (mp4, i) / (float)samplerate;
        ftype = "mp4";
    }
    else {
        trace ("mp4 track not found\n");
    }

    mp4ff_close (mp4);

    if (duration < 0) {
        trace ("trying raw aac\n");
        // not an mp4, try raw aac
        deadbeef->rewind (fp);
        if (parse_aac_stream (fp)) {
            trace ("not aac stream either\n");
            deadbeef->fclose (fp);
            return NULL;
        }
        else {
            trace ("yes, it's aac indeed\n");
            ftype = "aac";
        }
    }
    
    DB_playItem_t *it = deadbeef->pl_item_alloc ();
    it->decoder = &plugin;
    it->fname = strdup (fname);
    it->filetype = ftype;
    it->duration = duration;

    // read tags
    if (ftype == "aac") {
        int apeerr = deadbeef->junk_read_ape (it, fp);
        int v2err = deadbeef->junk_read_id3v2 (it, fp);
        int v1err = deadbeef->junk_read_id3v1 (it, fp);
    }
    deadbeef->pl_add_meta (it, "title", NULL);

    deadbeef->fclose (fp);

    return deadbeef->pl_insert_item (after, it);
}

static const char * exts[] = { "aac", "mp4", "m4a", NULL };
static const char *filetypes[] = { "aac", "mp4", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "faad2 AAC decoder",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .init = aac_init,
    .free = aac_free,
    .read_int16 = aac_read_int16,
    .seek = aac_seek,
    .seek_sample = aac_seek_sample,
    .insert = aac_insert,
    .exts = exts,
    .id = "aac",
    .filetypes = filetypes
};

DB_plugin_t *
faad2_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
