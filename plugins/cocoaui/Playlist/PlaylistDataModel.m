//
//  PlaylistDataModel.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "PlaylistDataModel.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface PlaylistDataModel()

@property (nonatomic) int playlistIter;

@end

@implementation PlaylistDataModel

- (instancetype)initWithIter:(int)iter {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _playlistIter = iter;

    return self;
}

- (void)lock {
    deadbeef->pl_lock ();
}

- (void)unlock {
    deadbeef->pl_unlock ();
}

- (int)rowCount {
    return deadbeef->pl_getcount ([self playlistIter]);
}

- (int)cursor {
    return deadbeef->pl_get_cursor([self playlistIter]);
}

- (void)setCursor:(int)cursor {
    int prev_cursor = deadbeef->pl_get_cursor([self playlistIter]);
    if (prev_cursor == cursor) {
        return;
    }
    deadbeef->pl_set_cursor ([self playlistIter], cursor);
    DB_playItem_t *it = deadbeef->pl_get_for_idx (cursor);
    if (it) {
        ddb_event_track_t *event = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_CURSOR_MOVED);
        event->track = it;
        deadbeef->event_send ((ddb_event_t *)event, PL_MAIN, 0);
    }
}

- (void)activate:(int)idx {
    DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (idx, [self playlistIter]);
    if (it) {
        int i = deadbeef->pl_get_idx_of (it);
        if (i != -1) {
            deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, i, 0);
        }
        deadbeef->pl_item_unref (it);
    }
}

- (DdbListviewRow_t)firstRow {
    return (DdbListviewRow_t)deadbeef->pl_get_first([self playlistIter]);
}

- (DdbListviewRow_t)nextRow:(DdbListviewRow_t)row {
    return (DdbListviewRow_t)deadbeef->pl_get_next((DB_playItem_t *)row, [self playlistIter]);
}

- (DdbListviewRow_t)invalidRow {
    return 0;
}

- (DdbListviewRow_t)rowForIndex:(int)idx {
    return (DdbListviewRow_t)deadbeef->pl_get_for_idx_and_iter (idx, [self playlistIter]);
}

- (void)refRow:(DdbListviewRow_t)row {
    deadbeef->pl_item_ref ((DB_playItem_t *)row);
}

- (void)unrefRow:(DdbListviewRow_t)row {
    deadbeef->pl_item_unref ((DB_playItem_t *)row);
}

- (void)selectRow:(DdbListviewRow_t)row withState:(BOOL)state {
    deadbeef->pl_set_selected ((DB_playItem_t *)row, state);
}

- (BOOL)rowSelected:(DdbListviewRow_t)row {
    return deadbeef->pl_is_selected ((DB_playItem_t *)row) ? YES : NO;
}

- (void)deselectAll {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
}

- (int)modificationIdx {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    int res = plt ? deadbeef->plt_get_modification_idx (plt) : 0;
    if (plt) {
        deadbeef->plt_unref (plt);
    }
    return res;
}

- (int)selectedCount {
    return deadbeef->pl_getselcount();
}

@end
