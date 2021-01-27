/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#import "DdbWidgetManager.h"
#import "WeakRefWrapper.h"

@interface DdbWidgetManager()

@property NSMutableArray<WeakRefWrapper *> *regWidgets;

@end

@implementation DdbWidgetManager

static DdbWidgetManager *_defaultWidgetManager = nil;

+ (DdbWidgetManager *)defaultWidgetManager {
    if (!_defaultWidgetManager) {
        _defaultWidgetManager = [DdbWidgetManager new];
    }
    return _defaultWidgetManager;
}

- (DdbWidgetManager *)init {
    self = [super init];
    if (self) {
        self.regWidgets = [NSMutableArray new];
    }
    return self;
}

- (void)addWidget:(DdbWidget *)widget {
    for (WeakRefWrapper *w in self.regWidgets) {
        if (w.object == widget) {
            NSLog (@"DdbWidgetManager: addWidget called with an object that's already registered\n");
            return;
        }
    }

    [self.regWidgets addObject:[[WeakRefWrapper alloc] initWithObject:widget]];
}

- (void)removeWidget:(DdbWidget *)widget {
    for (WeakRefWrapper *w in self.regWidgets) {
        if (w.object == widget) {
            [self.regWidgets removeObject:w];
            break;
        }
    }
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.regWidgets enumerateObjectsUsingBlock:^(WeakRefWrapper *w, NSUInteger idx, BOOL *stop) {
        [w.object widgetMessage:_id ctx:ctx p1:p1 p2:p2];
    }];
    return 0;
}

@end
