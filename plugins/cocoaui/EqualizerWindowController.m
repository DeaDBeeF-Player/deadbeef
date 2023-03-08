//
//  EqualizerWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 1/29/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "EqualizerWindowController.h"
#import "PropertySheetViewController.h"
#import "PropertySheetContentView.h"
#include <deadbeef/deadbeef.h>
#include "eqpreset.h"

extern DB_functions_t *deadbeef;

@interface EQPropertySheetDataSource : NSObject<PropertySheetDataSource>

@property (nonatomic,unsafe_unretained,readonly) ddb_dsp_context_t *supereq;

@end

@implementation EQPropertySheetDataSource

- (NSString *)propertySheet:(PropertySheetViewController *)vc configForItem:(id)item {
    return @"property \"\" hbox[19] hmg fill expand border=0 spacing=8 height=200 width=600 noclip itemwidth=30;\n"
    "property \"Preamp\" vscale[20,-20,0.5] vert 0 0;\n"
    "property \"55\" vscale[20,-20,0.5] vert 1 0;\n"
    "property \"77\" vscale[20,-20,0.5] vert 2 0;\n"
    "property \"110\" vscale[20,-20,0.5] vert 3 0;\n"
    "property \"156\" vscale[20,-20,0.5] vert 4 0;\n"
    "property \"220\" vscale[20,-20,0.5] vert 5 0;\n"
    "property \"311\" vscale[20,-20,0.5] vert 6 0;\n"
    "property \"440\" vscale[20,-20,0.5] vert 7 0;\n"
    "property \"622\" vscale[20,-20,0.5] vert 8 0;\n"
    "property \"880\" vscale[20,-20,0.5] vert 9 0;\n"
    "property \"1.2K\" vscale[20,-20,0.5] vert 10 0;\n"
    "property \"1.8K\" vscale[20,-20,0.5] vert 11 0;\n"
    "property \"2.5K\" vscale[20,-20,0.5] vert 12 0;\n"
    "property \"3.5K\" vscale[20,-20,0.5] vert 13 0;\n"
    "property \"5K\" vscale[20,-20,0.5] vert 14 0;\n"
    "property \"7K\" vscale[20,-20,0.5] vert 15 0;\n"
    "property \"10K\" vscale[20,-20,0.5] vert 16 0;\n"
    "property \"14K\" vscale[20,-20,0.5] vert 17 0;\n"
    "property \"20K\" vscale[20,-20,0.5] vert 18 0;\n";
}

- (ddb_dsp_context_t *)supereq {
    ddb_dsp_context_t *dsp = deadbeef->streamer_get_dsp_chain ();
    while (dsp) {
        if (!strcmp (dsp->plugin->plugin.id, "supereq")) {
            return dsp;
        }
        dsp = dsp->next;
    }

    return NULL;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc valueForKey:(NSString *)key def:(NSString *)def item:(id)item {
    ddb_dsp_context_t *eq = self.supereq;
    if (!eq) {
        return def;
    }

    char str[200];
    eq->plugin->get_param (eq, key.intValue, str, sizeof (str));
    return [NSString stringWithUTF8String:str];
}

- (void)propertySheet:(PropertySheetViewController *)vc setValue:(NSString *)value forKey:(NSString *)key item:(id)item {
    ddb_dsp_context_t *eq = self.supereq;
    if (!eq) {
        return;
    }

    eq->plugin->set_param (eq, key.intValue, value.UTF8String);
    deadbeef->streamer_dsp_chain_save ();
}

- (void)propertySheetBeginChanges {
}

- (void)propertySheetCommitChanges {
}

@end

#pragma mark -

@interface EqualizerWindowController ()

@property (strong) IBOutlet PropertySheetViewController *propertySheetViewController;

@property (nonatomic) EQPropertySheetDataSource *propertySheetDataSource;

@property (nonatomic) BOOL enabled;
@property (strong) IBOutlet NSMenu *presetsMenu;

@end

@implementation EqualizerWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    self.propertySheetViewController.labelFontSize = 10;
    self.propertySheetViewController.contentFontSize = 11;
    self.propertySheetViewController.unitSpacing = 4;
    self.propertySheetViewController.autoAlignLabels = NO;
    self.propertySheetViewController.labelFixedWidth = 50;
    self.propertySheetDataSource = [EQPropertySheetDataSource new];
    self.propertySheetViewController.dataSource = self.propertySheetDataSource;

    [self willChangeValueForKey:@"enabled"];
    [self didChangeValueForKey:@"enabled"];
}

- (void)setEnabled:(BOOL)enabled {
    ddb_dsp_context_t *eq = self.propertySheetDataSource.supereq;
    if (!eq) {
        return;
    }

    eq->enabled = enabled;
    deadbeef->streamer_dsp_chain_save();

    // a horrible hack to notify the preferences window about the change in the dsp chain
    [NSNotificationCenter.defaultCenter postNotificationName:@"DSPChainDidChange" object:nil];
}

- (BOOL)enabled {
    ddb_dsp_context_t *eq = self.propertySheetDataSource.supereq;
    if (!eq) {
        return NO;
    }

    return eq->enabled;
}

- (void)applyPresetWithPreamp:(float)preamp bands:(float[18])bands {
    // apply and save config
    ddb_dsp_context_t *eq = self.propertySheetDataSource.supereq;
    if (!eq) {
        return;
    }

    char str[50];
    snprintf (str, sizeof(str), "%f", preamp);
    eq->plugin->set_param(eq, 0, str);

    for (int i = 0; i < 18; i++) {
        snprintf (str, sizeof(str), "%f", bands[i]);
        eq->plugin->set_param(eq, i+1, str);
    }
    [self.propertySheetViewController reload];
    deadbeef->streamer_dsp_chain_save ();
    [NSNotificationCenter.defaultCenter postNotificationName:@"DSPChainDidChange" object:nil];
}

#pragma mark - Button actions

- (IBAction)zeroAllAction:(id)sender {
    for (int i = 0; i < 19; i++) {
        [self.propertySheetDataSource propertySheet:self.propertySheetViewController setValue:@"0" forKey:[NSString stringWithFormat:@"%d", i] item:nil];
    }
    [self.propertySheetViewController reload];
    deadbeef->streamer_dsp_chain_save ();
    [NSNotificationCenter.defaultCenter postNotificationName:@"DSPChainDidChange" object:nil];
}

- (IBAction)zeroPreampAction:(id)sender {
    [self.propertySheetDataSource propertySheet:self.propertySheetViewController setValue:@"0" forKey:@"0" item:nil];
    [self.propertySheetViewController reload];
    deadbeef->streamer_dsp_chain_save ();
    [NSNotificationCenter.defaultCenter postNotificationName:@"DSPChainDidChange" object:nil];
}

- (IBAction)zeroBandsAction:(id)sender {
    for (int i = 1; i < 19; i++) {
        [self.propertySheetDataSource propertySheet:self.propertySheetViewController setValue:@"0" forKey:[NSString stringWithFormat:@"%d", i] item:nil];
    }
    [self.propertySheetViewController reload];
    deadbeef->streamer_dsp_chain_save ();
    [NSNotificationCenter.defaultCenter postNotificationName:@"DSPChainDidChange" object:nil];
}


- (IBAction)presetsButtonAction:(id)sender {
    [NSMenu popUpContextMenu:self.presetsMenu withEvent:NSApplication.sharedApplication.currentEvent forView:sender];
}

- (IBAction)loadPresetAction:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories = NO;
    panel.allowedFileTypes = @[@"ddbeq"];
    if ([panel runModal] == NSModalResponseOK)
    {
        NSString *fileName = panel.URLs.firstObject.path;
        float preamp;
        float bands[18];
        if (!eq_preset_load(fileName.UTF8String, &preamp, bands)) {
            [self applyPresetWithPreamp:preamp bands:bands];
        }
    }
}

- (IBAction)savePresetAction:(id)sender {
    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.title = @"Save Playlist";
    panel.canCreateDirectories = YES;
    panel.extensionHidden = NO;
    panel.allowedFileTypes = @[@"ddbeq"];
    panel.allowsOtherFileTypes = NO;

    if ([panel runModal] == NSModalResponseOK) {
        NSString *fname = panel.URL.path;
        eq_preset_save (fname.UTF8String);
    }
}

- (IBAction)importFoobar2000PresetAction:(id)sender {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.allowsMultipleSelection = NO;
    panel.canChooseDirectories = NO;
    panel.allowedFileTypes = @[@"feq"];
    if ([panel runModal] == NSModalResponseOK)
    {
        NSString *fileName = panel.URLs.firstObject.path;
        float bands[18];
        if (!eq_preset_load_fb2k (fileName.UTF8String, bands)) {
            [self applyPresetWithPreamp:0 bands:bands];
        }
    }
}

@end
