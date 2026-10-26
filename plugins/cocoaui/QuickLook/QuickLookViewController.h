//
//  QuickLookViewController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 10/26/26.
//  Copyright © 2026 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

/// Finder-like quicklook panel for the playlist: shows album art and basic track info for the cursor track.
/// The panel UI is created from code, and never takes keyboard focus.
@interface QuickLookViewController : NSViewController

@property (class, nonatomic, readonly) QuickLookViewController *shared;

@property (nonatomic, readonly) BOOL isVisible;

/// Show the panel for the track, or hide it if it's already visible.
/// @param origin Point in screen coordinates which the zoom animation starts from / ends at.
- (void)toggleForTrack:(nullable DB_playItem_t *)track zoomOrigin:(NSPoint)origin;

/// Update the contents of the panel, if it's visible. Does nothing otherwise.
/// @param origin Point in screen coordinates which the zoom out animation ends at.
- (void)updateWithTrack:(nullable DB_playItem_t *)track zoomOrigin:(NSPoint)origin;

@end

NS_ASSUME_NONNULL_END
