/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>

@interface ReplayGainScannerController : NSWindowController<NSWindowDelegate,NSTableViewDataSource>

@property (strong) IBOutlet NSPanel *scanProgressWindow;
@property (strong) IBOutlet NSPanel *updateTagsProgressWindow;

@property (unsafe_unretained) IBOutlet NSTableView *resultsTableView;
- (IBAction)updateFileTagsAction:(id)sender;
- (IBAction)resultsCancelAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSTextField *progressText;
@property (unsafe_unretained) IBOutlet NSTextField *statusLabel;
@property (unsafe_unretained) IBOutlet NSProgressIndicator *progressIndicator;
@property (unsafe_unretained) IBOutlet NSTextField *resultStatusLabel;

- (IBAction)progressCancelAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSTextField *updateTagsProgressText;
@property (unsafe_unretained) IBOutlet NSProgressIndicator *updateTagsProgressIndicator;
@property (unsafe_unretained) IBOutlet NSTextField *updateTagsStatusLabel;
- (IBAction)updateTagsCancelAction:(id)sender;

+ (ReplayGainScannerController *)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count;
+ (ReplayGainScannerController *)removeRgTagsFromTracks:(DB_playItem_t **)tracks count:(int)count;
+ (void)cleanup;
@end
