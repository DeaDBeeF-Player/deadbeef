//
//  LyricsWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "LyricsViewController.h"
#import "LyricsWidget.h"

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

    self.viewController = [[LyricsViewController alloc] initWithNibName:@"LyricsViewController" bundle:nil];

    NSView *view = self.viewController.view;

    [self.topLevelView addSubview:view];

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_SONGCHANGED) {
        ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
        self.viewController.track = ev->to;
    }
}

@end
