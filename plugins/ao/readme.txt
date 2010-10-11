this version of AOSDK is not the original work.
main change is that all engines had been made reentrant to work correctly in deadbeef player

=== original AOSDK readme starts here === 

it has STL code stripped off, + many bug fixes that are not upstream (yet)
for original AdPlug information see below

Audio Overload SDK - Development Release 1.4.8  February 15, 2009

Copyright (c) 2007-2009 R. Belmont and Richard Bannister.
All rights reserved.
=========================================================

Please refer to license.txt for the specific licensing details of this software.

This SDK opens up some of the music file format engines developed for the Audio Overload project.
You may use this code to play the formats on systems we don't support or inside of applications 
other than AO.

Configurables in the makefile:

- Uncomment the line that defines LONG_IS_64BIT for compilation on 64-bit Linux, *BSD, and other operating
systems using the AMD64 recommended ABI (not 64-bit Windows).

- Change LSB_FIRST=1 to =0 for big-endian platforms.

New in Release 1.4.8
- Guard against invalid data sometimes created by makessf.py (fixes crashing Pebble Beach ST-V rips)


Entry points of an AO engine are as follows:

int32 XXX_start(uint8 *, uint32)

This function attempts to recognize and load a file of a specific type.  It is assumed external code has 
already checked the file's signature in cases where that's possible.  The first parameter is a pointer to
the entire file in memory, and the second is the length of the file.  The return value is AO_SUCCESS if 
the engine properly loaded the file and AO_FAIL if it didn't.


int32 XXX_gen(int16 *, uint32)

This function actually plays the song and generates signed 16-bit stereo samples at 44100 Hz.  The first 
parameter is a pointer to a buffer in which to place the samples (stereo interleaved), and the second is
the number of stereo samples to generate (so the output buffer size must be (number of samples) * 2 * 2
bytes in length).


int32 XXX_stop(void)

This function ceases playback and cleans up the engine.  You must call _start again after this to play more
music.


int32 XXX_command(int32, int32)

For some engines, this allows you to send commands while a song is playing.  The first parameter is the 
command (these are defined in ao.h), the second is the parameter.  These commands are as follows:

COMMAND_PREV (parameter ignored) - for file formats which have more than one song in a file (NSF), this 
moves back one song.

COMMAND_NEXT (parameter ignored) - for file formats which have more than one song in a file (NSF), this 
moves forward one song.

COMMAND_RESTART	(parameter ignored) - Restarts the current song from the beginning.  Not supported by 
all engines.

COMMAND_HAS_PREV (parameter ignored) - for file formats which have more than one song in a file (NSF),
this checks if moving backwards from the current song is a valid operation.  (Returns AO_FAIL if not)

COMMAND_HAS_NEXT (parameter ignored) - for file formats which have more than one song in a file (NSF),
this checks if moving forward from the current song is a valid operation.  (Returns AO_FAIL if not)

COMMAND_GET_MIN (parameter ignored) - for file formats which have more than one song in a file (NSF), 
this returns the lowest valid song number.

COMMAND_GET_MAX (parameter ignored) - for file formats which have more than one song in a file (NSF), 
this returns the highest valid song number.

COMAND_JUMP - for file formats which have more than one song in a file (NSF), this command jumps directly
to a specific song number, which is passed in as the parameter.


int32 XXX_fillinfo(ao_display_info *)

This function fills out the ao_display_info struct (see ao.h for details) with information about the currently
playing song.  The information provided varies by engine.
