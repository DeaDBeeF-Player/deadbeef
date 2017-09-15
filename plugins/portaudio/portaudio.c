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

#define trace(...) {deadbeef->log (__VA_ARGS__);}
//#define trace(...) { fprintf(stdout, __VA_ARGS__); }
#define info(...) { deadbeef->log_detailed (&plugin.plugin, 1, __VA_ARGS__); }

static void
portaudio_thread (void *context);

static int
portaudio_init (void);

static int
portaudio_free (void);

static int
portaudio_setformat (ddb_waveformat_t *fmt);

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
                    void *userData );

static DB_output_t plugin;
DB_functions_t *deadbeef;

static int state;
static ddb_waveformat_t plugin_fmt;
static uintptr_t mutex;
static ddb_waveformat_t requested_fmt;

PaStream *stream;
int samplesize = 0;
PaDeviceIndex OutputDevice;

static int
portaudio_init (void) {
    trace ("portaudio_init\n");
    const PaVersionInfo *version_info = Pa_GetVersionInfo ();
    info (version_info->versionText);
    deadbeef->mutex_lock (mutex);
    PaError err;
    err = Pa_Initialize ();
    if (err != paNoError) {
        trace ("Failed to initialize PortAudio. %s\n", Pa_GetErrorText(err));
        deadbeef->mutex_unlock (mutex);
        return -1;
    }

    // Use generic format if no fmt
    if (!plugin_fmt.channels) {
        plugin_fmt.bps = 16;
        plugin_fmt.is_float = 0;
        plugin_fmt.channels = 2;
        plugin_fmt.samplerate = 44100;
        plugin_fmt.channelmask = 3;
    }
    PaSampleFormat sampleformat;
    switch (plugin_fmt.bps) {
    case 8:
        trace("uint8\n");
        sampleformat = paUInt8;
        samplesize = 1;
        break;
    case 16:
        trace("int16\n");
        sampleformat = paInt16;
        samplesize = 2;
        break;
    case 24:
        trace("int24\n");
        sampleformat = paInt24;
        samplesize = 3;
        break;
    case 32:
        if (plugin_fmt.is_float){
            trace("float32\n");
            sampleformat = paFloat32;
        }
        else
        {
            trace("int32\n");
            sampleformat = paInt32;
        }
        samplesize = 4;
        break;
    default:
        trace("int16def\n");
        sampleformat = paInt16;
    };
    trace ("frame size %d\n",plugin_fmt.channels*(plugin_fmt.bps/8));
    trace  ("stream open samplerate %d\n",plugin_fmt.samplerate);
    fflush(stdout);
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream ( &stream,
                                 0,                              /* no input channels */
                                 2,                              /* stereo output */
                                 sampleformat,                   /* output format */
                                 plugin_fmt.samplerate,          /* Sample Rate */
                                 paFramesPerBufferUnspecified,   /* frames per buffer, i.e. the number
                                                                 of sample frames that PortAudio will
                                                                 request from the callback. Many apps
                                                                 may want to use
                                                                 paFramesPerBufferUnspecified, which
                                                                 tells PortAudio to pick the best,
                                                                 porequested_fmtibly changing, buffer size.*/
                                 portaudio_callback,             /* callback function */
                                 NULL );                         /* pointer that will be passed to callback*/
    if (err != paNoError) {
        trace("Failed to open stream. %s\n", Pa_GetErrorText(err));
        deadbeef->mutex_unlock (mutex);
        return -1;
    }
    state = OUTPUT_STATE_STOPPED;
    deadbeef->mutex_unlock (mutex);
    return 0;
}
/*
typedef struct {
    int bps;
    int channels;
    int samplerate;
    uint32_t channelmask;
    int is_float; // bps must be 32 if this is true
    int is_bigendian;
} ddb_waveformat_t;
*/
static int
portaudio_setformat (ddb_waveformat_t *fmt) {
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    trace ("portaudio_setformat %dbit %s %dch %dHz channelmask=%X\n", requested_fmt.bps, fmt->is_float ? "float" : "int", fmt->channels, fmt->samplerate, fmt->channelmask);

    if (!memcmp (fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        trace ("portaudio_setformat ignored\n");
        return 0;
    }
    
    trace ("switching format:\n"
    "bps %d -> %d\n"
    "is_float %d -> %d\n"
    "channels %d -> %d\n"
    "samplerate %d -> %d\n"
    "channelmask %d -> %d\n"
    , plugin_fmt.bps, fmt->bps
    , plugin_fmt.is_float, fmt->is_float
    , plugin_fmt.channels, fmt->channels
    , plugin_fmt.samplerate, fmt->samplerate
    , plugin_fmt.channelmask, fmt->channelmask
    );

    PaSampleFormat sampleformat;
    switch (requested_fmt.bps) {
    case 8:
        trace("uint8\n");
        sampleformat = paUInt8;
        samplesize = 1;
        break;
    case 16:
        trace("int16\n");
        sampleformat = paInt16;
        samplesize = 2;
        break;
    case 24:
        trace("int24\n");
        sampleformat = paInt24;
        samplesize = 3;
        break;
    case 32:
        if (plugin_fmt.is_float){
            trace("float32\n");
            sampleformat = paFloat32;
        }
        else
        {
            trace("int32\n");
            sampleformat = paInt32;
        }
        samplesize = 4;
        break;
    default:
        trace("int16def\n");
        sampleformat = paInt16;
    };
    fflush(stdout);
    PaError err;
    OutputDevice = Pa_GetDefaultOutputDevice ();

    const PaStreamParameters parameters = {OutputDevice, requested_fmt.channels, sampleformat, 0.0, NULL};
    err = Pa_IsFormatSupported (NULL, &parameters, requested_fmt.samplerate);
    if (err != paNoError) {
        trace ("Failed to change format. %s\n", Pa_GetErrorText(err));
        // even if it failed -- copy the format
        memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
        return -1;
    }
    // even if it failed -- copy the format
    memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));


    return 0;
}

static int
portaudio_free (void) {
    trace("portaudio_free\n");
    state = OUTPUT_STATE_STOPPED;
    return 0;
}

static int
portaudio_play (void) {
    if (!stream) {
        trace ("portaudio_play: calling init\n");
        portaudio_init ();
    }

    PaError err;
    err = Pa_StartStream (stream);
    if (err != paNoError) {
        trace ("Failed to start stream. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    trace ("Starting stream.\n");
    state = OUTPUT_STATE_PLAYING;
    return 0;
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

static void portaudio_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata) {
    PaDeviceIndex  device_count = Pa_GetDeviceCount ();
    if (!device_count)
        trace("no devices found?\n");
    PaDeviceIndex i = 0;
    trace ("portaudio_enum_soundcards have %d devices\n",device_count);
    for (i=0;i<device_count;i++) {
        const PaDeviceInfo* device = Pa_GetDeviceInfo (i);
        if (!device) {
            trace ("reading device info failed\n");
        }
        
        char * name_charset = deadbeef->junk_detect_charset (device->name);
        char *name_converted = device->name;
        /*
        if (name_charset != NULL) {
            trace ( "name using %s charset, converting\n",name_charset);
            name_converted = malloc (strlen(device->name) * 4);
            if (name_converted) {
                deadbeef->junk_iconv (device->name, strlen(device->name), name_converted, strlen(device->name)*4, "cp1250", "UTF-8");
            }
        }
        */
        #ifdef __MINGW32__
        wchar_t wideName[255];
        int err = MultiByteToWideChar(CP_UTF8, 0, device->name, -1, wideName, 255);
        char convName[255];
        sprintf(&convName,"%ws", wideName);
        name_converted = convName;
        #endif
        if( device->name && callback)
            callback ("Something", name_converted, userdata);
        trace ("device: %s err: %d\n",name_converted,err);
    }
}


static int
portaudio_callback (const void *in, void *out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData ) {
    if (state != OUTPUT_STATE_PLAYING)
        return paAbort;
    else if (!deadbeef->streamer_ok_to_read (-1)) {
        usleep (10000);
    }
    deadbeef->streamer_read (out, framesPerBuffer*plugin_fmt.channels*samplesize);
    return paContinue;
}

static int
portaudio_get_state (void) {
    return state;
}

static int
p_portaudio_start (void) {
    mutex = deadbeef->mutex_create ();
    return 0;
}

static int
p_portaudio_stop (void) {
    deadbeef->mutex_free (mutex);
    return 0;
}

DB_plugin_t *
portaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
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
    .fmt = {.samplerate = 44100, .channels = 2, .bps = 16, .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT},
    .enum_soundcards = portaudio_enum_soundcards
};