//
//  DdbSearchWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSearchWidget.h"
#import "DdbPlaylistViewController.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbSearchWidget

- (void)setDelegate:(id<DdbListviewDelegate>) delegate {
    _delegate = delegate;
    [_listview setDelegate:delegate];
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    return [(DdbPlaylistViewController *)_delegate handleListviewMessage:_listview id:_id ctx:ctx p1:p1 p2:p2];
}

@end
