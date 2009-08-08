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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include "sidplay/sidplay2.h"
#include "sidplay/builders/resid.h"
extern "C" {
#include "codec.h"
#include "playlist.h"
#include "csid.h"
#include "md5/md5.h"
#include "common.h"
#include "playback.h"
}

static inline void
le_int16 (int16_t in, char *out) {
    char *pin = (char *)&in;
#if !BIGENDIAN
    out[0] = pin[0];
    out[1] = pin[1];
#else
    out[1] = pin[0];
    out[0] = pin[1];
#endif
}

static sidplay2 *sidplay;
static ReSIDBuilder *resid;
static SidTune *tune;
static uint32_t csid_voicemask = 0;
// SLDB support costs ~1M!!!
// current hvsc sldb size is ~35k songs
#define SLDB_MAX_SONGS 40000
// ~50k subsongs in current sldb
#define SLDB_POOL_SIZE 55000
static uint8_t sldb_digests[SLDB_MAX_SONGS][16];
static int16_t sldb_pool[SLDB_POOL_SIZE];
static int sldb_poolmark;
static int16_t *sldb_lengths[SLDB_MAX_SONGS];
static int sldb_size;
static int sldb_loaded;
static const char *sldb_fname = "/home/waker/hvsc/C64Music/DOCUMENTS/Songlengths.txt";

static void sldb_load(const char *fname)
{
    if (sldb_loaded) {
        return;
    }
    sldb_loaded = 1;
    FILE *fp = fopen (fname, "r");
    if (!fp) {
        return;
    }
    char str[1024];

    int line = 1;
    if (fgets (str, 1024, fp) != str) {
        goto fail; // eof
    }
    if (strncmp (str, "[Database]", 10)) {
        goto fail; // bad format
    }

    while (fgets (str, 1024, fp) == str) {
        if (sldb_size >= SLDB_MAX_SONGS) {
            printf ("sldb loader ran out of memory.\n");
            break;
        }
        line++;
        if (str[0] == ';') {
//            printf ("reading songlength for %s", str);
            continue; // comment
        }
        // read/validate md5
        const char *p = str;
        uint8_t digest[16];
        int sz = 0;
        char byte[3];
        while (*p && sz < 16) {
            byte[0] = tolower (*p);
            if (!((byte[0] >= '0' && byte[0] <= '9') || (byte[0] >= 'a' && byte[0] <= 'f'))) {
                printf ("invalid byte 0 in md5: %c\n", byte[0]);
                break;
            }
            p++;
            if (!(*p)) {
                break;
            }
            byte[1] = tolower (*p);
            if (!((byte[1] >= '0' && byte[1] <= '9') || (byte[1] >= 'a' && byte[1] <= 'f'))) {
                printf ("invalid byte 1 in md5: %c\n", byte[1]);
                break;
            }
            byte[2] = 0;
            p++;

            // convert from ascii hex to uint8_t
            if (byte[1] < 'a') {
                digest[sz] = byte[1] - '0';
            }
            else {
                digest[sz] = byte[1] - 'a' + 10;
            }
            if (byte[0] < 'a') {
                digest[sz] |= (byte[0] - '0') << 4;
            }
            else {
                digest[sz] |= (byte[0] - 'a' + 10) << 4;
            }
            sz++;
        }
        if (sz < 16) {
            printf ("bad md5 (sz=%d, line=%d)\n", sz, line);
            continue; // bad song md5
        }
//        else {
//            printf ("digest: ");
//            for (int j = 0; j < 16; j++) {
//                printf ("%02x", (int)digest[j]);
//            }
//            printf ("\n");
//            exit (0);
//        }
        memcpy (sldb_digests[sldb_size], digest, 16);
        sldb_lengths[sldb_size] = &sldb_pool[sldb_poolmark];
        sldb_size++;
        // check '=' sign
        if (*p != '=') {
            continue; // no '=' sign
        }
        p++;
        if (!(*p)) {
            continue; // unexpected eol
        }
        int subsong = 0;
        while (*p >= ' ') {
            // read subsong lengths until eol
            char timestamp[7]; // up to MMM:SS
            sz = 0;
            while (*p > ' ' && *p != '(' && sz < 7) {
                timestamp[sz++] = *p;
                p++;
            }
            if (sz < 4 || sz == 6 && *p > ' ' && *p != '(') {
                break; // bad timestamp
            }
            timestamp[sz] = 0;
            // check for unknown time
            int16_t time = -1;
            if (!strcmp (timestamp, "-:--")) {
                time = -1;
            }
            else {
                // parse timestamp
                const char *colon = strchr (timestamp, ':');
                if (!colon) {
                    break; // bad timestamp
                }
                // minute
                char minute[4];
                strncpy (minute, timestamp, colon-timestamp);
                minute[colon-timestamp] = 0;
                // second
                char second[3];
                strncpy (second, colon+1, 3);
                //printf ("subsong %d, time %s:%s\n", subsong, minute, second);
                time = atoi (minute) * 60 + atoi (second);
            }
            if (sldb_poolmark >= SLDB_POOL_SIZE) {
                printf ("sldb ran out of memory\n");
                goto fail;
            }
            
            sldb_lengths[sldb_size-1][subsong] = time;
            sldb_poolmark++;
            subsong++;

            // prepare for next timestamp
            if (*p == '(') {
                // skip until next whitespace
                while (*p > ' ') {
                    p++;
                }
            }
            if (*p < ' ') {
                break; // eol
            }
            // skip white spaces
            while (*p == ' ') {
                p++;
            }
            if (*p < ' ') {
                break; // eol
            }
        }
    }

fail:
    fclose (fp);
    printf ("HVSC sldb loaded %d songs, %d subsongs total\n", sldb_size, sldb_poolmark);
}

static int
sldb_find (const uint8_t *digest) {
    for (int i = 0; i < sldb_size; i++) {
        if (!memcmp (digest, sldb_digests[i], 16)) {
            return i;
        }
    }
    return -1;
}

extern "C" int
csid_init (const char *fname, int track, float start, float end) {
    sidplay = new sidplay2;
    resid = new ReSIDBuilder ("wtf");
    resid->create (sidplay->info ().maxsids);
//    resid->create (1);
    resid->filter (true);
    resid->sampling (p_get_rate ());
    tune = new SidTune (fname);
#if 0
    // calc md5
    uint8_t sig[16];
    md5_t md5;
    md5_init (&md5);
    md5_process (&md5, (const char *)tune->cache.get () + tune->fileOffset, tune->getInfo ().c64dataLen);
    char tmp[2];
    le_int16 (tune->getInfo ().initAddr, tmp);
    md5_process (&md5, tmp, 2);
    le_int16 (tune->getInfo ().playAddr, tmp);
    md5_process (&md5, tmp, 2);
    le_int16 (tune->getInfo ().songs, tmp);
    md5_process (&md5, tmp, 2);
    for (int s = 1; s <= tune->getInfo ().songs; s++)
    {
        tune->selectSong (s);
        // songspeed is uint8_t, so no need for byteswap
        md5_process (&md5, &tune->getInfo ().songSpeed, 1);
    }
    if (tune->getInfo ().clockSpeed == SIDTUNE_CLOCK_NTSC) {
        md5_process (&md5, &tune->getInfo ().clockSpeed, sizeof (tune->getInfo ().clockSpeed));
    }
    md5_finish (&md5, sig);
#endif

    tune->selectSong (track+1);
    csid.info.channels = tune->isStereo () ? 2 : 1;
    sid2_config_t conf;
    conf = sidplay->config ();
    conf.frequency = p_get_rate ();
    conf.precision = 16;
    conf.playback = csid.info.channels == 2 ? sid2_stereo : sid2_mono;
    conf.sidEmulation = resid;
    conf.optimisation = 0;
    sidplay->config (conf);
    sidplay->load (tune);
    csid.info.bitsPerSample = 16;
    csid.info.samplesPerSecond = p_get_rate ();
    csid.info.position = 0;

    int maxsids = sidplay->info ().maxsids;
    for (int k = 0; k < maxsids; k++) {
        sidemu *emu = resid->getsidemu (k);
        if (emu) {
            for (int i = 0; i < 3; i++) {
                bool mute = csid_voicemask & (1 << i) ? true : false;
                emu->voice (i, mute ? 0x00 : 0xff, mute);
            }
        }
    }
    return 0;
}

extern "C" void
csid_free (void) {
    delete sidplay;
    sidplay = 0;
    delete resid;
    resid = 0;
    delete tune;
    tune = 0;
}

extern "C" int
csid_read (char *bytes, int size) {
    if (csid.info.position > playlist_current.duration) {
        return 0;
    }
    int rd = sidplay->play (bytes, size/csid.info.channels);
    csid.info.position += size/csid.info.channels/2 / (float)csid.info.samplesPerSecond;
    return rd * csid.info.channels;
}

extern "C" int
csid_seek (float time) {
    float t = time;
    if (t < csid.info.position) {
        // reinit
        sidplay->load (tune);
    }
    else {
        t -= csid.info.position;
    }
    resid->filter (false);
    int samples = t * csid.info.samplesPerSecond;
    samples *= 2 * csid.info.channels;
    uint16_t buffer[4096 * csid.info.channels];
    while (samples > 0) {
        int n = min (samples, 4096) * csid.info.channels;
        int done = sidplay->play (buffer, n);
        if (done < n) {
            printf ("sid seek failure\n");
            return -1;
        }
        samples -= done;
    }
    resid->filter (true);
    csid.info.position = time;

    return 0;
}

static const char *
convstr (const char* str) {
    int sz = strlen (str);
    static char out[2048];
    const char *enc = "iso8859-1";
    iconv_t cd = iconv_open ("utf8", enc);
    if (!cd) {
        printf ("unknown encoding: %s\n", enc);
        return NULL;
    }
    else {
        size_t inbytesleft = sz;
        size_t outbytesleft = 2047;
        char *pin = (char*)str;
        char *pout = out;
        memset (out, 0, sizeof (out));
        size_t res = iconv (cd, &pin, &inbytesleft, &pout, &outbytesleft);
        iconv_close (cd);
    }
    return out;
}

extern "C" playItem_t *
csid_insert (playItem_t *after, const char *fname) {
    sldb_load(sldb_fname);
    SidTune *tune;
    tune = new SidTune (fname);
    int tunes = tune->getInfo ().songs;

    uint8_t sig[16];
    md5_t md5;
    md5_init (&md5);
    md5_process (&md5, (const char *)tune->cache.get () + tune->fileOffset, tune->getInfo ().c64dataLen);
    char tmp[2];
    le_int16 (tune->getInfo ().initAddr, tmp);
    md5_process (&md5, tmp, 2);
    le_int16 (tune->getInfo ().playAddr, tmp);
    md5_process (&md5, tmp, 2);
    le_int16 (tune->getInfo ().songs, tmp);
    md5_process (&md5, tmp, 2);
    for (int s = 1; s <= tunes; s++)
    {
        tune->selectSong (s);
        // songspeed is uint8_t, so no need for byteswap
        md5_process (&md5, &tune->getInfo ().songSpeed, 1);
    }
    if (tune->getInfo ().clockSpeed == SIDTUNE_CLOCK_NTSC) {
        md5_process (&md5, &tune->getInfo ().clockSpeed, sizeof (tune->getInfo ().clockSpeed));
    }
    md5_finish (&md5, sig);

    sldb_load(sldb_fname);
    int song = -1;
    if (sldb_loaded) {
        song = sldb_find (sig);
    }

    for (int s = 0; s < tunes; s++) {
        if (tune->selectSong (s+1)) {
            playItem_t *it = (playItem_t*)malloc (sizeof (playItem_t));
            memset (it, 0, sizeof (playItem_t));
            it->codec = &csid;
            it->fname = strdup (fname);
            it->tracknum = s;
            it->timestart = 0;
            it->timeend = 0;
            SidTuneInfo sidinfo;
            tune->getInfo (sidinfo);
            int i = sidinfo.numberOfInfoStrings;
            int title_added = 0;
            if (i >= 1 && sidinfo.infoString[0] && sidinfo.infoString[0][0]) {
                const char *meta;
                if (sidinfo.songs > 1) {
                    meta = "album";
                }
                else {
                    meta = "title";
                    title_added = 1;
                }
                ps_add_meta (it, meta, convstr (sidinfo.infoString[0]));
            }
            if (i >= 2 && sidinfo.infoString[1] && sidinfo.infoString[1][0]) {
                ps_add_meta (it, "artist", convstr (sidinfo.infoString[1]));
            }
            if (i >= 3 && sidinfo.infoString[2] && sidinfo.infoString[2][0]) {
                ps_add_meta (it, "copyright", convstr (sidinfo.infoString[2]));
            }

            for (int j = 3; j < i; j++)
            {
                if (sidinfo.infoString[j] && sidinfo.infoString[j][0]) {
                    ps_add_meta (it, "info", convstr (sidinfo.infoString[j]));
                }
            }
            char trk[10];
            snprintf (trk, 10, "%d", s+1);
            ps_add_meta (it, "track", trk);
            if (!title_added) {
                ps_add_meta (it, "title", NULL);
            }

            float length = 120;
            sldb_load(sldb_fname);
            if (sldb_loaded) {
                if (song >= 0 && sldb_lengths[song][s] >= 0) {
                    length = sldb_lengths[song][s];
                }
                //        if (song < 0) {
                //            printf ("song %s not found in db, md5: ", fname);
                //            for (int j = 0; j < 16; j++) {
                //                printf ("%02x", (int)sig[j]);
                //            }
                //            printf ("\n");
                //        }
            }
            it->duration = length;
            it->filetype = "SID";

            after = ps_insert_item (after, it);
        }
    }
    delete tune;
    return after;
}

static const char *exts[]=
{
	"sid",NULL
};

extern "C" const char **csid_getexts (void) {
    return exts;
}

int
csid_numvoices (void) {
    return 3;
}

void
csid_mutevoice (int voice, int mute) {
    csid_voicemask &= ~ (1<<voice);
    csid_voicemask |= ((mute ? 1 : 0) << voice);
    if (resid) {
        int maxsids = sidplay->info ().maxsids;
        for (int k = 0; k < maxsids; k++) {
            sidemu *emu = resid->getsidemu (k);
            if (emu) {
                for (int i = 0; i < 3; i++) {
                    bool mute = csid_voicemask & (1 << i) ? true : false;
                    emu->voice (i, mute ? 0x00 : 0xff, mute);
                }
            }
        }
    }
}

codec_t csid = {
    csid_init,
    csid_free,
    csid_read,
    csid_seek,
    csid_insert,
    csid_getexts,
    csid_numvoices,
    csid_mutevoice
};

