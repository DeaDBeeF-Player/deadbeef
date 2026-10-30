/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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

#ifndef mp4tagutil_h
#define mp4tagutil_h

#include <deadbeef/deadbeef.h>
#include <mp4p/mp4p.h>

int
mp4_read_metadata (DB_playItem_t *it);

int
mp4_read_metadata_file (DB_playItem_t *it, mp4p_file_callbacks_t *cb);

int
mp4_write_metadata (DB_playItem_t *it);

mp4p_atom_t *
mp4tagutil_modify_meta (mp4p_atom_t *mp4file, DB_playItem_t *it);

void
mp4_init_ddb_file_callbacks (mp4p_file_callbacks_t *cb);

void
mp4_load_tags (mp4p_atom_t *mp4file, DB_playItem_t *it);

mp4p_atom_t *
mp4_get_cover_atom (mp4p_atom_t *mp4file);

typedef struct {
    char *title; // may be NULL
    int64_t startsample;
    int64_t endsample;
} mp4_chapter_t;

/// Load QuickTime/iTunes text-track chapters referenced by the audio track's `tref/chap` atom.
/// @param trak The audio `trak` atom.
/// @param fp File to read the chapter samples from.
/// @param junk Size of the leading junk (e.g. ID3v2) preceding the MP4 data.
/// @return Array of @c *num_chapters items, or NULL if there are none. Free with @c mp4_chapters_free.
mp4_chapter_t *
mp4_load_chapters (mp4p_atom_t *mp4file, mp4p_atom_t *trak, DB_FILE *fp, int junk, int samplerate, int *num_chapters);

void
mp4_chapters_free (mp4_chapter_t *chapters, int num_chapters);

/// Insert chapters as subtracks of @c origin, copying the tags from @c origin.
/// @return The last inserted item (with a reference held by the caller), or NULL if nothing was inserted.
DB_playItem_t *
mp4_insert_with_chapters (
    ddb_playlist_t *plt,
    DB_playItem_t *after,
    DB_playItem_t *origin,
    const mp4_chapter_t *chapters,
    int num_chapters,
    const char *filetype,
    int samplerate);

#endif /* mp4tagutil_h */
