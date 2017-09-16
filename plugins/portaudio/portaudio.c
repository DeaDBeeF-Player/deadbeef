/*
    Portaudio output plugin for DeaDBeeF Player
    Copyright (C) 2017 Jakub Wasylków

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <stdio.h>
#include <string.h>

#include "../../deadbeef.h"
#include <portaudio.h>

#ifdef __MINGW32__
#include "windows.h"
#endif

//#define trace(...) {deadbeef->log (__VA_ARGS__);}
//#define trace(...) { fprintf(stdout, __VA_ARGS__); }
#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 1, __VA_ARGS__); }
#define info(...) { deadbeef->log_detailed (&plugin.plugin, 1, __VA_ARGS__); }

static PaSampleFormat
pa_GetSampleFormat (int bps, int is_float);

static void
portaudio_thread (void *context);

static int
portaudio_init (void);

static int
portaudio_free (void);

static int
portaudio_setformat (ddb_waveformat_t *fmt);

static int
portaudio_reset (void);

static int
portaudio_play (void);

static int
portaudio_stop (void);

static int
portaudio_pause (void);

static int
portaudio_unpause (void);

static int
portaudio_callback (const void *inputBuffer, void *outputBuffer,
                    unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo* timeInfo,
                    PaStreamCallbackFlags statusFlags,
                    void *abortStreamFlag );

static DB_output_t plugin;
DB_functions_t *deadbeef;
static uintptr_t mutex;

static int state;

// actual stream
static PaStream *stream;
static ddb_waveformat_t plugin_fmt;
static PaStreamParameters stream_parameters;
static int samplesize;
static int * abortStreamFlag;

static int
portaudio_init (void) {
    trace ("portaudio_init\n");
    const PaVersionInfo *version_info = Pa_GetVersionInfo ();
    info (version_info->versionText);

    deadbeef->mutex_lock (mutex);

    PaSampleFormat sampleformat = pa_GetSampleFormat (plugin_fmt.bps, plugin_fmt.is_float);
    samplesize = Pa_GetSampleSize (sampleformat);
    trace ("portaudio_init: frame size %d\n",plugin_fmt.channels*samplesize);
    fflush(stdout);

    if (stream_parameters.device == -1) {
        stream_parameters.device = Pa_GetDefaultOutputDevice ();
    }
    if (!abortStreamFlag)
        abortStreamFlag = calloc(1,sizeof(int));
    else {
        trace ("abortStreamFlag allocated!\n");
        abortStreamFlag = calloc(1,sizeof(int));
    }
    /* Open an audio I/O stream. */
    PaError err;
    err = Pa_OpenStream (       &stream,                        // stream pointer
                                NULL,                           // inputParameters
                                &stream_parameters,             // outputParameters
                                plugin_fmt.samplerate,          // sampleRate
                                4096,                           // framesPerBuffer
                                paNoFlag,                       // flags
                                portaudio_callback,             // callback
                                abortStreamFlag);               // abortStreamFlag

    if (err != paNoError) {
        trace("Failed to open stream. %s\n", Pa_GetErrorText(err));
        deadbeef->mutex_unlock (mutex);
        return -1;
    }
    state = OUTPUT_STATE_STOPPED;
    deadbeef->mutex_unlock (mutex);
    return 0;
}

static int
portaudio_setformat (ddb_waveformat_t *fmt) {
    trace ("portaudio_setformat %dbit %s %dch %dHz channelmask=%X\n", plugin_fmt.bps, plugin_fmt.is_float ? "float" : "int", plugin_fmt.channels, plugin_fmt.samplerate, plugin_fmt.channelmask);
    memcpy (&plugin.fmt, &plugin_fmt, sizeof (ddb_waveformat_t));
    stream_parameters.device = stream_parameters.device;
    stream_parameters.channelCount = plugin_fmt.channels;
    stream_parameters.sampleFormat = pa_GetSampleFormat (plugin_fmt.bps, plugin_fmt.is_float);
    stream_parameters.suggestedLatency = 0.0;
    stream_parameters.hostApiSpecificStreamInfo = NULL;
    PaError err;
    err = Pa_IsFormatSupported (NULL, &stream_parameters, plugin_fmt.samplerate);
    if (err != paNoError) {
        trace ("Failed to change format. %s\n", Pa_GetErrorText(err));
        // even if it failed -- continue
    }
    if (state == OUTPUT_STATE_PLAYING)
        portaudio_reset ();
    return 0;
}

static int
portaudio_free (void) {
    trace("portaudio_free\n");
    if (stream){
        PaError err;
        err = Pa_AbortStream (stream);
        if (err != paNoError) {
            trace ("Failed to free stream. %s\n", Pa_GetErrorText(err));
            return -1;
        }
    }
    state = OUTPUT_STATE_STOPPED;
    return 0;
}

static int
portaudio_play (void) {
    trace ("portaudio_play\n");
    if (!stream) {
        trace ("portaudio_play: calling init\n");
        portaudio_init ();
    }

    if (!Pa_IsStreamActive(stream)){
        PaError err;
        err = Pa_StartStream (stream);
        if (err != paNoError) {
            trace ("Failed to start stream. %s\n", Pa_GetErrorText(err));
            return -1;
        }
        trace ("Starting stream.\n");
    }
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static PaSampleFormat pa_GetSampleFormat (int bps, int is_float) {
    PaSampleFormat sampleformat;
    switch (bps) {
    case 8:
        return paUInt8;
        break;
    case 16:
        return paInt16;
        break;
    case 24:
        return paInt24;
        break;
    case 32:
        if (is_float)
            return paFloat32;
        else
            return paInt32;
        break;
    };
    trace("portaudio: unknown sample format, using INT16 anyway\n");
    return paInt16;
}

static void portaudio_set_default (){
    memcpy (&plugin_fmt, &plugin.fmt, sizeof (ddb_waveformat_t));
    stream_parameters.device = -1;
    stream_parameters.channelCount = plugin_fmt.channels;
    stream_parameters.sampleFormat = pa_GetSampleFormat (plugin_fmt.bps, plugin_fmt.is_float);
    stream_parameters.suggestedLatency = 0.0;
    stream_parameters.hostApiSpecificStreamInfo = NULL;
}

static int
portaudio_stop (void) {
    if (state == OUTPUT_STATE_STOPPED) {
        return -1;
    }
    PaError err;
    err = Pa_AbortStream (stream);
    if (err != paNoError) {
        trace ("Failed to abort stream. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    state = OUTPUT_STATE_STOPPED;
    deadbeef->streamer_reset (1);
    return 0;
}

static int
portaudio_pause (void) {
    if (state == OUTPUT_STATE_STOPPED) {
        return -1;
    }
    PaError err;
    err = Pa_AbortStream (stream);
    //err = Pa_StopStream (stream);
    if (err != paNoError) {
        trace ("Failed to pause stream. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    // set pause state
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

static int
portaudio_unpause (void) {
    if (!(state == OUTPUT_STATE_PAUSED)) {
        return -1;
    }
    return portaudio_play ();
}

static int portaudio_get_endiannerequested_fmt (void) {
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
}

static int
portaudio_reset (void) {
    trace ("portaudio_reset");
    // tell running portaudio_thread to finish stream
    *abortStreamFlag = 1;
    abortStreamFlag = 0;
    // start new stream
    portaudio_init ();
    PaError err;
    err = Pa_StartStream (stream);
    if (err != paNoError) {
            trace ("Failed to start stream. %s\n", Pa_GetErrorText(err));
            return -1;
        }
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int
portaudio_callback (const void *in, void *out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *abortStreamFlag ) {
    if (!deadbeef->streamer_ok_to_read (-1)) {
        trace ("portaudio_callback: wait\n");
        usleep (10000);
    }
    if ( *((int *) abortStreamFlag) ) {
        trace ("portaudio_callback: format changed\n");
        free (abortStreamFlag);
        return paAbort;
    }
    if (state != OUTPUT_STATE_PLAYING){
        trace ("portaudio_callback: abort\n");
        return paAbort;
    }
    deadbeef->streamer_read (out, framesPerBuffer*plugin_fmt.channels*2);
    return paContinue;
}

static int
portaudio_get_state (void) {
    return state;
}

static int
p_portaudio_start (void) {
    mutex = deadbeef->mutex_create ();
    PaError err;
    err = Pa_Initialize ();
    if (err != paNoError) {
        trace ("Failed to initialize PortAudio. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    portaudio_set_default ();
    return 0;
}

static int
p_portaudio_stop (void) {
    deadbeef->mutex_free (mutex);
    PaError err;
    err = Pa_Terminate ();
    if (err != paNoError) {
        trace ("Failed to terminate PortAudio. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    return 0;
}

DB_plugin_t *
portaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

static DB_output_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 10,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "portaudio",
    .plugin.name = "PortAudio output plugin",
    .plugin.descr = "This plugin plays audio using PortAudio library.",
    .plugin.copyright =
    "PortAudio output plugin for DeaDBeeF Player\n"
    "Copyright (C) 2017 Jakub Wasylków\n"
    "\n"
    "This software is provided 'as-is', without any express or implied\n"
    "warranty.  In no event will the authors be held liable for any damages\n"
    "arising from the use of this software.\n"
    "\n"
    "Permission is granted to anyone to use this software for any purpose,\n"
    "including commercial applications, and to alter it and redistribute it\n"
    "freely, subject to the following restrictions:\n"
    "\n"
    "1. The origin of this software must not be misrepresented; you must not\n"
    " claim that you wrote the original software. If you use this software\n"
    " in a product, an acknowledgment in the product documentation would be\n"
    " appreciated but is not required.\n"
    "\n"
    "2. Altered source versions must be plainly marked as such, and must not be\n"
    " misrepresented as being the original software.\n"
    "\n"
    "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://github.com/kuba160",
    .plugin.start = p_portaudio_start,
    .plugin.stop = p_portaudio_stop,
    .init = portaudio_init,
    .free = portaudio_free,
    .setformat = portaudio_setformat,
    .play = portaudio_play,
    .stop = portaudio_stop,
    .pause = portaudio_pause,
    .unpause = portaudio_unpause,
    .state = portaudio_get_state,
    .fmt = {.samplerate = 44100, .channels = 2, .bps = 16, .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT}
};
