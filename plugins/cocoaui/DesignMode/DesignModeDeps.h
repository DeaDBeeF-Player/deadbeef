//
//  DesignModeDeps.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 22/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface DesignModeDeps : NSObject<DesignModeDepsProtocol>

@property (nonatomic,class,readonly) DesignModeDeps *sharedInstance;
+ (void)cleanup;

@end

NS_ASSUME_NONNULL_END
