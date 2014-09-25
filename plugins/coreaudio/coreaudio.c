/*
    DeaDBeeF CoreAudio output plugin
    Copyright (C) 2009-2014 Alexey Yakovenko and other contributors

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
#include <CoreAudio/CoreAudio.h>

static DB_functions_t *deadbeef;
static DB_output_t plugin;

static AudioDeviceID device_id;
static AudioStreamBasicDescription default_format;
static AudioStreamBasicDescription req_format;
static int state = OUTPUT_STATE_STOPPED;

static OSStatus
ca_fmtchanged (AudioDeviceID inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, void *inClientData);

static OSStatus
ca_buffer_callback(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData);

static int
ca_apply_format (void) {
    UInt32 sz;
    if (req_format.mSampleRate > 0) {
        sz = sizeof (req_format);
        if (AudioDeviceSetProperty(device_id, NULL, 0, 0, kAudioDevicePropertyStreamFormat, sz, &req_format)) {
            if (AudioDeviceSetProperty(device_id, NULL, 0, 0, kAudioDevicePropertyStreamFormat, sz, &default_format)) {
                return -1;
            }
        }
    }

    return 0;
}

static int
ca_init (void) {
    UInt32 sz;
    char device_name[128];

    sz = sizeof(device_id);
    if (AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice, &sz, &device_id)) {
        return -1;
    }

    sz = sizeof (device_name);
    if (AudioDeviceGetProperty (device_id, 1, 0, kAudioDevicePropertyDeviceName, &sz, device_name)) {
           return -1;
    }
    
    sz = sizeof (default_format);
    if (AudioDeviceGetProperty (device_id, 0, 0, kAudioDevicePropertyStreamFormat, &sz, &default_format)) {
        return -1;
    }

    UInt32 bufsize = 4096;
    sz = sizeof (bufsize);
    if (AudioDeviceSetProperty(device_id, NULL, 0, 0, kAudioDevicePropertyBufferFrameSize, sz, &bufsize)) {
        fprintf (stderr, "Failed to set buffer size\n");
    }

    if (ca_apply_format ()) {
        return -1;
    }

    if (AudioDeviceAddIOProc (device_id, ca_buffer_callback, NULL)) {
        return -1;
    }
    
    if (AudioDeviceAddPropertyListener (device_id, 0, 0, kAudioDevicePropertyStreamFormat, ca_fmtchanged, NULL)) {
        return -1;
    }
    
    ca_fmtchanged(0, 0, 0, kAudioDevicePropertyStreamFormat, NULL);

    state = OUTPUT_STATE_STOPPED;

    return 0;
}

static int
ca_free (void) {
    if (device_id) {
        AudioDeviceStop(device_id, ca_buffer_callback);
        AudioDeviceRemovePropertyListener(device_id, 0, 0, kAudioDevicePropertyStreamFormat, ca_fmtchanged);
        AudioDeviceRemoveIOProc(device_id, ca_buffer_callback);
    }
    return 0;
}

static int
ca_setformat (ddb_waveformat_t *fmt) {
    memset (&req_format, 0, sizeof (req_format));
    req_format.mSampleRate = fmt->samplerate;
    req_format.mFormatID = kAudioFormatLinearPCM;
    req_format.mFormatFlags = (fmt->is_float ? kLinearPCMFormatFlagIsFloat : 0) | (fmt->is_bigendian ? kLinearPCMFormatFlagIsBigEndian : 0) | kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    req_format.mBytesPerPacket = 8;
    req_format.mFramesPerPacket = 1;
    req_format.mBytesPerFrame = (fmt->bps >> 3) * fmt->channels;
    req_format.mChannelsPerFrame = fmt->channels;
    req_format.mBitsPerChannel = fmt->bps;
    
    if (device_id) {
        ca_apply_format ();
    }
    
    return 0;
}

static int
ca_play (void) {
    if (!device_id) {
        if (ca_init()) {
            return -1;
        }
    }
    if (AudioDeviceStart (device_id, ca_buffer_callback)) {
        return -1;
    }
    state = OUTPUT_STATE_PLAYING;

    return 0;
}

static int
ca_stop (void) {
    if (!device_id) {
        return 0;
    }
    if (AudioDeviceStop (device_id, ca_buffer_callback)) {
        return -1;
    }
    state = OUTPUT_STATE_STOPPED;

    return 0;
}

static int
ca_pause (void) {
    if (AudioDeviceStop (device_id, ca_buffer_callback)) {
        return -1;
    }
    state = OUTPUT_STATE_PAUSED;

    return 0;
}

static int
ca_unpause (void) {
    if (AudioDeviceStart (device_id, ca_buffer_callback))
    {
        return -1;
    }
    state = OUTPUT_STATE_PLAYING;

    return 0;
}

static int
ca_state (void) {
    return state;
}

static OSStatus
ca_fmtchanged (AudioDeviceID inDevice, UInt32 inChannel, Boolean isInput, AudioDevicePropertyID inPropertyID, void *inClientData) {
    
    AudioStreamBasicDescription device_format;
    UInt32 sz = sizeof (device_format);
    if (!AudioDeviceGetProperty (device_id, 0, 0, kAudioDevicePropertyStreamFormat, &sz, &device_format)) {
        plugin.fmt.bps = device_format.mBitsPerChannel;
        plugin.fmt.channels = device_format.mChannelsPerFrame;
        plugin.fmt.is_float = 1;
        plugin.fmt.samplerate = device_format.mSampleRate;
        plugin.fmt.channelmask = 0;
        for (int i = 0; i < plugin.fmt.channels; i++) {
            plugin.fmt.channelmask |= (1<<i);
        }
    }
    
    return 0;
}
                                        
static OSStatus
ca_buffer_callback(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData) {

    UInt32 sz;
    char *buffer = outOutputData->mBuffers[0].mData;
    sz = outOutputData->mBuffers[0].mDataByteSize;

    if (state == OUTPUT_STATE_PLAYING && deadbeef->streamer_ok_to_read (-1)) {
        int br = deadbeef->streamer_read (buffer, sz);
        if (br <= 0) {
            memset (buffer, 0, sz);
            return -1;
        }
        if (br != sz) {
            memset (buffer+br, 0, sz-br);
        }
    }
    else {
        memset (buffer, 0, sz);
    }

    return 0;
}

static DB_output_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "coreaudio-ng",
    .plugin.name = "CoreAudio",
    .plugin.descr = "CoreAudio output plugin",
    .plugin.copyright =
        "DeaDBeeF CoreAudio output plugin\n"
        "Copyright (C) 2009-2014 Alexey Yakovenko and other contributors\n"
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
};

DB_plugin_t *
coreaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
