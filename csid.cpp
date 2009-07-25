#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
    tune->selectSong (0);
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

extern "C" int
csid_add (const char *fname) {
    playItem_t *it = (playItem_t*)malloc (sizeof (playItem_t));
    memset (it, 0, sizeof (playItem_t));
    it->codec = &csid;
    it->fname = strdup (fname);
    it->tracknum = 0;
    it->timestart = 0;
    it->timeend = 0;
    ps_append_item (it);
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

