/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/


#import "AlbumArtWidgetViewController.h"
#import "AlbumArtImageView.h"
#import "CoverManager.h"
#import "DesignModeDefs.h"
#include <deadbeef/deadbeef.h>
#include "artwork.h"
#import "Weakify.h"

extern DB_functions_t *deadbeef;

@interface AlbumArtWidgetViewController () <CoverManagerListener, NSMenuDelegate>

@property (nonatomic) NSImageView *imageView;
@property (nonatomic) ddb_playItem_t *track;
@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;

@property (nonatomic) int64_t sourceId;

@property (nonatomic) dispatch_block_t throttleBlock;
@property (nonatomic) NSInteger requestIndex;

@property (nonatomic) NSMenuItem *playingTrackItem;
@property (nonatomic) NSMenuItem *selectedTrackItem;
@property (nonatomic) NSMenuItem *playingOrSelectedTrackItem;

@end

@implementation AlbumArtWidgetViewController

- (void)loadView {
    [super loadView];
    self.imageView = [AlbumArtImageView new];
    self.imageView.imageAlignment = NSImageAlignCenter;
    self.imageView.imageScaling = NSImageScaleProportionallyUpOrDown;

    self.view = self.imageView;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    _artwork_plugin->add_listener (artwork_listener, (__bridge void *)self);

    self.sourceId = _artwork_plugin->allocate_source_id();

    // create view

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(frameDidChange:) name:NSViewFrameDidChangeNotification object:self.imageView];

    [CoverManager.shared addListener:self];
    [self createContextMenu];
}

static void
artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    AlbumArtWidgetViewController *self = (__bridge AlbumArtWidgetViewController *)user_data;
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

    ddb_playItem_t *it = NULL;
    switch (self.displayMode) {
    case albumArtDisplayModePlayingOrSelected:
        {
            it = deadbeef->streamer_get_playing_track_safe ();
            if(it != NULL) {
                break;
            }
        }
        // intentional fallthrough otherwise
    case albumArtDisplayModeSelected:
        {
            int cursor = deadbeef->pl_get_cursor (PL_MAIN);
            if (cursor != -1) {
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt == NULL) {
                    break;
                }
                it = deadbeef->plt_get_item_for_idx (plt, cursor, PL_MAIN);
                deadbeef->plt_unref (plt);
                plt = NULL;
            }
        }
        break;
    case albumArtDisplayModePlaying:
        it = deadbeef->streamer_get_playing_track_safe ();
        break;
    }

    if (!it) {
        self.imageView.image = nil;
        return;
    }

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

- (void)refresh {
    weakify(self);
    dispatch_async(dispatch_get_main_queue(), ^{
        strongify(self);
        [self throttledUpdate];
    });
}

#pragma mark - Context Menu

- (void)createContextMenu {
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"Album Art Display Mode"];
    menu.font = [NSFont systemFontOfSize:NSFont.smallSystemFontSize];
    menu.autoenablesItems = NO;

    self.playingTrackItem = [menu addItemWithTitle:@"Playing Track"
                                            action:@selector(playingTrackAction:)
                                     keyEquivalent:@""];
    [self.playingTrackItem setTarget:self];

    self.selectedTrackItem = [menu addItemWithTitle:@"Selected Track"
                                             action:@selector(selectedTrackAction:)
                                      keyEquivalent:@""];
    [self.selectedTrackItem setTarget:self];

    self.playingOrSelectedTrackItem = [menu addItemWithTitle:@"Playing or Selected Track"
                                                      action:@selector(playingOrSelectedTrackAction:)
                                               keyEquivalent:@""];
    [self.playingOrSelectedTrackItem setTarget:self];

    menu.delegate = self;

    self.imageView.menu = menu;
}

- (void)playingTrackAction:(id)sender {
    self.displayMode = albumArtDisplayModePlaying;
    [self update];
    [self.delegate configurationDidChange];
}

- (void)selectedTrackAction:(id)sender {
    self.displayMode = albumArtDisplayModeSelected;
    [self update];
    [self.delegate configurationDidChange];
}

- (void)playingOrSelectedTrackAction:(id)sender {
    self.displayMode = albumArtDisplayModePlayingOrSelected;
    [self update];
    [self.delegate configurationDidChange];
}

#pragma mark - NSMenuDelegate

- (void)menuNeedsUpdate:(NSMenu *)menu{
    self.playingTrackItem.state = self.displayMode == albumArtDisplayModePlaying ? NSControlStateValueOn : NSControlStateValueOff;
    self.selectedTrackItem.state = self.displayMode == albumArtDisplayModeSelected ? NSControlStateValueOn : NSControlStateValueOff;
    self.playingOrSelectedTrackItem.state = self.displayMode == albumArtDisplayModePlayingOrSelected ? NSControlStateValueOn : NSControlStateValueOff;
}

#pragma mark - CoverManagerListener

- (void)coverManagerDidReset {
    [self throttledUpdate];
}



@end
