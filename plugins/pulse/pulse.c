/*
    PulseAudio output plugin for DeaDBeeF Player
    Copyright (C) 2011 Jan D. Behrens <zykure@web.de>
    Copyright (C) 2010-2012 Alexey Yakovenko <waker@users.sourceforge.net>
    Copyright (C) 2010 Anton Novikov <tonn.post@gmail.com>

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

#include <pulse/simple.h>

#include <stdint.h>
#include <unistd.h>

#ifdef __linux__
    #include <sys/prctl.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stdout, __VA_ARGS__); }
#define trace(fmt,...)

static DB_output_t plugin;
DB_functions_t * deadbeef;

#define CONFSTR_PULSE_SERVERADDR "pulse.serveraddr"
#define CONFSTR_PULSE_BUFFERSIZE "pulse.buffersize"
#define PULSE_DEFAULT_BUFFERSIZE 4096

static intptr_t pulse_tid;
static int pulse_terminate;

static pa_simple *s;
static pa_sample_spec ss;
static ddb_waveformat_t requested_fmt;
static int state = OUTPUT_STATE_STOPPED;
static uintptr_t mutex;
static int in_callback;

static int buffer_size;

static void pulse_thread(void *context);

static void pulse_callback(char *stream, int len);

static int pulse_init();

static int pulse_free();

static int pulse_setformat(ddb_waveformat_t *fmt);

static int pulse_play();

static int pulse_stop();

static int pulse_pause();

static int pulse_unpause();

static int pulse_set_spec(ddb_waveformat_t *fmt)
{
    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    if (!plugin.fmt.channels) {
        // generic format
        plugin.fmt.bps = 16;
        plugin.fmt.is_float = 0;
        plugin.fmt.channels = 2;
        plugin.fmt.samplerate = 44100;
        plugin.fmt.channelmask = 3;
    }

    trace ("format %dbit %s %dch %dHz channelmask=%X\n", plugin.fmt.bps, plugin.fmt.is_float ? "float" : "int", plugin.fmt.channels, plugin.fmt.samplerate, plugin.fmt.channelmask);

    ss.channels = plugin.fmt.channels;
    // Try to auto-configure the channel map, see <pulse/channelmap.h> for details
    pa_channel_map channel_map;
    pa_channel_map_init_extend(&channel_map, ss.channels, PA_CHANNEL_MAP_WAVEEX);
    trace ("pulse: channels: %d\n", ss.channels);

    // Read samplerate from config
    //ss.rate = deadbeef->conf_get_int(CONFSTR_PULSE_SAMPLERATE, 44100);
    ss.rate = plugin.fmt.samplerate;
    trace ("pulse: samplerate: %d\n", ss.rate);

    switch (plugin.fmt.bps) {
    case 8:
        ss.format = PA_SAMPLE_U8;
        break;
    case 16:
        ss.format = PA_SAMPLE_S16LE;
        break;
    case 24:
        ss.format = PA_SAMPLE_S24LE;
        break;
    case 32:
        if (plugin.fmt.is_float) {
            ss.format = PA_SAMPLE_FLOAT32LE;
        }
        else {
            ss.format = PA_SAMPLE_S32LE;
        }
        break;
    default:
        return -1;
    };

    if (s) {
        pa_simple_free(s);
    }

    pa_buffer_attr * attr = NULL;
    //attr->maxlength = Maximum length of the buffer.
    //attr->tlength = Playback only: target length of the buffer.
    //attr->prebuf = Playback only: pre-buffering.
    //attr->minreq = Playback only: minimum request.
    //attr->fragsize = Recording only: fragment size.

    buffer_size = deadbeef->conf_get_int(CONFSTR_PULSE_BUFFERSIZE, PULSE_DEFAULT_BUFFERSIZE);

    // TODO: where list of all available devices? add this option to config too..
    char * dev = NULL;

    int error;

    // Read serveraddr from config
    deadbeef->conf_lock ();
    const char * server = deadbeef->conf_get_str_fast (CONFSTR_PULSE_SERVERADDR, NULL);

    if (server) {
        server = strcmp(server, "default") ? server : NULL;
    }

    s = pa_simple_new(server, "Deadbeef", PA_STREAM_PLAYBACK, dev, "Music", &ss, &channel_map, attr, &error);
    deadbeef->conf_unlock ();

    if (!s)
    {
        trace ("pulse_init failed (%d)\n", error);
        return -1;
    }

    return 0;
}

static int pulse_init(void)
{
    trace ("pulse_init\n");
    state = OUTPUT_STATE_STOPPED;
    trace ("pulse_terminate=%d\n", pulse_terminate);
    assert (!pulse_terminate);

    if (requested_fmt.samplerate != 0) {
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
    }

    if (0 != pulse_set_spec(&plugin.fmt)) {
        return -1;
    }

    pulse_tid = deadbeef->thread_start(pulse_thread, NULL);

    return 0;
}

static int pulse_setformat (ddb_waveformat_t *fmt)
{
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    if (!s) {
        return -1;
    }
    if (!memcmp (fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        return 0;
    }

    deadbeef->mutex_lock(mutex);
    pulse_set_spec(fmt);
    deadbeef->mutex_unlock(mutex);

    return 0;
}

static int pulse_free(void)
{
    trace("pulse_free\n");

    if (!pulse_tid) {
        return 0;
    }

    deadbeef->mutex_lock(mutex);
    if (in_callback) {
        pulse_terminate = 1;
        deadbeef->mutex_unlock(mutex);
        return 0;
    }
    deadbeef->mutex_unlock(mutex);

    pulse_terminate = 1;
    deadbeef->thread_join(pulse_tid);

    return 0;
}

static int pulse_play(void)
{
    trace ("pulse_play\n");
    if (!pulse_tid)
    {
        if (pulse_init () < 0)
        {
            return -1;
        }
    }

    pa_simple_flush (s, NULL);
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int pulse_stop(void)
{
    trace ("pulse_stop\n");
    state = OUTPUT_STATE_STOPPED;
    pulse_free();
    return 0;
}

static int pulse_pause(void)
{
    trace ("pulse_pause\n");
    pulse_free();
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

static int pulse_unpause(void)
{
    trace ("pulse_unpause\n");
    if (state == OUTPUT_STATE_PAUSED)
    {
        if (pulse_init () < 0)
        {
            return -1;
        }
        state = OUTPUT_STATE_PLAYING;
    }

    return 0;
}

static void pulse_thread(void *context)
{
#ifdef __linux__
    prctl(PR_SET_NAME, "deadbeef-pulse", 0, 0, 0, 0);
#endif

    while (!pulse_terminate)
    {
        if (state != OUTPUT_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1))
        {
            usleep(10000);
            continue;
        }

        int sample_size = plugin.fmt.channels * (plugin.fmt.bps / 8);
        int bs = buffer_size;
        int mod = bs % sample_size;
        if (mod > 0) {
            bs -= mod;
        }

        char buf[bs];
        deadbeef->mutex_lock(mutex);
        in_callback = 1;
        int bytesread = deadbeef->streamer_read(buf, bs);
        in_callback = 0;
        if (pulse_terminate) {
            deadbeef->mutex_unlock(mutex);
            break;
        }
        if (bytesread < 0) {
            bytesread = 0;
        }
        if (bytesread < bs)
        {
            memset (buf + bytesread, 0, bs-bytesread);
        }

        int error;

        int res = pa_simple_write(s, buf, sizeof (buf), &error);
        deadbeef->mutex_unlock(mutex);

        if (res < 0)
        {
            fprintf(stderr, "pulse: failed to write buffer\n");
            usleep(10000);
        }
    }

    state = OUTPUT_STATE_STOPPED;
    if (s)
    {
        pa_simple_free(s);
        s = NULL;
    }
    pulse_terminate = 0;
    pulse_tid = 0;
    trace ("pulse_thread finished\n");
}

static int pulse_get_state(void)
{
    return state;
}

static int pulse_plugin_start(void)
{
    mutex = deadbeef->mutex_create();

    return 0;
}

static int pulse_plugin_stop(void)
{
    deadbeef->mutex_free(mutex);

    return 0;
}

DB_plugin_t * pulse_load(DB_functions_t *api)
{
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static const char settings_dlg[] =
    "property \"PulseAudio server\" entry " CONFSTR_PULSE_SERVERADDR " default;\n"
    "property \"Preferred buffer size\" entry " CONFSTR_PULSE_BUFFERSIZE " " STR(PULSE_DEFAULT_BUFFERSIZE) ";\n";

static DB_output_t plugin =
{
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "pulseaudio",
    .plugin.name = "PulseAudio output plugin",
    .plugin.descr = "At the moment of this writing, PulseAudio seems to be very unstable in many (or most) GNU/Linux distributions.\nIf you experience problems - please try switching to ALSA or OSS output.\nIf that doesn't help - please uninstall PulseAudio from your system, and try ALSA or OSS again.\nThanks for understanding",
    .plugin.copyright =
        "PulseAudio output plugin for DeaDBeeF Player\n"
        "Copyright (C) 2011 Jan D. Behrens <zykure@web.de>\n"
        "Copyright (C) 2010-2012 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "Copyright (C) 2010 Anton Novikov <tonn.post@gmail.com>\n"
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
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = pulse_plugin_start,
    .plugin.stop = pulse_plugin_stop,
    .plugin.configdialog = settings_dlg,
    .init = pulse_init,
    .free = pulse_free,
    .setformat = pulse_setformat,
    .play = pulse_play,
    .stop = pulse_stop,
    .pause = pulse_pause,
    .unpause = pulse_unpause,
    .state = pulse_get_state,
};
