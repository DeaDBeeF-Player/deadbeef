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

#define DEFAULT_BUFFER_SIZE 8192
#define DEFAULT_BUFFER_SIZE_STR "8192"

//#define trace(...) {deadbeef->log (__VA_ARGS__);}
//#define trace(...) { fprintf(stdout, __VA_ARGS__); }
#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 1, __VA_ARGS__); }
#define warn(...) {deadbeef->log (__VA_ARGS__);}
#define info(...) { deadbeef->log_detailed (&plugin.plugin, 1, __VA_ARGS__); }

// This is my 4th rewrite of portaudio plugin. Ok, actually 5th, because idea in 4th rewrite didn't work.

static PaSampleFormat
pa_GetSampleFormat (int bps, int is_float);

static void
pa_SetDefault ();

static void
portaudio_stream_start (void);

static void
portaudio_thread (void *context);

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

static void
pa_stream_finished_callback (void *uData);

static DB_output_t plugin;
DB_functions_t *deadbeef;
static uintptr_t mutex;
static intptr_t portaudio_tid;

static ddb_playback_state_t state;

// actual stream
static PaStream *stream = 0;
// we use plugin.fmt as fmt information
static PaStreamParameters stream_parameters;
// stream control
static struct uData * userData;
// requested stream
static ddb_waveformat_t requested_fmt;

#define STREAMS_TO_CLOSE_MAX 15
#define LOOP_CLOSE 10
// streams to be closed in callback later
static PaStream * streams_to_close[STREAMS_TO_CLOSE_MAX] = {NULL, NULL};


#define P_UDATA(x) (*((struct uData *) x))

#define STREAM_CONTINUE 0
#define STREAM_COMPLETE 1
#define STREAM_ABORT 2

struct uData {
    // tells callback to finish stream (through STREAM_* definitions)
    int abort;
    // 'thread' number which can help with debugging
    int num;
    // loop number
    int i;
    // stream uData belongs to
    PaStream *stream;
    // adds stream to terminate queue when uData reaches pa_stream_finished_callback
    unsigned char terminate;
    // size of the frame
    int framesize;
    // buffer size
    unsigned long buffer_size;
};

unsigned char num_assign = 0;

// portaudio_stream_start opens a stream using stream_parameters and plugin.fmt information
static void
portaudio_stream_start (void) {
    //trace ("portaudio_stream_start\n");

    deadbeef->mutex_lock (mutex);

    // Use default device if none selected
    {
        deadbeef->conf_lock ();
        const char * portaudio_soundcard_string = deadbeef->conf_get_str_fast ("portaudio_soundcard", "default");
        if (strcmp(portaudio_soundcard_string, "default") == 0) {
            stream_parameters.device = Pa_GetDefaultOutputDevice ();
            deadbeef->conf_unlock ();
        }
        else {
            deadbeef->conf_unlock ();
            stream_parameters.device = deadbeef->conf_get_int ("portaudio_soundcard", -1);
        }
    }
    static struct uData * uData;
    uData = calloc(1,sizeof(struct uData));
    uData->num = num_assign++;
    userData = uData;

    // Using paFramesPerBufferUnspecified with alsa gives warnings about underrun
    int buffer_size_config = deadbeef->conf_get_int ("portaudio.buffer", DEFAULT_BUFFER_SIZE);
    if (buffer_size_config == -1)
        uData->buffer_size = paFramesPerBufferUnspecified;
    else
        uData->buffer_size = buffer_size_config;

    trace ("portaudio_stream_start [%d]: buffer size %lu\n", uData->num, uData->buffer_size);
    /* Open an audio I/O stream. */
    PaError err;
    err = Pa_OpenStream (       &stream,                        // stream pointer
                                NULL,                           // inputParameters
                                &stream_parameters,             // outputParameters
                                plugin.fmt.samplerate,          // sampleRate
                                uData->buffer_size,             // framesPerBuffer
                                paNoFlag,                       // flags
                                portaudio_callback,             // callback
                                uData);                         // userData

    if (err != paNoError) {
        trace("Failed to open stream. %s\n", Pa_GetErrorText(err));
        deadbeef->mutex_unlock (mutex);
        return;
    }
    uData->stream = stream;
    uData->framesize =  plugin.fmt.channels*plugin.fmt.bps/8;
    err = Pa_SetStreamFinishedCallback (stream, pa_stream_finished_callback);
    if (err != paNoError) {
        trace ("Failed to set stream finished callback. %s\n", Pa_GetErrorText(err));
    }
    deadbeef->mutex_unlock (mutex);

    return;
}

static void
pa_stream_finished_callback (void *uData) {
    trace ("pa_stream_finished_callback %x\n",uData); 
    if (P_UDATA(uData).terminate) {
        int i;
        for (i = 0; i < STREAMS_TO_CLOSE_MAX; i++) {
            if (i+1 == STREAMS_TO_CLOSE_MAX) {
                // flush list
                warn ("pa_stream_finished_callback: streams_to_close full, flushing\n");
                portaudio_tid = deadbeef->thread_start (portaudio_thread, NULL);
                while (streams_to_close[0] != NULL)
                    usleep (20000);
                i = 0;
            }
            if (streams_to_close[i] != NULL)
                continue;
            trace ("settings stream to close on pos %d\n",i);
            streams_to_close[i++] = P_UDATA(uData).stream;
            streams_to_close[i] = NULL;
            break;
        }
        free (uData);
        return;
    }
}

static void
portaudio_thread (void *context) {
    int i;
    for (i = 0; i < STREAMS_TO_CLOSE_MAX; i++) {
        if (streams_to_close[i] == NULL)
            break;
        trace ("portaudio_thread: closing stream No. %d\n", i);
        PaError err;
        err = Pa_CloseStream (streams_to_close[i]);
        if (err != paNoError) {
            trace ("Failed to close stream. %s\n", Pa_GetErrorText(err));
        }
    }
    memset (&streams_to_close, 0, STREAMS_TO_CLOSE_MAX * sizeof(PaStream *));
    return;
}

static int
portaudio_init (void) {
    // this function is for now left for no reason
    trace ("portaudio_init\n");
    //Pa_Sleep (1000);
    return 0;
}

// since we can't change stream parameters, we have to abort actual stream, set values and start streaming again
static int
portaudio_setformat (ddb_waveformat_t *fmt) {
    memcpy (&requested_fmt, fmt, sizeof (ddb_waveformat_t));
    trace ("portaudio_setformat %dbit %s %dch %dHz channelmask=%X\n", requested_fmt.bps, fmt->is_float ? "float" : "int", fmt->channels, fmt->samplerate, fmt->channelmask);

    if (!memcmp (&requested_fmt, &plugin.fmt, sizeof (ddb_waveformat_t))) {
        trace ("portaudio_setformat ignored\n");
        return 0;
    }
    else {
        trace ("switching format: (requested->actual)\n"
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

    // Tell ongoing thread to abort stream (if any)
    if (userData) {
        trace ("portaudio_setformat: abort [%d]\n",userData->num);
        // Pa_StopStream takes too much time to do.
        //Pa_StopStream (stream);
        userData->terminate = 1;
        userData->abort = STREAM_COMPLETE;
        userData = 0;
    }

    memcpy (&plugin.fmt, &requested_fmt, sizeof (ddb_waveformat_t));
    
    // Set new values for new stream
    // TODO: get which device was requested?
    PaError err;
    stream_parameters.device = Pa_GetDefaultOutputDevice ();
    stream_parameters.channelCount = plugin.fmt.channels;
    stream_parameters.sampleFormat = pa_GetSampleFormat (plugin.fmt.bps,plugin.fmt.is_float);
    stream_parameters.suggestedLatency = 0.0;
    stream_parameters.hostApiSpecificStreamInfo = NULL;
    
    err = Pa_IsFormatSupported (NULL, &stream_parameters, plugin.fmt.samplerate);
    if (err != paNoError) {
        trace ("Unsupported format. %s\n", Pa_GetErrorText(err));
        // even if it failed -- continue
    }

    // start new stream if was playing before
    if (stream) {
        stream = 0;
        portaudio_stream_start ();
        err = Pa_StartStream (stream);
        if (err != paNoError) {
            trace ("Failed to start stream. %s\n", Pa_GetErrorText(err));
            state = DDB_PLAYBACK_STATE_STOPPED;
            return -1;
        }
        trace ("portaudio_setformat: Started stream.\n");
        state = DDB_PLAYBACK_STATE_PLAYING;
    }
    return 0;
}

static int
portaudio_free (void) {
    // called when plugin changes
    trace("portaudio_free\n");
    if (stream) {
        /*if (userData)
            userData->abort = STREAM_ABORT;
        */
        PaError err;
        err = Pa_AbortStream (stream);
        if (err != paNoError) {
            trace ("Failed to abort stream. %s\n", Pa_GetErrorText(err));
        }
        trace ("portaudio_free: closing stream No. %d\n", userData->num);
        err = Pa_CloseStream (stream);
        stream = 0;
        if (err != paNoError) {
            trace ("Failed to close stream. %s\n", Pa_GetErrorText(err));
        }
    }
    return 0;
}

static int
portaudio_play (void) {
    trace ("portaudio_play\n");
    if (!stream) {
        trace ("portaudio_play: opening stream\n");
        portaudio_stream_start ();
    }
    state = DDB_PLAYBACK_STATE_PLAYING;
    userData->abort = STREAM_CONTINUE;
    if (!Pa_IsStreamActive(stream)) {
        PaError err;
        err = Pa_StartStream (stream);
        if (err != paNoError) {
            trace ("Failed to start stream. %s\n", Pa_GetErrorText(err));
            state = DDB_PLAYBACK_STATE_STOPPED;
            return -1;
        }
        trace ("portaudio_play: Started stream.\n");
    }
    return 0;
}

static PaSampleFormat pa_GSFerr () { warn ("portaudio: Sample format wrong? Using Int16.\n"); return 0; }
static PaSampleFormat
pa_GetSampleFormat (int bps, int is_float) {
    return bps ==  8 ?  paUInt8   :
           bps == 16 ?  paInt16   :
           bps == 24 ?  paInt24   :
           bps == 32 && is_float  ? paFloat32 :
           bps == 32 ?  paInt32   :
           pa_GSFerr ();
}

static int
portaudio_stop (void) {
    trace ("portaudio_stop\n");
    if (state == DDB_PLAYBACK_STATE_STOPPED) {
        return -1;
    }
    if (stream) {
        PaError err;
        if (!Pa_IsStreamStopped(stream)) {
            err = Pa_AbortStream (stream);
            if (err != paNoError) {
                trace ("Failed to abort stream. %s\n", Pa_GetErrorText(err));
                return -1;
            }
        }
    }
    state = DDB_PLAYBACK_STATE_STOPPED;
    deadbeef->streamer_reset (1);
    return 0;
}

static int
portaudio_pause (void) {
    trace ("portaudio_pause\n");
    if (state == DDB_PLAYBACK_STATE_STOPPED) {
        // If option 'Resume previous session on startup' is enabled deadbeef will call:
        // portaudio_stop() and portaudio_pause() (no portaudio_play()).
        // If it's the case we should just set the state and do the main job later.
        state = DDB_PLAYBACK_STATE_PAUSED;
        return 0;
    }
    PaError err;
    err = Pa_AbortStream (stream);
    if (err != paNoError) {
        trace ("Failed to pause stream. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    // set pause state
    state = DDB_PLAYBACK_STATE_PAUSED;
    return 0;
}

static int
portaudio_unpause (void) {
    trace ("portaudio_unpause\n");
    if (!(state == DDB_PLAYBACK_STATE_PAUSED)) {
        return -1;
    }
    return portaudio_play ();
}

/*
static int portaudio_get_endiannerequested_fmt (void) {
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
}
*/

static int
portaudio_configchanged (void) {
    int portaudio_soundcard = 0;
    {
        deadbeef->conf_lock ();
        const char * portaudio_soundcard_string = deadbeef->conf_get_str_fast ("portaudio_soundcard", "default");
        if (strcmp(portaudio_soundcard_string, "default") == 0) {
            portaudio_soundcard = Pa_GetDefaultOutputDevice ();
            deadbeef->conf_unlock ();
        }
        else {
            deadbeef->conf_unlock ();
            portaudio_soundcard = deadbeef->conf_get_int ("portaudio_soundcard", -1);
        }
    }
    int buffer = deadbeef->conf_get_int ("portaudio.buffer", DEFAULT_BUFFER_SIZE);
    if ((stream && Pa_IsStreamActive (stream) && portaudio_soundcard != stream_parameters.device) || (userData && userData->buffer_size != buffer)) {
        trace ("portaudio: config option changed, restarting\n");
        deadbeef->sendmessage (DB_EV_REINIT_SOUND, 0, 0, 0);
    }
    return 0;
}

static void portaudio_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata) {
    PaDeviceIndex  device_count = Pa_GetDeviceCount ();
    if (!device_count)
        warn ("portaudio: no devices found?\n");

    PaDeviceIndex i = 0;
    trace ("portaudio_enum_soundcards have %d devices\n",device_count);
    // Charset conversion
    #ifdef __MINGW32__
    char * charset = 0;
    int devenc_list = deadbeef->conf_get_int ("portaudio.devenc_list", 0);
    if (devenc_list == 0)
        charset = "UTF-8"; // all non-utf8 chars will be discarded
    else if (devenc_list == 1)
        charset = "cp1250";
    else if (devenc_list == 2) {
        charset = malloc (255);
        if (!charset)
            return;
        deadbeef->conf_get_str ("portaudio.devenc_custom", "", charset, 255);
    }
    if (charset)
        trace ("portaudio: converting device names from charset %s\n", charset);
    #endif
    for (i=0;i<device_count;i++) {
        const PaDeviceInfo* device = Pa_GetDeviceInfo (i);
        const PaHostApiInfo * api_info = Pa_GetHostApiInfo(device->hostApi);
        if (!device) {
            warn ("portaudio: reading device info failed\n");
        }
        if (device->maxOutputChannels < 1) {
            continue;
        }
        
        char *name_converted = (char *) device->name;

        // Convert to UTF-8 on windows
        #ifdef __MINGW32__
        char name_converted_allocated = 0;
        if (api_info->type == paDirectSound || api_info->type == paMME) {
            // it turns out these APIs do not return strings in UTF-8
            if (charset && strlen(charset)) {
                name_converted = malloc (strlen(device->name) * 4);
                name_converted_allocated = 1;
                deadbeef->junk_iconv (device->name, strlen(device->name), name_converted, strlen(device->name)*4, charset, "UTF-8//IGNORE");
            }
        }
        #endif

        char full_name[255];
        snprintf (full_name, 255, "%s: %s", api_info->name, name_converted);

        if (device->name && callback) {
            char num[8];
            snprintf (num, 8, "%d", i);
            callback (num, full_name, userdata);
        }
        #ifdef __MINGW32__
        if (name_converted_allocated) {
            free (name_converted);
        }
        #endif
        // trace ("device: %s\n",name_converted);
    }
    #ifdef __MINGW32__
    if (devenc_list == 2 && charset) {
        free (charset);
    }
    #endif
}

static int
portaudio_callback (const void *in, void *out, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *uData ) {
    if (!deadbeef->streamer_ok_to_read (-1)) {
        trace ("portaudio_callback [%d]: wait\n",P_UDATA(uData).num);
        usleep (20000);
    }
    if (P_UDATA(uData).i == LOOP_CLOSE) {
        portaudio_tid = deadbeef->thread_start (portaudio_thread, NULL);
        P_UDATA(uData).i += 1;
    }
    else if (P_UDATA(uData).i < LOOP_CLOSE) {
        P_UDATA(uData).i += 1;
    }
    if (state != DDB_PLAYBACK_STATE_PLAYING) {
        trace ("portaudio_callback [%d]: abort\n", P_UDATA(uData).num);
        return paAbort;
    }
    // do not get data from streamer
    if (P_UDATA(uData).abort == STREAM_COMPLETE) {
        trace ("portaudio_callback [%d]: slowly aborting stream\n",P_UDATA(uData).num);
        statusFlags = paOutputUnderflow;
        memset (out, 0, framesPerBuffer * P_UDATA(uData).framesize);
        return paComplete;
    }
    deadbeef->streamer_read (out, framesPerBuffer * P_UDATA(uData).framesize);
    // check if we are supposed to play this audio data
    if (P_UDATA(uData).abort == STREAM_COMPLETE) {
        trace ("portaudio_callback [%d]: slowly aborting stream\n",P_UDATA(uData).num);
        statusFlags = paOutputUnderflow;
        memset (out, 0, framesPerBuffer * P_UDATA(uData).framesize);
        return paComplete;
    }
    else if ( P_UDATA(uData).abort == STREAM_ABORT) {
        trace ("portaudio_callback [%d]: aborting stream\n", P_UDATA(uData).num);
        return paAbort;
    }
    return paContinue;
}

static void pa_SetDefault () {
    stream_parameters.device = -1;
    stream_parameters.channelCount = plugin.fmt.channels;
    stream_parameters.sampleFormat = pa_GetSampleFormat (plugin.fmt.bps, plugin.fmt.is_float);
    stream_parameters.suggestedLatency = 0.0;
    stream_parameters.hostApiSpecificStreamInfo = NULL;
}

static ddb_playback_state_t
portaudio_get_state (void) {
    return state;
}

static int
portaudio_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CONFIGCHANGED:
        portaudio_configchanged ();
        break;
    }
    return 0;
}

static const char settings_dlg[] =
    "property \"Buffer size (-1 to use optimal value chosen by portaudio)\" entry portaudio.buffer " DEFAULT_BUFFER_SIZE_STR ";\n"
#ifdef __MINGW32__
    "property \"Device name encoding\" select[3] portaudio.devenc_list 0 \"ASCII / UTF-8\" cp1250 \"Defined below\";\n"
    "property \"Custom device name encoding\" entry portaudio.devenc_custom \"\";\n"
#endif
;

static int
p_portaudio_start (void) {
    mutex = deadbeef->mutex_create ();
    PaError err;
    err = Pa_Initialize ();
    if (err != paNoError) {
        trace ("Failed to initialize PortAudio. %s\n", Pa_GetErrorText(err));
        return -1;
    }
    pa_SetDefault ();
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
    .plugin.version_minor = 5,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "portaudio",
    .plugin.name = "PortAudio output plugin",
    .plugin.descr = "This plugin plays audio using PortAudio library.\n"
    "\n"
    "Changes in version 1.5:\n"
    "    * Fixed issues when resuming previous session.\n"
    "Changes in version 1.4:\n"
    "    * Fix device enumeration when ASCII used.\n"
    "    * Invalid characters will be discarded.\n"
    "Changes in version 1.3:\n"
    "    * Manual charset selection for device names on Windows.\n"
    "    * Changing device will change output device.\n"
    "    * Changing buffer size in settings will reset stream.\n\n"
    "Changes in version 1.1:\n"
    "    * Better format handling, less possibility of playing static",
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
    .plugin.website = "http://github.com/kuba160/ddb_portaudio",
    .plugin.start = p_portaudio_start,
    .plugin.stop = p_portaudio_stop,
    .plugin.configdialog = settings_dlg,
    .plugin.message = portaudio_message,
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
