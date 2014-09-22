//
//  DdbWidgetManager.h
//  deadbeef
//
//  Created by waker on 29/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DdbWidget.h"

@interface DdbWidgetManager : NSObject {
    NSMutableArray *_regWidgets;
}

+ (DdbWidgetManager *)defaultWidgetManager;
- (void)addWidget:(DdbWidget *)widget;
- (void)removeWidget:(DdbWidget *)widget;
- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;
@end
