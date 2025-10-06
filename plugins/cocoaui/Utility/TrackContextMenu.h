//
//  TrackContextMenu.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/27/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

@class TrackContextMenu;

@protocol TrackContextMenuDelegate <NSMenuDelegate>

- (void)trackContextMenuShowTrackProperties:(TrackContextMenu *)trackContextMenu;
- (void)trackContextMenuDidReloadMetadata:(TrackContextMenu *)trackContextMenu;
- (void)trackContextMenuDidDeleteFiles:(TrackContextMenu *)trackContextMenu cancelled:(BOOL)cancelled;

@end

@interface TrackContextMenu : NSMenu

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithTitle:(NSString *)title NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;

- (instancetype)initWithView:(NSView *)view NS_DESIGNATED_INITIALIZER;

- (void)update:(ddb_playlist_t * _Nullable)playlist actionContext:(ddb_action_context_t)actionContext;
- (void)update:(ddb_playlist_t *)playlist actionContext:(ddb_action_context_t)actionContext isMediaLib:(BOOL)isMediaLib;
- (void)updateWithTrackList:(ddb_playItem_t * _Nullable * _Nullable)tracks count:(NSUInteger)count playlist:(ddb_playlist_t * _Nullable)plt currentTrack:(ddb_playItem_t * _Nullable)currentTrack currentTrackIdx:(int)currentTrackIdx;

@end

NS_ASSUME_NONNULL_END
