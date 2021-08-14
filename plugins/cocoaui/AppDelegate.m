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
#import "ReplayGainScannerController.h"
#import "DdbShared.h"
#import "NowPlayable.h"
#import "LogWindowController.h"
#import "HelpWindowController.h"
#import "EqualizerWindowController.h"
#import "NSMenu+ActionItems.h"
#import "WidgetDefs.h"
#include "conf.h"
#include "streamer.h"
#include "junklib.h"
#include "../../common.h"
#include "../../deadbeef.h"
#include "../../plugins/artwork/artwork.h"
#include <sys/time.h>

extern DB_functions_t *deadbeef;

extern BOOL g_CanQuit;

AppDelegate *g_appDelegate;

@interface AppDelegate () {
    char *_titleScript;
    char *_artistAlbumScript;
}


@property (nonatomic) NowPlayable *nowPlayable;
@property (nonatomic) BOOL equalizerAvailable;

@property (nonatomic) PreferencesWindowController *prefWindow;
@property (nonatomic) SearchWindowController *searchWindow;
@property (nonatomic) LogWindowController *logWindow;
@property (nonatomic) HelpWindowController *helpWindow;
@property (nonatomic) EqualizerWindowController *equalizerWindow;
@property (weak) IBOutlet NSMenuItem *equalizerMenuItem;

@property (nonatomic) NSMenuItem *dockMenuNPHeading;
@property (nonatomic) NSMenuItem *dockMenuNPTitle;
@property (nonatomic) NSMenuItem *dockMenuNPArtistAlbum;
@property (nonatomic) NSMenuItem *dockMenuNPSeparator;

@property (nonatomic) NSInteger firstSelected;

@property (nonatomic,readwrite) MediaLibraryManager *mediaLibraryManager;

@property (nonatomic) id<WidgetProtocol> rootWidget;


@end

@implementation AppDelegate

- (void)volumeChanged {
    [_mainWindow updateVolumeBar];
}

- (void)outputDeviceChanged {
    if (_prefWindow && _prefWindow.window.visible) {
        [_prefWindow outputDeviceChanged];
    }
}

- (void)configChanged {
    NSMenuItem *shuffle_items[] = {
        _orderLinear,
        _orderShuffle,
        _orderRandom,
        _orderShuffleAlbums,
        nil
    };
    
    ddb_shuffle_t shuffle = deadbeef->streamer_get_shuffle ();
    for (ddb_shuffle_t i = 0; shuffle_items[i]; i++) {
        shuffle_items[i].state = i==shuffle?NSControlStateValueOn:NSControlStateValueOff;
    }
    
    NSMenuItem *repeat_items[] = {
        _loopAll,
        _loopNone,
        _loopSingle,
        nil
    };
    
    ddb_repeat_t repeat = deadbeef->streamer_get_repeat ();
    for (ddb_repeat_t i = 0; repeat_items[i]; i++) {
        repeat_items[i].state = i==repeat?NSControlStateValueOn:NSControlStateValueOff;
    }
    
    _scrollFollowsPlayback.state = deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)?NSControlStateValueOn:NSControlStateValueOff;
    _cursorFollowsPlayback.state = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)?NSControlStateValueOn:NSControlStateValueOff;
    
    _stopAfterCurrent.state = deadbeef->conf_get_int ("playlist.stop_after_current", 0)?NSControlStateValueOn:NSControlStateValueOff;
    _stopAfterCurrentAlbum.state = deadbeef->conf_get_int ("playlist.stop_after_album", 0)?NSControlStateValueOn:NSControlStateValueOff;

    _descendingSortMode.state = deadbeef->conf_get_int ("cocoaui.sort_desc", 0) ? NSControlStateValueOn : NSControlStateValueOff;

    [self volumeChanged];

    [_mainWindow updateTitleBarConfig];
    [_mainWindow updateTitleBar];
}

static int fileadd_cancelled = 0;

static void fileadd_begin (ddb_fileadd_data_t *data, void *user_data) {
    fileadd_cancelled = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_appDelegate.mainWindow.window beginSheet:g_appDelegate.addFilesWindow completionHandler:^(NSModalResponse returnCode) {

        }];
    });
}

static void fileadd_end (ddb_fileadd_data_t *data, void *user_data) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_appDelegate.mainWindow.window endSheet:g_appDelegate.addFilesWindow returnCode:NSModalResponseOK];
        [[[g_appDelegate mainWindow] window] makeKeyAndOrderFront:g_appDelegate];
    });
}

static BOOL _settingLabel = NO;

static int file_added (ddb_fileadd_data_t *data, void *user_data) {
    const char *uri = deadbeef->pl_find_meta (data->track, ":URI");
    if (!_settingLabel) {
        // HACK: we want to set the label asynchronously, to minimize delays,
        // but we also want to avoid sending multiple labels, becuase that's meaningless.
        // So we use a basic flag, to see if the label is being set already.
        NSString *s = [NSString stringWithUTF8String:uri];
        _settingLabel = YES;
        dispatch_async(dispatch_get_main_queue(), ^{
            g_appDelegate.addFilesLabel.stringValue = s;
            _settingLabel = NO;
        });
    }
    return fileadd_cancelled ? -1 : 0;
}

- (IBAction)addFilesCancel:(id)sender {
    fileadd_cancelled = 1;
}

- (void)awakeFromNib {
    self.mediaLibraryManager = [MediaLibraryManager new];
    [self initMainMenu];
    [self initMainWindow];
    [self initSearchWindow];
    [self initLogWindow];
    [self initEqualizerWindow];

    [self bind];

}

- (void)initMainWindow {
//    _mainWindow = [[MainWindowController alloc] initWithWindowNibName:@"MainWindow"];
    _mainWindow = [[MainWindowController alloc] initWithWindowNibName:@"SplitViewMainWindow"];
    _mainWindow.shouldCascadeWindows = NO;
    _mainWindow.window.releasedWhenClosed = NO;
    _mainWindow.window.excludedFromWindowsMenu = YES;
    _mainWindow.window.isVisible = YES;
}

- (void)initSearchWindow {
    _searchWindow = [[SearchWindowController alloc] initWithWindowNibName:@"Search"];
    _searchWindow.shouldCascadeWindows = NO;
}

- (void)initLogWindow {
    _logWindow = [[LogWindowController alloc] initWithWindowNibName:@"Log"];
    _logWindow.window.excludedFromWindowsMenu = YES;
}

- (void)initMainMenu {
    // add new actions
    [self.mainMenu addActionItemsForContext:DDB_ACTION_CTX_MAIN track:nil filter:^BOOL(DB_plugin_action_t * _Nonnull action) {
        return (action->flags & (DB_ACTION_COMMON|DB_ACTION_ADD_MENU)) == (DB_ACTION_COMMON|DB_ACTION_ADD_MENU);
    }];
}

- (BOOL)equalizerAvailable {
    ddb_dsp_context_t *dsp = deadbeef->streamer_get_dsp_chain ();
    while (dsp) {
        if (!strcmp (dsp->plugin->plugin.id, "supereq")) {
            return YES;
        }
        dsp = dsp->next;
    }
    return NO;
}

- (void)initEqualizerWindow {
    if (!_equalizerWindow) {
        _equalizerWindow = [[EqualizerWindowController alloc] initWithWindowNibName:@"EqualizerWindowController"];
        _equalizerWindow.window.excludedFromWindowsMenu = YES;
    }
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    if (g_CanQuit) {
        return NSTerminateNow;
    }

    if (deadbeef->have_background_jobs ()) {
        NSAlert *alert = [NSAlert new];
        alert.messageText = @"Do you really want to quit now?";
        alert.informativeText = @"DeaDBeeF is currently running background tasks. If you quit now, the tasks will be cancelled or interrupted. This may result in data loss.";
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Yes"];
        NSModalResponse res = [alert runModal];
        if (res == NSAlertFirstButtonReturn) {
            return NSTerminateCancel;
        }
    }

    deadbeef->sendmessage(DB_EV_TERMINATE, 0, 0, 0);
    return NSTerminateLater;
}

extern void
main_cleanup_and_quit (void);

- (void)applicationWillTerminate:(NSNotification *)notification {
    @autoreleasepool {
        [ConverterWindowController cleanup];
        [ReplayGainScannerController cleanup];
        [_searchWindow close];
        _searchWindow = nil;
        self.nowPlayable = nil;
        [self.logWindow close];
        self.logWindow = nil;

        [self unbind];
        [_mainWindow cleanup];
        [self.mainWindow.window close];
        self.mainWindow = nil;
    }
    self.mediaLibraryManager = nil;
    main_cleanup_and_quit();
}

- (void)bind {
    [_mainWindowToggleMenuItem bind:@"state" toObject:_mainWindow.window withKeyPath:@"visible" options:nil];
    [_logWindowToggleMenuItem bind:@"state" toObject:_logWindow.window withKeyPath:@"visible" options:nil];
    [_equalizerWindowToggleMenuItem bind:@"state" toObject:_equalizerWindow.window withKeyPath:@"visible" options:nil];
    [_equalizerMenuItem bind:@"hidden" toObject:self withKeyPath:@"equalizerAvailable" options:@{
        NSValueTransformerNameBindingOption:NSNegateBooleanTransformerName
    }];
}

- (void)unbind {
    [_mainWindowToggleMenuItem unbind:@"state"];
    [_logWindowToggleMenuItem unbind:@"state"];
    [_equalizerWindowToggleMenuItem unbind:@"state"];
    [_equalizerMenuItem unbind:@"hidden"];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // high sierra would terminate the app on SIGPIPE by default, which breaks converter error handling
    signal(SIGPIPE, SIG_IGN);

    _firstSelected = -1;

    // initialize gui from settings
    [self configChanged];
    
    deadbeef->listen_file_add_beginend (fileadd_begin, fileadd_end, NULL);
    deadbeef->listen_file_added (file_added, NULL);

    g_appDelegate = self;

#if !DISABLE_MM_KEY_GRABBER
    self.nowPlayable = [NowPlayable new];
#endif

    [self updateDockNowPlaying];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag{
    _mainWindow.window.isVisible = YES;
    return YES;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        char str[100];
        add_paths([filename UTF8String], (int)[filename length], 0, str, 100);
    });
    return YES; // assume that everything went ok
}


- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames {
    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        char str[100];
        // building single paths string for the deadbeef function, paths must be separated by '\0'
        NSString *paths =[filenames componentsJoinedByString:@"\0"];
        add_paths([paths UTF8String], (int)[paths length], 0, str, 100);
    });
}


- (IBAction)showMainWinAction:(id)sender {
    BOOL vis = ![_mainWindow.window isVisible];
    _mainWindow.window.isVisible = vis;
    if (vis) {
        [_mainWindow.window makeKeyWindow];
    }
}

- (IBAction)showLogWindowAction:(id)sender {
    BOOL vis = ![_logWindow.window isVisible];
    _logWindow.window.isVisible = vis;
    if (vis) {
        [_logWindow.window makeKeyWindow];
    }
}

- (IBAction)showEqualizerWindowAction:(id)sender {
    BOOL vis = ![_equalizerWindow.window isVisible];
    _equalizerWindow.window.isVisible = vis;
    if (vis) {
        [_equalizerWindow.window makeKeyWindow];
    }
}

// playlist delegate
- (NSIndexSet *)tableView:(NSTableView *)tableView selectionIndexesForProposedSelection:(NSIndexSet *)proposedSelectionIndexes
{
    self.firstSelected = -1;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        //deadbeef->plt_deselect_all (plt);
        NSUInteger __block i = 0;
        DB_playItem_t __block *it = deadbeef->plt_get_first(plt, PL_MAIN);
        [proposedSelectionIndexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
            if (self.firstSelected == -1) {
                self.firstSelected = idx;
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

- (void)openFiles:(BOOL)clear play:(BOOL)play {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    openDlg.canChooseFiles = YES;
    openDlg.allowsMultipleSelection = YES;
    openDlg.canChooseDirectories = NO;
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        NSArray* files = [openDlg URLs];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (!deadbeef->plt_add_files_begin (plt, 0)) {
            if (clear) {
                deadbeef->plt_clear(plt);
                deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
            }
            dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            dispatch_async(aQueue, ^{
                for (NSUInteger i = 0; i < files.count; i++) {
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
        else {
            deadbeef->plt_unref (plt);
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
    openDlg.canChooseFiles = NO;
    openDlg.allowsMultipleSelection = YES;
    openDlg.canChooseDirectories = YES;
    if ( [openDlg runModal] == NSModalResponseOK )
    {
        NSArray* files = [openDlg URLs];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (!deadbeef->plt_add_files_begin (plt, 0)) {
            dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            dispatch_async(aQueue, ^{
                for (NSUInteger i = 0; i < files.count; i++) {
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
        else {
            deadbeef->plt_unref (plt);
        }
    }
}

- (IBAction)addLocationAction:(id)sender {
    _addLocationTextField.stringValue = @"";
    [_mainWindow.window beginSheet:_addLocationPanel completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            NSString *text = [self.addLocationTextField stringValue];

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
                    deadbeef->plt_unref (plt);
                    deadbeef->pl_save_current ();
                });
            }
            else {
                deadbeef->plt_unref (plt);
            }
        }
    }];
}

- (IBAction)addLocationOKAction:(id)sender {
    [_mainWindow.window endSheet:_addLocationPanel returnCode:NSModalResponseOK];
}

- (IBAction)addLocationCancelAction:(id)sender {
    [_mainWindow.window endSheet:_addLocationPanel returnCode:NSModalResponseCancel];
}

- (IBAction)clearAction:(id)sender {
    deadbeef->pl_clear();
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (void)sortPlaylistByTF:(const char *)tf {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, tf, _descendingSortMode.state == NSControlStateValueOff ? DDB_SORT_ASCENDING : DDB_SORT_DESCENDING);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistByTitle:(id)sender {
    [self sortPlaylistByTF:"%title%"];
}

- (IBAction)sortPlaylistByTrackNumber:(id)sender {
    [self sortPlaylistByTF:"%tracknumber%"];
}

- (IBAction)sortPlaylistByAlbum:(id)sender {
    [self sortPlaylistByTF:"%album%"];
}

- (IBAction)sortPlaylistByArtist:(id)sender {
    [self sortPlaylistByTF:"%artist%"];
}

- (IBAction)sortPlaylistByDate:(id)sender {
    [self sortPlaylistByTF:"%year%"];
}

- (IBAction)sortPlaylistRandom:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, -1, NULL, DDB_SORT_RANDOM);
    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)sortPlaylistCustom:(id)sender {
    deadbeef->pl_lock ();
    _customSortEntry.stringValue = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.custom_sort_tf", "")];
    deadbeef->pl_unlock ();
    _customSortDescending.state = deadbeef->conf_get_int ("cocoaui.sort_desc", 0) ? NSControlStateValueOn : NSControlStateValueOff;
    [_mainWindow.window beginSheet:_customSortPanel completionHandler:^(NSModalResponse returnCode) {
        NSInteger state = self.customSortDescending.state;
        self.descendingSortMode.state =  state;
        deadbeef->conf_set_int ("cocoaui.sort_desc", state == NSControlStateValueOn ? 1 : 0);
        deadbeef->conf_set_str ("cocoaui.custom_sort_tf", [[self.customSortEntry stringValue] UTF8String]);

        if (returnCode == NSModalResponseOK) {
            [self sortPlaylistByTF:self.customSortEntry.stringValue.UTF8String];
        }
        deadbeef->conf_save ();
    }];

}

- (IBAction)customSortOKAction:(id)sender {
    [_mainWindow.window endSheet:_customSortPanel returnCode:NSModalResponseOK];
}

- (IBAction)customSortCancelAction:(id)sender {
    [_mainWindow.window endSheet:_customSortPanel returnCode:NSModalResponseCancel];
}

- (IBAction)toggleDescendingSortOrderAction:(id)sender {
    int st = !deadbeef->conf_get_int ("cocoaui.sort_desc", 0);
    deadbeef->conf_set_int ("cocoaui.sort_desc", st);
    _descendingSortMode.state = st ? NSControlStateValueOn : NSControlStateValueOff;
    deadbeef->conf_save ();
}

- (IBAction)delete:(id)sender {
    deadbeef->pl_delete_selected ();
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (IBAction)orderLinearAction:(id)sender {
    deadbeef->streamer_set_shuffle (DDB_SHUFFLE_OFF);
}

- (IBAction)orderRandomAction:(id)sender {
    deadbeef->streamer_set_shuffle (DDB_SHUFFLE_RANDOM);
}

- (IBAction)orderShuffleAction:(id)sender {
    deadbeef->streamer_set_shuffle (DDB_SHUFFLE_TRACKS);
}

- (IBAction)orderShuffleAlbumsAction:(id)sender {
    deadbeef->streamer_set_shuffle (DDB_SHUFFLE_ALBUMS);
}

- (IBAction)loopAllAction:(id)sender {
    deadbeef->streamer_set_repeat (DDB_REPEAT_ALL);
}

- (IBAction)loopNoneAction:(id)sender {
    deadbeef->streamer_set_repeat (DDB_REPEAT_OFF);
}

- (IBAction)loopSingleAction:(id)sender {
    deadbeef->streamer_set_repeat (DDB_REPEAT_SINGLE);
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
    int state = deadbeef->conf_get_int ("playlist.stop_after_album", 0);
    state = 1 - state;
    deadbeef->conf_set_int ("playlist.stop_after_album", state);
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

    [g_appDelegate.mainWindow.rootWidget message:_id ctx:ctx p1:p1 p2:p2];
    [g_appDelegate.searchWindow.viewController sendMessage:_id ctx:ctx p1:p1 p2:p2];

    if (_id == DB_EV_CONFIGCHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(configChanged) withObject:nil waitUntilDone:NO];
    }
    else if (_id == DB_EV_SONGSTARTED) {
        [g_appDelegate performSelectorOnMainThread:@selector(updateDockNowPlaying) withObject:nil waitUntilDone:NO];
    }
    else if (_id == DB_EV_SONGFINISHED) {
        [g_appDelegate performSelectorOnMainThread:@selector(clearDockNowPlaying) withObject:nil waitUntilDone:NO];
    }
    else if (_id == DB_EV_SONGCHANGED || _id == DB_EV_TRACKINFOCHANGED || (_id == DB_EV_PLAYLISTCHANGED && p1 == DDB_PLAYLIST_CHANGE_CONTENT)) {
        [g_appDelegate performSelectorOnMainThread:@selector(updateTitleBar) withObject:nil waitUntilDone:NO];
    }
    else if (_id == DB_EV_VOLUMECHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(volumeChanged) withObject:nil waitUntilDone:NO];
    }
    else if (_id == DB_EV_OUTPUTCHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(outputDeviceChanged) withObject:nil waitUntilDone:NO];
    }

    return 0;
}

- (void)updateTitleBar {
    [_mainWindow updateTitleBar];
}

- (void) clearDockNowPlaying {
    if (_dockMenuNPHeading) {
        [_dockMenu removeItem:_dockMenuNPHeading];
        [_dockMenu removeItem:_dockMenuNPTitle];
        [_dockMenu removeItem:_dockMenuNPArtistAlbum];
        [_dockMenu removeItem:_dockMenuNPSeparator];
        _dockMenuNPHeading = _dockMenuNPTitle = _dockMenuNPArtistAlbum = _dockMenuNPSeparator = nil;
    }
}

- (void) updateDockNowPlaying {
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (!it) {
        [self clearDockNowPlaying];
        return;
    }
    if (_dockMenuNPHeading) {
        [_dockMenu removeItem:_dockMenuNPTitle];
        [_dockMenu removeItem:_dockMenuNPArtistAlbum];
    }

    if (!_dockMenuNPHeading) {
        _dockMenuNPHeading = [[NSMenuItem alloc] initWithTitle:@"Now playing:" action:nil keyEquivalent:@""];
        _dockMenuNPHeading.enabled = NO;
        _dockMenuNPSeparator = [NSMenuItem separatorItem];
        [_dockMenu insertItem:_dockMenuNPHeading atIndex:0];
        [_dockMenu insertItem:_dockMenuNPSeparator atIndex:1];
    }

    if (!_titleScript) {
        _titleScript = deadbeef->tf_compile ("   %title%");
    }
    if (!_artistAlbumScript) {
        _artistAlbumScript = deadbeef->tf_compile ("   %artist%[ - %album%]");
    }
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = it,
        .plt = deadbeef->plt_get_curr (),
    };

    char title[1024] = "";
    char artistAlbum[1024] = "";
    deadbeef->tf_eval (&ctx, _titleScript, title, sizeof (title));
    deadbeef->tf_eval (&ctx, _artistAlbumScript, artistAlbum, sizeof (artistAlbum));
    if (ctx.plt) {
        deadbeef->plt_unref (ctx.plt);
    }
    if (it) {
        deadbeef->pl_item_unref (it);
    }
    _dockMenuNPTitle = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:title] action:nil keyEquivalent:@""];
    _dockMenuNPTitle.enabled = NO;
    _dockMenuNPArtistAlbum = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:artistAlbum] action:nil keyEquivalent:@""];
    _dockMenuNPArtistAlbum.enabled = NO;
    [_dockMenu insertItem:_dockMenuNPTitle atIndex:1];
    [_dockMenu insertItem:_dockMenuNPArtistAlbum atIndex:2];
}

- (IBAction)performFindPanelAction:(id)sender {
    _searchWindow.window.isVisible = YES;
    [_searchWindow.window makeKeyWindow];
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
    openDlg.canChooseFiles = YES;
    openDlg.allowsMultipleSelection = YES;
    openDlg.canChooseDirectories = NO;
    if ([openDlg runModal] == NSModalResponseOK)
    {
        NSArray* files = [openDlg URLs];
        if ([files count] < 1) {
            return;
        }
        NSString *fname = [[files firstObject] path];
        dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_async(aQueue, ^{
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (!deadbeef->plt_add_files_begin (plt, 0)) {
                    deadbeef->plt_clear (plt);
                    int abort = 0;
                    deadbeef->plt_load2 (0, plt, NULL, [fname UTF8String], &abort, NULL, NULL);
                    deadbeef->plt_save_config (plt);
                    deadbeef->plt_add_files_end (plt, 0);
            }
            deadbeef->plt_unref (plt);
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        });
    }
}

- (IBAction)savePlaylistAction:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.title = @"Save Playlist";
    panel.canCreateDirectories = YES;
    panel.extensionHidden = NO;

    NSString *message = @"Supported file types: .dbpl";

    NSMutableArray *types = [NSMutableArray new];
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

    panel.message = message;
    panel.allowedFileTypes = types;
    panel.allowsOtherFileTypes = NO;

    if ([panel runModal] == NSModalResponseOK) {
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

- (IBAction)openMedialibPrefs:(id)sender {
    if (!_prefWindow) {
        _prefWindow = [[PreferencesWindowController alloc] initWithWindowNibName:@"Preferences"];
    }
    [_prefWindow switchToTab:@"Medialib"];
    [_prefWindow showWindow:self];
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender {
    return _dockMenu;
}

- (IBAction)showHelp:(id)sender {
    if (!_helpWindow) {
        _helpWindow = [[HelpWindowController alloc] initWithWindowNibName:@"HelpViewer"];
    }

    if (![_helpWindow.window isVisible]) {
        [_helpWindow showWindow:self];
    }
}

@end
