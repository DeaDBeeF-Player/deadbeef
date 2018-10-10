#import <Cocoa/Cocoa.h>
#include "../../deadbeef.h"

@protocol PluginConfigurationValueAccessor
- (NSString *)getValueForKey:(NSString *)key def:(NSString *)def;
- (void)setValueForKey:(NSString *)key value:(NSString *)value;
- (int)count;
- (NSString *)keyForIndex:(int)index;
@end

@interface PluginConfigurationValueAccessorConfig : NSObject<PluginConfigurationValueAccessor>
@end

@interface PluginConfigurationValueAccessorDSP : NSObject<PluginConfigurationValueAccessor>
- (id)initWithPresetManager:(id)presetMgr presetIndex:(int)presetIndex;
@end

@interface PluginConfigurationViewController : NSViewController

- (void)initPluginConfiguration:(const char *)config accessor:(NSObject<PluginConfigurationValueAccessor> *)accessor;
- (void)savePluginConfiguration;
- (void)resetPluginConfigToDefaults;

@end
