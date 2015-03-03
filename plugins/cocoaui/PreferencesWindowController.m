//
//  PreferencesWindowController.m
//  deadbeef
//
//  Created by waker on 18/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import "PreferencesWindowController.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface PreferencesWindowController ()

@end

@implementation PreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    [_toolbar setDelegate:(id<NSToolbarDelegate>)self];
    [_toolbar setSelectedItemIdentifier:@"Playback"];

    [self setInitialValues];

    [self switchToView:_playbackView];
}

- (void)setInitialValues {
    [_replaygain_mode selectItemAtIndex: deadbeef->conf_get_int ("replaygain_mode", 0)];
    [_replaygain_scale setState: deadbeef->conf_get_int ("replaygain_scale", 1) ? NSOnState : NSOffState];
}

- (NSArray *)toolbarSelectableItemIdentifiers: (NSToolbar *)toolbar;
{
    return [NSArray arrayWithObjects:@"Playback",
            @"DSP",
            @"GUI",
            @"Appearance",
            @"Network",
            @"Plugins",
            nil];
}

- (void)switchToView:(NSView *)view {
    [[self window] setContentView:nil];

    NSRect oldFrame = [[self window] frame];
    NSRect rc = [[self window] frameRectForContentRect:[view frame]];
    rc.origin.x = oldFrame.origin.x;
    rc.origin.y = oldFrame.origin.y + oldFrame.size.height - rc.size.height;
    [[self window] setContentView:view];
    [[self window] setFrame:rc display:YES animate:YES];
}

- (IBAction)playbackAction:(id)sender {
    [self switchToView:_playbackView];
}

- (IBAction)appearanceAction:(id)sender {
    [self switchToView:_appearanceView];
}

- (IBAction)dspAction:(id)sender {
    [self switchToView:_dspView];
}

- (IBAction)guiAction:(id)sender {
    [self switchToView:_guiView];
}

- (IBAction)networkAction:(id)sender {
    [self switchToView:_networkView];
}

- (IBAction)pluginsAction:(id)sender {
    [self switchToView:_pluginsView];
}

- (IBAction)dspAddAction:(id)sender {
}

- (IBAction)dspRemoveAction:(id)sender {
}

- (IBAction)dspConfigureAction:(id)sender {
}

- (IBAction)dspMoveUpAction:(id)sender {
}

- (IBAction)dspMoveDownAction:(id)sender {
}

- (IBAction)dspSaveAction:(id)sender {
}

- (IBAction)dspLoadAction:(id)sender {
}
- (IBAction)networkEditContentTypeMapping:(id)sender {
}
- (IBAction)pluginConfigure:(id)sender {
}

- (IBAction)pluginShowCopyright:(id)sender {
}

- (IBAction)pluginOpenWebsite:(id)sender {
}
@end
