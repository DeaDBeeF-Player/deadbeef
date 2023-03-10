/*
    PulseAudio output plugin for DeaDBeeF Player
    Copyright (C) 2011 Jan D. Behrens <zykure@web.de>
    Copyright (C) 2010-2012 Oleksiy Yakovenko <waker@users.sourceforge.net>
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
#include <pulse/error.h>

#include <stdint.h>
#include <unistd.h>

#ifdef __linux__
    #include <sys/prctl.h>
#endif

#include <stdio.h>
#include <string.h>
#include <deadbeef/deadbeef.h>

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

static DB_output_t plugin;
DB_functions_t * deadbeef;

// serveraddr2 is a version bump, since the handling has changed, and "default"
// value has different meaning.
#define CONFSTR_PULSE_SERVERADDR "pulse.serveraddr2"
#define CONFSTR_PULSE_BUFFERSIZE "pulse.buffersize"
#define PULSE_DEFAULT_BUFFERSIZE 4096

static intptr_t pulse_tid;
static int pulse_terminate;

static pa_simple *s;
static pa_sample_spec ss;
static int _setformat_requested;
static ddb_waveformat_t requested_fmt;
static ddb_playback_state_t state = DDB_PLAYBACK_STATE_STOPPED;
static uintptr_t mutex;
static int in_callback;

static int buffer_size;

static void pulse_thread(void *context);

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
    char server[1000];

    // migrate from 0.7.x
    deadbeef->conf_lock ();
    int has_server2 = deadbeef->conf_get_str_fast (CONFSTR_PULSE_SERVERADDR, NULL) != NULL;
    deadbeef->conf_unlock ();

    if (!has_server2) {
        deadbeef->conf_get_str ("pulse.serveraddr", "", server, sizeof (server));
        // convert default
        if (!strcasecmp (server, "default")) {
            *server = 0;
        }
    }
    else {
        deadbeef->conf_get_str (CONFSTR_PULSE_SERVERADDR, "", server, sizeof (server));
    }

    for (;;) {
        s = pa_simple_new(*server ? server : NULL, "Deadbeef", PA_STREAM_PLAYBACK, dev, "Music", &ss, &channel_map, attr, &error);

        if (s != NULL || ss.rate <= 192000) {
            break;
        }

        // Older pulseaudio versions couldn't handle more than 192KHz,
        // so try to lower it down
        ss.rate = plugin.fmt.samplerate = 192000;
    }

    if (s == NULL)
    {
        const char *strerr = pa_strerror (error);
        fprintf (stderr, "pa_simple_new failed: %s\n", strerr);
        return -1;
    }

    return 0;
}

static int
pulse_init(void) {
    trace ("pulse_init\n");
    deadbeef->mutex_lock (mutex);
    state = DDB_PLAYBACK_STATE_STOPPED;
    trace ("pulse_terminate=%d\n", pulse_terminate);
    if (pulse_terminate) {
        deadbeef->mutex_unlock (mutex);
        return -1;
    }

    if (requested_fmt.samplerate != 0) {
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
    }

    if (0 != pulse_set_spec(&plugin.fmt)) {
        deadbeef->mutex_unlock (mutex);
        return -1;
    }

    pulse_tid = deadbeef->thread_start(pulse_thread, NULL);
    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int pulse_free(void);
static int pulse_play(void);
static int pulse_pause(void);

static int
_setformat_apply (void) {
    _setformat_requested = 0;
    if (!memcmp (&requested_fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        return 0;
    }

    return pulse_set_spec(&requested_fmt);
}

static int
pulse_setformat (ddb_waveformat_t *fmt)
{
    deadbeef->mutex_lock(mutex);
    _setformat_requested = 1;
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    deadbeef->mutex_unlock(mutex);
    return 0;
}

static int pulse_free(void)
{
    trace("pulse_free\n");

    state = DDB_PLAYBACK_STATE_STOPPED;
    deadbeef->mutex_lock(mutex);

    if (!pulse_tid || pulse_terminate == 1) {
        deadbeef->mutex_unlock(mutex);
        return 0;
    }

    if (in_callback) {
        pulse_terminate = 1;
        deadbeef->mutex_unlock(mutex);
        return 0;
    }

    pulse_terminate = 1;
    deadbeef->mutex_unlock(mutex);

    deadbeef->thread_join(pulse_tid);
    pulse_terminate = 0;

    return 0;
}

static int pulse_play(void)
{
    trace ("pulse_play\n");
    deadbeef->mutex_lock (mutex);
    if (!pulse_tid)
    {
        if (pulse_init () < 0)
        {
            deadbeef->mutex_unlock (mutex);
            return -1;
        }
    }

    pa_simple_flush (s, NULL);
    state = DDB_PLAYBACK_STATE_PLAYING;
    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int pulse_stop(void)
{
    trace ("pulse_stop\n");
    pulse_free();
    return 0;
}

static int pulse_pause(void)
{
    trace ("pulse_pause\n");
    pulse_free();
    state = DDB_PLAYBACK_STATE_PAUSED;
    return 0;
}

static int pulse_unpause(void)
{
    trace ("pulse_unpause\n");
    deadbeef->mutex_lock (mutex);
    if (state == DDB_PLAYBACK_STATE_PAUSED)
    {
        if (pulse_init () < 0)
        {
            deadbeef->mutex_unlock (mutex);
            return -1;
        }
        state = DDB_PLAYBACK_STATE_PLAYING;
    }

    deadbeef->mutex_unlock (mutex);

    return 0;
}

static void pulse_thread(void *context)
{
#ifdef __linux__
    prctl(PR_SET_NAME, "deadbeef-pulse", 0, 0, 0, 0);
#endif

    trace ("pulse thread started \n");
    while (!pulse_terminate)
    {
        if (state != DDB_PLAYBACK_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1))
        {
            usleep(10000);
            continue;
        }

        // setformat
        deadbeef->mutex_lock (mutex);
        int res = 0;
        if (_setformat_requested) {
            res = _setformat_apply ();
        }
        if (res != 0) {
            deadbeef->thread_detach (pulse_tid);
            pulse_terminate = 1;
            deadbeef->mutex_unlock(mutex);
            break;
        }
        deadbeef->mutex_unlock(mutex);

        int sample_size = plugin.fmt.channels * (plugin.fmt.bps / 8);
        char buf[buffer_size];

        in_callback = 1;
        int bytesread = deadbeef->streamer_read(buf, buffer_size);
        in_callback = 0;
        if (pulse_terminate) {
            break;
        }
        if (bytesread < 0) {
            bytesread = 0;
        }

        int error;

        res = 0;
        if (bytesread > 0) {
            deadbeef->mutex_lock (mutex);
            res = pa_simple_write(s, buf, bytesread, &error);
            deadbeef->mutex_unlock(mutex);
        }

        if (pulse_terminate) {
            break;
        }

        if (res < 0)
        {
            usleep(10000);
        }
    }

    deadbeef->mutex_lock (mutex);
    state = DDB_PLAYBACK_STATE_STOPPED;
    if (s)
    {
        pa_simple_drain (s, NULL);
        pa_simple_free(s);
        s = NULL;
    }
    pulse_terminate = 0;
    pulse_tid = 0;
    deadbeef->mutex_unlock (mutex);
    trace ("pulse_thread finished\n");
}

static ddb_playback_state_t pulse_get_state(void)
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
    "property \"PulseAudio server (leave empty for default)\" entry " CONFSTR_PULSE_SERVERADDR " \"\";\n"
    "property \"Preferred buffer size\" entry " CONFSTR_PULSE_BUFFERSIZE " " STR(PULSE_DEFAULT_BUFFERSIZE) ";\n";

static DB_output_t plugin =
{
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id = "pulseaudio",
    .plugin.name = "PulseAudio output plugin",
    .plugin.descr = "At the moment of this writing, PulseAudio seems to be very unstable in many (or most) GNU/Linux distributions.\nIf you experience problems - please try switching to ALSA or OSS output.\nIf that doesn't help - please uninstall PulseAudio from your system, and try ALSA or OSS again.\nThanks for understanding",
    .plugin.copyright =
        "PulseAudio output plugin for DeaDBeeF Player\n"
        "Copyright (C) 2011 Jan D. Behrens <zykure@web.de>\n"
        "Copyright (C) 2010-2012 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
