//
//  MediaLibraryCoverQueryData.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/14/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>

@class MediaLibraryItem;
@class MediaLibraryOutlineViewController;

NS_ASSUME_NONNULL_BEGIN

@interface MediaLibraryCoverQueryData : NSObject

@property (nonatomic) MediaLibraryItem *item;
@property (nonatomic) MediaLibraryOutlineViewController *viewController;

@end

NS_ASSUME_NONNULL_END
