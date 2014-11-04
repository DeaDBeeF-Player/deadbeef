//
//  DdbTabStrip.h
//  deadbeef
//
//  Created by waker on 03/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidget.h"

@interface DdbTabStrip : DdbWidget

@property int hscrollpos;
@property int dragging;
@property int prepare;
@property int movepos;
@property int tab_clicked;
@property int scroll_direction;
@property NSPoint dragpt;
@property int prev_x;
@property int tab_moved;

@property NSImage *tabLeft;
@property NSImage *tabFill;
@property NSImage *tabRight;
@property NSImage *tabUnselLeft;
@property NSImage *tabUnselFill;
@property NSImage *tabUnselRight;
@property NSImage *tabBottomFill;

@end
