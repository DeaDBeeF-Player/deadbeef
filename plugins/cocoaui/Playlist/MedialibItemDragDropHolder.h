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

@interface MedialibItemDragDropHolder : NSObject<NSPasteboardReading, NSPasteboardWriting>

- (instancetype)initWithItem:(ddb_playItem_t *)item NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
