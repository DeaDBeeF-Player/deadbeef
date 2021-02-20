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

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _viewController = [[PlaylistViewController alloc] initWithNibName:nil bundle:nil];
    PlaylistView *view = [PlaylistView new];
    _viewController.view = view;
    [_viewController setup];

    return self;
}

- (NSView *)view {
    return self.viewController.view;
}

- (nonnull NSString *)serializedString {
    return @"{}";
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.viewController sendMessage:_id ctx:ctx p1:p1 p2:p2];
}

@end
