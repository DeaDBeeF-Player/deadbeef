/*
    DeaDBeeF CoreAudio output plugin
    Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#include "coreaudio.h"
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

static DB_functions_t *deadbeef;
static DB_output_t plugin;

#define trace(...) { deadbeef->log_detailed (&plugin.plugin, DDB_LOG_LAYER_INFO, __VA_ARGS__); }

static ddb_playback_state_t state = DDB_PLAYBACK_STATE_STOPPED;
static uint64_t mutex;

static AudioDeviceID device_id;
static AudioDeviceIOProcID process_id;
static AudioStreamBasicDescription default_format;
static AudioStreamBasicDescription req_format;

static int *avail_samplerates;
static int num_avail_samplerates;

static OSStatus
ca_fmtchanged (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* inClientData);

static OSStatus
ca_buffer_callback(AudioDeviceID inDevice, const AudioTimeStamp * inNow, const AudioBufferList * inInputData, const AudioTimeStamp * inInputTime, AudioBufferList * outOutputData, const AudioTimeStamp * inOutputTime, void * inClientData);

static ddb_playback_state_t
ca_state (void);

static void
ca_set_state(ddb_playback_state_t st);

static int ca_free (void);
static int ca_init (void);
static int ca_play (void);
static int ca_pause (void);

static UInt32
GetNumberAvailableNominalSampleRateRanges()
{
    UInt32 theAnswer = 0;
    AudioObjectPropertyAddress theAddress = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    UInt32 theSize = 0;

    OSStatus err = AudioObjectGetPropertyDataSize(device_id, &theAddress, 0, NULL, &theSize);
    if (err != noErr) {
        trace ("AudioObjectGetPropertyDataSize kAudioDevicePropertyAvailableNominalSampleRates %x", err);
        return 0;
    }
    theAnswer = theSize / sizeof(AudioValueRange);
    return theAnswer;
}

static void
get_avail_samplerates(void)
{
    AudioObjectPropertyAddress theAddress = {
        kAudioDevicePropertyAvailableNominalSampleRates,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 num = GetNumberAvailableNominalSampleRateRanges ();
    if (num > 0) {
        AudioValueRange *nsrs = calloc (num, sizeof(AudioValueRange));
        UInt32 dataSize = num * sizeof(AudioValueRange);
        AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &dataSize, nsrs);

        avail_samplerates = malloc (sizeof (int) * num);
        for (int i = 0; i < (int)num; i++) {
            avail_samplerates[i] = (int)nsrs[i].mMinimum;
        }
        num_avail_samplerates = num;
        free (nsrs);
    }
}

int
get_best_samplerate (int samplerate, int *all_samplerates, int count) {
    int64_t nearest = 0;
    int index = -1;

    for (int i = 0; i < count; i++) {
        // score is based on distance and modulo, with slightly more weight put on distance
        int64_t dist = llabs(all_samplerates[i] - samplerate);
        int64_t mod = samplerate > all_samplerates[i] ? (samplerate % all_samplerates[i]) : (all_samplerates[i] % samplerate);
        int64_t score = dist*2+mod;

        // upscaling is generally better than downscaling
        if (all_samplerates[i] < samplerate) {
            score *= 100;
        }

        if (index == -1 || score < nearest) {
            nearest = score;
            index = i;
        }
    }

    return all_samplerates[index];
}

static int
ca_apply_format (void) {
    int res = -1;
    OSStatus err;
    UInt32 sz;
    deadbeef->mutex_lock (mutex);
    if (req_format.mSampleRate > 0) {
        req_format.mSampleRate = get_best_samplerate ((int)req_format.mSampleRate, avail_samplerates, num_avail_samplerates);

        AudioObjectPropertyAddress theAddress = {
            kAudioDevicePropertyStreamFormat,
            kAudioDevicePropertyScopeOutput,
            kAudioObjectPropertyElementMaster
        };
        sz = sizeof (AudioStreamBasicDescription);

        // NOTE: for unsupported formats, this call may cause bogus messages to appear in console / debug output.
        err = AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &req_format);
        if (err != noErr && req_format.mChannelsPerFrame == 1) {
            req_format.mChannelsPerFrame = 2;
            req_format.mBytesPerFrame *= 2;
            req_format.mBytesPerPacket *= 2;
            err = AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &req_format);
        }
        if (err != noErr) {
            err = AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &default_format);
            // ignore the result of this operation -- it may fail even when attempting to change to the same format that's current right now
        }

        ca_fmtchanged(device_id, 1, &theAddress, NULL);
    }

    res = 0;
error:
    deadbeef->mutex_unlock (mutex);
    return res;
}

OSStatus propertiesChanged(AudioObjectID inObjectID,
                          UInt32 inNumberAddresses,
                          const AudioObjectPropertyAddress inAddresses[],
                          void *inClientData) {
    ddb_playback_state_t st = state;
    ca_free ();
    ca_init ();
    if (st == DDB_PLAYBACK_STATE_PLAYING) {
        ca_play();
    }
    else if (st == DDB_PLAYBACK_STATE_PAUSED) {
        ca_pause();
    }
    return noErr;
}

static void
ca_get_deviceid (void) {
    device_id = 0;

    char newdev[100];
    deadbeef->conf_get_str ("coreaudio_soundcard", "", newdev, sizeof (newdev));

    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);
    if(kAudioHardwareNoError != status) {
        trace ("AudioObjectGetPropertyDataSize (kAudioHardwarePropertyDevices) failed: %i\n", status);
        return;
    }

    UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);

    AudioDeviceID *audioDevices = malloc(dataSize);

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize, audioDevices);
    if(kAudioHardwareNoError != status) {
        trace ("AudioObjectGetPropertyData (kAudioHardwarePropertyDevices) failed: %i\n", status);
        free(audioDevices);
        return;
    }

    // find device with specified name
    propertyAddress.mScope = kAudioDevicePropertyScopeOutput;
    for(UInt32 i = 0; i < deviceCount; ++i) {
        // Query device name
        CFStringRef deviceName = NULL;
        dataSize = sizeof(deviceName);
        propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
        status = AudioObjectGetPropertyData(audioDevices[i], &propertyAddress, 0, NULL, &dataSize, &deviceName);
        if(kAudioHardwareNoError != status) {
            trace ("AudioObjectGetPropertyData (kAudioDevicePropertyDeviceNameCFString) failed: %i\n", status);
            continue;
        }

        char buf[100];
        CFStringGetCString(deviceName, buf, sizeof(buf), kCFStringEncodingUTF8);
        CFRelease(deviceName);
        if (!strcmp (buf, newdev)) {
            device_id = audioDevices[i];
            break;
        }
    }

    free(audioDevices);

}

static int
ca_init (void) {
    OSStatus err;
    UInt32 sz;
    char device_name[128];
    AudioObjectPropertyAddress theAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    ca_free ();

    ca_get_deviceid ();
    if (device_id == 0) {
        sz = sizeof(device_id);
        err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &sz, &device_id);
        if (err != noErr) {
            trace ("AudioObjectGetPropertyData kAudioHardwarePropertyDefaultOutputDevice: %x\n", err);
            return -1;
        }
    }

    get_avail_samplerates ();

    sz = sizeof (device_name);
    theAddress.mSelector = kAudioDevicePropertyDeviceName;
    theAddress.mScope = kAudioDevicePropertyScopeOutput;
    theAddress.mElement = kAudioObjectPropertyElementMaster;
    
    err = AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, device_name);
    if (err != noErr) {
        trace ("AudioObjectGetPropertyData kAudioDevicePropertyDeviceName: %x\n", err);
        return -1;
    }

    sz = sizeof (default_format);
    theAddress.mSelector = kAudioDevicePropertyStreamFormat;
    theAddress.mElement = kAudioObjectPropertyElementMaster;
    err = AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, &default_format);
    if (err != noErr) {
        trace ("AudioObjectGetPropertyData kAudioDevicePropertyStreamFormat: %x\n", err);
        return -1;
    }

    UInt32 bufsize = 1024;
    sz = sizeof (bufsize);
    theAddress.mSelector = kAudioDevicePropertyBufferFrameSize;
    err = AudioObjectSetPropertyData(device_id, &theAddress, 0, NULL, sz, &bufsize);
    if (err != noErr) {
        // non-critical
        trace ("AudioObjectSetPropertyData kAudioDevicePropertyBufferFrameSize: %x\n", err);
    }

    if (ca_apply_format ()) {
        return -1;
    }

    err = AudioDeviceCreateIOProcID(device_id, ca_buffer_callback, NULL, &process_id);
    if (err != noErr) {
        trace ("AudioDeviceCreateIOProcID: %x\n", err);
        return -1;
    }

    theAddress.mSelector = kAudioDevicePropertyStreamFormat;
    
    err = AudioObjectAddPropertyListener(device_id, &theAddress, ca_fmtchanged, NULL);
    if (err != noErr) {
        trace ("AudioObjectAddPropertyListener kAudioDevicePropertyStreamFormat: %x\n", err);
        return -1;
    }

    ca_fmtchanged(device_id, 1, &theAddress, NULL);

    AudioObjectPropertyAddress outputDeviceAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
    AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                   &outputDeviceAddress,
                                   &propertiesChanged, nil);

    UInt32 transportType = 0;
    sz = sizeof (transportType);
    theAddress.mScope = kAudioDevicePropertyScopeOutput;
    theAddress.mElement = kAudioObjectPropertyElementMaster;
    theAddress.mSelector = kAudioDevicePropertyTransportType;
    err = AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, &transportType);
    if (err != noErr) {
        trace ("AudioObjectGetPropertyData kAudioDevicePropertyTransportType: %x\n", err);
        return -1;
    }


    if (transportType == kAudioDeviceTransportTypeAirPlay) {
        plugin.plugin.flags |= DDB_COREAUDIO_FLAG_AIRPLAY;
    }
    else {
        plugin.plugin.flags &= ~DDB_COREAUDIO_FLAG_AIRPLAY;
    }

    state = DDB_PLAYBACK_STATE_STOPPED;

    return 0;
}

static int
ca_free (void) {
    OSStatus err;
    
    if (device_id != 0) {
        deadbeef->mutex_lock (mutex);
        AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreamFormat,
            kAudioDevicePropertyScopeOutput,
            0 };
        
        err = AudioDeviceStop(device_id, ca_buffer_callback);
        if (err != noErr) {
            trace ("AudioDeviceStop: %x\n", err);
        }
        
        err = AudioObjectRemovePropertyListener(device_id, &theAddress, ca_fmtchanged, NULL);
        if (err != noErr) {
            trace ("AudioObjectRemovePropertyListener kAudioDevicePropertyStreamFormat: %x\n", err);
        }

        err = AudioDeviceDestroyIOProcID(device_id, process_id);
        if (err != noErr) {
            trace ("AudioDeviceDestroyIOProcID: %x\n", err);
        }

        if (avail_samplerates) {
            free (avail_samplerates);
            avail_samplerates = NULL;
        }
        num_avail_samplerates = 0;

        process_id = 0;
        device_id = 0;
        deadbeef->mutex_unlock (mutex);
    }
    return 0;
}

static int
ca_setformat (ddb_waveformat_t *fmt) {
    memset (&req_format, 0, sizeof (req_format));

    int samplerate = fmt->samplerate;
    int is_float = fmt->is_float;
    int bps = fmt->bps;

    memset (&req_format, 0, sizeof (req_format));
    req_format.mSampleRate = (Float64)samplerate;

    // audioqueue happily accepts ultra-high samplerates, but doesn't really play them
    if (req_format.mSampleRate > 192000) {
        req_format.mSampleRate = 192000;
    }
    req_format.mFormatID = kAudioFormatLinearPCM;

    if (is_float) {
        req_format.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    }
    else {
        req_format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
    }

    if (fmt->is_bigendian) {
        req_format.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
    }

    req_format.mBytesPerPacket = bps / 8 * fmt->channels;
    req_format.mFramesPerPacket = 1;
    req_format.mBytesPerFrame = bps / 8 * fmt->channels;
    req_format.mChannelsPerFrame = fmt->channels;
    req_format.mBitsPerChannel = bps;

    if (device_id != 0) {
        return ca_apply_format ();
    }

    return -1;
}

static IOPMAssertionID assertion_id = 0;
static int sleep_prevented = 0;

static void
ca_prevent_sleep (void) {
    if (sleep_prevented) {
        return; // already preventing
    }
    IOPMAssertionID assertionID = 0;
    CFStringRef reasonForActivity= CFSTR("Deadbeef playback");

    IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoIdleSleep,
                                                   kIOPMAssertionLevelOn, reasonForActivity, &assertionID);
    if (success == kIOReturnSuccess) {
        sleep_prevented = 1;
        assertion_id = assertionID;
    }
}

static void
ca_allow_sleep (void) {
    if (sleep_prevented) {
        IOPMAssertionRelease(assertion_id);
        assertion_id = 0;
        sleep_prevented = 0;
    }
}

static int
ca_play (void) {
    OSStatus err;

    if (device_id == 0) {
        if (ca_init()) {
            return -1;
        }
    }

    ddb_playback_state_t current_state = ca_state();

    if (current_state != DDB_PLAYBACK_STATE_PLAYING) {
        err = AudioDeviceStart (device_id, ca_buffer_callback);
        if (err != noErr) {
            trace ("AudioDeviceStart: %x\n", err);
            ca_set_state(DDB_PLAYBACK_STATE_STOPPED);
            return -1;
        }
        ca_set_state(DDB_PLAYBACK_STATE_PLAYING);
    }

    ca_prevent_sleep ();

    return 0;
}

static int
ca_stop (void) {
    ca_allow_sleep ();

    OSStatus err;
    if (device_id == 0) {
        return 0;
    }

    ddb_playback_state_t curr_state = ca_state();

    if (curr_state != DDB_PLAYBACK_STATE_STOPPED) {
        err = AudioDeviceStop (device_id, ca_buffer_callback);
        ca_set_state(DDB_PLAYBACK_STATE_STOPPED);
        if (err != noErr) {
            trace ("AudioDeviceStop: %x\n", err);
            return -1;
        }
    }

    return 0;
}

static int
ca_pause (void) {
    OSStatus err;
    if (device_id == 0) {
        if (ca_init()) {
            return -1;
        }
    }

    ddb_playback_state_t curr_state = ca_state();

    if (curr_state != DDB_PLAYBACK_STATE_PAUSED) {
        ca_set_state(DDB_PLAYBACK_STATE_PAUSED);
        err = AudioDeviceStop (device_id, ca_buffer_callback);
        if (err != noErr) {
            trace ("AudioDeviceStop: %x\n", err);
            ca_set_state(DDB_PLAYBACK_STATE_STOPPED);
            return -1;
        }
    }
    return 0;
}

static OSStatus
ca_fmtchanged (AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* inClientData) {
    OSStatus err;
    
    AudioStreamBasicDescription device_format;
    UInt32 sz = sizeof (device_format);
    AudioObjectPropertyAddress theAddress = {
        kAudioDevicePropertyStreamFormat,
        kAudioDevicePropertyScopeOutput,
        kAudioObjectPropertyElementMaster
    };

    deadbeef->mutex_lock (mutex);
    err = AudioObjectGetPropertyData(device_id, &theAddress, 0, NULL, &sz, &device_format);
    if (err != noErr) {
        trace ("AudioObjectGetPropertyData kAudioDevicePropertyStreamFormat: %x\n", err);
    }
    else {

//        trace ("requested %d, obtained %d\n", (int)req_format.mSampleRate, (int)device_format.mSampleRate);
        plugin.fmt.bps = device_format.mBitsPerChannel;
        plugin.fmt.channels = device_format.mChannelsPerFrame;
        plugin.fmt.is_float = 1;
        plugin.fmt.samplerate = (int)device_format.mSampleRate;
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

    deadbeef->mutex_lock (mutex);
    int st = state;
    deadbeef->mutex_unlock (mutex);

    if (st == DDB_PLAYBACK_STATE_PLAYING && deadbeef->streamer_ok_to_read (-1)) {
        int br = deadbeef->streamer_read (buffer, sz);
        if (br < 0) {
            br = 0;
        }
        if (br < (int)sz) {
            memset (buffer+br, 0, sz-br);
        }
    }
    else {
        memset (buffer, 0, sz);
    }

    return 0;
}

static int
ca_unpause (void) {
    return ca_play ();
}

static ddb_playback_state_t
ca_state (void) {
    deadbeef->mutex_lock (mutex);
    int ret = state;
    deadbeef->mutex_unlock (mutex);
    return ret;
}

static void
ca_set_state(ddb_playback_state_t st) {
    deadbeef->mutex_lock (mutex);
    state = st;
    deadbeef->mutex_unlock (mutex);
}

static void ca_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata) {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 dataSize = 0;
    OSStatus status = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize);
    if(kAudioHardwareNoError != status) {
        trace ("AudioObjectGetPropertyDataSize (kAudioHardwarePropertyDevices) failed: %i\n", status);
        return;
    }

    UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);

    AudioDeviceID *audioDevices = malloc(dataSize);

    status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &dataSize, audioDevices);
    if(kAudioHardwareNoError != status) {
        trace ("AudioObjectGetPropertyData (kAudioHardwarePropertyDevices) failed: %i\n", status);
        free(audioDevices);
        return;
    }

    // Iterate through all the devices and determine which are output-capable
    propertyAddress.mScope = kAudioDevicePropertyScopeOutput;
    for(UInt32 i = 0; i < deviceCount; ++i) {
        // Query device UID
        CFStringRef deviceUID = NULL;
        dataSize = sizeof(deviceUID);
        propertyAddress.mSelector = kAudioDevicePropertyDeviceUID;
        status = AudioObjectGetPropertyData(audioDevices[i], &propertyAddress, 0, NULL, &dataSize, &deviceUID);
        if(kAudioHardwareNoError != status) {
            trace ("AudioObjectGetPropertyData (kAudioDevicePropertyDeviceUID) failed: %i\n", status);
            continue;
        }

        // Query device name
        CFStringRef deviceName = NULL;
        dataSize = sizeof(deviceName);
        propertyAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
        status = AudioObjectGetPropertyData(audioDevices[i], &propertyAddress, 0, NULL, &dataSize, &deviceName);
        if(kAudioHardwareNoError != status) {
            trace ("AudioObjectGetPropertyData (kAudioDevicePropertyDeviceNameCFString) failed: %i\n", status);
            continue;
        }

        // test for number of channels
        dataSize = 0;
        propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
        status = AudioObjectGetPropertyDataSize(audioDevices[i], &propertyAddress, 0, NULL, &dataSize);
        if(kAudioHardwareNoError != status) {
            trace ("AudioObjectGetPropertyDataSize (kAudioDevicePropertyStreamConfiguration) failed: %i\n", status);
            continue;
        }

        AudioBufferList *bufferList = malloc(dataSize);
        status = AudioObjectGetPropertyData(audioDevices[i], &propertyAddress, 0, NULL, &dataSize, bufferList);
        if(kAudioHardwareNoError != status || 0 == bufferList->mNumberBuffers) {
            free(bufferList);
            continue;
        }

        if (callback) {
            char buf[100];
            CFStringGetCString(deviceName, buf, sizeof(buf), kCFStringEncodingUTF8);
            callback (buf, "", userdata);
        }

        free(bufferList);
    }

    free(audioDevices);
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
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 2,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.flags = DDB_PLUGIN_FLAG_LOGGING,
    .plugin.id = "coreaudio",
    .plugin.name = "CoreAudio",
    .plugin.descr = "CoreAudio output plugin",
    .plugin.copyright =
        "DeaDBeeF CoreAudio output plugin\n"
        "Copyright (C) 2009-2017 Oleksiy Yakovenko and other contributors\n"
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
    .enum_soundcards = ca_enum_soundcards,
    .plugin.start = ca_plugin_start,
    .plugin.stop = ca_plugin_stop,
};

DB_plugin_t *
coreaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
