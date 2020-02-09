//
//  DesignableViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/9/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface DesignableViewController : NSViewController

@property (nonatomic) NSArray *subviewControllers;

- (NSDictionary *)serialize;

- (int)sendMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 NS_REQUIRES_SUPER;

- (void)cleanup;

@end

NS_ASSUME_NONNULL_END
