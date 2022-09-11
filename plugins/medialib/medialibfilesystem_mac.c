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

#include <CoreServices/CoreServices.h>
#include <jansson.h>
#include "medialibcommon.h"
#include "medialibfilesystem.h"
#include "medialibsource.h"

static void
_FSEventStreamCallback(ConstFSEventStreamRef streamRef, void * __nullable clientCallBackInfo, size_t numEvents, void *eventPaths,  const FSEventStreamEventFlags  * _Nonnull eventFlags, const FSEventStreamEventId * _Nonnull eventIds) {
    medialib_source_t *source = clientCallBackInfo;
    ml_notify_listeners (source, DDB_MEDIASOURCE_EVENT_OUT_OF_SYNC);
}

void
ml_watch_fs_start (medialib_source_t *source) {
    ml_watch_fs_stop(source);

    FSEventStreamContext context = {0};
    context.info = source;

    size_t count = json_array_size(source->musicpaths_json);
    CFMutableArrayRef arrayRef = CFArrayCreateMutable(NULL, count, NULL);
    for (int i = 0; i < count; i++) {
        json_t *data = json_array_get (source->musicpaths_json, i);
        if (json_is_string (data)) {
            const char *bytes = json_string_value (data);
            CFStringRef stringRef = CFStringCreateWithBytes(NULL, (const UInt8 *)bytes, strlen(bytes), kCFStringEncodingUTF8, FALSE);
            CFArrayAppendValue(arrayRef, stringRef);
        }
    }

    FSEventStreamRef eventStream = FSEventStreamCreate(NULL, _FSEventStreamCallback, &context, arrayRef, kFSEventStreamEventIdSinceNow, 1.0, kFSEventStreamCreateFlagWatchRoot);
    CFRelease(arrayRef);

    FSEventStreamScheduleWithRunLoop(eventStream, CFRunLoopGetMain(), kCFRunLoopCommonModes);
    FSEventStreamStart(eventStream);

    source->fs_watcher = eventStream;
}

void
ml_watch_fs_stop (medialib_source_t *source) {
    if (source->fs_watcher == NULL) {
        return;
    }

    FSEventStreamRef eventStream = source->fs_watcher;
    FSEventStreamStop(eventStream);
    FSEventStreamUnscheduleFromRunLoop(eventStream, CFRunLoopGetMain(), kCFRunLoopCommonModes);
    FSEventStreamRelease(eventStream);
    source->fs_watcher = NULL;
}

