//
//  DspPreferencesViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/26/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "DspPreferencesViewController.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorViewController.h"
#include "scriptable_dsp.h"
#include "pluginsettings.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface DspPreferencesViewController () <ScriptableNodeEditorCustomButtonsInitializer>

@property (weak) IBOutlet NSView *dspPresetSelectorContainer;
@property (weak) IBOutlet NSView *dspNodeEditorContainer;

@property (nonatomic) ScriptableSelectViewController *dspSelectViewController;
@property (nonatomic) ScriptableNodeEditorViewController *dspNodeEditorViewController;

@property (nonatomic) ScriptableTableDataSource *dspChainDataSource;
@property (nonatomic) ScriptableTableDataSource *dspPresetsDataSource;

@property (nonatomic) scriptableItem_t *currentDspChain;

@property (weak) IBOutlet NSPanel *dspPresetNamePanel;
@property (weak) IBOutlet NSTextField *dspPresetNameTextField;

@end

@implementation DspPreferencesViewController

- (void)dealloc
{
    if (_currentDspChain) {
        scriptableItemFree (_currentDspChain);
        _currentDspChain = NULL;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.

    // dsp
    self.currentDspChain = scriptableDspPresetFromDspChain (deadbeef->streamer_get_dsp_chain ());
    self.dspChainDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:self.currentDspChain pasteboardItemIdentifier:@"deadbeef.dspnode.preferences"];

    self.dspPresetsDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:scriptableDspRoot() pasteboardItemIdentifier:@"deadbeef.dsppreset.preferences"];

    // preset list and browse button
    self.dspSelectViewController = [[ScriptableSelectViewController alloc] initWithNibName:@"ScriptableSelectView" bundle:nil];
    self.dspSelectViewController.dataSource = self.dspPresetsDataSource;
    self.dspSelectViewController.delegate = self;
    self.dspSelectViewController.view.frame = _dspPresetSelectorContainer.bounds;
    [_dspPresetSelectorContainer addSubview:self.dspSelectViewController.view];

    // current dsp chain node list / editor
    self.dspNodeEditorViewController = [[ScriptableNodeEditorViewController alloc] initWithNibName:@"ScriptableNodeEditorView" bundle:nil];
    self.dspNodeEditorViewController.customButtonsInitializer = self;
    self.dspNodeEditorViewController.dataSource = self.dspChainDataSource;
    self.dspNodeEditorViewController.delegate = self;
    self.dspNodeEditorViewController.view.frame = _dspNodeEditorContainer.bounds;
    [_dspNodeEditorContainer addSubview:self.dspNodeEditorViewController.view];

    self.dspSelectViewController.scriptable = scriptableDspRoot();
}

#pragma mark - ScriptableNodeEditorCustomButtonsInitializer

- (void)customButtonsInitializer:(ScriptableNodeEditorViewController *)controller initButtonsInSegmentedControl:(NSSegmentedControl *)segmentedControl {
    segmentedControl.segmentCount = 1;
    [segmentedControl setLabel:@"Save as preset" forSegment:0];
    segmentedControl.target = self;
    segmentedControl.action = @selector(segmentedControlAction:);
}

- (void)segmentedControlAction:(NSSegmentedControl *)sender {
    [self.view.window beginSheet:self.dspPresetNamePanel completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            scriptableItem_t *preset = scriptableItemClone (self.currentDspChain);
            scriptableItem_t *presets = scriptableDspRoot();
            scriptableItemSetUniqueNameUsingPrefixAndRoot(preset, self.dspPresetNameTextField.stringValue.UTF8String, presets);
            scriptableItemAddSubItem(presets, preset);
            [self.dspSelectViewController reloadData];
        }
    }];
}

- (IBAction)presetNameOK:(id)sender {
    [self.view.window endSheet:self.dspPresetNamePanel returnCode:NSModalResponseOK];
}

- (IBAction)presetNameCancel:(id)sender {
    [self.view.window endSheet:self.dspPresetNamePanel returnCode:NSModalResponseCancel];
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable change:(ScriptableItemChange)change {
    if (scriptable == self.dspChainDataSource.scriptable
        || scriptableItemIndexOfChild(self.dspChainDataSource.scriptable, scriptable) >= 0) {
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (self.dspChainDataSource.scriptable);
        deadbeef->streamer_set_dsp_chain (chain);
        deadbeef->dsp_preset_free (chain);
    }
    else if (scriptable == self.dspPresetsDataSource.scriptable) { // list of presets changed
        [self.dspSelectViewController reloadData];
    }
}

@end
