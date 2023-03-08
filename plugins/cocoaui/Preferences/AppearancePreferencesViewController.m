//
//  AppearancePreferencesViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 13/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "AppearancePreferencesViewController.h"
#import "VisualizationSettingsUtil.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface AppearancePreferencesViewController ()

@property (weak) IBOutlet NSButton *overrideColorButton;
@property (weak) IBOutlet NSColorWell *colorWell;

@property (weak) IBOutlet NSButton *overrideBackgroundColorButton;
@property (weak) IBOutlet NSColorWell *backgroundColorWell;

@end

@implementation AppearancePreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.colorWell.color = VisualizationSettingsUtil.shared.baseColor;
    self.backgroundColorWell.color = VisualizationSettingsUtil.shared.backgroundColor;

    [self updateOverrideColor:deadbeef->conf_get_int ("cocoaui.vis.override_base_color", 0)];
    [self updateOverrideBackgroundColor:deadbeef->conf_get_int ("cocoaui.vis.override_background_color", 0)];
}

- (void)updateOverrideColor:(BOOL)enable {
    self.overrideColorButton.state = enable ? NSControlStateValueOn : NSControlStateValueOff;
    self.colorWell.enabled = enable;
}

- (void)updateOverrideBackgroundColor:(BOOL)enable {
    self.overrideBackgroundColorButton.state = enable ? NSControlStateValueOn : NSControlStateValueOff;
    self.backgroundColorWell.enabled = enable;
}

- (IBAction)overrideBaseColorButtonAction:(NSButton *)sender {
    int newValue = sender.state == NSControlStateValueOff ? 0 : 1;
    deadbeef->conf_set_int ("cocoaui.vis.override_base_color", newValue);
    [self updateOverrideColor:newValue];

    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)colorWellAction:(NSColorWell *)sender {
    VisualizationSettingsUtil.shared.baseColor = sender.color;
}

- (IBAction)overrideBackgroundColorButtonAction:(NSButton *)sender {
    int newValue = sender.state == NSControlStateValueOff ? 0 : 1;
    deadbeef->conf_set_int ("cocoaui.vis.override_background_color", newValue);
    [self updateOverrideBackgroundColor:newValue];

    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)backgroundColorWellAction:(NSColorWell *)sender {
    VisualizationSettingsUtil.shared.backgroundColor = sender.color;
}

@end
