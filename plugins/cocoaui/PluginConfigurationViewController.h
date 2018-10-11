#import <Cocoa/Cocoa.h>
#include "../../deadbeef.h"

@protocol PluginConfigurationValueAccessor
@required
- (NSString *)getValueForKey:(NSString *)key def:(NSString *)def;
- (void)setValueForKey:(NSString *)key value:(NSString *)value;
@optional
- (int)count;
- (NSString *)keyForIndex:(int)index;
- (NSArray<NSString *> *)getItemTypes;
- (NSString *)getItemNameWithType:(NSString *)type;
- (void)addItemWithType:(NSString *)type;
@end

@interface PluginConfigurationValueAccessorConfig : NSObject<PluginConfigurationValueAccessor>
@end

@interface PluginConfigurationScriptableBackend : NSObject<PluginConfigurationValueAccessor>
// FIXME: can't import deadbeef-Swift.h here
- (id)initWithScriptable:(id)scriptable;
@end

@interface PluginConfigurationViewController : NSViewController

- (void)initPluginConfiguration:(const char *)config accessor:(id<PluginConfigurationValueAccessor>)accessor;
- (void)savePluginConfiguration;
- (void)resetPluginConfigToDefaults;

@end
