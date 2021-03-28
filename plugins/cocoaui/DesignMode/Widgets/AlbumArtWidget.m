//
//  AlbumArtWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/03/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "AlbumArtImageView.h"
#import "AlbumArtWidget.h"
#import "CoverManager.h"
#import "deadbeef.h"
#import "artwork.h"

extern DB_functions_t *deadbeef;

@interface AlbumArtWidget()

@property (nonatomic) NSImageView *imageView;
@property (nonatomic) ddb_playItem_t *track;
@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;

@end

@implementation AlbumArtWidget

+ (NSString *)widgetType {
    return @"AlbumArt";
}

static void
artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    AlbumArtWidget *self = (__bridge AlbumArtWidget *)user_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [CoverManager.defaultCoverManager resetCache];
        [self update];
    });
}

- (void)dealloc
{
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

    return self;
}

- (void)frameDidChange:(NSNotification *)notification {
    [self update];
}

typedef struct {
    void *widget;
    ddb_playItem_t *track;
    CGFloat albumArtSpaceWidth;
    BOOL ignoreResult;
} cover_avail_info_t;

static void coverAvailCallback (NSImage *image, void *user_data) {
    cover_avail_info_t *info = user_data;
    AlbumArtWidget *widget = (__bridge_transfer AlbumArtWidget *)info->widget;

    if (info->track == widget.track) {
        if (image != nil) {
            NSSize desiredSize = [CoverManager.defaultCoverManager artworkDesiredSizeForImageSize:image.size albumArtSpaceWidth:info->albumArtSpaceWidth];
            widget.imageView.image = [CoverManager.defaultCoverManager createCachedImage:image size:desiredSize];
        }
        else {
            widget.imageView.image = nil;
        }
    }
    deadbeef->pl_item_unref (info->track);
    free (info);
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

                deadbeef->pl_item_ref (it);

                cover_avail_info_t *info = calloc (sizeof (cover_avail_info_t), 1);
                info->widget = (__bridge_retained void *)self;
                info->track = it;
                info->albumArtSpaceWidth = self.imageView.frame.size.width;

                NSImage *image = [CoverManager.defaultCoverManager getCoverForTrack:it withCallbackWhenReady:coverAvailCallback withUserDataForCallback:info];

                if (image != nil) {
                    NSSize desiredSize = [CoverManager.defaultCoverManager artworkDesiredSizeForImageSize:image.size albumArtSpaceWidth:info->albumArtSpaceWidth];
                    self.imageView.image = [CoverManager.defaultCoverManager createCachedImage:image size:desiredSize];
                    deadbeef->pl_item_unref (it);
                    free (info);
                }
            }

            deadbeef->plt_unref (plt);
        }
    }
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
    case DB_EV_CURSOR_MOVED: {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self update];
        });
    }
        break;
    }
}

@end
