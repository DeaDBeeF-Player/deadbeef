//
//  DesignModeState.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface DesignModeState : NSObject<DesignModeStateProtocol>

@property (nonatomic,class,readonly) DesignModeState *sharedInstance;
@property (nonatomic) BOOL enabled;

@end

NS_ASSUME_NONNULL_END
