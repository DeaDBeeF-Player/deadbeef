//
//  MediaLibraryOutlineViewController.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>

struct ddb_cover_query_s;
struct ddb_cover_info_s;

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryOutlineViewController : NSObject

- (instancetype)initWithOutlineView:(NSOutlineView *)outlineView searchField:(NSSearchField *)searchField NS_DESIGNATED_INITIALIZER;
- (int)widgetMessage:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@end

NS_ASSUME_NONNULL_END
