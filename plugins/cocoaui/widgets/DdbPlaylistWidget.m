//
//  DdbPlaylistWidgetView.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 22/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbPlaylistWidget.h"
#import "PlaylistDelegate.h"
#include "../../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbPlaylistWidget

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        NSRect listFrame = frame;
        listFrame.origin.x = 0;
        listFrame.origin.y = 0;
        _listview = [[DdbListview alloc] initWithFrame:listFrame];
        PlaylistDelegate *del = [[PlaylistDelegate alloc] init];
        [_listview setDelegate:(id<DdbListviewDelegate>)del];
        [_listview setNeedsDisplay:YES];
        [_listview setAutoresizingMask:NSViewMinXMargin|NSViewWidthSizable|NSViewMaxXMargin|NSViewMinYMargin|NSViewHeightSizable|NSViewMaxYMargin];
        [self addSubview:_listview];
        
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    
    // Drawing code here.
}

- (void)songChanged:(DB_playItem_t*)from to:(DB_playItem_t*)to {
    int to_idx = -1;
    if (to) {
        int cursor_follows_playback = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1);
        int scroll_follows_playback = deadbeef->conf_get_int ("playlist.scroll.followplayback", 1);
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != -1) {
            if (plt != deadbeef->plt_get_curr_idx ()) {
                ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
                if (p) {
                    to_idx = deadbeef->plt_get_item_idx (p, to, PL_MAIN);
                    if (cursor_follows_playback) {
                        deadbeef->plt_deselect_all (p);
                        deadbeef->pl_set_selected (to, 1);
                        deadbeef->plt_set_cursor (p, PL_MAIN, to_idx);
                    }
                    deadbeef->plt_unref (p);
                }
                return;
            }
            to_idx = deadbeef->pl_get_idx_of (to);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    [_listview setCursor:to_idx noscroll:YES];
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr_idx ()) {
                    [_listview scrollToRowWithIndex: to_idx];
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            [_listview drawRow:idx];
        }
    }
    if (to && to_idx != -1) {
        [_listview drawRow:to_idx];
    }
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
        case DB_EV_SONGCHANGED: {
            ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
            DB_playItem_t *from = ev->from;
            DB_playItem_t *to = ev->to;
            if (from)
                deadbeef->pl_item_ref (from);
            if (to)
                deadbeef->pl_item_ref (to);
            dispatch_async(dispatch_get_main_queue(), ^{
                DB_playItem_t *it;
                int idx = 0;
                deadbeef->pl_lock ();
                for (it = deadbeef->pl_get_first (PL_MAIN); it; idx++) {
                    if (deadbeef->pl_playqueue_test (it) != -1) {
                        [_listview drawRow:idx];
                    }
                    DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                    deadbeef->pl_item_unref (it);
                    it = next;
                }
                [self songChanged:from to:to];
                if (from)
                    deadbeef->pl_item_unref (from);
                if (to)
                    deadbeef->pl_item_unref (to);
                deadbeef->pl_unlock ();
            });
        }
            break;
        case DB_EV_TRACKINFOCHANGED: {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            DB_playItem_t *track = ev->track;
            if (track) {
                deadbeef->pl_item_ref (track);
                dispatch_async(dispatch_get_main_queue(), ^{
                    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                    if (plt) {
                        int idx = deadbeef->plt_get_item_idx (plt, track, PL_MAIN);
                        if (idx != -1) {
                            [_listview drawRow:deadbeef->pl_get_idx_of (track)];
                        }
                        deadbeef->plt_unref (plt);
                    }
                    deadbeef->pl_item_unref (track);
                });
            }
        }
            break;
        case DB_EV_PAUSED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
                if (curr) {
                    int idx = deadbeef->pl_get_idx_of (curr);
                    [_listview drawRow:idx];
                    deadbeef->pl_item_unref (curr);
                }
            });
        }
            break;
        case DB_EV_PLAYLISTCHANGED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                [_listview reloadData];
            });
        }
            break;
        case DB_EV_PLAYLISTSWITCHED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
                    int cursor = deadbeef->plt_get_cursor (plt, PL_MAIN);
                    int scroll = deadbeef->plt_get_scroll (plt);
                    if (cursor != -1) {
                        DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
                        if (it) {
                            deadbeef->pl_set_selected (it, 1);
                            deadbeef->pl_item_unref (it);
                        }
                    }
                    deadbeef->plt_unref (plt);

                    [_listview reloadData];
                    [_listview setVScroll:scroll];
                }
            });
        }
            break;
        case DB_EV_TRACKFOCUSCURRENT: {
            dispatch_async(dispatch_get_main_queue(), ^{
                deadbeef->pl_lock ();
                DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
                if (it) {
                    ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
                    if (plt) {
                        deadbeef->plt_set_curr (plt);
                        int idx = deadbeef->pl_get_idx_of (it);
                        if (idx != -1) {
                            [_listview setCursor:idx noscroll:YES];
                            [_listview scrollToRowWithIndex:idx];
                        }
                        deadbeef->plt_unref (plt);
                    }
                    deadbeef->pl_item_unref (it);
                }
                deadbeef->pl_unlock ();
            });
        }
            break;
        case DB_EV_CONFIGCHANGED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                [_listview setNeedsDisplay:YES];
            });
        }
            break;
        case DB_EV_SELCHANGED: {
            if (ctx != (uintptr_t)_listview || p2 == PL_SEARCH) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [_listview reloadData];
                });
            }
        }
            break;
    }
    return 0;
}

@end
