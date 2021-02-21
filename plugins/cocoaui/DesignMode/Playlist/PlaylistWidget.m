//
//  PlaylistWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "PlaylistWidget.h"
#import "PlaylistViewController.h"

@interface PlaylistWidget()

@property (nonatomic) PlaylistViewController *viewController;

@end

@implementation PlaylistWidget

- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState menuBuilder:(nullable id<WidgetMenuBuilderProtocol>)menuBuilder {
    self = [super initWithDesignModeState:designModeState menuBuilder:menuBuilder];
    if (self == nil) {
        return nil;
    }

    _viewController = [[PlaylistViewController alloc] initWithNibName:nil bundle:nil];
    PlaylistView *view = [PlaylistView new];
    _viewController.view = view;
    [_viewController setup];

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:view];
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (nonnull NSString *)serializedString {
    return @"{}";
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.viewController sendMessage:_id ctx:ctx p1:p1 p2:p2];
}

- (void)makeFirstResponder {
    PlaylistView *playlistView = (PlaylistView *)self.viewController.view;
    [playlistView.window makeFirstResponder:playlistView.contentView];
}

@end
