//
//  PlaylistContentView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "PinnedGroupTitleView.h"
#import "PlaylistContentView.h"
#import "PlaylistView.h"
#import "DdbShared.h"
#import "MedialibItemDragDropHolder.h"
#import "PlaylistLocalDragDropHolder.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

//#define DEBUG_DRAW_GROUP_TITLES 1

#define BLANK_GROUP_SUBDIVISION 100

static int rowheight = 19;
static int grouptitleheight = 22;

@interface PlaylistContentView ()

@property (nonatomic) NSPoint lastDragLocation;
@property (nonatomic) BOOL draggingInView;
@property (nonatomic) BOOL dragwait;
@property (nonatomic) NSPoint lastpos;

@property (nonatomic) BOOL areaselect;
@property (nonatomic) CGFloat areaselect_y;
@property (nonatomic) int area_selection_start;
@property (nonatomic) int area_selection_end;

@property (nonatomic) int shift_sel_anchor;

@property (nonatomic) DdbListviewGroup_t *groups;
@property (nonatomic,readwrite) int grouptitle_height;
@property (nonatomic) int groups_build_idx;
@property (nonatomic) int fullwidth;
@property (nonatomic) int fullheight;
@property (nonatomic) int scroll_direction;
@property (nonatomic) int scroll_pointer_y;

@property (nonatomic) PinnedGroupTitleView *pinnedGroupTitleView;

@property (nonatomic) NSLayoutConstraint *widthConstraint;
@property (nonatomic) NSLayoutConstraint *heightConstraint;

@end

@implementation PlaylistContentView

- (instancetype)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];

    self.groups_build_idx = -1;

    [self registerForDraggedTypes:[NSArray arrayWithObjects:ddbPlaylistItemsUTIType, ddbMedialibItemUTIType, NSFilenamesPboardType, nil]];

    _pinnedGroupTitleView = [PinnedGroupTitleView new];
    _pinnedGroupTitleView.hidden = YES;
    [self addSubview:_pinnedGroupTitleView];

    return self;
}

- (void)setDelegate:(id<DdbListviewDelegate>)delegate {
    _delegate = delegate;
    self.pinnedGroupTitleView.delegate = delegate;
}

- (void)cleanup
{
    [self freeGroups];
}

- (void)dealloc
{
    [self cleanup];
}

#pragma mark - Drag and drop

- (void)drawLineIndicator:(NSRect)dirtyRect yy:(int)yy  {

    int yyline = yy;
    float indicatorLineWith = 1.f;
    if ( yyline > 0 ) {
        yyline -= (indicatorLineWith / 2.f );
    }
    [[NSGraphicsContext currentContext] saveGraphicsState];
    NSBezierPath.defaultLineWidth =  indicatorLineWith;
    [NSColor.alternateSelectedControlColor set];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(dirtyRect.origin.x, yyline) toPoint: NSMakePoint( dirtyRect.origin.x + dirtyRect.size.width, yyline ) ];
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

#pragma mark NSDraggingDestination

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender {

    _draggingInView = YES;

    NSUInteger modifiers = (NSEvent.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask);

    if (modifiers == NSEventModifierFlagOption)   {
        return NSDragOperationCopy;
    }

    return NSDragOperationMove;
}

- (BOOL)wantsPeriodicDraggingUpdates {

    return NO;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {

    _lastDragLocation = [self convertPoint:[sender draggingLocation] fromView:nil];
    self.needsDisplay = YES;

    NSUInteger modifiers = (NSEvent.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask);

    if (modifiers == NSEventModifierFlagOption)   {
        return NSDragOperationCopy;
    }

    return NSDragOperationMove;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {

    _draggingInView = NO;
    self.needsDisplay = YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {

    NSPasteboard *pboard = [sender draggingPasteboard];

    int sel;

    NSPoint draggingLocation = [self convertPoint:[sender draggingLocation] fromView:nil];

    DdbListviewRow_t row = [self.dataModel invalidRow];
    sel = [self dragInsertPointForYPos:draggingLocation.y];
    if (-1 != sel) {
        row = [self.dataModel rowForIndex:sel];
    }

    if ([pboard.types containsObject:ddbPlaylistItemsUTIType]) {
        NSArray *classes = [[NSArray alloc] initWithObjects:[PlaylistLocalDragDropHolder class], nil];
        NSDictionary *options = [NSDictionary dictionary];
        NSArray<PlaylistLocalDragDropHolder *> *draggedItems = [pboard readObjectsForClasses:classes options:options];

        for (PlaylistLocalDragDropHolder *holder in draggedItems) {
            NSInteger from_playlist = holder.playlistIdx;
            uint32_t *indices = calloc (sizeof (uint32_t), holder.itemsIndices.count);
            int i = 0;
            for (NSNumber * number in holder.itemsIndices) {
                indices[i] = (uint32_t)number.unsignedIntValue;
                ++i;
            }

            NSUInteger length = holder.itemsIndices.count;

            NSDragOperation op = sender.draggingSourceOperationMask;
            [self.delegate dropItems:(int)from_playlist before:row indices:indices count:(int)length copy:op==NSDragOperationCopy];
            free(indices);
        }
    }
    if ([pboard.types containsObject:ddbMedialibItemUTIType]) {
        NSArray *classes = [[NSArray alloc] initWithObjects:[MedialibItemDragDropHolder class], nil];
        NSDictionary *options = [NSDictionary dictionary];
        NSArray<MedialibItemDragDropHolder *> *draggedItems = [pboard readObjectsForClasses:classes options:options];

        NSInteger count = draggedItems.count;
        DdbListviewRow_t *items = calloc (count, sizeof (DdbListviewRow_t));
        size_t itemCount = 0;
        for (MedialibItemDragDropHolder *holder in draggedItems) {
            ddb_playItem_t *it = holder.playItem;
            if (it) {
                items[itemCount++] = (DdbListviewRow_t)it;
            }
        }
        [self.delegate dropPlayItems:items before:row count:(int)itemCount];
        free (items);
    }
    else if ([pboard.types containsObject:NSFilenamesPboardType]) {

        NSArray *paths = [pboard propertyListForType:NSFilenamesPboardType];
        if (row != [self.dataModel invalidRow]) {
            // add before selected row
            [self.delegate externalDropItems:paths after: [self.dataModel rowForIndex:sel-1] ];
        }
        else {
            // no selected row, add to end
            DdbListviewRow_t lastRow = [self.dataModel rowForIndex:([self.dataModel rowCount]-1)];
            [self.delegate externalDropItems:paths after:lastRow];
        }
    }

    if (row != [self.dataModel invalidRow]) {
        [self.dataModel unrefRow:row];
    }
    _draggingInView = NO;
    return YES;
}

#pragma mark NSDraggingSource

- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
    switch(context) {
        case NSDraggingContextWithinApplication:
            return NSDragOperationCopy | NSDragOperationMove;
        case NSDraggingContextOutsideApplication:
            return NSDragOperationNone; // FIXME
    }
    return NSDragOperationNone;
}


#pragma mark - Album art

- (void)renderAlbumArtForGroup:(DdbListviewGroup_t *)grp
                    groupIndex:(int)groupIndex
                 isPinnedGroup:(BOOL)isPinnedGroup
                nextGroupCoord:(int)grp_next_y
                          yPos:(int)y
                     viewportY:(CGFloat)viewportY
                    clipRegion:(NSRect)clip {
    int x = 0;

    int title_height = [self grouptitle_height];
    for (DdbListviewCol_t col = [self.delegate firstColumn];
         col != [self.delegate invalidColumn];
         col = [self.delegate nextColumn:col]) {

        int w = [self.delegate columnWidth:col];

        if ([self.delegate isAlbumArtColumn:col] && x + w > clip.origin.x) {
            NSColor *clr = [NSColor.controlAlternatingRowBackgroundColors objectAtIndex:0];
            [clr set];
            [NSBezierPath fillRect:NSMakeRect (x, y, w, grp_next_y - y)];
            if (title_height > 0) {
                [self.delegate drawAlbumArtForGroup:grp groupIndex:groupIndex inColumn:col isPinnedGroup:isPinnedGroup nextGroupCoord:grp_next_y xPos:x yPos:y viewportY:viewportY width:w height:grp->height];
            }
        }
        x += w;
    }
}

#pragma mark - Drawing

- (void)drawGroupTitle:(DdbListviewGroup_t *)grp grp_y:(int)grp_y title_height:(int)title_height {
    NSRect groupRect = NSMakeRect(0, grp_y, self.frame.size.width, title_height);
    NSColor *clr = [NSColor.controlAlternatingRowBackgroundColors objectAtIndex:0];
    [clr set];
#if DEBUG_DRAW_GROUP_TITLES
    [NSColor.greenColor set];
#endif
    [NSBezierPath fillRect:groupRect];
    [self.delegate drawGroupTitle:grp->head inRect:groupRect];
}

- (void)updatePinnedGroup {
    NSScrollView *scrollView = self.enclosingScrollView;
    NSRect visibleRect = [scrollView documentVisibleRect];

    DdbListviewGroup_t *grp = self.groups;

    CGFloat scrollPos = visibleRect.origin.y;

    int idx = 0;
    int grp_y = 0;
    int groupIndex = 0;
    while (grp && grp_y + grp->height < scrollPos) {
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        groupIndex++;
    }

    if (self.delegate.pinGroups && grp && grp_y < scrollPos && grp_y + grp->height >= scrollPos) {
        self.pinnedGroupTitleView.hidden = NO;
        self.pinnedGroupTitleView.group = grp;
        CGFloat pos = scrollPos;
        int grp_next_y = grp_y + grp->height;
        if (grp->next && pos + self.grouptitle_height > grp_next_y) {
            pos = grp_next_y - self.grouptitle_height;
        }
        self.pinnedGroupTitleView.frame = NSMakeRect(0, pos, NSWidth(self.frame), self.grouptitle_height);
    }
    else {
        self.pinnedGroupTitleView.hidden = YES;
    }

    if ([self.delegate respondsToSelector:@selector(scrollChanged:)]) {
        [self.delegate scrollChanged:scrollPos];
    }
}

- (void)setFrameSize:(NSSize)newSize {
    [super setFrameSize:newSize];
    [self updatePinnedGroup];
}

- (void)scrollChanged:(NSRect)visibleRect {
    [self updatePinnedGroup];
}

- (BOOL)becomeFirstResponder {
    self.needsDisplay = YES;
    return YES;
}

- (BOOL)resignFirstResponder {
    self.needsDisplay = YES;
    return YES;
}

- (void)drawListView:(NSRect)dirtyRect {
    [self groupCheck];

    DdbListviewGroup_t *grp = [self groups];

    CGFloat clip_y = dirtyRect.origin.y;
    CGFloat clip_h = dirtyRect.size.height;


    // find 1st group
    int idx = 0;
    int grp_y = 0;
    int groupIndex = 0;
    while (grp && grp_y + grp->height < dirtyRect.origin.y) {
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        groupIndex++;
    }

    int cursor = [self.dataModel cursor];
    DdbListviewRow_t cursor_it = [self.dataModel invalidRow];
    if (cursor != -1) {
        cursor_it = [self.dataModel rowForIndex:cursor];
    }

    int title_height = [self grouptitle_height];

    BOOL focused = self == self.window.firstResponder;

    int dragIdx = -1;
    if (_draggingInView) {
        dragIdx = [self dragInsertPointForYPos:_lastDragLocation.y];
    }

    while (grp && grp_y < clip_y + clip_h) {
        DdbListviewRow_t it = grp->head;
        [self.dataModel refRow:it];

        if (clip_y <= grp_y + title_height) {
            // draw group title
            if (title_height > 0) {
                [self drawGroupTitle:grp grp_y:grp_y title_height:title_height];
            }
        }


        int ii = 0;
        for (int i = 0, yy = grp_y + title_height; it && i < grp->num_items && yy < clip_y + clip_h; i++, yy += rowheight) {
            ii++;

            if (yy + rowheight >= clip_y) {
                // draw row
                NSColor *clr = [NSColor.controlAlternatingRowBackgroundColors objectAtIndex:ii % 2];
                [clr set];
                [NSBezierPath fillRect:NSMakeRect(dirtyRect.origin.x, yy, dirtyRect.size.width, rowheight)];

                int x = 0;
                for (DdbListviewCol_t col = [self.delegate firstColumn]; col != [self.delegate invalidColumn]; col = [self.delegate nextColumn:col]) {
                    int w = [self.delegate columnWidth:col];
                    if (CGRectIntersectsRect(dirtyRect, NSMakeRect(x, yy, w, rowheight))) {
                        [self.delegate drawCell:idx+i forRow: it forColumn:col inRect:NSMakeRect(x, yy, w, rowheight-1) focused:focused];
                    }
                    x += w;
                }

                if (x < dirtyRect.size.width) {
                    [self.delegate drawCell:idx+i forRow:it forColumn:[self.delegate invalidColumn] inRect:NSMakeRect(x, yy, dirtyRect.size.width-x, rowheight-1) focused:focused];
                }

                if (it == cursor_it) {
                    [[NSGraphicsContext currentContext] saveGraphicsState];
                    NSRect rect = NSMakeRect(self.frame.origin.x+0.5, yy+0.5, self.frame.size.width-1, rowheight-1);
                    NSBezierPath.defaultLineWidth = 1.f;
                    [NSColor.textColor set];
                    [NSBezierPath strokeRect:rect];
                    [[NSGraphicsContext currentContext] restoreGraphicsState];
                }

                // draw dnd line
                if (dragIdx != -1 && dragIdx == idx + i) {
                    [self drawLineIndicator:dirtyRect yy:yy];
                }

            }
            DdbListviewRow_t next = [self.dataModel nextRow:it];
            [self.dataModel unrefRow:it];
            it = next;
            if (it == [self.dataModel invalidRow]) {
                break; // sanity check, in case groups were not rebuilt yet
            }
        }
        if (it != [self.dataModel invalidRow]) {
            [self.dataModel unrefRow:it];
        }

        // draw album art
        int grp_next_y = grp_y + grp->height;
        [self renderAlbumArtForGroup:grp groupIndex:groupIndex isPinnedGroup:NO nextGroupCoord:grp_next_y yPos:grp_y + title_height viewportY:dirtyRect.origin.y clipRegion:dirtyRect];

        idx += grp->num_items;
        grp_y += grp->height;
        grp = grp->next;
        groupIndex++;
    }

//    if (pin_grp) {
//        self.pinnedGroupTitleView.hidden = NO;
//    }
//    else {
//        self.pinnedGroupTitleView.hidden = NO;
//    }
//        [self drawGroupTitle:pin_grp grp_y:pinnedGrpPos title_height:title_height];

    if (cursor_it != [self.dataModel invalidRow]) {
        [self.dataModel unrefRow:cursor_it];
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    // we always need to draw the list in the entire visible area,
    // so we get the full size from scrollview, and patch the clip rect
    [NSGraphicsContext saveGraphicsState];

    [self drawListView:dirtyRect];

    // draw rows below the real list
    if ([self fullheight] < dirtyRect.origin.y + dirtyRect.size.height) {
        int y = [self fullheight];
        int ii = [self.dataModel rowCount]+1;
        while (y < dirtyRect.origin.y + dirtyRect.size.height) {
            if (y + rowheight >= dirtyRect.origin.y) {
                NSColor *clr = [NSColor.controlAlternatingRowBackgroundColors objectAtIndex:ii % 2];
                [clr set];
                [NSBezierPath fillRect:NSMakeRect(dirtyRect.origin.x, y, dirtyRect.size.width, rowheight)];
            }
            y += rowheight;
            ii++;
        }
    }

#if 0 // draw random colored overlay, to see what's been repainted
    NSColor *clr = [NSColor colorWithDeviceRed:rand()/(float)RAND_MAX green:rand()/(float)RAND_MAX blue:rand()/(float)RAND_MAX alpha:0.5f];
    [clr set];
    [NSBezierPath fillRect:NSMakeRect(dirtyRect.origin.x, dirtyRect.origin.y, dirtyRect.size.width, dirtyRect.size.height)];
#endif

    [NSGraphicsContext restoreGraphicsState];
}

- (BOOL)isFlipped {
    return YES;
}

#pragma mark - Context menu

- (NSMenu *)menuForEvent:(NSEvent *)event {
    if ((event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeLeftMouseDown)
        && (event.buttonNumber == 1
            || (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagControl))))
    {
        if (event.buttonNumber == 0) {
            // ctrl+click blocks the mouseDown handler, do it now
            [self mouseDown:event];
        }

        return [self.delegate contextMenuForEvent:event forView:self];
    }
    return nil;
}

#pragma mark - Event handling

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self mouseDown:theEvent];
    [super rightMouseDown:theEvent];
}

- (void)mouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self];

    [self groupCheck];

    if (![self.dataModel rowCount]) {
        return;
    }

    DdbListviewGroup_t *grp;
    int grp_index;
    int sel;
    NSPoint convPt = [self convertPoint:[event locationInWindow] fromView:nil];
    self.lastpos = convPt;

    if (-1 == [self pickPoint:convPt.y group:&grp groupIndex:&grp_index index:&sel]) {
        [self deselectAll];
        return;
    }

    int cursor = [self.dataModel cursor];

    if (event.clickCount == 2 && event.buttonNumber == 0) {
        if (sel != -1 && cursor != -1) {
            [self.dataModel activate:cursor];
        }
        return;
    }

    int prev = cursor;
    if (sel != -1) {
        self.dataModel.cursor = sel;
        DdbListviewRow_t it = [self.dataModel rowForIndex:sel];
        if (it) {
            [self drawRow:sel];
            [self.dataModel unrefRow:it];
        }
        self.shift_sel_anchor = [self.dataModel cursor];
    }

    // single selection
    if (event.buttonNumber != 0 || !(event.modifierFlags & (NSEventModifierFlagCommand|NSEventModifierFlagShift))) {
        [self clickSelection:convPt grp:grp grp_index:grp_index sel:sel dnd:YES button:1];
    }
    else if (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagCommand)) {
        // toggle selection
        if (sel != -1) {
            DdbListviewRow_t it = [self.dataModel rowForIndex:sel];
            if (it != [self.dataModel invalidRow]) {
                [self.dataModel selectRow:it withState:![self.dataModel rowSelected:it]];
                [self drawRow:sel];
                [self.delegate selectionChanged:it];
                [self.dataModel unrefRow:it];
            }
        }
    }
    else if (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagShift)) {
        // select range
        int sel_cursor = sel;
        if (sel_cursor == -1) {
            // find group
            DdbListviewGroup_t *g = self.groups;
            int idx = 0;
            while (g) {
                if (g == grp) {
                    sel_cursor = idx - 1;
                    break;
                }
                idx += g->num_items;
                g = g->next;
            }
        }
        int start = MIN (prev, sel_cursor);
        int end = MAX (prev, sel_cursor);
        int idx = 0;
        for (DdbListviewRow_t it = [self.dataModel firstRow]; it != [self.dataModel invalidRow]; idx++) {
            if (idx >= start && idx <= end) {
                if (![self.dataModel rowSelected:it]) {
                    [self.dataModel selectRow:it withState:YES];
                    [self drawRow:idx];
                    [self.delegate selectionChanged:it];
                }
            }
            else {
                if ([self.dataModel rowSelected:it]) {
                    [self.dataModel selectRow:it withState:NO];
                    [self drawRow:idx];
                    [self.delegate selectionChanged:it];
                }
            }
            DdbListviewRow_t next = [self.dataModel nextRow:it];
            [self.dataModel unrefRow:it];
            it = next;
        }
    }
    cursor = [self.dataModel cursor];
    if (cursor != -1 && sel == -1) {
        [self drawRow:cursor];
    }
    if (prev != -1 && prev != cursor) {
        [self drawRow:prev];
    }
}

- (void)mouseUp:(NSEvent *)event
{
    if (_dragwait) {
        _dragwait = NO;
        DdbListviewGroup_t *grp;
        int grp_index;
        int sel;
        NSPoint convPt = [self convertPoint:[event locationInWindow] fromView:nil];
        if (![self pickPoint:convPt.y group:&grp groupIndex:&grp_index index:&sel]) {
            [self selectSingle:sel];
            self.needsDisplay = YES;
        }
        else {
            self.dataModel.cursor = -1;
            DdbListviewRow_t it = [self.dataModel firstRow];
            int idx = 0;
            while (it != [self.dataModel invalidRow]) {
                if ([self.dataModel rowSelected:it]) {
                    [self.dataModel selectRow:it withState:NO];
                    [self drawRow:idx];
                    [self.delegate selectionChanged:it];
                }
                DdbListviewRow_t next = [self.dataModel nextRow:it];
                [self.dataModel unrefRow:it];
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

- (void)mouseDragged:(NSEvent *)event
{
    NSPoint dragLocation;
    dragLocation=[self convertPoint:[event locationInWindow]
                           fromView:nil];

    [self listMouseDragged:event];

    // support automatic scrolling during a drag
    // by calling NSView's autoscroll: method
    [self autoscroll:event];

    // act on the drag as appropriate to the application
}

- (void)listMouseDragged:(NSEvent *)event {
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
    if (_dragwait) {
        if (fabs (_lastpos.x - pt.x) > 3 || fabs (_lastpos.y - pt.y) > 3) {
            // begin dnd
            NSPasteboard *pboard;

            // Need playlist identifier and all playlist items when dragging internally,
            // this is represented with the DdbListviewLocalDragDropHolder interface

            pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            PlaylistLocalDragDropHolder *data = [[PlaylistLocalDragDropHolder alloc] initWithSelectedItemsOfPlaylist:plt];
            deadbeef->plt_unref (plt);
            [pboard declareTypes:[NSArray arrayWithObject:ddbPlaylistItemsUTIType]  owner:self];
            [pboard clearContents];
            if (![pboard writeObjects:[NSArray arrayWithObject:data]])
                NSLog(@"Unable to write to pasteboard.");

            NSImage *img = [NSImage imageNamed:NSImageNameMultipleDocuments];

            NSPoint dpt = pt;
            dpt.x -= [img size].width/2;
            dpt.y += [img size].height;

            [self dragImage:img at:dpt offset:NSMakeSize(0.0, 0.0) event:event pasteboard:pboard source:self slideBack:YES];
            _dragwait = NO;

        }
    }
    else if (_areaselect) {
        DdbListviewGroup_t *grp = NULL;
        int grp_index;
        int sel;
        if ([self pickPoint:pt.y group:&grp groupIndex:&grp_index index:&sel] == -1) {
            // past playlist bounds -> set to last track
            if (pt.y < 0) {
                sel = 0;
            }
            else {
                sel = [self.dataModel rowCount] - 1;
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
        int prev = [self.dataModel cursor];
        if (sel != -1) {
            self.dataModel.cursor = sel;
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
            int start = MIN (y, _shift_sel_anchor);
            int end = MAX (y, _shift_sel_anchor);

            int nchanged = 0;

            // don't touch anything in process_start/end range
            int process_start = MIN (start, _area_selection_start);
            int process_end = MAX (end, _area_selection_end);
#define NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW 10
            idx=process_start;
            DdbListviewRow_t it = [self.dataModel rowForIndex:idx];
            for (; it && idx <= process_end; idx++) {
                int selected = [self.dataModel rowSelected:it];
                if (idx >= start && idx <= end) {
                    if (!selected) {
                        [self.dataModel selectRow:it withState:YES];
                        nchanged++;
                        if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                            [self drawRow:idx];
                            [self.delegate selectionChanged:it];
                        }
                    }
                }
                else if (selected) {
                    [self.dataModel selectRow:it withState:NO];
                    nchanged++;
                    if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                        [self drawRow:idx];
                        [self.delegate selectionChanged:it];
                    }
                }
                DdbListviewRow_t next = [self.dataModel nextRow:it];
                [self.dataModel unrefRow:it];
                it = next;
            }
            if (it != [self.dataModel invalidRow]) {
                [self.dataModel unrefRow:it];
            }
            if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                self.needsDisplay = YES;
                [self.delegate selectionChanged:it]; // that means "selection changed a lot, redraw everything
            }
            _area_selection_start = start;
            _area_selection_end = end;
        }
        if (sel != -1 && sel != prev) {
            if (prev != -1) {
                DdbListviewRow_t it = [self.dataModel rowForIndex:prev];
                if (it != [self.dataModel invalidRow]) {
                    [self drawRow:prev];
                    [self.dataModel unrefRow:it];
                }
            }
            DdbListviewRow_t it = [self.dataModel rowForIndex:sel];
            if (it != [self.dataModel invalidRow]) {
                [self drawRow:sel];
                [self.dataModel unrefRow:it];
            }
        }
    }
}

- (void)clickSelection:(NSPoint)pt grp:(DdbListviewGroup_t *)grp grp_index:(int)grp_index sel:(int)sel dnd:(BOOL)dnd button:(int)button {

    _areaselect = 0;
    [self groupCheck];

    // clicked album art column?
    int album_art_column = 0; // FIXME

    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];

    if (sel == -1 && !album_art_column && (!grp || (pt.y > _grouptitle_height && grp_index >= grp->num_items))) {
        // clicked empty space, deselect everything
        [self deselectAll];
    }
    else if ((sel != -1 && grp && grp_index == -1) || (pt.y <= _grouptitle_height + vis.origin.y && [self.delegate pinGroups]) || album_art_column) {
        // clicked group title, select group
        DdbListviewRow_t it;
        int idx = 0;
        int cnt = -1;
        for (it = [self.dataModel firstRow]; it != [self.dataModel invalidRow]; idx++) {
            if (it == grp->head) {
                cnt = grp->num_items;
                cnt = grp->num_items;
            }
            if (cnt > 0) {
                if (![self.dataModel rowSelected:it]) {
                    [self.dataModel selectRow:it withState:YES];
                    [self drawRow:idx];
                    [self.delegate selectionChanged:it];
                }
                cnt--;
            }
            else {
                if ([self.dataModel rowSelected:it]) {
                    [self.dataModel selectRow:it withState:NO];
                    [self drawRow:idx];
                    [self.delegate selectionChanged:it];
                }
            }
            DdbListviewRow_t next = [self.dataModel nextRow:it];
            [self.dataModel unrefRow:it];
            it = next;
        }
    }
    else {
        // clicked specific item - select, or start drag-n-drop
        DdbListviewRow_t it = [self.dataModel rowForIndex:sel];
        if (it == [self.dataModel invalidRow] || ![self.dataModel rowSelected:it]
            || (![self.delegate hasDND] && button == 1)) // HACK: don't reset selection by right click in search window
        {
            // reset selection, and set it to single item
            [self selectSingle:sel];
            self.needsDisplay = YES;
            if (dnd) {
                _areaselect = 1;
                _areaselect_y = pt.y;
                _shift_sel_anchor = [self.dataModel cursor];
            }
        }
        else if (dnd) {
            _dragwait = YES;
        }
        [self.dataModel unrefRow:it];
    }
}

- (void)selectSingle:(int)sel {

    DdbListviewRow_t sel_it = [self.dataModel rowForIndex:sel];
    if (sel_it == [self.dataModel invalidRow]) {
        return;
    }

    [self.dataModel deselectAll];
    [self.dataModel selectRow:sel_it withState:YES];
    [self.dataModel unrefRow:sel_it];

    [self.delegate selectionChanged:[self.dataModel invalidRow]];

    _area_selection_start = sel;
    _area_selection_end = sel;
}

- (int)dragInsertPointForYPos:(CGFloat)y {
    int idx = 0;
    int grp_y = 0;
    int gidx = 0;
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    while (grp) {
        int h = grp->height;
        if (y >= grp_y - _grouptitle_height/2 && y < grp_y + h - rowheight/2) {
            y -= grp_y;
            // over title
            if (y < _grouptitle_height) {
                return idx;
            }
            // within group
            else if (y < _grouptitle_height + grp->num_items * rowheight - rowheight/2) {
                return idx + (int)((y - _grouptitle_height + rowheight/2) / rowheight);
            }
            // just before the next group
            else {
                return idx + grp->num_items;
            }
        }
        grp_y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
        gidx++;
    }
    return -1;
}

- (int)pickPoint:(CGFloat)y group:(DdbListviewGroup_t **)group groupIndex:(int *)group_idx index:(int *)global_idx {
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
                *group_idx = (int)((y - _grouptitle_height) / rowheight);
                *global_idx = -1;
            }
            else {
                *group_idx = (int)((y - _grouptitle_height) / rowheight);
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

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)theEvent {
    NSString *theArrow = [theEvent charactersIgnoringModifiers];
    unichar keyChar = 0;
    if ( [theArrow length] == 0 )
        return;            // reject dead keys
    if ( [theArrow length] == 1 ) {

        int prev = [self.dataModel cursor];
        int cursor = prev;

        NSScrollView *sv = self.enclosingScrollView;
        NSRect vis = [sv documentVisibleRect];
        keyChar = [theArrow characterAtIndex:0];

        switch (keyChar) {
            case NSDownArrowFunctionKey:
                if (theEvent.modifierFlags & NSEventModifierFlagCommand) {
                    cursor = [self.dataModel rowCount]-1;
                }
                else {
                    if (cursor < [self.dataModel rowCount]-1) {
                        cursor++;
                    }
                }
                break;
            case NSUpArrowFunctionKey:
                if (theEvent.modifierFlags & NSEventModifierFlagCommand) {
                    cursor = 0;
                }
                else {
                    if (cursor > 0) {
                        cursor--;
                    }
                    else if (cursor < 0 && [self.dataModel rowCount] > 0) {
                            cursor = 0;
                    }
                }
                break;
            case NSPageDownFunctionKey: {
                [self scrollPoint:NSMakePoint(vis.origin.x, vis.origin.y + vis.size.height - rowheight)];
                break;
            }
            case NSPageUpFunctionKey:
                [self scrollPoint:NSMakePoint(vis.origin.x, vis.origin.y - vis.size.height + rowheight)];
                break;
            case NSHomeFunctionKey:
                [self scrollPoint:NSMakePoint(vis.origin.x, 0)];
                break;
            case NSEndFunctionKey:
                [self scrollPoint:NSMakePoint(vis.origin.x, (self.frame.size.height - sv.contentSize.height))];
                break;
            default:
                [super keyDown:theEvent];
                return;
        }

        if ([theEvent modifierFlags] & NSEventModifierFlagShift) {
            if (cursor != prev) {
                self.dataModel.cursor = cursor;
                self.scrollForPos = [self rowPosForIndex:cursor];
                // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
                int start = MIN (cursor, _shift_sel_anchor);
                int end = MAX (cursor, _shift_sel_anchor);

                int nchanged = 0;
                int idx = 0;
                DdbListviewRow_t it;
                for (it = [self.dataModel firstRow]; it != [self.dataModel invalidRow]; idx++) {
                    if (idx >= start && idx <= end) {
                        [self.dataModel selectRow:it withState:YES];
                        if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                            [self drawRow:idx];
                            [self.delegate selectionChanged:it];
                        }
                    }
                    else if ([self.dataModel rowSelected:it]) {
                        [self.dataModel selectRow:it withState:NO];
                        if (nchanged < NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                            [self drawRow:idx];
                            [self.delegate selectionChanged:it];
                        }
                    }
                    DdbListviewRow_t next = [self.dataModel nextRow:it];
                    [self.dataModel unrefRow:it];
                    it = next;
                }
                if (nchanged >= NUM_CHANGED_ROWS_BEFORE_FULL_REDRAW) {
                    [self.delegate selectionChanged:[self.dataModel invalidRow]];
                }
            }
        }
        else if (prev != cursor) {
            _shift_sel_anchor = cursor;
            [self setCursor:cursor noscroll:NO];
        }
    }
}

- (void)deselectAll {
    DdbListviewRow_t it;
    int idx = 0;
    for (it = [self.dataModel firstRow]; it != [self.dataModel invalidRow]; idx++) {
        if ([self.dataModel rowSelected:it]) {
            [self.dataModel selectRow:it withState:NO];
            [self drawRow:idx];
            [self.delegate selectionChanged:it];
        }
        DdbListviewRow_t next = [self.dataModel nextRow:it];
        [self.dataModel unrefRow:it];
        it = next;
    }
}

- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll {
    self.dataModel.cursor = cursor;

    DdbListviewRow_t row = [self.dataModel rowForIndex:cursor];
    if (row != [self.dataModel invalidRow] && ![self.dataModel rowSelected:row]) {
        [self selectSingle:cursor];
    }
    if (row != [self.dataModel invalidRow]) {
        [self.dataModel unrefRow:row];
    }

    if (!noscroll) {
        self.scrollForPos = [self rowPosForIndex:cursor];
    }
    self.needsDisplay = YES;
}

#pragma mark - Grouping

- (void)freeGroups {
    while (_groups) {
        if (_groups->head != [self.dataModel invalidRow]) {
            [self.dataModel unrefRow:_groups->head];
        }
        DdbListviewGroup_t *next = _groups->next;
        free (_groups);
        _groups = next;
    }
}

- (void)initGroups {
    self.groups_build_idx = [self.dataModel modificationIdx];

    [self freeGroups];

    _fullwidth = 0;
    _fullheight = 0;
    DdbListviewGroup_t *grp = NULL;

    NSString *str;
    NSString *curr;

    int min_height= 0;
    for (DdbListviewCol_t c = [self.delegate firstColumn]; c != [self.delegate invalidColumn]; c = [self.delegate nextColumn:c]) {
        if ([self.delegate columnMinHeight:c] && [self.delegate columnWidth:c] > min_height) {
            min_height = [self.delegate columnGroupHeight:c];
        }
        _fullwidth += [self.delegate columnWidth:c];
    }

    _grouptitle_height = grouptitleheight;

    deadbeef->pl_lock ();
    int idx = 0;
    DdbListviewRow_t it = [self.dataModel firstRow];
    while (it != [self.dataModel invalidRow]) {
        curr = [self.delegate rowGroupStr:it];

        if (!curr) {
            _grouptitle_height = 0;
        }

        if (!grp || (!curr && grp->num_items >= BLANK_GROUP_SUBDIVISION) || (curr && [str isNotEqualTo:curr])) {
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
            [self.dataModel refRow:it];
            grp->num_items = 0;
            grp->height = _grouptitle_height;
        }
        grp->height += rowheight;
        grp->num_items++;
        DdbListviewRow_t next = [self.dataModel nextRow:it];
        [self.dataModel unrefRow:it];
        it = next;
        idx++;
    }
    deadbeef->pl_unlock ();
    if (it != [self.dataModel invalidRow]) {
        [self.dataModel unrefRow:it];
    }
    if (grp) {
        if (grp->height - _grouptitle_height < min_height) {
            grp->height = min_height + _grouptitle_height;
        }
        _fullheight += grp->height;
    }
    [self updateContentFrame];
}

- (void)groupCheck {
    if ([self.dataModel modificationIdx] != self.groups_build_idx) {
        [self initGroups];
    }
}

- (void)reloadData {
    [self initGroups];
    self.needsDisplay = YES;
}

#pragma mark Group drawing

- (int)getDrawInfo:(int)row group:(DdbListviewGroup_t **)pgrp isEven:(int *)even cursor:(int *)cursor group_y:(int *)group_y rect:(NSRect *)rect {
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    int idx = 0;
    int idx2 = 0;
    rect->origin.y = 0;

    int totalwidth = 0;
    for (DdbListviewCol_t col = [self.delegate firstColumn]; col != [self.delegate invalidColumn]; col = [self.delegate nextColumn:col]) {
        totalwidth += [self.delegate columnWidth:col];
    }

    while (grp) {
        int grpheight = grp->height;
        if (idx <= row && idx + grp->num_items > row) {
            // found
            int idx_in_group = row - idx;
            *pgrp = grp;
            *even = (idx2 + 1 + idx_in_group) & 1;
            *cursor = (row == [self.dataModel cursor]) ? 1 : 0;
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

    if (rect.origin.y > self.bounds.origin.y + self.bounds.size.height) {
        return;
    }

    NSScrollView *sv = self.enclosingScrollView;
    NSRect vis = [sv documentVisibleRect];
    rect.origin.x = vis.origin.x;
    rect.size.width = vis.size.width;

    self.needsDisplayInRect = rect;
}

- (void)drawGroup:(int)idx {
    int i = 0;
    int y = 0;
    for (DdbListviewGroup_t *grp = _groups; grp; grp = grp->next) {
        if (idx == i) {
            NSScrollView *sv = self.enclosingScrollView;
            NSRect vis = [sv documentVisibleRect];
            NSRect rect = NSMakeRect(vis.origin.x, y, vis.size.width, grp->height);
            self.needsDisplayInRect = rect;
            break;
        }
        i++;
        y += grp->height;
    }
}

#pragma mark - Navigation

// returns YES if scroll has occured as result of changing the cursor position
- (BOOL)setScrollForPos:(int)pos {
    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    CGFloat scrollpos = vis.origin.y;
    CGFloat newscroll = scrollpos;

    if (![self.delegate pinGroups] && pos < scrollpos) {
        newscroll = pos;
    }
    else if ([self.delegate pinGroups] && pos < scrollpos + _grouptitle_height) {
        newscroll = pos - _grouptitle_height;
    }
    else if (pos + rowheight >= scrollpos + vis.size.height) {
        newscroll = pos + rowheight - vis.size.height;
        if (newscroll < 0) {
            newscroll = 0;
        }
    }
    if (scrollpos != newscroll) {
        [self scrollPoint:NSMakePoint(vis.origin.x, newscroll)];
        return YES;
    }
    return NO;
}

- (int)rowPosForIndex:(int)row_idx {
    int y = 0;
    int idx = 0;
    [self groupCheck];
    DdbListviewGroup_t *grp = _groups;
    while (grp) {
        if (idx + grp->num_items > row_idx) {
            int i = y + _grouptitle_height + (row_idx - idx) * rowheight;
            return i;
        }
        y += grp->height;
        idx += grp->num_items;
        grp = grp->next;
    }
    return y;
}

- (void)scrollToRowWithIndex:(int)idx {
    int pos = [self rowPosForIndex:idx];
    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];

    if (pos < vis.origin.y || pos + rowheight >= vis.origin.y + vis.size.height) {
        [self scrollPoint:NSMakePoint(vis.origin.x, pos - vis.size.height/2)];
    }
}

- (void)scrollVerticalPosition:(CGFloat)verticalPosition {
    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    [self scrollPoint:NSMakePoint(vis.origin.x, verticalPosition)];
}

- (void)updateContentFrame {
    _fullwidth = 0;
    for (DdbListviewCol_t c = [self.delegate firstColumn]; c != [self.delegate invalidColumn]; c = [self.delegate nextColumn:c]) {
        _fullwidth += [self.delegate columnWidth:c];
    }

    if (!self.widthConstraint) {
        self.widthConstraint = [self.widthAnchor constraintGreaterThanOrEqualToConstant:_fullwidth];
        self.heightConstraint = [self.heightAnchor constraintGreaterThanOrEqualToConstant:_fullheight];
        self.widthConstraint.priority = NSLayoutPriorityDefaultHigh;
        self.heightConstraint.priority = NSLayoutPriorityDefaultHigh;
        self.widthConstraint.active = YES;
        self.heightConstraint.active = YES;
    }
    else {
        self.widthConstraint.constant = _fullwidth;
        self.heightConstraint.constant = _fullheight;
    }

    [self updatePinnedGroup];
}


@end

