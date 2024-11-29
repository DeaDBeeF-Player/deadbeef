/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2020 Oleksiy Yakovenko and other contributors

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

@interface DdbSeekBar() <CALayerDelegate> {
    float _position;
    NSFormatter *_formatter;
}

@property (nonatomic,readwrite) BOOL dragging;

@property (nonatomic) CALayer *track;
@property (nonatomic) CALayer *trackPos;
@property (nonatomic) CALayer *thumb;

@property (nonatomic) NSColor *trackFillColor;
@property (nonatomic) NSColor *trackInactiveFillColor;
@property (nonatomic) NSColor *trackBorderColor;
@property (nonatomic) NSColor *trackBorderInactiveColor;
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

- (void)setEnabled:(BOOL)enabled {
    super.enabled = enabled;
    [self updateThumbVisibility];
}

- (void)initColors {
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        self.trackPosBackgroundColor = NSColor.controlAccentColor;
    }
    else
#endif
    {
        self.trackPosBackgroundColor = [NSColor.alternateSelectedControlColor highlightWithLevel:0.2];
    }
    self.trackPosInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.thumbBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.7];
    self.thumbInactiveBackgroundColor = [NSColor.whiteColor shadowWithLevel:0.4];
    self.thumbBorderColor = NSColor.windowBackgroundColor;
    self.thumbInactiveBorderColor = NSColor.windowBackgroundColor;

    if (NSApp.effectiveAppearance == [NSAppearance appearanceNamed:NSAppearanceNameAqua]) {
        self.trackFillColor = [NSColor colorWithRed:0.874 green:0.878 blue:0.882 alpha:1];
        self.trackBorderColor = [NSColor colorWithRed:0.824 green:0.827 blue:0.831 alpha:1];
        self.trackInactiveFillColor = [NSColor colorWithRed:0.843 green:0.847 blue:0.847 alpha:1];
        self.trackBorderInactiveColor = [NSColor colorWithRed:0.792 green:0.796 blue:0.8 alpha:1];
    }
    else {
        self.trackFillColor = [NSColor colorWithRed:0.270 green:0.275 blue:0.278 alpha:1];
        self.trackInactiveFillColor = [NSColor colorWithRed:0.216 green:0.220 blue:0.227 alpha:1];
    }
}

- (void)setup {
    self.wantsLayer = YES;

    [self initColors];

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

    self.track.cornerRadius = 2.5;
    self.trackPos.cornerRadius = 2.5;
    self.track.borderWidth = 1;

    self.thumb.borderWidth = 1;
    self.thumb.cornerRadius = 3;
    self.thumb.shadowColor = NSColor.blackColor.CGColor;
    self.thumb.shadowOffset = NSMakeSize(0, -1);
    self.thumb.shadowRadius = 1;
    self.thumb.shadowOpacity = 0.3;
    self.thumb.hidden = YES;

    [self.layer addSublayer:self.track];
    [self.layer addSublayer:self.trackPos];
    [self.layer addSublayer:self.thumb];

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(becameKey:) name:NSWindowDidBecomeKeyNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(resignedKey:) name:NSWindowDidResignKeyNotification object:nil];

    NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:NSTrackingActiveInActiveApp|NSTrackingInVisibleRect|NSTrackingMouseEnteredAndExited owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];


    NSLayoutConstraint *minWidthConstraint = [self.widthAnchor constraintGreaterThanOrEqualToConstant:100];
    minWidthConstraint.priority = NSLayoutPriorityDefaultHigh;
    NSLayoutConstraint *maxWidthConstraint = [self.widthAnchor constraintLessThanOrEqualToConstant:10000];
    maxWidthConstraint.priority = NSLayoutPriorityDefaultHigh;
    [NSLayoutConstraint activateConstraints:@[
        minWidthConstraint,
        maxWidthConstraint,
    ]];

    [self setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationHorizontal];
}

- (void)updateThumbVisibility {
    NSPoint pos = NSEvent.mouseLocation;
    NSRect rect = NSMakeRect(pos.x, pos.y, 0, 0);
    rect = [self.window convertRectFromScreen:rect];
    pos = rect.origin;
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

- (void)layout {
    [super layout];
    [self initColors];
    [self updateTrackColors];
    [self updateTrackPosColors];
    [self updateThumbColors];
}

- (void)updateTrackColors {
    if (self.isKey) {
        self.track.backgroundColor = self.trackFillColor.CGColor;
        self.track.borderColor = self.trackBorderColor.CGColor;
    }
    else {
        self.track.backgroundColor = self.trackInactiveFillColor.CGColor;
        self.track.borderColor = self.trackBorderInactiveColor.CGColor;
    }
    if (NSApp.effectiveAppearance == [NSAppearance appearanceNamed:NSAppearanceNameAqua]) {
        self.track.borderWidth = 1;
    }
    else {
        self.track.borderWidth = 0;
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
    if (notification.object != self.window) {
        return;
    }
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    self.isKey = YES;
    [self updateTrackColors];
    [self updateTrackPosColors];
    [self updateThumbColors];
    [CATransaction commit];
}

- (void)resignedKey:(NSNotification *)notification {
    if (notification.object != self.window) {
        return;
    }
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
        self.track.frame = NSMakeRect(3, y-2, NSWidth(self.frame)-6, 4);
        [self layoutThumbLayer];
        [CATransaction commit];
    }
}

- (void)layoutThumbLayer {
    CGFloat y = NSHeight(self.frame)/2;
    CGFloat x = self.position / 100 * (NSWidth(self.frame)-6);
    if (!self.thumb.hidden) {
        self.thumb.frame = NSMakeRect(x, y-6, 6, 12);
    }
    self.trackPos.frame = NSMakeRect(3, y-2, x, 4);
}

- (void)setPosition:(float)position {
    _position = position;

    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    [self layoutThumbLayer];
    [CATransaction commit];
}

- (void)updatePosition:(float)position {
    position = MAX(0, MIN(100, position));
    self.position = position;
    self.overlay.text = self.stringValue;

    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    [self layoutThumbLayer];
    [CATransaction commit];
}

- (float)floatValue {
    return self.position;
}

- (void)setFloatValue:(float)value {
    self.position = value;
}

- (double)doubleValue {
    return (double)self.position;
}

- (NSString *)stringValue {
    return [self.formatter stringForObjectValue:self];
}

- (void)setFormatter:(__kindof NSFormatter *)formatter {
    _formatter = formatter;
}

- (NSFormatter *)formatter {
    return _formatter;
}

- (void)refreshOverlayTimer {
    [self.overlayTimer invalidate];
    self.overlayTimer = [NSTimer scheduledTimerWithTimeInterval:1 repeats:NO block:^(NSTimer * _Nonnull timer) {
        self.overlay.hidden = YES;
    }];
}

- (void)scrollWheel:(NSEvent *)event {
    [self updatePosition:self.floatValue + event.scrollingDeltaX + event.scrollingDeltaY];
    [self sendAction:self.action to:self.target];
    self.overlay.hidden = NO;
    [self refreshOverlayTimer];
}

- (float)percentageFromMouseEvent:(NSEvent *)event {
    float pos = [self convertPoint:event.locationInWindow fromView:nil].x;
    pos = (pos-3)/(NSWidth(self.frame)-6)*100;
    return pos;
}

- (void)mouseDown:(NSEvent *)theEvent {
    if (theEvent.type != NSEventTypeLeftMouseDown) {
        return;
    }

    if (!self.enabled) {
        return;
    }

    [self updatePosition:[self percentageFromMouseEvent:theEvent]];

    self.dragging = YES;

    self.overlay.hidden = NO;

    for (;;) {
        NSEvent *event = [self.window nextEventMatchingMask: NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged];

        if (event.type == NSEventTypeLeftMouseDragged) {
            [self updatePosition:[self percentageFromMouseEvent:event]];
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
