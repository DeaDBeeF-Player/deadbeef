/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#ifndef medialibscanner_h
#define medialibscanner_h

#include <stdint.h>
#include <deadbeef/deadbeef.h>
#include "medialib.h"
#include "medialibsource.h"

typedef struct {
    int64_t scanner_index; // can be compared with source.scanner_current_index and source.scanner_terminate_index
    char **medialib_paths;
    size_t medialib_paths_count;
}  ml_scanner_configuration_t;

typedef struct {
    medialib_source_t *source;
    ddb_playlist_t *plt; // The playlist which gets populated with new tracks during scan
    ddb_playItem_t **tracks; // The reused tracks from the current medialib playlist
    int track_count; // Current count of tracks
    int track_reserved_count; // Reserved / available space for tracks
    ml_db_t db; // The new db, with reused items transferred from source
} scanner_state_t;

void
ml_index (scanner_state_t *scanner, const ml_scanner_configuration_t *conf, int can_terminate);

void
scanner_thread (medialib_source_t *source, ml_scanner_configuration_t conf);

void
ml_scanner_init (DB_mediasource_t *_plugin, DB_functions_t *_deadbeef);

void
ml_scanner_free (void);

#endif /* medialibscanner_h */
