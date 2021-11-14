//
//  ScopePreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "ScopePreferencesViewController.h"
#import "ScopeSettings.h"

@interface ScopePreferencesViewController ()
@property (weak) IBOutlet NSButton *useCustomColorButton;
@property (weak) IBOutlet NSColorWell *customColorWell;

@end

@implementation ScopePreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    if (self.settings != nil) {
        [self updateFromSettings];
    }
}

- (void)setSettings:(ScopeSettings *)settings {
    _settings = settings;
    [self updateFromSettings];
}

- (void)updateFromSettings {
    if (self.settings.customColor != nil) {
        self.customColorWell.color = self.settings.customColor;
    }
    [self updateUseCustomColor:self.settings.useCustomColor];
}

- (void)updateUseCustomColor:(BOOL)enabled {
    self.customColorWell.enabled = enabled;
    self.useCustomColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (IBAction)doneButtonAction:(id)sender {
    if (self.popover == nil) {
        [NSApp endSheet:self.view.window returnCode:NSModalResponseOK];
    }
    else {
        [self.popover close];
    }
}

- (IBAction)useCustomColorButtonAction:(NSButton *)sender {
    self.settings.useCustomColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomColor:sender.state == NSControlStateValueOn];
}

- (IBAction)customColorWellAction:(NSColorWell *)sender {
    self.settings.customColor = sender.color;
}

@end
