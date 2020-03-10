/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2020 Alexey Yakovenko and other contributors

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

#import <QuartzCore/CATransaction.h>
#import "DdbSeekBar.h"

@interface DdbSeekBar() <CALayerDelegate>

@property (nonatomic,readwrite) BOOL dragging;

@property (nonatomic) CALayer *track;
@property (nonatomic) CALayer *trackPos;
@property (nonatomic) CALayer *thumb;

@property (nonatomic) NSColor *trackBackgroundColor;
@property (nonatomic) NSColor *trackInactiveBackgroundColor;
@property (nonatomic) NSColor *trackPosBackgroundColor;
@property (nonatomic) NSColor *trackPosInactiveBackgroundColor;
@property (nonatomic) NSColor *thumbBackgroundColor;
@property (nonatomic) NSColor *thumbBorderColor;
@property (nonatomic) NSColor *thumbInactiveBackgroundColor;
@property (nonatomic) NSColor *thumbInactiveBorderColor;

@end

@implementation DdbSeekBar

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (void)setup {
    self.wantsLayer = YES;

    self.layer = [CALayer new];
    self.layer.delegate = self;

    self.track = [CALayer new];
    self.trackPos = [CALayer new];
    self.thumb = [CALayer new];

    self.trackBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.trackInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.2];
    self.trackPosBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.6];
    self.trackPosInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.thumbBackgroundColor = [NSColor.alternateSelectedControlColor highlightWithLevel:0.2];
    self.thumbInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.thumbBorderColor = [NSColor.whiteColor shadowWithLevel:0.1];
    self.thumbInactiveBorderColor = [NSColor.whiteColor shadowWithLevel:0.1];

    self.track.backgroundColor = self.trackBackgroundColor.CGColor;
    self.track.cornerRadius = 2;
    self.trackPos.backgroundColor = self.trackPosBackgroundColor.CGColor;
    self.trackPos.cornerRadius = 2;

    self.thumb.backgroundColor = self.thumbBackgroundColor.CGColor;
    self.thumb.borderColor = self.thumbBorderColor.CGColor;
    self.thumb.borderWidth = 1;
    self.thumb.cornerRadius = 3;

    [self.layer addSublayer:self.track];
    [self.layer addSublayer:self.trackPos];
    [self.layer addSublayer:self.thumb];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(becameKey:) name:NSWindowDidBecomeKeyNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(resignedKey:) name:NSWindowDidResignKeyNotification object:nil];
}

- (void)becameKey:(NSNotification *)notification {
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.track.backgroundColor = self.trackBackgroundColor.CGColor;
    self.trackPos.backgroundColor = self.trackPosBackgroundColor.CGColor;
    self.thumb.backgroundColor = self.thumbBackgroundColor.CGColor;
    self.thumb.borderColor = self.thumbBorderColor.CGColor;
    [CATransaction commit];
}

- (void)resignedKey:(NSNotification *)notification {
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.track.backgroundColor = self.trackInactiveBackgroundColor.CGColor;
    self.trackPos.backgroundColor = self.trackPosInactiveBackgroundColor.CGColor;
    self.thumb.backgroundColor = self.thumbInactiveBackgroundColor.CGColor;
    self.thumb.borderColor = self.thumbInactiveBorderColor.CGColor;
    [CATransaction commit];
}

- (void)layoutSublayersOfLayer:(CALayer *)layer {
    if (layer == self.layer) {
        [CATransaction begin];
        [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
        CGFloat y = NSHeight(self.frame)/2;
        self.track.frame = NSMakeRect(3, y-1.5, NSWidth(self.frame)-6, 3);
        [self layoutThumbLayer];
        [CATransaction commit];
    }
}

- (void)layoutThumbLayer {
    CGFloat y = NSHeight(self.frame)/2;
    CGFloat x = self.floatValue / 100 * (NSWidth(self.frame)-6);\
    self.thumb.frame = NSMakeRect(x, y-6, 6, 12);
    self.trackPos.frame = NSMakeRect(3, y-1.5, x, 3);
}

- (void)setFloatValue:(float)floatValue {
    [super setFloatValue:floatValue];

    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    [self layoutThumbLayer];
    [CATransaction commit];
}

- (void)updateThumb:(NSEvent * _Nonnull)theEvent {
    CGFloat pos = [self convertPoint:theEvent.locationInWindow fromView:nil].x;

    pos = (pos-3)/(NSWidth(self.frame)-6)*100;
    pos = MAX(0, MIN(100, pos));
    self.floatValue = pos;

    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    [self layoutThumbLayer];
    [CATransaction commit];
}

- (void)mouseDown:(NSEvent *)theEvent {
    if (theEvent.type != NSEventTypeLeftMouseDown) {
        return;
    }

    [self updateThumb:theEvent];

    self.dragging = YES;

    for (;;) {
        NSEvent *event = [self.window nextEventMatchingMask: NSEventMaskLeftMouseUp |
                          NSEventMaskLeftMouseDragged];

        if (event.type == NSEventTypeLeftMouseDragged) {
            [self updateThumb:event];
        }
        else if (event.type == NSEventTypeLeftMouseUp) {
            [self sendAction:self.action to:self.target];
            break;
        }
    }

    self.dragging = NO;
}

@end
