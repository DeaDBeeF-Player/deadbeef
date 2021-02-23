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

@interface HSplitterWidget()

@property (nonatomic) NSSplitView *splitView;
@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

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

    return self;
}

- (void)appendChild:(id<WidgetProtocol>)child {
    // should not ever be called, since "isPlaceholder" is NO
    [super appendChild:child];
}

- (void)removeChild:(id<WidgetProtocol>)child {
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
}

- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild {
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
}

@end


@implementation VSplitterWidget

+ (NSString *)widgetType {
    return @"VSplitter";
}

- (BOOL)isVertical {
    return YES;
}

@end
