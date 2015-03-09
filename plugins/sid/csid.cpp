/*
    SID plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <ctype.h>
#include <sys/stat.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "sidplay/sidplay2.h"
#include "sidplay/builders/resid.h"
//#include "md5.h"
// #include "sidplay/sidendian.h"

#include "../../deadbeef.h"
#include "csid.h"

extern DB_decoder_t sid_plugin;

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

DB_functions_t *deadbeef;

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

typedef struct {
    DB_fileinfo_t info;
    sidplay2 *sidplay;
    ReSIDBuilder *resid;
    SidTune *tune;
    float duration; // of the current song
} sid_info_t;

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
static int sldb_disable;

static int chip_voices = 0xff;
static int chip_voices_changed = 0;

static int conf_hvsc_enable = 0;

static void
sldb_load()
{
    if (sldb_disable) {
        return;
    }
    trace ("sldb_load\n");
    if (sldb_loaded || !conf_hvsc_enable) {
        sldb_disable = 1;
        return;
    }
    char conf_hvsc_path[1000];
    deadbeef->conf_get_str ("hvsc_path", "", conf_hvsc_path, sizeof (conf_hvsc_path));
    if (!conf_hvsc_path[0]) {
        sldb_disable = 1;
        return;
    }
    sldb_loaded = 1;
    const char *fname = conf_hvsc_path;
    FILE *fp = fopen (fname, "r");
    if (!fp) {
        trace ("sid: failed to open file %s\n", fname);
        sldb_disable = 1;
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
            trace ("sldb loader ran out of memory.\n");
            break;
        }
        line++;
        if (str[0] == ';') {
//            trace ("reading songlength for %s", str);
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
                trace ("invalid byte 0 in md5: %c\n", byte[0]);
                break;
            }
            p++;
            if (!(*p)) {
                break;
            }
            byte[1] = tolower (*p);
            if (!((byte[1] >= '0' && byte[1] <= '9') || (byte[1] >= 'a' && byte[1] <= 'f'))) {
                trace ("invalid byte 1 in md5: %c\n", byte[1]);
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
            trace ("bad md5 (sz=%d, line=%d)\n", sz, line);
            continue; // bad song md5
        }
//        else {
//            trace ("digest: ");
//            for (int j = 0; j < 16; j++) {
//                trace ("%02x", (int)digest[j]);
//            }
//            trace ("\n");
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
                //trace ("subsong %d, time %s:%s\n", subsong, minute, second);
                time = atoi (minute) * 60 + atoi (second);
            }
            if (sldb->sldb_poolmark >= SLDB_POOL_SIZE) {
                trace ("sldb ran out of memory\n");
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
    sldb_disable = 1;
    fclose (fp);
    trace ("HVSC sldb loaded %d songs, %d subsongs total\n", sldb->sldb_size, sldb->sldb_poolmark);
}

static int
sldb_find (const uint8_t *digest) {
    if (!sldb) {
        trace ("sldb not loaded\n");
        return -1;
    }
    for (int i = 0; i < sldb->sldb_size; i++) {
        if (!memcmp (digest, sldb->sldb_digests[i], 16)) {
            return i;
        }
    }
    return -1;
}

DB_fileinfo_t *
csid_open (uint32_t hints) {
    DB_fileinfo_t *_info = (DB_fileinfo_t *)malloc (sizeof (sid_info_t));
    memset (_info, 0, sizeof (sid_info_t));
    return _info;
}

static void
csid_mute_voices (sid_info_t *info, int chip_voices) {
    int maxsids = info->sidplay->info ().maxsids;
    for (int k = 0; k < maxsids; k++) {
        sidemu *emu = info->resid->getsidemu (k);
        if (emu) {
            for (int i = 0; i < 3; i++) {
                bool mute = chip_voices & (1 << i) ? false : true;
                emu->voice (i, mute ? 0x00 : 0xff, mute);
            }
        }
    }
}

int
csid_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    sid_info_t *info = (sid_info_t *)_info;
    
    // libsidplay crashes if file doesn't exist
    // so i have to check it here
    deadbeef->pl_lock ();
    DB_FILE *fp = deadbeef->fopen (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();
    if (!fp ){
        return -1;
    }
    deadbeef->fclose (fp);

    info->sidplay = new sidplay2;
    info->resid = new ReSIDBuilder ("wtf");
    info->resid->create (info->sidplay->info ().maxsids);
    info->resid->filter (true);

    int samplerate = deadbeef->conf_get_int ("sid.samplerate", 44100);
    int bps = deadbeef->conf_get_int ("sid.bps", 16);
    if (bps != 16 && bps != 8) {
        bps = 16;
    }

    info->resid->sampling (samplerate);
    info->duration = deadbeef->pl_get_item_duration (it);
    deadbeef->pl_lock ();
    info->tune = new SidTune (deadbeef->pl_find_meta (it, ":URI"));
    deadbeef->pl_unlock ();

    info->tune->selectSong (deadbeef->pl_find_meta_int (it, ":TRACKNUM", 0)+1);
    sid2_config_t conf;
    conf = info->sidplay->config ();
    conf.frequency = samplerate;
    conf.precision = bps;

    conf.playback = deadbeef->conf_get_int ("sid.mono", 0) ? sid2_mono : sid2_stereo;
    conf.sidEmulation = info->resid;
    conf.optimisation = 0;
    info->sidplay->config (conf);
    info->sidplay->load (info->tune);

    _info->plugin = &sid_plugin;
    _info->fmt.channels = conf.playback == sid2_stereo ? 2 : 1;
    _info->fmt.bps = bps;
    _info->fmt.samplerate = conf.frequency;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);
    _info->readpos = 0;

    chip_voices = deadbeef->conf_get_int ("chip.voices", 0xff);
    csid_mute_voices (info, chip_voices);
    return 0;
}

void
csid_free (DB_fileinfo_t *_info) {
    sid_info_t *info = (sid_info_t *)_info;
    if (info) {
        delete info->sidplay;
        delete info->resid;
        delete info->tune;
        free (info);
    }
}

int
csid_read (DB_fileinfo_t *_info, char *bytes, int size) {
    sid_info_t *info = (sid_info_t *)_info;
    if (_info->readpos > info->duration) {
        return 0;
    }

    if (chip_voices_changed) {
        chip_voices = deadbeef->conf_get_int ("chip.voices", 0xff);
        chip_voices_changed = 0;
        csid_mute_voices (info, chip_voices);
    }

    int rd = info->sidplay->play (bytes, size);

    int samplesize = (_info->fmt.bps>>3) * _info->fmt.channels;

    _info->readpos += rd / samplesize / (float)_info->fmt.samplerate;

    return size;

}

int
csid_seek (DB_fileinfo_t *_info, float time) {
    sid_info_t *info = (sid_info_t *)_info;
    float t = time;
    if (t < _info->readpos) {
        // reinit
        info->sidplay->load (info->tune);
        csid_mute_voices (info, chip_voices);
    }
    else {
        t -= _info->readpos;
    }
    info->resid->filter (false);
    int samples = t * _info->fmt.samplerate;
    samples *= (_info->fmt.bps>>3) * _info->fmt.channels;
    uint16_t buffer[2048 * _info->fmt.channels];
    while (samples > 0) {
        int n = min (samples, 2048) * _info->fmt.channels;
        int done = info->sidplay->play (buffer, n);
        if (done < n) {
            trace ("sid seek failure\n");
            return -1;
        }
        samples -= done;
    }
    info->resid->filter (true);
    _info->readpos = time;

    return 0;
}

static const char *
convstr (const char* str, int sz, char *out, int out_sz) {
    const char *cs = deadbeef->junk_detect_charset (str);
    if (!cs) {
        return str;
    }
    else {
        if (deadbeef->junk_iconv (str, sz, out, out_sz, cs, "utf-8") >= 0) {
            return out;
        }
    }
    return NULL;
}

static void
find_hvsc_path_from_fname (const char *fname) {
    if (conf_hvsc_enable && !sldb_loaded && !sldb_disable) {
        char conf_hvsc_path[1000];
        deadbeef->conf_get_str ("hvsc_path", "", conf_hvsc_path, sizeof (conf_hvsc_path));
        if (!conf_hvsc_path[0]) {
            strcpy (conf_hvsc_path, fname);
            char *p;
            while ((p = strrchr (conf_hvsc_path, '/'))) {
                strcpy (p, "/DOCUMENTS/Songlengths.txt");
                struct stat st;
                int err = stat (conf_hvsc_path, &st);
                if (!err && (st.st_mode & S_IFREG)) {
                    deadbeef->conf_set_str ("hvsc_path", conf_hvsc_path);
                    deadbeef->conf_save ();
                    break;
                }
                *p = 0;
            }
        }
    }
}

extern "C" DB_playItem_t *
csid_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    trace ("inserting %s\n", fname);

    find_hvsc_path_from_fname (fname);

    sldb_load ();
    SidTune *tune;
    trace ("new SidTune\n");
    tune = new SidTune (fname);
    int tunes = tune->getInfo ().songs;
    trace ("subtunes: %d\n", tunes);
    uint8_t sig[16];
    unsigned char tmp[2];
#if 1
    trace ("calculating md5\n");
    DB_md5_t md5;
    deadbeef->md5_init (&md5);
    deadbeef->md5_append (&md5, (const uint8_t *)tune->cache.get () + tune->fileOffset, tune->getInfo ().c64dataLen);
    le_int16 (tune->getInfo ().initAddr, tmp);
    deadbeef->md5_append (&md5, tmp, 2);
    le_int16 (tune->getInfo ().playAddr, tmp);
    deadbeef->md5_append (&md5, tmp, 2);
    le_int16 (tune->getInfo ().songs, tmp);
    deadbeef->md5_append (&md5, tmp, 2);
    for (int s = 1; s <= tunes; s++)
    {
        tune->selectSong (s);
        // songspeed is uint8_t, so no need for byteswap
        deadbeef->md5_append (&md5, &tune->getInfo ().songSpeed, 1);
    }
    if (tune->getInfo ().clockSpeed == SIDTUNE_CLOCK_NTSC) {
        deadbeef->md5_append (&md5, &tune->getInfo ().clockSpeed, sizeof (tune->getInfo ().clockSpeed));
    }
    deadbeef->md5_finish (&md5, sig);
#else
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
    {
        // Include song speed for each song.
        for (uint_least16_t s = 1; s <= tune->getInfo ().songs; s++)
        {
            tune->selectSong (s);
            myMD5.append (&tune->getInfo ().songSpeed,1);
        }
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

    int song = -1;
    if (sldb_loaded) {
        song = sldb_find (sig);
    }

    trace ("inserting tunes...\n");
    for (int s = 0; s < tunes; s++) {
        trace ("select %d...\n", s);
        if (tune->selectSong (s+1)) {
            DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, sid_plugin.plugin.id);
            deadbeef->pl_set_meta_int (it, ":TRACKNUM", s);
            SidTuneInfo sidinfo;
            tune->getInfo (sidinfo);
            int i = sidinfo.numberOfInfoStrings;
            int title_added = 0;
            trace ("set %d metainfo...\n", s);
            char temp[2048];
            if (i >= 1 && sidinfo.infoString[0] && sidinfo.infoString[0][0]) {
                const char *meta;
                if (sidinfo.songs > 1) {
                    meta = "album";
                }
                else {
                    meta = "title";
                    title_added = 1;
                }
                deadbeef->pl_add_meta (it, meta, convstr (sidinfo.infoString[0], strlen (sidinfo.infoString[0]), temp, sizeof (temp)));
            }
            if (i >= 2 && sidinfo.infoString[1] && sidinfo.infoString[1][0]) {
                deadbeef->pl_add_meta (it, "artist", convstr (sidinfo.infoString[1], strlen (sidinfo.infoString[1]), temp, sizeof (temp)));
            }
            if (i >= 3 && sidinfo.infoString[2] && sidinfo.infoString[2][0]) {
                deadbeef->pl_add_meta (it, "copyright", convstr (sidinfo.infoString[2], strlen (sidinfo.infoString[2]), temp, sizeof (temp)));
            }

            for (int j = 3; j < i; j++)
            {
                if (sidinfo.infoString[j] && sidinfo.infoString[j][0]) {
                    deadbeef->pl_add_meta (it, "info", convstr (sidinfo.infoString[j], strlen (sidinfo.infoString[j]), temp, sizeof (temp)));
                }
            }
            char trk[10];
            snprintf (trk, 10, "%d", s+1);
            deadbeef->pl_add_meta (it, "track", trk);
            if (!title_added) {
                deadbeef->pl_add_meta (it, "title", NULL);
            }

            float length = deadbeef->conf_get_float ("sid.defaultlength", 180);
            if (sldb_loaded) {
                if (song >= 0 && sldb->sldb_lengths[song][s] >= 0) {
                    length = sldb->sldb_lengths[song][s];
                }
                //        if (song < 0) {
                //            trace ("song %s not found in db, md5: ", fname);
                //            for (int j = 0; j < 16; j++) {
                //                trace ("%02x", (int)sig[j]);
                //            }
                //            trace ("\n");
                //        }
            }
            deadbeef->plt_set_item_duration (plt, it, length);
            deadbeef->pl_add_meta (it, ":FILETYPE", "SID");

            after = deadbeef->plt_insert_item (plt, after, it);
            deadbeef->pl_item_unref (it);
        }
    }
    trace ("delete sidtune\n");
    delete tune;
    return after;
}

static int
sid_configchanged (void) {
    conf_hvsc_enable = deadbeef->conf_get_int ("hvsc_enable", 0);
    int disable = !conf_hvsc_enable;
    if (disable != sldb_disable) {
        sldb_disable = disable;
    }

    // pick up new sldb filename in case it was changed
    if (sldb) {
        free (sldb);
        sldb = NULL;
        sldb_loaded = 0;
    }

    if (chip_voices != deadbeef->conf_get_int ("chip.voices", 0xff)) {
        chip_voices_changed = 1;
    }

    return 0;
}

int
sid_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        sid_configchanged ();
        break;
    }
    return 0;
}

int
csid_start (void) {
    sid_configchanged ();
    return 0;
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
    return DB_PLUGIN (&sid_plugin);
}

