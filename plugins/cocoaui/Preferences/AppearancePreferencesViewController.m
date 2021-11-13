//
//  AppearancePreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 13/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "AppearancePreferencesViewController.h"
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

    // fetch accent color as default!
    NSColor *color;
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        color = NSColor.controlAccentColor;
    }
#endif
    color = NSColor.alternateSelectedControlColor;

    char str[100];
    deadbeef->conf_get_str ("cocoaui.vis.base_color", "", str, sizeof (str));
    if (str[0]) {
        NSString *colorString = [NSString stringWithUTF8String:str];
        NSArray<NSString *> *componentStrings = [colorString componentsSeparatedByString:@" "];

        if (componentStrings.count == 4) {
            CGFloat components[4];
            for (int i = 0; i < 4; i++) {
                components[i] = componentStrings[i].doubleValue;
            }
            color = [NSColor colorWithColorSpace:NSColorSpace.sRGBColorSpace components:components count:4];
        }
    }
    self.visualizationColorWell.color = color;

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
    NSColor *color = [sender.color colorUsingColorSpace:NSColorSpace.sRGBColorSpace];

    CGFloat components[4];
    [color getComponents:components];

    NSString *colorString = [NSString stringWithFormat:@"%0.2lf %0.2lf %0.2f %0.2lf", components[0], components[1], components[2], components[3]];

    deadbeef->conf_set_str ("cocoaui.vis.base_color", colorString.UTF8String);

    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
