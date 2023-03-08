//
//  PluginActionMenuItem.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/23/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

@interface PluginActionMenuItem : NSMenuItem

@property (nonatomic,unsafe_unretained) DB_plugin_action_t *pluginAction;
@property (nonatomic) ddb_action_context_t pluginActionContext;

@end

NS_ASSUME_NONNULL_END
