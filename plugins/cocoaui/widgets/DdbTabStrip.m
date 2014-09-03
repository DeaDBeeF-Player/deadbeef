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

static int text_left_padding = 4;
static int text_right_padding = 0; // calculated from widget height
static int text_vert_offset = -2;
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

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

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
    
    int y = 4;
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
    
    NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    [textStyle setAlignment:NSLeftTextAlignment];
    [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];
    NSDictionary *attrs = [NSDictionary dictionaryWithObjectsAndKeys
                           : textStyle, NSParagraphStyleAttributeName
                           , nil];

    for (idx = 0; idx < cnt; idx++) {
        w = widths[idx];
        NSRect area = NSMakeRect(x, 0, w, a.height);
        if (idx != tab_selected) {
            // ****************************
            // ***** draw tab in area *****
            // ****************************
            
            [[NSColor controlShadowColor] set];
            [NSBezierPath fillRect:area];

            NSString *tab_title = plt_get_title_wrapper (idx);
            [tab_title drawInRect:NSMakeRect(x + text_left_padding, y + text_vert_offset, w - (text_left_padding + text_right_padding - 1), a.height) withAttributes:attrs];
        }
        x += w - tab_overlap_size;
    }
    
    // *** separator here ***

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
        // **** draw selected tab here ***
        [[NSColor redColor] set];
        [NSBezierPath fillRect:NSMakeRect(x, 0, w, [self bounds].size.height)];
        
        NSString *tab_title = plt_get_title_wrapper (idx);
        [tab_title drawInRect:NSMakeRect(x + text_left_padding, y + text_vert_offset, w - (text_left_padding + text_right_padding - 1), a.height) withAttributes:attrs];

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
                    [[NSColor controlShadowColor] set];
                    [NSBezierPath fillRect:NSMakeRect(x, 0, w, a.height)];
                    
                    NSString *tab_title = plt_get_title_wrapper (tab_selected);
                    [tab_title drawInRect:NSMakeRect(x + text_left_padding, y + text_vert_offset, w - (text_left_padding + text_right_padding - 1), a.height) withAttributes:attrs];
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

@end
