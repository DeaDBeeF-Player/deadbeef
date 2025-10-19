//
//  PlaylistContextMenu.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#import "DdbShared.h"
#import "DeletePlaylistConfirmationController.h"
#import "NSMenu+ActionItems.h"
#import "RenamePlaylistViewController.h"
#import "PlaylistContextMenu.h"

extern DB_functions_t *deadbeef;

@interface PlaylistContextMenu()

@property (nonatomic) ddb_playlist_t *playlist;
@property (nonatomic) NSPopover *renamePlaylistPopover;
@property (nonatomic) NSMenuItem *autosortPlaylistItem;

@end

@implementation PlaylistContextMenu

- (void)cleanup {
    [super cleanup];
    self.playlist = NULL;
}

- (void)setPlaylist:(ddb_playlist_t *)playlist {
    if (_playlist != NULL) {
        deadbeef->plt_unref (_playlist);
    }
    _playlist = playlist;
    if (_playlist != NULL) {
        deadbeef->plt_ref (_playlist);
    }

}

- (void)updateWithPlaylist:(ddb_playlist_t *)playlist {
    [self removeAllItems];

    self.playlist = playlist;
    [self update:playlist actionContext:DDB_ACTION_CTX_PLAYLIST];

    [self insertItemWithTitle:@"Add New Playlist" action:@selector(addNewPlaylist:) keyEquivalent:@"" atIndex:0].target = self;
    if (playlist != NULL) {
        [self insertItemWithTitle:@"Rename Playlist" action:@selector(renamePlaylistAction:) keyEquivalent:@"" atIndex:0].target = self;

        [self insertItemWithTitle:@"Delete Playlist" action:@selector(closePlaylist:) keyEquivalent:@"" atIndex:1].target = self;

        if (playlist != NULL) {
            self.autosortPlaylistItem = [[NSMenuItem alloc] initWithTitle:@"Enable Autosort" action:@selector(enableAutosortAction:) keyEquivalent:@""];
            self.autosortPlaylistItem.target = self;
            self.autosortPlaylistItem.toolTip = @"Re-apply the last sort you chose every time when adding new files to this playlist";
            [self updateEnableAutosort];
            [self insertItem:self.autosortPlaylistItem atIndex:2];
        }

        [self insertItem:NSMenuItem.separatorItem atIndex:3];

        [self addItem:NSMenuItem.separatorItem];
    }
}

- (void)updateEnableAutosort {
    if (self.playlist != NULL) {
        int enabled = deadbeef->plt_find_meta_int(self.playlist, "autosort_enabled", 0);
        self.autosortPlaylistItem.state = enabled ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

- (void)addNewPlaylist:(id)sender {
    int playlist = cocoaui_add_new_playlist ();
    if (playlist != -1) {
        deadbeef->plt_set_curr_idx (playlist);
    }
}

- (void)closePlaylist:(id)sender {
    DeletePlaylistConfirmationController *controller = [DeletePlaylistConfirmationController new];
    controller.window = self.parentView.window;

    char buffer[1000];
    deadbeef->plt_get_title (self.playlist, buffer, sizeof (buffer));
    NSString *title = @(buffer);

    controller.title = title;
    controller.delegate = self.deletePlaylistDelegate;
    [controller run];
}

- (void)renamePlaylistAction:(NSMenuItem *)sender {
    if (self.renamePlaylistPopover != nil) {
        [self.renamePlaylistPopover close];
        self.renamePlaylistPopover = nil;
    }

    self.renamePlaylistPopover = [NSPopover new];
    self.renamePlaylistPopover.behavior = NSPopoverBehaviorTransient;

    RenamePlaylistViewController *viewController = [RenamePlaylistViewController new];
    int l = deadbeef->plt_get_title (self.playlist, NULL, 0);
    char buf[l+1];
    deadbeef->plt_get_title (self.playlist, buf, (int)sizeof buf);
    viewController.name = @(buf);

    viewController.popover = self.renamePlaylistPopover;
    viewController.delegate = self.renamePlaylistDelegate;

    self.renamePlaylistPopover.contentViewController = viewController;

    NSRect rect = NSMakeRect(self.clickPoint.x, self.clickPoint.y, 1, 1);
    [self.renamePlaylistPopover showRelativeToRect:rect ofView:self.parentView preferredEdge:NSRectEdgeMaxY];
}

- (void)enableAutosortAction:(NSMenuItem *)sender {
    deadbeef->plt_set_meta_int(self.playlist, "autosort_enabled", sender.state == NSControlStateValueOff ? 1 : 0);
    [self updateEnableAutosort];
}

@end
