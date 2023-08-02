//
//  LyricsWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "LyricsViewController.h"
#import "LyricsWidget.h"

extern DB_functions_t *deadbeef;

@interface LyricsWidget()

@property (nonatomic) LyricsViewController *viewController;

@end

@implementation LyricsWidget

+ (NSString *)widgetType {
    return @"Lyrics";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.viewController = [LyricsViewController new]; 
    NSView *view = self.viewController.view;

    [self.topLevelView addSubview:view];

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    [self update];

    return self;
}

- (void)update {
    int cursor = deadbeef->pl_get_cursor(PL_MAIN);
    if (cursor == -1) {
        self.viewController.track = NULL;
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr();
        if (plt) {
            ddb_playItem_t *track = deadbeef->plt_get_item_for_idx(plt, cursor, PL_MAIN);

            if (track != NULL) {
                self.viewController.track = track;
                deadbeef->pl_item_unref (track);
            }

            deadbeef->plt_unref (plt);
        }
    }
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_CURSOR_MOVED) {
        dispatch_async (dispatch_get_main_queue(), ^{
            [self update];
        });
    }
}

@end
