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

@implementation DdbSeekBar {
    BOOL _dragging;
}

-(void)setNeedsDisplayInRect:(NSRect)invalidRect{
    [super setNeedsDisplayInRect:[self bounds]];
}

- (BOOL)dragging {
    return _dragging;
}

- (void)mouseDown:(NSEvent *)theEvent {
    if (theEvent.type == NSLeftMouseDown) {
        _dragging = YES;
    }
    [super mouseDown:theEvent];
    // NSSlider mouseDown short-circuits the mainloop, and runs drag tracking from within the mouseDown handler,
    // so this needs to be done here instead of mouseUp handler, which is never fired.
    if (theEvent.type == NSLeftMouseDown) {
        _dragging = NO;
    }
}

@end

@implementation DdbSeekBarCell {
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    return self;
}

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

    NSPoint convPt = [controlView convertPoint:NSMakePoint(0,y) fromView:nil];
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [gc setPatternPhase:convPt];

    // foreground

    rc.size.width = (int)(rc.size.width * self.doubleValue / ([self maxValue] - [self minValue]));
    rc.size.height -= 2;
    rc.origin.y += 1;
    NSWindow *window = [controlView window];
    if ([window isKeyWindow]) {
        [[NSColor keyboardFocusIndicatorColor] set];
    }
    else {
        [[NSColor controlShadowColor] set];
    }
    [NSBezierPath fillRect:rc];

    [gc restoreGraphicsState];
}

@end
