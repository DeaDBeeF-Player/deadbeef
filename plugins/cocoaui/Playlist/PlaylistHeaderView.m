//
//  HeaderView.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/1/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "PlaylistView.h"
#import "PlaylistHeaderView.h"
#import "PlaylistContentView.h"
#include <deadbeef/deadbeef.h>

@interface PlaylistHeaderView()

@property (nonatomic) int origColWidth;
@property (nonatomic) CGFloat dragColPos;
@property (nonatomic) CGFloat dragDelta;
@property (nonatomic) DdbListviewCol_t dragging;
@property (nonatomic) DdbListviewCol_t sizing;
@property (nonatomic) NSPoint dragPt;
@property (nonatomic) BOOL prepare;
@property (nonatomic) DdbListviewCol_t sortColumn;
@property (nonatomic) NSColor *separatorColor;

@property (nonatomic) BOOL isKeyWindow;
@property (nonatomic,readonly) NSColor *headerTextColor;
@property (nonatomic,nullable) NSDictionary *titleAttributesCurrent;
@property (nonatomic,readonly) NSDictionary *titleAttributes;

@property (nonatomic,readonly) id<DdbListviewDelegate> delegate;
@property (nonatomic,readonly) id<DdbListviewDataModelProtocol> dataModel;

@property (nonatomic) CGFloat scrollGroupOffset;
@property (nonatomic) NSInteger scrollFirstGroup;

@end

@implementation PlaylistHeaderView

- (id<DdbListviewDelegate>)delegate {
    return self.listview.delegate;
}

- (id<DdbListviewDataModelProtocol>)dataModel {
    return self.listview.dataModel;
}

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
    self.dragging = -1;
    self.sizing = -1;
    self.separatorColor = [NSColor.headerColor colorWithAlphaComponent:0.5];
    NSTrackingAreaOptions options = NSTrackingInVisibleRect | NSTrackingCursorUpdate | NSTrackingMouseMoved | NSTrackingActiveInActiveApp;
    NSTrackingArea *area = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil];
    [self addTrackingArea:area];

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(windowDidBecomeOrResignKey:)
                                               name:NSWindowDidBecomeKeyNotification
                                             object:self.window];
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(windowDidBecomeOrResignKey:)
                                               name:NSWindowDidResignKeyNotification
                                             object:self.window];

    return self;
}

- (void)windowDidBecomeOrResignKey:(NSNotification *)notification {
    self.needsDisplay = YES;
}

- (void)drawColumnHeader:(DdbListviewCol_t)col inRect:(NSRect)rect {
    int columnCount = self.delegate.columnCount;
    int sortColumnIndex = self.delegate.sortColumnIndex;
    if (col < columnCount) {
        CGFloat width = rect.size.width-6;
        if (col == sortColumnIndex) {
            width -= 16;
        }
        if (width < 0) {
            width = 0;
        }

        [[self.delegate columnTitleAtIndex:col] drawInRect:NSMakeRect(rect.origin.x+4, rect.origin.y-2, width, rect.size.height-2) withAttributes:self.titleAttributes];

        enum ddb_sort_order_t sortOrder = [self.delegate columnSortOrderAtIndex:col];


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

    [self.separatorColor set];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x, 0,rect.size.width,1)];

    id <DdbListviewDelegate> delegate = [self.listview delegate];

    CGFloat x = -rc.origin.x;
    for (DdbListviewCol_t col = [self.delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        NSRect colRect = NSMakeRect(x, 0, w, self.frame.size.height);
        if (self.dragging != col) {
            if (CGRectIntersectsRect(dirtyRect, colRect)) {
                [self drawColumnHeader:col inRect:colRect];
            }
        }
        [self.separatorColor set];
        [NSBezierPath fillRect:NSMakeRect(colRect.origin.x + colRect.size.width - 1, colRect.origin.y+3,1,colRect.size.height-6)];
        x += w;
    }

    x = -rc.origin.x;
    for (DdbListviewCol_t col = [self.delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        CGFloat cx = x;
        if (self.dragging == col) {
            cx = self.dragColPos + self.dragDelta;
            NSRect colRect = NSMakeRect(cx, 1, w, self.frame.size.height-2);
            if (CGRectIntersectsRect(dirtyRect, colRect)) {
                [[NSColor.whiteColor colorWithAlphaComponent:0.4] set];
                [NSBezierPath fillRect:colRect];

                [self drawColumnHeader:col inRect:colRect];
                [self.separatorColor set];
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

- (void)cursorUpdate:(NSEvent *)event {
    id <DdbListviewDelegate> delegate = [self.listview delegate];
    NSScrollView *sv = [self.listview.contentView enclosingScrollView];
    NSRect rc = [sv documentVisibleRect];
    CGFloat x = -rc.origin.x;
    int idx = 0;
    NSPoint pt = [self convertPoint:[event locationInWindow] fromView:nil];
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

    self.dragging = [delegate invalidColumn];
    self.sizing = [delegate invalidColumn];
    self.prepare = YES;
    self.scrollFirstGroup = -1;

    int idx = 0;
    for (DdbListviewCol_t col = [delegate firstColumn]; col != [delegate invalidColumn]; col = [delegate nextColumn:col]) {
        int w = [delegate columnWidth:col];

        if ((idx == 0 || convPt.x - x > 5) && convPt.x < x + w - 5) {
            self.dragDelta = 0;
            self.dragging = col;
            self.dragPt = convPt;
            self.dragColPos = x;
            self.listview.needsDisplay = YES;
            break;
        }

        x += w;

        if (fabs(convPt.x - x) < 5) {
            self.sizing = col;
            self.dragPt = convPt;
            self.origColWidth = [delegate columnWidth:col];

            // find the first visible group and offset,
            // to preserve visual focus on the first visible row
            self.scrollGroupOffset = 0;
            self.scrollFirstGroup = [self.listview.contentView getScrollFocusGroupAndOffset:&_scrollGroupOffset];

            break;
        }
    }
}

- (void)mouseUp:(NSEvent *)theEvent {
    id <DdbListviewDelegate> delegate = [self.listview delegate];

    if (self.dragging != delegate.invalidColumn && self.prepare) { // clicked
        self.sortColumn = self.dragging;
        [delegate sortColumn:self.dragging];
    }
    else if (self.dragging != [delegate invalidColumn] || self.sizing != [delegate invalidColumn]) {
        [delegate columnsDidChange];
    }
    self.dragging = [delegate invalidColumn];
    self.sizing = [delegate invalidColumn];
    self.scrollFirstGroup = -1;
    self.listview.contentView.needsDisplay = YES;
}

- (void)mouseDragged:(NSEvent *)theEvent {
    NSPoint convPt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    self.prepare = NO;

    id <DdbListviewDelegate> delegate = [self.listview delegate];
    if (self.sizing != [delegate invalidColumn]) {
        CGFloat dx = convPt.x - self.dragPt.x;

        int w = self.origColWidth + (int)dx;
        if (w < 10) {
            w = 10;
        }
        if ([delegate columnWidth:self.sizing] != w) {
            NSScrollView *sv = [self.listview.contentView enclosingScrollView];
            NSRect rc = [sv documentVisibleRect];

            CGFloat scroll = -rc.origin.x;

            [delegate setColumnWidth:w forColumn:self.sizing];
            self.needsDisplay = YES;

            rc = [sv documentVisibleRect];
            scroll += rc.origin.x;
            self.dragPt = NSMakePoint(self.dragPt.x - scroll, self.dragPt.y);

            [self.listview.contentView reloadData];
            [self.listview.contentView layoutSubtreeIfNeeded];

            if (self.scrollFirstGroup != -1) {
                // find the new position of the group, and scroll to it with offset
                CGFloat scrollPos = [self.listview.contentView groupPositionAtIndex:self.scrollFirstGroup];
                [self.listview.contentView scrollVerticalPosition:scrollPos - self.scrollGroupOffset];
            }
        }
    }
    else if (self.dragging != [delegate invalidColumn]) {
        self.dragDelta = convPt.x - self.dragPt.x;

        NSScrollView *sv = [self.listview.contentView enclosingScrollView];
        NSRect rc = [sv documentVisibleRect];

        CGFloat x = -rc.origin.x;

        DdbListviewCol_t inspos = [delegate invalidColumn];

        // FIXME: DdbListviewCol_t is not always index -- account for this
        CGFloat cx = self.dragColPos + self.dragDelta;
        for (DdbListviewCol_t cc = [delegate firstColumn]; cc != [delegate invalidColumn]; cc = [delegate nextColumn:cc]) {
            int cw = [delegate columnWidth:cc];

            if (cc < self.dragging && cx <= x + cw/2) {
                inspos = cc;
                break;
            }
            else if (cc > self.dragging && cx > x + cw/2 - [delegate columnWidth:self.dragging]) {
                inspos = cc;
            }

            x += cw;
        }

        if (inspos != [delegate invalidColumn] && inspos != self.dragging) {
            [delegate moveColumn:self.dragging to:inspos];
            self.dragging = inspos;
            self.sortColumn = [delegate invalidColumn];
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
