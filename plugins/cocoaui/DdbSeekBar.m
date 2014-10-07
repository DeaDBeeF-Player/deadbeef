//
//  DdbSeekBar.m
//  deadbeef
//
//  Created by waker on 27/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSeekBar.h"

@implementation DdbSeekBar

-(void)setNeedsDisplayInRect:(NSRect)invalidRect{
    [super setNeedsDisplayInRect:[self bounds]];
}

@end

@implementation DdbSeekBarCell

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self) {
#if 0
        _backCapLeft = [NSImage imageNamed:@"sb_back_cap_left.tiff"];
        _backCapRight = [NSImage imageNamed:@"sb_back_cap_right.tiff"];
        _backFiller = [NSImage imageNamed:@"sb_back_filler.tiff"];
        [_backCapLeft setFlipped:YES];
        [_backCapRight setFlipped:YES];
        [_backFiller setFlipped:YES];
#endif

        _frontCapLeft = [NSImage imageNamed:@"sb_front_cap_left.tiff"];
        _frontCapRight = [NSImage imageNamed:@"sb_front_cap_right.tiff"];
        _frontFiller = [NSImage imageNamed:@"sb_front_filler.tiff"];
        [_frontCapLeft setFlipped:YES];
        [_frontCapRight setFlipped:YES];
        [_frontFiller setFlipped:YES];
    }
    return self;
}

//- (NSRect)knobRectFlipped:(BOOL)flipped
//{
//    return NSMakeRect(0,0,1,1);
//}

- (void)drawKnob:(NSRect)knobRect {
}

//- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
- (void)drawBarInside:(NSRect)aRect flipped:(BOOL)flipped {
    [super drawBarInside:aRect flipped:flipped];

    NSView *controlView = [self controlView];

    NSRect rc = aRect;

    int h = rc.size.height;
    int y = rc.origin.y + (int)rc.size.height/2 - (int)h/2;
    rc.origin.y = y;
    rc.size.height = h;

#if 0
    NSSize backCapLeftSize = [_backCapLeft size];
    NSSize backCapRightSize = [_backCapRight size];
#endif
    NSSize frontCapLeftSize = [_frontCapLeft size];
    NSSize frontCapRightSize = [_frontCapRight size];

    NSPoint convPt = [controlView convertPoint:NSMakePoint(0,y) fromView:nil];
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [gc setPatternPhase:convPt];

#if 0
    NSRect innerRect = NSMakeRect (rc.origin.x + backCapLeftSize.width, rc.origin.y, rc.size.width - (backCapLeftSize.width + backCapRightSize.width), rc.size.height);

    // background
    if (![self isEnabled]) {
        [_backCapLeft drawAtPoint:rc.origin fromRect:NSMakeRect(0, 0, backCapLeftSize.width, backCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    }
    [_backCapRight drawAtPoint:NSMakePoint(rc.origin.x+rc.size.width-backCapRightSize.width, rc.origin.y) fromRect:NSMakeRect(0, 0, backCapLeftSize.width, backCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    [[NSColor colorWithPatternImage:_backFiller] set];
    [NSBezierPath fillRect:innerRect];
#endif

    // foreground
    if ([self isEnabled]) {
        [_frontCapLeft drawAtPoint:rc.origin fromRect:NSMakeRect(0, 0, frontCapLeftSize.width, frontCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    }

    rc.origin.x += frontCapLeftSize.width;
    rc.size.width -= frontCapLeftSize.width + frontCapRightSize.width;
    
    [[NSColor colorWithPatternImage:_frontFiller] set];
    rc.size.width = (int)(rc.size.width * _value / ([self maxValue] - [self minValue]));
    [NSBezierPath fillRect:rc];

    if ([self isEnabled]) {
        [_frontCapRight drawAtPoint:NSMakePoint(rc.origin.x+rc.size.width, rc.origin.y) fromRect:NSMakeRect(0, 0, frontCapRightSize.width, frontCapRightSize.height) operation:NSCompositeSourceOver fraction:1];
    }

    [gc restoreGraphicsState];
}

@end
