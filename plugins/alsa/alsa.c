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
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/prctl.h>
#include "deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static DB_output_t plugin;
DB_functions_t *deadbeef;

static snd_pcm_t *audio;
static int bufsize = -1;
static int alsa_terminate;
static int alsa_rate = 44100;
static int state; // 0 = stopped, 1 = playing, 2 = pause
static uintptr_t mutex;
static intptr_t alsa_tid;

static int conf_alsa_resample = 0;
static char conf_alsa_soundcard[100] = "default";

static void
palsa_callback (char *stream, int len);

static void
palsa_thread (void *context);

static int
palsa_init (void);

static int
palsa_free (void);

static int
palsa_change_rate (int rate);

static int
palsa_play (void);

static int
palsa_stop (void);

static int
palsa_pause (void);

static int
palsa_unpause (void);

static int
palsa_get_rate (void);

static int
palsa_get_bps (void);

static int
palsa_get_channels (void);

static int
palsa_get_endianness (void);

static void
palsa_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata);

static int
palsa_set_hw_params (int samplerate) {
    snd_pcm_hw_params_t *hw_params = NULL;
//    int alsa_resample = conf_get_int ("alsa.resample", 0);
    int err = 0;

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        trace ("cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_any (audio, hw_params)) < 0) {
        trace ("cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_set_access (audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        trace ("cannot set access type (%s)\n",
                snd_strerror (err));
        goto error;
    }

    snd_pcm_format_t fmt;
#if WORDS_BIGENDIAN
    fmt = SND_PCM_FORMAT_S16_BE;
#else
    fmt = SND_PCM_FORMAT_S16_LE;
#endif
    if ((err = snd_pcm_hw_params_set_format (audio, hw_params, fmt)) < 0) {
        trace ("cannot set sample format (%s)\n",
                snd_strerror (err));
        goto error;
    }

    snd_pcm_hw_params_get_format (hw_params, &fmt);
    trace ("chosen sample format: %04Xh\n", (int)fmt);

    int val = samplerate;
    int ret = 0;

    if ((err = snd_pcm_hw_params_set_rate_resample (audio, hw_params, conf_alsa_resample)) < 0) {
        trace ("cannot setup resampling (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_set_rate_near (audio, hw_params, &val, &ret)) < 0) {
        trace ("cannot set sample rate (%s)\n",
                snd_strerror (err));
        goto error;
    }
    alsa_rate = val;
    trace ("chosen samplerate: %d Hz\n", alsa_rate);

    if ((err = snd_pcm_hw_params_set_channels (audio, hw_params, 2)) < 0) {
        trace ("cannot set channel count (%s)\n",
                snd_strerror (err));
        goto error;
    }

    int nchan;
    snd_pcm_hw_params_get_channels (hw_params, &nchan);
    trace ("alsa channels: %d\n", nchan);

    unsigned int buffer_time = 500000;
    int dir;
    if ((err = snd_pcm_hw_params_set_buffer_time_near (audio, hw_params, &buffer_time, &dir)) < 0) {
        trace ("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
        goto error;
    }
    trace ("alsa buffer time: %d ms\n", buffer_time);
    snd_pcm_uframes_t size;
    if ((err = snd_pcm_hw_params_get_buffer_size (hw_params, &size)) < 0) {
        trace ("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        goto error;
    }
    trace ("alsa buffer size: %d frames\n", (int)size);
    bufsize = size;

    if ((err = snd_pcm_hw_params (audio, hw_params)) < 0) {
        trace ("cannot set parameters (%s)\n",
                snd_strerror (err));
        goto error;
    }
error:
    if (hw_params) {
        snd_pcm_hw_params_free (hw_params);
    }
    return err;
}

int
palsa_init (void) {
    int err;

    // get and cache conf variables
    strcpy (conf_alsa_soundcard, deadbeef->conf_get_str ("alsa_soundcard", "default"));
    conf_alsa_resample = deadbeef->conf_get_int ("alsa.resample", 0);
    trace ("alsa_soundcard: %s\n", conf_alsa_soundcard);
    trace ("alsa.resample: %d\n", conf_alsa_resample);

    snd_pcm_sw_params_t *sw_params = NULL;
    state = 0;
    //const char *conf_alsa_soundcard = conf_get_str ("alsa_soundcard", "default");
    if ((err = snd_pcm_open (&audio, conf_alsa_soundcard, SND_PCM_STREAM_PLAYBACK, 0))) {
        trace ("could not open audio device (%s)\n",
                snd_strerror (err));
        return -1;
    }

    mutex = deadbeef->mutex_create ();

    if (palsa_set_hw_params (alsa_rate) < 0) {
        goto open_error;
    }

    if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
        trace ("cannot allocate software parameters structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    if ((err = snd_pcm_sw_params_current (audio, sw_params)) < 0) {
        trace ("cannot initialize software parameters structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    if ((err = snd_pcm_sw_params_set_avail_min (audio, sw_params, bufsize/2)) < 0) {
        trace ("cannot set minimum available count (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    snd_pcm_uframes_t av;
    if ((err = snd_pcm_sw_params_get_avail_min (sw_params, &av)) < 0) {
        trace ("snd_pcm_sw_params_get_avail_min failed (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    trace ("alsa period size: %d frames\n", (int)av);

    if ((err = snd_pcm_sw_params_set_start_threshold (audio, sw_params, 0U)) < 0) {
        trace ("cannot set start mode (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    if ((err = snd_pcm_sw_params (audio, sw_params)) < 0) {
        trace ("cannot set software parameters (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    snd_pcm_sw_params_free (sw_params);
    sw_params = NULL;

    /* the interface will interrupt the kernel every N frames, and ALSA
       will wake up this program very soon after that.
       */

    if ((err = snd_pcm_prepare (audio)) < 0) {
        trace ("cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_start (audio);

    alsa_terminate = 0;
    alsa_tid = deadbeef->thread_start (palsa_thread, NULL);

    return 0;

open_error:
    if (sw_params) {
        snd_pcm_sw_params_free (sw_params);
    }
    if (audio != NULL) {
        palsa_free ();
    }

    return -1;
}

int
palsa_change_rate (int rate) {
    if (!audio) {
        return 0;
    }
    if (rate == alsa_rate) {
        trace ("palsa_change_rate: same rate (%d), ignored\n", rate);
        return rate;
    }
    trace ("trying to change samplerate to: %d\n", rate);
    deadbeef->mutex_lock (mutex);
    snd_pcm_drop (audio);
    int ret = palsa_set_hw_params (rate);
    if (state != 0) {
        snd_pcm_start (audio);
    }
    deadbeef->mutex_unlock (mutex);
    if (ret < 0) {
        return -1;
    }
    trace ("chosen samplerate: %d Hz\n", alsa_rate);
    return alsa_rate;
}

int
palsa_free (void) {
    trace ("palsa_free\n");
    if (audio && !alsa_terminate) {
        alsa_terminate = 1;
        printf ("waiting for alsa thread to finish\n");
        deadbeef->thread_join (alsa_tid);
        alsa_tid = 0;
        snd_pcm_close(audio);
        audio = NULL;
        deadbeef->mutex_free (mutex);
        state = 0;
        alsa_terminate = 0;
    }
    return 0;
}

static void
palsa_hw_pause (int pause) {
    if (!audio) {
        return;
    }
    if (state == 0) {
        return;
    }
    if (pause == 1) {
        snd_pcm_drop (audio);
    }
    else {
        snd_pcm_prepare (audio);
        snd_pcm_start (audio);
    }
}

int
palsa_play (void) {
    int err;
    if (state == 0) {
        if (!audio) {
            if (palsa_init () < 0) {
                state = 0;
                return -1;
            }
        }
        else {
            if ((err = snd_pcm_prepare (audio)) < 0) {
                trace ("cannot prepare audio interface for use (%s)\n",
                        snd_strerror (err));
                return -1;
            }
        }
    }
    if (state != 1) {
        state = 1;
        snd_pcm_start (audio);
    }
    return 0;
}


int
palsa_stop (void) {
    if (!audio) {
        return 0;
    }
    state = 0;
    if (deadbeef->conf_get_int ("alsa.freeonstop", 0))  {
        palsa_free ();
    }
    else {
        deadbeef->mutex_lock (mutex);
        snd_pcm_drop (audio);
        deadbeef->mutex_unlock (mutex);
    }
    deadbeef->streamer_reset (1);
    return 0;
}

int
palsa_pause (void) {
    if (state == 0 || !audio) {
        return -1;
    }
    // set pause state
    palsa_hw_pause (1);
    state = 2;
    return 0;
}

int
palsa_unpause (void) {
    // unset pause state
    if (state == 2) {
        state = 1;
        palsa_hw_pause (0);
    }
    return 0;
}

int
palsa_get_rate (void) {
    if (!audio) {
        palsa_init ();
    }
    return alsa_rate;
}

int
palsa_get_bps (void) {
    return 16;
}

int
palsa_get_channels (void) {
    return 2;
}

static int
palsa_get_endianness (void) {
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
}

static void
palsa_thread (void *context) {
    prctl (PR_SET_NAME, "deadbeef-alsa", 0, 0, 0, 0);
    int err;
    for (;;) {
        if (alsa_terminate) {
            break;
        }
        if (state != 1) {
            usleep (10000);
            continue;
        }
        
        deadbeef->mutex_lock (mutex);
        /* wait till the interface is ready for data, or 1 second
           has elapsed.
         */
        if ((err = snd_pcm_wait (audio, 500)) < 0 && state == 1) {
            if (err == -ESTRPIPE) {
                trace ("alsa: trying to recover from suspend...\n");
                deadbeef->sendmessage (M_REINIT_SOUND, 0, 0, 0);
                deadbeef->mutex_unlock (mutex);
                break;
            }
            else {
                trace ("alsa: trying to recover from xrun...\n");
                snd_pcm_prepare (audio);
                deadbeef->mutex_unlock (mutex);
                continue;
            }
        }	           

        /* find out how much space is available for playback data */

        snd_pcm_sframes_t frames_to_deliver;
        if ((frames_to_deliver = snd_pcm_avail_update (audio)) < 0) {
            if (frames_to_deliver == -EPIPE) {
                deadbeef->mutex_unlock (mutex);
                trace ("an xrun occured\n");
                continue;
            } else {
                deadbeef->mutex_unlock (mutex);
                trace ("unknown ALSA avail update return value (%d)\n", 
                        (int)frames_to_deliver);
                continue;
            }
        }

        /* deliver the data */
        // FIXME: under some conditions, frames_to_deliver may become huge
        // like 20M. this case is not handled here.
        char buf[frames_to_deliver*4];
        palsa_callback (buf, frames_to_deliver*4);
        if ((err = snd_pcm_writei (audio, buf, frames_to_deliver)) < 0) {
            trace ("write %d frames failed (%s)\n", frames_to_deliver, snd_strerror (err));
            snd_pcm_prepare (audio);
            snd_pcm_start (audio);
        }
        deadbeef->mutex_unlock (mutex);
        usleep (1000); // this must be here to prevent mutex deadlock
    }
}

static void
palsa_callback (char *stream, int len) {
    if (!deadbeef->streamer_ok_to_read (len)) {
        memset (stream, 0, len);
        return;
    }
    int bytesread = deadbeef->streamer_read (stream, len);

// FIXME: move volume control to streamer_read for copy optimization
#if 0
    int16_t vol[4];
    vol[0] = volume_get_amp () * 255; // that will be extra 8 bits
    // pack 4 times
    vol[1] = vol[2] = vol[3] = vol[0];

    // apply volume with mmx
    __asm__ volatile(
            "  mov %0, %%ecx\n\t"
            "  shr $4, %%ecx\n\t"
            "  mov %1, %%eax\n\t"
            "  movq %2, %mm1\n\t"
            "1:\n\t"
            "  movq [%%eax], %mm0\n\t"
            "  movq %mm0, %mm2\n\t"
            "  movq %mm0, %mm3\n\t"
            "  pmullw %mm1, %mm2\n\t"
            "  pmulhw %mm1, %mm3\n\t"
            "  psrlw $8, %mm2\n\t" // discard lowest 8 bits
            "  psllw $8, %mm3\n\t" // shift left 8 lsbs of hiwords
            "  por %mm3, %mm2\n\t" // OR them together
            "  movq %mm3, [%%eax]\n\t" // load back to memory
            "  add $8, %%eax\n\t"
            "  dec %%ecx\n\t"
            "  jnz 1b\n\t"
            :
            : "r"(len), "r"(stream), "r"(vol)
            : "%ecx", "%eax"
       );

#else
    int16_t ivolume = deadbeef->volume_get_amp () * 1000;
    for (int i = 0; i < bytesread/2; i++) {
        ((int16_t*)stream)[i] = (int16_t)(((int32_t)(((int16_t*)stream)[i])) * ivolume / 1000);
    }
#endif
    if (bytesread < len) {
        memset (stream + bytesread, 0, len-bytesread);
    }
}

static int
palsa_configchanged (DB_event_t *ev, uintptr_t data) {
    int alsa_resample = deadbeef->conf_get_int ("alsa.resample", 0);
    const char *alsa_soundcard = deadbeef->conf_get_str ("alsa_soundcard", "default");
    if (alsa_resample != conf_alsa_resample
            || strcmp (alsa_soundcard, conf_alsa_soundcard)) {
        deadbeef->sendmessage (M_REINIT_SOUND, 0, 0, 0);
    }
    return 0;
}

// derived from alsa-utils/aplay.c
void
palsa_enum_soundcards (void (*callback)(const char *name, const char *desc, void *), void *userdata) {
    void **hints, **n;
    char *name, *descr, *io;
    const char *filter = "Output";
    if (snd_device_name_hint(-1, "pcm", &hints) < 0)
        return;
    n = hints;
    while (*n != NULL) {
        name = snd_device_name_get_hint(*n, "NAME");
        descr = snd_device_name_get_hint(*n, "DESC");
        io = snd_device_name_get_hint(*n, "IOID");
        if (io == NULL || !strcmp(io, filter)) {
            if (name && descr && callback) {
                callback (name, descr, userdata);
            }
        }
        if (name != NULL)
            free(name);
        if (descr != NULL)
            free(descr);
        if (io != NULL)
            free(io);
        n++;
    }
    snd_device_name_free_hint(hints);
}

int
palsa_get_state (void) {
    return state;
}

int
alsa_start (void) {
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (palsa_configchanged), 0);
    return 0;
}

int
alsa_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (palsa_configchanged), 0);
    return 0;
}

DB_plugin_t *
alsa_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_output_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.nostop = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.name = "ALSA output plugin",
    .plugin.descr = "plays sound through linux standard alsa library",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = alsa_start,
    .plugin.stop = alsa_stop,
    .init = palsa_init,
    .free = palsa_free,
    .change_rate = palsa_change_rate,
    .play = palsa_play,
    .stop = palsa_stop,
    .pause = palsa_pause,
    .unpause = palsa_unpause,
    .state = palsa_get_state,
    .samplerate = palsa_get_rate,
    .bitspersample = palsa_get_bps,
    .channels = palsa_get_channels,
    .endianness = palsa_get_endianness,
    .enum_soundcards = palsa_enum_soundcards,
};
