/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Oleksiy Yakovenko and other contributors

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

#import <Cocoa/Cocoa.h>
#import "MainWindowController.h"
#import "MediaLibraryManager.h"
#import "SearchWindowController.h"
#import "PreferencesWindowController.h"
#include <deadbeef/deadbeef.h>

#define MAX_COLUMNS 20

@interface AppDelegate : NSObject <NSApplicationDelegate>

- (int)ddb_message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@property (nonatomic) MainWindowController *mainWindow;

@property (nonatomic,readonly) MediaLibraryManager *mediaLibraryManager;

@property (unsafe_unretained) IBOutlet NSMenuItem *mainWindowToggleMenuItem;
@property (unsafe_unretained) IBOutlet NSMenuItem *logWindowToggleMenuItem;
@property (weak) IBOutlet NSMenuItem *equalizerWindowToggleMenuItem;


@property (unsafe_unretained) IBOutlet NSMenu *mainMenu;
@property (unsafe_unretained) IBOutlet NSMenu *dockMenu;


@property (unsafe_unretained) IBOutlet NSWindow *addFilesWindow;
@property (unsafe_unretained) IBOutlet NSTextField *addFilesLabel;
- (IBAction)addFilesCancel:(id)sender;

@property (unsafe_unretained) IBOutlet NSPanel *customSortPanel;
@property (unsafe_unretained) IBOutlet NSTextField *customSortEntry;
@property (unsafe_unretained) IBOutlet NSButton *customSortDescending;


- (IBAction)customSortCancelAction:(id)sender;
- (IBAction)customSortOKAction:(id)sender;

- (IBAction)toggleDescendingSortOrderAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSMenuItem *descendingSortMode;

- (IBAction)openPrefWindow:(id)sender;

// file menu
- (IBAction)openFilesAction:(id)sender;
- (IBAction)addFilesAction:(id)sender;
- (IBAction)addFoldersAction:(id)sender;
- (IBAction)addLocationAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSPanel *addLocationPanel;
- (IBAction)addLocationOKAction:(id)sender;
- (IBAction)addLocationCancelAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSTextField *addLocationTextField;

- (IBAction)newPlaylistAction:(id)sender;
- (IBAction)loadPlaylistAction:(id)sender;
- (IBAction)savePlaylistAction:(id)sender;


// edit menu
- (IBAction)clearAction:(id)sender;

- (IBAction)sortPlaylistByTitle:(id)sender;
- (IBAction)sortPlaylistByTrackNumber:(id)sender;
- (IBAction)sortPlaylistByAlbum:(id)sender;
- (IBAction)sortPlaylistByArtist:(id)sender;
- (IBAction)sortPlaylistByDate:(id)sender;
- (IBAction)sortPlaylistRandom:(id)sender;
- (IBAction)sortPlaylistCustom:(id)sender;

// playback menu
- (IBAction)previousAction:(id)sender;
- (IBAction)playAction:(id)sender;
- (IBAction)pauseAction:(id)sender;
- (IBAction)stopAction:(id)sender;
- (IBAction)nextAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSMenuItem *orderLinear;
@property (unsafe_unretained) IBOutlet NSMenuItem *orderRandom;
@property (unsafe_unretained) IBOutlet NSMenuItem *orderShuffle;
@property (unsafe_unretained) IBOutlet NSMenuItem *orderShuffleAlbums;
- (IBAction)orderLinearAction:(id)sender;
- (IBAction)orderRandomAction:(id)sender;
- (IBAction)orderShuffleAction:(id)sender;
- (IBAction)orderShuffleAlbumsAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSMenuItem *loopNone;
@property (unsafe_unretained) IBOutlet NSMenuItem *loopAll;
@property (unsafe_unretained) IBOutlet NSMenuItem *loopSingle;
- (IBAction)loopNoneAction:(id)sender;
- (IBAction)loopAllAction:(id)sender;
- (IBAction)loopSingleAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSMenuItem *cursorFollowsPlayback;
- (IBAction)cursorFollowsPlaybackAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSMenuItem *scrollFollowsPlayback;
- (IBAction)scrollFollowsPlaybackAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSMenuItem *stopAfterCurrent;
- (IBAction)stopAfterCurrentAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSMenuItem *stopAfterCurrentAlbum;
- (IBAction)stopAfterCurrentAlbumAction:(id)sender;

// window menu
- (IBAction)showMainWinAction:(id)sender;
- (IBAction)showLogWindowAction:(id)sender;

@end
