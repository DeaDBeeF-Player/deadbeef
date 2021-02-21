//
//  DesignModeState.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeState.h"

@implementation DesignModeState

+ (DesignModeState *)sharedInstance {
    static DesignModeState *instance;

    if (instance == nil) {
        instance = [DesignModeState new];
    }

    return instance;
}

@end
