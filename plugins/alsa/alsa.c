/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#include <pthread.h>
#include <deadbeef/deadbeef.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

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

static int _setformat_requested;
static ddb_waveformat_t requested_fmt;
static ddb_playback_state_t state;
static uintptr_t mutex;
static intptr_t alsa_tid;

static snd_pcm_uframes_t buffer_size;
static snd_pcm_uframes_t period_size;

static snd_pcm_uframes_t req_buffer_size;
static snd_pcm_uframes_t req_period_size;

static int conf_alsa_resample = 1;
static char conf_alsa_soundcard[100] = "default";

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

static ddb_playback_state_t
palsa_get_state (void);

static int
palsa_resume_playback (void);

static int
palsa_open (void);

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
        plugin.fmt.flags &= ~DDB_WAVEFORMAT_FLAG_IS_DOP;
    }

    snd_pcm_nonblock(audio, 0);
    snd_pcm_drain (audio);
    snd_pcm_nonblock(audio, 1);

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
        fprintf (stderr, "cannot set sample format to %d bps (error: %s), trying all supported formats\n", plugin.fmt.bps, snd_strerror (err));

        int fmt_cnt[] = { 16, 24, 32, 32, 8 };
#if WORDS_BIGENDIAN
        int fmt[] = { SND_PCM_FORMAT_S16_BE, SND_PCM_FORMAT_S24_3BE, SND_PCM_FORMAT_S32_BE, SND_PCM_FORMAT_FLOAT_BE, SND_PCM_FORMAT_S8, -1 };
        int fmt_dop[] = { SND_PCM_FORMAT_S24_3BE, SND_PCM_FORMAT_S32_BE, -1 };
#else
        int fmt[] = { SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_FLOAT_LE, SND_PCM_FORMAT_S8, -1 };
        int fmt_dop[] = { SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S32_LE, -1 };
#endif

        // 1st try formats with higher bps
        int i = 0;
        if(plugin.fmt.flags & DDB_WAVEFORMAT_FLAG_IS_DOP) {
            for (i = 0; fmt_dop[i] != -1; i++) {
                if (fmt_dop[i] != sample_fmt) {
                    if (snd_pcm_hw_params_set_format (audio, hw_params, fmt_dop[i]) >= 0) {
                        fprintf (stderr, "Found DoP compatible format\n");
                        sample_fmt = fmt_dop[i];
                        break;
                    }
                }
            }
        }
        else {
            for (i = 0; fmt[i] != -1; i++) {
                if (fmt[i] != sample_fmt && fmt_cnt[i] > plugin.fmt.bps) {
                    if (snd_pcm_hw_params_set_format (audio, hw_params, fmt[i]) >= 0) {
                        fprintf (stderr, "Found compatible format %d bps\n", fmt_cnt[i]);
                        sample_fmt = fmt[i];
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
                            fprintf (stderr, "Found compatible format %d bps\n", fmt_cnt[i]);
                            sample_fmt = fmt[i];
                            break;
                        }
                    }
                }
            }
        }

        if (fmt[i] == -1) {
            fprintf (stderr, "Fallback format could not be found\n");
            goto error;
        }
    }

    snd_pcm_hw_params_get_format (hw_params, &sample_fmt);
    trace ("chosen sample format: %04Xh\n", (int)sample_fmt);

    unsigned val = (unsigned)plugin.fmt.samplerate;
    int ret = 0;

    int resample = conf_alsa_resample && !(plugin.fmt.flags & DDB_WAVEFORMAT_FLAG_IS_DOP);
    if ((err = snd_pcm_hw_params_set_rate_resample (audio, hw_params, resample)) < 0) {
        fprintf (stderr, "cannot setup resampling (%s)\n",
                snd_strerror (err));
        goto error;
    }

    if(plugin.fmt.flags & DDB_WAVEFORMAT_FLAG_IS_DOP) {
        if ((err = snd_pcm_hw_params_set_rate (audio, hw_params, val, 0)) < 0) {
            fprintf (stderr, "cannot set sample rate (%s)\n",
                    snd_strerror (err));
            goto error;
        }
    }
    else {
        if ((err = snd_pcm_hw_params_set_rate_near (audio, hw_params, &val, &ret)) < 0) {
            fprintf (stderr, "cannot set sample rate (%s)\n",
                    snd_strerror (err));
            goto error;
        }
        plugin.fmt.samplerate = val;
    }
    trace ("chosen samplerate: %d Hz\n", val);

    unsigned chanmin, chanmax;
    snd_pcm_hw_params_get_channels_min (hw_params, &chanmin);
    snd_pcm_hw_params_get_channels_max (hw_params, &chanmax);

    trace ("minchan: %d, maxchan: %d\n", chanmin, chanmax);
    unsigned nchan = (unsigned)plugin.fmt.channels;
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
    default:
        fprintf (stderr, "Unsupported sample format %d\n", sample_fmt);
        goto error;
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

static int
palsa_open (void) {
    int err;

    // get and cache conf variables
    conf_alsa_resample = deadbeef->conf_get_int ("alsa.resample", 1);
    deadbeef->conf_get_str ("alsa_soundcard", "default", conf_alsa_soundcard, sizeof (conf_alsa_soundcard));
    trace ("alsa_soundcard: %s\n", conf_alsa_soundcard);

    snd_pcm_sw_params_t *sw_params = NULL;
    if ((err = snd_pcm_open (&audio, conf_alsa_soundcard, SND_PCM_STREAM_PLAYBACK, 0))) {
        fprintf (stderr, "could not open audio device (%s)\n",
                snd_strerror (err));
        return -1;
    }

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


    if ((err = snd_pcm_sw_params (audio, sw_params)) < 0) {
        fprintf (stderr, "cannot set software parameters (%s)\n",
                snd_strerror (err));
        goto open_error;
    }
    snd_pcm_sw_params_free (sw_params);
    sw_params = NULL;

    if ((err = snd_pcm_prepare (audio)) < 0) {
        fprintf (stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror (err));
        goto open_error;
    }

    return 0;

open_error:
    if (sw_params) {
        snd_pcm_sw_params_free (sw_params);
    }
    if (audio != NULL) {
        snd_pcm_drop (audio);
        snd_pcm_close (audio);
        audio = NULL;
    }

    return -1;
}

static int
palsa_init (void) {
    if (palsa_open () != 0) {
        return -1;
    }

    alsa_terminate = 0;
    alsa_tid = deadbeef->thread_start (palsa_thread, NULL);

    return 0;
}

static int
_setformat_apply (void) {
    _setformat_requested = 0;

    trace ("palsa_setformat %dbit %s %dch %dHz channelmask=%X\n", requested_fmt.bps, requested_fmt.is_float ? "float" : "int", requested_fmt.channels, requested_fmt.samplerate, requested_fmt.channelmask);
    if (!audio
        || !memcmp (&requested_fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        return 0;
    }
    else {
        trace ("switching format to (requsted -> actual):\n"
        "bps %d -> %d\n"
        "is_float %d -> %d\n"
        "channels %d -> %d\n"
        "samplerate %d -> %d\n"
        "channelmask %d -> %d\n"
        , requested_fmt.bps, plugin.fmt.bps
        , requested_fmt.is_float, plugin.fmt.is_float
        , requested_fmt.channels, plugin.fmt.channels
        , requested_fmt.samplerate, plugin.fmt.samplerate
        , requested_fmt.channelmask, plugin.fmt.channelmask
        );
    }

    if (audio != NULL) {
        trace ("alsa: new format: draining...\n");
        snd_pcm_nonblock(audio, 0);
        snd_pcm_drain (audio);
        snd_pcm_nonblock(audio, 1);
        trace ("alsa: new format: reinitializing\n");
        snd_pcm_drop (audio);
        snd_pcm_close (audio);
        audio = NULL;
        if (palsa_open() < 0) {
            return -1;
        }
    }


    int ret = palsa_set_hw_params (&requested_fmt);
    if (ret < 0) {
        trace ("palsa_setformat: impossible to set requested format\n");
        // even if it failed -- copy the format
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
        return -1;
    }
    trace ("new format %dbit %s %dch %dHz channelmask=%X\n", plugin.fmt.bps, plugin.fmt.is_float ? "float" : "int", plugin.fmt.channels, plugin.fmt.samplerate, plugin.fmt.channelmask);

    switch (state) {
    case DDB_PLAYBACK_STATE_STOPPED:
    case DDB_PLAYBACK_STATE_PAUSED:
        return 0;
    case DDB_PLAYBACK_STATE_PLAYING:
        return palsa_resume_playback();
    }

    return 0;
}

static int
palsa_setformat (ddb_waveformat_t *fmt) {
    LOCK;
    _setformat_requested = 1;
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    UNLOCK;
    return 0;
}

static int
palsa_free (void) {
    trace ("palsa_free\n");
    LOCK;
    if (!alsa_tid) {
        UNLOCK;
        return 0;
    }

    alsa_terminate = 1;
    UNLOCK;
    deadbeef->thread_join (alsa_tid);
    return 0;
}

static void
palsa_hw_pause (int pause) {
    if (!audio) {
        return;
    }
    if (state == DDB_PLAYBACK_STATE_STOPPED) {
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

static int
palsa_resume_playback (void) {
    int err = snd_pcm_prepare (audio);
    if (err < 0) {
        fprintf (stderr, "snd_pcm_prepare: %s\n", snd_strerror (err));
        return err;
    }
    snd_pcm_start (audio);
    return 0;
}

static int
palsa_play (void) {
    int err = 0;
    LOCK;
    if (!audio) {
        err = palsa_init ();
    }
    if (err < 0) {
        UNLOCK;
        return err;
    }
    state = DDB_PLAYBACK_STATE_STOPPED;
    err = snd_pcm_drop (audio);
    if (err < 0) {
        UNLOCK;
        fprintf (stderr, "snd_pcm_drop: %s\n", snd_strerror (err));
        return err;
    }
    err = palsa_resume_playback ();
    if (err != 0) {
        UNLOCK;
        return -1;
    }
    state = DDB_PLAYBACK_STATE_PLAYING;
    UNLOCK;
    return 0;
}


static int
palsa_stop (void) {
    if (!audio) {
        return 0;
    }

    LOCK;
    state = DDB_PLAYBACK_STATE_STOPPED;
    UNLOCK;

    snd_pcm_drop (audio);

    palsa_free ();
    return 0;
}

int
palsa_pause (void) {
    int err = 0;
    LOCK;
    if (!audio) {
        err = palsa_init ();
    }
    if (err < 0) {
        UNLOCK;
        return err;
    }
    // set pause state
    palsa_hw_pause (1);
    state = DDB_PLAYBACK_STATE_PAUSED;
    UNLOCK;
    return 0;
}

static int
palsa_unpause (void) {
    // unset pause state
    LOCK;
    if (!audio) {
        if (palsa_init ()) {
            UNLOCK;
            return -1;
        }
        if (palsa_play ()) {
            UNLOCK;
            return -1;
        }
    }
    else if (state == DDB_PLAYBACK_STATE_PAUSED) {
        state = DDB_PLAYBACK_STATE_PLAYING;
        palsa_hw_pause (0);
    }
    UNLOCK;
    return 0;
}

// Returns:
// 0: successfully recovered
// -1: failed to recover -- playback stopped
// 1: no recovery possible
static int
alsa_recover (int err) {
    // these errors are auto-fixed by snd_pcm_recover
    if (err == -EINTR || err == -EPIPE || err == -ESTRPIPE) {
        trace ("alsa_recover: %d: %s\n", err, snd_strerror (err));
        err = snd_pcm_recover (audio, err, 1);
        if (err < 0) {
            trace ("snd_pcm_recover: %d: %s\n", err, snd_strerror (err));
            return -1; // failed to handle the error
        }
    }
    else {
        trace ("alsa_recover: ignored error %d: %s\n", err, snd_strerror (err));
        return 1; // error is unknown / ignored
    }
    return err;
}

static void
palsa_thread (void *context) {
    prctl (PR_SET_NAME, "deadbeef-alsa", 0, 0, 0, 0);
    int err = 0;
    int avail;
    for (;;) {
        if (alsa_terminate) {
            break;
        }

        LOCK;

        if (state != DDB_PLAYBACK_STATE_PLAYING) {
            UNLOCK;
            usleep (10000);
            continue;
        }

        // setformat
        int res = 0;
        if (_setformat_requested) {
            res = _setformat_apply ();
        }

        if (res != 0) {
            deadbeef->thread_detach (alsa_tid);
            alsa_terminate = 1;
            UNLOCK;
            break;
        }

        res = 0;
        // wait for buffer
        avail = snd_pcm_avail_update (audio);
        if (avail < 0) {
            //fprintf (stderr, "snd_pcm_avail_update err %d (%s)\n", avail, snd_strerror (avail));
            avail = alsa_recover (avail);
        }
        if (avail < 0) {
            UNLOCK;
            usleep (10000);
            continue;
        }
        int maxwait = period_size * 1000 / plugin.fmt.samplerate;
        if (avail >= period_size) {
            int sz = avail * (plugin.fmt.bps>>3) * plugin.fmt.channels;
            char buf[sz];

            int br = palsa_callback (buf, sz);

            int err = 0;
            int frames = snd_pcm_bytes_to_frames(audio, br);

            err = snd_pcm_writei (audio, buf, frames);

            if (err < 0) {
                err = alsa_recover (err);

                if (!err) {
                    int new_avail = snd_pcm_avail_update (audio);
                    avail = new_avail;
                    UNLOCK;
                    continue;
                }
            }
            avail -= period_size;
        }

        UNLOCK;

        // sleep up to 1 period
        if (avail < 0) {
           avail = 0;
        }
        else if (avail > period_size) {
            continue;
        }

        int frames = period_size - avail;
        int ms = frames * 1000 / plugin.fmt.samplerate;
        usleep (ms * 1000);
    }

    LOCK;
    if (audio != NULL) {
        snd_pcm_close(audio);
        audio = NULL;
    }
    alsa_terminate = 0;
    alsa_tid = 0;
    UNLOCK;
}

static int
palsa_callback (char *stream, int len) {
    if (state != DDB_PLAYBACK_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1)) {
        memset (stream, 0, len);
        return len;
    }
    int bytesread = deadbeef->streamer_read (stream, len);

    if (bytesread < len) {
        if (bytesread < 0) {
            bytesread = 0;
        }
        memset (stream + bytesread, 0, len-bytesread);
        bytesread = len;
    }
    return bytesread;
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

static ddb_playback_state_t
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
    mutex = deadbeef->mutex_create ();
    return 0;
}

static int
alsa_stop (void) {
    if (mutex) {
        deadbeef->mutex_free (mutex);
        mutex = 0;
    }
    return 0;
}

DB_plugin_t *
alsa_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static const char settings_dlg[] =
    "property \"Use ALSA resampling\" checkbox alsa.resample 1;\n"
    "property \"Preferred buffer size\" entry alsa.buffer " DEFAULT_BUFFER_SIZE_STR ";\n"
    "property \"Preferred period size\" entry alsa.period " DEFAULT_PERIOD_SIZE_STR ";\n"
;

// define plugin interface
static DB_output_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
//    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id = "alsa",
    .plugin.name = "ALSA output plugin",
    .plugin.descr = "plays sound through linux standard alsa library",
    .plugin.copyright =
        "Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
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
