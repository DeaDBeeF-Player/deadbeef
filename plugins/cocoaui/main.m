/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and other contributors

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

#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"
#import "DdbCocoaUI.h"
#include <deadbeef/deadbeef.h>
#import "DdbUndoBuffer.h"
#import "NSUndoManager+DdbUndoBuffer.h"
#import "Undo/UndoIntegration.h"

extern DB_functions_t *deadbeef;

int cocoaui_start(void) {
    char *argv[1];
    argv[0] = "FIXME";
    UndoIntegrationInit();
    return NSApplicationMain(1, (const char **)argv);
}

int cocoaui_stop(void) {
    dispatch_sync(dispatch_get_main_queue(), ^{
        [NSApp replyToApplicationShouldTerminate:YES];
    });
    return 0;
}

int cocoaui_message (uint32_t _id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    AppDelegate *appDelegate = NSApp.delegate;
    return [appDelegate ddb_message:_id ctx:ctx p1:p1 p2:p2];
}

static id<DdbCocoaUIProtocol>
cocoaui_get_api (void) {
    return DdbCocoaUI.sharedInstance;
}

ddb_cocoaui_api_t plugin = {
    .gui.plugin.type = DB_PLUGIN_GUI,
    .gui.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .gui.plugin.api_vminor = DB_API_VERSION_MINOR,
    .gui.plugin.version_major = 1,
    .gui.plugin.version_minor = 0,
    .gui.plugin.id = "cocoaui",
    .gui.plugin.name = "Cocoa UI",
    .gui.plugin.start = cocoaui_start,
    .gui.plugin.stop = cocoaui_stop,
    .gui.plugin.message = cocoaui_message,
    // NSApplicationMain doesn't return, so it doesn't seem it's possible to cleanup
    .get_api = cocoaui_get_api,
};

DB_plugin_t *cocoaui_load (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
    return (DB_plugin_t *)&plugin;
}
