/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#include <deadbeef/deadbeef.h>
#import "DdbShared.h"
#import "DeletePlaylistConfirmationController.h"
#import "DesignModeDeps.h"
#import "DesignModeState.h"
#import "GuiPreferencesWindowController.h"
#import "MainWindowController.h"
#ifdef ENABLE_MEDIALIB
#import "MediaLibraryManager.h"
#endif
#import "PlaylistWidget.h"
#import "PlaylistWithTabsWidget.h"
#import "PreferencesWindowController.h"
#import "ScriptableErrorViewer.h"
#import "ScriptableSelectViewController.h"
#import "SidebarSplitViewController.h"
#import "TrackPositionFormatter.h"
#include <sys/time.h>

extern DB_functions_t *deadbeef;

@interface MainWindowController () <NSMenuDelegate,DeletePlaylistConfirmationControllerDelegate,ScriptableSelectDelegate,ScriptableItemDelegate> {
    NSTimer *_updateTimer;
    char *_titlebar_playing_script;
    char *_titlebar_playing_subtitle_script;
    char *_titlebar_stopped_script;
    char *_titlebar_stopped_subtitle_script;
    char *_statusbar_playing_script;
    int _prevSeekBarPos;
    int _deletePlaylistIndex;
}

@property (nonatomic) ScriptableTableDataSource *mlQueriesDataSource;
@property (nonatomic) ScriptableSelectViewController *tfQuerySelectViewController;

@property (nonatomic,weak) IBOutlet NSView *designableContainerView;
@property (nonatomic,weak) IBOutlet NSView *playlistWithTabsView;
@property (nonatomic) IBOutlet SidebarSplitViewController *splitViewController;
@property (nonatomic) MainContentViewController *mainContentViewController;

@property (weak) NSMenuItem *designModeMenuItem;

@property (strong) IBOutlet NSMenu *volumeScaleMenu;
@property (weak) IBOutlet NSMenuItem *volumeDbScaleItem;
@property (weak) IBOutlet NSMenuItem *volumeLinearScaleItem;
@property (weak) IBOutlet NSMenuItem *volumeCubicScaleItem;
@property (weak) IBOutlet NSView *tfQueryContainer;

@end


@implementation MainWindowController

- (void)cleanup {
    // not releasing this timer explicitly causes a reference cycle
    if (_updateTimer) {
        [_updateTimer invalidate];
        _updateTimer = nil;
    }
    [self.sidebarOutlineViewController close];
    [DesignModeDeps cleanup];
}

- (void)dealloc {
    [self cleanup];
    [self freeTitleBarConfig];
}

// Find and activate the first visible playlist widget
- (BOOL)setupInitialFirstResponder:(id<WidgetProtocol>)widget {
    BOOL visible = widget.view.window != nil; // inactive tab check within NSTabView
    BOOL responder = [widget respondsToSelector:@selector(makeFirstResponder)];
    BOOL isPlaylistWithTabs = [widget.widgetType isEqualToString:PlaylistWithTabsWidget.widgetType];
    BOOL isPlaylist = [widget.widgetType isEqualToString:PlaylistWidget.widgetType];

    if (visible && responder
        && (isPlaylist || isPlaylistWithTabs)) {
        [widget makeFirstResponder];
        return YES;
    }

    for (id<WidgetProtocol> child in widget.childWidgets) {
        if ([self setupInitialFirstResponder:child]) {
            return YES;
        }
    }

    return NO;
}

- (void)windowDidLoad {
    [super windowDidLoad];

    // This doesn't work reliably when set in XIB
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
    if (@available(macOS 10.16, *)) {
        self.window.toolbarStyle = NSWindowToolbarStyleExpanded;
    }
#endif

#if ENABLE_MEDIALIB
    self.playlistWithTabsView = self.splitViewController.bodyViewController.wrapperView;
    self.designableContainerView = self.splitViewController.bodyViewController.designableView;


    if (self.mediaLibraryManager.medialibPlugin && self.mediaLibraryManager.source) {
        scriptableItem_t *tfQueryRoot =  self.mediaLibraryManager.medialibPlugin->get_queries_scriptable(self.mediaLibraryManager.source);
        self.mlQueriesDataSource = [ScriptableTableDataSource dataSourceWithScriptable:tfQueryRoot];
    }

    // preset list and browse button
    self.tfQuerySelectViewController = [ScriptableSelectViewController new];
    self.tfQuerySelectViewController.scriptableItemDelegate = self;
    self.tfQuerySelectViewController.scriptableSelectDelegate = self;
    self.tfQuerySelectViewController.view.frame = _tfQueryContainer.bounds;
    [_tfQueryContainer addSubview:self.tfQuerySelectViewController.view];
    self.tfQuerySelectViewController.errorViewer = ScriptableErrorViewer.sharedInstance;
    self.tfQuerySelectViewController.dataSource = self.mlQueriesDataSource;

    if (self.mlQueriesDataSource.scriptable != NULL) {
        NSString *preset = self.mediaLibraryManager.preset;
        scriptableItem_t *currentPreset = scriptableItemSubItemForName(self.mlQueriesDataSource.scriptable, preset.UTF8String);
        if (currentPreset != NULL) {
            [self.tfQuerySelectViewController selectItem:currentPreset];
        }
    }

#else
    self.mainContentViewController = [MainContentViewController new];
    [self.designableContainerView addSubview:self.mainContentViewController.view];
    [NSLayoutConstraint activateConstraints:@[
        [self.designableContainerView.topAnchor constraintEqualToAnchor:self.mainContentViewController.view.topAnchor],
        [self.designableContainerView.bottomAnchor constraintEqualToAnchor:self.mainContentViewController.view.bottomAnchor],
        [self.designableContainerView.leadingAnchor constraintEqualToAnchor:self.mainContentViewController.view.leadingAnchor],
        [self.designableContainerView.trailingAnchor constraintEqualToAnchor:self.mainContentViewController.view.trailingAnchor],
    ]];

    self.designableContainerView = self.mainContentViewController.designableView;
#endif

    id<WidgetProtocol> rootWidget = DesignModeState.sharedInstance.rootWidget;
    NSView *view = rootWidget.view;
    view.translatesAutoresizingMaskIntoConstraints = NO;

    [self.designableContainerView addSubview:view];

    [view.topAnchor constraintEqualToAnchor:self.designableContainerView.topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.designableContainerView.bottomAnchor].active = YES;
    [view.leadingAnchor constraintEqualToAnchor:self.designableContainerView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.designableContainerView.trailingAnchor].active = YES;

    NSLayoutYAxisAnchor *topAnchor;
    if (self.window.contentLayoutGuide && self.playlistWithTabsView) {
        // HACK: this is not well-documented and not safe.
        // This code constrains the contentview to contentLayoutGuide object,
        // in order to avoid clipping of the playlist header view.
        // The information was obtained from https://developer.apple.com/videos/play/wwdc2016/239/
        topAnchor = [self.window.contentLayoutGuide valueForKey:@"topAnchor"];

        NSLayoutConstraint *constraint = [self.playlistWithTabsView.topAnchor constraintEqualToAnchor:topAnchor];
        constraint.priority = NSLayoutPriorityRequired;
        constraint.active = YES;
    }
    // seekbar value formatter
    self.seekBar.formatter = [TrackPositionFormatter new];

    [self ensureRefresh];
}

- (void)ensureRefresh {
    if (_updateTimer != nil) {
        return;
    }
    if (deadbeef->get_output()->state() != DDB_PLAYBACK_STATE_PLAYING) {
        [self frameUpdate];
        return;
    }

    __weak MainWindowController *weakSelf = self;
    _updateTimer = [NSTimer timerWithTimeInterval:1.0f/10.0f repeats:YES block:^(NSTimer * _Nonnull timer) {
        MainWindowController *strongSelf = weakSelf;
        if (strongSelf == nil) {
            return;
        }

        [strongSelf frameUpdate];

        if (deadbeef->get_output()->state() != DDB_PLAYBACK_STATE_PLAYING) {
            [strongSelf->_updateTimer invalidate];
            strongSelf->_updateTimer = nil;
        }
    }];

    [[NSRunLoop currentRunLoop] addTimer:_updateTimer forMode:NSRunLoopCommonModes];
}

// update status bar and window title
static char sb_text[512];

#define _(x) x

- (void)updateSonginfo {
    DB_output_t *output = deadbeef->get_output ();
    char sbtext_new[512] = "-";
    
    float pl_totaltime = roundf(deadbeef->pl_get_totaltime ());
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
    
    DB_playItem_t *track = deadbeef->streamer_get_playing_track_safe ();
    
    if (!output || (output->state () == DDB_PLAYBACK_STATE_STOPPED || !track)) {
        snprintf (sbtext_new, sizeof (sbtext_new), _("Stopped | %d tracks | %s total playtime"), deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }
    else {
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = track,
            .iter = PL_MAIN,
        };

        char buffer[200];
        deadbeef->tf_eval (&ctx, _statusbar_playing_script, buffer, sizeof (buffer));
        snprintf (sbtext_new, sizeof (sbtext_new), "%s | %d tracks | %s total playtime", buffer, deadbeef->pl_getcount (PL_MAIN), totaltime_str);
    }
    
    if (strcmp (sbtext_new, sb_text)) {
        strcpy (sb_text, sbtext_new);
        self.statusBar.stringValue = @(sb_text);
    }
    
    if (track) {
        deadbeef->pl_item_unref (track);
    }
}

- (void)refreshSeekBar:(DB_playItem_t *)trk {
    float dur = -1;
    float perc = 0;
    if (trk) {
        dur = deadbeef->pl_get_item_duration (trk);
        if (dur > 0) {
            perc = deadbeef->streamer_get_playpos () / dur * 100.f;
            if (perc < 0) {
                perc = 0;
            }
            else if (perc > 100) {
                perc = 100;
            }
        }
        else {
            perc = 0;
        }
    }

    if (!_seekBar.dragging) {
        int cmp =(int)(perc*4000);
        if (cmp != _prevSeekBarPos) {
            _prevSeekBarPos = cmp;
            _seekBar.floatValue = perc;
        }
    }

    BOOL st = YES;
    if (!trk || dur < 0) {
        st = NO;
    }
    if (_seekBar.isEnabled != st) {
        _seekBar.enabled = st;
    }
}

- (void)frameUpdate {
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track_safe ();

    [self refreshSeekBar:trk];

    [self updateSonginfo];

    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
}

- (IBAction)seekBarAction:(DdbSeekBar *)sender {
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track_safe ();
    if (trk) {
        float dur = deadbeef->pl_get_item_duration (trk);
        if (dur >= 0) {
            float time = sender.floatValue / 100.f;
            time *= dur;
            deadbeef->sendmessage (DB_EV_SEEK, 0, (uint32_t)(time * 1000), 0);
        }
        deadbeef->pl_item_unref (trk);
    }
}

- (void)updateVolumeBar {
    char scale[10];
    deadbeef->conf_get_str ("playback.volume.scale", "dB", scale, sizeof(scale));

    if (!strcmp (scale, "linear")) {
        self.volumeBar.floatValue = deadbeef->volume_get_amp () * 100;
    }
    else if (!strcmp (scale, "cubic")) {
        self.volumeBar.floatValue = cbrt(deadbeef->volume_get_amp ()) * 100;
    }
    else {
        float range = -deadbeef->volume_get_min_db ();
        float vol = (deadbeef->volume_get_db () + range) / range * 100;
        self.volumeBar.floatValue = vol;
    }

    [self updateVolumeBarTooltip];
}

- (IBAction)volumeBarAction:(NSSlider *)sender {
    char scale[10];
    deadbeef->conf_get_str ("playback.volume.scale", "dB", scale, sizeof(scale));

    float value = sender.floatValue / 100.f;

    if (!strcmp (scale, "linear")) {
        deadbeef->volume_set_amp (value);
    }
    else if (!strcmp (scale, "cubic")) {
        deadbeef->volume_set_amp (value * value * value);
    }
    else {
        float range = -deadbeef->volume_get_min_db ();
        float volume = value * range - range;
        if (volume < -range) {
            volume = -range;
        }
        if (volume > 0) {
            volume = 0;
        }

        deadbeef->volume_set_db (volume);
    }

    [self updateVolumeBarTooltip];
}

- (void)updateVolumeBarTooltip {
    char scale[10];
    deadbeef->conf_get_str ("playback.volume.scale", "dB", scale, sizeof(scale));

    if (!strcmp (scale, "linear") || !strcmp (scale, "cubic")) {
        int percents = (int)(deadbeef->volume_get_amp() * 100);
        self.volumeBar.toolTip = [NSString stringWithFormat:@"%d%%", percents];
    }
    else {
        int db = (int)deadbeef->volume_get_db();
        self.volumeBar.toolTip = [NSString stringWithFormat:@"%s%ddB", db < 0 ? "" : "+", db];
    }
}

- (IBAction)selectDbScaleAction:(NSMenuItem *)sender {
    if (sender.state != NSControlStateValueOn) {
        deadbeef->conf_set_str("playback.volume.scale", "dB");
        deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
        deadbeef->conf_save();
    }
}

- (IBAction)selectLinearScaleAction:(NSMenuItem *)sender {
    if (sender.state != NSControlStateValueOn) {
        deadbeef->conf_set_str("playback.volume.scale", "linear");
        deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
        deadbeef->conf_save();
    }
}

- (IBAction)selectCubicScaleAction:(NSMenuItem *)sender {
    if (sender.state != NSControlStateValueOn) {
        deadbeef->conf_set_str("playback.volume.scale", "cubic");
        deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
        deadbeef->conf_save();
    }
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

- (IBAction)performCloseTabAction:(id)sender {
    _deletePlaylistIndex = deadbeef->plt_get_curr_idx ();
    if (_deletePlaylistIndex != -1) {
        DeletePlaylistConfirmationController *controller = [DeletePlaylistConfirmationController new];
        controller.window = self.window;
        controller.title = plt_get_title_wrapper (_deletePlaylistIndex);
        controller.delegate = self;
        [controller run];
    }
}

- (void)freeTitleBarConfig {
    if (_titlebar_playing_script) {
        deadbeef->tf_free (_titlebar_playing_script);
        _titlebar_playing_script = NULL;
    }

    if (_titlebar_playing_subtitle_script) {
        deadbeef->tf_free (_titlebar_playing_subtitle_script);
        _titlebar_playing_subtitle_script = NULL;
    }

    if (_titlebar_stopped_script) {
        deadbeef->tf_free (_titlebar_stopped_script);
        _titlebar_stopped_script = NULL;
    }

    if (_titlebar_stopped_subtitle_script) {
        deadbeef->tf_free (_titlebar_stopped_subtitle_script);
        _titlebar_stopped_subtitle_script = NULL;
    }

    if (_statusbar_playing_script) {
        deadbeef->tf_free (_statusbar_playing_script);
        _statusbar_playing_script = NULL;
    }
}

- (void)updateTitleBarConfig {
    [self freeTitleBarConfig];

    char script[2000];

    deadbeef->conf_get_str ("cocoaui.titlebar_playing", DEFAULT_TITLEBAR_PLAYING_VALUE, script, sizeof (script));
    _titlebar_playing_script = deadbeef->tf_compile (script);

    deadbeef->conf_get_str ("cocoaui.titlebar_subtitle_playing", DEFAULT_TITLEBAR_SUBTITLE_PLAYING_VALUE, script, sizeof (script));
    _titlebar_playing_subtitle_script = deadbeef->tf_compile (script);

    deadbeef->conf_get_str ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_STOPPED_VALUE, script, sizeof (script));
    _titlebar_stopped_script = deadbeef->tf_compile (script);

    deadbeef->conf_get_str ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_SUBTITLE_STOPPED_VALUE, script, sizeof (script));
    _titlebar_stopped_subtitle_script = deadbeef->tf_compile (script);

    _statusbar_playing_script = deadbeef->tf_compile ("$if2($strcmp(%ispaused%,),Paused | )$if2($upper(%codec%),-) |[ %playback_bitrate% kbps |][ %samplerate%Hz |][ %:BPS% bit |][ %channels% |] %playback_time% / %length%");
}

- (void)updateTitleBar {
    if (!_titlebar_playing_script || !_titlebar_stopped_script) {
        [self updateTitleBarConfig];
    }

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = deadbeef->streamer_get_playing_track_safe (),
        .iter = PL_MAIN,
    };

    char titleBuffer[200];
    deadbeef->tf_eval (&ctx, ctx.it ? _titlebar_playing_script : _titlebar_stopped_script, titleBuffer, sizeof (titleBuffer));

    char subtitleBuffer[200];
    deadbeef->tf_eval (&ctx, ctx.it ? _titlebar_playing_subtitle_script : _titlebar_stopped_subtitle_script, subtitleBuffer, sizeof (subtitleBuffer));

    if (ctx.it) {
        deadbeef->pl_item_unref (ctx.it);
    }

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
    if (@available(macOS 10.16, *)) {
        self.window.title = @(titleBuffer);
        self.window.subtitle = @(subtitleBuffer);
    } else
#endif
    {
        NSString *title = @(titleBuffer);
        NSString *subTitle = @(subtitleBuffer);

        self.window.title = [NSString stringWithFormat:@"%@%@%@", subTitle, (titleBuffer[0] && subtitleBuffer[0]) ? @" - " : @"", title];
    }
}

- (void)menuNeedsUpdate:(NSMenu *)menu {
    if (menu == self.volumeScaleMenu) {
        char scale[10];
        deadbeef->conf_get_str ("playback.volume.scale", "dB", scale, sizeof(scale));

        self.volumeDbScaleItem.state = !strcmp (scale, "dB") ? NSControlStateValueOn : NSControlStateValueOff;
        self.volumeLinearScaleItem.state = !strcmp (scale, "linear") ? NSControlStateValueOn : NSControlStateValueOff;
        self.volumeCubicScaleItem.state = !strcmp (scale, "cubic") ? NSControlStateValueOn : NSControlStateValueOff;
    }
}

- (void)message:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_PLAYBACK_STATE_DID_CHANGE) {
        [self performSelectorOnMainThread:@selector(ensureRefresh) withObject:nil waitUntilDone:NO];
    }
}

#pragma mark - DeletePlaylistConfirmationControllerDelegate

- (void)deletePlaylistDone:(DeletePlaylistConfirmationController *)controller {
    if (_deletePlaylistIndex != -1) {
        deadbeef->plt_remove (_deletePlaylistIndex);
        _deletePlaylistIndex = -1;
    }
}

#pragma mark - ScriptableSelectDelegate

- (void)scriptableSelectItemSelected:(scriptableItem_t *)item {
#if ENABLE_MEDIALIB
    const char *name = scriptableItemPropertyValueForKey(item, "name");
    self.mediaLibraryManager.preset = @(name);
#endif
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemDidChange:(scriptableItem_t *)scriptable change:(ScriptableItemChange)change {
#if ENABLE_MEDIALIB
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [self.tfQuerySelectViewController reloadData];

        // FIXME: save only when the dialog is closed
        if (self.mediaLibraryManager.medialibPlugin && self.mediaLibraryManager.source) {
            scriptableItem_t *tfQueryRoot =  self.mediaLibraryManager.medialibPlugin->get_queries_scriptable(self.mediaLibraryManager.source);
            if (tfQueryRoot != NULL) {
                scriptableItemSave(tfQueryRoot);
            }

        }
    });
#endif
}

@end
