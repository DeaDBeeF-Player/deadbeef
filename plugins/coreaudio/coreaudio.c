/*
	CoreAudio Output Deadbeef Plugin
	Copyright (c) 2011-2013 Carlos Nunes <carloslnunes@gmail.com>
 
	This plugin is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
 
	This plugin is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.
 
	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 
 */

#include "../../deadbeef.h"

#include <AudioUnit/AudioUnit.h>        // AudioUnit
#include <CoreAudio/CoreAudio.h>		// AudioDeviceID
#include <AudioToolbox/AudioFormat.h>   // AudioFormatGetProperty
#include <CoreServices/CoreServices.h>


//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_output_t plugin;
DB_functions_t *deadbeef;

static int state; // playing/stopped/paused
static AudioUnit output_unit; // output unit for the audio
static int au_state;

static int
coreaudio_plugin_start (void) {
	
	// This is a largely undocumented but absolutely necessary
	// requirement starting with OS-X 10.6.  If not called, queries and
	// updates to various audio device properties are not handled
	// correctly.
	// Many thanks to the rtaudio project for documenting this.
	CFRunLoopRef theRunLoop = NULL;
	AudioObjectPropertyAddress property = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
	OSStatus err = AudioObjectSetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
	if (err) { trace ("AudioObjectSetPropertyData-plugin_start= %s\n", GetMacOSStatusErrorString(err) ); return -1; }

	au_state = 0;
	
	return 0;
}

static int
coreaudio_plugin_stop (void) {
	return 0;
}

int
coreaudio_set_data_format(ddb_waveformat_t *fmt) {

	AudioStreamBasicDescription streamFormat;
	
	 // lets assume that if it has more than one channel it is interleaved
	bool inIsNonInterleaved = false;
	if (fmt->channels == 1)
		inIsNonInterleaved = true;
	
	UInt32 flags = (fmt->is_float ? kAudioFormatFlagIsFloat : kAudioFormatFlagIsSignedInteger) |
					(fmt->is_bigendian ? ((UInt32)kAudioFormatFlagIsBigEndian) : 0)             |
					((!(fmt->is_float) ) ?
        			kAudioFormatFlagIsPacked : kAudioFormatFlagIsAlignedHigh)           |
					(inIsNonInterleaved ? ((UInt32)kAudioFormatFlagIsNonInterleaved) : 0);
	
	streamFormat.mSampleRate = fmt->samplerate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = flags;
    streamFormat.mBytesPerPacket = (inIsNonInterleaved ? 1 : fmt->channels) * (fmt->bps/8);
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerFrame = (inIsNonInterleaved ? 1 : fmt->channels) * (fmt->bps/8);
    streamFormat.mChannelsPerFrame = fmt->channels;
    streamFormat.mBitsPerChannel = fmt->bps;
		
	OSStatus err = noErr;
	err = AudioUnitSetProperty (output_unit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Input,
								0,
								&streamFormat,
								sizeof(streamFormat));
	if (err) { trace ("AudioUnitSetProperty-SF= %s\n", GetMacOSStatusErrorString(err) ); return -1; }
	
    return 0;
}

/* data callback for the audio unit */
OSStatus
coreaudio_callback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
	
	if(ioData == NULL && ioData->mNumberBuffers < 1)
	{ trace ("no buffers\n"); return -1; /* not an OSStatus but it is not that important... */ }

	int sample_size = plugin.fmt.channels * (plugin.fmt.bps / 8);
	int block_size = ioData->mBuffers[0].mDataByteSize;
	int mod = block_size % sample_size;
	if ( mod > 0)
		block_size -= mod; // should not happen but just in case...
		
	char * buffer = NULL;
	buffer = (char*) malloc( block_size );
	memset(buffer, 0, block_size);
	
	deadbeef->streamer_read( buffer, block_size); // fetching the data from stream
		
	for (int i = 0; i < ioData->mNumberBuffers; ++i) {
		
		AudioBuffer audio_buffer = ioData->mBuffers[i];
		void * frame_buffer = audio_buffer.mData;
		void * stream_buffer = buffer;
		
		memcpy(frame_buffer, stream_buffer, audio_buffer.mDataByteSize);
	}

	free(buffer);
	
	return noErr;
}

static int
coreaudio_init (void) {
    trace ("coreaudio_init\n");
	
	//selecting the default output unit
	ComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	OSStatus err = noErr;
	Component comp = FindNextComponent(NULL, &desc);
	if (comp == NULL) { trace ("FindNextComponent= failed to find the default output component.\n"); return -1; }
	
	err = OpenAComponent(comp, &output_unit);
	if (comp == NULL) { trace ("OpenAComponent= %s\n", GetMacOSStatusErrorString(err)); return -1; }
	
	// filling out the description for linear PCM data (can only be called after opening audio component)
    if (coreaudio_set_data_format(&plugin.fmt) < 0)
		return -1;
	
	// callback
	AURenderCallbackStruct input_cb;
	input_cb.inputProc = coreaudio_callback;
	input_cb.inputProcRefCon = NULL;
	
	err = AudioUnitSetProperty(output_unit,
							   kAudioUnitProperty_SetRenderCallback,
							   kAudioUnitScope_Input,
							   0,
							   &input_cb,
							   sizeof(input_cb));
	if (err)
		{ trace ("AudioUnitSetProperty-CB= %s\n", GetMacOSStatusErrorString(err)); return -1; }
	
	// Initialize unit
	err = AudioUnitInitialize(output_unit);
	if (err) { trace ("AudioUnitInitialize= %s\n", GetMacOSStatusErrorString(err)); return -1; }
	
	
	au_state = 1; // audio unit initialised
    state = OUTPUT_STATE_STOPPED;
	
	return 0;
}

static int
coreaudio_free(void) {
	
	trace("coreaudio_free\n");
	
	state = OUTPUT_STATE_STOPPED;
	
	if(!au_state) // audio unit already unintialized
		return 0;
	
	OSStatus err = AudioUnitUninitialize (output_unit);
	if (err) { trace ("AudioUnitUninitialize= %s\n", GetMacOSStatusErrorString(err)); return -1; }
	au_state = 0;
	
	CloseComponent(output_unit);

	return 0;
}

static int
coreaudio_play (void) {
	
	if (coreaudio_init() < 0)
		return -1;
	
	OSStatus err = AudioOutputUnitStart (output_unit);
	if (err) { trace ("AudioOutputUnitStart= %s\n", GetMacOSStatusErrorString(err)); return -1; }
	
	state = OUTPUT_STATE_PLAYING;	
	return 0;
}

static int
coreaudio_stop(void) {
	
	trace("coreaudio_stop\n");

	state = OUTPUT_STATE_STOPPED;
    deadbeef->streamer_reset(1);
	
	if(!au_state) // no audio unit to stop
		return 0;
	
	OSStatus err = AudioOutputUnitStop (output_unit);
	if (err) { trace ("AudioOutputUnitStop= %s\n", GetMacOSStatusErrorString(err)); return -1; }
	
    return coreaudio_free();
}

static int
coreaudio_pause(void) {

    state = OUTPUT_STATE_PAUSED;
	AudioOutputUnitStop(output_unit);

    return 0;
}

static int
coreaudio_unpause(void) {
	
	trace("coreaudio_unpause\n");
	
	OSStatus err = AudioOutputUnitStart (output_unit);
	if (err) { trace ("AudioOutputUnitStart= %s\n", GetMacOSStatusErrorString(err)); return -1; }

	state = OUTPUT_STATE_PLAYING;

	return 0;
}

static int
coreaudio_setformat (ddb_waveformat_t *fmt) {
    trace ("coreaudio_setformat\n");
	
    memcpy (&plugin.fmt, fmt, sizeof (ddb_waveformat_t));
	
	switch (state) {
		case OUTPUT_STATE_STOPPED:
			return coreaudio_stop ();
		case OUTPUT_STATE_PLAYING:
			return coreaudio_play ();
		case OUTPUT_STATE_PAUSED:
			if (0 != coreaudio_play ()) {
				return -1;
			}
			if (0 != coreaudio_pause ()) {
				return -1;
			}
			break;
    }
	
    return 0;
	
}

static int
coreaudio_get_state(void) {
	return state;
}
 
DB_plugin_t *
coreaudio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

// define plugin interface
static DB_output_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_OUTPUT,
    .plugin.id = "coreaudio",
    .plugin.name = "core audio output plugin",
    .plugin.descr = "Uses the core audio framework to output sound.",
    .plugin.copyright = 
	"CoreAudio Output Deadbeef Plugin\n" 
	"Copyright (c) 2011-2013 Carlos Nunes <carloslnunes@gmail.com>\n"
	"\n"
	"CoreAudio Output Deadbeef Plugin is licensed under the GNU \n"
	"Lesser General Public License version 2.\n"
    ,
    .plugin.website = "http://spontaneouscoders.com/projects/coreaudio-output-deadbeef-plugin",
    .plugin.start = coreaudio_plugin_start,
    .plugin.stop = coreaudio_plugin_stop,
    .init = coreaudio_init,
    .free = coreaudio_free,
    .setformat = coreaudio_setformat,
    .play = coreaudio_play,
    .stop = coreaudio_stop,
    .pause = coreaudio_pause,
    .unpause = coreaudio_unpause,
    .state = coreaudio_get_state,
    .fmt = {-1},
};

