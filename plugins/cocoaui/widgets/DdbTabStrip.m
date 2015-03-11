//
//  DdbTabStrip.m
//  deadbeef
//
//  Created by waker on 03/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//
#import "DdbTabStrip.h"
#import "DdbShared.h"
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbTabStrip

static int text_left_padding = 15;
static int text_right_padding = 0; // calculated from widget height
static int text_vert_offset = 3;
static int tab_overlap_size = 0; // widget_height/2
static int tabs_left_margin = 4;
static int tab_vert_padding = 1;
static int min_tab_size = 80;
static int max_tab_size = 200;
#define arrow_sz 10
#define arrow_widget_width (arrow_sz+4)

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        _dragging = -1;
        _tab_clicked = -1;
        
        _tabLeft = [NSImage imageNamed:@"tab_left"];
        [_tabLeft setFlipped:YES];
        _tabFill = [NSImage imageNamed:@"tab_fill"];
        [_tabFill setFlipped:YES];
        _tabRight = [NSImage imageNamed:@"tab_right"];
        [_tabRight setFlipped:YES];
        
        _tabUnselLeft = [NSImage imageNamed:@"tab_unsel_left"];
        [_tabUnselLeft setFlipped:YES];
        _tabUnselFill = [NSImage imageNamed:@"tab_unsel_fill"];
        [_tabUnselFill setFlipped:YES];
        _tabUnselRight = [NSImage imageNamed:@"tab_unsel_right"];
        [_tabUnselRight setFlipped:YES];
        
        _tabBottomFill = [NSImage imageNamed:@"tab_bottom_fill"];
        [_tabUnselRight setFlipped:YES];
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

- (BOOL)needArrows {
    int cnt = deadbeef->plt_get_count ();
    int w = 0;
    NSRect a = [self bounds];
    for (int idx = 0; idx < cnt; idx++) {
        w += [self getTabWith:idx] - tab_overlap_size;
        if (w >= a.size.width) {
            return YES;
        }
    }
    w += tab_overlap_size + 3;
    if (w >= a.size.width) {
        return YES;
    }
    return NO;
}

- (void)scrollToTabInt:(int)tab redraw:(BOOL)redraw {
    int w = 0;
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
                _hscrollpos = w;
                deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
                if (redraw) {
                    [self setNeedsDisplay:YES];
                }
            }
            else if (w + tab_w >= boundary) {
                _hscrollpos += (w+tab_w) - boundary;
                deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
                if (redraw) {
                    [self setNeedsDisplay:YES];
                }
            }
            break;
        }
        w += tab_w - tab_overlap_size;
    }
}

- (void)adjustHScroll {
    _hscrollpos = deadbeef->conf_get_int ("cocoaui.tabscroll", 0);
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
                _hscrollpos = w - (a.width - arrow_widget_width*2);
                deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
            }
            [self scrollToTabInt:deadbeef->plt_get_curr_idx () redraw:NO];
        }
        else {
            _hscrollpos = 0;
            deadbeef->conf_set_int ("cocoaui.tabscroll", _hscrollpos);
        }
    }
}

- (void)drawTab:(int)idx area:(NSRect)area selected:(BOOL)sel {
    NSImage *tleft = sel ? _tabLeft : _tabUnselLeft;
    NSImage *tright = sel ? _tabRight : _tabUnselRight;
    NSImage *tfill = sel ? _tabFill : _tabUnselFill;
    
    if (!sel) {
        area.origin.y += 2;
    }
    
    [tleft drawAtPoint:area.origin fromRect:NSMakeRect(0,0,[tleft size].width,[tleft size].height) operation:NSCompositeSourceOver fraction:1];
    
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [[NSColor colorWithPatternImage:tfill] set];
    NSPoint convPt = [self convertPoint:NSMakePoint(0, area.origin.y) fromView:nil];
    [gc setPatternPhase:convPt];
    [NSBezierPath fillRect:NSMakeRect(area.origin.x + [tleft size].width, area.origin.y, area.size.width-[tleft size].width-[tright size].width, [tfill size].height)];
    [gc restoreGraphicsState];
    
    [tright drawAtPoint:NSMakePoint(area.origin.x+area.size.width-[tleft size].width, area.origin.y) fromRect:NSMakeRect(0,0,[_tabRight size].width,[tright size].height) operation:NSCompositeSourceOver fraction:1];

    NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    [textStyle setAlignment:NSLeftTextAlignment];
    [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];
    
    NSDictionary *attrs = [NSDictionary dictionaryWithObjectsAndKeys
                           : textStyle, NSParagraphStyleAttributeName
                           , nil];
    NSString *tab_title = plt_get_title_wrapper (idx);
    
    [tab_title drawInRect:NSMakeRect(area.origin.x + text_left_padding, area.origin.y + text_vert_offset, area.size.width - (text_left_padding + text_right_padding - 1), area.size.height) withAttributes:attrs];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    [[NSColor windowBackgroundColor] set];
    [NSBezierPath fillRect:[self bounds]];
    
    [self adjustHScroll];

    int cnt = deadbeef->plt_get_count ();
    int hscroll = _hscrollpos;
    
    int need_arrows = [self needArrows];
    if (need_arrows) {
        hscroll -= arrow_widget_width;
    }
    
    int x = -hscroll;
    int w = 0;
    NSSize a = [self bounds].size;
    
    int h = a.height;

    tab_overlap_size = (h-4)/2;
    text_right_padding = h - 3;
    
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
    
    h = a.height - 4;
    int need_draw_moving = 0;
    int idx;
    int widths[cnt];
    for (idx = 0; idx < cnt; idx++) {
        NSString *title = plt_get_title_wrapper (idx);
        NSSize sz = [title sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:nil]];
        widths[idx] = sz.width;
        widths[idx] += text_left_padding + text_right_padding;
        if (widths[idx] < min_tab_size) {
            widths[idx] = min_tab_size;
        }
        else if (widths[idx] > max_tab_size) {
            widths[idx] = max_tab_size;
        }
    }
    
    x = -hscroll + tabs_left_margin;
    
    // draw tabs on the left
    int c = tab_selected == -1 ? cnt : tab_selected;
    for (idx = 0; idx < c; idx++) {
        w = widths[idx];
        NSRect area = NSMakeRect(x, tab_vert_padding, w, a.height);
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
            NSRect area = NSMakeRect(x, tab_vert_padding, w, a.height);
            [self drawTab:idx area:area selected:NO];
        }
    }
    
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [[NSColor colorWithPatternImage:_tabBottomFill] set];
    int offs = [_tabLeft size].height-3+tab_vert_padding;
    NSPoint convPt = [self convertPoint:NSMakePoint(0,offs) fromView:nil];
    [gc setPatternPhase:convPt];
    [NSBezierPath fillRect:NSMakeRect(0, offs, [self bounds].size.width, [_tabBottomFill size].height)];
    [gc restoreGraphicsState];
    

    // calc position for drawin selected tab
    x = -hscroll;
    for (idx = 0; idx < tab_selected; idx++) {
        x += widths[idx] - tab_overlap_size;
    }
    x += tabs_left_margin;
    // draw selected
    if (_dragging < 0 || _prepare || tab_selected != _dragging) {
        idx = tab_selected;
        w = widths[tab_selected];
        [self drawTab:idx area:NSMakeRect(x, tab_vert_padding, w, [self bounds].size.height) selected:YES];
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
                if (x >= a.width) {
                    break;
                }
                if (w > 0) {
                    // ***** draw dragging tab here *****
                    [self drawTab:tab_selected area:NSMakeRect(x, tab_vert_padding, w, a.height) selected:YES];
                }
                break;
            }
            x += w - tab_overlap_size;
        }
    }
    if (need_arrows) {
#if 0
        int sz = a.height-3;
        gtk_paint_arrow (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_LEFT, TRUE, 2, sz/2-arrow_sz/2, arrow_sz, arrow_sz);
        gtk_paint_arrow (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_NONE, NULL, widget, NULL, GTK_ARROW_RIGHT, TRUE, widget->allocation.width-arrow_sz-2, 1+sz/2-arrow_sz/2, arrow_sz, arrow_sz);
#endif
    }
}

-(int)tabUnderCursor:(int)x {
    int hscroll = _hscrollpos;
    BOOL need_arrows = [self needArrows];
    if (need_arrows) {
        hscroll -= arrow_widget_width;
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

#if 0
- (BOOL)scrollCB {
    if (scroll_direction < 0) {
        [self scrollLeft];
    }
    else if (ts->scroll_direction > 0) {
        [self scrollRigth];
    }
    else {
        return NO;
    }
    return YES;
}
#endif

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

- (void)mouseDown:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _tab_clicked = [self tabUnderCursor:coord.x];
    if (event.type == NSLeftMouseDown) {
        BOOL need_arrows = [self needArrows];
        if (need_arrows) {
            NSSize a = [self bounds].size;
            if (coord.x < arrow_widget_width) {
                [self scrollLeft];
                _scroll_direction = -1;
                // start periodic upd
//                scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                return;
            }
            else if (coord.x >= a.width - arrow_widget_width) {
                [self scrollRight];
                _scroll_direction = 1;
            // start periodic upd
//                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                return;
            }
        }
    
        if (_tab_clicked != -1) {
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
        if (need_arrows) {
            [self scrollToTab:_tab_clicked];
        }
        
        int hscroll = _hscrollpos;
        if (need_arrows) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int idx;
        for (idx = 0; idx < _tab_clicked; idx++) {
            int width = [self getTabWith:idx];
            x += width - tab_overlap_size;
        }
        _dragpt = [self convertPoint:[event locationInWindow] fromView:nil];
        _dragpt.x -= x;
        _prepare = 1;
        _dragging = _tab_clicked;
        _prev_x = _dragpt.x;
        _tab_moved = 0;
    }
}

-(void)rightMouseDown:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    _tab_clicked = [self tabUnderCursor:coord.x];
    if (event.type == NSRightMouseDown) {
        // FIXME: right click menu
    }
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
                deadbeef->plt_remove (_tab_clicked);
                // force invalidation of playlist cache
// FIXME: gtkui calls search_refresh here
                int playlist = deadbeef->plt_get_curr_idx ();
                deadbeef->conf_set_int ("playlist.current", playlist);
            }
        }
    }
}

-(void)mouseUp:(NSEvent *)event
{
    if (event.type == NSLeftMouseUp) {
        // FIXME: cancel repeating
#if 0
        if (scroll_timer > 0) {
            scroll_direction = 0;
            g_source_remove (scroll_timer);
            scroll_timer = 0;
        }
#endif
        if (_prepare || _dragging >= 0) {
            _dragging = -1;
            _prepare = 0;
            [self setNeedsDisplay:YES];
        }
    }
}

- (void)viewDidEndLiveResize {
    [self adjustHScroll];
}

- (void)mouseDragged:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    if (([NSEvent pressedMouseButtons] & 1) && _prepare) {
        if (abs (coord.x - _prev_x) > 3) {
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

-(void)mouseMoved:(NSEvent *)event {
    NSPoint coord = [self convertPoint:[event locationInWindow] fromView:nil];
    int tab = [self tabUnderCursor:coord.x];
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
}

- (BOOL)isFlipped {
    return YES;
}
// FIXME dnd motion must activate playlist
// ...

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
        case DB_EV_PLAYLISTCHANGED:
        case DB_EV_PLAYLISTSWITCHED:
            [self setNeedsDisplay:YES];
            break;
    }
    return 0;
}

@end
