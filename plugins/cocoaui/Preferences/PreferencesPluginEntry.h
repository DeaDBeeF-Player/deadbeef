//
//  PreferencesPluginEntry.h
//  PreferencesPluginEntry
//
//  Created by Oleksiy Yakovenko on 19/07/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

@interface PreferencesPluginEntry : NSObject

@property DB_plugin_t *plugin;

@end

NS_ASSUME_NONNULL_END
