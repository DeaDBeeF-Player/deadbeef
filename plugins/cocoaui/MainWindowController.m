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

#import "MainWindowController.h"
#import "PreferencesWindowController.h"
#include "../../deadbeef.h"
#include <sys/time.h>

extern DB_functions_t *deadbeef;


@interface MainWindowController () {
    NSTimer *_updateTimer;
    char *_titlebar_playing_script;
    char *_titlebar_stopped_script;
    char *_statusbar_playing_script;
    int _prevSeekBarPos;
}
@end

@interface NSView (AppKitDetails)
- (void)_addKnownSubview:(NSView *)subview;
@end


@implementation MainWindowController

- (void)dealloc {
    [self cleanup];
}

- (void)cleanup {
    if (_updateTimer) {
        [_updateTimer invalidate];
        _updateTimer = nil;
    }
    [self freeTitleBarConfig];

    [_playlistViewController cleanup];
}

- (void)windowDidLoad {
    [super windowDidLoad];

    _updateTimer = [NSTimer timerWithTimeInterval:1.0f/10.0f target:self selector:@selector(frameUpdate:) userInfo:nil repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:_updateTimer forMode:NSRunLoopCommonModes];
}

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
    
    if (!output || (output->state () == OUTPUT_STATE_STOPPED || !track)) {
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
        [[self statusBar] setStringValue:[NSString stringWithUTF8String:sb_text]];
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
            [_seekBar setFloatValue:perc];
        }
    }

    BOOL st = YES;
    if (!trk || dur < 0) {
        st = NO;
    }
    if ([_seekBar isEnabled] != st) {
        [_seekBar setEnabled:st];
    }
}

- (void)frameUpdate:(id)userData
{
    if (![[self window] isVisible]) {
        return;
    }

    DB_playItem_t *trk = deadbeef->streamer_get_playing_track ();

    [self advanceSeekBar:trk];

    [self updateSonginfo];

    if (trk) {
        deadbeef->pl_item_unref (trk);
    }
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

- (IBAction)volumeBarAction:(id)sender {
    float range = -deadbeef->volume_get_min_db ();
    float volume = [(NSSlider*)sender floatValue] / 100.f * range - range;
    if (volume < -range) {
        volume = -range;
    }
    if (volume > 0) {
        volume = 0;
    }

    deadbeef->volume_set_db (volume);
    int db = volume;
    [sender setToolTip:[NSString stringWithFormat:@"%s%ddB", db < 0 ? "" : "+", db]];
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

- (IBAction)renamePlaylistAction:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_for_idx ([_tabStrip clickedTab]);
    int l = deadbeef->plt_get_title (plt, NULL, 0);
    char buf[l+1];
    deadbeef->plt_get_title (plt, buf, (int)sizeof buf);
    deadbeef->plt_unref (plt);
    [_renamePlaylistTitle setStringValue:[NSString stringWithUTF8String:buf]];
    [NSApp beginSheet:self.renamePlaylistWindow modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndRenamePlaylist:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndRenamePlaylist:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    [sheet orderOut:self];

    if (returnCode == NSOKButton) {
        ddb_playlist_t *plt = deadbeef->plt_get_for_idx ([_tabStrip clickedTab]);
        deadbeef->plt_set_title (plt, [[_renamePlaylistTitle stringValue] UTF8String]);
        deadbeef->plt_save_config (plt);
        deadbeef->plt_unref (plt);
    }
}

- (IBAction)renamePlaylistCancelAction:(id)sender {
    [NSApp endSheet:self.renamePlaylistWindow returnCode:NSCancelButton];
}

- (IBAction)renamePlaylistOKAction:(id)sender {
    [NSApp endSheet:self.renamePlaylistWindow returnCode:NSOKButton];
}

- (void)updateVolumeBar {
    float range = -deadbeef->volume_get_min_db ();
    int vol = (deadbeef->volume_get_db () + range) / range * 100;
    [[self volumeBar] setFloatValue:vol];
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

    deadbeef->conf_get_str ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_STOPPED_VALUE, script, sizeof (script));
    _titlebar_stopped_script = deadbeef->tf_compile (script);

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

    char buffer[200];
    deadbeef->tf_eval (&ctx, ctx.it ? _titlebar_playing_script : _titlebar_stopped_script, buffer, sizeof (buffer));

    if (ctx.it) {
        deadbeef->pl_item_unref (ctx.it);
    }

    [[self window] setTitle:[NSString stringWithUTF8String:buffer]];
}

@end
