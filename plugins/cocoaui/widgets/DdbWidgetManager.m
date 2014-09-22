//
//  DdbWidgetManager.m
//  deadbeef
//
//  Created by waker on 29/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidgetManager.h"

@implementation DdbWidgetManager

static DdbWidgetManager *_defaultWidgetManager = nil;

+ (DdbWidgetManager *)defaultWidgetManager {
    if (!_defaultWidgetManager) {
        _defaultWidgetManager = [[DdbWidgetManager alloc] init];
    }
    return _defaultWidgetManager;
}

- (DdbWidgetManager *)init {
    self = [super init];
    if (self) {
        _regWidgets = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)addWidget:(DdbWidget *)widget {
    [_regWidgets addObject:widget];
}

- (void)removeWidget:(DdbWidget *)widget {
    [_regWidgets removeObject:widget];
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [_regWidgets enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        [obj widgetMessage:_id ctx:ctx p1:p1 p2:p2];
    }];
    return 0;
}

@end
