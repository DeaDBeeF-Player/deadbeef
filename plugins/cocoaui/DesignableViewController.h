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

@end

NS_ASSUME_NONNULL_END
