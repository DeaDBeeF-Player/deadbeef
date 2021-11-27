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

- (BOOL)addPluginActionItemsForSelectedTrack:(nullable ddb_playItem_t *)selected selectedCount:(int)selectedCount actionContext:(ddb_action_context_t)actionContext;

@end

NS_ASSUME_NONNULL_END
