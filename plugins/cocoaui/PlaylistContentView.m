//
//  PlaylistContentView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "PlaylistContentView.h"
#import "PlaylistView.h"
#import "DdbShared.h"
#import "PlaylistLocalDragDropHolder.h"
#include "../../deadbeef.h"

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
@property (nonatomic) int areaselect_y;
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


@end

@implementation PlaylistContentView

- (instancetype)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];

    self.groups_build_idx = -1;

    [self registerForDraggedTypes:[NSArray arrayWithObjects:ddbPlaylistItemsUTIType, NSFilenamesPboardType, nil]];

    return self;
}

- (void)dealloc
{
    [self freeGroups];
}

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

    DdbListviewGroup_t *grp;
    int grp_index;
    int sel;

    NSPoint draggingLocation = [self convertPoint:[sender draggingLocation] fromView:nil];
    id<DdbListviewDelegate> delegate = self.delegate;

    DdbListviewRow_t row = [delegate invalidRow];
    if ( -1 != [self pickPoint:draggingLocation.y group:&grp groupIndex:&grp_index index:&sel]) {
        row = [delegate rowForIndex:sel];
    }

    if ( [[pboard types] containsObject:ddbPlaylistItemsUTIType ] ) {
        NSArray *classes = [[NSArray alloc] initWithObjects:[PlaylistLocalDragDropHolder class], nil];
        NSDictionary *options = [NSDictionary dictionary];
        NSArray *draggedItems = [pboard readObjectsForClasses:classes options:options];

        PlaylistLocalDragDropHolder *holder = [draggedItems firstObject];
        NSInteger from_playlist = holder.playlistIdx;
        uint32_t * indices = malloc (sizeof (uint32_t *) * holder.count);
        int i = 0;
        for (NSNumber * number in holder.itemsIndices) {
            indices[i] = [number unsignedIntValue];
            ++i;
        }

        int length = holder.count;

        NSDragOperation op = sender.draggingSourceOperationMask;
        [delegate dropItems:(int)from_playlist before:row indices:indices count:length copy:op==NSDragOperationCopy];
        free(indices);
    }
    else if ( [[pboard types] containsObject:NSFilenamesPboardType] ) {

        NSArray *paths = [pboard propertyListForType:NSFilenamesPboardType];
        if (row != [delegate invalidRow]) {
            // add before selected row
            [delegate externalDropItems:paths after: [delegate rowForIndex:sel-1] ];
        }
        else {
            // no selected row, add to end
            DdbListviewRow_t lastRow = [delegate rowForIndex:([delegate rowCount]-1)];
            [delegate externalDropItems:paths after: lastRow];
        }
    }

    if (row != [delegate invalidRow]) {
        [delegate unrefRow:row];
    }
    _draggingInView = NO;
    return YES;
}

- (void)renderAlbumArtForGroup:(DdbListviewGroup_t *)grp
                    groupIndex:(int)groupIndex
                 isPinnedGroup:(BOOL)isPinnedGroup
                nextGroupCoord:(int)grp_next_y
                          yPos:(int)y
                     viewportY:(int)viewportY
                    clipRegion:(NSRect)clip {
    int x = 0;
    id<DdbListviewDelegate> delegate = self.delegate;

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
                [delegate drawAlbumArtForGroup:grp groupIndex:groupIndex inColumn:col isPinnedGroup:isPinnedGroup nextGroupCoord:grp_next_y xPos:x yPos:y viewportY:viewportY width:w height:grp->height];
            }
        }
        x += w;
    }
}


- (void) drawLineIndicator:(NSRect)dirtyRect yy:(int)yy  {

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

- (void)drawListView:(NSRect)dirtyRect {
    id<DdbListviewDelegate> delegate = self.delegate;

    [self groupCheck];

    DdbListviewGroup_t *grp = [self groups];

    int clip_y = dirtyRect.origin.y;
    int clip_h = dirtyRect.size.height;

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
    DdbListviewGroup_t *pin_grp = [delegate pinGroups] && grp && grp_y < dirtyRect.origin.y && grp_y + grp->height >= dirtyRect.origin.y ? grp : NULL;

    int cursor = [delegate cursor];
    DdbListviewRow_t cursor_it = [delegate invalidRow];
    if (cursor != -1) {
        cursor_it = [delegate rowForIndex:cursor];
    }

    int title_height = [self grouptitle_height];

    BOOL focused = [self.window isKeyWindow];

    while (grp && grp_y < clip_y + clip_h) {
        DdbListviewRow_t it = grp->head;
        [self.delegate refRow:it];

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
                    [self.delegate drawCell:idx+i forRow:it forColumn:[delegate invalidColumn] inRect:NSMakeRect(x, yy, dirtyRect.size.width-x, rowheight-1) focused:focused];
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
                if (_draggingInView) {
                    if ( _lastDragLocation.y > self.fullheight ) {
                        [self drawLineIndicator:dirtyRect yy: self.fullheight];
                    }
                    else if ( _lastDragLocation.y > yy && _lastDragLocation.y < yy + rowheight ) {
                        [self drawLineIndicator:dirtyRect yy:yy];
                    }
                }

            }
            DdbListviewRow_t next = [self.delegate nextRow:it];
            [self.delegate unrefRow:it];
            it = next;
            if (it == [delegate invalidRow]) {
                break; // sanity check, in case groups were not rebuilt yet
            }
        }
        if (it != [delegate invalidRow]) {
            [self.delegate unrefRow:it];
        }

        // draw album art
        int grp_next_y = grp_y + grp->height;
        [self renderAlbumArtForGroup:grp groupIndex:groupIndex isPinnedGroup:pin_grp==grp nextGroupCoord:grp_next_y yPos:grp_y + title_height viewportY:dirtyRect.origin.y clipRegion:dirtyRect];

        #define min(x,y) ((x)<(y)?(x):(y))
        if (pin_grp == grp && clip_y-dirtyRect.origin.y <= title_height) {
            // draw pinned group title
            // scrollx, 0, total_width, min(title_height, grp_next_y)
            NSRect groupRect = NSMakeRect(0, dirtyRect.origin.y, self.frame.size.width, min (title_height, grp_next_y));
            NSColor *clr = [NSColor.controlAlternatingRowBackgroundColors objectAtIndex:0];
            [clr set];
#if DEBUG_DRAW_GROUP_TITLES
            [NSColor.redColor set];
#endif

            [NSBezierPath fillRect:groupRect];
            if (title_height > 0) {
                // scrollx, min(0, grp_next_y-title_height), total_width, title_height
                groupRect.origin.y = min (dirtyRect.origin.y, grp_next_y-title_height);
                groupRect.size.height = title_height;
                [delegate drawGroupTitle:grp->head inRect:groupRect];
            }
        }
        else if (clip_y <= grp_y + title_height) {
            // draw normal group title
            if (title_height > 0) {
                // scrollx, grp_y, total_width, title_height
                NSRect groupRect = NSMakeRect(0, grp_y, self.frame.size.width, title_height);
                NSColor *clr = [NSColor.controlAlternatingRowBackgroundColors objectAtIndex:0];
                [clr set];
#if DEBUG_DRAW_GROUP_TITLES
                [NSColor.greenColor set];
#endif
                [NSBezierPath fillRect:groupRect];
                [delegate drawGroupTitle:grp->head inRect:groupRect];
            }
        }

        idx += grp->num_items;
        grp_y += grp->height;
        grp = grp->next;
        groupIndex++;
    }

    if (cursor_it != [delegate invalidRow]) {
        [delegate unrefRow:cursor_it];
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
        int ii = [self.delegate rowCount]+1;
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

- (NSMenu *)menuForEvent:(NSEvent *)event {
    if ((event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeLeftMouseDown)
        && (event.buttonNumber == 1
            || (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagControl))))
    {
        if (event.buttonNumber == 0) {
            // ctrl+click blocks the mouseDown handler, do it now
            [self mouseDown:event];
        }

        id<DdbListviewDelegate> delegate = self.delegate;
        return [delegate contextMenuForEvent:event forView:self];
    }
    return nil;
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self mouseDown:theEvent];
    [super rightMouseDown:theEvent];
}

- (void)mouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self];

    [self groupCheck];

    id<DdbListviewDelegate> delegate = self.delegate;

    if (![delegate rowCount]) {
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

    int cursor = [delegate cursor];

    if (event.clickCount == 2 && event.buttonNumber == 0) {
        if (sel != -1 && cursor != -1) {
            [delegate activate:cursor];
        }
        return;
    }

    int prev = cursor;
    if (sel != -1) {
        // if (rowspan) {
        //     sel -= grp_index;
        // }

        delegate.cursor = sel;
        DdbListviewRow_t it = [delegate rowForIndex:sel];
        if (it) {
            [self drawRow:sel];
            [delegate unrefRow:it];
        }
        self.shift_sel_anchor = [delegate cursor];
    }

    // single selection
    if (event.buttonNumber != 0 || !(event.modifierFlags & (NSEventModifierFlagCommand|NSEventModifierFlagShift))) {
        [self clickSelection:convPt grp:grp grp_index:grp_index sel:sel dnd:YES button:1];
    }
    else if (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagCommand)) {
        // toggle selection
        if (sel != -1) {
            DdbListviewRow_t it = [delegate rowForIndex:sel];
            if (it != [delegate invalidRow]) {
                [delegate selectRow:it withState:![delegate rowSelected:it]];
                [self drawRow:sel];
                [delegate selectionChanged:it];
                [delegate unrefRow:it];
            }
        }
    }
    else if (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagShift)) {
        // select range
        int cursor = sel;
        if (cursor == -1) {
            // find group
            DdbListviewGroup_t *g = self.groups;
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
                    [self drawRow:idx];
                    [delegate selectionChanged:it];
                }
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
    cursor = [delegate cursor];
    if (cursor != -1 && sel == -1) {
        [self drawRow:cursor];
    }
    if (prev != -1 && prev != cursor) {
        [self drawRow:prev];
    }
}

- (void)mouseUp:(NSEvent *)event
{
    [self listMouseUp:event];
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
            PlaylistLocalDragDropHolder *data = [[PlaylistLocalDragDropHolder alloc] initWithSelectedPlaylistItems:plt];
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
            _delegate.cursor = sel;
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
                self.needsDisplay = YES;
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
    else if ((sel != -1 && grp && grp_index == -1) || (pt.y <= _grouptitle_height + vis.origin.y && [_delegate pinGroups]) || album_art_column) {
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
            self.needsDisplay = YES;
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

    DdbListviewRow_t sel_it = [_delegate rowForIndex:sel];
    if (sel_it == [_delegate invalidRow]) {
        return;
    }

    [_delegate deselectAll];
    [_delegate selectRow:sel_it withState:YES];
    [_delegate unrefRow:sel_it];

    [_delegate selectionChanged:[_delegate invalidRow]];

    _area_selection_start = sel;
    _area_selection_end = sel;
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

- (void)initGroups {
    self.groups_build_idx = [_delegate modificationIdx];

    [self freeGroups];

    _fullwidth = 0;
    _fullheight = 0;
    DdbListviewGroup_t *grp = NULL;

    NSString *str;
    NSString *curr;

    int min_height= 0;
    for (DdbListviewCol_t c = [_delegate firstColumn]; c != [_delegate invalidColumn]; c = [_delegate nextColumn:c]) {
        if ([_delegate columnMinHeight:c] && [_delegate columnWidth:c] > min_height) {
            min_height = [_delegate columnGroupHeight:c];
        }
        _fullwidth += [_delegate columnWidth:c];
    }

    _grouptitle_height = grouptitleheight;

    deadbeef->pl_lock ();
    int idx = 0;
    DdbListviewRow_t it = [_delegate firstRow];
    while (it != [_delegate invalidRow]) {
        curr = [_delegate rowGroupStr:it];

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
    deadbeef->pl_unlock ();
    if (it != [_delegate invalidRow]) {
        [_delegate unrefRow:it];
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
    if ([_delegate modificationIdx] != self.groups_build_idx) {
        [self initGroups];
    }
}

- (void)reloadData {
    [self initGroups];
    self.needsDisplay = YES;
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
            /*if (rect.origin.y < 0) {
                rect.size.height += rect.origin.y;
                rect.origin.y = 0;
            }
            if (rect.origin.y + rect.size.height >= vis.size.height) {
                rect.size.height = vis.size.height - rect.origin.y;
            }
            rect.origin.x = vis.origin.x;
            rect.size.width = vis.size.width;*/
            self.needsDisplayInRect = rect;
            break;
        }
        i++;
        y += grp->height;
    }
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


- (void)listMouseUp:(NSEvent *)event {
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
            _delegate.cursor = -1;
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
            return NSDragOperationCopy | NSDragOperationMove;
        case NSDraggingContextOutsideApplication:
            return NSDragOperationNone; // FIXME
    }
    return NSDragOperationNone;
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

        int prev = [_delegate cursor];
        int cursor = prev;

        NSScrollView *sv = self.enclosingScrollView;
        NSRect vis = [sv documentVisibleRect];
        keyChar = [theArrow characterAtIndex:0];

        switch (keyChar) {
            case NSDownArrowFunctionKey:
                if (theEvent.modifierFlags & NSEventModifierFlagCommand) {
                    cursor = [_delegate rowCount]-1;
                }
                else {
                    if (cursor < [_delegate rowCount]-1) {
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
                    else if (cursor < 0 && [_delegate rowCount] > 0) {
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
                _delegate.cursor = cursor;
                self.scrollForPos = [self rowPosForIndex:cursor];
                // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
                int start = MIN (cursor, _shift_sel_anchor);
                int end = MAX (cursor, _shift_sel_anchor);

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

- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll {
    _delegate.cursor = cursor;

    DdbListviewRow_t row = [_delegate rowForIndex:cursor];
    if (row != [_delegate invalidRow] && ![_delegate rowSelected:row]) {
        [self selectSingle:cursor];
    }
    if (row != [_delegate invalidRow]) {
        [_delegate unrefRow:row];
    }

    if (!noscroll) {
        self.scrollForPos = [self rowPosForIndex:cursor];
    }
    self.needsDisplay = YES;
}

// returns YES if scroll has occured as result of changing the cursor position
- (BOOL)setScrollForPos:(int)pos {
    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    int scrollpos = vis.origin.y;
    int newscroll = scrollpos;

    if (![_delegate pinGroups] && pos < scrollpos) {
        newscroll = pos;
    }
    else if ([_delegate pinGroups] && pos < scrollpos + _grouptitle_height) {
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

- (void)freeGroups {
    while (_groups) {
        if (_groups->head != [_delegate invalidRow]) {
            [_delegate unrefRow:_groups->head];
        }
        DdbListviewGroup_t *next = _groups->next;
        free (_groups);
        _groups = next;
    }
}

- (void)scrollVerticalPosition:(CGFloat)verticalPosition {
    NSScrollView *sv = [self enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    [self scrollPoint:NSMakePoint(vis.origin.x, verticalPosition)];
}

- (void)updateContentFrame {
    _fullwidth = 0;
    for (DdbListviewCol_t c = [_delegate firstColumn]; c != [_delegate invalidColumn]; c = [_delegate nextColumn:c]) {
        _fullwidth += [_delegate columnWidth:c];
    }

    NSScrollView *sv = [self enclosingScrollView];
    NSSize size = [sv contentSize];
    NSRect frame = self.frame;
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
    self.frame = frame;
}
@end

