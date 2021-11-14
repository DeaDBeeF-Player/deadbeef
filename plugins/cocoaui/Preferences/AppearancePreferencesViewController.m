//
//  AppearancePreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 13/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "AppearancePreferencesViewController.h"
#import "VisualizationSettingsUtil.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface AppearancePreferencesViewController ()

@property (weak) IBOutlet NSButton *overrideVisualizationBaseColorButton;
@property (weak) IBOutlet NSColorWell *visualizationColorWell;


@end

@implementation AppearancePreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    int override_vis_color = deadbeef->conf_get_int ("cocoaui.vis.override_base_color", 0);

    self.visualizationColorWell.color = VisualizationSettingsUtil.shared.baseColor;

    [self updateOverrideVisualizationBaseColor:override_vis_color];
}

- (void)updateOverrideVisualizationBaseColor:(BOOL)enable {
    self.overrideVisualizationBaseColorButton.state = enable ? NSControlStateValueOn : NSControlStateValueOff;
    self.visualizationColorWell.enabled = enable;
}

- (IBAction)overrideVisualizationBaseColorButtonAction:(NSButton *)sender {
    int newValue = sender.state == NSControlStateValueOff ? 0 : 1;
    deadbeef->conf_set_int ("cocoaui.vis.override_base_color", newValue);
    [self updateOverrideVisualizationBaseColor:newValue];

    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)visualizationColorWellAction:(NSColorWell *)sender {
    VisualizationSettingsUtil.shared.baseColor = sender.color;
}

@end
