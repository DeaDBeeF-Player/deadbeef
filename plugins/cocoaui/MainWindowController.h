//
//  MainWindowController.h
//  deadbeef
//
//  Created by waker on 27/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "DdbTabStrip.h"
#import "DdbListview.h"

@interface MainWindowController : NSWindowController

@property (unsafe_unretained) IBOutlet DdbTabStrip *tabStrip;
@property (unsafe_unretained) IBOutlet NSTextField *statusBar;
@property (unsafe_unretained) IBOutlet NSSlider *seekBar;
- (IBAction)seekBarAction:(id)sender;

- (IBAction)tbClicked:(id)sender;
@property (strong) IBOutlet NSPanel *renamePlaylistWindow;
@property (unsafe_unretained) IBOutlet NSTextField *renamePlaylistTitle;
- (IBAction)renamePlaylistCancelAction:(id)sender;
- (IBAction)renamePlaylistOKAction:(id)sender;

- (IBAction)renamePlaylistAction:(id)sender;
@end
