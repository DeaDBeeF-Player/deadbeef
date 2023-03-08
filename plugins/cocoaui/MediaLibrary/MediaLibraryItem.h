//
//  MediaLibraryItem.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 2/5/17.
//  Copyright © 2017 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <deadbeef/deadbeef.h>
#include "../../medialib/medialib.h"

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryItem : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithItem:(const ddb_medialib_item_t *)item;

@property (nonatomic,readonly) NSUInteger numberOfChildren;
- (MediaLibraryItem *)childAtIndex:(NSUInteger)index;

@property (nonatomic,readonly) NSArray<MediaLibraryItem *> *children;
@property (nonatomic,readonly) NSString *stringValue;

@property (nonatomic,readonly) ddb_playItem_t *playItem;

@property (nonatomic) NSImage *coverImage;

@property (nonatomic) BOOL coverObtained;

@property (nonatomic,readonly) const ddb_medialib_item_t *medialibItem;

@end

NS_ASSUME_NONNULL_END
