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

#import "SelectionPropertiesWidget.h"
#import "TrackPropertiesListViewController.h"
#import <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface SelectionPropertiesWidget()

@property (nonatomic) TrackPropertiesListViewController *trackPropertiesListViewController;

@end

@implementation SelectionPropertiesWidget

+ (NSString *)widgetType {
    return @"SelectionProperties";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.trackPropertiesListViewController = [TrackPropertiesListViewController new];

    NSView *view = self.trackPropertiesListViewController.view;

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:view];
    [view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    [self selectionDidChange];

    return self;
}

- (void)selectionDidChange {
    dispatch_async(dispatch_get_main_queue(), ^{
        ddb_playlist_t *playlist = deadbeef->plt_get_curr();
        if (playlist == NULL) {
            return;
        }

        [self.trackPropertiesListViewController loadFromPlaylist:playlist context:DDB_ACTION_CTX_SELECTION flags:TrackPropertiesListFlagMetadata|TrackPropertiesListFlagProperties];

        deadbeef->plt_unref(playlist);
    });
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
    case DB_EV_TRACKINFOCHANGED:
    case DB_EV_PLAYLISTCHANGED:
        if (p1 == DDB_PLAYLIST_CHANGE_CONTENT || p1 == DDB_PLAYLIST_CHANGE_SELECTION) {
            [self selectionDidChange];
        }
        break;
    case DB_EV_PLAYLISTSWITCHED:
        [self selectionDidChange];
        break;
    }
}

- (BOOL)makeFirstResponder {
    [self.trackPropertiesListViewController.view.window makeFirstResponder:self.trackPropertiesListViewController.view];
    return YES;
}

@end
