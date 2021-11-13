//
//  VisBaseColorUtil.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 13/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "VisBaseColorUtil.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation VisBaseColorUtil

+ (instancetype)shared {
    static VisBaseColorUtil *instance;
    if (instance == nil) {
        instance = [VisBaseColorUtil new];
    }

    return instance;
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

    int override_vis_color = deadbeef->conf_get_int ("cocoaui.vis.override_base_color", 0);
    if (!override_vis_color) {
        return color;
    }

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

    return color;
}

@end
