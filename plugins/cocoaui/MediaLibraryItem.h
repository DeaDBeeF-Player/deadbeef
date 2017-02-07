//
//  MediaLibraryItem.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/5/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "../../deadbeef.h"
#include "../medialib/medialib.h"

@interface MediaLibraryItem : NSObject

+ (id)initTree:(ddb_medialib_list_t *)list;

- (id)initRoot:(ddb_medialib_list_t *)list;
- (id)initItemNode:(ddb_medialib_item_t *)item parent:(MediaLibraryItem *)parent;
- (id)initTrackNode:(DB_playItem_t *)data parent:(MediaLibraryItem *)parent;

- (NSUInteger)numberOfChildren;
- (MediaLibraryItem *)childAtIndex:(NSUInteger)index;

- (NSArray *)children;

- (NSString *)stringValue;

@end
