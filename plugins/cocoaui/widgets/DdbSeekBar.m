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
#import "SeekbarOverlay.h"

static void *kEffectiveAppearanceContext = &kEffectiveAppearanceContext;

@interface DdbSeekBar() <CALayerDelegate>

@property (nonatomic,readwrite) BOOL dragging;

@property (nonatomic) CALayer *track;
@property (nonatomic) CALayer *trackPos;
@property (nonatomic) CALayer *thumb;

@property (nonatomic,copy) NSColor *trackBackgroundColor;
@property (nonatomic,copy) NSColor *trackInactiveBackgroundColor;
@property (nonatomic,copy) NSColor *trackPosBackgroundColor;
@property (nonatomic,copy) NSColor *trackPosInactiveBackgroundColor;
@property (nonatomic,copy) NSColor *thumbBackgroundColor;
@property (nonatomic,copy) NSColor *thumbBorderColor;
@property (nonatomic,copy) NSColor *thumbInactiveBackgroundColor;
@property (nonatomic,copy) NSColor *thumbInactiveBorderColor;

@property (nonatomic) BOOL isKey;

@property (nonatomic) SeekbarOverlay *overlay;
@property (nonatomic) NSTimer *overlayTimer;

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

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (void)dealloc {
    [self removeObserver:self forKeyPath:@"effectiveAppearance"];
}

- (void)setEnabled:(BOOL)enabled {
    [super setEnabled:enabled];
    [self updateThumbVisibility];
}

- (void)initColors {
    self.trackBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.42];
    self.trackInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.2];
    if (@available(macOS 10.14, *)) {
        self.trackPosBackgroundColor = NSColor.controlAccentColor;
    }
    else {
        self.trackPosBackgroundColor = [NSColor.alternateSelectedControlColor highlightWithLevel:0.2];
    }
    self.trackPosInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.thumbBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.7];
    self.thumbInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.thumbBorderColor = [NSColor.windowBackgroundColor shadowWithLevel:0];
    self.thumbInactiveBorderColor = [NSColor.windowBackgroundColor shadowWithLevel:0];
}

- (void)setup {
    self.wantsLayer = YES;

    [self initColors];

    [self addObserver:self forKeyPath:@"effectiveAppearance" options:0 context:kEffectiveAppearanceContext];

    _overlay = [SeekbarOverlay new];
    [self addSubview:_overlay];
    _overlay.translatesAutoresizingMaskIntoConstraints = NO;
    [_overlay.centerXAnchor constraintEqualToAnchor:self.centerXAnchor].active = YES;
    [_overlay.centerYAnchor constraintEqualToAnchor:self.centerYAnchor].active = YES;
    _overlay.hidden = YES;


    self.layer = [CALayer new];
    self.layer.delegate = self;

    self.track = [CALayer new];
    self.trackPos = [CALayer new];
    self.thumb = [CALayer new];

    self.track.cornerRadius = 2;
    self.trackPos.cornerRadius = 2;

    self.thumb.borderWidth = 1;
    self.thumb.cornerRadius = 3;
    self.thumb.hidden = YES;

    [self.layer addSublayer:self.track];
    [self.layer addSublayer:self.trackPos];
    [self.layer addSublayer:self.thumb];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(becameKey:) name:NSWindowDidBecomeKeyNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(resignedKey:) name:NSWindowDidResignKeyNotification object:nil];

    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:NSTrackingActiveInActiveApp|NSTrackingInVisibleRect|NSTrackingMouseEnteredAndExited owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)updateThumbVisibility {
    NSPoint pos = NSEvent.mouseLocation;
    pos = [self.window convertPointFromScreen:pos];
    pos = [self convertPoint:pos fromView:nil];

    BOOL visible = self.enabled && (self.dragging || NSPointInRect(pos, self.bounds));
    self.thumb.hidden = !visible;

    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    [self layoutThumbLayer];
    [CATransaction commit];
}

- (void)mouseEntered:(NSEvent *)event {
    [self updateThumbVisibility];
}

- (void)mouseExited:(NSEvent *)event {
    [self updateThumbVisibility];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == kEffectiveAppearanceContext) {
        [self initColors];
        [self updateTrackColors];
        [self updateTrackPosColors];
        [self updateThumbColors];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)updateTrackColors {
    if (self.isKey) {
        self.track.backgroundColor = self.trackBackgroundColor.CGColor;
    }
    else {
        self.track.backgroundColor = self.trackInactiveBackgroundColor.CGColor;
    }
}

- (void)updateTrackPosColors {
    if (self.isKey) {
        self.trackPos.backgroundColor = self.trackPosBackgroundColor.CGColor;
    }
    else {
        self.trackPos.backgroundColor = self.trackPosInactiveBackgroundColor.CGColor;
    }
}

- (void)updateThumbColors {
    if (self.isKey) {
        self.thumb.backgroundColor = self.thumbBackgroundColor.CGColor;
        self.thumb.borderColor = self.thumbBorderColor.CGColor;
    }
    else {
        self.thumb.backgroundColor = self.thumbInactiveBackgroundColor.CGColor;
        self.thumb.borderColor = self.thumbInactiveBorderColor.CGColor;
    }
}

- (void)becameKey:(NSNotification *)notification {
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.isKey = YES;
    [self updateTrackColors];
    [self updateTrackPosColors];
    [self updateThumbColors];
    [CATransaction commit];
}

- (void)resignedKey:(NSNotification *)notification {
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.isKey = NO;
    [self updateTrackColors];
    [self updateTrackPosColors];
    [self updateThumbColors];
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
    if (!self.thumb.hidden) {
        self.thumb.frame = NSMakeRect(x, y-6, 6, 12);
    }
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
    self.overlay.text = self.stringValue;

    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    [self layoutThumbLayer];
    [CATransaction commit];
}

- (void)refreshOverlayTimer {
    [self.overlayTimer invalidate];
    self.overlayTimer = [NSTimer scheduledTimerWithTimeInterval:1 repeats:NO block:^(NSTimer * _Nonnull timer) {
        self.overlay.hidden = YES;
    }];
}

- (void)mouseDown:(NSEvent *)theEvent {
    if (theEvent.type != NSEventTypeLeftMouseDown) {
        return;
    }

    if (!self.enabled) {
        return;
    }

    [self updateThumb:theEvent];

    self.dragging = YES;

    self.overlay.hidden = NO;

    for (;;) {
        NSEvent *event = [self.window nextEventMatchingMask: NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged];

        if (event.type == NSEventTypeLeftMouseDragged) {
            [self updateThumb:event];
        }
        else if (event.type == NSEventTypeLeftMouseUp) {
            [self sendAction:self.action to:self.target];
            break;
        }
    }

    [self refreshOverlayTimer];
    self.dragging = NO;
}

@end
