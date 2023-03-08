//
//  PlaylistLocalDragDropHolder.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/1/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistLocalDragDropHolder : NSObject<NSPasteboardReading, NSPasteboardWriting, NSSecureCoding>

@property (nonatomic) NSInteger playlistIdx;
@property (nonatomic) NSArray *itemsIndices;

- (instancetype)init NS_UNAVAILABLE; 
- (instancetype)initWithSelectedItemsOfPlaylist:(ddb_playlist_t *)playlist;

@end

NS_ASSUME_NONNULL_END
