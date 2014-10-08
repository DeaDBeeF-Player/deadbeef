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
#import "DdbSearchViewController.h"

#include "../../deadbeef.h"
#include <sys/time.h>

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

- (void)configChanged
{
    id order_items[] = {
        self.orderLinear,
        self.orderShuffle,
        self.orderRandom,
        self.orderShuffleAlbums,
        nil
    };
    
    int order = deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    for (int i = 0; order_items[i]; i++) {
        [order_items[i] setState:i==order?NSOnState:NSOffState];
    }
    
    id loop_items[] = {
        self.loopAll,
        self.loopNone,
        self.loopSingle,
        nil
    };
    
    int loop = deadbeef->conf_get_int ("playback.loop", PLAYBACK_MODE_LOOP_ALL);
    for (int i = 0; loop_items[i]; i++) {
        [loop_items[i] setState:i==loop?NSOnState:NSOffState];
    }
    
    [self.scrollFollowsPlayback setState:deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)?NSOnState:NSOffState];
    [self.cursorFollowsPlayback setState:deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)?NSOnState:NSOffState];
    
    [self.stopAfterCurrent setState:deadbeef->conf_get_int ("playlist.stop_after_current", 0)?NSOnState:NSOffState];
    [self.stopAfterCurrentAlbum setState:deadbeef->conf_get_int ("playlist.stop_after_current_album", 0)?NSOnState:NSOffState];
}

static int fileadd_cancelled = 0;

static void fileadd_begin (ddb_fileadd_data_t *data, void *user_data) {
    fileadd_cancelled = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp beginSheet:g_appDelegate.addFilesWindow modalForWindow:g_appDelegate.window modalDelegate:nil didEndSelector:nil contextInfo:nil];
    });
}

static void fileadd_end (ddb_fileadd_data_t *data, void *user_data) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [g_appDelegate.addFilesWindow orderOut:g_appDelegate];
        [NSApp endSheet:g_appDelegate.addFilesWindow];
        [g_appDelegate.window makeKeyAndOrderFront:g_appDelegate];
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
    [self createPlaylistView];
    [self initSearchWindow];
}

- (void)createPlaylistView {
    DdbPlaylistViewController *vc = [[DdbPlaylistViewController alloc] init];
    NSView *view = [vc view];

    [view setFrame:NSMakeRect(0, [self.statusBar frame].size.height+2, [[self.window contentView] frame].size.width, [self.tabStrip frame].origin.y - [self.statusBar frame].size.height - 1)];
    [view setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];

    [[self.window contentView] addSubview:view];
}

- (void)initSearchWindow {
    DdbSearchViewController *vc = [[DdbSearchViewController alloc] init];
    NSView *view = [vc view];

    NSRect frame = [[_searchWindow contentView] frame];

    [view setFrame:frame];
    [view setAutoresizingMask:NSViewWidthSizable|NSViewHeightSizable];

    [[_searchWindow contentView] addSubview:view];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [self.window setReleasedWhenClosed:NO];
    [self.window setExcludedFromWindowsMenu:YES];

    playImg = [NSImage imageNamed:@"btnplayTemplate.pdf"];
    pauseImg = [NSImage imageNamed:@"btnpauseTemplate.pdf"];
    bufferingImg = [NSImage imageNamed:@"bufferingTemplate.pdf"];

    [playlist setDelegate:(id<NSTableViewDelegate>)self];
    [playlist setDataSource:(id<NSTableViewDataSource>)self];
    [playlist setDoubleAction:@selector(playlistDoubleAction)];
    
    // initialize gui from settings
    [self configChanged];
    
    NSTimer *updateTimer = [NSTimer timerWithTimeInterval:1.0f/10.0f target:self selector:@selector(frameUpdate:) userInfo:nil repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:updateTimer forMode:NSDefaultRunLoopMode];
    
    [self.addFilesWindow setParentWindow:self.window];
    
    deadbeef->listen_file_add_beginend (fileadd_begin, fileadd_end, NULL);
    deadbeef->listen_file_added (file_added, NULL);

    [self initColumns];

    g_appDelegate = self;
    [[NSApp dockTile] setContentView: self.dockTileView];
//    [[NSApp dockTile] setBadgeLabel:@"Hello"];
    [[NSApp dockTile] display];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag{
    [self.window setIsVisible:YES];
    return YES;
}

- (IBAction)showMainWinAction:(id)sender {
    NSInteger st = [sender state];
    [self.window setIsVisible:st!=NSOnState];
    [sender setState:st==NSOnState?NSOffState:NSOnState];
}

int prevSeekbar = -1;

// update status bar and window title
static char sb_text[512];
static char sbitrate[20] = "";
static struct timeval last_br_update;

#define _(x) x

- (void)updateSonginfo {
    DB_output_t *output = deadbeef->get_output ();
    char sbtext_new[512] = "-";
    
    float pl_totaltime = deadbeef->pl_get_totaltime ();
    int daystotal = (int)pl_totaltime / (3600*24);
    int hourtotal = ((int)pl_totaltime / 3600) % 24;
    int mintotal = ((int)pl_totaltime/60) % 60;
    int sectotal = ((int)pl_totaltime) % 60;
    
    char totaltime_str[512] = "";
    if (daystotal == 0) {
        snprintf (totaltime_str, sizeof (totaltime_str), "%d:%02d:%02d", hourtotal, mintotal, sectotal);
    }
    else if (daystotal == 1) {
        snprintf (totaltime_str, sizeof (totaltime_str), _("1 day %d:%02d:%02d"), hourtotal, mintotal, sectotal);
    }
    else {
        snprintf (totaltime_str, sizeof (totaltime_str), _("%d days %d:%02d:%02d"), daystotal, hourtotal, mintotal, sectotal);
    }
    
    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    DB_fileinfo_t *c = deadbeef->streamer_get_current_fileinfo (); // FIXME: might crash streamer
    
    float duration = track ? deadbeef->pl_get_item_duration (track) : -1;
    
    if (!output || (output->state () == OUTPUT_STATE_STOPPED || !track || !c)) {
        snprintf (sbtext_new, sizeof (sbtext_new), _("Stopped | %d tracks | %s total playtime"), deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }
    else {
        float playpos = deadbeef->streamer_get_playpos ();
        int minpos = playpos / 60;
        int secpos = playpos - minpos * 60;
        int mindur = duration / 60;
        int secdur = duration - mindur * 60;
        
        const char *mode;
        char temp[20];
        if (c->fmt.channels <= 2) {
            mode = c->fmt.channels == 1 ? _("Mono") : _("Stereo");
        }
        else {
            snprintf (temp, sizeof (temp), "%dch Multichannel", c->fmt.channels);
            mode = temp;
        }
        int samplerate = c->fmt.samplerate;
        int bitspersample = c->fmt.bps;
        //        codec_unlock ();
        
        char t[100];
        if (duration >= 0) {
            snprintf (t, sizeof (t), "%d:%02d", mindur, secdur);
        }
        else {
            strcpy (t, "-:--");
        }
        
        struct timeval tm;
        gettimeofday (&tm, NULL);
        if (tm.tv_sec - last_br_update.tv_sec + (tm.tv_usec - last_br_update.tv_usec) / 1000000.0 >= 0.3) {
            memcpy (&last_br_update, &tm, sizeof (tm));
            int bitrate = deadbeef->streamer_get_apx_bitrate ();
            if (bitrate > 0) {
                snprintf (sbitrate, sizeof (sbitrate), _("| %4d kbps "), bitrate);
            }
            else {
                sbitrate[0] = 0;
            }
        }
        const char *spaused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED ? _("Paused | ") : "";
        char filetype[20];
        if (!deadbeef->pl_get_meta (track, ":FILETYPE", filetype, sizeof (filetype))) {
            strcpy (filetype, "-");
        }
        snprintf (sbtext_new, sizeof (sbtext_new), _("%s%s %s| %dHz | %d bit | %s | %d:%02d / %s | %d tracks | %s total playtime"), spaused, filetype, sbitrate, samplerate, bitspersample, mode, minpos, secpos, t, deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }
    
    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);
        [[self statusBar] setStringValue:[NSString stringWithUTF8String:sb_text]];
    }
    
    if (track) {
        deadbeef->pl_item_unref (track);
    }
}


- (void)frameUpdate:(id)userData
{
    if (![[self window] isVisible]) {
        return;
    }
    float dur = -1;
    float perc = 0;
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        dur = deadbeef->pl_get_item_duration (trk);
        if (dur >= 0) {
            perc = deadbeef->streamer_get_playpos () / dur * 100.f;
            if (perc < 0) {
                perc = 0;
            }
            else if (perc > 100) {
                perc = 100;
            }
        }
        deadbeef->pl_item_unref (trk);
    }
    
    int cmp =(int)(perc*4000);
    if (cmp != prevSeekbar) {
        prevSeekbar = cmp;
        [self.seekBar setFloatValue:perc];
    }
    
    BOOL st = YES;
    if (!trk || dur < 0) {
        st = NO;
    }
    if ([self.seekBar isEnabled] != st) {
        [self.seekBar setEnabled:st];
    }
    
    [self updateSonginfo];    
}

- (void)playlistDoubleAction
{
    int row = (int)[playlist clickedRow];
    deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, row, 0);
}


- (IBAction)tbClicked:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];
    
    switch (selectedSegment) {
        case 0:
            deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
            break;
        case 1:
            deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
            break;
        case 2:
            deadbeef->sendmessage(DB_EV_TOGGLE_PAUSE, 0, 0, 0);
            break;
        case 3:
            deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
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

- (void)tfRedraw:(id)userInfo
{
    [self.playlist reloadData];
    [self.tfRedrawTimer invalidate];
    self.tfRedrawTimer = nil;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
    id ret = nil;
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

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();
    if ([[aTableColumn identifier] isEqualToString:@"playing"] && it == playing_track) {
        NSImage *img = NULL;
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
        ret = img;
    }
    else {
        int cidx = -1;
        // FIXME: nstableview can't render alternating text/image in one column
        /*if ([[aTableColumn identifier] isEqualToString:@"playing"]) {
            cidx = 0;
        }
        else*/
        if ([[aTableColumn identifier] isEqualToString:@"albumartist"]) {
            cidx = 1;
        }
        else if ([[aTableColumn identifier] isEqualToString:@"trknum"]) {
            cidx = 2;
        }
        else if ([[aTableColumn identifier] isEqualToString:@"title"]) {
            cidx = 3;
        }
        else if ([[aTableColumn identifier] isEqualToString:@"duration"]) {
            cidx = 4;
        }

        if (cidx != -1) {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .it = it,
                .plt = deadbeef->plt_get_curr (),
                .idx = -1,
                .id = columns[cidx]._id
            };
            char text[1024] = "";
            deadbeef->tf_eval (&ctx, columns[cidx].bytecode, columns[cidx].bytecode_len, text, sizeof (text));
            if (ctx.update > 0 && !self.tfRedrawTimer) {
                if (ctx.idx >= 0) {
                    self.tf_redraw_track_idx = ctx.idx;
                }
                else {
                    self.tf_redraw_track_idx = deadbeef->plt_get_item_idx (ctx.plt, it, PL_MAIN);
                }
                self.tfRedrawTimer = [NSTimer timerWithTimeInterval:ctx.update*0.001f target:self selector:@selector(tfRedraw:) userInfo:nil repeats:YES];
                [[NSRunLoop currentRunLoop] addTimer:self.tfRedrawTimer forMode:NSDefaultRunLoopMode];
                self.tf_redraw_track = it;
                deadbeef->pl_item_ref (it);
            }
            if (ctx.plt) {
                deadbeef->plt_unref (ctx.plt);
                ctx.plt = NULL;
            }
            char *lb = strchr (text, '\r');
            if (lb) {
                *lb = 0;
            }
            lb = strchr (text, '\n');
            if (lb) {
                *lb = 0;
            }
            ret = [NSString stringWithUTF8String:text];
        }
    }

    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }

    return ret;
}

- (IBAction)openFilesAction:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:YES];
    [openDlg setCanChooseDirectories:NO];
    if ( [openDlg runModal] == NSOKButton )
    {
        NSArray* files = [openDlg URLs];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        deadbeef->plt_clear(plt);
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
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [playlist reloadData];
                    });
                });
            }
        }
    }
}

- (IBAction)addFilesAction:(id)sender {
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
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [playlist reloadData];
                    });
                });
            }
        }
    }
}

- (IBAction)clearAction:(id)sender {
    deadbeef->pl_clear();
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    [playlist reloadData];
}

- (IBAction)delete:(id)sender {
    deadbeef->pl_delete_selected ();
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
    [playlist reloadData];
    [playlist deselectAll:self];
    if (firstSelected != -1) {
        [playlist selectRowIndexes:[[NSIndexSet alloc] initWithIndex:firstSelected] byExtendingSelection:NO];
    }
    [self tableView:playlist selectionIndexesForProposedSelection: firstSelected==-1?[[NSIndexSet alloc] init]:[[NSIndexSet alloc] initWithIndex:firstSelected]];
}

- (void)reloadPlaylistData {
    [playlist reloadData];
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

- (IBAction)jumpToCurrentAction:(id)sender {
    deadbeef->sendmessage (DB_EV_TRACKFOCUSCURRENT, 0, 0, 0);
}

- (void)focusCurrent
{
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
        if (plt) {
            deadbeef->plt_set_curr (plt);
            int idx = deadbeef->pl_get_idx_of (it);
            if (idx != -1) {
                [playlist scrollRowToVisible:idx];
            }
            deadbeef->plt_unref (plt);
        }
        deadbeef->pl_item_unref (it);
    }
    deadbeef->pl_unlock ();
}

- (IBAction)seekBarAction:(id)sender {
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
    if (trk) {
        float dur = deadbeef->pl_get_item_duration (trk);
        if (dur >= 0) {
            float time = [(NSSlider*)sender floatValue] / 100.f;
            time *= dur;
            deadbeef->sendmessage (DB_EV_SEEK, 0, time * 1000, 0);
        }
        deadbeef->pl_item_unref (trk);
    }
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
}

- (IBAction)selectionCropAction:(id)sender {
}

- (void)handleSimpleMessage:(NSNumber *)_id {
    int idx = -1;
    DB_playItem_t *it = NULL;
    switch ([_id intValue]) {
        case DB_EV_PLAYLISTCHANGED:
        case DB_EV_PLAYLISTSWITCHED:
            [[self tabStrip] setNeedsDisplay:YES];
            [self reloadPlaylistData];
            break;
        case DB_EV_TRACKFOCUSCURRENT:
            [self focusCurrent];
            break;
        case DB_EV_PAUSED:
            it = deadbeef->streamer_get_playing_track ();
            if (it) {
                idx = deadbeef->pl_get_idx_of (it);
                deadbeef->pl_item_unref (it);
            }
            if (idx) {
                [playlist reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:idx]
                                    columnIndexes:[NSIndexSet indexSetWithIndex:0]];
            }
            break;
        case DB_EV_SONGCHANGED:
            it = deadbeef->streamer_get_playing_track ();
            if (it) {
                idx = deadbeef->pl_get_idx_of (it);
                deadbeef->pl_item_unref (it);
            }
            if (idx != -1) {
                if (deadbeef->conf_get_int ("playlist.scroll.followplayback", 1)) {
                    [playlist scrollRowToVisible:idx];
                }
                if (deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1)) {
                    [playlist selectRowIndexes:[NSIndexSet indexSetWithIndex:idx] byExtendingSelection:NO];
                }
            }
            break;
    }
}

- (void)trackInfoChanged:(NSNumber *)trk {
        [playlist reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:[trk intValue]]
                         columnIndexes:[NSIndexSet indexSetWithIndex:0]];
}

+ (int)ddb_message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2
{
    [[DdbWidgetManager defaultWidgetManager] widgetMessage:_id ctx:ctx p1:p1 p2:p2];
    
    if (_id == DB_EV_PAUSED || _id == DB_EV_PLAYLIST_REFRESH || _id == DB_EV_PLAYLISTCHANGED || _id == DB_EV_PLAYLISTSWITCHED || _id == DB_EV_TRACKFOCUSCURRENT || _id == DB_EV_SONGCHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(handleSimpleMessage:) withObject:[[NSNumber alloc] initWithInt:_id] waitUntilDone:NO];
    }
    else if (_id == DB_EV_TRACKINFOCHANGED) {
        ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
        int idx = deadbeef->pl_get_idx_of_iter (ev->track, PL_MAIN);
        if (idx != -1) {
            [g_appDelegate performSelectorOnMainThread:@selector(trackInfoChanged:) withObject:[[NSNumber alloc] initWithInt:idx] waitUntilDone:NO];
        }
    }
    else if (_id == DB_EV_CONFIGCHANGED) {
        [g_appDelegate performSelectorOnMainThread:@selector(configChanged) withObject:nil waitUntilDone:NO];

    }
    return 0;
}

- (void)performFindPanelAction:(id)sender {
    [_searchWindow setIsVisible:YES];
    [_searchWindow makeKeyWindow];
}

@end
