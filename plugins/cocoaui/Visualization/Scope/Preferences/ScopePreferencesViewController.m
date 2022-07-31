//
//  ScopePreferencesViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 14/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "ScopePreferencesViewController.h"
#import "ScopeSettings.h"
#import "VisualizationSettingsUtil.h"

@interface ScopePreferencesViewController ()
@property (weak) IBOutlet NSButton *useCustomColorButton;
@property (weak) IBOutlet NSButton *useCustomBackgroundColorButton;

@property (weak) IBOutlet NSColorWell *customColorWell;
@property (weak) IBOutlet NSColorWell *customBackgroundColorWell;

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
    else {
        self.customColorWell.color = VisualizationSettingsUtil.shared.defaultBaseColor;
    }
    if (self.settings.customBackgroundColor != nil) {
        self.customBackgroundColorWell.color = self.settings.customBackgroundColor;
    }
    else {
        self.customBackgroundColorWell.color = VisualizationSettingsUtil.shared.defaultBackgroundColor;
    }
    [self updateUseCustomColor:self.settings.useCustomColor];
    [self updateUseCustomBackgroundColor:self.settings.useCustomBackgroundColor];
}

- (void)updateUseCustomColor:(BOOL)enabled {
    self.customColorWell.enabled = enabled;
    self.useCustomColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)updateUseCustomBackgroundColor:(BOOL)enabled {
    self.customBackgroundColorWell.enabled = enabled;
    self.useCustomBackgroundColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
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

- (IBAction)useCustomBackgroundColorAction:(NSButton *)sender {
    self.settings.useCustomBackgroundColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomBackgroundColor:sender.state == NSControlStateValueOn];
}

- (IBAction)customColorWellAction:(NSColorWell *)sender {
    self.settings.customColor = sender.color;
}

- (IBAction)customBackgroundColorWellAction:(NSColorWell *)sender {
    self.settings.customBackgroundColor = sender.color;
}



@end
