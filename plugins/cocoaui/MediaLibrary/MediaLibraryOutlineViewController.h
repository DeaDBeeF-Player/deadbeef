//
//  MediaLibraryOutlineViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>

struct ddb_cover_query_s;
struct ddb_cover_info_s;

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryOutlineViewController : NSObject<NSOutlineViewDataSource,NSOutlineViewDelegate>

- (instancetype)initWithOutlineView:(NSOutlineView *)outlineView NS_DESIGNATED_INITIALIZER;
- (void)coverGetCallbackWithQuery:(struct ddb_cover_query_s *)query coverInfo:(struct ddb_cover_info_s *)cover error:(int)error;

@end

NS_ASSUME_NONNULL_END
