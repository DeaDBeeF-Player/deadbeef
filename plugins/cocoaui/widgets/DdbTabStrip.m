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
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@interface DdbTabStrip () {
    NSButton *_scrollLeftBtn;
    NSButton *_scrollRightBtn;
    BOOL _needArrows;
    int _hscrollpos;
    int _dragging;
    int _prepare;
    int _movepos;
    int _tab_clicked;
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
@end

@implementation DdbTabStrip

static int text_left_padding = 15;
static int text_right_padding = 0; // calculated from widget height
static int text_vert_offset = 3;
static int tab_overlap_size = 0; // approximately widget_height/2
static int tabs_left_margin = 0;
static int tab_vert_padding = 1;
static int min_tab_size = 80;
static int max_tab_size = 200;
static int close_btn_right_offs = 16;
#define arrow_widget_width ([self frame].size.height)

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        _dragging = -1;
        _tab_clicked = -1;
        
        _scrollLeftBtn = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, frame.size.height, frame.size.height-6)];
        [_scrollLeftBtn setBordered:NO];
        [_scrollLeftBtn setImage:[NSImage imageNamed:NSImageNameGoLeftTemplate]];
        [[_scrollLeftBtn cell] setImageScaling:NSImageScaleProportionallyDown];
        [_scrollLeftBtn setHidden:YES];
        [_scrollLeftBtn setAutoresizingMask:NSViewMaxXMargin];
        [_scrollLeftBtn setTarget:self];
        [_scrollLeftBtn setAction:@selector(scrollLeft)];
        [self addSubview:_scrollLeftBtn];

        _scrollRightBtn = [[NSButton alloc] initWithFrame:NSMakeRect(frame.size.width-frame.size.height, 0, frame.size.height, frame.size.height-6)];
        [_scrollRightBtn setBordered:NO];
        [_scrollRightBtn setImage:[NSImage imageNamed:NSImageNameGoRightTemplate]];
        [[_scrollRightBtn cell] setImageScaling:NSImageScaleProportionallyDown];
        [_scrollRightBtn setHidden:YES];
        [_scrollRightBtn setAutoresizingMask:NSViewMinXMargin];
        [_scrollRightBtn setTarget:self];
        [_scrollRightBtn setAction:@selector(scrollRight)];
        [self addSubview:_scrollRightBtn];

        _lastMouseCoord.x = -100000;
        _pointedTab = -1;

        [self setAutoresizesSubviews:YES];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleResizeNotification) name:NSViewFrameDidChangeNotification object:self];

        [self setupTrackingArea];

        [self setScrollPos:deadbeef->conf_get_int ("cocoaui.tabscroll", 0)];

        [self registerForDraggedTypes:[NSArray arrayWithObjects:ddbPlaylistItemsUTIType, NSFilenamesPboardType, nil]];
    }
    return self;
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

- (int)getTabWith:(int)tab {
    NSString *title = plt_get_title_wrapper (tab);
    NSSize sz = [title sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:nil]];
    sz.width += text_left_padding + text_right_padding;
    if (sz.width < min_tab_size) {
        sz.width = min_tab_size;
    }
    else if (sz.width > max_tab_size) {
        sz.width = max_tab_size;
    }
    return sz.width;
}

- (void)recalculateNeedArrows {
    BOOL origValue = _needArrows;
    _needArrows = NO;
    int cnt = deadbeef->plt_get_count ();
    int w = 0;
    NSRect a = [self bounds];
    for (int idx = 0; idx < cnt; idx++) {
        w += [self getTabWith:idx] - tab_overlap_size;
        if (w >= a.size.width) {
            _needArrows = YES;
            break;
        }
    }
    w += tab_overlap_size + 3;
    if (w >= a.size.width) {
        _needArrows = YES;
    }
    if (origValue != _needArrows) {
        [_scrollLeftBtn setHidden:!_needArrows];
        [_scrollRightBtn setHidden:!_needArrows];
        [self needsDisplay];
    }
}

- (BOOL)needArrows {
    return _needArrows;
}

- (int)getFullWidth {
    int fullwidth = 0;
    int cnt = deadbeef->plt_get_count ();
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = [self getTabWith:idx];
        if (idx == cnt-1) {
            tab_w += 3;
        }
        fullwidth += tab_w - tab_overlap_size;
    }
    fullwidth += tab_overlap_size;
    return fullwidth;
}

- (void)setScrollPos:(int)scrollPos {
    _hscrollpos = scrollPos;
}

- (void)updateScrollButtons {
    int tab_selected = deadbeef->plt_get_curr_idx ();
    [_scrollLeftBtn setEnabled:tab_selected > 0];
    [_scrollRightBtn setEnabled:tab_selected < deadbeef->plt_get_count ()-1];
}

- (void)scrollToTabInt:(int)tab redraw:(BOOL)redraw {
    int w = tabs_left_margin;
    int cnt = deadbeef->plt_get_count ();
    NSSize a = [self bounds].size;
    int boundary = a.width - arrow_widget_width*2 + _hscrollpos;
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = [self getTabWith:idx];
        if (idx == cnt-1) {
            tab_w += 3;
        }
        if (idx == tab) {
            if (w < _hscrollpos) {
                [self setScrollPos:w];
                deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
                if (redraw) {
                    [self setNeedsDisplay:YES];
                }
            }
            else if (w + tab_w >= boundary) {
                [self setScrollPos:_hscrollpos + (w + tab_w) - boundary];
                deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
                if (redraw) {
                    [self setNeedsDisplay:YES];
                }
            }
            break;
        }
        w += tab_w - tab_overlap_size;
    }
    [self updateScrollButtons];
}

- (void)adjustHScroll {
    if (deadbeef->plt_get_count () > 0) {
        BOOL need_arrows = [self needArrows];
        if (need_arrows) {
            NSSize a = [self bounds].size;
            int w = 0;
            int cnt = deadbeef->plt_get_count ();
            for (int idx = 0; idx < cnt; idx++) {
                w += [self getTabWith:idx] - tab_overlap_size;
            }
            w += tab_overlap_size + 3;
            if (_hscrollpos > w - (a.width - arrow_widget_width*2)) {
                [self setScrollPos:w - (a.width - arrow_widget_width*2)];
                deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
            }
            [self scrollToTabInt:deadbeef->plt_get_curr_idx () redraw:NO];
        }
        else {
            [self setScrollPos:0];
            deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
        }
    }
    [self mouseMovedHandler];
}

- (void)drawTab:(int)idx area:(NSRect)area selected:(BOOL)sel {
    [[NSGraphicsContext currentContext] saveGraphicsState];

    NSRect tabRect = area;

#if 0
    // curved tabs
    NSBezierPath *tab = [NSBezierPath bezierPath];
    NSPoint pt = tabRect.origin;
    pt.y += 0.5;

    [tab moveToPoint:pt]; // origin

    pt.x += 2;
    pt.y += 2;
    [tab curveToPoint:pt controlPoint1:NSMakePoint(pt.x, pt.y-2) controlPoint2:NSMakePoint(pt.x, pt.y+2)];
    pt.y += tabRect.size.height - (sel ? 8 : 10);
    [tab lineToPoint:pt];
    pt.x += 2;
    pt.y += 2;
    [tab curveToPoint:pt controlPoint1:NSMakePoint(pt.x-1, pt.y) controlPoint2:NSMakePoint(pt.x+1, pt.y)];
    pt.x += tabRect.size.width-9;
    [tab lineToPoint:pt];
    pt.x += 2;
    pt.y -= 2;
    [tab curveToPoint:pt controlPoint1:NSMakePoint(pt.x, pt.y-1) controlPoint2:NSMakePoint(pt.x, pt.y+1)];
    pt.y = tabRect.origin.y+3.5;
    [tab lineToPoint:pt];
    pt.x += 2;
    pt.y -= 2;
    [tab curveToPoint:pt controlPoint1:NSMakePoint(pt.x-2, pt.y) controlPoint2:NSMakePoint(pt.x+2, pt.y)];
    [tab closePath];
    [[[NSColor controlBackgroundColor] shadowWithLevel:sel?0.2:0.3] set];
    [tab fill];
    [[[NSColor controlBackgroundColor] shadowWithLevel:0.5] set];
    [tab stroke];
#endif

    // rectangular tabs
    tabRect.size.height++;

    if (!sel) {
        [[[NSColor blackColor] colorWithAlphaComponent:0.2] set];
        NSRect rc = tabRect;
        rc.size.width -= 1;
        NSBezierPath *tab = [NSBezierPath bezierPathWithRect:rc];
        [tab fill];
    }
    NSColor *clr = [_hiddenVertLine borderColor];
    [[clr colorWithAlphaComponent:0.5] set];
    NSBezierPath *line = [NSBezierPath bezierPath];
    [line moveToPoint:NSMakePoint(tabRect.origin.x + tabRect.size.width-0.5, tabRect.origin.y-1.5)];
    [line lineToPoint:NSMakePoint(tabRect.origin.x + tabRect.size.width-0.5, tabRect.origin.y+tabRect.size.height-1)];
    if (sel && _dragging >= 0) {
        [line moveToPoint:NSMakePoint(tabRect.origin.x-0.5, tabRect.origin.y-1.5)];
        [line lineToPoint:NSMakePoint(tabRect.origin.x-0.5, tabRect.origin.y+tabRect.size.height-1)];
    }
    [line stroke];

    // tab title
    int textoffs = sel ? 1 : 0;

    NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    [textStyle setAlignment:NSLeftTextAlignment];
    [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];

    NSFont *font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    NSDictionary *attrs = @{
        NSParagraphStyleAttributeName: textStyle,
        NSFontAttributeName:font,
        NSForegroundColorAttributeName:[NSColor controlTextColor]
    };

    NSString *tab_title = plt_get_title_wrapper (idx);

    [tab_title drawInRect:NSMakeRect(area.origin.x + text_left_padding, area.origin.y + text_vert_offset + textoffs - 10, area.size.width - (text_left_padding + text_right_padding - 1), area.size.height) withAttributes:attrs];

    // close button
    NSPoint from = NSMakePoint(area.origin.x + area.size.width - tab_overlap_size - close_btn_right_offs + 0.5, area.origin.y + 8+0.5);
    NSPoint to = from;
    to.x+=8;
    to.y+=8;
    NSRect atRect = [self getTabCloseRect:area];
    if (NSPointInRect (_lastMouseCoord, atRect)) {
        NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:atRect xRadius:1 yRadius:1];
        [[[NSColor controlTextColor] colorWithAlphaComponent:0.2] set];
        [path fill];
    }
    [[NSColor controlTextColor] set];
    [NSBezierPath strokeLineFromPoint: from toPoint: to ];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(from.x, to.y) toPoint: NSMakePoint(to.x, from.y) ];
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)clipTabArea {
    NSRect rect = NSMakeRect([self frame].size.height, 0, [self frame].size.width - [self frame].size.height*2, [self frame].size.height);
    [NSBezierPath clipRect:rect];
}

- (void)calculateTabDimensions {
    NSSize a = [self bounds].size;

    int h = a.height;

    tab_overlap_size = 0;//(h-4)/2;
    text_right_padding = h - 3 + 5;
}

- (NSRect)getTabRect:(int)xPos tabWidth:(int)tabWidth tabHeight:(int)tabHeight {
    return NSMakeRect(xPos, tab_vert_padding, tabWidth, tabHeight);

}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    int cnt = deadbeef->plt_get_count ();
    int hscroll = _hscrollpos;
    
    int need_arrows = [self needArrows];
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }
    
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
        NSString *title = plt_get_title_wrapper (idx);
        NSSize sz = [title sizeWithAttributes:nil];
        widths[idx] = sz.width;
        widths[idx] += text_left_padding + text_right_padding;
        if (widths[idx] < min_tab_size) {
            widths[idx] = min_tab_size;
        }
        else if (widths[idx] > max_tab_size) {
            widths[idx] = max_tab_size;
        }
    }
    
    [[NSGraphicsContext currentContext] saveGraphicsState];
    if ([self needArrows]) {
        [self clipTabArea];
    }

    // draw selected
    // calc position for drawin selected tab
    x = -hscroll;
    for (idx = 0; idx < tab_selected; idx++) {
        x += widths[idx] - tab_overlap_size;
    }
    x += tabs_left_margin;
    NSRect selectedTabRect;
    if (_dragging < 0 || _prepare || tab_selected != _dragging) {
        idx = tab_selected;
        w = widths[tab_selected];
        selectedTabRect = [self getTabRect:x tabWidth:w tabHeight:[self bounds].size.height];
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
                if (x >= [self bounds].size.width) {
                    break;
                }
                if (w > 0) {
                    // ***** draw dragging tab here *****
                    selectedTabRect = [self getTabRect:x tabWidth:w tabHeight:[self bounds].size.height];
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
        NSRect area = [self getTabRect:x tabWidth:w tabHeight:[self bounds].size.height];
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
            NSRect area = [self getTabRect:x tabWidth:w tabHeight:[self bounds].size.height];
            [self drawTab:idx area:area selected:NO];
        }
    }
    if ([self needArrows]) {
        [[NSGraphicsContext currentContext] restoreGraphicsState];
    }

    if ([self needArrows]) {
        [[NSGraphicsContext currentContext] saveGraphicsState];
        [self clipTabArea];
    }

    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

-(NSRect)tabRectForIndex:(int)tab {
    int width = 0;
    int cnt = deadbeef->plt_get_count ();
    for (int idx = 0; idx < cnt; idx++) {
        NSString *title = plt_get_title_wrapper (idx);
        NSSize sz = [title sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:nil]];
        int w = sz.width;
        w += text_left_padding + text_right_padding;
        if (w < min_tab_size) {
            w = min_tab_size;
        }
        else if (w > max_tab_size) {
            w = max_tab_size;
        }
        if (idx == tab) {
            return NSMakeRect(width - _hscrollpos + ([self needArrows] ? arrow_widget_width : 0), 0, w, [self frame].size.height);
        }
        width += w - tab_overlap_size;
    }
    return NSMakeRect(0, 0, 0, 0);
}

-(int)tabUnderCursor:(int)x {
    int hscroll = _hscrollpos;
    BOOL need_arrows = [self needArrows];
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }
    if (need_arrows && (x < arrow_widget_width || x >= [self frame].size.width - arrow_widget_width)) {
        return -1;
    }
    int idx;
    int cnt = deadbeef->plt_get_count ();
    int fw = tabs_left_margin - hscroll;
    for (idx = 0; idx < cnt; idx++) {
        NSString *title = plt_get_title_wrapper (idx);
        NSSize ex = [title sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:nil]];
        ex.width += text_left_padding + text_right_padding;
        if (ex.width < min_tab_size) {
            ex.width = min_tab_size;
        }
        else if (ex.width > max_tab_size) {
            ex.width = max_tab_size;
        }
        fw += ex.width;
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
    _pointedTab = tab;
}

-(void)closePointedTab {
    if (_pointedTab != -1) {
        _tab_clicked = _pointedTab;
        _pointedTab = -1;
        [self closePlaylist:self];
    }
}

-(void)scrollWheel:(NSEvent*)event {
    if (event.deltaY < 0 || event.deltaX < 0)
    {
        [self scrollRight];
    }
    else if (event.deltaY > 0 || event.deltaX > 0)
    {
        [self scrollLeft];
    }
}

-(NSRect)getTabCloseRect:(NSRect)area {
    NSPoint from = NSMakePoint(area.origin.x + area.size.width - tab_overlap_size - close_btn_right_offs + 0.5, area.origin.y + 8+0.5);
    NSRect atRect;
    atRect.origin = from;
    atRect.origin.x -= 2;
    atRect.origin.y -= 2;
    atRect.size = NSMakeSize(12, 12);
    return atRect;
}

- (BOOL)handleClickedTabCloseRect {
    int hscroll = _hscrollpos;
    if ([self needArrows]) {
        hscroll -= arrow_widget_width;
    }
    int x = -hscroll + tabs_left_margin;
    int idx;
    for (idx = 0; idx < _tab_clicked; idx++) {
        int width = [self getTabWith:idx];
        x += width - tab_overlap_size;
    }
    int w = [self getTabWith:_tab_clicked];
    NSRect area = NSMakeRect(x, tab_vert_padding, w, [self bounds].size.height);

    NSRect atRect = [self getTabCloseRect:area];

    if (!NSPointInRect(_lastMouseCoord, atRect)) {
        return NO;
    }

    [self updatePointedTab:_tab_clicked];
    atRect.size.height -= 4;
    _closeTabButtonRect = atRect;
    _closeTabCapture = YES;
    [self setNeedsDisplayInRect:atRect];
    return YES;
}

- (void)mouseDown:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _lastMouseCoord = coord;
    _tab_clicked = [self tabUnderCursor:coord.x];
    if (event.type == NSLeftMouseDown) {
        if ([self needArrows]) {
            NSSize a = [self bounds].size;
            if (coord.x < arrow_widget_width || coord.x >= a.width - arrow_widget_width) {
                [super mouseDown:event];
                return;
            }
        }
    
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
                    [self scrollToTab:playlist];
                }
                return;
            }
            return;
        }

        // adjust scroll if clicked tab spans border
        if ([self needArrows]) {
            [self scrollToTab:_tab_clicked];
        }

        int hscroll = _hscrollpos;
        if ([self needArrows]) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int idx;
        for (idx = 0; idx < _tab_clicked; idx++) {
            int width = [self getTabWith:idx];
            x += width - tab_overlap_size;
        }

        _dragpt = coord;
        _dragpt.x -= x;
        _prepare = 1;
        _dragging = _tab_clicked;
        _prev_x = _dragpt.x;
        _tab_moved = 0;
        _movepos = coord.x - _dragpt.x;
    }
}

- (void)closePlaylist:(id)sender {
    if (_tab_clicked != -1) {
        deadbeef->plt_remove (_tab_clicked);
        int playlist = deadbeef->plt_get_curr_idx ();
        deadbeef->conf_set_int ("playlist.current", playlist);
        [self scrollToTab:playlist];
        _tab_clicked = -1;
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
    if ((theEvent.type == NSRightMouseDown || theEvent.type == NSLeftMouseDown)
        && (theEvent.buttonNumber == 1
            || (theEvent.buttonNumber == 0 && (theEvent.modifierFlags & NSControlKeyMask)))) {
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@"TabMenu"];
        [menu setDelegate:(id<NSMenuDelegate>)self];
        [menu setAutoenablesItems:NO];
        [[menu insertItemWithTitle:@"Add New Playlist" action:@selector(addNewPlaylist:) keyEquivalent:@"" atIndex:0] setTarget:self];
        if (_tab_clicked != -1) {
            [[menu insertItemWithTitle:@"Close Playlist" action:@selector(closePlaylist:) keyEquivalent:@"" atIndex:0] setTarget:self];

            // ignore the warning, the message is sent to 1st responder, which will be the mainwincontroller in this case
            [menu insertItemWithTitle:@"Rename Playlist" action:@selector(renamePlaylistAction:) keyEquivalent:@"" atIndex:0];
        }
        return menu;
    }
    return nil;
}

-(void)otherMouseDown:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _tab_clicked = [self tabUnderCursor:coord.x];
    if (event.type == NSOtherMouseDown) {
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
    if (event.type == NSLeftMouseUp) {
        if (_prepare || _dragging >= 0) {
            int dragged = _dragging;
            _dragging = -1;
            _prepare = 0;
            if (dragged != -1) {
                [self updatePointedTab:dragged];
            }
            [self setNeedsDisplay:YES];
        }
        if (_closeTabCapture) {
            _closeTabCapture = NO;
            if (NSPointInRect(_lastMouseCoord, _closeTabButtonRect)) {
                [self closePointedTab];
            }
            else {
                [self setNeedsDisplayInRect:_closeTabButtonRect];
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

    NSRect frame = [self frame];
    _trackingArea = [[NSTrackingArea alloc] initWithRect:NSMakeRect(0,0,frame.size.width,frame.size.height) options:NSTrackingMouseMoved|NSTrackingMouseEnteredAndExited|NSTrackingActiveAlways owner:self userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)handleResizeNotification {
    [self calculateTabDimensions];
    [self recalculateNeedArrows];
    [self adjustHScroll];
    [self setupTrackingArea];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _lastMouseCoord = coord;

    if (_closeTabCapture) {
        [self setNeedsDisplayInRect:_closeTabButtonRect];
        return;
    }

    if (([NSEvent pressedMouseButtons] & 1) && _prepare) {
        if (fabs (coord.x - _prev_x) > 3) {
            _prepare = 0;
        }
    }
    if (!_prepare && _dragging >= 0) {
        _movepos = coord.x - _dragpt.x;

        // find closest tab to the left
        int idx;
        int hscroll = _hscrollpos;
        BOOL need_arrows = [self needArrows];
        if (need_arrows) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int inspos = -1;
        int cnt = deadbeef->plt_get_count ();
        int dw = [self getTabWith:_dragging] - tab_overlap_size;
        for (idx = 0; idx < cnt; idx++) {
            int width = [self getTabWith:idx] - tab_overlap_size;

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
        [self setNeedsDisplay:YES];
    }
}

- (BOOL)mouseDownCanMoveWindow {
    return NO;
}

-(void)mouseMovedHandler {
    int tab = [self tabUnderCursor:_lastMouseCoord.x];
    if (tab >= 0) {
        NSString *s = plt_get_title_wrapper (tab);

        NSSize sz = [s sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:nil]];
        sz.width += text_left_padding + text_right_padding;
        if (sz.width > max_tab_size) {
            [self setToolTip:s];
        }
        else {
            [self setToolTip:nil];
        }
    }
    else {
        [self setToolTip:nil];
    }
    [self updatePointedTab:tab];
    [self setNeedsDisplay:YES];
}

-(void)mouseMoved:(NSEvent *)event {
    _lastMouseCoord = [self convertPoint:[event locationInWindow] fromView:nil];
    [self mouseMovedHandler];
}

-(void)mouseExited:(NSEvent *)event {
    _lastMouseCoord.x = -100000;
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
                [self performSelectorOnMainThread:@selector(handleResizeNotification) withObject:nil waitUntilDone:NO];
                [self setNeedsDisplay:YES];
                break;
            case DB_EV_PLAYLISTCHANGED:
                [self setNeedsDisplay:YES];
                break;
            }
        });
    }
    return 0;
}

- (int)clickedTab {
    return _tab_clicked;
}

@end
