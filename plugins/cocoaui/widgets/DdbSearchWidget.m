//
//  DdbSearchWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSearchWidget.h"

@implementation DdbSearchWidget

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


@end
