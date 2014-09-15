//
//  DdbListview.m
//  deadbeef
//
//  Created by waker on 13/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbListview.h"

int rowheight = 18;

@interface DdbListHeaderView : NSView {
    DdbListview *listview;
}
- (void)setListView:(DdbListview *)lv;
@end

@implementation DdbListHeaderView
- (DdbListHeaderView *)initWithFrame:(NSRect)rect {
    return [super initWithFrame:rect];
}

- (void)setListView:(DdbListview *)lv {
    listview = lv;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    NSRect rect = [self bounds];
    [[NSColor lightGrayColor] set];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x, rect.origin.y+rect.size.height-1,rect.size.width,1)];

    NSGradient *gr = [[NSGradient alloc] initWithColorsAndLocations:
                      [NSColor whiteColor], 0.f,
                      [NSColor colorWithCalibratedWhite:0.9f alpha:1.f], 0.55f,
                      [NSColor colorWithCalibratedWhite:0.88f alpha:1.f], 0.55f,
                      [NSColor colorWithCalibratedWhite:0.88f alpha:1.f], 0.89f,
                      [NSColor colorWithCalibratedWhite:0.9f alpha:1.f], 0.90f,
                      [NSColor whiteColor], 1.f, nil];

    [gr drawInRect:NSMakeRect(rect.origin.x, rect.origin.y,rect.size.width,rect.size.height-1) angle:270];

    int x = -rc.origin.x;
    id <DdbListviewDelegate> delegate = [listview delegate];
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];
        if (CGRectIntersectsRect(dirtyRect, NSMakeRect(x, 0, w, [self frame].size.height))) {
            [delegate drawColumnHeader:col inRect:NSMakeRect(x, 0, w, [self frame].size.height)];
        }
        x += w;
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
                        [listview.delegate drawCell:it forColumn:col inRect:NSMakeRect(x, grp_row_y, w, rowheight)];
                    }
                    x += w;
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
                //listview->binding->draw_group_title (listview, cr, grp->head, -listview->hscrollpos, y - pushback, listview->totalwidth, listview->grouptitle_height);
            }
        }
        else if (grp_y + [listview grouptitle_height] >= dirtyRect.origin.y && grp_y < dirtyRect.origin.y + dirtyRect.size.height) {
            //ddb_listview_list_render_row_background (listview, cr, NULL, 1, 0, -listview->hscrollpos, grp_y - listview->scrollpos, listview->totalwidth, listview->grouptitle_height);
            if ([listview grouptitle_height] > 0) {
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
    NSScrollView *sv = [self enclosingScrollView];
    dirtyRect = [sv documentVisibleRect];

    [NSGraphicsContext saveGraphicsState];
    NSBezierPath* clipPath = [NSBezierPath bezierPath];
    [clipPath appendBezierPathWithRect:dirtyRect];
    [clipPath setClip];

//    [[NSColor yellowColor] set];
//    [NSBezierPath fillRect:dirtyRect];

    [self drawListView:dirtyRect];

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
    NSPoint convPt = [self convertPoint:[event locationInWindow] toView:nil];
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
// FIXME        listview.shift_sel_anchor = [delegate cursor];
    }

    // single selection
    if (0 == (event.modifierFlags & (NSCommandKeyMask|NSShiftKeyMask))) {
        [listview clickSelection:convPt grp:grp grp_index:grp_index sel:sel dnd:YES button:1];
    }
    [delegate unlock];
}

- (void)mouseUp:(NSEvent *)event
{
}

- (void)mouseDragged:(NSEvent *)event
{
    NSPoint dragLocation;
    dragLocation=[self convertPoint:[event locationInWindow]
                           fromView:nil];

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
        DdbListHeaderView *thv = [[DdbListHeaderView alloc] initWithFrame:NSMakeRect(0, 0, rect.size.width, 16)];
        [thv setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMaxYMargin];
        [self addSubview:thv];
        [thv setListView:self];
        headerView = thv;

        NSScrollView *sv = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 16, rect.size.width, rect.size.height-17)];
        [sv setHasVerticalScroller:YES];
        [sv setHasHorizontalScroller:YES];
        [sv setAutohidesScrollers:YES];
        [sv setBorderType:NSBezelBorder];
        [sv setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewHeightSizable|NSViewMaxYMargin];
        [self addSubview:sv];

        DdbListContentView *lcv = [[DdbListContentView alloc] initWithFrame:rect];
        [sv setDocumentView:lcv];
        [lcv setListView:self];
        contentView = lcv;

        NSView *synchronizedContentView = [sv contentView];
        [synchronizedContentView setPostsBoundsChangedNotifications:YES];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(scrollChanged:) name:NSViewBoundsDidChangeNotification object:synchronizedContentView];

    }
    return self;
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

// must be called from within pl_lock
- (void)initGroups {
    [delegate lock];
    int old_height = _fullheight;
    groups_build_idx = [delegate modificationIdx];

    [self freeGroups];

    _fullheight = 0;
    DdbListviewGroup_t *grp = NULL;

    NSString *str;
    NSString *curr;

    int min_height= 0;
    for (DdbListviewCol_t c = [delegate firstColumn]; c != [delegate invalidColumn]; c = [delegate nextColumn:c]) {
        if ([delegate columnMinHeight:c] && [delegate columnWidth:c] > min_height) {
            min_height = [delegate columnWidth:c];
        }
    }

    _grouptitle_height = rowheight;

    DdbListviewRow_t it = [delegate firstRow];
    while (it != [delegate invalidRow]) {
        curr = [delegate rowGroupStr:it];
        if (!curr) {
            grp = malloc (sizeof (DdbListviewGroup_t));
            _groups = grp;
            memset (grp, 0, sizeof (DdbListviewGroup_t));
            grp->head = it;
            grp->num_items = [delegate rowCount];
            _grouptitle_height = 0;
            grp->height = _grouptitle_height + grp->num_items * rowheight;
            _fullheight = grp->height;
            _fullheight += _grouptitle_height;
            if (old_height != _fullheight) {
                NSRect frame = [contentView frame];
                frame.size.height = _fullheight;
                contentView.frame = frame;
            }
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
            [delegate refRow:it];
            grp->num_items = 0;
            grp->height = _grouptitle_height;
        }
        grp->height += rowheight;
        grp->num_items++;
        DdbListviewRow_t next = [delegate nextRow:it];
        [delegate unrefRow:it];
        it = next;
    }
    if (grp) {
        if (grp->height - _grouptitle_height < min_height) {
            grp->height = min_height + _grouptitle_height;
        }
        _fullheight += grp->height;
    }
    if (old_height != _fullheight) {
        NSRect frame = [contentView frame];
        frame.size.height = _fullheight;
        contentView.frame = frame;
    }
    [delegate unlock];
}

- (void)groupCheck {
//    if ([delegate modificationIdx] != groups_build_idx) {
        [self initGroups];
//    }
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

    if (rect.origin.y > [self bounds].origin.y + [self bounds].size.height) {
        return;
    }

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

    DdbListviewRow_t it = [delegate firstRow]; // FIXME: search window needs to go over PL_MAIN here
    while (it != [delegate invalidRow]) {
        [delegate selectRow:it withState:it == sel_it];
        DdbListviewRow_t next = [delegate nextRow:it];
        [delegate unrefRow:it];
        it = next;
    }

    [delegate unrefRow:sel_it];
    [delegate unlock];

//    FIXME: notify all widgets that data selection has changed
//    was: ddb_listview_refresh (ps, DDB_REFRESH_LIST);

    [delegate selectionChanged:[delegate invalidRow]]; // that means "selection changed a lot, redraw everything"

    [self setNeedsDisplay:YES]; // FIXME: this should be triggered elsewhere, see above

    _area_selection_start = sel;
    _area_selection_end = sel;
}

@end
