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

#import "AppearancePreferencesViewController.h"
#import "DspPreferencesViewController.h"
#import "GuiPreferencesWindowController.h"
#import "NetworkPreferencesViewController.h"
#import "PlaybackPreferencesViewController.h"
#import "PluginsPreferencesViewController.h"
#import "PreferencesWindowController.h"
#import "SoundPreferencesViewController.h"
#import "MediaLibraryPreferencesViewController.h"

@interface PreferencesWindowController ()

@property (unsafe_unretained) IBOutlet NSToolbar *toolbar;

@property (strong) IBOutlet DspPreferencesViewController *dspViewController;
@property (strong) IBOutlet GuiPreferencesWindowController *guiViewController;
@property (strong) IBOutlet SoundPreferencesViewController *soundViewController;
@property (strong) IBOutlet PlaybackPreferencesViewController *playbackViewController;
@property (strong) IBOutlet NetworkPreferencesViewController *networkViewController;
@property (strong) IBOutlet PluginsPreferencesViewController *pluginsViewController;
@property (strong) IBOutlet MediaLibraryPreferencesViewController *mediaLibraryPreferencesViewController;
@property (weak) IBOutlet NSView *appearancePaneContainerView;

@property (nonatomic) AppearancePreferencesViewController *appearancePreferencesViewController;

@property (nonatomic) NSString *initialTabIdentifier;


@end

@implementation PreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    // This doesn't work reliably when set in XIB
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
    if (@available(macOS 10.16, *)) {
        self.window.toolbarStyle = NSWindowToolbarStylePreference;
    }
#endif

    _toolbar.delegate = self;
    _toolbar.selectedItemIdentifier = @"Sound";


    BOOL enableMedialib = NO;

    enableMedialib = self.mediaLibraryPreferencesViewController.isAvailable;

    if (!enableMedialib) {
        [self.toolbar removeItemAtIndex:4];
    }

    self.appearancePreferencesViewController = [AppearancePreferencesViewController new];
    [self.appearancePaneContainerView addSubview:self.appearancePreferencesViewController.view];

    [self.appearancePreferencesViewController.view.leadingAnchor constraintEqualToAnchor:self.appearancePaneContainerView.leadingAnchor].active = YES;
    [self.appearancePreferencesViewController.view.trailingAnchor constraintEqualToAnchor:self.appearancePaneContainerView.trailingAnchor].active = YES;
    [self.appearancePreferencesViewController.view.topAnchor constraintEqualToAnchor:self.appearancePaneContainerView.topAnchor].active = YES;
    [self.appearancePreferencesViewController.view.bottomAnchor constraintEqualToAnchor:self.appearancePaneContainerView.bottomAnchor].active = YES;



    if (self.initialTabIdentifier) {
        _toolbar.selectedItemIdentifier = self.initialTabIdentifier;
        self.initialTabIdentifier = nil;
        [self switchToView:self.mediaLibraryPreferencesViewController.view];
    }
    else {
        [self switchToView:self.playbackViewController.view];
    }
}

- (NSArray *)toolbarSelectableItemIdentifiers: (NSToolbar *)toolbar {
    return @[@"Sound",
            @"Playback",
            @"DSP",
            @"GUI",
            @"Medialib",
            @"Network",
            @"Plugins"];
}

- (void)switchToView:(NSView *)view {
    self.window.contentView = nil;

    NSRect oldFrame = (self.window).frame;
    NSRect rc = [self.window frameRectForContentRect:view.frame];
    rc.origin.x = oldFrame.origin.x;
    rc.origin.y = oldFrame.origin.y + oldFrame.size.height - rc.size.height;
    self.window.contentView = view;
    [self.window setFrame:rc display:YES animate:YES];
}

- (IBAction)soundAction:(id)sender {
    [self switchToView:self.playbackViewController.view];
}

- (IBAction)playbackAction:(id)sender {
    [self switchToView:self.soundViewController.view];
}

- (IBAction)dspAction:(id)sender {
    [self switchToView:self.dspViewController.view];
}

- (IBAction)guiAction:(id)sender {
    [self switchToView:self.guiViewController.view];
}

- (IBAction)networkAction:(id)sender {
    [self switchToView:self.networkViewController.view];
}

- (IBAction)pluginsAction:(id)sender {
    [self switchToView:self.pluginsViewController.view];
}

- (IBAction)medialibAction:(id)sender {
    [self switchToView:self.mediaLibraryPreferencesViewController.view];
}

- (void)outputDeviceChanged {
    [self.soundViewController outputDeviceChanged];
}

- (void)switchToTab:(NSString *)identifier {
    if (!self.window) {
        self.initialTabIdentifier = identifier;
    }
    else {
        _toolbar.selectedItemIdentifier = identifier;
        [self switchToView:self.mediaLibraryPreferencesViewController.view];
    }
}

- (void)cancel:(id)sender {
    [self.window close];
}

@end
