//
//  PlaybackPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/23/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "PlaybackPreferencesViewController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface PlaybackPreferencesViewController ()

// ReplayGain
@property (nonatomic) ddb_rg_source_mode_t rgSourceMode;
@property (nonatomic) NSUInteger rgProcessingIdx;

@property (nonatomic) float rgPreampWithRg;
@property (nonatomic) float rgPreampWithoutRg;
@property (nonatomic) NSString *rgPreampWithRgLabel;
@property (nonatomic) NSString *rgPreampWithoutRgLabel;

// Other settings
@property (nonatomic) BOOL cliAddToSpecificPlaylist;
@property (nonatomic) NSString *cliSpecificPlaylist;
@property (nonatomic) BOOL resumeLastSession;
@property (nonatomic) BOOL ignoreArchives;
@property (nonatomic) BOOL stopAfterCurrentReset;
@property (nonatomic) BOOL stopAfterCurrentAlbumReset;

@property (weak) IBOutlet NSSlider *visBufferSlider;
@property (weak) IBOutlet NSTextField *visBufferValue;

@end

@implementation PlaybackPreferencesViewController

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    // ReplayGain
    _rgSourceMode = deadbeef->conf_get_int ("replaygain.source_mode", 0);

    NSUInteger processing_idx = 0;
    ddb_rg_processing_t processing_flags = deadbeef->conf_get_int ("replaygain.processing_flags", 0);
    if (processing_flags == DDB_RG_PROCESSING_GAIN) {
        processing_idx = 1;
    }
    else if (processing_flags == (DDB_RG_PROCESSING_GAIN|DDB_RG_PROCESSING_PREVENT_CLIPPING)) {
        processing_idx = 2;
    }
    else if (processing_flags == DDB_RG_PROCESSING_PREVENT_CLIPPING) {
        processing_idx = 3;
    }

    _rgProcessingIdx = processing_idx;

    _rgPreampWithRg = deadbeef->conf_get_float ("replaygain.preamp_with_rg", 0);
    _rgPreampWithoutRg = deadbeef->conf_get_float ("replaygain.preamp_without_rg", 0);

    [self updateRGLabels];

    // Other settings
    // playback
    _cliAddToSpecificPlaylist =  deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1) ? YES : NO;
    _cliSpecificPlaylist = conf_get_nsstr ("cli_add_playlist_name", "Default");
    _resumeLastSession = deadbeef->conf_get_int ("resume_last_session", 1) ? YES : NO;
    _ignoreArchives = deadbeef->conf_get_int ("ignore_archives", 1) ? YES : NO;
    _stopAfterCurrentReset = deadbeef->conf_get_int ("playlist.stop_after_current_reset", 0) ? YES : NO;
    _stopAfterCurrentAlbumReset = deadbeef->conf_get_int ("playlist.stop_after_album_reset", 0) ? YES : NO;

    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    int value = deadbeef->conf_get_int ("playback.vis_buffer", 1000);
    self.visBufferSlider.intValue = value;
    self.visBufferValue.stringValue = [NSString stringWithFormat:@"%d ms", value];
}

#pragma mark - ReplayGain

- (void)updateRGLabels {
    float value = _rgPreampWithRg;
    self.rgPreampWithRgLabel = [NSString stringWithFormat:@"%s%0.2fdB", value >= 0 ? "+" : "", value];

    value = _rgPreampWithoutRg;
    self.rgPreampWithoutRgLabel = [NSString stringWithFormat:@"%s%0.2fdB", value >= 0 ? "+" : "", value];
}


- (void)setRgPreampWithRg:(float)rgPreampWithRg {
    _rgPreampWithRg = rgPreampWithRg;
    deadbeef->conf_set_float ("replaygain.preamp_with_rg", rgPreampWithRg);
    [self updateRGLabels];
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setRgPreampWithoutRg:(float)rgPreampWithoutRg {
    _rgPreampWithoutRg = rgPreampWithoutRg;
    deadbeef->conf_set_float ("replaygain.preamp_without_rg", rgPreampWithoutRg);
    [self updateRGLabels];
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setRgSourceMode:(ddb_rg_source_mode_t)rgSourceMode {
    _rgSourceMode = rgSourceMode;
    deadbeef->conf_set_int ("replaygain.source_mode", rgSourceMode);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setRgProcessingIdx:(NSUInteger)idx {
    _rgProcessingIdx = idx;
    uint32_t flags = 0;
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

#pragma mark - Other settings

- (void)setCliAddToSpecificPlaylist:(BOOL)cliAddToSpecificPlaylist {
    _cliAddToSpecificPlaylist = cliAddToSpecificPlaylist;
    deadbeef->conf_set_int ("cli_add_to_specific_playlist", cliAddToSpecificPlaylist);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setCliSpecificPlaylist:(NSString *)cliSpecificPlaylist {
    _cliSpecificPlaylist = cliSpecificPlaylist;
    conf_set_nsstr("cli_add_playlist_name", cliSpecificPlaylist);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setIgnoreArchives:(BOOL)ignoreArchives {
    _ignoreArchives = ignoreArchives;
    deadbeef->conf_set_int ("ignore_archives", ignoreArchives);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setResumeLastSession:(BOOL)resumeLastSession {
    _resumeLastSession = resumeLastSession;
    deadbeef->conf_set_int ("resume_last_session", resumeLastSession);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setStopAfterCurrentReset:(BOOL)stopAfterCurrentReset {
    _stopAfterCurrentReset = stopAfterCurrentReset;
    deadbeef->conf_set_int ("playlist.stop_after_current_reset", stopAfterCurrentReset);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setStopAfterCurrentAlbumReset:(BOOL)stopAfterCurrentAlbumReset {
    _stopAfterCurrentAlbumReset = stopAfterCurrentAlbumReset;
    deadbeef->conf_set_int ("playlist.stop_after_album_reset", stopAfterCurrentAlbumReset);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)visBufferSliderAction:(NSSlider *)sender {
    self.visBufferValue.stringValue = [NSString stringWithFormat:@"%d ms", sender.intValue];
    deadbeef->conf_set_int ("playback.vis_buffer", sender.intValue);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}


@end
