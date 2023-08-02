//
//  VisualizationViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationViewController : NSViewController

// Override to draw the content
- (void)draw;
- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 NS_REQUIRES_SUPER;

@end

NS_ASSUME_NONNULL_END
