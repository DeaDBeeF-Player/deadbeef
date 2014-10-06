//
//  DdbTabStrip.h
//  deadbeef
//
//  Created by waker on 03/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidget.h"

@interface DdbTabStrip : DdbWidget {
    int _hscrollpos;
    int _dragging;
    int _prepare;
    int _movepos;
    int _tab_clicked;
    int _scroll_direction;
    NSPoint _dragpt;
    int _prev_x;
    int _tab_moved;

    NSImage *_tabLeft;
    NSImage *_tabFill;
    NSImage *_tabRight;
    NSImage *_tabUnselLeft;
    NSImage *_tabUnselFill;
    NSImage *_tabUnselRight;
    NSImage *_tabBottomFill;
}
@end
