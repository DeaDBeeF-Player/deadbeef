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

#include <stdlib.h>
#include "medialibcommon.h"
#include "medialibsource.h"

void
ml_notify_listeners (medialib_source_t *source, int event) {
    dispatch_sync (source->sync_queue, ^{
        for (int i = 0; i < MAX_LISTENERS; i++) {
            if (source->ml_listeners[i]) {
                source->ml_listeners[i](event, source->ml_listeners_userdatas[i]);
            }
        }
    });
}

void
ml_free_music_paths (char **medialib_paths, size_t medialib_paths_count) {
    if (medialib_paths) {
        for (int i = 0; i < medialib_paths_count; i++) {
            free (medialib_paths[i]);
        }
    }
    free (medialib_paths);
}
