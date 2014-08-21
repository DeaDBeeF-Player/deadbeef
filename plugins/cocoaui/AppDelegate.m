/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2014 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "AppDelegate.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation AppDelegate

@synthesize playlist;

DB_playItem_t *prev = NULL;
int prevIdx = -1;
NSImage *playImg;
NSImage *pauseImg;
NSImage *bufferingImg;
AppDelegate *g_appDelegate;
NSInteger firstSelected = -1;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    playImg = [NSImage imageNamed:@"btnplayTemplate.pdf"];
    pauseImg = [NSImage imageNamed:@"btnpauseTemplate.pdf"];
    bufferingImg = [NSImage imageNamed:@"bufferingTemplate.pdf"];

    [playlist setDelegate:(id<NSTableViewDelegate>)self];
    [playlist setDataSource:(id<NSTableViewDataSource>)self];
    [playlist setDoubleAction:@selector(playlistDoubleAction)];
    g_appDelegate = self;
}

- (void)playlistDoubleAction
{
    int row = [playlist clickedRow];
    deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, row, 0);
}


- (IBAction)tbClicked:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];
    
    switch (selectedSegment) {
        case 0:
            deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
            break;
        case 1:
            deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
            break;
        case 2:
            deadbeef->sendmessage(DB_EV_TOGGLE_PAUSE, 0, 0, 0);
            break;
        case 3:
            deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
            break;
        case 4:
            deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
            break;
    }
}


// playlist delegate
- (NSIndexSet *)tableView:(NSTableView *)tableView selectionIndexesForProposedSelection:(NSIndexSet *)proposedSelectionIndexes
{
    firstSelected = -1;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        //deadbeef->plt_deselect_all (plt);
        int __block i = 0;
        DB_playItem_t __block *it = deadbeef->plt_get_first(plt, PL_MAIN);
        [proposedSelectionIndexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
            if (firstSelected == -1) {
                firstSelected = idx;
            }
            while (i < idx && it) {
                deadbeef->pl_set_selected (it, 0);
                DB_playItem_t *prev = it;
                it = deadbeef->pl_get_next(prev, PL_MAIN);
                deadbeef->pl_item_unref(prev);
                i++;
            }
            if (!it) {
                return;
            }
            deadbeef->pl_set_selected (it, 1);
            DB_playItem_t *prev = it;
            it = deadbeef->pl_get_next(prev, PL_MAIN);
            deadbeef->pl_item_unref(prev);
            i++;
        }];
        while (it) {
            deadbeef->pl_set_selected (it, 0);
            DB_playItem_t *prev = it;
            it = deadbeef->pl_get_next(prev, PL_MAIN);
            deadbeef->pl_item_unref(prev);
            i++;
        }
        deadbeef->plt_unref (plt);
    }
    return proposedSelectionIndexes;
}

// playlist datasource
- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    int cnt = deadbeef->pl_getcount(PL_MAIN);
    return cnt;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
    DB_playItem_t *it = NULL;
    
    if (prevIdx != -1) {
        if (prevIdx == rowIndex) {
            it = prev;
            deadbeef->pl_item_ref (it);
        }
        else if (prevIdx == rowIndex - 1) {
            it = deadbeef->pl_get_next (prev, PL_MAIN);
        }
        else if (prevIdx == rowIndex + 1) {
            it = deadbeef->pl_get_prev (prev, PL_MAIN);
        }
    
    }
    if (!it) {
        it = deadbeef->pl_get_for_idx (rowIndex);
    }
    
    if (prev) {
        deadbeef->pl_item_unref (prev);
    }
    prev = it;
    prevIdx = rowIndex;

    if ([[aTableColumn identifier] isEqualToString:@"playing"]) {
        DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();
        NSImage *img = NULL;
        if (it == playing_track) {
            int paused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED;
            int buffering = !deadbeef->streamer_ok_to_read (-1);
            if (paused) {
                img = pauseImg;
            }
            else if (!buffering) {
                img = playImg;
            }
            else {
                img = bufferingImg;
            }
        }
        if (playing_track) {
            deadbeef->pl_item_unref (playing_track);
        }
        return img;
    }
    else if ([[aTableColumn identifier] isEqualToString:@"albumartist"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%a - %b");
        return [NSString stringWithUTF8String:buf];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"trknum"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%n");
        return [NSString stringWithUTF8String:buf];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"title"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%t");
        return [NSString stringWithUTF8String:buf];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"duration"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%l");
        return [NSString stringWithUTF8String:buf];
    }
    
    return @"Hello";
}

- (IBAction)openFilesAction:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setCanChooseDirectories:NO];
    if ( [openDlg runModalForDirectory:nil file:nil] == NSOKButton )
    {
        NSArray* files = [openDlg filenames];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        deadbeef->plt_clear(plt);
        if (plt) {
            if (!deadbeef->plt_add_files_begin (plt, 0)) {
                for( int i = 0; i < [files count]; i++ )
                {
                    NSString* fileName = [files objectAtIndex:i];
                    deadbeef->plt_add_file2 (0, plt, [fileName UTF8String], NULL, NULL);
                }
                deadbeef->plt_add_files_end (plt, 0);
                deadbeef->plt_unref (plt);
            }
        }
    }
    [playlist reloadData];
    deadbeef->pl_save_current();
    deadbeef->conf_save();
}

- (IBAction)addFilesAction:(id)sender {
}

- (IBAction)addFoldersAction:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:NO];
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setCanChooseDirectories:YES];
    if ( [openDlg runModalForDirectory:nil file:nil] == NSOKButton )
    {
        NSArray* files = [openDlg filenames];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            if (!deadbeef->plt_add_files_begin (plt, 0)) {
                for( int i = 0; i < [files count]; i++ )
                {
                    NSString* fileName = [files objectAtIndex:i];
                    deadbeef->plt_add_dir2 (0, plt, [fileName UTF8String], NULL, NULL);
                }
                deadbeef->plt_add_files_end (plt, 0);
                deadbeef->plt_unref (plt);
            }
        }
    }
    [playlist reloadData];
    deadbeef->pl_save_current();
}

- (IBAction)clearAction:(id)sender {
    deadbeef->pl_clear();
    deadbeef->pl_save_current();
    [playlist reloadData];
}

- (IBAction)removeSelectionAction:(id)sender {
    deadbeef->pl_delete_selected ();
    deadbeef->pl_save_current();
    [playlist reloadData];
    [playlist deselectAll:self];
    if (firstSelected != -1) {
        [playlist selectRowIndexes:[[NSIndexSet alloc] initWithIndex:firstSelected] byExtendingSelection:NO];
    }
    [self tableView:playlist selectionIndexesForProposedSelection: firstSelected==-1?[NSIndexSet alloc]:[[NSIndexSet alloc] initWithIndex:firstSelected]];
}

- (void)reloadPlaylistData {
    [playlist reloadData];
}

+ (int)ddb_message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2
{

    if (_id == DB_EV_TOGGLE_PAUSE || _id == DB_EV_PLAYLIST_REFRESH || _id == DB_EV_PLAYLISTCHANGED || _id == DB_EV_PLAYLISTSWITCHED || _id == DB_EV_TRACKINFOCHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(reloadPlaylistData) withObject:nil waitUntilDone:NO];
    }
    return 0;
}

@end
