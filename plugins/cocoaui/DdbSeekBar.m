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
        _backCapLeft = [NSImage imageNamed:@"sb_back_cap_left.tiff"];
        _backCapRight = [NSImage imageNamed:@"sb_back_cap_right.tiff"];
        _backFiller = [NSImage imageNamed:@"sb_back_filler.tiff"];
        _frontCapLeft = [NSImage imageNamed:@"sb_front_cap_left.tiff"];
        _frontCapRight = [NSImage imageNamed:@"sb_front_cap_right.tiff"];
        _frontFiller = [NSImage imageNamed:@"sb_front_filler.tiff"];

        [_backCapLeft setFlipped:YES];
        [_backCapRight setFlipped:YES];
        [_backFiller setFlipped:YES];
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

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
    _trackRect = cellFrame;
    NSRect rc = controlView.bounds;
    [controlView setNeedsDisplayInRect:rc];
    
    int h = [_backFiller size].height;
    int y = rc.origin.y + (int)rc.size.height/2 - (int)h/2;
    rc.origin.y = y;
    rc.size.height = h;
    
    NSSize backCapLeftSize = [_backCapLeft size];
    NSSize backCapRightSize = [_backCapRight size];
    NSSize frontCapLeftSize = [_frontCapLeft size];
    NSSize frontCapRightSize = [_frontCapRight size];
    
    if (![self isEnabled]) {
        [_backCapLeft drawAtPoint:rc.origin fromRect:NSMakeRect(0, 0, backCapLeftSize.width, backCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    }
    else {
        [_frontCapLeft drawAtPoint:rc.origin fromRect:NSMakeRect(0, 0, frontCapLeftSize.width, frontCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];
    }
    [_backCapRight drawAtPoint:NSMakePoint(rc.origin.x+rc.size.width-backCapRightSize.width, rc.origin.y) fromRect:NSMakeRect(0, 0, backCapLeftSize.width, backCapLeftSize.height) operation:NSCompositeSourceOver fraction:1];

    rc.origin.x += backCapLeftSize.width;
    rc.size.width -= backCapLeftSize.width + backCapRightSize.width;
    
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    NSPoint convPt = [controlView convertPoint:NSMakePoint(0,y) fromView:nil];
    [gc setPatternPhase:convPt];
    [[NSColor colorWithPatternImage:_backFiller] set];
    [NSBezierPath fillRect:rc];
    [[NSColor colorWithPatternImage:_frontFiller] set];
    rc.size.width = (int)(rc.size.width * _value / ([self maxValue] - [self minValue]));
    [NSBezierPath fillRect:rc];
    [gc restoreGraphicsState];
    
    if ([self isEnabled]) {
        [_frontCapRight drawAtPoint:NSMakePoint(rc.origin.x+rc.size.width, rc.origin.y) fromRect:NSMakeRect(0, 0, frontCapRightSize.width, frontCapRightSize.height) operation:NSCompositeSourceOver fraction:1];
    }
}

@end
