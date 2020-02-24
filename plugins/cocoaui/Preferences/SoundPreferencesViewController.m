//
//  SoundPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/23/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "SoundPreferencesViewController.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface SoundPreferencesViewController ()

// ReplayGain
@property (unsafe_unretained) IBOutlet NSPopUpButton *replaygain_source_mode;
@property (unsafe_unretained) IBOutlet NSPopUpButton *replaygain_processing;
@property (unsafe_unretained) IBOutlet NSSlider *replaygain_preamp_with_rg;
@property (unsafe_unretained) IBOutlet NSSlider *replaygain_preamp_without_rg;
@property (unsafe_unretained) IBOutlet NSTextField *replaygain_preamp_with_rg_label;
@property (unsafe_unretained) IBOutlet NSTextField *replaygain_preamp_without_rg_label;

@property (unsafe_unretained) IBOutlet NSButton *cli_add_to_specific_playlist;
@property (unsafe_unretained) IBOutlet NSTextField *cli_add_playlist_name;
@property (unsafe_unretained) IBOutlet NSButton *resume_last_session;
@property (unsafe_unretained) IBOutlet NSButton *ignore_archives;
@property (unsafe_unretained) IBOutlet NSButton *stop_after_current_reset;
@property (unsafe_unretained) IBOutlet NSButton *stop_after_album_reset;

@property (nonatomic) NSString *cliSpecificPlaylist;

@end

@implementation SoundPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // playback
    [_replaygain_source_mode selectItemAtIndex: deadbeef->conf_get_int ("replaygain.source_mode", 0)];

    int processing_idx = 0;
    int processing_flags = deadbeef->conf_get_int ("replaygain.processing_flags", 0);
    if (processing_flags == DDB_RG_PROCESSING_GAIN) {
        processing_idx = 1;
    }
    else if (processing_flags == (DDB_RG_PROCESSING_GAIN|DDB_RG_PROCESSING_PREVENT_CLIPPING)) {
        processing_idx = 2;
    }
    else if (processing_flags == DDB_RG_PROCESSING_PREVENT_CLIPPING) {
        processing_idx = 3;
    }

    [self.replaygain_processing selectItemAtIndex:processing_idx];
    self.replaygain_preamp_with_rg.floatValue = deadbeef->conf_get_float ("replaygain.preamp_with_rg", 0);
    self.replaygain_preamp_without_rg.floatValue = deadbeef->conf_get_float ("replaygain.preamp_without_rg", 0);
    [self updateRGLabels];

    _cli_add_to_specific_playlist.state =  deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1) ? NSOnState : NSOffState;
    _cli_add_playlist_name.stringValue =  conf_get_nsstr ("cli_add_playlist_name", "Default");
    _resume_last_session.state =  deadbeef->conf_get_int ("resume_last_session", 1) ? NSOnState : NSOffState;
    _ignore_archives.state =  deadbeef->conf_get_int ("ignore_archives", 1) ? NSOnState : NSOffState;
    _stop_after_current_reset.state =  deadbeef->conf_get_int ("playlist.stop_after_current_reset", 0) ? NSOnState : NSOffState;
    _stop_after_album_reset.state =  deadbeef->conf_get_int ("playlist.stop_after_album_reset", 0) ? NSOnState : NSOffState;

    _cliSpecificPlaylist = conf_get_nsstr("cli_add_playlist_name", "Default");
    [self willChangeValueForKey:@"cliSpecificPlaylist"];
    [self didChangeValueForKey:@"cliSpecificPlaylist"];
}

- (void)setCliSpecificPlaylist:(NSString *)cliSpecificPlaylist {
    _cliSpecificPlaylist = cliSpecificPlaylist;
    conf_set_nsstr("cli_add_playlist_name", cliSpecificPlaylist);
}

- (IBAction)ignoreArchivesAction:(id)sender {
    deadbeef->conf_set_int ("ignore_archives", _ignore_archives.state == NSOnState);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)cliAddToSpecificPlaylistAction:(id)sender {
    deadbeef->conf_set_int ("cli_add_to_specific_playlist", _cli_add_to_specific_playlist.state == NSOnState);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)resumeLastSessionAction:(NSButton *)sender {
    deadbeef->conf_set_int ("resume_last_session", sender.state == NSOnState);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)stopAfterCurrentResetAction:(id)sender {
    deadbeef->conf_set_int ("playlist.stop_after_current_reset", _stop_after_current_reset.state == NSOnState);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)stopAfterCurrentAlbumResetAction:(id)sender {
    deadbeef->conf_set_int ("playlist.stop_after_album_reset", _stop_after_album_reset.state == NSOnState);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

#pragma mark - ReplayGain

- (void)updateRGLabels {
    float value = [_replaygain_preamp_with_rg floatValue];
    _replaygain_preamp_with_rg_label.stringValue = [NSString stringWithFormat:@"%s%0.2fdB", value >= 0 ? "+" : "", value];
    value = [_replaygain_preamp_without_rg floatValue];
    _replaygain_preamp_without_rg_label.stringValue = [NSString stringWithFormat:@"%s%0.2fdB", value >= 0 ? "+" : "", value];
}

- (IBAction)replaygain_preamp_with_rg_action:(id)sender {
    float value = [sender floatValue];
    deadbeef->conf_set_float ("replaygain.preamp_with_rg", value);
    [self updateRGLabels];
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)replaygain_preamp_without_rg_action:(id)sender {
    float value = [sender floatValue];
    deadbeef->conf_set_float ("replaygain.preamp_without_rg", value);
    [self updateRGLabels];
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)replaygain_source_mode_action:(id)sender {
    NSInteger idx = [_replaygain_source_mode indexOfSelectedItem];
    deadbeef->conf_set_int ("replaygain.source_mode", (int)idx);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)replaygain_processing_action:(id)sender {
    uint32_t flags = 0;
    NSInteger idx = [_replaygain_processing indexOfSelectedItem];
    if (idx == 1) {
        flags = DDB_RG_PROCESSING_GAIN;
    }
    if (idx == 2) {
        flags = DDB_RG_PROCESSING_GAIN | DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }
    if (idx == 3) {
        flags = DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }

    deadbeef->conf_set_int ("replaygain.processing_flags", flags);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
