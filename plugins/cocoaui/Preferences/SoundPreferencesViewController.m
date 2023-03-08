//
//  SoundPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/25/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "SoundPreferencesViewController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface SoundPreferencesViewController ()

@property NSMutableArray<NSString *> *audioDevices;

@property (weak) IBOutlet NSPopUpButton *outputPluginsPopupButton;
@property (weak) IBOutlet NSPopUpButton *audioDevicesPopupButton;

@property (nonatomic) NSUInteger outputPluginsIndex;
@property (nonatomic) NSUInteger audioDevicesIndex;
@property (nonatomic) BOOL audioDevicesEnabled;
@property (nonatomic) BOOL overrideSamplerate;
@property (nonatomic) NSString *targetSamplerate;
@property (nonatomic) BOOL basedOnInputSamplerate;
@property (nonatomic) NSString *samplerateForMultiplesOf48;
@property (nonatomic) NSString *samplerateForMultiplesOf44;

@end

@implementation SoundPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    NSInteger index = 0;

    char curplug[200];
    deadbeef->conf_get_str ("output_plugin", "coreaudio", curplug, sizeof (curplug));
    DB_output_t **o = deadbeef->plug_get_output_list ();
    for (index = 0; o[index]; index++) {
        if (!strcmp (o[index]->plugin.id, curplug)) {
            _outputPluginsIndex = index;
            break;
        }
    }


    self.overrideSamplerate = deadbeef->conf_get_int ("streamer.override_samplerate", 0) ? YES : NO;
    self.targetSamplerate = @(deadbeef->conf_get_int ("streamer.samplerate", 44100)).stringValue;

    self.basedOnInputSamplerate = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0) ? YES : NO;

    self.samplerateForMultiplesOf48 = @(deadbeef->conf_get_int ("streamer.samplerate_mult_48", 48000)).stringValue;
    self.samplerateForMultiplesOf44 = @(deadbeef->conf_get_int ("streamer.samplerate_mult_44", 44100)).stringValue;

    [self initializeAudioTab];
}

- (void)outputDeviceChanged {
    [self initAudioDeviceList];
}

static void
ca_enum_callback (const char *s, const char *d, void *userdata) {
    NSMutableArray<NSString *> *devices = (__bridge NSMutableArray<NSString *> *)userdata;

    [devices addObject:[NSString stringWithUTF8String:s]];
}

- (void)initAudioDeviceList {
    DB_output_t *output = deadbeef->get_output ();

    self.audioDevices = [NSMutableArray new];
    [self.audioDevices addObject:@"System Default"];
    _audioDevicesEnabled = output->enum_soundcards ? YES : NO;

    if (output->enum_soundcards) {
        output->enum_soundcards (ca_enum_callback, (__bridge void *)(self.audioDevices));
    }

    NSString *conf_name = [[NSString stringWithUTF8String:output->plugin.id] stringByAppendingString:@"_soundcard"];
    char curdev[200];
    deadbeef->conf_get_str ([conf_name UTF8String], "", curdev, sizeof (curdev));
    _audioDevicesIndex = 0;
    NSUInteger index = 0;
    [self.audioDevicesPopupButton removeAllItems];
    for (NSString *dev in self.audioDevices) {
        if (!strcmp ([dev UTF8String], curdev)) {
            _audioDevicesIndex = index;
        }
        index++;
        [self.audioDevicesPopupButton addItemWithTitle:dev];
    }
    [self.audioDevicesPopupButton selectItemAtIndex:_audioDevicesIndex];
}

- (void)initializeAudioTab {
    // output plugins

    NSInteger index = 0;
    [self.outputPluginsPopupButton removeAllItems];

    char curplug[200];
    deadbeef->conf_get_str ("output_plugin", "coreaudio", curplug, sizeof (curplug));
    DB_output_t **o = deadbeef->plug_get_output_list ();
    for (index = 0; o[index]; index++) {
        [self.outputPluginsPopupButton addItemWithTitle:[NSString stringWithUTF8String:o[index]->plugin.name]];
    }

    // audio devices
    [self initAudioDeviceList];
}

- (void)setOutputPluginsIndex:(NSUInteger)outputPluginsIndex {
    _outputPluginsIndex = outputPluginsIndex;
    DB_output_t **o = deadbeef->plug_get_output_list ();
    deadbeef->conf_set_str ("output_plugin", o[outputPluginsIndex]->plugin.id);
    deadbeef->sendmessage(DB_EV_REINIT_SOUND, 0, 0, 0);
}

- (IBAction)outputDeviceAction:(NSPopUpButton *)sender {
    self.audioDevicesIndex = sender.indexOfSelectedItem;
    NSString *title = self.audioDevices[self.audioDevicesIndex];
    DB_output_t *output = deadbeef->get_output ();
    NSString *dev = [[NSString stringWithUTF8String:output->plugin.id] stringByAppendingString:@"_soundcard"];
    deadbeef->conf_set_str (dev.UTF8String, title.UTF8String);
    deadbeef->sendmessage(DB_EV_REINIT_SOUND, 0, 0, 0);
}

- (void)setOverrideSamplerate:(BOOL)overrideSamplerate {
    _overrideSamplerate = overrideSamplerate;
    deadbeef->conf_set_int ("streamer.override_samplerate", overrideSamplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

static NSUInteger
clamp_samplerate (NSUInteger val) {
    if (val < 8000) {
        return 8000;
    }
    else if (val > 768000) {
        return 768000;
    }
    return val;
}

- (void)setTargetSamplerate:(NSString *)targetSamplerate {
    NSUInteger samplerate = clamp_samplerate (atoi(targetSamplerate.UTF8String));
    _targetSamplerate = @(samplerate).stringValue;
    deadbeef->conf_set_int ("streamer.samplerate", (int)samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setBasedOnInputSamplerate:(BOOL)basedOnInputSamplerate {
    _basedOnInputSamplerate = basedOnInputSamplerate;
    deadbeef->conf_set_int ("streamer.use_dependent_samplerate", basedOnInputSamplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setSamplerateForMultiplesOf48:(NSString *)samplerateForMultiplesOf48 {
    NSUInteger samplerate = clamp_samplerate (atoi(samplerateForMultiplesOf48.UTF8String));
    _samplerateForMultiplesOf48 = @(samplerate).stringValue;
    deadbeef->conf_set_int ("streamer.samplerate_mult_48", (int)samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (void)setSamplerateForMultiplesOf44:(NSString *)samplerateForMultiplesOf44 {
    NSUInteger samplerate = clamp_samplerate (atoi(samplerateForMultiplesOf44.UTF8String));
    _samplerateForMultiplesOf44 = @(samplerate).stringValue;
    deadbeef->conf_set_int ("streamer.samplerate_mult_44", (int)samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}



@end
