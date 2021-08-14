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

#import "DdbTabStrip.h"
#import "DdbShared.h"
#import "NSMenu+ActionItems.h"
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@interface DdbTabStrip () {
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

@property (nonatomic) BOOL playlistConfirmationAlertOpen;

@property (nonatomic) BOOL dragReallyBegan;

@property (nonatomic) int tab_clicked;
@property (weak) IBOutlet NSTextField *renamePlaylistTitle;
@property (nonatomic) BOOL needScroll;
@property (nonatomic) CGFloat scrollPos;
@property (nonatomic,readonly) int fullWidth;

@end

@implementation DdbTabStrip

static int text_left_padding = 24;
static int text_right_padding = 24;
static int tab_overlap_size = 0;
static int tabs_left_margin = 0;
static int tab_vert_padding = 1;
static int min_tab_size = 80;
static int max_tab_size = 200;
static int close_btn_left_offs = 8;

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
    _dragging = -1;
    self.tab_clicked = -1;
    self.autoresizesSubviews = NO;

    _lastMouseCoord.x = -100000;
    _pointedTab = -1;

    [self setupTrackingArea];

    [self scrollToTabInt:deadbeef->plt_get_curr_idx() redraw:NO];

    [self registerForDraggedTypes:[NSArray arrayWithObjects:ddbPlaylistItemsUTIType, NSFilenamesPboardType, nil]];

    [NSNotificationCenter.defaultCenter addObserver:self
                                             selector:@selector(windowDidBecomeKey:)
                                                 name:NSWindowDidBecomeKeyNotification
                                               object:self.window];
    [NSNotificationCenter.defaultCenter addObserver:self
                                             selector:@selector(windowDidBecomeKey:)
                                                 name:NSWindowDidResignKeyNotification
                                               object:self.window];
}

- (void)windowDidBecomeKey:(id)sender {
    self.needsDisplay = YES;
}

static NSString *
plt_get_title_wrapper (int plt) {
    if (plt == -1) {
        return @"";
    }
    ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
    
    char buffer[1000];
    deadbeef->plt_get_title (p, buffer, sizeof (buffer));
    deadbeef->plt_unref (p);
    return [NSString stringWithUTF8String:buffer];
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
        [self needsDisplay];
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

- (void)drawTab:(int)idx area:(NSRect)area selected:(BOOL)sel {
    [[NSGraphicsContext currentContext] saveGraphicsState];
    NSBezierPath.defaultLineWidth = 1;

    NSRect tabRect = area;

    // rectangular tabs
    tabRect.size.height++;

    // tab background
    if (sel) {
        NSColor *backgroundColor = self.tabBackgroundColor;
        [backgroundColor set];

        NSRect rc = tabRect;
        rc.origin.x -= 1;
        rc.size.width += 1;
        NSBezierPath *tab = [NSBezierPath bezierPathWithRect:rc];
        [tab fill];
    }


    // tab divider
    NSColor *clr = [_hiddenVertLine borderColor];
    [[clr colorWithAlphaComponent:0.5] set];
    NSBezierPath *line = [NSBezierPath bezierPath];
    [line moveToPoint:NSMakePoint(tabRect.origin.x + tabRect.size.width-0.5, tabRect.origin.y+5)];
    [line lineToPoint:NSMakePoint(tabRect.origin.x + tabRect.size.width-0.5, tabRect.origin.y+tabRect.size.height-10)];
    if (sel && _dragging >= 0) {
        [line moveToPoint:NSMakePoint(tabRect.origin.x-0.5, tabRect.origin.y+5)];
        [line lineToPoint:NSMakePoint(tabRect.origin.x-0.5, tabRect.origin.y+tabRect.size.height-10)];
    }
    [line stroke];

    // tab title
    NSDictionary *attrs = sel ? self.titleAttributesSelected : self.titleAttributes;

    NSString *tab_title = plt_get_title_wrapper (idx);

    NSSize size = [tab_title sizeWithAttributes:attrs];

    [tab_title drawInRect:NSMakeRect(area.origin.x + text_left_padding, area.origin.y + area.size.height/2 - size.height/2, area.size.width - (text_left_padding + text_right_padding - 1), size.height) withAttributes:attrs];

    [[NSGraphicsContext currentContext] restoreGraphicsState];

    // close button
    if (idx == _pointedTab && (_dragging == -1 || !self.dragReallyBegan)) {
        NSRect atRect = [self tabCloseButtonRectForTabRect:area];
        NSPoint from = atRect.origin;
        from.x += 2;
        from.y += 2;
        NSPoint to = from;
        to.x+=8;
        to.y+=8;
        if (NSPointInRect (_lastMouseCoord, atRect)) {
            NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:atRect xRadius:1 yRadius:1];
            CGFloat alpha = _closeTabCapture?0.4:0.2;
            if (!self.isKeyWindow) {
                alpha *= 0.7;
            }

            NSColor *closeButtonBackgroundColor = [NSColor.controlTextColor colorWithAlphaComponent:alpha];

            [closeButtonBackgroundColor set];
            [path fill];
        }
        [self.tabTextColor set];
        NSBezierPath.defaultLineWidth = 2;
        [NSBezierPath strokeLineFromPoint: from toPoint: to ];
        [NSBezierPath strokeLineFromPoint: NSMakePoint(from.x, to.y) toPoint: NSMakePoint(to.x, from.y) ];
    }
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
    if (tab_selected == -1) {
        return;
    }
    
    int tab_playing = -1;
    DB_playItem_t *playing = deadbeef->streamer_get_playing_track ();
    if (playing) {
        ddb_playlist_t *plt = deadbeef->pl_get_playlist (playing);
        if (plt) {
            tab_playing = deadbeef->plt_get_idx (plt);
            deadbeef->plt_unref (plt);
        }
        deadbeef->pl_item_unref (playing);
    }
    
    int need_draw_moving = 0;
    int idx;
    int widths[cnt];
    for (idx = 0; idx < cnt; idx++) {
        widths[idx] = [self tabWidthForIndex:idx];
    }
    
    [NSGraphicsContext.currentContext saveGraphicsState];

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
        [self drawTab:idx area:selectedTabRect selected:YES];
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
                    [self drawTab:tab_selected area:selectedTabRect selected:YES];

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
                int undercursor = [self tabUnderCursor:_lastMouseCoord.x];
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
    int c = tab_selected == -1 ? cnt : tab_selected;
    for (idx = 0; idx < c; idx++) {
        w = widths[idx];
        NSRect area = [self tabRectForXPos:x width:w height:self.bounds.size.height];
        [self drawTab:idx area:area selected:NO];
        x += w - tab_overlap_size;
    }
    // draw tabs on the right
    if (tab_selected != -1 && tab_selected != cnt-1) {
        x = -hscroll + tabs_left_margin;
        for (idx = 0; idx < cnt; idx++) {
            x += widths[idx] - tab_overlap_size;
        }
        for (idx = cnt-1; idx > tab_selected; idx--) {
            w = widths[idx];
            x -= w - tab_overlap_size;
            NSRect area = [self tabRectForXPos:x width:w height:self.bounds.size.height];
            [self drawTab:idx area:area selected:NO];
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

-(int)tabUnderCursor:(int)x {
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
    return -1;
}

-(void)scrollToTab:(int)tab {
    [self scrollToTabInt:tab redraw:YES];
}

-(void)scrollLeft {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab > 0) {
        tab--;
        cocoaui_playlist_set_curr (tab);
    }
    [self scrollToTab:tab];
}

-(void)scrollRight {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab < deadbeef->plt_get_count ()-1) {
        tab++;
        cocoaui_playlist_set_curr (tab);
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
    if (_pointedTab != -1) {
        self.tab_clicked = _pointedTab;
        _pointedTab = -1;
        [self closePlaylist:self];
    }
}

-(void)scrollWheel:(NSEvent*)event {
    CGFloat newScroll = self.scrollPos + event.deltaY;
    newScroll -= event.deltaY * 4;
    self.scrollPos = newScroll;
    self.needsDisplay = YES;
}

-(NSRect)tabCloseButtonRectForTabRect:(NSRect)tabRect {
    NSPoint from = NSMakePoint(tabRect.origin.x + close_btn_left_offs + 0.5, tabRect.origin.y + tabRect.size.height/2 - 6);
    NSRect atRect;
    atRect.origin = from;
    atRect.origin.x -= 2;
    atRect.size = NSMakeSize(12, 12);
    return atRect;
}

- (BOOL)handleClickedTabCloseRect {
    int hscroll = self.scrollPos;
    int x = -hscroll + tabs_left_margin;
    int idx;
    for (idx = 0; idx < self.tab_clicked; idx++) {
        int width = [self tabWidthForIndex:idx];
        x += width - tab_overlap_size;
    }
    int w = [self tabWidthForIndex:_tab_clicked];

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
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _lastMouseCoord = coord;
    _tab_clicked = [self tabUnderCursor:coord.x];
    if (event.type == NSEventTypeLeftMouseDown) {

        if (_tab_clicked != -1) {
            if ([self handleClickedTabCloseRect]) {
                return;
            }
            cocoaui_playlist_set_curr (_tab_clicked);
        }
        else {
            if (event.clickCount == 2) {
                // new tab
                int playlist = cocoaui_add_new_playlist ();
                if (playlist != -1) {
                    cocoaui_playlist_set_curr (playlist);
                }
                return;
            }
            return;
        }

        // adjust scroll if clicked tab spans border
        if (self.needScroll) {
            [self scrollToTab:_tab_clicked];
        }

        int hscroll = self.scrollPos;
        int x = -hscroll + tabs_left_margin;
        int idx;
        for (idx = 0; idx < _tab_clicked; idx++) {
            int width = [self tabWidthForIndex:idx];
            x += width - tab_overlap_size;
        }

        _dragpt = coord;
        _dragpt.x -= x;
        _prepare = 1;
        _dragging = _tab_clicked;
        _prev_x = _dragpt.x;
        _tab_moved = 0;
        _movepos = coord.x - _dragpt.x;
        self.dragReallyBegan = NO;
    }
}

- (void)closePlaylist:(id)sender {
    if (_tab_clicked != -1) {
        NSAlert *alert = [NSAlert new];

        alert.messageText = @"Removing playlist";
        alert.informativeText = [NSString stringWithFormat:@"Do you really want to remove the playlist '%@'?", plt_get_title_wrapper (_tab_clicked)];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Yes"];
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
        if (@available(macOS 10.16, *)) {
            alert.buttons[1].hasDestructiveAction = YES;
        }
#endif

        self.playlistConfirmationAlertOpen = YES;

        [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
            self.playlistConfirmationAlertOpen = NO;
            if (returnCode == NSAlertFirstButtonReturn) {
                self.needsDisplay = YES;
                return;
            }
            deadbeef->plt_remove (self.tab_clicked);
            int playlist = deadbeef->plt_get_curr_idx ();
            deadbeef->conf_set_int ("playlist.current", playlist);
            [self scrollToTab:playlist];
            self.tab_clicked = -1;
            self.needsDisplay = YES;
        }];
    }
}

- (void)addNewPlaylist:(id)sender {
    int playlist = cocoaui_add_new_playlist ();
    if (playlist != -1) {
        cocoaui_playlist_set_curr (playlist);
    }
}

- (NSMenu *)menuForEvent:(NSEvent *)theEvent {
    NSPoint coord = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    _tab_clicked = [self tabUnderCursor:coord.x];
    if ((theEvent.type == NSEventTypeRightMouseDown || theEvent.type == NSEventTypeLeftMouseDown)
        && (theEvent.buttonNumber == 1
            || (theEvent.buttonNumber == 0 && (theEvent.modifierFlags & NSEventModifierFlagControl)))) {
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@"TabMenu"];
        menu.delegate = self;
        menu.autoenablesItems = YES;
        [menu insertItemWithTitle:@"Add New Playlist" action:@selector(addNewPlaylist:) keyEquivalent:@"" atIndex:0].target = self;
        if (_tab_clicked != -1) {
            [menu insertItemWithTitle:@"Close Playlist" action:@selector(closePlaylist:) keyEquivalent:@"" atIndex:0].target = self;

            // ignore the warning, the message is sent to 1st responder, which will be the mainwincontroller in this case
            NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:@"Rename Playlist" action:@selector(renamePlaylistAction:) keyEquivalent:@""];
            [menu insertItem:item atIndex:0];
        }

        [menu addActionItemsForContext:DDB_ACTION_CTX_PLAYLIST track:NULL filter:^BOOL(DB_plugin_action_t * _Nonnull action) {

            if (!(action->flags & DB_ACTION_MULTIPLE_TRACKS)) {
                return NO;
            }

            if (action->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST) {
                return NO;
            }

            return YES;
        }];


        return menu;
    }
    return nil;
}

-(void)otherMouseDown:(NSEvent *)event {
    if (self.playlistConfirmationAlertOpen) {
        return;
    }

    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _tab_clicked = [self tabUnderCursor:coord.x];
    if (event.type == NSEventTypeOtherMouseDown) {
        if (_tab_clicked == -1) {
            // new tab
            int playlist = cocoaui_add_new_playlist ();
            if (playlist != -1) {
                cocoaui_playlist_set_curr (playlist);
            }
            return;
        }
        else if (deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1)) {
            if (_tab_clicked != -1) {
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
            _dragging = -1;
            _prepare = 0;
            if (dragged != -1) {
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
    [super setFrame:frame];
    [self frameDidChange];
}

- (void)frameDidChange {
    [self recalculateNeedScroll];
    [self adjustHScroll];
    [self setupTrackingArea];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
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
        int inspos = -1;
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
            deadbeef->conf_set_int ("playlist.current", _dragging);
        }
        self.needsDisplay = YES;
    }
}

- (BOOL)mouseDownCanMoveWindow {
    return NO;
}

-(void)mouseMovedHandler {
    int tab = [self tabUnderCursor:_lastMouseCoord.x];
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
    _lastMouseCoord = [self convertPoint:[event locationInWindow] fromView:nil];
    [self mouseMovedHandler];
}

-(void)mouseExited:(NSEvent *)event {
    _lastMouseCoord.x = -100000;
    if (_pointedTab != -1) {
        [self updatePointedTab:-1];
        self.needsDisplay = YES;
    }

}

- (BOOL)wantsPeriodicDraggingUpdates {
    // we only want to be informed of dnd drag updates when mouse moves
    return NO;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {

    NSPoint coord = [sender draggingLocation];
    int tabUnderCursor = [self tabUnderCursor: coord.x];
    if (tabUnderCursor != -1) {
        cocoaui_playlist_set_curr (tabUnderCursor);
    }

    return NSDragOperationNone;
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
    return _tab_clicked;
}

- (IBAction)renamePlaylistAction:(id)sender {
    int clicked = _tab_clicked;
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx (clicked);
    int l = deadbeef->plt_get_title (plt, NULL, 0);
    char buf[l+1];
    deadbeef->plt_get_title (plt, buf, (int)sizeof buf);
    self.renamePlaylistTitle.stringValue = [NSString stringWithUTF8String:buf];
    [self.window beginSheet:self.renamePlaylistWindow completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            deadbeef->plt_set_title (plt, [[self.renamePlaylistTitle stringValue] UTF8String]);
            deadbeef->plt_save_config (plt);
            deadbeef->plt_unref (plt);
            [self scrollToTab:clicked];
        }
    }];
}

- (IBAction)renamePlaylistCancelAction:(id)sender {
    [NSApp endSheet:self.renamePlaylistWindow returnCode:NSModalResponseCancel];
}

- (IBAction)renamePlaylistOKAction:(id)sender {
    [NSApp endSheet:self.renamePlaylistWindow returnCode:NSModalResponseOK];
}

@end
