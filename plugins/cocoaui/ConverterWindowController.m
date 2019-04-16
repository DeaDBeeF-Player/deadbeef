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

#import "ConverterWindowController.h"
#include "converter.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;
static NSString *default_format = @"[%tracknumber%. ][%artist% - ]%title%";

@interface ConverterWindowController () {
    ddb_converter_t *_converter_plugin;

    NSCondition *_overwritePromptCondition;

    // conversion context
    DB_playItem_t **_convert_items;
    ddb_playlist_t *_convert_playlist;
    int _convert_items_count;
    NSString *_outfolder;
    NSString *_outfile;
    int _preserve_folder_structure;
    int _write_to_source_folder;
    int _output_bps;
    int _output_is_float;
    int _overwrite_action;
    ddb_encoder_preset_t *_encoder_preset;
    ddb_dsp_preset_t *_dsp_preset;
    int _cancelled;
    NSInteger _overwritePromptResult;
    BOOL _working;
    NSInteger _bypassSameFormatState;
    NSInteger _retagAfterCopyState;
}
@end

static NSMutableArray *g_converterControllers;

@implementation ConverterWindowController

- (void)dealloc {
    [self reset];
}

- (NSString *)encoderPresetTitleForPreset:(ddb_encoder_preset_t *)thePreset {
    NSString *title = [NSString stringWithUTF8String:thePreset->title];
    if (thePreset->readonly) {
        title = [@"[Built-in] " stringByAppendingString:title];
    }
    return title;
}

- (void)initializeWidgets {
    deadbeef->conf_lock ();
    const char *out_folder = deadbeef->conf_get_str_fast ("converter.output_folder", "");
    if (!out_folder[0]) {
        out_folder = getenv("HOME");
    }

    [_outputFolder setStringValue:[NSString stringWithUTF8String:out_folder]];
    [_outputFileName setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("converter.output_file", "")]];
    [_preserveFolderStructure setState:deadbeef->conf_get_int ("converter.preserve_folder_structure", 0) ? NSOnState : NSOffState];
    [_bypassSameFormat setState:deadbeef->conf_get_int ("converter.bypass_same_format", 0)];
    [_retagAfterCopy setState:deadbeef->conf_get_int ("converter.retag_after_copy", 0)];

    int write_to_source_folder = deadbeef->conf_get_int ("converter.write_to_source_folder", 0);
    [_writeToSourceFolder setState:write_to_source_folder ? NSOnState : NSOffState];
    [_outputFolder setEnabled:!write_to_source_folder];
    [_preserveFolderStructure setEnabled:!write_to_source_folder];

    [_fileExistsAction selectItemAtIndex:deadbeef->conf_get_int ("converter.overwrite_action", 0)];
    deadbeef->conf_unlock ();

    // fill encoder presets
    [self fillEncoderPresets];

    // TODO: fill dsp presets
    [_dspPreset addItemWithTitle:@"Pass through"];
    [_dspPreset selectItemAtIndex:deadbeef->conf_get_int ("converter.dsp_preset", -1) + 1];

    [_outputFormat selectItemAtIndex:deadbeef->conf_get_int ("converter.output_format", 0)];
    [_fileExistsAction selectItemAtIndex:deadbeef->conf_get_int ("converter.overwrite_action", 0)];
}

-(void)fillEncoderPresets {
    [_encoderPreset removeAllItems];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_list ();
    while (p) {
        [_encoderPreset addItemWithTitle:[self encoderPresetTitleForPreset:p]];
        p = p->next;
    }
    [_encoderPreset selectItemAtIndex:deadbeef->conf_get_int ("converter.encoder_preset", 0)];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    if (textField == _outputFolder) {
        [self outputFolderChanged:self];
    }
    else if (textField == _outputFileName) {
        [self outputPathChanged:self];
    }
}

- (IBAction)outputFolderChanged:(id)sender {
    [self updateFilenamesPreview];
    deadbeef->conf_set_str ("converter.output_folder", [[_outputFolder stringValue] UTF8String]);
    deadbeef->conf_save ();
}

- (IBAction)preserveFolderStructureChanged:(id)sender {
    [self updateFilenamesPreview];
    deadbeef->conf_set_int ("converter.preserve_folder_structure", [_preserveFolderStructure state] == NSOnState);
    deadbeef->conf_save ();
}

- (IBAction)bypassSameFormatChanged:(id)sender {
    deadbeef->conf_set_int ("converter.bypass_same_format", [_bypassSameFormat state] == NSOnState);
    deadbeef->conf_save ();

    [_retagAfterCopy setEnabled:[_bypassSameFormat state] == NSOnState];
}

- (IBAction)retagAfterCopyChanged:(id)sender {
    deadbeef->conf_set_int ("converter.retag_after_copy", [_retagAfterCopy state] == NSOnState);
    deadbeef->conf_save ();
}

-(void)updateFilenamesPreview {
    NSMutableArray *convert_items_preview = [NSMutableArray arrayWithCapacity:_convert_items_count];

    int enc_preset = (int)[_encoderPreset indexOfSelectedItem];
    ddb_encoder_preset_t *encoder_preset = NULL;

    if (enc_preset >= 0) {
        encoder_preset = _converter_plugin->encoder_preset_get_for_idx (enc_preset);
    }

    if (!encoder_preset) {
        return;
    }

    NSString *outfile = [_outputFileName stringValue];

    if ([outfile isEqual:@""]) {
        outfile = default_format;
    }

    for (int n = 0; n < _convert_items_count; n++) {
        DB_playItem_t *it = _convert_items[n];
        if (it) {
            char outpath[PATH_MAX];

            _converter_plugin->get_output_path2 (it, _convert_playlist, [[_outputFolder stringValue] UTF8String], [outfile UTF8String], encoder_preset, [_preserveFolderStructure state] == NSOnState, "", [_writeToSourceFolder state] == NSOnState, outpath, sizeof (outpath));

            [convert_items_preview addObject:[NSString stringWithUTF8String:outpath]];
        }
    }

    [_filenamePreviewController setContent:convert_items_preview];
}

- (IBAction)outputPathChanged:(id)sender {
    [self updateFilenamesPreview];
    deadbeef->conf_set_str ("converter.output_file", [[_outputFileName stringValue] UTF8String]);
    deadbeef->conf_save ();
}

- (IBAction)writeToSourceFolderChanged:(id)sender {
    [self updateFilenamesPreview];
    int active = [sender state] == NSOnState;
    deadbeef->conf_set_int ("converter.write_to_source_folder", active);
    deadbeef->conf_save ();
    [_outputFolder setEnabled:!active];
    [_preserveFolderStructure setEnabled:!active];
}


- (IBAction)encoderPresetChanged:(id)sender {
    deadbeef->conf_set_int ("converter.encoder_preset", (int)[_encoderPreset indexOfSelectedItem]);
    [self updateFilenamesPreview];
}

- (IBAction)dspPresetChanged:(id)sender {
    deadbeef->conf_set_int ("converter.dsp_preset", (int)[_dspPreset indexOfSelectedItem]-1);
    deadbeef->conf_save ();
}

- (IBAction)overwritePromptChanged:(id)sender {
    deadbeef->conf_set_int ("converter.overwrite_action", (int)[_fileExistsAction indexOfSelectedItem]);
    deadbeef->conf_save ();
}

- (IBAction)outputFormatChanged:(id)sender {
    deadbeef->conf_set_int ("converter.output_format", (int)[_outputFormat indexOfSelectedItem]);
    deadbeef->conf_save ();
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
    _converter_plugin = (ddb_converter_t *)deadbeef->plug_get_for_id ("converter");

    _encoderPresetsTableView.dataSource = self;
    _encoderPresetsTableView.delegate = self;
    [self initializeWidgets];
    self.window.delegate = self;
}

- (IBAction)progressCancelAction:(id)sender {
    _cancelled = YES;
}

- (void)reset {
    if (_convert_items) {
        for (int n = 0; n < _convert_items_count; n++) {
            deadbeef->pl_item_unref (_convert_items[n]);
        }
        free (_convert_items);
        _convert_items = NULL;
    }
    _convert_items_count = 0;

    if (_convert_playlist) {
        deadbeef->plt_unref (_convert_playlist);
        _convert_playlist = NULL;
    }

    _outfolder = nil;
    _outfile = nil;
    if (_encoder_preset) {
        _converter_plugin->encoder_preset_free (_encoder_preset);
        _encoder_preset = NULL;
    }
    if (_dsp_preset) {
        _converter_plugin->dsp_preset_free (_dsp_preset);
        _dsp_preset = NULL;
    }
}

- (void)run:(int)ctx {
    if (_converter_plugin) {
        [self initializeWidgets];
    }
    [self showWindow:self];
    [[self window] makeKeyWindow];

    // reinit everything
    [self reset];

    // store list of tracks
    deadbeef->pl_lock ();
    switch (ctx) {
        case DDB_ACTION_CTX_MAIN:
        case DDB_ACTION_CTX_SELECTION:
        {
            // copy list
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                _convert_playlist = plt;
                _convert_items_count = deadbeef->plt_getselcount (plt);
                if (0 < _convert_items_count) {
                    _convert_items = malloc (sizeof (DB_playItem_t *) * _convert_items_count);
                    if (_convert_items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            if (deadbeef->pl_is_selected (it)) {
                                assert (n < _convert_items_count);
                                deadbeef->pl_item_ref (it);
                                _convert_items[n++] = it;
                            }
                            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                            deadbeef->pl_item_unref (it);
                            it = next;
                        }
                    }
                }
            }
            break;
        }
        case DDB_ACTION_CTX_PLAYLIST:
        {
            // copy list
            ddb_playlist_t *plt = deadbeef->plt_get_curr ();
            if (plt) {
                _convert_playlist = plt;
                _convert_items_count = deadbeef->plt_get_item_count (plt, PL_MAIN);
                if (0 < _convert_items_count) {
                    _convert_items = malloc (sizeof (DB_playItem_t *) * _convert_items_count);
                    if (_convert_items) {
                        int n = 0;
                        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
                        while (it) {
                            _convert_items[n++] = it;
                            it = deadbeef->pl_get_next (it, PL_MAIN);
                        }
                    }
                }
            }
            break;
        }
        case DDB_ACTION_CTX_NOWPLAYING:
        {
            DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
            if (it) {
                _convert_playlist = deadbeef->pl_get_playlist (it);
                _convert_items_count = 1;
                _convert_items = malloc (sizeof (DB_playItem_t *) * _convert_items_count);
                if (_convert_items) {
                    _convert_items[1] = it;
                }
            }
        }
            break;
    }
    deadbeef->pl_unlock ();
    [self updateFilenamesPreview];
}


- (void)converterFinished:(id)instance withResult:(int)result {
    if (g_converterControllers) {
        [g_converterControllers removeObject:instance];
    }
}

- (IBAction)cancelAction:(id)sender {
    [[self window] close];
}

- (void)windowWillClose:(NSNotification *)notification {
    if (!_working) {
        [self converterFinished:self withResult:0];
    }
}

- (IBAction)openOutputFolderAction:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    [panel setCanChooseDirectories:YES];
    [panel setCanChooseFiles:NO];
    [panel setAllowsMultipleSelection:NO];
    [panel setMessage:@"Choose output folder"];

    // Display the panel attached to the document's window.
    [panel beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result){
        if (result == NSFileHandlingPanelOKButton) {
            NSURL * url = [panel URL];
            [_outputFolder setStringValue: [url path]];
        }
    }];
}

// encoder presets sheet
- (IBAction)editEncoderPresetsAction:(id)sender {
    [NSApp beginSheet:_encoderPresetsPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndEncoderPresetList:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndEncoderPresetList:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_encoderPresetsPanel orderOut:self];
    [self fillEncoderPresets];
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

- (BOOL)tableView:(NSTableView *)aTableView shouldEditTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx ((int)rowIndex);
    return p && !p->readonly;
}

- (NSString *)uniqueEncoderPresetTitle:(NSString *)title {
    NSString *uniqueTitle = title;
    int nr = 1;
    for (;;) {
        ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_list ();
        while (p) {
            if (!strcmp ([uniqueTitle UTF8String], p->title)) {
                break;
            }
            p = p->next;
        }
        if (!p) {
            return uniqueTitle;
        }
        uniqueTitle = [NSString stringWithFormat:@"%@ (%d)", title, nr];
        nr++;
    }
    return nil;
}

- (IBAction)addEncoderPresetAction:(id)sender {
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_alloc ();
    NSString *title = [self uniqueEncoderPresetTitle:@"New preset"];
    p->title = strdup ([title UTF8String]);
    p->encoder = strdup ("");
    p->ext = strdup ("");

    int cnt = 0;
    ddb_encoder_preset_t *pp = _converter_plugin->encoder_preset_get_list ();
    while (pp) {
        cnt++;
        pp = pp->next;
    }

    _converter_plugin->encoder_preset_append (p);

    [_encoderPresetsTableView reloadData];
    [_encoderPresetsTableView editColumn:0 row:cnt withEvent:nil select:YES];
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx ((int)rowIndex);
    if (p) {
        if (p->title) {
            char path[PATH_MAX];
            if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_system_dir (DDB_SYS_DIR_CONFIG), p->title) > 0) {
                unlink (path);
            }
            free (p->title);
        }
        NSString *title = [self uniqueEncoderPresetTitle:anObject];
        p->title = strdup ([title UTF8String]);
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)removeEncoderPresetAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if (p) {
        if (!p->readonly) {
            _converter_plugin->encoder_preset_remove(p);
            [_encoderPresetsTableView reloadData];
        }
    }
}


- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    [self setPresetInfo:[_encoderPresetsTableView selectedRow]];
}

- (void)setPresetInfo:(NSInteger)idx {
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx ((int)idx);
    if (p) {

        if (p->tag_id3v2 ^ [[self encoderPresetID3v2Tag] state] ) { [[self encoderPresetID3v2Tag] setNextState]; };
        if (p->tag_id3v1 ^ [[self encoderPresetID3v1Tag] state]) { [[self encoderPresetID3v1Tag] setNextState]; };
        if (p->tag_apev2 ^ [[self encoderPresetApeTag] state] ) { [[self encoderPresetApeTag] setNextState]; };
        if (p->tag_flac ^ [[self encoderPresetFlacTag] state]) { [[self encoderPresetFlacTag] setNextState]; };
        if (p->tag_oggvorbis ^ [[self encoderPresetOggVorbisTag] state]) { [[self encoderPresetOggVorbisTag] setNextState]; };
        if (p->tag_mp4 ^ [[self encoderPresetMP4Tag] state] ) { [[self encoderPresetMP4Tag] setNextState]; };

        [[self encoderPresetOutputFileExtension] setStringValue: [NSString stringWithFormat:@"%s", p->ext] ];
        [[self encoderPresetCommandLine] setStringValue: [NSString stringWithFormat:@"%s", p->encoder] ];

        [[self encoderPresetExecutionMethod] selectItemAtIndex:(p->method)];
        [[self encoderPresetID3v2TagVersion] selectItemAtIndex:(p->id3v2_version)];

        BOOL enabled = !(p->readonly);
        [[self encoderPresetOutputFileExtension] setEnabled: enabled ];
        [[self encoderPresetCommandLine] setEnabled: enabled ];
        [[self encoderPresetExecutionMethod] setEnabled: enabled ];
        [[self encoderPresetID3v2TagVersion] setEnabled: enabled ];
        [[self encoderPresetApeTag] setEnabled: enabled ];
        [[self encoderPresetFlacTag] setEnabled: enabled ];
        [[self encoderPresetOggVorbisTag] setEnabled: enabled ];
        [[self encoderPresetID3v1Tag] setEnabled: enabled ];
        [[self encoderPresetID3v2Tag] setEnabled: enabled ];
        [[self encoderPresetMP4Tag] setEnabled: enabled ];
    }
}

- (IBAction)encoderPresetOutputFileExtensionChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->ext = strdup ([[sender stringValue] UTF8String]);
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetCommandLineChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->encoder = strdup ([[sender stringValue] UTF8String]);
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetExecutionMethodChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->method = (int)[sender indexOfSelectedItem];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetID3v2TagVersionChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->id3v2_version = (int)[sender indexOfSelectedItem];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetApeTagChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->tag_apev2 = [sender state];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetFlacTagChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->tag_flac = [sender state];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetOggVorbisTagChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->tag_oggvorbis = [sender state];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetID3v1TagChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->tag_id3v1 = [sender state];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetID3v2TagChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->tag_id3v2 = [sender state];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}

- (IBAction)encoderPresetMP4TagChangedAction:(id)sender {
    int idx = (int)[_encoderPresetsTableView selectedRow];
    ddb_encoder_preset_t *p = _converter_plugin->encoder_preset_get_for_idx (idx);
    if(p) {
        p->tag_mp4 = [sender state];
        _converter_plugin->encoder_preset_save (p, 1);
    }
}


// dsp presets sheet
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

- (IBAction)okAction:(id)sender {
    _outfolder = [_outputFolder stringValue];

    _outfile = [_outputFileName stringValue];

    if ([_outfile isEqual:@""]) {
        _outfile = default_format;
    }

    _preserve_folder_structure = [_preserveFolderStructure state] == NSOnState;

    _write_to_source_folder = [_writeToSourceFolder state] == NSOnState;

    _overwrite_action = (int)[_fileExistsAction indexOfSelectedItem];

    int selected_format = (int)[_outputFormat indexOfSelectedItem];
    switch (selected_format) {
        case 1 ... 4:
            _output_bps = selected_format * 8;
            _output_is_float = 0;
            break;
        case 5:
            _output_bps = 32;
            _output_is_float = 1;
            break;
        default:
            _output_bps = -1; // same as input, or encoder default
            break;
    }

    int enc_preset = (int)[_encoderPreset indexOfSelectedItem];
    ddb_encoder_preset_t *encoder_preset = NULL;

    if (enc_preset >= 0) {
        encoder_preset = _converter_plugin->encoder_preset_get_for_idx (enc_preset);
    }

    if (!encoder_preset) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"OK"];
        [alert setMessageText:@"Encoder is not selected."];
        [alert setInformativeText:@"Please select one of the encoders from the list."];
        [alert setAlertStyle:NSCriticalAlertStyle];
        [alert beginSheetModalForWindow:[self window] modalDelegate:self didEndSelector:nil contextInfo:nil];

        return;
    }

    int dsp_idx = (int)[_dspPreset indexOfSelectedItem] - 1;

    ddb_dsp_preset_t *dsp_preset = NULL;
    if (dsp_idx >= 0) {
        dsp_preset = _converter_plugin->dsp_preset_get_for_idx (dsp_idx);
    }

    // copy selected presets, to guarantee they don't change while converting
    if (encoder_preset) {
        _encoder_preset = _converter_plugin->encoder_preset_alloc ();
        _converter_plugin->encoder_preset_copy (_encoder_preset, encoder_preset);
    }
    if (dsp_preset) {
        _dsp_preset = _converter_plugin->dsp_preset_alloc ();
        _converter_plugin->dsp_preset_copy (_dsp_preset, dsp_preset);
    }

    _cancelled = NO;
    [[self window] setIsVisible:NO];

    [_progressText setStringValue:@""];
    [_progressOutText setStringValue:@""];
    [_progressNumeric setStringValue:@""];
    [_progressBar setMinValue:0];
    [_progressBar setMaxValue:_convert_items_count-1];
    [_progressBar setDoubleValue:0];
    [_progressPanel setIsVisible:YES];

    _working = YES;

    _bypassSameFormatState = [_bypassSameFormat state];
    _retagAfterCopyState = [_retagAfterCopy state];

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        [self converterWorker];
    });
}

- (void)converterWorker {
    deadbeef->background_job_increment ();

    char root[2000] = "";
    size_t rootlen = 0;
    // prepare for preserving folder struct
    if (_preserve_folder_structure && _convert_items_count >= 1) {
        // start with the 1st track path
        deadbeef->pl_get_meta (_convert_items[0], ":URI", root, sizeof (root));
        char *sep = strrchr (root, '/');
        if (sep) {
            *sep = 0;
        }
        // reduce
        rootlen = strlen (root);
        for (int n = 1; n < _convert_items_count; n++) {
            deadbeef->pl_lock ();
            const char *path = deadbeef->pl_find_meta (_convert_items[n], ":URI");
            if (strncmp (path, root, rootlen)) {
                // find where path splits
                char *r = root;
                while (*path && *r) {
                    if (*path != *r) {
                        // find new separator
                        while (r > root && *r != '/') {
                            r--;
                        }
                        *r = 0;
                        rootlen = r-root;
                        break;
                    }
                    path++;
                    r++;
                }
            }
            deadbeef->pl_unlock ();
        }
    }

    ddb_converter_settings_t settings = {
        .output_bps = _output_bps,
        .output_is_float = _output_is_float,
        .encoder_preset = _encoder_preset,
        .dsp_preset = _dsp_preset,
        .bypass_conversion_on_same_format = (_bypassSameFormatState == NSOnState),
        .rewrite_tags_after_copy = (_retagAfterCopyState == NSOnState),
    };

    for (int n = 0; n < _convert_items_count; n++) {
        deadbeef->pl_lock ();
        NSString *text = [NSString stringWithUTF8String:deadbeef->pl_find_meta (_convert_items[n], ":URI")];
        deadbeef->pl_unlock ();
        char outpath[PATH_MAX];
        _converter_plugin->get_output_path2 (_convert_items[n], _convert_playlist, [_outfolder UTF8String], [_outfile UTF8String], _encoder_preset, _preserve_folder_structure, root, _write_to_source_folder, outpath, sizeof (outpath));
        NSString *nsoutpath = [NSString stringWithUTF8String:outpath];

        dispatch_async(dispatch_get_main_queue(), ^{
            [_progressBar setDoubleValue:n];
            [_progressText setStringValue:text];
            [_progressOutText setStringValue:nsoutpath];
            [_progressNumeric setStringValue:[NSString stringWithFormat:@"%d/%d", n+1, _convert_items_count]];
        });

        int skip = 0;
        char *real_out = realpath(outpath, NULL);
        if (real_out) {
            skip = 1;
            deadbeef->pl_lock();
            char *real_in = realpath(deadbeef->pl_find_meta(_convert_items[n], ":URI"), NULL);
            deadbeef->pl_unlock();
            const int paths_match = real_in && !strcmp(real_in, real_out);
            free(real_in);
            free(real_out);
            if (paths_match) {
                fprintf (stderr, "converter: destination file is the same as source file, skipping\n");
            }
            else if (_overwrite_action == 2) {
                unlink (outpath);
                skip = 0;
            }
            else {
                NSInteger result = [self overwritePrompt:[NSString stringWithUTF8String:outpath]];
                if (result == NSAlertSecondButtonReturn) {
                    unlink (outpath);
                    skip = 0;
                }
                else if (result == NSAlertThirdButtonReturn) {
                    _cancelled = YES;
                }
            }
        }

        if (!skip) {
            _converter_plugin->convert2 (&settings, _convert_items[n], outpath, &_cancelled);
        }
        if (_cancelled) {
            break;
        }
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        [_progressPanel close];
        [self converterFinished:self withResult:1];
    });

    deadbeef->background_job_decrement ();

    _working = NO;
}

- (NSInteger)overwritePrompt:(NSString *)path {
    _overwritePromptCondition = [[NSCondition alloc] init];
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"Cancel"];
        [alert setMessageText:@"The file already exists. Overwrite?"];
        [alert setInformativeText:path];
        [alert setAlertStyle:NSCriticalAlertStyle];

        [alert beginSheetModalForWindow:_progressPanel modalDelegate:self didEndSelector:@selector(alertDidEndOverwritePrompt:returnCode:contextInfo:) contextInfo:nil];
    });

    [_overwritePromptCondition lock];
    [_overwritePromptCondition wait];
    [_overwritePromptCondition unlock];
    _overwritePromptCondition = nil;
    return _overwritePromptResult;
}

- (void)alertDidEndOverwritePrompt:(NSAlert *)alert returnCode:(NSInteger)returnCode
        contextInfo:(void *)contextInfo {

    [_overwritePromptCondition lock];
    _overwritePromptResult = returnCode;
    [_overwritePromptCondition signal];
    [_overwritePromptCondition unlock];
}

+ (void)runConverter:(int)ctx {
    ConverterWindowController *conv = [[ConverterWindowController alloc] initWithWindowNibName:@"Converter"];

    if (!g_converterControllers) {
        g_converterControllers = [[NSMutableArray alloc] init];
    }
    [g_converterControllers addObject:conv];
    [conv run:DDB_ACTION_CTX_SELECTION];
}

+ (void)cleanup {
    g_converterControllers = nil;
}

@end
