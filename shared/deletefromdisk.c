//
//  deletefromdisk.c
//  ddbcore
//
//  Created by Oleksiy Yakovenko on 24/01/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#include <stdlib.h>
#include <string.h>
#include "deletefromdisk.h"

extern DB_functions_t *deadbeef;

#pragma mark - Track list

typedef struct {
    ddb_playlist_t *plt;
    ddb_action_context_t ctx;
    DB_playItem_t *it_current_song;
    int idx_current_song;
    DB_playItem_t **tracklist;
    unsigned trackcount;
} ddbUtilTrackListData_t;

typedef struct {
    ddbUtilTrackListData_t *trackList;
    unsigned shouldSkipDeletedTracks;
    unsigned trackListUnowned;
    void *userData;
    ddbDeleteFromDiskControllerDelegate_t delegate;
} ddbDeleteFromDiskControllerData_t;

ddbUtilTrackList_t
ddbUtilTrackListAlloc (void) {
    return calloc (1, sizeof (ddbUtilTrackListData_t));
}

ddbUtilTrackList_t
ddbUtilTrackListInitWithPlaylist (ddbUtilTrackList_t trackList, ddb_playlist_t *plt, ddb_action_context_t ctx) {
    ddbUtilTrackListData_t *data = trackList;

    data->ctx = ctx;

    deadbeef->plt_ref (plt);
    data->plt = plt;

    data->it_current_song = deadbeef->streamer_get_playing_track_safe ();
    data->idx_current_song = -1;

    if (ctx == DDB_ACTION_CTX_SELECTION) {
        unsigned selcount = deadbeef->plt_getselcount (plt);
        data->tracklist = calloc (selcount, sizeof (DB_playItem_t *));
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            if (data->trackcount == selcount) {
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->pl_is_selected (it) && deadbeef->is_local_file (uri)) {
                if (it == data->it_current_song) {
                    data->idx_current_song = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                }
                deadbeef->pl_item_ref (it);
                data->tracklist[data->trackcount++] = it;
            }
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        unsigned count = deadbeef->plt_get_item_count (plt, PL_MAIN);
        data->tracklist = calloc (count, sizeof (DB_playItem_t *));
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            if (data->trackcount == count) {
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                deadbeef->pl_item_ref (it);
                data->tracklist[data->trackcount++] = it;
            }
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
        if (it) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (deadbeef->is_local_file (uri)) {
                data->idx_current_song = deadbeef->plt_get_item_idx (plt, it, PL_MAIN);
                if (data->idx_current_song != -1) {
                    data->tracklist = calloc (1, sizeof (DB_playItem_t *));
                    deadbeef->pl_item_ref (it);
                    data->tracklist[data->trackcount++] = it;
                }
            }
            deadbeef->pl_item_unref (it);
        }
    }
    return trackList;
}

ddbUtilTrackList_t
ddbUtilTrackListInitWithWithTracks (ddbUtilTrackList_t trackList, ddb_playlist_t *plt, ddb_action_context_t ctx, ddb_playItem_t **tracks, unsigned count, ddb_playItem_t *currentTrack, int currentTrackIdx) {
    ddbUtilTrackListData_t *data = trackList;

    data->ctx = ctx;
    if (plt) {
        data->plt = plt;
        deadbeef->plt_ref (plt);
    }

    if (currentTrack) {
        deadbeef->pl_item_ref (currentTrack);
    }
    data->it_current_song = currentTrack;

    data->idx_current_song = currentTrackIdx;

    if (tracks) {
        data->tracklist = calloc (count, sizeof (ddb_playItem_t *));
        for (int i = 0; i < count; i++) {
            ddb_playItem_t *track = tracks[i];
            deadbeef->pl_item_ref (track);
            data->tracklist[i] = track;
        }
    }
    data->trackcount = count;

    return trackList;
}

void ddbUtilTrackListFree (ddbUtilTrackList_t trackList) {
    ddbUtilTrackListData_t *data = trackList;

    if (data->tracklist) {
        for (unsigned i = 0; i < data->trackcount; i++) {
            deadbeef->pl_item_unref (data->tracklist[i]);
        }
        free (data->tracklist);
    }

    if (data->it_current_song) {
        deadbeef->pl_item_unref (data->it_current_song);
    }

    if (data->plt) {
        deadbeef->plt_unref (data->plt);
    }

    free (data);
}

ddb_playItem_t **
ddbUtilTrackListGetTracks (ddbUtilTrackList_t trackList) {
    ddbUtilTrackListData_t *data = trackList;
    return data->tracklist;
}

unsigned
ddbUtilTrackListGetTrackCount (ddbUtilTrackList_t trackList) {
    ddbUtilTrackListData_t *data = trackList;
    return data->trackcount;
}

ddb_playlist_t *ddbUtilTrackListGetPlaylist (ddbUtilTrackList_t trackList) {
    ddbUtilTrackListData_t *data = trackList;
    return data->plt;
}

#pragma mark - Controller

ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerAlloc (void) {
    return calloc(1, sizeof (ddbDeleteFromDiskControllerData_t));
}

ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerInitWithPlaylist (ddbDeleteFromDiskController_t ctl, ddb_playlist_t *plt, ddb_action_context_t ctx) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    data->trackList = ddbUtilTrackListInitWithPlaylist(ddbUtilTrackListAlloc(), plt, ctx);
    return ctl;
}

ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerInitWithTrackList (ddbDeleteFromDiskController_t ctl, ddbUtilTrackList_t trackList) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    data->trackList = trackList;
    data->trackListUnowned = 1;
    return ctl;
}

static void
_remove_file_from_all_playlists (const char *search_uri) {
    // The caller is responsible for pl_lock
    int n = deadbeef->plt_get_count ();
    for (int i = 0; i < n; ++i) {
        ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
        DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
        while (it) {
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            if (strcmp (uri, search_uri) == 0) {
                deadbeef->plt_remove_item (plt, it);
            }
            deadbeef->pl_item_unref (it);
            it = next;
        }

        deadbeef->plt_unref (plt);
    }
}

static void
_delete_and_remove_track_from_all_playlists (ddbDeleteFromDiskController_t ctl, const char *uri, ddb_playlist_t *plt, ddb_playItem_t *it) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    int deleted = data->delegate.deleteFile(ctl, uri);

    if (deleted) {
        _remove_file_from_all_playlists (uri);
    }
}

void ddbDeleteFromDiskControllerSetShouldSkipDeletedTracks (ddbDeleteFromDiskController_t ctl, int shouldSkipDeletedTracks) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    data->shouldSkipDeletedTracks = shouldSkipDeletedTracks;
}

void ddbDeleteFromDiskControllerSetUserData (ddbDeleteFromDiskController_t ctl, void *userData) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    data->userData = userData;
}

void *ddbDeleteFromDiskControllerGetUserData (ddbDeleteFromDiskController_t ctl) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    return data->userData;
}

static void
_warningCallback (ddbDeleteFromDiskController_t ctl, int shouldCancel) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    ddbUtilTrackListData_t *trackListData = data->trackList;

    if (shouldCancel) {
        data->delegate.completed(ctl, shouldCancel);
        return;
    }

    DB_playItem_t **tracklist = trackListData->tracklist;
    unsigned trackcount = trackListData->trackcount;
    ddb_playlist_t *plt = trackListData->plt;

    if (tracklist) {
        for (unsigned i = 0; i < trackcount; i++) {
            const char *uri = deadbeef->pl_find_meta (tracklist[i], ":URI");
            _delete_and_remove_track_from_all_playlists (ctl, uri, plt, tracklist[i]);
        }
    }

    if (data->shouldSkipDeletedTracks
        && ((plt == NULL) || deadbeef->plt_get_item_idx (plt, trackListData->it_current_song, PL_MAIN) == -1)
        && deadbeef->streamer_get_current_playlist () == deadbeef->plt_get_curr_idx ()
        && deadbeef->get_output ()->state () == DDB_PLAYBACK_STATE_PLAYING) {

        if (trackListData->idx_current_song != -1
            && deadbeef->playqueue_get_count () == 0
            && deadbeef->streamer_get_shuffle () == DDB_SHUFFLE_OFF) {
            deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, trackListData->idx_current_song, 0);
        }
        else {
            deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
        }
    }

    data->delegate.completed(ctl, shouldCancel);
}


void ddbDeleteFromDiskControllerRunWithDelegate (ddbDeleteFromDiskController_t ctl, ddbDeleteFromDiskControllerDelegate_t delegate) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    data->delegate = delegate;
    ddbUtilTrackListData_t *trackListData = data->trackList;

    data->delegate.warningMessageForCtx (ctl, trackListData->ctx, trackListData->trackcount, _warningCallback);
}

void ddbDeleteFromDiskControllerFree (ddbDeleteFromDiskController_t ctl) {
    ddbDeleteFromDiskControllerData_t *data = ctl;
    if (data->trackList && !data->trackListUnowned) {
        ddbUtilTrackListFree(data->trackList);
    }
    free (data);
}
