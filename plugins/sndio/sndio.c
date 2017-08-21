/*
 * sndio - Output plugin using OpenBSD sndio API
 * Copyright (c) 2012 Alexandr Shadchin <shadchin@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sndio.h>
#include "../../deadbeef.h"

static DB_output_t plugin;
DB_functions_t *deadbeef;

static int state = OUTPUT_STATE_STOPPED;
static int sndio_terminate;
static intptr_t sndio_tid;
static uintptr_t sndio_mutex;

static struct sio_hdl *hdl;
static size_t bufsz;
static char *buf = NULL;
static float min_db;
static float vol_db;

static int sndio_init(void);
static int sndio_free(void);
static int sndio_setformat(ddb_waveformat_t *);
static int sndio_play(void);
static int sndio_stop(void);
static int sndio_pause(void);
static int sndio_unpause(void);
static void sndio_thread(void *context);
static int sndio_callback(char *stream, int len);

static void
vol_cb(void *unused, unsigned int newvol)
{
    float newvol_db;

    newvol_db = min_db * (1 - (float)newvol / SIO_MAXVOL);
    if (newvol_db == vol_db)
        return;
    vol_db = newvol_db;
    deadbeef->volume_set_db(vol_db);
}

static void
pause_do(void)
{
    if (!hdl)
        return;

    deadbeef->mutex_lock(sndio_mutex);
    sio_stop(hdl);
    sio_start(hdl);
    deadbeef->mutex_unlock(sndio_mutex);
}

static int
_formatchanged (void) {
    if (!hdl) {
        return -1;
    }
    int buffer_ms = deadbeef->conf_get_int("sndio.buffer", 250);
    struct sio_par par;
    sio_initpar(&par);
    par.pchan = plugin.fmt.channels;
    par.rate = plugin.fmt.samplerate;
    par.bits = plugin.fmt.bps;
    par.bps = SIO_BPS(plugin.fmt.bps);
    par.sig = 1;
    par.le = SIO_LE_NATIVE;
    par.appbufsz = par.rate * buffer_ms / 1000;

    deadbeef->mutex_lock(sndio_mutex);
    if (!sio_setpar(hdl, &par) || !sio_getpar(hdl, &par)) {
        fprintf(stderr, "sndio: failed to set parameters\n");
        deadbeef->mutex_unlock (sndio_mutex);
        return -1;
    }

    //fprintf(stderr, "sndio: got format %dbit %s %dch %dHz %dbps\n", par.bits, plugin.fmt.is_float ? "float" : "int", par.pchan, par.rate, par.bps);

    plugin.fmt.bps = par.bits;
    plugin.fmt.samplerate = par.rate;
    plugin.fmt.channels = par.pchan;

    bufsz = par.bps * par.pchan * par.round;
    buf = malloc(bufsz);
    if (!buf) {
        fprintf(stderr, "sndio: failed malloc buf\n");
        deadbeef->mutex_unlock (sndio_mutex);
        return -1;
    }

    deadbeef->mutex_unlock (sndio_mutex);
    return 0;
}

static int
sndio_init(void)
{
    const char *device = deadbeef->conf_get_str_fast("sndio.device", SIO_DEVANY);

    if (plugin.fmt.is_float) {
        fprintf(stderr, "sndio: float format is not supported\n");
        goto error;
    }

    hdl = sio_open(device, SIO_PLAY, 0);
    if (!hdl) {
        fprintf(stderr, "sndio: failed to open audio device\n");
        goto error;
    }

    if (_formatchanged ()) {
        goto error;
    }

    min_db = deadbeef->volume_get_min_db();
    sio_onvol(hdl, vol_cb, NULL);
    if (!sio_start(hdl)) {
        fprintf(stderr, "sndio: failed to start audio device\n");
        goto error;
    }

    sndio_tid = deadbeef->thread_start(sndio_thread, NULL);

    return 0;

error:
    sndio_free();

    return -1;
}

static int
sndio_free(void)
{
    if (sndio_tid) {
        sndio_terminate = 1;
        deadbeef->thread_join(sndio_tid);
        sndio_tid = 0;
    }
    if (hdl) {
        sio_close(hdl);
        hdl = NULL;
    }
    if (buf) {
        free(buf);
        buf = NULL;
    }

    state = OUTPUT_STATE_STOPPED;
    sndio_terminate = 0;

    return 0;
}

static int
sndio_setformat(ddb_waveformat_t *fmt)
{
    deadbeef->mutex_lock (sndio_mutex);
    if (!memcmp(&plugin.fmt, fmt, sizeof(ddb_waveformat_t))) {
        deadbeef->mutex_unlock (sndio_mutex);
        return 0;
    }

    //printf ("sndio requested format: %d %d %d %d\n", fmt->samplerate, fmt->channels, fmt->bps, fmt->is_float);

    memcpy(&plugin.fmt, fmt, sizeof(ddb_waveformat_t));
    if (plugin.fmt.is_float) {
        plugin.fmt.is_float = 0;
        plugin.fmt.bps = 24;
    }

    _formatchanged ();

    deadbeef->mutex_unlock (sndio_mutex);

    return 0;
}

static int
sndio_play(void)
{
    if (!sndio_tid && sndio_init() < 0)
        return -1;

    state = OUTPUT_STATE_PLAYING;

    return 0;
}

static int
sndio_stop(void)
{
    if (state != OUTPUT_STATE_STOPPED) {
        state = OUTPUT_STATE_STOPPED;
        pause_do();
    }
    deadbeef->streamer_reset(1);

    return 0;
}

static int
sndio_pause(void)
{
    if (state == OUTPUT_STATE_STOPPED)
        return -1;
    else if (state != OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PAUSED;
        pause_do();
    }

    return 0;
}

static int
sndio_unpause(void)
{
    state = OUTPUT_STATE_PLAYING;

    return 0;
}

static void
sndio_thread(void *context)
{
    float newvol_db;
    size_t size;

    while (!sndio_terminate) {
        if (state != OUTPUT_STATE_PLAYING) {
            usleep(10000);
            continue;
        }

        deadbeef->mutex_lock(sndio_mutex);
        newvol_db = deadbeef->volume_get_db();
        if (newvol_db != vol_db) {
            vol_db = newvol_db;
            sio_setvol(hdl, (1 - vol_db / min_db) * SIO_MAXVOL);
        }
        size = sndio_callback(buf, bufsz);
        memset(buf + size, 0, bufsz - size);
        size = sio_write(hdl, buf, bufsz);
        deadbeef->mutex_unlock(sndio_mutex);
        if (size != bufsz) {
            fprintf(stderr, "sndio: failed to write buffer\n");
            sndio_free();
        }
    }
}

static int
sndio_callback(char *stream, int len)
{
    return deadbeef->streamer_read(stream, len);
}

static int
sndio_get_state(void)
{
    return state;
}

static int
sndio_plugin_start(void)
{
    sndio_mutex = deadbeef->mutex_create();

    return 0;
}

static int
sndio_plugin_stop(void)
{
    if (sndio_mutex) {
        deadbeef->mutex_free(sndio_mutex);
        sndio_mutex = 0;
    }

    return 0;
}

DB_plugin_t *
sndio_load(DB_functions_t *api)
{
    deadbeef = api;

    return DB_PLUGIN(&plugin);
}

static const char settings_dlg[] =
    "property \"Device\" entry sndio.device " SIO_DEVANY ";\n"
    "property \"Buffer size (ms)\" entry sndio.buffer 250;\n";

/* define plugin interface */
static DB_output_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "sndio",
    .plugin.name = "sndio output plugin",
    .plugin.descr = "plays sound through sndio library",
    .plugin.copyright =
        "Copyright (c) 2012 Alexandr Shadchin <shadchin@openbsd.org>\n"
        "\n"
        "Permission to use, copy, modify, and distribute this software for any\n"
        "purpose with or without fee is hereby granted, provided that the above\n"
        "copyright notice and this permission notice appear in all copies.\n"
        "\n"
        "THE SOFTWARE IS PROVIDED 'AS IS' AND THE AUTHOR DISCLAIMS ALL WARRANTIES\n"
        "WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF\n"
        "MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR\n"
        "ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\n"
        "WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\n"
        "ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF\n"
        "OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n",
    .plugin.website = "http://www.openbsd.org/",
    .plugin.start = sndio_plugin_start,
    .plugin.stop = sndio_plugin_stop,
    .plugin.configdialog = settings_dlg,
    .init = sndio_init,
    .free = sndio_free,
    .setformat = sndio_setformat,
    .play = sndio_play,
    .stop = sndio_stop,
    .pause = sndio_pause,
    .unpause = sndio_unpause,
    .state = sndio_get_state,
    .has_volume = 1,
    .fmt.samplerate = 48000,
    .fmt.channels = 2,
    .fmt.bps = 16,
    .fmt.is_float = 0,
    .fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT
};
