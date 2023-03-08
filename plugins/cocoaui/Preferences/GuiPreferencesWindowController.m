//
//  GuiPreferencesWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/26/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "GuiPreferencesWindowController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface GuiPreferencesWindowController ()

// GUI misc properties
@property (nonatomic) BOOL enableShiftJISDetection;
@property (nonatomic) BOOL enableCP1251Detection;
@property (nonatomic) BOOL enableCP936Detection;
@property (nonatomic) NSString *titlebarPlaying;
@property (nonatomic) NSString *titlebarStopped;

// GUI Playlist properties
@property (nonatomic) BOOL mmbDeletePlaylist;
@property (nonatomic) BOOL hideRemoveFromDisk;
@property (nonatomic) BOOL namePlaylistFromFolder;

// Delete from Disk
@property (nonatomic) BOOL deleteFromDiskUseBin;
@property (nonatomic) BOOL deleteFromDiskEnableConfirmationDialog;
@property (nonatomic) BOOL deleteFromDiskSkipDeletedTracks;

@end

@implementation GuiPreferencesWindowController

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    // gui/misc -> player
    _enableShiftJISDetection =  deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0) ? YES : NO;
    _enableCP1251Detection =  deadbeef->conf_get_int ("junk.enable_cp1251_detection", 0) ? YES : NO;
    _enableCP936Detection =  deadbeef->conf_get_int ("junk.enable_cp936_detection", 0) ? YES : NO;
    _titlebarPlaying = conf_get_nsstr ("cocoaui.titlebar_playing", DEFAULT_TITLEBAR_PLAYING_VALUE);
    _titlebarStopped = conf_get_nsstr ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_STOPPED_VALUE);

    // gui/misc -> playlist
    _mmbDeletePlaylist =  deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1) ? YES : NO;
    _hideRemoveFromDisk =  deadbeef->conf_get_int ("cocoaui.hide_remove_from_disk", 0) ? YES : NO;
    _namePlaylistFromFolder =  deadbeef->conf_get_int ("cocoaui.name_playlist_from_folder", 1) ? YES : NO;

    // delete from disk
    _deleteFromDiskUseBin = deadbeef->conf_get_int ("cocoaui.delete_use_bin", 1) ? YES : NO;
    _deleteFromDiskEnableConfirmationDialog = deadbeef->conf_get_int ("cocoaui.delete_files_confirm", 1) ? YES : NO;
    _deleteFromDiskSkipDeletedTracks = deadbeef->conf_get_int ("cocoaui.skip_deleted_tracks", 0) ? YES : NO;

    return self;
}

- (void)setEnableShiftJISDetection:(BOOL)enableShiftJISDetection {
    _enableShiftJISDetection = enableShiftJISDetection;
    deadbeef->conf_set_int ("junk.enable_shift_jis_detection", enableShiftJISDetection);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setEnableCP1251Detection:(BOOL)enableCP1251Detection {
    _enableCP1251Detection = enableCP1251Detection;
    deadbeef->conf_set_int ("junk.enable_cp1251_detection", enableCP1251Detection);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setEnableCP936Detection:(BOOL)enableCP936Detection {
    _enableCP936Detection = enableCP936Detection;
    deadbeef->conf_set_int ("junk.enable_cp936_detection", enableCP936Detection);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setTitlebarPlaying:(NSString *)titlebarPlaying {
    _titlebarPlaying = titlebarPlaying;
    conf_set_nsstr("cocoaui.titlebar_playing", titlebarPlaying);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setTitlebarStopped:(NSString *)titlebarStopped {
    _titlebarStopped = titlebarStopped;
    conf_set_nsstr("cocoaui.titlebar_stopped", titlebarStopped);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setMmbDeletePlaylist:(BOOL)mmbDeletePlaylist {
    _mmbDeletePlaylist = mmbDeletePlaylist;
    deadbeef->conf_set_int ("cocoaui.mmb_delete_playlist", mmbDeletePlaylist);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setHideRemoveFromDisk:(BOOL)hideRemoveFromDisk {
    _hideRemoveFromDisk = hideRemoveFromDisk;
    deadbeef->conf_set_int ("cocoaui.hide_remove_from_disk", hideRemoveFromDisk);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setNamePlaylistFromFolder:(BOOL)namePlaylistFromFolder {
    _namePlaylistFromFolder = namePlaylistFromFolder;
    deadbeef->conf_set_int ("cocoaui.name_playlist_from_folder", namePlaylistFromFolder);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}


- (void)setDeleteFromDiskUseBin:(BOOL)deleteFromDiskUseBin {
    _deleteFromDiskUseBin = deleteFromDiskUseBin;
    deadbeef->conf_set_int ("cocoaui.delete_use_bin", deleteFromDiskUseBin);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setDeleteFromDiskEnableConfirmationDialog:(BOOL)deleteFromDiskEnableConfirmationDialog {
    _deleteFromDiskEnableConfirmationDialog = deleteFromDiskEnableConfirmationDialog;
    deadbeef->conf_set_int ("cocoaui.delete_files_confirm", deleteFromDiskEnableConfirmationDialog);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setDeleteFromDiskSkipDeletedTracks:(BOOL)deleteFromDiskSkipDeletedTracks {
    _deleteFromDiskSkipDeletedTracks = deleteFromDiskSkipDeletedTracks;
    deadbeef->conf_set_int ("cocoaui.skip_deleted_tracks", deleteFromDiskSkipDeletedTracks);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
