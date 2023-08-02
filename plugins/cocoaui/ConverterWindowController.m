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

#import "ConverterWindowController.h"
#import "ScriptableErrorViewer.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableSelectViewController.h"
#import "scriptable_dsp.h"
#import "scriptable_encoder.h"
#include "../converter/converter.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;
static NSString *default_format = @"[%tracknumber%. ][%artist% - ]%title%";

@interface ConverterWindowController () <ScriptableSelectDelegate,ScriptableItemDelegate,NSControlTextEditingDelegate>

@property (nonatomic) ddb_converter_t *converter_plugin;

@property (nonatomic) NSCondition *overwritePromptCondition;

// conversion context
@property (nonatomic) NSString *outfolder;
@property (nonatomic) NSString *outfile;
@property (nonatomic) int preserve_folder_structure;
@property (nonatomic) int write_to_source_folder;
@property (nonatomic) int output_bps;
@property (nonatomic) int output_is_float;
@property (nonatomic) int overwrite_action;
@property (nonatomic) int cancelled;
@property (nonatomic) NSInteger overwritePromptResult;
@property (nonatomic) BOOL working;
@property (nonatomic) NSInteger bypassSameFormatState;
@property (nonatomic) NSInteger retagAfterCopyState;


@property (nonatomic) ddb_playItem_t **convert_items;
@property (nonatomic) ddb_playlist_t *convert_playlist;
@property (nonatomic) NSInteger convert_items_count;

@property (nonatomic,unsafe_unretained) ddb_dsp_preset_t *dsp_preset;
@property (nonatomic,unsafe_unretained) ddb_encoder_preset_t *encoder_preset;

@property (nonatomic) ScriptableSelectViewController *dspSelectViewController;
@property (nonatomic) ScriptableTableDataSource *dspPresetsDataSource;
@property (weak) IBOutlet NSView *dspPresetSelectorContainer;

@property (nonatomic) ScriptableSelectViewController *encoderSelectViewController;
@property (nonatomic) ScriptableTableDataSource *encoderPresetsDataSource;
@property (weak) IBOutlet NSView *encoderPresetSelectorContainer;

@property (unsafe_unretained) IBOutlet NSTextField *outputFolder;
@property (unsafe_unretained) IBOutlet NSButton *writeToSourceFolder;
@property (unsafe_unretained) IBOutlet NSButton *preserveFolderStructure;
@property (unsafe_unretained) IBOutlet NSButton *bypassSameFormat;
@property (weak) IBOutlet NSButton *retagAfterCopy;
@property (unsafe_unretained) IBOutlet NSTextField *outputFileName;
@property (unsafe_unretained) IBOutlet NSArrayController *filenamePreviewController;
@property (unsafe_unretained) IBOutlet NSPopUpButton *outputFormat;
@property (unsafe_unretained) IBOutlet NSPopUpButton *fileExistsAction;
- (IBAction)cancelAction:(id)sender;
- (IBAction)okAction:(id)sender;

- (IBAction)openOutputFolderAction:(id)sender;

- (IBAction)writeToSourceFolderChanged:(id)sender;
- (IBAction)preserveFolderStructureChanged:(id)sender;
- (IBAction)bypassSameFormatChanged:(id)sender;
- (IBAction)retagAfterCopyChanged:(id)sender;
- (IBAction)overwritePromptChanged:(id)sender;
- (IBAction)outputFormatChanged:(id)sender;

@property (strong) IBOutlet NSPanel *progressPanel;
@property (unsafe_unretained) IBOutlet NSTextField *progressText;
@property (unsafe_unretained) IBOutlet NSTextField *progressOutText;
@property (unsafe_unretained) IBOutlet NSTextField *progressNumeric;

@property (unsafe_unretained) IBOutlet NSProgressIndicator *progressBar;
- (IBAction)progressCancelAction:(id)sender;



@end

static NSMutableArray *g_converterControllers;

@implementation ConverterWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
    self.converter_plugin = (ddb_converter_t *)deadbeef->plug_get_for_id ("converter");

    [self initializeWidgets];
    self.window.delegate = self;

    self.dspPresetsDataSource = [ScriptableTableDataSource dataSourceWithScriptable:scriptableDspRoot()];
    self.dspSelectViewController.dataSource = self.dspPresetsDataSource;
    self.dspSelectViewController = [ScriptableSelectViewController new];
    self.dspSelectViewController.view.frame = self.dspPresetSelectorContainer.bounds;
    [_dspPresetSelectorContainer addSubview:self.dspSelectViewController.view];
    self.dspSelectViewController.dataSource = self.dspPresetsDataSource;
    self.dspSelectViewController.scriptableItemDelegate = self;
    self.dspSelectViewController.scriptableSelectDelegate = self;
    self.dspSelectViewController.errorViewer = ScriptableErrorViewer.sharedInstance;

    char dsp_preset_name[100];
    deadbeef->conf_get_str ("converter.dsp_preset_name", "", dsp_preset_name, sizeof(dsp_preset_name));
    scriptableItem_t *dspPreset = scriptableItemSubItemForName(scriptableDspRoot(), dsp_preset_name);
    if (dspPreset) {
        [self.dspSelectViewController selectItem:dspPreset];
    }

    self.encoderPresetsDataSource = [ScriptableTableDataSource dataSourceWithScriptable:scriptableEncoderRoot()];
    self.encoderSelectViewController.dataSource = self.dspPresetsDataSource;

    self.encoderSelectViewController = [ScriptableSelectViewController new];
    self.encoderSelectViewController.view.frame = self.encoderPresetSelectorContainer.bounds;
    [self.encoderPresetSelectorContainer addSubview:self.encoderSelectViewController.view];
    self.encoderSelectViewController.dataSource = self.encoderPresetsDataSource;
    self.encoderSelectViewController.scriptableItemDelegate = self;
    self.encoderSelectViewController.scriptableSelectDelegate = self;
    self.encoderSelectViewController.errorViewer = ScriptableErrorViewer.sharedInstance;

    char enc_preset_name[100];
    deadbeef->conf_get_str ("converter.encoder_preset_name", "", enc_preset_name, sizeof(enc_preset_name));
    scriptableItem_t *encPreset = scriptableItemSubItemForName(scriptableEncoderRoot(), enc_preset_name);
    if (encPreset) {
        [self.encoderSelectViewController selectItem:encPreset];
    }

}

- (void)dealloc {
    [self reset];
}

- (void)initializeWidgets {
    deadbeef->conf_lock ();
    const char *out_folder = deadbeef->conf_get_str_fast ("converter.output_folder", "");
    if (!out_folder[0]) {
        out_folder = getenv("HOME");
    }

    self.outputFolder.stringValue = @(out_folder);
    self.outputFileName.stringValue = @(deadbeef->conf_get_str_fast ("converter.output_file", ""));
    self.preserveFolderStructure.state = deadbeef->conf_get_int ("converter.preserve_folder_structure", 0) ? NSControlStateValueOn : NSControlStateValueOff;
    self.bypassSameFormat.state = deadbeef->conf_get_int ("converter.bypass_same_format", 0);
    self.retagAfterCopy.state = deadbeef->conf_get_int ("converter.retag_after_copy", 0);
    self.retagAfterCopy.enabled = self.bypassSameFormat.state == NSControlStateValueOn;

    int write_to_source_folder = deadbeef->conf_get_int ("converter.write_to_source_folder", 0);
    self.writeToSourceFolder.state = write_to_source_folder ? NSControlStateValueOn : NSControlStateValueOff;
    self.outputFolder.enabled = !write_to_source_folder;
    self.preserveFolderStructure.enabled = !write_to_source_folder;

    [_fileExistsAction selectItemAtIndex:deadbeef->conf_get_int ("converter.overwrite_action", 0)];
    deadbeef->conf_unlock ();

    [_outputFormat selectItemAtIndex:deadbeef->conf_get_int ("converter.output_format", 0)];
    [_fileExistsAction selectItemAtIndex:deadbeef->conf_get_int ("converter.overwrite_action", 0)];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = notification.object;
    if (textField == self.outputFolder) {
        [self updateFilenamesPreview];
        deadbeef->conf_set_str ("converter.output_folder", _outputFolder.stringValue.UTF8String);
        deadbeef->conf_save ();
    }
    else if (textField == self.outputFileName) {
        [self updateFilenamesPreview];
        deadbeef->conf_set_str ("converter.output_file", _outputFileName.stringValue.UTF8String);
        deadbeef->conf_save ();
    }
}

- (IBAction)preserveFolderStructureChanged:(id)sender {
    [self updateFilenamesPreview];
    deadbeef->conf_set_int ("converter.preserve_folder_structure", self.preserveFolderStructure.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)bypassSameFormatChanged:(id)sender {
    deadbeef->conf_set_int ("converter.bypass_same_format", self.bypassSameFormat.state == NSControlStateValueOn);
    deadbeef->conf_save ();

    self.retagAfterCopy.enabled = self.bypassSameFormat.state == NSControlStateValueOn;
}

- (IBAction)retagAfterCopyChanged:(id)sender {
    deadbeef->conf_set_int ("converter.retag_after_copy", self.retagAfterCopy.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

-(void)updateFilenamesPreview {
    NSMutableArray *convert_items_preview = [NSMutableArray arrayWithCapacity:_convert_items_count];


    NSInteger selectedEncoderPreset = self.encoderSelectViewController.indexOfSelectedItem;
    if (selectedEncoderPreset == -1) {
        return;
    }
    scriptableItem_t *preset = scriptableItemChildAtIndex(scriptableEncoderRoot(), (unsigned int)selectedEncoderPreset);
    ddb_encoder_preset_t *encoder_preset = self.converter_plugin->encoder_preset_alloc();
    scriptableEncoderPresetToConverterEncoderPreset(preset, encoder_preset);

    NSString *outfile = _outputFileName.stringValue;

    if ([outfile isEqual:@""]) {
        outfile = default_format;
    }

    for (int n = 0; n < self.convert_items_count; n++) {
        DB_playItem_t *it = self.convert_items[n];
        if (it) {
            char outpath[PATH_MAX];

            self.converter_plugin->get_output_path2 (it, self.convert_playlist, _outputFolder.stringValue.UTF8String, outfile.UTF8String, encoder_preset, self.preserveFolderStructure.state == NSControlStateValueOn, "", self.writeToSourceFolder.state == NSControlStateValueOn, outpath, sizeof (outpath));

            [convert_items_preview addObject:@(outpath)];
        }
    }

    self.filenamePreviewController.content = convert_items_preview;
}

- (IBAction)writeToSourceFolderChanged:(NSButton *)sender {
    [self updateFilenamesPreview];
    int active = sender.state == NSControlStateValueOn;
    deadbeef->conf_set_int ("converter.write_to_source_folder", active);
    deadbeef->conf_save ();
    self.outputFolder.enabled = !active;
    self.preserveFolderStructure.enabled = !active;
}


- (IBAction)overwritePromptChanged:(id)sender {
    deadbeef->conf_set_int ("converter.overwrite_action", (int)_fileExistsAction.indexOfSelectedItem);
    deadbeef->conf_save ();
}

- (IBAction)outputFormatChanged:(id)sender {
    deadbeef->conf_set_int ("converter.output_format", (int)_outputFormat.indexOfSelectedItem);
    deadbeef->conf_save ();
}

- (IBAction)progressCancelAction:(id)sender {
    self.cancelled = YES;
}

- (void)reset {
    if (_convert_items) {
        for (NSInteger n = 0; n < self.convert_items_count; n++) {
            deadbeef->pl_item_unref (_convert_items[n]);
        }
        free (_convert_items);
        self.convert_items = NULL;
    }
    self.convert_items_count = 0;

    if (_convert_playlist) {
        deadbeef->plt_unref (_convert_playlist);
        self.convert_playlist = NULL;
    }

    self.outfolder = nil;
    self.outfile = nil;

    if (self.encoder_preset) {
        self.encoder_preset = NULL;
    }
    if (self.dsp_preset) {
        self.dsp_preset = NULL;
    }
}

- (void)runWithTracks:(ddb_playItem_t **)tracks count:(NSInteger)count playlist:(ddb_playlist_t *)plt {
    if (self.converter_plugin) {
        [self initializeWidgets];
    }
    [self showWindow:self];
    [self.window makeKeyWindow];

    // reinit everything
    [self reset];

    if (plt) {
        deadbeef->plt_ref (plt);
        self.convert_playlist = plt;
    }

    // store list of tracks
    self.convert_items_count = count;
    self.convert_items = calloc (count, sizeof (ddb_playItem_t *));

    for (NSInteger i = 0; i < count; i++) {
        ddb_playItem_t *it = tracks[i];
        deadbeef->pl_item_ref (it);
        self.convert_items[i] = it;
    }

    [self updateFilenamesPreview];
}


- (void)converterFinished:(id)instance withResult:(int)result {
    if (g_converterControllers) {
        [g_converterControllers removeObject:instance];
    }
}

- (IBAction)cancelAction:(id)sender {
    [self.window close];
}

- (void)windowWillClose:(NSNotification *)notification {
    if (!self.working) {
        [self converterFinished:self withResult:0];
    }
}

- (IBAction)openOutputFolderAction:(id)sender {
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    panel.canChooseDirectories = YES;
    panel.canChooseFiles = NO;
    panel.allowsMultipleSelection = NO;
    panel.message = @"Choose output folder";

    // Display the panel attached to the document's window.
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result){
        if (result == NSModalResponseOK) {
            NSURL * url = panel.URL;
            self.outputFolder.stringValue =  url.path;
        }
    }];
}

- (void)setEncoder_preset:(ddb_encoder_preset_t *)encoder_preset {
    if (_encoder_preset) {
        self.converter_plugin->encoder_preset_free (_encoder_preset);
    }
    _encoder_preset = encoder_preset;
}

- (void)setDsp_preset:(ddb_dsp_preset_t *)dsp_preset {
    if (_dsp_preset) {
        self.converter_plugin->dsp_preset_free (_dsp_preset);
    }
    _dsp_preset = dsp_preset;
}

- (IBAction)okAction:(id)sender {
    self.outfolder = _outputFolder.stringValue;

    self.outfile = _outputFileName.stringValue;

    if ([self.outfile isEqual:@""]) {
        self.outfile = default_format;
    }

    self.preserve_folder_structure = self.preserveFolderStructure.state == NSControlStateValueOn;

    self.write_to_source_folder = self.writeToSourceFolder.state == NSControlStateValueOn;

    self.overwrite_action = (int)_fileExistsAction.indexOfSelectedItem;

    int selected_format = (int)_outputFormat.indexOfSelectedItem;
    switch (selected_format) {
        case 1 ... 4:
            self.output_bps = selected_format * 8;
            self.output_is_float = 0;
            break;
        case 5:
            self.output_bps = 32;
            self.output_is_float = 1;
            break;
        default:
            self.output_bps = -1; // same as input, or encoder default
            break;
    }

    NSInteger selectedEncoderPreset = self.encoderSelectViewController.indexOfSelectedItem;
    if (selectedEncoderPreset != -1) {
        scriptableItem_t *preset = scriptableItemChildAtIndex(scriptableEncoderRoot(), (unsigned int)selectedEncoderPreset);
        self.encoder_preset = self.converter_plugin->encoder_preset_alloc();
        scriptableEncoderPresetToConverterEncoderPreset(preset, self.encoder_preset);
    }


    if (!self.encoder_preset) {
        NSAlert *alert = [NSAlert new];
        [alert addButtonWithTitle:@"OK"];
        alert.messageText = @"Encoder is not selected.";
        alert.informativeText = @"Please select one of the encoders from the list.";
        alert.alertStyle = NSAlertStyleCritical;
        [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
        }];

        return;
    }

    NSInteger selectedDspPreset = self.dspSelectViewController.indexOfSelectedItem;
    if (selectedDspPreset != -1) {
        scriptableItem_t *preset = scriptableItemChildAtIndex(scriptableDspRoot(), (unsigned int)selectedDspPreset);
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain(preset);
        if (chain) {
            self.dsp_preset = self.converter_plugin->dsp_preset_alloc ();
            self.dsp_preset->chain = chain;
        }
    }

    self.cancelled = NO;
    self.window.isVisible = NO;

    self.progressText.stringValue = @"";
    self.progressOutText.stringValue = @"";
    self.progressNumeric.stringValue = @"";
    self.progressBar.minValue = 0;
    self.progressBar.maxValue = self.convert_items_count-1;
    self.progressBar.doubleValue = 0;
    self.progressPanel.isVisible = YES;

    self.working = YES;

    self.bypassSameFormatState = self.bypassSameFormat.state;
    self.retagAfterCopyState = self.retagAfterCopy.state;

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
    if (self.preserve_folder_structure && self.convert_items_count >= 1) {
        // start with the 1st track path
        deadbeef->pl_get_meta (_convert_items[0], ":URI", root, sizeof (root));
        char *sep = strrchr (root, '/');
        if (sep) {
            *sep = 0;
        }
        // reduce
        rootlen = strlen (root);
        for (int n = 1; n < self.convert_items_count; n++) {
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
        .output_bps = self.output_bps,
        .output_is_float = self.output_is_float,
        .encoder_preset = self.encoder_preset,
        .dsp_preset = self.dsp_preset,
        .bypass_conversion_on_same_format = (self.bypassSameFormatState == NSControlStateValueOn),
        .rewrite_tags_after_copy = (self.retagAfterCopyState == NSControlStateValueOn),
    };

    for (int n = 0; n < self.convert_items_count; n++) {
        deadbeef->pl_lock ();
        NSString *text = @(deadbeef->pl_find_meta (_convert_items[n], ":URI"));
        deadbeef->pl_unlock ();
        char outpath[PATH_MAX];
        self.converter_plugin->get_output_path2 (_convert_items[n], self.convert_playlist, (self.outfolder).UTF8String, (self.outfile).UTF8String, self.encoder_preset, self.preserve_folder_structure, root, self.write_to_source_folder, outpath, sizeof (outpath));
        NSString *nsoutpath = @(outpath);

        dispatch_async(dispatch_get_main_queue(), ^{
            self.progressBar.doubleValue = n;
            self.progressText.stringValue = text;
            self.progressOutText.stringValue = nsoutpath;
            self.progressNumeric.stringValue = [NSString stringWithFormat:@"%d/%d", n+1, (int)self.convert_items_count];
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
            else if (self.overwrite_action == 2) {
                unlink (outpath);
                skip = 0;
            }
            else {
                NSInteger result = [self overwritePrompt:@(outpath)];
                if (result == NSAlertSecondButtonReturn) {
                    unlink (outpath);
                    skip = 0;
                }
                else if (result == NSAlertThirdButtonReturn) {
                    self.cancelled = YES;
                }
            }
        }

        if (!skip) {
            self.converter_plugin->convert2 (&settings, self.convert_items[n], outpath, &_cancelled);
        }
        if (self.cancelled) {
            break;
        }
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.progressPanel close];
        [self converterFinished:self withResult:1];
    });

    deadbeef->background_job_decrement ();

    self.working = NO;
}

- (NSInteger)overwritePrompt:(NSString *)path {
    self.overwritePromptCondition = [NSCondition new];
    dispatch_sync(dispatch_get_main_queue(), ^{
        NSAlert *alert = [NSAlert new];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"Cancel"];
        alert.messageText = @"The file already exists. Overwrite?";
        alert.informativeText = path;
        alert.alertStyle = NSAlertStyleCritical;

        [alert beginSheetModalForWindow:_progressPanel completionHandler:^(NSModalResponse returnCode) {
            [self.overwritePromptCondition lock];
            self.overwritePromptResult = returnCode;
            [self.overwritePromptCondition signal];
            [self.overwritePromptCondition unlock];
        }];
    });

    [self.overwritePromptCondition lock];
    [self.overwritePromptCondition wait];
    [self.overwritePromptCondition unlock];
    self.overwritePromptCondition = nil;
    return self.overwritePromptResult;
}

+ (void)runConverterWithTracks:(ddb_playItem_t **)tracks count:(NSInteger)count playlist:(ddb_playlist_t *)plt {
    ConverterWindowController *conv = [[ConverterWindowController alloc] initWithWindowNibName:@"Converter"];

    if (!g_converterControllers) {
        g_converterControllers = [NSMutableArray new];
    }
    [g_converterControllers addObject:conv];
    [conv runWithTracks:tracks count:count playlist:plt];
}

+ (void)cleanup {
    g_converterControllers = nil;
}

#pragma mark - ScriptableSelectDelegate

- (void)scriptableSelectItemSelected:(nonnull scriptableItem_t *)item {
    if (item->parent == scriptableEncoderRoot()) {
        const char *name = scriptableItemPropertyValueForKey(item, "name");
        deadbeef->conf_set_str ("converter.encoder_preset_name", name);
        [self updateFilenamesPreview];
    }
    else if (item->parent == scriptableDspRoot()) {
        const char *name = scriptableItemPropertyValueForKey(item, "name");
        deadbeef->conf_set_str ("converter.dsp_preset_name", name);
    }
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t * _Nonnull)scriptable change:(ScriptableItemChange)change {
    if (scriptable == scriptableEncoderRoot()) {
        NSInteger selectedEncPresetIndex = self.encoderSelectViewController.indexOfSelectedItem;
        scriptableItem_t *selectedEncPreset = scriptableItemChildAtIndex(scriptableEncoderRoot(), (unsigned int)selectedEncPresetIndex);
        [self scriptableSelectItemSelected:selectedEncPreset];
        [self.encoderSelectViewController reloadData];
    }
    else if (scriptable == scriptableDspRoot()) {
        NSInteger selectedDspPresetIndex = self.dspSelectViewController.indexOfSelectedItem;
        scriptableItem_t *selectedDspPreset = scriptableItemChildAtIndex(scriptableDspRoot(), (unsigned int)selectedDspPresetIndex);
        [self scriptableSelectItemSelected:selectedDspPreset];
        [self.dspSelectViewController reloadData];
    }
}

@end
