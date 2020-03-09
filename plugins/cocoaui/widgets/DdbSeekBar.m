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
@property (nonatomic) CALayer *thumb;

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
    self.thumb = [CALayer new];

    self.track.borderWidth = 2;
    self.track.borderColor = [NSColor.alternateSelectedControlColor colorWithAlphaComponent:0.8].CGColor;
    self.track.cornerRadius = 5;

    self.thumb.backgroundColor = [NSColor.alternateSelectedControlColor colorWithAlphaComponent:0.8].CGColor;
    self.thumb.cornerRadius = 5;

    [self.layer addSublayer:self.thumb];
    [self.layer addSublayer:self.track];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(becameKey:) name:NSWindowDidBecomeKeyNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(resignedKey:) name:NSWindowDidResignKeyNotification object:nil];
}

- (void)becameKey:(NSNotification *)notification {
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.track.borderColor = [NSColor.alternateSelectedControlColor colorWithAlphaComponent:0.8].CGColor;
    self.thumb.backgroundColor = [NSColor.alternateSelectedControlColor colorWithAlphaComponent:0.8].CGColor;
    [CATransaction commit];
}

- (void)resignedKey:(NSNotification *)notification {
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.track.borderColor = NSColor.controlShadowColor.CGColor;
    self.thumb.backgroundColor = NSColor.controlShadowColor.CGColor;
    [CATransaction commit];
}

- (void)layoutSublayersOfLayer:(CALayer *)layer {
    if (layer == self.layer) {
        [CATransaction begin];
        [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
        CGFloat y = (NSHeight(self.frame)-10)/2;
        self.track.frame = NSMakeRect(0, y, NSWidth(self.frame), 10);
        [self layoutThumbLayer];
        [CATransaction commit];
    }
}

- (void)layoutThumbLayer {
    CGFloat y = (NSHeight(self.frame)-10)/2;
    CGFloat w = self.floatValue / 100 * NSWidth(self.frame);
    self.thumb.frame = NSMakeRect(0, y, w, 10);
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

    pos = pos/NSWidth(self.frame)*100;
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
