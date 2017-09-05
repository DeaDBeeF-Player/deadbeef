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
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>

static DB_functions_t *deadbeef;
static DB_output_t plugin;

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }

static int state = OUTPUT_STATE_STOPPED;
static uint64_t mutex;

static AudioStreamBasicDescription req_format;
static AudioStreamBasicDescription default_format;
static AudioComponentInstance outputUnit;

static OSStatus ca_buffer_callback(void *inRefCon,
                                   AudioUnitRenderActionFlags *ioActionFlags,
                                   const AudioTimeStamp *inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList *ioData);

static void
ca_fmtchanged (void *				inRefCon,
               AudioUnit			inUnit,
               AudioUnitPropertyID	inID,
               AudioUnitScope		inScope,
               AudioUnitElement	inElement);

AudioDeviceID getDefaultDevice(void)
{
    UInt32 dataSize = 0;
    OSStatus err = 0;

    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    dataSize = sizeof(AudioDeviceID);

    AudioDeviceID outputDevice;

    err = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                     &propertyAddress,
                                     0,
                                     NULL,
                                     &dataSize,
                                     &outputDevice);

    if (err != noErr) {
        trace ("CoreAudio: Get default output device failed with error %x\n", err);
        return 0;
    }

    return outputDevice;
}

static int
ca_apply_format (void) {
    OSStatus err = noErr;

    int res = -1;

    deadbeef->mutex_lock (mutex);

    if (req_format.mSampleRate > 0) {
        err = AudioUnitSetProperty(
                                   outputUnit,
                                   kAudioUnitProperty_StreamFormat,
                                   kAudioUnitScope_Input, // FIXME: maybe Input ?
                                   0,
                                   &req_format,
                                   sizeof (AudioStreamBasicDescription)
                                   );

        if (err != noErr) {
            trace ("CoreAudio: Could not set StreamFormat, error %x\n", err);
            goto error;
        }
    }

    res = 0;
error:
    deadbeef->mutex_unlock (mutex);
    return res;
}

static int
ca_free (void) {
    if (outputUnit) {
        deadbeef->mutex_lock (mutex);
        OSStatus err = noErr;
        err = AudioOutputUnitStop(outputUnit);
        err = AudioUnitUninitialize(outputUnit);
        err = AudioComponentInstanceDispose(outputUnit);
        deadbeef->mutex_unlock (mutex);
        outputUnit = NULL;
    }
    return 0;
}

static int
ca_init (void) {
    ca_free ();

    OSStatus err = noErr;

    // Get Component
    AudioComponent compOutput;
    AudioComponentDescription descAUHAL;

    memset (&descAUHAL, 0, sizeof (descAUHAL));

    descAUHAL.componentType = kAudioUnitType_Output;
    descAUHAL.componentSubType = kAudioUnitSubType_HALOutput;
    descAUHAL.componentManufacturer = kAudioUnitManufacturer_Apple;

    compOutput = AudioComponentFindNext(NULL, &descAUHAL);
    if (!compOutput) {
        trace ("CoreAudio: AudioComponentFindNext failed and returned NULL\n");
        return -1;
    }

    err = AudioComponentInstanceNew(compOutput, &outputUnit);
    if (err != noErr) {
        trace ("CoreAudio: AudioComponentInstanceNew failed with error %x\n", err);
        return -1;
    }

    // Get Current Output Device
    AudioDeviceID audioDevice = getDefaultDevice();

    // Set AUHAL to Current Device
    err = AudioUnitSetProperty(
                               outputUnit,
                               kAudioOutputUnitProperty_CurrentDevice,
                               kAudioUnitScope_Global,
                               0,
                               &audioDevice,
                               sizeof (audioDevice)
                               );
    if (err != noErr) {
        trace ("CoreAudio: Could not set CurrentDevice\n");
        return -1;
    }

    // get current format as default
    UInt32 sz = sizeof (default_format);
    if (AudioUnitGetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Global, 0, &default_format, &sz)) {
        return -1;
    }

    if (!req_format.mSampleRate) {
        memcpy (&req_format, &default_format, sizeof (AudioStreamBasicDescription));
    }

    // another format could have been set already, so apply it now
    if (ca_apply_format ()) {
        return -1;
    }

    // Set Render Callback
    AURenderCallbackStruct out;
    memset (&out, 0, sizeof (out));
    out.inputProc = ca_buffer_callback;

    err = AudioUnitSetProperty(
                               outputUnit,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Global,
                               0,
                               &out,
                               sizeof (out)
                               );
    if (err != noErr) {
        trace ("CoreAudio: Set RenderCallback property failed with error %x\n", err);
        return -1;
    }

    // audio format changed listener
    err = AudioUnitAddPropertyListener (outputUnit, kAudioUnitProperty_StreamFormat, ca_fmtchanged, NULL);
    if (err != noErr) {
        trace ("CoreAudio: AudioUnitAddPropertyListener failed with error %x\n", err);
        return -1;
    }

    //Initialize AUHAL
    err = AudioUnitInitialize (outputUnit);
    if (err != noErr) {
        state = OUTPUT_STATE_STOPPED;
        deadbeef->mutex_unlock (mutex);
        trace ("CoreAudio: AudioUnitInitialize failed with error %x\n", err);
        return -1;
    }

    // fetch current format
    ca_fmtchanged(NULL, outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0);

    state = OUTPUT_STATE_STOPPED;

    return 0;
}

static int
ca_setformat (ddb_waveformat_t *fmt) {
    deadbeef->mutex_lock (mutex);
    memset (&req_format, 0, sizeof (req_format));
    req_format.mSampleRate = (Float64)fmt->samplerate;
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

    req_format.mBytesPerPacket = fmt->bps / 8 * 2;
    req_format.mFramesPerPacket = 1;
    req_format.mBytesPerFrame = fmt->bps / 8 * fmt->channels;
    req_format.mChannelsPerFrame = fmt->channels;
    req_format.mBitsPerChannel = fmt->bps;

    if (outputUnit) {
        ca_apply_format ();
    }
    deadbeef->mutex_unlock (mutex);
    
    return 0;
}

static int
ca_play (void) {
    deadbeef->mutex_lock (mutex);
    if (!outputUnit) {
        if (ca_init()) {
            return -1;
        }
    }

    OSStatus err;
    err = AudioOutputUnitStart (outputUnit);
    if (err != noErr) {
        state = OUTPUT_STATE_STOPPED;
        deadbeef->mutex_unlock (mutex);
        trace ("CoreAudio: AudioOutputUnitStart failed with error %x\n", err);
        return -1;
    }

    state = OUTPUT_STATE_PLAYING;
    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int
ca_stop (void) {
    if (!outputUnit) {
        return 0;
    }
    deadbeef->mutex_lock (mutex);
    if (state != OUTPUT_STATE_STOPPED) {
        state = OUTPUT_STATE_STOPPED;
        OSStatus err = AudioOutputUnitStop(outputUnit);
        if (err != noErr) {
            deadbeef->mutex_unlock (mutex);
            trace ("CoreAudio: AudioOutputUnitStop (stop) failed with error %x", err);
            return -1;
        }
    }

    deadbeef->mutex_unlock (mutex);

    return 0;
}

static int
ca_pause (void) {
    deadbeef->mutex_lock (mutex);

    if (!outputUnit) {
        if (ca_init()) {
            deadbeef->mutex_unlock (mutex);
            return -1;
        }
    }

    if (state != OUTPUT_STATE_PAUSED) {
        state = OUTPUT_STATE_PAUSED;
        OSStatus err = AudioOutputUnitStop(outputUnit);
        if (err != noErr) {
            deadbeef->mutex_unlock (mutex);
            trace ("CoreAudio: AudioOutputUnitStop (pause) failed with error %x", err);
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

static void
ca_fmtchanged (void *				inRefCon,
               AudioUnit			inUnit,
               AudioUnitPropertyID	inID,
               AudioUnitScope		inScope,
               AudioUnitElement	inElement) {
    AudioStreamBasicDescription fmt;
    UInt32 sz = sizeof (fmt);
    OSStatus err = AudioUnitGetProperty(outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Global, 0, &fmt, &sz);
    if (err != noErr) {
        trace ("CoreAudio: failed to get StreamFormat, error: %x\n", err)
        return;
    }

    deadbeef->mutex_lock (mutex);
    plugin.fmt.bps = fmt.mBitsPerChannel;
    plugin.fmt.channels = fmt.mChannelsPerFrame;
    plugin.fmt.is_float = (fmt.mFormatFlags & kAudioFormatFlagIsFloat) ? 1 : 0;
    plugin.fmt.samplerate = fmt.mSampleRate;
    plugin.fmt.channelmask = 0;
    for (int i = 0; i < plugin.fmt.channels; i++) {
        plugin.fmt.channelmask |= (1<<i);
    }
    deadbeef->mutex_unlock (mutex);
}

static OSStatus ca_buffer_callback(
                                   void *inRefCon,
                                   AudioUnitRenderActionFlags *ioActionFlags,
                                   const AudioTimeStamp *inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList *ioData
                                   ) {
    char *buffer = ioData->mBuffers[0].mData;
    UInt32 sz = ioData->mBuffers[0].mDataByteSize;

    if (state == OUTPUT_STATE_PLAYING && deadbeef->streamer_ok_to_read (-1)) {
        int br = deadbeef->streamer_read (buffer, sz);
        if (br < 0) {
            br = 0;
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
    .plugin.start = ca_plugin_start,
    .plugin.stop = ca_plugin_stop,
};

DB_plugin_t *
coreaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
