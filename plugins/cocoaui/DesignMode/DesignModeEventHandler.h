//
//  DesignModeEventHandler.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 21/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface DesignModeEventHandler : NSObject

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps NS_DESIGNATED_INITIALIZER;
- (BOOL)sendEvent:(NSEvent *)event;

@end

NS_ASSUME_NONNULL_END
