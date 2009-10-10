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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "sidplay/sidplay2.h"
#include "sidplay/builders/resid.h"
// #include "md5/MD5.h" // include those 2 files if you want to use md5 impl from libsidplay2
// #include "sidplay/sidendian.h"

#include "deadbeef.h"

extern "C" {
#include "md5/md5.h"
#include "conf.h"
}

// forward decls
extern "C" {
    int csid_init (DB_playItem_t *it);
    void csid_free (void);
    int csid_read (char *bytes, int size);
    int csid_seek (float time);
    DB_playItem_t *csid_insert (DB_playItem_t *after, const char *fname);
    int csid_numvoices (void);
    void csid_mutevoice (int voice, int mute);
    int csid_stop (void);
}

static const char *exts[] = { "sid",NULL };
const char *filetypes[] = { "SID", NULL };

// define plugin interface
static DB_decoder_t plugin = {
    { // plugin
        // C macro won't work here, so do it by hand
        /* .plugin.type = */DB_PLUGIN_DECODER,
        /* .api_vmajor = */DB_API_VERSION_MAJOR,
        /* .api_vminor = */DB_API_VERSION_MINOR,
        /* .plugin.version_major = */0,
        /* .plugin.version_minor = */1,
        /* .inactive = */0,
        /* .plugin.name = */"SID decoder",
        /* .plugin.descr = */"based on libsidplay2",
        /* .plugin.author = */"Alexey Yakovenko",
        /* .plugin.email = */"waker@users.sourceforge.net",
        /* .plugin.website = */"http://deadbeef.sf.net",
        /* .plugin.start = */NULL,
        /* .plugin.stop = */csid_stop,
        /* .plugin.exec_cmdline = */NULL,
    },
    { // info
        /* .info.bps = */0,
        /* .info.channels = */0,
        /* .info.samplerate = */0,
        /* .info.readpos = */0,
    },
    /* .init = */csid_init,
    /* .free = */csid_free,
    /* .read_int16 = */csid_read,
    /* .read_float32 = */NULL,
    /* .seek = */csid_seek,
    /* .seek_sample = */NULL,
    /* .insert = */csid_insert,
    /* .numvoices = */csid_numvoices,
    /* .mutevoice = */csid_mutevoice,
    /* .exts = */exts,
    /* .filetypes = */filetypes,
    /* .id = */"stdsid"
};

static DB_functions_t *deadbeef;

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

static inline void
le_int16 (int16_t in, unsigned char *out) {
    char *pin = (char *)&in;
#if !WORDS_BIGENDIAN
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
static float duration; // of the current song
static uint32_t csid_voicemask = 0;

// SLDB support costs ~1M!!!
// current hvsc sldb size is ~35k songs
#define SLDB_MAX_SONGS 40000
// ~50k subsongs in current sldb
#define SLDB_POOL_SIZE 55000
typedef struct {
    uint8_t sldb_digests[SLDB_MAX_SONGS][16];
    int16_t sldb_pool[SLDB_POOL_SIZE];
    int sldb_poolmark;
    int16_t *sldb_lengths[SLDB_MAX_SONGS];
    int sldb_size;
} sldb_t;
static int sldb_loaded;
static sldb_t *sldb;

static void sldb_load()
{
    fprintf (stderr, "sldb_load\n");
    int conf_hvsc_enable = conf_get_int ("hvsc_enable", 0);
    if (sldb_loaded || !conf_hvsc_enable) {
        return;
    }
    const char *conf_hvsc_path = conf_get_str ("hvsc_path", NULL);
    if (!conf_hvsc_path) {
        return;
    }
    sldb_loaded = 1;
    const char *fname = conf_hvsc_path;
    FILE *fp = fopen (fname, "r");
    if (!fp) {
        fprintf (stderr, "sid: failed to open file %s\n", fname);
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

    if (!sldb) {
        sldb = (sldb_t *)malloc (sizeof (sldb_t));
        memset (sldb, 0, sizeof (sldb_t));
    }
    while (fgets (str, 1024, fp) == str) {
        if (sldb->sldb_size >= SLDB_MAX_SONGS) {
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
        memcpy (sldb->sldb_digests[sldb->sldb_size], digest, 16);
        sldb->sldb_lengths[sldb->sldb_size] = &sldb->sldb_pool[sldb->sldb_poolmark];
        sldb->sldb_size++;
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
            if (sldb->sldb_poolmark >= SLDB_POOL_SIZE) {
                printf ("sldb ran out of memory\n");
                goto fail;
            }
            
            sldb->sldb_lengths[sldb->sldb_size-1][subsong] = time;
            sldb->sldb_poolmark++;
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
    fprintf (stderr, "HVSC sldb loaded %d songs, %d subsongs total\n", sldb->sldb_size, sldb->sldb_poolmark);
}

static int
sldb_find (const uint8_t *digest) {
    if (!sldb) {
        fprintf (stderr, "sldb not loaded\n");
        return -1;
    }
    for (int i = 0; i < sldb->sldb_size; i++) {
        if (!memcmp (digest, sldb->sldb_digests[i], 16)) {
            return i;
        }
    }
    return -1;
}

extern "C" int
csid_init (DB_playItem_t *it) {
    sidplay = new sidplay2;
    resid = new ReSIDBuilder ("wtf");
    resid->create (sidplay->info ().maxsids);
//    resid->create (1);
    resid->filter (true);
    resid->sampling (deadbeef->playback_get_samplerate ());
    duration = it->duration;
    tune = new SidTune (it->fname);

    tune->selectSong (it->tracknum+1);
    plugin.info.channels = tune->isStereo () ? 2 : 1;
    sid2_config_t conf;
    conf = sidplay->config ();
    conf.frequency = deadbeef->playback_get_samplerate ();
    conf.precision = 16;
    conf.playback = plugin.info.channels == 2 ? sid2_stereo : sid2_mono;
    conf.sidEmulation = resid;
    conf.optimisation = 0;
    sidplay->config (conf);
    sidplay->load (tune);
    plugin.info.bps = 16;
    plugin.info.samplerate = conf.frequency;
    plugin.info.readpos = 0;

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
    if (plugin.info.readpos > duration) {
        return 0;
    }
    int rd = sidplay->play (bytes, size/plugin.info.channels);
    plugin.info.readpos += size/plugin.info.channels/2 / (float)plugin.info.samplerate;

#if WORDS_BIGENDIAN
    // convert samples from le to be
    int n = rd * plugin.info.channels/2;
    int16_t *ptr = (int16_t *)bytes;
    while (n > 0) {
        int16_t out;
        le_int16 (*ptr, (unsigned char *)&out);
        *ptr = out;
        ptr++;
        n--;
    }
#endif
    return rd * plugin.info.channels;
}

extern "C" int
csid_seek (float time) {
    float t = time;
    if (t < plugin.info.readpos) {
        // reinit
        sidplay->load (tune);
    }
    else {
        t -= plugin.info.readpos;
    }
    resid->filter (false);
    int samples = t * plugin.info.samplerate;
    samples *= 2 * plugin.info.channels;
    uint16_t buffer[4096 * plugin.info.channels];
    while (samples > 0) {
        int n = min (samples, 4096) * plugin.info.channels;
        int done = sidplay->play (buffer, n);
        if (done < n) {
            printf ("sid seek failure\n");
            return -1;
        }
        samples -= done;
    }
    resid->filter (true);
    plugin.info.readpos = time;

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

extern "C" DB_playItem_t *
csid_insert (DB_playItem_t *after, const char *fname) {
    sldb_load ();
    SidTune *tune;
    tune = new SidTune (fname);
    int tunes = tune->getInfo ().songs;
    uint8_t sig[16];
    unsigned char tmp[2];
    md5_t md5;
    md5_init (&md5);
    md5_process (&md5, (const char *)tune->cache.get () + tune->fileOffset, tune->getInfo ().c64dataLen);
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
#if 0
    // md5 calc from libsidplay2
    MD5 myMD5;
    myMD5.append ((const char *)tune->cache.get() + tune->fileOffset, tune->getInfo ().c64dataLen);
    // Include INIT and PLAY address.
    endian_little16 (tmp,tune->getInfo ().initAddr);
    myMD5.append    (tmp,sizeof(tmp));
    endian_little16 (tmp,tune->getInfo ().playAddr);
    myMD5.append    (tmp,sizeof(tmp));
    // Include number of songs.
    endian_little16 (tmp,tune->getInfo ().songs);
    myMD5.append    (tmp,sizeof(tmp));
    {   // Include song speed for each song.
        //uint_least16_t currentSong = tune->getInfo ().currentSong;
        for (uint_least16_t s = 1; s <= tune->getInfo ().songs; s++)
        {
            tune->selectSong (s);
            myMD5.append (&tune->getInfo ().songSpeed,1);
        }
        // Restore old song
        //tune->selectSong (currentSong);
    }
    // Deal with PSID v2NG clock speed flags: Let only NTSC
    // clock speed change the MD5 fingerprint. That way the
    // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
    // PSID v2NG format is the same.
    if (tune->getInfo ().clockSpeed == SIDTUNE_CLOCK_NTSC) {
        myMD5.append (&tune->getInfo ().clockSpeed,sizeof(tune->getInfo ().clockSpeed));
    }
    myMD5.finish ();
    memcpy (sig, myMD5.getDigest (), 16);
#endif

    sldb_load ();
    int song = -1;
    if (sldb_loaded) {
        song = sldb_find (sig);
    }

    for (int s = 0; s < tunes; s++) {
        if (tune->selectSong (s+1)) {
            DB_playItem_t *it = deadbeef->pl_item_alloc ();
            it->decoder = &plugin;
            it->fname = strdup (fname);
            it->tracknum = s;
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
                deadbeef->pl_add_meta (it, meta, convstr (sidinfo.infoString[0]));
            }
            if (i >= 2 && sidinfo.infoString[1] && sidinfo.infoString[1][0]) {
                deadbeef->pl_add_meta (it, "artist", convstr (sidinfo.infoString[1]));
            }
            if (i >= 3 && sidinfo.infoString[2] && sidinfo.infoString[2][0]) {
                deadbeef->pl_add_meta (it, "copyright", convstr (sidinfo.infoString[2]));
            }

            for (int j = 3; j < i; j++)
            {
                if (sidinfo.infoString[j] && sidinfo.infoString[j][0]) {
                    deadbeef->pl_add_meta (it, "info", convstr (sidinfo.infoString[j]));
                }
            }
            char trk[10];
            snprintf (trk, 10, "%d", s+1);
            deadbeef->pl_add_meta (it, "track", trk);
            if (!title_added) {
                deadbeef->pl_add_meta (it, "title", NULL);
            }

            float length = 120;
            if (sldb_loaded) {
                if (song >= 0 && sldb->sldb_lengths[song][s] >= 0) {
                    length = sldb->sldb_lengths[song][s];
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

            after = deadbeef->pl_insert_item (after, it);
        }
    }
    delete tune;
    return after;
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

int
csid_stop (void) {
    if (sldb) {
        free (sldb);
        sldb = NULL;
    }
    sldb_loaded = 0;
    return 0;
}

extern "C" DB_plugin_t *
sid_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
