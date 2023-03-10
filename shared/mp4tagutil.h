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

#endif /* mp4tagutil_h */
