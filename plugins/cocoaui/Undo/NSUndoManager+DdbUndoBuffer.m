/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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

#import "DdbUndoBuffer.h"
#import "NSUndoManager+DdbUndoBuffer.h"
#include "undomanager.h"

@implementation NSUndoManager (DdbUndoBuffer)

- (void)registerUndoBuffer:(DdbUndoBuffer *)undoBuffer {
    [self registerUndoWithTarget:undoBuffer handler:^(id  _Nonnull target) {
        // Undo-able operations are supposed to be only allowed on the main thread,
        // and we expect that at this point the current undobuffer is empty
        [undoBuffer apply];
        undobuffer_t *undobuffer = undomanager_consume_buffer(undomanager_shared());
        DdbUndoBuffer *redoBuffer = [[DdbUndoBuffer alloc] initWithUndoBuffer:undobuffer];
        [self registerUndoBuffer:redoBuffer];
    }];
}

@end
