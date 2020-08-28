//
//  MedialibItemDragDropHolder.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "deadbeef.h"

NS_ASSUME_NONNULL_BEGIN

@interface MedialibItemDragDropHolder : NSObject<NSPasteboardReading, NSPasteboardWriting, NSSecureCoding>

@property (nullable,nonatomic,readonly) ddb_playItem_t *playItem;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithItem:(ddb_playItem_t * _Nullable)item NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
