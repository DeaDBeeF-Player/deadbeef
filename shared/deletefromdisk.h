//
//  deletefromdisk.h
//  ddbcore
//
//  Created by Alexey Yakovenko on 24/01/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#ifndef deletefromdisk_h
#define deletefromdisk_h

#include "../deadbeef.h"

typedef void *ddbDeleteFromDiskController_t;
typedef void *ddbDeleteFromDiskTrackList_t;

typedef void (*ddbDeleteFromDiskControllerWarningCallback_t)(ddbDeleteFromDiskController_t ctl, int shouldCancel);

typedef struct {
    void (*warningMessageForCtx) (ddbDeleteFromDiskController_t ctl, ddb_action_context_t ctx, unsigned trackcount, ddbDeleteFromDiskControllerWarningCallback_t callback);
    int (*deleteFile) (ddbDeleteFromDiskController_t ctl, const char *uri);
    void (*completed) (ddbDeleteFromDiskController_t ctl);
} ddbDeleteFromDiskControllerDelegate_t;

#pragma mark - DeleteFromDiskController
ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerAlloc (void);
ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerInitWithPlaylist (ddbDeleteFromDiskController_t ctl, ddb_playlist_t *plt, ddb_action_context_t ctx);
ddbDeleteFromDiskController_t ddbDeleteFromDiskControllerInitWithTrackList (ddbDeleteFromDiskController_t ctl, ddbDeleteFromDiskTrackList_t trackList);
void ddbDeleteFromDiskControllerFree (ddbDeleteFromDiskController_t ctl);

void ddbDeleteFromDiskControllerSetShouldSkipDeletedTracks (ddbDeleteFromDiskController_t ctl, int shouldSkipDeletedTracks);
void ddbDeleteFromDiskControllerSetUserData (ddbDeleteFromDiskController_t ctl, void *userData);
void *ddbDeleteFromDiskControllerGetUserData (ddbDeleteFromDiskController_t ctl);
void ddbDeleteFromDiskControllerRunWithDelegate (ddbDeleteFromDiskController_t ctl, ddbDeleteFromDiskControllerDelegate_t delegate);

#pragma mark - DeleteFromDiskTrackList
ddbDeleteFromDiskTrackList_t ddbDeleteFromDiskTrackListAlloc (void);
ddbDeleteFromDiskTrackList_t ddbDeleteFromDiskTrackListInitWithPlaylist (ddbDeleteFromDiskTrackList_t trackList, ddb_playlist_t *plt, ddb_action_context_t ctx);
ddbDeleteFromDiskTrackList_t ddbDeleteFromDiskTrackListInitWithWithTracks (ddbDeleteFromDiskTrackList_t trackList, ddb_playlist_t *plt, ddb_action_context_t ctx, ddb_playItem_t **tracks, unsigned count);
void ddbDeleteFromDiskTrackListFree (ddbDeleteFromDiskTrackList_t trackList);

ddb_playItem_t **ddbDeleteFromDiskTrackListGetTracks (ddbDeleteFromDiskTrackList_t trackList);
unsigned ddbDeleteFromDiskTrackListGetTrackCount (ddbDeleteFromDiskTrackList_t trackList);

#endif /* deletefromdisk_h */
