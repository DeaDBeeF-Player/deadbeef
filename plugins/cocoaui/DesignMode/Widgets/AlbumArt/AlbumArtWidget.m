//
//  AlbumArtWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/03/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "AlbumArtWidget.h"
#import "AlbumArtWidgetViewController.h"
#include <deadbeef/deadbeef.h>

@interface AlbumArtWidget() <AlbumArtWidgetViewControllerDelegate>

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic) AlbumArtWidgetViewController *viewController;

@end

@implementation AlbumArtWidget

+ (NSString *)widgetType {
    return @"AlbumArt";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.deps = deps;

    self.viewController = [AlbumArtWidgetViewController new];
    [self.topLevelView addSubview:self.viewController.view];

    self.viewController.view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.viewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [self.viewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [self.viewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [self.viewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    self.viewController.delegate = self;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
    case DB_EV_PLAYLISTSWITCHED:
    case DB_EV_CURSOR_MOVED:
        [self.viewController refresh];
        break;
    }
}
- (nullable NSDictionary *)serializedSettingsDictionary {
    NSMutableDictionary *settings = [NSMutableDictionary new];

    switch (self.viewController.displayMode) {
    case albumArtDisplayModeSelected:
        settings[@"displayMode"] = @"selected";
        break;
    case albumArtDisplayModePlaying:
        settings[@"displayMode"] = @"playing";
        break;
    case albumArtDisplayModePlayingOrSelected:
        settings[@"displayMode"] = @"playingOrSelected";
        break;
    }

    return settings;
}

- (BOOL)deserializeFromSettingsDictionary:(nullable NSDictionary *)dictionary {
    id displayModeObject = dictionary[@"displayMode"];

    if ([displayModeObject isKindOfClass:NSString.class]) {
        NSString *displayMode = displayModeObject;
        if ([displayMode isEqualToString:@"selected"]) {
            self.viewController.displayMode = albumArtDisplayModeSelected;
        }
        else if ([displayMode isEqualToString:@"playing"]) {
            self.viewController.displayMode = albumArtDisplayModePlaying;
        }
        else if ([displayMode isEqualToString:@"playingOrSelected"]) {
            self.viewController.displayMode = albumArtDisplayModePlayingOrSelected;
        }
    }

    return YES;
}

#pragma mark - AlbumArtWidgetViewControllerDelegate

- (void)configurationDidChange {
    [self.deps.state layoutDidChange];
}

@end
