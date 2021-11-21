//
//  PlaylistBrowserViewController.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistBrowserViewController : NSViewController

- (void)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@end

NS_ASSUME_NONNULL_END
