//
//  DdbPlaceholderWidget.m
//  deadbeef
//
//  Created by waker on 29/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

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

        [[NSColor lightGrayColor] set];
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
