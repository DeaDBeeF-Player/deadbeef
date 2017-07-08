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
static AudioDeviceIOProcID process_id;
static AudioStreamBasicDescription default_format;
static AudioStreamBasicDescription current_format;
static AudioStreamBasicDescription req_format;
static int state = OUTPUT_STATE_STOPPED;
static uint64_t mutex;

static OSStatus
ca_fmtchanged (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* inClientData);

static OSStatus
ca_buffer_callback(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData);

static int
ca_free (void);

static int
ca_apply_format (void) {
    int res = -1;
    deadbeef->mutex_lock (mutex);
    UInt32 sz;
    if (req_format.mSampleRate > 0) {
        AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreamFormat,
                                                  kAudioDevicePropertyScopeOutput,
                                                  0 };
        sz = sizeof (current_format);
        if (AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, &current_format)) {
            goto error;
        }
        if (current_format.mSampleRate == req_format.mSampleRate &&
            current_format.mChannelsPerFrame == req_format.mChannelsPerFrame) {
            deadbeef->mutex_unlock (mutex);
            return 0;
        }
        sz = sizeof (req_format);
        if (AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &req_format)) {
            if (AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &default_format)) {
                goto error;
            }
        }
    }

    res = 0;
error:
    deadbeef->mutex_unlock (mutex);
    return res;
}

static int
ca_init (void) {
    ca_free ();
    mutex = deadbeef->mutex_create ();
    UInt32 sz;
    char device_name[128];
    AudioObjectPropertyAddress theAddress = { kAudioHardwarePropertyDefaultOutputDevice,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };

    sz = sizeof(device_id);
    if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &sz, &device_id)) {
        return -1;
    }

    sz = sizeof (device_name);
    theAddress.mSelector = kAudioDevicePropertyDeviceName;
    theAddress.mScope = kAudioDevicePropertyScopeOutput;
    theAddress.mElement = 1;
    if (AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, device_name)) {
           return -1;
    }
    
    sz = sizeof (default_format);
    theAddress.mSelector = kAudioDevicePropertyStreamFormat;
    theAddress.mElement = 0;
    if (AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, &default_format)) {
        return -1;
    }

    UInt32 bufsize = 4096;
    sz = sizeof (bufsize);
    theAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
    if (AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &bufsize)) {
        fprintf (stderr, "Failed to set buffer size\n");
    }

    if (ca_apply_format ()) {
        return -1;
    }

    if (AudioDeviceCreateIOProcID(device_id, ca_buffer_callback, NULL, &process_id)) {
        return -1;
    }
    
    theAddress.mSelector = kAudioDevicePropertyStreamFormat;
    if (AudioObjectAddPropertyListener(device_id, &theAddress, ca_fmtchanged, NULL)) {
        return -1;
    }
    
    ca_fmtchanged(device_id, 1, &theAddress, NULL);

    state = OUTPUT_STATE_STOPPED;

    return 0;
}

static int
ca_free (void) {
    if (device_id) {
        deadbeef->mutex_lock (mutex);
        AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreamFormat,
                                                  kAudioDevicePropertyScopeOutput,
                                                  0 };
        AudioDeviceStop(device_id, ca_buffer_callback);
        AudioObjectRemovePropertyListener(device_id, &theAddress, ca_fmtchanged, NULL);
        AudioDeviceDestroyIOProcID(device_id, process_id);
        process_id = 0;
        device_id = 0;
        deadbeef->mutex_unlock (mutex);
    }
    if (mutex) {
        deadbeef->mutex_free (mutex);
        mutex = 0;
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

    deadbeef->mutex_lock (mutex);
    if (state != OUTPUT_STATE_PLAYING) {
        if (AudioDeviceStart (device_id, ca_buffer_callback)) {
            state = OUTPUT_STATE_STOPPED;
            deadbeef->mutex_unlock (mutex);
            return -1;
        }

        state = OUTPUT_STATE_PLAYING;
    }
    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int
ca_stop (void) {
    if (!device_id) {
        return 0;
    }
    deadbeef->mutex_lock (mutex);
    if (state != OUTPUT_STATE_STOPPED) {
        state = OUTPUT_STATE_STOPPED;
        if (AudioDeviceStop (device_id, ca_buffer_callback)) {
            deadbeef->mutex_unlock (mutex);
            return -1;
        }
    }

    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int
ca_pause (void) {
    if (!device_id) {
        if (ca_init()) {
            return -1;
        }
    }

    deadbeef->mutex_lock (mutex);

    if (state != OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PAUSED;
        if (AudioDeviceStop (device_id, ca_buffer_callback)) {
            state = OUTPUT_STATE_STOPPED;
            deadbeef->mutex_unlock (mutex);
            return -1;
        }
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

static OSStatus
ca_fmtchanged (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* inClientData) {
    AudioStreamBasicDescription device_format;
    UInt32 sz = sizeof (device_format);
    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreamFormat,
                                              kAudioDevicePropertyScopeOutput,
                                              0 };
    deadbeef->mutex_lock (mutex);
    if (!AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, &device_format)) {
        plugin.fmt.bps = device_format.mBitsPerChannel;
        plugin.fmt.channels = device_format.mChannelsPerFrame;
        plugin.fmt.is_float = 1;
        plugin.fmt.samplerate = device_format.mSampleRate;
        plugin.fmt.channelmask = 0;
        for (int i = 0; i < plugin.fmt.channels; i++) {
            plugin.fmt.channelmask |= (1<<i);
        }
    }
    deadbeef->mutex_unlock (mutex);

    return 0;
}
                                        
static OSStatus
ca_buffer_callback(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData) {

    UInt32 sz;
    char *buffer = outOutputData->mBuffers[0].mData;
    sz = outOutputData->mBuffers[0].mDataByteSize;

    if (state == OUTPUT_STATE_PLAYING && deadbeef->streamer_ok_to_read (-1)) {
        int br = deadbeef->streamer_read (buffer, sz);
        if (br < 0) {
            state = OUTPUT_STATE_STOPPED;

            deadbeef->mutex_lock (mutex);
            AudioDeviceStop (device_id, ca_buffer_callback);
            deadbeef->mutex_unlock (mutex);

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
};

DB_plugin_t *
coreaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
