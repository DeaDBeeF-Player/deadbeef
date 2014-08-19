/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"
#include "../../config.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

#define LOCK {deadbeef->mutex_lock (mutex); /*fprintf (stderr, "alsa lock %s:%d\n", __FILE__, __LINE__);*/}
#define UNLOCK {deadbeef->mutex_unlock (mutex); /*fprintf (stderr, "alsa unlock %s:%d\n", __FILE__, __LINE__);*/}

#define DEFAULT_BUFFER_SIZE 8192
#define DEFAULT_PERIOD_SIZE 1024
#define DEFAULT_BUFFER_SIZE_STR "8192"
#define DEFAULT_PERIOD_SIZE_STR "1024"

static DB_output_t plugin;
DB_functions_t *deadbeef;

static snd_pcm_t *audio;
static int alsa_terminate;
static ddb_waveformat_t requested_fmt;
static int state; // one of output_state_t
static uintptr_t mutex;
static intptr_t alsa_tid;

static snd_pcm_uframes_t buffer_size;
static snd_pcm_uframes_t period_size;

static snd_pcm_uframes_t req_buffer_size;
static snd_pcm_uframes_t req_period_size;

static int conf_alsa_resample = 1;
static char conf_alsa_soundcard[100] = "default";

static int alsa_formatchanged = 0;

static int
palsa_callback (char *stream, int len);

static void
palsa_thread (void *context);

static int
palsa_init (void);

static int
palsa_free (void);

static int
palsa_setformat (ddb_waveformat_t *fmt);

static int
palsa_play (void);

static int
palsa_stop (void);

static int
palsa_pause (void);

static int
palsa_unpause (void);

static int
palsa_get_channels (void);

static int
palsa_get_endianness (void);

static void
palsa_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata);

static int
palsa_set_hw_params (ddb_waveformat_t *fmt) {
    snd_pcm_hw_params_t *hw_params = NULL;
    int err = 0;

    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
    if (!plugin.fmt.channels) {
        // generic format
        plugin.fmt.bps = 16;
        plugin.fmt.is_float = 0;
        plugin.fmt.channels = 2;
        plugin.fmt.samplerate = 44100;
        plugin.fmt.channelmask = 3;
    }
retry:

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_any (audio, hw_params)) < 0) {
        fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_set_access (audio, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf (stderr, "cannot set access type (%s)\n",
                snd_strerror (err));
        goto error;
    }

    snd_pcm_format_t sample_fmt;
    switch (plugin.fmt.bps) {
    case 8:
        sample_fmt = SND_PCM_FORMAT_S8;
        break;
    case 16:
#if WORDS_BIGENDIAN
        sample_fmt = SND_PCM_FORMAT_S16_BE;
#else
        sample_fmt = SND_PCM_FORMAT_S16_LE;
#endif
        break;
    case 24:
#if WORDS_BIGENDIAN
        sample_fmt = SND_PCM_FORMAT_S24_3BE;
#else
        sample_fmt = SND_PCM_FORMAT_S24_3LE;
#endif
        break;
    case 32:
        if (plugin.fmt.is_float) {
#if WORDS_BIGENDIAN
            sample_fmt = SND_PCM_FORMAT_FLOAT_BE;
#else
            sample_fmt = SND_PCM_FORMAT_FLOAT_LE;
#endif
        }
        else {
#if WORDS_BIGENDIAN
            sample_fmt = SND_PCM_FORMAT_S32_BE;
#else
            sample_fmt = SND_PCM_FORMAT_S32_LE;
#endif
        }
        break;
    }

    if ((err = snd_pcm_hw_params_set_format (audio, hw_params, sample_fmt)) < 0) {
        fprintf (stderr, "cannot set sample format (%s), trying all supported formats\n", snd_strerror (err));

        int fmt_cnt[] = { 16, 24, 32, 32, 8 };
#if WORDS_BIGENDIAN
        int fmt[] = { SND_PCM_FORMAT_S16_BE, SND_PCM_FORMAT_S24_3BE, SND_PCM_FORMAT_S32_BE, SND_PCM_FORMAT_FLOAT_BE, SND_PCM_FORMAT_S8, -1 };
#else
        int fmt[] = { SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_FLOAT_LE, SND_PCM_FORMAT_S8, -1 };
#endif

        // 1st try formats with higher bps
        int i = 0;
        for (i = 0; fmt[i] != -1; i++) {
            if (fmt[i] != sample_fmt && fmt_cnt[i] > plugin.fmt.bps) {
                if (snd_pcm_hw_params_set_format (audio, hw_params, fmt[i]) >= 0) {
                    fprintf (stderr, "cannot set sample format (%s), trying all supported formats\n", snd_strerror (err));
                    break;
                }
            }
        }
        if (fmt[i] == -1) {
            // next try formats with lower bps
            i = 0;
            for (i = 0; fmt[i] != -1; i++) {
                if (fmt[i] != sample_fmt && fmt_cnt[i] < plugin.fmt.bps) {
                    if (snd_pcm_hw_params_set_format (audio, hw_params, fmt[i]) >= 0) {
                        fprintf (stderr, "cannot set sample format (%s), trying all supported formats\n", snd_strerror (err));
                        break;
                    }
                }
            }
        }

        if (fmt[i] == -1) {
            goto error;
        }
    }

    snd_pcm_hw_params_get_format (hw_params, &sample_fmt);
    trace ("chosen sample format: %04Xh\n", (int)sample_fmt);

    int val = plugin.fmt.samplerate;
    int ret = 0;

    if ((err = snd_pcm_hw_params_set_rate_resample (audio, hw_params, conf_alsa_resample)) < 0) {
        fprintf (stderr, "cannot setup resampling (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if ((err = snd_pcm_hw_params_set_rate_near (audio, hw_params, &val, &ret)) < 0) {
        fprintf (stderr, "cannot set sample rate (%s)\n",
                snd_strerror (err));
        goto error;
    }
    plugin.fmt.samplerate = val;
    trace ("chosen samplerate: %d Hz\n", val);

    int chanmin, chanmax;
    snd_pcm_hw_params_get_channels_min (hw_params, &chanmin);
    snd_pcm_hw_params_get_channels_max (hw_params, &chanmax);

    trace ("minchan: %d, maxchan: %d\n", chanmin, chanmax);
    int nchan = plugin.fmt.channels;
    if (nchan > chanmax) {
        nchan = chanmax;
    }
    else if (nchan < chanmin) {
        nchan = chanmin;
    }
    trace ("setting chan=%d\n", nchan);
    if ((err = snd_pcm_hw_params_set_channels (audio, hw_params, nchan)) < 0) {
        fprintf (stderr, "cannot set channel count (%s)\n",
                snd_strerror (err));
    }

    snd_pcm_hw_params_get_channels (hw_params, &nchan);
    trace ("alsa channels: %d\n", nchan);

    req_buffer_size = deadbeef->conf_get_int ("alsa.buffer", DEFAULT_BUFFER_SIZE);
    req_period_size = deadbeef->conf_get_int ("alsa.period", DEFAULT_PERIOD_SIZE);
    buffer_size = req_buffer_size;
    period_size = req_period_size;
    trace ("trying buffer size: %d frames\n", (int)buffer_size);
    trace ("trying period size: %d frames\n", (int)period_size);
    snd_pcm_hw_params_set_buffer_size_near (audio, hw_params, &buffer_size);
    snd_pcm_hw_params_set_period_size_near (audio, hw_params, &period_size, NULL);
    trace ("alsa buffer size: %d frames\n", (int)buffer_size);
    trace ("alsa period size: %d frames\n", (int)period_size);

    if ((err = snd_pcm_hw_params (audio, hw_params)) < 0) {
        fprintf (stderr, "cannot set parameters (%s)\n",
                snd_strerror (err));
        goto error;
    }

    plugin.fmt.is_float = 0;
    switch (sample_fmt) {
    case SND_PCM_FORMAT_S8:
        plugin.fmt.bps = 8;
        break;
    case SND_PCM_FORMAT_S16_BE:
    case SND_PCM_FORMAT_S16_LE:
        plugin.fmt.bps = 16;
        break;
    case SND_PCM_FORMAT_S24_3BE:
    case SND_PCM_FORMAT_S24_3LE:
        plugin.fmt.bps = 24;
        break;
    case SND_PCM_FORMAT_S32_BE:
    case SND_PCM_FORMAT_S32_LE:
        plugin.fmt.bps = 32;
        break;
    case SND_PCM_FORMAT_FLOAT_LE:
    case SND_PCM_FORMAT_FLOAT_BE:
        plugin.fmt.bps = 32;
        plugin.fmt.is_float = 1;
        break;
    }

    trace ("chosen bps: %d (%s)\n", plugin.fmt.bps, plugin.fmt.is_float ? "float" : "int");

    plugin.fmt.channels = nchan;
    plugin.fmt.channelmask = 0;
    if (nchan == 1) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT;
    }
    if (nchan == 2) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT;
    }
    if (nchan == 3) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_LOW_FREQUENCY;
    }
    if (nchan == 4) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT;
    }
    if (nchan == 5) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER;
    }
    if (nchan == 6) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER | DDB_SPEAKER_LOW_FREQUENCY;
    }
    if (nchan == 7) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER | DDB_SPEAKER_SIDE_LEFT | DDB_SPEAKER_SIDE_RIGHT;
    }
    if (nchan == 8) {
        plugin.fmt.channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT | DDB_SPEAKER_BACK_LEFT | DDB_SPEAKER_BACK_RIGHT | DDB_SPEAKER_FRONT_CENTER | DDB_SPEAKER_SIDE_LEFT | DDB_SPEAKER_SIDE_RIGHT | DDB_SPEAKER_LOW_FREQUENCY;
    }
error:
    if (err < 0) {
        memset (&plugin.fmt, 0, sizeof (ddb_waveformat_t));
    }
    if (hw_params) {
        snd_pcm_hw_params_free (hw_params);
    }
    return err;
}

int
palsa_init (void) {
    int err;
    alsa_tid = 0;
    mutex = 0;

    // get and cache conf variables
    conf_alsa_resample = deadbeef->conf_get_int ("alsa.resample", 1);
    deadbeef->conf_get_str ("alsa_soundcard", "default", conf_alsa_soundcard, sizeof (conf_alsa_soundcard));
    trace ("alsa_soundcard: %s\n", conf_alsa_soundcard);

    snd_pcm_sw_params_t *sw_params = NULL;
    state = OUTPUT_STATE_STOPPED;
    //const char *conf_alsa_soundcard = conf_get_str ("alsa_soundcard", "default");
    if ((err = snd_pcm_open (&audio, conf_alsa_soundcard, SND_PCM_STREAM_PLAYBACK, 0))) {
        fprintf (stderr, "could not open audio device (%s)\n",
                snd_strerror (err));
        return -1;
    }

    mutex = deadbeef->mutex_create ();

    if (requested_fmt.samplerate != 0) {
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
    }

    if (palsa_set_hw_params (&plugin.fmt) < 0) {
        goto open_error;
    }

    if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
        fprintf (stderr, "cannot allocate software parameters structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    if ((err = snd_pcm_sw_params_current (audio, sw_params)) < 0) {
        fprintf (stderr, "cannot initialize software parameters structure (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_sw_params_set_start_threshold (audio, sw_params, buffer_size - period_size);

    if ((err = snd_pcm_sw_params_set_avail_min (audio, sw_params, period_size)) < 0) {
        fprintf (stderr, "cannot set minimum available count (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    snd_pcm_uframes_t av;
    if ((err = snd_pcm_sw_params_get_avail_min (sw_params, &av)) < 0) {
        fprintf (stderr, "snd_pcm_sw_params_get_avail_min failed (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    trace ("alsa avail_min: %d frames\n", (int)av);


//    if ((err = snd_pcm_sw_params_set_start_threshold (audio, sw_params, 0U)) < 0) {
//        trace ("cannot set start mode (%s)\n",
//                snd_strerror (err));
//        goto open_error;
//    }

    if ((err = snd_pcm_sw_params (audio, sw_params)) < 0) {
        fprintf (stderr, "cannot set software parameters (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    snd_pcm_sw_params_free (sw_params);
    sw_params = NULL;

    /* the interface will interrupt the kernel every N frames, and ALSA
       will wake up this program very soon after that.
       */

    if ((err = snd_pcm_prepare (audio)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

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
palsa_setformat (ddb_waveformat_t *fmt) {
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    trace ("palsa_setformat %dbit %s %dch %dHz channelmask=%X\n", requested_fmt.bps, fmt->is_float ? "float" : "int", fmt->channels, fmt->samplerate, fmt->channelmask);
    if (!audio) {
        return -1;
    }
    if (!memcmp (&requested_fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        trace ("palsa_setformat ignored\n");
        return 0;
    }
    else {
        trace ("switching format:\n"
        "bps %d -> %d\n"
        "is_float %d -> %d\n"
        "channels %d -> %d\n"
        "samplerate %d -> %d\n"
        "channelmask %d -> %d\n"
        , fmt->bps, plugin.fmt.bps
        , fmt->is_float, plugin.fmt.is_float
        , fmt->channels, plugin.fmt.channels
        , fmt->samplerate, plugin.fmt.samplerate
        , fmt->channelmask, plugin.fmt.channelmask
        );
    }
    LOCK;
    int s = state;
    state = OUTPUT_STATE_STOPPED;
    snd_pcm_drop (audio);
    int ret = palsa_set_hw_params (&requested_fmt);
    if (ret < 0) {
        trace ("palsa_setformat: impossible to set requested format\n");
        // even if it failed -- copy the format
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
        UNLOCK;
        return -1;
    }
    trace ("new format %dbit %s %dch %dHz channelmask=%X\n", plugin.fmt.bps, plugin.fmt.is_float ? "float" : "int", plugin.fmt.channels, plugin.fmt.samplerate, plugin.fmt.channelmask);

    int res = -1;
    switch (s) {
    case OUTPUT_STATE_STOPPED:
        res = palsa_stop ();
        break;
    case OUTPUT_STATE_PLAYING:
        res = palsa_play ();
        break;
    case OUTPUT_STATE_PAUSED:
        if (0 != palsa_play ()) {
            res = -1;
        }
        if (0 != palsa_pause ()) {
            res = -1;
        }
        break;
    }
    trace ("alsa_formatchanged=1\n");
    alsa_formatchanged = 1;
    UNLOCK;
    return res;
}

int
palsa_free (void) {
    trace ("palsa_free\n");
    if (audio && !alsa_terminate) {
        LOCK;
        alsa_terminate = 1;
        UNLOCK;
        trace ("waiting for alsa thread to finish\n");
        if (alsa_tid) {
            deadbeef->thread_join (alsa_tid);
            alsa_tid = 0;
        }
        snd_pcm_close(audio);
        audio = NULL;
        if (mutex) {
            deadbeef->mutex_free (mutex);
            mutex = 0;
        }
        state = OUTPUT_STATE_STOPPED;
        alsa_terminate = 0;
    }
    return 0;
}

static void
palsa_hw_pause (int pause) {
    if (!audio) {
        return;
    }
    if (state == OUTPUT_STATE_STOPPED) {
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
    if (state == OUTPUT_STATE_STOPPED) {
        if (!audio) {
            if (palsa_init () < 0) {
                state = OUTPUT_STATE_STOPPED;
                return -1;
            }
        }
        else {
            if ((err = snd_pcm_prepare (audio)) < 0) {
                fprintf (stderr, "cannot prepare audio interface for use (%d, %s)\n",
                        err, snd_strerror (err));
                return -1;
            }
        }
    }
    if (state != OUTPUT_STATE_PLAYING) {
        LOCK;
//        trace ("alsa: installing async handler\n");
//        if (snd_async_add_pcm_handler (&pcm_callback, audio, alsa_callback, NULL) < 0) {
//            perror ("snd_async_add_pcm_handler");
//        }
//        trace ("pcm_callback=%p\n", pcm_callback);
        snd_pcm_start (audio);
        UNLOCK;
        state = OUTPUT_STATE_PLAYING;
    }
    return 0;
}


int
palsa_stop (void) {
    if (!audio) {
        return 0;
    }
    state = OUTPUT_STATE_STOPPED;
    LOCK;
    snd_pcm_drop (audio);
#if 0
    if (pcm_callback) {
        snd_async_del_handler (pcm_callback);
        pcm_callback = NULL;
    }
#endif
    UNLOCK;
    deadbeef->streamer_reset (1);
    DB_playItem_t *ts = deadbeef->streamer_get_streaming_track ();
    DB_playItem_t *tp = deadbeef->streamer_get_playing_track ();
    if (deadbeef->conf_get_int ("alsa.freeonstop", 0) && !ts && !tp)  {
        palsa_free ();
        trace ("\033[0;31malsa released!\033[37;0m\n");
    }
    else {
        trace ("\033[0;32malsa not released!\033[37;0m\n");
    }
    if (tp) {
        deadbeef->pl_item_unref (tp);
    }
    if (ts) {
        deadbeef->pl_item_unref (ts);
    }
    return 0;
}

int
palsa_pause (void) {
    if (state == OUTPUT_STATE_STOPPED || !audio) {
        return -1;
    }
    // set pause state
    LOCK;
    palsa_hw_pause (1);
    UNLOCK;
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

int
palsa_unpause (void) {
    // unset pause state
    if (state == OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PLAYING;
        LOCK;
        palsa_hw_pause (0);
        UNLOCK;
    }
    return 0;
}

static void
palsa_thread (void *context) {
    prctl (PR_SET_NAME, "deadbeef-alsa", 0, 0, 0, 0);
    int err;
    for (;;) {
        if (alsa_terminate) {
            break;
        }
        if (state != OUTPUT_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1)) {
            usleep (10000);
            continue;
        }
        LOCK;
        if (alsa_formatchanged) {
            trace ("handled alsa_formatchanged [1]\n");
            alsa_formatchanged = 0;
            UNLOCK;
            continue;
        }
        char buf[period_size * (plugin.fmt.bps>>3) * plugin.fmt.channels];
        int bytes_to_write = 0;
        
        /* find out how much space is available for playback data */
        snd_pcm_sframes_t frames_to_deliver = snd_pcm_avail_update (audio);

        // FIXME: pushing data without waiting for next buffer will drain entire
        // streamer buffer, and might lead to stuttering
        // however, waiting for buffer does a lot of cpu wakeups
        while (/*state == OUTPUT_STATE_PLAYING*/frames_to_deliver >= period_size) {
            if (alsa_terminate) {
                break;
            }
            err = 0;
            if (!bytes_to_write) {
                UNLOCK; // holding a lock here may cause deadlock in the streamer
                bytes_to_write = palsa_callback (buf, period_size * (plugin.fmt.bps>>3) * plugin.fmt.channels);
                LOCK;
                if (OUTPUT_STATE_PLAYING != state || alsa_terminate) {
                    break;
                }
            }

            if (bytes_to_write >= (plugin.fmt.bps>>3) * plugin.fmt.channels) {
                UNLOCK;
                err = snd_pcm_writei (audio, buf, snd_pcm_bytes_to_frames(audio, bytes_to_write));
                LOCK;
                if (alsa_formatchanged) {
                    trace ("handled alsa_formatchanged [2]\n");
                    alsa_formatchanged = 0;
                    UNLOCK;
                    break;
                }
                if (alsa_terminate) {
                    break;
                }
            }
            else {
                UNLOCK;
                usleep (10000);
                bytes_to_write = 0;
                LOCK;
                if (alsa_formatchanged) {
                    trace ("handled alsa_formatchanged [3]\n");
                    alsa_formatchanged = 0;
                    break;
                }
                continue;
            }

            if (err < 0) {
                if (err == -ESTRPIPE) {
                    fprintf (stderr, "alsa: trying to recover from suspend... (error=%d, %s)\n", err,  snd_strerror (err));
                    while ((err = snd_pcm_resume(audio)) == -EAGAIN) {
                        sleep(1); /* wait until the suspend flag is released */
                    }
                    if (err < 0) {
                        err = snd_pcm_prepare(audio);
                        if (err < 0) {
                            fprintf (stderr, "Can't recovery from suspend, prepare failed: %s", snd_strerror(err));
                            exit (-1);
                        }
                    }
            //        deadbeef->sendmessage (DB_EV_REINIT_SOUND, 0, 0, 0);
            //        break;
                }
                else {
                    //if (err != -EPIPE) {
                    //    fprintf (stderr, "alsa: snd_pcm_writei error=%d, %s\n", err, snd_strerror (err));
                    //}
                    snd_pcm_prepare (audio);
                    snd_pcm_start (audio);
                }
                continue;
            }
            bytes_to_write = 0;
            frames_to_deliver = snd_pcm_avail_update (audio);
        }
        UNLOCK;
        int sleeptime = period_size-frames_to_deliver;
        if (sleeptime > 0 && plugin.fmt.samplerate > 0 && plugin.fmt.channels > 0) {
            usleep (sleeptime * 1000 / plugin.fmt.samplerate * 1000);
        }
    }
}

static int
palsa_callback (char *stream, int len) {
    return deadbeef->streamer_read (stream, len);
}

static int
alsa_configchanged (void) {
    deadbeef->conf_lock ();
    int alsa_resample = deadbeef->conf_get_int ("alsa.resample", 1);
    const char *alsa_soundcard = deadbeef->conf_get_str_fast ("alsa_soundcard", "default");
    int buffer = deadbeef->conf_get_int ("alsa.buffer", DEFAULT_BUFFER_SIZE);
    int period = deadbeef->conf_get_int ("alsa.period", DEFAULT_PERIOD_SIZE);
    if (audio &&
            (alsa_resample != conf_alsa_resample
            || strcmp (alsa_soundcard, conf_alsa_soundcard)
            || buffer != req_buffer_size
            || period != req_period_size)) {
        trace ("alsa: config option changed, restarting\n");
        deadbeef->sendmessage (DB_EV_REINIT_SOUND, 0, 0, 0);
    }
    deadbeef->conf_unlock ();
    return 0;
}

// derived from alsa-utils/aplay.c
static void
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

static int
palsa_get_state (void) {
    return state;
}

static int
alsa_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        alsa_configchanged ();
        break;
    }
    return 0;
}

static int
alsa_start (void) {
    return 0;
}

static int
alsa_stop (void) {
    return 0;
}

DB_plugin_t *
alsa_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static const char settings_dlg[] =
    "property \"Use ALSA resampling\" checkbox alsa.resample 1;\n"
    "property \"Release device while stopped\" checkbox alsa.freeonstop 0;\n"
    "property \"Preferred buffer size\" entry alsa.buffer " DEFAULT_BUFFER_SIZE_STR ";\n"
    "property \"Preferred period size\" entry alsa.period " DEFAULT_PERIOD_SIZE_STR ";\n"
;

// define plugin interface
static DB_output_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "alsa",
    .plugin.name = "ALSA output plugin",
    .plugin.descr = "plays sound through linux standard alsa library",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>\n"
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
    .plugin.start = alsa_start,
    .plugin.stop = alsa_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.message = alsa_message,
    .init = palsa_init,
    .free = palsa_free,
    .setformat = palsa_setformat,
    .play = palsa_play,
    .stop = palsa_stop,
    .pause = palsa_pause,
    .unpause = palsa_unpause,
    .state = palsa_get_state,
    .enum_soundcards = palsa_enum_soundcards,
};
