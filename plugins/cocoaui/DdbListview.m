//
//  DdbListview.m
//  deadbeef
//
//  Created by waker on 13/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbListview.h"

int headerheight = 17;
int rowheight = 19;

@interface DdbListHeaderView : NSView {
    DdbListview *listview;
    int _orig_col_width;
    int _drag_col_pos;
    int _drag_delta;
    DdbListviewCol_t _dragging;
    DdbListviewCol_t _sizing;
    NSPoint _dragPt;
}
- (void)setListView:(DdbListview *)lv;
@end

@implementation DdbListHeaderView
- (DdbListHeaderView *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    _dragging = -1;
    _sizing = -1;
    return self;
}

- (void)setListView:(DdbListview *)lv {
    listview = lv;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

//    [self updateCursorRects];

    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    NSRect rect = [self bounds];
    [[NSColor lightGrayColor] set];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x, rect.origin.y+rect.size.height-1,rect.size.width,1)];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x, 0,rect.size.width,1)];

    NSGradient *gr = [[NSGradient alloc] initWithColorsAndLocations:
                      [NSColor whiteColor], 0.f,
                      [NSColor colorWithCalibratedWhite:0.9f alpha:1.f], 0.55f,
                      [NSColor colorWithCalibratedWhite:0.88f alpha:1.f], 0.55f,
                      [NSColor colorWithCalibratedWhite:0.88f alpha:1.f], 0.89f,
                      [NSColor colorWithCalibratedWhite:0.9f alpha:1.f], 0.90f,
                      [NSColor whiteColor], 1.f, nil];

    [gr drawInRect:NSMakeRect(rect.origin.x, rect.origin.y+1,rect.size.width,rect.size.height-2) angle:270];

    id <DdbListviewDelegate> delegate = [listview delegate];

    int x = -rc.origin.x;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        if (_dragging != col) {
            if (CGRectIntersectsRect(dirtyRect, NSMakeRect(x, 0, w, [self frame].size.height))) {
                [delegate drawColumnHeader:col inRect:NSMakeRect(x, 0, w, [self frame].size.height)];
            }
        }
        x += w;
    }

    x = -rc.origin.x;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        int cx = x;
        if (_dragging == col) {
            cx = _drag_col_pos + _drag_delta;
            NSRect colRect = NSMakeRect(cx, 1, w, [self frame].size.height-2);
            if (CGRectIntersectsRect(dirtyRect, colRect)) {
                [gr drawInRect:colRect angle:90];
                [delegate drawColumnHeader:col inRect:colRect];
            }
        }
        x += w;
    }

}

- (void)updateCursorRects {
    [self resetCursorRects];

    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    int x = -rc.origin.x;
    id <DdbListviewDelegate> delegate = [listview delegate];

    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        x += w;

        [self addCursorRect:NSMakeRect(x-3, 0, 6, [self bounds].size.height) cursor:[NSCursor resizeLeftRightCursor]];

    }
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    NSPoint convPt = [self convertPoint:[theEvent locationInWindow] fromView:nil];

    int x = -rc.origin.x;
    id <DdbListviewDelegate> delegate = [listview delegate];

    _dragging = [delegate invalidColumn];
    _sizing = [delegate invalidColumn];

    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        if (CGRectContainsPoint(NSMakeRect(x+3, 0, w-6, [self bounds].size.height), convPt)) {
            _drag_delta = 0;
            _dragging = col;
            _dragPt = convPt;
            _drag_col_pos = x;
            [listview setNeedsDisplay:YES];
            break;
        }

        x += w;

        if (CGRectContainsPoint (NSMakeRect(x-3, 0, 6, [self bounds].size.height), convPt)) {
            _sizing = col;
            _dragPt = convPt;
            _orig_col_width = [delegate columnWidth:col];
            break;
        }
    }
}

- (void)mouseUp:(NSEvent *)theEvent {
    id <DdbListviewDelegate> delegate = [listview delegate];
    if (_dragging != [delegate invalidColumn] || _sizing != [delegate invalidColumn]) {
        [delegate columnsChanged];
        [listview updateContentFrame];
        _dragging = [delegate invalidColumn];
        _sizing = [delegate invalidColumn];
        [listview setNeedsDisplay:YES];
    }
}

- (void)mouseDragged:(NSEvent *)theEvent {
    NSPoint convPt = [self convertPoint:[theEvent locationInWindow] fromView:nil];

    id <DdbListviewDelegate> delegate = [listview delegate];
    if (_sizing != [delegate invalidColumn]) {
        int dx = convPt.x - _dragPt.x;

        int w = _orig_col_width + dx;
        if (w < 10) {
            w = 10;
        }
        if ([delegate columnWidth:_sizing] != w) {
            [delegate setColumnWidth:w forColumn:_sizing];
            [listview setNeedsDisplay:YES];
        }
    }
    else if (_dragging != [delegate invalidColumn]) {
        _drag_delta = convPt.x - _dragPt.x;

        NSScrollView *sv = [listview.contentView enclosingScrollView];
        NSRect rc = [sv documentVisibleRect];

        int x = -rc.origin.x;

        DdbListviewCol_t inspos = [delegate invalidColumn];
        int x1 = -1, x2 = -1;
        for (DdbListviewCol_t cc = [delegate firstColumn]; cc != [delegate invalidColumn]; cc = [delegate nextColumn:cc]) {
            if (x < convPt.x && x + [delegate columnWidth:_dragging] > convPt.x) {
                inspos = cc;
                x1 = x;
            }
            else if (cc == _dragging) {
                x2 = x;
            }
            x += [delegate columnWidth:cc];
        }

        if (inspos != [delegate invalidColumn] && inspos != _dragging) {
            [delegate moveColumn:_dragging to:inspos];
            _dragging = inspos;
            [listview reloadData];
        }
        [listview setNeedsDisplay:YES];
    }
}
@end

@interface DdbListContentView : NSView {
    DdbListview *listview;
}
- (void)setListView:(DdbListview *)lv;
@end

@implementation DdbListContentView
- (DdbListContentView *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    return self;
}

- (void)setListView:(DdbListview *)lv {
    listview = lv;
}

- (void)drawListView:(NSRect)dirtyRect {
    int idx = 0;
    int abs_idx = 0;

    id<DdbListviewDelegate> delegate = listview.delegate;

    [delegate lock];

    [listview groupCheck];

    DdbListviewGroup_t *grp = [listview groups];

    int grp_y = 0;
    int grp_next_y = 0;
    DdbListviewGroup_t *pinned_grp = NULL;

    while (grp && grp_y + grp->height < dirtyRect.origin.y) {
        if (grp_y < 0 && grp_y + grp->height >= 0) {
            pinned_grp = grp;
            grp->pinned = 1;
        }
        grp_y += grp->height;
        idx += grp->num_items + 1;
        abs_idx += grp->num_items;
        grp = grp->next;
    }

    if (grp && !pinned_grp && grp_y < 0) {
        grp->pinned = 1;
        pinned_grp = grp;
    }
    else if (grp && pinned_grp && pinned_grp->next == grp) {
        grp->pinned = 2;
    }

    int ii = 0;
    while (grp && grp_y < dirtyRect.origin.y + dirtyRect.size.height) {
        DdbListviewRow_t it = grp->head;
        int grp_height = [listview grouptitle_height] + grp->num_items * rowheight;
        int grp_height_total = grp->height;

        if (grp_y >= dirtyRect.origin.y + dirtyRect.size.height) {
            break;
        }
        [listview.delegate refRow:it];

        grp_next_y = grp_y + grp_height_total;
        for (int i = 0; i < grp->num_items; i++) {
            ii++;
            int grp_row_y = grp_y + [listview grouptitle_height] + i * rowheight;
            if (grp_row_y >= dirtyRect.origin.y + dirtyRect.size.height) {
                break;
            }
            if (grp_y + [listview grouptitle_height] + (i+1) * rowheight >= dirtyRect.origin.y
                && grp_row_y < dirtyRect.origin.y + dirtyRect.size.height) {

                NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:ii % 2];
                [clr set];
                [NSBezierPath fillRect:NSMakeRect(dirtyRect.origin.x, grp_row_y, dirtyRect.size.width, rowheight)];

                int x = 0;
                for (DdbListviewCol_t col = [listview.delegate firstColumn]; col != [listview.delegate invalidColumn]; col = [listview.delegate nextColumn:col]) {
                    int w = [listview.delegate columnWidth:col];
                    if (CGRectIntersectsRect(dirtyRect, NSMakeRect(x, grp_row_y, w, rowheight))) {
                        [listview.delegate drawCell:it forColumn:col inRect:NSMakeRect(x, grp_row_y, w, rowheight-1) focused:YES];
                    }
                    x += w;
                }

                if (x < dirtyRect.size.width) {
                    [listview.delegate drawCell:it forColumn:[delegate invalidColumn] inRect:NSMakeRect(x, grp_row_y, dirtyRect.size.width-x, rowheight-1) focused:YES];
                }
            }
            DdbListviewRow_t next = [listview.delegate nextRow:it];
            [listview.delegate unrefRow:it];
            it = next;
            if (!it) {
                break; // sanity check, in case groups were not rebuilt yet
            }
        }

        idx += grp->num_items + 1;
        abs_idx += grp->num_items;

        int filler = grp_height_total - (grp_height);
        if (filler > 0) {
            // fill with background color: x, grp_y - listview->scrollpos + grp_height, w, filler
        }

        //ddb_listview_list_render_album_art (listview, cr, it, grp->head, 0, grp->height, grp->pinned, grp_next_y - listview->scrollpos, -listview->hscrollpos, grp_y + listview->grouptitle_height - listview->scrollpos, listview->totalwidth, grp_height_total);

        if (grp->pinned == 1 && [listview groups_pinned] && dirtyRect.origin.y <= 0) {
            // draw pinned group title
            int pushback = 0;
            if (grp_next_y <= [listview grouptitle_height]) {
                pushback = [listview grouptitle_height] - grp_next_y;
            }

            //ddb_listview_list_render_row_background (listview, cr, NULL, 1, 0, -
              //                                                    listview->hscrollpos, y - pushback, listview->totalwidth, listview->grouptitle_height);
            if ([listview grouptitle_height] > 0) {
                [delegate drawGroupTitle:grp->head inRect:NSMakeRect(0, dirtyRect.origin.y - pushback, [self frame].size.width, rowheight-1)];
            }
        }
        else if (grp_y + [listview grouptitle_height] >= dirtyRect.origin.y && grp_y < dirtyRect.origin.y + dirtyRect.size.height) {
            //ddb_listview_list_render_row_background (listview, cr, NULL, 1, 0, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            if ([listview grouptitle_height] > 0) {
                [delegate drawGroupTitle:grp->head inRect:NSMakeRect(0, grp_y, [self frame].size.width, rowheight-1)];
                //listview->binding->draw_group_title (listview, cr, grp->head, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            }
        }

        if (it) {
            [listview.delegate unrefRow:it];
        }
        grp_y += grp_height_total;
        if (grp->pinned == 1) {
            grp = grp->next;
            if (grp) {
                grp->pinned = 2;
            }
        }
        else {
            grp = grp->next;
            if (grp) {
                grp->pinned = 0;
            }
        }
    }

    if (grp_y < dirtyRect.origin.y + dirtyRect.size.height) {
        int hh = dirtyRect.origin.y + dirtyRect.size.height - grp_y;
        //cairo_rectangle (cr, x, grp_y - listview->scrollpos, w, hh);
    }
    [delegate unlock];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    // we always need to draw the list in the entire visible area,
    // so we get the full size from scrollview, and patch the clip rect
    [NSGraphicsContext saveGraphicsState];

    [self drawListView:dirtyRect];

    // draw rows below the real list
    if ([listview fullheight] < dirtyRect.origin.y + dirtyRect.size.height) {
        int y = [listview fullheight];
        int ii = [listview.delegate rowCount]+1;
        while (y + rowheight >= dirtyRect.origin.y && y < dirtyRect.size.height) {
            NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:ii % 2];
            [clr set];
            [NSBezierPath fillRect:NSMakeRect(dirtyRect.origin.x, y, dirtyRect.size.width, rowheight)];
            y += rowheight;
            ii++;
        }
    }

    [NSGraphicsContext restoreGraphicsState];
}

- (BOOL)isFlipped {
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    [listview groupCheck];

    id<DdbListviewDelegate> delegate = listview.delegate;

    [delegate lock];

    if (![delegate rowCount]) {
        [delegate unlock];
        return;
    }

    DdbListviewGroup_t *grp;
    int grp_index;
    int sel;
    NSPoint convPt = [self convertPoint:[event locationInWindow] fromView:nil];
    listview.lastpos = convPt;

    if (-1 == [listview pickPoint:convPt.y group:&grp groupIndex:&grp_index index:&sel]) {
        [delegate unlock];
        return;
    }

    int cursor = [delegate cursor];

    if (event.clickCount == 2) {
        if (sel != -1 && cursor != -1) {
            [delegate activate:cursor];
        }
        [delegate unlock];
        return;
    }

    int prev = cursor;
    if (sel != -1) {
        // if (rowspan) {
        //     sel -= grp_index;
        // }

        [delegate setCursor:sel];
        DdbListviewRow_t it = [delegate rowForIndex:sel];
        if (it) {
            [listview drawRow:sel];
            [delegate unrefRow:it];
        }
        listview.shift_sel_anchor = [delegate cursor];
    }

    // single selection
    if (0 == (event.modifierFlags & (NSCommandKeyMask|NSShiftKeyMask))) {
        [listview clickSelection:convPt grp:grp grp_index:grp_index sel:sel dnd:YES button:1];
    }
    else if (event.modifierFlags & NSCommandKeyMask) {
        // toggle selection
        if (sel != -1) {
            DdbListviewRow_t it = [delegate rowForIndex:sel];
            if (it != [delegate invalidRow]) {
                [delegate selectRow:it withState:![delegate rowSelected:it]];
                [listview drawRow:sel];
                [delegate selectionChanged:it];
                [delegate unrefRow:it];
            }
        }
    }
    else if (event.modifierFlags & NSShiftKeyMask) {
        // select range
        int cursor = sel;
        if (cursor == -1) {
            // find group
            DdbListviewGroup_t *g = listview.groups;
            int idx = 0;
            while (g) {
                if (g == grp) {
                    cursor = idx - 1;
                    break;
                }
                idx += g->num_items;
                g = g->next;
            }
        }
#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))
        int start = min (prev, cursor);
        int end = max (prev, cursor);
        int idx = 0;
        for (DdbListviewRow_t it = [delegate firstRow]; it != [delegate invalidRow]; idx++) {
            if (idx >= start && idx <= end) {
                if (![delegate rowSelected:it]) {
                    [delegate selectRow:it withState:YES];
                    [listview drawRow:idx];
                    [delegate selectionChanged:it];
                }
            }
            else {
                if ([delegate rowSelected:it]) {
                    [delegate selectRow:it withState:NO];
                    [listview drawRow:idx];
                    [delegate selectionChanged:it];
                }
            }
            DdbListviewRow_t next = [delegate nextRow:it];
            [delegate unrefRow:it];
            it = next;
        }
    }
    cursor = [delegate cursor];
    if (cursor != -1 && sel == -1) {
        [listview drawRow:cursor];
    }
    if (prev != -1 && prev != cursor) {
        [listview drawRow:prev];
    }

    [delegate unlock];
}

- (void)mouseUp:(NSEvent *)event
{
    [listview listMouseUp:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    NSPoint dragLocation;
    dragLocation=[self convertPoint:[event locationInWindow]
                           fromView:nil];

    [listview listMouseDragged:event];

    // support automatic scrolling during a drag
    // by calling NSView's autoscroll: method
    [self autoscroll:event];

    // act on the drag as appropriate to the application
}

@end

@implementation DdbListview

@synthesize delegate;
@synthesize headerView;
@synthesize contentView;

- (DdbListview *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    if (self) {
        groups_build_idx = -1;
        DdbListHeaderView *thv = [[DdbListHeaderView alloc] initWithFrame:NSMakeRect(0, 0, rect.size.width, headerheight)];
        [thv setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMaxYMargin];
        [self addSubview:thv];
        [thv setListView:self];
        headerView = thv;

        NSScrollView *sv = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, headerheight, rect.size.width, rect.size.height-headerheight)];
        [self addSubview:sv];

        NSSize size = [sv contentSize];
        NSRect lcvrect = NSMakeRect(0, 0, size.width, size.height-16);
        DdbListContentView *lcv = [[DdbListContentView alloc] initWithFrame:lcvrect];
        [lcv setAutoresizingMask:NSViewWidthSizable];
        [lcv setListView:self];
        contentView = lcv;

        [sv setDocumentView:lcv];

        [sv setHasVerticalScroller:YES];
        [sv setHasHorizontalScroller:YES];
        [sv setAutohidesScrollers:YES];
        [sv setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewHeightSizable|NSViewMaxYMargin];
        [sv.contentView setCopiesOnScroll:NO];

        NSView *synchronizedContentView = [sv contentView];
        [synchronizedContentView setPostsBoundsChangedNotifications:YES];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(scrollChanged:) name:NSViewBoundsDidChangeNotification object:synchronizedContentView];

        [sv addObserver:self forKeyPath:@"frameSize" options:0 context:NULL];

    }
    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if ([keyPath isEqualToString:@"frameSize"]) {
        [self updateContentFrame];
    }
}

-(void)scrollChanged:(id)contentView {
    [self.headerView setNeedsDisplay:YES];
}

- (BOOL)isFlipped {
    return YES;
}

- (void)freeGroups {
    while (_groups) {
        if (_groups->head) {
            [delegate unrefRow:_groups->head];
        }
        DdbListviewGroup_t *next = _groups->next;
        free (_groups);
        _groups = next;
    }
}

- (void)updateContentFrame {
    _fullwidth = 0;
    for (DdbListviewCol_t c = [delegate firstColumn]; c != [delegate invalidColumn]; c = [delegate nextColumn:c]) {
        _fullwidth += [delegate columnWidth:c];
    }

    NSScrollView *sv = [contentView enclosingScrollView];
    NSSize size = [sv contentSize];
    NSRect frame = [contentView frame];
    if (_fullwidth > size.width) {
        frame.size.width = _fullwidth;
    }
    else {
        frame.size.width = size.width;
    }
    if (_fullheight > size.height) {
        frame.size.height = _fullheight;
    }
    else {
        frame.size.height = size.height;
    }
    [contentView setFrame:frame];
}

// must be called from within pl_lock
- (void)initGroups {
    [delegate lock];
    groups_build_idx = [delegate modificationIdx];

    [self freeGroups];

    _fullwidth = 0;
    _fullheight = 0;
    DdbListviewGroup_t *grp = NULL;

    NSString *str;
    NSString *curr;

    int min_height= 0;
    for (DdbListviewCol_t c = [delegate firstColumn]; c != [delegate invalidColumn]; c = [delegate nextColumn:c]) {
        if ([delegate columnMinHeight:c] && [delegate columnWidth:c] > min_height) {
            min_height = [delegate columnWidth:c];
        }
        _fullwidth += [delegate columnWidth:c];
    }

    _grouptitle_height = rowheight;

    int idx = 0;
    DdbListviewRow_t it = [delegate firstRow];
    while (it != [delegate invalidRow]) {
        curr = [delegate rowGroupStr:it];
        if (!curr) {
            grp = malloc (sizeof (DdbListviewGroup_t));
            _groups = grp;
            memset (grp, 0, sizeof (DdbListviewGroup_t));
            grp->head = it;
            grp->head_idx = idx;
            grp->num_items = [delegate rowCount];
            _grouptitle_height = 0;
            grp->height = _grouptitle_height + grp->num_items * rowheight;
            _fullheight = grp->height;
            _fullheight += _grouptitle_height;

            [self updateContentFrame];
            [delegate unlock];
            return;
        }

        if (!grp || [str isNotEqualTo:curr]) {
            str = curr;
            DdbListviewGroup_t *newgroup = malloc (sizeof (DdbListviewGroup_t));
            if (grp) {
                if (grp->height - _grouptitle_height < min_height) {
                    grp->height = min_height + _grouptitle_height;
                }
                _fullheight += grp->height;
                grp->next = newgroup;
            }
            else {
                _groups = newgroup;
            }
            grp = newgroup;
            memset (grp, 0, sizeof (DdbListviewGroup_t));
            grp->head = it;
            grp->head_idx = idx;
            [delegate refRow:it];
            grp->num_items = 0;
            grp->height = _grouptitle_height;
        }
        grp->height += rowheight;
        grp->num_items++;
        DdbListviewRow_t next = [delegate nextRow:it];
        [delegate unrefRow:it];
        it = next;
        idx++;
    }
    if (grp) {
        if (grp->height - _grouptitle_height < min_height) {
            grp->height = min_height + _grouptitle_height;
        }
        _fullheight += grp->height;
    }
    [self updateContentFrame];
    [delegate unlock];
}

- (void)groupCheck {
    if ([delegate modificationIdx] != groups_build_idx) {
        [self initGroups];
    }
}

- (void)reloadData {
    [self initGroups];
}

- (int)pickPoint:(int)y group:(DdbListviewGroup_t **)group groupIndex:(int *)group_idx index:(int *)global_idx {
    int idx = 0;
    int grp_y = 0;
    int gidx = 0;
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    while (grp) {
        int h = grp->height;
        if (y >= grp_y && y < grp_y + h) {
            *group = grp;
            y -= grp_y;
            if (y < _grouptitle_height) {
                *group_idx = -1;
                *global_idx = idx;
            }
            else if (y >= _grouptitle_height + grp->num_items * rowheight) {
                *group_idx = (y - _grouptitle_height) / rowheight;
                *global_idx = -1;
            }
            else {
                *group_idx = (y - _grouptitle_height) / rowheight;
                *global_idx = idx + *group_idx;
            }
            return 0;
        }
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        gidx++;
    }
    return -1;
}

- (int)getDrawInfo:(int)row group:(DdbListviewGroup_t **)pgrp isEven:(int *)even cursor:(int *)cursor group_y:(int *)group_y rect:(NSRect *)rect {
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    int idx = 0;
    int idx2 = 0;
    rect->origin.y = 0;

    int totalwidth = 0;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        totalwidth += [delegate columnWidth:col];
    }

    while (grp) {
        int grpheight = grp->height;
        if (idx <= row && idx + grp->num_items > row) {
            // found
            int idx_in_group = row - idx;
            *pgrp = grp;
            *even = (idx2 + 1 + idx_in_group) & 1;
            *cursor = (row == [delegate cursor]) ? 1 : 0;
            *group_y = idx_in_group * rowheight;
            rect->origin.x = 0;
            rect->origin.y += _grouptitle_height + (row - idx) * rowheight;
            rect->size.width = totalwidth;
            rect->size.height = rowheight;
            return 0;
        }
        rect->origin.y += grpheight;
        idx += grp->num_items;
        idx2 += grp->num_items + 1;
        grp = grp->next;
    }
    return -1;

}

- (void)drawRow:(int)idx {
    DdbListviewGroup_t *grp;
    int even;
    int cursor;
    NSRect rect;
    int group_y;
    if ([self getDrawInfo:idx group:&grp isEven:&even cursor:&cursor group_y:&group_y rect:&rect] == -1) {
        return;
    }

    if (rect.origin.y + rect.size.height <= 0) {
        return;
    }

    if (rect.origin.y > [contentView bounds].origin.y + [contentView bounds].size.height) {
        return;
    }

    NSScrollView *sv = [contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    rect.origin.x = vis.origin.x;
    rect.size.width = vis.size.width;

    [contentView setNeedsDisplayInRect:rect];
}

- (void)clickSelection:(NSPoint)pt grp:(DdbListviewGroup_t *)grp grp_index:(int)grp_index sel:(int)sel dnd:(BOOL)dnd button:(int)button {

    _areaselect = 0;
    [self groupCheck];

    // clicked album art column?
    int album_art_column = 0; // FIXME

    if (sel == -1 && !album_art_column && (!grp || (pt.y > _grouptitle_height && grp_index >= grp->num_items))) {
        // clicked empty space, deselect everything
        DdbListviewRow_t it;
        int idx = 0;
        for (it = [delegate firstRow]; it != [delegate invalidRow]; idx++) {
            if ([delegate rowSelected:it]) {
                [delegate selectRow:it withState:NO];
                [self drawRow:idx];
                [delegate selectionChanged:it];
            }
            DdbListviewRow_t next = [delegate nextRow:it];
            [delegate unrefRow:it];
            it = next;
        }
    }
    else if ((sel != -1 && grp && grp_index == -1) || (pt.y <= _grouptitle_height && _groups_pinned) || album_art_column) {
        // clicked group title, select group
        DdbListviewRow_t it;
        int idx = 0;
        int cnt = -1;
        for (it = [delegate firstRow]; it != [delegate invalidRow]; idx++) {
            if (it == grp->head) {
                cnt = grp->num_items;
                cnt = grp->num_items;
            }
            if (cnt > 0) {
                if (![delegate rowSelected:it]) {
                    [delegate selectRow:it withState:YES];
                    [self drawRow:idx];
                    [delegate selectionChanged:it];
                }
                cnt--;
            }
            else {
                if ([delegate rowSelected:it]) {
                    [delegate selectRow:it withState:NO];
                    [self drawRow:idx];
                    [delegate selectionChanged:it];
                }
            }
            DdbListviewRow_t next = [delegate nextRow:it];
            [delegate unrefRow:it];
            it = next;
        }
    }
    else {
        // clicked specific item - select, or start drag-n-drop
        DdbListviewRow_t it = [delegate rowForIndex:sel];
        if (it == [delegate invalidRow] || ![delegate rowSelected:it]
            || (![delegate hasDND] && button == 1)) // HACK: don't reset selection by right click in search window
        {
            // reset selection, and set it to single item
            [self selectSingle:sel];
            if (dnd) {
                _areaselect = 1;
                _areaselect_y = pt.y;
                _shift_sel_anchor = [delegate cursor];
            }
        }
        else if (dnd) {
            _dragwait = YES;
        }
        [delegate unrefRow:it];
    }
}

- (void)selectSingle:(int)sel {
    [delegate lock];

    DdbListviewRow_t sel_it = [delegate rowForIndex:sel];
    if (sel_it == [delegate invalidRow]) {
        [delegate unlock];
        return;
    }

    int idx = 0;
    DdbListviewRow_t it = [delegate firstRow]; // FIXME: search window needs to go over PL_MAIN here
    while (it != [delegate invalidRow]) {
        BOOL sel = [delegate rowSelected:it];
        [delegate selectRow:it withState:it == sel_it];
        if (sel != (it == sel)) {
            [self drawRow:idx];
        }
        DdbListviewRow_t next = [delegate nextRow:it];
        [delegate unrefRow:it];
        it = next;
        idx ++;
    }

    [delegate unrefRow:sel_it];
    [delegate unlock];

    [delegate selectionChanged:[delegate invalidRow]];

    _area_selection_start = sel;
    _area_selection_end = sel;
}

- (void)listMouseUp:(NSEvent *)event {
    if (_dragwait) {
        _dragwait = NO;
        DdbListviewGroup_t *grp;
        int grp_index;
        int sel;
        NSPoint convPt = [self convertPoint:[event locationInWindow] fromView:nil];
        if (![self pickPoint:convPt.y group:&grp groupIndex:&grp_index index:&sel]) {
            [self selectSingle:sel];
        }
        else {
            [delegate setCursor:-1];
            DdbListviewRow_t it = [delegate firstRow];
            int idx = 0;
            while (it != [delegate invalidRow]) {
                if ([delegate rowSelected:it]) {
                    [delegate selectRow:it withState:NO];
                    [self drawRow:idx];
                    [delegate selectionChanged:it];
                    DdbListviewRow_t next = [delegate nextRow:it];
                    [delegate unrefRow:it];
                    it = next;
                }
                idx++;
            }
        }
    }
    else if (_areaselect) {
        _scroll_direction = 0;
        _scroll_pointer_y = -1;
        _areaselect = 0;
    }
}

- (void)listMouseDragged:(NSEvent *)event {
    [delegate lock];
    NSPoint pt = [contentView convertPoint:[event locationInWindow] fromView:nil];
    if (_dragwait) {
        if (abs (_lastpos.x - pt.x) > 3 || abs (_lastpos.y - pt.y) > 3) {
            _dragwait = 0;
            // begin dnd
        }
    }
    else if (_areaselect) {
        DdbListviewGroup_t *grp;
        int grp_index;
        int sel;
        if ([self pickPoint:pt.y group:&grp groupIndex:&grp_index index:&sel] == -1) {
            // past playlist bounds -> set to last track
            if (pt.y < 0) {
                sel = 0;
            }
            else {
                sel = [delegate rowCount] - 1;
            }
        }
        else if (sel == -1) {
            if (grp_index == -1) {
                if (_areaselect_y < pt.y) {
                    // below anchor, take last track in prev group
                    sel = grp->head_idx - 1;
                }
                else if (_areaselect_y > pt.y) {
                    // above, select 1st track in group
                    sel = grp->head_idx;
                }
                else {
                    sel = _shift_sel_anchor;
                }
            }
            else {
                if (_areaselect_y < pt.y) {
                    // below anchor, take last track in group
                    sel = grp->head_idx + grp->num_items - 1;
                }
                else if (_areaselect_y > pt.y) {
                    // above, select 1st track in next group
                    if (grp->next) {
                        sel = grp->next->head_idx;
                    }
                }
                else {
                    sel = _shift_sel_anchor;
                }
            }
        }
        int prev = [delegate cursor];
        if (sel != -1) {
            [delegate setCursor:sel];
        }
        {
            // select range of items
            int y = sel;
            int idx = 0;
            if (y == -1) {
                // find group
                [self groupCheck];
                DdbListviewGroup_t *g = _groups;
                while (g) {
                    if (g == grp) {
                        y = idx - 1;
                        break;
                    }
                    idx += g->num_items;
                    g = g->next;
                }
            }
            int start = min (y, _shift_sel_anchor);
            int end = max (y, _shift_sel_anchor);

            int nchanged = 0;

            // don't touch anything in process_start/end range
            int process_start = min (start, _area_selection_start);
            int process_end = max (end, _area_selection_end);
#define NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW 10
            idx=process_start;
            DdbListviewRow_t it = [delegate rowForIndex:idx];
            for (; it && idx <= process_end; idx++) {
                int selected = [delegate rowSelected:it];
                if (idx >= start && idx <= end) {
                    if (!selected) {
                        [delegate selectRow:it withState:YES];
                        nchanged++;
                        if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                            [self drawRow:idx];
                            [delegate selectionChanged:it];
                        }
                    }
                }
                else if (selected) {
                    [delegate selectRow:it withState:NO];
                    nchanged++;
                    if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        [self drawRow:idx];
                        [delegate selectionChanged:it];
                    }
                }
                DdbListviewRow_t next = [delegate nextRow:it];
                [delegate unrefRow:it];
                it = next;
            }
            if (it != [delegate invalidRow]) {
                [delegate unrefRow:it];
            }
            if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                [contentView setNeedsDisplay:YES];
                [delegate selectionChanged:it]; // that means "selection changed a lot, redraw everything
            }
            _area_selection_start = start;
            _area_selection_end = end;
        }
        if (sel != -1 && sel != prev) {
            if (prev != -1) {
                DdbListviewRow_t it = [delegate rowForIndex:prev];
                if (it != [delegate invalidRow]) {
                    [self drawRow:prev];
                    [delegate unrefRow:it];
                }
            }
            DdbListviewRow_t it = [delegate rowForIndex:sel];
            if (it != [delegate invalidRow]) {
                [self drawRow:sel];
                [delegate unrefRow:it];
            }
        }
    }

    [delegate unlock];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent {
    //if ([theEvent modifierFlags] & NSNumericPadKeyMask) {
    {
        NSString *theArrow = [theEvent charactersIgnoringModifiers];
        unichar keyChar = 0;
        if ( [theArrow length] == 0 )
            return;            // reject dead keys
        if ( [theArrow length] == 1 ) {

            int prev = [delegate cursor];
            int cursor = prev;

            keyChar = [theArrow characterAtIndex:0];
            switch (keyChar) {
                case NSDownArrowFunctionKey:
                    if (cursor < [delegate rowCount]-1) {
                        cursor++;
                    }
                    break;
                case NSUpArrowFunctionKey:
                    if (cursor > 0) {
                        cursor--;
                    }
                    else if (cursor < 0 && [delegate rowCount] > 0) {
                            cursor = 0;
                    }
                    break;
                case NSPageDownFunctionKey: {
                    NSScrollView *sv = [contentView enclosingScrollView];
                    NSRect vis = [sv documentVisibleRect];
                    [contentView scrollPoint:NSMakePoint(vis.origin.x, vis.origin.y + vis.size.height - rowheight)];
                    break;
                }
                case NSPageUpFunctionKey: {
                    NSScrollView *sv = [contentView enclosingScrollView];
                    NSRect vis = [sv documentVisibleRect];
                    [contentView scrollPoint:NSMakePoint(vis.origin.x, vis.origin.y - vis.size.height + rowheight)];
                    break;
                }
                default:
                    [super keyDown:theEvent];
                    return;
            }

            if ([theEvent modifierFlags] & NSShiftKeyMask) {
                if (cursor != prev) {
                    [delegate setCursor:cursor];
                    [self setScrollForPos:[self getRowPos:cursor]];
                    // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
                    int start = min (cursor, _shift_sel_anchor);
                    int end = max (cursor, _shift_sel_anchor);
                    
                    int nchanged = 0;
                    int idx = 0;
                    DdbListviewRow_t it;
                    for (it = [delegate firstRow]; it != [delegate invalidRow]; idx++) {
                        if (idx >= start && idx <= end) {
                            [delegate selectRow:it withState:YES];
                            if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                                [self drawRow:idx];
                                [delegate selectionChanged:it];
                            }
                        }
                        else if ([delegate rowSelected:it]) {
                            [delegate selectRow:it withState:NO];
                            if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                                [self drawRow:idx];
                                [delegate selectionChanged:it];
                            }
                        }
                        DdbListviewRow_t next = [delegate nextRow:it];
                        [delegate unrefRow:it];
                        it = next;
                    }
                    if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        [delegate selectionChanged:[delegate invalidRow]];
                    }
                }
            }
            else if (prev != cursor) {
                _shift_sel_anchor = cursor;
                [self setCursor:cursor noscroll:NO];
            }
        }
    }
}

- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll {
    int prev = [delegate cursor];
    DdbListviewRow_t prev_it = [delegate rowForIndex:prev];
    [delegate setCursor:cursor];

    BOOL prev_selected = NO;

    if (prev_it != [delegate invalidRow]) {
        prev_selected = [delegate rowSelected:prev_it];
    }

    [self selectSingle:cursor];

    if (prev_it != [delegate invalidRow]) {
        if (!prev_selected) {
            [self drawRow:prev];
        }

        [delegate unrefRow:prev_it];
    }

    if (!noscroll) {
        [self setScrollForPos:[self getRowPos:cursor]];
    }
}

- (void)setScrollForPos:(int)pos {
    NSScrollView *sv = [contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    int scrollpos = vis.origin.y;
    int cursor_scroll = pos;
    int newscroll = scrollpos;

    if (!_groups_pinned && cursor_scroll < scrollpos) {
        newscroll = cursor_scroll;
    }
    else if (_groups_pinned && cursor_scroll < scrollpos + _grouptitle_height) {
        newscroll = cursor_scroll - _grouptitle_height;
    }
    else if (cursor_scroll + rowheight >= scrollpos + vis.size.height) {
        newscroll = cursor_scroll + rowheight - vis.size.height + 1;
        if (newscroll < 0) {
            newscroll = 0;
        }
    }
    if (scrollpos != newscroll) {
        [contentView scrollPoint:NSMakePoint(vis.origin.x, newscroll)];
    }

}

- (int)getRowPos:(int)row_idx {
    int y = 0;
    int idx = 0;
    [delegate lock];
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    while (grp) {
        if (idx + grp->num_items > row_idx) {
            int i = y + _grouptitle_height + (row_idx - idx) * rowheight;
            [delegate unlock];
            return i;
        }
        y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    [delegate unlock];
    return y;
}

- (void)scrollToRowWithIndex:(int)idx {
    int pos = [self getRowPos:idx];
    NSScrollView *sv = [contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];

    if (pos < vis.origin.y || pos + rowheight >= vis.origin.y + vis.size.height) {
        [contentView scrollPoint:NSMakePoint(vis.origin.x, pos - vis.size.height/2)];
    }
}

- (void)setVScroll:(int)scroll {
    NSScrollView *sv = [contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    [contentView scrollPoint:NSMakePoint(vis.origin.x, scroll)];
}
@end
