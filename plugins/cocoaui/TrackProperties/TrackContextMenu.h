//
//  TrackContextMenu.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/27/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "deadbeef.h"

NS_ASSUME_NONNULL_BEGIN

@protocol TrackContextMenuDelegate <NSMenuDelegate>

- (void)trackProperties;
- (void)playlistChanged;

@end

@interface TrackContextMenu : NSMenu

@property (nonatomic,weak) NSView *view; // the view to associate with this menu
- (void)update:(ddb_playlist_t *)playlist iter:(int)playlistIter;

@end

NS_ASSUME_NONNULL_END
