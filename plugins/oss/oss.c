/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <stdint.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <soundcard.h>
#include <fcntl.h>
#include <stdlib.h>
#include "../../deadbeef.h"

#if OSS_VERSION<0x040000
#error oss4 plugin: at least oss v4.0 is required to build this plugin
#endif

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_output_t plugin;
DB_functions_t *deadbeef;

static intptr_t oss_tid;
static int oss_terminate;
static int oss_rate = 44100;
static int state;
static int fd;
static uintptr_t mutex;

static void
oss_thread (void *context);

static void
oss_callback (char *stream, int len);

static int
oss_init (void) {
    trace ("oss_init\n");
    state = OUTPUT_STATE_STOPPED;
    oss_terminate = 0;
    mutex = 0;

    // prepare oss for playback
    const char *name = "/dev/dsp";
    fd = open (name, O_WRONLY);
    if (fd == -1) {
        trace ("oss: failed to open file\n");
        perror (name);
        plugin.free ();
        return -1;
    }

    int fmt = AFMT_S16_NE;
    if (ioctl (fd, SNDCTL_DSP_SETFMT, &fmt) == -1) {
        trace ("oss: failed to set format\n");
        perror ("SNDCTL_DSP_SETFMT");
        plugin.free ();
        return -1;
    }

    if (fmt != AFMT_S16_NE) {
        fprintf (stderr, "oss: device doesn't support 16 bit sample format\n");
        plugin.free ();
        return -1;
    }

    int channels = 2;
    if (ioctl (fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        trace ("oss: failed to set channels\n");
        perror ("SNDCTL_DSP_CHANNELS");
        plugin.free ();
        return -1;
    }
    if (channels != 2) {
        trace ("oss: device doesn't support stereo output\n");
        plugin.free ();
        return -1;
    }

    if (ioctl (fd, SNDCTL_DSP_SPEED, &oss_rate) == -1) {
        trace ("oss: failed to set samplerate\n");
        perror ("SNDCTL_DSP_CHANNELS");
        plugin.free ();
        return -1;
    }

    trace ("oss: samplerate: %d\n", oss_rate);

    mutex = deadbeef->mutex_create ();

    oss_tid = deadbeef->thread_start (oss_thread, NULL);
    return 0;
}

static int
oss_change_rate (int rate) {
    if (!fd) {
        oss_rate = rate;
        return oss_rate;
    }
    if (rate == oss_rate) {
        trace ("oss_change_rate: same rate (%d), ignored\n", rate);
        return rate;
    }
    deadbeef->mutex_lock (mutex);
    if (ioctl (fd, SNDCTL_DSP_SPEED, &rate) == -1) {
        trace ("oss: can't switch to %d samplerate\n", rate);
        perror ("SNDCTL_DSP_CHANNELS");
        plugin.free ();
        return -1;
    }
    oss_rate = rate;
    deadbeef->mutex_unlock (mutex);
    return oss_rate;
}

static int
oss_free (void) {
    trace ("oss_free\n");
    if (!oss_terminate) {
        if (oss_tid) {
            oss_terminate = 1;
            deadbeef->thread_join (oss_tid);
        }
        oss_tid = 0;
        state = OUTPUT_STATE_STOPPED;
        oss_terminate = 0;
        if (fd) {
            close (fd);
        }
        if (mutex) {
            deadbeef->mutex_free (mutex);
            mutex = 0;
        }
    }
    return 0;
}

static int
oss_play (void) {
    if (!oss_tid) {
        oss_init ();
    }
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int
oss_stop (void) {
    state = OUTPUT_STATE_STOPPED;
    deadbeef->streamer_reset (1);
    return 0;
}

static int
oss_pause (void) {
    if (state == OUTPUT_STATE_STOPPED) {
        return -1;
    }
    // set pause state
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

static int
oss_unpause (void) {
    // unset pause state
    if (state == OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PLAYING;
    }
    return 0;
}

static int
oss_get_rate (void) {
    return oss_rate;
}

static int
oss_get_bps (void) {
    return 16;
}

static int
oss_get_channels (void) {
    return 2;
}

static int
oss_get_endianness (void) {
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
}

static void
oss_thread (void *context) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-oss", 0, 0, 0, 0);
#endif
    for (;;) {
        if (oss_terminate) {
            break;
        }
        if (state != OUTPUT_STATE_PLAYING) {
            usleep (10000);
            continue;
        }
        
        char buf[1024];
        oss_callback (buf, 1024);
        deadbeef->mutex_lock (mutex);
        int res = write (fd, buf, sizeof (buf));
        deadbeef->mutex_unlock (mutex);
        if (res != sizeof (buf)) {
            fprintf (stderr, "oss: failed to write buffer\n");
        }
        usleep (1000); // this must be here to prevent mutex deadlock
    }
}

static void
oss_callback (char *stream, int len) {
    if (!deadbeef->streamer_ok_to_read (len)) {
        memset (stream, 0, len);
        return;
    }
    int bytesread = deadbeef->streamer_read (stream, len);
    int16_t ivolume = deadbeef->volume_get_amp () * 1000;
    for (int i = 0; i < bytesread/2; i++) {
        ((int16_t*)stream)[i] = (int16_t)(((int32_t)(((int16_t*)stream)[i])) * ivolume / 1000);
    }

    if (bytesread < len) {
        memset (stream + bytesread, 0, len-bytesread);
    }
}

static int
oss_get_state (void) {
    return state;
}

static int
oss_plugin_start (void) {
    return 0;
}

static int
oss_plugin_stop (void) {
    return 0;
}

DB_plugin_t *
oss_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_output_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.nostop = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "oss",
    .plugin.name = "OSS output plugin",
    .plugin.descr = "plays sound via OSS API",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = oss_plugin_start,
    .plugin.stop = oss_plugin_stop,
    .init = oss_init,
    .free = oss_free,
    .change_rate = oss_change_rate,
    .play = oss_play,
    .stop = oss_stop,
    .pause = oss_pause,
    .unpause = oss_unpause,
    .state = oss_get_state,
    .samplerate = oss_get_rate,
    .bitspersample = oss_get_bps,
    .channels = oss_get_channels,
    .endianness = oss_get_endianness,
};
