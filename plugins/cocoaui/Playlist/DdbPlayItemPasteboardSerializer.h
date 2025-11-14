//
//  DdbPlayItemPasteboardSerializer.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>

/// Serialize/deserialize an array of playItems as playlist.
/// The resulting items would not be the same memory locations after serialization,
/// since they go through playlist saving and loading procedure.
@interface DdbPlayItemPasteboardSerializer : NSObject<NSPasteboardReading, NSPasteboardWriting, NSSecureCoding>

- (instancetype _Nonnull)init NS_UNAVAILABLE;
- (instancetype _Nonnull)initWithItem:(ddb_playItem_t * _Nullable)item;
- (instancetype _Nonnull)initWithItems:(ddb_playItem_t * _Nonnull * _Nullable)items count:(NSInteger)count NS_DESIGNATED_INITIALIZER;

@property (nonatomic,readonly) ddb_playItem_t  * _Nullable * _Nonnull items;
@property (nonatomic,readonly) NSInteger count;

@end
