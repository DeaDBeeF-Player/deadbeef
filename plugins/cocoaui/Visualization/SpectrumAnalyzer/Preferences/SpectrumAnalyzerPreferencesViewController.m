//
//  SpectrumAnalyzerPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "SpectrumAnalyzerPreferencesViewController.h"
#import "VisualizationSettingsUtil.h"
#import "SpectrumAnalyzerSettings.h"

@interface SpectrumAnalyzerPreferencesViewController ()
@property (weak) IBOutlet NSButton *useCustomPeakColorButton;
@property (weak) IBOutlet NSButton *useCustomBarColorButton;
@property (weak) IBOutlet NSButton *useCustomBackgroundColorButton;


@property (weak) IBOutlet NSColorWell *peakColorWell;
@property (weak) IBOutlet NSColorWell *barColorWell;
@property (weak) IBOutlet NSColorWell *backgroundColorWell;

@end

@implementation SpectrumAnalyzerPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    if (self.settings != nil) {
        [self updateFromSettings];
    }
}

- (void)setSettings:(SpectrumAnalyzerSettings *)settings {
    _settings = settings;
    [self updateFromSettings];
}

- (void)updateFromSettings {
    if (self.settings.customPeakColor != nil) {
        self.peakColorWell.color = self.settings.customPeakColor;
    }
    if (self.settings.customBarColor != nil) {
        self.barColorWell.color = self.settings.customBarColor;
    }
    if (self.settings.customBackgroundColor != nil) {
        self.backgroundColorWell.color = self.settings.customBackgroundColor;
    }

    [self updateUseCustomPeakColor:self.settings.useCustomPeakColor];
    [self updateUseCustomBarColor:self.settings.useCustomBarColor];
    [self updateUseCustomBackgroundColor:self.settings.useCustomBackgroundColor];
}

- (void)updateUseCustomPeakColor:(BOOL)enabled {
    self.peakColorWell.enabled = enabled;
    self.useCustomPeakColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)updateUseCustomBarColor:(BOOL)enabled {
    self.barColorWell.enabled = enabled;
    self.useCustomBarColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)updateUseCustomBackgroundColor:(BOOL)enabled {
    self.backgroundColorWell.enabled = enabled;
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

- (IBAction)useCustomPeakColorButtonAction:(NSButton *)sender {
    self.settings.useCustomPeakColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomPeakColor:sender.state == NSControlStateValueOn];
}

- (IBAction)useCustomBarColorButtonAction:(NSButton *)sender {
    self.settings.useCustomBarColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomBarColor:sender.state == NSControlStateValueOn];
}

- (IBAction)useCustomBackgroundButtonAction:(NSButton *)sender {
    self.settings.useCustomBackgroundColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomBackgroundColor:sender.state == NSControlStateValueOn];

}

- (IBAction)peakColorWellAction:(NSColorWell *)sender {
    self.settings.customPeakColor = sender.color;
}

- (IBAction)barColorWellAction:(NSColorWell *)sender {
    self.settings.customBarColor = sender.color;
}

- (IBAction)backgroundColorWellAction:(NSColorWell *)sender {
    self.settings.customBackgroundColor = sender.color;
}

@end
