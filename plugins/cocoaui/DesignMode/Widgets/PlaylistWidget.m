//
//  PlaylistWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "PlaylistWidget.h"
#import "PlaylistViewController.h"

@interface PlaylistWidget()

@property (nonatomic) PlaylistViewController *viewController;

@end

@implementation PlaylistWidget

+ (NSString *)widgetType {
    return @"Playlist";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _viewController = [PlaylistViewController new];
    PlaylistView *view = (PlaylistView *)_viewController.view;

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:view];
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.viewController sendMessage:_id ctx:ctx p1:p1 p2:p2];
}

- (BOOL)makeFirstResponder {
    PlaylistView *playlistView = (PlaylistView *)self.viewController.view;
    [playlistView.window makeFirstResponder:playlistView.contentView];
    return YES;
}

@end
