//
//  DdbSeekBar.m
//  deadbeef
//
//  Created by waker on 27/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSeekBar.h"

@implementation DdbSeekBar

@synthesize backCapLeft;
@synthesize backCapRight;
@synthesize backFiller;
@synthesize frontCapLeft;
@synthesize frontCapRight;
@synthesize frontFiller;

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self) {
        backCapLeft = [NSImage imageNamed:@"sb_back_cap_left.tiff"];
        backCapRight = [NSImage imageNamed:@"sb_back_cap_right.tiff"];
        backFiller = [NSImage imageNamed:@"sb_back_filler.tiff"];
        frontCapLeft = [NSImage imageNamed:@"sb_front_cap_left.tiff"];
        frontCapRight = [NSImage imageNamed:@"sb_front_cap_right.tiff"];
        frontFiller = [NSImage imageNamed:@"sb_front_filler.tiff"];

        [backCapLeft setFlipped:YES];
        [backCapRight setFlipped:YES];
        [backFiller setFlipped:YES];
        [frontCapLeft setFlipped:YES];
        [frontCapRight setFlipped:YES];
        [frontFiller setFlipped:YES];
    }
    return self;
}

- (NSRect)knobRectFlipped:(BOOL)flipped
{
    return NSMakeRect(0,0,1,1);
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    NSRect rc = controlView.bounds;
    [controlView setNeedsDisplayInRect:rc];
    
    int h = [backFiller size].height;
    int y = rc.origin.y + (int)rc.size.height/2 - (int)h/2;
    rc.origin.y = y;
    rc.size.height = h;
    
    NSSize backCapLeftSize = [backCapLeft size];
    NSSize backCapRightSize = [backCapRight size];
    NSSize frontCapLeftSize = [frontCapLeft size];
    NSSize frontCapRightSize = [frontCapRight size];
    
    if (![self isEnabled]) {
        [backCapLeft drawAtPoint:rc.origin fromRect:NSMakeRect(0, 0, backCapLeftSize.width, backCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    }
    else {
        [frontCapLeft drawAtPoint:rc.origin fromRect:NSMakeRect(0, 0, frontCapLeftSize.width, frontCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    }
    [backCapRight drawAtPoint:NSMakePoint(rc.origin.x+rc.size.width-backCapRightSize.width, rc.origin.y) fromRect:NSMakeRect(0, 0, backCapLeftSize.width, backCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];

    rc.origin.x += backCapLeftSize.width;
    rc.size.width -= backCapLeftSize.width + backCapRightSize.width;
    
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [gc setPatternPhase:NSMakePoint(0,y)];
    [[NSColor colorWithPatternImage:backFiller] set];
    [NSBezierPath fillRect:rc];
    [[NSColor colorWithPatternImage:frontFiller] set];
    rc.size.width = (int)(rc.size.width * [self floatValue] / ([self maxValue] - [self minValue]));
    [NSBezierPath fillRect:rc];
    [gc restoreGraphicsState];
    
    if ([self isEnabled]) {
        [frontCapRight drawAtPoint:NSMakePoint(rc.origin.x+rc.size.width, rc.origin.y) fromRect:NSMakeRect(0, 0, frontCapRightSize.width, frontCapRightSize.height) operation:NSCompositeSourceOver fraction:1];
    }
}

@end
