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

#import "DdbPlaceholderWidget.h"

@implementation DdbPlaceholderWidget

const NSInteger GRIDSIZE = 16;

@synthesize checker;

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        checker = [[NSImage alloc] initWithSize:NSMakeSize (12, 12)];
        [checker lockFocus];

        [NSColor.lightGrayColor set];
        [NSBezierPath strokeLineFromPoint:NSMakePoint(0, 0) toPoint:NSMakePoint(1,1)];
        
        [checker unlockFocus];
        
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor colorWithPatternImage:checker] set];
    [NSBezierPath fillRect:dirtyRect];

    [super drawRect:dirtyRect];
    
}

@end
