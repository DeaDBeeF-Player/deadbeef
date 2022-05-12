//
//  VisualizationSettingsUtil.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 13/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationSettingsUtil.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation VisualizationSettingsUtil

+ (instancetype)shared {
    static VisualizationSettingsUtil *instance;
    if (instance == nil) {
        instance = [VisualizationSettingsUtil new];
    }

    return instance;
}

- (NSString *)stringForColor:(NSColor *)color {
    color = [color colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat components[4];
    [color getComponents:components];

    NSString *colorString = [NSString stringWithFormat:@"%0.3lf %0.3lf %0.3f %0.3lf", components[0], components[1], components[2], components[3]];
    return colorString;
}

- (NSColor *)colorForString:(NSString *)colorString {
    NSArray<NSString *> *componentStrings = [colorString componentsSeparatedByString:@" "];

    if (componentStrings.count == 4) {
        CGFloat components[4];
        for (int i = 0; i < 4; i++) {
            components[i] = componentStrings[i].doubleValue;
        }
        return [NSColor colorWithColorSpace:NSColorSpace.sRGBColorSpace components:components count:4];
    }

    return nil;
}

- (void)setColor:(NSColor *)color forKey:(NSString *)key {
    deadbeef->conf_set_str (key.UTF8String, [self stringForColor:color].UTF8String);

    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (NSColor *)colorForKey:(NSString *)key {
    char str[100];
    deadbeef->conf_get_str (key.UTF8String, "", str, sizeof (str));
    if (str[0]) {
        NSString *colorString = [NSString stringWithUTF8String:str];
        return [self colorForString:colorString];
    }

    return nil;
}

- (void)setBaseColor:(NSColor *)color {
    [self setColor:color forKey:@"cocoaui.vis.base_color"];
}

- (NSColor *)baseColor {
    // fetch accent color as default!
    NSColor *color;
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        color = NSColor.controlAccentColor;
    }
    else
#endif
    {
        color = NSColor.alternateSelectedControlColor;
    }

    NSString *key = @"cocoaui.vis.override_base_color";

    int override_vis_color = deadbeef->conf_get_int (key.UTF8String, 0);
    if (!override_vis_color) {
        return color;
    }

    NSColor *overrideColor = [self colorForKey:@"cocoaui.vis.base_color"];
    if (overrideColor != nil) {
        color = overrideColor;
    }
    return color;
}

- (NSColor *)backgroundColor {
    NSString *key = @"cocoaui.vis.override_background_color";

    int override_vis_color = deadbeef->conf_get_int (key.UTF8String, 0);
    if (!override_vis_color) {
        return NSColor.blackColor;
    }

    NSColor *color = NSColor.blackColor;
    NSColor *overrideColor = [self colorForKey:@"cocoaui.vis.background_color"];
    if (overrideColor != nil) {
        color = overrideColor;
    }
    return color;
}

- (void)setBackgroundColor:(NSColor *)color {
    [self setColor:color forKey:@"cocoaui.vis.background_color"];
}

- (void)setSpectrumAnalyzerPeakColor:(NSColor *)color {
    [self setColor:color forKey:@"cocoaui.vis.spectrum_peak_color"];
}

- (NSColor *)spectrumAnalyzerPeakColor {
    return [self colorForKey:@"cocoaui.vis.spectrum_peak_color"];
}

- (void)setSpectrumAnalyzerBarColor:(NSColor *)color {
    [self setColor:color forKey:@"cocoaui.vis.spectrum_bar_color"];
}

- (NSColor *)spectrumAnalyzerBarColor {
    return [self colorForKey:@"cocoaui.vis.spectrum_bar_color"];
}

@end
