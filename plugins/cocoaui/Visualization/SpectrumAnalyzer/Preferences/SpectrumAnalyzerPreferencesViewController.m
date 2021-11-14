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

@property (weak) IBOutlet NSColorWell *peakColorWell;
@property (weak) IBOutlet NSColorWell *barColorWell;

@end

@implementation SpectrumAnalyzerPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

}

- (void)setSettings:(SpectrumAnalyzerSettings *)settings {
    _settings = settings;
    if (settings.customPeakColor != nil) {
        self.peakColorWell.color = settings.customPeakColor;
    }
    if (settings.customBarColor != nil) {
        self.barColorWell.color = settings.customBarColor;
    }
    [self updateUseCustomPeakColor:settings.useCustomPeakColor];
    [self updateUseCustomBarColor:settings.useCustomBarColor];
}

- (void)updateUseCustomPeakColor:(BOOL)enabled {
    self.peakColorWell.enabled = enabled;
    self.useCustomPeakColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)updateUseCustomBarColor:(BOOL)enabled {
    self.barColorWell.enabled = enabled;
    self.useCustomBarColorButton.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
}

- (IBAction)doneAction:(id)sender {
    [NSApp endSheet:self.view.window returnCode:NSModalResponseOK];
}

- (IBAction)useCustomPeakColorButtonAction:(NSButton *)sender {
    self.settings.useCustomPeakColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomPeakColor:sender.state == NSControlStateValueOn];
//    [self.delegate spectrumAnalyzerPreferencesUseCustomPeakColorDidChange:sender.state == NSControlStateValueOn];
}

- (IBAction)useCustomBarColorButtonAction:(NSButton *)sender {
    self.settings.useCustomBarColor = sender.state == NSControlStateValueOn;
    [self updateUseCustomBarColor:sender.state == NSControlStateValueOn];
//    [self.delegate spectrumAnalyzerPreferencesUseCustomBarColorDidChange:sender.state == NSControlStateValueOn];
}

- (IBAction)peakColorWellAction:(NSColorWell *)sender {
    self.settings.customPeakColor = sender.color;
//    [self.delegate spectrumAnalyzerPreferencesCustomPeakColorDidChange:sender.color];
}

- (IBAction)barColorWellAction:(NSColorWell *)sender {
    self.settings.customBarColor = sender.color;
//    [self.delegate spectrumAnalyzerPreferencesCustomBarColorDidChange:sender.color];
}

@end
