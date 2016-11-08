//
//  PluginConfigurationViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/11/2016.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "../../deadbeef.h"

@interface PluginConfigurationViewController : NSViewController

- (void)initPluginConfiguration:(const char *)config dsp:(ddb_dsp_context_t *)dsp;
- (void)savePluginConfiguration;
- (void)resetPluginConfigToDefaults;

@end
