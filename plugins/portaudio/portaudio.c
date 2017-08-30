/*
    Null output plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Alexey Yakovenko

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
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <stdio.h>
#include <string.h>
#include "../../deadbeef.h"
#include "portaudio.h"

// #define trace(...) deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__);

static DB_output_t plugin;
DB_functions_t *deadbeef;

static intptr_t null_tid;
static int null_terminate;
static int state;

PaStream *stream;
PaError err;

static int portaudio_callback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData );

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
    deadbeef->log ("portaudio_init\n");
    err = Pa_Initialize ();
    if( err != paNoError ){
        deadbeef->log("Failed to initialize PortAudio. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }

    /* Open an audio I/O stream. */

    err = Pa_OpenDefaultStream( &stream,
                                0,          /* no input channels */
                                2,          /* stereo output */
                                paInt32,  /* 32 bit floating point output */
                                44100,
                                256,        /* frames per buffer, i.e. the number
                                                   of sample frames that PortAudio will
                                                   request from the callback. Many apps
                                                   may want to use
                                                   paFramesPerBufferUnspecified, which
                                                   tells PortAudio to pick the best,
                                                   possibly changing, buffer size.*/
                                NULL, /* this is your callback function */
                                NULL ); /*This is a pointer that will be passed to
                                                   your callback*/
    if( err != paNoError ){
        deadbeef->log("Failed to open stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    state = OUTPUT_STATE_STOPPED;
    null_terminate = 0;
    null_tid = deadbeef->thread_start (portaudio_thread, NULL);
    return 0;
}

int
portaudio_setformat (ddb_waveformat_t *fmt) {

    return 0;
}

int
portaudio_free (void) {
    deadbeef->log ("portaudio_free\n");
    err = Pa_Terminate();
    if( err != paNoError ){
        deadbeef->log("Failed to initialize PortAudio. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    if (!null_terminate) {
        if (null_tid) {
            null_terminate = 1;
            deadbeef->thread_join (null_tid);
        }
        null_tid = 0;
        state = OUTPUT_STATE_STOPPED;
        null_terminate = 0;
    }
    return 0;
}

int
portaudio_play (void) {
    if (!null_tid) {
        portaudio_init ();
    }
    err = Pa_StartStream( stream );
    if( err != paNoError ){
        deadbeef->log("Failed to start stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    deadbeef->log("Starting stream.\n");
    state = OUTPUT_STATE_PLAYING;
    return 0;
}

static int
portaudio_stop (void) {
    state = OUTPUT_STATE_STOPPED;
    err = Pa_AbortStream( stream );
    if( err != paNoError ){
        deadbeef->log("Failed to abort stream. %s\n", Pa_GetErrorText( err ) );
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
    err = Pa_StopStream( stream );
    if( err != paNoError ){
        deadbeef->log("Failed to pause stream. %s\n", Pa_GetErrorText( err ) );
        return 1;
    }
    // set pause state
    state = OUTPUT_STATE_PAUSED;
    return 0;
}

int
portaudio_unpause (void) {
    // unset pause state
    if (state == OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PLAYING;
    }
    return 0;
}

static int
portaudio_get_endianness (void) {
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
        if (null_terminate) {
            break;
        }
        if (state != OUTPUT_STATE_PLAYING ) {
            usleep (10000);
            continue;
        }
        int bs = 256;
        char buf[bs];
        int bytesread = deadbeef->streamer_read(buf, bs);
        if (bytesread < 0) {
            bytesread = 0;
        }
        if (bytesread < bs)
        {
            memset (buf + bytesread, 0, bs-bytesread);
        }

        //deadbeef->mutex_lock (mutex);
        PaError err = Pa_WriteStream(stream, buf, sizeof (buf));
        //deadbeef->mutex_unlock(mutex);
    }
}

static int portaudio_callback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    paTestData *data = (paTestData*)userData;
    (void) inputBuffer; /* Prevent unused variable warning. */

    deadbeef->streamer_read (outputBuffer, framesPerBuffer);
    //deadbeef->log("portaudio_callback: buffer[0]  %x\n",(int*)outputBuffer);
    //Pa_Sleep(1*1000);
    return paContinue;
}
int
portaudio_get_state (void) {
    return state;
}

int
p_portaudio_start (void) {
    return 0;
}

int
p_portaudio_stop (void) {
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
    .plugin.website = "http://deadbeef.sf.net",
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
