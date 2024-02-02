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
#import <deadbeef/deadbeef.h>
#import "UndoIntegration.h"

extern DB_functions_t *deadbeef;

@interface DdbUndoBuffer()

@property (nonatomic) struct ddb_undobuffer_s *buffer;

@end

@implementation DdbUndoBuffer

- (void)dealloc {
    ddb_undo->free_buffer (_buffer);
}

- (instancetype)initWithUndoBuffer:(struct ddb_undobuffer_s *)buffer {
    self = [super init];
    _buffer = buffer;
    return self;
}

- (void)apply {
    ddb_undo->execute_buffer (self.buffer);

    deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}

@end
