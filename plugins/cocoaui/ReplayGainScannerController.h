//
//  ReplayGainScannerController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 01/10/16.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "../../deadbeef.h"

@interface ReplayGainScannerController : NSWindowController

@property (strong) IBOutlet NSWindow *resultsWindow;

@property (unsafe_unretained) IBOutlet NSTableView *resultsTableView;
- (IBAction)updateFileTagsAction:(id)sender;
- (IBAction)resultsCancelAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSTextField *progressText;
@property (unsafe_unretained) IBOutlet NSTextField *statusLabel;
@property (unsafe_unretained) IBOutlet NSProgressIndicator *progressIndicator;

- (IBAction)progressCancelAction:(id)sender;

+ (ReplayGainScannerController *)runScanner:(int)mode forTracks:(DB_playItem_t **)tracks count:(int)count;
@end
