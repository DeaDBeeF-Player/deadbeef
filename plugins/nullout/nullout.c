/*
    Null output plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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

static intptr_t null_tid;
static int null_terminate;
static int state;

static void
pnull_callback (char *stream, int len);

static void
pnull_thread (void *context);

static int
pnull_init (void);

static int
pnull_free (void);

int
pnull_setformat (ddb_waveformat_t *fmt);

static int
pnull_play (void);

static int
pnull_stop (void);

static int
pnull_pause (void);

static int
pnull_unpause (void);

int
pnull_init (void) {
    trace ("pnull_init\n");
    state = DDB_PLAYBACK_STATE_STOPPED;
    null_terminate = 0;
    null_tid = deadbeef->thread_start (pnull_thread, NULL);
    return 0;
}

int
pnull_setformat (ddb_waveformat_t *fmt) {
    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    return 0;
}

int
pnull_free (void) {
    trace ("pnull_free\n");
    if (!null_terminate) {
        if (null_tid) {
            null_terminate = 1;
            deadbeef->thread_join (null_tid);
        }
        null_tid = 0;
        state = DDB_PLAYBACK_STATE_STOPPED;
        null_terminate = 0;
    }
    return 0;
}

int
pnull_play (void) {
    if (!null_tid) {
        pnull_init ();
    }
    state = DDB_PLAYBACK_STATE_PLAYING;
    return 0;
}

int
pnull_stop (void) {
    state = DDB_PLAYBACK_STATE_STOPPED;
    deadbeef->streamer_reset (1);
    return 0;
}

int
pnull_pause (void) {
    if (state == DDB_PLAYBACK_STATE_STOPPED) {
        return -1;
    }
    // set pause state
    state = DDB_PLAYBACK_STATE_PAUSED;
    return 0;
}

int
pnull_unpause (void) {
    // unset pause state
    if (state == DDB_PLAYBACK_STATE_PAUSED) {
        state = DDB_PLAYBACK_STATE_PLAYING;
    }
    return 0;
}

static void
pnull_thread (void *context) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-null", 0, 0, 0, 0);
#endif
    for (;;) {
        if (null_terminate) {
            break;
        }
        if (state != DDB_PLAYBACK_STATE_PLAYING) {
            usleep (10000);
            continue;
        }
        
        char buf[4096];
        pnull_callback (buf, 1024);
        usleep(1);
    }
}

static void
pnull_callback (char *stream, int len) {
    if (!deadbeef->streamer_ok_to_read (len)) {
        memset (stream, 0, len);
        return;
    }
    int bytesread = deadbeef->streamer_read (stream, len);

    if (bytesread < len) {
        memset (stream + bytesread, 0, len-bytesread);
    }
}

ddb_playback_state_t
pnull_get_state (void) {
    return state;
}

int
null_start (void) {
    return 0;
}

int
null_stop (void) {
    return 0;
}

DB_plugin_t *
nullout_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_output_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "nullout",
    .plugin.name = "Null output plugin",
    .plugin.descr = "This plugin takes the audio data, and discards it,\nso nothing will play.\nThis is useful for testing.",
    .plugin.copyright = 
    "Null output plugin for DeaDBeeF Player\n"
    "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
    "\n"
    "This software is provided 'as-is', without any express or implied\n"
    "warranty.  In no event will the authors be held liable for any damages\n"
    "arising from the use of this software.\n"
    "\n"
    "Permission is granted to anyone to use this software for any purpose,\n"
    "including commercial applications, and to alter it and redistribute it\n"
    "freely, subject to the following restrictions:\n"
    "\n"
    "1. The origin of this software must not be misrepresented; you must not\n"
    " claim that you wrote the original software. If you use this software\n"
    " in a product, an acknowledgment in the product documentation would be\n"
    " appreciated but is not required.\n"
    "\n"
    "2. Altered source versions must be plainly marked as such, and must not be\n"
    " misrepresented as being the original software.\n"
    "\n"
    "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = null_start,
    .plugin.stop = null_stop,
    .init = pnull_init,
    .free = pnull_free,
    .setformat = pnull_setformat,
    .play = pnull_play,
    .stop = pnull_stop,
    .pause = pnull_pause,
    .unpause = pnull_unpause,
    .state = pnull_get_state,
    .fmt = {.samplerate = 44100, .channels = 2, .bps = 16, .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT}
};
