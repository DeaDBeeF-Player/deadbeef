//
//  SoundPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "SoundPreferencesViewController.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface SoundPreferencesViewController ()

@property (weak) IBOutlet NSPopUpButton *outputPluginsPopupButton;

@property NSMutableArray<NSString *> *audioDevices;
@property (weak) IBOutlet NSPopUpButton *audioDevicesPopupButton;
@property (weak) IBOutlet NSTextField *targetSamplerateLabel;
@property (weak) IBOutlet NSTextField *multiplesOf48Label;
@property (weak) IBOutlet NSTextField *multiplesOf44Label;

@property (weak) IBOutlet NSButton *overrideSamplerateCheckbox;
@property (weak) IBOutlet NSComboBox *targetSamplerateComboBox;
@property (weak) IBOutlet NSButton *basedOnInputSamplerateCheckbox;
@property (weak) IBOutlet NSComboBox *multiplesOf48ComboBox;
@property (weak) IBOutlet NSComboBox *multiplesOf44ComboBox;

@end

@implementation SoundPreferencesViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
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
    [self.audioDevicesPopupButton removeAllItems];

    self.audioDevices = [NSMutableArray new];
    DB_output_t *output = deadbeef->get_output ();
    if (!output->enum_soundcards) {
        self.audioDevicesPopupButton.enabled = NO;
        return;
    }

    self.audioDevicesPopupButton.enabled = YES;

    output->enum_soundcards (ca_enum_callback, (__bridge void *)(self.audioDevices));

    NSString *conf_name = [[NSString stringWithUTF8String:output->plugin.id] stringByAppendingString:@"_soundcard"];
    char curdev[200];
    deadbeef->conf_get_str ([conf_name UTF8String], "", curdev, sizeof (curdev));
    [self.audioDevicesPopupButton removeAllItems];
    [self.audioDevicesPopupButton addItemWithTitle:@"System Default"];
    [self.audioDevicesPopupButton selectItemAtIndex:0];
    NSInteger index = 1;
    for (NSString *dev in self.audioDevices) {
        [self.audioDevicesPopupButton addItemWithTitle:dev];
        if (!strcmp ([dev UTF8String], curdev)) {
            [self.audioDevicesPopupButton selectItemAtIndex:index];
        }
        index++;
    }
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
        if (!strcmp (o[index]->plugin.id, curplug)) {
            [self.outputPluginsPopupButton selectItemAtIndex:index];
        }
    }

    // audio devices
    [self initAudioDeviceList];

    self.overrideSamplerateCheckbox.state = deadbeef->conf_get_int ("streamer.override_samplerate", 0) ? NSControlStateValueOn : NSControlStateValueOff;
    self.targetSamplerateComboBox.stringValue = conf_get_nsstr ("streamer.samplerate", "44100");

    self.basedOnInputSamplerateCheckbox.state = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0) ? NSControlStateValueOn : NSControlStateValueOff;
    self.multiplesOf48ComboBox.stringValue = conf_get_nsstr ("streamer.samplerate_mult_48", "48000");
    self.multiplesOf44ComboBox.stringValue = conf_get_nsstr ("streamer.samplerate_mult_44", "44100");
    [self validateAudioSettingsViews];
}

- (void)validateAudioSettingsViews {
    BOOL override = deadbeef->conf_get_int ("streamer.override_samplerate", 0) ? YES : NO;
    BOOL useDependent = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0) ? YES : NO;

    self.targetSamplerateLabel.enabled = override;
    self.targetSamplerateComboBox.enabled = override;

    self.basedOnInputSamplerateCheckbox.enabled = override;

    self.multiplesOf48Label.enabled = override && useDependent;
    self.multiplesOf48ComboBox.enabled = override && useDependent;

    self.multiplesOf44Label.enabled = override && useDependent;
    self.multiplesOf44ComboBox.enabled = override && useDependent;
}

- (IBAction)outputPluginAction:(id)sender {
    DB_output_t **o = deadbeef->plug_get_output_list ();
    deadbeef->conf_set_str ("output_plugin", o[[self.outputPluginsPopupButton indexOfSelectedItem]]->plugin.id);
    deadbeef->sendmessage(DB_EV_REINIT_SOUND, 0, 0, 0);
}

- (IBAction)playbackDeviceAction:(NSPopUpButton *)sender {
    NSString *title = [[sender selectedItem] title];
    DB_output_t *output = deadbeef->get_output ();
    NSString *dev = [[NSString stringWithUTF8String:output->plugin.id] stringByAppendingString:@"_soundcard"];
    deadbeef->conf_set_str ([dev UTF8String], [title UTF8String]);
    deadbeef->sendmessage(DB_EV_REINIT_SOUND, 0, 0, 0);
}

- (IBAction)overrideSamplerateAction:(NSButton *)sender {
    deadbeef->conf_set_int ("streamer.override_samplerate", sender.state == NSControlStateValueOn ? 1 : 0);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    [self validateAudioSettingsViews];
}

static int
clamp_samplerate (int val) {
    if (val < 8000) {
        return 8000;
    }
    else if (val > 768000) {
        return 768000;
    }
    return val;
}

- (IBAction)targetSamplerateAction:(NSComboBox *)sender {
    int samplerate = clamp_samplerate ((int)[sender integerValue]);
    deadbeef->conf_set_int ("streamer.samplerate", samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)basedOnInputSamplerateAction:(NSButton *)sender {
    deadbeef->conf_set_int ("streamer.use_dependent_samplerate", sender.state == NSControlStateValueOn ? 1 : 0);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    [self validateAudioSettingsViews];
}

- (IBAction)multiplesOf48Action:(NSComboBox *)sender {
    int samplerate = clamp_samplerate ((int)[sender integerValue]);
    deadbeef->conf_set_int ("streamer.samplerate_mult_48", samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)multiplesOf44Action:(NSComboBox *)sender {
    int samplerate = clamp_samplerate ((int)[sender integerValue]);
    deadbeef->conf_set_int ("streamer.samplerate_mult_44", samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}



@end
