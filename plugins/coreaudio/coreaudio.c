/*
    DeaDBeeF CoreAudio output plugin
    Copyright (C) 2009-2017 Alexey Yakovenko and other contributors

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

#include "../../deadbeef.h"
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>

static DB_functions_t *deadbeef;
static DB_output_t plugin;

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

static int state = OUTPUT_STATE_STOPPED;
static uint64_t mutex;
static intptr_t tid;
static int stoprequest;
#define MAXBUFFERS 8
#define BUFFERSIZE_BYTES 8192
static AudioQueueRef queue;
static AudioQueueBufferRef buffers[MAXBUFFERS];
static AudioQueueBufferRef availbuffers[MAXBUFFERS];

static int
ca_stop (void);

static int
ca_free (void) {
    ca_stop ();
    return 0;
}

static int
ca_init (void) {
    deadbeef->mutex_lock (mutex);
    state = OUTPUT_STATE_STOPPED;
    deadbeef->mutex_unlock (mutex);

    return 0;
}

static void aqOutputCallback (void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    *((AudioQueueBufferRef *)(inBuffer->mUserData)) = inBuffer;
}

static int
_initqueue (ddb_waveformat_t *fmt) {
    OSStatus err;

    AudioStreamBasicDescription req_format;

    memset (&req_format, 0, sizeof (req_format));
    req_format.mSampleRate = (Float64)fmt->samplerate;

    // audioqueue happily accepts ultra-high samplerates, but doesn't really play them
    if (req_format.mSampleRate > 192000) {
        req_format.mSampleRate = 192000;
    }
    req_format.mFormatID = kAudioFormatLinearPCM;

    if (fmt->is_float) {
        req_format.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    }
    else {
        req_format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
    }

    if (fmt->is_bigendian) {
        req_format.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
    }

    req_format.mBytesPerPacket = fmt->bps / 8 * fmt->channels;
    req_format.mFramesPerPacket = 1;
    req_format.mBytesPerFrame = fmt->bps / 8 * fmt->channels;
    req_format.mChannelsPerFrame = fmt->channels;
    req_format.mBitsPerChannel = fmt->bps;

    err = AudioQueueNewOutput(&req_format, aqOutputCallback, NULL, NULL, NULL, 0, &queue);
    if (err != noErr) {
        trace ("AudioQueueNewOutput error %x\n", err);
        return -1;
    }

    // get the new format
    AudioStreamBasicDescription actual_fmt;
    UInt32 actual_fmt_size = sizeof (actual_fmt);
    err = AudioQueueGetProperty(queue, kAudioQueueProperty_StreamDescription, &actual_fmt, &actual_fmt_size);
    if (err != noErr) {
        trace ("AudioQueueGetProperty kAudioQueueProperty_StreamDescription error %x\n", err);
        return -1;
    }

    // set real format to the plugin
    plugin.fmt.bps = actual_fmt.mBitsPerChannel;
    plugin.fmt.channels = actual_fmt.mChannelsPerFrame;
    plugin.fmt.is_bigendian = 0;
    plugin.fmt.is_float = (actual_fmt.mFormatFlags & kAudioFormatFlagIsFloat) ? 1 : 0;
    plugin.fmt.samplerate = actual_fmt.mSampleRate;
    plugin.fmt.channelmask = 0;
    for (int c = 0; c < actual_fmt.mChannelsPerFrame; c++) {
        plugin.fmt.channelmask |= (1<<c);
    }

    for (int i = 0; i < MAXBUFFERS; i++) {
        err = AudioQueueAllocateBuffer(queue, BUFFERSIZE_BYTES, &buffers[i]);
        if (err != noErr) {
            trace ("AudioQueueAllocateBuffer error %x\n", err);
            return -1;
        }
        buffers[i]->mUserData  = availbuffers + i;
        availbuffers[i] = buffers[i];
    }

    return 0;
}

static void
_free_queue () {
    OSStatus err;

    err = AudioQueueStop (queue, true);
    if (err != noErr) {
        trace ("AudioQueueStop error %x\n", err);
    }
    err = AudioQueueDispose (queue, false);
    if (err != noErr) {
        trace ("AudioQueueDispose error %x\n", err);
    }
    memset (buffers, 0, sizeof (buffers));
    memset (availbuffers, 0, sizeof (availbuffers));
    queue = NULL;
}

// This is called from process_output_block (during streamer_reda), and it needs to know *immediately* which format it was able to set.
// To achieve that, we need to create a new audio queue here, and if it's unsuccessful -- try some guesswork.
// However, for now we consider that this can never fail, and the format will always be set successfully.
static int
ca_setformat (ddb_waveformat_t *fmt) {
    int res;

    if (queue) {
        _free_queue ();
    }

    res = _initqueue (fmt);

    return res;
}

static void
ca_thread (void *user_data) {
    char *data = malloc (BUFFERSIZE_BYTES);
    int datasize = 0;

    OSStatus err;

    while (!stoprequest) {
        if (state == OUTPUT_STATE_PLAYING && deadbeef->streamer_ok_to_read (-1) && datasize == 0) {
            int br = deadbeef->streamer_read (data, BUFFERSIZE_BYTES);
            if (br <= 0) {
                usleep (10000);
                continue;
            }
            datasize = br;
        }
        else if (datasize == 0) {
            usleep (10000);
            continue;
        }

        if (!queue) {
            int res = _initqueue(&plugin.fmt);
            if (res < 0) {
                usleep (10000);
                continue;
            }
        }

        // sync state
        UInt32 isRunning;
        UInt32 isRunningSize = sizeof (isRunning);
        err = AudioQueueGetProperty (queue, kAudioQueueProperty_IsRunning, &isRunning, &isRunningSize);
        if (err != noErr) {
            trace ("AudioQueueGetProperty kAudioQueueProperty_IsRunning error %x\n", err);
        }
        else {
            if (!isRunning && state == OUTPUT_STATE_PLAYING) {
                err = AudioQueueStart (queue, NULL);
                if (err != noErr) {
                    trace ("AudioQueueStart error %x\n", err);
                }
            }
            else if (isRunning && state != OUTPUT_STATE_PLAYING) {
                err = AudioQueueStop (queue, true);
                if (err != noErr) {
                    trace ("AudioQueueStart error %x\n", err);
                }
            }
        }

        if (state != OUTPUT_STATE_PLAYING) {
            usleep (10000);
            continue;
        }

        // find any available buffer
        int nextAvail = -1;
        for (int i = 0; i < MAXBUFFERS; i++) {
            if (availbuffers[i]) {
                nextAvail = i;
                break;
            }
        }
        if (nextAvail == -1) {
            usleep (10000);
            continue;
        }

        // fill and enqueue
        memcpy (availbuffers[nextAvail]->mAudioData, data, datasize);
        availbuffers[nextAvail]->mAudioDataByteSize = datasize;
        datasize = 0;

        availbuffers[nextAvail] = NULL;
        err = AudioQueueEnqueueBuffer(queue, buffers[nextAvail], 0, NULL);
        if (err != noErr) {
            trace ("AudioQueueEnqueueBuffer error %x\n", err);
        }
    }

    if (queue) {
        _free_queue ();
    }
    free (data);
}

static int
ca_play (void) {
    deadbeef->mutex_lock (mutex);
    if (state == OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PLAYING;
    }
    else if (state == OUTPUT_STATE_STOPPED) {
        state = OUTPUT_STATE_PLAYING;
        stoprequest = 0;
        tid = deadbeef->thread_start (ca_thread, NULL);
    }
    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int
ca_stop (void) {
    if (tid) {
        stoprequest = 1;
        deadbeef->thread_join (tid);
        state = OUTPUT_STATE_STOPPED;
        tid = 0;
    }
    return 0;
}

static int
ca_pause (void) {
    deadbeef->mutex_lock (mutex);
    if (state == OUTPUT_STATE_PLAYING) {
        state = OUTPUT_STATE_PAUSED;
    }
    else if (state == OUTPUT_STATE_STOPPED) {
        state = OUTPUT_STATE_PAUSED;
        stoprequest = 0;
        tid = deadbeef->thread_start (ca_thread, NULL);
    }

    deadbeef->mutex_unlock (mutex);
    return 0;
}

static int
ca_unpause (void) {
    return ca_play ();
}

static int
ca_state (void) {
    return state;
}

static int
ca_plugin_start (void) {
    mutex = deadbeef->mutex_create ();
    return 0;
}

static int
ca_plugin_stop (void) {
    deadbeef->mutex_free (mutex);
    return 0;
}

static DB_output_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 2,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id = "coreaudio",
    .plugin.name = "CoreAudio",
    .plugin.descr = "CoreAudio output plugin",
    .plugin.copyright =
        "DeaDBeeF CoreAudio output plugin\n"
        "Copyright (C) 2009-2017 Alexey Yakovenko and other contributors\n"
        "Copyright (C) 2016 Christopher Snowhill\n"
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
    .init = ca_init,
    .free = ca_free,
    .setformat = ca_setformat,
    .play = ca_play,
    .stop = ca_stop,
    .pause = ca_pause,
    .unpause = ca_unpause,
    .state = ca_state,
    .plugin.start = ca_plugin_start,
    .plugin.stop = ca_plugin_stop,
};

DB_plugin_t *
coreaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
