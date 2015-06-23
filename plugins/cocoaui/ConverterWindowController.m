//
//  ConverterWindowController.m
//  deadbeef
//
//  Created by waker on 16/06/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import "ConverterWindowController.h"
#include "converter.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface ConverterWindowController () {
    ddb_converter_t *_converter_plugin;
}
@end

@implementation ConverterWindowController

- (NSString *)encoderPresetTitleForPreset:(ddb_encoder_preset_t *)thePreset {
    NSString *title = [NSString stringWithUTF8String:thePreset->title];
    if (thePreset->readonly) {
        title = [@"[Built-in] " stringByAppendingString:title];
    }
    return title;
}

- (void)initializeWidgets {
    [_encoderPreset removeAllItems];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_list ();
    while (p) {
        [_encoderPreset addItemWithTitle:[self encoderPresetTitleForPreset:p]];
        p = p->next;
    }
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
    _converter_plugin = (ddb_converter_t *)deadbeef->plug_get_for_id ("converter");
    [_encoderPresetsTableView setDataSource:(id<NSTableViewDataSource>)self];
    [self initializeWidgets];
}

- (void)run {
    if (_converter_plugin) {
        [self initializeWidgets];
    }
    [self showWindow:self];
    [[self window] makeKeyWindow];
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_list ();
    int cnt = 0;
    while (p) {
        cnt++;
        p = p->next;
    }
    return cnt;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_list ();
    int i = 0;
    while (p && i < rowIndex) {
        i++;
        p = p->next;
    }
    return [self encoderPresetTitleForPreset:p];
}

- (IBAction)cancelAction:(id)sender {
    [[self window] close];
}

- (IBAction)okAction:(id)sender {
}

- (IBAction)openOutputFolderAction:(id)sender {
}

- (IBAction)editEncoderPresetsAction:(id)sender {
    [NSApp beginSheet:_encoderPresetsPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndEncoderPresetList:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndEncoderPresetList:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_encoderPresetsPanel orderOut:self];
}

- (IBAction)editDSPPresetsAction:(id)sender {
    [NSApp beginSheet:_dspPresetsPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndDSPPresetList:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndDSPPresetList:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_dspPresetsPanel orderOut:self];
}

- (IBAction)closeEncoderPresetsAction:(id)sender {
    [NSApp endSheet:_encoderPresetsPanel returnCode:NSOKButton];
}

- (IBAction)closeDSPPresetsAction:(id)sender {
    [NSApp endSheet:_dspPresetsPanel returnCode:NSOKButton];
}

@end
