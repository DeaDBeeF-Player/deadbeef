//
//  PlaylistContextMenu.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "TrackContextMenu.h"

NS_ASSUME_NONNULL_BEGIN

@protocol RenamePlaylistViewControllerDelegate;
@protocol DeletePlaylistConfirmationControllerDelegate;

@interface PlaylistContextMenu : TrackContextMenu

@property (nonatomic,weak) id<RenamePlaylistViewControllerDelegate> renamePlaylistDelegate;
@property (nonatomic,weak) id<DeletePlaylistConfirmationControllerDelegate> deletePlaylistDelegate;
@property (nonatomic,weak) NSView *parentView;
@property (nonatomic) NSPoint clickPoint;

- (void)updateWithPlaylist:(ddb_playlist_t *)playlist;

@end

NS_ASSUME_NONNULL_END
