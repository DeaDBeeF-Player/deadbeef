//
//  NSMenu+ActionItems.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/23/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>


#import <Cocoa/Cocoa.h>
#include "deadbeef.h"

NS_ASSUME_NONNULL_BEGIN

@interface NSMenu (ActionItems)

- (void)addActionItemsForContext:(ddb_action_context_t)context track:(nullable DB_playItem_t *)track filter:(BOOL(^)(DB_plugin_action_t *action))filter;

@end

NS_ASSUME_NONNULL_END
