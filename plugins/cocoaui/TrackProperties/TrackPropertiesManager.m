/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2023 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#import "TrackPropertiesManager.h"
#import "TrackPropertiesWindowController.h"

extern DB_functions_t *deadbeef;

static TrackPropertiesManager *_shared;

@interface TrackPropertiesManager() <TrackPropertiesWindowControllerDelegate>

@property (nonatomic) TrackPropertiesWindowController *trkProperties;

@end

@implementation TrackPropertiesManager

+ (TrackPropertiesManager *)shared {
    if (_shared == nil) {
        _shared = [TrackPropertiesManager new];
    }
    return _shared;
}

+ (void)deinitializeSharedInstance {
    _shared = nil;
}

- (void)displayTrackProperties {
    if (!self.trkProperties) {
        self.trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
        self.trkProperties.delegate = self;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    [self.trkProperties setPlaylist:plt context:DDB_ACTION_CTX_SELECTION];
    deadbeef->plt_unref (plt);
    [self.trkProperties showWindow:self];
}

#pragma mark - TrackPropertiesWindowControllerDelegate

- (void)trackPropertiesWindowControllerDidUpdateTracks:(TrackPropertiesWindowController *)windowController {
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}


@end
