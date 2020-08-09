//
//  PlaylistLocalDragDropHolder.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "deadbeef.h"

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistLocalDragDropHolder : NSObject<NSPasteboardReading, NSPasteboardWriting, NSSecureCoding>

@property (nonatomic) NSInteger playlistIdx;
@property (nonatomic) NSArray *itemsIndices;
@property (nonatomic) BOOL isMedialib;

- (instancetype)init NS_UNAVAILABLE; 
- (instancetype)initWithSelectedItemsOfPlaylist:(ddb_playlist_t *)playlist;
- (instancetype)initWithMedialibItemIndex:(NSUInteger)index;

@end

NS_ASSUME_NONNULL_END
