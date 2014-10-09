//
//  DdbPlaylistWidgetView.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 22/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbPlaylistWidget.h"
#import "DdbPlaylistViewController.h"
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbPlaylistWidget

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        NSRect listFrame = frame;
        listFrame.origin.x = 0;
        listFrame.origin.y = 0;
        _listview = [[DdbListview alloc] initWithFrame:listFrame];
        [_listview setNeedsDisplay:YES];
        [_listview setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMinYMargin|NSViewHeightSizable|NSViewMaxYMargin];
        [self addSubview:_listview];
        
    }
    return self;
}

- (void)setDelegate:(id<DdbListviewDelegate>)delegate {
    _delegate = delegate;
    [_listview setDelegate:(id<DdbListviewDelegate>)delegate];
}


- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    return [(DdbPlaylistViewController *)_delegate handleListviewMessage:_listview id:_id ctx:ctx p1:p1 p2:p2];
}

@end
