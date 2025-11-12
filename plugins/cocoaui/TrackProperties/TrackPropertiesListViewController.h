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

#import <AppKit/AppKit.h>
#import "TrackPropertiesListItem.h"
#include <deadbeef/deadbeef.h>

// UI Interactions:
// * Display a list of metadata fields for a list of tracks
// * Display one or more categories, which can be editable or non-editable
// * Edit fields in-place for all tracks at once
// * Edit fields in tabular view for each track individually
// * Context menu with actions like copy, paste, delete, new

// Inputs:
// * A playlist, or a list of MediaLibraryItem
// * Action context: for building track list from specified playlist
//     * TODO: Wouldn't it be better to pass list of tracks directly?
// * List of categories to show (can be metadata, properties, or both)
// * Filter: which keys to exclude

// Outputs:
// * store: array of TrackPropertiesListItem for each unique key

@class MediaLibraryItem;

typedef NS_ENUM(NSUInteger, TrackPropertiesListFlag) {
    TrackPropertiesListFlagMetadata = 0x1,
    TrackPropertiesListFlagProperties = 0x2,
    TrackPropertiesListFlagEditable = 0x4,
    TrackPropertiesListFlagSectionHeaders = 0x8,
    TrackPropertiesListFlagSmallFont = 0x10,
};

@interface TrackPropertiesListViewController : NSViewController

@property (nonatomic) NSMutableArray<TrackPropertiesListItem *> *store;
@property (nonatomic) BOOL isModified;
@property (nonatomic,readonly) ddb_playItem_t **tracks;
@property (nonatomic,readonly) int numtracks;

- (void)loadFromPlaylist:(ddb_playlist_t *)playlist context:(ddb_action_context_t)context flags:(NSUInteger)flags;
- (void)loadFromMediaLibraryItems:(NSArray<MediaLibraryItem *> *)mediaLibraryItems flags:(NSUInteger)flags;

- (void)reloadData;

@end
