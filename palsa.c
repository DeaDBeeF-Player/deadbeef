/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "palsa.h"
#include "threading.h"
#include "streamer.h"
#include "conf.h"
#include "volume.h"
#include "messagepump.h"
#include "deadbeef.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

static snd_pcm_t *audio;
static int16_t *samplebuffer;
static int bufsize = 2048*4;
static int alsa_terminate;
static int alsa_rate = 48000;
static int state; // 0 = stopped, 1 = playing, 2 = pause
static uintptr_t mutex;
static int canpause;
static intptr_t alsa_tid;

static void
palsa_callback (char *stream, int len);

static void
palsa_thread (uintptr_t context);

int
palsa_init (void) {
    int err;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    state = 0;
    alsa_rate = conf_get_int ("samplerate", 48000);
    const char *conf_alsa_soundcard = conf_get_str ("alsa_soundcard", "default");

    if ((err = snd_pcm_open (&audio, conf_alsa_soundcard, SND_PCM_STREAM_PLAYBACK, 0))) {
        trace ("could not open audio device (%s)\n",
                snd_strerror (err));
        exit (-1);
    }

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        trace ("cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    if ((err = snd_pcm_hw_params_any (audio, hw_params)) < 0) {
        trace ("cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    if ((err = snd_pcm_hw_params_set_access (audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        trace ("cannot set access type (%s)\n",
                snd_strerror (err));
        goto open_error;
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
        goto open_error;
    }

    snd_pcm_hw_params_get_format (hw_params, &fmt);
    printf ("chosen format: %d\n", (int)fmt);

    canpause = 0;//snd_pcm_hw_params_can_pause (hw_params);

    int val = alsa_rate;
    int ret = 0;

    if ((err = snd_pcm_hw_params_set_rate_near (audio, hw_params, &val, &ret)) < 0) {
        trace ("cannot set sample rate (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    alsa_rate = val;
    printf ("chosen samplerate: %d\n", alsa_rate);

    if ((err = snd_pcm_hw_params_set_channels (audio, hw_params, 2)) < 0) {
        trace ("cannot set channel count (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    int nchan;
    snd_pcm_hw_params_get_channels (hw_params, &nchan);
    printf ("nchannels: %d\n", nchan);

    if ((err = snd_pcm_hw_params (audio, hw_params)) < 0) {
        trace ("cannot set parameters (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_hw_params_free (hw_params);

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
    if ((err = snd_pcm_sw_params_set_avail_min (audio, sw_params, bufsize/4)) < 0) {
        trace ("cannot set minimum available count (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_uframes_t nsamp;
    snd_pcm_sw_params_get_avail_min (sw_params, &nsamp);
    printf ("nsamples: %d\n", (int)nsamp);
    bufsize = nsamp * 4;

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

    /* the interface will interrupt the kernel every N frames, and ALSA
       will wake up this program very soon after that.
       */

    if ((err = snd_pcm_prepare (audio)) < 0) {
        trace ("cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_start (audio);

    // take returned fragsize
    samplebuffer = malloc (bufsize);

    if (!samplebuffer)
    {
        printf ("AUDIO: Unable to allocate memory for sample buffers.\n");
        goto open_error;
    }

    alsa_terminate = 0;
    mutex = mutex_create ();
    alsa_tid = thread_start (palsa_thread, 0);

    return 0;

open_error:
    if (audio != NULL)
    {
        palsa_free ();
    }

    return -1;
}

void
palsa_free (void) {
    if (audio) {
        mutex_lock (mutex);
        alsa_terminate = 1;
        if (alsa_tid) {
            thread_join (alsa_tid);
            alsa_tid = 0;
        }
        snd_pcm_close(audio);
        audio = NULL;
        if (samplebuffer) {
            free (samplebuffer);
            samplebuffer = NULL;
        }
        mutex_unlock (mutex);
        mutex_free (mutex);
    }
}

static int hwpaused;
static void
palsa_hw_pause (int pause) {
    if (canpause) {
        snd_pcm_pause (audio, pause);
    }
    else {
        if (pause == 1) {
            snd_pcm_drop (audio);
        }
        else {
            snd_pcm_prepare (audio);
            snd_pcm_start (audio);
        }
        hwpaused = pause;
    }
    hwpaused = pause;
}

int
palsa_play (void) {
    // start updating thread
    int err;
    if (state == 0) {
        if ((err = snd_pcm_prepare (audio)) < 0) {
            trace ("cannot prepare audio interface for use (%s)\n",
                    snd_strerror (err));
        }
        streamer_reset (1);
    }
    if (state != 1) {
        state = 1;
        snd_pcm_start (audio);
    }
    return 0;
}


int
palsa_stop (void) {
    // set stop state
    state = 0;
    snd_pcm_drop (audio);
    return 0;
}

int
palsa_isstopped (void) {
    return (state == 0);
}

int
palsa_ispaused (void) {
    // return pause state
    return (state == 2);
}

int
palsa_pause (void) {
    // set pause state
    state = 2;
    palsa_hw_pause (1);
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
    return alsa_rate;
}

static void
palsa_thread (uintptr_t context) {
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
        mutex_lock (mutex);
        /* wait till the interface is ready for data, or 1 second
           has elapsed.
         */
        if ((err = snd_pcm_wait (audio, 1000)) < 0 && state == 1) {
            trace ("snd_pcm_wait failed, restarting alsa\n");
            messagepump_push (M_REINIT_SOUND, 0, 0, 0);
            mutex_unlock (mutex);
            break;
        }	           

        /* find out how much space is available for playback data */

        snd_pcm_sframes_t frames_to_deliver;
        if ((frames_to_deliver = snd_pcm_avail_update (audio)) < 0) {
            if (frames_to_deliver == -EPIPE) {
                mutex_unlock (mutex);
                trace ("an xrun occured\n");
                continue;
            } else {
                mutex_unlock (mutex);
                trace ("unknown ALSA avail update return value (%d)\n", 
                        (int)frames_to_deliver);
                continue;
            }
        }

        frames_to_deliver = frames_to_deliver > bufsize/4 ? bufsize/4 : frames_to_deliver;

        /* deliver the data */
        char buf[bufsize];
        palsa_callback (buf, frames_to_deliver*4);
        if ((err = snd_pcm_writei (audio, buf, frames_to_deliver)) < 0) {
            trace ("write %d frames failed (%s)\n", frames_to_deliver, snd_strerror (err));
            snd_pcm_prepare (audio);
            snd_pcm_start (audio);
        }
        mutex_unlock (mutex);
        usleep (1000); // removing this causes deadlock on exit
    }
}

static void
palsa_callback (char *stream, int len) {
    if (!streamer_ok_to_read (len)) {
        memset (stream, 0, len);
        return;
    }
    int bytesread = streamer_read (stream, len);

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
    int16_t ivolume = volume_get_amp () * 1000;
    for (int i = 0; i < bytesread/2; i++) {
        ((int16_t*)stream)[i] = (int16_t)(((int32_t)(((int16_t*)stream)[i])) * ivolume / 1000);
    }
#endif
    if (bytesread < len) {
        memset (stream + bytesread, 0, len-bytesread);
    }
}
