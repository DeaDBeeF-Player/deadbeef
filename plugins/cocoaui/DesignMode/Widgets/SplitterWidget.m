//
//  SplitterWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "SplitterWidget.h"
#import "WidgetFactory.h"
#import "PlaceholderWidget.h"

typedef NS_ENUM(NSInteger,HoldingMode) {
    HoldingModeProportional = 0,
    HoldingModeFirst = 1,
    HoldingModeSecond = 2,
};

#pragma mark - HSplitterWidget

@interface HSplitterWidget() <NSSplitViewDelegate>

@property (nonatomic) NSSplitView *splitView;
@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@property (nonatomic) CGFloat normalizedDividerPosition;
@property (nonatomic) HoldingMode holdingMode;
@property (nonatomic) BOOL isLocked;

@property (nonatomic) BOOL layoutFinalized; // must be set to true when the splitview gets its final bounds

//@property (nonatomic,readonly) CGFloat viewDividerPosition;
@property (nonatomic,readonly) CGFloat splitViewSize;
@property (nonatomic,readonly) CGFloat firstPaneSize;
@property (nonatomic,readonly) CGFloat secondPaneSize;


@end

@implementation HSplitterWidget

+ (NSString *)widgetType {
    return @"HSplitter";
}

- (BOOL)isVertical {
    return NO;
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _deps = deps;

    _splitView = [[NSSplitView alloc] initWithFrame:NSZeroRect];
    _splitView.vertical = !self.isVertical; // The NSSplitView.vertical means the divider line orientation

    // Default settings

    // FIXME: accessing _holdingMode fails to compile
    self.holdingMode = HoldingModeProportional;
    _normalizedDividerPosition = 0.5;
    _isLocked = NO;
    _splitView.dividerStyle = NSSplitViewDividerStyleThin;

    id<WidgetProtocol> pane1 = [deps.factory createWidgetWithType:PlaceholderWidget.widgetType];
    id<WidgetProtocol> pane2 = [deps.factory createWidgetWithType:PlaceholderWidget.widgetType];

    [self appendChild:pane1];
    [self appendChild:pane2];

    [_splitView insertArrangedSubview:pane1.view atIndex:0];
    [_splitView insertArrangedSubview:pane2.view atIndex:1];

    _splitView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:_splitView];
    [_splitView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_splitView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_splitView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_splitView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(splitViewBoundsDidChange:) name:NSViewBoundsDidChangeNotification object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(splitViewBoundsDidChange:) name:NSViewFrameDidChangeNotification object:nil];

    _splitView.delegate = self;

    return self;
}

- (void)appendChild:(id<WidgetProtocol>)child {
    // should not ever be called, since "isPlaceholder" is NO
    [super appendChild:child];
}

- (void)removeChild:(id<WidgetProtocol>)child {
    CGFloat normalizedDividerPosition = self.normalizedDividerPosition;
    [super removeChild:child];
    id<WidgetProtocol> pane = [self.deps.factory createWidgetWithType:PlaceholderWidget.widgetType];
    if (self.splitView.arrangedSubviews[0] == child.view) {
        [self.splitView removeArrangedSubview:child.view];
        [_splitView insertArrangedSubview:pane.view atIndex:0];
    }
    else {
        [self.splitView removeArrangedSubview:child.view];
        [_splitView insertArrangedSubview:pane.view atIndex:1];
    }
    [super appendChild:pane];
    [self updateDividerPositionFromNormalized:normalizedDividerPosition];
}

- (void)updateDividerPositionFromNormalized:(CGFloat)dividerPosition {
    CGFloat splitViewSize = self.splitViewSize;

    switch (self.holdingMode) {
    case HoldingModeProportional:
        [self.splitView setPosition:(dividerPosition * splitViewSize) ofDividerAtIndex:0];
        break;
    case HoldingModeFirst:
        [self.splitView setPosition:dividerPosition ofDividerAtIndex:0];
        break;
    case HoldingModeSecond:
        [self.splitView setPosition:(splitViewSize - dividerPosition) ofDividerAtIndex:0];
        break;
    }
}

- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild {
    CGFloat dividerPosition = self.normalizedDividerPosition;

    if (self.splitView.arrangedSubviews[0] == child.view) {
        [self.splitView removeArrangedSubview:child.view];
        [_splitView insertArrangedSubview:newChild.view atIndex:0];
        [self.childWidgets removeObject:child];
        [self.childWidgets insertObject:newChild atIndex:0];
    }
    else {
        [self.splitView removeArrangedSubview:child.view];
        [_splitView insertArrangedSubview:newChild.view atIndex:1];
        [self.childWidgets removeObject:child];
        [self.childWidgets insertObject:newChild atIndex:1];
    }
    child.parentWidget = nil;
    newChild.parentWidget = self;

    [self updateDividerPositionFromNormalized:dividerPosition];
}

- (CGFloat)splitViewSize {
    return self.splitView.isVertical ? NSWidth(self.splitView.frame) : NSHeight(self.splitView.frame);
}

- (CGFloat)firstPaneSize {
    return self.splitView.isVertical ? NSWidth(self.splitView.arrangedSubviews[0].frame) : NSHeight(self.splitView.arrangedSubviews[0].frame);
}

- (CGFloat)secondPaneSize {
    return self.splitView.isVertical ? NSWidth(self.splitView.arrangedSubviews[1].frame) : NSHeight(self.splitView.arrangedSubviews[1].frame);
}

- (void)updateNormalizedDividerPosition {
    if (!self.layoutFinalized) {
        return;
    }
    CGFloat splitViewSize = self.splitViewSize;

    switch (self.holdingMode) {
    case HoldingModeProportional:
        if (splitViewSize > 1) {
            self.normalizedDividerPosition = self.firstPaneSize / splitViewSize;
        }
        else {
            self.normalizedDividerPosition = 0.5;
        }
        break;
    case HoldingModeFirst:
        self.normalizedDividerPosition = self.firstPaneSize;
        break;
    case HoldingModeSecond:
        self.normalizedDividerPosition = self.secondPaneSize;
        break;
    }
}

- (HoldingMode)holdingMode {
    if ([self.splitView holdingPriorityForSubviewAtIndex:0] == NSLayoutPriorityDefaultLow && [self.splitView holdingPriorityForSubviewAtIndex:1] == NSLayoutPriorityDefaultLow) {
        return HoldingModeProportional;
    }
    else if ([self.splitView holdingPriorityForSubviewAtIndex:0] == (NSLayoutPriorityDefaultLow+1)) {
        return HoldingModeFirst;
    }
    else {
        return HoldingModeSecond;
    }
}

- (void)setHoldingMode:(HoldingMode)holdingMode {
    switch (holdingMode) {
    case HoldingModeProportional:
        [self.splitView setHoldingPriority:NSLayoutPriorityDefaultLow forSubviewAtIndex:0];
        [self.splitView setHoldingPriority:NSLayoutPriorityDefaultLow forSubviewAtIndex:1];
        break;
    case HoldingModeFirst:
        [self.splitView setHoldingPriority:(NSLayoutPriorityDefaultLow+1) forSubviewAtIndex:0];
        [self.splitView setHoldingPriority:NSLayoutPriorityDefaultLow forSubviewAtIndex:1];
        break;
    case HoldingModeSecond:
        [self.splitView setHoldingPriority:NSLayoutPriorityDefaultLow forSubviewAtIndex:0];
        [self.splitView setHoldingPriority:(NSLayoutPriorityDefaultLow+1) forSubviewAtIndex:1];
        break;
    }
}

- (NSDictionary *)serializedSettingsDictionary {
    return @{
        @"holdingMode": @(self.holdingMode),
        @"position": @(self.normalizedDividerPosition),
        @"isLocked": @(self.isLocked),
        @"thickDivider": @(self.splitView.dividerStyle == NSSplitViewDividerStyleThick),
    };
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    id holdingModeObject = dictionary[@"holdingMode"];
    if ([holdingModeObject isKindOfClass:NSNumber.class]) {
        NSNumber *holdingModeNumber = holdingModeObject;
        self.holdingMode = (HoldingMode)holdingModeNumber.integerValue;
    }

    id positionObject = dictionary[@"position"];
    if ([positionObject isKindOfClass:NSNumber.class]) {
        NSNumber *positionNumber = positionObject;
        self.normalizedDividerPosition = positionNumber.doubleValue;

        // Update the splitview if it's ready
        [self updateDividerPositionFromNormalized:self.normalizedDividerPosition];
    }

    id isLockedObject = dictionary[@"isLocked"];
    if ([isLockedObject isKindOfClass:NSNumber.class]) {
        NSNumber *isLockedNumber = isLockedObject;
        self.isLocked = isLockedNumber.boolValue;
    }

    id thickDividerObject = dictionary[@"thickDivider"];
    if ([thickDividerObject isKindOfClass:NSNumber.class]) {
        NSNumber *thickDividerNumber = thickDividerObject;
        self.splitView.dividerStyle = thickDividerNumber.boolValue ? NSSplitViewDividerStyleThick : NSSplitViewDividerStyleThin;
    }
    return YES;
}

- (void)splitViewBoundsDidChange:(NSNotification *)notification {
    if (NSEqualSizes(self.splitView.bounds.size, NSZeroSize)
        || self.layoutFinalized) {
        return;
    }
    self.splitView.delegate = nil;

    self.layoutFinalized = YES;
    [self updateDividerPositionFromNormalized: self.normalizedDividerPosition];

    self.splitView.delegate = self;
}

- (NSArray<NSMenuItem *> *)menuItems {
    NSMenuItem *holdFirst = [[NSMenuItem alloc] initWithTitle:@"Hold First" action:@selector(holdFirstAction:) keyEquivalent:@""];
    holdFirst.state = self.holdingMode == HoldingModeFirst ? NSControlStateValueOn : NSControlStateValueOff;
    holdFirst.target = self;

    NSMenuItem *holdSecond = [[NSMenuItem alloc] initWithTitle:@"Hold Second" action:@selector(holdSecondAction:) keyEquivalent:@""];
    holdSecond.state = self.holdingMode == HoldingModeSecond ? NSControlStateValueOn : NSControlStateValueOff;
    holdSecond.target = self;

    NSMenuItem *holdProportional = [[NSMenuItem alloc] initWithTitle:@"Proportional" action:@selector(holdProportionalAction:) keyEquivalent:@""];
    holdProportional.state = self.holdingMode == HoldingModeProportional ? NSControlStateValueOn : NSControlStateValueOff;
    holdProportional.target = self;

    NSMenuItem *lockItem = [[NSMenuItem alloc] initWithTitle:@"Lock Splitter Position" action:@selector(lockAction:) keyEquivalent:@""];
    lockItem.state = self.isLocked ? NSControlStateValueOn : NSControlStateValueOff;
    lockItem.target = self;

    NSMenuItem *dividerSize = [[NSMenuItem alloc] initWithTitle:@"Thick Divider" action:@selector(thickDividerToggleAction:) keyEquivalent:@""];
    dividerSize.target = self;
    BOOL isThick = (self.splitView.dividerStyle == NSSplitViewDividerStyleThick);
    dividerSize.state = isThick ? NSControlStateValueOn : NSControlStateValueOff;

    return @[
        lockItem,
        dividerSize,
        [NSMenuItem separatorItem],
        holdFirst,
        holdSecond,
        holdProportional,
    ];
}

- (void)holdFirstAction:(NSMenuItem *)sender {
    self.holdingMode = HoldingModeFirst;
    [self.deps.state layoutDidChange];
}

- (void)holdSecondAction:(NSMenuItem *)sender {
    self.holdingMode = HoldingModeSecond;
    [self.deps.state layoutDidChange];
}

- (void)holdProportionalAction:(NSMenuItem *)sender {
    self.holdingMode = HoldingModeProportional;
    [self.deps.state layoutDidChange];
}

- (void)lockAction:(NSMenuItem *)sender {
    self.isLocked = !self.isLocked;
    [self.deps.state layoutDidChange];
}

- (void)thickDividerToggleAction:(NSMenuItem *)sender {
    BOOL isThick = (self.splitView.dividerStyle != NSSplitViewDividerStyleThick);
    self.splitView.dividerStyle = isThick ? NSSplitViewDividerStyleThick : NSSplitViewDividerStyleThin;
    [self.deps.state layoutDidChange];
}

#pragma mark - NSSplitViewDelegate

// Called when user or the splitview itself resizes subview -- then we need to recalculate the normalized position
// Update should not occur, if the split view is not fully configured (doesn't have both panes, or the initial position was not applied yet)
- (void)splitViewDidResizeSubviews:(NSNotification *)notification {
    CGFloat splitViewSize = self.splitViewSize;
    if (splitViewSize == 0 || !self.layoutFinalized) {
        return;
    }

    [self updateNormalizedDividerPosition];
    [self.deps.state layoutDidChange];
}

- (CGFloat)splitView:(NSSplitView *)splitView constrainSplitPosition:(CGFloat)proposedPosition ofSubviewAt:(NSInteger)dividerIndex {
    return self.isLocked ? self.firstPaneSize : proposedPosition;
}

@end

#pragma mark - VSplitterWidget

@implementation VSplitterWidget

+ (NSString *)widgetType {
    return @"VSplitter";
}

- (BOOL)isVertical {
    return YES;
}

@end
