#import <Cocoa/Cocoa.h>
#include "../../deadbeef.h"

@interface PluginConfigurationViewController : NSViewController

- (void)initPluginConfiguration:(const char *)config dsp:(ddb_dsp_context_t *)dsp;
- (void)savePluginConfiguration;
- (void)resetPluginConfigToDefaults;

@end
