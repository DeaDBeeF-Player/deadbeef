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
#import "DdbPlaylistWidget.h"
#import "DdbPlaylistViewController.h"
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbPlaylistWidget

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        NSRect listFrame = frame;
        listFrame.origin.x = 0;
        listFrame.origin.y = 0;
        _listview = [[DdbListview alloc] initWithFrame:listFrame];
        [_listview setNeedsDisplay:YES];
        [_listview setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMinYMargin|NSViewHeightSizable|NSViewMaxYMargin];
        [self addSubview:_listview];
        
    }
    return self;
}

- (void)setDelegate:(id<DdbListviewDelegate>)delegate {
    _delegate = delegate;
    [_listview setDelegate:(id<DdbListviewDelegate>)delegate];
}


- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    return [(DdbPlaylistViewController *)_delegate handleListviewMessage:_listview id:_id ctx:ctx p1:p1 p2:p2];
}

@end
