//
//  DesignModeState.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface DesignModeState : NSObject

@property (nonatomic,class,readonly) DesignModeState *sharedInstance;
@property (nonatomic,getter=isEnabled) BOOL enabled;

@end

NS_ASSUME_NONNULL_END
