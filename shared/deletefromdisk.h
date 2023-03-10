//
//  deletefromdisk.h
//  ddbcore
//
//  Created by Oleksiy Yakovenko on 24/01/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#ifndef deletefromdisk_h
#define deletefromdisk_h

#include <deadbeef/deadbeef.h>

#pragma mark - UtilTrackList

typedef void *ddbUtilTrackList_t;

ddbUtilTrackList_t ddbUtilTrackListAlloc (void);
ddbUtilTrackList_t ddbUtilTrackListInitWithPlaylist (ddbUtilTrackList_t trackList, ddb_playlist_t *plt, ddb_action_context_t ctx);
ddbUtilTrackList_t ddbUtilTrackListInitWithWithTracks (ddbUtilTrackList_t trackList, ddb_playlist_t *plt, ddb_action_context_t ctx, ddb_playItem_t **tracks, unsigned count, ddb_playItem_t *currentTrack, int currentTrackIdx);
void ddbUtilTrackListFree (ddbUtilTrackList_t trackList);

ddb_playItem_t **ddbUtilTrackListGetTracks (ddbUtilTrackList_t trackList);
unsigned ddbUtilTrackListGetTrackCount (ddbUtilTrackList_t trackList);
ddb_playlist_t *ddbUtilTrackListGetPlaylist (ddbUtilTrackList_t trackList);

#pragma mark - DeleteFromDiskController

typedef void *ddbDeleteFromDiskController_t;

typedef void (*ddbDeleteFromDiskControllerWarningCallback_t)(ddbDeleteFromDiskController_t ctl, int shouldCancel);

typedef struct {
    void (*warningMessageForCtx) (ddbDeleteFromDiskController_t ctl, ddb_action_context_t ctx, unsigned trackcount, ddbDeleteFromDiskControllerWarningCallback_t callback);
    int (*deleteFile) (ddbDeleteFromDiskController_t ctl, const char *uri);
    void (*completed) (ddbDeleteFromDiskController_t ctl, int cancelled);
} ddbDeleteFromDiskControllerDelegate_t;

ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerAlloc (void);
ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerInitWithPlaylist (ddbDeleteFromDiskController_t ctl, ddb_playlist_t *plt, ddb_action_context_t ctx);
ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerInitWithTrackList (ddbDeleteFromDiskController_t ctl, ddbUtilTrackList_t trackList);
void ddbDeleteFromDiskControllerFree (ddbDeleteFromDiskController_t ctl);

void ddbDeleteFromDiskControllerSetShouldSkipDeletedTracks (ddbDeleteFromDiskController_t ctl, int shouldSkipDeletedTracks);
void ddbDeleteFromDiskControllerSetUserData (ddbDeleteFromDiskController_t ctl, void *userData);
void *ddbDeleteFromDiskControllerGetUserData (ddbDeleteFromDiskController_t ctl);
void ddbDeleteFromDiskControllerRunWithDelegate (ddbDeleteFromDiskController_t ctl, ddbDeleteFromDiskControllerDelegate_t delegate);

#endif /* deletefromdisk_h */
