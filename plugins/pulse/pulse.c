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

    PulseAudio output plugin
    Copyright (C) 2010 Anton Novikov <tonn.post@gmail.com>
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
#include "../../deadbeef.h"

#define trace(...) { fprintf(stdout, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_output_t plugin;
DB_functions_t * deadbeef;

#define CONFSTR_PULSE_SAMPLERATE "pulse.samplerate"
#define CONFSTR_PULSE_SERVERADDR "pulse.serveraddr"
#define CONFSTR_PULSE_BUFFERSIZE "pulse.buffersize"

static intptr_t pulse_tid;
static int pulse_terminate;

static pa_simple *s;
static pa_sample_spec ss;
static int state;
static uintptr_t mutex;

static int buffer_size;

static void pulse_thread(void *context);

static void pulse_callback(char *stream, int len);

static int pulse_init(void)
{
    trace ("pulse_init\n");
    state = OUTPUT_STATE_STOPPED;
    pulse_terminate = 0;

    // Read serveraddr from config
    const char * server = deadbeef->conf_get_str(CONFSTR_PULSE_SERVERADDR, NULL);

    if (server)
        server = strcmp(server, "default") ? server : NULL;

    // Read samplerate from config
    ss.rate = deadbeef->conf_get_int(CONFSTR_PULSE_SAMPLERATE, 44100);
    trace ("pulse: samplerate: %d\n", ss.rate);

    // TODO: add config for this
    pa_channel_map * map = NULL;//pa_channel_map_init_stereo(NULL);
    ss.channels = 2;
    ss.format = PA_SAMPLE_S16NE;

    // TODO: where list of all available devices? add this option to config too..
    char * dev = NULL;

    pa_buffer_attr * attr = NULL;
    //attr->maxlength = Maximum length of the buffer.
    //attr->tlength = Playback only: target length of the buffer.
    //attr->prebuf = Playback only: pre-buffering.
    //attr->minreq = Playback only: minimum request.
    //attr->fragsize = Recording only: fragment size.

    buffer_size = deadbeef->conf_get_int(CONFSTR_PULSE_BUFFERSIZE, 4096);

    int error;
    s = pa_simple_new(server, "Deadbeef", PA_STREAM_PLAYBACK, dev, "Music", &ss, map, attr, &error);

    if (!s)
    {
        return -1;
    }

    pulse_tid = deadbeef->thread_start(pulse_thread, NULL);

    return 0;
}

static int pulse_free(void)
{
    trace("pulse_free\n");

    if (pulse_tid)
    {
        pulse_terminate = 1;
        deadbeef->thread_join(pulse_tid);
    }

    pulse_tid = 0;
    state = OUTPUT_STATE_STOPPED;
    pa_simple_free(s);
    s = NULL;

    return 0;
}

static int pulse_play(void)
{
    if (!pulse_tid)
    {
        if (pulse_init () < 0)
        {
            return -1;
        }
    }

    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int pulse_stop(void)
{
    state = OUTPUT_STATE_STOPPED;
    deadbeef->streamer_reset(1);
    return 0;
}

static int pulse_pause(void)
{
    if (state == OUTPUT_STATE_STOPPED)
    {
        return -1;
    }

    state = OUTPUT_STATE_PAUSED;

    return 0;
}

static int pulse_unpause(void)
{
    if (state == OUTPUT_STATE_PAUSED)
    {
        state = OUTPUT_STATE_PLAYING;
    }

    return 0;
}

static int pulse_change_rate(int rate)
{
    pulse_free();
    ss.rate = rate;

    if (!pulse_init())
        return -1;

    return ss.rate;
}

static int pulse_get_rate(void)
{
    return ss.rate;
}

static int pulse_get_bps(void)
{
    return 16;
}

static int pulse_get_channels(void)
{
    return ss.channels;
}

static int pulse_get_endianness(void)
{
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
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
            usleep(1000);
            continue;
        }

        char buf[buffer_size];
        pulse_callback (buf, sizeof (buf));
        int error;

        deadbeef->mutex_lock(mutex);
        int res = pa_simple_write(s, buf, sizeof (buf), &error);
        deadbeef->mutex_unlock(mutex);

        if (res < 0)
        {
            fprintf(stderr, "pulse: failed to write buffer\n");
            pulse_tid = 0;
            pulse_free ();
            break;
        }
    }
}

static void pulse_callback(char *stream, int len)
{
    if (!deadbeef->streamer_ok_to_read (len))
    {
        memset (stream, 0, len);
        return;
    }

    int bytesread = deadbeef->streamer_read(stream, len);
    int16_t ivolume = deadbeef->volume_get_amp() * 1000;

    for (int i = 0; i < bytesread/2; i++)
    {
        ((int16_t*)stream)[i] = (int16_t)(((int32_t)(((int16_t*)stream)[i])) * ivolume / 1000);
    }

    if (bytesread < len)
    {
        memset (stream + bytesread, 0, len-bytesread);
    }
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

static const char settings_dlg[] =
    "property \"PulseAudio server\" entry " CONFSTR_PULSE_SERVERADDR " default;\n"
    "property \"Preferred buffer size\" entry " CONFSTR_PULSE_BUFFERSIZE " 4096;\n"
    "property \"Samplerate\" entry " CONFSTR_PULSE_SAMPLERATE " 44100;\n";

static DB_output_t plugin =
{
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.nostop = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.name = "PulseAudio output plugin",
    .plugin.descr = "plays sound via pulse API",
    .plugin.author = "Anton Novikov",
    .plugin.email = "tonn.post@gmail.com",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = pulse_plugin_start,
    .plugin.stop = pulse_plugin_stop,
    .plugin.configdialog = settings_dlg,
    .init = pulse_init,
    .free = pulse_free,
    .change_rate = pulse_change_rate,
    .play = pulse_play,
    .stop = pulse_stop,
    .pause = pulse_pause,
    .unpause = pulse_unpause,
    .state = pulse_get_state,
    .samplerate = pulse_get_rate,
    .bitspersample = pulse_get_bps,
    .channels = pulse_get_channels,
    .endianness = pulse_get_endianness,
};
