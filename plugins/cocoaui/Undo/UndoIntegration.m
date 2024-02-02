/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

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

#import <AppKit/AppKit.h>
#import "AppDelegate.h"
#import "DdbUndoBuffer.h"
#import "NSUndoManager+DdbUndoBuffer.h"
#include "UndoIntegration.h"

extern DB_functions_t *deadbeef;

ddb_undo_interface_t *ddb_undo;

static void
_undo_initialize (ddb_undo_interface_t *interface) {
    ddb_undo = ddb_undo;
}

static int
_undo_process_action (struct ddb_undobuffer_s *undobuffer, const char *action_name) {
    AppDelegate *appDelegate = (AppDelegate *)NSApp.delegate;
    NSUndoManager *undoManager = appDelegate.mainWindow.window.undoManager;
    DdbUndoBuffer *buffer = [[DdbUndoBuffer alloc] initWithUndoBuffer:undobuffer];

    NSString *actionName = @(action_name ?: "");
    actionName = [actionName stringByReplacingOccurrencesOfString:@"&" withString:@"&&"];
    [undoManager setActionName:actionName];
    [undoManager registerUndoBuffer:buffer];

    return 0;
}

static ddb_undo_hooks_t _undo_hooks = {
    ._size = sizeof (ddb_undo_hooks_t),
    .initialize = _undo_initialize,
    .process_action = _undo_process_action,
};


void UndoIntegrationInit(void) {
    deadbeef->register_for_undo (&_undo_hooks);
}
