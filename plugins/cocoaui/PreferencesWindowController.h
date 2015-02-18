//
//  PreferencesWindowController.h
//  deadbeef
//
//  Created by waker on 18/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PreferencesWindowController : NSWindowController
@property (strong) IBOutlet NSView *playbackView;
@property (strong) IBOutlet NSView *dspView;
@property (strong) IBOutlet NSView *guiView;
@property (strong) IBOutlet NSView *appearanceView;
@property (strong) IBOutlet NSView *networkView;
@property (strong) IBOutlet NSView *pluginsView;

- (IBAction)playbackAction:(id)sender;
- (IBAction)appearanceAction:(id)sender;
- (IBAction)dspAction:(id)sender;
- (IBAction)guiAction:(id)sender;
- (IBAction)networkAction:(id)sender;
- (IBAction)pluginsAction:(id)sender;


@end
