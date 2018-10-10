#import <Cocoa/Cocoa.h>
#include "../../deadbeef.h"

@protocol PluginConfigurationValueAccessor
- (NSString *)getValueForKey:(NSString *)key def:(NSString *)def;
- (void)setValueForKey:(NSString *)key value:(NSString *)value;
- (int)count;
- (NSString *)keyForIndex:(int)index;
- (NSArray<NSString *> *)getItemTypes;
- (NSString *)getItemNameWithType:(NSString *)type;
- (void)addItemWithType:(NSString *)type;
@end

@interface PluginConfigurationValueAccessorConfig : NSObject<PluginConfigurationValueAccessor>
@end

@interface PluginConfigurationValueAccessorPreset : NSObject<PluginConfigurationValueAccessor>
- (id)initWithPresetManager:(id)presetMgr presetIndex:(int)presetIndex;
@end

@interface PluginConfigurationViewController : NSViewController

- (void)initPluginConfiguration:(const char *)config accessor:(id<PluginConfigurationValueAccessor>)accessor;
- (void)savePluginConfiguration;
- (void)resetPluginConfigToDefaults;

@end
