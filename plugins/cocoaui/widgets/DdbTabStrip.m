//
//  DdbTabStrip.m
//  deadbeef
//
//  Created by waker on 03/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbTabStrip.h"
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbTabStrip

@synthesize hscrollpos;
@synthesize dragging;
@synthesize prepare;
@synthesize movepos;
@synthesize tab_clicked;
@synthesize scroll_direction;
@synthesize dragpt;
@synthesize prev_x;
@synthesize tab_moved;
@synthesize tabLeft;
@synthesize tabFill;
@synthesize tabRight;
@synthesize tabUnselLeft;
@synthesize tabUnselFill;
@synthesize tabUnselRight;
@synthesize tabBottomFill;

static int text_left_padding = 15;
static int text_right_padding = 0; // calculated from widget height
static int text_vert_offset = 5;
static int tab_overlap_size = 0; // widget_height/2
static int tabs_left_margin = 4;
static int min_tab_size = 80;
static int max_tab_size = 200;
#define arrow_sz 10
#define arrow_widget_width (arrow_sz+4)

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        dragging = -1;
        tab_clicked = -1;
        
        tabLeft = [NSImage imageNamed:@"tab_left"];
        [tabLeft setFlipped:YES];
        tabFill = [NSImage imageNamed:@"tab_fill"];
        [tabFill setFlipped:YES];
        tabRight = [NSImage imageNamed:@"tab_right"];
        [tabRight setFlipped:YES];
        
        tabUnselLeft = [NSImage imageNamed:@"tab_unsel_left"];
        [tabUnselLeft setFlipped:YES];
        tabUnselFill = [NSImage imageNamed:@"tab_unsel_fill"];
        [tabUnselFill setFlipped:YES];
        tabUnselRight = [NSImage imageNamed:@"tab_unsel_right"];
        [tabUnselRight setFlipped:YES];
        
        tabBottomFill = [NSImage imageNamed:@"tab_bottom_fill"];
        [tabUnselRight setFlipped:YES];
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
    int boundary = a.width - arrow_widget_width*2 + hscrollpos;
    for (int idx = 0; idx < cnt; idx++) {
        int tab_w = [self getTabWith:idx];
        if (idx == cnt-1) {
            tab_w += 3;
        }
        if (idx == tab) {
            if (w < hscrollpos) {
                hscrollpos = w;
                deadbeef->conf_set_int ("cocoaui.tabscroll", hscrollpos);
                if (redraw) {
                    [self setNeedsDisplay:YES];
                }
            }
            else if (w + tab_w >= boundary) {
                hscrollpos += (w+tab_w) - boundary;
                deadbeef->conf_set_int ("cocoaui.tabscroll", hscrollpos);
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
    hscrollpos = deadbeef->conf_get_int ("cocoaui.tabscroll", 0);
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
            if (hscrollpos > w - (a.width - arrow_widget_width*2)) {
                hscrollpos = w - (a.width - arrow_widget_width*2);
                deadbeef->conf_set_int ("cocoaui.tabscroll", hscrollpos);
            }
            [self scrollToTabInt:deadbeef->plt_get_curr_idx () redraw:NO];
        }
        else {
            hscrollpos = 0;
            deadbeef->conf_set_int ("cocoaui.tabscroll", hscrollpos);
        }
    }
}

- (void)drawTab:(int)idx area:(NSRect)area selected:(BOOL)sel {
    NSImage *tleft = sel ? tabLeft : tabUnselLeft;
    NSImage *tright = sel ? tabRight : tabUnselRight;
    NSImage *tfill = sel ? tabFill : tabUnselFill;
    
    [tleft drawAtPoint:area.origin fromRect:NSMakeRect(0,0,[tleft size].width,[tleft size].height) operation:NSCompositeSourceOver fraction:1];
    
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [[NSColor colorWithPatternImage:tfill] set];
    NSPoint convPt = [self convertPoint:NSMakePoint(0,0) toView:nil];
    [gc setPatternPhase:convPt];
    [NSBezierPath fillRect:NSMakeRect(area.origin.x + [tleft size].width, area.origin.y, area.size.width-[tleft size].width-[tright size].width, [tfill size].height)];
    [gc restoreGraphicsState];
    
    [tright drawAtPoint:NSMakePoint(area.origin.x+area.size.width-[tleft size].width, area.origin.y) fromRect:NSMakeRect(0,0,[tabRight size].width,[tright size].height) operation:NSCompositeSourceOver fraction:1];

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
    int hscroll = hscrollpos;
    
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
        NSRect area = NSMakeRect(x, 0, w, a.height);
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
            NSRect area = NSMakeRect(x, 0, w, a.height);
            [self drawTab:idx area:area selected:NO];
        }
    }
    
    NSGraphicsContext *gc = [NSGraphicsContext currentContext];
    [gc saveGraphicsState];
    [[NSColor colorWithPatternImage:tabBottomFill] set];
    int offs = [tabLeft size].height-2;
    NSPoint convPt = [self convertPoint:NSMakePoint(0,offs) toView:nil];
    [gc setPatternPhase:convPt];
    [NSBezierPath fillRect:NSMakeRect(0, offs, [self bounds].size.width, [tabBottomFill size].height)];
    [gc restoreGraphicsState];
    

    // calc position for drawin selected tab
    x = -hscroll;
    for (idx = 0; idx < tab_selected; idx++) {
        x += widths[idx] - tab_overlap_size;
    }
    x += tabs_left_margin;
    // draw selected
    if (dragging < 0 || prepare || tab_selected != dragging) {
        idx = tab_selected;
        w = widths[tab_selected];
        [self drawTab:idx area:NSMakeRect(x, 0, w, [self bounds].size.height) selected:YES];
    }
    else {
        need_draw_moving = 1;
    }
    if (need_draw_moving) {
        x = -hscroll + tabs_left_margin;
        for (idx = 0; idx < cnt; idx++) {
            w = widths[idx];
            if (idx == dragging) {
                x = movepos;
                if (x >= a.width) {
                    break;
                }
                if (w > 0) {
                    // ***** draw dragging tab here *****
                    [self drawTab:tab_selected area:NSMakeRect(x, 0, w, a.height) selected:YES];
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
    int hscroll = hscrollpos;
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

static void
_playlist_set_curr (int playlist) {
    deadbeef->plt_set_curr_idx (playlist);
    deadbeef->conf_set_int ("playlist.current", playlist);
}

-(void)scrollLeft {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab > 0) {
        tab--;
        _playlist_set_curr (tab);
    }
    [self scrollToTab:tab];
}

-(void)scrollRight {
    int tab = deadbeef->plt_get_curr_idx ();
    if (tab < deadbeef->plt_get_count ()-1) {
        tab++;
        _playlist_set_curr (tab);
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
    NSLog(@"wheel %f %f", event.deltaX, event.deltaY);
    
    if (event.deltaY < 0 || event.deltaX < 0)
    {
        [self scrollLeft];
    }
    else if (event.deltaY > 0 || event.deltaX > 0)
    {
        [self scrollRight];
    }
}

#define _(x) x

static int
_add_new_playlist (void) {
    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            strcpy (name, _("New Playlist"));
        }
        else {
            snprintf (name, sizeof (name), _("New Playlist (%d)"), idx);
        }
        deadbeef->pl_lock ();
        for (i = 0; i < cnt; i++) {
            char t[100];
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
            deadbeef->plt_get_title (plt, t, sizeof (t));
            deadbeef->plt_unref (plt);
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        deadbeef->pl_unlock ();
        if (i == cnt) {
            return deadbeef->plt_add (cnt, name);
        }
        idx++;
    }
    return -1;
}


- (void)mouseDown:(NSEvent *)event {
    tab_clicked = [self tabUnderCursor:[event locationInWindow].x];
    if (event.type == NSLeftMouseDown) {
        BOOL need_arrows = [self needArrows];
        if (need_arrows) {
            NSSize a = [self bounds].size;
            if ([event locationInWindow].x < arrow_widget_width) {
                [self scrollLeft];
                scroll_direction = -1;
                // start periodic upd
//                scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                return;
            }
            else if ([event locationInWindow].x >= a.width - arrow_widget_width) {
                [self scrollRight];
                scroll_direction = 1;
            // start periodic upd
//                    ts->scroll_timer = g_timeout_add (300, tabstrip_scroll_cb, ts);
                return;
            }
        }
    
        if (tab_clicked != -1) {
            _playlist_set_curr (tab_clicked);
        }
        else {
            if (event.clickCount == 2) {
                // new tab
                int playlist = _add_new_playlist ();
                if (playlist != -1) {
                    _playlist_set_curr (playlist);
                }
                return;
            }
            return;
        }
        
        // adjust scroll if clicked tab spans border
        if (need_arrows) {
            [self scrollToTab:tab_clicked];
        }
        
        int hscroll = hscrollpos;
        if (need_arrows) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int idx;
        for (idx = 0; idx < tab_clicked; idx++) {
            int width = [self getTabWith:idx];
            x += width - tab_overlap_size;
        }
        dragpt = [event locationInWindow];
        dragpt.x -= x;
        prepare = 1;
        dragging = tab_clicked;
        prev_x = dragpt.x;
        tab_moved = 0;
    }
}

-(void)rightMouseDown:(NSEvent *)event {
    tab_clicked = [self tabUnderCursor:[event locationInWindow].x];
    if (event.type == NSRightMouseDown) {
        // FIXME: right click menu
    }
}

-(void)otherMouseDown:(NSEvent *)event {
    tab_clicked = [self tabUnderCursor:[event locationInWindow].x];
    if (event.type == NSOtherMouseDown) {
        if (tab_clicked == -1) {
            // new tab
            int playlist = _add_new_playlist ();
            if (playlist != -1) {
                _playlist_set_curr (playlist);
            }
            return;
        }
        else if (deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1)) {
            if (tab_clicked != -1) {
                deadbeef->plt_remove (tab_clicked);
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
        if (prepare || dragging >= 0) {
            dragging = -1;
            prepare = 0;
            [self setNeedsDisplay:YES];
        }
    }
}

- (void)viewDidEndLiveResize {
    [self adjustHScroll];
}

- (void)mouseDragged:(NSEvent *)event {
    if (([NSEvent pressedMouseButtons] & 1) && prepare) {
        if (abs ([event locationInWindow].x - prev_x) > 3) {
            prepare = 0;
        }
    }
    if (!prepare && dragging >= 0) {
        movepos = [event locationInWindow].x - dragpt.x;
        
        // find closest tab to the left
        int idx;
        int hscroll = hscrollpos;
        BOOL need_arrows = [self needArrows];
        if (need_arrows) {
            hscroll -= arrow_widget_width;
        }
        int x = -hscroll + tabs_left_margin;
        int inspos = -1;
        int cnt = deadbeef->plt_get_count ();
        for (idx = 0; idx < cnt; idx++) {
            int width = [self getTabWith:idx];
            if (idx != dragging && x <= movepos && x + width/2 - tab_overlap_size  > movepos) {
                inspos = idx;
                break;
            }
            x += width - tab_overlap_size;
        }
        if (inspos >= 0 && inspos != dragging) {
            deadbeef->plt_move (dragging, inspos);
            tab_moved = 1;
            dragging = inspos;
            deadbeef->conf_set_int ("playlist.current", dragging);
        }
        [self setNeedsDisplay:YES];
    }
}

-(void)mouseMoved:(NSEvent *)event {
    int tab = [self tabUnderCursor:[event locationInWindow].x];
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

@end
