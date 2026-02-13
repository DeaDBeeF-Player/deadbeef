/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include "../medialibsource.h"
#include "../medialibfilesystem.h"
#include "fsmonitor.h"

struct ml_watch_s {
    ddb_fsmonitor_t *monitor;

    dispatch_queue_t queue;
    dispatch_source_t debounce;

    void (*cb)(void *);
    void *userdata;
};

static void
debounce_reset(ml_watch_t *w) {
    dispatch_source_set_timer(
                              w->debounce,
                              dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC),
                              DISPATCH_TIME_FOREVER,
                              0);
}

static void
monitor_event(void *ud) {
    ml_watch_t *w = ud;
    debounce_reset(w);
}

ml_watch_t *
ml_watch_fs_start(json_t *json,
                  void (*eventCallback)(void *),
                  void *userdata)
{
    ml_watch_t *w = calloc(1, sizeof(*w));

    w->cb = eventCallback;
    w->userdata = userdata;
    w->queue = dispatch_queue_create("ml.watch", 0);

    size_t count = json_array_size(json);
    const char **paths = malloc(sizeof(char*) * count);

    for (size_t i = 0; i < count; i++)
        paths[i] = json_string_value(json_array_get(json, i));

    w->monitor =
    ddb_fsmonitor_create(paths, count,
                         monitor_event, w);

    free(paths);

    w->debounce =
    dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                           0, 0, w->queue);

    dispatch_source_set_event_handler(w->debounce, ^{
        w->cb(w->userdata);
    });

    dispatch_resume(w->debounce);

    return w;
}

void
ml_watch_fs_stop(ml_watch_t *w) {
    if (!w) return;

    ddb_fsmonitor_free(w->monitor);
    dispatch_source_cancel(w->debounce);
    free(w);
}

