/*
    MPRIS plugin for DeaDBeeF Player
    Copyright (C) Peter Lamby and other contributors
    See the file COPYING for more details
*/

#ifndef MPRISSERVER_H_
#define MPRISSERVER_H_

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#define DDB_API_LEVEL 10
#define DDB_WARN_DEPRECATED 1
#include <deadbeef/deadbeef.h>
#include "../artwork/artwork.h"

#define SETTING_PREVIOUS_ACTION "mpris2.previous_action"
#define PREVIOUS_ACTION_PREVIOUS 0
#define PREVIOUS_ACTION_PREV_OR_RESTART 1

typedef struct ArtworkData_s {
    ddb_artwork_plugin_t *artwork;
    int64_t source_id;
    DB_playItem_t *track;
    char *path;
    char *default_path;
} ArtworkData_t;

struct MprisData {
    DB_functions_t *deadbeef;
    ArtworkData_t artworkData;
    DB_plugin_action_t *prevOrRestart;
    GDBusNodeInfo *gdbusNodeInfo;
    int previousAction;
};

void* startServer(void*);
void stopServer(void);

void emitVolumeChanged(float);
void emitSeeked(float);
void emitMetadataChanged(int, struct MprisData*);
void emitPlaybackStatusChanged(int, struct MprisData*);
void emitLoopStatusChanged(int);
void emitShuffleStatusChanged(int);
void emitCanGoChanged(struct MprisData *);

#endif
