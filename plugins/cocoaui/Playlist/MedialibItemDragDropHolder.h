//
//  MedialibItemDragDropHolder.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>

@interface MedialibItemDragDropHolder : NSObject<NSPasteboardReading, NSPasteboardWriting, NSSecureCoding>

@property (nonatomic,readonly) ddb_playItem_t * _Nonnull * _Nullable items;
@property (nonatomic,readonly) NSInteger count;

- (instancetype _Nonnull)init NS_UNAVAILABLE;
- (instancetype _Nonnull)initWithItem:(ddb_playItem_t * _Nullable)item;
- (instancetype _Nonnull)initWithItems:(ddb_playItem_t * _Nonnull * _Nullable)items count:(NSInteger)count NS_DESIGNATED_INITIALIZER;

@end
