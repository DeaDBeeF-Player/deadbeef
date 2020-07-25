//
//  HeaderView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "PlaylistView.h"
#import "PlaylistHeaderView.h"
#import "PlaylistContentView.h"
#include "deadbeef.h"

@interface PlaylistHeaderView() {
    int _orig_col_width;
    CGFloat _drag_col_pos;
    CGFloat _drag_delta;
    DdbListviewCol_t _dragging;
    DdbListviewCol_t _sizing;
    NSPoint _dragPt;
    BOOL _prepare;
    DdbListviewCol_t _sortColumn;
    NSColor *_separatorColor;
}

@property (nonatomic) BOOL isKeyWindow;
@property (nonatomic,readonly) NSColor *headerTextColor;
@property (nonatomic,nullable) NSDictionary *titleAttributesCurrent;
@property (nonatomic,readonly) NSDictionary *titleAttributes;

@end

@implementation PlaylistHeaderView

- (NSColor *)headerTextColor {
    NSColor *textColor = NSColor.controlTextColor;
    if (!self.isKeyWindow) {
        textColor = [textColor colorWithAlphaComponent:0.7];
    }
    return textColor;
}

- (void)updateTextAttributes {
    if (self.window.isKeyWindow == self.isKeyWindow && self.titleAttributesCurrent) {
        return;
    }

    self.isKeyWindow = self.window.isKeyWindow;

    NSMutableParagraphStyle *textStyle = [NSParagraphStyle.defaultParagraphStyle mutableCopy];

    textStyle.alignment = NSTextAlignmentLeft;
    textStyle.lineBreakMode = NSLineBreakByTruncatingTail;

    self.titleAttributesCurrent = @{
        NSFontAttributeName:[NSFont controlContentFontOfSize:NSFont.smallSystemFontSize],
        NSBaselineOffsetAttributeName: @0,
        NSForegroundColorAttributeName: self.headerTextColor,
        NSParagraphStyleAttributeName: textStyle
    };

}

- (NSDictionary *)titleAttributes {
    [self updateTextAttributes];

    return self.titleAttributesCurrent;
}

- (PlaylistHeaderView *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    _dragging = -1;
    _sizing = -1;
    _separatorColor = [NSColor.headerColor colorWithAlphaComponent:0.5];
    NSTrackingAreaOptions options = NSTrackingInVisibleRect | NSTrackingCursorUpdate | NSTrackingMouseMoved | NSTrackingActiveInActiveApp;
    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:self.bounds options:options owner:self userInfo:nil];
    [self addTrackingArea:area];
    return self;
}

- (void)setListView:(PlaylistView *)lv {
    self.listview = lv;
}

- (void)drawColumnHeader:(DdbListviewCol_t)col inRect:(NSRect)rect {
    id <DdbListviewDelegate> delegate = [self.listview delegate];
    int columnCount = delegate.columnCount;
    int sortColumnIndex = delegate.sortColumnIndex;
    if (col < columnCount) {
        CGFloat width = rect.size.width-6;
        if (col == sortColumnIndex) {
            width -= 16;
        }
        if (width < 0) {
            width = 0;
        }
//        [self.headerTextColor set]; // FIXME -- check if needed

        [[delegate columnTitleAtIndex:col] drawInRect:NSMakeRect(rect.origin.x+4, rect.origin.y-2, width, rect.size.height-2) withAttributes:self.titleAttributes];

        enum ddb_sort_order_t sortOrder = [delegate columnSortOrderAtIndex:col];


        if (col == sortColumnIndex) {
            [[NSColor.controlTextColor highlightWithLevel:0.5] set];
            NSBezierPath *path = [NSBezierPath new];
            path.lineWidth = 2;
            if (sortOrder == DDB_SORT_ASCENDING) {
                [path moveToPoint:NSMakePoint(rect.origin.x+4+width+4, rect.origin.y+10)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+8, rect.origin.y+10+4)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+12, rect.origin.y+10)];
            }
            else if (sortOrder == DDB_SORT_DESCENDING) {
                [path moveToPoint:NSMakePoint(rect.origin.x+4+width+4, rect.origin.y+10+4)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+8, rect.origin.y+10)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+12, rect.origin.y+10+4)];
            }
            [path stroke];
        }
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSScrollView *sv = [self.listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    NSRect rect = self.bounds;

    [_separatorColor set];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x, 0,rect.size.width,1)];

    id <DdbListviewDelegate> delegate = [self.listview delegate];

    CGFloat x = -rc.origin.x;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        NSRect colRect = NSMakeRect(x, 0, w, self.frame.size.height);
        if (_dragging != col) {
            if (CGRectIntersectsRect(dirtyRect, colRect)) {
                [self drawColumnHeader:col inRect:colRect];
            }
        }
        [_separatorColor set];
        [NSBezierPath fillRect:NSMakeRect(colRect.origin.x + colRect.size.width - 1, colRect.origin.y+3,1,colRect.size.height-6)];
        x += w;
    }

    x = -rc.origin.x;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        CGFloat cx = x;
        if (_dragging == col) {
            cx = _drag_col_pos + _drag_delta;
            NSRect colRect = NSMakeRect(cx, 1, w, self.frame.size.height-2);
            if (CGRectIntersectsRect(dirtyRect, colRect)) {
                [[NSColor.whiteColor colorWithAlphaComponent:0.4] set];
                [NSBezierPath fillRect:colRect];

                [self drawColumnHeader:col inRect:colRect];
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
    id <DdbListviewDelegate> delegate = [self.listview delegate];
    NSScrollView *sv = [self.listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];
    CGFloat x = -rc.origin.x;
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
    NSScrollView *sv = [self.listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];

    NSPoint convPt = [self convertPoint:[theEvent locationInWindow] fromView:nil];

    CGFloat x = -rc.origin.x;
    id <DdbListviewDelegate> delegate = [self.listview delegate];

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
            self.listview.needsDisplay = YES;
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
    id <DdbListviewDelegate> delegate = [self.listview delegate];

    if (_prepare) { // clicked
        _sortColumn = _dragging;
        [delegate sortColumn:_dragging];
    }
    else if (_dragging != [delegate invalidColumn] || _sizing != [delegate invalidColumn]) {
        [delegate columnsChanged];
        [self.listview.contentView updateContentFrame];
    }
    _dragging = [delegate invalidColumn];
    _sizing = [delegate invalidColumn];
    self.listview.contentView.needsDisplay = YES;
}

- (void)mouseDragged:(NSEvent *)theEvent {
    NSPoint convPt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    _prepare = NO;

    id <DdbListviewDelegate> delegate = [self.listview delegate];
    if (_sizing != [delegate invalidColumn]) {
        CGFloat dx = convPt.x - _dragPt.x;

        int w = _orig_col_width + (int)dx;
        if (w < 10) {
            w = 10;
        }
        if ([delegate columnWidth:_sizing] != w) {
            NSScrollView *sv = [self.listview.contentView enclosingScrollView];
            NSRect rc = [sv documentVisibleRect];

            CGFloat scroll = -rc.origin.x;

            [delegate setColumnWidth:w forColumn:_sizing];
            [self.listview.contentView updateContentFrame];
            self.listview.contentView.needsDisplay = YES;
            self.needsDisplay = YES;

            rc = [sv documentVisibleRect];
            scroll += rc.origin.x;
            _dragPt.x -= scroll;
            [self.listview.contentView reloadData];
        }
    }
    else if (_dragging != [delegate invalidColumn]) {
        _drag_delta = convPt.x - _dragPt.x;

        NSScrollView *sv = [self.listview.contentView enclosingScrollView];
        NSRect rc = [sv documentVisibleRect];

        CGFloat x = -rc.origin.x;

        DdbListviewCol_t inspos = [delegate invalidColumn];

        // FIXME: DdbListviewCol_t is not always index -- account for this
        CGFloat cx = _drag_col_pos + _drag_delta;
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
            [self.listview.contentView reloadData];
        }
        else {
            self.needsDisplay = YES;
        }
    }
}

- (DdbListviewCol_t)columnIndexForCoord:(NSPoint)theCoord {
    id <DdbListviewDelegate> delegate = [self.listview delegate];
    NSScrollView *sv = [self.listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];
    NSPoint convPt = [self convertPoint:theCoord fromView:nil];

    CGFloat x = -rc.origin.x;
    DdbListviewCol_t col;
    for (col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        if (CGRectContainsPoint(NSMakeRect(x, 0, w, self.bounds.size.height), convPt)) {
            break;
        }

        x += w;
    }
    return col;
}

- (NSMenu *)menuForEvent:(NSEvent *)event {
    if ((event.type == NSEventTypeRightMouseDown || event.type == NSEventTypeLeftMouseDown)
        && (event.buttonNumber == 1
        || (event.buttonNumber == 0 && (event.modifierFlags & NSEventModifierFlagControl)))) {
        id <DdbListviewDelegate> delegate = [self.listview delegate];
        DdbListviewCol_t col = [self columnIndexForCoord:[event locationInWindow]];
        return [delegate contextMenuForColumn:col withEvent:event forView:self];
    }
    return nil;
}

@end
