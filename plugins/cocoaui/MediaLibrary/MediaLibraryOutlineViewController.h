//
//  MediaLibraryOutlineViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryOutlineViewController : NSObject<NSOutlineViewDataSource,NSOutlineViewDelegate>

- (instancetype)initWithOutlineView:(NSOutlineView *)outlineView NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
