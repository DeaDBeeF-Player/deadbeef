//
//  PlaylistContextMenu.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#include "deadbeef.h"
#import "DdbShared.h"
#import "DeletePlaylistConfirmationController.h"
#import "NSMenu+ActionItems.h"
#import "RenamePlaylistViewController.h"
#import "PlaylistContextMenu.h"

extern DB_functions_t *deadbeef;

@interface PlaylistContextMenu()

@property (nonatomic) ddb_playlist_t *playlist;
@property (nonatomic) NSPopover *renamePlaylistPopover;

@end

@implementation PlaylistContextMenu

- (void)dealloc {
    if (self.playlist != NULL) {
        deadbeef->plt_unref (self.playlist);
        self.playlist = NULL;
    }
}

- (void)updateWithPlaylist:(ddb_playlist_t *)playlist {
    [self removeAllItems];

    if (self.playlist != NULL) {
        deadbeef->plt_unref (self.playlist);
        self.playlist = NULL;
    }
    self.playlist = playlist;
    if (playlist != NULL) {
        deadbeef->plt_ref (playlist);
    }

    [self insertItemWithTitle:@"Add New Playlist" action:@selector(addNewPlaylist:) keyEquivalent:@"" atIndex:0].target = self;
    if (playlist != NULL) {
        [self insertItemWithTitle:@"Delete Playlist" action:@selector(closePlaylist:) keyEquivalent:@"" atIndex:0].target = self;

        // ignore the warning, the message is sent to 1st responder, which will be the mainwincontroller in this case
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:@"Rename Playlist" action:@selector(renamePlaylistAction:) keyEquivalent:@""];
        item.target = self;
        [self insertItem:item atIndex:0];

        [self insertItem:NSMenuItem.separatorItem atIndex:2];

        [self addItem:NSMenuItem.separatorItem];

        [self addActionItemsForContext:DDB_ACTION_CTX_PLAYLIST track:NULL filter:^BOOL(DB_plugin_action_t * _Nonnull action) {

            if (!(action->flags & DB_ACTION_MULTIPLE_TRACKS)) {
                return NO;
            }

            if (action->flags & DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST) {
                return NO;
            }

            return YES;
        }];
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
    NSString *title = [NSString stringWithUTF8String:buffer];

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

    RenamePlaylistViewController *viewController = [[RenamePlaylistViewController alloc] initWithNibName:@"RenamePlaylistViewController" bundle:nil];
    int l = deadbeef->plt_get_title (self.playlist, NULL, 0);
    char buf[l+1];
    deadbeef->plt_get_title (self.playlist, buf, (int)sizeof buf);
    viewController.name = [NSString stringWithUTF8String:buf];

    viewController.popover = self.renamePlaylistPopover;
    viewController.delegate = self.renamePlaylistDelegate;

    self.renamePlaylistPopover.contentViewController = viewController;

    NSRect rect = NSMakeRect(self.clickPoint.x, self.clickPoint.y, 1, 1);
    [self.renamePlaylistPopover showRelativeToRect:rect ofView:self.parentView preferredEdge:NSRectEdgeMaxY];
}


@end
