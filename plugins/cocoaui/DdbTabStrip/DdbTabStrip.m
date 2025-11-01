/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#import "DdbShared.h"
#import "DdbTabStrip.h"
#import "DeletePlaylistConfirmationController.h"
#import "PlaylistContextMenu.h"
#import "RenamePlaylistViewController.h"
#import "TrackPropertiesWindowController.h"
#import "Weakify.h"

extern DB_functions_t *deadbeef;

static const int no_tab = -1;
static const NSPoint no_point = { .x = -100000, .y = -100000 };

static const int text_left_padding = 24;
static const int text_right_padding = 24;

static const int tab_overlap_size = 0;
static const int tabs_left_margin = 0;
static const int tab_vert_padding = 0;

static const int min_tab_size = 80;
static const int max_tab_size = 200;

static const int tab_close_btn_size = 12;

@interface DdbTabStrip () <RenamePlaylistViewControllerDelegate,DeletePlaylistConfirmationControllerDelegate,TrackContextMenuDelegate,TrackPropertiesWindowControllerDelegate> {
    int _dragging;
    int _prepare;
    int _movepos;
    int _scroll_direction;
    NSPoint _dragpt;
    int _prev_x;
    int _tab_moved;
    int _pointedTab;
    NSTrackingArea *_trackingArea;
    NSPoint _lastMouseCoord;

    NSRect _closeTabButtonRect;
    BOOL _closeTabCapture;

}

@property (nonatomic,readonly) NSColor *tabTextColor;
@property (nonatomic,nullable) NSDictionary *titleAttributesCurrent;
@property (nonatomic,readonly) NSDictionary *titleAttributes;
@property (nonatomic) BOOL isDarkMode;
@property (nonatomic) BOOL isKeyWindow;
@property (nonatomic,nullable) NSDictionary *titleAttributesSelectedCurrent;
@property (nonatomic,readonly) NSDictionary *titleAttributesSelected;
@property (nonatomic) NSColor *tabBackgroundColorDark;
@property (nonatomic) NSColor *tabBackgroundColorLight;
@property (nonatomic,readonly) NSColor *tabBackgroundColor;

@property (nonatomic) BOOL dragReallyBegan;

@property (nonatomic) int clickedTabIndex;
@property (nonatomic) BOOL needScroll;
@property (nonatomic) CGFloat scrollPos;
@property (nonatomic,readonly) int fullWidth;

@property (nonatomic) TrackPropertiesWindowController *trkProperties;

@property (nonatomic) NSTimer *pickDragTimer;

@end

@implementation DdbTabStrip

- (void)dealloc {
    [self.trkProperties close];
    self.trkProperties = nil;
}

- (NSColor *)accentColor {
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        return NSColor.controlAccentColor;
    }
    else
#endif
    {
        return NSColor.alternateSelectedControlColor;
    }
}

- (NSColor *)tabTextColor {
    NSColor *textColor = NSColor.controlTextColor;
    if (!self.isKeyWindow) {
        textColor = [textColor colorWithAlphaComponent:0.7];
    }
    return textColor;
}

- (void)updateTitleAttributes {
    NSString *osxMode = [NSUserDefaults.standardUserDefaults stringForKey:@"AppleInterfaceStyle"];
    BOOL isDarkMode = [osxMode isEqualToString:@"Dark"];

    if (isDarkMode == self.isDarkMode && self.window.isKeyWindow == self.isKeyWindow && self.titleAttributesCurrent) {
        return;
    }

    self.isDarkMode = isDarkMode;
    self.isKeyWindow = self.window.isKeyWindow;

    NSMutableParagraphStyle *textStyle = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
    textStyle.alignment = NSTextAlignmentLeft;
    textStyle.lineBreakMode = NSLineBreakByTruncatingTail;

    NSFont *font = [NSFont systemFontOfSize:NSFont.smallSystemFontSize weight:NSFontWeightSemibold];
    NSColor *textColor = self.isDarkMode ? NSColor.controlTextColor : self.accentColor;

    self.titleAttributesSelectedCurrent = @{
        NSParagraphStyleAttributeName: textStyle,
        NSFontAttributeName:font,
        NSForegroundColorAttributeName:textColor
    };

    textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    textStyle.alignment = NSTextAlignmentLeft;
    textStyle.lineBreakMode = NSLineBreakByTruncatingTail;

    font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize] weight:NSFontWeightMedium];
    textColor = self.tabTextColor;

    self.titleAttributesCurrent = @{
        NSParagraphStyleAttributeName: textStyle,
        NSFontAttributeName:font,
        NSForegroundColorAttributeName:textColor
    };
}

- (NSDictionary *)titleAttributes {
    [self updateTitleAttributes];

    return self.titleAttributesCurrent;
}

- (NSDictionary *)titleAttributesSelected {
    [self updateTitleAttributes];

    return self.titleAttributesSelectedCurrent;
}

- (NSColor *)tabBackgroundColor {
    if (@available(macOS 13.0, *)) {
        return NSColor.selectedTextBackgroundColor;
    }
    NSString *osxMode = [NSUserDefaults.standardUserDefaults stringForKey:@"AppleInterfaceStyle"];
    BOOL isKey = self.window.isKeyWindow;
    if ([osxMode isEqualToString:@"Dark"]) {
        return [self.accentColor colorWithAlphaComponent:isKey?0.5:0.3];
    }
    else {
        return [self.accentColor colorWithAlphaComponent:(isKey?0.3:0.2)];
    }
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (!self) {
        return nil;
    }
    [self setup];
    return self;
}

- (void)setup {
    _dragging = no_tab;
    self.clickedTabIndex = no_tab;
    self.autoresizesSubviews = NO;

    _lastMouseCoord = no_point;
    _pointedTab = no_tab;

    [self setupTrackingArea];

    [self scrollToTabInt:deadbeef->plt_get_curr_idx() redraw:NO];

    [self registerForDraggedTypes:@[ddbPlaylistItemsUTIType, NSFilenamesPboardType]];

    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(windowDidBecomeKey:)
                                               name:NSWindowDidBecomeKeyNotification
                                             object:nil];
    [NSNotificationCenter.defaultCenter addObserver:self
                                           selector:@selector(windowDidBecomeKey:)
                                               name:NSWindowDidResignKeyNotification
                                             object:nil];
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    if (notification.object == self.window) {
        self.needsDisplay = YES;
    }
}

- (int)tabWidthForIndex:(int)tab {
    NSString *title = plt_get_title_wrapper (tab);
    NSSize sz = [title sizeWithAttributes:self.titleAttributesSelected];
    int w = (int)sz.width;
    w += text_left_padding + text_right_padding;
    if (w < min_tab_size) {
        w = min_tab_size;
    }
    else if (w > max_tab_size) {
        w = max_tab_size;
    }
    return w;
}

- (void)recalculateNeedScroll {
    BOOL origValue = self.needScroll;
    self.needScroll = NO;
    int cnt = deadbeef->plt_get_count ();
    int w = 0;
    NSRect a = self.bounds;
    for (int idx = 0; idx < cnt; idx++) {
        w += [self tabWidthForIndex:idx] - tab_overlap_size;
        if (w >= a.size.width) {
            self.needScroll = YES;
            break;
        }
    }
    w += tab_overlap_size + 3;
    if (w >= a.size.width) {
        self.needScroll = YES;
    }
    if (origValue != self.needScroll) {
        self.needsDisplay = YES;
    }
}

- (int)fullWidth {
    int fullwidth = 0;
    int cnt = deadbeef->plt_get_count ();
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = [self tabWidthForIndex:idx];
        if (idx == cnt-1) {
            tab_w += 3;
        }
        fullwidth += tab_w - tab_overlap_size;
    }
    fullwidth += tab_overlap_size;
    return fullwidth;
}

- (void)setScrollPos:(CGFloat)scrollPos {
    _scrollPos = MAX(0, MIN(self.fullWidth - NSWidth(self.bounds), scrollPos));
}

- (void)scrollToTabInt:(int)tab redraw:(BOOL)redraw {
    int w = tabs_left_margin;
    int cnt = deadbeef->plt_get_count ();
    NSSize a = self.bounds.size;
    int boundary = a.width + self.scrollPos;
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = [self tabWidthForIndex:idx];
        if (idx == cnt-1) {
            tab_w += 3;
        }
        if (idx == tab) {
            if (w < self.scrollPos) {
                self.scrollPos = w;
                if (redraw) {
                    self.needsDisplay = YES;
                }
            }
            else if (w + tab_w >= boundary) {
                self.scrollPos = self.scrollPos + (w + tab_w) - boundary;
                if (redraw) {
                    self.needsDisplay = YES;
                }
            }
            break;
        }
        w += tab_w - tab_overlap_size;
    }
}

- (void)adjustHScroll {
    if (deadbeef->plt_get_count () > 0) {
        self.scrollPos = _scrollPos;
    }
    [self mouseMovedHandler];
}

- (void)drawTab:(int)idx area:(NSRect)area selectedIdx:(int)selectedIdx {
    [NSGraphicsContext.currentContext saveGraphicsState];
    const NSRect tabRect = area;

    const BOOL selected = idx == selectedIdx;
    const BOOL highlighted = idx == _pointedTab;

    // Layer 1 - Tab background (pill-shaped), only for selected or highlighted.
    const NSRect bgRect = NSInsetRect(tabRect, 0, 2);
    if (selected) {
        NSColor *backgroundColor = self.tabBackgroundColor;
        [backgroundColor set];

        CGFloat radius = NSHeight(bgRect) / 2;
        NSBezierPath *tab = [NSBezierPath bezierPathWithRoundedRect:bgRect xRadius:radius yRadius:radius];
        [tab fill];
    } else if (highlighted && (_dragging == no_tab || !self.dragReallyBegan)) {
        NSColor *backgroundColor = [self.tabBackgroundColor colorWithAlphaComponent:0.5];
        [backgroundColor set];

        CGFloat radius = NSHeight(bgRect) / 2;
        NSBezierPath *tab = [NSBezierPath bezierPathWithRoundedRect:bgRect xRadius:radius yRadius:radius];
        [tab fill];
    }

    // Layer 2 - Tab divider to the right, skipping active and highlighted tab.
    NSBezierPath.defaultLineWidth = 1;
    if ((idx < selectedIdx - 1 || idx > selectedIdx) && (idx < _pointedTab - 1 || idx > _pointedTab)) {
        NSColor *clr = _hiddenVertLine.borderColor;
        [[clr colorWithAlphaComponent:0.5] set];
        NSBezierPath *line = [NSBezierPath bezierPath];
        [line moveToPoint:NSMakePoint(NSMaxX(tabRect) - 0.5, NSMinY(tabRect) + 5)];
        [line lineToPoint:NSMakePoint(NSMaxX(tabRect) - 0.5, NSMaxY(tabRect) - 5)];
        [line stroke];
    }

    // Layer 3 - Tab title.
    NSDictionary *attrs = selected ? self.titleAttributesSelected : self.titleAttributes;

    NSString *tab_title = plt_get_title_wrapper (idx);

    NSSize size = [tab_title sizeWithAttributes:attrs];

    NSRect textRect = bgRect;
    // Inset by paddings
    textRect.origin.x += text_left_padding;
    textRect.size.width -= text_left_padding + text_right_padding;
    // Center vertically
    textRect.origin.y = NSMidY(bgRect) - size.height / 2;
    textRect.size.height = size.height;

    [tab_title drawInRect:textRect withAttributes:attrs];

    // Layer 4 - Close button
    if (highlighted && (_dragging == no_tab || !self.dragReallyBegan)) {
        const NSRect closeBgRect = [self tabCloseButtonRectForTabRect:bgRect];
        if (NSPointInRect(_lastMouseCoord, closeBgRect)) {
            NSBezierPath *path = [NSBezierPath bezierPathWithOvalInRect:closeBgRect];
            CGFloat alpha = _closeTabCapture ? 0.4 : 0.2 ;
            if (!self.isKeyWindow) {
                alpha *= 0.7;
            }

            NSColor *closeButtonBackgroundColor = [NSColor.controlTextColor colorWithAlphaComponent:alpha];

            [closeButtonBackgroundColor set];
            [path fill];
        }

        CGFloat closeRadius = closeBgRect.size.width / 2;
        NSRect closeRect = NSInsetRect(closeBgRect, closeRadius / 2, closeRadius / 2);

        [self.tabTextColor set];
        NSBezierPath.defaultLineWidth = 1;
        [NSBezierPath strokeLineFromPoint:NSMakePoint(NSMinX(closeRect), NSMinY(closeRect))
                                  toPoint:NSMakePoint(NSMaxX(closeRect), NSMaxY(closeRect))];
        [NSBezierPath strokeLineFromPoint:NSMakePoint(NSMinX(closeRect), NSMaxY(closeRect))
                                  toPoint:NSMakePoint(NSMaxX(closeRect), NSMinY(closeRect))];
    }

    [NSGraphicsContext.currentContext restoreGraphicsState];
}

- (NSRect)tabRectForXPos:(int)xPos width:(int)tabWidth height:(int)tabHeight {
    return NSMakeRect(xPos, tab_vert_padding, tabWidth, tabHeight);

}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    int cnt = deadbeef->plt_get_count ();
    int hscroll = self.scrollPos;

    int x = -hscroll;
    int w = 0;
    int tab_selected = deadbeef->plt_get_curr_idx ();
    if (tab_selected == no_tab) {
        return;
    }

    int need_draw_moving = 0;
    int idx;
    int widths[cnt];
    for (idx = 0; idx < cnt; idx++) {
        widths[idx] = [self tabWidthForIndex:idx];
    }

    [NSGraphicsContext.currentContext saveGraphicsState];
    [NSBezierPath clipRect:self.frame];

    // draw selected
    // calc position for drawin selected tab
    x = -hscroll;
    for (idx = 0; idx < tab_selected && idx < cnt; idx++) {
        x += widths[idx] - tab_overlap_size;
    }
    x += tabs_left_margin;
    NSRect selectedTabRect;
    if (_dragging < 0 || _prepare || tab_selected != _dragging) {
        idx = tab_selected;
        w = widths[tab_selected];
        selectedTabRect = [self tabRectForXPos:x width:w height:self.bounds.size.height];
        [self drawTab:idx area:selectedTabRect selectedIdx:tab_selected];
    }
    else {
        need_draw_moving = 1;
    }
    if (need_draw_moving) {
        x = -hscroll + tabs_left_margin;
        for (idx = 0; idx < cnt; idx++) {
            w = widths[idx];
            if (idx == _dragging) {
                x = _movepos;
                if (x >= self.bounds.size.width) {
                    break;
                }
                if (w > 0) {
                    // ***** draw dragging tab here *****
                    selectedTabRect = [self tabRectForXPos:x width:w height:self.bounds.size.height];
                    [self drawTab:tab_selected area:selectedTabRect selectedIdx:tab_selected];

                    NSBezierPath *clipPath = [NSBezierPath bezierPath];

                    if (selectedTabRect.origin.x > 0) {
                        NSRect clipLeft = NSMakeRect(0, 0, selectedTabRect.origin.x, selectedTabRect.size.height);
                        [clipPath appendBezierPathWithRect:clipLeft];
                    }
                    int xx = selectedTabRect.origin.x+selectedTabRect.size.width;
                    NSRect clipRight = NSMakeRect(xx, 0, self.bounds.size.width - xx, selectedTabRect.size.height);
                    [clipPath appendBezierPathWithRect:clipRight];
                    [clipPath setClip];
                }
                int undercursor = [self tabUnderCursor:_lastMouseCoord];
                if (undercursor == _dragging) {
                    [self updatePointedTab:idx];
                }
                break;
            }
            x += w - tab_overlap_size;
        }
    }

    x = -hscroll + tabs_left_margin;

    // draw tabs on the left
    int c = tab_selected == no_tab ? cnt : tab_selected;
    for (idx = 0; idx < c; idx++) {
        w = widths[idx];
        NSRect area = [self tabRectForXPos:x width:w height:self.bounds.size.height];
        [self drawTab:idx area:area selectedIdx:tab_selected];
        x += w - tab_overlap_size;
    }
    // draw tabs on the right
    if (tab_selected != no_tab && tab_selected != cnt-1) {
        x = -hscroll + tabs_left_margin;
        for (idx = 0; idx < cnt; idx++) {
            x += widths[idx] - tab_overlap_size;
        }
        for (idx = cnt-1; idx > tab_selected; idx--) {
            w = widths[idx];
            x -= w - tab_overlap_size;
            NSRect area = [self tabRectForXPos:x width:w height:self.bounds.size.height];
            [self drawTab:idx area:area selectedIdx:tab_selected];
        }
    }

    [NSGraphicsContext.currentContext restoreGraphicsState];
}

-(NSRect)tabRectForIndex:(int)tab {
    int width = 0;
    int cnt = deadbeef->plt_get_count ();
    for (int idx = 0; idx < cnt; idx++) {
        int w = [self tabWidthForIndex:tab];
        if (idx == tab) {
            return NSMakeRect(width - self.scrollPos, 0, w, self.frame.size.height);
        }
        width += w - tab_overlap_size;
    }
    return NSMakeRect(0, 0, 0, 0);
}

-(int)tabUnderCursor:(NSPoint)point {
    if (NSEqualPoints(point, no_point)) {
        return no_tab;
    }

    int x = point.x;

    int hscroll = self.scrollPos;

    int idx;
    int cnt = deadbeef->plt_get_count ();
    int fw = tabs_left_margin - hscroll;
    for (idx = 0; idx < cnt; idx++) {
        int w = [self tabWidthForIndex:idx];
        fw += w;
        fw -= tab_overlap_size;
        if (fw > x) {
            return idx;
        }
    }
    return no_tab;
}

-(void)scrollToTab:(int)tab {
    [self scrollToTabInt:tab redraw:YES];
}

-(void)scrollLeft {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab > 0) {
        tab--;
        deadbeef->plt_set_curr_idx (tab);
    }
    [self scrollToTab:tab];
}

-(void)scrollRight {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab < deadbeef->plt_get_count () - 1) {
        tab++;
        deadbeef->plt_set_curr_idx (tab);
    }
    [self scrollToTab:tab];
}

-(void)updatePointedTab:(int)tab {
    if (!_closeTabCapture) {
        _pointedTab = tab;
        self.needsDisplay = YES;
    }
    else if (_closeTabCapture) {
        self.needsDisplayInRect = _closeTabButtonRect;
    }
}

-(void)closePointedTab {
    if (_pointedTab != no_tab) {
        self.clickedTabIndex = _pointedTab;
        _pointedTab = no_tab;
        [self closePlaylist:self];
    }
}

-(void)scrollWheel:(NSEvent*)event {
    CGFloat newScroll;
    if (event.hasPreciseScrollingDeltas) {
        // Precision Scrolling - Apple Trackpad and Magic mouse.
        newScroll = self.scrollPos - (event.scrollingDeltaX + event.scrollingDeltaY);
    } else {
        // Non precision scrolling - discrete wheel scroolling mice.
        newScroll = self.scrollPos - (event.deltaX + event.deltaY) * 4;
    }
    self.scrollPos = newScroll;
    self.needsDisplay = YES;
}

-(NSRect)tabCloseButtonRectForTabRect:(NSRect)tabRect {
    NSSize size = NSMakeSize(tab_close_btn_size, tab_close_btn_size);
    CGFloat offset = (tabRect.size.height - size.height) / 2;
    NSPoint from = NSMakePoint(tabRect.origin.x + offset, tabRect.origin.y + offset);
    NSRect atRect;
    atRect.origin = from;
    atRect.size = size;
    return atRect;
}

- (BOOL)handleClickedTabCloseRect {
    int hscroll = self.scrollPos;
    int x = -hscroll + tabs_left_margin;
    int idx;
    for (idx = 0; idx < self.clickedTabIndex; idx++) {
        int width = [self tabWidthForIndex:idx];
        x += width - tab_overlap_size;
    }
    int w = [self tabWidthForIndex:_clickedTabIndex];

    NSRect tabRect = [self tabRectForXPos:x width:w height:self.bounds.size.height];

    NSRect atRect = [self tabCloseButtonRectForTabRect:tabRect];

    if (!NSPointInRect(_lastMouseCoord, atRect)) {
        return NO;
    }

    _closeTabButtonRect = atRect;
    _closeTabCapture = YES;
    self.needsDisplayInRect = atRect;
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint coord = [self convertPoint:event.locationInWindow fromView:nil];
    _lastMouseCoord = coord;
    _clickedTabIndex = [self tabUnderCursor:coord];
    if (event.type == NSEventTypeLeftMouseDown) {

        if (_clickedTabIndex != no_tab) {
            if ([self handleClickedTabCloseRect]) {
                return;
            }
            deadbeef->plt_set_curr_idx (_clickedTabIndex);

            if (event.clickCount == 2) {
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                int cur = deadbeef->plt_get_cursor (plt, PL_MAIN);
                deadbeef->plt_unref (plt);

                if (cur == -1) {
                    cur = 0;
                }
                deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, cur, 0);
            }
        }
        else {
            if (event.clickCount == 2) {
                // new tab
                int playlist = cocoaui_add_new_playlist ();
                if (playlist != -1) {
                    deadbeef->plt_set_curr_idx (playlist);
                }
                return;
            }
            return;
        }

        // adjust scroll if clicked tab spans border
        if (self.needScroll) {
            [self scrollToTab:_clickedTabIndex];
        }

        int hscroll = self.scrollPos;
        int x = -hscroll + tabs_left_margin;
        int idx;
        for (idx = 0; idx < _clickedTabIndex; idx++) {
            int width = [self tabWidthForIndex:idx];
            x += width - tab_overlap_size;
        }

        _dragpt = coord;
        _dragpt.x -= x;
        _prepare = 1;
        _dragging = _clickedTabIndex;
        _prev_x = _dragpt.x;
        _tab_moved = 0;
        _movepos = coord.x - _dragpt.x;
        self.dragReallyBegan = NO;
    }
}

- (void)closePlaylist:(id)sender {
    DeletePlaylistConfirmationController *controller = [DeletePlaylistConfirmationController new];
    controller.window = self.window;
    controller.title = plt_get_title_wrapper (_clickedTabIndex);
    controller.delegate = self;
    [controller run];
}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent {
    NSPoint coord = [self convertPoint:theEvent.locationInWindow fromView:nil];
    _clickedTabIndex = [self tabUnderCursor:coord];
    if ((theEvent.type == NSEventTypeRightMouseDown || theEvent.type == NSEventTypeLeftMouseDown)
        && (theEvent.buttonNumber == 1
            || (theEvent.buttonNumber == 0 && (theEvent.modifierFlags & NSEventModifierFlagControl)))) {
        PlaylistContextMenu *menu = [[PlaylistContextMenu alloc] initWithView:self];
        menu.parentView = self;
        menu.clickPoint = coord;
        menu.delegate = self;
        menu.renamePlaylistDelegate = self;
        menu.deletePlaylistDelegate = self;
        menu.autoenablesItems = YES;

        ddb_playlist_t *plt = deadbeef->plt_get_for_idx ((int)_clickedTabIndex);
        deadbeef->action_set_playlist (plt);
        [menu updateWithPlaylist:plt];
        if (plt) {
            deadbeef->plt_unref (plt);
        }
        return menu;
    }
    return nil;
}

-(void)otherMouseDown:(NSEvent *)event {
    if (self.window.attachedSheet != nil) {
        return;
    }

    NSPoint coord = [self convertPoint:event.locationInWindow fromView:nil];
    _clickedTabIndex = [self tabUnderCursor:coord];
    if (event.type == NSEventTypeOtherMouseDown) {
        if (_clickedTabIndex == no_tab) {
            // new tab
            int playlist = cocoaui_add_new_playlist ();
            if (playlist != -1) {
                deadbeef->plt_set_curr_idx (playlist);
            }
            return;
        }
        else if (deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1)) {
            if (_clickedTabIndex != no_tab) {
                [self closePlaylist:self];
            }
        }
    }
}

-(void)mouseUp:(NSEvent *)event
{
    if (event.type == NSEventTypeLeftMouseUp) {
        if (_prepare || _dragging >= 0) {
            int dragged = _dragging;
            _dragging = no_tab;
            _prepare = 0;
            if (dragged != no_tab) {
                [self updatePointedTab:dragged];
            }
            self.needsDisplay = YES;
        }
        if (_closeTabCapture) {
            _closeTabCapture = NO;
            if (NSPointInRect(_lastMouseCoord, _closeTabButtonRect)) {
                [self closePointedTab];
            }
            else {
                self.needsDisplayInRect = _closeTabButtonRect;
            }
        }
    }
}

- (void)setupTrackingArea {
    // setup tracking area covering entire view, for managing the tab close buttons and tooltips
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }

    NSRect frame = self.frame;
    _trackingArea = [[NSTrackingArea alloc] initWithRect:NSMakeRect(0,0,frame.size.width,frame.size.height) options:NSTrackingMouseMoved|NSTrackingMouseEnteredAndExited|NSTrackingActiveAlways owner:self userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)setFrame:(NSRect)frame {
    super.frame = frame;
    [self frameDidChange];
}

- (void)frameDidChange {
    [self recalculateNeedScroll];
    [self adjustHScroll];
    [self setupTrackingArea];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint coord = [self convertPoint:event.locationInWindow fromView:nil];
    _lastMouseCoord = coord;

    if (_closeTabCapture) {
        self.needsDisplayInRect = _closeTabButtonRect;
        return;
    }

    if (([NSEvent pressedMouseButtons] & 1) && _prepare) {
        if (fabs (coord.x - _prev_x) > 3) {
            _prepare = 0;
        }
    }
    if (!_prepare && _dragging >= 0) {
        _movepos = coord.x - _dragpt.x;
        self.dragReallyBegan = YES;

        // find closest tab to the left
        int idx;
        int hscroll = self.scrollPos;
        int x = -hscroll + tabs_left_margin;
        int inspos = no_tab;
        int cnt = deadbeef->plt_get_count ();
        int dw = [self tabWidthForIndex:_dragging] - tab_overlap_size;
        for (idx = 0; idx < cnt; idx++) {
            int width = [self tabWidthForIndex:idx] - tab_overlap_size;

            if (idx < _dragging && _movepos <= x + width/2) {
                inspos = idx;
                break;
            }
            else if (idx > _dragging && _movepos > x + width/2 - dw) {
                inspos = idx;
            }

            x += width;
        }

        if (inspos >= 0 && inspos != _dragging) {
            deadbeef->plt_move (_dragging, inspos);
            _tab_moved = 1;
            _dragging = inspos;
            deadbeef->plt_set_curr_idx (_dragging);
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_POSITION, 0);
        }
        self.needsDisplay = YES;
    }
}

- (BOOL)mouseDownCanMoveWindow {
    return NO;
}

-(void)mouseMovedHandler {
    int tab = [self tabUnderCursor:_lastMouseCoord];
    if (tab >= 0) {
        NSString *s = plt_get_title_wrapper (tab);

        int width = [self tabWidthForIndex:tab];
        width += text_left_padding + text_right_padding;
        if (width > max_tab_size) {
            self.toolTip = s;
        }
        else {
            self.toolTip = nil;
        }
    }
    else {
        self.toolTip = nil;
    }

    [self updatePointedTab:tab];
}

-(void)mouseMoved:(NSEvent *)event {
    _lastMouseCoord = [self convertPoint:event.locationInWindow fromView:nil];
    [self mouseMovedHandler];
}

-(void)mouseExited:(NSEvent *)event {
    _lastMouseCoord = no_point;
    if (_pointedTab != no_tab) {
        [self updatePointedTab:no_tab];
        self.needsDisplay = YES;
    }
}

- (BOOL)wantsPeriodicDraggingUpdates {
    // we only want to be informed of dnd drag updates when mouse moves
    return NO;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    [self.pickDragTimer invalidate];

    NSPoint coord = sender.draggingLocation;
    coord = [self convertPoint:coord fromView:nil];
    weakify(self);
    self.pickDragTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:NO block:^(NSTimer * _Nonnull timer) {
        strongify(self);
        if (self == nil) {
            return;
        }
        int tabUnderCursor = [self tabUnderCursor:coord];
        if (tabUnderCursor != no_tab) {
            deadbeef->plt_set_curr_idx (tabUnderCursor);
        }
        self.pickDragTimer = nil;
    }];

    return NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    [self.pickDragTimer invalidate];
    self.pickDragTimer = nil;
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    // redraw if playlist switches, recalculate tabs when title changes
    if (_id == DB_EV_PLAYLISTSWITCHED || _id == DB_EV_PLAYLISTCHANGED) {
        dispatch_async(dispatch_get_main_queue(), ^{
            switch (_id) {
            case DB_EV_PLAYLISTSWITCHED:
                [self frameDidChange];
                [self scrollToTab:deadbeef->plt_get_curr_idx()];
                self.needsDisplay = YES;
                break;
            case DB_EV_PLAYLISTCHANGED:
                self.needsDisplay = YES;
                break;
            }
        });
    }
    return 0;
}

- (int)clickedTab {
    return _clickedTabIndex;
}

#pragma mark - RenamePlaylistViewControllerDelegate

- (void)renamePlaylist:(RenamePlaylistViewController *)viewController doneWithName:(NSString *)name {
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (_clickedTabIndex);
    if (plt == NULL) {
        return;
    }
    deadbeef->plt_set_title (plt, name.UTF8String);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    [self scrollToTab:_clickedTabIndex];
}

#pragma mark - DeletePlaylistConfirmationControllerDelegate

- (void)deletePlaylistDone:(DeletePlaylistConfirmationController *)controller {
    deadbeef->plt_remove (self.clickedTabIndex);
    self.clickedTabIndex = no_tab;
    self.needsDisplay = YES; // NOTE: this was added to redraw after a context menu
}

#pragma mark - TrackContextMenuDelegate


- (void)trackContextMenuDidDeleteFiles:(nonnull TrackContextMenu *)trackContextMenu cancelled:(BOOL)cancelled {
}

- (void)trackContextMenuDidReloadMetadata:(nonnull TrackContextMenu *)trackContextMenu {
}

- (void)trackContextMenuShowTrackProperties:(nonnull TrackContextMenu *)trackContextMenu {
    if (!self.trkProperties) {
        self.trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
        self.trkProperties.context = DDB_ACTION_CTX_PLAYLIST;
        self.trkProperties.delegate = self;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx ((int)self.clickedTabIndex);
    self.trkProperties.playlist =  plt;
    deadbeef->plt_unref (plt);
    [self.trkProperties showWindow:self];
}

#pragma mark - TrackPropertiesWindowControllerDelegate

- (void)trackPropertiesWindowControllerDidUpdateTracks:(TrackPropertiesWindowController *)windowController {
}

@end
