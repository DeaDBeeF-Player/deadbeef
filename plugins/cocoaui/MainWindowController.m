/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#import "DdbPlaceholderWidget.h"
#import "DesignableViewController.h"
#import "GuiPreferencesWindowController.h"
#import "MainWindowController.h"
#import "PlaylistViewController.h"
#import "PreferencesWindowController.h"
#include "deadbeef.h"
#include <sys/time.h>

extern DB_functions_t *deadbeef;

@interface TrackPositionFormatter : NSFormatter

- (NSString *)stringForObjectValue:(NSControl *)obj;

- (BOOL)getObjectValue:(out id  _Nullable *)obj forString:(NSString *)string errorDescription:(out NSString * _Nullable *)error;

@end

@implementation TrackPositionFormatter

- (NSString *)stringForObjectValue:(NSControl *)obj {

    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    if (!track) {
        return @"--:--:--";
    }

    double pos = obj.doubleValue / 100;
    float duration = 0;
    duration = deadbeef->pl_get_item_duration (track);
    double time = duration * pos;
    int hr = time/3600;
    int mn = (time-hr*3600)/60;
    int sc = round(time-hr*3600-mn*60);

    NSString *res = [NSString stringWithFormat:@"%02d:%02d:%02d", hr, mn, sc];

    deadbeef->pl_item_unref (track);

    return res;
}

- (BOOL)getObjectValue:(out id  _Nullable __autoreleasing *)obj forString:(NSString *)string errorDescription:(out NSString * _Nullable __autoreleasing *)error {
    *error = @"error";
    return NO;
}

@end

#pragma mark -


@interface MainWindowController () {
    NSTimer *_updateTimer;
    char *_titlebar_playing_script;
    char *_titlebar_playing_subtitle_script;
    char *_titlebar_stopped_script;
    char *_titlebar_stopped_subtitle_script;
    char *_statusbar_playing_script;
    int _prevSeekBarPos;
}

@property (weak) IBOutlet NSView *designableContainerView;

@end


@implementation MainWindowController

- (void)cleanup {
    // not releasing this timer explicitly causes a reference cycle
    if (_updateTimer) {
        [_updateTimer invalidate];
        _updateTimer = nil;
    }

    [self.rootViewController cleanup];
}

- (void)dealloc {
    [self cleanup];
    [self freeTitleBarConfig];
}

- (void)windowDidLoad {
    [super windowDidLoad];

    PlaylistViewController *pvc = [[PlaylistViewController alloc] initWithNibName:nil bundle:nil];
    PlaylistView *view = [PlaylistView new];
    pvc.view = view;
    [pvc setup];
    self.rootViewController = pvc;

    view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.designableContainerView addSubview:view];

    NSLayoutYAxisAnchor *topAnchor;
    if (self.window.contentLayoutGuide) {
        // HACK: this is not well-documented and not safe.
        // This code constrains the contentview to contentLayoutGuide object,
        // in order to avoid clipping of the playlist header view.
        // The information was obtained from https://developer.apple.com/videos/play/wwdc2016/239/
        topAnchor = [self.window.contentLayoutGuide valueForKey:@"topAnchor"];
    }
    else {
        topAnchor = self.designableContainerView.topAnchor;
    }
    [view.topAnchor constraintEqualToAnchor:topAnchor].active = YES;
    [view.bottomAnchor constraintEqualToAnchor:self.designableContainerView.bottomAnchor].active = YES;
    [view.leadingAnchor constraintEqualToAnchor:self.designableContainerView.leadingAnchor].active = YES;
    [view.trailingAnchor constraintEqualToAnchor:self.designableContainerView.trailingAnchor].active = YES;

    // seekbar value formatter
    self.seekBar.formatter = [TrackPositionFormatter new];

    // add tab strip to the window titlebar
    NSTitlebarAccessoryViewController* vc = [NSTitlebarAccessoryViewController new];

    vc.view = _tabStrip;
    vc.fullScreenMinHeight = _tabStrip.bounds.size.height;
    vc.layoutAttribute = NSLayoutAttributeBottom;

    [self.window addTitlebarAccessoryViewController:vc];

    _updateTimer = [NSTimer timerWithTimeInterval:1.0f/10.0f target:self selector:@selector(frameUpdate:) userInfo:nil repeats:YES];
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
    
    DB_playItem_t *track = deadbeef->streamer_get_playing_track ();
    
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
        [self statusBar].stringValue = [NSString stringWithUTF8String:sb_text];
    }
    
    if (track) {
        deadbeef->pl_item_unref (track);
    }
}

- (void)advanceSeekBar:(DB_playItem_t *)trk {
    float dur = -1;
    float perc = 0;
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
    }

    if (![_seekBar dragging]) {
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

- (void)frameUpdate:(id)userData
{
    if (![self.window isVisible]) {
        return;
    }

    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();

    [self advanceSeekBar:trk];

    [self updateSonginfo];

    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
}

- (IBAction)seekBarAction:(DdbSeekBar *)sender {
    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();
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

- (IBAction)volumeBarAction:(NSControl *)sender {
    float range = -deadbeef->volume_get_min_db ();
    float volume = [(NSSlider*)sender floatValue] / 100.f * range - range;
    if (volume < -range) {
        volume = -range;
    }
    if (volume > 0) {
        volume = 0;
    }

    deadbeef->volume_set_db (volume);
    int db = (int)volume;
    sender.toolTip = [NSString stringWithFormat:@"%s%ddB", db < 0 ? "" : "+", db];
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
    int idx = deadbeef->plt_get_curr_idx ();
    if (idx != -1) {
        deadbeef->plt_remove (idx);
    }
}

- (void)updateVolumeBar {
    float range = -deadbeef->volume_get_min_db ();
    float vol = (deadbeef->volume_get_db () + range) / range * 100;
    [self volumeBar].floatValue = vol;
}

- (void)freeTitleBarConfig {
    if (_titlebar_playing_script) {
        deadbeef->tf_free (_titlebar_playing_script);
        _titlebar_playing_script = NULL;
    }

    if (_titlebar_stopped_script) {
        deadbeef->tf_free (_titlebar_stopped_script);
        _titlebar_stopped_script = NULL;
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
        .it = deadbeef->streamer_get_playing_track (),
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
        self.window.title = [NSString stringWithUTF8String:titleBuffer];
        self.window.subtitle = [NSString stringWithUTF8String:subtitleBuffer];
    } else
#endif
    {
        self.window.title = [NSString stringWithFormat:@"%s%s%s", subtitleBuffer, (titleBuffer[0] && subtitleBuffer[0]) ? " - " : "", titleBuffer];
    }
}

@end
