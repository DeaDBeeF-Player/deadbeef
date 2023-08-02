//
//  PlaylistBrowserWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "PlaylistBrowserWidget.h"
#import "PlaylistBrowserViewController.h"

@interface PlaylistBrowserWidget()

@property (nonatomic) PlaylistBrowserViewController *viewController;

@end

@implementation PlaylistBrowserWidget

+ (NSString *)widgetType {
    return @"PlaylistBrowser";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.viewController = [PlaylistBrowserViewController new];

    NSView *view = self.viewController.view;

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:view];
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.viewController widgetMessage:_id ctx:ctx p1:p1 p2:p2];
}

@end
