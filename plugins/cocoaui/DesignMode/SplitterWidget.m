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
    HoldingModeEqual = 0,
    HoldingModeFirst = 1,
    HoldingModeSecond = 2,
};

#pragma mark - HSplitterWidget

@interface HSplitterWidget() <NSSplitViewDelegate>

@property (nonatomic) NSSplitView *splitView;
@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@property (nonatomic,readonly) CGFloat dividerPosition;
@property (nonatomic,readonly) CGFloat viewDividerPosition;
@property (nonatomic,readonly) CGFloat splitViewSize;
@property (nonatomic,readonly) CGFloat firstPaneSize;
@property (nonatomic,readonly) CGFloat secondPaneSize;

@property (nonatomic) HoldingMode holdingMode;

// deserialized values (consumed once)
@property (nonatomic) NSDictionary *deserializedSettings;


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

    _splitView.delegate = self;

    return self;
}

- (void)appendChild:(id<WidgetProtocol>)child {
    // should not ever be called, since "isPlaceholder" is NO
    [super appendChild:child];
}

- (void)removeChild:(id<WidgetProtocol>)child {
    CGFloat dividerPosition = self.dividerPosition;
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
    [self updateDividerPosition:dividerPosition];
}

- (void)updateDividerPosition:(CGFloat)dividerPosition {
    CGFloat splitViewSize = self.splitViewSize;
    if (splitViewSize == 0) {
        return;
    }

    switch (self.holdingMode) {
    case HoldingModeEqual:
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
    CGFloat dividerPosition = self.dividerPosition;

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

    [self updateDividerPosition:dividerPosition];
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

- (CGFloat)dividerPosition {
    CGFloat splitViewSize = self.splitViewSize;

    switch (self.holdingMode) {
    case HoldingModeEqual:
        if (splitViewSize > 1) {
            return self.firstPaneSize / splitViewSize;
        }
        else {
            return 0.5;
        }
    case HoldingModeFirst:
        return self.firstPaneSize;
    case HoldingModeSecond:
        return self.secondPaneSize;
    }
}

- (NSDictionary *)serializedSettingsDictionary {
    return @{
        @"holdingMode": @(self.holdingMode),
        @"position": @(self.dividerPosition),
    };
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    self.deserializedSettings = dictionary;
    return YES;
}

- (void)splitViewDidResizeSubviews:(NSNotification *)notification {
    [self.deps.state layoutDidChange];
}

- (void)splitViewBoundsDidChange:(NSNotification *)notification {
    if (self.deserializedSettings == nil) {
        return;
    }

    id holdingModeObject = self.deserializedSettings[@"holdingMode"];
    if ([holdingModeObject isKindOfClass:NSNumber.class]) {
        NSNumber *holdingModeNumber = holdingModeObject;
        self.holdingMode = (HoldingMode)holdingModeNumber.integerValue;
    }

    id positionObject = self.deserializedSettings[@"position"];
    if ([positionObject isKindOfClass:NSNumber.class]) {
        NSNumber *positionNumber = positionObject;
        CGFloat position = positionNumber.doubleValue;
        [self updateDividerPosition:position];
    }

    self.deserializedSettings = nil;
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
