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
#include "portaudio.h"

//#define trace(...) {deadbeef->log (__VA_ARGS__)}
#define trace(...) { fprintf(stdout, __VA_ARGS__); }

static DB_output_t plugin;
DB_functions_t *deadbeef;

static intptr_t portaudio_tid;
static int portaudio_terminate;
static int state;
static int in_callback;

static ddb_waveformat_t plugin_fmt;
PaSampleFormat sampleformat;
static uintptr_t mutex;

PaStream *stream;
PaError err;
#define BUFFER_SIZE 32

static void
portaudio_thread (void *context);

static int
portaudio_init (void);

static int
portaudio_free (void);

int
portaudio_setformat (ddb_waveformat_t *fmt);

static int
portaudio_play (void);

static int
portaudio_stop (void);

static int
portaudio_pause (void);

static int
portaudio_unpause (void);

typedef struct
{
    float left_phase;
    float right_phase;
} paTestData;
static paTestData data;

int
portaudio_init (void) {
    trace ("portaudio_init\n");
    deadbeef->mutex_lock (mutex);
    PaError err;
    err = Pa_Initialize ();

    if( err != paNoError ){
        trace("Failed to initialize PortAudio. %s\n", Pa_GetErrorText( err ) );
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
        sampleformat = paInt16;
    }

    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream( &stream,
                                0,              /* no input channels */
                                2,              /* stereo output */
                                sampleformat,   /* output format */
                                44100,          /* Sample Rate */
                                BUFFER_SIZE,    /* frames per buffer, i.e. the number
                                                   of sample frames that PortAudio will
                                                   request from the callback. Many apps
                                                   may want to use
                                                   paFramesPerBufferUnspecified, which
                                                   tells PortAudio to pick the best,
                                                   porequested_fmtibly changing, buffer size.*/
                                NULL,           /* callback function */
                                NULL );         /*This is a pointer that will be parequested_fmted to
                                                   your callback*/
    if( err != paNoError ){
        trace("Failed to open stream. %s\n", Pa_GetErrorText( err ) );
        deadbeef->mutex_unlock (mutex);
        return -1;
    }
    state = OUTPUT_STATE_STOPPED;
    portaudio_terminate = 0;
    portaudio_tid = deadbeef->thread_start (portaudio_thread, NULL);
    trace("portaudio tid == %x\n",portaudio_tid);
    deadbeef->mutex_unlock (mutex);
    return 0;
}

int portaudio_setformat(ddb_waveformat_t *fmt)
{
    memcpy (&plugin_fmt, fmt, sizeof (ddb_waveformat_t));

    switch (plugin_fmt.bps) {
    case 8:
        sampleformat = paUInt8;
        break;
    case 16:
        sampleformat = paInt16;
        break;
    case 24:
        sampleformat = paInt24;
        break;
    case 32:
        if (plugin_fmt.is_float) {
            sampleformat = paFloat32;
        }
        else {
            sampleformat = paInt32;
        }
        break;
    default:
        return -1;
    };

    return 0;
}

int portaudio_free(void)
{
    trace("portaudio_free\n");

    state = OUTPUT_STATE_STOPPED;

    if (!portaudio_tid) {
        return 0;
    }

    if (in_callback) {
        portaudio_terminate = 1;
        return 0;
    }

    portaudio_terminate = 1;

    deadbeef->thread_join(portaudio_tid);

    return 0;
}

int
portaudio_play (void) {
    if (!portaudio_tid) {
        trace("portaudio_play: calling init\n");
        portaudio_init ();
    }
    PaError err;
    err = Pa_StartStream( stream );
    if( err != paNoError ){
        trace("Failed to start stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    trace("Starting stream.\n");
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int
portaudio_stop (void) {
    state = OUTPUT_STATE_STOPPED;
    PaError err;
    err = Pa_AbortStream( stream );
    if( err != paNoError ){
        trace("Failed to abort stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    deadbeef->streamer_reset (1);
    return 0;
}

int
portaudio_pause (void) {
    if (state == OUTPUT_STATE_STOPPED) {
        return -1;
    }
    PaError err;
    err = Pa_StopStream( stream );
    if( err != paNoError ){
        trace("Failed to pause stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    // set pause state
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

int
portaudio_unpause (void) {
    if (!(state == OUTPUT_STATE_PAUSED)) {
        return -1;
    }
    PaError err;
    err = Pa_StartStream( stream );
    if( err != paNoError ){
        trace("Failed to start stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    // set pause state
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int
portaudio_get_endiannerequested_fmt (void) {
#if WORDS_BIGENDIAN
    return 1;
#else
    return 0;
#endif
}

static void
portaudio_thread (void *context) {
#ifdef __linux__
    prctl (PR_SET_NAME, "deadbeef-portaudio", 0, 0, 0, 0);
#endif
    for (;;) {
        if (portaudio_terminate) {
            break;
        }
        if (state != OUTPUT_STATE_PLAYING || !deadbeef->streamer_ok_to_read (-1)) {
            usleep (10000);
            continue;
        }
        if(state == OUTPUT_STATE_PLAYING) {
            signed long bs = BUFFER_SIZE;//Pa_GetStreamWriteAvailable (stream);
            if (bs < 0) {
                trace ("Portaudio: error in getting number of frames ready to read\n");
                continue;
            }
            else if (bs == 0) {
                trace ("PortAudio: no frames avaliable to write, waiting\n");
                usleep (10000);
                continue;
            }
            deadbeef->mutex_lock (mutex);
            char *buf = malloc(bs);
            if(!buf){
                trace("allocating buffer failed\n");
                deadbeef->mutex_unlock (mutex);
                continue;
            }
            in_callback = 1;
            int bytesread = deadbeef->streamer_read(buf, bs);
            in_callback = 0;
            if (bytesread < 0) {
                bytesread = 0;
            }
            if (bytesread == 0){
                free(buf);
                continue;
            }

            if (bytesread < bs)
            {
                memset (buf + bytesread, 0, bs-bytesread);
            }
            if(bytesread != bs){
                trace("streamer sent other value than requested (%d != %d)\n",bs,bytesread);
            }
            PaError err = Pa_WriteStream(stream, buf, sizeof (buf));
            if( err != paNoError ){
                trace("Failed to write stream. %s\n", Pa_GetErrorText( err ) );
                free(buf);
                continue;
            }
            free(buf);
            deadbeef->mutex_unlock(mutex);

            /*int sleeptime = bytesread;
            if (sleeptime > 0 ) {
                usleep (sleeptime * 6);
            }*/
        }
    }
}

int
portaudio_get_state (void) {
    return state;
}

int
p_portaudio_start (void) {
    mutex = deadbeef->mutex_create();
    return 0;
}

int
p_portaudio_stop (void) {
    deadbeef->mutex_free(mutex);
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
    .fmt = {.samplerate = 44100, .channels = 2, .bps = 16, .channelmask = DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT}
};
