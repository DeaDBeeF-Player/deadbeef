//
//  WidgetBase.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "WidgetBase.h"

@interface WidgetBase()

@end

@implementation WidgetBase

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _childWidgets = [NSMutableArray new];

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    for (id<WidgetProtocol> widget in self.childWidgets) {
        [widget message:_id ctx:ctx p1:p1 p2:p2];
    }
}

@end
