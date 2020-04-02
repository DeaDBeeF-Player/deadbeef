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

@interface DspPreferencesViewController () <ScriptableNodeEditorCustomButtonsInitializer, ScriptableErrorViewer, ScriptableSelectDelegate>

@property (weak) IBOutlet NSView *dspPresetSelectorContainer;
@property (weak) IBOutlet NSView *dspNodeEditorContainer;

@property (nonatomic) ScriptableSelectViewController *dspSelectViewController;
@property (nonatomic) ScriptableNodeEditorViewController *dspNodeEditorViewController;

@property (nonatomic) ScriptableTableDataSource *dspChainDataSource;
@property (nonatomic) ScriptableTableDataSource *dspPresetsDataSource;

//@property (nonatomic) scriptableItem_t *currentDspChain;

@property (weak) IBOutlet NSPanel *dspPresetNamePanel;
@property (weak) IBOutlet NSTextField *dspPresetNameTextField;

@property (nonatomic) scriptableItem_t *currentDspChain; // owned by this object, must be freed

@end

@implementation DspPreferencesViewController

- (void)setCurrentDspChain:(scriptableItem_t *)currentDspChain {
    if (_currentDspChain) {
        scriptableItemFree(_currentDspChain);
    }
    _currentDspChain = currentDspChain;
}

- (void)dealloc {
    self.currentDspChain = nil; // required because of the setter
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.

    // dsp
    _currentDspChain = scriptableDspPresetFromDspChain (deadbeef->streamer_get_dsp_chain ());
    self.dspChainDataSource = [ScriptableTableDataSource dataSourceWithScriptable:_currentDspChain];

    self.dspPresetsDataSource = [ScriptableTableDataSource dataSourceWithScriptable:scriptableDspRoot()];

    // preset list and browse button
    self.dspSelectViewController = [[ScriptableSelectViewController alloc] initWithNibName:@"ScriptableSelectView" bundle:nil];
    self.dspSelectViewController.scriptableItemDelegate = self;
    self.dspSelectViewController.view.frame = _dspPresetSelectorContainer.bounds;
    [_dspPresetSelectorContainer addSubview:self.dspSelectViewController.view];
    self.dspSelectViewController.scriptableSelectDelegate = self;
    self.dspSelectViewController.errorViewer = self;
    self.dspSelectViewController.dataSource = self.dspPresetsDataSource;

    // current dsp chain node list / editor
    self.dspNodeEditorViewController = [[ScriptableNodeEditorViewController alloc] initWithNibName:@"ScriptableNodeEditorView" bundle:nil];
    self.dspNodeEditorViewController.scriptableNodeEditorDelegate = self;
    self.dspNodeEditorViewController.dataSource = self.dspChainDataSource;
    self.dspNodeEditorViewController.delegate = self;
    self.dspNodeEditorViewController.view.frame = _dspNodeEditorContainer.bounds;
    [_dspNodeEditorContainer addSubview:self.dspNodeEditorViewController.view];

}

#pragma mark - ScriptableNodeEditorCustomButtonsInitializer

- (void)scriptableNodeEditorCustomButtonsInitializer:(ScriptableNodeEditorViewController *)controller initButtonsInSegmentedControl:(NSSegmentedControl *)segmentedControl {
    segmentedControl.segmentCount = 1;
    [segmentedControl setLabel:@"Save as preset" forSegment:0];
    segmentedControl.target = self;
    segmentedControl.action = @selector(segmentedControlAction:);
}

- (void)segmentedControlAction:(NSSegmentedControl *)sender {
    [self.view.window beginSheet:self.dspPresetNamePanel completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            const char *name = self.dspPresetNameTextField.stringValue.UTF8String;
            scriptableItem_t *preset = scriptableItemClone (self.dspChainDataSource.scriptable);
            scriptableItem_t *presets = scriptableDspRoot();
            scriptableItemSetUniqueNameUsingPrefixAndRoot(preset, name, presets);
            scriptableItemAddSubItem(presets, preset);
            [self.dspSelectViewController reloadData];
        }
    }];
}

- (void)displayDuplicateNameError {
    NSAlert *alert = [NSAlert new];
    alert.messageText = @"Preset with this name already exists.";
    alert.informativeText = @"Try a different name.";
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle:@"OK"];

    [alert runModal];
}

- (void)displayInvalidNameError {
    NSAlert *alert = [NSAlert new];
    alert.messageText = @"This name is not allowed.";
    alert.informativeText = @"Try a different name.";
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle:@"OK"];

    [alert runModal];
}

- (IBAction)presetNameOK:(id)sender {
    const char *name = self.dspPresetNameTextField.stringValue.UTF8String;
    if (scriptableItemContainsSubItemWithName(scriptableDspRoot(), name)) {
        [self displayDuplicateNameError];
        return;
    }
    if (!scriptableItemIsSubItemNameAllowed(scriptableDspRoot(), name)) {
        [self displayInvalidNameError];
        return;
    }

    [self.view.window endSheet:self.dspPresetNamePanel returnCode:NSModalResponseOK];
}

- (IBAction)presetNameCancel:(id)sender {
    [self.view.window endSheet:self.dspPresetNamePanel returnCode:NSModalResponseCancel];
}

#pragma mark - ScriptableErrorViewer

- (void)scriptableErrorViewer:(id)sender duplicateNameErrorForItem:(scriptableItem_t *)item {
    [self displayDuplicateNameError];
}

- (void)scriptableErrorViewer:(id)sender invalidNameErrorForItem:(scriptableItem_t *)item {
    [self displayInvalidNameError];
}

#pragma mark - ScriptableItemDelegate

- (void)scriptableItemChanged:(scriptableItem_t *)scriptable change:(ScriptableItemChange)change {
    if (scriptable == self.dspChainDataSource.scriptable
        || scriptableItemIndexOfChild(self.dspChainDataSource.scriptable, scriptable) >= 0) {
        // create dsp chain from the new state
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (self.dspChainDataSource.scriptable);
        deadbeef->streamer_set_dsp_chain (chain);
        deadbeef->dsp_preset_free (chain);
    }
    else if (scriptable == self.dspPresetsDataSource.scriptable) { // list of presets changed
        [self.dspSelectViewController reloadData];
    }
}

#pragma mark - ScriptableSelectDelegate

- (void)scriptableSelectItemSelected:(nonnull scriptableItem_t *)item {
    self.dspChainDataSource.scriptable = scriptableItemClone(item);
    [self.dspNodeEditorViewController reloadData];
}

#pragma mark -

@end
