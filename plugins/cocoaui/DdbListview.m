/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "DdbListview.h"

int headerheight = 17;
int rowheight = 19;
int grouptitleheight = 22;

@interface DdbListHeaderView : NSView {
    DdbListview *listview;
    int _orig_col_width;
    int _drag_col_pos;
    int _drag_delta;
    DdbListviewCol_t _dragging;
    DdbListviewCol_t _sizing;
    NSPoint _dragPt;
    BOOL _prepare;
    int _sortOrder;
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
    _prepare = YES;

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

    if (_prepare) { // clicked
        switch (_sortOrder) {
        case 0:
            _sortOrder = 1;
            break;
        case 1:
            _sortOrder = 2;
            break;
        case 2:
            _sortOrder = 1;
            break;
        }
        [delegate sortColumn:_dragging withOrder:_sortOrder-1];
        [listview setNeedsDisplay:YES];
    }
    else if (_dragging != [delegate invalidColumn] || _sizing != [delegate invalidColumn]) {
        [delegate columnsChanged];
        [listview updateContentFrame];
        [listview setNeedsDisplay:YES];
    }
    _dragging = [delegate invalidColumn];
    _sizing = [delegate invalidColumn];
}

- (void)mouseDragged:(NSEvent *)theEvent {
    NSPoint convPt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    _prepare = NO;

    id <DdbListviewDelegate> delegate = [listview delegate];
    if (_sizing != [delegate invalidColumn]) {
        int dx = convPt.x - _dragPt.x;

        int w = _orig_col_width + dx;
        if (w < 10) {
            w = 10;
        }
        if ([delegate columnWidth:_sizing] != w) {
            NSScrollView *sv = [listview.contentView enclosingScrollView];
            NSRect rc = [sv documentVisibleRect];

            int scroll = -rc.origin.x;

            [delegate setColumnWidth:w forColumn:_sizing];
            [listview updateContentFrame];
            [listview setNeedsDisplay:YES];

            rc = [sv documentVisibleRect];
            scroll += rc.origin.x;
            _dragPt.x -= scroll;
        }
    }
    else if (_dragging != [delegate invalidColumn]) {
        _drag_delta = convPt.x - _dragPt.x;

        NSScrollView *sv = [listview.contentView enclosingScrollView];
        NSRect rc = [sv documentVisibleRect];

        int x = -rc.origin.x;

        DdbListviewCol_t inspos = [delegate invalidColumn];

        // FIXME: DdbListviewCol_t is not always index -- account for this
        int cx = _drag_col_pos + _drag_delta;
        for (DdbListviewCol_t cc = [delegate firstColumn]; cc != [delegate invalidColumn]; cc = [delegate nextColumn:cc]) {
            int cw = [delegate columnWidth:cc];

            if (cc < _dragging && cx <= x + cw/2) {
                inspos = cc;
                break;
            }
            else if (cc > _dragging && cx > x + cw/2 - [delegate columnWidth:_dragging]) {
                inspos = cc;
            }

            x += cw;
        }

        if (inspos != [delegate invalidColumn] && inspos != _dragging) {
            [delegate moveColumn:_dragging to:inspos];
            _dragging = inspos;
            [listview reloadData];
        }
        else {
            [self setNeedsDisplay:YES];
        }
    }
}

- (DdbListviewCol_t)columnIndexForCoord:(NSPoint)theCoord {
    id <DdbListviewDelegate> delegate = [listview delegate];
    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];
    NSPoint convPt = [self convertPoint:theCoord fromView:nil];

    int x = -rc.origin.x;
    DdbListviewCol_t col;
    for (col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        if (CGRectContainsPoint(NSMakeRect(x, 0, w, [self bounds].size.height), convPt)) {
            break;
        }

        x += w;
    }
    return col;
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    id <DdbListviewDelegate> delegate = [listview delegate];
    DdbListviewCol_t col = [self columnIndexForCoord:[theEvent locationInWindow]];
    [delegate contextMenuForColumn:col withEvent:theEvent forView:self];
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

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {
    NSLog(@"[%@ %@]", NSStringFromClass([self class]), NSStringFromSelector(_cmd));

    NSPasteboard *pboard = [sender draggingPasteboard];

    if ([[pboard types] containsObject:NSStringPboardType]) {
    }
    return NSDragOperationCopy;
}

- (void)drawListView:(NSRect)dirtyRect {
    int idx = 0;
    int abs_idx = 0;

    id<DdbListviewDelegate> delegate = listview.delegate;

    [delegate lock];

    [listview groupCheck];

    DdbListviewGroup_t *grp = [listview groups];

    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];

    int grp_y = 0;
    int grp_next_y = 0;
    DdbListviewGroup_t *pinned_grp = NULL;

    int cursor = [delegate cursor];
    DdbListviewRow_t cursor_it = [delegate invalidRow];
    if (cursor != -1) {
        cursor_it = [delegate rowForIndex:cursor];
    }

    while (grp && grp_y + grp->height < dirtyRect.origin.y) {
        if (grp_y < vis.origin.y && grp_y + grp->height >= vis.origin.y) {
            pinned_grp = grp;
            grp->pinned = 1;
        }
        grp_y += grp->height;
        idx += grp->num_items + 1;
        abs_idx += grp->num_items;
        grp = grp->next;
    }

    if (grp && !pinned_grp && grp_y < vis.origin.y) {
        grp->pinned = 1;
        pinned_grp = grp;
    }
    else if (grp && pinned_grp && pinned_grp->next == grp) {
        grp->pinned = 2;
    }

    while (grp && grp_y < dirtyRect.origin.y + dirtyRect.size.height) {
        DdbListviewRow_t it = grp->head;
        int grp_height = [listview grouptitle_height] + grp->num_items * rowheight;
        int grp_height_total = grp->height;

        if (grp_y >= dirtyRect.origin.y + dirtyRect.size.height) {
            break;
        }
        [listview.delegate refRow:it];

        grp_next_y = grp_y + grp_height_total;
        int ii = 0;
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
                        [listview.delegate drawCell:abs_idx forRow: it forColumn:col inRect:NSMakeRect(x, grp_row_y, w, rowheight-1) focused:YES];
                    }
                    x += w;
                }

                if (x < dirtyRect.size.width) {
                    [listview.delegate drawCell:abs_idx forRow:it forColumn:[delegate invalidColumn] inRect:NSMakeRect(x, grp_row_y, dirtyRect.size.width-x, rowheight-1) focused:YES];
                }
            }
            if (it == cursor_it) {
                [[NSGraphicsContext currentContext] saveGraphicsState];
                [NSBezierPath setDefaultLineWidth:2.f];
                [[NSColor textColor] set];
                NSRect rect = NSMakeRect(dirtyRect.origin.x, grp_row_y, dirtyRect.size.width, rowheight-1);
                [NSBezierPath clipRect:rect];
                [NSBezierPath strokeRect:rect];
                [[NSGraphicsContext currentContext] restoreGraphicsState];
            }
            DdbListviewRow_t next = [listview.delegate nextRow:it];
            [listview.delegate unrefRow:it];
            it = next;
            if (!it) {
                break; // sanity check, in case groups were not rebuilt yet
            }
            abs_idx++;
        }

        idx += grp->num_items + 1;

        int filler = grp_height_total - (grp_height);
        if (filler > 0) {
            // fill with background color: x, grp_y - listview->scrollpos + grp_height, w, filler
        }

        //ddb_listview_list_render_album_art (listview, cr, it, grp->head, 0, grp->height, grp->pinned, grp_next_y - listview->scrollpos, -listview->hscrollpos, grp_y + listview->grouptitle_height - listview->scrollpos, listview->totalwidth, grp_height_total);

        if (grp->pinned == 1 && [listview groups_pinned]/* && dirtyRect.origin.y <= 0*/) {
            // draw pinned group title
            int pushback = 0;
            if (grp_next_y <= [listview grouptitle_height]) {
                pushback = [listview grouptitle_height] - grp_next_y;
            }

            NSRect groupRect = NSMakeRect(0, vis.origin.y - pushback, [self frame].size.width, [listview grouptitle_height]);
            NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:0];
            [clr set];
            [NSBezierPath fillRect:groupRect];
            if ([listview grouptitle_height] > 0) {
                [delegate drawGroupTitle:grp->head inRect:groupRect];
            }
        }
        else if (grp_y + [listview grouptitle_height] >= dirtyRect.origin.y && grp_y < dirtyRect.origin.y + dirtyRect.size.height) {
            if ([listview grouptitle_height] > 0) {
                NSRect groupRect = NSMakeRect(0, grp_y, [self frame].size.width, [listview grouptitle_height]);
                NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:0];
                [clr set];
                [NSBezierPath fillRect:groupRect];
                [delegate drawGroupTitle:grp->head inRect:groupRect];
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

    if (cursor_it != [delegate invalidRow]) {
        [delegate unrefRow:cursor_it];
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

- (void)trackProperties {
    id<DdbListviewDelegate> delegate = listview.delegate;
    [delegate trackProperties];
}

- (void)reloadMetadata {
    id<DdbListviewDelegate> delegate = listview.delegate;
    [delegate reloadMetadata];
}

- (void)convertSelection {
    id<DdbListviewDelegate> delegate = listview.delegate;
    [delegate convertSelection];
}

- (NSMenu *)menuForEvent:(NSEvent *)event {
    if (event.buttonNumber == 1
        || (event.buttonNumber == 0 && (event.modifierFlags & NSControlKeyMask))) {
        if (event.buttonNumber == 0) {
            // ctrl+click blocks the mouseDown handler, do it now
            [self mouseDown:event];
        }
        NSMenu *theMenu = [[NSMenu alloc] initWithTitle:@"Playlist Context Menu"];
        id<DdbListviewDelegate> delegate = listview.delegate;
        BOOL enabled = [delegate selectedCount] != 0;

        [[theMenu insertItemWithTitle:@"Track Properties" action:@selector(trackProperties) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

        [[theMenu insertItemWithTitle:@"Reload metadata" action:@selector(reloadMetadata) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

        // FIXME: should be added via plugin action
        [[theMenu insertItemWithTitle:@"Convert" action:@selector(convertSelection) keyEquivalent:@"" atIndex:0] setEnabled:enabled];
        [theMenu setAutoenablesItems:NO];
        return theMenu;
    }
    return nil;
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    if (![[self window] isKeyWindow]) {
        return;
    }
    [self mouseDown:theEvent];
    [super rightMouseDown:theEvent];
}

- (void)mouseDown:(NSEvent *)event {
    [[self window] makeFirstResponder:listview];

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
        [listview deselectAll];
        [delegate unlock];
        return;
    }

    int cursor = [delegate cursor];

    if (event.clickCount == 2 && event.buttonNumber == 0) {
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
    if (event.buttonNumber != 0 || !(event.modifierFlags & (NSCommandKeyMask|NSShiftKeyMask))) {
        [listview clickSelection:convPt grp:grp grp_index:grp_index sel:sel dnd:YES button:1];
    }
    else if (event.buttonNumber == 0 && (event.modifierFlags & NSCommandKeyMask)) {
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
    else if (event.buttonNumber == 0 && (event.modifierFlags & NSShiftKeyMask)) {
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

@synthesize headerView;
@synthesize contentView;

- (DdbListview *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    if (self) {
        _groups_pinned = YES;
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

        [contentView registerForDraggedTypes:[NSArray arrayWithObjects:NSStringPboardType, nil]];

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
            [_delegate unrefRow:_groups->head];
        }
        DdbListviewGroup_t *next = _groups->next;
        free (_groups);
        _groups = next;
    }
}

- (void)updateContentFrame {
    _fullwidth = 0;
    for (DdbListviewCol_t c = [_delegate firstColumn]; c != [_delegate invalidColumn]; c = [_delegate nextColumn:c]) {
        _fullwidth += [_delegate columnWidth:c];
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
    [_delegate lock];
    groups_build_idx = [_delegate modificationIdx];

    [self freeGroups];

    _fullwidth = 0;
    _fullheight = 0;
    DdbListviewGroup_t *grp = NULL;

    NSString *str;
    NSString *curr;

    int min_height= 0;
    for (DdbListviewCol_t c = [_delegate firstColumn]; c != [_delegate invalidColumn]; c = [_delegate nextColumn:c]) {
        if ([_delegate columnMinHeight:c] && [_delegate columnWidth:c] > min_height) {
            min_height = [_delegate columnWidth:c];
        }
        _fullwidth += [_delegate columnWidth:c];
    }

    _grouptitle_height = grouptitleheight;

    int idx = 0;
    DdbListviewRow_t it = [_delegate firstRow];
    while (it != [_delegate invalidRow]) {
        curr = [_delegate rowGroupStr:it];
        if (!curr) {
            grp = malloc (sizeof (DdbListviewGroup_t));
            _groups = grp;
            memset (grp, 0, sizeof (DdbListviewGroup_t));
            grp->head = it;
            grp->head_idx = idx;
            grp->num_items = [_delegate rowCount];
            _grouptitle_height = 0;
            grp->height = _grouptitle_height + grp->num_items * rowheight;
            _fullheight = grp->height;
            _fullheight += _grouptitle_height;

            [self updateContentFrame];
            [_delegate unlock];
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
            [_delegate refRow:it];
            grp->num_items = 0;
            grp->height = _grouptitle_height;
        }
        grp->height += rowheight;
        grp->num_items++;
        DdbListviewRow_t next = [_delegate nextRow:it];
        [_delegate unrefRow:it];
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
    [_delegate unlock];
}

- (void)groupCheck {
    if ([_delegate modificationIdx] != groups_build_idx) {
        [self initGroups];
    }
}

- (void)reloadData {
    [self initGroups];
    [self setNeedsDisplay:YES];
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
    for (DdbListviewCol_t col = [_delegate firstColumn]; col != [_delegate invalidColumn]; col = [_delegate nextColumn:col]) {
        totalwidth += [_delegate columnWidth:col];
    }

    while (grp) {
        int grpheight = grp->height;
        if (idx <= row && idx + grp->num_items > row) {
            // found
            int idx_in_group = row - idx;
            *pgrp = grp;
            *even = (idx2 + 1 + idx_in_group) & 1;
            *cursor = (row == [_delegate cursor]) ? 1 : 0;
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

- (void)deselectAll {
    DdbListviewRow_t it;
    int idx = 0;
    for (it = [_delegate firstRow]; it != [_delegate invalidRow]; idx++) {
        if ([_delegate rowSelected:it]) {
            [_delegate selectRow:it withState:NO];
            [self drawRow:idx];
            [_delegate selectionChanged:it];
        }
        DdbListviewRow_t next = [_delegate nextRow:it];
        [_delegate unrefRow:it];
        it = next;
    }
}

- (void)clickSelection:(NSPoint)pt grp:(DdbListviewGroup_t *)grp grp_index:(int)grp_index sel:(int)sel dnd:(BOOL)dnd button:(int)button {

    _areaselect = 0;
    [self groupCheck];

    // clicked album art column?
    int album_art_column = 0; // FIXME

    NSScrollView *sv = [contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];

    if (sel == -1 && !album_art_column && (!grp || (pt.y > _grouptitle_height && grp_index >= grp->num_items))) {
        // clicked empty space, deselect everything
        [self deselectAll];
    }
    else if ((sel != -1 && grp && grp_index == -1) || (pt.y <= _grouptitle_height + vis.origin.y && _groups_pinned) || album_art_column) {
        // clicked group title, select group
        DdbListviewRow_t it;
        int idx = 0;
        int cnt = -1;
        for (it = [_delegate firstRow]; it != [_delegate invalidRow]; idx++) {
            if (it == grp->head) {
                cnt = grp->num_items;
                cnt = grp->num_items;
            }
            if (cnt > 0) {
                if (![_delegate rowSelected:it]) {
                    [_delegate selectRow:it withState:YES];
                    [self drawRow:idx];
                    [_delegate selectionChanged:it];
                }
                cnt--;
            }
            else {
                if ([_delegate rowSelected:it]) {
                    [_delegate selectRow:it withState:NO];
                    [self drawRow:idx];
                    [_delegate selectionChanged:it];
                }
            }
            DdbListviewRow_t next = [_delegate nextRow:it];
            [_delegate unrefRow:it];
            it = next;
        }
    }
    else {
        // clicked specific item - select, or start drag-n-drop
        DdbListviewRow_t it = [_delegate rowForIndex:sel];
        if (it == [_delegate invalidRow] || ![_delegate rowSelected:it]
            || (![_delegate hasDND] && button == 1)) // HACK: don't reset selection by right click in search window
        {
            // reset selection, and set it to single item
            [self selectSingle:sel];
            [contentView setNeedsDisplay:YES];
            if (dnd) {
                _areaselect = 1;
                _areaselect_y = pt.y;
                _shift_sel_anchor = [_delegate cursor];
            }
        }
        else if (dnd) {
            _dragwait = YES;
        }
        [_delegate unrefRow:it];
    }
}

- (void)selectSingle:(int)sel {
    [_delegate lock];

    DdbListviewRow_t sel_it = [_delegate rowForIndex:sel];
    if (sel_it == [_delegate invalidRow]) {
        [_delegate unlock];
        return;
    }

    [_delegate deselectAll];
    [_delegate selectRow:sel_it withState:YES];
    [_delegate unrefRow:sel_it];
    [_delegate unlock];

    [_delegate selectionChanged:[_delegate invalidRow]];

    _area_selection_start = sel;
    _area_selection_end = sel;
}

- (void)listMouseUp:(NSEvent *)event {
    if (_dragwait) {
        _dragwait = NO;
        DdbListviewGroup_t *grp;
        int grp_index;
        int sel;
        NSPoint convPt = [contentView convertPoint:[event locationInWindow] fromView:nil];
        if (![self pickPoint:convPt.y group:&grp groupIndex:&grp_index index:&sel]) {
            [self selectSingle:sel];
            [contentView setNeedsDisplay:YES];
        }
        else {
            [_delegate setCursor:-1];
            DdbListviewRow_t it = [_delegate firstRow];
            int idx = 0;
            while (it != [_delegate invalidRow]) {
                if ([_delegate rowSelected:it]) {
                    [_delegate selectRow:it withState:NO];
                    [self drawRow:idx];
                    [_delegate selectionChanged:it];
                }
                DdbListviewRow_t next = [_delegate nextRow:it];
                [_delegate unrefRow:it];
                it = next;
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

// NSDraggingSource protocol
- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
    switch(context) {
        case NSDraggingContextWithinApplication:
            return NSDragOperationCopy | NSDragOperationMove | NSDragOperationDelete;
    }
    return NSDragOperationNone;
}

- (void)listMouseDragged:(NSEvent *)event {
    [_delegate lock];
    NSPoint pt = [contentView convertPoint:[event locationInWindow] fromView:nil];
    if (_dragwait) {
        if (abs (_lastpos.x - pt.x) > 3 || abs (_lastpos.y - pt.y) > 3) {
            // begin dnd
            NSPasteboard *pboard;

            pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
            [pboard declareTypes:[NSArray arrayWithObject:NSStringPboardType]  owner:self];
            [pboard setData:[@"Hello" dataUsingEncoding:NSASCIIStringEncoding] forType:NSStringPboardType];

            NSImage *img = [NSImage imageNamed:NSImageNameMultipleDocuments];

            NSPoint dpt = pt;
            dpt.x -= [img size].width/2;
            dpt.y += [img size].height;

            [self dragImage:img at:dpt offset:NSMakeSize(0.0, 0.0) event:event pasteboard:pboard source:self slideBack:YES];
            _dragwait = NO;

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
                sel = [_delegate rowCount] - 1;
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
        int prev = [_delegate cursor];
        if (sel != -1) {
            [_delegate setCursor:sel];
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
            DdbListviewRow_t it = [_delegate rowForIndex:idx];
            for (; it && idx <= process_end; idx++) {
                int selected = [_delegate rowSelected:it];
                if (idx >= start && idx <= end) {
                    if (!selected) {
                        [_delegate selectRow:it withState:YES];
                        nchanged++;
                        if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                            [self drawRow:idx];
                            [_delegate selectionChanged:it];
                        }
                    }
                }
                else if (selected) {
                    [_delegate selectRow:it withState:NO];
                    nchanged++;
                    if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        [self drawRow:idx];
                        [_delegate selectionChanged:it];
                    }
                }
                DdbListviewRow_t next = [_delegate nextRow:it];
                [_delegate unrefRow:it];
                it = next;
            }
            if (it != [_delegate invalidRow]) {
                [_delegate unrefRow:it];
            }
            if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                [contentView setNeedsDisplay:YES];
                [_delegate selectionChanged:it]; // that means "selection changed a lot, redraw everything
            }
            _area_selection_start = start;
            _area_selection_end = end;
        }
        if (sel != -1 && sel != prev) {
            if (prev != -1) {
                DdbListviewRow_t it = [_delegate rowForIndex:prev];
                if (it != [_delegate invalidRow]) {
                    [self drawRow:prev];
                    [_delegate unrefRow:it];
                }
            }
            DdbListviewRow_t it = [_delegate rowForIndex:sel];
            if (it != [_delegate invalidRow]) {
                [self drawRow:sel];
                [_delegate unrefRow:it];
            }
        }
    }

    [_delegate unlock];
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

            int prev = [_delegate cursor];
            int cursor = prev;

            keyChar = [theArrow characterAtIndex:0];
            switch (keyChar) {
                case NSDownArrowFunctionKey:
                    if (cursor < [_delegate rowCount]-1) {
                        cursor++;
                    }
                    break;
                case NSUpArrowFunctionKey:
                    if (cursor > 0) {
                        cursor--;
                    }
                    else if (cursor < 0 && [_delegate rowCount] > 0) {
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
                    [_delegate setCursor:cursor];
                    [self setScrollForPos:[self getRowPos:cursor]];
                    // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
                    int start = min (cursor, _shift_sel_anchor);
                    int end = max (cursor, _shift_sel_anchor);
                    
                    int nchanged = 0;
                    int idx = 0;
                    DdbListviewRow_t it;
                    for (it = [_delegate firstRow]; it != [_delegate invalidRow]; idx++) {
                        if (idx >= start && idx <= end) {
                            [_delegate selectRow:it withState:YES];
                            if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                                [self drawRow:idx];
                                [_delegate selectionChanged:it];
                            }
                        }
                        else if ([_delegate rowSelected:it]) {
                            [_delegate selectRow:it withState:NO];
                            if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                                [self drawRow:idx];
                                [_delegate selectionChanged:it];
                            }
                        }
                        DdbListviewRow_t next = [_delegate nextRow:it];
                        [_delegate unrefRow:it];
                        it = next;
                    }
                    if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        [_delegate selectionChanged:[_delegate invalidRow]];
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
    [_delegate setCursor:cursor];

    DdbListviewRow_t row = [_delegate rowForIndex:cursor];
    if (![_delegate rowSelected:row]) {
        [self selectSingle:cursor];
    }

    BOOL need_redraw = YES;
    if (!noscroll) {
        if ([self setScrollForPos:[self getRowPos:cursor]]) {
            need_redraw = NO;
        }
    }
    if (need_redraw) {
        [contentView setNeedsDisplay:YES];
    }
}

- (BOOL)setScrollForPos:(int)pos {
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
        return YES;
    }
    return NO;
}

- (int)getRowPos:(int)row_idx {
    int y = 0;
    int idx = 0;
    [_delegate lock];
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    while (grp) {
        if (idx + grp->num_items > row_idx) {
            int i = y + _grouptitle_height + (row_idx - idx) * rowheight;
            [_delegate unlock];
            return i;
        }
        y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    [_delegate unlock];
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

- (id<DdbListviewDelegate>)delegate {
    return _delegate;
}

- (void)setDelegate:(id<DdbListviewDelegate>)delegate {
    _delegate = delegate;
}

@end
