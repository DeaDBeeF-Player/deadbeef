//
//  AlbumArtWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/03/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "AlbumArtImageView.h"
#import "AlbumArtWidget.h"
#import "CoverManager.h"
#include <deadbeef/deadbeef.h>
#include "artwork.h"
#import "Weakify.h"

extern DB_functions_t *deadbeef;

@interface AlbumArtWidget() <CoverManagerListener>

@property (nonatomic) NSImageView *imageView;
@property (nonatomic) ddb_playItem_t *track;
@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;

@property (nonatomic) int64_t sourceId;

@property (nonatomic) dispatch_block_t throttleBlock;
@property (nonatomic) NSInteger requestIndex;

@end

@implementation AlbumArtWidget

+ (NSString *)widgetType {
    return @"AlbumArt";
}

static void
artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    AlbumArtWidget *self = (__bridge AlbumArtWidget *)user_data;
    weakify(self);
    dispatch_async(dispatch_get_main_queue(), ^{
        strongify(self);
        if (self.track != NULL && (ddb_playItem_t *)p1 == self.track) {
            [self throttledUpdate];
        }
    });
}

- (void)dealloc
{
    [CoverManager.shared removeListener:self];
    if (_artwork_plugin != NULL) {
        _artwork_plugin->remove_listener (artwork_listener, (__bridge void *)self);
        _artwork_plugin = NULL;
    }
    if (self.track != NULL) {
        deadbeef->pl_item_unref (self.track);
        self.track = NULL;
    }
}

- (void)cleanup {
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    _artwork_plugin->add_listener (artwork_listener, (__bridge void *)self);

    self.sourceId = _artwork_plugin->allocate_source_id();

    // create view
    self.imageView = [AlbumArtImageView new];
    self.imageView.imageAlignment = NSImageAlignCenter;
    self.imageView.imageScaling = NSImageScaleProportionallyUpOrDown;

    // add view
    [self.topLevelView addSubview:self.imageView];

    // constrain view
    self.imageView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.imageView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [self.imageView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [self.imageView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [self.imageView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(frameDidChange:) name:NSViewFrameDidChangeNotification object:self.imageView];

    [CoverManager.shared addListener:self];

    return self;
}

- (void)throttledUpdate {
    if (self.throttleBlock != nil) {
        dispatch_block_cancel(self.throttleBlock);
    }
    weakify(self);
    self.throttleBlock = dispatch_block_create(0, ^{
        strongify(self);
        [self update];
        self.throttleBlock = nil;
    });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC/100), dispatch_get_main_queue(), self.throttleBlock);
}

- (void)frameDidChange:(NSNotification *)notification {
    [self throttledUpdate];
}

- (void)update {
    if (self.imageView.frame.size.width == 0 || self.imageView.frame.size.height == 0) {
        return;
    }

    if (self.track != nil) {
        deadbeef->pl_item_unref (self.track);
        self.track = NULL;
    }
    int cursor = deadbeef->pl_get_cursor(PL_MAIN);
    if (cursor == -1) {
        self.imageView.image = nil;
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr();
        if (plt) {
            ddb_playItem_t *it = deadbeef->plt_get_item_for_idx(plt, cursor, PL_MAIN);

            if (it) {
                self.track = it;

                self.artwork_plugin->cancel_queries_with_source_id(self.sourceId);

                CGSize availableSize = [self.view convertSizeToBacking:self.imageView.frame.size];

                NSImageView *imageView = self.imageView;
                NSInteger currentIndex = self.requestIndex++;
                NSImage *image = [CoverManager.shared coverForTrack:it sourceId:self.sourceId completionBlock:^(NSImage *img) {
                    if (currentIndex != self.requestIndex-1) {
                        return;
                    }
                    if (img != nil) {
                        NSSize desiredSize = [CoverManager.shared desiredSizeForImageSize:img.size availableSize:availableSize];
                        imageView.image = [CoverManager.shared createScaledImage:img newSize:desiredSize];
                    }
                    else {
                        imageView.image = nil;
                    }
                }];

                if (image != nil) {
                    NSSize desiredSize = [CoverManager.shared desiredSizeForImageSize:image.size availableSize:availableSize];
                    imageView.image = [CoverManager.shared createScaledImage:image newSize:desiredSize];
                }
            }

            deadbeef->plt_unref (plt);
        }
    }
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
    case DB_EV_PLAYLISTSWITCHED:
    case DB_EV_CURSOR_MOVED: {
        weakify(self);
        dispatch_async(dispatch_get_main_queue(), ^{
            strongify(self);
            [self throttledUpdate];
        });
    }
        break;
    }
}

#pragma mark - CoverManagerListener

- (void)coverManagerDidReset {
    [self throttledUpdate];
}

@end
