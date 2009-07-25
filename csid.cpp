#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iconv.h>
#include "sidplay/sidplay2.h"
#include "sidplay/builders/resid.h"
extern "C" {
#include "codec.h"
#include "playlist.h"
}
#include "csid.h"

static sidplay2 *sidplay;
static ReSIDBuilder *resid;
static SidTune *tune;
extern int sdl_player_freq; // hack!

extern "C" int
csid_init (const char *fname, int track, float start, float end) {
    sidplay = new sidplay2;
    resid = new ReSIDBuilder ("wtf");
    resid->create (sidplay->info ().maxsids);
    resid->filter (true);
    resid->sampling (sdl_player_freq);
    tune = new SidTune (fname);
    tune->selectSong (track+1);
    csid.info.channels = tune->isStereo () ? 2 : 1;
    sid2_config_t conf;
    conf = sidplay->config ();
    conf.frequency = sdl_player_freq;
    conf.precision = 16;
    conf.playback = csid.info.channels == 2 ? sid2_stereo : sid2_mono;
    conf.sidEmulation = resid;
    conf.optimisation = 0;
    sidplay->config (conf);
    sidplay->load (tune);
    csid.info.bitsPerSample = 16;
    csid.info.samplesPerSecond = sdl_player_freq;
    csid.info.duration = 120;
    csid.info.position = 0;

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
    int rd = sidplay->play (bytes, size/csid.info.channels);
    return rd * csid.info.channels;
}

extern "C" int
csid_seek (float time) {
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

extern "C" int
csid_add (const char *fname) {
    SidTune *tune;
    tune = new SidTune (fname);
    int tunes = tune->getInfo ().songs;
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
            int i = sidinfo.numberOfCommentStrings;
            if (i >= 1 && sidinfo.infoString[0] && sidinfo.infoString[0][0]) {
                ps_add_meta (it, sidinfo.songs > 1 ? "album" : "title", convstr (sidinfo.infoString[0]));
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
            ps_append_item (it);
        }
    }
    delete tune;
    return 0;
}

static const char * exts[]=
{
	"sid",NULL
};

extern "C" const char **csid_getexts (void) {
    return exts;
}

codec_t csid = {
    csid_init,
    csid_free,
    csid_read,
    csid_seek,
    csid_add,
    csid_getexts
};

