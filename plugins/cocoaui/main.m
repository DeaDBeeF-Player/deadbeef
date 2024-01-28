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
#include <deadbeef/deadbeef.h>
#import "DdbUndoBuffer.h"
#import "NSUndoManager+DdbUndoBuffer.h"
#include "undobuffer.h"

extern DB_functions_t *deadbeef;

int cocoaui_start(void) {
    char *argv[1];
    argv[0] = "FIXME";
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

int cocoaui_command (int command, ...) {
    if (command == 111) {
        // register undo
        va_list args;
        va_start(args, command);
        undobuffer_t *undobuffer = va_arg(args, undobuffer_t *);
        const char *name = va_arg(args, const char *);
        va_end(args);

        AppDelegate *appDelegate = (AppDelegate *)NSApp.delegate;
        NSUndoManager *undoManager = appDelegate.mainWindow.window.undoManager;
        DdbUndoBuffer *buffer = [[DdbUndoBuffer alloc] initWithUndoBuffer:undobuffer];

        [undoManager setActionName:[NSString stringWithUTF8String:name]];
        [undoManager registerUndoBuffer:buffer];
    }

    return 0;
}

DB_gui_t plugin = {
    .plugin.type = DB_PLUGIN_GUI,
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.id = "cocoaui",
    .plugin.name = "Cocoa UI",
    .plugin.start = cocoaui_start,
    .plugin.stop = cocoaui_stop,
    .plugin.message = cocoaui_message,
    .plugin.command = cocoaui_command,
    // NSApplicationMain doesn't return, so it doesn't seem it's possible to cleanup
};

DB_plugin_t *cocoaui_load (DB_functions_t *_deadbeef) {
    deadbeef = _deadbeef;
    return (DB_plugin_t *)&plugin;
}
