//
//  PlaylistGroup.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 15/03/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PlaylistDataModel.h"

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistGroup : NSObject {
@public DdbListviewRow_t head;
@public int head_idx;
@public int32_t height;
@public int32_t min_height;
@public int32_t num_items;
@public BOOL hasCachedImage;
@public NSImage *cachedImage;
}

@end

NS_ASSUME_NONNULL_END
