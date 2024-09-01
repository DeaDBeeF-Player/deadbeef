/*
    DeaDBeeF -- the music player
    Copyright (C) 2017 Oleksiy Yakovenko and other contributors

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

// Load cuesheet, find the corresponding audiofiles, and add them as tracks into playlist, if they can be found.
//
// If namelist is not NULL (result of scandir), it helps to find the audio files in the cuesheet directory,
// and allows to mark them as used to avoid adding the same files multiple times.
//
// Internally, the function finds and loads the cue file, and passes it to `plt_load_cuesheet_from_buffer`, see below for more information.
//
// Argument breakdown:
//  `playlist` where to add files.
//  The item `after` which to insert the files. NULL means beginning of playlist.
//  `fullname` is the fully qualified path to the cuesheet file.
//  `dirname` is full directory path, in which scandir was performed.
//  `namelist` and `n` is the scandir output.
//  The `dirname`, `namelist` and `n` can be either all set to NULL, otherwise they all must be valid values.
playItem_t *
plt_load_cue_file (
    playlist_t *playlist,
    playItem_t *after,
    const char *fullname,
    const char *dirname,
    struct dirent **namelist,
    int n);

// This is a more internal function, to load cuesheet from buffer.
// Semantics are the same as `plt_load_cue_file`.
//
// Additional arguments:
//  `fname`: the file from which the cuesheet was loaded, either the cue file itself, or its audio file.
//  `embedded_origin`: the track from which the embedded cuesheet was extracted.
//  `embedded_numsamples`: number of samples in the whole track.
//  `embedded_samplerate`: samplerate of the track.
//  `buffer`: pointer to the string containing cuesheet.
//  `buffersize`: size of the buffer.
playItem_t *
plt_load_cuesheet_from_buffer (
    playlist_t *playlist,
    playItem_t *after,
    const char *fname,
    playItem_t *embedded_origin,
    int64_t embedded_numsamples,
    int embedded_samplerate,
    const uint8_t *buffer,
    int buffersize,
    const char *dirname,
    struct dirent **namelist,
    int n);
