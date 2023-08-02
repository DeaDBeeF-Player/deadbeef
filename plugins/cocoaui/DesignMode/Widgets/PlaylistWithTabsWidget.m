//
//  PlaylistWithTabsWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "PlaylistWithTabsWidget.h"
#import "PlaylistViewController.h"
#import "DdbTabStripViewController.h"

@interface PlaylistWithTabsWidget()

@property (nonatomic) PlaylistViewController *viewController;
@property (nonatomic) DdbTabStripViewController *tabStripViewViewController;

@end

@implementation PlaylistWithTabsWidget

+ (NSString *)widgetType {
    return @"PlaylistWithTabs";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.tabStripViewViewController = [DdbTabStripViewController new];

    NSView *tabStripView = self.tabStripViewViewController.view;

    self.viewController = [PlaylistViewController new];
    PlaylistView *playlistView = (PlaylistView *)self.viewController.view;

    [self.topLevelView addSubview:self.tabStripViewViewController.view];
    [self.topLevelView addSubview:playlistView];

    tabStripView.translatesAutoresizingMaskIntoConstraints = NO;
    playlistView.translatesAutoresizingMaskIntoConstraints = NO;

    [tabStripView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [tabStripView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [tabStripView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;

    [playlistView.topAnchor constraintEqualToAnchor:tabStripView.bottomAnchor].active = YES;
    [playlistView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [playlistView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [playlistView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.tabStripViewViewController widgetMessage:_id ctx:ctx p1:p1 p2:p2];
    [self.viewController sendMessage:_id ctx:ctx p1:p1 p2:p2];
}

- (BOOL)makeFirstResponder {
    PlaylistView *playlistView = (PlaylistView *)self.viewController.view;
    [playlistView.window makeFirstResponder:playlistView.contentView];
    return YES;
}

@end
