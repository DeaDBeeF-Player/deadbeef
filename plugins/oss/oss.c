/*
    OSS output plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and contributors

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
#ifdef HAVE_CONFIG_H
#  include "../../config.h"
#endif
#include <stdint.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <deadbeef/deadbeef.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_output_t plugin;
DB_functions_t *deadbeef;

static intptr_t oss_tid;
static int oss_terminate;
static int oss_rate = 44100;
static ddb_playback_state_t state;
static int fd;
static uintptr_t mutex;

static char oss_device[100];

#define BLOCKSIZE 8192

static void
oss_thread (void *context);

static int
oss_callback (char *stream, int len);

int
oss_set_hwparams (ddb_waveformat_t *fmt) {
    int samplefmt;
    switch (fmt->bps) {
    case 8:
        samplefmt = AFMT_S8;
        break;
    case 16:
        samplefmt = AFMT_S16_NE;
        break;
    default:
        samplefmt = AFMT_S16_NE;
        break;
    }
    if (ioctl (fd, SNDCTL_DSP_SETFMT, &samplefmt) == -1) {
        fprintf (stderr, "oss: failed to set format (return: %d)\n", samplefmt);
        perror ("SNDCTL_DSP_SETFMT");
        return -1;
    }

    int channels = fmt->channels;
    if (ioctl (fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        if (channels != 2) {
            fprintf (stderr, "oss: failed to set %d channels, trying fallback to stereo\n", fmt->channels);
            channels = 2;
            if (ioctl (fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
                fprintf (stderr, "oss: stereo fallback failed\n");
                perror ("SNDCTL_DSP_CHANNELS");
                return -1;
            }
        }
        else {
                fprintf (stderr, "oss: failed to set %d channels\n", fmt->channels);
                perror ("SNDCTL_DSP_CHANNELS");
                return -1;
        }
    }
    int rate = fmt->samplerate;
    if (ioctl (fd, SNDCTL_DSP_SPEED, &rate) == -1) {
        fprintf (stderr, "oss: can't switch to %d samplerate\n", rate);
        perror ("SNDCTL_DSP_CHANNELS");
        return -1;
    }

    plugin.fmt.samplerate = rate;
    plugin.fmt.channels = channels;
    plugin.fmt.is_float = 0;
    switch (samplefmt) {
    case AFMT_S8:
        plugin.fmt.bps = 8;
        break;
    case AFMT_S16_LE:
    case AFMT_S16_BE:
        plugin.fmt.bps = 16;
        break;
    default:
        fprintf (stderr, "oss: unsupported output format: 0x%X\n", samplefmt);
        return -1;
    }
    plugin.fmt.channelmask = 0;
    for (int i = 0; i < plugin.fmt.channels; i++) {
        plugin.fmt.channelmask |= 1 << i;
    }

    return 0;
}

static int
oss_init (void) {
    trace ("oss_init\n");
    state = DDB_PLAYBACK_STATE_STOPPED;
    oss_terminate = 0;
    mutex = 0;

    // prepare oss for playback
    fd = open (oss_device, O_WRONLY);
    if (fd == -1) {
        fprintf (stderr, "oss: failed to open file %s\n", oss_device);
        perror (oss_device);
        plugin.free ();
        return -1;
    }

    if (!plugin.fmt.channels) {
        // generic format
        plugin.fmt.bps = 16;
        plugin.fmt.is_float = 0;
        plugin.fmt.channels = 2;
        plugin.fmt.samplerate = 44100;
        plugin.fmt.channelmask = 3;
        plugin.fmt.flags &= ~DDB_WAVEFORMAT_FLAG_IS_DOP;
    }

    oss_set_hwparams (&plugin.fmt);

    mutex = deadbeef->mutex_create ();

    oss_tid = deadbeef->thread_start (oss_thread, NULL);
    return 0;
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
        state = DDB_PLAYBACK_STATE_STOPPED;
        oss_terminate = 0;
        if (fd) {
            close (fd);
            fd = 0;
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
        if (oss_init () < 0) {
            return -1;
        }
    }
    state = DDB_PLAYBACK_STATE_PLAYING;
    return 0;
}

static int
oss_stop (void) {
    state = DDB_PLAYBACK_STATE_STOPPED;
    deadbeef->streamer_reset (1);
    return 0;
}

static int
oss_pause (void) {
    if (state == DDB_PLAYBACK_STATE_STOPPED) {
        return -1;
    }
    state = DDB_PLAYBACK_STATE_PAUSED;
    return 0;
}


static int
oss_setformat (ddb_waveformat_t *fmt) {
    trace ("oss_setformat\n");
    if (!fd) {
        memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    }
    if (!memcmp (fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        return 0;
    }

    int _state = state;

    deadbeef->mutex_lock (mutex);

    if (fd) {
        close (fd);
        fd = 0;
    }
    fd = open (oss_device, O_WRONLY);
    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    if (0 != oss_set_hwparams (fmt)) {
        deadbeef->mutex_unlock (mutex);
        return -1;
    }

    deadbeef->mutex_unlock (mutex);

    switch (_state) {
    case DDB_PLAYBACK_STATE_STOPPED:
        return oss_stop ();
    case DDB_PLAYBACK_STATE_PLAYING:
        return oss_play ();
    case DDB_PLAYBACK_STATE_PAUSED:
        if (0 != oss_play ()) {
            return -1;
        }
        if (0 != oss_pause ()) {
            return -1;
        }
        break;
    }
    return 0;
}

static int
oss_unpause (void) {
    oss_play ();
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

        int sample_size = plugin.fmt.channels * (plugin.fmt.bps / 8);

        if (state != DDB_PLAYBACK_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1) || sample_size == 0) {
            usleep (10000);
            continue;
        }

        int res = 0;
        
        int bs = BLOCKSIZE;
        int mod = bs % sample_size;
        if (mod > 0) {
            bs -= mod;
        }
        char buf[bs];

        int write_size = oss_callback (buf, sizeof (buf));
        deadbeef->mutex_lock (mutex);
        if ( write_size > 0 ) {
            res = write (fd, buf, write_size);
        }

        deadbeef->mutex_unlock (mutex);
//        if (res != write_size) {
//            perror ("oss write");
//            fprintf (stderr, "oss: failed to write buffer\n");
//        }
        usleep (1000); // this must be here to prevent mutex deadlock
    }
}

static int
oss_callback (char *stream, int len) {
    return deadbeef->streamer_read (stream, len);
}

static ddb_playback_state_t
oss_get_state (void) {
    return state;
}

static int
oss_configchanged (void) {
    deadbeef->conf_lock ();
    const char *dev = deadbeef->conf_get_str_fast ("oss.device", "/dev/dsp");
    if (strcmp (dev, oss_device)) {
        strncpy (oss_device, dev, sizeof (oss_device)-1);
        trace ("oss: config option changed, restarting\n");
        deadbeef->sendmessage (DB_EV_REINIT_SOUND, 0, 0, 0);
    }
    deadbeef->conf_unlock ();
    return 0;
}

static int
oss_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        oss_configchanged ();
        break;
    }
    return 0;
}

static int
oss_plugin_start (void) {
    deadbeef->conf_get_str ("oss.device", "/dev/dsp", oss_device, sizeof (oss_device));
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

static const char settings_dlg[] =
    "property \"Device file\" entry oss.device /dev/dsp;\n";

// define plugin interface
static DB_output_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "oss",
    .plugin.name = "OSS output plugin",
    .plugin.descr = "plays sound via OSS API",
    .plugin.copyright = 
        "OSS output plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko and contributors\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = oss_plugin_start,
    .plugin.stop = oss_plugin_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.message = oss_message,
    .init = oss_init,
    .free = oss_free,
    .setformat = oss_setformat,
    .play = oss_play,
    .stop = oss_stop,
    .pause = oss_pause,
    .unpause = oss_unpause,
    .state = oss_get_state,
    .fmt = {-1},
};
