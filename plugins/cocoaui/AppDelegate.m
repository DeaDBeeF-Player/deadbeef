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
#import "dispatch/dispatch.h"
#import "DdbWidgetManager.h"
#import "DdbPlaylistViewController.h"
#import "DdbShared.h"

#include "../../deadbeef.h"
#include <sys/time.h>

extern DB_functions_t *deadbeef;

extern BOOL g_CanQuit;
@implementation AppDelegate

DB_playItem_t *prev = NULL;
int prevIdx = -1;
NSImage *playImg;
NSImage *pauseImg;
NSImage *bufferingImg;
AppDelegate *g_appDelegate;
NSInteger firstSelected = -1;

- (NSWindow *)mainWindow {
    return [_mainWindow window];
}

- (void)configChanged
{
    id order_items[] = {
        _orderLinear,
        _orderShuffle,
        _orderRandom,
        _orderShuffleAlbums,
        nil
    };
    
    int order = deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    for (int i = 0; order_items[i]; i++) {
        [order_items[i] setState:i==order?NSOnState:NSOffState];
    }
    
    id loop_items[] = {
        _loopAll,
        _loopNone,
        _loopSingle,
        nil
    };
    
    int loop = deadbeef->conf_get_int ("playback.loop", PLAYBACK_MODE_LOOP_ALL);
    for (int i = 0; loop_items[i]; i++) {
        [loop_items[i] setState:i==loop?NSOnState:NSOffState];
    }
    
    [_scrollFollowsPlayback setState:deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)?NSOnState:NSOffState];
    [_cursorFollowsPlayback setState:deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)?NSOnState:NSOffState];
    
    [_stopAfterCurrent setState:deadbeef->conf_get_int ("playlist.stop_after_current", 0)?NSOnState:NSOffState];
    [_stopAfterCurrentAlbum setState:deadbeef->conf_get_int ("playlist.stop_after_current_album", 0)?NSOnState:NSOffState];
}

static int fileadd_cancelled = 0;

static void fileadd_begin (ddb_fileadd_data_t *data, void *user_data) {
    fileadd_cancelled = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp beginSheet:g_appDelegate.addFilesWindow modalForWindow:[g_appDelegate mainWindow] modalDelegate:nil didEndSelector:nil contextInfo:nil];
    });
}

static void fileadd_end (ddb_fileadd_data_t *data, void *user_data) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_appDelegate.addFilesWindow orderOut:g_appDelegate];
        [NSApp endSheet:g_appDelegate.addFilesWindow];
        [g_appDelegate.mainWindow makeKeyAndOrderFront:g_appDelegate];
    });
}

static int file_added (ddb_fileadd_data_t *data, void *user_data) {
    const char *uri = deadbeef->pl_find_meta (data->track, ":URI");
    NSString *s = [NSString stringWithUTF8String:uri];
    dispatch_sync(dispatch_get_main_queue(), ^{
        [g_appDelegate.addFilesLabel setStringValue:s];
    });
    return fileadd_cancelled ? -1 : 0;
}

- (IBAction)addFilesCancel:(id)sender {
    fileadd_cancelled = 1;
}

- (void)awakeFromNib {
    [self initMainWindow];
    [self initSearchWindow];
}

- (void)initMainWindow {
    _mainWindow = [[MainWindowController alloc] initWithWindowNibName:@"MainWindow"];
    [_mainWindow setShouldCascadeWindows:NO];
    [[_mainWindow window] setReleasedWhenClosed:NO];
    [[_mainWindow window]  setExcludedFromWindowsMenu:YES];
    [[_mainWindow window] setIsVisible:YES];

/*
    DdbPlaylistViewController *vc = [[DdbPlaylistViewController alloc] init];
    NSView *view = [vc view];

    [view setFrame:NSMakeRect(0, [_statusBar frame].size.height+2, [[_window contentView] frame].size.width, [_tabStrip frame].origin.y - [_statusBar frame].size.height - 1)];
    [view setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];

    [[_window contentView] addSubview:view];
    */
}

- (void)initSearchWindow {
    _searchWindow = [[SearchWindowController alloc] initWithWindowNibName:@"Search"];
    [_searchWindow setShouldCascadeWindows:NO];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    if (g_CanQuit) {
        return NSTerminateNow;
    }

    [_mainWindow close];
    [_searchWindow close];

    deadbeef->sendmessage(DB_EV_TERMINATE, 0, 0, 0);
    return NSTerminateLater;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    playImg = [NSImage imageNamed:@"btnplayTemplate.pdf"];
    pauseImg = [NSImage imageNamed:@"btnpauseTemplate.pdf"];
    bufferingImg = [NSImage imageNamed:@"bufferingTemplate.pdf"];

    // initialize gui from settings
    [self configChanged];
    
    deadbeef->listen_file_add_beginend (fileadd_begin, fileadd_end, NULL);
    deadbeef->listen_file_added (file_added, NULL);

    [self initColumns];

    g_appDelegate = self;
    [[NSApp dockTile] setContentView: _dockTileView];
//    [[NSApp dockTile] setBadgeLabel:@"Hello"];
    [[NSApp dockTile] display];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag{
    [[_mainWindow window] setIsVisible:YES];
    return YES;
}

- (IBAction)showMainWinAction:(id)sender {
    NSInteger st = [sender state];
    [[_mainWindow window] setIsVisible:st!=NSOnState];
    [sender setState:st==NSOnState?NSOffState:NSOnState];
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
typedef struct {
    int _id; // predefined col type
    char *format;
    char *bytecode;
    int bytecode_len;
} col_info_t;

static col_info_t columns[MAX_COLUMNS];
static int ncolumns;

void
init_column (int i, int _id, const char *format) {
    columns[i]._id = _id;
    columns[i].format = strdup (format);
    if (format) {
        char *bytecode;
        int res = deadbeef->tf_compile (format, &bytecode);
        if (res >= 0) {
            columns[i].bytecode = bytecode;
            columns[i].bytecode_len = res;
        }
    }
}

- (void)initColumns {
    init_column(ncolumns++, DB_COLUMN_PLAYING, "%playstatus%");
    init_column(ncolumns++, -1, "%artist% - %album%");
    init_column(ncolumns++, -1, "%track%");
    init_column(ncolumns++, -1, "%title%");
    init_column(ncolumns++, -1, "%length%");
}

- (void)openFiles:(BOOL)clear play:(BOOL)play {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setCanChooseDirectories:NO];
    if ( [openDlg runModal] == NSOKButton )
    {
        NSArray* files = [openDlg URLs];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (clear) {
            deadbeef->plt_clear(plt);
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        }
        if (plt) {
            if (!deadbeef->plt_add_files_begin (plt, 0)) {
                dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
                dispatch_async(aQueue, ^{
                    for( int i = 0; i < [files count]; i++ )
                    {
                        NSString* fileName = [[files objectAtIndex:i] path];
                        if (fileName) {
                            deadbeef->plt_add_file2 (0, plt, [fileName UTF8String], NULL, NULL);
                        }
                    }
                    deadbeef->plt_add_files_end (plt, 0);
                    deadbeef->plt_unref (plt);
                    deadbeef->pl_save_current();
                    if (play) {
                        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
                        deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, 0, 0);
                    }
                });
            }
        }
    }
}

- (IBAction)openFilesAction:(id)sender {
    [self openFiles:YES play:YES];
}

- (IBAction)addFilesAction:(id)sender {
    [self openFiles:NO play:NO];
}

- (IBAction)addFoldersAction:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:NO];
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setCanChooseDirectories:YES];
    if ( [openDlg runModal] == NSOKButton )
    {
        NSArray* files = [openDlg URLs];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            if (!deadbeef->plt_add_files_begin (plt, 0)) {
                dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
                dispatch_async(aQueue, ^{
                    for( int i = 0; i < [files count]; i++ )
                    {
                        NSString *fileName = [[files objectAtIndex:i] path];
                        if (fileName) {
                            deadbeef->plt_add_dir2 (0, plt, [fileName UTF8String], NULL, NULL);
                        }
                    }
                    deadbeef->plt_add_files_end (plt, 0);
                    deadbeef->plt_unref (plt);
                    deadbeef->pl_save_current();
                    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
                });
            }
        }
    }
}

- (IBAction)addLocationAction:(id)sender {
    [_addLocationTextField setStringValue:@""];
    [NSApp beginSheet:_addLocationPanel modalForWindow:[_mainWindow window] modalDelegate:nil didEndSelector:nil contextInfo:nil];
}

- (IBAction)addLocationOKAction:(id)sender {
    NSString *text = [_addLocationTextField stringValue];

    [_addLocationPanel orderOut:self];
    [NSApp endSheet:_addLocationPanel];

    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!deadbeef->plt_add_files_begin (plt, 0)) {
        dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_async(aQueue, ^{
            DB_playItem_t *tail = deadbeef->plt_get_last (plt, PL_MAIN);
            deadbeef->plt_insert_file2 (0, plt, tail, [text UTF8String], NULL, NULL, NULL);
            if (tail) {
                deadbeef->pl_item_unref (tail);
            }
            deadbeef->plt_add_files_end (plt, 0);
            if (plt) {
                deadbeef->plt_unref (plt);
            }
        });
    }
}

- (IBAction)addLocationCancelAction:(id)sender {
    [_addLocationPanel orderOut:self];
    [NSApp endSheet:_addLocationPanel];
}

- (IBAction)clearAction:(id)sender {
    deadbeef->pl_clear();
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistByTitle:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%title%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistByTrackNumber:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%tracknumber%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistByAlbum:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%album%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistByArtist:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%artist%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistByDate:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, "%year%", DDB_SORT_ASCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistRandom:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistCustom:(id)sender {
    // TODO
}

- (IBAction)delete:(id)sender {
    deadbeef->pl_delete_selected ();
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)orderLinearAction:(id)sender {
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)orderRandomAction:(id)sender {
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_RANDOM);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)orderShuffleAction:(id)sender {
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE_TRACKS);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)orderShuffleAlbumsAction:(id)sender {
    deadbeef->conf_set_int ("playback.order", PLAYBACK_ORDER_SHUFFLE_ALBUMS);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)loopAllAction:(id)sender {
    deadbeef->conf_set_int ("playback.loop", 0);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)loopNoneAction:(id)sender {
    deadbeef->conf_set_int ("playback.loop", 1);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)loopSingleAction:(id)sender {
    deadbeef->conf_set_int ("playback.loop", 2);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)centerSelectionInVisibleArea:(id)sender {
    deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
}

- (IBAction)previousAction:(id)sender {
    deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
}

- (IBAction)playAction:(id)sender {
    deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
}

- (IBAction)pauseAction:(id)sender {
    deadbeef->sendmessage(DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}

- (IBAction)stopAction:(id)sender {
    deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
}

- (IBAction)nextAction:(id)sender {
    deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
}

- (IBAction)cursorFollowsPlaybackAction:(id)sender {
}

- (IBAction)scrollFollowsPlaybackAction:(id)sender {
}

- (IBAction)stopAfterCurrentAction:(id)sender {
    int state = deadbeef->conf_get_int ("playlist.stop_after_current", 0);
    state = 1 - state;
    deadbeef->conf_set_int ("playlist.stop_after_current", state);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)stopAfterCurrentAlbumAction:(id)sender {
    int state = deadbeef->conf_get_int ("playlist.stop_after_current_album", 0);
    state = 1 - state;
    deadbeef->conf_set_int ("playlist.stop_after_current_album", state);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)selectAll:(id)sender {
    deadbeef->pl_select_all ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

- (IBAction)deselectAllAction:(id)sender {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

- (IBAction)invertSelectionAction:(id)sender {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        else {
            deadbeef->pl_set_selected (it, 1);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

- (IBAction)selectionCropAction:(id)sender {
}

+ (int)ddb_message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2
{
    [[DdbWidgetManager defaultWidgetManager] widgetMessage:_id ctx:ctx p1:p1 p2:p2];
    
    if (_id == DB_EV_CONFIGCHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(configChanged) withObject:nil waitUntilDone:NO];

    }
    return 0;
}

- (IBAction)performFindPanelAction:(id)sender {
    [[_searchWindow window] setIsVisible:YES];
    [[_searchWindow window] makeKeyWindow];
    [_searchWindow reset];
}

- (IBAction)newPlaylistAction:(id)sender {
    int playlist = cocoaui_add_new_playlist ();
    if (playlist != -1) {
        cocoaui_playlist_set_curr (playlist);
    }
}

- (IBAction)loadPlaylistAction:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setCanChooseDirectories:NO];
    if ([openDlg runModal] == NSOKButton)
    {
        NSArray* files = [openDlg URLs];
        if ([files count] < 1) {
            return;
        }
        NSString *fname = [[files firstObject] path];
        dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_async(aQueue, ^{
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                if (!deadbeef->plt_add_files_begin (plt, 0)) {
                    deadbeef->plt_clear (plt);
                    int abort = 0;
                    deadbeef->plt_load2 (0, plt, NULL, [fname UTF8String], &abort, NULL, NULL);
                    deadbeef->plt_save_config (plt);
                    deadbeef->plt_add_files_end (plt, 0);
                }
                deadbeef->plt_unref (plt);
            }
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        });
    }
}

- (IBAction)savePlaylistAction:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel];
    [panel setTitle:@"Save Playlist"];
    [panel setCanCreateDirectories:YES];
    [panel setExtensionHidden:NO];

    NSString *message = @"Supported file types: .dbpl";

    NSMutableArray *types = [[NSMutableArray alloc] init];
    [types addObject:@"dbpl"];

    DB_playlist_t **plug = deadbeef->plug_get_playlist_list ();
    for (int i = 0; plug[i]; i++) {
        if (plug[i]->extensions && plug[i]->load) {
            const char **exts = plug[i]->extensions;
            if (exts && plug[i]->save) {
                for (int e = 0; exts[e]; e++) {
                    NSString *ext = [NSString stringWithUTF8String:exts[e]];
                    [types addObject:ext];

                    message = [message stringByAppendingString:@", "];
                    message = [message stringByAppendingPathExtension:ext];
                }
            }
        }
    }

    [panel setMessage:message];
    [panel setAllowedFileTypes:types];
    [panel setAllowsOtherFileTypes:NO];

    if ([panel runModal] == NSOKButton) {
        NSString *fname = [[panel URL] path];
        if (fname) {
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                deadbeef->plt_save (plt, NULL, NULL, [fname UTF8String], NULL, NULL, NULL);
                deadbeef->plt_unref (plt);
            }
        }
    }
}
- (IBAction)openPrefWindow:(id)sender {
    if (!_prefWindow) {
        _prefWindow = [[PreferencesWindowController alloc] initWithWindowNibName:@"Preferences"];
    }
    [_prefWindow showWindow:self];
}

@end
