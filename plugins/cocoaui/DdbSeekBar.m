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

    NSSize frontCapLeftSize = [_frontCapLeft size];
    NSSize frontCapRightSize = [_frontCapRight size];

    NSPoint convPt = [controlView convertPoint:NSMakePoint(0,y) fromView:nil];
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [gc setPatternPhase:convPt];

    // foreground

    if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_9) {
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
    }
    else {
        rc.size.width = (int)(rc.size.width * _value / ([self maxValue] - [self minValue]));
        rc.size.height -= 2;
        rc.origin.y += 1;
        [[NSColor keyboardFocusIndicatorColor] set];
        [NSBezierPath fillRect:rc];
    }

    [gc restoreGraphicsState];
}

@end
