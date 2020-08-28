//
//  TrackContextMenu.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/27/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "TrackContextMenu.h"
#import "ConverterWindowController.h"
#import "ReplayGainScannerController.h"
#import "NSMenu+ActionItems.h"
#include "rg_scanner.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface TrackContextMenu()<NSMenuDelegate>

@property (nonatomic,readonly) int selectedCount;
@property (nonatomic) ddb_playlist_t *playlist;
@property (nonatomic) int playlistIter;

@property (nonatomic) NSMenuItem *reloadMetadataItem;
@property (nonatomic) NSMenuItem *rgMenuItem;
@property (nonatomic) NSMenuItem *addToQueueItem;
@property (nonatomic) NSMenuItem *removeFromQueueItem;
@property (nonatomic) NSMenuItem *convertItem;

@property (nonatomic) NSMenuItem *rgScanPerFileItem;
@property (nonatomic) NSMenuItem *rgScanAsSingleAlbumItem;
@property (nonatomic) NSMenuItem *rgScanAsAlbumsItem;
@property (nonatomic) NSMenuItem *rgRemoveInformationItem;

@property (nonatomic) NSMenuItem *trackPropertiesItem;

@end

@implementation TrackContextMenu

- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }

    self.reloadMetadataItem = [self insertItemWithTitle:@"Reload metadata" action:@selector(reloadMetadata) keyEquivalent:@"" atIndex:0];
    NSMenu *rgMenu = [[NSMenu alloc] initWithTitle:@"ReplayGain"];
    rgMenu.autoenablesItems = NO;

    self.rgScanPerFileItem = [rgMenu addItemWithTitle:@"Scan Per-file Track Gain" action:@selector(rgScanTracks:) keyEquivalent:@""];
    self.rgScanAsSingleAlbumItem = [rgMenu addItemWithTitle:@"Scan Selection As Single Album" action:@selector(rgScanAlbum:) keyEquivalent:@""];
    self.rgScanAsAlbumsItem = [rgMenu addItemWithTitle:@"Scan Selection As Albums (By Tags)" action:@selector(rgScanAlbumsAuto:) keyEquivalent:@""];
    self.rgRemoveInformationItem = [rgMenu addItemWithTitle:@"Remove ReplayGain Information" action:@selector(rgRemove:) keyEquivalent:@""];

    self.rgMenuItem = [[NSMenuItem alloc] initWithTitle:@"ReplayGain" action:nil keyEquivalent:@""];
    self.rgMenuItem.submenu = rgMenu;
    [self addItem:self.rgMenuItem];

    self.addToQueueItem = [self addItemWithTitle:@"Add To Playback Queue" action:@selector(addToPlaybackQueue) keyEquivalent:@""];
    self.addToQueueItem.target = self;

    self.removeFromQueueItem = [self addItemWithTitle:@"Remove From Playback Queue" action:@selector(removeFromPlaybackQueue) keyEquivalent:@""];
    self.removeFromQueueItem.target = self;

    [self addItem:NSMenuItem.separatorItem];

    [self addItem:NSMenuItem.separatorItem];

    self.convertItem = [self addItemWithTitle:@"Convert" action:@selector(convertSelection) keyEquivalent:@""];
    self.convertItem.target = self;

    [self addPluginActions];

    [self addItem:NSMenuItem.separatorItem];

    self.trackPropertiesItem = [self addItemWithTitle:@"Track Properties" action:@selector(trackProperties) keyEquivalent:@""];
    self.trackPropertiesItem.target = self;

    self.autoenablesItems = NO;

    return self;
}

- (void)update:(ddb_playlist_t *)playlist iter:(int)playlistIter {
    self.playlist = playlist;
    self.playlistIter = playlistIter;

    BOOL enabled = self.selectedCount != 0;
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


    self.addToQueueItem.enabled = enabled;
    self.removeFromQueueItem.enabled = enabled;
    self.convertItem.enabled = enabled;
    self.trackPropertiesItem.enabled = enabled;
    self.trackPropertiesItem.target = self;
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
    } forIter:PL_MAIN];

    *canBeRGScanned = can_be_rg_scanned;
    *hasRGInfo = has_rg_info;
}

- (int)selectedCount {
    return deadbeef->plt_getselcount(self.playlist);
}

- (void)reloadMetadata {
    DB_playItem_t *it = deadbeef->plt_get_first (self.playlist, PL_MAIN);
    while (it) {
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        deadbeef->pl_unlock ();

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
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    [((id<TrackContextMenuDelegate>)self.delegate) playlistChanged];
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

- (void)forEachTrack:(BOOL (^)(DB_playItem_t *it))block forIter:(int)iter {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->plt_get_first (self.playlist, iter);
    while (it) {
        BOOL res = block (it);
        if (!res) {
            deadbeef->pl_item_unref (it);
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, iter);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);
}

- (DB_playItem_t **)getSelectedTracksForRg:(int *)pcount withRgTags:(BOOL)withRgTags {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->pl_lock ();
    DB_playItem_t __block **tracks = NULL;
    int numtracks = deadbeef->plt_getselcount (plt);
    if (!numtracks) {
        deadbeef->pl_unlock ();
        return NULL;
    }

    ddb_replaygain_settings_t __block s;
    s._size = sizeof (ddb_replaygain_settings_t);

    tracks = calloc (numtracks, sizeof (DB_playItem_t *));
    int __block n = 0;
    [self forEachTrack:^(DB_playItem_t *it) {
        if (deadbeef->pl_is_selected (it)) {
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
        }
        return YES;
    }  forIter:PL_MAIN];
    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);

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

- (void)addPluginActions {
#if 0 // FIXME: this part of the menu needs to be rebuilt on each menu invocation
    DB_playItem_t *track = NULL;
    int selcount = self.selectedCount;

    if (selcount == 1) {
        DB_playItem_t *it = deadbeef->plt_get_first (self.playlist, PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);

            deadbeef->pl_item_unref (it);
            it = next;
        }
        track = it;
    }

    [self addActionItemsForContext:DDB_ACTION_CTX_SELECTION track:track filter:^BOOL(DB_plugin_action_t * _Nonnull action) {

        return (selcount==1 && (action->flags&DB_ACTION_SINGLE_TRACK)) || (selcount > 1 && (action->flags&DB_ACTION_MULTIPLE_TRACKS));
    }];
#endif
}

#pragma mark -

- (void)convertSelection {
    [ConverterWindowController runConverter:DDB_ACTION_CTX_SELECTION];
}

- (void)addToPlaybackQueue {
    int iter = [self playlistIter];
    DB_playItem_t *it = deadbeef->plt_get_first(self.playlist, iter);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_push (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, iter);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

- (void)removeFromPlaybackQueue {
    int iter = [self playlistIter];
    DB_playItem_t *it = deadbeef->plt_get_first(self.playlist, iter);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_remove (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, iter);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

- (void)trackProperties {
    [((id<TrackContextMenuDelegate>)self.delegate) trackProperties];
}

@end
