//
//  DdbSearchWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSearchWidget.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbSearchWidget

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    switch (_id) {
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
                [_listview reloadData];
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
