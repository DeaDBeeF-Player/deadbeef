//
//  TrackContextMenu.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/27/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#include <sys/stat.h>
#include "deadbeef.h"
#include "deletefromdisk.h"
#include "rg_scanner.h"
#include "artwork.h"
#import "ConverterWindowController.h"
#import "NSMenu+ActionItems.h"
#import "PluginActionMenuItem.h"
#import "ReplayGainScannerController.h"
#import "TrackContextMenu.h"

extern DB_functions_t *deadbeef;

@interface TrackContextMenu()<NSMenuDelegate>

@property (nonatomic,weak) NSView *view; // the view to associate with this menu

@property (nonatomic) ddb_playlist_t *playlist;

@property (nonatomic) NSMenuItem *reloadMetadataItem;
@property (nonatomic) NSMenuItem *rgMenuItem;
@property (nonatomic) NSMenuItem *addToFrontOfQueueItem;
@property (nonatomic) NSMenuItem *addToQueueItem;
@property (nonatomic) NSMenuItem *removeFromQueueItem;
@property (nonatomic) NSMenuItem *convertItem;

@property (nonatomic) NSMenuItem *rgScanPerFileItem;
@property (nonatomic) NSMenuItem *rgScanAsSingleAlbumItem;
@property (nonatomic) NSMenuItem *rgScanAsAlbumsItem;
@property (nonatomic) NSMenuItem *rgRemoveInformationItem;

@property (nonatomic) NSMenuItem *showInFinderItem;
@property (nonatomic) NSMenuItem *deleteFromDiskItem;

@property (nonatomic) NSMenuItem *pluginActionsSeparatorItem;

@property (nonatomic) NSMenuItem *trackPropertiesItem;

@property (nonatomic) ddbUtilTrackList_t selectedTracksList;
@property (nonatomic) ddbDeleteFromDiskController_t deleteFromDiskController;

@end

@implementation TrackContextMenu

- (instancetype)initWithTitle:(NSString *)title {
    self = [self initWithView:[NSView new]];
    return nil;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [self initWithView:[NSView new]];
    return nil;
}

- (instancetype)initWithView:(NSView *)view {
    self = [super initWithTitle:@""];
    self.view = view;
    return self;
}

- (void)dealloc {
    if (_deleteFromDiskController) {
        ddbDeleteFromDiskControllerFree(_deleteFromDiskController);
        _deleteFromDiskController = NULL;
    }

    if (_selectedTracksList) {
        ddbUtilTrackListFree(_selectedTracksList);
        _selectedTracksList = NULL;
    }

    if (_playlist) {
        deadbeef->plt_unref (_playlist);
        _playlist = NULL;
    }
}

- (void)updateMenuItems {
    self.deleteFromDiskItem.hidden = deadbeef->conf_get_int("cocoaui.hide_remove_from_disk", 0);
}

- (void)setSelectedTracksList:(ddbUtilTrackList_t)selectedTracksList {
    if (_selectedTracksList != NULL) {
        ddbUtilTrackListFree(_selectedTracksList);
    }
    _selectedTracksList = selectedTracksList;
}

- (void)update:(ddb_playlist_t *)playlist actionContext:(ddb_action_context_t)actionContext {
    [self removeAllItems];

    if (actionContext == DDB_ACTION_CTX_PLAYLIST && playlist == NULL) {
        return;
    }

    if (actionContext == DDB_ACTION_CTX_PLAYLIST) {
        ddbUtilTrackList_t *trackList = ddbUtilTrackListAlloc();
        ddbUtilTrackListInitWithPlaylist(trackList, playlist, actionContext);
        self.selectedTracksList = trackList;
    }

    self.reloadMetadataItem = [self insertItemWithTitle:@"Reload Metadata" action:@selector(reloadMetadata) keyEquivalent:@"" atIndex:0];
    self.reloadMetadataItem.target = self;

    NSMenu *rgMenu = [[NSMenu alloc] initWithTitle:@"ReplayGain"];
    rgMenu.autoenablesItems = NO;

    self.rgScanPerFileItem = [rgMenu addItemWithTitle:@"Scan Per-file Track Gain" action:@selector(rgScanTracks:) keyEquivalent:@""];
    self.rgScanPerFileItem.target = self;
    self.rgScanAsSingleAlbumItem = [rgMenu addItemWithTitle:@"Scan Selection as Single Album" action:@selector(rgScanAlbum:) keyEquivalent:@""];
    self.rgScanAsSingleAlbumItem.target = self;
    self.rgScanAsAlbumsItem = [rgMenu addItemWithTitle:@"Scan Selection as Albums (By Tags)" action:@selector(rgScanAlbumsAuto:) keyEquivalent:@""];
    self.rgScanAsAlbumsItem.target = self;
    self.rgRemoveInformationItem = [rgMenu addItemWithTitle:@"Remove ReplayGain Information" action:@selector(rgRemove:) keyEquivalent:@""];
    self.rgScanAsAlbumsItem.target = self;

    self.rgMenuItem = [[NSMenuItem alloc] initWithTitle:@"ReplayGain" action:nil keyEquivalent:@""];
    self.rgMenuItem.submenu = rgMenu;
    [self addItem:self.rgMenuItem];

    self.addToFrontOfQueueItem = [self addItemWithTitle:@"Play Next" action:@selector(addToFrontOfPlaybackQueue) keyEquivalent:@""];
    self.addToFrontOfQueueItem.target = self;

    self.addToQueueItem = [self addItemWithTitle:@"Play Later" action:@selector(addToPlaybackQueue) keyEquivalent:@""];
    self.addToQueueItem.target = self;

    self.removeFromQueueItem = [self addItemWithTitle:@"Remove from Playback Queue" action:@selector(removeFromPlaybackQueue) keyEquivalent:@""];
    self.removeFromQueueItem.target = self;

    [self addItem:NSMenuItem.separatorItem];

    self.convertItem = [self addItemWithTitle:@"Convert" action:@selector(convertSelection) keyEquivalent:@""];
    self.convertItem.target = self;

    self.showInFinderItem = [self addItemWithTitle:@"Show in Finder" action:@selector(showInFinder) keyEquivalent:@""];
    self.showInFinderItem.target = self;
    self.deleteFromDiskItem = [self addItemWithTitle:@"Delete from Disk" action:@selector(deleteFromDisk) keyEquivalent:@""];
    self.deleteFromDiskItem.target = self;

    [self addItem:NSMenuItem.separatorItem];

    int selected_count = 0;
    ddb_playItem_t **tracks = NULL;

    if (self.selectedTracksList != NULL) {
        selected_count = ddbUtilTrackListGetTrackCount (self.selectedTracksList);
        tracks = ddbUtilTrackListGetTracks (self.selectedTracksList);
    }

    DB_playItem_t *selected = NULL;

    if (selected_count > 0) {
        selected = tracks[0];
    }

    self.showInFinderItem.enabled = selected_count != 0;
    self.deleteFromDiskItem.enabled = selected_count != 0;

    if ([self addPluginActionItemsForSelectedTrack:selected selectedCount:selected_count actionContext:actionContext]) {
        [self addItem:NSMenuItem.separatorItem];
    }

    self.trackPropertiesItem = [self addItemWithTitle:@"Track Properties" action:@selector(trackProperties) keyEquivalent:@""];
    self.trackPropertiesItem.target = self;

    self.autoenablesItems = NO;

    [self updateMenuItems];
    if (self.playlist != NULL){
        deadbeef->plt_unref (self.playlist);
    }
    self.playlist = playlist;
    if (playlist) {
        deadbeef->plt_ref (playlist);
    }
    BOOL enabled = selected_count != 0;
    self.reloadMetadataItem.enabled = enabled;
    self.reloadMetadataItem.target = self;
    BOOL has_rg_info = NO;
    BOOL can_be_rg_scanned = NO;
    if (enabled) {
        [self menuRGState:&can_be_rg_scanned hasRGInfo:&has_rg_info];
    }
    self.rgMenuItem.enabled = enabled;

    self.rgScanPerFileItem.enabled = can_be_rg_scanned;
    self.rgScanAsSingleAlbumItem.enabled = can_be_rg_scanned;
    self.rgScanAsAlbumsItem.enabled = can_be_rg_scanned;
    self.rgRemoveInformationItem.enabled = has_rg_info;

    self.addToFrontOfQueueItem.enabled = enabled;
    self.addToQueueItem.enabled = enabled;
    self.removeFromQueueItem.enabled = enabled;
    self.convertItem.enabled = enabled;
    self.trackPropertiesItem.enabled = enabled;
    self.trackPropertiesItem.target = self;
}

- (void)updateWithTrackList:(ddb_playItem_t **)tracks count:(NSUInteger)count playlist:(ddb_playlist_t *)plt currentTrack:(ddb_playItem_t *)currentTrack currentTrackIdx:(int)currentTrackIdx {
    [self updateMenuItems];
    self.selectedTracksList = NULL;
    self.deleteFromDiskController = NULL;
    if (tracks != NULL) {
        self.selectedTracksList = ddbUtilTrackListInitWithWithTracks( ddbUtilTrackListAlloc(), plt, DDB_ACTION_CTX_SELECTION, tracks, (unsigned)count, currentTrack, currentTrackIdx);
    }
}

- (void)menuRGState:(BOOL *)canBeRGScanned hasRGInfo:(BOOL *)hasRGInfo {
    BOOL __block has_rg_info = NO;
    BOOL __block can_be_rg_scanned = NO;
    ddb_replaygain_settings_t __block s;
    s._size = sizeof (ddb_replaygain_settings_t);

    [self forEachTrack:^(DB_playItem_t *it){
        if (deadbeef->pl_is_selected (it)) {
            if (deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI"))) {
                if (deadbeef->pl_get_item_duration (it) > 0) {
                    can_be_rg_scanned = YES;
                }
                deadbeef->replaygain_init_settings (&s, it);
                if (s.has_album_gain || s.has_track_gain) {
                    has_rg_info = YES;
                    return NO;
                }
            }
        }
        return YES;
    }];

    *canBeRGScanned = can_be_rg_scanned;
    *hasRGInfo = has_rg_info;
}

- (void)reloadMetadata {
    [self forEachTrack:^BOOL(DB_playItem_t *it) {
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;

        if (match) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            if (!(f & DDB_IS_SUBTRACK)) {
                f &= ~DDB_TAG_MASK;
                deadbeef->pl_set_item_flags (it, f);
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int i = 0; decoders[i]; i++) {
                    if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                        if (decoders[i]->read_metadata) {
                            decoders[i]->read_metadata (it);
                        }
                        break;
                    }
                }
            }
        }
        return YES;
    }];

    [((id<TrackContextMenuDelegate>)self.delegate) trackContextMenuDidReloadMetadata:self];
}

#pragma mark -

- (void)rgRemove:(id)sender {
    int count;
    DB_playItem_t **tracks = [self getSelectedTracksForRg:&count withRgTags:YES];
    if (!tracks) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    [ReplayGainScannerController removeRgTagsFromTracks:tracks count:count];
}

- (void)rgScanAlbum:(id)sender {
    [self rgScan:DDB_RG_SCAN_MODE_SINGLE_ALBUM];
}

- (void)rgScanAlbumsAuto:(id)sender {
    [self rgScan:DDB_RG_SCAN_MODE_ALBUMS_FROM_TAGS];
}

- (void)rgScanTracks:(id)sender {
    [self rgScan:DDB_RG_SCAN_MODE_TRACK];
}

- (void)forEachTrack:(BOOL (^)(DB_playItem_t *it))block {
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks (self.selectedTracksList);
    int count = ddbUtilTrackListGetTrackCount(self.selectedTracksList);

    for (int i = 0; i < count; i++) {
        BOOL res = block (tracks[i]);
        if (!res) {
            return;
        }
    }
}

- (DB_playItem_t **)getSelectedTracksForRg:(int *)pcount withRgTags:(BOOL)withRgTags {
    int numtracks = ddbUtilTrackListGetTrackCount(self.selectedTracksList);
    if (!numtracks) {
        return NULL;
    }

    ddb_replaygain_settings_t __block s;
    s._size = sizeof (ddb_replaygain_settings_t);

    ddb_playItem_t __block **tracks = calloc (numtracks, sizeof (ddb_playItem_t *));
    int __block n = 0;
    [self forEachTrack:^(DB_playItem_t *it) {
        assert (n < numtracks);
        BOOL hasRgTags = NO;
        if (withRgTags) {
            deadbeef->replaygain_init_settings (&s, it);
            if (s.has_album_gain || s.has_track_gain) {
                hasRgTags = YES;
            }
        }
        if (!withRgTags || hasRgTags) {
            deadbeef->pl_item_ref (it);
            tracks[n++] = it;
        }
        return YES;
    }];

    if (!n) {
        free (tracks);
        return NULL;
    }
    *pcount = n;
    return tracks;
}

- (void)rgScan:(int)mode {
    int count;
    DB_playItem_t **tracks = [self getSelectedTracksForRg:&count withRgTags:NO];
    if (!tracks) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    [ReplayGainScannerController runScanner:mode forTracks:tracks count:count];
}

#pragma mark -

- (void)convertSelection {
    ddb_playItem_t **tracks = ddbUtilTrackListGetTracks(self.selectedTracksList);
    unsigned count = ddbUtilTrackListGetTrackCount(self.selectedTracksList);
    [ConverterWindowController runConverterWithTracks:tracks count:count playlist:self.playlist];
}

#pragma mark - Delete from Disk

static void _warningMessageForCtx (ddbDeleteFromDiskController_t ctl, ddb_action_context_t ctx, unsigned trackcount, ddbDeleteFromDiskControllerWarningCallback_t callback) {
    void *userData = ddbDeleteFromDiskControllerGetUserData(ctl);
    TrackContextMenu *menu = (__bridge TrackContextMenu *)userData;
    [menu deleteFromDiskWarningMessage:ctl ctx:ctx trackCount:trackcount callback:callback];
}

static int
_deleteFile (ddbDeleteFromDiskController_t ctl, const char *uri) {
    NSString *str = [NSString stringWithUTF8String:uri];

    NSURL *url;

    if (uri[0] == '/') {
        url = [NSURL fileURLWithPath:str];
    }
    else if (!strncmp(uri, "file://", 7)) {
        url = [NSURL URLWithString:str];
    }

    if (!url) {
        return -1;
    }
    if (deadbeef->conf_get_int ("cocoaui.delete_use_bin", 1)) {
        [NSWorkspace.sharedWorkspace recycleURLs:@[url] completionHandler:nil];
    } else {
        (void)unlink (uri);

        // check if file still exists
        struct stat buf;
        memset (&buf, 0, sizeof (buf));
        int stat_res = stat (uri, &buf);

        if (stat_res == 0) {
            //            trace ("Failed to delete file: %s\n", uri);
            return 0;
        }
    }

    return 1;
}

static void
_deleteCompleted (ddbDeleteFromDiskController_t ctl, int cancelled) {
    void *userData = ddbDeleteFromDiskControllerGetUserData(ctl);
    TrackContextMenu *menu = (TrackContextMenu *)CFBridgingRelease(userData);

    [menu deleteCompleted:ctl cancelled:cancelled ? YES : NO];
}

- (void)deleteFromDiskWarningMessage:(ddbDeleteFromDiskController_t)ctl ctx:(ddb_action_context_t)ctx trackCount:(unsigned)trackcount callback:(ddbDeleteFromDiskControllerWarningCallback_t)callback {
    if (self.view == nil) {
        return;
    }
    if (deadbeef->conf_get_int ("cocoaui.delete_files_confirm", 1)) {
        NSString *buf;
        NSString *buf2 = deadbeef->conf_get_int ("cocoaui.delete_use_bin", 1) ?
        @" The files will be moved to Recycle Bin.\n\n(This dialog can be turned off in the Preferences)" :
        @" The files will be deleted permanently.\n\n(This dialog can be turned off in the Preferences)";

        if (ctx == DDB_ACTION_CTX_SELECTION) {
            int selected_files = trackcount;
            if (selected_files == 1) {
                buf = [NSString stringWithFormat:@"Do you really want to delete the selected file?%@", buf2];
            } else {
                buf = [NSString stringWithFormat:@"Do you really want to delete all %d selected files?%@", selected_files, buf2];
            }
        }
        else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
            int files = trackcount;
            buf = [NSString stringWithFormat:@"Do you really want to delete all %d files from the current playlist?%@", files, buf2];
        }
        else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
            buf = [NSString stringWithFormat:@"Do you really want to delete the currently playing file?%@", buf2];
        }

        NSAlert *alert = [NSAlert new];
        alert.alertStyle = NSAlertStyleWarning;
        alert.messageText = @"Delete files from disk";
        alert.informativeText = buf ?: @"";
        [alert addButtonWithTitle:@"Cancel"];
        [alert addButtonWithTitle:@"Delete"];
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
        if (@available(macOS 10.16, *)) {
            alert.buttons[1].hasDestructiveAction = YES;
        }
#endif

        [alert beginSheetModalForWindow:self.view.window completionHandler:^(NSModalResponse returnCode) {
            BOOL shouldCancel = returnCode != NSAlertSecondButtonReturn;
            // Defer the execution to next tick,
            // to ensure that the alert is dismissed before proceeding.
            dispatch_async(dispatch_get_main_queue(), ^{
                callback(ctl, shouldCancel);
            });
        }];
    }
    else {
        callback(ctl, 0);
    }
}

- (void)deleteCompleted:(ddbDeleteFromDiskController_t)ctl cancelled:(BOOL)cancelled {
    ddbDeleteFromDiskControllerFree(ctl);
    _deleteFromDiskController = NULL;
    [((id<TrackContextMenuDelegate>)self.delegate) trackContextMenuDidDeleteFiles:self cancelled:cancelled];
}

- (void)setDeleteFromDiskController:(ddbDeleteFromDiskController_t)deleteFromDiskController {
    if (_deleteFromDiskController) {
        ddbDeleteFromDiskControllerFree(_deleteFromDiskController);
    }
    _deleteFromDiskController = deleteFromDiskController;
}

- (void)deleteFromDisk {
    if (self.deleteFromDiskController) {
        return;
    }

    ddbDeleteFromDiskControllerDelegate_t delegate = {
        .warningMessageForCtx = _warningMessageForCtx,
        .deleteFile = _deleteFile,
        .completed = _deleteCompleted,
    };

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!plt) {
        return;
    }

    ddb_action_context_t ctx = DDB_ACTION_CTX_SELECTION;

    if (self.selectedTracksList) {
        self.deleteFromDiskController = ddbDeleteFromDiskControllerInitWithTrackList(ddbDeleteFromDiskControllerAlloc(), self.selectedTracksList);
    }
    else {
        self.deleteFromDiskController = ddbDeleteFromDiskControllerInitWithPlaylist(ddbDeleteFromDiskControllerAlloc(), plt, ctx);
    }

    ddbDeleteFromDiskControllerSetShouldSkipDeletedTracks(self.deleteFromDiskController, deadbeef->conf_get_int ("cocoaui.skip_deleted_tracks", 0));
    ddbDeleteFromDiskControllerSetUserData(self.deleteFromDiskController, (void *)CFBridgingRetain(self));

    ddbDeleteFromDiskControllerRunWithDelegate(self.deleteFromDiskController, delegate);

    deadbeef->plt_unref (plt);
}

- (void)showInFinder {
    NSMutableArray *urls = [NSMutableArray arrayWithCapacity:ddbUtilTrackListGetTrackCount(self.selectedTracksList)];
    [self forEachTrack:^(DB_playItem_t *it) {
        if (deadbeef->pl_is_selected (it)) {
            const char *uri = deadbeef->pl_find_meta (it, ":URI");
            NSString *str = [NSString stringWithUTF8String:uri];
            NSURL *url = [NSURL URLWithString:str];
            if (!url) {
                url = [NSURL fileURLWithPath:str];
            }
            if (url) {
                [urls addObject:url];
            }
        }
        return YES;
    }];
    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:urls];
}

#pragma mark - Playback Queue

- (void)addToFrontOfPlaybackQueue {
    __block int n = 0;
    [self forEachTrack:^BOOL(DB_playItem_t *it) {
        deadbeef->playqueue_insert_at (n++, it);
        return YES;
    }];
}

- (void)addToPlaybackQueue {
    [self forEachTrack:^BOOL(DB_playItem_t *it) {
        deadbeef->playqueue_push (it);
        return YES;
    }];
}

- (void)removeFromPlaybackQueue {
    [self forEachTrack:^BOOL(DB_playItem_t *it) {
        deadbeef->playqueue_remove (it);
        return YES;
    }];
}

- (void)trackProperties {
    [((id<TrackContextMenuDelegate>)self.delegate) trackContextMenuShowTrackProperties:self];
}

@end
