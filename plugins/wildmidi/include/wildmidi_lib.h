/*
	wildmidi_lib.h

 	Midi Wavetable Processing library
 
 	Copyright (C)2001-2004 Chris Ison
 
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Email: cisos@bigpond.net.au
		 wildcode@users.sourceforge.net
 
 	$Id: wildmidi_lib.h,v 1.6 2004/01/26 02:24:33 wildcode Exp $
*/

#define WM_MO_LINEAR_VOLUME	0x0001  
#define WM_MO_EXPENSIVE_INTERPOLATION 0x0002
#define WM_MO_REVERB		0x0004
#define WM_MO_BIG_ENDIAN_OUTPUT	0x0020  

#define WM_GS_VERSION		0x0001

struct _WM_Info {
	unsigned long int current_sample;
	unsigned long int approx_total_samples;
	unsigned short int mixer_options;
};

typedef void midi;

extern const char * WildMidi_GetString (unsigned short int info);
extern int WildMidi_Init (const char * config_file, unsigned short int rate, unsigned short int options);
extern int WildMidi_MasterVolume (unsigned char master_volume);
extern midi * WildMidi_Open (const char *midifile);
extern midi * WildMidi_OpenBuffer (unsigned char *midibuffer, unsigned long int size);
extern int WildMidi_LoadSamples ( midi * handle);
extern int WildMidi_GetOutput (midi * handle, char * buffer, unsigned long int size);
extern int WildMidi_SetOption (midi * handle, unsigned short int options, unsigned short int setting);
extern struct _WM_Info * WildMidi_GetInfo ( midi * handle );
extern int WildMidi_FastSeek ( midi * handle, unsigned long int *sample_pos);
extern int WildMidi_SampledSeek ( midi * handle, unsigned long int *sample_pos);
extern int WildMidi_Close (midi * handle);
extern int WildMidi_Shutdown ( void );
// extern void WildMidi_ReverbSet(midi * handle, float width, float wet, float dry, float damp, float roomsize);
