//
//  PreferencesPluginEntry.h
//  PreferencesPluginEntry
//
//  Created by Alexey Yakovenko on 19/07/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "deadbeef.h"

NS_ASSUME_NONNULL_BEGIN

@interface PreferencesPluginEntry : NSObject

@property DB_plugin_t *plugin;

@end

NS_ASSUME_NONNULL_END
