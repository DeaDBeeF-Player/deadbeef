/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#import "PlaylistView.h"
#import "DdbSeekBar.h"
#import "MainWindowSidebarViewController.h"

@protocol WidgetProtocol;

@interface MainWindowController : NSWindowController

@property (unsafe_unretained) IBOutlet NSTextField *statusBar;
@property (unsafe_unretained) IBOutlet DdbSeekBar *seekBar;
@property (unsafe_unretained) IBOutlet NSSlider *volumeBar;
@property (strong) IBOutlet MainWindowSidebarViewController *sidebarOutlineViewController;


- (IBAction)seekBarAction:(id)sender;
- (IBAction)volumeBarAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSSegmentedControl *buttonBar;

- (IBAction)tbClicked:(id)sender;

- (void)updateVolumeBar;
- (void)updateTitleBarConfig;
- (void)updateTitleBar;

- (void)cleanup;

- (void)message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

- (BOOL)setupInitialFirstResponder:(id<WidgetProtocol>)widget;

@end
