//
//  DesignModeEventHandler.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface DesignModeEventHandler : NSObject

- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState NS_DESIGNATED_INITIALIZER;
- (BOOL)sendEvent:(NSEvent *)event;

@end

NS_ASSUME_NONNULL_END
