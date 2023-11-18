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
#include <dispatch/dispatch.h>
#import <Foundation/Foundation.h>
#include "medialibcommon.h"
#include "medialibfilesystem.h"
#include "medialibsource.h"

struct ml_watch_s {
    void *watch; // strong DdbMLWatch *
};

@interface DdbMLWatch: NSObject {
    FSEventStreamRef _eventStream;
    void (*_callback)(void *);
    void *_userData;
    NSTimer *_debounceTimer;
}

@end

@implementation DdbMLWatch

static void
_FSEventStreamCallback(ConstFSEventStreamRef streamRef, void * __nullable clientCallBackInfo, size_t numEvents, void *eventPaths,  const FSEventStreamEventFlags  * _Nonnull eventFlags, const FSEventStreamEventId * _Nonnull eventIds) {
    DdbMLWatch *self = (__bridge DdbMLWatch *)clientCallBackInfo;
    [self streamCallback];
}

- (instancetype)initWithMusicPath:(struct json_t * _Nonnull)musicpathsJson callback:(void (* _Nonnull)(void *))callback userData:(void *)userData {
    self = [super init];

    _callback = callback;
    _userData = userData;

    FSEventStreamContext context = {0};
    context.info = (__bridge void *)self;

    size_t count = json_array_size(musicpathsJson);
    CFMutableArrayRef arrayRef = CFArrayCreateMutable(NULL, count, NULL);
    for (int i = 0; i < count; i++) {
        json_t *data = json_array_get (musicpathsJson, i);
        if (json_is_string (data)) {
            const char *bytes = json_string_value (data);
            CFStringRef stringRef = CFStringCreateWithBytes(NULL, (const UInt8 *)bytes, strlen(bytes), kCFStringEncodingUTF8, FALSE);
            CFArrayAppendValue(arrayRef, stringRef);
        }
    }

    FSEventStreamRef eventStream = FSEventStreamCreate(NULL, _FSEventStreamCallback, &context, arrayRef, kFSEventStreamEventIdSinceNow, 1.0, kFSEventStreamCreateFlagWatchRoot);
    CFRelease(arrayRef);

    FSEventStreamSetDispatchQueue(eventStream, dispatch_get_main_queue());
    FSEventStreamStart(eventStream);
    _eventStream = eventStream;

    return self;
}

- (void)stop {
    [_debounceTimer invalidate];
    FSEventStreamInvalidate(_eventStream);
    FSEventStreamStop(_eventStream);
    FSEventStreamRelease(_eventStream);
}

- (void)streamCallback {
   [_debounceTimer invalidate];
    __weak DdbMLWatch *weakSelf = self;
    _debounceTimer = [NSTimer timerWithTimeInterval:5 repeats:NO block:^(NSTimer * _Nonnull timer) {
        DdbMLWatch *self = weakSelf;
        if (self != nil) {
            self->_callback(self->_userData);
            self->_debounceTimer = nil;
        }
    }];
    [NSRunLoop.mainRunLoop addTimer:_debounceTimer forMode:NSRunLoopCommonModes];
}

@end

ml_watch_t *
ml_watch_fs_start (struct json_t *musicpathsJson, void (*eventCallback)(void *), void *userdata) {
    ml_watch_t *wrapper = calloc(sizeof (ml_watch_t), 1);
    wrapper->watch = (__bridge_retained void *)[[DdbMLWatch alloc] initWithMusicPath:musicpathsJson callback:eventCallback userData:userdata];
    return wrapper;
}

void
ml_watch_fs_stop (ml_watch_t *wrapper) {
    if (wrapper == NULL) {
        return;
    }
    DdbMLWatch *watch = (__bridge_transfer DdbMLWatch *)wrapper->watch;
    free (wrapper);
    wrapper = NULL;

    [watch stop];
}

