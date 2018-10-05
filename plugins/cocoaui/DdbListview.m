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
#import "DdbShared.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

// data has to be serialized, so we code idx and not pointers
@interface DdbListviewLocalDragDropHolder : NSObject<NSPasteboardReading, NSPasteboardWriting, NSSecureCoding> {
    NSInteger _playlistIdx;
    NSArray *_itemsIndices;
}
- (DdbListviewLocalDragDropHolder *)initWithSelectedPlaylistItems:(ddb_playlist_t *)playlist;
@end

@implementation DdbListviewLocalDragDropHolder

// NSSecureCoding
@dynamic supportsSecureCoding;
+ (BOOL) supportsSecureCoding {
    return YES;
}

// NSCoding

- (instancetype)initWithCoder:(NSCoder *)aDecoder {

    self = [super init];
    if (self) {
        _playlistIdx = [aDecoder decodeIntegerForKey:@"Playlist"];
        _itemsIndices = [aDecoder decodeObjectOfClass:[NSMutableArray class] forKey:@"Items"];
    }

    return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {

    [aCoder encodeInteger:_playlistIdx forKey:@"Playlist"];
    [aCoder encodeObject:_itemsIndices forKey:@"Items"];
}


// NSPasteboardReading

+ (NSArray<NSString *> *)readableTypesForPasteboard:(NSPasteboard *)pasteboard {
    return [NSArray arrayWithObjects:ddbPlaylistItemsUTIType, nil];
}

+ (NSPasteboardReadingOptions)readingOptionsForType:(NSString *)type pasteboard:(NSPasteboard *)pasteboard {

    return NSPasteboardReadingAsKeyedArchive;
}

// NSPasteboardWriting

- (NSArray<NSString *> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard {

    return [NSArray arrayWithObjects:ddbPlaylistItemsUTIType, nil];
}

- (id)pasteboardPropertyListForType:(NSString *)type {

    if( [type isEqualToString:ddbPlaylistItemsUTIType] ) {
        return [NSKeyedArchiver archivedDataWithRootObject:self];
    }

    return nil;
}

- (int) count {
    return (int)[_itemsIndices count];
}

- (int) playlistIdx {

    return (int)_playlistIdx;
}

- (uint32_t *) indices {

    uint32_t * indices = malloc (sizeof (uint32_t *) * [self count] );
    int i = 0;
    for (NSNumber * number in _itemsIndices) {
        indices[i] = [number unsignedIntValue];
        ++i;
    }

    return indices;
}

- (DdbListviewLocalDragDropHolder *)initWithSelectedPlaylistItems:(ddb_playlist_t *)playlist {
    deadbeef->pl_lock ();
    _playlistIdx = deadbeef->plt_get_idx (playlist);

    int count = deadbeef->plt_getselcount (playlist);
    NSMutableArray *indices = [NSMutableArray arrayWithCapacity:(NSUInteger)count];
    if (count) {

        DB_playItem_t *it = deadbeef->plt_get_first (playlist, PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                [indices addObject: [NSNumber numberWithInt: deadbeef->plt_get_item_idx(playlist, it, PL_MAIN)]];
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    _itemsIndices = (NSArray*) indices;
    deadbeef->pl_unlock ();
    return self;
}

@end

//#define DEBUG_DRAW_GROUP_TITLES 1

#define BLANK_GROUP_SUBDIVISION 100

int headerheight = 23;
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
    DdbListviewCol_t _sortColumn;
    NSColor *_separatorColor;
}
- (void)setListView:(DdbListview *)lv;
@end

@implementation DdbListHeaderView
- (DdbListHeaderView *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    _dragging = -1;
    _sizing = -1;
    _separatorColor = [[NSColor headerColor] colorWithAlphaComponent:0.5];
    NSTrackingAreaOptions options = NSTrackingInVisibleRect | NSTrackingCursorUpdate | NSTrackingMouseMoved | NSTrackingActiveInActiveApp;
    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:self.bounds options:options owner:self userInfo:nil];
    [self addTrackingArea:area];
    return self;
}

- (void)setListView:(DdbListview *)lv {
    listview = lv;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    NSRect rect = [self bounds];

    [_separatorColor set];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x, 0,rect.size.width,1)];

    id <DdbListviewDelegate> delegate = [listview delegate];

    int x = -rc.origin.x;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        NSRect colRect = NSMakeRect(x, 0, w, [self frame].size.height);
        if (_dragging != col) {
            if (CGRectIntersectsRect(dirtyRect, colRect)) {
                [delegate drawColumnHeader:col inRect:colRect];
            }
        }
        [_separatorColor set];
        [NSBezierPath fillRect:NSMakeRect(colRect.origin.x + colRect.size.width - 1, colRect.origin.y+3,1,colRect.size.height-6)];
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
                [[[NSColor whiteColor] colorWithAlphaComponent:0.4] set];
                [NSBezierPath fillRect:colRect];

                [delegate drawColumnHeader:col inRect:colRect];
                [_separatorColor set];
                [NSBezierPath fillRect:NSMakeRect(colRect.origin.x, colRect.origin.y,1,colRect.size.height)];
                [NSBezierPath fillRect:NSMakeRect(colRect.origin.x + colRect.size.width - 1, colRect.origin.y,1,colRect.size.height)];
            }
        }
        x += w;
    }

}

- (void)mouseMoved:(NSEvent *)event {
    [self cursorUpdate:event];
}

- (void)cursorUpdate:(NSEvent *)event
{
    id <DdbListviewDelegate> delegate = [listview delegate];
    NSScrollView *sv = [listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];
    int x = -rc.origin.x;
    int idx = 0;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:self];
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];
        x += w;
        if (fabs(pt.x-x) < 5) {
            if (idx == 0) {
                [[NSCursor resizeRightCursor] set];
            }
            else {
                [[NSCursor resizeLeftRightCursor] set];
            }
            return;
        }
        idx++;
    }
    [[NSCursor arrowCursor] set];

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

    int idx = 0;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        if ((idx == 0 || convPt.x - x > 5) && convPt.x < x + w - 5) {
            _drag_delta = 0;
            _dragging = col;
            _dragPt = convPt;
            _drag_col_pos = x;
            [listview setNeedsDisplay:YES];
            break;
        }

        x += w;

        if (fabs(convPt.x - x) < 5) {
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
        if (_sortColumn != _dragging) {
            _sortOrder = 0;
        }
        _sortColumn = _dragging;
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
    [self setNeedsDisplay:YES];
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
            [self setNeedsDisplay:YES];

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
            _sortColumn = [delegate invalidColumn];
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

- (NSMenu *)menuForEvent:(NSEvent *)event {
    if ((event.type == NSRightMouseDown || event.type == NSLeftMouseDown)
        && (event.buttonNumber == 1
        || (event.buttonNumber == 0 && (event.modifierFlags & NSControlKeyMask)))) {
        id <DdbListviewDelegate> delegate = [listview delegate];
        DdbListviewCol_t col = [self columnIndexForCoord:[event locationInWindow]];
        return [delegate contextMenuForColumn:col withEvent:event forView:self];
    }
    return nil;
}

@end

@interface DdbListContentView : NSView {
    DdbListview *listview;
    NSPoint _lastDragLocation;
    BOOL    _draggingInView;
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

    _draggingInView = YES;
    return NSDragOperationCopy;
}

- (BOOL)wantsPeriodicDraggingUpdates {

    return NO;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {

    _lastDragLocation = [self convertPoint:[sender draggingLocation] fromView:nil];
    [self setNeedsDisplay:YES];

    return NSDragOperationCopy;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {

    _draggingInView = NO;
    [self setNeedsDisplay:YES];
}


- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {

    NSPasteboard *pboard = [sender draggingPasteboard];

    DdbListviewGroup_t *grp;
    int grp_index;
    int sel;

    NSPoint draggingLocation = [self convertPoint:[sender draggingLocation] fromView:nil];
    id<DdbListviewDelegate> delegate = listview.delegate;

    DdbListviewRow_t row = [delegate invalidRow];
    if ( -1 != [listview pickPoint:draggingLocation.y group:&grp groupIndex:&grp_index index:&sel]) {
        row = [delegate rowForIndex:sel];
    }

    if ( [[pboard types] containsObject:ddbPlaylistItemsUTIType ] ) {

        NSArray *classes = [[NSArray alloc] initWithObjects:[DdbListviewLocalDragDropHolder class], nil];
        NSDictionary *options = [NSDictionary dictionary];
        NSArray *draggedItems = [pboard readObjectsForClasses:classes options:options];

        DdbListviewLocalDragDropHolder *holder = [draggedItems firstObject];
        int from_playlist = [holder playlistIdx];
        uint32_t * indices = [holder indices];
        int length = [holder count];

        [delegate dropItems:from_playlist before:row indices:indices count:length copy:NO];
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
    id<DdbListviewDelegate> delegate = listview.delegate;

    int title_height = [listview grouptitle_height];
    for (DdbListviewCol_t col = [listview.delegate firstColumn];
         col != [listview.delegate invalidColumn];
         col = [listview.delegate nextColumn:col]) {

        int w = [listview.delegate columnWidth:col];

        if ([listview.delegate isAlbumArtColumn:col] && x + w > clip.origin.x) {
            NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:0];
            [clr set];
            [NSBezierPath fillRect:NSMakeRect (x, y, w, grp_next_y - y)];
            if (title_height > 0) {
                [delegate drawAlbumArtForGroup:grp groupIndex:groupIndex inColumn:col isPinnedGroup:isPinnedGroup nextGroupCoord:grp_next_y xPos:x yPos:y viewportY:viewportY width:w height:grp->height];
            }
        }
    }
}


- (void) drawLineIndicator:(NSRect)dirtyRect yy:(int)yy  {

    int yyline = yy;
    float indicatorLineWith = 1.f;
    if ( yyline > 0 ) {
        yyline -= (indicatorLineWith / 2.f );
    }
    [[NSGraphicsContext currentContext] saveGraphicsState];
    [NSBezierPath setDefaultLineWidth: indicatorLineWith];
    [[NSColor alternateSelectedControlColor] set];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(dirtyRect.origin.x, yyline) toPoint: NSMakePoint( dirtyRect.origin.x + dirtyRect.size.width, yyline ) ];
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)drawListView:(NSRect)dirtyRect {
    id<DdbListviewDelegate> delegate = listview.delegate;

    [listview groupCheck];

    DdbListviewGroup_t *grp = [listview groups];

    NSScrollView *sv = [self enclosingScrollView];

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

    int title_height = [listview grouptitle_height];

    BOOL focused = [[self window] isKeyWindow];

    while (grp && grp_y < clip_y + clip_h) {
        DdbListviewRow_t it = grp->head;
        [listview.delegate refRow:it];

        int ii = 0;
        for (int i = 0, yy = grp_y + title_height; it && i < grp->num_items && yy < clip_y + clip_h; i++, yy += rowheight) {
            ii++;

            if (yy + rowheight >= clip_y) {
                // draw row
                NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:ii % 2];
                [clr set];
                [NSBezierPath fillRect:NSMakeRect(dirtyRect.origin.x, yy, dirtyRect.size.width, rowheight)];

                int x = 0;
                for (DdbListviewCol_t col = [listview.delegate firstColumn]; col != [listview.delegate invalidColumn]; col = [listview.delegate nextColumn:col]) {
                    int w = [listview.delegate columnWidth:col];
                    if (CGRectIntersectsRect(dirtyRect, NSMakeRect(x, yy, w, rowheight))) {
                        [listview.delegate drawCell:idx+i forRow: it forColumn:col inRect:NSMakeRect(x, yy, w, rowheight-1) focused:focused];
                    }
                    x += w;
                }

                if (x < dirtyRect.size.width) {
                    [listview.delegate drawCell:idx+i forRow:it forColumn:[delegate invalidColumn] inRect:NSMakeRect(x, yy, dirtyRect.size.width-x, rowheight-1) focused:focused];
                }

                if (it == cursor_it) {
                    [[NSGraphicsContext currentContext] saveGraphicsState];
                    NSRect rect = NSMakeRect(_frame.origin.x+0.5, yy+0.5, _frame.size.width-1, rowheight-1);
                    [NSBezierPath setDefaultLineWidth:1.f];
                    [[NSColor textColor] set];
                    [NSBezierPath strokeRect:rect];
                    [[NSGraphicsContext currentContext] restoreGraphicsState];
                }

                // draw dnd line
                if (_draggingInView) {
                    if ( _lastDragLocation.y > listview.fullheight ) {
                        [self drawLineIndicator:dirtyRect yy: listview.fullheight];
                    }
                    else if ( _lastDragLocation.y > yy && _lastDragLocation.y < yy + rowheight ) {
                        [self drawLineIndicator:dirtyRect yy:yy];
                    }
                }

            }
            DdbListviewRow_t next = [listview.delegate nextRow:it];
            [listview.delegate unrefRow:it];
            it = next;
            if (it == [delegate invalidRow]) {
                break; // sanity check, in case groups were not rebuilt yet
            }
        }
        if (it != [delegate invalidRow]) {
            [listview.delegate unrefRow:it];
        }

        // draw album art
        int grp_next_y = grp_y + grp->height;
        [self renderAlbumArtForGroup:grp groupIndex:groupIndex isPinnedGroup:pin_grp==grp nextGroupCoord:grp_next_y yPos:grp_y + title_height viewportY:dirtyRect.origin.y clipRegion:dirtyRect];

        #define min(x,y) ((x)<(y)?(x):(y))
        if (pin_grp == grp && clip_y-dirtyRect.origin.y <= title_height) {
            // draw pinned group title
            // scrollx, 0, total_width, min(title_height, grp_next_y)
            NSRect groupRect = NSMakeRect(0, dirtyRect.origin.y, [self frame].size.width, min (title_height, grp_next_y));
            NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:0];
            [clr set];
#if DEBUG_DRAW_GROUP_TITLES
            [[NSColor redColor] set];
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
                NSRect groupRect = NSMakeRect(0, grp_y, [self frame].size.width, title_height);
                NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:0];
                [clr set];
#if DEBUG_DRAW_GROUP_TITLES
                [[NSColor greenColor] set];
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
    if ([listview fullheight] < dirtyRect.origin.y + dirtyRect.size.height) {
        int y = [listview fullheight];
        int ii = [listview.delegate rowCount]+1;
        while (y < dirtyRect.origin.y + dirtyRect.size.height) {
            if (y + rowheight >= dirtyRect.origin.y) {
                NSColor *clr = [[NSColor controlAlternatingRowBackgroundColors] objectAtIndex:ii % 2];
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
    if ((event.type == NSRightMouseDown || event.type == NSLeftMouseDown)
        && (event.buttonNumber == 1
        || (event.buttonNumber == 0 && (event.modifierFlags & NSControlKeyMask))))
    {
        if (event.buttonNumber == 0) {
            // ctrl+click blocks the mouseDown handler, do it now
            [self mouseDown:event];
        }

        id<DdbListviewDelegate> delegate = listview.delegate;
        return [delegate contextMenuForEvent:event forView:self];
    }
    return nil;
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    [self mouseDown:theEvent];
    [super rightMouseDown:theEvent];
}

- (void)mouseDown:(NSEvent *)event {
    [[self window] makeFirstResponder:listview];

    [listview groupCheck];

    id<DdbListviewDelegate> delegate = listview.delegate;

    if (![delegate rowCount]) {
        return;
    }

    DdbListviewGroup_t *grp;
    int grp_index;
    int sel;
    NSPoint convPt = [self convertPoint:[event locationInWindow] fromView:nil];
    listview.lastpos = convPt;

    if (-1 == [listview pickPoint:convPt.y group:&grp groupIndex:&grp_index index:&sel]) {
        [listview deselectAll];
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

@implementation DdbListview {
    id<DdbListviewDelegate> _delegate;
    DdbListviewGroup_t *_groups;
    int _grouptitle_height;
    int groups_build_idx;
    int _fullwidth;
    int _fullheight;
    BOOL _areaselect;
    int _areaselect_y;
    int _area_selection_start;
    int _area_selection_end;
    int _shift_sel_anchor;
    BOOL _dragwait;
    int _scroll_direction;
    int _scroll_pointer_y;
    NSPoint _lastpos;
}

@synthesize headerView;
@synthesize contentView;

- (void)dealloc {
    [self cleanup];
}

- (void)cleanup {
    [self freeGroups];
}

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

        [contentView registerForDraggedTypes:[NSArray arrayWithObjects:ddbPlaylistItemsUTIType, NSFilenamesPboardType, nil]];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidBecomeKey:)
                                                     name:NSWindowDidBecomeKeyNotification
                                                   object:[self window]];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidBecomeKey:)
                                                     name:NSWindowDidResignKeyNotification
                                                   object:[self window]];
    }
    return self;
}

- (void)windowDidBecomeKey:(id)sender {
    [self setNeedsDisplay:YES];
}


- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if ([keyPath isEqualToString:@"frameSize"]) {
        [self updateContentFrame];
    }
}

- (void)scrollChanged:(id)notification {
    [self.headerView setNeedsDisplay:YES];

    NSScrollView *sv = [contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    if ([(NSObject *)_delegate respondsToSelector:@selector(scrollChanged:)]) {
        [_delegate scrollChanged:vis.origin.y];
    }
}

- (BOOL)isFlipped {
    return YES;
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
    if ([_delegate modificationIdx] != groups_build_idx) {
        [self initGroups];
    }
}

- (void)reloadData {
    [self initGroups];
    [self.contentView setNeedsDisplay:YES];
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

- (void)drawGroup:(int)idx {
    int i = 0;
    int y = 0;
    for (DdbListviewGroup_t *grp = _groups; grp; grp = grp->next) {
        if (idx == i) {
            NSScrollView *sv = [contentView enclosingScrollView];
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
            [contentView setNeedsDisplayInRect:rect];
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
        case NSDraggingContextOutsideApplication:
            return NSDragOperationNone; // FIXME
    }
    return NSDragOperationNone;
}


- (void)listMouseDragged:(NSEvent *)event {
    NSPoint pt = [contentView convertPoint:[event locationInWindow] fromView:nil];
    if (_dragwait) {
        if (abs (_lastpos.x - pt.x) > 3 || abs (_lastpos.y - pt.y) > 3) {
            // begin dnd
            NSPasteboard *pboard;

            // Need playlist identifier and all playlist items when dragging internally,
            // this is represented with the DdbListviewLocalDragDropHolder interface

            pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            DdbListviewLocalDragDropHolder *data = [[DdbListviewLocalDragDropHolder alloc] initWithSelectedPlaylistItems:plt];
            deadbeef->plt_unref (plt);
            [pboard declareTypes:[NSArray arrayWithObject:ddbPlaylistItemsUTIType]  owner:self];
            if (![pboard writeObjects:[NSArray arrayWithObject:data]])
                NSLog(@"Unable to write to pasteboard.");
//            [pboard setData:[@"Hello" dataUsingEncoding:NSASCIIStringEncoding] forType:NSStringPboardType];

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

        NSScrollView *sv = [contentView enclosingScrollView];
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
                [contentView scrollPoint:NSMakePoint(vis.origin.x, vis.origin.y + vis.size.height - rowheight)];
                break;
            }
            case NSPageUpFunctionKey:
                [contentView scrollPoint:NSMakePoint(vis.origin.x, vis.origin.y - vis.size.height + rowheight)];
                break;
            case NSHomeFunctionKey:
                [contentView scrollPoint:NSMakePoint(vis.origin.x, 0)];
                break;
            case NSEndFunctionKey:
                [contentView scrollPoint:NSMakePoint(vis.origin.x, (contentView.frame.size.height - sv.contentSize.height))];
                break;
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

- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll {
    [_delegate setCursor:cursor];

    DdbListviewRow_t row = [_delegate rowForIndex:cursor];
    if (row != [_delegate invalidRow] && ![_delegate rowSelected:row]) {
        [self selectSingle:cursor];
    }
    if (row != [_delegate invalidRow]) {
        [_delegate unrefRow:row];
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

    if (![_delegate pinGroups] && cursor_scroll < scrollpos) {
        newscroll = cursor_scroll;
    }
    else if ([_delegate pinGroups] && cursor_scroll < scrollpos + _grouptitle_height) {
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
