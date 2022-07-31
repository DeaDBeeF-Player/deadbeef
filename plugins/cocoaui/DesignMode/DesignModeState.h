//
//  DesignModeState.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 21/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface DesignModeState : NSObject<DesignModeStateProtocol>

@property (nonatomic,class,readonly) DesignModeState *sharedInstance;
+ (void)freeSharedInstance;

@property (nonatomic) BOOL enabled;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps NS_DESIGNATED_INITIALIZER;
- (void)load;

@end

NS_ASSUME_NONNULL_END
