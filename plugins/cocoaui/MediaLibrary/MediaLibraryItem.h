//
//  MediaLibraryItem.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/5/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "../../../deadbeef.h"
#include "../../medialib/medialib.h"

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryItem : NSObject

- (id)initWithItem:(ddb_medialib_item_t *)item NS_DESIGNATED_INITIALIZER;

@property (nonatomic,readonly) NSUInteger numberOfChildren;
- (MediaLibraryItem *)childAtIndex:(NSUInteger)index;

@property (nonatomic,readonly) NSArray *children;
@property (nonatomic,readonly) NSString *stringValue;

@property (nonatomic,readonly) ddb_playItem_t *playItem;

@property (nonatomic) NSImage *coverImage;

@property (nonatomic) BOOL coverObtained;

@end

NS_ASSUME_NONNULL_END
