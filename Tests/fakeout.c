/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2018 Oleksiy Yakovenko and other contributors

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

#include <stdint.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <deadbeef/deadbeef.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_output_t plugin;
static DB_functions_t *deadbeef;

static intptr_t fakeout_tid;
static int fakeout_terminate;
static int state;

// in manual mode, output will run manually through calling fakeout_consume
static int _manual;

// in realtime mode, simulate real time audio playback with sleeps calculated from bytes / samplerate
static int _realtime;

static int
fakeout_callback (char *stream, int len);

static void
fakeout_thread (void *context);

static int
fakeout_init (void);

static int
fakeout_free (void);

int
fakeout_setformat (ddb_waveformat_t *fmt);

static int
fakeout_play (void);

static int
fakeout_stop (void);

static int
fakeout_pause (void);

static int
fakeout_unpause (void);

int
fakeout_init (void) {
    trace ("fakeout_init\n");
    state = DDB_PLAYBACK_STATE_STOPPED;
    if (!_manual) {
        fakeout_terminate = 0;
        fakeout_tid = deadbeef->thread_start (fakeout_thread, NULL);
    }
    return 0;
}

int
fakeout_setformat (ddb_waveformat_t *fmt) {
    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    return 0;
}

int
fakeout_free (void) {
    trace ("fakeout_free\n");
    if (_manual) {
        return 0;
    }
    if (!fakeout_terminate) {
        if (fakeout_tid) {
            fakeout_terminate = 1;
            deadbeef->thread_join (fakeout_tid);
        }
        fakeout_tid = 0;
        state = DDB_PLAYBACK_STATE_STOPPED;
        fakeout_terminate = 0;
    }
    return 0;
}

int
fakeout_play (void) {
    if (!fakeout_tid && !_manual) {
        fakeout_init ();
    }
    state = DDB_PLAYBACK_STATE_PLAYING;
    return 0;
}

int
fakeout_stop (void) {
    state = DDB_PLAYBACK_STATE_STOPPED;
    deadbeef->streamer_reset (1);
    fakeout_free();
    return 0;
}

int
fakeout_pause (void) {
    if (state == DDB_PLAYBACK_STATE_STOPPED) {
        return -1;
    }
    // set pause state
    state = DDB_PLAYBACK_STATE_PAUSED;
    return 0;
}

int
fakeout_unpause (void) {
    // unset pause state
    if (state == DDB_PLAYBACK_STATE_PAUSED) {
        state = DDB_PLAYBACK_STATE_PLAYING;
    }
    return 0;
}

static void
fakeout_thread (void *context) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-fakeout", 0, 0, 0, 0);
#endif

    char buf[4096];

    for (;;) {
        if (fakeout_terminate) {
            break;
        }

        if (state != DDB_PLAYBACK_STATE_PLAYING) {
            usleep (10000);
            continue;
        }

        fakeout_callback (buf, sizeof (buf));
    }
}

static int
fakeout_callback (char *stream, int len) {
    if (!deadbeef->streamer_ok_to_read (len)) {
        memset (stream, 0, len);
        return 0; // NOTE: this is not what real output plugins should do, but better for testing
    }
    int bytesread = deadbeef->streamer_read (stream, len);

    return bytesread;
}

ddb_playback_state_t
fakeout_get_state (void) {
    return state;
}

int
fakeout_plugin_start (void) {
    return 0;
}

int
fakeout_plugin_stop (void) {
    return 0;
}

DB_plugin_t *
fakeout_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_output_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "fakeout",
    .plugin.name = "Fake output plugin",
    .plugin.start = fakeout_plugin_start,
    .plugin.stop = fakeout_plugin_stop,
    .init = fakeout_init,
    .free = fakeout_free,
    .setformat = fakeout_setformat,
    .play = fakeout_play,
    .stop = fakeout_stop,
    .pause = fakeout_pause,
    .unpause = fakeout_unpause,
    .state = fakeout_get_state,
    .fmt = {.samplerate = 44100, .channels = 2, .bps = 32, .is_float = 1, .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT}
};

/////////////////////

void
fakeout_set_manual (int manual) {
    _manual = manual;
}

void
fakeout_consume (int nbytes) {
    // for now, always assume playing state
    char buf[4096];
    while (nbytes > 0) {
        int n = sizeof (buf);
        if (n > nbytes) {
            n = nbytes;
        }
        int rb = fakeout_callback (buf, n);
        if (rb > 0) {
            nbytes -= rb;
        }
        if (_realtime) {
            usleep ((useconds_t)((uint64_t)n * 1000 / (44100 * 4) * 2));
        }
    }
}

void
fakeout_set_realtime (int realtime) {
    _realtime = realtime;
}
