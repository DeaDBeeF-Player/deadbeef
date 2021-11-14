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

- (void)setSettings:(ScopeSettings *)settings {
    _settings = settings;
    if (settings.customColor != nil) {
        self.customColorWell.color = settings.customColor;
    }
    [self updateUseCustomColor:settings.useCustomColor];
}

- (void)updateUseCustomColor:(BOOL)enabled {
    self.customColorWell.enabled = enabled;
    self.useCustomColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (IBAction)doneButtonAction:(id)sender {
    [NSApp endSheet:self.view.window returnCode:NSModalResponseOK];
}

- (IBAction)useCustomColorButtonAction:(NSButton *)sender {
    self.settings.useCustomColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomColor:sender.state == NSControlStateValueOn];
}

- (IBAction)customColorWellAction:(NSColorWell *)sender {
    self.settings.customColor = sender.color;
}

@end
